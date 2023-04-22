#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>

#include <linux/power/hisi/hisi_bci_battery.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/huawei_charger_power_interface.h>

#define HWLOG_TAG power_if
HWLOG_REGIST();


struct power_if_device_info *g_power_if_info = NULL;

static DEFINE_MUTEX(g_power_if_sysfs_get_mutex);
static DEFINE_MUTEX(g_power_if_sysfs_set_mutex);


static const char *power_if_op_user_table[] = {
	[POWER_IF_OP_USER_DEFAULT] = "default",
	[POWER_IF_OP_USER_RC] = "rc",
	[POWER_IF_OP_USER_HIDL] = "hidl",
	[POWER_IF_OP_USER_HEALTHD] = "healthd",
	[POWER_IF_OP_USER_LIMIT_CURRENT] = "limit_current",
	[POWER_IF_OP_USER_CHARGE_MONITOR] = "charge_monitor",
	[POWER_IF_OP_USER_ATCMD] = "atcmd",
	[POWER_IF_OP_USER_THERMAL] = "thermal",
	[POWER_IF_OP_USER_AI] = "ai",
	[POWER_IF_OP_USER_RUNNING] = "running",
	[POWER_IF_OP_USER_FWK] = "fwk",
	[POWER_IF_OP_USER_APP] = "app",
	[POWER_IF_OP_USER_SHELL] = "shell",
	[POWER_IF_OP_USER_KERNEL] = "kernel",
};

static const char *power_if_op_type_table[] = {
	[POWER_IF_OP_TYPE_DCP] = "dcp",
	[POWER_IF_OP_TYPE_DCP_SH] = "dcp_sh",
	[POWER_IF_OP_TYPE_OTG] = "otg",
	[POWER_IF_OP_TYPE_FCP] = "fcp",
	[POWER_IF_OP_TYPE_FCP_AUX] = "fcp_aux",
	[POWER_IF_OP_TYPE_PD] = "pd",
	[POWER_IF_OP_TYPE_PD_AUX] = "pd_aux",
	[POWER_IF_OP_TYPE_LVC] = "lvc",
	[POWER_IF_OP_TYPE_LVC_AUX] = "lvc_aux",
	[POWER_IF_OP_TYPE_LVC_SH] = "lvc_sh",
	[POWER_IF_OP_TYPE_LVC_AUX_SH] = "lvc_aux_sh",
	[POWER_IF_OP_TYPE_SC] = "sc",
	[POWER_IF_OP_TYPE_SC_AUX] = "sc_aux",
	[POWER_IF_OP_TYPE_SC_SH] = "sc_sh",
	[POWER_IF_OP_TYPE_SC_AUX_SH] = "sc_aux_sh",
	[POWER_IF_OP_TYPE_WL] = "wl",
	[POWER_IF_OP_TYPE_WL_LVC] = "wl_lvc",
	[POWER_IF_OP_TYPE_WL_SC] = "wl_sc",
	[POWER_IF_OP_TYPE_WL_REVERSE] = "wl_reverse",
	[POWER_IF_OP_TYPE_ALL] = "all",
};

static const char *power_if_sysfs_type_table[] = {
	[POWER_IF_SYSFS_ENABLE_CHARGER] = "enable_charger",
};


static const char *power_if_get_op_user_name(unsigned int index)
{
	if((index >= POWER_IF_OP_USER_BEGIN) && (index < POWER_IF_OP_USER_END)) {
		return power_if_op_user_table[index];
	}

	return "illegal user";
}

static const char *power_if_get_op_type_name(unsigned int index)
{
	if((index >= POWER_IF_OP_TYPE_BEGIN) || (index < POWER_IF_OP_TYPE_END)) {
		return power_if_op_type_table[index];
	}

	return "illegal type";
}

static const char *power_if_get_sysfs_type_name(unsigned int index)
{
	if((index >= POWER_IF_SYSFS_BEGIN) || (index < POWER_IF_SYSFS_END)) {
		return power_if_sysfs_type_table[index];
	}

	return "illegal sysfs_type";
}

static int power_if_get_op_user(const char *str)
{
	unsigned int i = 0;

	for (i = 0; i < ARRAY_SIZE(power_if_op_user_table); i++) {
		if (!strncmp(str, power_if_op_user_table[i], strlen(str))) {
			return i;
		}
	}

	return -1;
}

