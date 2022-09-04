/*
 * BQ2589x battery charging driver
 *
 * Copyright (C) 2013 Texas Instruments
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/power/charger-manager.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/version.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/usb/phy.h>
#include <uapi/linux/usb/charger.h>
#include <linux/usb/typec.h>
#include <linux/usb/sc27xx_typec.h>
#include "mp2723_reg.h"

enum mp2723_vbus_type {
	MP2723_VBUS_NONE = 0,
	MP2723_VBUS_NONSTAND,	// adapter
	MP2723_VBUS_APPLE_1_0,
	MP2723_VBUS_APPLE_2_1,
	MP2723_VBUS_APPLE_2_4,
	MP2723_VBUS_USB_SDP,
	MP2723_VBUS_USB_CDP,
	MP2723_VBUS_USB_DCP,
	MP2723_VBUS_TYPE_NUM,
};

enum mp2723_part_no {
	MP2723 = 0x0,
};

enum {
	MP2723_CHG_OK,
	MP2723_BATTERY_ERR,
	MP2723_WATCHDOG_FAULT,
	MP2723_CHG_INPUT_ERR,/*CHARGE POWER IS ERROR */
	MP2723_CHG_THERMAL_SHUTDOWN,
	MP2723_CHG_TIMEOUT,
	MP2723_CHG_TS_WARM,
	MP2723_CHG_TS_COOL,
	MP2723_CHG_TS_COLD,
	MP2723_CHG_TS_HOT,
};

#define MP2723_STATUS_PLUGIN		0x0001
#define MP2723_STATUS_NTC_FLOAT		0x0002
#define MP2723_STATUS_THERMAL		0x0004
#define	MP2723_STATUS_CHARGE_ENABLE 0x0008
#define MP2723_STATUS_FAULT			0x0010
#define MP2723_STATUS_EXIST			0x0100

struct mp2723_config {
	bool	enable_detect_dpdm;
	bool	enable_otg;

	int		charge_voltage;
	int		charge_current;

	bool	enable_term;
	int		term_current;

	bool 	enable_ico;

	int		prechg_current;
	int		prechg_to_fastchg_threshold;
	int		rechg_threshold_offset;
	int		vsys_min;
	int 	iin_limit;
	int		vin_limit;
	int		treg;

	bool	power_supply_type_adapter;
	bool	power_supply_type_usb;
};


struct mp2723 {
	struct device *dev;
	struct i2c_client *client;
	enum   mp2723_part_no part_no;

	unsigned int    status;
	int				vbus_type;
	u8              charge_fault;
	int     charge_type;
    int     charge_status;
	int     health_status;

	bool	enabled;
	int		vbus_volt;
	int		vbat_volt;

	int		rsoc;
	struct	mp2723_config	cfg;
	struct work_struct irq_work;
//	struct delayed_work adapter_in_work;
//	struct delayed_work adapter_out_work;
	struct delayed_work monitor_work;
	struct delayed_work ico_work;
//	struct delayed_work pe_volt_tune_work;
//	struct delayed_work check_pe_tuneup_work;
	struct workqueue_struct *adp_workqueue;

	struct power_supply *usb;
	struct power_supply *wall;
//	struct power_supply *batt_psy;
	bool chage_enable;

	int irq_pin;
};

struct pe_ctrl {
	bool enable;
	bool tune_up_volt;
	bool tune_down_volt;
	bool tune_done;
	bool tune_fail;
	int  tune_count;
	int  target_volt;
	int  high_volt_level;/* vbus volt > this threshold means tune up successfully */
	int  low_volt_level; /* vbus volt < this threshold means tune down successfully */
	int  vbat_min_volt;  /* to tune up voltage only when vbat > this threshold */
};

static struct pe_ctrl pe;

#define MP2723_IIN_LIMIT_DEFAULT	400
#define MP2723_VIN_LIMIT_MIN		3700
#define MP2723_VIN_LIMIT_MAX		5200
#define MP2723_VIN_LIMIT_DEFAULT	4600
#define MP2723_ICC_MIN	320	
#define MP2723_ICC_MAX	2966
#define MP2723_NO_INPUT_VOL_TH	3500
static void mp2723_adjust_vin_limit(struct mp2723 *mp);


static DEFINE_MUTEX(mp2723_i2c_lock);

static int mp2723_read_byte(struct mp2723 *mp, u8 *data, u8 reg)
{
	int ret;

	mutex_lock(&mp2723_i2c_lock);
	ret = i2c_smbus_read_byte_data(mp->client, reg);
	if (ret < 0) {
		dev_err(mp->dev, "failed to read 0x%.2x\n", reg);
		mutex_unlock(&mp2723_i2c_lock);
		return ret;
	}

	*data = (u8)ret;
	mutex_unlock(&mp2723_i2c_lock);

	return 0;
}

static int mp2723_write_byte(struct mp2723 *mp, u8 reg, u8 data)
{
	int ret;
	mutex_lock(&mp2723_i2c_lock);
	ret = i2c_smbus_write_byte_data(mp->client, reg, data);
	mutex_unlock(&mp2723_i2c_lock);
	return ret;
}

static int mp2723_update_bits(struct mp2723 *mp, u8 reg, u8 mask, u8 data)
{
	int ret;
	u8 tmp;

	ret = mp2723_read_byte(mp, &tmp, reg);

	if (ret)
		return ret;

	tmp &= ~mask;
	tmp |= data & mask;

	return mp2723_write_byte(mp, reg, tmp);
}


static enum mp2723_vbus_type mp2723_get_vbus_type(struct mp2723 *mp)
{
	u8 val = 0;
	int ret;

	ret = mp2723_read_byte(mp, &val, MP2723_REG_0C);
	if (ret < 0)
		return 0;
	val &= MP2723_VIN_STAT_MASK;
	val >>= MP2723_VIN_STAT_SHIFT;

	return val;
}

static int mp2723_enable_otg(struct mp2723 *mp)
{
	u8 val = MP2723_CHG_CONFIG_OTG << MP2723_CHG_CONFIG_SHIFT;

	return mp2723_update_bits(mp, MP2723_REG_04, MP2723_CHG_CONFIG_MASK, val);
}

static int mp2723_disable_otg(struct mp2723 *mp)
{
	u8 val = MP2723_CHG_ENABLE << MP2723_CHG_CONFIG_SHIFT;

	return mp2723_update_bits(mp, MP2723_REG_04, MP2723_CHG_CONFIG_MASK, val);
}
EXPORT_SYMBOL_GPL(mp2723_disable_otg);

static int mp2723_set_otg_volt(struct mp2723 *mp, int volt)
{
	u8 val = 0;

	if (volt < MP2723_VINDSCHG_BASE)
		volt = MP2723_VINDSCHG_BASE;
	if (volt > MP2723_VINDSCHG_BASE + (MP2723_VINDSCHG_MASK >> MP2723_VINDSCHG_SHIFT) * MP2723_VINDSCHG_LSB)
		volt = MP2723_VINDSCHG_BASE + (MP2723_VINDSCHG_MASK >> MP2723_VINDSCHG_SHIFT) * MP2723_VINDSCHG_LSB;

	val = ((volt - MP2723_VINDSCHG_BASE) / MP2723_VINDSCHG_LSB) << MP2723_VINDSCHG_SHIFT;

	return mp2723_update_bits(mp, MP2723_REG_03, MP2723_VINDSCHG_MASK, val);

}
EXPORT_SYMBOL_GPL(mp2723_set_otg_volt);

static int mp2723_set_otg_current(struct mp2723 *mp, int curr)
{
	u8 temp;

	if (curr == 500)
		temp = MP2723_IINDSCHG_0_5A;
	else if (curr == 800)
		temp = MP2723_IINDSCHG_0_8A;
	else if (curr == 1100)
		temp = MP2723_IINDSCHG_1_1A;
	else if (curr == 1500)
		temp = MP2723_IINDSCHG_1_5A;
	else
		temp = MP2723_IINDSCHG_0_5A;

	return mp2723_update_bits(mp, MP2723_REG_03, MP2723_IINDSCHG_MASK, temp << MP2723_IINDSCHG_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_otg_current);

static int mp2723_enable_charger(struct mp2723 *mp)
{
	int ret;
	u8 val = MP2723_CHG_ENABLE << MP2723_CHG_CONFIG_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_04, MP2723_CHG_CONFIG_MASK, val);
	if (ret == 0)
		mp->status |= MP2723_STATUS_CHARGE_ENABLE;
	return ret;
}

static int mp2723_disable_charger(struct mp2723 *mp)
{
	int ret;
	u8 val = MP2723_CHG_DISABLE << MP2723_CHG_CONFIG_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_04, MP2723_CHG_CONFIG_MASK, val);
	if (ret == 0)
		mp->status &= ~MP2723_STATUS_CHARGE_ENABLE;
	return ret;
}
EXPORT_SYMBOL_GPL(mp2723_disable_charger);

