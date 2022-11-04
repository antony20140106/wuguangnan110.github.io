# 概述

BMS功能介绍

# 参考

* [0008_BMS相关介绍](refers/BMS相关介绍)

# 测试要求

![0008_0000.png](images/0008_0000.png)

# 68电量异常关闭充电

68%电量，bms服务关闭了充电，异常打印如下：
```log
11-02 10:08:01.317  1001  1001 D PaxBatteryManagerService: onReceive action = android.intent.action.BOOT_COMPLETED
11-02 10:08:03.495  1001  2114 D PaxBatteryManagerService: Wait ACTION_BOOT_COMPLETED
11-02 10:08:03.495  1001  2114 D PaxBatteryManagerService: PaxBatteryThread run1
11-02 10:08:03.496  1001  2114 D PaxBatteryManagerService: PaxBatteryManagerService init
11-02 10:08:03.521  1001  2114 D PaxBatteryManagerService: batterylog_db path = /data/system/batterylog.db
11-02 10:08:03.521  1001  2114 D PaxBatteryManagerService: mode = 0
11-02 10:08:03.521  1001  2114 D PaxBatteryManagerService: init initcurMode = 1
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: onReceive action = android.intent.action.BATTERY_CHANGED
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: onReceive bms_switch = 1
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: mode = 0
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: chargeState = 1
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: getInitcurMode = 0
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: batteryTermInfo.getCurmode() = 0
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: mBatteryLevel = 68
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: batteryTermInfo.getFullcharge() = 0
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: batteryTermInfo.getRecharge() = 0
11-02 10:08:03.527  1001  1001 D PaxBatteryManagerService: discharge
11-02 10:08:03.528  1001  1001 D PaxBatteryManagerService: setChargeState: 0
11-02 10:08:03.531  1001  1001 D PaxBatteryManagerService: setChargeState over
11-02 10:08:03.532  1001  1001 D PaxBatteryManagerService: disablePowerPath
11-02 10:08:03.558  1001  2114 D PaxBatteryManagerService: mBatteryManagerInternal.getPlugType() = 2
11-02 10:08:03.558  1001  2114 D PaxBatteryManagerService: count_times = 0
11-02 10:08:03.559  1001  2114 D PaxBatteryManagerService: Boot_flag = 1
11-02 10:08:03.559  1001  2114 D PaxBatteryManagerService: Remove_outdated_data

#40秒后才获取到Curmode
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: onReceive action = android.intent.action.BATTERY_CHANGED
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: onReceive bms_switch = 1
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: mode = 0
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: chargeState = 0
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: getInitcurMode = 0
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: batteryTermInfo.getCurmode() = 1
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: mBatteryLevel = 72
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: batteryTermInfo.getFullcharge() = 100
11-02 10:08:43.057   976   976 D PaxBatteryManagerService: batteryTermInfo.getRecharge() = 85
11-02 10:08:49.598   976  2115 D PaxBatteryManagerService: count_times = 180
11-02 10:08:49.599   976  2115 D PaxBatteryManagerService: Boot_flag = 0
11-02 10:08:49.599   976  2115 D PaxBatteryManagerService: mode = 0

```

底层收到关闭的ioctrl，log如下：
```log
11-02 10:08:35.350 I/PAX_BMS (    0): SET_CHG_EN: 0
11-02 10:08:35.350 E/        (    0): bms_notify_call_chain
11-02 10:08:35.350 E/PAX_CHG (    0): bms_notify_event evt = SET_CHG_EN en:0
11-02 10:08:35.352 E/PAX_CHG (    0): enable_charging en: 0 last_en: 1
11-02 10:08:35.352 E/PAX_CHG (    0): pax_charger_update, delay<40>
11-02 10:08:35.352 I/PAX_BMS (    0): type: NC_DISABLE_CHG_BY_USER, disable: 1, vote: 0x200000
11-02 10:08:35.352 I/PAX_BMS (    0): SET_POWER_PATH: 0
11-02 10:08:35.352 E/        (    0): bms_notify_call_chain
11-02 10:08:35.352 E/PAX_CHG (    0): bms_notify_event evt = SET_POWER_PATH en:0
11-02 10:08:35.353 E/PAX_CHG (    0): enable_powerpath en: 0
11-02 10:08:35.353 E/PAX_CHG (    0): pax_charger_update, delay<40>
```

