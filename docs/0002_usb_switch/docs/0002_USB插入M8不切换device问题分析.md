# 概述

M8项目目前的机器，不接皮套，在电量较低（比如低于50%），单体M8连接PC USB，无法识别没切换device。

# 参考log

* [kernel_log_2022_0509_201153.curf.utc](refers/kernel_log_2022_0509_201153.curf.utc)

# 分析过程

* 1.首先发现插入usb过程中，typec cc检测`Attached-> SINK`，包括充电都是没问题的：

```log
行 43: 05-09 20:11:37.849534 <5>[11494.196315]  (4)[13047:kworker/u16:5][MUSB]otg_tcp_notifier_call 497: TCP_NOTIFY_DR_SWAP, new role=0
行 137: 05-09 20:11:41.719300 <6>[11498.066081] -(6)[243:irq/28-mt6370_p]pogo_dev_state: 1  //pogo在线
行 149: 05-09 20:11:41.719784 <7>[11498.066565] -(6)[243:irq/28-mt6370_p][pax_gpio]: SET_POGO_DEV_STATE: send pogo dev input event 1
行 153: 05-09 20:11:41.720672 <5>[11498.067453]  (7)[112:pd_dbg_info]<11498.067>TCPC-TYPEC:[CC_Alert] 0/0
行 198: 05-09 20:11:41.749180 <5>[11498.095961]  (7)[112:pd_dbg_info]<11498.067>TCPC-TCPC:[Warning] ps_changed 2 ->1
行 200: 05-09 20:11:41.749180 <5>[11498.095961] <11498.067>TCPC-TYPEC:Detach_CC (HardReset), compatible apple TA
行 216: 05-09 20:11:41.749225 <5>[11498.096006] <11498.090>TCPC-TCPC:bat_update_work_func battery update soc
行 218: 05-09 20:11:41.749271 <5>[11498.096052] <11498.090>TCPC-TCPC:bat_update_work_func Battery Idle
行 287: 05-09 20:11:41.921457 <5>[11498.268238]  (7)[112:pd_dbg_info]<11498.267>TCPC-TYPEC:[CC_Change] 0/0
行 289: 05-09 20:11:41.921457 <5>[11498.268238] <11498.268>TCPC-TYPEC:** UnattachWait.PE
行 291: 05-09 20:11:41.921457 <5>[11498.268238] <11498.268>TCPC-TYPEC:Attached-> NULL
行 294: 05-09 20:11:41.921471 <5>[11498.268252]  (7)[112:pd_dbg_info]68>TCPC-TCPC:usb_port_detached
行 298: 05-09 20:11:41.921999 <5>[11498.268780]  (7)[11853:kworker/u16:10][MUSB]otg_tcp_notifier_call 404: TCP_NOTIFY_TYPEC_STATE, old_state=1, new_state=0
行 335: 05-09 20:11:41.949100 <5>[11498.295881] <11498.273>TCPC-TYPEC:** Unattached
行 338: 05-09 20:11:41.949104 <5>[11498.295885] <11498.275>TCPC-TYPEC:[CC_Alert] 15/15
行 340: 05-09 20:11:41.949104 <5>[11498.295885] <11498.279>TCPC-TCPC:bat_update_work_func battery update soc = 46
行 364: 05-09 20:11:41.977132 <5>[11498.323913]  (7)[112:pd_dbg_info]<11498.297>TCPC-TCPC:bat_update_work_func battery update soc = 46
行 365: 05-09 20:11:41.977132 <5>[11498.323913] <11498.297>TCPC-TCPC:bat_update_work_func Battery Discharging
行 383: 05-09 20:11:41.981960 <5>[11498.328741]  (7)[11853:kworker/u16:10][MUSB]otg_tcp_notifier_call 395: source vbus = 0mv
行 463: 05-09 20:11:42.027694 <5>[11498.374475]  (4)[112:pd_dbg_info]<11498.374>TCPC-TCPC:bat_update_work_func battery update soc = 46
行 464: 05-09 20:11:42.027694 <5>[11498.374475] <11498.374>TCPC-TCPC:bat_update_work_func Battery Discharging
行 541: 05-09 20:11:42.924184 <6>[11499.270965] -(0)[0:swapper/0]pogo_dev_state: 1
行 547: 05-09 20:11:42.924441 <7>[11499.271222] -(0)[0:swapper/0][pax_gpio]: SET_POGO_DEV_STATE: send pogo dev input event 1
行 1941: 05-09 20:11:47.554548 <5>[11503.901329]  (5)[112:pd_dbg_info]<11503.900>TCPC-TCPC:ps_change=2
行 1945: 05-09 20:11:47.581210 <5>[11503.927991]  (5)[112:pd_dbg_info]<11503.901>TCPC-TYPEC:norp_src=1
行 1949: 05-09 20:11:47.619094 <5>[11503.965875]  (5)[112:pd_dbg_info]<11503.965>TCPC-TCPC:Wakeup
行 1951: 05-09 20:11:47.619094 <5>[11503.965875] <11503.965>TCPC-TCPC:wakeup_timer
行 1954: 05-09 20:11:47.645263 <5>[11503.992044]  (4)[112:pd_dbg_info]<11503.965>TCPC-TYPEC:[CC_Alert] 0/5
行 1956: 05-09 20:11:47.645263 <5>[11503.992044] <11503.966>TCPC-TYPEC:** AttachWait.SNK
行 1958: 05-09 20:11:47.645263 <5>[11503.992044] <11503.966>TCPC-TCPC:ps_change=2
行 1961: 05-09 20:11:47.740054 <5>[11504.086835]  (4)[112:pd_dbg_info]<11504.086>TCPC-TYPEC:[CC_Change] 0/5
行 1963: 05-09 20:11:47.740054 <5>[11504.086835] <11504.086>TCPC-TYPEC:** Try.SRC
行 1966: 05-09 20:11:47.765213 <5>[11504.111994]  (4)[112:pd_dbg_info]<11504.088>TCPC-TYPEC:[CC_Alert] 0/0
行 1970: 05-09 20:11:47.841260 <5>[11504.188041]  (4)[112:pd_dbg_info]<11504.187>TCPC-TYPEC:[CC_Change] 0/0
行 1972: 05-09 20:11:47.841260 <5>[11504.188041] <11504.187>TCPC-TYPEC:** TryWait.SNK
行 1974: 05-09 20:11:47.841423 <5>[11504.188204]  (7)[11104:kworker/u16:0][MUSB]otg_tcp_notifier_call 395: source vbus = 0mv
行 1980: 05-09 20:11:47.869150 <5>[11504.215931]  (4)[112:pd_dbg_info]<11504.189>TCPC-TYPEC:[CC_Alert] 0/5
行 1983: 05-09 20:11:47.962730 <5>[11504.309511]  (7)[112:pd_dbg_info]<11504.309>TCPC-TYPEC:[CC_Change] 0/5
行 1985: 05-09 20:11:47.962730 <5>[11504.309511] <11504.309>TCPC-TYPEC:** Attached.SNK
行 1995: 05-09 20:11:47.964148 <5>[11504.310929]  (5)[10950:kworker/u16:1][MUSB]otg_tcp_notifier_call 404: TCP_NOTIFY_TYPEC_STATE, old_state=0, new_state=1 //准备切device
行 2058: 05-09 20:11:47.989186 <5>[11504.335967]  (7)[112:pd_dbg_info]<11504.310>TCPC-TYPEC:Attached-> SINK
行 2060: 05-09 20:11:47.989186 <5>[11504.335967] <11504.310>TCPC-TCPC:usb_port_attached
行 2062: 05-09 20:11:47.989186 <5>[11504.335967] <11504.320>TCPC-TCPC:bat_update_work_func battery
行 2064: 05-09 20:11:47.989202 <5>[11504.335983] <11504.320>TCPC-TCPC:bat_update_work_func Battery Charging
行 2092: 05-09 20:11:48.017225 <5>[11504.364006]  (7)[112:pd_dbg_info]<11504.346>TCPC-TCPC:bat_update_work_func battery update soc = 46
行 2093: 05-09 20:11:48.017225 <5>[11504.364006] <11504.346>TCPC-TCPC:bat_update_work_func Battery Charging
行 2095: 05-09 20:11:48.017237 <5>[11504.364018]  (7)[112:pd_dbg_info]504.360>TCPC-TYPEC:type=1, ret,chg_type=0,2, count=1
行 2203: 05-09 20:11:48.389444 <5>[11504.736225]  (4)[13047:kworker/u16:5][MUSB]otg_tcp_notifier_call 497: TCP_NOTIFY_DR_SWAP, new role=0
行 2460: 05-09 20:11:52.433388 <5>[11508.780169]  (6)[112:pd_dbg_info]<11508.780>TCPC-TCPC:bat_update_work_func battery update soc = 46
行 2461: 05-09 20:11:52.433388 <5>[11508.780169] <11508.780>TCPC-TCPC:bat_update_work_func Battery Charging
行 2610: 05-09 20:11:53.091889 <5>[11509.438670]  (0)[11333:kworker/u16:9][MUSB]otg_tcp_notifier_call 497: TCP_NOTIFY_DR_SWAP, new role=0
```

