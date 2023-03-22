# README

总结一下linux平台各种设备驱动。

# docs

NO.|文件名称|摘要
:--:|:--|:--
0004| [linux_电源管理之regulator机制流程](docs/0004_linux_电源管理之regulator机制流程.md) | Regulator模块用于控制系统中某些设备的电压/电流供应。在嵌入式系统（尤其是手机）中，控制耗电量很重要，直接影响到电池的续航时间。所以，如果系统中某一个模块暂时不需要使用，就可以通过regulator关闭其电源供应；或者降低提供给该模块的电压、电流大小。
0003| [qcom_qcm2290_i2c子系统](docs/0003_qcom_qcm2290_i2c子系统.md) | linux i2c子系统学习。
0002| [linux_platform设备驱动](docs/0002_linux_platform设备驱动.md) | linux platform设备驱动介绍。
0001| [linux_input子系统](docs/0001_linux_input子系统.md) | 以qcm2290平台为例，linux input子系统介绍。
