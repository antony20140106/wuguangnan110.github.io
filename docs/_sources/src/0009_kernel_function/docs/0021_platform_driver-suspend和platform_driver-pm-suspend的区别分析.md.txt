# 概述

最近在看Linux电源管理模块，发现在platform_driver下有suspend/resume函数，在platform_driver->pm-下也有suspend/resume函数。下面分析一下他们的区别。

# 参考

* [platform_driver->suspend 和platform_driver-pm->suspend的区别分析](https://blog.csdn.net/u012830148/article/details/78667420)
* [探究platform_driver中“多态”思想](https://www.cnblogs.com/swnuwangyun/p/4233821.html)

# 分析

```C++
static const struct dev_pm_ops s_atc260x_irkeypad_pm_ops = {
	.suspend = atc260x_irkeypad_suspend,
	.resume = atc260x_irkeypad_resume,
};
 
 
static struct platform_driver atc260x_irkeypad_driver = {
	.driver = {
		.name = "atc260x-irkeypad",
		.owner = THIS_MODULE,
		.pm = &s_atc260x_irkeypad_pm_ops,
		.of_match_table = of_match_ptr(atc260x_irkey_of_match),
	},
	.probe = atc260x_irkeypad_probe,
	.remove = atc260x_irkeypad_remove,
	.shutdown = atc260x_irkeypad_shutdown,
};

第二种：
static struct platform_driver atc260x_adckeypad_driver = {
	.driver = {
		.name = "atc260x-adckeypad",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(atc260x_adckey_of_match),
	},
	.probe = atc260x_adckeypad_probe,
	.remove = atc260x_adckeypad_remove,
	.suspend = atc260x_adckeypad_suspend,
	.resume = atc260x_adckeypad_resume,
};
```

# 区别

* 如果driver->pm为空的话就回去调用platform_legacy_suspend，如果不为空的话就调用driver->pm，platform_legacy_suspend则是去调用platform_driver中的suspend。
```C++

plaPlatform.c (kernel\drivers\base)
int platform_pm_suspend(struct device *dev)
{
	struct device_driver *drv = dev->driver;
	int ret = 0;
 
	if (!drv)
		return 0;
 
	if (drv->pm) {
		if (drv->pm->suspend)
			ret = drv->pm->suspend(dev);
	} else {
		ret = platform_legacy_suspend(dev, PMSG_SUSPEND);
	}
 
	return ret;
}

static int platform_legacy_suspend(struct device *dev, pm_message_t mesg)
{
	struct platform_driver *pdrv = to_platform_driver(dev->driver);
	struct platform_device *pdev = to_platform_device(dev);
	int ret = 0;
 
	if (dev->driver && pdrv->suspend)
		ret = pdrv->suspend(pdev, mesg);
 
	return ret;
}
```

第二方式是一种比较旧的方式，建议使用最新的`dev_pm_ops`方式来进行电源管理。