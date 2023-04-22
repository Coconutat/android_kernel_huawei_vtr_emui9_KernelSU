#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/wireless_direct_charger.h>
#include <huawei_platform/power/direct_charger.h>
#include <huawei_platform/power/wired_channel_switch.h>
#ifdef CONFIG_BOOST_5V
#include <huawei_platform/power/boost_5v.h>
#endif
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif

#define HWLOG_TAG wireless_sc
HWLOG_REGIST();

static struct wldc_device_info *g_wireless_sc_di = NULL;
static struct loadswitch_ops* g_wireless_ls_ops = NULL;
static struct batinfo_ops* g_wireless_bi_ops = NULL;


static int wireless_sc_set_enable_charger(unsigned int val)
{
	struct wldc_device_info *di = g_wireless_sc_di;

	if (NULL == di) {
		hwlog_err("%s di is NULL\n", __func__);
		return -1;
	}

	if ((val < 0) || (val > 1)) {
		return -1;
	}

	di->sysfs_data.enable_charger = val;
	hwlog_info("wl_sc: set enable_charger = %ld\n", di->sysfs_data.enable_charger);

	return 0;
}

static int wireless_sc_get_enable_charger(unsigned int *val)
{
	struct wldc_device_info *di = g_wireless_sc_di;

	if (NULL == di) {
		hwlog_err("%s di is NULL\n", __func__);
		return -1;
	}

	*val = di->sysfs_data.enable_charger;

	return 0;
}

/* define public power interface */
static struct power_if_ops wireless_sc_power_if_ops = {
	.set_enable_charger = wireless_sc_set_enable_charger,
	.get_enable_charger = wireless_sc_get_enable_charger,
	.type_name = "wl_sc",
};


