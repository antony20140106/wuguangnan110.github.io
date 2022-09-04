# 概述

kernel中为了方便对battery的管理，专门提供了power supply framework

* 1、电池监控（fuelgauge）

fuelgauge驱动主要是负责向上层android系统提供当前电池的电量以及健康状态信息等等，另外除了这个以外，它也向charger驱动提供电池的相关信息

*2、充放电管理（charger）

charger驱动主要负责电源线的插拔检测，以及充放电的过程管理。对于battery管理，硬件上有电量计IC和充放电IC

# 参考

* [Linux Power supply子系统分析](https://blog.csdn.net/weixin_46376201/article/details/125201962)

# Power Supply软件架构

power supply framework在kernel/drivers/power/下，主要由3部分组成：

* 1、power supply core，用于抽象核心数据结构、实现公共逻辑。位于drivers/power/power_supply_core.c中
* 2、power supply sysfs，实现sysfs以及uevent功能。位于drivers/power/power_supply_sysfs.c中
* 3、power supply leds，基于linux led class，提供PSY设备状态指示的通用实现。位于drivers/power/power_suppply_leds.c中。

# struct power_supply

struct power_supply为power supply class的核心数据结构，用于抽象PSY设备
```C++
struct power_supply {
    const char *name;   //该PSY的名称
    enum power_supply_type type; //该PSY的类型，枚举类型，一般包括:battery、USB-charger、DC-charger。
    enum power_supply_property *properties; //该PSY具有的属性列表，枚举型。
    size_t num_properties;  //属性的个数
 
    char **supplied_to; //一个字符串数组，保存了由该PSY供电的PSY列表，以此可将PAY组成互相级联的PSY链表。这些被供电的PSY，称作supplicant（客户端，乞求者）
    size_t num_supplicants;//supplicant的个数。
 
    char **supplied_from; //一个字符串数组，保存了该PSY供电的PSY列表，也称作supply（提供者），从另外一个方向组织PSY之间的级联关系。
    size_t num_supplies; //supply的个数
#ifdef CONFIG_OF
    struct device_node *of_node; //用于保存of_node指针。
#endif
 
    int (*get_property)(struct power_supply *psy, //PSY driver需要实现的回调函数，用于获取属性值。
                enum power_supply_property psp,
                union power_supply_propval *val);
    int (*set_property)(struct power_supply *psy,  //PSY driver需要实现的回调函数，用于设置属性值。
                            enum power_supply_property psp,
                const union power_supply_propval *val);
    int (*property_is_writeable)(struct power_supply *psy,  //返回指定的属性值是否可写（用于sysfs）；
                     enum power_supply_property psp);
    void (*external_power_changed)(struct power_supply *psy); //当一个PSY设备存在supply PSY，且该supply PSY的属性发生改变（如online offline）时，power supply core会调用该回调函数，通知PSY driver以便让它做出相应处理。
    void (*set_charged)(struct power_supply *psy);     //该回调函数的应用场景有点奇怪，外部模块通知PAY driver，该PSY设备的状态改变了，自己改变了自己不知道，要外部通知，希望大家在实际工作中不要遇到，太纠结了。
 
    /* For APM emulation, think legacy userspace. */
    int use_for_apm;
 
    /* private */
    struct device *dev;
    struct work_struct changed_work; //用户处理状态改变的work queue，主要思路是当该PSY的状态发生改变，启动一个workqueue，查询并通知所有的supplicants。
    spinlock_t changed_lock;
    bool changed;
#ifdef CONFIG_THERMAL
    struct thermal_zone_device *tzd;
    struct thermal_cooling_device *tcd;
#endif
 
#ifdef CONFIG_LEDS_TRIGGERS
    struct led_trigger *charging_full_trig;
    char *charging_full_trig_name;
    struct led_trigger *charging_trig;
    char *charging_trig_name;
    struct led_trigger *full_trig;
    char *full_trig_name;
    struct led_trigger *online_trig; //如果配置了CONFIC_LED_TRIGGERS，则调用的linux led class的接口，注册相应的LED设备，用于PSY状态指示。
    char *online_trig_name;
    struct led_trigger *charging_blink_full_solid_trig;
    char *charging_blink_full_solid_trig_name;
#endif
}
```

# PSY的类型

```C++
enum power_supply_type {
    POWER_SUPPLY_TYPE_UNKNOWN = 0, //未知
    POWER_SUPPLY_TYPE_BATTERY,     //电池，手机平板上面最常用的
    POWER_SUPPLY_TYPE_UPS,         //不间断供电的，一般用户服务器   
    POWER_SUPPLY_TYPE_MAINS,        //主供电设备，如笔记本电脑适配器，特点是可以单独供电，当其断开时，再由辅助设备供电。
    POWER_SUPPLY_TYPE_USB,      /* Standard Downstream Port */
    POWER_SUPPLY_TYPE_USB_DCP,  /* Dedicated Charging Port */
    POWER_SUPPLY_TYPE_USB_CDP,  /* Charging Downstream Port */
    POWER_SUPPLY_TYPE_USB_ACA,  /* Accessory Charger Adapters */
};
```

# PSY属性

power supply class将所有可能PSY属性，以枚举型变量形式抽象出来，PSY driver可以根据设备的实际情况，从中选取一些。

```C++
enum power_supply_property {
    /* Properties of type `int' */
    POWER_SUPPLY_PROP_STATUS = 0, //该PSY的status，主要是充电状态，包括：unknown,charging,discharging,not charging full,
    POWER_SUPPLY_PROP_CHARGE_TYPE,//充电类型
    POWER_SUPPLY_PROP_HEALTH, //健康状况，包括：good dead  over voltage等
    POWER_SUPPLY_PROP_PRESENT, //电量百分比
    POWER_SUPPLY_PROP_ONLINE,  //是否在线
    POWER_SUPPLY_PROP_AUTHENTIC,
    POWER_SUPPLY_PROP_TECHNOLOGY, //采用的技术
    POWER_SUPPLY_PROP_CYCLE_COUNT,
    POWER_SUPPLY_PROP_VOLTAGE_MAX,
    POWER_SUPPLY_PROP_VOLTAGE_MIN,
    POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_VOLTAGE_AVG,
    POWER_SUPPLY_PROP_VOLTAGE_OCV,
    POWER_SUPPLY_PROP_CURRENT_MAX,
    POWER_SUPPLY_PROP_CURRENT_NOW,
    POWER_SUPPLY_PROP_CURRENT_AVG,
    POWER_SUPPLY_PROP_POWER_NOW,
    POWER_SUPPLY_PROP_POWER_AVG,
    POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
    POWER_SUPPLY_PROP_CHARGE_EMPTY_DESIGN,
    POWER_SUPPLY_PROP_CHARGE_FULL,
    POWER_SUPPLY_PROP_CHARGE_EMPTY,
    POWER_SUPPLY_PROP_CHARGE_NOW,
    POWER_SUPPLY_PROP_CHARGE_AVG,
    POWER_SUPPLY_PROP_CHARGE_COUNTER,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX,
    POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
    POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
    POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
    POWER_SUPPLY_PROP_ENERGY_EMPTY_DESIGN,
    POWER_SUPPLY_PROP_ENERGY_FULL,
    POWER_SUPPLY_PROP_ENERGY_EMPTY,
    POWER_SUPPLY_PROP_ENERGY_NOW,
    POWER_SUPPLY_PROP_ENERGY_AVG,
    POWER_SUPPLY_PROP_CAPACITY, /* in percents! */
    POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN, /* in percents! */
    POWER_SUPPLY_PROP_CAPACITY_ALERT_MAX, /* in percents! */
    POWER_SUPPLY_PROP_CAPACITY_LEVEL, //容量
    POWER_SUPPLY_PROP_TEMP,
    POWER_SUPPLY_PROP_TEMP_ALERT_MIN,
    POWER_SUPPLY_PROP_TEMP_ALERT_MAX,
    POWER_SUPPLY_PROP_TEMP_AMBIENT,
    POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN,
    POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX,
    POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
    POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
    POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
    POWER_SUPPLY_PROP_TIME_TO_FULL_AVG,
    POWER_SUPPLY_PROP_TYPE, /* use power_supply.type instead */
    POWER_SUPPLY_PROP_SCOPE,
    /* Local extensions */
    POWER_SUPPLY_PROP_USB_HC,
    POWER_SUPPLY_PROP_USB_OTG,
    POWER_SUPPLY_PROP_CHARGE_ENABLED,
    /* Local extensions of type int64_t */
    POWER_SUPPLY_PROP_CHARGE_COUNTER_EXT,
    /* Properties of type `const char *' */
    POWER_SUPPLY_PROP_MODEL_NAME,
    POWER_SUPPLY_PROP_MANUFACTURER,
    POWER_SUPPLY_PROP_SERIAL_NUMBER
```

# PSY driver提供的API

power supply class 首要任务，是向PSY driver提供统一的驱动编程接口，主要包括：

## 1.PSY 的register/unregister API

```C++
extern int power_supply_register(struct device *parent,
                 struct power_supply *psy);
extern void power_supply_unregister(struct power_supply *psy);
extern int power_supply_powers(struct power_supply *psy, struct device *dev
```

## 2.PSY状态改变时通知power supply core的API

* extern void power_supply_changed(struct power_supply *psy);

当PSY driver检测到该设备某些属性改变时，需要调用这个接口，通知power supply core，power supply core会有如下动作
如果该PSY是其他PSY的供电源，调用这些PSY的external_power_changed回调函数，通知他们（这些PSY具体要做什么，由他们自身的逻辑决定）；
发送notifier通知那些关心PSY设备状态的drivers；
以统一的格式向用户空间发送uevent。

其他一些接口
```C++
extern struct power_supply *power_supply_get_by_name(const char *name); //通过名字获取PSY指针
extern struct power_supply *power_supply_get_by_phandle(struct device_node *np, 
                                                           const char *property); //从DTS中，解析出对应的PSY指针。
extern int power_supply_am_i_supplied(struct power_supply *psy);//查询自己是否由其他PSY供电
extern int power_supply_set_battery_charged(struct power_supply *psy);//调用指定的PSY的set_changed回调。
extern int power_supply_is_system_supplied(void); //检查系统中是否有有效的或者处于online状态的PSY，如果没有，可能为桌面系统、
 
extern int power_supply_powers(struct power_supply *psy, struct device *dev);//在指定的设备的sysfs目录下创建指定的PSY符合连接。
```

## 3.devm_power_supply_get_by_phandle接口使用

参考：
* [power_supply 探究设备 关系](https://blog.csdn.net/hanguangce/article/details/116270277)

* power_supply_get_by_phandle实例

以下是从dts中获取
```C++
dts:
        mt6370_chg: charger {
                    compatible = "mediatek,mt6370_pmu_charger";
                    charger = <&mt6370_chg>;
        }

code：

static int mt6370_pmu_charger_probe(struct platform_device *pdev)
{
#ifdef CONFIG_TCPC_CLASS
	chg_data->chg_psy = devm_power_supply_get_by_phandle(&pdev->dev,
							     "charger");
	if (IS_ERR(chg_data->chg_psy)) {
		dev_notice(&pdev->dev, "Failed to get charger psy\n");
		ret = PTR_ERR(chg_data->chg_psy);
		goto err_psy_get_phandle;
	}
#endif
}

static const struct of_device_id mt_ofid_table[] = {
	{ .compatible = "mediatek,mt6370_pmu_charger", },
	{ },
};
MODULE_DEVICE_TABLE(of, mt_ofid_table);

static const struct platform_device_id mt_id_table[] = {
	{ "mt6370_pmu_charger", 0},
	{ },
};
MODULE_DEVICE_TABLE(platform, mt_id_table);

static struct platform_driver mt6370_pmu_charger = {
	.driver = {
		.name = "mt6370_pmu_charger",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mt_ofid_table),
	},
	.probe = mt6370_pmu_charger_probe,
	.remove = mt6370_pmu_charger_remove,
	.id_table = mt_id_table,
};
```

* 原理
```C++
* devm_power_supply_get_by_phandle(struct device *dev,
  * power_supply_get_by_phandle(dev->of_node, property);
    * power_supply_np = of_parse_phandle(np, property, 0);
    * dev = class_find_device(power_supply_class, NULL, power_supply_np,power_supply_match_device_node);
    * psy = dev_get_drvdata(dev); //私有数据我们找到设备-通过私有数据找到 psy
```

* 注册原理
```C++
* power_supply_register(struct device *parent,
  * __power_supply_register(parent, desc, cfg, true);
    * dev->class = power_supply_class;
    * dev_set_drvdata(dev, psy);  //和前面对应构建psy  信息 并创建个设备 ，设置为dev 的私有数据
```

还有一种更简单的方法，通过psy设备名获取，但是可移植性不强，如下：
```C++
qphy->usb_psy = power_supply_get_by_name("pax-usb");
```


## 4.power_supply_get_drvdata获取私有数据

我们可以在注册psy设备时，将结构体数据保存在power_supply提供的drv_data中
```C++
static void pax_charger_external_power_changed(struct power_supply *psy)
{
	struct pax_charger *info;
	union power_supply_propval prop, prop2;
	int ret;

	info = (struct pax_charger *)power_supply_get_drvdata(psy);
}

static int pax_charger_probe(struct platform_device *pdev)
{
	info->psy_cfg1.drv_data = info;
	info->psy1 = power_supply_register(&pdev->dev, &info->psy_desc1,
			&info->psy_cfg1);
}
```

* 原理：drv_data是一个vold类型的数据，获取时直接返回。
```C++
struct power_supply {
	/* Driver private data */
	void *drv_data;
}

void *power_supply_get_drvdata(struct power_supply *psy)
{
	return psy->drv_data;
}

```


# 怎样基于power supply class编写PSY driver

（1）根据硬件spec，确定PSY设备具备哪些特性，并把他们和enum power_supply_property对应。
（2）根据实际情况，实现这些properties的get/set接口。
（3）定义一个struct power_supply 变量，并初始化必要字段后，调用power_supply_register或者power_supply_register_no_ws，将其注册到kernel中。
（4）根据实际情况，启动设备属性变化的监控逻辑，例如中断，轮询等，并在发生改变时，调用power_supply_changed，通知power suopply core。

* 向用户空间程序提供的API

power supply class通过两种形式向用户空间提供接口。
* 1）uevent 以“名字=value”的形式，上报所有property的值 uevent一般会在PSY设备添加到kernel时，或者PSY属性发生改变时发送。
* 2）sysfs 如果某个PSY设备具有某个属性，该属性对应的attribute就会体现在sysfs中（一般位于“/sys/class/power_supply/xxx/”中）

作者：酥酥肉
链接：https://www.jianshu.com/p/36ddaa857e5c
来源：简书
著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。