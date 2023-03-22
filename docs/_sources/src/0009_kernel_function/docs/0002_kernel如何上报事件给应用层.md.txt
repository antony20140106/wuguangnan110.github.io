# 概述

本文主要讲解kernel层如何上传事件给到用户空间的几种方法。

## 上报方式

### 1.上报键值方式

* [高通驱动实现 GPIO 中断上报键值](https://blog.csdn.net/Ciellee/article/details/101312350)

#### 1.确认keycode值，同步修改上层键值映射表

从Generic.kl文件中，我们可以看到`CHANNEL_UP CHANNEL_DOWN `是没人使用的，为避免和默认代码有冲突，我们使用 key 402 403 （0x192 0x193）来做为按键 keycoe。

```java
frameworks/base/data/keyboards/Generic.kl:

key 402   CHANNEL_UP
key 403   CHANNEL_DOWN
```

* kernel对应keycode:

```C
kernel-4.19/include/uapi/linux/input-event-codes.h
#define KEY_CHANNELUP           0x192   /* Channel Increment */
#define KEY_CHANNELDOWN         0x193   /* Channel Decrement */
```

#### 2.驱动代码

```C++
/* input device */
static int create_input_device(struct xxxxx_gpio_set *data)
{
	int ret = 0;

	data->input = input_allocate_device();//为新添加的输入设备分配内存
	if (!data->input) {
		xxx_GPIO_ERR("err: not enough memory for input device");
		return -ENOMEM;
	}

	data->input->id.bustype = BUS_HOST;
	data->input->name = "xxxxx_gpios";
	__set_bit(EV_KEY, data->input->evbit);//支持的事件,EV_KEY事件，事件类型由input_dev.evbit表示
	__set_bit(KEY_WAKEUP, data->input->keybit);// 为了快速看到现象, 可以上报 KEY_POWER 事件, 看是否会亮灭屏，后续注释该代码  ++++
	__set_bit(KEY_CHANNELUP, data->input->keybit);
	__set_bit(KEY_CHANNELDOWN, data->input->keybit);

	//data->input->evbit[0] = BIT_MASK(EV_KEY);
	//input_set_capability(data->input, EV_KEY, KEY_WAKEUP);

	ret = input_register_device(data->input);//将input_dev(输入设备结构体)注册到输入子系统核心中
	if (ret) {
		xxx_GPIO_ERR("err: input_register_device failed");
		input_free_device(data->input);
		data->input = NULL;
		return -ENOMEM;
	}

	return 0;
}

static int xxxxx_gpios_probe(struct platform_device *pdev)
{
        /* regitster input dev */
        if (create_input_device(data)) {
                goto input_dev_fail;
        }
}


static int r15_status_notify_call(struct notifier_block *self, unsigned long event, void *value)
{
	int power_en = 0;

	switch (event) {
		case SET_POWER_EN:
			power_en = *(int *)value;
			xxx_GPIO_DBG("SET_POWER_EN: %d", power_en);
			break;
		case SET_POWER_STATUS:
			power_en = *(int *)value;
			xxx_GPIO_DBG("SET_POWER_STATUS: %d", power_en);
			break;
        case SET_POGO_DEV_STATE:
		    power_en = *(int *)value;
			if (power_en) {
				input_report_key(g_xxxxx_gpio_set->input, KEY_CHANNELUP, 1); //上报键值
				input_sync(g_xxxxx_gpio_set->input);
				input_report_key(g_xxxxx_gpio_set->input, KEY_CHANNELUP, 0);
				input_sync(g_xxxxx_gpio_set->input);
			}
			else {
				input_report_key(g_xxxxx_gpio_set->input, KEY_CHANNELDOWN, 1);
				input_sync(g_xxxxx_gpio_set->input);
				input_report_key(g_xxxxx_gpio_set->input, KEY_CHANNELDOWN, 0);
				input_sync(g_xxxxx_gpio_set->input);
			}
            xxx_GPIO_DBG("SET_POGO_DEV_STATE: send pogo dev input event %d",power_en);
           break;

		default:
			break;
	};

	return NOTIFY_DONE;
}
```

#### 3.调试
如果映射不对，可以调用`adb shell dumpsys input`看下调用的是哪个 kl 文件，然后把对应的键值 定义在 kl 文件中，就可以了。

如下：

```log
adb dumpsys input :
   1: xxxxx_gpios
      Classes: 0x00000001
      Path: /dev/input/event4
      Enabled: true
      Descriptor: 3c6af5b5eb72daa3072fe644e93c50ba6c83a9dd
      Location:
      ControllerNumber: 0
      UniqueId:
      Identifier: bus=0x0019, vendor=0x0000, product=0x0000, version=0x0000
      KeyLayoutFile: /system/usr/keylayout/Generic.kl
      KeyCharacterMapFile: /system/usr/keychars/Generic.kcm
      ConfigurationFile:
      HaveKeyboardLayoutOverlay: false
      VideoDevice: <none>
```

#### 4.framework处理流程

* `frameworks/base/services/core/java/com/android/server/policy/PhoneWindowManager.java`:

```java
按键处理流程：
* dispatchUnhandledKey(IBinder focusedToken, KeyEvent event, int policyFlags)
  *interceptFallback(IBinder focusedToken, KeyEvent fallbackEvent,
    * interceptKeyBeforeQueueing
```
* 增加按键接收并发送广播：

```diff
--- a/frameworks/base/services/core/java/com/android/server/policy/PhoneWindowManager.java
+++ b/frameworks/base/services/core/java/com/android/server/policy/PhoneWindowManager.java
@@ -368,6 +368,8 @@ public class PhoneWindowManager implements WindowManagerPolicy {
     /** Amount of time (in milliseconds) a toast window can be shown. */
     public static final int TOAST_WINDOW_TIMEOUT = 3500; // 3.5 seconds

+
+
     /**
      * Lock protecting internal state.  Must not call out into window
      * manager with lock held.  (This lock will be acquired in places
@@ -669,6 +671,11 @@ public class PhoneWindowManager implements WindowManagerPolicy {
     private long startTime = -1l;
     private static Queue<Long> queue = new LinkedBlockingQueue<Long>();

+    //[FEATURE]-Add-BEGIN by (lib@xxxxx.com) 2022/04/11
+    private static final int POGE_PIN_INSERT_TIME = 200;
+    private int insertState = -1;
+    //[FEATURE]-Add-END by (lib@xxxxx.com) 2021/04/11
+

     // private boolean isGoogleSetupEnable(){
     //     int state = 0;
@@ -4144,7 +4151,20 @@ public class PhoneWindowManager implements WindowManagerPolicy {

                 break;
             }

public int interceptKeyBeforeQueueing(KeyEvent event, int policyFlags) {

+            //[FEATURE]-Add-BEGIN by (lib@xxxxx.com) 2022/04/11
+            case KeyEvent.KEYCODE_CHANNEL_DOWN:
+                if (!down){
+                    mHandler.removeCallbacks(pogoOutRunable);
+                    mHandler.postDelayed(pogoOutRunable,POGE_PIN_INSERT_TIME);
+                }
+                break;
+            case KeyEvent.KEYCODE_CHANNEL_UP:
+                if(!down){
+                    mHandler.removeCallbacks(pogoInsertRunable);
+                    mHandler.postDelayed(pogoInsertRunable,POGE_PIN_INSERT_TIME);
+                }
+                break;
+            //[FEATURE]-Add-END by (lib@xxxxx.com) 2022/04/11
             case KeyEvent.KEYCODE_MEDIA_PLAY:
             case KeyEvent.KEYCODE_MEDIA_PAUSE:
             case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
@@ -4301,8 +4321,37 @@ public class PhoneWindowManager implements WindowManagerPolicy {
         intent.setPackage(SystemProperties.get("persist.sys.wake.package","com.xxxxx.paydroid.tester"));
         mContext.sendBroadcastAsUser(intent, UserHandle.ALL);
     }

+    private Runnable pogoInsertRunable = new Runnable() {
+        @Override
+        public void run() {
+            sendR15InsertBroadcast(true);
+        }
+    };
+
+    private Runnable pogoOutRunable = new Runnable() {
+        @Override
+        public void run() {
+            sendR15InsertBroadcast(false);
+        }
+    };
+
+    private void sendR15InsertBroadcast(boolean isInsert){
+
+        if(insertState == 1 && isInsert|| insertState == 0 && !isInsert){
+            return;
+        }
+        insertState = isInsert?1:0;
+        Intent intent = new Intent("com.xxxxxdroid.usbserial.insert.state");
+        intent.putExtra("device","R15");
+        intent.putExtra("state", isInsert?1:0);//1表示R15在线 0表示R15不在线
+        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
+    }
     //[FEATURE]-Add-END by (lib@xxxxx.com) 2021/03/23
```

### 2.uevent事件方式

```C
int authinfo_supply_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	int ret = 0, j;
	char *prop_buf;
	char *attrname;

	prop_buf = (char *)get_zeroed_page(GFP_KERNEL);
	if (!prop_buf)
		return -ENOMEM;

	for (j = 0; j < authinfo.num_properties; j++) {
		struct device_attribute *attr;
		char *line;

		attr = &authinfo_supply_attrs[authinfo.properties[j]];

		ret = authinfo_supply_show_property(dev, attr, prop_buf);
		if (ret == -ENODEV || ret == -ENODATA) {
			/* When a battery is absent, we expect -ENODEV. Don't abort;
			   send the uevent with at least the the PRESENT=0 property */
			ret = 0;
			continue;
		}

		if (ret < 0)
			goto out;

		line = strchr(prop_buf, '\n');
		if (line)
			*line = 0;

		attrname = kstruprdup(attr->attr.name, GFP_KERNEL);
		if (!attrname) {
			ret = -ENOMEM;
			goto out;
		}

		dev_dbg(dev, "prop %s=%s\n", attrname, prop_buf);
		ret = add_uevent_var(env, "AUTHINFO_SUPPLY_%s=%s", attrname, prop_buf);
		kfree(attrname);
		if (ret)
			goto out;
	}

out:
	free_page((unsigned long)prop_buf);
	return ret;
}

static int authinfo_probe(struct platform_device *pdev)
{
	authinfo.authinfo_supply_class = class_create(THIS_MODULE, "authinfo");
	if (IS_ERR(authinfo.authinfo_supply_class))
		return PTR_ERR(authinfo.authinfo_supply_class);

	authinfo.authinfo_supply_class->dev_uevent = authinfo_supply_uevent;
}
```

* `kernel/msm-4.19/drivers/base/core.c`:原理：
```c
static const struct kset_uevent_ops device_uevent_ops = {
        .filter =       dev_uevent_filter,
        .name =         dev_uevent_name,
        .uevent =       dev_uevent,
};

static ssize_t uevent_show(struct device *dev, struct device_attribute *attr,
                           char *buf)
{
        struct kobject *top_kobj;
        struct kset *kset;
        struct kobj_uevent_env *env = NULL;
        int i;
        size_t count = 0;
        int retval;

        /* search the kset, the device belongs to */
        top_kobj = &dev->kobj;
        while (!top_kobj->kset && top_kobj->parent)
                top_kobj = top_kobj->parent;
        if (!top_kobj->kset)
                goto out;

        kset = top_kobj->kset;
        if (!kset->uevent_ops || !kset->uevent_ops->uevent)
                goto out;

        /* respect filter */
        if (kset->uevent_ops && kset->uevent_ops->filter)
                if (!kset->uevent_ops->filter(kset, &dev->kobj))
                        goto out;

        env = kzalloc(sizeof(struct kobj_uevent_env), GFP_KERNEL);
        if (!env)
                return -ENOMEM;

        /* let the kset specific function add its keys */
        retval = kset->uevent_ops->uevent(kset, &dev->kobj, env);
        if (retval)
                goto out;

        /* copy keys to file */
        for (i = 0; i < env->envp_idx; i++)
                count += sprintf(&buf[count], "%s\n", env->envp[i]);
out:
        kfree(env);
        return count;
}

static ssize_t uevent_store(struct device *dev, struct device_attribute *attr,
                            const char *buf, size_t count)
{
        int rc;

        rc = kobject_synth_uevent(&dev->kobj, buf, count);

        if (rc) {
                dev_err(dev, "uevent: failed to send synthetic uevent\n");
                return rc;
        }

        return count;
}
static DEVICE_ATTR_RW(uevent);

/**
 * kobject_synth_uevent - send synthetic uevent with arguments
 *
 * @kobj: struct kobject for which synthetic uevent is to be generated
 * @buf: buffer containing action type and action args, newline is ignored
 * @count: length of buffer
 *
 * Returns 0 if kobject_synthetic_uevent() is completed with success or the
 * corresponding error when it fails.
 */
int kobject_synth_uevent(struct kobject *kobj, const char *buf, size_t count)
{
        char *no_uuid_envp[] = { "SYNTH_UUID=0", NULL };
        enum kobject_action action;
        const char *action_args;
        struct kobj_uevent_env *env;
        const char *msg = NULL, *devpath;
        int r;

        r = kobject_action_type(buf, count, &action, &action_args);
        if (r) {
                msg = "unknown uevent action string\n";
                goto out;
        }

        if (!action_args) {
                r = kobject_uevent_env(kobj, action, no_uuid_envp);
                goto out;
        }

        r = kobject_action_args(action_args,
                                count - (action_args - buf), &env);
        if (r == -EINVAL) {
                msg = "incorrect uevent action arguments\n";
                goto out;
        }

        if (r)
                goto out;

        r = kobject_uevent_env(kobj, action, env->envp);
        kfree(env);
out:
        if (r) {
                devpath = kobject_get_path(kobj, GFP_KERNEL);
                printk(KERN_WARNING "synth uevent: %s: %s",
                       devpath ?: "unknown device",
                       msg ?: "failed to send uevent");
                kfree(devpath);
        }
        return r;
}

```

* 测试：
```log
A665x:/sys/class/authinfo/sp_authinfo # cat uevent
AUTHINFO_SUPPLY_SPPOWERFLAG=1
AUTHINFO_SUPPLY_SECMODE=3
AUTHINFO_SUPPLY_BOOTLEVEL=1
AUTHINFO_SUPPLY_TAMPERCLEAR=1
AUTHINFO_SUPPLY_LASTBBLSTATUS=0
AUTHINFO_SUPPLY_SNDOWNLOADSUM=0
AUTHINFO_SUPPLY_AUTHDOWNSN=0
AUTHINFO_SUPPLY_FIRMDEBUGSTATUS=0
AUTHINFO_SUPPLY_APPDEBUGSTATUS=1
AUTHINFO_SUPPLY_USPUKLEVEL=3
AUTHINFO_SUPPLY_CUSTOMER=255
AUTHINFO_SUPPLY_UPDATESLEEPTIME=30
AUTHINFO_SUPPLY_NOWPUKLEVEL=0
AUTHINFO_SUPPLY_PUKSTATUS=0
AUTHINFO_SUPPLY_GPIOSLEEPSP=0
AUTHINFO_SUPPLY_GPIOWAKEAP=0
```