#ifndef __BMS_H__
#define __BMS_H__

#include <linux/notifier.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/alarmtimer.h>

#define BMS_DEBUG

struct chg_info {
	int online;
	int type;
	int vbus;
	int ibus;
};

struct bat_info {
	int present;
	int status;
	int vol;
	int cur;
	int temp;
	int soc;
	int resistance;
};

struct bms_thermal_info {
	struct thermal_zone_device *tz;
	const char *name;
	int weight;
};

struct bms_thermal_chg_info {
	int temp;
	int max_chg_current;
};

struct bms_data {
	struct class *bms_class;
	struct device *dev;
	struct power_supply *usb_psy;
	struct power_supply *bat_psy;
	const char *usb_psy_name;
	const char *bat_psy_name;
	struct delayed_work dwork;
	wait_queue_head_t bms_wait_q;
	bool bms_wait_timeout;

	struct notifier_block psy_nb;
	struct notifier_block bms_nb;

	struct wakeup_source bms_ws;
	struct alarm bms_alarm;

	struct delayed_work thermal_dwork;
	struct bms_thermal_info *tinfo;
	int tinfo_cnt;
	struct bms_thermal_chg_info *tchg_info;
	int tchg_info_cnt;

	int skin_temp;
#ifdef BMS_DEBUG
	int skin_temp_ratio;
#endif

	bool bms_thermal_support;

	unsigned long long notify_code;
	unsigned long long chg_vote;

	unsigned long long charge_start_time;
	unsigned long long charge_total_time;

	struct chg_info chg_info;
	struct bat_info bat_info;

	int chg_ic_err;
	int gauge_ic_err;

	int max_chg_vol;
	int max_chg_time;
	int max_bat_vol;
	int min_bat_vol;
	int max_bat_temp;
	int min_bat_temp;
	int max_bat_cur;

	int suspend_current_ua;
};


#define BMS_NAME "pax_bms"

#define MAX_CHG_VOL			5500
#define MAX_CHG_TIME		(12 * 60 * 60)

#define MAX_BAT_VOL			4500
#define MIN_BAT_VOL			3450
#define MAX_BAT_TEMP		600
#define MIN_BAT_TEMP		-100
#define MAX_BAT_CUR			5000

#define BMS_POLL_INTERVAL 5000 //5s
#define BMS_DEBOUNCE	12


#define CHG_VOL_ANTI_SHAKE_STEP			500000		//unit: uV
#define BAT_CUR_ANTI_SHAKE_STEP			500000		//unit: uA
#define BAT_VOL_ANTI_SHAKE_STEP			500000		//unit: uV
#define BAT_TEMP_ANTI_SHAKE_STEP		20			//unit: 0.1度

#if 0
//A800
#define NC_CHG_VBUS_OV_STATUS  (0)
#define NC_CHG_BAT_OT_STATUS   (1)
#define NC_CHG_OC_STATUS       (2)
#define NC_CHG_BAT_OV_STATUS   (3)
#define NC_CHG_ST_TMO_STATUS   (4)
#define NC_CHG_BAT_LT_STATUS   (5)
#define NC_CHG_TYPEC_WD_STATUS (6)
#define NC_CHG_IBUS_OCP        (7)
#define NC_CHG_I2C_ERR         (8)
#define NC_CHG_VSYS_SHORT      (9)
#define NC_CHG_VSYS_OVP        (10)
#define NC_CHG_OTG_OVP         (11)
#define NC_CHG_OTG_UVP         (12)
#define NC_CHG_SAFETY_TIMEOUT  (13)
#endif

/*start: notify code, should compatible with A800 */
#define NC_CHG_OV				(0)
#define NC_BAT_OT				(1)
#define NC_BAT_OC				(2)
#define NC_BAT_OV				(3)
#define NC_BAT_UT				(5)
#define NC_CHG_IBUS_OCP         (7)
#define NC_CHG_I2C_ERR          (8)

#define NC_CHG_TMO				(13)
#define NC_BAT_I2C_ERR          (14)
#define NC_BAT_UV				(18)

#define NC_DISABLE_CHG_BY_USER  (63)
#define NC_MAX					(64)
#define DISABLE_ALL             0xFFFFFFFFFFFFFFFF
/*end: notify code */

static const char *const notify_type_name[NC_MAX] = {
	[NC_CHG_OV] = "NC_CHG_OV",
	[NC_CHG_TMO] = "NC_CHG_TMO",
	[NC_BAT_OV] = "NC_BAT_OV",
	[NC_BAT_OT] = "NC_BAT_OT",
	[NC_BAT_UT] = "NC_BAT_UT",
	[NC_BAT_OC] = "NC_BAT_OC",
	[NC_BAT_UV] = "NC_BAT_UV",
	[NC_CHG_IBUS_OCP] = "NC_CHG_IBUS_OCP",
	[NC_CHG_I2C_ERR] = "NC_CHG_I2C_ERR",
	[NC_BAT_I2C_ERR] = "NC_BAT_I2C_ERR",

	[NC_DISABLE_CHG_BY_USER] = "NC_DISABLE_CHG_BY_USER",
};

#endif
