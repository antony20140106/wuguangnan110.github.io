#include <linux/slab.h>
#include <linux/of.h>
#include <linux/thermal.h>

#include "bms.h"
#include "thermal.h"

static void bms_thermal_dwork(struct work_struct *work)
{
	struct bms_data *data = container_of(work, struct bms_data, thermal_dwork.work);

	data->bms_thermal_support = true;
}

void bms_thermal_init(struct device_node *np, struct bms_data *data)
{
	int count = 0;
	int i;
	int ret = 0;

	data->bms_thermal_support = false;

	count = of_property_count_strings(np, "thermal_names");
	if (count <= 0) {
		pr_info("not found thermals");
		return;
	}

	data->tinfo = kzalloc(sizeof(*(data->tinfo)) * count, GFP_KERNEL);
	if (IS_ERR_OR_NULL(data->tinfo)) {
		pr_err("alloc memery for bms tinfo failed.\n");
		return;
	}
	data->tinfo_cnt = count;

	for (i = 0; i < count; i++) {
		ret = of_property_read_string_index(np, "thermal_names", i, &data->tinfo[i].name);
		if (ret < 0) {
			data->tinfo[i].name = NULL;
			pr_err("get thermal name(%d) failed.\n", i);
			continue;
		}

		ret = of_property_read_u32_index(np, "thermal_weights", i, &data->tinfo[i].weight);
		if (ret < 0) {
			pr_err("get thermal weight(%d) failed.\n", i);
			data->tinfo[i].weight = 0;
			continue;
		}
	}

	count = of_property_count_u32_elems(np, "chg_skin_temps");
	if (count <= 0) {
		pr_info("not found chg skin temps\n");
		return;
	}

	data->tchg_info = kzalloc(sizeof(*(data->tchg_info)) * count, GFP_KERNEL);
	if (IS_ERR_OR_NULL(data->tchg_info)) {
		pr_err("alloc memery for bms tchg info failed.\n");
		return;
	}
	data->tchg_info_cnt = count;

	for (i = 0; i < count; i++) {
		of_property_read_u32_index(np, "chg_skin_temps", i, &data->tchg_info[i].temp);
		of_property_read_u32_index(np, "chg_currents", i, &data->tchg_info[i].max_chg_current);
	}

	INIT_DELAYED_WORK(&data->thermal_dwork, bms_thermal_dwork);
	schedule_delayed_work(&data->thermal_dwork, msecs_to_jiffies(10 * 1000));

}

static int get_skin_temp(struct bms_data *data)
{
	int i = 0;
	int skin_temp = 0;
	int temp= 0;
	int ret = 0;
	int total_weight = 0;

	for (i = 0; i < data->tinfo_cnt; i++) {
		if (IS_ERR_OR_NULL(data->tinfo[i].name) || (data->tinfo[i].weight <= 0))
			continue;

		data->tinfo[i].tz = thermal_zone_get_zone_by_name(data->tinfo[i].name);
		if (IS_ERR_OR_NULL(data->tinfo[i].tz))
			continue;

		ret = thermal_zone_get_temp(data->tinfo[i].tz, &temp);
		if (ret) {
			continue;
		}

		pr_debug("%s: %p: %s: %d: %d\n", __func__, data->tinfo[i].tz, data->tinfo[i].name, data->tinfo[i].weight, temp);
		if (temp == INVALID_TEMP)
			continue;

		skin_temp += temp * data->tinfo[i].weight;
		total_weight += data->tinfo[i].weight;
	}

	if (total_weight > 0)
		skin_temp = skin_temp / total_weight;
	else
		skin_temp = 25000;

#ifdef BMS_DEBUG
	skin_temp = skin_temp * data->skin_temp_ratio / 100;
#endif

	data->skin_temp = skin_temp / 100;
	pr_debug("%s: skin_temp: %d\n", __func__, skin_temp);

	return skin_temp;
}

static int get_temp_range_index(struct bms_data *data, int temp)
{
	int i = 0;

	for (i = 0; i < data->tchg_info_cnt; i++) {
		if (temp < data->tchg_info[i].temp)
			return i;
	}

	return data->tchg_info_cnt - 1;
}

int bms_thermal_check(struct bms_data *data)
{
	int skin_temp = 25000;
	int cur_index = 0;
	static int last_index = -1;    //should reset to -1, when charge start
	static int stable_cnt = 0;
	int update_flag = 0;
	int delta_temp = 1000;

	if (!data->bms_thermal_support)
		return 0;

	if (data->bat_info.status != POWER_SUPPLY_STATUS_CHARGING) {
		last_index = -1;
		stable_cnt = 0;
		return 0;
	}

	skin_temp = get_skin_temp(data);
	cur_index = get_temp_range_index(data, skin_temp);
	pr_debug("%s: origin last_index: %d, cur_index: %d\n", __func__, last_index, cur_index);

	if (last_index == -1) {
		last_index = cur_index;
		update_flag = 1;
	}
	else if ((last_index != cur_index)) {
		if (last_index < cur_index)
			delta_temp = 0 - delta_temp;

		cur_index = get_temp_range_index(data, skin_temp + delta_temp);
		pr_debug("%s: actrual last_index: %d, cur_index: %d\n", __func__, last_index, cur_index);
		if (last_index == cur_index)
			stable_cnt = 0;

		stable_cnt++;
		if (stable_cnt >= TEMP_STABLE_CNT) {
			stable_cnt = 0;
			if (last_index < cur_index) {
				last_index ++;
				update_flag = 1;
			}
			else if (last_index > cur_index) {
				last_index --;
				update_flag = 1;
			}
		}
	}
	else {
		stable_cnt = 0;
	}

	if (update_flag) {
		if (data->usb_psy) {
			union power_supply_propval val = {0};
			val.intval = data->tchg_info[last_index].max_chg_current;
			power_supply_set_property(data->usb_psy, POWER_SUPPLY_PROP_CURRENT_MAX, &val);
		}
	}

	return 0;
}



