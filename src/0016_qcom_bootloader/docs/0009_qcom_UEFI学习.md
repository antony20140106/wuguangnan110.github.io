# README

高通UEFI学习。

# refer

* [UEFI原理与编程（二）：UEFI工程模块文件-标准应用程序工程模块](https://blog.csdn.net/sevensevensevenday/article/details/70789517)

# 模块(Module)和包(Package)

在EDK2环境下编程之前，先介绍EDK2的两个概念模块(Module)和包(Package).
　　“包”是一组模块及平台描述文件(.dsc文件)、包声明文件(.dec文件)则、组成的集合，多在以*pkg命名的文件夹中，一般也称这样的文件夹为一个包。
　　模块是UEFI系统的一个特色。模块(可执行文件，即.efi文件)像插件一样可以动态地加载到UEFI内核中。对应到源文件，EDK2中的每个工程模块由元数据文件(.inf)和源文件(有些情况也可以包含.efi文件)组成。
主要介绍3种应用程序模块、UEFI驱动模块和库模块。

![0009_0000.png](images/0009_0000.png)

# Protocol服务

UEFI 驱动程序使用Protocol服务来访问其他模块产生的Protocol接口。 UEFI 规范定义了一组引导服务来处理Protocol，
包含：
实现动态链接时使用Protocol。 对于静态链接使用库。 要实现任何想要动态使用的新服务，请使用Protocol。