int wireless_sc_ops_register(struct loadswitch_ops* ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_wireless_ls_ops = ops;
	} else {
		hwlog_err("sc ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}
int wireless_sc_batinfo_ops_register(struct batinfo_ops* ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_wireless_bi_ops = ops;
	} else {
		hwlog_err("batinfo ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}
int wireless_sc_get_di(struct wldc_device_info **di)
{
	if (!g_wireless_sc_di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}
	*di = g_wireless_sc_di;
	return 0;
}
int wireless_sc_charge_check(void)
{
	int ret;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	struct wldc_device_info *di = g_wireless_sc_di;
	if (!di) {
		hwlog_err("%s: di is null!\n", __func__);
		return -1;
	}

	ret = wldc_check_ops_valid(di);
	if (ret) {
		hwlog_err("%s: di ops null\n", __func__);
		return -1;
	}
	if (!di->sysfs_data.enable_charger) {
		hwlog_err("[%s] wireless_sc is disabled\n",__func__);
		return -1;
	}
	if ((WLDC_STAGE_CHARGE_DONE == di->wldc_stage)) {
		hwlog_debug("[%s] wireless_sc charge done\n", __func__);
		return -1;
	}
	if (WIRED_CHANNEL_ON == wireless_charge_get_wired_channel_state()) {
		hwlog_err("%s: wired vbus connect, stop check\n", __func__);
		return -1;
	}
	if (di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
		hwlog_err("%s: wireless tx disconnect, stop check\n", __func__);
		return -1;
	}
	if (di->error_cnt >= WLDC_ERR_CNT_MAX) {
		hwlog_debug("%s: error exceed %d times, wireless_sc is disabled\n", __func__, WLDC_ERR_CNT_MAX);
		if (FALSE == di->wldc_err_report_flag && di->dc_open_retry_cnt <= WLDC_OPEN_RETRY_CNT_MAX) {
			hwlog_err("%s\n", di->wldc_err_dsm_buff);
			power_dsm_dmd_report(POWER_DSM_BATTERY, ERROR_WIRELESS_ERROR, di->wldc_err_dsm_buff);
			memset(di->wldc_err_dsm_buff, 0, sizeof(di->wldc_err_dsm_buff));
			di->wldc_err_report_flag = TRUE;
		}
		return -1;
	}
	if (di->warning_cnt >= WLDC_WARNING_CNT_MAX) {
		hwlog_debug("%s: warning exceed %d times, wireless_sc is disabled\n",
			__func__, WLDC_WARNING_CNT_MAX);
		return -1;
	}
	wldc_set_charge_stage(WLDC_STAGE_CHECK);
	ret = can_vbatt_do_wldc_charge(di);
	if (ret) {
		hwlog_debug("%s: vbatt out of range, try next loop!\n", __func__);
		return -1;
	}
	ret = can_tbatt_do_wldc_charge();
	if (ret) {
		hwlog_debug("%s: batt temp out of range, try next loop!\n", __func__);
		return -1;
	}
#ifdef CONFIG_BOOST_5V
	boost_5v_enable(BOOST_5V_ENABLE, BOOST_CTRL_WLDC);
#endif
	if (!di->wldc_stop_charging_complete_flag) {
		hwlog_err("%s: in wireless sc, ingnore wireless_sc check", __func__);
		return -1;
	}
	di->wldc_stop_charging_complete_flag = false;

	ret = wldc_chip_init(di);
	if (ret) {
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: sc_chip_init fail, try next loop!\n", __func__);
		wldc_stop_charging(di);
		return -1;
	}
	ret = wldc_set_rx_init_vout(di);
	if (ret){
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: set initial vout fail, try next loop!\n", __func__);
		wldc_stop_charging(di);
		return -1;
	}
	wireless_charge_chip_init(WILREESS_SC_CHIP_INIT);
	ret = wldc_cutt_off_normal_charge_path();
	if (ret) {
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: cutt_off_normal_charge fail, try next loop!\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "cutt_off_normal_charge fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		wldc_stop_charging(di);
		return -1;
	}
#ifdef CONFIG_TCPC_CLASS
	pd_dpm_ignore_vbus_only_event(true);
#endif
	ret = wired_chsw_set_wired_reverse_channel(WIRED_REVERSE_CHANNEL_RESTORE);
	if (ret) {
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: open wired_reverse_channel fail, try next loop!\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "open wired_reverse_channel fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		wldc_stop_charging(di);
		return -1;
	}
	ret = wldc_security_check(di);
	if (ret) {
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: security_check fail, try next loop!\n", __func__);
		wldc_stop_charging(di);
		return -1;
	}
	wldc_set_charge_stage(WLDC_STAGE_SUCCESS);
	wldc_start_charging(di);
	return 0;
}

/*
 * There are a numerous options that are configurable on the wireless receiver
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */
 #ifdef CONFIG_SYSFS
#define WIRELESS_SC_SYSFS_FIELD(_name, n, m, store)	\
{					\
	.attr = __ATTR(_name, m, wireless_sc_sysfs_show, store),	\
	.name = WLDC_SYSFS_##n,		\
}
#define WIRELESS_SC_SYSFS_FIELD_RW(_name, n)               \
	WIRELESS_SC_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, wireless_sc_sysfs_store)
#define WIRELESS_SC_SYSFS_FIELD_RO(_name, n)               \
	WIRELESS_SC_SYSFS_FIELD(_name, n, S_IRUGO, NULL)
static ssize_t wireless_sc_sysfs_show(struct device *dev,
				struct device_attribute *attr, char *buf);
static ssize_t wireless_sc_sysfs_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count);
struct wireless_sc_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};
static struct wireless_sc_sysfs_field_info wireless_sc_sysfs_field_tbl[] = {
	WIRELESS_SC_SYSFS_FIELD_RW(enable_charger, ENABLE_CHARGER),
	WIRELESS_SC_SYSFS_FIELD_RW(iin_thermal, IIN_THERMAL),
};
static struct attribute *wireless_sc_sysfs_attrs[ARRAY_SIZE(wireless_sc_sysfs_field_tbl) + 1];
static const struct attribute_group wireless_sc_sysfs_attr_group = {
	.attrs = wireless_sc_sysfs_attrs,
};
static void wireless_sc_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(wireless_sc_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		wireless_sc_sysfs_attrs[i] = &wireless_sc_sysfs_field_tbl[i].attr.attr;

	wireless_sc_sysfs_attrs[limit] = NULL;
}
static struct wireless_sc_sysfs_field_info *wireless_sc_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(wireless_sc_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp(name, wireless_sc_sysfs_field_tbl[i].attr.attr.name, strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &wireless_sc_sysfs_field_tbl[i];
}
static int wireless_sc_sysfs_create_group(struct wldc_device_info *di)
{
	wireless_sc_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &wireless_sc_sysfs_attr_group);
}
static void wireless_sc_sysfs_remove_group(struct wldc_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &wireless_sc_sysfs_attr_group);
}
static int wireless_sc_create_sysfs(struct wldc_device_info *di)
{
	int ret = 0;
	struct class *power_class = NULL;

	ret = wireless_sc_sysfs_create_group(di);
	if (ret) {
		hwlog_err("create sysfs entries failed!\n");
		return ret;
	}
	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL)
			charge_dev = device_create(power_class, NULL, 0, NULL, "charger");
		ret = sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "wireless_sc");
		if (ret) {
			hwlog_err("create link to wireless_charge fail.\n");
			wireless_sc_sysfs_remove_group(di);
			return ret;
		}
	}
	return 0;
}

