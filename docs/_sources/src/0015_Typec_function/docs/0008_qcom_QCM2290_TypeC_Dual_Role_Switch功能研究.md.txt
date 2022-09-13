# 概述

qcom qcm2290 验证普通TypeC接拓展坞作为host功能，插入外电至拓展坞后，外电反向给拓展坞及整机供电验证。


# 分析过程

由于typec的data role和power role是分开控制的，讲道理，当设备data role为host时，power role是可以作为sink的，也就是被供电。

## A6650接拓展坞

很奇怪，当插入外电至拓展坞后，整机typec消息由host变成device，重点是后面typec发送了`PR_SWAP`信号，将power_role设置为`PD_ROLE_SINK`状态，紧接着会收到`SINK_VBUS`消息，将charger vbus关闭，由拓展坞供电。

* [a6650拓展坞接U盘插外电.txt](log/a6650拓展坞接U盘插外电.txt)
```shell
console:/ # [  309.766809] pd_tcp_notifier_call event = SOURCE_VBUS
[  309.771997] pd_tcp_notifier_call source vbus 5000mV
[  309.777510] pd_tcp_notifier_call - source vbus 5v output
[  309.790970] pd_tcp_notifier_call event = TYPEC_STATE
[  309.796230] tcpc_notifier_call, old_state = UNATTACHED, new_state = ATTACHED_SRC
[  309.803767] pd_tcp_notifier_call OTG plug in, polarity = 0
[  309.818793] msm-usb-ssphy-qmp 1615000.ssphy: USB QMP PHY: Update TYPEC CTRL(2)
[  310.293210] pd_tcp_notifier_call event = SOURCE_VBUS
[  310.298317] pd_tcp_notifier_call source vbus 5000mV
[  310.303341] pd_tcp_notifier_call - source vbus 5v output
[  310.361099] pd_tcp_notifier_call event = PD_STATE
[  310.366126] PAX_CHG: psy_charger_set_property: prop:122 1
[  310.371683] PAX_CHG: pd_status:1
[  310.375332] PAX_CHG: _wake_up_charger:
[  310.379576] PAX_CHG: psy_charger_set_property: prop:124 0
[  310.379696] PAX_CHG: pax_is_charger_on chr_type = [Unknown] last_chr_type = [Unknown]
[  310.385399] PAX_CHG: set pd_charging_current_max:0 ma
[  310.398186] PAX_CHG: _wake_up_charger:
[  310.402151] PAX_CHG: pax_is_charger_on chr_type = [Unknown] last_chr_type = [Unknown]
[  310.797103] Battery: [ status:Discharging, health:Good, present:1, tech:Li-ion, capcity:24,cap_rm:1246 mah, vol:3740 mv, temp:29, curr:-535 ma, ui_soc:24, notify_code: 0 ]
[  318.972125] Battery: [ status:Discharging, health:Good, present:1, tech:Li-ion, capcity:24,cap_rm:1244 mah, vol:3677 mv, temp:29, curr:-924 ma, ui_soc:24, notify_code: 0 ]
[  321.225755] ext_spk_switch_put: set Ext_Spk_Switch val 1
[  321.250355] msm_pcm_chmap_ctl_put: substream ref_count:0 invalid
[  321.284723] send_afe_cal_type: No cal sent for cal_index 0, port_id = 0xb030! ret -22
[  324.745671] ext_spk_switch_put: set Ext_Spk_Switch val 0
[  325.596391] pd_tcp_notifier_call event = SOURCE_VBUS
[  325.601705] pd_tcp_notifier_call source vbus 0mV
[  325.606558] pd_tcp_notifier_call - source vbus 0v output
[  325.612781] pd_tcp_notifier_call event = EXT_DISCHARGE
[  325.645460] pd_tcp_notifier_call event = EXT_DISCHARGE
[  325.651091] pd_tcp_notifier_call event = TYPEC_STATE
[  325.656575] tcpc_notifier_call, old_state = ATTACHED_SRC, new_state = ATTACHED_SNK //不做任何动作
[  325.664514] pd_tcp_notifier_call event = PR_SWAP //这里是重点
[  325.669254] mp2721_charger_irq_workfunc:mp2721 irq
[  325.674384] PAX_CHG: psy_charger_set_property: prop:122 10
[  325.679970] PAX_CHG: pd_status:10
[  325.683525] PAX_CHG: _wake_up_charger:
[  325.687509] PAX_CHG: pax_is_charger_on chr_type = [Unknown] last_chr_type = [Unknown]
[  325.981498] pd_tcp_notifier_call event = SINK_VBUS
[  325.986766] charger soc:charger: pd_tcp_notifier_call sink vbus 5000mV 500mA type(0x84)
[  325.995013] pd_tcp_notifier_call - sink vbus
```

插电前role状态：
```log
console:/sys/class/typec/port0 # cat power_role
[source] sink
console:/sys/class/typec/port0 # cat data_role
[host] device
```

