# 概述

charger/typec相关名词、连接状态解释

## 翻译

Alternate Mode Adapter(AMA)：支持PD USB交替模式的设备，作为UFP存在

Alternate Mode Controller (AMC)：支持PD USB交替模式的主机，作为DFP存在

Augmented Power Data Object(APDO)：体现Source端的供电能力或者Sink的耗电能力，是一个数据对象

Atomic Message Sequence(AMS)：一个固定的信息序列，一般作为PE_SRC_Ready, PE_SNK_Ready or PE_CBL_Ready的开始或者结束

Binary Frequency Shift Keying (BFSK)：二进制频移键控

Biphase Mark Coding(BMC)：双相位标识编码，通过CC通信

Configuration Channel (CC)：配置通道，用于识别、控制等

Constant Voltage (CV)：恒定电压，不随负载变化而变化

Current Limit (CL)：电流限制

Device Policy Manager(DPM)：设备策略管理器

Downstream Facing Port(DFP)：下行端口，即为HOST或者HUB下行端口

Upstream Facing Port(UFP)：上行端口，即为Device或者HUB的上行端口

Dual-Role Data (DRD)：能作为DFP/UFP

Dual-Role Power (DRP)：能做为Sink/Source

End of Packet (EOP):结束包

IR Drop：在Sink和Source之间的电压降

Over-Current Protection(OCP)：过流保护

Over-Temperature Protection(OTP)：过温保护

Over-Voltage Protection(OVP)：过压保护

PD Power (PDP)：Source的功耗输出，由制造商在PDOs字段中展示

Power Data Object (PDO)：用来表示Source的输出能力和Sink的消耗能力的数据对象

Programmable Power Supply (PPS)：电源输出受程序控制

PSD：一种吃电但是没有数据的设备，如充电宝

SOP Packet：Start of Packet，所有的PD传输流程，都是以SOP Packet开始，SOP*代表SOP，SOP’，SOP’’

Standard ID(SID)：标准ID

Standard or Vendor ID(SVID)：标准或产商ID

System Policy Manager(SPM)：系统策略管理，运行在Host端。

VCONN Powered Accessory(VPA)：由VCONN供电的附件

VCONN Powered USB Device(VPD)：由VCONN供电的设备

Vendor Data Object (VDO)：产商特定信息数据对象

Vendor Defined Message(VDM)：产商定义信息

Vendor ID (VID)：产商ID