/* interfaces that can be called by other module */
int mp2723_adc_start(struct mp2723 *mp, bool oneshot)
{
	u8 val;
	int ret;

	ret = mp2723_read_byte(mp, &val, MP2723_REG_03);
	if (ret < 0) {
		dev_err(mp->dev, "%s failed to read register 0x03:%d\n", __func__, ret);
		return ret;
	}

	if (oneshot) {
		ret = mp2723_update_bits(mp, MP2723_REG_03, MP2723_ADC_RATE_MASK, MP2723_ADC_CONTINUE_DISABLE << MP2723_ADC_RATE_SHIFT);
		ret = mp2723_update_bits(mp, MP2723_REG_03, MP2723_ADC_START_MASK, MP2723_ADC_START_ENABLE << MP2723_ADC_START_SHIFT);
	} else {
		ret = mp2723_update_bits(mp, MP2723_REG_03, MP2723_ADC_RATE_MASK, MP2723_ADC_CONTINUE_ENABLE << MP2723_ADC_RATE_SHIFT);
		ret = mp2723_update_bits(mp, MP2723_REG_03, MP2723_ADC_START_MASK, MP2723_ADC_START_ENABLE << MP2723_ADC_START_SHIFT);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(mp2723_adc_start);

int mp2723_adc_stop(struct mp2723 *mp)
{
	int ret;
	ret = mp2723_update_bits(mp, MP2723_REG_03, MP2723_ADC_RATE_MASK, MP2723_ADC_CONTINUE_DISABLE << MP2723_ADC_RATE_SHIFT);
	ret = mp2723_update_bits(mp, MP2723_REG_02, MP2723_ADC_START_MASK, MP2723_ADC_START_DISABLE << MP2723_ADC_START_SHIFT);
	return ret;
}
EXPORT_SYMBOL_GPL(mp2723_adc_stop);

int mp2723_adc_read_battery_volt(struct mp2723 *mp)
{
	uint8_t val;
	int volt;
	int ret;
	ret = mp2723_read_byte(mp, &val, MP2723_REG_0E);
	if (ret < 0) {
		dev_err(mp->dev, "read battery voltage failed :%d\n", ret);
		return ret;
	} else{
		volt = MP2723_BATV_BASE + ((val & MP2723_BATV_MASK) >> MP2723_BATV_SHIFT) * MP2723_BATV_LSB ;
		return volt;
	}
}
EXPORT_SYMBOL_GPL(mp2723_adc_read_battery_volt);

int mp2723_adc_read_sys_volt(struct mp2723 *mp)
{
	uint8_t val;
	int volt;
	int ret;
	ret = mp2723_read_byte(mp, &val, MP2723_REG_0F);
	if (ret < 0) {
		dev_err(mp->dev, "read system voltage failed :%d\n", ret);
		return ret;
	} else{
		volt = MP2723_SYSV_BASE + ((val & MP2723_SYSV_MASK) >> MP2723_SYSV_SHIFT) * MP2723_SYSV_LSB;
		return volt;
	}
}
EXPORT_SYMBOL_GPL(mp2723_adc_read_sys_volt);

int mp2723_adc_read_vin_volt(struct mp2723 *mp)
{
	uint8_t val;
	int volt;
	int ret;
	ret = mp2723_read_byte(mp, &val, MP2723_REG_11);
	if (ret < 0) {
		dev_err(mp->dev, "read vbus voltage failed :%d\n", ret);
		return ret;
	} else{
		volt = MP2723_VIN_BASE + ((val & MP2723_VIN_MASK) >> MP2723_VIN_SHIFT) * MP2723_VIN_LSB ;
		return volt;
	}
}
EXPORT_SYMBOL_GPL(mp2723_adc_read_vin_volt);

int mp2723_adc_read_temperature(struct mp2723 *mp)
{
	uint8_t val;
	int temp;
	int ret;
	ret = mp2723_read_byte(mp, &val, MP2723_REG_10);
	if (ret < 0) {
		dev_err(mp->dev, "read temperature failed :%d\n", ret);
		return ret;
	} else{
		temp = MP2723_NTC_BASE + ((val & MP2723_NTC_MASK) >> MP2723_NTC_SHIFT) * MP2723_NTC_LSB ;
		return temp;
	}
}
EXPORT_SYMBOL_GPL(mp2723_adc_read_temperature);

int mp2723_adc_read_input_current(struct mp2723 *mp)	// uA
{
	uint8_t val;
	int volt;
	int ret;
	ret = mp2723_read_byte(mp, &val, MP2723_REG_13);
	if (ret < 0) {
		dev_err(mp->dev, "read input current failed :%d\n", ret);
		return ret;
	} else{
		volt = (int)(MP2723_IIN_BASE + ((val & MP2723_IIN_MASK) >> MP2723_IIN_SHIFT) * MP2723_IIN_LSB) ;
		return volt;
	}
}
EXPORT_SYMBOL_GPL(mp2723_adc_read_input_current);

int mp2723_adc_read_charge_current(struct mp2723 *mp)	// uA
{
	uint8_t val;
	int volt;
	int ret;
	ret = mp2723_read_byte(mp, &val, MP2723_REG_12);
	if (ret < 0) {
		dev_err(mp->dev, "read charge current failed :%d\n", ret);
		return ret;
	} else{
		volt = (int)(MP2723_ICHGR_BASE + ((val & MP2723_ICHGR_MASK) >> MP2723_ICHGR_SHIFT) * MP2723_ICHGR_LSB) ;
		return volt;
	}
}
EXPORT_SYMBOL_GPL(mp2723_adc_read_charge_current);

int mp2723_set_chargecurrent(struct mp2723 *mp, int curr)
{
	u8 ichg;

	ichg = (curr - MP2723_ICC_BASE)/MP2723_ICC_LSB;
	return mp2723_update_bits(mp, MP2723_REG_05, MP2723_ICC_MASK, ichg << MP2723_ICC_SHIFT);

}
EXPORT_SYMBOL_GPL(mp2723_set_chargecurrent);

int mp2723_get_chargecurrent_set_val(struct mp2723 *mp)
{
	uint8_t val;
	int volt;
	int ret;

	ret = mp2723_read_byte(mp, &val, MP2723_REG_05);
	if (ret < 0) {
		dev_err(mp->dev, "read chargecurrent setting val failed :%d\n", ret);
		return ret;
	} else {
		volt = (int)(MP2723_ICC_BASE + ((val & MP2723_ICC_MASK) >> MP2723_ICC_SHIFT) * MP2723_ICC_LSB) ;
		return volt;
	}
}
EXPORT_SYMBOL_GPL(mp2723_get_chargecurrent_set_val);

int mp2723_set_term_current(struct mp2723 *mp, int curr)
{
	u8 iterm;

	iterm = (curr - MP2723_ITERM_BASE) / MP2723_ITERM_LSB;

	return mp2723_update_bits(mp, MP2723_REG_06, MP2723_ITERM_MASK, iterm << MP2723_ITERM_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_term_current);

int mp2723_set_prechg_current(struct mp2723 *mp, int curr)
{
	u8 iprechg;

	iprechg = (curr - MP2723_IPRECHG_BASE) / MP2723_IPRECHG_LSB;

	return mp2723_update_bits(mp, MP2723_REG_06, MP2723_IPRECHG_MASK, iprechg << MP2723_IPRECHG_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_prechg_current);

int mp2723_set_batlowv(struct mp2723 *mp, int volt)
{
	u8 val;

	if (volt == 2800)
		val = MP2723_VBATTPRE_2_8V;
	else if (volt == 3000)
		val = MP2723_VBATTPRE_3V;
	else
		return -EINVAL;

	return mp2723_update_bits(mp, MP2723_REG_05, MP2723_VBATTPRE_MASK, val << MP2723_VBATTPRE_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_batlowv);

int mp2723_set_chargevoltage(struct mp2723 *mp, int volt)
{
	u8 val;

	val = (volt - MP2723_VBATTREG_BASE)/MP2723_VBATTREG_LSB;
	return mp2723_update_bits(mp, MP2723_REG_07, MP2723_VBATTREG_MASK, val << MP2723_VBATTREG_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_chargevoltage);

int mp2723_set_recharge_offset_voltage(struct mp2723 *mp, int offset)
{
	u8 val;

	if (offset == 100)
		val = MP2723_VRECHG_100MV;
	else if (offset == 200)
		val = MP2723_VRECHG_200MV;
	else
		return -EINVAL;
      
    	return mp2723_update_bits(mp, MP2723_REG_07, MP2723_VRECHG_MASK, val << MP2723_VRECHG_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_recharge_offset_voltage);

int mp2723_set_vsys_min(struct mp2723 *mp, int volt)
{
	u8 val;

	if (volt == 3000)
		val = MP2723_VSYSMIN_3V;
	else if (volt == 3150)
		val = MP2723_VSYSMIN_3_15V;
	else if (volt == 3300)
		val = MP2723_VSYSMIN_3_3V;
	else if (volt == 3450)
		val = MP2723_VSYSMIN_3_45V;
	else if (volt == 3525)
		val = MP2723_VSYSMIN_3_525V;
	else if (volt == 3600)
		val = MP2723_VSYSMIN_3_6V;
	else if (volt == 3675)
		val = MP2723_VSYSMIN_3_675V;
	else if (volt == 3750)
		val = MP2723_VSYSMIN_3_75V;
	else 
		return -EINVAL;

	return mp2723_update_bits(mp, MP2723_REG_04, MP2723_VSYSMIN_MASK, val << MP2723_VSYSMIN_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_vsys_min);

int mp2723_set_input_volt_limit(struct mp2723 *mp, int volt)
{
	u8 val;
	val = (volt - MP2723_VINMIN_BASE) / MP2723_VINMIN_LSB;
	return mp2723_update_bits(mp, MP2723_REG_01, MP2723_VINMIN_MASK, val << MP2723_VINMIN_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_input_volt_limit);

int mp2723_set_input_current_limit(struct mp2723 *mp, int curr)
{
	u8 val;

	val = (curr - MP2723_IINLIM_BASE) / MP2723_IINLIM_LSB;
	return mp2723_update_bits(mp, MP2723_REG_00, MP2723_IINLIM_MASK, val << MP2723_IINLIM_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_input_current_limit);

// TODO
// int mp2723_set_vindpm_offset(struct mp2723 *mp, int offset)
// {
// 	u8 val;

// 	val = (offset - MP2723_VINDPMOS_BASE)/MP2723_VINDPMOS_LSB;
// 	return mp2723_update_bits(mp, MP2723_REG_01, MP2723_VINDPMOS_MASK, val << MP2723_VINDPMOS_SHIFT);
// }
// EXPORT_SYMBOL_GPL(mp2723_set_vindpm_offset);

int mp2723_get_charging_status(struct mp2723 *mp)
{
	u8 val = 0;
	int ret;

	ret = mp2723_read_byte(mp, &val, MP2723_REG_0C);
	if (ret < 0) {
		dev_err(mp->dev, "%s Failed to read register 0x0b:%d\n", __func__, ret);
		return ret;
	}
	val &= MP2723_CHG_STAT_MASK;
	val >>= MP2723_CHG_STAT_SHIFT;
	return val;
}
EXPORT_SYMBOL_GPL(mp2723_get_charging_status);

void mp2723_set_otg(struct mp2723 *mp, int enable)
{
	int ret;

	if (enable) {
		ret = mp2723_enable_otg(mp);
		if (ret < 0) {
			dev_err(mp->dev, "%s:Failed to enable otg-%d\n", __func__, ret);
			return;
		}
	} else{
		ret = mp2723_disable_otg(mp);
		if (ret < 0)
			dev_err(mp->dev, "%s:Failed to disable otg-%d\n", __func__, ret);
	}
}
EXPORT_SYMBOL_GPL(mp2723_set_otg);

int mp2723_set_watchdog_timer(struct mp2723 *mp, u8 timeout)
{
	return mp2723_update_bits(mp, MP2723_REG_08, MP2723_WDT_MASK, (u8)((timeout - MP2723_WDT_BASE) / MP2723_WDT_LSB) << MP2723_WDT_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_watchdog_timer);

int mp2723_disable_watchdog_timer(struct mp2723 *mp)
{
	u8 val = MP2723_WDT_DISABLE << MP2723_WDT_SHIFT;

	return mp2723_update_bits(mp, MP2723_REG_08, MP2723_WDT_MASK, val);
}
EXPORT_SYMBOL_GPL(mp2723_disable_watchdog_timer);

int mp2723_reset_watchdog_timer(struct mp2723 *mp)
{
	u8 val = MP2723_WDT_RESET << MP2723_WDT_RST_SHIFT;

	return mp2723_update_bits(mp, MP2723_REG_08, MP2723_WDT_RST_MASK, val);
}
EXPORT_SYMBOL_GPL(mp2723_reset_watchdog_timer);

int mp2723_set_force_dpdm(struct mp2723 *mp, int enable)
{
	int ret;
	u8 val;

	if (enable)
		val = MP2723_USB_DET_ENABLE << MP2723_USB_DET_EN_SHIFT;
	else
		val = MP2723_USB_DET_DISABLE << MP2723_USB_DET_EN_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_0B, MP2723_USB_DET_EN_MASK, val);
	if (ret)
		return ret;

	msleep(20);/*TODO: how much time needed to finish dpdm detect?*/
	return 0;

}
EXPORT_SYMBOL_GPL(mp2723_set_force_dpdm);

int mp2723_reset_chip(struct mp2723 *mp)
{
	int ret;
	u8 val = MP2723_RESET << MP2723_RESET_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_01, MP2723_RESET_MASK, val);
	return ret;
}
EXPORT_SYMBOL_GPL(mp2723_reset_chip);

int mp2723_enter_ship_mode(struct mp2723 *mp)
{
	int ret;
	u8 val = MP2723_BATFET_DIS_OFF << MP2723_BATFET_DIS_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_0A, MP2723_BATFET_DIS_MASK, val);
	return ret;

}
EXPORT_SYMBOL_GPL(mp2723_enter_ship_mode);

static int mp2723_enter_ship_mode_delay(struct mp2723 *mp)
{
	int ret;
	u8 val;
	
	val = MP2723_SLEEP_DELAY_10S << MP2723_SLEEP_DELAY_SHIFT;
	ret = mp2723_update_bits(mp, MP2723_REG_02, MP2723_SLEEP_DELAY_MASK, val);
	
	val = MP2723_BATFET_DIS_OFF << MP2723_BATFET_DIS_SHIFT;
	ret = mp2723_update_bits(mp, MP2723_REG_0A, MP2723_BATFET_DIS_MASK, val);
	
	val = MP2723_FET_OFF_TIME_4S << MP2723_FET_OFF_TIME_SHIFT;
	ret = mp2723_update_bits(mp, MP2723_REG_0A, MP2723_FET_OFF_TIME_MASK, val);
	
	ret = mp2723_read_byte(mp, &val, MP2723_REG_0A);
	
	printk("kernel_debug:func:%s:line:%d,reg_0x0a=0x%2x.\n", __func__,__LINE__,val);
	return ret;
}

static int mp2723_clean_ship_mode_no_delay(struct mp2723 *mp)
{
	int ret;
	u8  val;
	
	val = MP2723_SLEEP_NO_DELAY << MP2723_SLEEP_DELAY_SHIFT;
	ret = mp2723_update_bits(mp, MP2723_REG_02, MP2723_SLEEP_DELAY_MASK, val);
	
	val = MP2723_BATFET_DIS_ON << MP2723_BATFET_DIS_SHIFT;
	ret = mp2723_update_bits(mp, MP2723_REG_0A, MP2723_BATFET_DIS_MASK, val);
	
	printk("kernel_debug:func:%s:line:%d.\n", __func__,__LINE__);	
	return ret;
}

int mp2723_enter_hiz_mode(struct mp2723 *mp)
{
	u8 val = MP2723_HIZ_ENABLE << MP2723_ENHIZ_SHIFT;

	return mp2723_update_bits(mp, MP2723_REG_00, MP2723_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(mp2723_enter_hiz_mode);

int mp2723_exit_hiz_mode(struct mp2723 *mp)
{

	u8 val = MP2723_HIZ_DISABLE << MP2723_ENHIZ_SHIFT;

	return mp2723_update_bits(mp, MP2723_REG_00, MP2723_ENHIZ_MASK, val);

}
EXPORT_SYMBOL_GPL(mp2723_exit_hiz_mode);

int mp2723_get_hiz_mode(struct mp2723 *mp, u8 *state)
{
	u8 val;
	int ret;

	ret = mp2723_read_byte(mp, &val, MP2723_REG_00);
	if (ret)
		return ret;
	*state = (val & MP2723_ENHIZ_MASK) >> MP2723_ENHIZ_SHIFT;

	return 0;
}
EXPORT_SYMBOL_GPL(mp2723_get_hiz_mode);

static int mp2723_auto_ico(struct mp2723 *mp)
{
	u8 val;
	int ret;

	val = MP2723_AICO_EN_MASK << MP2723_AICO_EN_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_02, MP2723_AICO_EN_MASK, val);

	return ret;
}
EXPORT_SYMBOL_GPL(mp2723_auto_ico);

static int mp2723_check_auto_ico_done(struct mp2723 *mp)
{
	u8 val;
	int ret;

	ret = mp2723_read_byte(mp, &val, MP2723_REG_15);
	if (ret)
		return ret;

	if (val & MP2723_AICO_STAT_MASK)
		return 0;  /* in progress*/ 
	else
		return 1;   /*finished*/
}
EXPORT_SYMBOL_GPL(mp2723_check_auto_ico_done);

static int mp2723_enable_term(struct mp2723* mp, bool enable)
{
	u8 val;
	int ret;

	if (enable)
		val = MP2723_TERM_ENABLE << MP2723_EN_TERM_SHIFT;
	else
		val = MP2723_TERM_DISABLE << MP2723_EN_TERM_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_08, MP2723_EN_TERM_MASK, val);

	return ret;
}
EXPORT_SYMBOL_GPL(mp2723_enable_term);

static int mp2723_enable_ico(struct mp2723* mp, bool enable)
{
	u8 val;
	int ret;
	
	if (enable)
		val = MP2723_AICO_EN_ENABLE << MP2723_AICO_EN_SHIFT;
	else
		val = MP2723_AICO_EN_DISABLE << MP2723_AICO_EN_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_02, MP2723_AICO_EN_MASK, val);

	return ret;

}
EXPORT_SYMBOL_GPL(mp2723_enable_ico);

static int mp2723_enable_otg_ntc(struct mp2723* mp, bool enable)
{
	u8 val;
	int ret;
	
	if (enable)
		val = MP2723_EN_OTG_NTC_ENABLE << MP2723_EN_OTG_NTC_SHIFT;
	else
		val = MP2723_EN_OTG_NTC_DISABLE << MP2723_EN_OTG_NTC_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_02, MP2723_EN_OTG_NTC_MASK, val);

	return ret;

}
EXPORT_SYMBOL_GPL(mp2723_enable_otg_ntc);

static int mp2723_enable_chg_ntc(struct mp2723* mp, bool enable)
{
	u8 val;
	int ret;
	
	if (enable)
		val = MP2723_EN_CHG_NTC_ENABLE << MP2723_EN_CHG_NTC_SHIFT;
	else
		val = MP2723_EN_CHG_NTC_DISABLE << MP2723_EN_CHG_NTC_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_02, MP2723_EN_CHG_NTC_MASK, val);

	return ret;

}
EXPORT_SYMBOL_GPL(mp2723_enable_chg_ntc);

static int mp2723_set_ntc_opt(struct mp2723* mp, int type)
{
	u8 val;
	int ret;
	
	if (type == 1)
		val = MP2723_NTC_OPT_PCB << MP2723_NTC_OPT_SHIFT;
	else
		val = MP2723_NTC_OPT_BATTERY << MP2723_NTC_OPT_SHIFT;

	ret = mp2723_update_bits(mp, MP2723_REG_02, MP2723_NTC_OPT_MASK, val);

	return ret;

}
EXPORT_SYMBOL_GPL(mp2723_set_ntc_opt);

static int mp2723_read_idpm_limit(struct mp2723 *mp)
{
	uint8_t val;
	int curr;
	int ret;

	ret = mp2723_read_byte(mp, &val, MP2723_REG_14);
	if (ret < 0) {
		dev_err(mp->dev, "read idpm limit failed :%d\n", ret);
		return ret;
	} else{
		curr = MP2723_IDPM_LIM_BASE + ((val & MP2723_IDPM_LIM_MASK) >> MP2723_IDPM_LIM_SHIFT) * MP2723_IDPM_LIM_LSB ;
		return curr;
	}
}
EXPORT_SYMBOL_GPL(mp2723_read_idpm_limit);

static bool mp2723_is_charge_done(struct mp2723 *mp)
{
	int ret;
	u8 val;

	ret = mp2723_read_byte(mp, &val, MP2723_REG_0C);
	if (ret < 0) {
		dev_err(mp->dev, "%s:read REG0B failed :%d\n", __func__, ret);
		return false;
	}
	val &= MP2723_CHG_STAT_MASK;
	val >>= MP2723_CHG_STAT_SHIFT;

	return (val == MP2723_CHG_STAT_CHGDONE);
}
EXPORT_SYMBOL_GPL(mp2723_is_charge_done);

int mp2723_set_treg(struct mp2723 *mp, int treg)
{
	u8 val;

	switch (treg) {
	case 60:
		val = MP2723_THERMALREG_60C;
		break ;
	case 80:
		val = MP2723_THERMALREG_80C;
		break ;
	case 100:
		val = MP2723_THERMALREG_100C;
		break ;
	case 120:
		val = MP2723_THERMALREG_120C;
		break ;
	default:
		return -EINVAL;
	}

	return mp2723_update_bits(mp, MP2723_REG_02, MP2723_THERMALREG_MASK, val << MP2723_THERMALREG_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_treg);

int mp2723_set_charging_safety_timer(struct mp2723 *mp, int hour)
{
	u8 val;

	switch (hour) {
	case 5:
		val = MP2723_CHG_TIMER_5HOURS;
		break ;
	case 8:
		val = MP2723_CHG_TIMER_8HOURS;
		break ;
	case 12:
		val = MP2723_CHG_TIMER_12HOURS;
		break ;
	case 20:
		val = MP2723_CHG_TIMER_20HOURS;
		break ;
	default:
		hour = 12;
		val = MP2723_CHG_TIMER_12HOURS;
		break ;
	}

	mp2723_update_bits(mp, MP2723_REG_08, MP2723_EN_TIMER_MASK, \
		MP2723_EN_TIMER_ENABLE << MP2723_EN_TIMER_SHIFT);
	mp2723_update_bits(mp, MP2723_REG_0A, MP2723_TMR2X_EN_MASK, \
		MP2723_TMR2X_EN_DISABLE << MP2723_TMR2X_EN_SHIFT);
	return mp2723_update_bits(mp, MP2723_REG_08, MP2723_CHG_TIMER_MASK, \
		val << MP2723_CHG_TIMER_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_set_charging_safety_timer);

static int mp2723_IINLIM_VINMIN_reset_enable(struct mp2723 *mp)
{
	return mp2723_update_bits(mp, MP2723_REG_18, MP2723_IINLIM_VINMIN_RSTEN_MASK, 1 << MP2723_IINLIM_VINMIN_RSTEN_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_IINLIM_VINMIN_reset_enable);

static int mp2723_IINLIM_VINMIN_reset_disable(struct mp2723 *mp)
{
	return mp2723_update_bits(mp, MP2723_REG_18, MP2723_IINLIM_VINMIN_RSTEN_MASK, 0 << MP2723_IINLIM_VINMIN_RSTEN_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_IINLIM_VINMIN_reset_disable);

static int mp2723_input_switch_off(struct mp2723 *mp)
{
	return mp2723_update_bits(mp, MP2723_REG_0B, MP2723_USB_DET_EN_MASK, MP2723_USB_DET_ENABLE << MP2723_USB_DET_EN_SHIFT);
}
EXPORT_SYMBOL_GPL(mp2723_input_switch_off);

static int mp2723_init_device(struct mp2723 *mp)
{
	int ret;
	u8 reg_val;

	/*common initialization*/
	mp2723_disable_watchdog_timer(mp);
	mp2723_clean_ship_mode_no_delay(mp);
	mp2723_set_force_dpdm(mp, mp->cfg.enable_detect_dpdm);
	mp2723_enable_term(mp, mp->cfg.enable_term);
	mp2723_enable_ico(mp, mp->cfg.enable_ico);
	if (mp->cfg.enable_otg) {
		ret = mp2723_enable_otg(mp);
		if (ret < 0) {
			dev_err(mp->dev, "%s:Failed to enable otg:%d\n", __func__, ret);
			return ret;
		}
	} else {
		ret = mp2723_disable_otg(mp);
		if (ret < 0) {
			dev_err(mp->dev, "%s:Failed to disable otg:%d\n", __func__, ret);
			return ret;
		}
	}

	ret = mp2723_set_term_current(mp, mp->cfg.term_current);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set termination current:%d\n", __func__, ret);
		return ret;
	}

	ret = mp2723_set_chargevoltage(mp, mp->cfg.charge_voltage);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set charge voltage:%d\n", __func__, ret);
		return ret;
	}

	ret = mp2723_set_chargecurrent(mp, MP2723_ICC_MIN);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set charge current:%d\n", __func__, ret);
		return ret;
	}

	ret = mp2723_set_recharge_offset_voltage(mp, mp->cfg.rechg_threshold_offset); 
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set recharge_offset:%d\n", __func__, ret);
		return ret;
	}

	ret = mp2723_set_batlowv(mp, mp->cfg.prechg_to_fastchg_threshold);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set prechg_to_fastchg_threshold:%d\n", \
			__func__, ret);
		return ret;
	}

	ret = mp2723_set_prechg_current(mp, mp->cfg.prechg_current);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set prechg_current:%d\n", __func__, ret);
		return ret;
	}
    
	ret = mp2723_set_vsys_min(mp, mp->cfg.vsys_min);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set vsys_min:%d\n", __func__, ret);
		return ret;
	}
	
	ret = mp2723_set_treg(mp, mp->cfg.treg);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set treg:%d\n", __func__, ret);
		return ret;
	}

	mp->status &= ~MP2723_STATUS_PLUGIN;
	mp2723_set_input_volt_limit(mp, MP2723_VIN_LIMIT_DEFAULT);
	mp2723_set_input_current_limit(mp, MP2723_IIN_LIMIT_DEFAULT);

	ret = mp2723_enable_charger(mp);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to enable charger:%d\n", __func__, ret);
		return ret;
	}
	mp->chage_enable = true;

	/*Start ADC 1s Continuous Conversion*/
	mp2723_adc_start(mp, false);

	mp2723_set_charging_safety_timer(mp, 12);

	/*read fault register to clear the reg*/
	mp2723_read_byte(mp, &reg_val, MP2723_REG_0D);
	mp2723_read_byte(mp, &reg_val, MP2723_REG_0D);

	ret = mp2723_IINLIM_VINMIN_reset_enable(mp);
	if (ret)
		return ret;

	return ret;
}


