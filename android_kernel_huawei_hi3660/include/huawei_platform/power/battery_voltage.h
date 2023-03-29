#ifndef _HW_BATT_VOL_H_
#define _HW_BATT_VOL_H_

#ifdef CONFIG_HISI_COUL
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#endif
#ifdef CONFIG_HISI_BCI_BATTERY
#include <linux/power/hisi/hisi_bci_battery.h>
#endif

#define HW_BATT_VOL_NUM                  (10)
#define HW_BATT_VOL_STR_MAX_LEM          (20)
#define HW_BATT_VOL_SINGLE_BATTERY       (1)
#define MAX_VOL_MV                       (100000)
#define HW_BATT_HISI_COUL                "hisi_coul"

enum hw_batt_vol_sysfs_type {
	HW_BATT_VOL_SYSFS_BAT_ID_0,
	HW_BATT_VOL_SYSFS_BAT_ID_1,
	HW_BATT_VOL_SYSFS_BAT_ID_ALL,
	HW_BATT_VOL_SYSFS_BAT_ID_MAX,
	HW_BATT_VOL_SYSFS_BAT_ID_MIN,
};

/* enum number must be fixed it is matched with DTS, new member must be added last line. */
enum hw_batt_id {
	BAT_ID_0 = 0,
	BAT_ID_1 = 1,
	BAT_ID_ALL = 2,
	BAT_ID_MAX = 3,
	BAT_ID_MIN = 4,
};

struct hw_batt_get_vol {
	int batt_id;
	char ops_name[HW_BATT_VOL_STR_MAX_LEM];
	int (*get_batt_vol)(void);
};

struct hw_batt_vol_info {
	struct platform_device *pdev;
	struct device *dev;
	struct hw_batt_get_vol vol_buff[HW_BATT_VOL_NUM];
	int total_vol;
};

struct hw_batt_vol_ops {
	int (*get_batt_vol)(void);
};

#ifdef CONFIG_HUAWEI_BATTERY_VOLTAGE
extern int hw_battery_voltage(enum hw_batt_id batt_id);

int hw_battery_voltage_ops_register(struct hw_batt_vol_ops* ops, char* ops_name);

#else

static inline int hw_battery_voltage(enum hw_batt_id batt_id)
{
	return hisi_battery_voltage();
}

static inline int hw_battery_voltage_ops_register(struct hw_batt_vol_ops* ops, char* ops_name)
{
	return 0;
}
#endif /* end of CONFIG_HUAWEI_BATTERY_VOLTAGE */

#endif /* end of _HW_BATT_VOL_H_ */