发现是bms没有设置`Fullcharge`和`Recharge`默认值，加上就好了：
```java
   public BatteryTermInfo(Context context,int curmode){
		mContext = context;
        this.model = SystemProperties.get("ro.product.model","");
		if(model.equals("M8")){
			this.predicted_hours = PREDICTED_HOURS_DEFAULT_M8;
		}else if(model.equals("M50")){
			this.predicted_hours = PREDICTED_HOURS_DEFAULT_M50;	
		}else if(model.equals("A6650")){
			this.predicted_hours = PREDICTED_HOURS_DEFAULT_A6650;	
		}else if(model.equals("M9200")){
			this.predicted_hours = PREDICTED_HOURS_DEFAULT_M9200;	
		}else{
			this.predicted_hours = PREDICTED_HOURS_DEFAULT;
		}
        //if(model.equals("M50") || model.equals("A800") || model.equals("M8")){//M50,3, , 5
		this.curmode = curmode;
		this.save_data_min =  (long)(60 * 60 *this.predicted_hours * PREDICTED_DAYS_MIN * 1000) * 95 / 100;//至少3天的数据
		Log.d("test","save_data_min0 = "+save_data_min);
		if(curmode == COUNTER_TOP_MODE){
			this.recharge = COUNTER_TOP_MODE_RECHARGE;
			this.fullcharge = COUNTER_TOP_MODE_FULLCHARGE;
		}else {
			this.recharge = MOBILE_MODE_RECHARGE;
			this.fullcharge = MOBILE_MODE_FULLCHARGE;
		}
		this.saveday = SAVE_DAYS;
       // }
   }
```

# PD快充插入充电无法关闭充电