static int mp2723_charge_type(struct mp2723 *mp)
{
	u8 val = 0;

	mp2723_read_byte(mp, &val, MP2723_REG_0C);
	val &= MP2723_CHG_STAT_MASK;
	val >>= MP2723_CHG_STAT_SHIFT;

	switch (val) {
	case MP2723_CHG_STAT_CCCHG:
		return POWER_SUPPLY_CHARGE_TYPE_FAST;
	case MP2723_CHG_STAT_TRICHG:
		return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
	case MP2723_CHG_STAT_CHGDONE:
	case MP2723_CHG_STAT_NOCHG:
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
	default:
		/* impossible run to here */
		return POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
	}
}

static int mp2723_charge_status(struct mp2723 *mp)
{
	u8 val = 0;

	mp2723_read_byte(mp, &val, MP2723_REG_0C);
	val &= MP2723_CHG_STAT_MASK;
	val >>= MP2723_CHG_STAT_SHIFT;

	switch (val) {
	case MP2723_CHG_STAT_CCCHG:
	case MP2723_CHG_STAT_TRICHG:
		return POWER_SUPPLY_STATUS_CHARGING;
	case MP2723_CHG_STAT_CHGDONE:
		/* if battery no online, status is MP2723_CHRG_STAT_CHGDONE */
		return POWER_SUPPLY_STATUS_FULL;
	case MP2723_CHG_STAT_NOCHG:
		return POWER_SUPPLY_STATUS_NOT_CHARGING;
	default:
		/* impossible run to here */
		return POWER_SUPPLY_STATUS_NOT_CHARGING;
	}
}

