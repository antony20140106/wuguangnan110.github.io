/************************************************************************************
*
*    DESCRIPTION:Copyright(c) 2007-2010 Xiamen Yealink Network Technology Co,.Ltd
*
*    AUTHOR:xxx
*
*    HISTORY:
*
*    DATE:2021-06-23 19:57:53
*
*************************************************************************************/
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

EFI_QCOM_I2C_PROTOCOL *efiQcomI2cProtocol;
static void *I2cDeviceHandle = NULL;
EFI_HANDLE *pHandles;

unsigned int linuxc_i2c_read_16bit_reg(uint8 slaveAddress,uint16 addr)
{
    uint32 bRead = 0;
	unsigned int getdata = 0;
    i2c_status i2cstatus = I2C_SUCCESS;
	unsigned char rdbuf[2] = {0};

   i2c_slave_config cfg = {
	   .bus_frequency_khz = 400,
	   .slave_address = slaveAddress,
	   .mode = I2C,
	   .slave_max_clock_stretch_us = 500,
	   .core_configuration1 = 0,
	   .core_configuration2 = 0
   };
    gBS->Stall(600000);
    i2cstatus = efiQcomI2cProtocol->read (I2cDeviceHandle, &cfg, addr, 1, rdbuf, 2, &bRead, 2500);
	getdata = rdbuf[1] & 0x00ff;
	getdata <<= 8;
	getdata |= rdbuf[0];
	
    if(I2C_SUCCESS != i2cstatus)
    {
      DEBUG((EFI_D_ERROR, "Read addr:0x%X error i2cstatus = %d\n", addr,i2cstatus));
    }
	else 
	{
	  DEBUG((EFI_D_ERROR, "Read rdbuf[0]= 0x%X , rdbuf[1] = 0x%X\n", rdbuf[0],rdbuf[1]));
	  DEBUG((EFI_D_ERROR, "Read addr:0x%X succ, getdata = 0x%X\n", addr,getdata));
	}
    return getdata;
}

unsigned int linuxc_i2c_write_16bit_reg(uint8 slaveAddress, uint16 addr, unsigned int reg_data)
{
    uint32 bWrote = 0;
    i2c_status i2cstatus = I2C_SUCCESS;
    unsigned char wdbuf[1]={0};
	wdbuf[0] = (unsigned char)(reg_data & 0x00ff);
	wdbuf[1] = (unsigned char)((reg_data & 0x00ff) >> 8);
    wdbuf[0]=reg_data;

   i2c_slave_config cfg = {
     .bus_frequency_khz = 400,
     .slave_address = slaveAddress,
     .mode = I2C,
     .slave_max_clock_stretch_us = 500,
     .core_configuration1 = 0,
     .core_configuration2 = 0
	};

	i2cstatus = efiQcomI2cProtocol->write (I2cDeviceHandle, &cfg, addr, 1, wdbuf, 2, &bWrote, 2500);
  
	if(I2C_SUCCESS != i2cstatus)
	{
	DEBUG((EFI_D_ERROR, " Write addr:0x%X data:0x%X error,i2cstatus = %d\n", addr, reg_data,i2cstatus));
	}
	else
	{
	  DEBUG((EFI_D_ERROR, " Write addr:0x%X data:0x%X succ,bWrote = 0x%X\n", addr, reg_data,bWrote));
	}
	return bWrote;
}

unsigned int linuxc_i2c_read_8bit_reg(uint8 slaveAddress,uint8 addr)
{
    uint32 bRead = 0;
	unsigned int getdata = 0;
    i2c_status i2cstatus = I2C_SUCCESS;
	unsigned char rdbuf[1] = {0};

	i2c_slave_config cfg = {
		.bus_frequency_khz = 400,
		.slave_address = slaveAddress,
		.mode = I2C,
		.slave_max_clock_stretch_us = 500,
		.core_configuration1 = 0,
		.core_configuration2 = 0
	};
    gBS->Stall(600000);
    i2cstatus = efiQcomI2cProtocol->read (I2cDeviceHandle, &cfg, addr, 1, rdbuf, 2, &bRead, 2500);
	getdata = rdbuf[0] & 0x00ff;
	
    if(I2C_SUCCESS != i2cstatus)
    {
      DEBUG((EFI_D_ERROR, "Read addr:0x%X error i2cstatus = %d\n", addr,i2cstatus));
    }
	else
	{
	  DEBUG((EFI_D_ERROR, "Read addr:0x%X succ, getdata = 0x%X\n", addr,getdata));
	}
    return getdata;
}

unsigned int linuxc_i2c_write_8bit_reg(uint8 slaveAddress, uint8 addr, unsigned int reg_data)
{
	uint32 bWrote = 0;
	i2c_status i2cstatus = I2C_SUCCESS;
	unsigned char wdbuf[1]={0};
	wdbuf[0] = (unsigned char)(reg_data & 0x00ff);

	i2c_slave_config cfg = {
		.bus_frequency_khz = 400,
		.slave_address = slaveAddress,
		.mode = I2C,
		.slave_max_clock_stretch_us = 500,
		.core_configuration1 = 0,
		.core_configuration2 = 0
	};

	i2cstatus = efiQcomI2cProtocol->write (I2cDeviceHandle, &cfg, addr, 1, wdbuf, 2, &bWrote, 2500);

	if(I2C_SUCCESS != i2cstatus)
	{
	  DEBUG((EFI_D_ERROR, " Write addr:0x%X data:0x%X error,i2cstatus = %d\n", addr, reg_data,i2cstatus));
	}
	else
	{
	  DEBUG((EFI_D_ERROR, " Write addr:0x%X data:0x%X succ,bWrote = 0x%X\n", addr, reg_data,bWrote));
	}
	return bWrote;
}

EFI_STATUS i2c_init(i2c_instance I2C_INSTANCE)
{
    //EFI_QCOM_I2C_PROTOCOL *efiQcomI2cProtocol;
    //void *I2cDeviceHandle;
	//EFI_HANDLE *pHandles;
    UINTN HandleCnt;
    EFI_STATUS efiStatus = EFI_DEVICE_ERROR;
    i2c_status i2cStatus;

    /**找出支持gQcomI2CProtocolGuid的所有设备**/
    efiStatus = gBS->LocateHandleBuffer (ByProtocol,
                                        &gQcomI2CProtocolGuid,
                                        NULL,
                                        &HandleCnt,
                                        &pHandles);
    if (efiStatus != EFI_SUCCESS)
    {
       // Print(L"LocateHandleBuffer error %r\r\n", efiStatus);
        goto I2C_EXIT;
    }

    efiStatus = gBS->HandleProtocol (
                                      pHandles[0],
                                      &gQcomI2CProtocolGuid,
                                      (VOID **) &efiQcomI2cProtocol
                                      );
    if (efiStatus != EFI_SUCCESS) 
    {
        //Print(L"handel EFI_SERIAL_IO_PROTOCOL[0] error %r\r\n", efiStatus);
        goto I2C_EXIT;
    }

    i2cStatus = efiQcomI2cProtocol->open(I2C_INSTANCE, &I2cDeviceHandle);
    if(i2cStatus != I2C_SUCCESS)
    {
        //Print(L"I2c open error %d\r\n", i2cStatus);
        goto I2C_EXIT;
    }
	
I2C_EXIT:
    if(pHandles != NULL)
        gBS->FreePool(pHandles);

    return efiStatus;
}