插电前role状态，可见power role变成了sink：
```
console:/sys/class/typec/port0 # cat data_role
[host] device
console:/sys/class/typec/port0 # cat power_role
source [sink]
```

## A800接拓展坞
* [A800拓展坞接U盘插外电.txt](log/A800拓展坞接U盘插外电.txt)

首先是接入拓展坞后，usb切换成host：
```log
<4>[  280.357829] <  280.357>TCPC-TYPEC:** TryWait.SRC
<5>[  280.369567]  (4)[238:tcpc_timer_type][MUSB]otg_tcp_notifier_call 334: source vbus = 5000mv
<5>[  280.369580]  (4)[238:tcpc_timer_type][MUSB]mt_usb_vbus_on 317: vbus_on
<5>[  280.369589]  (4)[238:tcpc_timer_type][MUSB]issue_vbus_work 309: issue work, ops<1>, delay<0>
<5>[  280.369624]  (6)[255:kworker/u16:6][MUSB]_set_vbus 157: op<1>, status<0>
<5>[  280.369635]  (6)[255:kworker/u16:6]PAX_CHG: bq25790_enable_otg: ==bq25790_enable_otg, en=1
<4>[  280.375558]  (6)[255:kworker/u16:6]==bq25790_set_otg_current_limit, uA=1500000
<5>[  280.381135]  (7)[240:type_c_port0]PAX_CHG: pd_tcp_notifier_call: PD charger event:14 5
<5>[  280.381146]  (7)[240:type_c_port0][MUSB]otg_tcp_notifier_call 343: TCP_NOTIFY_TYPEC_STATE, old_state=0, new_state=2
<5>[  280.381153]  (7)[240:type_c_port0][MUSB]otg_tcp_notifier_call 346: OTG Plug in
<5>[  280.381159]  (7)[240:type_c_port0][MUSB]mt_usb_host_connect 268: connect
<5>[  280.381167]  (7)[240:type_c_port0][MUSB]issue_host_work 256: issue work, ops<2>, delay<0>, on_st<1>
<5>[  280.381195]  (6)[255:kworker/u16:6][MUSB]do_host_work 561: PASS,init_done:1,is_ready:1,inited:0, TO:0
<5>[  280.381207]  (6)[255:kworker/u16:6][MUSB]do_host_work 575: work start, is_host=0, host_on=1
<6>[  280.383843]  (7)[61:pd_dbg_info]///PD dbg info 453d
<4>[  280.383864]  (7)[61:pd_dbg_info]<  280.358>TCPC-TYPEC:[CC_Alert] 2/1
<4>[  280.383864] <  280.368>TCPC-TYPEC:[CC_Change] 2/1
<4>[  280.383864] <  280.368>TCPC-TYPEC:** Attached.SRC
<4>[  280.383864] <  280.368>T
<4>[  280.383872]  (7)[61:pd_dbg_info]CPC-TYPEC:wait_ps=SRC_VSafe5V
<4>[  280.383872] <  280.370>TCPC-TYPEC:[CC_Alert] 2/0
<4>[  280.383872] <  280.379>TCPC-TCPC:ps_change=0
<4>[  280.383872] <  280.381>TCPC-TCPC:ps_c
<4>[  280.383887]  (7)[61:pd_dbg_info]hange=2
<4>[  280.383887] <  280.381>TCPC-TYPEC:wait_ps=Disable
<4>[  280.383887] <  280.381>TCPC-TYPEC:Attached-> SOURCE
<4>[  280.383887] <  280.381>TCPC-TCPC:usb_port_attached
<4>[  280.383889]  (7)[61:pd_dbg_info]
<4>[  280.383889] <  280.381>TCPC-DC> dc_dfp_none
<4>[  280.383889] <  280.381>TCPC-PE:PD-> SRC_START
<6>[  280.391808]  (0)[237:kworker/0:1]==[26,26,26] temp=26
<6>[  280.411862]  (7)[61:pd_dbg_info]///PD dbg info 193d
<4>[  280.411874]  (7)[61:pd_dbg_info]<  280.401>TCPC-PE-EVT:timer
<4>[  280.411874] <  280.401>TCPC-PE-EVT:tcp_event(disc_cable), 28
<4>[  280.411874] <  280.401>TCPC-PE:VDM-> SRC_VDM_ID_REQ
<4>[  280.411874] <  280.
<4>[  280.411878]  (7)[61:pd_dbg_info]407>TCPC-PE-EVT:tx_err
<4>[  280.411878] <  280.407>TCPC-PE:VDM-> SRC_VDM_ID_NAK
<7>[  280.419840]  (0)[255:kworker/u16:6]mtk_audio_request_sram(), user = 000000002293891c, length = 35200, count = 0
<7>[  280.419845]  (0)[255:kworker/u16:6]AudDrv_Clk_On, Aud_AFE_Clk_cntr:0
<6>[  280.419861]  (0)[255:kworker/u16:6]AudDrv_AUDINTBUS_Sel(), parentidx = 1, CLK_CFG_4 = 0x03838101
<4>[  280.419885]  (0)[255:kworker/u16:6]CheckSramAvail SramBlock->mValid == 0 i = 6
<6>[  280.419891]  (0)[255:kworker/u16:6]CheckSramAvail MaxSramSize = 24576 mSramLength = 35200 mSramBlockidx = 0 mSramBlocknum= 6
<4>[  280.419896]  (0)[255:kworker/u16:6]mtk_audio_request_sram(), allocate sram fail, ret -12
<7>[  280.419902]  (0)[255:kworker/u16:6]!! AudDrv_Clk_Off, Aud_AFE_Clk_cntr:1
<6>[  280.419918]  (0)[255:kworker/u16:6]AudDrv_AUDINTBUS_Sel(), parentidx = 0, CLK_CFG_4 = 0x03838100
<5>[  280.419926]  (0)[255:kworker/u16:6][MUSB]gpd_switch_to_sram 346: NO MEMORY!!!
<5>[  280.419932]  (0)[255:kworker/u16:6][MUSB]do_host_work 588: gpd_switch_to_sram, ret<-12>
<4>[  280.419971] -(0)[255:kworker/u16:6]usb_6765_dpidle_request: 3 callbacks suppressed
<5>[  280.419974] -(0)[255:kworker/u16:6][MUSB]usb_6765_dpidle_request 104: USB_DPIDLE_FORBIDDEN, skip_cnt<3>
<4>[  280.419988] -(0)[255:kworker/u16:6]usb_6765_dpidle_request: 3 callbacks suppressed
<5>[  280.419991] -(0)[255:kworker/u16:6][MUSB]usb_6765_dpidle_request 97: USB_DPIDLE_ALLOWED, skip_cnt<3>
<5>[  280.420129]  (0)[255:kworker/u16:6][MUSB]mt_usb_set_vbus 192: is_on<1>, control<0>
<5>[  280.420136]  (0)[255:kworker/u16:6][MUSB]mt_usb_enable 346: begin <0,0>,<3,2,2,2>
<5>[  280.431040]  (0)[255:kworker/u16:6][MUSB]set_usb_phy_mode 516: force PHY to mode 1, 0x6c=3f2f
<5>[  280.431063]  (0)[255:kworker/u16:6][MUSB]usb_phy_recover 766: apply efuse setting, RG_USB20_INTR_CAL=0x15
<5>[  280.431071]  (0)[255:kworker/u16:6][MUSB]usb_phy_recover 776: usb recovery success
<5>[  280.431077]  (0)[255:kworker/u16:6][MUSB]mt_usb_enable 379: end, <3,2,3,2>
<6>[  280.439817]  (7)[61:pd_dbg_info]///PD dbg info 133d
<4>[  280.439826]  (7)[61:pd_dbg_info]<  280.427>TCPC-PE-EVT:timer
<4>[  280.439826] <  280.427>TCPC-PE:PD-> SRC_SEND_CAP
<4>[  280.439826] <  280.432>TCPC-PE-EVT:tx_err
<4>[  280.439826] <  280.432>TCPC-PE:PD-> SRC_D
<4>[  280.439830]  (7)[61:pd_dbg_info]ISC
<6>[  280.468708]  (7)[61:pd_dbg_info]///PD dbg info 80d
<4>[  280.468716]  (7)[61:pd_dbg_info]<  280.468>TCPC-PE-EVT:timer
<4>[  280.468716] <  280.468>TCPC-PE-EVT:tcp_event(disc_cable), 28
<6>[  280.495809]  (7)[61:pd_dbg_info]///PD dbg info 175d
<4>[  280.495819]  (7)[61:pd_dbg_info]<  280.468>TCPC-PE:VDM-> SRC_VDM_ID_REQ
<4>[  280.495819] <  280.474>TCPC-PE-EVT:tx_err
<4>[  280.495819] <  280.474>TCPC-PE:VDM-> SRC_VDM_ID_NAK
<4>[  280.495819] <  280.474>TCPC
<4>[  280.495823]  (7)[61:pd_dbg_info]-PE:cable_rev=1
<4>[  280.495823] <  280.494>TCPC-PE-EVT:timer
<5>[  280.535810]  (7)[99:pmic_thread][PMIC] [PMIC_INT] Reg[0x91a]=0x40
<5>[  280.535818]  (0)[255:kworker/u16:6][MUSB]set_usb_phy_mode 516: force PHY to mode 0, 0x6c=3f11
<5>[  280.540827]  (0)[255:kworker/u16:6][MUSB]set_usb_phy_mode 516: force PHY to mode 2, 0x6c=3f2d
<5>[  280.540836]  (0)[255:kworker/u16:6][MUSB]musb_start 1340: start, is_host=1 is_active=0
<5>[  280.540843]  (0)[255:kworker/u16:6][MUSB]mt_usb_enable 346: begin <1,1>,<4,2,3,2>
<5>[  280.540852]  (0)[255:kworker/u16:6][MUSB]musb_start 1385: set ignore babble MUSB_ULPI_REG_DATA=88
<5>[  280.540857]  (0)[255:kworker/u16:6][MUSB]musb_start 1393: add softconn
<5>[  280.540863]  (0)[255:kworker/u16:6][MUSB]do_host_work 670: work end, is_host=1
<6>[  280.552910]  (4)[61:pd_dbg_info]///PD dbg info 68d
<4>[  280.552919]  (4)[61:pd_dbg_info]<  280.552>TCPC-PE-EVT:timer
<4>[  280.552919] <  280.552>TCPC-PE:PD-> SRC_SEND_CAP
<6>[  280.579817]  (4)[61:pd_dbg_info]///PD dbg info 65d
<4>[  280.579828]  (4)[61:pd_dbg_info]<  280.558>TCPC-PE-EVT:tx_err
<4>[  280.579828] <  280.558>TCPC-PE:PD-> SRC_DISC
<6>[  280.607815]  (4)[61:pd_dbg_info]///PD dbg info 193d
<4>[  280.607827]  (4)[61:pd_dbg_info]<  280.594>TCPC-PE-EVT:timer
<4>[  280.607827] <  280.594>TCPC-PE-EVT:tcp_event(disc_cable), 28
<4>[  280.607827] <  280.594>TCPC-PE:VDM-> SRC_VDM_ID_REQ
<4>[  280.607827] <  280.
<4>[  280.607831]  (4)[61:pd_dbg_info]601>TCPC-PE-EVT:tx_err
<4>[  280.607831] <  280.601>TCPC-PE:VDM-> SRC_VDM_ID_NAK
<6>[  280.635832]  (7)[61:pd_dbg_info]///PD dbg info 30d
<4>[  280.635842]  (7)[61:pd_dbg_info]<  280.621>TCPC-PE-EVT:timer
<6>[  280.679235]  (7)[61:pd_dbg_info]///PD dbg info 68d
<4>[  280.679249]  (7)[61:pd_dbg_info]<  280.679>TCPC-PE-EVT:timer
<4>[  280.679249] <  280.679>TCPC-PE:PD-> SRC_SEND_CAP
<6>[  280.703815]  (7)[61:pd_dbg_info]///PD dbg info 65d
<4>[  280.703826]  (7)[61:pd_dbg_info]<  280.684>TCPC-PE-EVT:tx_err
<4>[  280.703826] <  280.684>TCPC-PE:PD-> SRC_DISC
<6>[  280.805005]  (4)[61:pd_dbg_info]///PD dbg info 68d
<4>[  280.805019]  (4)[61:pd_dbg_info]<  280.804>TCPC-PE-EVT:timer
<4>[  280.805019] <  280.804>TCPC-PE:PD-> SRC_SEND_CAP
<6>[  280.831814]  (4)[61:pd_dbg_info]///PD dbg info 305d
<4>[  280.831827]  (4)[61:pd_dbg_info]<  280.807>TCPC-PE-EVT:good_crc
<4>[  280.831827] <  280.811>TCPC-PE-EVT:request
<4>[  280.831827] <  280.811>TCPC-PE:PD-> SRC_NEG_CAP
<4>[  280.831827] <  280.811>TCPC-PE:pd_rev=
<4>[  280.831831]  (4)[61:pd_dbg_info]2
<4>[  280.831831] <  280.811>TCPC-DPM:RequestCap1
<4>[  280.831831] <  280.811>TCPC-DPM:CAP_MISMATCH
<4>[  280.831831] <  280.811>TCPC-PE-EVT:dpm_ack
<4>[  280.831831] <  280.811>TCPC-PE:PD-> SR
<4>[  280.831845]  (4)[61:pd_dbg_info]C_TRANS_SUPPLY
<4>[  280.831845] <  280.813>TCPC-PE-EVT:good_crc
<5>[  280.840139]  (5)[486:tcpc_event_type][MUSB]otg_tcp_notifier_call 334: source vbus = 5000mv
<5>[  280.840150]  (5)[486:tcpc_event_type][MUSB]mt_usb_vbus_on 317: vbus_on
<5>[  280.840158]  (5)[486:tcpc_event_type][MUSB]issue_vbus_work 309: issue work, ops<1>, delay<0>
<5>[  280.840208]  (1)[255:kworker/u16:6][MUSB]_set_vbus 157: op<1>, status<1>
<6>[  280.859813]  (4)[61:pd_dbg_info]///PD dbg info 177d
<4>[  280.859822]  (4)[61:pd_dbg_info]<  280.840>TCPC-PE-EVT:timer
<4>[  280.859822] <  280.840>TCPC-PE-EVT:vbus_stable
<4>[  280.859822] <  280.840>TCPC-PE:PD-> SRC_TRANS_SUPPLY2
<4>[  280.859822] <  280.841>TCPC-PE-
<4>[  280.859826]  (4)[61:pd_dbg_info]EVT:good_crc
<4>[  280.859826] <  280.841>TCPC-PE:PD-> SRC_READY
<6>[  280.887818]  (4)[61:pd_dbg_info]///PD dbg info 61d
<4>[  280.887828]  (4)[61:pd_dbg_info]<  280.866>TCPC-PE-EVT:timer
<4>[  280.887828] <  280.866>TCPC-PE:sink_tx_ng
<7>[  280.913760]  (5)[585:VSyncThread_0][DISP][disp_lowpower]_vdo_mode_leave_idle
<7>[  280.913870]  (5)[585:VSyncThread_0][DISP]primary display will switch from DECOUPLE to DIRECT_LINK
<7>[  280.914614]  (5)[585:VSyncThread_0][PWM]TOPCKGEN node exist
<7>[  280.914618]  (5)[585:VSyncThread_0][PWM]clk_req=0 clkid=24
<7>[  280.914630]  (5)[585:VSyncThread_0][PWM]PWM_MUX 3838180->3838180
<7>[  280.914786]  (5)[585:VSyncThread_0][DISP]primary display is DIRECT_LINK mode now
<6>[  280.915824]  (5)[61:pd_dbg_info]///PD dbg info 619d
<4>[  280.915838]  (5)[61:pd_dbg_info]<  280.891>TCPC-PE-EVT:timer
<4>[  280.915838] <  280.891>TCPC-PE-EVT:tcp_event(get_snk_cap), 11
<4>[  280.915838] <  280.891>TCPC-PE:PD-> SRC_GET_CAP
<4>[  280.915838] <  280.893
<4>[  280.915845]  (5)[61:pd_dbg_info]>TCPC-PE-EVT:good_crc
<4>[  280.915845] <  280.894>TCPC-PE-EVT:sink_cap
<4>[  280.915845] <  280.894>TCPC-PE:PD-> SRC_READY
<4>[  280.915845] <  280.894>TCPC-PE-EVT:tcp_event(disc
<4>[  280.915860]  (5)[61:pd_dbg_info]_id), 29
<4>[  280.915860] <  280.894>TCPC-PE:VDM-> D_UID_REQ
<4>[  280.915860] <  280.896>TCPC-PE-EVT:good_crc
<4>[  280.915860] <  280.905>TCPC-PE-EVT:vdm
<4>[  280.915860] <  280.905>TCPC-PE:Di
<4>[  280.915863]  (5)[61:pd_dbg_info]scoverID:ACK
<4>[  280.915863] <  280.905>TCPC-PE:VDM-> D_UID_A
<4>[  280.915863] <  280.905>TCPC-PE-EVT:dpm_ack
<4>[  280.915863] <  280.905>TCPC-PE:VDM-> SRC_READY
```