static void mp2723_get_charge_fault(struct mp2723 * mp, u8 status, u8 fault)
{
	u8 temp;

	if (!fault) {
		mp->charge_fault = MP2723_CHG_OK;
		return;
	}

	/*FIXME:Maybe there are multiple fault at the same time*/

	if (fault & MP2723_FAULT_WDT_MASK) {
		mp->charge_fault = MP2723_WATCHDOG_FAULT;
		return ;
	}
	if (fault & MP2723_FAULT_BAT_MASK) {
		mp->charge_fault = MP2723_BATTERY_ERR;
		return ;
	}
	temp = fault & MP2723_FAULT_NTC_MASK;
	temp >>= MP2723_FAULT_NTC_SHIFT;
	if (temp) {
        switch (temp) {
		case MP2723_FAULT_NTC_WARM:
			mp->charge_fault = MP2723_CHG_TS_WARM;
			return;
		case MP2723_FAULT_NTC_COOL:
			mp->charge_fault = MP2723_CHG_TS_COOL;
			return;
		case MP2723_FAULT_NTC_COLD:
			mp->charge_fault = MP2723_CHG_TS_COLD;
			return;
		case MP2723_FAULT_NTC_HOT:                
            mp->charge_fault = MP2723_CHG_TS_HOT;
            return;
		}  
	}

	if (fault & MP2723_FAULT_INPUT_MASK) {
		mp->charge_fault = MP2723_CHG_INPUT_ERR;
		return ;
	}

	if (fault & MP2723_FAULT_THERM_SHUTDOWN_MASK) {
		mp->charge_fault = MP2723_CHG_THERMAL_SHUTDOWN;
		return ;
	}

	return ;
}

static int mp2723_health_status(struct mp2723 *mp)
{
	if (mp->charge_fault == MP2723_WATCHDOG_FAULT)
		return POWER_SUPPLY_HEALTH_WATCHDOG_TIMER_EXPIRE;
	else if (mp->charge_fault == MP2723_BATTERY_ERR)
		return POWER_SUPPLY_HEALTH_BATTERY_ERROR;
	else if (mp->charge_fault == MP2723_CHG_TS_COLD)
		return POWER_SUPPLY_HEALTH_COLD;
	else if (mp->charge_fault == MP2723_CHG_TS_HOT)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (mp->charge_fault == MP2723_CHG_INPUT_ERR && (mp->status & MP2723_STATUS_PLUGIN))
		return POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	else if (mp->charge_fault == MP2723_CHG_THERMAL_SHUTDOWN)
		return POWER_SUPPLY_HEALTH_CHARGE_IC_ERROR;
	else if (mp->charge_fault == MP2723_CHG_TIMEOUT)
		return POWER_SUPPLY_HEALTH_SAFETY_TIMER_EXPIRE;
	else 
		return POWER_SUPPLY_HEALTH_GOOD;
}

static int mp2723_charger_is_present(struct mp2723 *mp)
{
	u8 status = 0;
	int ret;
	int vbus_type_tmp;
	static int last_vbus_type = MP2723_VBUS_NONSTAND;

	ret = mp2723_read_byte(mp, &status, MP2723_REG_0C);
	if (ret) 
		return last_vbus_type;

	vbus_type_tmp = (status & MP2723_VIN_STAT_MASK) >> MP2723_VIN_STAT_SHIFT;
	if(vbus_type_tmp == MP2723_VBUS_NONE) {	
			ret = mp2723_adc_read_vin_volt(mp);
			if(ret >= MP2723_NO_INPUT_VOL_TH) {
				msleep(2000);
				ret = mp2723_read_byte(mp, &status, MP2723_REG_0C);
				vbus_type_tmp = (status & MP2723_VIN_STAT_MASK) >> MP2723_VIN_STAT_SHIFT;
				if(vbus_type_tmp)
				{
					vbus_type_tmp = last_vbus_type;	
				}			
		}
	}
	else {
		last_vbus_type = vbus_type_tmp;
	}

	return vbus_type_tmp;
}

static enum power_supply_property mp2723_charger_props[] = {
	POWER_SUPPLY_PROP_CHARGE_TYPE, /* Charger status output */
	POWER_SUPPLY_PROP_ONLINE, /* External power source */
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
};


