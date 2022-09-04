#define pr_fmt(fmt) "PAX_CHG: BMS "fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/sys.h>
#include <linux/sysfs.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/pm_wakeup.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>

#include "bms.h"
#include "bms_notify.h"
#include "thermal.h"

extern struct class *g_class_pax;
struct bms_data g_bms_data;

static void bms_wakeup(void);

static void bms_status_notify()
{
	char *env[2] = { "BMS_STAT=1", NULL };

	kobject_uevent_env(&g_bms_data.dev->kobj, KOBJ_CHANGE, env);
}

static int bms_chg_vote(int type, bool disable)
{
	int chg_en = BMS_CHG_ENABLE;

	if (type >= NC_MAX)
		return 0;

	if (disable) {
		g_bms_data.chg_vote |= (1 << type);
	}
	else {
		g_bms_data.chg_vote &= ~(1 << type);
	}

	if (g_bms_data.chg_vote)
		chg_en = BMS_CHG_DISABLE;

	bms_notify_call_chain(SET_CHG_EN, &chg_en);

	pr_info("type: %s, disable: %d, vote: 0x%x\n", notify_type_name[type], disable, g_bms_data.chg_vote);

	return 0;
}

static bool bms_notify_code_is_set(int type)
{
	if (g_bms_data.notify_code & (1 << type))
		return true;

	return false;
}

static int bms_set_notify_code(int type, bool set, bool update_vote)
{
	if (type >= NC_MAX)
		return 0;

	pr_info("notify_code: 0x%x\n", g_bms_data.notify_code);

	if (set) {
		g_bms_data.notify_code |= (1 << type);
	}
	else {
		g_bms_data.notify_code &= ~(1 << type);
	}

	pr_info("type: %s, set: %d, notify_code: 0x%x\n", notify_type_name[type], set, g_bms_data.notify_code);

	if (update_vote) {
		bms_chg_vote(type, set);
	}

	bms_status_notify();

	return 0;
}

static void bms_chg_vol_check()
{
	int ret;
	union power_supply_propval val = {0};
	static int abnormal_cnt = 0;
	static int normal_cnt = 0;

	if (!g_bms_data.usb_psy)
		return;

	ret = power_supply_get_property(g_bms_data.usb_psy, POWER_SUPPLY_PROP_ONLINE, &val);
	if (ret < 0)
		return;
	if (!val.intval) {
		abnormal_cnt = 0;
		normal_cnt = 0;
		return;
	}

	ret = power_supply_get_property(g_bms_data.usb_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
	if (ret < 0)
		return;
	g_bms_data.chg_info.vbus = val.intval;

	if (val.intval > g_bms_data.max_chg_vol) {
		normal_cnt = 0;
		if (!bms_notify_code_is_set(NC_CHG_OV)) {
			abnormal_cnt ++;
			if (abnormal_cnt > BMS_DEBOUNCE) {
				pr_err("vol: %d, max_chg_vol: %d, out of range.\n", val.intval, g_bms_data.max_chg_vol);
				bms_set_notify_code(NC_CHG_OV, true, true);
			}
		}
	}
	else {
		abnormal_cnt = 0;
		if (bms_notify_code_is_set(NC_CHG_OV)) {
			if (val.intval < (g_bms_data.max_chg_vol - CHG_VOL_ANTI_SHAKE_STEP)) {
				normal_cnt ++;
				if (normal_cnt > BMS_DEBOUNCE) {
					bms_set_notify_code(NC_CHG_OV, false, true);
				}
			}
			else {
				normal_cnt = 0;
			}
		}
	}
}

static void bms_bat_vol_check()
{
	int ret;
	union power_supply_propval val = {0};
	static int abnormal_cnt = 0;
	static int normal_cnt = 0;

	if (!g_bms_data.bat_psy)
		return;

	ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &val);
	if (ret < 0)
		return;
	g_bms_data.bat_info.vol = val.intval;

	ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_RESISTANCE, &val);
	if (!ret) {
		g_bms_data.bat_info.resistance = val.intval;
	}

	g_bms_data.bat_info.vol = g_bms_data.bat_info.vol - (g_bms_data.bat_info.cur / 1000) * (g_bms_data.bat_info.resistance / 1000);

	if (g_bms_data.bat_info.vol > g_bms_data.max_bat_vol) {
		normal_cnt = 0;
		if (!bms_notify_code_is_set(NC_BAT_OV)) {
			abnormal_cnt ++;
			pr_err("bat vol: %d, cnt: %d\n", g_bms_data.bat_info.vol, abnormal_cnt);
			if (abnormal_cnt > BMS_DEBOUNCE) {
				pr_err("vol: %d, max_bat_vol: %d, out of range.\n", g_bms_data.bat_info.vol, g_bms_data.max_bat_vol);
				bms_set_notify_code(NC_BAT_OV, true, true);
			}
		}
	}
	else if (g_bms_data.bat_info.vol < g_bms_data.min_bat_vol) {
		normal_cnt = 0;
		if (!g_bms_data.chg_info.online && !bms_notify_code_is_set(NC_BAT_UV)) {
			abnormal_cnt ++;
			pr_err("bat vol: %d, cnt: %d\n", g_bms_data.bat_info.vol, abnormal_cnt);
			if (abnormal_cnt > BMS_DEBOUNCE) {
				pr_err("vol: %d, min_bat_vol: %d, out of range.\n", g_bms_data.bat_info.vol, g_bms_data.min_bat_vol);
				bms_set_notify_code(NC_BAT_UV, true, false);
			}
		}
	}
	else {
		abnormal_cnt = 0;
		if (bms_notify_code_is_set(NC_BAT_OV)) {
			if (g_bms_data.bat_info.vol < (g_bms_data.max_bat_vol - BAT_VOL_ANTI_SHAKE_STEP)) {
				normal_cnt ++;
				if (normal_cnt > BMS_DEBOUNCE) {
					bms_set_notify_code(NC_BAT_OV, false, false);
				}
			}
			else {
				normal_cnt = 0;
			}
		}
	}
}