首先确认一下PD充电的时序，log如下：
```log
11-03 04:10:33.995 I/<61888.910>TCPC-TYPEC(    0): [CC_Alert] 7/0
11-03 04:10:33.995 I/<61888.911>TCPC-TCPC(    0): [Warning] ps_changed 1 -> 0
11-03 04:10:33.995 I/<61888.912>TCPC-TYPEC(    0): ** AttachWait.SNK
11-03 04:10:33.995 I/        (    0): <6
11-03 04:10:33.995 I/1888.913>TCPC-TCPC(    0): Alert:0x220000, Mask:0x20fff
11-03 04:10:33.995 I/<61888.913>TCPC-TCPC(    0): tcpci_alert_power_status_changed_v10 ++
11-03 04:10:33.995 I/        (    0): <61888.914>TCPC-TCP
11-03 04:10:33.995 I/        (    0): C:ps_change=0
11-03 04:10:33.995 I/<61888.917>TCPC-TCPC(    0): Alert:0x200000, Mask:0x20fff
11-03 04:10:34.096 I/        (    0): ///PD dbg info 80d
11-03 04:10:34.096 I/<61889.032>TCPC-TYPEC(    0): [CC_Change] 7/0
11-03 04:10:34.096 I/<61889.032>TCPC-TYPEC(    0): wait_ps=SNK_VSafe5V
11-03 04:10:34.127 I/        (    0): ///PD dbg info 111d
11-03 04:10:34.127 I/<61889.063>TCPC-TCPC(    0): Alert:0x200002, Mask:0x20fff
11-03 04:10:34.127 I/<61889.063>TCPC-TCPC(    0): tcpci_alert_power_status_changed_v10 ++
11-03 04:10:34.151 I/        (    0): ///PD dbg info 382d
11-03 04:10:34.151 I/<61889.064>TCPC-TCPC(    0): ps_change=2
11-03 04:10:34.151 I/<61889.065>TCPC-TYPEC(    0): wait_ps=Disable
11-03 04:10:34.151 I/<61889.065>TCPC-TYPEC(    0): ** Try.SRC
11-03 04:10:34.151 I/<61889.067>TCPC-TCPC(    0): Ale
11-03 04:10:34.151 I/rt      (    0): 0x200001, Mask:0x20fff
11-03 04:10:34.151 I/<61889.068>TCPC-TCPC(    0): tcpci_alert_cc_changed ++
11-03 04:10:34.151 I/<61889.069>TCPC-TYPEC(    0): [CC_Alert] 0/1
11-03 04:10:34.151 I/        (    0): <61889.086>TCPC-TC
11-03 04:10:34.151 I/PC      (    0): Alert:0x200002, Mask:0x20fff
11-03 04:10:34.151 I/<61889.086>TCPC-TCPC(    0): tcpci_alert_power_status_changed_v10 ++
11-03 04:10:34.151 I/<61889.087>TCPC-TCPC(    0): ps_change=1
11-03 04:10:34.175 I/        (    0): ///PD dbg info 144d
11-03 04:10:34.175 I/<61889.100>TCPC-TCPC(    0): Alert:0x220000, Mask:0x20fff
11-03 04:10:34.175 I/<61889.100>TCPC-TCPC(    0): tcpci_alert_power_status_changed_v10 ++
11-03 04:10:34.175 I/        (    0): <61889.101>TCPC-T
11-03 04:10:34.175 I/CPC     (    0): ps_change=0
11-03 04:10:34.232 I/        (    0): ///PD dbg info 75d
11-03 04:10:34.232 I/<61889.167>TCPC-TYPEC(    0): [CC_Change] 0/1
11-03 04:10:34.232 I/<61889.167>TCPC-TYPEC(    0): ** TryWait.SNK
11-03 04:10:34.232 E/        (    0): pd_tcp_notifier_call event = SOURCE_VBUS
11-03 04:10:34.232 E/        (    0): pd_tcp_notifier_call source vbus 0mV
11-03 04:10:34.232 E/        (    0): pd_tcp_notifier_call - source vbus 0v output
11-03 04:10:34.232 E/PAX_CHG (    0): vbus_off
11-03 04:10:34.232 E/PAX_CHG (    0): issue work, ops<0>, delay<0>
11-03 04:10:34.232 E/PAX_CHG (    0): _set_otg_enable now_status:0 set_status:0
11-03 04:10:34.255 I/        (    0): ///PD dbg info 134d
11-03 04:10:34.255 I/<61889.171>TCPC-TCPC(    0): Alert:0x200001, Mask:0x20fff
11-03 04:10:34.255 I/<61889.171>TCPC-TCPC(    0): tcpci_alert_cc_changed ++
11-03 04:10:34.255 I/<61889.172>TCPC-TYPEC(    0): [CC_Alert
11-03 04:10:34.255 I/        (    0): ] 7/0
11-03 04:10:34.357 I/        (    0): ///PD dbg info 80d
11-03 04:10:34.357 I/<61889.293>TCPC-TYPEC(    0): [CC_Change] 7/0
11-03 04:10:34.357 I/<61889.293>TCPC-TYPEC(    0): wait_ps=SNK_VSafe5V
11-03 04:10:34.389 I/        (    0): ///PD dbg info 111d
11-03 04:10:34.389 I/<61889.324>TCPC-TCPC(    0): Alert:0x200002, Mask:0x20fff
11-03 04:10:34.389 I/<61889.325>TCPC-TCPC(    0): tcpci_alert_power_status_changed_v10 ++
11-03 04:10:34.392 E/        (    0): pd_tcp_notifier_call event = SINK_VBUS
11-03 04:10:34.392 E/charger soc(    0): charger: pd_tcp_notifier_call sink vbus 5000mV 3000mA type(0x01)
11-03 04:10:34.392 E/        (    0): pd_tcp_notifier_call - sink vbus
11-03 04:10:34.392 E/        (    0): pd_tcp_notifier_call event = TYPEC_STATE
11-03 04:10:34.392 E/        (    0): tcpc_notifier_call, old_state = UNATTACHED, new_state = ATTACHED_SNK //1. ATTACHED_SNK
11-03 04:10:34.392 E/        (    0): pd_tcp_notifier_call Charger plug in, polarity = 0
11-03 04:10:34.392 E/PAX_CHG (    0): handle_typec_attach_dettach: ++ en:1
11-03 04:10:34.403 I/charger soc(    0): charger: usb_dwork_handler Device
11-03 04:10:34.411 I/        (    0): ///PD dbg info 186d
11-03 04:10:34.411 I/<61889.325>TCPC-TCPC(    0): ps_change=2
11-03 04:10:34.411 I/<61889.326>TCPC-TYPEC(    0): wait_ps=Disable
11-03 04:10:34.411 I/<61889.326>TCPC-TYPEC(    0): ** Attached.SNK
11-03 04:10:34.411 I/        (    0): <61889.327>TCPC-TYP
11-03 04:10:34.411 I/EC      (    0): Attached-> SINK
11-03 04:10:34.411 I/<61889.327>TCPC-TCPC(    0): usb_port_attached
11-03 04:10:34.442 I/        (    0): ///PD dbg info 56d
11-03 04:10:34.442 I/<61889.378>TCPC-TYPEC(    0): type=1, ret,chg_type=0,0, count=1
11-03 04:10:34.492 I/        (    0): ///PD dbg info 56d
11-03 04:10:34.492 I/<61889.428>TCPC-TYPEC(    0): type=1, ret,chg_type=0,0, count=2
11-03 04:10:34.522 I/        (    0): ///PD dbg info 50d
11-03 04:10:34.522 I/<61889.457>TCPC-TCPC(    0): Alert:0x200001, Mask:0x20fff
11-03 04:10:34.543 I/        (    0): ///PD dbg info 178d
11-03 04:10:34.543 I/<61889.458>TCPC-TCPC(    0): tcpci_alert_cc_changed ++
11-03 04:10:34.543 I/<61889.459>TCPC-TYPEC(    0): [CC_Alert] 7/0
11-03 04:10:34.543 I/<61889.470>TCPC-TYPEC(    0): [CC_Change] 7/0
11-03 04:10:34.543 I/        (    0): <61889
11-03 04:10:34.543 I/.478>TCPC-TYPEC(    0): type=1, ret,chg_type=0,0, count=3
11-03 04:10:34.571 I/        (    0): ///PD dbg info 356d
11-03 04:10:34.571 I/<61889.483>TCPC-TCPC(    0): Alert:0x200001, Mask:0x20fff
11-03 04:10:34.571 I/<61889.483>TCPC-TCPC(    0): tcpci_alert_cc_changed ++
11-03 04:10:34.571 I/<61889.485>TCPC-TYPEC(    0): [CC_Alert
11-03 04:10:34.571 I/        (    0): ] 7/0
11-03 04:10:34.571 I/<61889.487>TCPC-TCPC(    0): Alert:0x200001, Mask:0x20fff
11-03 04:10:34.571 I/<61889.487>TCPC-TCPC(    0): tcpci_alert_cc_changed ++
11-03 04:10:34.571 I/<61889.488>TCPC-TYPEC(    0): [CC
11-03 04:10:34.571 I/        (    0): _Alert] 7/0
11-03 04:10:34.571 I/<61889.490>TCPC-TCPC(    0): Alert:0x200000, Mask:0x20fff
11-03 04:10:34.571 I/<61889.499>TCPC-TYPEC(    0): [CC_Change] 7/0
11-03 04:10:34.595 I/        (    0): ///PD dbg info 56d
11-03 04:10:34.595 I/<61889.528>TCPC-TYPEC(    0): type=1, ret,chg_type=0,0, count=4
11-03 04:10:34.643 I/        (    0): ///PD dbg info 56d
11-03 04:10:34.643 I/<61889.578>TCPC-TYPEC(    0): type=1, ret,chg_type=0,0, count=5
11-03 04:10:34.655 E/PAX_CHG (    0): set charge_type: DCP //2. qcom完成bc1.2检测
11-03 04:10:34.656 E/PAX_CHG (    0): pax_is_charger_on chr_type = [DCP] last_chr_type = [Unknown]
11-03 04:10:34.656 E/PAX_CHG (    0): pax_charger_plug_in
11-03 04:10:34.656 E/PAX_CHG (    0): enable_charging en: 1 last_en: 0 //3. 使能充电
11-03 04:10:34.657 E/PAX_CHG (    0): pax_charger_update, delay<40> 4. 上报信息
11-03 04:10:34.657 E/PAX_CHG (    0): [SW_JEITA] Battery Normal Temperature between 15 and 45 !!
11-03 04:10:34.657 E/PAX_CHG (    0): [SW_JEITA]preState:3 newState:3 tmp:30 cv:0
11-03 04:10:34.657 E/PAX_CHG (    0): tmp:30 (jeita:1 sm:3 cv:0 en:1) thm_sm:1 en:1 can_en:1
11-03 04:10:34.658 E/PAX_CHG (    0): is typec adapter rp = 3000
11-03 04:10:34.658 E/PAX_CHG (    0): type-C pd_type:0 typec_rp_level:3000
11-03 04:10:34.658 E/PAX_CHG (    0): chg:-1,-1,2000,4140 type:5:0 aicl:-1 bootmode:0 pd:0
11-03 04:10:34.658 E/PAX_CHG (    0): do_algorithm input_current_limit:2000 charging_current_limit:4140
11-03 04:10:34.662 E/PAX_CHG (    0): CHG [online: 0, type: DCP, status: Not charging, fault: 0x0, ICHG = 4080mA, AICR = 2000mA, MIVR = 4360mV, IEOC = 240mA, CV = 4375mV]
11-03 04:10:34.690 I/afe_close(    0): port_id = 0xb030
11-03 04:10:34.693 I/        (    0): ///PD dbg info 56d
11-03 04:10:34.693 I/<61889.629>TCPC-TYPEC(    0): type=1, ret,chg_type=0,5, count=6
11-03 04:10:34.715 I/        (    0): ///PD dbg info 207d
11-03 04:10:34.715 I/        (    0): <61889.629>TCPC-DC> dc_dfp_none
11-03 04:10:34.715 I/<61889.629>TCPC-PE(    0): PD-> SNK_START
11-03 04:10:34.715 I/<61889.630>TCPC-PE-EVT(    0): reset_prl_done
11-03 04:10:34.715 I/<61889.630>TCPC-PE(    0): PD->
11-03 04:10:34.715 I/        (    0): SNK_DISC
11-03 04:10:34.715 I/<61889.630>TCPC-PE-EVT(    0): vbus_high
11-03 04:10:34.715 I/<61889.630>TCPC-PE(    0): PD-> SNK_WAIT_CAP
11-03 04:10:34.741 E/PAX_BAT (    0): [status:Discharging, health:Good, present:1, tech:Li-ion, capcity:80,cap_rm:3578 mah, vol:4038 mv, temp:30, curr:-421 ma, ui_soc:80]
11-03 04:10:34.741 E/PAX_BAT (    0): pax_battery_external_power_changed event, online:0, status:3, cur_chr_type:5 //5. Battery收到回调，此时online = 0 非充电状态
11-03 04:10:34.741 E/PAX_BMS (    0): bms_wakeup
11-03 04:10:34.747 W/audit   (    0): audit_lost=70627 audit_rate_limit=5 audit_backlog_limit=64
11-03 04:10:34.747 E/audit   (    0): rate limit exceeded
11-03 04:10:34.751 E/PAX_BMS (    0): pax_bms_external_power_changed online = 0
11-03 04:10:34.751 E/PAX_BMS (    0): CHG [online: 0, type: 5, vol: 5000000, cur: 500000, time: 0], BAT [present: 1, status: 3, vol: 4038000, cur: -421000, resistance: 0, temp: 300, soc: 80], OTHER [skin_temp: 0, chg_vote: 0x0, notify_code: 0x0],
11-03 04:10:34.762 W/healthd (    0): battery l=80 v=4038 t=30.0 h=2 st=3 c=-421000 fc=4512000 cc=0 chg=u
11-03 04:10:34.785 W/healthd (    0): battery l=80 v=4038 t=30.0 h=2 st=3 c=-421000 fc=4512000 cc=0 chg=u
11-03 04:10:34.804 I/        (    0): ///PD dbg info 50d
11-03 04:10:34.804 I/<61889.740>TCPC-TCPC(    0): Alert:0x200004, Mask:0x20fff
11-03 04:10:34.814 E/        (    0): pd_tcp_notifier_call event = SINK_VBUS
11-03 04:10:34.814 E/charger soc(    0): charger: pd_tcp_notifier_call sink vbus 9000mV 277mA type(0x86)
11-03 04:10:34.814 E/        (    0): pd_tcp_notifier_call - sink vbus
11-03 04:10:34.814 E/PAX_CHG (    0): pd_status:1 // 6. 接下来PD设置快充电压电流，先设置了9v充电电压
11-03 04:10:34.814 E/PAX_CHG (    0): set pd_charging_voltage_max:9000 mv
11-03 04:10:34.814 E/PAX_CHG (    0): set pd_charging_current_max:277 ma
11-03 04:10:34.827 I/        (    0): ///PD dbg info 466d
11-03 04:10:34.827 I/<61889.742>TCPC-PE-EVT(    0): src_cap // 7. 根据PD策略选择档位信息
11-03 04:10:34.827 I/<61889.742>TCPC-PE(    0): PD-> SNK_EVA_CAP
11-03 04:10:34.827 I/<61889.742>TCPC-PE(    0): pd_rev=2
11-03 04:10:34.827 I/<61889.742>TCPC-DPM(    0): Policy=0x31
11-03 04:10:34.827 I/        (    0): <
11-03 04:10:34.827 I/61889.742>TCPC-DPM(    0): Select SrcCap2
11-03 04:10:34.827 I/<61889.742>TCPC-PE-EVT(    0): dpm_ack
11-03 04:10:34.827 I/<61889.742>TCPC-PE(    0): PD-> SNK_SEL_CAP
11-03 04:10:34.827 I/<61889.745>TCPC-TCPC(    0): Alert:
11-03 04:10:34.827 I/0x200040, Mask(    0): 0x20fff
11-03 04:10:34.827 I/<61889.745>TCPC-PE-EVT(    0): good_crc
11-03 04:10:34.827 I/<61889.749>TCPC-TCPC(    0): Alert:0x200004, Mask:0x20fff
11-03 04:10:34.827 I/<61889.750>TCPC-PE-EVT(    0):  
11-03 04:10:34.827 I/        (    0): accept
11-03 04:10:34.827 I/<61889.750>TCPC-PE(    0): PD-> SNK_TRANS_SINK
11-03 04:10:34.827 I/<61889.750>TCPC-PE(    0): VC_HIGHV_PROT: 1
11-03 04:10:34.973 I/        (    0): ///PD dbg info 50d
11-03 04:10:34.973 I/<61889.909>TCPC-TCPC(    0): Alert:0x200004, Mask:0x20fff
11-03 04:10:34.974 E/        (    0): pd_tcp_notifier_call event = SINK_VBUS
11-03 04:10:34.974 E/charger soc(    0): charger: pd_tcp_notifier_call sink vbus 9000mV 2000mA type(0x84) //8. 收到tcpc notify设置9v/2a属性
11-03 04:10:34.974 E/        (    0): pd_tcp_notifier_call - sink vbus
11-03 04:10:34.974 E/PAX_CHG (    0): set pd_charging_voltage_max:9000 mv
11-03 04:10:34.974 E/PAX_CHG (    0): set pd_charging_current_max:2000 ma
11-03 04:10:34.995 I/        (    0): ///PD dbg info 64d
11-03 04:10:34.995 I/<61889.910>TCPC-PE-EVT(    0): ps_rdy
11-03 04:10:34.995 I/<61889.910>TCPC-PE(    0): PD-> SNK_READY
11-03 04:10:35.275 I/        (    0): ///PD dbg info 122d
11-03 04:10:35.275 I/<61890.210>TCPC-PE-EVT(    0): timer
11-03 04:10:35.275 I/<61890.210>TCPC-PE-EVT(    0): tcp_event(get_src_cap_ext), 17
11-03 04:10:35.275 I/<61890.210>TCPC-PE(    0): PD-> SNK_GET_CAP_EX
11-03 04:10:35.298 E/        (    0): pd_tcp_notifier_call event = PD_STATE
11-03 04:10:35.298 I/charger soc(    0): charger: pd_tcp_notifier_call pd state = 6 // 9.PD设置完成，连接状态为PD_CONNECT_PE_READY_SNK_PD30
11-03 04:10:35.299 E/PAX_CHG (    0): pd_status:1
11-03 04:10:35.299 E/PAX_CHG (    0): set pd_charging_current_max:2000 ma
11-03 04:10:35.299 I/        (    0): ///PD dbg info 750d
11-03 04:10:35.299 I/<61890.214>TCPC-TCPC(    0): Alert:0x200040, Mask:0x20fff
11-03 04:10:35.299 I/<61890.215>TCPC-PE-EVT(    0): good_crc
11-03 04:10:35.299 I/<61890.218>TCPC-TCPC(    0): Alert:0x200004, Mask:0x20
11-03 04:10:35.299 I/        (    0): fff
11-03 04:10:35.299 I/<61890.220>TCPC-PE-EVT(    0): no_support
11-03 04:10:35.299 I/<61890.220>TCPC-PE(    0): PD-> SNK_READY
11-03 04:10:35.299 I/<61890.220>TCPC-PE-EVT(    0): tcp_event(disc_id), 29
11-03 04:10:35.299 I/        (    0): <61890.220
11-03 04:10:35.299 I/>TCPC-PE(    0): VDM-> D_UID_REQ
11-03 04:10:35.299 I/<61890.223>TCPC-TCPC(    0): Alert:0x200040, Mask:0x20fff
11-03 04:10:35.299 I/<61890.224>TCPC-PE-EVT(    0): good_crc
11-03 04:10:35.299 I/<61890.232>TCPC-TCPC(    0):  
11-03 04:10:35.299 I/Alert   (    0): 0x200004, Mask:0x20fff
11-03 04:10:35.299 I/<61890.234>TCPC-PE-EVT(    0): no_support
11-03 04:10:35.299 I/<61890.234>TCPC-PE(    0): PD-> SNK_NO_SUPP_RECV
11-03 04:10:35.299 I/<61890.234>TCPC-PE-EVT(    0): d
11-03 04:10:35.299 I/        (    0): pm_ack
11-03 04:10:35.299 I/<61890.234>TCPC-PE(    0): PD-> SNK_READY
11-03 04:10:35.299 I/<61890.234>TCPC-PE-EVT(    0): vdm_not_support
11-03 04:10:35.299 I/<61890.234>TCPC-PE(    0): VDM-> D_UID_N
11-03 04:10:35.299 I/        (    0): <61890.234>TCPC
11-03 04:10:35.299 I/-PE-EVT (    0): dpm_ack
11-03 04:10:35.299 I/<61890.234>TCPC-PE(    0): VDM-> SNK_READY
11-03 04:10:35.299 I/<61890.234>TCPC-DPM(    0): PE_READY
11-03 04:10:35.299 I/<61890.234>TCPC-PE(    0): pd_state=6
11-03 04:10:36.123 E/PAX_BAT (    0): [status:Discharging, health:Good, present:1, tech:Li-ion, capcity:80,cap_rm:3578 mah, vol:4254 mv, temp:30, curr:2537 ma, ui_soc:80]
11-03 04:10:36.129 W/audit   (    0): audit_lost=70649 audit_rate_limit=5 audit_backlog_limit=64
11-03 04:10:36.129 E/audit   (    0): rate limit exceeded
11-03 04:10:36.136 W/healthd (    0): battery l=80 v=4254 t=30.0 h=2 st=3 c=2537000 fc=4512000 cc=0 chg=u
11-03 04:10:39.657 E/PAX_CHG (    0): pax_is_charger_on chr_type = [DCP] last_chr_type = [DCP]
11-03 04:10:39.659 E/PAX_CHG (    0): [SW_JEITA] Battery Normal Temperature between 15 and 45 !!
11-03 04:10:39.659 E/PAX_CHG (    0): [SW_JEITA]preState:3 newState:3 tmp:30 cv:0
11-03 04:10:39.659 E/PAX_CHG (    0): tmp:30 (jeita:1 sm:3 cv:0 en:1) thm_sm:1 en:1 can_en:1
11-03 04:10:39.660 E/PAX_CHG (    0): chg:-1,-1,2000,4160 type:5:6 aicl:-1 bootmode:0 pd:1
11-03 04:10:39.660 E/PAX_CHG (    0): do_algorithm input_current_limit:2000 charging_current_limit:4160
11-03 04:10:39.665 E/PAX_CHG (    0): CHG [online: 1, type: DCP, status: Charging, fault: 0x0, ICHG = 4160mA, AICR = 2000mA, MIVR = 4360mV, IEOC = 240mA, CV = 4375mV]
11-03 04:10:39.958 E/PAX_BMS (    0): CHG [online: 1, type: 5, vol: 5000000, cur: 2000000, time: 0], BAT [present: 1, status: 3, vol: 4254000, cur: 2537000, resistance: 0, temp: 300, soc: 80], OTHER [skin_temp: 0, chg_vote: 0x0, notify_code: 0x0],
```