static int mp2723_usb_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		mp->vbus_type = mp2723_charger_is_present(mp);
		if (mp->vbus_type == MP2723_VBUS_USB_SDP || mp->vbus_type == MP2723_VBUS_USB_CDP)
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = mp->charge_type;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = mp->charge_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = mp->health_status;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mp2723_wall_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		mp->vbus_type = mp2723_charger_is_present(mp);
		if (!mp->cfg.power_supply_type_usb) {
			/* usb supply power also detect to be adapter */
			if (mp->vbus_type != MP2723_VBUS_NONE) {
				val->intval = 1;
			}	
			else {
				val->intval = 0;
			}		
		} 
		else {
			if ((mp->vbus_type == MP2723_VBUS_NONSTAND)
				|| (mp->vbus_type == MP2723_VBUS_APPLE_1_0)
				|| (mp->vbus_type == MP2723_VBUS_APPLE_2_1)
				|| (mp->vbus_type == MP2723_VBUS_APPLE_2_4)
				|| (mp->vbus_type == MP2723_VBUS_USB_DCP)) {

				val->intval = 1;
			}
			else {
				val->intval = 0;
			}	
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = mp->charge_type;	
		break;
	case POWER_SUPPLY_PROP_CHARGE_ENABLE:		
		val->intval = mp->chage_enable;
		break ;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = mp->charge_status;
		break ;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = mp->health_status;
		break ;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mp2723_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	int ret = 0;
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGE_ENABLE:
		if (val->intval == 1) {
			mp->chage_enable = true;
			ret = mp2723_enable_charger(mp);
			dev_info(mp->dev, "%s() enable charge:%d\n", __func__, ret);
		} else if (val->intval == 0) {
			mp->chage_enable = false;
			ret = mp2723_disable_charger(mp);
			dev_info(mp->dev, "%s() disable charge:%d\n", __func__, ret);
		}
		break ;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		if (val->intval != 0) {
			if (val->intval < MP2723_VBATTREG_BASE || val->intval > MP2723_VBATTREG_MAX) {
				dev_info(mp->dev, "%s() ERR set charge voltage, Invalid voltage: %dmV\n", \
				__func__, val->intval);
				return -EINVAL;
			}

			mp->cfg.charge_voltage = val->intval;
			ret = mp2723_set_chargevoltage(mp, mp->cfg.charge_voltage);
			dev_info(mp->dev, "%s() set charge voltage:%dmV, ret:%d\n", \
				__func__, mp->cfg.charge_voltage, ret);
		}
		break ;
	case POWER_SUPPLY_PROP_TERM_CHARGE_CURRENT:
		if (val->intval != 0) {
			mp->cfg.term_current = val->intval;
			ret = mp2723_set_term_current(mp, mp->cfg.term_current);
			dev_info(mp->dev, "%s() set term current:%d, ret:%d\n", \
				__func__, mp->cfg.term_current, ret);
		}
		break ;
	case POWER_SUPPLY_PROP_ENTER_SHIP_MODE:
		mp2723_enter_ship_mode_delay(mp);
		break ;
	default:
		break;
	}

	return ret;
}

static struct power_supply_desc psy_desc_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.properties = mp2723_charger_props,
	.num_properties = ARRAY_SIZE(mp2723_charger_props),
	.get_property = mp2723_usb_get_property,
	.set_property = mp2723_set_property,
	.external_power_changed = NULL,
};

static struct power_supply_desc psy_desc_adapter = {
	.name = "adapter",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties = mp2723_charger_props,
	.num_properties = ARRAY_SIZE(mp2723_charger_props),
	.get_property = mp2723_wall_get_property,
	.set_property = mp2723_set_property,
	.external_power_changed = NULL,
};

static int mp2723_psy_register(struct mp2723 *mp)
{
	struct power_supply_config psy_cfg = {};
	psy_cfg.drv_data = mp;
	psy_cfg.num_supplicants = 0;

	if (mp->cfg.power_supply_type_usb) {
		mp->usb = power_supply_register(mp->dev, &psy_desc_usb, &psy_cfg);
		if (IS_ERR(mp->usb)) {
			dev_err(mp->dev, "%s:failed to register usb psy\n", __func__);
			return -EBUSY;
		}
	}

	if (mp->cfg.power_supply_type_adapter) {
		mp->wall = power_supply_register(mp->dev, &psy_desc_adapter, &psy_cfg);
		if (IS_ERR(mp->wall)) {
			dev_err(mp->dev, "%s:failed to register wall psy\n", __func__);
			goto fail_1;
		}
	}

	return 0;

fail_1:
	if (mp->cfg.power_supply_type_usb)
		power_supply_unregister(mp->usb);

	return -EBUSY;
}

static void mp2723_psy_unregister(struct mp2723 *mp)
{
	if (mp->cfg.power_supply_type_usb && mp->usb)
		power_supply_unregister(mp->usb);
	if (mp->cfg.power_supply_type_adapter && mp->wall)
		power_supply_unregister(mp->wall);
}


static ssize_t mp2723_show_registers(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 addr;
	u8 val;
	u8 tmpbuf[300];
	int len;
	int idx = 0;
	int ret ;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	idx = snprintf(buf, PAGE_SIZE, "%s:\n", "Charger IC");
	for (addr = 0x0; addr <= 0x18; addr++) {
		ret = mp2723_read_byte(mp, &val, addr);
		if (ret == 0) {
			len = snprintf(tmpbuf, PAGE_SIZE - idx,"Reg[0x%.2x] = 0x%.2x\n", addr, val);
			memcpy(&buf[idx], tmpbuf, len);
			idx += len;
		}
	}

	return idx;
}