* usb20_host处理如下`TYPEC_UNATTACHED -> TYPEC_ATTACHED_SNK`：

```log
enum typec_attach_type {
	TYPEC_UNATTACHED = 0,
	TYPEC_ATTACHED_SNK,
	TYPEC_ATTACHED_SRC,
	TYPEC_ATTACHED_AUDIO,
	TYPEC_ATTACHED_DEBUG,	

05-09 20:11:47.964108 <6>[11504.310889]  (5)[10950:kworker/u16:1]pd_tcp_notifier_call USB Plug in, pol = 1
05-09 20:11:47.964148 <5>[11504.310929]  (5)[10950:kworker/u16:1][MUSB]otg_tcp_notifier_call 404: TCP_NOTIFY_TYPEC_STATE, old_state=0, new_state=1

根据代码：
else if (noti->typec_state.old_state == TYPEC_UNATTACHED &&
			noti->typec_state.new_state == TYPEC_ATTACHED_SNK) {
			if (!((mtk_musb->pogo_dev_detect_type == POGO_DETECT_BY_EXT_PIN) && (mtk_musb->pogo_dev_state == POGO_DEV_STATE_ONLINE))) {
				mt_usb_host_disconnect(0);
				mt_usb_connect();
			}

根据代码判断是POGO_DETECT_BY_EXT_PIN，也就是说pogo pin是由外部gpio中断检测的：
enum pogo_dev_detect_type {
	POGO_DETECT_UNKNOWN,
	POGO_DETECT_NONE,
	POGO_DETECT_BY_EXT_PIN,
	POGO_DETECT_BY_CC,
};
&usb {
        bootmode = <&chosen>;
        default_mode = <1>;
    pogo_dev_detect_type = <2>;
};



需要满足1个条件才切换host：
1.pogo不在线

但是根据以下log判断是在线状态(硬件有问题，实际是不在线)，所以没有执行断开host操作`mt_usb_host_disconnect`,
行 541: 05-09 20:11:42.924184 <6>[11499.270965] -(0)[0:swapper/0]pogo_dev_state: 1

正常切host应该有如下打印：
05-09 19:51:11.060304 <5>[10267.407064]  (7)[11263:kworker/u16:8][MUSB]otg_tcp_notifier_call 404: TCP_NOTIFY_TYPEC_STATE, old_state=0, new_state=1
05-09 19:51:11.060333 <5>[10267.407093]  (7)[11263:kworker/u16:8][MUSB]mt_usb_host_disconnect 325: disconnect
```

也就是说pogo pin插入了，底层就不切device模式，由应用层弹框作切换，但是应用层也没弹框，据了解，弹框需要满足几个条件，如下：

* 1.收到usb插入广播ACTION_USB_PORT_CHANGED usbCharge=1--UsbPortStatus{connected=true
* 2.接入pogo pin

## 结论

1.后续发现插入usb后，pogo pin not insert，所以app没弹框，继续测量check引脚，发现高电量check脚不接皮套2.4v，而低电量的只有1.12v，硬件回复是硬件这边改了阻值造成的，具体为什么会造成这样  我还得查查。

2.最终结论：

* 硬件之前为了修复盐雾试验问题(R15供电一直打开着)，加了R15硬件插入检测，加了三极管和各种旁路，增加了关机功耗。

* 低电量三极管无法导通，只有1.12v。