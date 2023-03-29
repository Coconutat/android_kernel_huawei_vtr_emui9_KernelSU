#ifndef _HW_VOLTAGE_DEV_H_
#define _HW_VOLTAGE_DEV_H_

#include <linux/list.h>
#include <linux/device.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/slab.h>

#define HW_VOLTAGE_ERR_COMMUNICATIN		-1
#define HW_VOLTAGE_HARDWARE_NULL		0
#define HW_VOLTAGE_OK_NO_THRESHOLD		1
#define HW_VOLTAGE_OK_WITH_THRESHOLD		2
#define HW_VOLTAFE_ERR_OUTOF_LOW_RANG		3
#define HW_VOLTAGE_ERR_OUTOF_HIGHT_RANGE	4

struct hw_voltage_info {
	int id;
	int channel;
	int voltage;
	int min_voltage;
	int max_voltage;
};

struct hw_voltage_data {
	int id;
	int state;
	void *data;
	struct list_head list;
	int (*hw_voltage_enable)(void *, int);
	int (*hw_voltage_getstate)(void *, int *);
	int (*hw_voltage_chennel)(void *, struct hw_voltage_info*);
	int (*hw_voltage_getvoltage)(void *, struct hw_voltage_info*);
	int (*hw_voltage_switch_enable)(void *, int);
};

struct hw_voltage_device {
	int voltage_states;
	struct device *dev;
	struct class *vol_class;
	struct hw_voltage_info info;
	struct list_head head;
	spinlock_t spinlock;
};

struct hw_voltage_data *hw_voltage_register(void *data, int id);
void hw_voltage_unregister(int id);
#endif
