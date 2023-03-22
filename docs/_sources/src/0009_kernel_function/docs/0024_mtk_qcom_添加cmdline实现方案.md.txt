# 概述

我们经常要在开机阶段检测某些硬件信息传递给kernel，可以通过cmdline方式传递。

# MTK

mtk平台在lk阶段使用`cmdline_append`接口即可追加cmdline信息：
* `app/mt_boot/bootargs.c`:
```C++
bool cmdline_append(const char *append_string)
{
        unsigned int append_string_len;

        enter_critical_section();
        append_string_len = strlen(append_string);
        validate_cmdline_boundary((cmdline_tail + append_string_len + 1), cmdline_end);
        *cmdline_tail++ = ' ';
        strncat(cmdline_tail, append_string, cmdline_end - cmdline_tail);
        cmdline_tail += append_string_len;
        exit_critical_section();
        return true;
}
```

* 实例：
```C++
//[FEATURE]-Add-BEGIN by (xxx@xxxxx.com), 2020/07/03, for LCD identification.
sprintf(cmdline_tmpbuf, "%s%s%s", CMDLINE_TMP_CONCAT_SIZE, "androidboot.lcm.select=", mt_disp_get_lcm_id());
cmdline_append(cmdline_tmpbuf);
//[FEATURE]-Add-END by (xxx@xxxxx.com), 2020/07/03, for LCD identification.
xxxxx_get_profile_id();
sprintf(cmdline_tmpbuf, "%s%s%d", CMDLINE_TMP_CONCAT_SIZE, "androidboot.battery.id=", battery_id);
cmdline_append(cmdline_tmpbuf);
sprintf(cmdline_tmpbuf, "%s%s%d", CMDLINE_TMP_CONCAT_SIZE, "androidboot.lcm.id=", lcm_id);
cmdline_append(cmdline_tmpbuf);
```

# QCOM

* 通过`update_bootargs`函数向`PaxCmdLine`添加我们要加的cmdline字符串。
* `edk2/QcomModulePkg/Library/bootLib/update_cmdline.c`:
```C++
/*Update command line: appends boot information to the original commandline
 *that is taken from boot image header*/
EFI_STATUS
UpdateCmdLine (CONST CHAR8 *CmdLine,
               CHAR8 *FfbmStr,
               BOOLEAN Recovery,
               BOOLEAN AlarmBoot,
               CONST CHAR8 *VBCmdLine,
               CHAR8 **FinalCmdLine,
               UINT32 HeaderVersion)
{
  /*Add by xxxSZ*/
  update_bootargs(PaxCmdLine);
  CmdLineLen += AsciiStrLen (PaxCmdLine);

  /*Add by xxxSZ*/
  Param.PaxCmdLine = PaxCmdLine;

  Status = UpdateCmdLineParams (&Param, FinalCmdLine);
  if (Status != EFI_SUCCESS) {
    return Status;
  }
}
```

