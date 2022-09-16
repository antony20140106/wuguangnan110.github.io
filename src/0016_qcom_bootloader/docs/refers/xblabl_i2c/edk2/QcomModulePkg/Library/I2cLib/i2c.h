/*******************************************************************
*
*	 DESCRIPTION:Copyright(c) 2007-2010 Xiamen Yealink Network Technology Co,.Ltd
*
*	 AUTHOR:xhwang
*
*	 HISTORY:
*
*	 DATE:2021-06-24 09:46:44
*
*******************************************************************/
#ifndef __I2C_H__
#define __I2C_H__
#include "EFII2C.h"

#define BQ27Z746_DeviceType  0x1746

EFI_STATUS i2c_init(i2c_instance I2C_INSTANCE);
unsigned int linuxc_i2c_write_8bit_reg(uint8 slaveAddress, uint8 addr, unsigned int reg_data);
unsigned int linuxc_i2c_read_8bit_reg(uint8 slaveAddress,uint8 addr);
unsigned int linuxc_i2c_write_16bit_reg(uint8 slaveAddress, uint16 addr, unsigned int reg_data);
unsigned int linuxc_i2c_read_16bit_reg(uint8 slaveAddress,uint16 addr);
#endif /**__I2C_H__**/