static void bms_bat_cur_check()
{
	int ret;
	union power_supply_propval val = {0};
	static int abnormal_cnt = 0;
	static int normal_cnt = 0;

	if (!g_bms_data.bat_psy)
		return;

	ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_CURRENT_NOW, &val);
	if (ret < 0)
		return;
	g_bms_data.bat_info.cur = val.intval;

	if (val.intval > g_bms_data.max_bat_cur) {
		normal_cnt = 0;
		if (!bms_notify_code_is_set(NC_BAT_OC)) {
			abnormal_cnt ++;
			if (abnormal_cnt > BMS_DEBOUNCE) {
				pr_err("cur: %d, max_bat_cur: %d, out of range.\n", val.intval, g_bms_data.max_bat_cur);
				bms_set_notify_code(NC_BAT_OC, true, true);
			}
		}
	}
	else {
		abnormal_cnt = 0;
		if (bms_notify_code_is_set(NC_BAT_OC)) {
			if (val.intval < (g_bms_data.max_bat_cur - BAT_CUR_ANTI_SHAKE_STEP)) {
				normal_cnt ++;
				if (normal_cnt > BMS_DEBOUNCE) {
					bms_set_notify_code(NC_BAT_OC, false, true);
				}
			}
			else {
				normal_cnt = 0;
			}
		}
	}
}