PD充电大概流程如下：
* 11-03 04:10:34.392 E/        (    0): tcpc_notifier_call, old_state = UNATTACHED, new_state = ATTACHED_SNK //1. ATTACHED_SNK
* 11-03 04:10:34.655 E/PAX_CHG (    0): set charge_type: DCP //2. qcom完成bc1.2检测
* 11-03 04:10:34.656 E/PAX_CHG (    0): enable_charging en: 1 last_en: 0 //3. 使能充电
* 11-03 04:10:34.657 E/PAX_CHG (    0): pax_charger_update, delay<40> 4. 上报信息
* 11-03 04:10:34.741 E/PAX_BAT (    0): pax_battery_external_power_changed event, online:0, status:3, cur_chr_type:5 //5. Battery收到回调，此时online = 0 非充电状态
* 11-03 04:10:34.814 E/PAX_CHG (    0): pd_status:1 //6. 接下来PD设置快充电压电流，先设置了9v充电电压
* 11-03 04:10:34.814 E/PAX_CHG (    0): set pd_charging_voltage_max:9000 mv
* 11-03 04:10:34.814 E/PAX_CHG (    0): set pd_charging_current_max:277 ma
* 11-03 04:10:34.827 I/<61889.742>TCPC-PE-EVT(    0): src_cap // 7. 根据PD策略选择档位信息
* 11-03 04:10:34.827 I/<61889.742>TCPC-PE(    0): PD-> SNK_EVA_CAP
* 11-03 04:10:34.827 I/<61889.742>TCPC-PE(    0): pd_rev=2
* 11-03 04:10:34.827 I/<61889.742>TCPC-DPM(    0): Policy=0x31
* 11-03 04:10:34.827 I/61889.742>TCPC-DPM(    0): Select SrcCap2
* 11-03 04:10:34.974 E/charger soc(    0): charger: pd_tcp_notifier_call sink vbus 9000mV 2000mA type(0x84) //7. 收到tcpc notify设置9v/2a属性
* 11-03 04:10:34.974 E/        (    0): pd_tcp_notifier_call - sink vbus
* 11-03 04:10:34.974 E/PAX_CHG (    0): set pd_charging_voltage_max:9000 mv
* 11-03 04:10:34.974 E/PAX_CHG (    0): set pd_charging_current_max:2000 ma
* 11-03 04:10:35.298 I/charger soc(    0): charger: pd_tcp_notifier_call pd state = 6 // 8.PD设置完成，连接状态为PD_CONNECT_PE_READY_SNK_PD30