static ssize_t mp2723_store_registers(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{

	int ret;
	unsigned int reg;
	unsigned int val;

	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	ret = sscanf(buf, "%x %x", &reg, &val);
	printk("reg=0x%02x val=0x%02x\n", reg, val);
	if (ret == 2 && reg <= 0x18) {
		mp2723_write_byte(mp, (unsigned char)reg, (unsigned char)val);
	}

	return count;
}

static ssize_t mp2723_show_vbus_volt(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	val = mp2723_adc_read_vin_volt(mp);
	if (val < 0)
		return val;
	else
		return scnprintf(buf, PAGE_SIZE, "%d\n", val);           
}

static ssize_t mp2723_show_sys_volt(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	val = mp2723_adc_read_sys_volt(mp);
 	if (val < 0)
		return val;
	else
		return scnprintf(buf, PAGE_SIZE, "%d\n", val);           
}

static ssize_t mp2723_show_battery_volt(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	val = mp2723_adc_read_battery_volt(mp);
 	if (val < 0)
		return val;
	else
		return scnprintf(buf, PAGE_SIZE, "%d\n", val);           
}

static ssize_t mp2723_show_charge_current_now(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	val = mp2723_adc_read_charge_current(mp);
	if (val < 0)
		return val;
	else
		return scnprintf(buf, PAGE_SIZE, "%d\n", val);           
}

static ssize_t mp2723_show_idpm_limit(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	val = mp2723_read_idpm_limit(mp);
	if (val < 0)
		return val;
	else
		return scnprintf(buf, PAGE_SIZE, "%d\n", val);           
}

static ssize_t mp2723_show_charge_fault(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	return scnprintf(buf, PAGE_SIZE, "%d\n", mp->charge_fault);
}

static ssize_t mp2723_show_input_enable(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val;
	u8 state;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	val = mp2723_get_hiz_mode(mp,&state);
	if (val < 0)
		return val;
	else
		return scnprintf(buf, PAGE_SIZE, "%d\n", !state);           
}

static ssize_t mp2723_store_input_enable(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int val,ret;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	if (sscanf(buf, "%d", &val) == 1) {
		if (val)
			ret = mp2723_exit_hiz_mode(mp);            
		else
			ret = mp2723_enter_hiz_mode(mp);

		if (ret)
			return -EINVAL;
		else
			return count ;
	} else {
		return -EINVAL;
	}
}


static ssize_t mp2723_show_charge_current_set(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int val;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	val = mp2723_get_chargecurrent_set_val(mp);
	if (val < 0)
		return val;
	else
		return scnprintf(buf, PAGE_SIZE, "%d\n", val);           
}

static ssize_t mp2723_store_charge_current_set(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int val,ret;
	struct power_supply *psy = dev_get_drvdata(dev);
	struct mp2723 *mp = power_supply_get_drvdata(psy);

	sscanf(buf, "%d", &val);
	if (val <= MP2723_ICC_MAX) {
		mp->cfg.charge_current = val;
		ret = mp2723_set_chargecurrent(mp, val);
		if (ret)
			return ret;
		else
			return count;
	} else {
		return -EINVAL;
	}
}

static DEVICE_ATTR(registers, 0600, mp2723_show_registers, mp2723_store_registers);
static DEVICE_ATTR(vbus_volt, S_IRUGO, mp2723_show_vbus_volt, NULL);
static DEVICE_ATTR(sys_volt, S_IRUGO, mp2723_show_sys_volt, NULL);
static DEVICE_ATTR(battery_volt, S_IRUGO, mp2723_show_battery_volt, NULL);
static DEVICE_ATTR(current_now, 0444, mp2723_show_charge_current_now, NULL);
static DEVICE_ATTR(idpm_limit, S_IRUGO, mp2723_show_idpm_limit, NULL);
static DEVICE_ATTR(charge_fault, S_IRUGO, mp2723_show_charge_fault, NULL);
static DEVICE_ATTR(input_enable, S_IRUGO | S_IWUSR, mp2723_show_input_enable, \
	mp2723_store_input_enable);
static DEVICE_ATTR(charge_cur_limit, 0644, mp2723_show_charge_current_set, \
	mp2723_store_charge_current_set);

static struct attribute *mp2723_attributes[] = {
	&dev_attr_registers.attr,
	&dev_attr_vbus_volt.attr,
	&dev_attr_sys_volt.attr,
	&dev_attr_battery_volt.attr,
	&dev_attr_current_now.attr,
	&dev_attr_idpm_limit.attr,
	&dev_attr_charge_fault.attr,
	&dev_attr_input_enable.attr,
	&dev_attr_charge_cur_limit.attr,
	NULL,
};

static const struct attribute_group mp2723_attr_group = {
	.attrs = mp2723_attributes,
};


static int mp2723_parse_dt(struct device *dev, struct mp2723 *mp)
{
	int ret;
	struct device_node *np = dev->of_node;
#if 0
	ret = of_property_read_u32(np, "mp2723,vbus-volt-high-level", &pe.high_volt_level);
	if (ret)
		return ret;

	ret = of_property_read_u32(np, "mp2723,vbus-volt-low-level", &pe.low_volt_level);
	if (ret)
		return ret;

	ret = of_property_read_u32(np, "mp2723,vbat-min-volt-to-tuneup", &pe.vbat_min_volt);
	if (ret)
		return ret;
#endif

	mp->cfg.enable_detect_dpdm = of_property_read_bool(np, "enable-detect-dpdm");
	mp->cfg.enable_otg = of_property_read_bool(np, "enable-otg");
	mp->cfg.enable_term = of_property_read_bool(np, "enable-termination");
	mp->cfg.enable_ico = of_property_read_bool(np, "enable-ico");
	mp->cfg.power_supply_type_adapter = of_property_read_bool(np, "power-supply-adapter");
	mp->cfg.power_supply_type_usb = of_property_read_bool(np, "power-supply-usb");

	ret = of_property_read_u32(np, "charge-voltage", &mp->cfg.charge_voltage);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() charge_voltage:%dmV\n",__func__, mp->cfg.charge_voltage);

	ret = of_property_read_u32(np, "charge-current", &mp->cfg.charge_current);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() charge_current:%dmA\n",__func__, mp->cfg.charge_current);

	ret = of_property_read_u32(np, "term-current", &mp->cfg.term_current);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() term_current:%dmA\n",__func__, mp->cfg.term_current);

	ret = of_property_read_u32(np, "prechg-current", &mp->cfg.prechg_current);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() prechg_current:%dmA\n",__func__, mp->cfg.prechg_current);

	ret =  of_property_read_u32(np, "prechg-to-fastchg-threshold", \
		&mp->cfg.prechg_to_fastchg_threshold);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() prechg_to_fastchg_threshold:%dmV\n",__func__, \
		mp->cfg.prechg_to_fastchg_threshold);

	ret =  of_property_read_u32(np, "rechg-threshold-offset", \
		&mp->cfg.rechg_threshold_offset);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() rechg_threshold_offset:%dmV\n",__func__, \
		mp->cfg.rechg_threshold_offset);

	ret =  of_property_read_u32(np, "vsys-min", &mp->cfg.vsys_min);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() vsys_min:%dmV\n",__func__, mp->cfg.vsys_min);

	ret =  of_property_read_u32(np, "input-current-limit", &mp->cfg.iin_limit);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() iin_limit:%dmA\n",__func__, mp->cfg.iin_limit);

	ret =  of_property_read_u32(np, "vin-limit", &mp->cfg.vin_limit);
	if (ret)
		mp->cfg.vin_limit = MP2723_VIN_LIMIT_DEFAULT;
	dev_info(mp->dev, "%s() vin_limit:%dmV\n",__func__, mp->cfg.vin_limit);

	ret =  of_property_read_u32(np, "thermal-regulation-threshold", \
		&mp->cfg.treg);
	if (ret)
		return ret;
	dev_info(mp->dev, "%s() treg:%d C\n",__func__, mp->cfg.treg);

	mp->irq_pin = of_get_named_gpio(np, "irq-pin", 0);
	if (mp->irq_pin < 0)
		return -1;
	dev_info(mp->dev, "%s() irq gpio:%d\n",__func__, mp->irq_pin);
	return 0;
}

static int mp2723_detect_device(struct mp2723 *mp)
{
	int ret;
	u8 data;

	ret = mp2723_read_byte(mp, &data, MP2723_REG_17);
	if (ret == 0) {
		mp->part_no = (data & MP2723_PN_MASK) >> MP2723_PN_SHIFT;
	}

	return ret;
}

// static int mp2723_read_batt_rsoc(struct mp2723 *mp)
// {
// 	union power_supply_propval ret = {0,};

// 	if (!mp->batt_psy) 
// 		mp->batt_psy = power_supply_get_by_name("battery");

// 	if (mp->batt_psy) {
// 		mp->batt_psy->desc->get_property(mp->batt_psy, POWER_SUPPLY_PROP_CAPACITY,&ret);
// 		return ret.intval;
// 	} else {
// 		return 50;
// 	}
// }

static void mp2723_adjust_vin_limit(struct mp2723 *mp)
{
	int ret;
	int vin_limit = mp->cfg.vin_limit;
	if (vin_limit < mp->cfg.charge_voltage + 150)
		vin_limit = mp->cfg.charge_voltage + 150;

	if (vin_limit < MP2723_VIN_LIMIT_MIN)
		vin_limit = MP2723_VIN_LIMIT_MIN;

	if (vin_limit > MP2723_VIN_LIMIT_MAX)
		vin_limit = MP2723_VIN_LIMIT_MAX;

	ret = mp2723_set_input_volt_limit(mp, vin_limit);
	if (ret < 0)
		dev_err(mp->dev, "%s:Set absolute vindpm threshold %d Failed:%d\n", __func__, vin_limit, ret);
	else
		dev_info(mp->dev, "%s:Set absolute vindpm threshold %d successfully\n", __func__, vin_limit);
}

#if 0
static void mp2723_adapter_in_workfunc(struct work_struct *work)
{
	struct mp2723 *mp = container_of(work, struct mp2723, adapter_in_work);
	int ret;

	mp2723_adjust_vin_limit(mp);
	ret = mp2723_set_input_current_limit(mp, mp->cfg.iin_limit);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set input current limit:%d\n", __func__, mp->cfg.iin_limit);
	} else {
	 	dev_info(mp->dev, "%s: Set input current limit to %dmA successfully\n", __func__, mp->cfg.iin_limit);
		if (mp->cfg.enable_ico)
	  		schedule_delayed_work(&mp->ico_work, 0);
	}

	msleep(5);

	ret = mp2723_set_chargecurrent(mp, mp->cfg.charge_current);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set charge current:%d\n", __func__, ret);
	}

	schedule_delayed_work(&mp->monitor_work, 0);
}

static void mp2723_adapter_out_workfunc(struct work_struct *work)
{
	struct mp2723 *mp = container_of(work, struct mp2723, adapter_out_work);
	int ret;

	ret = mp2723_set_input_volt_limit(mp, MP2723_VIN_LIMIT_DEFAULT);
	if (ret < 0)
		dev_err(mp->dev,"%s:reset vindpm threshold to default failed:%d\n",__func__,ret);
	else
		dev_info(mp->dev,"%s:reset vindpm threshold to default(%dmV) successfully\n",
			__func__, MP2723_VIN_LIMIT_DEFAULT);

	ret = mp2723_set_input_current_limit(mp, MP2723_IIN_LIMIT_DEFAULT);
	if (ret < 0) 
		dev_err(mp->dev, "%s:Failed to set input current limit to default\n", __func__);
	else
		dev_info(mp->dev, "%s: Set input current limit to default(%dmA) successfully\n",
			 __func__, MP2723_IIN_LIMIT_DEFAULT);

	cancel_delayed_work_sync(&mp->monitor_work);
}
#endif

static void mp2723_adapter_in(struct mp2723 *mp)
{
	int ret;

	mp2723_adjust_vin_limit(mp);
	ret = mp2723_set_input_current_limit(mp, mp->cfg.iin_limit);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set input current limit:%d\n", __func__, mp->cfg.iin_limit);
	} else {
	 	dev_info(mp->dev, "%s: Set input current limit to %dmA successfully\n", __func__, mp->cfg.iin_limit);
		if (mp->cfg.enable_ico)
	  		schedule_delayed_work(&mp->ico_work, 0);
	}

	msleep(5);

	ret = mp2723_set_chargecurrent(mp, mp->cfg.charge_current);
	if (ret < 0) {
		dev_err(mp->dev, "%s:Failed to set charge current:%d\n", __func__, ret);
	}

	schedule_delayed_work(&mp->monitor_work, 0);
	//printk("kernel_debug:func:%s:line:%d.\n", __func__,__LINE__);
}

static void mp2723_adapter_out(struct mp2723 *mp)
{
	int ret;

	ret = mp2723_set_input_volt_limit(mp, MP2723_VIN_LIMIT_DEFAULT);
	if (ret < 0)
		dev_err(mp->dev,"%s:reset vindpm threshold to default failed:%d\n",__func__,ret);
	else
		dev_info(mp->dev,"%s:reset vindpm threshold to default(%dmV) successfully\n",
			__func__, MP2723_VIN_LIMIT_DEFAULT);

	ret = mp2723_set_input_current_limit(mp, MP2723_IIN_LIMIT_DEFAULT);
	if (ret < 0) 
		dev_err(mp->dev, "%s:Failed to set input current limit to default\n", __func__);
	else
		dev_info(mp->dev, "%s: Set input current limit to default(%dmA) successfully\n",
			 __func__, MP2723_IIN_LIMIT_DEFAULT);

	cancel_delayed_work_sync(&mp->monitor_work);
	//printk("kernel_debug:func:%s:line:%d.\n", __func__,__LINE__);
}

static void mp2723_ico_workfunc(struct work_struct *work)
{
	struct mp2723 *mp = container_of(work, struct mp2723, ico_work.work);
	int ret;
	int idpm;
	u8 status;
	static bool ico_issued = false;
	int chg_current;

	chg_current = mp2723_adc_read_charge_current(mp);
	dev_info(mp->dev, "%s(), chg_current:%d\n", __func__, chg_current);
	if (chg_current == 0) {
		/*maybe battery no online */
		ico_issued = false;
		return ;
	}

	if (!ico_issued) {
		ret = mp2723_auto_ico(mp);
		if (ret < 0) {
			schedule_delayed_work(&mp->ico_work, HZ); /* retry 1 second later*/
			dev_info(mp->dev, "%s:ICO command issued failed:%d\n", __func__, ret);
		} else {
			ico_issued = true;
			schedule_delayed_work(&mp->ico_work, 3 * HZ);
			dev_info(mp->dev, "%s:ICO command issued successfully\n", __func__);
		}
	} else {
		ico_issued = false;
		ret = mp2723_check_auto_ico_done(mp);
		if (ret) {/*ico done*/
			ret = mp2723_read_byte(mp, &status, MP2723_REG_14);
			if (ret == 0) {
				idpm = ((status & MP2723_IDPM_LIM_MASK) >> MP2723_IDPM_LIM_SHIFT) * MP2723_IDPM_LIM_LSB + MP2723_IDPM_LIM_BASE;
				dev_info(mp->dev, "%s:ICO done, result is:%d mA\n", __func__, idpm);
			}
		} else {
			ret = mp2723_read_byte(mp, &status, MP2723_REG_14);
			if (ret == 0) {
				idpm = ((status & MP2723_IDPM_LIM_MASK) >> MP2723_IDPM_LIM_SHIFT) * MP2723_IDPM_LIM_LSB + MP2723_IDPM_LIM_BASE;
				dev_info(mp->dev, "%s:ICO no done, result is:%d mA\n", __func__, idpm);
			}
		}
	}
	//printk("kernel_debug:func:%s:line:%d.\n", __func__,__LINE__);
}

