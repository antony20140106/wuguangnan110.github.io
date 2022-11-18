# BUCK和LDO介绍

硬件中BUCK和LDO介绍

# 参考

* [电子技术（三十六）——BUCK和LDO](https://zhuanlan.zhihu.com/p/565368371)

# 简介

BUCK电路：降压电路（就是输出电压小于输入电压）

BOOST电路：升压电路（输出电压大于输入电压）

CCM：电感电流连续工作模式

DCM：电感电流不连续工作模式

BCM：电感电流连续工作模式（周期结束时电感电流刚好降为0）

# 相关定义

BUCK：“A buck converter is a voltage step down and current step up converter.”――来自维基百科的解释。翻译成中文应该是：“BUCK转换器是一个通过降低电压来增加电流的转换器。”

BUCK是DC-DC中的一种。DC-DC主要有buck（降压）,boost（升压）,buck-boost（升降压）三种。

什么又是DC－DC呢？

DC-DC的意思是直流变（到）直流（不同直流电源值的转换），只要符合这个定义都可以叫DC-DC转换器，包括LDO。但是一般的说法是把直流变（到）直流由开关方式实现的器件叫DC-DC。

LDO：“A low-dropout or LDO regulator is a DC linear voltage regulator which can operate with a very small input–output differential voltage.” ――来自维基百科的解释。可以这样理解：“LDO是一个直流线性电压控制器，它运作在输入输出压差非常小时。”

# 两者区别

LDO的效率大约等于输出电压比输入电压，所以当输出电压和输入电压相差较大时，效率低。

而BUCK在重载时可以到96%，轻载80%以上。所以：

1、高输入电压（>5V)、高输入/输出压差时，宜用BUCK；反之，宜用LDO。

2、输出电流>2A时，宜用BUCK；2A以内时宜用LDO。尽管LDO有3A、5A、7.5A，甚至8A的，但必须压差低，散热条件好的情况下才能达到。

否则因自身功耗（压差x电流）大，升温快，易保护而关闭输出（特别在高温环境下使用时）。

3、LDO也有高输入电压型（28V、80V等），但其输出电流小，仅200mA左右。

4、BUCK的输出纹波及稳压性不如LDO好，因此像DSP、ARM、FPGA等内核电源（1.2V、1.5V、2.5V等）宜用LDO。当输入电压高（或输入/出压差大），且输出电流比较大时，可采用“BUCK+LDO”方案。

5、BUCK电路要用外部电感，体积较大，有些还要使用外部MOS管；LDO电路则很简单，仅需1只电容即可。

6、BUCK的转换效率比LDO高，只有在输入/出压差很低时（<500mV)，LDO的转换效率才能与BUCK接近，LDO的热温特性比BUCK差。