可以看到在步骤4上报信息时，charger还是`online = 0`非充电状态，此时上报电池状态肯定是不对的，我们需要等到PD完成时再上报一次，可以看到步骤8，当收到tcpc notify设置9v/2a属性时，PD设置完成，连接状态为PD_CONNECT_PE_READY_SNK_PD30，我们在这里再上报一次，修改如下：
```diff
--- a/UM.9.15/kernel/msm-4.19/drivers/misc/pax/power/paxpd-charger-manager.c
+++ b/UM.9.15/kernel/msm-4.19/drivers/misc/pax/power/paxpd-charger-manager.c
@@ -217,14 +217,6 @@ static void pd_sink_set_vol_and_cur(int mv, int ma, uint8_t type)
        unsigned long sel = 0;
        union power_supply_propval val = {.intval = 0};

-       // Charger plug-in first time
-       smblib_get_prop(POWER_SUPPLY_PROP_PD_ACTIVE, &val);
-       if (val.intval == POWER_SUPPLY_PD_INACTIVE) {
-               val.intval = POWER_SUPPLY_PD_ACTIVE;
-               //pax_charger_dev_enable_vbus_ovp(g_info->chg1_dev, false);
-               smblib_set_prop(POWER_SUPPLY_PROP_PD_ACTIVE, &val);
-       }
-
        switch (type) {
        case TCP_VBUS_CTRL_PD_HRESET:
        case TCP_VBUS_CTRL_PD_PR_SWAP:
@@ -1087,8 +1079,9 @@ static int pax_charger_plug_in(struct pax_charger *info,

        info->chr_type = chr_type;
        info->charger_thread_polling = true;
-
        info->can_charging = true;
+       info->bms_charge_enable = 1;
+
        //info->enable_dynamic_cv = true;

        chr_info("%s\n", __func__);
@@ -1467,6 +1460,10 @@ int psy_charger_set_property(struct power_supply *psy,
        switch (psp) {
        case POWER_SUPPLY_PROP_PD_ACTIVE:
                info->pd_active = val->intval;
+               /* PD SNK complite */
+               if (info->pd_active) {
+                       pax_charger_update(20);
+               }
                chr_info("pd_status:%d\n", info->pd_active);
```

