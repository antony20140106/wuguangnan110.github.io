# README

防止Paydroid下载老版本软件，在fastboot阶段阻止。

# refers

* [MTK fastboot代码流程](https://blog.csdn.net/ljj342018214/article/details/108646693)

# 方案

参考其他项目的方法，MTK平台首先要解锁解才能烧录，那就新增解锁命令，当匹配到版本后解锁进行烧录：
```
# M50&&M8 Android paydroidboot commands
paydroidboot flash getvar check-firmware-ver-mtk-mt6762-m8m50-android11.0 //相当于匹配版本
paydroidboot flash boot_a         boot.img
paydroidboot flash boot_b         boot.img
```

我们同时增加注册常量来查询版本信息
```diff
commit 1b0e316cd497073a263b466229fdd15e007f3b84
Author: wugn <wugangnan@paxsz.com>
Date:   Wed Oct 26 14:00:04 2022 +0800

    [Title]增加Paydroid Tool版本检测命令check-firmware-ver，用于防止Paydroid下载老版本软件。
    
    [Summary]:
            1.增加Paydroid Tool版本检测命令check-firmware-ver，只有输入该命令检测版本后，才能进行Paydroid Tool下载。
            2.检测命令只存在于1026版本后的Uniphiz包中，防止回退版本。
    
    [Test Plan]:
            1.升级最新软件后，再升级老软件，看是否能成功。
    
    [Module]: Paydroid Tool
    
    [Model]: M50/M8
    
    [author]: wugangnan@paxsz.com
    
    [date]: 2022-10-26

diff --git a/paxdroid/device/PayDroid/M50_M8_EEA_Release_Android_scatter.txt b/paxdroid/device/PayDroid/M50_M8_EEA_Release_Android_scatter.txt
index 2236e0cc137..8307f626538 100755
--- a/paxdroid/device/PayDroid/M50_M8_EEA_Release_Android_scatter.txt
+++ b/paxdroid/device/PayDroid/M50_M8_EEA_Release_Android_scatter.txt
@@ -1,5 +1,5 @@
 # M50&&M8 Android paydroidboot commands
-paydroidboot flash boot         boot.img
+paydroidboot flash getvar check-firmware-ver-mtk-mt6762-m8m50-android11.0
 paydroidboot flash boot_a         boot.img
 paydroidboot flash boot_b         boot.img
 paydroidboot flash dtbo_a         dtbo-verified.img
diff --git a/paxdroid/device/PayDroid/M50_M8_Release_Android_scatter.txt b/paxdroid/device/PayDroid/M50_M8_Release_Android_scatter.txt
index 2236e0cc137..8307f626538 100755
--- a/paxdroid/device/PayDroid/M50_M8_Release_Android_scatter.txt
+++ b/paxdroid/device/PayDroid/M50_M8_Release_Android_scatter.txt
@@ -1,5 +1,5 @@
 # M50&&M8 Android paydroidboot commands
-paydroidboot flash boot         boot.img
+paydroidboot flash getvar check-firmware-ver-mtk-mt6762-m8m50-android11.0
 paydroidboot flash boot_a         boot.img
 paydroidboot flash boot_b         boot.img
 paydroidboot flash dtbo_a         dtbo-verified.img
diff --git a/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/fastboot.c b/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/fastboot.c
index a757f6ed3e1..3af32027361 100755
--- a/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/fastboot.c
+++ b/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/fastboot.c
@@ -63,7 +63,6 @@ extern int get_off_mode_charge_status(void);  // FIXME!!! #include <xxx.h>
 #include "sec_unlock.h"
 #endif
 #include <platform/boot_mode.h>
-#define MAX_RSP_SIZE 64
 /* MAX_USBFS_BULK_SIZE: if use USB3 QMU GPD mode: cannot exceed 63 * 1024 */
 #define MAX_USBFS_BULK_SIZE (16 * 1024)
 
@@ -471,16 +470,6 @@ again:
 			}
 
 			dprintf(ALWAYS,"[Cmd process]-[buf:%s]-[lenBuf:%s]\n", buffer,  buffer + cmd->prefix_len);
-			//[FEATURE]-Add-BEGIN by (wugangnan@paxsz.com), 2020/6/19,for PayDroid Tool
-#ifdef MTK_SEC_FASTBOOT_UNLOCK_SUPPORT
-			if ((strcmp(buffer + cmd->prefix_len, "boot") == 0) && (0 == get_unlocked_status()))
-			{
-				r = fastboot_oem_unlock_chk();
-				if (r != 0) break;
-				dprintf(ALWAYS, "fastboot_oem_unlock_chk\n");
-			}
-#endif
-			//[FEATURE]-Add-END by (wugangnan@paxsz.com), 2020/6/19,for PayDroid Tool
 
 #ifdef MTK_SECURITY_SW_SUPPORT 
 			extern unsigned int seclib_sec_boot_enabled(unsigned int);
@@ -857,7 +846,6 @@ static void publish_ab_variables(void)
 
 #endif  // MTK_AB_OTA_UPDATER
 
-
 /******************************************************************************
 ******************************************************************************/
 int fastboot_init(void *base, unsigned size)
@@ -878,6 +866,9 @@ int fastboot_init(void *base, unsigned size)
 	timer_set_periodic(&wdt_timer, 5000, (timer_callback)mtk_wdt_restart, NULL);
 
 	fastboot_register("getvar:", cmd_getvar, TRUE, FALSE);
+	//[FEATURE]-Add-BEGIN by (wugangnan@paxsz.com), 2020/10/26, for PayDroid Tool check-firmware-ver and unlock
+	fastboot_publish ("check-firmware-ver", VerName);
+	//[FEATURE]-Add-END by (wugangnan@paxsz.com), 2020/10/26, for PayDroid Tool check-firmware-ver and unlock
 	fastboot_publish("version", "0.5");
 	fastboot_publish("version-preloader", g_boot_arg->pl_version);
 	fastboot_publish("version-bootloader", LK_VER_TAG);
diff --git a/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/fastboot.h b/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/fastboot.h
old mode 100644
new mode 100755
index 63472a126e6..a946f228bdc
--- a/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/fastboot.h
+++ b/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/fastboot.h
@@ -36,6 +36,11 @@
 #define STATE_COMMAND   1
 #define STATE_COMPLETE  2
 #define STATE_ERROR 3
+#define MAX_RSP_SIZE 64
+
+//[FEATURE]-Add-BEGIN by wugnangnan@paxsz.com, 2022/10/26, for PayDroid Tool check-firmware-ver and unlock
+static char VerName[MAX_RSP_SIZE] = "mtk-mt6762-m8m50-android11.0";
+//[FEATURE]-Add-BEGIN by wugnangnan@paxsz.com, 2021/10/26, for PayDroid Tool check-firmware-ver and unlock
 
 struct fastboot_cmd {
 	struct fastboot_cmd *next;
diff --git a/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/sys_commands.c b/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/sys_commands.c
index 639510a14e0..8ee06d9b64f 100755
--- a/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/sys_commands.c
+++ b/vendor/mediatek/proprietary/bootable/bootloader/lk/app/mt_boot/sys_commands.c
@@ -591,6 +591,33 @@ void cmd_getvar(const char *arg, void *data, unsigned sz)
 {
 	struct fastboot_var *var;
 	char response[MAX_RSP_SIZE];
+	char Buff[MAX_RSP_SIZE];
+	int ret;
+
+	//[FEATURE]-Add-BEGIN by (wugangnan@paxsz.com), 2020/10/26, for PayDroid Tool check-firmware-ver and unlock
+    // paydroidboot getvar check-firmware-ver-mtk-mt6762-m8m50-android11.0
+    if (strstr(arg, "check-firmware-ver-") != NULL) {
+		memset(Buff, 0, MAX_RSP_SIZE);
+		strcat(Buff, "check-firmware-ver-");
+		strcat(Buff, VerName);
+		if (!strcmp (Buff, arg)) {
+			if (0 == get_unlocked_status())
+			{
+				ret = fastboot_oem_unlock_chk();
+			}
+			fastboot_okay("firmware version match");
+			return;
+		}
+		else {
+			if (0 != get_unlocked_status())
+			{
+				ret = fastboot_oem_lock_chk();
+			}
+			fastboot_fail("firmware version do not match");
+			return;
+		}
+	}
+	//[FEATURE]-Add-END by (wugangnan@paxsz.com), 2022/10/26, for PayDroid Tool check-firmware-ver and unlock
 
 	if (!strcmp(arg, "all")) {
 		for (var = varlist; var; var = var->next) {
```

# 测试

* 获取版本信息：
```shell
C:\Users\wugangnan>fastboot  getvar check-firmware-ver
check-firmware-ver: mtk-mt6762-m8m50-android11.0
```

* 版本匹配后，解锁并烧录：
```shell
C:\Users\wugangnan>fastboot  getvar check-firmware-ver-mtk-mt6762-m8m50-android11.0
check-firmware-ver-mtk-mt6762-m8m50-android11.0: firmware version match
Finished. Total time: 0.041s

C:\Users\wugangnan>paydroidboot flash boot         boot.img
paydroidboot build for pax May 25 2020 10:41:00
target reported max download size of 134217728 bytes
sending 'boot' (33554432 bytes)...
OKAY [  0.759s]
writing 'boot'...
OKAY [  0.431s]
finished. total time: 1.194s
```

* 版本不匹配，上锁烧录失败：
```shell
:\Users\wugangnan>fastboot  getvar check-firmware-ver-mtk-mt6762-m8m50-android11.
getvar:check-firmware-ver-mtk-mt6762-m8m50-android11. FAILED (remote: 'firmware version do not match')
Finished. Total time: 0.051s

C:\Users\wugangnan>paydroidboot flash boot         boot.img
paydroidboot build for pax May 25 2020 10:41:00
target reported max download size of 134217728 bytes
sending 'boot' (33554432 bytes)...
OKAY [  0.736s]
writing 'boot'...
FAILED (remote: not allowed in locked state)
finished. total time: 0.748s
```