* 通过`bootargs_add`接口追加cmdline，具体库函数如下：
```C++
static void bootargs_add(char *key, char *value, char *cmdline)
{
	AsciiStrCat(cmdline, " ");
	AsciiStrCat(cmdline, key);
	AsciiStrCat(cmdline, value);
}

void update_bootargs(char *cmdline)
{
	//char cmdline[2048] = {0};
	char tmpbuf[128] = {0};
	char cfginfo[1024];
    char mac[16] = {0};
	char *ptr = NULL;
    char cfginfo_bak[1024] = {0};

	memset(cfginfo, 0, sizeof(cfginfo));
	update_cfg_info(cfginfo); //将配置文件cfg全部追加到cmdline
	ptr = AsciiStrStr(cfginfo, "SN=");
	if (ptr != NULL) {
        int len = ptr - cfginfo -1; //decrease one blank
		memset(tmpbuf, 0, sizeof(tmpbuf));
        AsciiSPrint(tmpbuf, MAX_PATH_SIZE, " xxxxx_config_begin=%d ",len);
        AsciiStrCat(cmdline, tmpbuf);

		memset(tmpbuf, 0, sizeof(tmpbuf));
        AsciiSPrint(tmpbuf, MAX_PATH_SIZE, " xxxxx_config_end=%d ",len);
        AsciiStrCpy(cfginfo_bak, ptr);
        AsciiStrCpy(ptr, tmpbuf);
        AsciiStrCpy(ptr + AsciiStrLen(tmpbuf), cfginfo_bak);
        AsciiStrCat(cmdline, cfginfo);
	}
	else {
		AsciiStrCat(cmdline,cfginfo);
	}

	memset(cfginfo, 0, sizeof(cfginfo));
	update_auth_info(cfginfo);
	AsciiStrCat(cmdline,cfginfo);

#if 1
	const char *console_debug_value = "enable";
#ifndef CONFIG_DEBUG
	console_debug_value = "disable";
#endif
#if 0
	memset(tmpbuf, 0, sizeof(tmpbuf));
	AsciiSPrint(tmpbuf, MAX_PATH_SIZE, " console_debug=%s", console_debug_value);
	AsciiStrCat(cmdline, tmpbuf);
#else
	bootargs_add("console_debug=", (char *)console_debug_value, cmdline);
#endif
#endif

//[FEATURE]-Add-BEGIN by xielianxiong@xxxxx.com, 2021/12/13, for init other cmdline property

	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(bootargs_getvalue("EXSN=", tmpbuf, cmdline) == 0)
	{
		bootargs_delete("EXSN=", cmdline);
		bootargs_add("xxxxxspexsn=", tmpbuf, cmdline);
	}else if(0 == GetEXSN(tmpbuf,MAX_OTHER_CFG_LENGTH)){
        bootargs_add("xxxxxspexsn=", tmpbuf, cmdline);
    }

	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(bootargs_getvalue("SN=", tmpbuf, cmdline) == 0)
	{
		bootargs_delete("SN=", cmdline);
		bootargs_add("xxxxxspsn=", tmpbuf, cmdline);
	}else if(0 == GetSN(tmpbuf,MAX_OTHER_CFG_LENGTH)){
        bootargs_add("xxxxxspsn=", tmpbuf, cmdline);
    }

	memset(tmpbuf, 0, sizeof(tmpbuf));
	if(bootargs_getvalue("MAC=", mac, cmdline) == 0)
	{
		bootargs_delete("MAC=", cmdline);
		AsciiSPrint(tmpbuf, MAX_PATH_SIZE, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],
				mac[6],mac[7],mac[8],mac[9],mac[10],mac[11]);
		bootargs_add("xxxxxspmac=", tmpbuf, cmdline);
	}else if(0 == GetMAC(tmpbuf,MAC_NV_LENGTH*2+5)){
        bootargs_add("xxxxxspmac=", tmpbuf, cmdline);//ethmac
    }

    memset(tmpbuf, 0, sizeof(tmpbuf));
	if(0 == GetWifiMAC(tmpbuf,MAC_NV_LENGTH*2+5)){
        bootargs_add("xxxxxwifimac=", tmpbuf, cmdline);//wifimac
    }

    memset(tmpbuf, 0, sizeof(tmpbuf));
	if(0 == GetBtMAC(tmpbuf,MAC_NV_LENGTH*2+5)){
        bootargs_add("xxxxxbtmac=", tmpbuf, cmdline);//btmac
    }

	//[FEATURE]-BEGIN by (xxx@xxxxx.com), 2022/09/08 for Turn off power off charging mode when no-battery-startup
	if (!is_battery_exist()) {
		bootargs_add("start_without_battery=", "1", cmdline);//Start without battery
	}
	else {
		bootargs_add("start_without_battery=", "0", cmdline);
	}
	//[FEATURE]-END by (xxx@xxxxx.com), 2022/09/08 for Turn off power off charging mode when no-battery-startup
//[FEATURE]-Add-END by xielianxiong@xxxxx.com, 2021/12/13, for init other cmdline property
}

int update_cfg_info(char *cfginfo)
{     
    int len = AsciiStrLen((void*)g_cfg_info.cfgContent);
    cfginfo[0] = ' ';
    AsciiStrnCpy(cfginfo+1, (void*)g_cfg_info.cfgContent, len);

    return len;
}
```

# kernel验证