static void bms_bat_temp_check()
{
	int ret;
	union power_supply_propval val = {0};
	static int abnormal_cnt = 0;
	static int normal_cnt = 0;

	if (!g_bms_data.bat_psy)
		return;

	ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_TEMP, &val);
	if (ret < 0)
		return;
	g_bms_data.bat_info.temp = val.intval;

	if (val.intval > g_bms_data.max_bat_temp) {
		normal_cnt = 0;
		if (!bms_notify_code_is_set(NC_BAT_OT)) {
			abnormal_cnt ++;
			if (abnormal_cnt > BMS_DEBOUNCE) {
				pr_err("temp: %d, max_bat_temp: %d, out of range.\n", val.intval, g_bms_data.max_bat_temp);
				bms_set_notify_code(NC_BAT_OT, true, true);
			}
		}
	}
	else if (val.intval < g_bms_data.min_bat_temp) {
		normal_cnt = 0;
		if (!bms_notify_code_is_set(NC_BAT_UT)) {
			abnormal_cnt ++;
			if (abnormal_cnt > BMS_DEBOUNCE) {
				pr_err("temp: %d, min_bat_temp: %d, out of range.\n", val.intval, g_bms_data.min_bat_temp);
				bms_set_notify_code(NC_BAT_UT, true, true);
			}
		}
	}
	else {
		abnormal_cnt = 0;
		if (bms_notify_code_is_set(NC_BAT_OT)) {
			if (val.intval < (g_bms_data.max_bat_temp - BAT_TEMP_ANTI_SHAKE_STEP)) {
				normal_cnt ++;
				if (normal_cnt > BMS_DEBOUNCE) {
					bms_set_notify_code(NC_BAT_OT, false, true);
				}
			}
			else {
				normal_cnt = 0;
			}
		}
		else if (bms_notify_code_is_set(NC_BAT_UT)) {
			if (val.intval > (g_bms_data.min_bat_temp + BAT_TEMP_ANTI_SHAKE_STEP)) {
				normal_cnt ++;
				if (normal_cnt > BMS_DEBOUNCE) {
					bms_set_notify_code(NC_BAT_UT, false, true);
				}
			}
			else {
				normal_cnt = 0;
			}
		}
	}
}

static void bms_chg_time_check()
{
	int ret;
	union power_supply_propval val = {0};
	struct timespec time_now;

	if (!g_bms_data.bat_psy)
		return;

	ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_STATUS, &val);
	if (ret < 0)
		return;

	get_monotonic_boottime(&time_now);

	if (val.intval == POWER_SUPPLY_STATUS_CHARGING) {
		if (!g_bms_data.charge_start_time) {
			g_bms_data.charge_start_time = time_now.tv_sec; 
			pr_info("charge start: %u\n", g_bms_data.charge_start_time);
		}

		g_bms_data.charge_total_time = time_now.tv_sec - g_bms_data.charge_start_time;

		if (g_bms_data.charge_total_time > g_bms_data.max_chg_time) {
			if (g_bms_data.chg_info.type != POWER_SUPPLY_TYPE_USB) {
				if (!bms_notify_code_is_set(NC_CHG_TMO)) {
					pr_err("time: %u, max_chg_time: %u, out of range.\n", g_bms_data.charge_total_time, g_bms_data.max_chg_time);
					bms_set_notify_code(NC_CHG_TMO, true, true);
				}
			}
		}
	}
	else
	{
		if (g_bms_data.charge_start_time > 0) {
			pr_info("charge end: charge_start_time: %u, charge_total_time: %u\n",
					g_bms_data.charge_start_time, g_bms_data.charge_total_time);
		}
		g_bms_data.charge_start_time = 0;
		g_bms_data.charge_total_time = 0;
	}
}

static void bms_chg_ic_check()
{
	if (g_bms_data.chg_ic_err) {
		bms_set_notify_code(NC_CHG_I2C_ERR, true, true);
	}
}

static void bms_gauge_ic_check()
{
	if (g_bms_data.gauge_ic_err) {
		bms_set_notify_code(NC_BAT_I2C_ERR, true, true);
	}
}

static void bms_info_update()
{
	union power_supply_propval val = {0};
	int ret;

	if (g_bms_data.bat_psy) {

		ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_PRESENT, &val);
		if (ret == 0)
			g_bms_data.bat_info.present = val.intval;

		ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_CAPACITY, &val);
		if (ret == 0)
			g_bms_data.bat_info.soc = val.intval;
	}

	if (g_bms_data.usb_psy) {
		ret = power_supply_get_property(g_bms_data.usb_psy, POWER_SUPPLY_PROP_CURRENT_NOW, &val);
		if (ret == 0)
			g_bms_data.chg_info.ibus = val.intval;
	}

}

