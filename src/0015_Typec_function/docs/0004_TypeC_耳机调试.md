# 概述

MTK TypeC 耳机补丁，拿别人提交的过来分析。 

## 参考

* [mtk平台typec模拟耳机补丁](https://blog.csdn.net/mike8825/article/details/112062132)
* [Android 耳机驱动知识](https://yunzhi.github.io/headset_knowledge)

## 补丁

```diff
From 7a3c41dd0bf70e28dd58a987c3980d337a725d82 Mon Sep 17 00:00:00 2001
From: xxx <xxx@xxxxx.com>
Date: Mon, 26 Jul 2021 13:44:59 +0800
Subject: [PATCH] =?UTF-8?q?[Title]:=20M8=E5=A2=9E=E5=8A=A0typec=E8=80=B3?=
 =?UTF-8?q?=E6=9C=BA=E6=94=AF=E6=8C=81(=E5=85=BC=E5=AE=B93.5mm=E8=80=B3?=
 =?UTF-8?q?=E6=9C=BA)?=
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit

[Summary]: M8增加typec耳机支持(兼容3.5mm耳机，不能同时插)

[Test Plan]: NO

[Module]: 请选择net/system/api/sp/other

[Model]: M8

[author]: xxx@xxxxx.com

[date]: 2021-07-26
---
 .../k62v1_64_xxxxx/BoardConfig.mk               |   3 +-
 .../arch/arm64/boot/dts/mediatek/M8.dts       |   6 +
 .../arch/arm64/boot/dts/mediatek/mt6765.dts   |   4 +
 .../configs/k62v1_64_xxxxx_debug_defconfig      |   2 +
 .../arch/arm64/configs/k62v1_64_xxxxx_defconfig |   2 +
 .../misc/mediatek/typec/tcpc/rt_pd_manager.c  |  25 ++
 kernel-4.19/drivers/misc/xxxxx/Kconfig          |   3 +-
 kernel-4.19/drivers/misc/xxxxx/Makefile         |   5 +-
 .../drivers/misc/xxxxx/audio_switch/Kconfig     |   4 +
 .../drivers/misc/xxxxx/audio_switch/Makefile    |   3 +
 .../xxxxx/audio_switch/typec_audio_switch.c     | 236 ++++++++++++++++++
 kernel-4.19/sound/soc/codecs/mt6357-accdet.c  |  55 ++++
 12 files changed, 344 insertions(+), 4 deletions(-)
 mode change 100644 => 100755 kernel-4.19/drivers/misc/mediatek/typec/tcpc/rt_pd_manager.c
 create mode 100755 kernel-4.19/drivers/misc/xxxxx/audio_switch/Kconfig
 create mode 100755 kernel-4.19/drivers/misc/xxxxx/audio_switch/Makefile
 create mode 100755 kernel-4.19/drivers/misc/xxxxx/audio_switch/typec_audio_switch.c
 mode change 100644 => 100755 kernel-4.19/sound/soc/codecs/mt6357-accdet.c

diff --git a/device/mediateksample/k62v1_64_xxxxx/BoardConfig.mk b/device/mediateksample/k62v1_64_xxxxx/BoardConfig.mk
index ac83324dabc..4c8ab82af52 100755
--- a/device/mediateksample/k62v1_64_xxxxx/BoardConfig.mk
+++ b/device/mediateksample/k62v1_64_xxxxx/BoardConfig.mk
@@ -48,8 +48,7 @@ BOARD_FLASH_BLOCK_SIZE := 4096
 KERNEL_OUT ?= $(OUT_DIR)/target/project/$(TARGET_DEVICE)/obj/KERNEL_OBJ
 # in-tree kernel modules installed to vendor
 # For Common
-BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_OUT)/sound/soc/codecs/mt6357-accdet.ko \
-			       $(KERNEL_OUT)/kernel/trace/trace_mmstat.ko \
+BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_OUT)/kernel/trace/trace_mmstat.ko
 # For Camera
 BOARD_VENDOR_KERNEL_MODULES +=
 # For Wifi
diff --git a/kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts b/kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts
index 077320514b8..df558fae089 100755
--- a/kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts
+++ b/kernel-4.19/arch/arm64/boot/dts/mediatek/M8.dts
@@ -716,6 +716,12 @@
 	status = "okay";
 };
 
+&xxxxx_typec_audio_switch {
+	audio-sel-gpio = <&pio 166 0>;
+	audio-sel-level = <1>;
+	mic-sel-gpio = <&pio 178 0>;
+	status = "okay";
+};
 
 #include "mediatek/mt6370_m8.dtsi"
 #include "mediatek/mt6370_pd_m8.dtsi"
diff --git a/kernel-4.19/arch/arm64/boot/dts/mediatek/mt6765.dts b/kernel-4.19/arch/arm64/boot/dts/mediatek/mt6765.dts
index 33538eb5a7f..4d6d5f2b598 100755
--- a/kernel-4.19/arch/arm64/boot/dts/mediatek/mt6765.dts
+++ b/kernel-4.19/arch/arm64/boot/dts/mediatek/mt6765.dts
@@ -3856,6 +3856,10 @@ firmware_class.path=/vendor/firmware";
 	xxxxx_usb_switch:xxxxx_usb_switch {
 		compatible = "xxxxx,usb_switch";
 	};
+
+	xxxxx_typec_audio_switch:xxxxx_typec_audio_switch {
+		compatible = "xxxxx,typec_audio_switch";
+	};
 };
 
 /*betterlife finger add */
diff --git a/kernel-4.19/arch/arm64/configs/k62v1_64_xxxxx_debug_defconfig b/kernel-4.19/arch/arm64/configs/k62v1_64_xxxxx_debug_defconfig
index d596b658c71..80e931ddca2 100755
--- a/kernel-4.19/arch/arm64/configs/k62v1_64_xxxxx_debug_defconfig
+++ b/kernel-4.19/arch/arm64/configs/k62v1_64_xxxxx_debug_defconfig
@@ -531,3 +531,5 @@ CONFIG_TRUSTKERNEL_TEE_FP_SUPPORT=y
 CONFIG_xxx_CAM_REGULATOR_SUPPORT=y
 CONFIG_SND_SOC_AW87XXX=y
 CONFIG_xxx_USB_SWITCH=y
+CONFIG_SND_SOC_MT6357_ACCDET=y
+CONFIG_xxx_TYPEC_AUDIO_SWITCH=y
diff --git a/kernel-4.19/arch/arm64/configs/k62v1_64_xxxxx_defconfig b/kernel-4.19/arch/arm64/configs/k62v1_64_xxxxx_defconfig
index c0646ff0671..47ed390baf7 100755
--- a/kernel-4.19/arch/arm64/configs/k62v1_64_xxxxx_defconfig
+++ b/kernel-4.19/arch/arm64/configs/k62v1_64_xxxxx_defconfig
@@ -509,3 +509,5 @@ CONFIG_TRUSTKERNEL_TEE_FP_SUPPORT=y
 CONFIG_xxx_CAM_REGULATOR_SUPPORT=y
 CONFIG_SND_SOC_AW87XXX=y
 CONFIG_xxx_USB_SWITCH=y
+CONFIG_SND_SOC_MT6357_ACCDET=y
+CONFIG_xxx_TYPEC_AUDIO_SWITCH=y
\ No newline at end of file
diff --git a/kernel-4.19/drivers/misc/mediatek/typec/tcpc/rt_pd_manager.c b/kernel-4.19/drivers/misc/mediatek/typec/tcpc/rt_pd_manager.c
old mode 100644
new mode 100755
index 21d141bbf78..a118bdf9fb2
--- a/kernel-4.19/drivers/misc/mediatek/typec/tcpc/rt_pd_manager.c
+++ b/kernel-4.19/drivers/misc/mediatek/typec/tcpc/rt_pd_manager.c
@@ -53,6 +53,15 @@ void __attribute__((weak)) usb_dpdm_pulldown(bool enable)
 	pr_notice("%s is not defined\n", __func__);
 }
 
+/* [FEATURE] Add-BEGIN by (xxx@xxxxx.com), 2021/07/21 */
+#ifdef CONFIG_xxx_TYPEC_AUDIO_SWITCH
+extern int xxxxx_switch_select_audio(int sel);
+#endif
+#ifdef CONFIG_SND_SOC_MT6357_ACCDET
+extern void typec_headphone_plug_handler(int state);
+#endif
+/* [FEATURE] Add-END by (xxx@xxxxx.com), 2021/07/21 */
+
 static int pd_tcp_notifier_call(struct notifier_block *nb,
 				unsigned long event, void *data)
 {
@@ -92,10 +101,26 @@ static int pd_tcp_notifier_call(struct notifier_block *nb,
 			noti->typec_state.new_state == TYPEC_ATTACHED_AUDIO) {
 			/* AUDIO plug in */
 			pr_info("%s audio plug in\n", __func__);
+			/* [FEATURE] Add-BEGIN by (xxx@xxxxx.com), 2021/07/21 */
+			#ifdef CONFIG_xxx_TYPEC_AUDIO_SWITCH
+			xxxxx_switch_select_audio(1);
+			#endif
+			#ifdef CONFIG_SND_SOC_MT6357_ACCDET
+			typec_headphone_plug_handler(1);
+			#endif
+			/* [FEATURE] Add-END by (xxx@xxxxx.com), 2021/07/21 */
 		} else if (noti->typec_state.old_state == TYPEC_ATTACHED_AUDIO
 			&& noti->typec_state.new_state == TYPEC_UNATTACHED) {
 			/* AUDIO plug out */
 			pr_info("%s audio plug out\n", __func__);
+			/* [FEATURE] Add-BEGIN by (xxx@xxxxx.com), 2021/07/21 */
+			#ifdef CONFIG_xxx_TYPEC_AUDIO_SWITCH
+			xxxxx_switch_select_audio(0);
+			#endif
+			#ifdef CONFIG_SND_SOC_MT6357_ACCDET
+			typec_headphone_plug_handler(0);
+			#endif
+			/* [FEATURE] Add-END by (xxx@xxxxx.com), 2021/07/21 */
 		}
 		break;
 	case TCP_NOTIFY_PD_STATE:
diff --git a/kernel-4.19/drivers/misc/xxxxx/Kconfig b/kernel-4.19/drivers/misc/xxxxx/Kconfig
index fddfbdd2b0d..c7766c4f322 100755
--- a/kernel-4.19/drivers/misc/xxxxx/Kconfig
+++ b/kernel-4.19/drivers/misc/xxxxx/Kconfig
@@ -6,4 +6,5 @@ config xxx_DRV_SUPPORT
 
 source "drivers/misc/xxxxx/gpio/Kconfig"
 source "drivers/misc/xxxxx/cam_regu/Kconfig"
-source "drivers/misc/xxxxx/usb_switch/Kconfig"
\ No newline at end of file
+source "drivers/misc/xxxxx/usb_switch/Kconfig"
+source "drivers/misc/xxxxx/audio_switch/Kconfig"
\ No newline at end of file
diff --git a/kernel-4.19/drivers/misc/xxxxx/Makefile b/kernel-4.19/drivers/misc/xxxxx/Makefile
index c74ca859888..26b5ceb6426 100755
--- a/kernel-4.19/drivers/misc/xxxxx/Makefile
+++ b/kernel-4.19/drivers/misc/xxxxx/Makefile
@@ -10,4 +10,7 @@ obj-$(CONFIG_xxx_GPIOS_CONTROL)  += gpio/
 # camera module regulator control
 obj-$(CONFIG_xxx_CAM_REGULATOR_SUPPORT)  += cam_regu/
 # xxxxx usb switch
-obj-$(CONFIG_xxx_USB_SWITCH) += usb_switch/
\ No newline at end of file
+obj-$(CONFIG_xxx_USB_SWITCH) += usb_switch/
+
+# xxxxx typec/audio switch
+obj-$(CONFIG_xxx_TYPEC_AUDIO_SWITCH) += audio_switch/
\ No newline at end of file
diff --git a/kernel-4.19/drivers/misc/xxxxx/audio_switch/Kconfig b/kernel-4.19/drivers/misc/xxxxx/audio_switch/Kconfig
new file mode 100755
index 00000000000..6cf0f2b8e44
--- /dev/null
+++ b/kernel-4.19/drivers/misc/xxxxx/audio_switch/Kconfig
@@ -0,0 +1,4 @@
+config xxx_TYPEC_AUDIO_SWITCH
+	tristate "xxxxx typec audio switch control"
+	depends on xxx_DRV_SUPPORT
+	default n
diff --git a/kernel-4.19/drivers/misc/xxxxx/audio_switch/Makefile b/kernel-4.19/drivers/misc/xxxxx/audio_switch/Makefile
new file mode 100755
index 00000000000..adfc0934aa7
--- /dev/null
+++ b/kernel-4.19/drivers/misc/xxxxx/audio_switch/Makefile
@@ -0,0 +1,3 @@
+obj-y += typec_audio_switch.o
+
+#ccflags-y += -Wno-unused-function
diff --git a/kernel-4.19/drivers/misc/xxxxx/audio_switch/typec_audio_switch.c b/kernel-4.19/drivers/misc/xxxxx/audio_switch/typec_audio_switch.c
new file mode 100755
index 00000000000..345f1c31919
--- /dev/null
+++ b/kernel-4.19/drivers/misc/xxxxx/audio_switch/typec_audio_switch.c
@@ -0,0 +1,236 @@
+#include <linux/init.h>
+#include <linux/module.h>
+#include <linux/kernel.h>
+#include <linux/device.h>
+#include <linux/platform_device.h>
+#include <linux/types.h>
+#include <linux/sys.h>
+#include <linux/sysfs.h>
+#include <linux/sched.h>
+#include <linux/delay.h>
+#include <linux/workqueue.h>
+#include <linux/uaccess.h>
+#include <linux/of.h>
+#include <linux/of_gpio.h>
+#include <linux/gpio.h>
+#include <linux/spinlock.h>
+#include <linux/pm_wakeup.h>
+
+
+enum {
+	SWITCH_SEL_USB = 0,
+	SWITCH_SEL_AUDIO = 1,
+};
+
+enum {
+	MIC_SWITCH_SEL_MIN = 0,
+	MIC_SWITCH_SEL_0 = MIC_SWITCH_SEL_MIN,
+	MIC_SWITCH_SEL_1 = 1,
+	MIC_SWITCH_SEL_MAX = MIC_SWITCH_SEL_1,
+};
+
+
+struct typec_audio_switch_data {
+	struct class *xxxxx_class;
+	struct device *dev;
+
+	struct mutex lock;
+
+	int audio_sel_gpio;
+	int audio_sel_gpio_level;
+	int audio_cur_sel;
+
+	int mic_sel_gpio;
+	int mic_cur_sel;
+};
+
+static struct typec_audio_switch_data *gp_audio_switch;
+
+extern struct class *g_class_xxxxx;
+extern const char *cmdline_get_value(const char *key);
+
+static int switch_init(struct typec_audio_switch_data *data, struct device_node *np)
+{
+	int ret;
+	unsigned int value;
+
+	/* dmdp / hprhpl select */
+	data->audio_sel_gpio = of_get_named_gpio(np, "audio-sel-gpio", 0);
+	ret = gpio_request(data->audio_sel_gpio, "audio_sel_gpio");
+	if (ret < 0) {
+		pr_err("%s error: failed to request audio_sel_gpio%d (ret = %d)\n",
+					__func__, data->audio_sel_gpio, ret);
+		return -1;
+	}
+	of_property_read_u32(np, "audio-sel-level", &value);
+	data->audio_sel_gpio_level = value;
+	data->audio_cur_sel = SWITCH_SEL_USB;
+	gpio_direction_output(data->audio_sel_gpio, !data->audio_sel_gpio_level);
+
+	/* mic&gnd path select */
+	data->mic_sel_gpio = of_get_named_gpio(np, "mic-sel-gpio", 0);
+	ret = gpio_request(data->mic_sel_gpio, "mic_sel_gpio");
+	if (ret < 0) {
+		pr_err("%s error: failed to request mic_sel_gpio%d (ret = %d)\n",
+					__func__, data->mic_sel_gpio, ret);
+		//return -1;
+	}
+	data->mic_cur_sel = MIC_SWITCH_SEL_1;
+	gpio_direction_output(data->mic_sel_gpio, (data->mic_cur_sel ? 1 : 0));
+
+	return ret;
+}
+
+static int typec_audio_swtich_probe(struct platform_device *pdev)
+{
+	int ret = 0;
+	struct typec_audio_switch_data *data;
+	struct device_node *np = pdev->dev.of_node;
+
+	pr_info("%s\n", __func__);
+
+	data = devm_kzalloc(&pdev->dev, sizeof(struct typec_audio_switch_data), GFP_KERNEL);
+	if (!data) {
+		pr_err("%s, alloc fail\n", __func__);
+		return -ENOMEM;
+	}
+
+	/* resource */
+	switch_init(data, np);
+	if (ret) {
+		goto req_res_fail;
+	}
+
+	if (g_class_xxxxx != NULL) {
+		data->xxxxx_class = g_class_xxxxx;
+	} else {
+		data->xxxxx_class = class_create(THIS_MODULE, "xxxxx");
+	}
+
+	mutex_init(&data->lock);
+
+	/* creat sysfs for debug /sys/devices/platform/xxxxx_usb_switch/xxxxx/usb_switch */
+	data->dev = device_create(data->xxxxx_class, &pdev->dev, 0, NULL, "usb_audio_switch");
+	platform_set_drvdata(pdev, data);
+    
+	gp_audio_switch = data;
+	return 0;
+
+req_res_fail:
+	devm_kfree(&pdev->dev, data);
+	return ret;
+}
+
+static int typec_audio_swtich_remove(struct platform_device *pdev)
+{
+	struct typec_audio_switch_data *data = platform_get_drvdata(pdev);
+	
+	if (data) {
+		gpio_free(data->audio_sel_gpio);
+		mutex_destroy(&data->lock);
+
+		devm_kfree(&pdev->dev, data);
+	}
+
+	return 0;
+}
+
+int xxxxx_switch_select_audio(int sel)
+{
+	if (gp_audio_switch) {
+		if (gp_audio_switch->audio_cur_sel != sel) {
+			gp_audio_switch->audio_cur_sel = ((sel == SWITCH_SEL_USB) ? SWITCH_SEL_USB : SWITCH_SEL_AUDIO);
+			if (gp_audio_switch->audio_cur_sel == SWITCH_SEL_AUDIO) {
+				gpio_direction_output(gp_audio_switch->audio_sel_gpio, gp_audio_switch->audio_sel_gpio_level);
+			} else {
+				gpio_direction_output(gp_audio_switch->audio_sel_gpio, !gp_audio_switch->audio_sel_gpio_level);
+			}
+		}
+	}
+
+	return 0;
+}
+EXPORT_SYMBOL(xxxxx_switch_select_audio);
+
+
+int xxxxx_switch_change_mic_sel_path(void)
+{
+	int sel;
+
+	if (gp_audio_switch) {
+		sel = gp_audio_switch->mic_cur_sel;
+		if (++sel > MIC_SWITCH_SEL_MAX) {
+			sel = MIC_SWITCH_SEL_MIN;
+		}
+
+		gp_audio_switch->mic_cur_sel = sel;
+		gpio_direction_output(gp_audio_switch->mic_sel_gpio, (sel ? 1 : 0));
+		pr_info("%s, mic sel: %d\n", __func__, sel);
+	}
+
+	return 0;
+}
+EXPORT_SYMBOL(xxxxx_switch_change_mic_sel_path);
+
+
+static const struct of_device_id audio_switch_of_match[] = {
+	{ .compatible = "xxxxx,typec_audio_switch" },
+	{},
+};
+
+MODULE_DEVICE_TABLE(of, audio_switch_of_match);
+
+
+static int typec_audio_switch_suspend(struct device *dev)
+{
+	pr_err("==%s\n", __func__);
+	return 0;
+}
+
+static int typec_audio_switch_resume(struct device *dev)
+{
+	pr_err("==%s\n", __func__);
+	return 0;
+}
+
+static void typec_audio_switch_shutdown(struct platform_device *pdev)
+{
+	pr_err("==%s\n", __func__);
+}
+
+static const struct dev_pm_ops typec_audio_swtich_pm_ops = {
+	.suspend = typec_audio_switch_suspend,
+	.resume = typec_audio_switch_resume,
+};
+
+static struct platform_driver typec_audio_switch_driver = {
+	.probe = typec_audio_swtich_probe,
+	.remove = typec_audio_swtich_remove,
+	.driver = {
+		.name = "xxxxx_typec_audio_switch",
+		.of_match_table = audio_switch_of_match,
+		.pm = &typec_audio_swtich_pm_ops,
+	},
+	.shutdown = typec_audio_switch_shutdown,
+};
+
+
+static int __init xxxxx_typec_audio_switch_init(void)
+{
+	return platform_driver_register(&typec_audio_switch_driver);
+}
+
+static void __exit xxxxx_typec_audio_switch_exit(void)
+{
+	platform_driver_unregister(&typec_audio_switch_driver);
+}
+
+
+//module_init(xxxxx_typec_audio_switch_init);
+late_initcall(xxxxx_typec_audio_switch_init);
+module_exit(xxxxx_typec_audio_switch_exit);
+
+
+MODULE_LICENSE("GPL");
+MODULE_AUTHOR("xxx");
+MODULE_DESCRIPTION("xxxxx usb switch");
diff --git a/kernel-4.19/sound/soc/codecs/mt6357-accdet.c b/kernel-4.19/sound/soc/codecs/mt6357-accdet.c
old mode 100644
new mode 100755
index b01d249ed35..ebc43697b4b
--- a/kernel-4.19/sound/soc/codecs/mt6357-accdet.c
+++ b/kernel-4.19/sound/soc/codecs/mt6357-accdet.c
@@ -161,6 +161,12 @@ static struct snd_soc_jack_pin accdet_jack_pins[] = {
 
 static struct platform_driver accdet_driver;
 
+/* [FEATURE] Add-BEGIN by (xxx@xxxxx.com), 2021/07/21 */
+static int g_typec_headphone_plug = 0;
+
+extern int xxxxx_switch_change_mic_sel_path(void);
+/* [FEATURE] Add-END by (xxx@xxxxx.com), 2021/07/21 */
+
 static atomic_t accdet_first;
 #define ACCDET_INIT_WAIT_TIMER (10 * HZ)
 static struct timer_list accdet_init_timer;
@@ -1201,12 +1207,18 @@ void accdet_set_debounce(int state, unsigned int debounce)
 static inline void check_cable_type(void)
 {
 	u32 cur_AB = 0;
+	u32 vol;
 
 	cur_AB = accdet_read(ACCDET_MEM_IN_ADDR) >> ACCDET_STATE_MEM_IN_OFFSET;
 		cur_AB = cur_AB & ACCDET_STATE_AB_MASK;
 
 	accdet->button_status = 0;
 
+	/* [FEATURE] Add-BEGIN by (xxx@xxxxx.com), 2021/07/21 */
+	pr_info("%s, accdet_status=%d, cur_AB=%d, eint_sync_flag=%d, auxadc=%d\n", 
+		__func__, accdet->accdet_status, cur_AB, accdet->eint_sync_flag, accdet_get_auxadc());
+	/* [FEATURE] Add-END by (xxx@xxxxx.com), 2021/07/21 */
+
 	switch (accdet->accdet_status) {
 	case PLUG_OUT:
 		if (cur_AB == ACCDET_STATE_AB_00) {
@@ -1214,6 +1226,20 @@ static inline void check_cable_type(void)
 			if (accdet->eint_sync_flag) {
 				accdet->cable_type = HEADSET_NO_MIC;
 				accdet->accdet_status = HOOK_SWITCH;
+
+				/* [FEATURE] Add-BEGIN by (xxx@xxxxx.com), 2021/07/21 */
+				if (g_typec_headphone_plug) {
+					xxxxx_switch_change_mic_sel_path();
+					mdelay(2);
+					vol = accdet_get_auxadc();
+					if (vol >= 600) {
+						accdet->accdet_status = MIC_BIAS;
+						accdet->cable_type = HEADSET_MIC;
+						accdet_set_debounce(accdet_state011,
+								cust_pwm_deb->debounce3 * 30);
+					}
+				}
+				/* [FEATURE] Add-END by (xxx@xxxxx.com), 2021/07/21 */
 			} else
 				pr_notice("accdet hp has been plug-out\n");
 			mutex_unlock(&accdet->res_lock);
@@ -1652,6 +1678,35 @@ static inline int ext_eint_setup(struct platform_device *platform_device)
 	return 0;
 }
 
+/* [FEATURE] Add-BEGIN by (xxx@xxxxx.com), 2021/07/21 */
+void typec_headphone_plug_handler(int state)
+{
+	pr_info("typec headpnone plug: %d\n", state);
+
+	if (accdet != NULL) {
+		if (accdet->cur_eint_state == EINT_PLUG_IN && !g_typec_headphone_plug) {
+			pr_info("wired headset already plugin\n");
+			return;
+		}
+
+		accdet_clear_bit(ACCDET_EINT0_EN_ADDR, ACCDET_EINT0_EN_SFT);
+		mdelay(2);
+
+		g_typec_headphone_plug = state;
+		if (state == 1) {
+			accdet->cur_eint_state = EINT_PLUG_IN;
+			mod_timer(&micbias_timer,jiffies + MICBIAS_DISABLE_TIMER);
+		} else {
+			accdet->cur_eint_state = EINT_PLUG_OUT;
+
+			accdet_update_bit(ACCDET_EINT0_EN_ADDR, ACCDET_EINT0_EN_SFT);
+		}
+		queue_work(accdet->eint_workqueue, &accdet->eint_work);
+	}
+}
+EXPORT_SYMBOL(typec_headphone_plug_handler);
+/* [FEATURE] Add-END by (xxx@xxxxx.com), 2021/07/21 */
+
 static int accdet_get_dts_data(void)
 {
 	int ret = 0;
-- 
2.17.1
```