当接入外电，接收到`pr_swap`消息，关闭vbus，然后由host切换成device，挂载的U盘会断开一下，但是A6650是拔出断开。
```log
<6>[  296.399300]  (6)[61:pd_dbg_info]///PD dbg info 67d
<4>[  296.399309]  (6)[61:pd_dbg_info]<  296.399>TCPC-PE-EVT:pr_swap
<4>[  296.399309] <  296.399>TCPC-PE:PD-> P_SRC_EVA
<6>[  296.423808]  (6)[61:pd_dbg_info]///PD dbg info 144d
<5>[  296.427891]  (5)[486:tcpc_event_type][MUSB]otg_tcp_notifier_call 334: source vbus = 0mv
<5>[  296.427900]  (5)[486:tcpc_event_type][MUSB]mt_usb_vbus_off 323: vbus_off
<5>[  296.427908]  (5)[486:tcpc_event_type][MUSB]issue_vbus_work 309: issue work, ops<0>, delay<0>
<6>[  296.427933]  (5)[486:tcpc_event_type]pd_tcp_notifier_call ext discharge = 1
<5>[  296.427937]  (4)[1052:kworker/u16:17][MUSB]_set_vbus 157: op<0>, status<1>
<5>[  296.427943]  (4)[1052:kworker/u16:17]PAX_CHG: bq25790_enable_otg: ==bq25790_enable_otg, en=0
<5>[  296.462940]  (7)[99:pmic_thread][PMIC] [PMIC_INT] Reg[0x91a]=0x40
<5>[  296.543827]  (7)[1053:kworker/u16:18]USB_BOOST, <boost_work(), 323> id:2, end of work
<7>[  296.543832]  (5)[340:kworker/u16:8][cpu_ctrl_cfp]start_cfp
<5>[  296.543837]  (4)[255:kworker/u16:6]USB_BOOST, <boost_work(), 323> id:1, end of work
<7>[  296.543843]  (5)[340:kworker/u16:8][LT]reg_loading_tracking_sp start_cfp success
<5>[  296.543878]  (5)[340:kworker/u16:8][Power/PPM] Shrink 1 PPM logs from last 1523 ms!
<5>[  296.543886]  (5)[340:kworker/u16:8][Power/PPM] (0xa0)(2751207)(0)(0-7)(15)(0)(4)(4) (15)(0)(4)(4) 
<5>[  296.543895]  (5)[340:kworker/u16:8]USB_BOOST, <boost_work(), 323> id:0, end of work
<6>[  296.548074] -(4)[0:swapper/4]tick broadcast enter counter cpu: 44, 23, 37, 18, 34, 28, 27, 16, success counter cpu: 5, 8, 9, 3, 15, 1, 13, 1, fail counter cpu: 0, 0, 0, 0, 0, 0, 0, 0, interrupt counter cpu: 10, 9, 15, 9, 14, 11, 20, 1, o: 0-3,7, p: , f: , t:  296596000000, 301828147092, 301828138784, 297160654709, 296553142093, 296545216631, 296544815862, 296589992923,
<6>[  296.611968]  (7)[240:type_c_port0]pd_tcp_notifier_call ext discharge = 0
<5>[  296.612594]  (4)[486:tcpc_event_type]PAX_CHG: pd_tcp_notifier_call: PD charger event:14 5
<6>[  296.612624]  (4)[486:tcpc_event_type]pd_tcp_notifier_call Source_to_Sink
<5>[  296.612713]  (4)[486:tcpc_event_type][MUSB]otg_tcp_notifier_call 343: TCP_NOTIFY_TYPEC_STATE, old_state=2, new_state=1
<6>[  296.612731]  (5)[257:chgdet_thread]charger type: charger IN
<5>[  296.612817]  (4)[486:tcpc_event_type]PAX_CHG: pd_tcp_notifier_call: PD charger event:16 0
<4>[  296.616984]  (7)[61:pd_dbg_info]<  296.399>TCPC-PE-EVT:dpm_ack
<4>[  296.616984] <  296.399>TCPC-PE:PD-> P_SRC_ACCEPT
<4>[  296.616984] <  296.400>TCPC-PE-EVT:good_crc
<4>[  296.616984] <  296.400>TCPC-PE:PD-> P
<4>[  296.617012]  (7)[61:pd_dbg_info]_SRC_TRANS_OFF
<6>[  296.643880]  (7)[61:pd_dbg_info]///PD dbg info 460d
<4>[  296.643928]  (7)[61:pd_dbg_info]<  296.427>TCPC-PE-EVT:timer
<4>[  296.643928] <  296.430>TCPC-TCPC:ps_change=1
<4>[  296.643928] <  296.611>TCPC-TCPC:ps_change=0
<4>[  296.643928] <  296.612>TCPC-PE-EVT:vbus_0v
<4>[  296.643942]  (7)[61:pd_dbg_info]
<4>[  296.643942] <  296.612>TCPC-PE:PD-> P_SRC_ASSERT
<4>[  296.643942] <  296.612>TCPC-TYPEC:** Attached.SNK
<4>[  296.643942] <  296.612>TCPC-TYPEC:Attached-> SINK
<4>[  296.643942] <  296.614
<4>[  296.643995]  (7)[61:pd_dbg_info]>TCPC-PE-EVT:dpm_ack
<4>[  296.643995] <  296.614>TCPC-PE:PD-> P_SRC_WAIT_ON
<4>[  296.643995] <  296.616>TCPC-TYPEC:[CC_Alert] 0/0
<4>[  296.643995] <  296.616>TCPC-PE-EVT:good_c
<4>[  296.644003]  (7)[61:pd_dbg_info]rc
<4>[  296.644003] <  296.627>TCPC-TYPEC:[CC_Alert] 5/0
<4>[  296.644003] <  296.636>TCPC-TCPC:ps_change=2
<6>[  296.729450]  (6)[61:pd_dbg_info]///PD dbg info 66d
<4>[  296.729486]  (6)[61:pd_dbg_info]<  296.729>TCPC-PE-EVT:ps_rdy
<4>[  296.729486] <  296.729>TCPC-PE:PD-> SNK_START
<6>[  296.756014]  (4)[61:pd_dbg_info]///PD dbg info 73d
<4>[  296.756045]  (4)[61:pd_dbg_info]<  296.729>TCPC-PE-EVT:reset_prl_done
<4>[  296.756045] <  296.729>TCPC-PE:PD-> SNK_DISC
<5>[  296.790764]  (7)[99:pmic_thread][PMIC] [PMIC_INT] Reg[0x91a]=0x40
<5>[  296.859097]  (4)[240:type_c_port0]PAX_CHG: pd_tcp_notifier_call: PD charger event:19 3
<6>[  296.859113]  (5)[61:pd_dbg_info]///PD dbg info 37d
<4>[  296.859134]  (5)[61:pd_dbg_info]<  296.859>TCPC-TCPC:HardResetAlert
<5>[  296.859170]  (4)[240:type_c_port0]PAX_CHG: notify_adapter_event: notify_adapter_event 0 7
<5>[  296.859194]  (4)[240:type_c_port0]PAX_CHG: notify_adapter_event: hreset status = 0
<5>[  296.879961]  (5)[257:chgdet_thread][MUSB]Charger_Detect_Init 798: Charger_Detect_Init
<5>[  296.968507]  (4)[257:chgdet_thread][MUSB]Charger_Detect_Release 818: Charger_Detect_Release
<6>[  296.968577]  (4)[257:chgdet_thread]charger type: 3, Non-standard Charger
<6>[  296.968621]  (4)[257:chgdet_thread]charger type: chrdet_inform_psy_changed: online = 1, type = 3
<6>[  296.968658]  (4)[257:chgdet_thread]mt_charger_set_property
<6>[  296.968691]  (4)[257:chgdet_thread]mt_charger_set_property
<6>[  296.968729]  (4)[257:chgdet_thread]dump_charger_name: charger type: 3, Non-standard Charger
<5>[  296.969046] -(5)[0:swapper/5][MUSB]musb_host_rx 2246: end 3 RX proto error,rxtoggle=0x30
<5>[  296.969124]  (7)[340:kworker/u16:8]PAX_CHG: mtk_charger_int_handler: mtk_charger_int_handler
<5>[  296.969185]  (7)[340:kworker/u16:8]PAX_CHG: mtk_charger_int_handler: wake_up_charger
<5>[  296.969336]  (7)[340:kworker/u16:8]FG daemon is disabled
<5>[  296.969458] -(5)[1745:batterywarning][MUSB]musb_host_rx 2246: end 3 RX proto error,rxtoggle=0x30
<5>[  296.969830] -(5)[476:health@2.0-serv][MUSB]musb_stage0_irq 1208: musb_stage0_irq:1208 MUSB_INTR_RESET (a_host)
<5>[  296.969879] -(5)[476:health@2.0-serv][MUSB]musb_stage0_irq 1217: Babble
<5>[  296.970009] -(6)[394:vold][MUSB]musb_stage0_irq 1208: musb_stage0_irq:1208 MUSB_INTR_RESET (a_host)
<5>[  296.970060] -(6)[394:vold][MUSB]musb_stage0_irq 1217: Babble
<5>[  296.970128] -(6)[394:vold][MUSB]musb_stage0_irq 1208: musb_stage0_irq:1208 MUSB_INTR_RESET (a_host)
<5>[  296.970165] -(6)[394:vold][MUSB]musb_stage0_irq 1217: Babble
<5>[  296.970233] -(5)[476:health@2.0-serv][MUSB]musb_stage0_irq 1208: musb_stage0_irq:1208 MUSB_INTR_RESET (a_host)
<5>[  296.970281] -(5)[476:health@2.0-serv][MUSB]musb_stage0_irq 1217: Babble
<5>[  296.970477] -(6)[0:swapper/6][MUSB]musb_stage0_irq 1208: musb_stage0_irq:1208 MUSB_INTR_RESET (a_host)
<5>[  296.970524] -(6)[0:swapper/6][MUSB]musb_stage0_irq 1217: Babble
<5>[  296.970596] -(5)[476:health@2.0-serv][MUSB]musb_stage0_irq 1208: musb_stage0_irq:1208 MUSB_INTR_RESET (a_host)
<5>[  296.970641] -(5)[476:health@2.0-serv][MUSB]musb_stage0_irq 1217: Babble
<5>[  296.970707] -(5)[476:health@2.0-serv][MUSB]musb_stage0_irq 1155: DISCONNECT (a_host) as Host, devctl 19
<5>[  296.970746] -(5)[476:health@2.0-serv]QMU_WARN,<musb_disable_q_all 333>, disable_q_all
<5>[  296.970814] -(5)[476:health@2.0-serv][MUSB]musb_h_pre_disable 3351: disable all endpoints
<5>[  296.970871] -(5)[476:health@2.0-serv][MUSB]musb_h_disable 3270: qh:000000009fe3321e, is_in:80, epnum:2, hep<00000000a695d900>
<5>[  296.970922] -(5)[476:health@2.0-serv][MUSB]musb_h_disable 3306: list_empty<0>, urb<00000000251f6130,0,-108>
<5>[  296.970999] -(5)[476:health@2.0-serv][MUSB]musb_h_disable 3270: qh:00000000a4e74cf2, is_in:80, epnum:3, hep<000000009663637e>
<5>[  296.971048] -(5)[476:health@2.0-serv][MUSB]musb_h_disable 3306: list_empty<0>, urb<00000000a4139c76,0,-108>
<5>[  296.971112] -(5)[476:health@2.0-serv][MUSB]musb_h_disable 3270: qh:0000000032e0a348, is_in:80, epnum:4, hep<00000000a894d4d6>
<5>[  296.971161] -(5)[476:health@2.0-serv][MUSB]musb_h_disable 3306: list_empty<0>, urb<00000000cc0fbd88,0,-108>
<5>[  296.971220] -(5)[476:health@2.0-serv][MUSB]musb_root_disconnect 208: host disconnect (a_host)
<5>[  296.971265] -(5)[476:health@2.0-serv][MUSB]mt_usb_try_idle 288: a_wait_bcon inactive, for idle timer for 600 ms
<5>[  296.971314] -(5)[476:health@2.0-serv][MUSB]musb_stage0_irq 1208: musb_stage0_irq:1208 MUSB_INTR_RESET (a_wait_bcon)
<5>[  296.971355] -(5)[476:health@2.0-serv]QMU_WARN,<musb_disable_q_all 333>, disable_q_all
<5>[  296.972401] -(5)[264:kworker/5:2][MUSB]musb_hub_control 379: port status 00010100,devctl=0x19
<6>[  296.972892]  (5)[264:kworker/5:2]usb 1-1: USB disconnect, device number 2
<6>[  296.972973]  (5)[264:kworker/5:2]usb 1-1.1: USB disconnect, device number 3
<6>[  296.973870]  (5)[264:kworker/5:2]smsc95xx 1-1.1:1.0 eth0: unregister 'smsc95xx' usb-musb-hdrc-1.1, smsc95xx USB 2.0 Ethernet
<6>[  296.974051]  (5)[264:kworker/5:2]smsc95xx 1-1.1:1.0 eth0: hardware isn't capable of remote wakeup
<12>[  296.979459]  (6)[476:health@2.0-serv]healthd: battery l=68 v=7348 t=26.0 h=2 st=4 c=-191 fc=2327 cc=12 chg=a
<5>[  296.981980] -(6)[476:health@2.0-serv]alarmtimer_enqueue, 1951351421339
<6>[  296.982270]  (6)[61:pd_dbg_info]///PD dbg info 34d
<4>[  296.982315]  (6)[61:pd_dbg_info]<  296.979>TCPC-TCPC:ps_change=1
```