static void bms_dump()
{
	pr_info("%s: CHG [online: %d, type: %d, vol: %d, cur: %d, time: %ld], "
			"BAT [present: %d, status: %d, vol: %d, cur: %d, resistance: %d, temp: %d, soc: %d], "
			"OTHER [skin_temp: %d, chg_vote: 0x%0x, notify_code: 0x%0x], "
			"\n",
			__func__,
			/*CHG*/
			g_bms_data.chg_info.online, g_bms_data.chg_info.type, g_bms_data.chg_info.vbus, g_bms_data.chg_info.ibus,
			g_bms_data.charge_total_time,
			/*BAT*/
			g_bms_data.bat_info.present, g_bms_data.bat_info.status, g_bms_data.bat_info.vol, g_bms_data.bat_info.cur,
			g_bms_data.bat_info.resistance, g_bms_data.bat_info.temp, g_bms_data.bat_info.soc,
			/*OTHER*/
			g_bms_data.skin_temp, g_bms_data.chg_vote, g_bms_data.notify_code);
}

static int psy_notifier_call(struct notifier_block *self, unsigned long event, void *data)
{
	struct power_supply *psy = data;
	union power_supply_propval val = {0};
	int ret = 0;

	if (!strcmp(psy->desc->name, g_bms_data.bat_psy_name)) {
		bms_wakeup();

		ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_STATUS, &val);
		if (ret == 0) {
			g_bms_data.bat_info.status = val.intval;
			if (val.intval == POWER_SUPPLY_STATUS_CHARGING) {
				if (!g_bms_data.bms_ws.active) {
					__pm_stay_awake(&g_bms_data.bms_ws);
				}
			}
			else {
				if (g_bms_data.bms_ws.active) {
					__pm_relax(&g_bms_data.bms_ws);
				}
			}
		}
	} else if (!strcmp(psy->desc->name, g_bms_data.usb_psy_name)) {
		ret = power_supply_get_property(g_bms_data.usb_psy, POWER_SUPPLY_PROP_TYPE, &val);
		if (ret == 0) {
			g_bms_data.chg_info.type = val.intval;
		}

		ret = power_supply_get_property(g_bms_data.usb_psy, POWER_SUPPLY_PROP_ONLINE, &val);
		if (ret == 0) {
			if ((g_bms_data.chg_info.online != val.intval)) {
				//clear NC_DISABLE_CHG_BY_USER flag if adapter plug in/out
				bms_chg_vote(NC_DISABLE_CHG_BY_USER, 0);
			}
			g_bms_data.chg_info.online = val.intval;
		}
	}

	return 0;
}

static void bms_wakeup()
{
	g_bms_data.bms_wait_timeout = true;
	wake_up(&g_bms_data.bms_wait_q);
}

static void bms_dwork(struct work_struct *work)
{
	bms_info_update();
	bms_chg_vol_check();
	bms_bat_cur_check();
	bms_bat_vol_check();
	bms_bat_temp_check();
	bms_chg_time_check();
	bms_chg_ic_check();
	bms_gauge_ic_check();
	bms_thermal_check(&g_bms_data);
	bms_dump();
	wait_event_timeout(g_bms_data.bms_wait_q,
			(g_bms_data.bms_wait_timeout == true), msecs_to_jiffies(BMS_POLL_INTERVAL));
	g_bms_data.bms_wait_timeout = false;
	schedule_delayed_work(&g_bms_data.dwork, 0);
}

static int bms_notifier_call(struct notifier_block *self, unsigned long event, void *data)
{
	switch (event) {
		case SET_CHG_IC_ERR:
			g_bms_data.chg_ic_err = *(int *)data;
			pr_err("SET_CHG_IC_ERR: %d\n", g_bms_data.chg_ic_err);
			bms_wakeup();
			break;

		case SET_GUAGE_IC_ERR:
			g_bms_data.gauge_ic_err = *(int *)data;
			pr_err("SET_GUAGE_IC_ERR: %d\n", g_bms_data.gauge_ic_err);
			bms_wakeup();
			break;

		default:
			break;
	};

	return NOTIFY_DONE;
}