#else
static int wireless_sc_create_sysfs(struct wldc_device_info *di)
{
	return 0;
}
#endif
static ssize_t wireless_sc_sysfs_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct wireless_sc_sysfs_field_info *info = NULL;
	struct wldc_device_info *di = g_wireless_sc_di;

	info = wireless_sc_sysfs_field_lookup(attr->attr.name);
	if (!info ||!di)
		return -EINVAL;

	switch (info->name) {
	case WLDC_SYSFS_ENABLE_CHARGER:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.enable_charger);
	case WLDC_SYSFS_IIN_THERMAL:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_data.iin_thermal);
		break;
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return 0;
}
static ssize_t wireless_sc_sysfs_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	struct wireless_sc_sysfs_field_info *info = NULL;
	struct wldc_device_info *di = g_wireless_sc_di;
	long val = 0;

	info = wireless_sc_sysfs_field_lookup(attr->attr.name);
	if (!info ||!di)
		return -EINVAL;

	switch (info->name) {
	case WLDC_SYSFS_ENABLE_CHARGER:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_data.enable_charger = val;
		hwlog_info("[%s] set enable_charger = %ld\n", __func__, di->sysfs_data.enable_charger);
		break;
	case WLDC_SYSFS_IIN_THERMAL:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1500))
			return -EINVAL;
		hwlog_info("[%s] set iin_thermal = %ld\n", __func__, val);
		if (0 == val) {
			di->sysfs_data.iin_thermal = di->volt_para[0].iout_th_high;
		} else if (val < di->volt_para[di->stage_size - 1].iout_th_low) {
			hwlog_info("[%s] iin_thermal = %ld < %d, ignored\n",
				__func__, val, di->volt_para[di->stage_size - 1].iout_th_low);
			return -EINVAL;
		} else {
			di->sysfs_data.iin_thermal = val;
		}
		break;
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return count;
}
static void wireless_sc_shutdown(struct platform_device *pdev)
{
	struct wireless_charge_device_info *di = platform_get_drvdata(pdev);

	hwlog_info("%s ++\n", __func__);
	if (NULL == di) {
		hwlog_err("%s di is null\n", __func__);
		return;
	}
	hwlog_info("%s --\n", __func__);
}

static int wireless_sc_remove(struct platform_device *pdev)
{
	struct wireless_charge_device_info *di = platform_get_drvdata(pdev);

	hwlog_info("%s ++\n", __func__);
	if (NULL == di) {
		hwlog_err("%s di is null\n", __func__);
		return -ENODEV;;
	}
	hwlog_info("%s --\n", __func__);

	return 0;
}

static int wireless_sc_probe(struct platform_device *pdev)
{
	int ret;
	struct wldc_device_info *di = NULL;
	struct device_node *np = NULL;
	di = devm_kzalloc(&pdev->dev,sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("%s: alloc di failed\n", __func__);
		return	-ENOMEM;
	}
	di->dev = &pdev->dev;
	np = di->dev->of_node;
	g_wireless_sc_di = di;
	di->ls_ops = g_wireless_ls_ops;
	di->bi_ops = g_wireless_bi_ops;
	platform_set_drvdata(pdev, di);

	ret = wldc_parse_dts(np,di);
	if (ret) {
		hwlog_err("%s: parse dts fail.\n", __func__);
		return ret;
	}
	wldc_update_basp_para(di);

	di->sysfs_data.iin_thermal = di->volt_para[0].iout_th_high;
	di->sysfs_data.enable_charger = 1;
	di->wldc_stop_charging_complete_flag = true;

	INIT_DELAYED_WORK(&di->wldc_ctrl_work, wldc_control_work);
	INIT_DELAYED_WORK(&di->wldc_calc_work, wldc_calc_work);
	ret = wireless_sc_create_sysfs(di);
	if (ret) {
		hwlog_err("wirelss_sc create sysfs fail.\n");
		return ret;
	}

	if (power_if_ops_register(&wireless_sc_power_if_ops)) {
		hwlog_err("register power_if_ops_register failed!\n");
	}

	hwlog_info("wireless_sc probe ok!\n");
	return 0;
}

static struct of_device_id wireless_sc_match_table[] = {
	{
	 .compatible = "huawei,wireless_sc",
	 .data = NULL,
	},
	{},
};

static struct platform_driver wireless_sc_driver = {
	.probe = wireless_sc_probe,
	.remove = wireless_sc_remove,
	.shutdown = wireless_sc_shutdown,
	.driver = {
		.name = "huawei,wireless_sc",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wireless_sc_match_table),
	},
};
static int __init wireless_sc_init(void)
{
	return platform_driver_register(&wireless_sc_driver);
}
static void __exit wireless_sc_exit(void)
{
	platform_driver_unregister(&wireless_sc_driver);
}

device_initcall_sync(wireless_sc_init);
module_exit(wireless_sc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("wireless sc module driver");
MODULE_AUTHOR("HUAWEI Inc");

