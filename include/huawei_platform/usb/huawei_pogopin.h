#ifndef HUAWEI_POGOPIN_H
#define HUAWEI_POGOPIN_H
struct pogopin_device_info {
	struct platform_device *pdev;
	struct pogopin_device_ops *ops;
	int usb_switch_gpio;
	int power_switch_gpio;
	int switch_power_gpio;
	int buck_boost_gpio;
	int pogopin_int_gpio;
	int pogopin_int_irq;
	int typec_int_gpio;
	struct work_struct work;
	int current_int_status;
};

struct cc_detect_ops{
	int (*typec_detect_disable)(bool);
};

struct pogopin_device_ops {
	struct pogopin_device_info *di;
	void (*enable_pogopin_charge)(void);
	void (*enable_pogopin_otg)(void);
	void (*typec_charge_otg)(void);
	int (*get_connect_status)(void);
	int (*get_interface_status)(void);
};


enum pogopin_sysfs_type {
	POGOPIN_SYSFS_INTERFACE_TYPE = 0,
};

enum current_working_interface {
	NO_INTERFACE = 0,
	TYPEC_INTERFACE,
	POGOPIN_INTERFACE,
	POGOPIN_AND_TYPEC,
};

void cc_detect_register_ops(struct cc_detect_ops *ops);
int is_pogopin_support(void);

#define hw_pogopin_dbg(format, arg...)    \
	do {                 \
		printk(KERN_INFO "[POGOPIN_DEBUG][%s]"format, __func__, ##arg); \
	} while (0)
#define hw_pogopin_err(format, arg...)    \
	do {                 \
		printk(KERN_ERR "[POGOPIN_DEBUG]"format, ##arg); \
	} while (0)

#endif