static enum alarmtimer_restart pax_battery_alarm_func(struct alarm *alarm, ktime_t ktime)
{
	pr_info("%s: enter\n", __func__);
	__pm_wakeup_event(&g_bms_data.bms_ws, 1000);

	return ALARMTIMER_NORESTART;
}

static int bms_start_alarm(unsigned long long secs)
{
	struct timespec time, time_now, end_time;
	ktime_t ktime;

	get_monotonic_boottime(&time_now);
	time.tv_sec = secs;
	time.tv_nsec = 0;
	end_time = timespec_add(time_now, time);
	ktime = ktime_set(end_time.tv_sec, end_time.tv_nsec);

	alarm_start(&g_bms_data.bms_alarm, ktime);

	return 0;
}

static int bms_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int bms_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long bms_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	void __user *user_data = (void __user *)arg;
	int ret = 0;
	int en = 1;

	switch (cmd) {
		case SET_CHG_EN:
			ret = copy_from_user(&en, user_data, sizeof(int));
			if (ret < 0) {
				pr_info("get chg en data failed.\n");
				ret = -1;
				break;
			}

			pr_info("SET_CHG_EN: %d\n", en);

			bms_chg_vote(NC_DISABLE_CHG_BY_USER, !en);

			break;
		case SET_POWER_PATH:
			ret = copy_from_user(&en, user_data, sizeof(int));
			if (ret < 0) {
				pr_info("get chg en data failed.\n");
				ret = -1;
				break;
			}

			pr_info("SET_POWER_PATH: %d\n", en);

			//bms_chg_vote(NC_DISABLE_CHG_BY_USER, !en);

			bms_notify_call_chain(SET_POWER_PATH, &en);

			break;
		default:
			pr_info("cmd: %u is not support.\n", cmd);
			ret = -1;
			break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long bms_compat_ioctl(struct file *file,
			unsigned int cmd, unsigned long arg)
{
	return bms_ioctl(file, cmd, arg);
}
#endif

static const struct file_operations bms_fops = {
	.owner = THIS_MODULE,
	.open = bms_open,
	.release = bms_release,
	.unlocked_ioctl = bms_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = bms_compat_ioctl,
#endif
};

static struct miscdevice bms_miscdev = {
	.minor      = MISC_DYNAMIC_MINOR,
	.name		= BMS_NAME,
	.fops		= &bms_fops,
};

#ifdef BMS_DEBUG
static ssize_t max_chg_vol_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d", &g_bms_data.max_chg_vol);
	pr_info("%d\n", g_bms_data.max_chg_vol);

	return size;
}
static ssize_t max_chg_vol_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.max_chg_vol);
}
static DEVICE_ATTR_RW(max_chg_vol);

static ssize_t max_chg_time_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d", &g_bms_data.max_chg_time);
	pr_info("%d\n", g_bms_data.max_chg_time);

	return size;
}
static ssize_t max_chg_time_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.max_chg_time);
}
static DEVICE_ATTR_RW(max_chg_time);

static ssize_t max_bat_vol_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d", &g_bms_data.max_bat_vol);
	pr_info("%d\n", g_bms_data.max_bat_vol);

	return size;
}
static ssize_t max_bat_vol_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.max_bat_vol);
}
static DEVICE_ATTR_RW(max_bat_vol);

static ssize_t min_bat_vol_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d", &g_bms_data.min_bat_vol);
	pr_info("%d\n", g_bms_data.min_bat_vol);

	return size;
}
static ssize_t min_bat_vol_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.min_bat_vol);
}
static DEVICE_ATTR_RW(min_bat_vol);

static ssize_t max_bat_temp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d", &g_bms_data.max_bat_temp);
	pr_info("%d\n", g_bms_data.max_bat_temp);

	return size;
}
static ssize_t max_bat_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.max_bat_temp);
}
static DEVICE_ATTR_RW(max_bat_temp);

static ssize_t min_bat_temp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d", &g_bms_data.min_bat_temp);
	pr_info("%d\n", g_bms_data.min_bat_temp);

	return size;
}
static ssize_t min_bat_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.min_bat_temp);
}
static DEVICE_ATTR_RW(min_bat_temp);

