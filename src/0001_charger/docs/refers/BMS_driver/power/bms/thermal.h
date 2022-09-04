#ifndef __BMS_THERMAL_H__
#define __BMS_THERMAL_H__

#define INVALID_TEMP  -127000
#define TEMP_STABLE_CNT 5

extern void bms_thermal_init(struct device_node *np, struct bms_data *data);
extern int bms_thermal_check(struct bms_data *data);

#endif
