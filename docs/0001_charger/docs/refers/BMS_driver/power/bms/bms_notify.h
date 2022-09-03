#ifndef __BMS_NOTIFY_H__
#define __BMS_NOTIFY_H__

#include <linux/notifier.h>
#include <linux/export.h>

#define SET_CHG_EN				_IOW('b', 0, int)
#define SET_POWER_PATH			_IOW('b', 1, int)
#define SET_CHG_IC_ERR			_IOW('b', 2, int)
#define SET_GUAGE_IC_ERR		_IOW('b', 3, int)

enum bms_chg_en {
	BMS_CHG_DISABLE = 0,
	BMS_CHG_ENABLE
};

enum bms_power_path {
	BMS_POWER_BY_CHARGER = 0,
	BMS_POWER_BY_BATTERY
};

extern int bms_notify_register_client(struct notifier_block *nb);
extern int bms_notify_unregister_client(struct notifier_block *nb);
extern int bms_notify_call_chain(unsigned long val, void *v);

#endif
