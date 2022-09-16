# README

记录高通Android平台相关差异性功能，比如XBL/ABL软件流程等。

## docs

NO.|文件名称|摘要
:--:|:--|:--
0010| [qcom_qcm2290_SBL1运行流程](docs/0010_qcom_qcm2290_SBL1运行流程.md) | 高通qcm2290 PBL-SBL1流程分析。全称：Secondary boot loader,SBL1是XBL前运行的一个流程，今天就要分析一下开机前几秒钟起着关键作用的sbl1.
0009| [qcom_UEFI学习](docs/0009_qcom_UEFI学习.md) | 高通UEFI学习。
0008| [qcom_qcm2290_ABL_增加i2c接口](docs/0008_qcom_qcm2290_ABL_增加i2c接口.md) | ABL阶段有时需要读取某些i2c设备状态，但默认没有i2c驱动，需要添加。
0007| [qcom_qcm2290_ABL_运行流程](docs/0007_qcom_qcm2290_ABL_运行流程.md) | 在前面[0005_qcom_XBL运行流程.md](0005_qcom_XBL运行流程.md)中，我们分析到，在BdsEntry() 中会调用 LaunchDefaultBDSApps() 回载默认app，包含ABL。
0006| [qcom_qcm2290_XBL_USB运行流程](docs/0006_qcom_qcm2290_XBL_USB运行流程.md) | 目前usb在fastboot模式下无法识别，跟进一下代码看看什么原因。
0005| [qcom_qcm2290_XBL运行流程](docs/0005_qcom_qcm2290_XBL运行流程.md) | 之前学习的lk阶段点亮LCD的流程算是比较经典，但是高通已经推出了很多种基于UEFI方案的启动架构。所以需要对这块比较新的技术进行学习。在学习之前，有必要了解一下高通UEFI启动流程。
0004| [qcom_qcm2290_kernel编译流程](docs/0004_qcom_qcm2290_kernel编译流程.md) | 将高通A6650平台的kernel编译流程分析一下。
0003| [qcom_qcm2290_dts兼容调试](docs/0003_qcom_qcm2290_dts兼容调试.md) | 将高通A6650平台的dts整合在一起。
0002| [梦源逻辑分析仪_示波器使用说明--基于DSview工具](docs/0002_梦源逻辑分析仪_示波器使用说明--基于DSview工具.md) | 简单记录下梦源逻辑分析仪和示波器简单实用方法。
0001| [mark](docs/0001_mark.md) | 简单记录下调试记录