static ssize_t max_bat_cur_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d", &g_bms_data.max_bat_cur);
	pr_info("%d\n", g_bms_data.max_bat_cur);

	return size;
}
static ssize_t max_bat_cur_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.max_bat_cur);
}
static DEVICE_ATTR_RW(max_bat_cur);

static ssize_t skin_temp_ratio_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	sscanf(buf, "%d", &g_bms_data.skin_temp_ratio);

	return size;
}
static ssize_t skin_temp_ratio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.skin_temp_ratio);
}
static DEVICE_ATTR_RW(skin_temp_ratio);

static ssize_t thermal_weights_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int i;
	char *token = NULL;
	char *tmp_buf = (char *)buf;

	for (i = 0; i < g_bms_data.tinfo_cnt; i++) {
		token = strsep(&tmp_buf, " ");
		if (!token)
			break;

		if (sscanf(token, "%d", &g_bms_data.tinfo[i].weight) != 1)
			break;
	}

	return size;
}
static ssize_t thermal_weights_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int i;

	for (i = 0; i < g_bms_data.tinfo_cnt; i++) {
		ret += sprintf(buf + ret, "%d ", g_bms_data.tinfo[i].weight);
	}

	return ret;
}
static DEVICE_ATTR_RW(thermal_weights);
#endif

static ssize_t bms_notify_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", g_bms_data.notify_code);
}
static DEVICE_ATTR_RO(bms_notify);

static struct attribute *bms_attrs[] = {
	&dev_attr_bms_notify.attr,
#ifdef BMS_DEBUG
	&dev_attr_max_chg_vol.attr,
	&dev_attr_max_chg_time.attr,
	&dev_attr_max_bat_vol.attr,
	&dev_attr_min_bat_vol.attr,
	&dev_attr_max_bat_temp.attr,
	&dev_attr_min_bat_temp.attr,
	&dev_attr_max_bat_cur.attr,
	&dev_attr_skin_temp_ratio.attr,
	&dev_attr_thermal_weights.attr,
#endif
	NULL
};

static const struct attribute_group bms_group = {
	.attrs = bms_attrs,
};

static const struct attribute_group *bms_groups[] = {
	&bms_group,
	NULL
};

static int bms_parse_dt(struct device_node *np)
{
	int ret = 0;

	ret = of_property_read_string(np, "usb_psy_name", &g_bms_data.usb_psy_name);
	if (ret < 0)
		g_bms_data.usb_psy_name = "usb";

	ret = of_property_read_string(np, "bat_psy_name", &g_bms_data.bat_psy_name);
	if (ret < 0)
		g_bms_data.bat_psy_name = "battery";

	ret = of_property_read_u32(np, "max_chg_vol", &g_bms_data.max_chg_vol);
	if (ret < 0)
		g_bms_data.max_chg_vol = MAX_CHG_VOL;

	ret = of_property_read_u32(np, "max_bat_vol", &g_bms_data.max_bat_vol);
	if (ret < 0)
		g_bms_data.max_bat_vol = MAX_BAT_VOL;

	ret = of_property_read_u32(np, "min_bat_vol", &g_bms_data.min_bat_vol);
	if (ret < 0)
		g_bms_data.min_bat_vol = MIN_BAT_VOL;

	ret = of_property_read_u32(np, "max_bat_temp", &g_bms_data.max_bat_temp);
	if (ret < 0)
		g_bms_data.max_bat_temp = MAX_BAT_TEMP;

	ret = of_property_read_u32(np, "min_bat_temp", &g_bms_data.min_bat_temp);
	if (ret < 0)
		g_bms_data.min_bat_temp = MIN_BAT_TEMP;

	ret = of_property_read_u32(np, "max_bat_cur", &g_bms_data.max_bat_cur);
	if (ret < 0)
		g_bms_data.max_bat_cur = MAX_BAT_CUR;

	ret = of_property_read_u32(np, "max_chg_time", &g_bms_data.max_chg_time);
	if (ret < 0)
		g_bms_data.max_chg_time = MAX_CHG_TIME;

	ret = of_property_read_u32(np, "suspend_current_ua", &g_bms_data.suspend_current_ua);
	if (ret) {
		g_bms_data.suspend_current_ua = 7500;
	}
	g_bms_data.suspend_current_ua = g_bms_data.suspend_current_ua * 2;

	return ret;
}

