# qcom

XBL阶段有时需要读取某些i2c设备状态，如外部charger ic，需要添加charger驱动。

# refer

* [QCM2290---UEFI之I2C](https://blog.csdn.net/weixin_40970718/article/details/119883448)
* [高通UEFI中的I2C的方式读取TP的id](https://www.cnblogs.com/schips/p/using_i2c_in_uefi_to_read_tp_id_in_qualcommm_uefi_xbl.html)

# 代码

* 直接上代码，需要先参考文章[0008_qcom_qcm2290_ABL_增加i2c接口.md](0008_qcom_qcm2290_ABL_增加i2c接口.md)打开`i2c_devcfg`。
* `QcomPkg/Drivers/QcomChargerDxe/PaxCharger.c`:
```C++
/* I2C Interfaces */
#include "i2c_api.h"
#include <Protocol/EFII2C.h>
#include "PaxCharger.h"
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/EventGroup.h>

static i2c_slave_config cfg;
static void *pI2cHandle = NULL;

i2c_status chg_i2c_init(UINT32 SlaveAddr, UINT32 I2cFreq)
{
	i2c_status i2cstatus = I2C_SUCCESS;
 
	cfg.bus_frequency_khz = I2cFreq;
	cfg.slave_address = SlaveAddr;
    cfg.mode = I2C;
    cfg.slave_max_clock_stretch_us = 500;
    cfg.core_configuration1 = 0;
    cfg.core_configuration2 = 0 ;
 
	i2cstatus = i2c_open((i2c_instance) (I2C_INSTANCE_001), &pI2cHandle);
	if (I2C_SUCCESS != i2cstatus)
	{
		DEBUG((EFI_D_ERROR, "Failed to initialize I2C %d\n", i2cstatus));
	}
	return i2cstatus;
}

unsigned int i2c_read_16bit_reg(unsigned int addr)
{
	uint32 bRead = 0;
	unsigned int getdata = 0;
	i2c_status i2cstatus = I2C_SUCCESS;
	unsigned char rdbuf[2] = {0};
	//gBS->Stall(600000);
	i2cstatus = i2c_read (pI2cHandle, &cfg, addr, 2, rdbuf, 1, &bRead, 2500);
	if(I2C_SUCCESS != i2cstatus)
	{
		DEBUG((EFI_D_ERROR, "Read addr:0x%X error\n", addr));
	}
	getdata=rdbuf[0] & 0x00ff;
	getdata<<= 8;
	getdata |=rdbuf[1];
	DEBUG((EFI_D_ERROR, "[dong]rdbuf[0] & 0x00ff is %d\n\n", rdbuf[0] & 0x00ff));
	return (rdbuf[0] & 0x00ff);//getdata;//
	
}

unsigned int i2c_writ_16bite_reg(unsigned char addr, unsigned int reg_data)
{
	uint32 bWrote = 0;
	i2c_status i2cstatus = I2C_SUCCESS;
	unsigned char wdbuf[2] = {0};
	wdbuf[1] = (unsigned char)(reg_data & 0x00ff);
	wdbuf[0] = (unsigned char)((reg_data & 0xff00)>>8);
	i2cstatus = i2c_write (pI2cHandle, &cfg, addr, 1, wdbuf, 2, &bWrote, 2500);
	if(I2C_SUCCESS != i2cstatus)
	{
		DEBUG((EFI_D_ERROR, "Write addr:0x%X data:0x%X error\n", addr, reg_data));
	}
	return bWrote;
}

unsigned int i2c_read_8bit_reg(unsigned int addr)
{
	uint32 bRead = 0;
	unsigned int getdata = 0;
	i2c_status i2cstatus = I2C_SUCCESS;
	unsigned char rdbuf[1] = {0};
	//gBS->Stall(600000);
	i2cstatus = i2c_read (pI2cHandle, &cfg, addr, 1, rdbuf, 2, &bRead, 2500);
	if(I2C_SUCCESS != i2cstatus)
	{
		DEBUG((EFI_D_ERROR, "Read addr:0x%X error\n", addr));
	}
	getdata=rdbuf[0] & 0x00ff;
	DEBUG((EFI_D_ERROR, "Read addr:0x%X data:0x%X \n", addr, getdata));
	return getdata;//getdata;//
	
}

unsigned int i2c_write_8bit_reg(unsigned char addr, unsigned int reg_data)
{
	uint32 bWrote = 0;
	i2c_status i2cstatus = I2C_SUCCESS;
	unsigned char wdbuf[1] = {0};
	wdbuf[0] = (unsigned char)(reg_data & 0x00ff);
	i2cstatus = i2c_write (pI2cHandle, &cfg, addr, 1, wdbuf, 2, &bWrote, 2500);
	DEBUG((EFI_D_WARN, "Write addr:0x%X data:0x%X \n", addr, reg_data));
	i2c_read_8bit_reg(addr);
	if(I2C_SUCCESS != i2cstatus)
	{
		DEBUG((EFI_D_ERROR, "Write addr:0x%X data:0x%X error\n", addr, reg_data));
	}
	return bWrote;
}

i2c_status i2c_deinit()
{
	return i2c_close(pI2cHandle);
}

void Pax_ChargerEnable(BOOLEAN enable)
{
	unsigned int data = 0;
	//int addr = 0;
 
	DEBUG((EFI_D_ERROR, "Pax_ChargerEnable:0x%X \n", enable));
	chg_i2c_init(0x3f, 400);
	//for (addr = 0; addr <= 0x16; addr++) {
	//	i2c_read_8bit_reg(addr);
	//}
	if (enable)
	{
		i2c_write_8bit_reg(0x01, 0x13);//INN 2000ma
		i2c_write_8bit_reg(0x02, 0xd6);//ICC 480ma
	}
	data = enable ? 0x53 : 0x50;
	i2c_write_8bit_reg(0x09, data);
	i2c_deinit();
}
```

* `QcomPkg/Drivers/QcomChargerDxe/PaxCharger.h`:
```C++
#ifndef _xxx_CHARGER_H_
#define _xxx_CHARGER_H_

extern void Pax_ChargerEnable(BOOLEAN enable);

#endif
```

* 需要使用到`I2CApiLib`库里面的i2c接口:
```diff
--- a/A665x_Unpacking_Tool/BOOT.XF.4.1/boot_images/QcomPkg/Drivers/QcomChargerDxe/QcomChargerDxeLA.inf
+++ b/A665x_Unpacking_Tool/BOOT.XF.4.1/boot_images/QcomPkg/Drivers/QcomChargerDxe/QcomChargerDxeLA.inf
@@ -35,6 +35,7 @@
   QcomChargerPlatform.c
   QcomChargerPlatform_File.c
   QcomChargerPlatform_FileLa.c
+  PaxCharger.c

 [Packages]
   MdePkg/MdePkg.dec
@@ -61,6 +62,7 @@
   ChargerLib
   SmemLib
   PrintLib
+  I2CApiLib

 [Protocols]
   gQcomChargerProtocolGuid
@@ -87,6 +89,7 @@
   gQcomPmicPwrOnProtocolGuid
   gEfiVariableServicesProtocolGuid        ## CONSUMES
   gEfiSimpleFileSystemProtocolGuid
+  gQcomI2CProtocolGuid
```