# 软件分析

* [0002_TypeC_PD驱动软件架构总结.md](docs/0015_TYPEC/docs/0002_TypeC_PD驱动软件架构总结.md)
参考上面的软件流程，我们知道普通typec接口处理role_swap函数就两个，分别是`typec_handle_role_swap_start`和`typec_handle_role_swap_stop`,`tcpc_typec_handle_timeout`是timer唤醒的线程处理函数，如下：
```C++
int tcpc_typec_handle_timeout(struct tcpc_device *tcpc, uint32_t timer_id)
{
#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	case TYPEC_RT_TIMER_ROLE_SWAP_START:
		typec_handle_role_swap_start(tcpc);
		break;

	case TYPEC_RT_TIMER_ROLE_SWAP_STOP:
		typec_handle_role_swap_stop(tcpc);
		break;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */
}
```

两个函数具体内容如下，`typec_handle_role_swap_start`中如果role变成Sink，只是将CC上电阻切为Rd，然后启动`TYPEC_RT_TIMER_ROLE_SWAP_STOP` timer，执行`typec_handle_role_swap_stop`来启动timer`TYPEC_TIMER_PDDEBOUNCE`防抖。
```C++
#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
static inline int typec_handle_role_swap_start(struct tcpc_device *tcpc)
{
	uint8_t role_swap = tcpc->typec_during_role_swap;

	if (role_swap == TYPEC_ROLE_SWAP_TO_SNK) {
		TYPEC_INFO("Role Swap to Sink\n");
		tcpci_set_cc(tcpc, TYPEC_CC_RD);
		tcpc_enable_timer(tcpc, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	} else if (role_swap == TYPEC_ROLE_SWAP_TO_SRC) {
		TYPEC_INFO("Role Swap to Source\n");
		tcpci_set_cc(tcpc, TYPEC_CC_RP);
		tcpc_enable_timer(tcpc, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	}

	return 0;
}

static inline int typec_handle_role_swap_stop(struct tcpc_device *tcpc)
{
	if (tcpc->typec_during_role_swap) {
		TYPEC_INFO("TypeC Role Swap Failed\n");
		tcpc->typec_during_role_swap = TYPEC_ROLE_SWAP_NONE;
		tcpc_enable_timer(tcpc, TYPEC_TIMER_PDDEBOUNCE);
	}

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */
```
# 结论

也就是说普通的swap并不支持power/data dual role独立切换，都是绑定死的，只有pd才支持该功能，具体PD如何处理的请参考`PD Typec CC处理流程`章节：

* [0002_TypeC_PD驱动软件架构总结.md](0002_TypeC_PD驱动软件架构总结.md)