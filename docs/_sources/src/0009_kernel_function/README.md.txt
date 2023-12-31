# README

记录kernel底层一些常用API或者通用性知识。

## docs

NO.|文件名称|摘要
:--:|:--|:--
0034| [linux中断集合](docs/0034_linux中断集合.md) | 汇总一下linux中断知识。
0033| [linux动态输出接口pr_debug使用方法](docs/0033_linux动态输出接口pr_debug使用方法.md) | 在系统运行过程中，维护者可以通过控制 pr_debug 的开关来动态的配置某个模块中的调试信息是否输出，相对于 printk 来说，它显然是更加灵活。
0032| [linux_i2c偶发读取错误数据问题分析](docs/0032_linux_i2c偶发读取错误数据问题分析.md) | 发现charger i2c老是读取的数据由问题，分析一下
0031| [qcom_qcm2290关机流程](docs/0031_qcom_qcm2290关机流程.md) | 梳理一下Android平台关机流程
0030| [qcom_qcm2290_i2c_runtime_pm驱动流程](docs/0030_qcom_qcm2290_i2c_runtime_pm驱动流程.md) | 高通平台i2c总线初始化和其他平台不同，休眠时会将pinctrl配置为gpio并且禁止上下拉，也就是SDA/SCL拉低了，看一下流程。
0029| [driver_probe_defer延迟机制](docs/0029_driver_probe_defer延迟机制.md) | probe defer延迟机制，让probe轮询起来。
0028| [platform_i2c_driver_probe匹配devices及调用流程](docs/0028_platform_i2c_driver_probe匹配devices及调用流程.md) | 本文主要讲解一下platform driver是如何匹配platform device并执行probe函数的。
0027| [设备树解析成device过程及platform设备注册](docs/0027_设备树解析成device过程及platform设备注册.md) | 一般把驱动程序分成两部分platform_device跟platform_driver这两部分，
0026| [kernel工作队列(workqueue)简介](docs/0026_kernel工作队列(workqueue)简介.md) | 特性：
0025| [内核卡死实例分析](docs/0025_内核卡死实例分析.md) | 项目实际遇到的一些内核卡死问题分析汇总。
0024| [mtk_qcom_添加cmdline实现方案](docs/0024_mtk_qcom_添加cmdline实现方案.md) | 我们经常要在开机阶段检测某些硬件信息传递给kernel，可以通过cmdline方式传递。
0023| [class_create_device_create功能分析](docs/0023_class_create_device_create功能分析.md) | 本文介绍linux中class_create和device_create的相关使用方法。
0022| [I2C驱动编程SMBUS介绍](docs/0022_I2C驱动编程SMBUS介绍.md) | SMBus（System Management Bus）是系统管理总线的简称，该总线由SBS-IF提出并维护（SBS-IF，Smart Battery System Implementers Forum，智能电池系统实现者论坛），该论坛发起者为Intel。SMBus总线主要应用于智能电池与MCU之间的通信，PC系统中与系统传感器之间的低速通信等。
0021| [platform_driver-suspend和platform_driver-pm-suspend的区别分析](docs/0021_platform_driver-suspend和platform_driver-pm-suspend的区别分析.md) | 最近在看Linux电源管理模块，发现在platform_driver下有suspend/resume函数，在platform_driver->pm-下也有suspend/resume函数。下面分析一下他们的区别。
0020| [linux内核API总结](docs/0020_linux内核API总结.md) | Android kernel经常需要用到一些内核API，总结以下用过的。
0019| [驱动模板](docs/0019_驱动模板.md) | Android kernel经常需要自己新写一个驱动，现在增加模板方便后续添加。
0018| [container_of的用法](docs/0018_container_of的用法.md) | Android kernel经常碰到调整驱动加载顺序，详细看一下。
0017| [Android_module_platform_driver与module_init区别](docs/0017_Android_module_platform_driver与module_init区别.md) | Android kernel经常碰到调整驱动加载顺序，详细看一下。
0016| [Android如何调整内核模块加载顺序](docs/0016_Android如何调整内核模块加载顺序.md) | Android kernel经常碰到调整驱动加载顺序，详细看一下。
0015| [Android上层调用底层不同方式实现](docs/0015_Android上层调用底层不同方式实现.md) | Android framework开发人员经常需要调用到底层的接口和底层通信，比如binder调用，下面介绍几种常用的方式。
0014| [qcom_qcm2290_DTS_加载流程](docs/0014_qcom_qcm2290_DTS_加载流程.md) | QCM2290平台dts加载流程分析。
0013| [kernel如何通过DTB文件进行设备的注册](docs/0013_kernel如何通过DTB文件进行设备的注册.md) | 本文将以系统开机的执行顺序简要分析kernel如何通过DTB文件进行设备的注册。
0012| [Makefile学习](docs/0012_Makefile学习.md) | 学会编写Makefile，不仅仅有益于你在Linux下编写大型工程。同时也能帮助你理解编译原理。远离IDE，了解编译过程。
0011| [linux并发机制学习](docs/0011_linux并发机制学习.md) | 内核模块引用CONFIG实例
0010| [如何在驱动中引用CONFIG定义](docs/0010_如何在驱动中引用CONFIG定义.md) | 内核模块引用CONFIG实例
0009| [module_param()手动传递变量](docs/0009_module_param()手动传递变量.md) | 内核模块module_param传参实例，并在sysfs目录下实操
0008| [wakeup唤醒源使用实例](docs/0008_wakeup唤醒源使用实例.md) | notify实例
0007| [pinctrl原理与基础](docs/0007_pinctrl原理与基础.md) | 以高通平台为例，分析一下Pinctrl 子系统，有助于dts gpio配置理解。
0006| [make_htmldocs生成kernel_html_API文档](docs/0006_make_htmldocs生成kernel_html_API文档.md) | 一般自己从源码编译好了Linux内核镜像文件后，如果要进一步进行内核程序开发，此时需要有一份API文档来协助指导，而内核的编译命令中有相应的编译安装命令，本文将给予说明。
0005| [内核编译指令](docs/0005_内核编译指令.md) | 一般自己从源码编译好了Linux内核镜像文件后，如果要进一步进行内核程序开发，此时需要有一份API文档来协助指导，而内核的编译命令中有相应的编译安装命令，本文将给予说明。
0004| [DTS基础知识学习](docs/0004_DTS基础知识学习.md) | 整理一下DTS语法，怕好久不用忘记了。
0003| [qcom_mtk多机型DTB兼容方案](docs/0003_qcom_mtk多机型DTB兼容方案.md) | 很多 SoC 供应商和 ODM 都支持在一台设备上使用多个 DT，从而使一个映像能够为多个 SKU/配置提供支持。
0002| [kernel如何上报事件给应用层](docs/0002_kernel如何上报事件给应用层.md) | 本文主要讲解kernel层如何上传事件给到用户空间的几种方法。
0001| [notify实例](docs/0001_notify实例.md) | notify实例