static int power_if_get_op_type(const char *str)
{
	unsigned int i = 0;

	for (i = 0; i < ARRAY_SIZE(power_if_op_type_table); i++) {
		if (!strncmp(str, power_if_op_type_table[i], strlen(str))) {
			return i;
		}
	}

	return -1;
}

static int power_if_check_op_user(unsigned int index)
{
	if ((index >= POWER_IF_OP_USER_BEGIN) && (index < POWER_IF_OP_USER_END)) {
		return index;
	}

	return -1;
}

static int power_if_check_op_type(unsigned int index)
{
	if ((index >= POWER_IF_OP_TYPE_BEGIN) || (index < POWER_IF_OP_TYPE_END)) {
		return index;
	}

	return -1;
}

static int power_if_check_sysfs_type(unsigned int index)
{
	if ((index >= POWER_IF_SYSFS_BEGIN) || (index < POWER_IF_SYSFS_END)) {
		return index;
	}

	return -1;
}

static int power_if_check_paras(unsigned int user, unsigned int type, unsigned int sysfs_type)
{
	if (power_if_check_op_user(user) == -1) {
		hwlog_err("error: invalid user(%d)!\n", user);
		return -1;
	}

	if (power_if_check_op_type(type) == -1) {
		hwlog_err("error: invalid type(%d)!\n", type);
		return -1;
	}

	if (power_if_check_sysfs_type(sysfs_type) == -1) {
		hwlog_err("error: invalid sysfs_type(%d)!\n", sysfs_type);
		return -1;
	}

	return 0;
}

static int power_if_operator_get(unsigned int type, unsigned int sysfs_type, unsigned int *value)
{
	int ret = POWER_IF_ERRCODE_INVAID_OP;

	if (g_power_if_info->ops[type] == NULL) {
		return ret;
	}

	switch (sysfs_type) {
		case POWER_IF_SYSFS_ENABLE_CHARGER:
			if (g_power_if_info->ops[type]->get_enable_charger != NULL) {
				ret = g_power_if_info->ops[type]->get_enable_charger(value);

				hwlog_info("get enable charger = %d\n", *value);
			}
		break;

		default:
		break;
	}

	return ret;
}

static int power_if_operator_set(unsigned int type, unsigned int sysfs_type, unsigned int value)
{
	int ret = 0;

	if (g_power_if_info->ops[type] == NULL) {
		return ret;
	}

	switch (sysfs_type) {
		case POWER_IF_SYSFS_ENABLE_CHARGER:
			if (g_power_if_info->ops[type]->set_enable_charger != NULL) {
				ret = g_power_if_info->ops[type]->set_enable_charger(value);

				hwlog_info("set enable charger = %d\n", value);
			}
		break;

		default:
		break;
	}

	return ret;
}

static int power_if_common_sysfs_get(unsigned int user, unsigned int type,
	unsigned int sysfs_type, char *buf)
{
	int ret = 0;
	unsigned int value = 0;
	unsigned int type_s = 0;
	unsigned int type_e = 0;
	char rd_buf[POWER_IF_RD_BUF_SIZE] = {0};

	if (power_if_check_paras(user, type, sysfs_type) == -1) {
		return -1;
	}

	hwlog_info("sysfs_get(%s): user(%s) type(%s)\n", \
		power_if_get_sysfs_type_name(sysfs_type), power_if_get_op_user_name(user), \
		power_if_get_op_type_name(type));

	mutex_lock(&g_power_if_sysfs_get_mutex);

	if (type == POWER_IF_OP_TYPE_ALL) {
		/* loop all type */
		type_s = POWER_IF_OP_TYPE_BEGIN;
		type_e = type;
	}
	else {
		/* specified one type */
		type_s = type;
		type_e = type + 1;
	}

	/* output all value */
	for (; type_s < type_e; type_s++) {
		value = 0;
		ret = power_if_operator_get(type_s, sysfs_type, &value);

		/* if op is invalid, must be skip output */
		if (ret == POWER_IF_ERRCODE_INVAID_OP) {
			continue;
		}

		memset(rd_buf, 0, POWER_IF_RD_BUF_SIZE);
		if (ret == 0) {
			scnprintf(rd_buf, POWER_IF_RD_BUF_SIZE, "%s: %d\n", power_if_get_op_type_name(type_s), value);
		}
		else {
			scnprintf(rd_buf, POWER_IF_RD_BUF_SIZE, "%s: invalid\n", power_if_get_op_type_name(type_s));
		}
		strncat(buf, rd_buf, strlen(rd_buf));
	}

	mutex_unlock(&g_power_if_sysfs_get_mutex);

	return strlen(buf);
}

