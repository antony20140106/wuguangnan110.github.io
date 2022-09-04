# 概述
mt6762平台如何设置开机默认host功能

## 软件分析

* 1.typec默认设置为`Try.SRC`模式。
* 2.初始化处理，包括usb（mt_usb_otg_init）及usb_switch（pax_tcpc_dev_init），这里都是驱动初始化走一遍。
* 3.CC状态回调切换逻辑`otg_tcp_notifier_call`处理。

```diff
From 030762fb27163d196409abcdca2dd2a963ec7a15 Mon Sep 17 00:00:00 2001
From: shanliangliang <shanliangliang@paxsz.com>
Date: Sun, 15 Aug 2021 15:10:03 +0800
Subject: [PATCH] =?UTF-8?q?[Title]:=20=E5=A2=9E=E5=8A=A0M8=20usb=20otg?=
 =?UTF-8?q?=E5=8A=9F=E8=83=BD?=
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit

[Summary]: 1. 增加M8 usb otg功能
2. 默认为host模式

[Test Plan]: usb otg功能正常

[Module]: USB

[Model]: M8

[author]: shanliangliang@paxsz.com

[date]: 2021-08-15
---
 .../arch/arm64/boot/dts/mediatek/M8.dts       |   5 +
 .../arm64/boot/dts/mediatek/mt6370_pd_m8.dtsi |   4 +
 .../misc/mediatek/usb20/mt6765/usb20.c        |   6 +
 .../misc/mediatek/usb20/mt6765/usb20_host.c   |  37 ++++-
 .../drivers/misc/mediatek/usb20/musb.h        |   5 +
 .../drivers/misc/mediatek/usb20/musb_core.c   |   3 +
 .../drivers/misc/mediatek/usb20/musb_core.h   |   3 +
 .../drivers/misc/pax/usb_switch/Makefile      |   2 +
 .../misc/pax/usb_switch/pax_usb_switch.c      | 143 +++++++++++++++++-
 9 files changed, 198 insertions(+), 10 deletions(-)

diff --git a/kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts b/kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts
index 82c2802e29d..38b81ea25d8 100755
--- a/kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts
+++ b/kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts
@@ -626,6 +626,11 @@
 	gl850_en_gpio = <&pio 151 0x0>;
 	otg_en_gpio  = <&pio 7 0x0>;
 	vcc_out_en_gpio  = <&pio 152 0x0>;
+	default_mode = <1>;  //0-device mode, 1-host mode
+};
+
+&usb {
+	default_mode = <1>;
 };
 
 &charger {
diff --git a/kernel-4.19/arch/arm64/boot/dts/mediatek/mt6370_pd_m8.dtsi b/kernel-4.19/arch/arm64/boot/dts/mediatek/mt6370_pd_m8.dtsi
index b83447ec585..46ae1628a36 100755
--- a/kernel-4.19/arch/arm64/boot/dts/mediatek/mt6370_pd_m8.dtsi
+++ b/kernel-4.19/arch/arm64/boot/dts/mediatek/mt6370_pd_m8.dtsi
@@ -4,3 +4,7 @@
  * Author: Owen Chen <owen.chen@mediatek.com>
  */
 
+&mt6370_typec {
+	/* 0: SNK Only, 1: SRC Only, 2: DRP, 3: Try.SRC, 4: Try.SNK */
+	mt-tcpc,role_def = <3>;
+};
diff --git a/kernel-4.19/drivers/misc/mediatek/usb20/mt6765/usb20.c b/kernel-4.19/drivers/misc/mediatek/usb20/mt6765/usb20.c
index c0a5772aef4..cf63a8f6dc7 100644
--- a/kernel-4.19/drivers/misc/mediatek/usb20/mt6765/usb20.c
+++ b/kernel-4.19/drivers/misc/mediatek/usb20/mt6765/usb20.c
@@ -1713,6 +1713,12 @@ static int mt_usb_probe(struct platform_device *pdev)
 	of_property_read_u32(np, "mode", (u32 *) &pdata->mode);
 #endif
 
+	/* Add-BEGIN by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+	ret = of_property_read_u32(np, "default_mode", (u32 *) &pdata->default_mode);
+	if (ret < 0)
+		pdata->default_mode = MUSB_PERIPHERAL;
+	/* Add-END by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+
 #ifdef CONFIG_MTK_UART_USB_SWITCH
 	ap_gpio_node =
 		of_find_compatible_node(NULL, NULL, AP_GPIO_COMPATIBLE_NAME);
diff --git a/kernel-4.19/drivers/misc/mediatek/usb20/mt6765/usb20_host.c b/kernel-4.19/drivers/misc/mediatek/usb20/mt6765/usb20_host.c
index 794465f9b1f..c6a346bfc2a 100644
--- a/kernel-4.19/drivers/misc/mediatek/usb20/mt6765/usb20_host.c
+++ b/kernel-4.19/drivers/misc/mediatek/usb20/mt6765/usb20_host.c
@@ -322,13 +322,31 @@ static int otg_tcp_notifier_call(struct notifier_block *nb,
 			noti->typec_state.old_state ==
 					TYPEC_ATTACHED_NORP_SRC) &&
 			noti->typec_state.new_state == TYPEC_UNATTACHED) {
-			if (is_host_active(mtk_musb)) {
-				DBG(0, "OTG Plug out\n");
-				mt_usb_host_disconnect(0);
-			} else {
-				DBG(0, "USB Plug out\n");
-				mt_usb_dev_disconnect();
-			}
+			/* Add-BEGIN by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+				if (mtk_musb->default_mode != MUSB_HOST) {
+					if (is_host_active(mtk_musb)) {
+						DBG(0, "OTG Plug out\n");
+						mt_usb_host_disconnect(0);
+					} else {
+						DBG(0, "USB Plug out\n");
+						mt_usb_dev_disconnect();
+					}
+				}
+				else { //默认host
+					if ((noti->typec_state.old_state != TYPEC_ATTACHED_SRC) && 
+							(noti->typec_state.new_state == TYPEC_UNATTACHED)) {
+						if (is_host_active(mtk_musb)) {
+							DBG(0, "OTG Plug out\n");
+							mt_usb_host_disconnect(0);
+						} else { //1.拔出前是device，先断device
+							DBG(0, "USB Plug out\n");
+							mt_usb_dev_disconnect();
+						}
+						msleep(50);
+						mt_usb_host_connect(100); //2.还原成host
+					}
+				}
+		/* Add-END by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
 #ifdef CONFIG_MTK_UART_USB_SWITCH
 		} else if ((noti->typec_state.new_state ==
 					TYPEC_ATTACHED_SNK ||
@@ -743,6 +761,11 @@ void mt_usb_otg_init(struct musb *musb)
 	musb->fifo_cfg_host = fifo_cfg_host;
 	musb->fifo_cfg_host_size = ARRAY_SIZE(fifo_cfg_host);
 
+	/* Add-BEGIN by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+	if (mtk_musb->default_mode == MUSB_HOST)
+		mt_usb_host_connect(0);   // 初始化成host模式
+	/* Add-END by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+
 }
 void mt_usb_otg_exit(struct musb *musb)
 {
diff --git a/kernel-4.19/drivers/misc/mediatek/usb20/musb.h b/kernel-4.19/drivers/misc/mediatek/usb20/musb.h
index eb9c4f389f8..1cdf864764f 100644
--- a/kernel-4.19/drivers/misc/mediatek/usb20/musb.h
+++ b/kernel-4.19/drivers/misc/mediatek/usb20/musb.h
@@ -104,6 +104,11 @@ struct musb_hdrc_platform_data {
 	/* MUSB_HOST, MUSB_PERIPHERAL, or MUSB_OTG */
 	u8 mode;
 
+	/* Add-BEGIN by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+	/* MUSB_HOST, MUSB_PERIPHERAL */
+	u8 default_mode;
+	/* Add-END by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+
 	/* for clk_get() */
 	const char *clock;
 
diff --git a/kernel-4.19/drivers/misc/mediatek/usb20/musb_core.c b/kernel-4.19/drivers/misc/mediatek/usb20/musb_core.c
index 747b3540495..5f7f4fb72d4 100644
--- a/kernel-4.19/drivers/misc/mediatek/usb20/musb_core.c
+++ b/kernel-4.19/drivers/misc/mediatek/usb20/musb_core.c
@@ -2407,6 +2407,9 @@ static int musb_init_controller
 	musb->board_set_power = plat->set_power;
 	musb->min_power = plat->min_power;
 	musb->ops = plat->platform_ops;
+	/* Add-BEGIN by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+	musb->default_mode = plat->default_mode;
+	/* Add-END by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
 	musb->nIrq = nIrq;
 	/* The musb_platform_init() call:
 	 *   - adjusts musb->mregs
diff --git a/kernel-4.19/drivers/misc/mediatek/usb20/musb_core.h b/kernel-4.19/drivers/misc/mediatek/usb20/musb_core.h
index d6a8291f762..95010c19d00 100644
--- a/kernel-4.19/drivers/misc/mediatek/usb20/musb_core.h
+++ b/kernel-4.19/drivers/misc/mediatek/usb20/musb_core.h
@@ -494,6 +494,9 @@ struct musb {
 #endif /* CONFIG_DUAL_ROLE_USB_INTF */
 	struct power_supply *usb_psy;
 	struct notifier_block psy_nb;
+/* Add-BEGIN by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
+	u8 default_mode;
+/* Add-END by (shanliangliang@paxsz.com), 2021/08/15 add for M8 usb otg */
 };
 
 static inline struct musb *gadget_to_musb(struct usb_gadget *g)
diff --git a/kernel-4.19/drivers/misc/pax/usb_switch/Makefile b/kernel-4.19/drivers/misc/pax/usb_switch/Makefile
index 4a5627c3025..0c3e640a3f8 100755
--- a/kernel-4.19/drivers/misc/pax/usb_switch/Makefile
+++ b/kernel-4.19/drivers/misc/pax/usb_switch/Makefile
@@ -1,5 +1,7 @@
 ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
 ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include/
+ccflags-y += -I$(srctree)/drivers/misc/mediatek/typec/tcpc/inc
+
 obj-y += pax_usb_switch.o
 
 ccflags-y += -Wno-unused-function
diff --git a/kernel-4.19/drivers/misc/pax/usb_switch/pax_usb_switch.c b/kernel-4.19/drivers/misc/pax/usb_switch/pax_usb_switch.c
index 840c39fd0d0..3fdf37eacc2 100755
--- a/kernel-4.19/drivers/misc/pax/usb_switch/pax_usb_switch.c
+++ b/kernel-4.19/drivers/misc/pax/usb_switch/pax_usb_switch.c
@@ -19,10 +19,16 @@
 #include <linux/interrupt.h>
 #include <mt-plat/mtk_boot.h>
 
+#include "tcpm.h"
+
 struct usb_switch_data {
 	struct class *pax_class;
 	struct device *dev;
 
+	struct tcpc_device *tcpc_dev;
+	struct notifier_block tcpc_nb;
+	struct delayed_work tcpc_dwork;
+
 	struct mutex lock;
 	struct delayed_work usb_switch_work;
 
@@ -33,6 +39,8 @@ struct usb_switch_data {
 	int otg_en_gpio;
 	int vcc_out_en_gpio;
 	int status;
+
+	u32 default_mode;
 };
 
 extern struct class *g_class_pax;
@@ -77,6 +85,7 @@ static int check_boot_mode(struct device *dev)
  *	usb plug in : sw1sw2 = 00(USB1 = OTG USB2 = CLOSE) host_en = 0  gl850_en = 0
  *  usb plug out : sw1sw2 = 11(USB1 = HOST USB2 = HOST) host_en = 1  gl850_en = 1
  */
+#if 0
 static irqreturn_t pax_otg_gpio_irq_handle(int irq, void *dev_id)
 {
 	struct usb_switch_data *data = (struct usb_switch_data *)dev_id;
@@ -102,12 +111,14 @@ static irqreturn_t pax_otg_gpio_irq_handle(int irq, void *dev_id)
 	}
 	return IRQ_HANDLED;	
 }
+#endif
 
 extern const char *cmdline_get_value(const char *key);
 
 int pax_charger_gpio_init(struct usb_switch_data *data, struct device_node *np)
 {
-	int ret,irq;
+	int ret;
+	//int irq;
 	data->usb_host_en_gpio = of_get_named_gpio(np, "usb_host_en_gpio", 0);
 	ret = gpio_request(data->usb_host_en_gpio, "usb_host_en_gpio");	
 	if (ret < 0) {
@@ -148,6 +159,11 @@ int pax_charger_gpio_init(struct usb_switch_data *data, struct device_node *np)
 		goto init_alert_err;
 	}
 
+	ret = of_property_read_u32(np, "default_mode", (u32 *)&data->default_mode);
+	if (ret < 0)
+		data->default_mode = 0;
+
+#if 0
 	data->otg_en_gpio = of_get_named_gpio(np, "otg_en_gpio", 0);
 	ret = gpio_request(data->otg_en_gpio, "otg_en_gpio");
 	ret = gpio_direction_input(data->otg_en_gpio);
@@ -159,13 +175,15 @@ int pax_charger_gpio_init(struct usb_switch_data *data, struct device_node *np)
 		data->otg_en_gpio, ret);
 		goto init_alert_err;
 	}
-
+#endif
 
 	return 0;
+
 init_alert_err:
 	return -EINVAL;
 }
 
+#if 0
 /**
  *     When starting the system, if there is an enumerated device on the usb port, 
  *     a gpio initialization is required.
@@ -192,6 +210,7 @@ void pax_charger_usbswitch_set(struct usb_switch_data *data)
 		data->status = 1;
 	}
 }
+#endif
 
 #define USB_SWITCH_TIME 300
 static void do_usb_switch_work(struct work_struct *work)
@@ -256,6 +275,121 @@ int pax_usb_switch_poweoff_charging_mode(struct device *dev)
 	return 0;
 }
 
+static void prog_vbus_en(struct usb_switch_data *data, int on)
+{
+	if (gpio_is_valid(data->vcc_out_en_gpio))
+		gpio_set_value(data->vcc_out_en_gpio, 1);
+}
+
+static void vbus_en(struct usb_switch_data *data, int on)
+{
+	if (gpio_is_valid(data->usb_host_en_gpio))
+		gpio_set_value(data->usb_host_en_gpio, 0);
+}
+
+static void usb_host_switch(struct usb_switch_data *data, int on)
+{
+	pr_info("%s: on:%d\n", __func__, on);
+	if (on) {
+		if (gpio_is_valid(data->gl850_en_gpio))
+			gpio_set_value(data->gl850_en_gpio, 1);
+		if (gpio_is_valid(data->usb_sw1_sel_gpio))
+			gpio_set_value(data->usb_sw1_sel_gpio, 1);
+		if (gpio_is_valid(data->usb_sw2_sel_gpio))
+			gpio_set_value(data->usb_sw2_sel_gpio, 1);
+
+		prog_vbus_en(data, 1);
+	}
+	else {
+		if (gpio_is_valid(data->gl850_en_gpio))
+			gpio_set_value(data->gl850_en_gpio, 0);
+		if (gpio_is_valid(data->usb_sw1_sel_gpio))
+			gpio_set_value(data->usb_sw1_sel_gpio, 0);
+		if (gpio_is_valid(data->usb_sw2_sel_gpio))
+			gpio_set_value(data->usb_sw2_sel_gpio, 0);
+
+		prog_vbus_en(data, 0);
+	}
+}
+
+static int tcpc_notifier_call(struct notifier_block *nb,  unsigned long action, void *noti_data)
+{
+	struct tcp_notify *noti = noti_data;
+	struct usb_switch_data *data = (struct usb_switch_data *)container_of(nb, struct usb_switch_data, tcpc_nb);
+
+	pr_info("%s: action:%d\n", __func__, action);;
+
+	switch(action) {
+		case TCP_NOTIFY_SOURCE_VBUS:
+			vbus_en(data, !!noti->vbus_state.mv);
+			break;
+		case TCP_NOTIFY_TYPEC_STATE:
+			if ((noti->typec_state.old_state == TYPEC_UNATTACHED) &&
+					(noti->typec_state.new_state == TYPEC_ATTACHED_SRC)) { //usb插入，M8识别host try_src，切host
+				usb_host_switch(data, 1);   
+			}
+			else if ((noti->typec_state.old_state == TYPEC_UNATTACHED) &&
+					(noti->typec_state.new_state == TYPEC_ATTACHED_SNK)) {//usb插入，M8识别device try_snk，切device
+				usb_host_switch(data, 0);
+			}
+			else if ((noti->typec_state.old_state != TYPEC_UNATTACHED) &&
+					(noti->typec_state.new_state == TYPEC_UNATTACHED)) { //usb拔出，m8切换成默认状态，目前默认是host
+					usb_host_switch(data, !!data->default_mode);
+			}
+
+			break;
+		case TCP_NOTIFY_DR_SWAP: //数据role
+			if (noti->swap_state.new_role == PD_ROLE_UFP) {  //UFP相当于device
+				usb_host_switch(data, 0);
+			}
+			else if (noti->swap_state.new_role == PD_ROLE_DFP) { //DFP相当于host
+				usb_host_switch(data, 1);
+			}
+			break;
+
+		case TCP_NOTIFY_PR_SWAP: //power role
+			if (noti->swap_state.new_role == PD_ROLE_SINK) { //当作为device时关闭vbus
+				vbus_en(data, 0);
+			}
+			else if (noti->swap_state.new_role == PD_ROLE_SOURCE) {//当作为host时打开vbus
+				vbus_en(data, 1);
+			}
+			break;
+	}
+
+	return NOTIFY_OK;
+}
+
+static void pax_tcpc_dev_init_work(struct work_struct *work)
+{
+	int ret = 0;
+	struct usb_switch_data *data = (struct usb_switch_data *)container_of(work, struct usb_switch_data, tcpc_dwork.work);
+
+	if (!data->tcpc_dev) {
+		data->tcpc_dev = tcpc_dev_get_by_name("type_c_port0");
+		if (!data->tcpc_dev) {
+			schedule_delayed_work(&data->tcpc_dwork, msecs_to_jiffies(200));
+			return;
+		}
+	}
+
+	data->tcpc_nb.notifier_call = tcpc_notifier_call;
+	ret = register_tcp_dev_notifier(data->tcpc_dev, &data->tcpc_nb,
+			TCP_NOTIFY_TYPE_VBUS | TCP_NOTIFY_TYPE_USB | TCP_NOTIFY_TYPE_MISC); //注册监听cc、vbus、misc(DR/PR)
+	if (ret < 0) {
+		schedule_delayed_work(&data->tcpc_dwork, msecs_to_jiffies(200));
+		return;
+	}
+}
+
+static void pax_tcpc_dev_init(struct usb_switch_data *data)
+{
+	usb_host_switch(data, !!data->default_mode); //开机默认切换一次
+
+	INIT_DELAYED_WORK(&data->tcpc_dwork, pax_tcpc_dev_init_work);
+	schedule_delayed_work(&data->tcpc_dwork, 0);
+}
+
 extern const char *cmdline_get_value(const char *key);
 static int pax_usb_swtich_probe(struct platform_device *pdev)
 {
@@ -287,8 +421,10 @@ static int pax_usb_swtich_probe(struct platform_device *pdev)
 	if (ret) {
 		goto req_res_fail;
 	}
+
+	pax_tcpc_dev_init(data);
 	
-	pax_charger_usbswitch_set(data);
+	//pax_charger_usbswitch_set(data);
 
 	if (g_class_pax != NULL) {
 		data->pax_class = g_class_pax;
@@ -310,6 +446,7 @@ static int pax_usb_swtich_probe(struct platform_device *pdev)
 
 	//schedule_delayed_work(&data->usb_switch_work, msecs_to_jiffies(time_interval));
 	
+	pr_info("%s: success.\n", __func__);
 	return 0;
 
 req_res_fail:
-- 
2.17.1
```