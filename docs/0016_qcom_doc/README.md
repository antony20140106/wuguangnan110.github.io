# README

kernel驱动分析

## docs

NO.|文件名称|摘要
:--:|:--|:--
0007| [qcom_ABL_运行流程](docs/0007_qcom_ABL_运行流程.md) | 在前面[0005_qcom_XBL运行流程.md](0005_qcom_XBL运行流程.md)中，我们分析到，在BdsEntry() 中会调用 LaunchDefaultBDSApps() 回载默认app，包含ABL。
0006| [qcom_XBL_USB运行流程](docs/0006_qcom_XBL_USB运行流程.md) | 目前usb在fastboot模式下无法识别，跟进一下代码看看什么原因。
0005| [qcom_XBL运行流程](docs/0005_qcom_XBL运行流程.md) | 之前学习的lk阶段点亮LCD的流程算是比较经典，但是高通已经推出了很多种基于UEFI方案的启动架构。所以需要对这块比较新的技术进行学习。在学习之前，有必要了解一下高通UEFI启动流程。
0004| [qcom_qcm2290_kernel编译流程](docs/0004_qcom_qcm2290_kernel编译流程.md) | 将高通A6650平台的kernel编译流程分析一下。
0003| [qcom_qcm2290_dts兼容调试](docs/0003_qcom_qcm2290_dts兼容调试.md) | 将高通A6650平台的dts整合在一起。
0002| [梦源逻辑分析仪_示波器使用说明--基于DSview工具](docs/0002_梦源逻辑分析仪_示波器使用说明--基于DSview工具.md) | 简单记录下梦源逻辑分析仪和示波器简单实用方法。
0001| [mark](docs/0001_mark.md) | 简单记录下调试记录