* kernel驱动api如下：
```C++
/* Copyright (C) 2017-2021  xxx
 * parse command line and provide cmdline_get_value() to get the value
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#define CMDLINE_SIZE	2048
#define CMDPARA_SIZE	200

struct cmd_para {
	const char *key;
	const char *value;
};

extern char *saved_command_line;
static char *cmdline_buf = NULL;
static struct cmd_para cmd_paras[CMDPARA_SIZE] = {{0}};

const char *cmdline_get_value(const char *key)
{
	int i = 0;

	while((i < CMDPARA_SIZE) && cmd_paras[i].key != NULL) {
		if (!strcmp(key, cmd_paras[i].key))
			return cmd_paras[i].value;
		i++;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(cmdline_get_value);

static int cmdops_init(void)
{
	int i = 0;
	char *ptr;
	char *x;
	char *value;

	//printk("saved_command_line: %s\n", saved_command_line);
	cmdline_buf = (char *)kzalloc(CMDLINE_SIZE, GFP_KERNEL);

	if (NULL == cmdline_buf) {
		printk("can not malloc buffer!\n");
		return -1;
	}
	memcpy(cmdline_buf, saved_command_line, strlen(saved_command_line));
	//printk("cmdline_buf: %s\n", cmdline_buf);

	ptr = cmdline_buf;
	while (ptr && *ptr && (i < CMDPARA_SIZE)) {
		x = strchr(ptr, ' ');
		if (NULL != x)
			*x++ = '\0';
		value = strchr(ptr, '=');
		if (NULL == value) {
			ptr = x;
			continue;
		}
		*value++ = '\0';
		//printk("key:%s, val:%s\n", ptr, value);
		cmd_paras[i].key = ptr;
		cmd_paras[i].value = value;

		ptr = x;
		i++;
	}
	printk("total %d parameters\n", i);

	//printk("test get %s\n", cmdline_get_value("TERMINAL_NAME"));

	return 0;
}

static void cmdops_exit(void)
{
	if(NULL != cmdline_buf)
		kfree(cmdline_buf);
}

postcore_initcall(cmdops_init);
module_exit(cmdops_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("fuzk@xxxxx.com");
```

* 例如查看`start_without_battery`参数，验证方案：
```C++
extern const char *cmdline_get_value(const char *key);

int xxxxx_bat_exist_from_cmdline(void)
{
        int ret = !!strcmp(cmdline_get_value("start_without_battery"), "0");
        return ret;
}
```

# 调试方法

可以通过`cat /proc/cmdline`节点的方式查看是否添加成功：
```shell
console:/ # cat /proc/cmdline
cgroup_disable=pressure rcupdate.rcu_expedited=1 rcu_nocbs=0-7 kpti=off console=ttyMSM0,115200n8 earlycon=[   49.397771] Battery: [ status:Charging, health:Good, present:1, tech:Li-ion, capcity:5,cap_rm:211 mah, vol:3850 mv, temp:30, curr:3310 ma, ui_soc:5, notify_code: 0 ]
msm_geni_serial,0x4a90000 androidboot.hardware=qcom androidboot.console=ttyMSM0 androidboot.memcg=1 lpm_levels.sleep_disabled=1 video=vfb:640x400,bpp=32,memsize=3072000 msm_rtb.filter=0x237 service_locator.enable=1 swiotlb=2048 loop.max_part=7 buildvariant=userdebug androidboot.verifiedbootstate=orange androidboot.keymaster=1 androidboot.vbmeta.device=PARTUUID=1bc4422a-31a7-12c2-a4b0-737a73250b0f androidboot.vbmeta.avb_version=1.0 androidboot.vbmeta.device_state=unlocked androidboot.vbmeta.hash_alg=sha256 androidboot.vbmeta.size=7360 androidboot.vbmeta.digest=a04754bbd280a596a2c4f1c5d8ec3b4bbf03a5b2790999a2a222bd3eaf9f4972 androidboot.vbmeta.invalidate_on_error=yes androidboot.veritymode=enforcing androidboot.bootdevice=4744000.sdhci androidboot.fstab_suffix=emmc androidboot.boot_devices=soc/4744000.sdhci androidboot.serialno=e4e19011 androidboot.baseband=msm msm_drm.dsi_display0=qcom,mdss_dsi_ili7807S_1080p_video_dpi_480: androidboot.slot_suffix=_a rootwait ro init=/init androidboot.dtbo_idx=0 androidboot.dtb_idx=0 TOUCH_SCREEN=257 LCD=257 FPM=11 WIFI=36 WIFI_PA=04 CAMERA_NUMBER=01 CAMERA_FRONT=80 MAIN_BOARD=V01 PORT_BOARD=V01 PN=A665x-AA200-260A-2N0-EA CONFIG_FILE_VER=2570000_V1.0 TERMINAL_NAME=A665x  SecMode=3  security_level=3  TamperClear=0  LastBblStatus=0  AppDebugStatus=1  FirmDebugStatus=1  SnDownLoadSum=0  UsPukLevel=3  Customer=255  console_debug=disable start_without_battery=0
```