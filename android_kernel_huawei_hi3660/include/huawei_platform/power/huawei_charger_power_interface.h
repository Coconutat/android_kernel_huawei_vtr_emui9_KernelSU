#ifndef __POWER_INTERFACE_H_
#define __POWER_INTERFACE_H_

#define POWER_IF_RD_BUF_SIZE (64)
#define POWER_IF_WR_BUF_SIZE (256)

enum power_if_sysfs_type {
	POWER_IF_SYSFS_BEGIN = 0,

	POWER_IF_SYSFS_ENABLE_CHARGER = POWER_IF_SYSFS_BEGIN, /* enable charger */

	POWER_IF_SYSFS_END,

};

enum power_if_op_user {
	POWER_IF_OP_USER_BEGIN = 0,

	POWER_IF_OP_USER_DEFAULT = POWER_IF_OP_USER_BEGIN, /* for default */
	POWER_IF_OP_USER_RC, /* for rc file */
	POWER_IF_OP_USER_HIDL, /* for hidl interface */
	POWER_IF_OP_USER_HEALTHD, /* for healthd daemon */
	POWER_IF_OP_USER_LIMIT_CURRENT, /* for limit_current native */
	POWER_IF_OP_USER_CHARGE_MONITOR, /* for charge_monitor native */
	POWER_IF_OP_USER_ATCMD, /* for atcmd native */
	POWER_IF_OP_USER_THERMAL, /* for thermal daemon */
	POWER_IF_OP_USER_AI, /* for ai */
	POWER_IF_OP_USER_RUNNING, /* for running apk */
	POWER_IF_OP_USER_FWK, /* for framework */
	POWER_IF_OP_USER_APP, /* for app */
	POWER_IF_OP_USER_SHELL, /* for shell command */
	POWER_IF_OP_USER_KERNEL, /* for kernel space */

	POWER_IF_OP_USER_END,
};

enum power_if_op_type {
	POWER_IF_OP_TYPE_BEGIN = 0,

	POWER_IF_OP_TYPE_DCP = POWER_IF_OP_TYPE_BEGIN, /* standard */
	POWER_IF_OP_TYPE_DCP_SH, /* standard for sensorhub */
	POWER_IF_OP_TYPE_OTG, /* otg */
	POWER_IF_OP_TYPE_FCP, /* fcp */
	POWER_IF_OP_TYPE_FCP_AUX, /* fcp auxiliary */
	POWER_IF_OP_TYPE_PD, /* pd */
	POWER_IF_OP_TYPE_PD_AUX, /*pd auxiliary */
	POWER_IF_OP_TYPE_LVC, /* lvc */
	POWER_IF_OP_TYPE_LVC_AUX, /* lvc auxiliary */
	POWER_IF_OP_TYPE_LVC_SH, /* lvc  for sensorhub */
	POWER_IF_OP_TYPE_LVC_AUX_SH, /* lvc auxiliary for sensorhub */
	POWER_IF_OP_TYPE_SC, /* sc */
	POWER_IF_OP_TYPE_SC_AUX, /* sc auxiliary */
	POWER_IF_OP_TYPE_SC_SH, /* sc  for sensorhub */
	POWER_IF_OP_TYPE_SC_AUX_SH, /* sc auxiliary  for sensorhub */
	POWER_IF_OP_TYPE_WL, /* wireless */
	POWER_IF_OP_TYPE_WL_LVC, /* wireless lvc */
	POWER_IF_OP_TYPE_WL_SC, /* wireless sc */
	POWER_IF_OP_TYPE_WL_REVERSE, /* wireless reverse */

	POWER_IF_OP_TYPE_ALL, /* for all type */
	POWER_IF_OP_TYPE_END,
};

enum power_if_error_code {
	POWER_IF_ERRCODE_INVAID_OP = -22,
	POWER_IF_ERRCODE_FAIL = -1,
	POWER_IF_ERRCODE_PASS = 0,
};

/* power interface oprator */
struct power_if_ops {
	int (*set_enable_charger)(unsigned int value); /* set enable charger interface */
	int (*get_enable_charger)(unsigned int *value); /* get enable charger interface */
	const char *type_name;
};

/* power interface info */
struct power_if_device_info {
	struct device *dev;

	unsigned int total_ops;
	struct power_if_ops *ops[POWER_IF_OP_TYPE_END];
};

int power_if_ops_register(struct power_if_ops *ops);
int power_if_kernel_sysfs_get(unsigned int type, unsigned int sysfs_type, unsigned int *value);
int power_if_kernel_sysfs_set(unsigned int type, unsigned int sysfs_type, unsigned int value);

#endif /* end of __POWER_INTERFACE_H_ */