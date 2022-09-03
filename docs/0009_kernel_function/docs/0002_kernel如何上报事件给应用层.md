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
static int create_input_device(struct pax_gpio_set *data)
{
	int ret = 0;

	data->input = input_allocate_device();//为新添加的输入设备分配内存
	if (!data->input) {
		PAX_GPIO_ERR("err: not enough memory for input device");
		return -ENOMEM;
	}

	data->input->id.bustype = BUS_HOST;
	data->input->name = "pax_gpios";
	__set_bit(EV_KEY, data->input->evbit);//支持的事件,EV_KEY事件，事件类型由input_dev.evbit表示
	__set_bit(KEY_WAKEUP, data->input->keybit);// 为了快速看到现象, 可以上报 KEY_POWER 事件, 看是否会亮灭屏，后续注释该代码  ++++
	__set_bit(KEY_CHANNELUP, data->input->keybit);
	__set_bit(KEY_CHANNELDOWN, data->input->keybit);

	//data->input->evbit[0] = BIT_MASK(EV_KEY);
	//input_set_capability(data->input, EV_KEY, KEY_WAKEUP);

	ret = input_register_device(data->input);//将input_dev(输入设备结构体)注册到输入子系统核心中
	if (ret) {
		PAX_GPIO_ERR("err: input_register_device failed");
		input_free_device(data->input);
		data->input = NULL;
		return -ENOMEM;
	}

	return 0;
}

static int pax_gpios_probe(struct platform_device *pdev)
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
			PAX_GPIO_DBG("SET_POWER_EN: %d", power_en);
			break;
		case SET_POWER_STATUS:
			power_en = *(int *)value;
			PAX_GPIO_DBG("SET_POWER_STATUS: %d", power_en);
			break;
        case SET_POGO_DEV_STATE:
		    power_en = *(int *)value;
			if (power_en) {
				input_report_key(g_pax_gpio_set->input, KEY_CHANNELUP, 1); //上报键值
				input_sync(g_pax_gpio_set->input);
				input_report_key(g_pax_gpio_set->input, KEY_CHANNELUP, 0);
				input_sync(g_pax_gpio_set->input);
			}
			else {
				input_report_key(g_pax_gpio_set->input, KEY_CHANNELDOWN, 1);
				input_sync(g_pax_gpio_set->input);
				input_report_key(g_pax_gpio_set->input, KEY_CHANNELDOWN, 0);
				input_sync(g_pax_gpio_set->input);
			}
            PAX_GPIO_DBG("SET_POGO_DEV_STATE: send pogo dev input event %d",power_en);
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
   1: pax_gpios
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

+    //[FEATURE]-Add-BEGIN by (lib@paxsz.com) 2022/04/11
+    private static final int POGE_PIN_INSERT_TIME = 200;
+    private int insertState = -1;
+    //[FEATURE]-Add-END by (lib@paxsz.com) 2021/04/11
+

     // private boolean isGoogleSetupEnable(){
     //     int state = 0;
@@ -4144,7 +4151,20 @@ public class PhoneWindowManager implements WindowManagerPolicy {

                 break;
             }

public int interceptKeyBeforeQueueing(KeyEvent event, int policyFlags) {

+            //[FEATURE]-Add-BEGIN by (lib@paxsz.com) 2022/04/11
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
+            //[FEATURE]-Add-END by (lib@paxsz.com) 2022/04/11
             case KeyEvent.KEYCODE_MEDIA_PLAY:
             case KeyEvent.KEYCODE_MEDIA_PAUSE:
             case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
@@ -4301,8 +4321,37 @@ public class PhoneWindowManager implements WindowManagerPolicy {
         intent.setPackage(SystemProperties.get("persist.sys.wake.package","com.pax.paydroid.tester"));
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
+        Intent intent = new Intent("com.paxdroid.usbserial.insert.state");
+        intent.putExtra("device","R15");
+        intent.putExtra("state", isInsert?1:0);//1表示R15在线 0表示R15不在线
+        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
+    }
     //[FEATURE]-Add-END by (lib@paxsz.com) 2021/03/23
```

### 2.uevent事件方式