修改后，状态重新上报没问题：
```log
11-02 10:09:15.501 I/charger soc(    0): charger: pd_tcp_notifier_call pd state = 6
11-02 10:09:15.501 E/PAX_CHG (    0): pax_charger_update, delay<20> //1.重新上报状态
11-02 10:09:15.502 E/PAX_CHG (    0): pd_status:1
11-02 10:09:15.502 E/PAX_CHG (    0): set pd_charging_current_max:2000 ma
11-02 10:09:15.532 I/        (    0): ///PD dbg info 44d
11-02 10:09:15.532 I/<  147.000>TCPC-PE-EVT(    0): tcp_event(dummy), 16
11-02 10:09:15.537 W/healthd (    0): battery l=81 v=4050 t=32.0 h=2 st=2 c=-442000 fc=4512000 cc=0 chg=u
11-02 10:09:15.566 E/PAX_BAT (    0): [status:Charging, health:Good, present:1, tech:Li-ion, capcity:81,cap_rm:3626 mah, vol:4050 mv, temp:32, curr:-442 ma, ui_soc:81]
11-02 10:09:15.566 E/PAX_BAT (    0): pax_battery_external_power_changed event, online:1, status:1, cur_chr_type:5 //2. Battery 状态获取正确
11-02 10:09:15.566 E/PAX_BMS (    0): bms_wakeup
11-02 10:09:15.574 E/PAX_BMS (    0): pax_bms_external_power_changed online = 1
11-02 10:09:15.576 I/PAX_BMS (    0): charge start: 147
11-02 10:09:15.576 E/PAX_BMS (    0): CHG [online: 1, type: 5, vol: 5000000, cur: 2000000, time: 0], BAT [present: 1, status: 1, vol: 4050000, cur: -442000, resistance: 0, temp: 320, soc: 81], OTHER [skin_temp: 0, chg_vote: 0x0, notify_code: 0x0],
11-02 10:09:15.576 W/healthd (    0): battery l=81 v=4050 t=32.0 h=2 st=2 c=-442000 fc=4512000 cc=0 chg=u
11-02 10:09:15.591 W/healthd (    0): battery l=81 v=4050 t=32.0 h=2 st=2 c=-442000 fc=4512000 cc=0 chg=u
11-02 10:09:15.740 I/PAX_BMS (    0): SET_CHG_EN: 0 // 3. BMS服务发现此时电量大于满充点，则关闭充电。
11-02 10:09:15.740 E/        (    0): bms_notify_call_chain
11-02 10:09:15.740 E/PAX_CHG (    0): bms_notify_event evt = SET_CHG_EN en:0
11-02 10:09:15.741 E/PAX_CHG (    0): enable_charging en: 0 last_en: 1
11-02 10:09:15.741 E/PAX_CHG (    0): pax_charger_update, delay<40>
11-02 10:09:15.741 I/PAX_BMS (    0): type: NC_DISABLE_CHG_BY_USER, disable: 1, vote: 0x200000
11-02 10:09:15.750 I/PAX_BMS (    0): SET_CHG_EN: 0
11-02 10:09:15.750 E/        (    0): bms_notify_call_chain
11-02 10:09:15.750 E/PAX_CHG (    0): bms_notify_event evt = SET_CHG_EN en:0
11-02 10:09:15.752 E/PAX_CHG (    0): enable_charging en: 0 last_en: 0
11-02 10:09:15.752 E/PAX_CHG (    0): pax_charger_update, delay<40>
11-02 10:09:15.752 I/PAX_BMS (    0): type: NC_DISABLE_CHG_BY_USER, disable: 1, vote: 0x200000
11-02 10:09:15.797 I/        (    0): ///PD dbg info 44d
11-02 10:09:15.797 I/<  147.266>TCPC-PE-EVT(    0): tcp_event(dummy), 16
11-02 10:09:15.798 W/healthd (    0): battery l=81 v=4050 t=32.0 h=2 st=4 c=-442000 fc=4512000 cc=0 chg=
11-02 10:09:15.812 E/PAX_BAT (    0): [status:Not charging, health:Good, present:1, tech:Li-ion, capcity:81,cap_rm:3626 mah, vol:4050 mv, temp:32, curr:-442 ma, ui_soc:81]
11-02 10:09:15.812 E/PAX_BAT (    0): pax_battery_external_power_changed event, online:1, status:3, cur_chr_type:5
11-02 10:09:15.812 E/PAX_BMS (    0): bms_wakeup
11-02 10:09:15.816 E/PAX_BMS (    0): pax_bms_external_power_changed online = 1
11-02 10:09:15.818 I/PAX_BMS (    0): charge end: charge_start_time: 147, charge_total_time: 0
```

