# README

kernel驱动分析

## docs

NO.|文件名称|摘要
:--:|:--|:--
0019| [驱动模板](docs/0019_驱动模板.md) | Android kernel经常需要自己新写一个驱动，现在增加模板方便后续添加。
0018| [container_of的用法](docs/0018_container_of的用法.md) | Android kernel经常碰到调整驱动加载顺序，详细看一下。
0017| [Android_module_platform_driver与module_init区别](docs/0017_Android_module_platform_driver与module_init区别.md) | Android kernel经常碰到调整驱动加载顺序，详细看一下。
0016| [Android如何调整内核模块加载顺序](docs/0016_Android如何调整内核模块加载顺序.md) | Android kernel经常碰到调整驱动加载顺序，详细看一下。
0015| [Android上层调用底层不同方式实现](docs/0015_Android上层调用底层不同方式实现.md) | Android framework开发人员经常需要调用到底层的接口和底层通信，比如binder调用，下面介绍几种常用的方式。
0014| [qcom_Android11_DTS_加载流程](docs/0014_qcom_Android11_DTS_加载流程.md) | mt6762平台dts加载流程分析。
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
0001| [kernel_device_dirver](docs/0001_kernel_device_dirver/README.md) | 总结一下linux平台各种设备驱动。
