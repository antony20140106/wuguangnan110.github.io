# README

Android平台charger、BMS相关知识及调试整理。

## docs

NO.|文件名称|摘要
:--:|:--|:--
0032| [Android增加health_hidl接口注意事项](docs/0032_Android增加health_hidl接口注意事项.md) | 尝试增加healthd HIDL相关接口，发现还需要修改lsdump部分，研究一下。
0031| [qcom_charger充电状态上报时间优化](docs/0031_qcom_charger充电状态上报时间优化.md) | 发现有的charger ic插入充电后，充电状态变化很慢，看一下原理
0030| [charger_ic中的一些特性说明](docs/0030_charger_ic中的一些特性说明.md) | 记录一下充电IC一些硬件特性。
0029| [qcom_qcm2290_charger_SGM41528调试记录](docs/0029_qcom_qcm2290_charger_SGM41528调试记录.md) | 记录一下M92xx项目双节升压充电charger驱动调试。
0028| [qcom_qcm2290_bms移植过程](docs/0028_qcom_qcm2290_bms移植过程.md) | 记录一下移植过程。
0027| [charger_jeita限流功能介绍](docs/0027_charger_jeita限流功能介绍.md) | JEITA（Japan Electronics and Information Technology Industries Association）
0026| [Android充电相关代码简单总结](docs/0026_Android充电相关代码简单总结.md) | charger/gauge相关，从上层到底层软件简单做个总结。
0025| [qcom_qcm2290_qpnp_power_on驱动分析](docs/0025_qcom_qcm2290_qpnp_power_on驱动分析.md) | 高通qcm2290平台，目前有专门的开关机原因检测驱动，分析一下。
0024| [qcom_qcm2290_无电池启动功能开发](docs/0024_qcom_qcm2290_无电池启动功能开发.md) | 高通qcm2290平台，目前要求无电池启动功能，开发一下。
0023| [qcom_qcm2290_关机充电模式分析](docs/0023_qcom_qcm2290_关机充电模式分析.md) | 高通qcm2290代码拿过来，关机插入usb直接开机，应该进入关机充电。
0022| [android_charger充电阶段介绍](docs/0022_android_charger充电阶段介绍.md) | charger充电阶段介绍。
0021| [qcom_charger充电实验](docs/0021_qcom_charger充电实验.md) | 目前battery电量计已经调试完成，对充电进行一次测试
0020| [qcom_charger频繁进入中断死机](docs/0020_qcom_charger频繁进入中断死机.md) | 目前charger由于插电脑未限制充电电流，开机过程中插usb会频繁进入中断，而且容易导致死机。
0019| [qcom_qcm2290_bc1.2_usb主从切换原理分析](docs/0019_qcom_qcm2290_bc1.2_usb主从切换原理分析.md) | 高通qcm2290 bc1.2目前使用纯micro usb方式无法获取，看一下原理
0018| [qcom_BQ27Z746_电量计问题调试](docs/0018_qcom_BQ27Z746_电量计问题调试.md) | 该电量计用在A665x上面。
0017| [qcom_charger架构](docs/0017_qcom_charger架构.md) | 目前charger使用外置的mp2721，qcom pm4125模块功能全部关闭了，需要自己搭建charger架构。
0016| [qcom_qcm2290_typec分析](docs/0016_qcom_qcm2290_typec分析.md) | 高通qcm2290 typec架构理解。
0015| [power_supply子系统-基础概括](docs/0015_power_supply子系统-基础概括.md) | kernel中为了方便对battery的管理，专门提供了power supply framework
0014| [qcom_healthd软件流程分析](docs/0014_qcom_healthd软件流程分析.md) | 高通A665x healthd 守护进程分析。
0013| [qcom_qcm2290_charger_mp2712调试记录](docs/0013_qcom_qcm2290_charger_mp2712调试记录.md) | 高通A665x charger mp2721调试记录，该款ic不做bc1.2检测，没有adc采样功能，电流电压检测都将放到电池IC里面。
0011| [mt6762_charger_相关参数说明](docs/0011_mt6762_charger_相关参数说明.md) | mtk平台charger相关参数说明
0010| [mt6762_charger_相关调试经验](docs/0010_mt6762_charger_相关调试经验.md) | charger相关改动经验
0009| [charger_BMS驱动及整体架构简介](docs/0009_charger_BMS驱动及整体架构简介.md) | BMS功能介绍
0008| [charger_BMS功能调试](docs/0008_charger_BMS功能调试.md) | BMS功能介绍
0007| [charger相关英语名字解释](docs/0007_charger相关英语名字解释.md) | charger/typec相关名词、连接状态解释
0006| [mt6762开机默认设置host及switch切换](docs/0006_mt6762开机默认设置host及switch切换.md) | mt6762平台如何设置开机默认host功能
0005| [Android_power_supply介绍及uevent上报详解](docs/0005_Android_power_supply介绍及uevent上报详解.md) | mt6762平台psy uevent上报分析
0004| [mt6762_Android_OTG识别过程](docs/0004_mt6762_Android_OTG识别过程.md) | mt6762 otg检测过程
0003| [mt6762_Android11充电类型识别(BC1.2)](docs/0003_mt6762_Android11充电类型识别(BC1.2).md) | mtk平台bc1.2分析
0002| [mt6762_Android11_chager中断事件处理](docs/0002_mt6762_Android11_chager中断事件处理.md) | 跟进一下mt6370中断处理流程。
0001| [mt6762_Android11充电流程](docs/0001_mt6762_Android11充电流程.md) | 讲述MTK平台充电流程
