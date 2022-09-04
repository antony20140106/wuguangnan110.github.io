# 概述

Android kernel经常碰到调整驱动加载顺序，详细看一下。

# 参考

* [module_platform_driver 与 module_init](https://blog.csdn.net/m0_37765662/article/details/106490792)

# 分析

1. 从代码中看到，module_platform_driver 追根溯源，发现最终还是调用了module_init，但是，又不仅仅是调用了module_init，还调用了platform_driver_register和platform_driver_unregister，这两个函数的作用就是注册和卸载平台驱动。

2. ，module_platform_driver就是对module_init进一步的封装，在module_init之外添加了一些功能，对于平台总线设备而言，直接调用module_platform_driver就可以避免在module_init函数中去注册平台驱动了，使得平台设备驱动的加载变得更方便了。

3. 一般某个设备或某个控制器挂载在处理器上时，肯定是通过某种总线来连接的，比喻说常见总线有SPI总线、IIC总线等等，但控制器一般是通过内部总线挂载处理器端的，对于这一类设备，linux抽象出了一个平台总线，来包含所有的处理器总线挂载的设备。而这类总线需要注册到内核中去，就需要用platform_driver_register来实现，平台总线是抽象出来的，所以所有通过总线直接连在处理器上的设备是不需要关心平台总线怎么运作的，因此这个平台总线的注册和注销都是通用的，所以在加载总线设备驱动时，直接调用module_platform_driver，就可以将平台驱动注册函数和卸载函数、以及总线设备加载一次性运行完，避免了总线驱动在每次加载驱动时都需要手动注册平台总线。

# 用法

那是不是说module_init可以与module_platform_driver就可以通用呢？当然不是，能用module_platform_driver的地方肯定可以用module_init，但能用module_init却不一定能用module_platform_driver，这主要看设备是不是通过平台总线的方式挂载处理器上的。


函数名	|非平台总线设备|平台总线设备
---|---|---
module_platform_driver	|不可用	|可用
module_init|	可用	|可用

* 平台总线设备，可以用两种方法来加载:
```C++
module_platform_driver(xxxx_driver);
```

* 但对于其他总线的设备，就不能使用module_platform_driver了，必须使用module_init，在module_init函数中再注册一遍其他总线，例如SPI总线设备。
```C++
static int __init spidev_init(void)
{
	int status;
 
	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
	if (status < 0)
		return status;
 
	spidev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spidev_class)) {
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
		return PTR_ERR(spidev_class);
	}
 
	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
	}
	return status;
}
module_init(spidev_init);
 
static void __exit spidev_exit(void)
{
	spi_unregister_driver(&spidev_spi_driver);
	class_destroy(spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
}
module_exit(spidev_exit);
```

# 特别注意

还有一种情况不能使用module_platform_driver，就是我们编写的驱动程序要使用之前某个驱动程序的功能时，就不能使用module_platform_driver，因为如果都使用module_platform_driver，内核编译链接时，并不知道哪个驱动程序会被链接再前面，也就不知道哪个驱动程序会先执行，如果使用自定义的驱动（使用其他驱动程序功能的程序）先执行，而其他驱动还未进行初始化，就会出现难以预料的问题，所以这种情况下就必须使用late_initcall