#if 0
static void mp2723_check_pe_tuneup_workfunc(struct work_struct *work)
{
	struct mp2723 *mp = container_of(work, struct mp2723, check_pe_tuneup_work.work);

	if (!pe.enable) {
		/* if vbus_type is MP2723_VBUS_USB_DCP,
		maybe wait for set absolute vindpm threshold done;
		ic auto  do ico as supply detect */
		schedule_delayed_work(&mp->ico_work, 5 * HZ);
		return;
	}

	mp->vbat_volt = mp2723_adc_read_battery_volt(mp);
	mp->rsoc = mp2723_read_batt_rsoc(mp); 

	if (mp->vbat_volt > pe.vbat_min_volt && mp->rsoc < 95) {
		dev_info(mp->dev, "%s:trying to tune up vbus voltage\n", __func__);
		pe.target_volt = pe.high_volt_level;
		pe.tune_up_volt = true;
		pe.tune_down_volt = false;
		pe.tune_done = false;
		pe.tune_count = 0;
		pe.tune_fail = false;
		schedule_delayed_work(&mp->pe_volt_tune_work, 0);
	} else if (mp->rsoc >= 95) {
		schedule_delayed_work(&mp->ico_work, 0);
	} else {
		/* wait battery voltage up enough to check again */
		schedule_delayed_work(&mp->check_pe_tuneup_work, 2*HZ);
	}
}
#endif

#if 0
static void mp2723_tune_volt_workfunc(struct work_struct *work)
{
	struct mp2723 *mp = container_of(work, struct mp2723, pe_volt_tune_work.work);
	int ret = 0;
	static bool pumpx_cmd_issued;

	mp->vbus_volt = mp2723_adc_read_vin_volt(mp);

	dev_info(mp->dev, "%s:vbus voltage:%d, Tune Target Volt:%d\n", __func__, mp->vbus_volt, pe.target_volt);

	if ((pe.tune_up_volt && mp->vbus_volt > pe.target_volt) ||
	    (pe.tune_down_volt && mp->vbus_volt < pe.target_volt)) {
		dev_info(mp->dev, "%s:voltage tune successfully\n", __func__);
		pe.tune_done = true;
		mp2723_adjust_vin_limit(mp);
		if (pe.tune_up_volt)
			schedule_delayed_work(&mp->ico_work, 0);
		return;
	}

	if (pe.tune_count > 10) {
		dev_info(mp->dev, "%s:voltage tune failed,reach max retry count\n", __func__);
		pe.tune_fail = true;
		mp2723_adjust_vin_limit(mp);

		if (pe.tune_up_volt)
			schedule_delayed_work(&mp->ico_work, 0);
		return;
	}

	if (!pumpx_cmd_issued) {
		if (pe.tune_up_volt)
			ret = mp2723_pumpx_increase_volt(mp);
		else if (pe.tune_down_volt)
			ret =  mp2723_pumpx_decrease_volt(mp);
		if (ret) {
			schedule_delayed_work(&mp->pe_volt_tune_work, HZ);
		} else {
			dev_info(mp->dev, "%s:pumpx command issued.\n", __func__);
			pumpx_cmd_issued = true;
			pe.tune_count++;
			schedule_delayed_work(&mp->pe_volt_tune_work, 3*HZ);
		}
	} else {
		if (pe.tune_up_volt)
			ret = mp2723_pumpx_increase_volt_done(mp);
		else if (pe.tune_down_volt)
			ret = mp2723_pumpx_decrease_volt_done(mp);
		if (ret == 0) {
			dev_info(mp->dev, "%s:pumpx command finishedd!\n", __func__);
			mp2723_adjust_vin_limit(mp);
			pumpx_cmd_issued = 0;
		}
		schedule_delayed_work(&mp->pe_volt_tune_work, HZ);
	}
}
#endif

// static int read_batt_vol(struct mp2723 *mp)
// {
// 	union power_supply_propval vol = {0};
// 	int err;

// 	if (!mp->batt_psy) {
// 		mp->batt_psy = power_supply_get_by_name("battery");
// 		if (!mp->batt_psy) {
// 			printk("get battery psy failed!\n");
// 			return -ENODEV;
// 		}
// 	}

// 	if (mp->batt_psy) {
// 		err = power_supply_get_property(mp->batt_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &vol);
// 		if (err) {
// 			printk("read battery voltage failed! err: %d\n", err);
// 			return err;
// 		}

// 		return vol.intval;	// uV 
// 	}

// 	return -EIO;
// }

static void mp2723_monitor_workfunc(struct work_struct *work)
{
	struct mp2723 *mp = container_of(work, struct mp2723, monitor_work.work);
	u8 status = 0;
	int ret, val;
	static u8 last_status = 0;

#if 0
	int chg_current;

	/* watchdog is disable
	mp2723_reset_watchdog_timer(mp);*/

	// mp->rsoc = mp2723_read_batt_rsoc(mp);

	mp->vbus_volt = mp2723_adc_read_vin_volt(mp);
	mp->vbat_volt = mp2723_adc_read_battery_volt(mp);
	chg_current = mp2723_adc_read_charge_current(mp);
#endif
	 mp->charge_type = mp2723_charge_type(mp);
	 mp->charge_status = mp2723_charge_status(mp);
	 mp->health_status = mp2723_health_status(mp);

	ret = mp2723_read_byte(mp, &status, MP2723_REG_14);
	if (ret == 0 && (status & MP2723_VDPM_STAT_MASK)) {
		if ((status & MP2723_VDPM_STAT_MASK) != (last_status & MP2723_VDPM_STAT_MASK))
			dev_info(mp->dev, "%s:VINDPM occurred\n", __func__);/* input v too low */
		val = mp2723_adc_read_input_current(mp);
		if (val > 0 && val < 200000) {	
			// in VinPPM and Iin less than 200mA, we detect adapet out.
			dev_info(mp->dev, "%s:turnoff input switch.\n", __func__);
			mp2723_input_switch_off(mp);
		}
	}

	if (ret == 0 && (status & MP2723_IDPM_STAT_MASK) 
			&& (status & MP2723_IDPM_STAT_MASK) != (last_status & MP2723_IDPM_STAT_MASK))
		dev_info(mp->dev, "%s:IINDPM occurred\n", __func__);/* input Current too big*/

	last_status = status;

#if 0
	if (mp->vbus_type == MP2723_VBUS_USB_DCP && mp->vbus_volt > pe.high_volt_level &&
	    mp->rsoc > 95 && !pe.tune_down_volt) {
		pe.tune_down_volt = true;
		pe.tune_up_volt = false;
		pe.target_volt = pe.low_volt_level;
		pe.tune_done = false;
		pe.tune_count = 0;
		pe.tune_fail = false;
		schedule_delayed_work(&mp->pe_volt_tune_work, 0);
	}
#endif
	/* read temperature,or any other check if need to decrease charge current*/	

#if 0
	int temperature;
	int sys_volt;
	u8 fault;
	int limit;
	int bat_vol;

	dev_info(mp->dev, "%s() vbus volt:%d\n", __func__, mp->vbus_volt);
	dev_info(mp->dev, "%s() vbat volt:%d,\n", __func__, mp->vbat_volt);
	dev_info(mp->dev, "%s() charge current:%d\n", __func__, chg_current);

	dev_info(mp->dev, "%s() vbus_type:%#x\n", __func__, mp->vbus_type);

	temperature = mp2723_adc_read_temperature(mp);
	dev_info(mp->dev, "%s() temperature:%d\n", __func__, temperature);
	sys_volt = mp2723_adc_read_sys_volt(mp);
	dev_info(mp->dev, "%s() sys_volt:%d\n", __func__, sys_volt);

	mp2723_read_byte(mp, &status, MP2723_REG_0B);
	mp2723_read_byte(mp, &fault, MP2723_REG_0C);

	dev_info(mp->dev, "%s() status:%#x, fault:%#x\n", __func__, status, fault);

	limit = mp2723_read_idpm_limit(mp);
	dev_info(mp->dev, "%s() limit:%d\n", __func__, limit);

	bat_vol = read_batt_vol(mp);
	dev_info(mp->dev, "%s() battery vol:%d\n", __func__, bat_vol);
#endif

	schedule_delayed_work(&mp->monitor_work, HZ);
}

static void mp2723_charger_irq_workfunc(struct work_struct *work)
{
	struct mp2723 *mp = container_of(work, struct mp2723, irq_work);
	u8 status = 0;
	u8 fault = 0;
	u8 safety_fault = 0;
	int ret;
	int vin_st;

	msleep(10);

	ret = mp2723_read_byte(mp, &fault, MP2723_REG_0D);
	if (ret)
		return;

	ret = mp2723_read_byte(mp, &safety_fault, MP2723_REG_17);
	if (ret)
		return;
	
	mp->charge_type = mp2723_charge_type(mp);
	mp->charge_status = mp2723_charge_status(mp);

	ret = mp2723_read_byte(mp, &status, MP2723_REG_0C);
	if (ret) 
		return;

	vin_st = (status & MP2723_VIN_STAT_MASK) >> MP2723_VIN_STAT_SHIFT;
	if ((vin_st == MP2723_VBUS_NONE) && (mp->status & MP2723_STATUS_PLUGIN)) {	
		dev_info(mp->dev, "%s:adapter removed\n", __func__);
		mp->status &= ~MP2723_STATUS_PLUGIN;
		//schedule_delayed_work(&mp->adapter_out_work, 0);
		mp2723_adapter_out(mp);
	} 
	else if ((vin_st != MP2723_VBUS_NONE) && !(mp->status & MP2723_STATUS_PLUGIN)) {
		dev_info(mp->dev, "%s:adapter plugged in\n", __func__);
		mp->status |= MP2723_STATUS_PLUGIN;
		//schedule_delayed_work(&mp->adapter_in_work, HZ);
		msleep(1000);
		mp2723_adapter_in(mp);
	}
	
	if ((status & MP2723_RNTCFLOAT_STAT_MASK) && !(mp->status & MP2723_STATUS_NTC_FLOAT))
		mp->status |= MP2723_STATUS_NTC_FLOAT;
	else if (!(status & MP2723_RNTCFLOAT_STAT_MASK) && (mp->status & MP2723_STATUS_NTC_FLOAT))
		mp->status &= ~MP2723_STATUS_NTC_FLOAT;
	
	if ((status & MP2723_THERM_STAT_MASK) && !(mp->status & MP2723_STATUS_THERMAL))
		mp->status |= MP2723_STATUS_THERMAL;
	else if (!(status & MP2723_THERM_STAT_MASK) && (mp->status & MP2723_STATUS_THERMAL))
		mp->status &= ~MP2723_STATUS_THERMAL;

	if (fault && !(mp->status & MP2723_STATUS_FAULT))
		mp->status |= MP2723_STATUS_FAULT;
	else if (!fault && (mp->status & MP2723_STATUS_FAULT))
		mp->status &= ~MP2723_STATUS_FAULT;

	if (fault)
		dev_info(mp->dev, "%s:charge fault:%02x, status:%02x\n", \
			__func__, fault, status);

	mp2723_get_charge_fault(mp, status, fault);

	if (safety_fault & MP2723_SAFTY_TIMER_MASK) {
		mp->charge_fault = MP2723_CHG_TIMEOUT;
	}

	mp->health_status = mp2723_health_status(mp);
	//printk("kernel_debug:func:%s:line:%d.\n", __func__,__LINE__);
}


