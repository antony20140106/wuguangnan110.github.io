/* I2C Interfaces */
#include "xxx_battery.h"
#include <Uefi.h>
//#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
//#include <Library/TestInterface.h>
#include "EFIPmicVreg.h"
//#include <Protocol/EFII2C.h>
#include "EFITlmm.h"
//#include <Protocol/EFIPmicGpio.h>
//#include <Library/QcomLib.h>
#include "DebugLib.h"
#include "i2c.h"

int battery_is_exist(VOID)
{
	unsigned int type = 0;

	linuxc_i2c_write_16bit_reg(0x55,0x3e,0x0001);
	type = linuxc_i2c_read_16bit_reg(0x55,0x40);//gauge中存id的寄存器地址
	DEBUG((EFI_D_ERROR, "bq27z746 get dev type:0x%X\n", type));

	if (type == BQ27Z746_DeviceType) {
		return 1;
	}

	return 0;
}