static int power_if_common_sysfs_set(unsigned int user, unsigned int type,
	unsigned int sysfs_type, unsigned int value)
{
	int ret = 0;
	unsigned int type_s = 0;
	unsigned int type_e = 0;

	if (power_if_check_paras(user, type, sysfs_type) == -1) {
		return -1;
	}

	hwlog_info("sysfs_set(%s): user(%s) type(%s) value(%d)\n", \
		power_if_get_sysfs_type_name(sysfs_type), power_if_get_op_user_name(user), \
		power_if_get_op_type_name(type), value);

	mutex_lock(&g_power_if_sysfs_set_mutex);

	if (type == POWER_IF_OP_TYPE_ALL) {
		/* loop all type */
		type_s = POWER_IF_OP_TYPE_BEGIN;
		type_e = type;
	}
	else {
		/* specified one type */
		type_s = type;
		type_e = type + 1;
	}

	for (; type_s < type_e; type_s++) {
		ret |= power_if_operator_set(type_s, sysfs_type, value);
	}

	mutex_unlock(&g_power_if_sysfs_set_mutex);

	return ret;
}

int power_if_kernel_sysfs_get(unsigned int type, unsigned int sysfs_type, unsigned int *value)
{
	hwlog_info("sysfs_get(%s): user(%s) type(%s)\n", \
		power_if_get_sysfs_type_name(sysfs_type), power_if_get_op_user_name(POWER_IF_OP_USER_KERNEL), \
		power_if_get_op_type_name(type));

	return power_if_operator_get(type, sysfs_type, value);
}
EXPORT_SYMBOL_GPL(power_if_kernel_sysfs_get);

int power_if_kernel_sysfs_set(unsigned int type, unsigned int sysfs_type, unsigned int value)
{
	return power_if_common_sysfs_set(POWER_IF_OP_USER_KERNEL, type, sysfs_type, value);
}
EXPORT_SYMBOL_GPL(power_if_kernel_sysfs_set);


#if defined(CONFIG_SYSFS)
#define POWER_IF_SYSFS_FIELD(_name, n, m, store) \
{ \
	.attr = __ATTR(_name, m, power_if_sysfs_show, store), \
	.name = POWER_IF_SYSFS_##n, \
}

#define POWER_IF_SYSFS_FIELD_RW(_name, n) \
	POWER_IF_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, power_if_sysfs_store)

#define POWER_IF_SYSFS_FIELD_RO(_name, n) \
	POWER_IF_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

struct power_if_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static ssize_t power_if_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t power_if_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static struct power_if_sysfs_field_info power_if_sysfs_field_tbl[] = {
	POWER_IF_SYSFS_FIELD_RW(enable_charger, ENABLE_CHARGER),
};

static struct attribute *power_if_sysfs_attrs[ARRAY_SIZE(power_if_sysfs_field_tbl) + 1];

static const struct attribute_group power_if_sysfs_attr_group = {
	.attrs = power_if_sysfs_attrs,
};

static void power_if_sysfs_init_attrs(void)
{
	int s, e = ARRAY_SIZE(power_if_sysfs_field_tbl);

	for (s = 0; s < e; s++) {
		power_if_sysfs_attrs[s] = &power_if_sysfs_field_tbl[s].attr.attr;
	}
	power_if_sysfs_attrs[e] = NULL;
}

static struct power_if_sysfs_field_info *power_if_sysfs_field_lookup(const char *name)
{
	int s, e = ARRAY_SIZE(power_if_sysfs_field_tbl);

	for (s = 0; s < e; s++) {
		if (!strncmp(name, power_if_sysfs_field_tbl[s].attr.attr.name, strlen(name)))
			break;
	}
	if (s >= e)
		return NULL;

	return &power_if_sysfs_field_tbl[s];
}