# BMS回充打印

将电量低于回充点65时启动充电。
```log
188669: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): onReceive action = android.intent.action.BATTERY_CHANGED
188670: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): onReceive bms_switch = 1
188671: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): mode = 2
188672: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): chargeState = 0
188673: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): getInitcurMode = 2
188674: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): batteryTermInfo.getCurmode() = 2
188675: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): mBatteryLevel = 65
188676: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): batteryTermInfo.getFullcharge() = 80
188677: 11-02 12:28:32.180 D/PaxBatteryManagerService( 1052): batteryTermInfo.getRecharge() = 65
188783: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): onReceive action = android.intent.action.BATTERY_CHANGED
188784: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): onReceive bms_switch = 1
188785: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): mode = 2
188786: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): chargeState = 0
188787: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): getInitcurMode = 2
188788: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): batteryTermInfo.getCurmode() = 2
188789: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): mBatteryLevel = 64
188790: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): batteryTermInfo.getFullcharge() = 80
188791: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): batteryTermInfo.getRecharge() = 65
188792: 11-02 12:28:40.382 D/PaxBatteryManagerService( 1052): chargeing //将电量低于回充点65时启动充电。
188796: 11-02 12:28:40.383 D/PaxBatteryManagerService( 1052): setChargeState: 1
188800: 11-02 12:28:40.384 D/PaxBatteryManagerService( 1052): setChargeState over
188902: 11-02 12:28:40.454 D/PaxBatteryManagerService( 1052): onReceive action = android.intent.action.BATTERY_CHANGED
188903: 11-02 12:28:40.454 D/PaxBatteryManagerService( 1052): onReceive bms_switch = 1
```