static int bms_dev_init()
{
	misc_register(&bms_miscdev);

	return 0;
}

static void bms_data_reset(void)
{
	memset(&g_bms_data, 0x0, sizeof(g_bms_data));

	g_bms_data.chg_info.online = 0;
	g_bms_data.chg_info.type = POWER_SUPPLY_TYPE_UNKNOWN;
	g_bms_data.bat_info.resistance = 150000;  //微欧
#ifdef BMS_DEBUG
	g_bms_data.skin_temp_ratio = 100;
#endif
}

static int bms_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;

	pr_info("start.\n");

	bms_data_reset();

	bms_parse_dt(np);

	g_bms_data.usb_psy = power_supply_get_by_name(g_bms_data.usb_psy_name);
	g_bms_data.bat_psy = power_supply_get_by_name(g_bms_data.bat_psy_name);

	if (!g_bms_data.bat_psy) {
		pr_info("get battery power supply failed.\n");
		return -ENODEV;
	}

	wakeup_source_init(&g_bms_data.bms_ws, "bms");

	bms_thermal_init(np, &g_bms_data);

	g_bms_data.psy_nb.notifier_call = psy_notifier_call;
	power_supply_reg_notifier(&g_bms_data.psy_nb);

	g_bms_data.bms_wait_timeout = false;
	init_waitqueue_head(&g_bms_data.bms_wait_q);
	INIT_DELAYED_WORK(&g_bms_data.dwork, bms_dwork);
	schedule_delayed_work(&g_bms_data.dwork, 0);

	g_bms_data.bms_nb.notifier_call = bms_notifier_call;
	bms_notify_register_client(&g_bms_data.bms_nb);

	if (g_class_pax)
		g_bms_data.bms_class = g_class_pax;
	else
		g_bms_data.bms_class = class_create(THIS_MODULE, "pax");

	g_bms_data.dev = device_create_with_groups(g_bms_data.bms_class, &pdev->dev, 0, NULL, bms_groups, "bms");

	bms_dev_init();

	alarm_init(&g_bms_data.bms_alarm, ALARM_BOOTTIME, pax_battery_alarm_func);

	pr_info("success.\n");

	return 0;
}

static void bms_shutdown(struct platform_device *pdev)
{
}

static int bms_suspend(struct platform_device *pdev, pm_message_t state)
{
	unsigned long long secs = 0;
	union power_supply_propval val = {0};
	int rm_cap, full_cap;
	int low_rm_cap;
	int ret;

	if (!g_bms_data.bat_psy)
		return 0;

	ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_CHARGE_FULL, &val);
	if (ret < 0)
		return 0;
	full_cap = val.intval;

	ret = power_supply_get_property(g_bms_data.bat_psy, POWER_SUPPLY_PROP_CHARGE_COUNTER, &val);
	if (ret < 0)
		return 0;
	rm_cap = val.intval;

	low_rm_cap = full_cap * 5 / 100;

	if (rm_cap <= low_rm_cap) {
		secs = 5 * 60;
	}
	else {
		secs = ((rm_cap - low_rm_cap) / g_bms_data.suspend_current_ua * 60 + 5) * 60;
	}

	bms_start_alarm(secs);

	return 0;
}

static int bms_resume(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id bms_match_table[] = {
	{.compatible = "pax,bms",},
	{}
};

static struct platform_driver bms_driver = {
	.probe = bms_probe,
	.shutdown = bms_shutdown,
	.suspend = bms_suspend,
	.resume = bms_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = BMS_NAME,
		.of_match_table = bms_match_table,
	},
};

static int __init bms_init(void)
{
	return platform_driver_register(&bms_driver);
}

late_initcall_sync(bms_init);