static ssize_t power_if_sysfs_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct power_if_sysfs_field_info *sysfs_info = NULL;

	sysfs_info = power_if_sysfs_field_lookup(attr->attr.name);
	if (!sysfs_info) {
		hwlog_err("error: get sysfs entries failed!\n");
		return -EINVAL;
	}

	return power_if_common_sysfs_get(POWER_IF_OP_USER_DEFAULT, POWER_IF_OP_TYPE_ALL, sysfs_info->name, buf);
}

static ssize_t power_if_sysfs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct power_if_sysfs_field_info *sysfs_info = NULL;

	char user_name[POWER_IF_RD_BUF_SIZE] = {0};
	char type_name[POWER_IF_RD_BUF_SIZE] = {0};
	unsigned int user = 0;
	unsigned int type = 0;
	unsigned int value = 0;

	sysfs_info = power_if_sysfs_field_lookup(attr->attr.name);
	if (!sysfs_info) {
		hwlog_err("error: get sysfs entries failed!\n");
		return -EINVAL;
	}

	if (count >= (POWER_IF_RD_BUF_SIZE - 2)) {
		hwlog_err("error: input too long!\n");
		return -EINVAL;
	}

	if (sscanf(buf, "%s %s %d", user_name, type_name, &value) != 3) {
		hwlog_err("error: unable to parse input:%s!\n", buf);
		return -EINVAL;
	}

	if ((user = power_if_get_op_user(user_name)) == -1) {
		hwlog_err("error: invalid user(%s)!\n", user_name);
		return -EINVAL;
	}

	if ((type = power_if_get_op_type(type_name)) == -1) {
		hwlog_err("error: invalid type(%s)!\n", type_name);
		return -EINVAL;
	}

	power_if_common_sysfs_set(user, type, sysfs_info->name, value);

	return count;
}
#endif

int power_if_ops_register(struct power_if_ops *ops)
{
	int type = 0;

	if (g_power_if_info == NULL || ops == NULL || ops->type_name == NULL) {
		hwlog_err("error: g_power_if_info or ops is null!\n");
		return -EPERM;
	}

	type = power_if_get_op_type(ops->type_name);

	if (type == -1) {
		hwlog_err("error: %s ops register fail!\n", ops->type_name);
		return -EPERM;
	}

	g_power_if_info->ops[type] = ops;
	g_power_if_info->total_ops++;

	hwlog_info("total_ops(%d) type(%d:%s) ops register ok\n", g_power_if_info->total_ops, type, ops->type_name);

	return 0;
}

static int __init power_interface_init(void)
{
	int ret = 0;
	struct power_if_device_info *info = NULL;
	struct class *power_class = NULL;

	hwlog_info("probe begin\n");

	info = kzalloc(sizeof(struct power_if_device_info), GFP_KERNEL);
	if (NULL == info) {
		hwlog_err("error: kzalloc failed!\n");
		return -ENOMEM;
	}
	g_power_if_info = info;

#if defined(CONFIG_SYSFS)
	power_if_sysfs_init_attrs();

	power_class = hw_power_get_class();
	if (power_class) {
		g_power_if_info->dev = device_create(power_class, NULL, 0, NULL, "interface");
		ret = sysfs_create_group(&g_power_if_info->dev->kobj, &power_if_sysfs_attr_group);
		if (ret) {
			hwlog_err("error: sysfs group create failed!\n");
			ret = -ENOMEM;
			goto power_if_fail_0;
		}
	}
#endif

	hwlog_info("probe end\n");
	return 0;

#if defined(CONFIG_SYSFS)
power_if_fail_0:
	kfree(info);
	g_power_if_info = NULL;

	return ret;
#endif
}

static void __exit power_interface_exit(void)
{
	hwlog_info("remove begin\n");

#if defined(CONFIG_SYSFS)
	sysfs_remove_group(&g_power_if_info->dev->kobj, &power_if_sysfs_attr_group);
	kobject_del(&g_power_if_info->dev->kobj);
	kobject_put(&g_power_if_info->dev->kobj);
#endif

	kfree(g_power_if_info);
	g_power_if_info = NULL;

	hwlog_info("remove ok\n");
}

fs_initcall_sync(power_interface_init);
module_exit(power_interface_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("power interface module driver");
MODULE_AUTHOR("HUAWEI Inc");
