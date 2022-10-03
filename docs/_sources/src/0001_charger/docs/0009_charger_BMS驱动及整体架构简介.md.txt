# 概述

BMS功能介绍

# 参考

* [0009_patch](refers/0009_patch)
* [bms_driver](H:/wugn/md/pax_kearning_doc/docs/0001_charger/docs/refers/BMS_driver/power)
* [epoll使用详解：epoll_create、epoll_ctl、epoll_wait、close](https://www.cnblogs.com/xuewangkai/p/11158576.html)
* [Android JNI 使用方法总结及原理分析](https://www.jianshu.com/p/9236ad3b9d40)
* [Android Service详解](https://www.jianshu.com/p/6bf03e3cb097)
* [HIDL服务编写实现](https://www.jianshu.com/p/f972f1a8f7dc)
* [Android C++语言 通过Binder通信调用activity: [android.app.IActivityManager] 服务发广播](https://blog.csdn.net/qq_37858386/article/details/121501400)

# 架构图

![0009_0000.png](images/0009_0000.png)

![0009_0014.png](images/0009_0014.png)

* 上面架构图有两个改进点：
  * `PaxBmsService.java`和`PaxBatteryManagerService.java`都是系统服务，都是在`system_server`里面，所以不需要binder通信，最好的做法是在`PaxBatteryManagerService.java`直接调用JNI方法。
  * HIDL中还是采用传统HAL方式，即module/device模型，这种需要将HAL编译成so库，其实是可以在HIDL中直接ioctrl驱动的，更加方便。

上图是BMS新的架构图，注意以下几点：
* 1. `mSystemServiceManager.startService`方法主要就是调用service的onstart方法。
* 2. `ServiceManager.addService("paxbms", mPaxBmsService)`方法是将服务加入到binder，作为binder sercice。
* 3. `PaxBmsService`是`binder server`端，`PaxBmsManager`是服务于`PaxBmsService`的`binder client`端，并给外部提供接口的。
* 4. `PaxBmsManager`调用方式一般都是在`SystemServiceRegistry.java`中先注册一个静态系统服务(不是四大件中的服务)，应用通过`getSystemService(Context.PAXBMS_SERVICE)`调用，两个函数就是put和get操作。

* `base/services/java/com/android/server/SystemServer.java`第一点说明:
```java
private SystemServiceManager mSystemServiceManager;
                        t.traceBegin("PaxBatteryManagerService ");
                        mSystemServiceManager.startService(PaxBatteryManagerService.class);
                        t.traceEnd();

将调用以下方法：
base/services/core/java/com/android/server/SystemServiceManager.java：
   public void startService(@NonNull final SystemService service) {
        // Register it.
        mServices.add(service);
        // Start it.
        long time = SystemClock.elapsedRealtime();
        try {
            service.onStart();
        } catch (RuntimeException ex) {
            throw new RuntimeException("Failed to start service " + service.getClass().getName()
                    + ": onStart threw an exception", ex);
        }
        warnIfTooLong(SystemClock.elapsedRealtime() - time, service, "onStart");
    }
主要功能是注册systemserver和执行onstart方法。
```

* `base/core/java/android/os/ServiceManager.java`第二点，`PaxBmsService`通过addService注册为binder服务端:

```java
    @UnsupportedAppUsage
    public static void addService(String name, IBinder service) {
        addService(name, service, false, IServiceManager.DUMP_FLAG_PRIORITY_DEFAULT);
    }

    /**
     * Place a new @a service called @a name into the service
     * manager.
     *
     * @param name the name of the new service
     * @param service the service object
     * @param allowIsolated set to true to allow isolated sandboxed processes
     * to access this service
     */
    @UnsupportedAppUsage
    public static void addService(String name, IBinder service, boolean allowIsolated) {
        addService(name, service, allowIsolated, IServiceManager.DUMP_FLAG_PRIORITY_DEFAULT);
    }

    /**
     * Place a new @a service called @a name into the service
     * manager.
     *
     * @param name the name of the new service
     * @param service the service object
     * @param allowIsolated set to true to allow isolated sandboxed processes
     * @param dumpPriority supported dump priority levels as a bitmask
     * to access this service
     */
    @UnsupportedAppUsage
    public static void addService(String name, IBinder service, boolean allowIsolated,
            int dumpPriority) {
        try {
            getIServiceManager().addService(name, service, allowIsolated, dumpPriority); //加入到binder sevice
        } catch (RemoteException e) {
            Log.e(TAG, "error in addService", e);
        }
    }
```

* `base/core/java/android/os/ServiceManager.java`第三点，`PaxBmsManager`通过getServiceOrThrow获取`PaxBmsService` binder服务:

```java
    /**
     * Returns a reference to a service with the given name, or throws
     * {@link ServiceNotFoundException} if none is found.
     *
     * @hide
     */
    public static IBinder getServiceOrThrow(String name) throws ServiceNotFoundException {
        final IBinder binder = getService(name);
        if (binder != null) {
            return binder;
        } else {
            throw new ServiceNotFoundException(name);
        }
    }

```

* `base/core/java/android/app/SystemServiceRegistry.java`第四点说明，可以看到以下通过put和get调用:

```java
                registerService(Context.PAXBMS_SERVICE, PaxBmsManager.class,
                new CachedServiceFetcher<PaxBmsManager>() {
                    @Override
                    public PaxBmsManager createService(ContextImpl ctx)
                            throws ServiceNotFoundException {
                        /*IBinder b = ServiceManager.getService("paxbms");
                        IPaxBms service = IPaxBms.Stub.asInterface(b);*/
                        return PaxBmsManager.getInstance();
                    }});

    /**
     * Statically registers a system service with the context.
     * This method must be called during static initialization only.
     */
    private static <T> void registerService(@NonNull String serviceName,
            @NonNull Class<T> serviceClass, @NonNull ServiceFetcher<T> serviceFetcher) {
        SYSTEM_SERVICE_NAMES.put(serviceClass, serviceName);
        SYSTEM_SERVICE_FETCHERS.put(serviceName, serviceFetcher);
        SYSTEM_SERVICE_CLASS_NAMES.put(serviceName, serviceClass.getSimpleName());
    }

    public static Object getSystemService(ContextImpl ctx, String name) {
        if (name == null) {
            return null;
        }
        final ServiceFetcher<?> fetcher = SYSTEM_SERVICE_FETCHERS.get(name);
        ...省略...
    }
```

后续应用层不直接访问driver或设备节点，主要通过Framework层PaxBatteryService和BatteryService来设置/获取相关BMS信息

这样设计主要是2点考虑：

* 1.不同平台的通用性和易移植性
* 2.安全

# 一、电池及充放电信息，通过接收Battery广播来获取

在android原生的基础上增加了下面几个广播信息

```java
public static final String EXTRA_CHARGER_VOLTAGE = "android.os.extra.CHARGER_VOLTAGE";     //充电器电压
public static final String EXTRA_CHARGER_CURRENT = "android.os.extra.CHARGER_CURRENT";     //充电器电流
public static final String EXTRA_SOH = "android.os.extra.SOH";                                                      //电池健康信息
public static final String EXTRA_MANUFACTURER = "android.os.extra.MANUFACTURER";                //电池厂商信息
public static final String EXTRA_SERIAL_NUMBER = "android.os.extra.SERIAL_NUMBER";                 //电池序列号
```
Healthd是对接power supply 驱动的hal层实现，主要是监听power supply 驱动通过uevent上报的事件，并将power supply信息分发给framework层BatteryService
下面的表格为pax要提供power supply信息。


类型 |	节点	| 值类型	| 单位
---|---|---|---
电池是否在位|	present|	int	| 
类型	   | type	| int	| 
电压	   | voltage_now	| int	| uV
开路电压	| voltage_ocv| 	int| 	uV
电流	   |  current_now| 	int| 	uA
电量	   | capacity	| int	
设计容量	 | charge_full_design	| int	
温度	   |  temp	| int| 	10*C | 
当前状态	 | Status	| int	| 
健康状态	 | health| 	str	| 
电池工艺	 | technology	| str	| 
健康信息	 | soh| 	int	| 
充放电周期	|  cycle_count| 	int	| 
厂商信息	 | manufacturer	| str	| 
序列号	  |     serial_number| 	str	| 
充电器是否在|位	online| 	int	| 
类型	   |     type| 	int	| 
电压	   |      voltage_now| 	int	| uV
电流	   |    current_now| 	int| 	uA
最大输入电压|	voltage_max| 	int| 	uV
最大输入电流|	current_max| 	str| 	uA



# 二、充放电控制BMS服务

PaxBmsService.java和PaxBatteryManagerService.java都是系统服务，都是在`system_server`里面，区别就是PaxBatteryManagerService.java跑了onstart方法。
关联文件：
```
frameworks/base/services/core/java/com/android/server/paxbms/OWNERS
frameworks/base/core/java/android/app/PaxBmsManager.java //代理PaxBmsService方法，供PaxBatteryManagerService调用
frameworks/base/services/core/java/com/android/server/paxbms/PaxBmsService.java //system bms service 主要实现打开/关闭 充电及power path功能
frameworks/base/services/core/java/com/pax/server/BatteryTermInfo.java
frameworks/base/services/core/java/com/pax/server/DatabaseHelper.java
frameworks/base/services/core/java/com/pax/server/PaxBatteryManagerService.java //system BatteryManager service 
frameworks/base/services/core/java/com/pax/util/OsPaxApiInternal.java
frameworks/base/services/core/jni/com_android_server_paxbms_PaxBmsService.cpp //jni封装
```

接口定义：
```java
frameworks/base/services/core/java/com/android/server/paxbms/PaxBmsService.java:
    static native void enableCharge_native();
    static native void disableCharge_native();
    static native void enablePowerPath_native();
    static native void disablePowerPath_native();
```

PaxBatteryService提供相关接口给测试程序， 

## 工作模式介绍

* 从电池的续航能力及电池安全两方面定义了2个电池工作模式：
  * 移动模式（Mobile Mode）：
    * 用户终端长期由电池供电为主
  * 桌面模式（Counter-Top Mode）：
    * 用户终端长期由外部供电为主

工作模式	|满充电量|	回充电量
---|---|---
移动模式|	100	|85
桌面模式|	80 |	65

在表格中，满充电量表示电池充电到某个电量后停止充电，由于存在适配器供电不足以满足系统供电场景和有些模块由电池直接供电，因此电量会缓慢下降，当电量下降至回充电量时，重新使能充电。

## 电池模式切换

* 主动切换模式
用户可以根据自己的喜好主动切换BMS工作模式

* 通过属性设置
可通过写”persist.sys.battery.type”属性设定电池工作模式，立即生效。
```
persist.sys.battery.type = 0；自动模式
persist.sys.battery.type = 1；移动模式
persist.sys.battery.type = 2；桌面模式
```

如果客户能够明确设备的应用场景，即可在设备出厂前要求设置为适合设备应用场景的模式。
例如：“POS终端+售卖机”的场景，由于这种场景下设备会始终连接电源，可设置其工作模式为桌面模式；在移动办公场景的设备，则可以设置为移动模式。

## 使用方法

* 在应用中进行设置，第一步：打开设置应用

![0009_0004.png](images/0009_0004.png)

* 第二步：点击 Battery -> Battery Manager

![0009_0005.png](images/0009_0005.png)
![0009_0006.png](images/0009_0006.png)

* 第三步：点击Select Charge Mode -> 选择模式

![0009_0007.png](images/0009_0007.png)
![0009_0008.png](images/0009_0008.png)

## 自动模式

自动工作模式是指根据用户使用设备的习惯预测电池充电模式。比如：通过前7天同一时间后的6个小时的使用习惯预测充电模式。

1.	设备开机后默认是Mobile Mode;
2.	电池管理服务PaxBatteryManagerService负责记录一个星期内的充电状态，每隔30s记录一次，设备只保留7天的数据，PaxBatteryManagerService每24小时删除一次过期的数据。关机的时间段，数据库中会填充4。
3.	PaxBatteryManagerService每30分钟查询一次当前时间+1~+（1+n）小时时间段的历史数据， n = 6。
当样本数据至少有PREDICTED_DAYS_MIN天的数据量时（取3天,设置了0.05的误差，即当数据量多于3*0.95天的数据量），查询充电状态的数据量，如果充电状态数据量多余非充电状态的数据量时，则切换当前模式为桌面模式（Counter-topMode），否则是移动模式（Mobile Mode）；有效数据量的计算方法：预测时间为7*6h-数据库中4的个数*30s。
4.	切换模式后，设置满充电量和回充电量。

## 工作流程图

![0009_0009.png](images/0009_0009.png)

系统记录电池状态的写数据库的相关操作如下：

![0009_0010.png](images/0009_0010.png)
BMS记录设备连接电源线及关机的状态和时间，保存在/data/system/batterylog.db


# 三、异常信息广播APP batterywarning

广播通知节点改为`/sys/class/pax/bms/bms_notify`：

```diff
--- a/device/mediatek/sepolicy/bsp/plat_private/genfs_contexts
+++ b/device/mediatek/sepolicy/bsp/plat_private/genfs_contexts
@@ -32,6 +32,7 @@ genfscon sysfs /devices/platform/11270000.usb3/musb-hdrc/udc/musb-hdrc u:object_
 #path="/sys/devices/platform/(charger|mt-battery)/BatteryNotify"
 genfscon sysfs /devices/platform/charger/BatteryNotify u:object_r:sysfs_battery_warning:s0
 genfscon sysfs /devices/platform/mt-battery/BatteryNotify u:object_r:sysfs_battery_warning:s0
+genfscon sysfs /devices/platform/pax_bms/pax/bms/bms_notify u:object_r:sysfs_battery_warning:s0

 # Purpose : Camera need read cl_cam_status
 # Package: com.mediatek.camera
diff --git a/vendor/mediatek/proprietary/frameworks/opt/batterywarning/batterywarning.cpp b/vendor/mediatek/proprietary/frameworks/opt/batterywarning/batterywarning.cpp
index d4bae84dee9..434e2b197c2 100644
--- a/vendor/mediatek/proprietary/frameworks/opt/batterywarning/batterywarning.cpp
+++ b/vendor/mediatek/proprietary/frameworks/opt/batterywarning/batterywarning.cpp
@@ -152,8 +152,9 @@ bool sendBroadcastMessage(String8 action, int value)


 static const char *charger_file_path[] = {
-    "/sys/devices/platform/charger/BatteryNotify",
-    "/sys/devices/platform/mt-battery/BatteryNotify",
+    //"/sys/devices/platform/charger/BatteryNotify",
+    //"/sys/devices/platform/mt-battery/BatteryNotify",
+       "/sys/class/pax/bms/bms_notify",
 };

 static int read_from_file(const char* path) {
@@ -238,7 +239,7 @@ static void uevent_event(uint32_t /*epevents*/) {

     while (*cp) {

-        if (!strncmp(cp, "CHGSTAT=", strlen("CHGSTAT="))) { // This CHGSTAT value will be provided by kernel driver
+        if (!strncmp(cp, "BMS_STAT=", strlen("BMS_STAT="))) { // This CHGSTAT value will be provided by kernel driver
             readType(buffer);
             break;
         }
```

## 1.异常信息定义

1.	坏电池异常: 系统开机时会先检测电池是否损坏，当连续5次读取电池电压低于阈值（单电芯为2V， 双电芯为4V），系统显示坏电池logo，提示用户电池损坏，需更换电池才能开机
2.	充电器过压异常:  5s检测一次，当连续12次检测到充电器电压高于过压阈值（最大输入电压的1.3倍），则上报异常信息，并停止充电，当读取充电器电压连续12次恢复至正常范围内，则恢复充电
3.	充电超时异常:  当单次充电总时长超过最大允许时长（一般为12小时），则上报异常信息，并停止充电，一旦出现该异常，后续不允许充电，重启才能恢复充电
4.	电池过压异常： 5s检测一次，当连续12次检测到电池电压高于过压阈值（额定电压的1.03倍），则上报异常信息，并停止充电，一旦出现该异常，后续不允许充电，重启才能恢复充电
5.	电池低压异常： 5s检测一次，当连续12次检测到电池电压低于低压阈值（单电芯为3.4V, 双电芯为6.6V），则上报异常信息，应该层收到该异常，启动关机流程
6.	电池高温异常：5s检测一次，当连续12次检测到电池温度高于最大充电温度（45度），则上报异常信息，并停止充电， 当连续12次检测到电池温度恢复至正常范围，则恢复充电
7.	电池低温异常：5s检测一次，当连续12次检测到电池温度低于最低充电温度（0度），则上报异常信息，并停止充电， 当连续12次检测到电池温度恢复至正常范围，则恢复充电
8.	充电IC通讯异常：当联系60次出现IC通讯异常，则上报异常信息，并停止充电，一旦出现该异常，后续不允许充电，重启才能恢复充电
9.	电量计IC通讯异常：处理机制同充电IC通讯异常
10. thermal温度监控并设置电流限制。

## 2.BMS轮询上报

* `bms.c`以下为工作队列轮询状态:
```C++
 bms_probe(struct platform_device *pdev)
 {
 	INIT_DELAYED_WORK(&g_bms_data.dwork, bms_dwork);
	schedule_delayed_work(&g_bms_data.dwork, 0);   
 }

 static void bms_dwork(struct work_struct *work)
{
	bms_info_update();
	bms_chg_vol_check();
	bms_bat_cur_check();
	bms_bat_vol_check();
	bms_bat_temp_check();
	bms_chg_time_check();
	bms_chg_ic_check();
	bms_gauge_ic_check();
	bms_thermal_check(&g_bms_data);
	bms_dump();
	wait_event_timeout(g_bms_data.bms_wait_q,
			(g_bms_data.bms_wait_timeout == true), msecs_to_jiffies(BMS_POLL_INTERVAL)); //阻塞5s，继续执行
	g_bms_data.bms_wait_timeout = false;
	schedule_delayed_work(&g_bms_data.dwork, 0);
}
```

* 异常状态上报是通过uevent时间上报，上层通过epoll检测到并查看`/sys/class/pax/bms/bms_notify`节点设置值，判断是哪种异常并发送广播上报给App处理：

```C++
bms_notify格式，数字表示第几个bit：
/*start: notify code, should compatible with A800 */
#define NC_CHG_OV				(0)
#define NC_BAT_OT				(1)
#define NC_BAT_OC				(2)
#define NC_BAT_OV				(3)
#define NC_BAT_UT				(5)
#define NC_CHG_IBUS_OCP         (7)
#define NC_CHG_I2C_ERR          (8)

#define NC_CHG_TMO				(13)
#define NC_BAT_I2C_ERR          (14)
#define NC_BAT_UV				(18)

#define NC_DISABLE_CHG_BY_USER  (63)
#define NC_MAX					(64)
#define DISABLE_ALL             0xFFFFFFFFFFFFFFFF
/*end: notify code */

比如电压异常：
bms_chg_vol_check()
{
    bms_set_notify_code(NC_CHG_OV, true, true);
}

static int bms_set_notify_code(int type, bool set, bool update_vote)
{
	if (type >= NC_MAX)
		return 0;

	pr_info("notify_code: 0x%x\n", g_bms_data.notify_code);

	if (set) {
		g_bms_data.notify_code |= (1 << type);
	}
	else {
		g_bms_data.notify_code &= ~(1 << type);
	}

	pr_info("type: %s, set: %d, notify_code: 0x%x\n", notify_type_name[type], set, g_bms_data.notify_code);

	if (update_vote) {
		bms_chg_vote(type, set);
	}

	bms_status_notify();

	return 0;
}

static void bms_status_notify()
{
	char *env[2] = { "BMS_STAT=1", NULL };

	kobject_uevent_env(&g_bms_data.dev->kobj, KOBJ_CHANGE, env);
}
```

主要来看一下thermal如何检测，原理是用电池、CPU、PMIC三个温度根据60%、30%、10%的权重组成一个新的温度值，该值可理解为手机表面温度，根据此温度区别，设置一个电流区间，实时进行限流。

 温度(℃) | 0~15 | 15~35 | 35~38 |38~42|42~45|45~50
------|-------|-------|-------|-------|-------|-------|------
电流限制(ma)| 0|1300|4000|3000|2000|1000|0   

```C++
dts:
    thermal_names = "mtktsbattery", "mtktsAP", "mtktspmic";
    thermal_weights = <60 30 10>;
    chg_skin_temps = <0 15000 35000 38000 42000 45000 50000>;
    chg_currents = <0 1300000 4000000 3000000 2000000 1000000 0>;

static int get_skin_temp(struct bms_data *data)
{
	int i = 0;
	int skin_temp = 0;
	int temp= 0;
	int ret = 0;
	int total_weight = 0;

	for (i = 0; i < data->tinfo_cnt; i++) {
		if (IS_ERR_OR_NULL(data->tinfo[i].name) || (data->tinfo[i].weight <= 0))
			continue;

		data->tinfo[i].tz = thermal_zone_get_zone_by_name(data->tinfo[i].name);
		if (IS_ERR_OR_NULL(data->tinfo[i].tz))
			continue;

		ret = thermal_zone_get_temp(data->tinfo[i].tz, &temp);
		if (ret) {
			continue;
		}

		pr_debug("%s: %p: %s: %d: %d\n", __func__, data->tinfo[i].tz, data->tinfo[i].name, data->tinfo[i].weight, temp);
		if (temp == INVALID_TEMP)
			continue;

		skin_temp += temp * data->tinfo[i].weight;
		total_weight += data->tinfo[i].weight;
	}

	if (total_weight > 0)
		skin_temp = skin_temp / total_weight;
	else
		skin_temp = 25000;

#ifdef BMS_DEBUG
	skin_temp = skin_temp * data->skin_temp_ratio / 100;
#endif

	data->skin_temp = skin_temp / 100;
	pr_debug("%s: skin_temp: %d\n", __func__, skin_temp);

	return skin_temp;
}

int bms_thermal_check(struct bms_data *data)
{
	int skin_temp = 25000;
	int cur_index = 0;
	static int last_index = -1;    //should reset to -1, when charge start
	static int stable_cnt = 0;
	int update_flag = 0;
	int delta_temp = 1000;

	if (!data->bms_thermal_support)
		return 0;

	if (data->bat_info.status != POWER_SUPPLY_STATUS_CHARGING) {
		last_index = -1;
		stable_cnt = 0;
		return 0;
	}

	skin_temp = get_skin_temp(data);
	cur_index = get_temp_range_index(data, skin_temp);
	pr_debug("%s: origin last_index: %d, cur_index: %d\n", __func__, last_index, cur_index);

	if (last_index == -1) {
		last_index = cur_index;
		update_flag = 1;
	}
	else if ((last_index != cur_index)) {
		if (last_index < cur_index)
			delta_temp = 0 - delta_temp;

		cur_index = get_temp_range_index(data, skin_temp + delta_temp);
		pr_debug("%s: actrual last_index: %d, cur_index: %d\n", __func__, last_index, cur_index);
		if (last_index == cur_index)
			stable_cnt = 0;

		stable_cnt++;
		if (stable_cnt >= TEMP_STABLE_CNT) {
			stable_cnt = 0;
			if (last_index < cur_index) {
				last_index ++;
				update_flag = 1;
			}
			else if (last_index > cur_index) {
				last_index --;
				update_flag = 1;
			}
		}
	}
	else {
		stable_cnt = 0;
	}

	if (update_flag) {
		if (data->usb_psy) {
			union power_supply_propval val = {0};
			val.intval = data->tchg_info[last_index].max_chg_current;
			power_supply_set_property(data->usb_psy, POWER_SUPPLY_PROP_CURRENT_MAX, &val);
		}
	}

	return 0;
}
```

## 3.batterywarning C应用epoll检测并发送广播上报

* `vendor/mediatek/proprietary/frameworks/opt/batterywarning/batterywarning.cpp`:
```C++
#define LOG_TAG "batterywarning"

#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <utils/String16.h>
#include <binder/BinderService.h>
#include <binder/Parcel.h>
#include <cutils/uevent.h>
#include <sys/epoll.h>
#include <errno.h>

#define MAX_CHAR 100
//M: Currently disabling battery path.. in later stage we will update in code to check path by order. @{
//#ifdef MTK_GM_30
//#define FILE_NAME "/sys/devices/platform/charger/BatteryNotify"
//#else
//#define FILE_NAME "/sys/devices/platform/mt-battery/BatteryNotify"
//#endif
//M: @}
#define ACTION "pax.intent.action.BATTERY_ABNORMAL"
#define NORMAL_ACTION "pax.intent.action.BATTERY_NORMAL"
#define TYPE "type"
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))
using namespace android;

#define MAX_EPOLL_EVENTS 40
static int uevent_fd;
static int epollfd;
static int eventct;

bool sendBroadcastMessage(String8 action, int value)
{
    ALOGD("sendBroadcastMessage(): Action: %s, Value: %d ", (char *)(action.string()), value);
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> am = sm->getService(String16("activity"));
    if (am != NULL) {
        Parcel data, reply;
        data.writeInterfaceToken(String16("android.app.IActivityManager"));
        data.writeStrongBinder(NULL);
        // Add for match AMS change on O
        data.writeInt32(1);
        // intent begin
        data.writeString8(action); // action
        data.writeInt32(0); // URI data type
        data.writeString8(NULL, 0); // type
        data.writeString8(NULL, 0); // mIdentifier
        data.writeInt32(0x04000000); // flags: FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
        data.writeString8(NULL, 0); // package name
        data.writeString8(NULL, 0); // component name
        data.writeInt32(0); // source bound - size
        data.writeInt32(0); // categories - size
        data.writeInt32(0); // selector - size
        data.writeInt32(0); // clipData - size
        data.writeInt32(-2); // contentUserHint: -2 -> UserHandle.USER_CURRENT
        data.writeInt32(-1); // bundle extras length
        data.writeInt32(0x4C444E42); // 'B' 'N' 'D' 'L'
        int oldPos = data.dataPosition();
        data.writeInt32(1);  // size
        // data.writeInt32(0); // VAL_STRING, need to remove because of analyze common intent
        data.writeString16(String16(TYPE));
        data.writeInt32(1); // VAL_INTEGER
        data.writeInt32(value);
        int newPos = data.dataPosition();
        data.setDataPosition(oldPos - 8);
        data.writeInt32(newPos - oldPos); // refill bundle extras length
        data.setDataPosition(newPos);
        // intent end
        data.writeString8(NULL, 0); // resolvedType
        data.writeStrongBinder(NULL); // resultTo
        data.writeInt32(0); // resultCode
        data.writeString8(NULL, 0); // resultData
        data.writeInt32(-1); // resultExtras
        data.writeString8(NULL, 0); // permission
        data.writeInt32(0); // appOp
        data.writeInt32(-1); // option
        data.writeInt32(1); // serialized: != 0 -> ordered
        data.writeInt32(0); // sticky
        data.writeInt32(-2); // userId: -2 -> UserHandle.USER_CURRENT

        status_t ret = am->transact(IBinder::FIRST_CALL_TRANSACTION + 13, data, &reply); // BROADCAST_INTENT_TRANSACTION
        if (ret == NO_ERROR) {
            int exceptionCode = reply.readExceptionCode();
            if (exceptionCode) {
                ALOGE("sendBroadcastMessage(%s) caught exception %d\n",
                        (char *)(action.string()), exceptionCode);
                return false;
            }
        } else {
            return false;
        }
    } else {
        ALOGE("getService() couldn't find activity service!\n");
        return false;
    }
    return true;
}


static const char *charger_file_path[] = {
    //"/sys/devices/platform/charger/BatteryNotify",
    //"/sys/devices/platform/mt-battery/BatteryNotify",
        "/sys/class/pax/bms/bms_notify",
};

static int read_from_file(const char* path) {
  if(!path) {
     return 0;

  }
  int fd =open(path,O_RDONLY);

  if(fd<0) {
      close(fd);
      return 0;

  }
  else {
      close(fd);
      return 1;
  }
}

int get_charger_file_path() {
  int i = 0;
  for(i=0;i<ARRAY_SIZE(charger_file_path);i++) {
      if(read_from_file(charger_file_path[i])) {
         return i;
      }
  }
  return 0;
}

void readType(char* buffer) {
    FILE * pFile;
    int file_index;
    file_index=get_charger_file_path();
    ALOGD("Inside file_index value : %d\n", file_index);
    pFile = fopen(charger_file_path[file_index], "r");
    if (pFile == NULL) {
        ALOGE("error opening file");
        return;
    } else {
        if (fgets(buffer, MAX_CHAR, pFile) == NULL) {
            fclose(pFile);
            ALOGE("can not get the string from the file");
            return;
        }
    }
    fclose(pFile);
    int type = atoi(buffer);


    if (type==0)
    {
        ALOGD("start activity by send intent to BatteryWarningReceiver to remove notification, type = %d\n", type);
        sendBroadcastMessage(String8(NORMAL_ACTION), type);
    }
    if (type > 0)
    {
        ALOGD("start activity by send intent to BatteryWarningReceiver, type = %d\n", type);
        sendBroadcastMessage(String8(ACTION), type);
    }
}

#define UEVENT_MSG_LEN 2048
static void uevent_event(uint32_t /*epevents*/) {
    char msg[UEVENT_MSG_LEN + 2];
    char *cp;
    char *status;
    int n;
    char *buffer = (char*) malloc(MAX_CHAR * sizeof(char));
    if (buffer == NULL) {
        ALOGD("malloc memory failed");
        return ;
    }
    n = uevent_kernel_multicast_recv(uevent_fd, msg, UEVENT_MSG_LEN);
    if (n <= 0) return;
    if (n >= UEVENT_MSG_LEN) /* overflow -- discard */
        return;

    msg[n] = '\0';
    msg[n + 1] = '\0';
    cp = msg;

    while (*cp) {

        if (!strncmp(cp, "BMS_STAT=", strlen("BMS_STAT="))) { // This CHGSTAT value will be provided by kernel driver
            readType(buffer);
            break;
        }

        /* advance to after the next \0 */
        while (*cp++)
            ;
    }
    free(buffer);
}

int batterywarn_register_event(int fd, void (*handler)(uint32_t)) {
    struct epoll_event ev;
    ev.events = EPOLLIN;

    //if (wakeup == EVENT_WAKEUP_FD) ev.events |= EPOLLWAKEUP;

    ev.data.ptr = (void*)handler;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        ALOGD("epoll_ctl failed; errno=%d\n", errno);
        return -1;
    }
    return 0;
}


static void uevent_init(void) {
    uevent_fd = uevent_open_socket(64 * 1024, true);
    if (uevent_fd < 0) {
        ALOGD("uevent_init: uevent_open_socket failed\n");
        return;
    }

    fcntl(uevent_fd, F_SETFL, O_NONBLOCK);
    if (batterywarn_register_event(uevent_fd, uevent_event))
        ALOGD("register for uevent events failed\n");
}
static void batterywarn_mainloop(void) {
    int nevents = 0;
    while (1) {
        struct epoll_event events[1];
        nevents = epoll_wait(epollfd, events, 1, -1);
        if (nevents == -1) {
            if (errno == EINTR) continue;
            ALOGD("batterywarn_mainloop: epoll_wait failed\n");
            break;
        }

        for (int n = 0; n < nevents; ++n) {
            if (events[n].data.ptr) (*(void (*)(int))events[n].data.ptr)(events[n].events);
        }
    }

    return;
}
static int batterywarn_init() {
    epollfd = epoll_create(MAX_EPOLL_EVENTS);
    if (epollfd == -1) {
        ALOGD("epoll_create failed; errno=%d\n", errno);
        return -1;
    }
    uevent_init();
    return 0;

}
int main()
{
    char *buffer = (char*) malloc(MAX_CHAR * sizeof(char));
    if (buffer == NULL) {
        ALOGD("malloc memory failed");
        return 0;
    }
  int ret;

  /* Read the status to catch the event when batterywarning is not started */
  readType(buffer);

  ret= batterywarn_init();
  if (ret) {
      ALOGD("Initialization failed, exiting\n");
      exit(1);
  }
  batterywarn_mainloop();
  free(buffer);
  return 0;
}
```

epoll机制暂时不说，我们看看C程序时如何发送广播的，原理是通过Binder通信调用activity: [android.app.IActivityManager] 服务发广播,提到`am->transact(IBinder::FIRST_CALL_TRANSACTION + 13, data, &reply);` 这个会调用哪个函数呢？上面的13是如何而来的呢？这个得分析一下,frameworks\base\core\java\android\app\IActivityManager.aidl，可以知道函数`broadcastIntent`对应第13项。仔细观察会发现transact后面的data参数跟broadcastIntent是对应的，这个是调用的核心的。

![0009_0012.png](images/0009_0012.png)

另外根据编译生成的文件`out/soong/.intermediates/frameworks/base/framework-minus-apex/android_common/javac/shard30/classes/android/app/IActivityManager$Stub.class`，反编译也可以看到对应函数：

![0009_0013.png](images/0009_0013.png)

参考：

* [Android C++语言 通过Binder通信调用activity: [android.app.IActivityManager] 服务发广播](https://blog.csdn.net/qq_37858386/article/details/121501400)

```C++
bool sendBroadcastMessage(String8 action, int value)
{
    ALOGD("sendBroadcastMessage(): Action: %s, Value: %d ", (char *)(action.string()), value);
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> am = sm->getService(String16("activity"));
    if (am != NULL) {
        Parcel data, reply;
        data.writeInterfaceToken(String16("android.app.IActivityManager"));
        data.writeStrongBinder(NULL);
        // Add for match AMS change on O
        data.writeInt32(1);
        // intent begin
        data.writeString8(action); // action
        data.writeInt32(0); // URI data type
        data.writeString8(NULL, 0); // type
        data.writeString8(NULL, 0); // mIdentifier
        data.writeInt32(0x04000000); // flags: FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
        data.writeString8(NULL, 0); // package name
        data.writeString8(NULL, 0); // component name
        data.writeInt32(0); // source bound - size
        data.writeInt32(0); // categories - size
        data.writeInt32(0); // selector - size
        data.writeInt32(0); // clipData - size
        data.writeInt32(-2); // contentUserHint: -2 -> UserHandle.USER_CURRENT
        data.writeInt32(-1); // bundle extras length
        data.writeInt32(0x4C444E42); // 'B' 'N' 'D' 'L'
        int oldPos = data.dataPosition();
        data.writeInt32(1);  // size
        // data.writeInt32(0); // VAL_STRING, need to remove because of analyze common intent
        data.writeString16(String16(TYPE));
        data.writeInt32(1); // VAL_INTEGER
        data.writeInt32(value);
        int newPos = data.dataPosition();
        data.setDataPosition(oldPos - 8);
        data.writeInt32(newPos - oldPos); // refill bundle extras length
        data.setDataPosition(newPos);
        // intent end
        data.writeString8(NULL, 0); // resolvedType
        data.writeStrongBinder(NULL); // resultTo
        data.writeInt32(0); // resultCode
        data.writeString8(NULL, 0); // resultData
        data.writeInt32(-1); // resultExtras
        data.writeString8(NULL, 0); // permission
        data.writeInt32(0); // appOp
        data.writeInt32(-1); // option
        data.writeInt32(1); // serialized: != 0 -> ordered
        data.writeInt32(0); // sticky
        data.writeInt32(-2); // userId: -2 -> UserHandle.USER_CURRENT

        status_t ret = am->transact(IBinder::FIRST_CALL_TRANSACTION + 13, data, &reply); // BROADCAST_INTENT_TRANSACTION
        if (ret == NO_ERROR) {
            int exceptionCode = reply.readExceptionCode();
            if (exceptionCode) {
                ALOGE("sendBroadcastMessage(%s) caught exception %d\n",
                        (char *)(action.string()), exceptionCode);
                return false;
            }
        } else {
            return false;
        }
    } else {
        ALOGE("getService() couldn't find activity service!\n");
        return false;
    }
    return true;
}
```

# 四、底层BMS功能接口提供

为了减少bms与charge、guage及平台的耦合性，BMS不和Charge、Guage直接交互，在他们中间增加了BMS Notify，BMS driver调用power_supply_get_property和power_supply_reg_notifier方法通过PSY core来获取Charger和Guage相关信息，同时可通过BMS Notify调bms_notify_call_chain向Charge发送相关指令. 

![0009_0011.png](images/0009_0011.png)

## HAL层接口实现

```C++
/home/wugn/M8-project/vendor/mediatek/proprietary/hardware/libpax_bms
wugn@jcrj-tf-compile:libpax_bms$ tree
.
├── 1.0
│   └── default
│       ├── android.hardware.pax_bms@1.0-service-lazy.rc
│       ├── android.hardware.pax_bms@1.0-service.rc //rc is for launch HAL service，将调用service.cpp main函数
│       ├── Android.mk
│       ├── PaxBMS.cpp
│       ├── PaxBMS.h
│       ├── service.cpp //defaultPassthroughServiceImplementation<IPaxBMS>(); Hal启动进程android.hardware.pax_bms@1.0-service
│       └── serviceLazy.cpp
├── Android.mk
├── pax_bms.c
└── pax_bms_test.c
```

hal层是采用传统的HAL架构，device调用module方式，关系图如下：

![0009_0003.png](images/0009_0003.png)
*  `pax_bms.c`作为`hw_module_t`为`hw_device_t`提供pax_bms_ctl(ioctrl)和close_pax_bms方法，主要是实现填充`hw_device_t`结构体中的方法。
* `PaxBMS.cpp`主要在构造函数中调用`getPaxBMSDevice`函数，函数中`hw_get_module`获取并调用hwModule的open方法，open方法会返回`hw_device_t`结构体指针`pax_bmsDevice`，其中就是`hw_module_t`的方法，这样就可以调用`hw_get_module`封装的pax_bms_ctl(ioctrl)和close_pax_bms方法了。

### hw_module_t pax_bms.c

pax_bms.c作为hw_module_t为hw_device_t提供pax_bms_ctl(ioctrl)和close_pax_bms方法。

```C++
vendor/mediatek/proprietary/hardware/libpax_bms/pax_bms.c
#define LOG_TAG "bms"


#include <log/log.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/pax_bms.h>

/******************************************************************************/

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

#define PAX_BMS_DEV			"/dev/pax_bms"

/**
 * device methods
 */

void init_globals(void)
{
    // init the mutex
    pthread_mutex_init(&g_lock, NULL);
}

static int open_pax_bms_dev(void)
{
	int fd = -1;

	fd = open(PAX_BMS_DEV, O_RDWR);

	return fd;
}

/** Close the pax bms device */
static int
close_pax_bms(struct pax_bms_dev_t *dev)
{
    if (dev) {
        free(dev);
    }
    return 0;
}

static int pax_bms_ctl(struct pax_bms_dev_t *dev, unsigned long cmd, void *data)
{
	static int pax_bms_fd = -1;
	int ret = -1;

	if (dev == NULL)
		return -1;

	if (pax_bms_fd < 0) {
		pax_bms_fd = open_pax_bms_dev();

		if (pax_bms_fd < 0) {
			return -ENODEV;
		}
	}

    pthread_mutex_lock(&g_lock);

	switch(cmd) {
		case SET_CHG_EN:
		case SET_POWER_PATH:
			{
				int en = *(int *)data;
				ret = ioctl(pax_bms_fd, cmd, &en);
			}
			break;
		default:
			break;
	};

    pthread_mutex_unlock(&g_lock);

	return ret;
}

/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a device using name */
static int open_pax_bms(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    pthread_once(&g_init, init_globals);

	if (strcmp(PAX_BMS_HARDWARE_MODULE_ID, name) != 0)
		return -1;

    struct pax_bms_dev_t *dev = malloc(sizeof(struct pax_bms_dev_t));
    if (!dev)
        return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_pax_bms;
    dev->bms_ctl = pax_bms_ctl;

    *device = (struct hw_device_t*)dev;
    return 0;
}


static struct hw_module_methods_t pax_bms_module_methods = {
    .open =  open_pax_bms,
};

/*
 * The pax bms Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    //.version_major = 1,
    //.version_minor = 0,
    .id = PAX_BMS_HARDWARE_MODULE_ID,
    .name = "pax_bms Module",
    .author = "pax",
    .methods = &pax_bms_module_methods,
};
```

### hw_device_t PaxBMS.cpp 

`PaxBMS.cpp`主要调用`hw_get_module`获取并调用hwModule的open方法，open方法会返回hw_device_t结构体指针`pax_bmsDevice`，这样就可以调用`hw_get_module`封装的ioctrl方法了。

```c++
vendor/mediatek/proprietary/hardware/libpax_bms/1.0/default/PaxBMS.cpp
/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "PaxBMS.h"

namespace android {
namespace hardware {
namespace pax_bms {
namespace V1_0 {
namespace implementation {

pax_bms_dev_t* getPaxBMSDevice() {
    pax_bms_dev_t* pax_bmsDevice = NULL;
    const hw_module_t* hwModule = NULL;

    int ret = hw_get_module (PAX_BMS_HARDWARE_MODULE_ID, &hwModule);
    if (ret == 0) {
        ret = hwModule->methods->open(hwModule, PAX_BMS_HARDWARE_MODULE_ID,
            reinterpret_cast<hw_device_t**>(&pax_bmsDevice));
        if (ret != 0) {
            ALOGE("pax_bms_open %s failed: %d", PAX_BMS_HARDWARE_MODULE_ID, ret);
        }
    } else {
        ALOGE("hw_get_module %s failed: %d", PAX_BMS_HARDWARE_MODULE_ID, ret);
    }

    if (ret == 0) {
        return pax_bmsDevice;
    } else {
        ALOGE("PaxBMS passthrough failed to load legacy HAL.");
        return nullptr;
    }
}

PaxBMS::PaxBMS()
  : mPaxBMS(NULL) {
	  mPaxBMS = getPaxBMSDevice();
}

PaxBMS::~PaxBMS() {
	if (mPaxBMS) {
		mPaxBMS->common.close(reinterpret_cast<hw_device_t*>(mPaxBMS));
		mPaxBMS = NULL;
	}
}

// Methods from ::android::hardware::pax_bms::V1_0::IPaxBMS follow.
Return<int32_t> PaxBMS::EnableCharge() {
	int ret = 0;
	int en = 1;

	if (mPaxBMS) {
		ret = mPaxBMS->bms_ctl(mPaxBMS, SET_CHG_EN, &en);
	}

	return ret;
}

// Methods from ::android::hardware::pax_bms::V1_0::IPaxBMS follow.
Return<int32_t> PaxBMS::DisableCharge() {
	int ret = 0;
	int en = 0;

	if (mPaxBMS) {
		ret = mPaxBMS->bms_ctl(mPaxBMS, SET_CHG_EN, &en);
	}

	return ret;
}

// Methods from ::android::hardware::pax_bms::V1_0::IPaxBMS follow.
Return<int32_t> PaxBMS::EnablePowerPath() {
	int ret = 0;
	int en = 1;

	if (mPaxBMS) {
		ret = mPaxBMS->bms_ctl(mPaxBMS, SET_POWER_PATH, &en);
	}

	return ret;
}

// Methods from ::android::hardware::pax_bms::V1_0::IPaxBMS follow.
Return<int32_t> PaxBMS::DisablePowerPath() {
	int ret = 0;
	int en = 0;

	if (mPaxBMS) {
		ret = mPaxBMS->bms_ctl(mPaxBMS, SET_POWER_PATH, &en);
	}

	return ret;
}

IPaxBMS* HIDL_FETCH_IPaxBMS(const char* /* name */) {

    return new PaxBMS();
}

} // namespace implementation
}  // namespace V1_0
}  // namespace pax_bms
}  // namespace hardware
}  // namespace android
```

## HIDL Arch

* HIDL层接口定义：

```C++
/home/wugn/M8-project/hardware/interfaces/pax_bms

wugn@jcrj-tf-compile:pax_bms$ tree
.
└── 1.0
    ├── Android.bp
    ├── IPaxBMS.hal
    └── types.hal

IPaxBMS.hal:
package android.hardware.pax_bms@1.0;

interface IPaxBMS {
    EnableCharge() generates (int32_t res);
    DisableCharge() generates (int32_t res);
    EnablePowerPath() generates (int32_t res);
    DisablePowerPath() generates (int32_t res);
};
```

### 注册HIDL服务

```diff
diff --git a/vendor/mediatek/proprietary/hardware/mtk-hidl-service/Android.mk b/vendor/mediatek/proprietary/hardware/mtk-hidl-service/Android.mk
index d8e5e306de1..d8f5f70318e 100644
--- a/vendor/mediatek/proprietary/hardware/mtk-hidl-service/Android.mk
+++ b/vendor/mediatek/proprietary/hardware/mtk-hidl-service/Android.mk
@@ -30,6 +30,11 @@ LOCAL_SHARED_LIBRARIES += \
     android.hardware.graphics.allocator@2.0 \
     vendor.mediatek.hardware.gpu@1.0 \
 
+#ADD BEGIN by shanliangliang@paxsz.com, add for BMS 2021/07/12
+LOCAL_SHARED_LIBRARIES += \
+    android.hardware.pax_bms@1.0 \
+#ADD END by shanliangliang@paxsz.com, add for BMS 2021/07/12
+
     LOCAL_REQUIRED_MODULES := \
         android.hardware.thermal@2.0-impl
 
diff --git a/vendor/mediatek/proprietary/hardware/mtk-hidl-service/service.cpp b/vendor/mediatek/proprietary/hardware/mtk-hidl-service/service.cpp
index be73b5d2be9..0461dbd9a22 100644
--- a/vendor/mediatek/proprietary/hardware/mtk-hidl-service/service.cpp
+++ b/vendor/mediatek/proprietary/hardware/mtk-hidl-service/service.cpp
@@ -27,6 +27,10 @@
 #include <android/hardware/gnss/2.1/IGnss.h>
 #endif
 
+//ADD BEGIN by shanliangliang@paxsz.com, add for BMS 2021/07/12
+#include <android/hardware/pax_bms/1.0/IPaxBMS.h>
+//ADD END by shanliangliang@paxsz.com, add for BMS 2021/07/12
+
 using ::android::OK;
 using ::android::status_t;
 using ::android::hardware::configureRpcThreadpool;
@@ -40,6 +44,10 @@ using ::vendor::mediatek::hardware::gpu::V1_0::IGraphicExt;
 using ::android::hardware::gnss::V2_1::IGnss;
 #endif
 
+//ADD BEGIN by shanliangliang@paxsz.com, add for BMS 2021/07/12
+using ::android::hardware::pax_bms::V1_0::IPaxBMS;
+//ADD END by shanliangliang@paxsz.com, add for BMS 2021/07/12
+
 #define register(service) do { \
     status_t err = registerPassthroughServiceImplementation<service>(); \
     if (err != OK) { \
@@ -59,6 +67,10 @@ void* mtkHidlService(void *data)
     register(IAllocator);
     register(IGraphicExt);
 
+//ADD BEGIN by shanliangliang@paxsz.com, add for BMS 2021/07/12
+    register(IPaxBMS);
+//ADD END by shanliangliang@paxsz.com, add for BMS 2021/07/12
+
 #ifdef MTK_GPS_SUPPORT
     register(IGnss);
     ::vendor::mediatek::hardware::lbs::V1_0::implementation::cpp_main();
-- 
2.17.1
```

### 添加HIDL服务编译选项

```DIFF
--- a/device/mediatek/mt6765/device.mk
+++ b/device/mediatek/mt6765/device.mk
@@ -1051,6 +1051,14 @@ PRODUCT_PACKAGES += \
 PRODUCT_PACKAGES += \
     android.hardware.lights-service.mediatek
 
+#Added BEGIN by shanliangliang@paxsz.com for BMS @2021-07-12
+PRODUCT_PACKAGES += pax_bms.mt6765
+#PRODUCT_PACKAGES += pax_bms_test
+PRODUCT_PACKAGES += android.hardware.pax_bms@1.0-impl
+PRODUCT_PACKAGES += android.hardware.pax_bms@1.0-service-lazy
+PRODUCT_PACKAGES += android.hardware.pax_bms@1.0-service
+#Added END by shanliangliang@paxsz.com for BMS @2021-07-12
```

### 添加HIDL manifest

```diff
--- a/device/mediatek/mt6765/manifest.xml
+++ b/device/mediatek/mt6765/manifest.xml
@@ -143,4 +143,16 @@
             <instance>default</instance>
         </interface>
     </hal>
+	<!--ADD BEGIN by shanliangliang@paxsz.com add for BMS 2021/07/12 -->
+    <hal format="hidl">
+        <name>android.hardware.pax_bms</name>
+        <transport>hwbinder</transport>
+        <impl level="generic"></impl>
+        <version>1.0</version>
+        <interface>
+            <name>IPaxBMS</name>
+            <instance>default</instance>
+        </interface>
+    </hal>
+	<!--ADD END by shanliangliang@paxsz.com add for BMS 2021/07/12 -->
 </manifest>
```

### 启动HIDL服务

```c++
vendor/mediatek/proprietary/hardware/libpax_bms/1.0/default/android.hardware.pax_bms@1.0-service.rc
@ -0,0 +1,9 @@
service pax_bms-hal-1-0 /vendor/bin/hw/android.hardware.pax_bms@1.0-service
    interface android.hardware.pax_bms@1.0::IPaxBMS default
    oneshot
    class hal
    user system
    group system
    # shutting off pax bms while powering-off
    shutdown critical

vendor/mediatek/proprietary/hardware/libpax_bms/1.0/default/service.cpp
@ -0,0 +1,28 @@
/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define NDEBUG 1
#define LOG_TAG "android.hardware.pax_bms@2.0-service-mediatek"

#include <android/hardware/pax_bms/1.0/IPaxBMS.h>
#include <hidl/LegacySupport.h>

using android::hardware::pax_bms::V1_0::IPaxBMS;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    return defaultPassthroughServiceImplementation<IPaxBMS>();
}
```

### 查看HIDL服务进程

实际上是一个`binder_ioctl_write_read`的进程。

```
PAYTABLETM8:/ # ps -A | grep bms
system          479      1 10776264  5036 binder_ioctl_write_read 0 S android.hardware.pax_bms@1.0-service
```