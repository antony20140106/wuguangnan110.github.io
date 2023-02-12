# qcom qcm2290 葆机压力测试USB失效问题分析

发现机器葆机后，USB无法使用，不枚举，充电状态不更新。

# log分析

```log
1. 电脑USB插入
01-01 08:01:55.362 E/        (    0): tcpc_notifier_call, old_state = UNATTACHED, new_state = ATTACHED_SNK
01-01 08:01:55.362 E/        (    0): pd_tcp_notifier_call Charger plug in, polarity = 1
01-01 08:01:55.525 E/PAX_CHG (    0): pax_is_charger_on chr_type = [SDP] last_chr_type = [Unknown]

拔出
01-14 23:58:35.373 E/        (    0): tcpc_notifier_call, old_state = ATTACHED_SNK, new_state = UNATTACHED
01-14 23:58:35.373 E/        (    0): pd_tcp_notifier_call Charger plug out
01-14 23:58:35.373 E/PAX_CHG (    0): handle_typec_attach_dettach: ++ en:0
01-14 23:58:35.373 E/PAX_CHG (    0): pd_status:0
01-14 23:58:35.373 E/PAX_CHG (    0): pax_is_charger_on chr_type = [Unknown] last_chr_type = [SDP]

继续插入电脑：
01-14 23:58:37.219 E/PAX_CHG (    0): set charge_type: SDP info->attach = 1
01-14 23:58:37.219 E/PAX_CHG (    0): pax_is_charger_on chr_type = [SDP] last_chr_type = [Unknown]

拔出:
01-14 23:58:38.817 E/PAX_CHG (    0): pax_is_charger_on chr_type = [Unknown] last_chr_type = [SDP]
01-14 23:58:38.817 E/PAX_CHG (    0): pax_charger_plug_out

插入：
01-14 23:58:40.600 E/PAX_CHG (    0): set charge_type: SDP info->attach = 1
01-14 23:58:40.600 E/PAX_CHG (    0): pax_is_charger_on chr_type = [SDP] last_chr_type = [Unknown]

拔出：
01-14 23:58:42.113 E/PAX_CHG (    0): handle_typec_attach_dettach: ++ en:0
01-14 23:58:42.113 E/PAX_CHG (    0): pax_is_charger_on chr_type = [Unknown] last_chr_type = [SDP]

最后一次插入适配器：
01-14 23:59:49.151 E/PAX_CHG (    0): set charge_type: DCP info->attach = 1
01-14 23:59:49.151 E/PAX_CHG (    0): pax_is_charger_on chr_type = [DCP] last_chr_type = [Unknown]
01-14 23:59:49.151 E/PAX_CHG (    0): pax_charger_plug_in

马上收到ftest发来的关闭充电：
01-14 23:59:49.187 I/PAX_BMS (    0): SET_CHG_EN: 0
01-14 23:59:49.187 E/        (    0): bms_notify_call_chain
01-14 23:59:49.187 E/PAX_CHG (    0): bms_notify_event evt = SET_CHG_EN en:0
01-14 23:59:49.189 E/PAX_CHG (    0): mp2721_enable_charger last: 1 en: 0
01-14 23:59:49.193 I/CAM_ERR (    0): CAM-CDM: cam_hw_cdm_work: 1362 NULL payload
01-14 23:59:49.195 E/PAX_CHG (    0): enable_charging en: 0 last_en: 1
01-14 23:59:49.195 E/PAX_CHG (    0): pax_charger_update, delay<40>
01-14 23:59:49.195 I/PAX_BMS (    0): type: NC_DISABLE_CHG_BY_USER, disable: 1, vote: 0x200000
```