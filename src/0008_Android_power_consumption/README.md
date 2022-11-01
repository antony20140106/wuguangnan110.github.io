# README

记录Android功耗问题分析。

## docs

NO.|文件名称|摘要
:--:|:--|:--
0007| [Android电源管理framwork休眠wakelock介绍](docs/0007_Android电源管理framwork休眠wakelock介绍.md) | 当启动一个应用程序的时候，它可以申请一个wake_lock唤醒锁，每当申请成功后都会在内核中注册一下（通知系统内核，现在已经有锁被申请，系统内核的wake_lock_store把它加入锁中），当应用程序在某种情况下释放wake_lock的时候，会注销之前所申请的wake_lock。特别要注意的是：只要是系统中有一个wake_lock的时候，系统此时都不能进行睡眠。
0006| [Android电源管理Power_Management_Interface](docs/0006_Android电源管理Power_Management_Interface.md) | Linux电源管理中，相当多的部分是在处理Hibernate、Suspend、Runtime PM等功能。而这些功能都基于一套相似的逻辑，即“Power management interface”。该Interface的代码实现于“include/linux/pm.h”、“drivers/base/power/main.c”等文件中。主要功能是：对下，定义Device PM相关的回调函数，让各个Driver实现；对上，实现统一的PM操作函数，供PM核心逻辑调用。
0005| [Android电源管理suspend和resume流程总结](docs/0005_Android电源管理suspend和resume流程总结.md) | Linux内核提供了三种Suspend: Freeze、Standby和STR(Suspend to RAM)，在用户空间向”/sys/power/state”文件分别写入”freeze”、”standby”和”mem”，即可触发它们。
0004| [qcom_qcm2290_功耗调试记录](docs/0004_qcom_qcm2290_功耗调试记录.md) | 记录一下qcm2290平台功耗调试记录。
0003| [Android_power_profile测量方法](docs/0003_Android_power_profile测量方法.md) | 介绍Android耗电统计使用的参数是如何测量的。
0002| [Android_batterystats软硬件电量统计](docs/0002_Android_batterystats软硬件电量统计.md) | 介绍Android耗电统计服务BatteryStats。
0001| [Esim休眠功耗问题分析](docs/0001_Esim休眠功耗问题分析.md) | Esim休眠功耗大问题分析。
