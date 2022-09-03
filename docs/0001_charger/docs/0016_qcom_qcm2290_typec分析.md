# 概述

高通qcm2290 typec架构理解。 

# 参考

* [USB Type-C连接器和电缆组件简介](https://www.elecfans.com/d/1826488.html)

USB标准对以下五种Type-C互连类型分别做了详细的要求：

* Type-C to Type-C 电缆组件(Type-C to Type-C Cable Assemblies)
* Type-C to传统电缆组件(Type-C to Legacy Cable Assemblies)
* Type-C to 传统适配器(Type-C to Legacy Adapters)
* Type-C 插座连接器（Receptacle Connector）
* 裸电缆(Raw Cable)

# 摘要

* XBL路径：
```
boot_images/QcomPkg/SocPkg/AgattiPkg/Library/UsbConfigLib/
.
├── UsbConfigLib.c
├── UsbConfigLib.inf
├── UsbConfigLibPrivate.h
├── UsbConfigLibPublic.h
├── UsbConfigUsbCores.c
├── UsbConfigUsbCoresPrivate.h
├── UsbConfigUsbCoresPublic.h
├── UsbConfigUsbPorts.c
├── UsbConfigUsbPortsPrivate.h
└── UsbConfigUsbPortsPublic.h
```

qpnp:qcom plug n play

# kernel软件流程

## dts

```C++
pm2250_charger: qcom,qpnp-smblite {
        compatible = "qcom,qpnp-smblite";
        #address-cells = <1>;
        #size-cells = <1>;
        #cooling-cells = <2>;

        qcom,typec@1500 {
                reg = <0x1500 0x100>;
                interrupts =
                        <0x0 0x15 0x0 IRQ_TYPE_EDGE_BOTH>,
                        <0x0 0x15 0x1 IRQ_TYPE_EDGE_BOTH>,
                        <0x0 0x15 0x2 IRQ_TYPE_EDGE_RISING>,
                        <0x0 0x15 0x4 IRQ_TYPE_EDGE_BOTH>,
                        <0x0 0x15 0x5 IRQ_TYPE_EDGE_RISING>,
                        <0x0 0x15 0x6 IRQ_TYPE_EDGE_RISING>,
                        <0x0 0x15 0x7 IRQ_TYPE_EDGE_RISING>;

                interrupt-names = "typec-or-rid-detect-change",  //这里是所有中断
                                  "typec-vpd-detect",
                                  "typec-cc-state-change",
                                  "typec-vbus-change",
                                  "typec-attach-detach",
                                  "typec-legacy-cable-detect",
                                  "typec-try-snk-src-detect";
        };
};
```

## 中断申请及初始化流程

* `qpnp_smblite.c`:
```C++
* static int smblite_probe(struct platform_device *pdev)
  * rc = smblite_init_hw(chip);
    * rc = smblite_init_connector_type(chg);
      * rc = smblite_lib_read(chg, TYPEC_U_USB_CFG_REG, &val); //读取usb状态，是typec还是usb20
        * type = !!(val & EN_MICRO_USB_MODE_BIT);
        * if (！type) chg->connector_type = POWER_SUPPLY_CONNECTOR_TYPEC;
          * rc = smblite_configure_typec(chg);
            * rc = smblite_lib_read(chg, LEGACY_CABLE_STATUS_REG, &val);
            * smblite_lib_write(chg, TYPE_C_INTERRUPT_EN_CFG_1_REG, 0);
            *  smblite_lib_masked_write(chg, TYPE_C_MODE_CFG_REG, //这些都是配置pm2250寄存器，没什么好看的
    * rc = smblite_init_otg(chip); //usb20 id脚判断主从
  * rc = smblite_request_interrupts(chip);
    * for_each_available_child_of_node(node, child) {
      * of_property_for_each_string(child, "interrupt-names", //读取dts
      * smblite_request_interrupt(chip, child, name);
        * smblite_irqs[irq_index].handler //中断处理函数，目前typec-vbus-change没有处理函数
          * .name		= "typec-attach-detach", .handler	= smblite_typec_attach_detach_irq_handler,
          * .name		= "typec-legacy-cable-detect", .handler	= smblite_default_irq_handler,
          * .name		= "typec-cc-state-change", .handler	= smblite_typec_state_change_irq_handler,
          * .name		= "typec-or-rid-detect-change", .handler	= smblite_typec_or_rid_detection_change_irq_handler,
```

## Type-C中断函数介绍

### smblite_default_irq_handler

* 首先就是检测到usb插入了，中断发生，第一个中断，只是一个打印
```C++
irqreturn_t smblite_default_irq_handler(int irq, void *data)
{
	struct smb_irq_data *irq_data = data;
	struct smb_charger *chg = irq_data->parent_data;

	smblite_lib_dbg(chg, PR_INTERRUPT, "IRQ: %s\n", irq_data->name);
	return IRQ_HANDLED;
}
```

### smblite_typec_or_rid_detection_change_irq_handler

* 这个中断也是啥也没干。

```C++
irqreturn_t smblite_typec_or_rid_detection_change_irq_handler(int irq,
								void *data)
{
	struct smb_irq_data *irq_data = data;
	struct smb_charger *chg = irq_data->parent_data;

	smblite_lib_dbg(chg, PR_INTERRUPT, "IRQ: %s\n", irq_data->name);
	if (chg->usb_psy)
		power_supply_changed(chg->usb_psy);

	return IRQ_HANDLED;
}
```

### smblite_typec_state_change_irq_handler

* CC状态变化的中断如下，主要获取CC状态：

```C++
所有状态如下：
static const char * const smblite_lib_typec_mode_name[] = {
	[POWER_SUPPLY_TYPEC_NONE]		  = "NONE",
	[POWER_SUPPLY_TYPEC_SOURCE_DEFAULT]	  = "SOURCE_DEFAULT",
	[POWER_SUPPLY_TYPEC_SOURCE_MEDIUM]	  = "SOURCE_MEDIUM",
	[POWER_SUPPLY_TYPEC_SOURCE_HIGH]	  = "SOURCE_HIGH",
	[POWER_SUPPLY_TYPEC_NON_COMPLIANT]	  = "NON_COMPLIANT",
	[POWER_SUPPLY_TYPEC_SINK]		  = "SINK",
	[POWER_SUPPLY_TYPEC_SINK_POWERED_CABLE]   = "SINK_POWERED_CABLE",
	[POWER_SUPPLY_TYPEC_SINK_DEBUG_ACCESSORY] = "SINK_DEBUG_ACCESSORY",
	[POWER_SUPPLY_TYPEC_SINK_AUDIO_ADAPTER]   = "SINK_AUDIO_ADAPTER",
	[POWER_SUPPLY_TYPEC_POWERED_CABLE_ONLY]   = "POWERED_CABLE_ONLY",
};

中断如下：
irqreturn_t smblite_typec_state_change_irq_handler(int irq, void *data)
{
	struct smb_irq_data *irq_data = data;
	struct smb_charger *chg = irq_data->parent_data;
	int typec_mode;

	if (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB) {
		smblite_lib_dbg(chg, PR_INTERRUPT,
				"Ignoring for micro USB\n");
		return IRQ_HANDLED;
	}

	typec_mode = smblite_lib_get_prop_typec_mode(chg); //获取CC状态
	if (chg->sink_src_mode != UNATTACHED_MODE
			&& (typec_mode != chg->typec_mode))
		smblite_lib_handle_rp_change(chg, typec_mode);
	chg->typec_mode = typec_mode;

//smblite_typec_state_change_irq_handler: IRQ: cc-state-change; Type-C SOURCE_DEFAULT detected 打印表示SOURCE插入
	smblite_lib_dbg(chg, PR_INTERRUPT, "IRQ: cc-state-change; Type-C %s detected\n",
				smblite_lib_typec_mode_name[chg->typec_mode]);

	power_supply_changed(chg->usb_psy);

	return IRQ_HANDLED;
}

static int smblite_lib_get_prop_typec_mode(struct smb_charger *chg)
{
	int rc;
	u8 stat;

	if (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB) //micro usb直接返回
		return POWER_SUPPLY_TYPEC_NONE;

	rc = smblite_lib_read(chg, TYPE_C_MISC_STATUS_REG, &stat);
	if (rc < 0) {
		smblite_lib_err(chg, "Couldn't read TYPE_C_MISC_STATUS_REG rc=%d\n",
				rc);
		return 0;
	}
	smblite_lib_dbg(chg, PR_REGISTER, "TYPE_C_MISC_STATUS_REG = 0x%02x\n",
				stat);

	if (stat & SNK_SRC_MODE_BIT)
		return smblite_lib_get_prop_dfp_mode(chg); //获取dfp(host)状态,音频插入或者sink插入
	else
		return smblite_lib_get_prop_ufp_mode(chg);
}

static int smblite_lib_get_prop_dfp_mode(struct smb_charger *chg)
{
	int rc;
	u8 stat;

	rc = smblite_lib_read(chg, TYPE_C_SRC_STATUS_REG, &stat);
	if (rc < 0) {
		smblite_lib_err(chg, "Couldn't read TYPE_C_SRC_STATUS_REG rc=%d\n",
				rc);
		return POWER_SUPPLY_TYPEC_NONE;
	}
	smblite_lib_dbg(chg, PR_REGISTER, "TYPE_C_SRC_STATUS_REG = 0x%02x\n",
				stat);

	switch (stat & DETECTED_SNK_TYPE_MASK) {
	case AUDIO_ACCESS_RA_RA_BIT:
		return POWER_SUPPLY_TYPEC_SINK_AUDIO_ADAPTER;
	case SRC_DEBUG_ACCESS_BIT:
		return POWER_SUPPLY_TYPEC_SINK_DEBUG_ACCESSORY;
	case SRC_RD_OPEN_BIT:
		return POWER_SUPPLY_TYPEC_SINK;
	default:
		break;
	}

	return POWER_SUPPLY_TYPEC_NONE;
}

static int smblite_lib_get_prop_ufp_mode(struct smb_charger *chg)
{
	int rc;
	u8 stat;

	rc = smblite_lib_read(chg, TYPE_C_SNK_STATUS_REG, &stat);
	if (rc < 0) {
		smblite_lib_err(chg, "Couldn't read TYPE_C_STATUS_1 rc=%d\n",
					rc);
		return POWER_SUPPLY_TYPEC_NONE;
	}
	smblite_lib_dbg(chg, PR_REGISTER, "TYPE_C_STATUS_1 = 0x%02x\n", stat);

	switch (stat & DETECTED_SRC_TYPE_MASK) {
	case SNK_RP_STD_BIT:
		return POWER_SUPPLY_TYPEC_SOURCE_DEFAULT;
	case SNK_RP_1P5_BIT:
		return POWER_SUPPLY_TYPEC_SOURCE_MEDIUM;
	case SNK_RP_3P0_BIT:
		return POWER_SUPPLY_TYPEC_SOURCE_HIGH;
	case SNK_RP_SHORT_BIT:
		return POWER_SUPPLY_TYPEC_NON_COMPLIANT;
	case SNK_DAM_500MA_BIT:
	case SNK_DAM_1500MA_BIT:
	case SNK_DAM_3000MA_BIT:
		return POWER_SUPPLY_TYPEC_SINK_DEBUG_ACCESSORY;
	default:
		break;
	}

	return POWER_SUPPLY_TYPEC_NONE;
}
```

### smblite_typec_attach_detach_irq_handler

* smblite_typec_attach_detach_irq_handler中断比较重要，会进行usb主从切换。

```C++
irqreturn_t smblite_typec_attach_detach_irq_handler(int irq, void *data)
{
	struct smb_irq_data *irq_data = data;
	struct smb_charger *chg = irq_data->parent_data;
	u8 stat;
	bool attached = false;
	int rc;

	/* IRQ not expected to be executed for uUSB, return */
	if (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB)
		return IRQ_HANDLED;

	smblite_lib_dbg(chg, PR_INTERRUPT, "IRQ: %s\n", irq_data->name);

	rc = smblite_lib_read(chg, TYPE_C_STATE_MACHINE_STATUS_REG, &stat);
	if (rc < 0) {
		smblite_lib_err(chg, "Couldn't read TYPE_C_STATE_MACHINE_STATUS_REG rc=%d\n",
			rc);
		return IRQ_HANDLED;
	}

	attached = !!(stat & TYPEC_ATTACH_DETACH_STATE_BIT);

	if (attached) {
		rc = smblite_lib_read(chg, TYPE_C_MISC_STATUS_REG, &stat);
		if (rc < 0) {
			smblite_lib_err(chg, "Couldn't read TYPE_C_MISC_STATUS_REG rc=%d\n",
				rc);
			return IRQ_HANDLED;
		}

		if (smblite_lib_get_prop_dfp_mode(chg) ==  //音频设备
				POWER_SUPPLY_TYPEC_SINK_AUDIO_ADAPTER) {
			chg->sink_src_mode = AUDIO_ACCESS_MODE;
			typec_ra_ra_insertion(chg);
		} else if (stat & SNK_SRC_MODE_BIT) {
			chg->sink_src_mode = SRC_MODE;
			typec_sink_insertion(chg); //sink插入
		} else {
			chg->sink_src_mode = SINK_MODE;
			typec_src_insertion(chg); //source插入
		}

		rc = typec_partner_register(chg);
		if (rc < 0)
			smblite_lib_err(chg, "Couldn't to register partner rc =%d\n",
					rc);
	} else {
		switch (chg->sink_src_mode) {
		case SRC_MODE:
			typec_sink_removal(chg); //sink移除
			break;
		case SINK_MODE:
		case AUDIO_ACCESS_MODE:
			typec_src_removal(chg); //SINK、音频设备移除
			break;
		case UNATTACHED_MODE:
		default:
			typec_mode_unattached(chg);  //默认移除
			break;
		}

		if (!chg->pr_swap_in_progress)
			chg->sink_src_mode = UNATTACHED_MODE;

		/*
		 * Restore DRP mode on type-C cable disconnect if role
		 * swap is not in progress, to ensure forced sink or src
		 * mode configuration is reset properly.
		 */

		if (chg->typec_port && !chg->pr_swap_in_progress) {
			/*
			 * Schedule the work to differentiate actual removal
			 * of cable and detach interrupt during role swap,
			 * unregister the partner only during actual cable
			 * removal.
			 */
			cancel_delayed_work(&chg->pr_swap_detach_work);
			vote(chg->awake_votable, DETACH_DETECT_VOTER, true, 0);
			schedule_delayed_work(&chg->pr_swap_detach_work,  //dettach工作队列
				msecs_to_jiffies(TYPEC_DETACH_DETECT_DELAY_MS));
			smblite_lib_force_dr_mode(chg, TYPEC_PORT_DRP); //强制进入DRP模式

			/*
			 * To handle cable removal during role
			 * swap failure.
			 */
			chg->typec_role_swap_failed = false;
		}
	}

	rc = smblite_lib_masked_write(chg, USB_CMD_PULLDOWN_REG,
			EN_PULLDOWN_USB_IN_BIT,
			attached ?  0 : EN_PULLDOWN_USB_IN_BIT);
	if (rc < 0)
		smblite_lib_err(chg, "Couldn't configure pulldown on USB_IN rc=%d\n",
				rc);

	power_supply_changed(chg->usb_psy);

	return IRQ_HANDLED;
}
```

看一下source接入流程,走的是`typec_src_insertion(chg)`，从下面可以看到主要是调用extcon驱动去切换usb状态的:

```C++
* typec_src_insertion
  * smblite_lib_notify_device_mode(chg, true);
    * smblite_lib_notify_extcon_props(chg, EXTCON_USB);
      * extcon_set_property(chg->extcon, id,EXTCON_PROP_USB_TYPEC_POLARITY, val);
      * extcon_set_property(chg->extcon, id,EXTCON_PROP_USB_SS, val);
    * extcon_set_state_sync(chg->extcon, EXTCON_USB, enable);

static void typec_src_insertion(struct smb_charger *chg)
{
	int rc = 0;
	u8 stat;

	smblite_lib_err(chg, "wugn test %s ", __func__);
	smblite_lib_notify_device_mode(chg, true);
	if (chg->pr_swap_in_progress) {
		vote(chg->usb_icl_votable, SW_ICL_MAX_VOTER, false, 0);
		return;
	}

	rc = smblite_lib_read(chg, LEGACY_CABLE_STATUS_REG, &stat);
	if (rc < 0) {
		smblite_lib_err(chg, "Couldn't read TYPE_C_STATE_MACHINE_STATUS_REG rc=%d\n",
					rc);
		return;
	}

	chg->typec_legacy = stat & TYPEC_LEGACY_CABLE_STATUS_BIT;

}

static void smblite_lib_notify_device_mode(struct smb_charger *chg, bool enable)
{
	smblite_lib_err(chg, "wugn test %s enable = %d", __func__, enable);
	if (enable)
		smblite_lib_notify_extcon_props(chg, EXTCON_USB);

	extcon_set_state_sync(chg->extcon, EXTCON_USB, enable);
}

static void smblite_lib_notify_extcon_props(struct smb_charger *chg, int id)
{
	union extcon_property_value val;
	union power_supply_propval prop_val;

	if (chg->connector_type == POWER_SUPPLY_CONNECTOR_TYPEC) {
		smblite_lib_get_prop_typec_cc_orientation(chg, &prop_val);
		val.intval = ((prop_val.intval == 2) ? 1 : 0);
		smblite_lib_err(chg, "wugn test %s val.intval = %d", __func__, val.intval);
		extcon_set_property(chg->extcon, id,
				EXTCON_PROP_USB_TYPEC_POLARITY, val);
		val.intval = true;
		extcon_set_property(chg->extcon, id,
				EXTCON_PROP_USB_SS, val);
	} else if (chg->connector_type == POWER_SUPPLY_CONNECTOR_MICRO_USB) {
		val.intval = false;
		extcon_set_property(chg->extcon, id,
				EXTCON_PROP_USB_SS, val);
	}
}
```



## typec插入电脑实例分析

以下是开发板typec接入电脑，所有打印：

```
[ 6639.687020] PM2250_charger: smblite_lib_usb_plugin_locked: IRQ: usbin-plugin attached
[ 6639.687083] PM2250_charger: smblite_usbin_uv_irq_handler: IRQ: usbin-uv
[ 6639.687272] PM2250_charger: smblite_temp_change_irq_handler: IRQ: temp-change
[ 6639.687739] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6639.688225] PM2250_charger: smblite_lib_get_prop_typec_power_role: TYPE_C_MODE_CFG_REG = 0x10
[ 6639.688246] PM2250_charger: smblite_lib_get_prop_typec_cc_orientation: TYPE_C_STATUS_4 = 0xa0
[ 6639.688267] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6639.688305] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6639.688324] PM2250_charger: smblite_lib_get_prop_ufp_mode: TYPE_C_STATUS_1 = 0x00
[ 6639.689325] PM2250_charger: smblite_lib_get_prop_typec_mode: TYPE_C_MISC_STATUS_REG = 0xa9
[ 6639.689343] PM2250_charger: smblite_lib_get_prop_ufp_mode: TYPE_C_STATUS_1 = 0x08
[ 6639.689348] PM2250_charger: smblite_typec_state_change_irq_handler: IRQ: cc-state-change; Type-C SOURCE_DEFAULT detected
[ 6639.689816] PM2250_charger: smblite_typec_attach_detach_irq_handler: IRQ: typec-attach-detach
[ 6639.689864] PM2250_charger: smblite_lib_get_prop_dfp_mode: TYPE_C_SRC_STATUS_REG = 0x00
[ 6639.689890] PM2250_charger: smblite_lib_get_prop_typec_cc_orientation: TYPE_C_STATUS_4 = 0xa9
[ 6639.689987] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6639.690084] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6639.690469] PM2250_charger: smblite_lib_get_prop_typec_mode: TYPE_C_MISC_STATUS_REG = 0xa9
[ 6639.690485] PM2250_charger: smblite_lib_get_prop_ufp_mode: TYPE_C_STATUS_1 = 0x08
[ 6639.691473] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6639.693269] PM2250_charger: smblite_lib_get_prop_typec_power_role: TYPE_C_MODE_CFG_REG = 0x10
[ 6639.693291] PM2250_charger: smblite_lib_get_prop_typec_cc_orientation: TYPE_C_STATUS_4 = 0xa9
[ 6639.693314] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6639.693345] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6639.693361] PM2250_charger: smblite_lib_get_prop_ufp_mode: TYPE_C_STATUS_1 = 0x08
[ 6639.693827] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6639.694631] PM2250_charger: smblite_lib_get_prop_typec_power_role: TYPE_C_MODE_CFG_REG = 0x10
[ 6639.694653] PM2250_charger: smblite_lib_get_prop_typec_cc_orientation: TYPE_C_STATUS_4 = 0xa9
[ 6639.694676] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6639.694709] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6639.694726] PM2250_charger: smblite_lib_get_prop_ufp_mode: TYPE_C_STATUS_1 = 0x08
[ 6639.696035] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6639.980128] smcinvoke: process_accept_req: process_accept_req txn 1 either invalid or removed from Q
[ 6639.980279] smcinvoke: process_accept_req: process_accept_req txn 2 either invalid or removed from Q
[ 6639.981490] smcinvoke: process_accept_req: process_accept_req txn 2 either invalid or removed from Q
[ 6639.981495] smcinvoke: process_accept_req: accept thread returning with ret: -11
[ 6639.998940] smcinvoke: process_accept_req: process_accept_req txn 1 either invalid or removed from Q
[ 6640.004458] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.009813] smcinvoke: process_accept_req: accept thread returning with ret: -11
[ 6640.012532] smcinvoke: process_accept_req: accept thread returning with ret: -11
[ 6640.017664] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6640.019914] smcinvoke: process_accept_req: accept thread returning with ret: -11
[ 6640.048726] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6640.048857] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.074229] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.085110] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6640.099793] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.109845] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6640.123542] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6640.124596] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6640.132131] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.140665] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.158574] PM2250_charger: smblite_lib_get_charge_param: input current limit status = 0 (0x00)
[ 6640.160237] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6640.167752] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.175480] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.202475] PM2250_charger: smblite_lib_set_prop_usb_type: Charger type request form USB driver type=4
[ 6640.221359] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6640.230087] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.239609] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6640.249315] msm-usb-ssphy-qmp 1615000.ssphy: USB QMP PHY: Update TYPEC CTRL(2)
[ 6640.257137] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6640.265199] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6640.274088] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6640.473092] audit: rate limit exceeded
[ 6640.477369] PM2250_charger: smblite_lib_set_prop_current_max: Current request from USB driver current=100000mA
[ 6640.487732] PM2250_charger: smblite_lib_get_prop_typec_mode: TYPE_C_MISC_STATUS_REG = 0xa9
[ 6640.497358] PM2250_charger: smblite_lib_get_prop_ufp_mode: TYPE_C_STATUS_1 = 0x08
[ 6640.577853] PM2250_charger: smblite_lib_set_prop_current_max: Current request from USB driver current=2000mA
[ 6640.588293] PM2250_charger: smblite_lib_get_prop_typec_mode: TYPE_C_MISC_STATUS_REG = 0xa9
[ 6640.601368] PM2250_charger: smblite_lib_get_prop_ufp_mode: TYPE_C_STATUS_1 = 0x08
[ 6640.615778] PM2250_charger: smblite_lib_set_prop_current_max: Current request from USB driver current=500000mA
[ 6640.626115] PM2250_charger: smblite_lib_get_prop_typec_mode: TYPE_C_MISC_STATUS_REG = 0xa9
[ 6640.634749] PM2250_charger: smblite_lib_get_prop_ufp_mode: TYPE_C_STATUS_1 = 0x08
[ 6640.712161] PM2250_charger: smblite_lib_thermal_regulation_work: exiting DIE_TEMP regulation work DIE_TEMP_STATUS=0 icl=0uA
[ 6641.061951] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6641.073296] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6641.082930] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6641.106458] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6641.115249] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6641.124539] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6641.717537] audit: rate limit exceeded
[ 6646.773812] audit: rate limit exceeded
[ 6646.806686] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6646.815168] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6646.827090] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6646.851361] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6646.860324] PM2250_charger: smblite_lib_get_prop_batt_health: CHARGER_VBAT_STATUS_REG = 0x00
[ 6646.869781] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6646.929746] PM2250_charger: smblite_lib_get_prop_usb_online: POWER_PATH_STATUS = 0x29
[ 6655.736955] audit: rate limit exceeded
[ 6660.746479] audit: rate limit exceeded
[ 6665.755519] audit: rate limit exceeded
```