static irqreturn_t mp2723_charger_interrupt(int irq, void *data)
{
	struct mp2723 *mp = data;

	queue_work(mp->adp_workqueue, &mp->irq_work);

	return IRQ_HANDLED;
}

static int mp2723_enable_typec_otg(struct regulator_dev *dev)
{
	struct mp2723 *mp = rdev_get_drvdata(dev);
	mp2723_enable_otg(mp);
}

static int mp2723_disable_typec_otg(struct regulator_dev *dev)
{
	struct mp2723 *mp = rdev_get_drvdata(dev);
	mp2723_disable_otg(mp);
}

static const struct regulator_ops mp2723_charger_vbus_ops = {
    .enable = mp2723_enable_typec_otg,
    .disable = mp2723_disable_typec_otg,
};

static const struct regulator_desc mp2723_charger_vbus_desc = {
    .name = "otg-vbus",
    .of_match = "otg-vbus",
    .type = REGULATOR_VOLTAGE,
    .owner = THIS_MODULE,
    .ops = &mp2723_charger_vbus_ops,
    .min_uV         = 4550000,
    .n_voltages     = ((5510000) - (4550000)) / (64000) + 1,
    .uV_step        = 64000,
    .linear_min_sel     = 0,
};

static int mp2723_charger_register_vbus_regulator(struct mp2723 *mp)
{
    struct regulator_config cfg = { };
    struct regulator_dev *reg;
    int ret = 0;

	printk("charger_debug:func:%s:line:%d.\n", __func__,__LINE__);
    cfg.dev = mp->dev;
    cfg.driver_data = mp;
	
    reg = devm_regulator_register(mp->dev,
                      &mp2723_charger_vbus_desc, &cfg);
    printk("charger_debug:func:%s:line:%d.\n", __func__,__LINE__);
	if (IS_ERR(reg)) {
        ret = PTR_ERR(reg);
        dev_err(mp->dev, "Can't register regulator:%d\n", ret);
    }
	printk("charger_debug:func:%s:line:%d.\n", __func__,__LINE__);
	
    return ret;
}

static int mp2723_charger_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct mp2723 *mp;
	int irqn;
	int ret;

	printk("kernel_debug:func:%s:line:%d.\n", __func__,__LINE__);
	mp = devm_kzalloc(&client->dev, sizeof(struct mp2723), GFP_KERNEL);
	if (!mp) {
		dev_err(&client->dev, "%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	mp->dev = &client->dev;
	mp->client = client;
	i2c_set_clientdata(client, mp);
	dev_set_drvdata(&client->dev, mp);

#if 0
	ret = mp2723_detect_device(mp);
	if (!ret && mp->part_no == MP2723) {
		mp->status |= MP2723_STATUS_EXIST;
		dev_info(mp->dev, "%s: charger device mp2723 detected\n", __func__);
	} else {
		dev_info(mp->dev, "%s: no mp2723 charger device found:%d\n", __func__, ret);
		return -ENODEV;
	}

	mp->batt_psy = power_supply_get_by_name("battery");
	if(mp->batt_psy == NULL)
		dev_info(mp->dev, "%s: no get power supply battery:%d\n", __func__, ret);
#endif

	if (client->dev.of_node) {
		ret = mp2723_parse_dt(&client->dev, mp);
		if (ret) {
			dev_err(mp->dev, "parse dt failed: %d\n", ret);
			goto err_0;
		}
	}

	ret = mp2723_init_device(mp);
	if (ret) {
		dev_err(mp->dev, "device init failure: %d\n", ret);
		goto err_0;
	}

	ret = gpio_request(mp->irq_pin, "mp2723 irq pin");
	if (ret) {
		dev_err(mp->dev, "%s: %d gpio request failed\n", __func__, mp->irq_pin);
		goto err_0;
	}
	gpio_direction_input(mp->irq_pin);

	irqn = gpio_to_irq(mp->irq_pin);
	if (irqn < 0) {
		dev_err(mp->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
		ret = irqn;
		goto err_1;
	}
	client->irq = irqn;

	ret = mp2723_psy_register(mp);
	if (ret)
		goto err_0;

	
	ret = mp2723_charger_register_vbus_regulator(mp);
    if (ret) {
        dev_err(mp->dev, "failed to register vbus regulator.\n");
        return ret;
    }
	
	INIT_WORK(&mp->irq_work, mp2723_charger_irq_workfunc);
//	INIT_DELAYED_WORK(&mp->adapter_in_work, mp2723_adapter_in_workfunc);
//	INIT_DELAYED_WORK(&mp->adapter_out_work, mp2723_adapter_out_workfunc);
	INIT_DELAYED_WORK(&mp->monitor_work, mp2723_monitor_workfunc);
	INIT_DELAYED_WORK(&mp->ico_work, mp2723_ico_workfunc);
//	INIT_DELAYED_WORK(&mp->pe_volt_tune_work, mp2723_tune_volt_workfunc);
//	INIT_DELAYED_WORK(&mp->check_pe_tuneup_work, mp2723_check_pe_tuneup_workfunc);
	mp->adp_workqueue = create_singlethread_workqueue("mp2723_thread");

	if (mp->cfg.power_supply_type_adapter) {
		ret = sysfs_create_group(&mp->wall->dev.kobj, &mp2723_attr_group);
		if (ret) {
			dev_err(mp->dev, "failed to register sysfs. err: %d\n", ret);
			goto err_sysfs_create;
		}
	}

	ret = request_irq(client->irq, mp2723_charger_interrupt, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "mp2723_charger_irq", mp);
	if (ret) {
		dev_err(mp->dev, "%s:Request IRQ %d failed: %d\n", __func__, client->irq, ret);
		goto err_irq;
	} else {
		dev_info(mp->dev, "%s:irq = %d\n", __func__, client->irq);
	}

	pe.enable = false;
	//mp->cfg.enter_hiz_mode_flag=0;
	queue_work(mp->adp_workqueue, &mp->irq_work);/*in case of adapter has been in when power off*/
	device_init_wakeup(mp->dev, 1);
	printk("kernel_debug:func:%s:line:%d.\n", __func__,__LINE__);
	return 0;

err_irq:
	if (mp->cfg.power_supply_type_adapter)
		sysfs_remove_group(&mp->wall->dev.kobj, &mp2723_attr_group);
err_sysfs_create:
	cancel_work_sync(&mp->irq_work);
	//cancel_delayed_work_sync(&mp->adapter_in_work);
	//cancel_delayed_work_sync(&mp->adapter_out_work);
	cancel_delayed_work_sync(&mp->monitor_work);
	cancel_delayed_work_sync(&mp->ico_work);
	// cancel_delayed_work_sync(&mp->check_pe_tuneup_work);
	// cancel_delayed_work_sync(&mp->pe_volt_tune_work);
	destroy_workqueue(mp->adp_workqueue);
err_1:
	gpio_free(mp->irq_pin);
err_0:
	return ret;
}

static int mp2723_charger_remove(struct i2c_client *client)
{
	struct mp2723 *mp = i2c_get_clientdata(client);

	dev_info(mp->dev, "%s: remove\n", __func__);

	mp2723_adc_stop(mp);

	free_irq(mp->client->irq, mp);
	gpio_free(mp->irq_pin);

	if (mp->cfg.power_supply_type_adapter)
		sysfs_remove_group(&mp->wall->dev.kobj, &mp2723_attr_group);

	mp2723_psy_unregister(mp);

	cancel_work_sync(&mp->irq_work);
	//cancel_delayed_work_sync(&mp->adapter_in_work);
	//cancel_delayed_work_sync(&mp->adapter_out_work);
	cancel_delayed_work_sync(&mp->monitor_work);
	cancel_delayed_work_sync(&mp->ico_work);
	// cancel_delayed_work_sync(&mp->check_pe_tuneup_work);
	// cancel_delayed_work_sync(&mp->pe_volt_tune_work);
	destroy_workqueue(mp->adp_workqueue);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mp2723_charger_suspend(struct device *dev)
{
	struct mp2723 *mp = dev_get_drvdata(dev);
	int irq = gpio_to_irq(mp->irq_pin);

	if (irq > 0) {
		disable_irq(irq);
		if (device_may_wakeup(dev)) {
			enable_irq_wake(irq);
		}
	}

	return 0;
}

static int mp2723_charger_resume(struct device *dev)
{
	struct mp2723 *mp = dev_get_drvdata(dev);
	int irq = gpio_to_irq(mp->irq_pin);

	if (irq > 0) {
		if (device_may_wakeup(dev)) {
			disable_irq_wake(irq);
		}
		enable_irq(irq);
	}

	return 0;
}

static struct dev_pm_ops mp2723_charger_pm = {
	.suspend = mp2723_charger_suspend,
	.resume = mp2723_charger_resume,
};
#define MP2723_CHARGER_PM 	&mp2723_charger_pm
#else
#define MP2723_CHARGER_PM	NULL
#endif

static struct of_device_id mp2723_charger_match_table[] = {
	{.compatible = "mp2723",},
	{},
};


static const struct i2c_device_id mp2723_charger_id[] = {
	{ "mp2723", MP2723 },
	{},
};

// MODULE_DEVICE_TABLE(i2c, mp2723_charger_id);

static struct i2c_driver mp2723_charger_driver = {
	.driver		= {
		.name	= "mp2723",
		.of_match_table = mp2723_charger_match_table,
		.pm = MP2723_CHARGER_PM,
	},
	.id_table	= mp2723_charger_id,
	.probe		= mp2723_charger_probe,
//	.shutdown   = mp2723_charger_shutdown,
	.remove		= mp2723_charger_remove,
};

module_i2c_driver(mp2723_charger_driver);

MODULE_DESCRIPTION("TI BQ2589x Charger Driver");
MODULE_LICENSE("GPL");
