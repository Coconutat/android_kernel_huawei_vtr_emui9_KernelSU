#include <huawei_platform/power/direct_charger.h>
#ifdef CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
#include <huawei_platform/power/usb_short_circuit_protect.h>
#endif

#define HWLOG_TAG direct_charge_lvc
HWLOG_REGIST();

static struct direct_charge_device *g_di;
static struct loadswitch_ops* g_lvc_ops;
static struct batinfo_ops* g_bi_lvc_ops;
struct atomic_notifier_head direct_charge_lvc_fault_notifier_list;
static struct dc_volt_para_info_group lvc_orig_volt_para[DC_VOLT_GROUP_MAX];
ATOMIC_NOTIFIER_HEAD(direct_charge_lvc_fault_notifier_list);


static int direct_charge_lvc_set_enable_charger(unsigned int val)
{
	struct direct_charge_device *di = g_di;
	int ret = 0;

	if (NULL == di) {
		hwlog_err("%s di is NULL\n", __func__);
		return -1;
	}

	if ((val < 0) || (val > 1)) {
		return -1;
	}

	ret = set_direct_charger_disable_flags(
		val?DIRECT_CHARGER_CLEAR_DISABLE_FLAGS:DIRECT_CHARGER_SET_DISABLE_FLAGS,
		DIRECT_CHARGER_SYS_NODE);
	if(ret) {
		hwlog_err("lvc: set direct charge disable flags failed\n");
	}
	hwlog_info("lvc: set enable_charger = %d\n", di->sysfs_enable_charger);

	return 0;
}

static int direct_charge_lvc_get_enable_charger(unsigned int *val)
{
	struct direct_charge_device *di = g_di;

	if (NULL == di) {
		hwlog_err("%s di is NULL\n", __func__);
		return -1;
	}

	*val = di->sysfs_enable_charger;

	return 0;
}

/* define public power interface */
static struct power_if_ops lvc_power_if_ops = {
	.set_enable_charger = direct_charge_lvc_set_enable_charger,
	.get_enable_charger = direct_charge_lvc_get_enable_charger,
	.type_name = "lvc",
};


void direct_charge_lvc_get_fault_notifier(struct atomic_notifier_head **notifier)
{
	*notifier = &direct_charge_lvc_fault_notifier_list;
}

void direct_charge_lvc_check(void)
{
	int local_mode = 0;
	enum charge_done_type charge_done_status = get_charge_done_type();
	struct direct_charge_device *di = NULL;
	if (NULL == g_di)
	{
		hwlog_err("%s g_di is NULL\n", __func__);
		return;
	}
	di = g_di;

	local_mode = direct_charge_get_local_mode();
	if(!(local_mode & LVC_MODE))
	{
		hwlog_err("%s : not support! \n", __func__);
		return;
	}

	if (INVALID == is_direct_charge_ops_valid(di))
	{
		hwlog_err("%s : NULL pointer \n", __func__);
		return;
	}

	if (di->error_cnt >= DC_ERR_CNT_MAX)
	{
		hwlog_info("%s error exceed %d times, direct charge is disabled\n", __func__, DC_ERR_CNT_MAX);
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_CHARGE_DISABLED;
		direct_charge_send_normal_charge_uevent();
		if (FALSE == di->dc_err_report_flag && di->dc_open_retry_cnt <= DC_OPEN_RETRY_CNT_MAX) {
			hwlog_err("%s", di->dc_err_dsm_buff);
			if (di->cc_cable_detect_ok)
				power_dsm_dmd_report(POWER_DSM_BATTERY, DSM_DIRECT_CHARGE_ERR_WITH_STD_CABLE, di->dc_err_dsm_buff);
			else
				power_dsm_dmd_report(POWER_DSM_BATTERY, DSM_DIRECT_CHARGE_ERR_WITH_NONSTD_CABLE, di->dc_err_dsm_buff);
			memset(di->dc_err_dsm_buff, 0, sizeof(di->dc_err_dsm_buff));
			di->dc_err_report_flag = TRUE;
		}
		return;
	}
	if (0 == di->sysfs_enable_charger)
	{
		hwlog_info("%s direct_charge is disabled\n",__func__);
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_CHARGE_DISABLED;
		direct_charge_send_normal_charge_uevent();
		return;
	}
	if (SCP_STAGE_DEFAULT == scp_get_stage_status())
	{
		if (is_support_scp())
		{
			hwlog_err("%s : not support scp \n", __func__);
			return;
		}
	}
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
	if (is_in_uscp_mode()) {
		scp_set_stage_status(SCP_STAGE_DEFAULT);
		hwlog_err("%s direct_charge is disabled by uscp\n",__func__);
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_CHARGE_DISABLED;
		return;
	}
#endif
	if (SCP_STAGE_ADAPTER_DETECT == scp_get_stage_status())
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_ADAPTOR_DETECT;
		di->scp_adaptor_detect_flag = SCP_ADAPTOR_DETECT;
		scp_cable_detect();
	}
	if (!can_battery_temp_do_direct_charge())
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_BAT_TEMP;
		scp_set_stage_status(SCP_STAGE_DEFAULT);
		hwlog_err("temp out of range, try next loop!\n");
		return;
	}
	if (!can_battery_vol_do_direct_charge())
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_BAT_VOL;
		scp_set_stage_status(SCP_STAGE_DEFAULT);
		hwlog_err("volt out of range, try next loop!\n");
		return;
	}
	if(SCP_STAGE_SWITCH_DETECT == scp_get_stage_status())
	{
		di->scp_stop_charging_complete_flag = 0;
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_SWITCH;
		if (SUCC == cutoff_normal_charge())
		{
			scp_set_stage_status(SCP_STAGE_CHARGE_INIT);
		}
		else
		{
			hwlog_err("%s : switch to direct charge failed \n", __func__);
			di->scp_stop_charging_flag_error =1;
			scp_stop_charging();
		}
	}
	if (SCP_STAGE_CHARGE_INIT == scp_get_stage_status())
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_INIT;
		if (SUCC == scp_direct_charge_init())
		{
			scp_set_stage_status(SCP_STAGE_SECURITY_CHECK);
		}
		else
		{
			hwlog_err("%s : direct_charge init failed \n", __func__);
			di->scp_stop_charging_flag_error =1;
			scp_stop_charging();
		}
	}
	if (SCP_STAGE_SECURITY_CHECK == scp_get_stage_status())
	{
		if (0 == scp_security_check())
		{
			scp_set_stage_status(SCP_STAGE_SUCCESS);
		}
		else
		{
			hwlog_err("%s : scp security check fail	\n", __func__);
			di->scp_stop_charging_flag_error =1;
			scp_stop_charging();
		}
	}
	if (SCP_STAGE_SUCCESS == scp_get_stage_status())
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_SUCC;
		scp_start_charging();
	}
	if (SCP_STAGE_CHARGING == scp_get_stage_status())
	{
		hwlog_info("%s : in direct charge process\n", __func__);
	}
	hwlog_info("[%s]direct charge stage  %s !!! \n", __func__, scp_check_stage[scp_get_stage_status()]);
}
/*lint -restore*/


/*lint -save -e* */
static void direct_charge_fault_work(struct work_struct *work)
{
	char buf[512] = { 0 };
	char reg_info[512] = { 0 };
	int bat_capacity = 0;
	struct direct_charge_device *di = container_of(work, struct direct_charge_device, fault_work);
	struct nty_data* data = di->fault_data;

	di->scp_stop_charging_flag_error = 1;
	snprintf(reg_info, sizeof(reg_info), "charge_fault = %d, addr = 0x%x, event1 = 0x%x, event2 = 0x%x\n",
		di->charge_fault, data->addr, data->event1, data->event2);
	strncat(di->dc_err_dsm_buff, reg_info, strlen(reg_info));
	switch (di->charge_fault) {
	case DIRECT_CHARGE_FAULT_VBUS_OVP:
		hwlog_err("vbus ovp happened!\n");
		snprintf(buf, sizeof(buf), "vbus ovp happened\n");
		strncat(buf, reg_info, strlen(reg_info));
		power_dsm_dmd_report(POWER_DSM_BATTERY, DSM_DIRECT_CHARGE_VBUS_OVP, buf);
		break;
	case DIRECT_CHARGE_FAULT_REVERSE_OCP:
		bat_capacity = hisi_battery_capacity();
		hwlog_err("reverse ocp happened! battery capacity is %d \n", bat_capacity);
		if(bat_capacity < BATTERY_CAPACITY_HIGH_TH){
			di->reverse_ocp_cnt ++;
		} else {
			hwlog_err("battery capacity is over threshold, dsm not report!\n");
		}
		if (di->reverse_ocp_cnt >= REVERSE_OCP_CNT) {
			di->reverse_ocp_cnt = REVERSE_OCP_CNT;
			snprintf(buf, sizeof(buf), "reverse ocp happened\n");
			strncat(buf, reg_info, strlen(reg_info));
			power_dsm_dmd_report(POWER_DSM_BATTERY, DSM_DIRECT_CHARGE_REVERSE_OCP, buf);
		}
		break;
	case DIRECT_CHARGE_FAULT_OTP:
		hwlog_err("otp happened!\n");
		di->otp_cnt++;
		if (di->otp_cnt >= OTP_CNT) {
			di->otp_cnt = OTP_CNT;
			snprintf(buf, sizeof(buf), "otp happened\n");
			strncat(buf, reg_info, strlen(reg_info));
			power_dsm_dmd_report(POWER_DSM_BATTERY, DSM_DIRECT_CHARGE_OTP, buf);
		}
		break;
	case DIRECT_CHARGE_FAULT_INPUT_OCP:
		hwlog_err("input ocp happened!\n");
		snprintf(buf, sizeof(buf), "input ocp happened\n");
		strncat(buf, reg_info, strlen(reg_info));
		power_dsm_dmd_report(POWER_DSM_BATTERY, DSM_DIRECT_CHARGE_INPUT_OCP, buf);
		break;
	case DIRECT_CHARGE_FAULT_VDROP_OVP:
		hwlog_err("vdrop ovp happened!\n");
		snprintf(buf, sizeof(buf), "vdrop ovp happened\n");
		strncat(buf, reg_info, strlen(reg_info));
		//power_dsm_dmd_report(POWER_DSM_BATTERY, DSM_DIRECT_CHARGE_VDROP_OVP, buf);
		break;
	default:
		hwlog_err("unknow fault: %d happened!\n", di->charge_fault);
		break;
	}
}

#ifdef CONFIG_SYSFS
static ssize_t direct_charge_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf);
static ssize_t direct_charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

static struct direct_charge_sysfs_field_info direct_charge_sysfs_field_tbl[] = {
	DIRECT_CHARGE_SYSFS_FIELD_RW(enable_charger, ENABLE_CHARGER),
	DIRECT_CHARGE_SYSFS_FIELD_RW(iin_thermal, IIN_THERMAL),
	DIRECT_CHARGE_SYSFS_FIELD_RO(adaptor_detect, ADAPTOR_DETECT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(loadswitch_id, LOADSWITCH_ID),
	DIRECT_CHARGE_SYSFS_FIELD_RO(loadswitch_name, LOADSWITCH_NAME),
	DIRECT_CHARGE_SYSFS_FIELD_RO(vbat, VBAT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(ibat, IBAT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(vadapt, VADAPT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(iadapt, IADAPT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(ls_vbus, LS_VBUS),
	DIRECT_CHARGE_SYSFS_FIELD_RO(ls_ibus, LS_IBUS),
	DIRECT_CHARGE_SYSFS_FIELD_RO(full_path_resistance, FULL_PATH_RESISTANCE),
	DIRECT_CHARGE_SYSFS_FIELD_RW(direct_charge_succ, DIRECT_CHARGE_SUCC),
	DIRECT_CHARGE_SYSFS_FIELD_RW(set_resistance_threshold, SET_RESISTANCE_THRESHOLD),
};

static struct attribute *direct_charge_sysfs_attrs[ARRAY_SIZE(direct_charge_sysfs_field_tbl) + 1];

static const struct attribute_group direct_charge_sysfs_attr_group = {
	.attrs = direct_charge_sysfs_attrs,
};

/**********************************************************
*  Function:       direct_charge_sysfs_init_attrs
*  Description:    initialize direct_charge_sysfs_attrs[] for direct_charge attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void direct_charge_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(direct_charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		direct_charge_sysfs_attrs[i] = &direct_charge_sysfs_field_tbl[i].attr.attr;
	}
	direct_charge_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}

/**********************************************************
*  Function:       direct_charge_sysfs_field_lookup
*  Description:    get the current device_attribute from direct_charge_sysfs_field_tbl by attr's name
*  Parameters:   name:device attribute name
*  return value:  direct_charge_sysfs_field_tbl[]
**********************************************************/
static struct direct_charge_sysfs_field_info *direct_charge_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(direct_charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp
		    (name, direct_charge_sysfs_field_tbl[i].attr.attr.name,
		     strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &direct_charge_sysfs_field_tbl[i];
}
/**********************************************************
*  Function:       direct_charge_sysfs_show
*  Description:    show the value for all direct charge device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t direct_charge_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct direct_charge_sysfs_field_info *info = NULL;
	struct direct_charge_device *di = dev_get_drvdata(dev);
	enum usb_charger_type type = charge_get_charger_type();
	int ret;

	info = direct_charge_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case DIRECT_CHARGE_SYSFS_ENABLE_CHARGER:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_enable_charger);
	case DIRECT_CHARGE_SYSFS_IIN_THERMAL:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_iin_thermal);
	case DIRECT_CHARGE_SYSFS_ADAPTOR_DETECT:
		ret = SCP_ADAPTOR_DETECT_FAIL;
		if (INVALID == is_direct_charge_ops_valid(di))
		{
			hwlog_err("(%s)invalid ops\n", __func__);
			return snprintf(buf, PAGE_SIZE, "%d\n", ret);
		}
		if(di->scp_ops->is_support_scp())
		{
			hwlog_err("(%s)not support scp\n", __func__);
			return snprintf(buf, PAGE_SIZE, "%d\n", ret);
		}
		if ((CHARGER_TYPE_STANDARD == type) ||
			((CHARGER_REMOVED == type) && (IN_SCP_CHARGING_STAGE == is_in_scp_charging_stage())))
		{
			if (SCP_ADAPTOR_DETECT == di->scp_adaptor_detect_flag)
			{
				ret = 0;
			}else{
				ret = ADAPTOR_SCP_DETECT_FAIL;
			}
			hwlog_info("(%s)adaptor_detect = %d\n", __func__, ret);
		}
		return snprintf(buf, PAGE_SIZE, "%d\n", ret);
	case DIRECT_CHARGE_SYSFS_LOADSWITCH_ID:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ls_id);
	case DIRECT_CHARGE_SYSFS_LOADSWITCH_NAME:
		return snprintf(buf, PAGE_SIZE, "%s\n", NULL == di->ls_name ? "ERROR" : di->ls_name);
	case DIRECT_CHARGE_SYSFS_VBAT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->vbat);
	case DIRECT_CHARGE_SYSFS_IBAT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ibat);
	case DIRECT_CHARGE_SYSFS_VADAPT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->vadapt);
	case DIRECT_CHARGE_SYSFS_IADAPT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->iadapt);
	case DIRECT_CHARGE_SYSFS_LS_VBUS:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ls_vbus);
	case DIRECT_CHARGE_SYSFS_LS_IBUS:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ls_ibus);
	case DIRECT_CHARGE_SYSFS_FULL_PATH_RESISTANCE:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->full_path_resistance);
	case DIRECT_CHARGE_SYSFS_DIRECT_CHARGE_SUCC:
		hwlog_info("(%s)direct_charge_succ_flag = %d\n", __func__, di->direct_charge_succ_flag);
		return snprintf(buf, PAGE_SIZE, "%d\n", di->direct_charge_succ_flag);
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return 0;
}

/**********************************************************
*  Function:       direct_charge_sysfs_store
*  Description:    set the value for charge_data's node which is can be written
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t direct_charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct direct_charge_sysfs_field_info *info = NULL;
	struct direct_charge_device *di = dev_get_drvdata(dev);
	long val = 0;
	int ret;

	info = direct_charge_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case DIRECT_CHARGE_SYSFS_ENABLE_CHARGER:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		ret = set_direct_charger_disable_flags(
				val?DIRECT_CHARGER_CLEAR_DISABLE_FLAGS:DIRECT_CHARGER_SET_DISABLE_FLAGS,
				DIRECT_CHARGER_SYS_NODE);
		if(ret) {
			hwlog_err("Set direct charge disable flags failed in %s.", __func__);
		}
		hwlog_info("set enable_charger = %d\n", di->sysfs_enable_charger);
		break;
	case DIRECT_CHARGE_SYSFS_IIN_THERMAL:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 8000))
			return -EINVAL;
		hwlog_info("set iin_thermal = %ld\n", val);
		if (0 == val)
		{
			di->sysfs_iin_thermal = di->iin_thermal_default;
		}
		else if (val < di->orig_volt_para_p[0].volt_info[di->stage_size - 1].cur_th_low)
		{
			hwlog_info("iin_thermal = %ld < %d, ignored\n", val, di->orig_volt_para_p[0].volt_info[di->stage_size - 1].cur_th_low);
			return -EINVAL;
		}
		else
		{
			di->sysfs_iin_thermal = val;
		}
		break;
	case DIRECT_CHARGE_SYSFS_SET_RESISTANCE_THRESHOLD:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > MAX_RESISTANCE))
			return -EINVAL;
		hwlog_info("set resistance threshold = %ld\n", val);
		di->standard_cable_full_path_res_max = val;
		di->full_path_res_max = val;
		break;
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return count;
}

/**********************************************************
*  Function:       direct_charge_sysfs_create_group
*  Description:    create the direct charge device sysfs group
*  Parameters:   di:direct_charge_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int direct_charge_sysfs_create_group(struct direct_charge_device *di)
{
	direct_charge_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &direct_charge_sysfs_attr_group);
}

/**********************************************************
*  Function:       direct_charge_sysfs_remove_group
*  Description:    remove the direct_charge device sysfs group
*  Parameters:   di:direct_charge_device
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static inline void direct_charge_sysfs_remove_group(struct direct_charge_device *di)
{
	sysfs_remove_group(&di->dev->kobj, &direct_charge_sysfs_attr_group);
}
#else
static int direct_charge_sysfs_create_group(struct direct_charge_device *di)
{
	return 0;
}

static inline void direct_charge_sysfs_remove_group(struct direct_charge_device *di)
{
}
#endif


int loadswitch_ops_register(struct loadswitch_ops* ops)
{
	int ret = 0;

	if (ops != NULL)
	{
		g_lvc_ops = ops;
	}
	else
	{
		hwlog_err("ls ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

int batinfo_lvc_ops_register(struct batinfo_ops* ops)
{
	int ret = 0;

	if (ops != NULL)
	{
		g_bi_lvc_ops = ops;
	}
	else {
		hwlog_err("batinfo lvc ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

int direct_charge_get_lvc_di(struct direct_charge_device **di)
{
	if (NULL == g_di)
	{
		hwlog_err("%s g_di is NULL\n", __func__);
		return -1;
	}
	*di = g_di;
	return 0;
}
static int direct_charge_lvc_probe(struct platform_device	*pdev)
{
	int ret = 0;
	struct direct_charge_device *di;
	struct class *power_class = NULL;
	struct device_node *np = NULL;
	struct smart_charge_ops* g_scp_ops;
	struct scp_power_supply_ops* g_scp_ps_ops;
	struct direct_charge_cable_detect_ops* g_cable_detect_ops;

	di = devm_kzalloc(&pdev->dev,sizeof(*di), GFP_KERNEL);
	if (!di)
	{
		hwlog_err("alloc di failed\n");
		return	-ENOMEM;
	}
	di->dev = &pdev->dev;
	np = di->dev->of_node;
	di->orig_volt_para_p = lvc_orig_volt_para;
	ret = direct_charge_parse_dts(np, di);
	if (ret)
	{
		hwlog_err("parse dts fail\n");
		goto fail_0;
	}

	direct_charge_get_g_scp_ops(&g_scp_ops);
	direct_charge_get_g_scp_ps_ops(&g_scp_ps_ops);
	direct_charge_get_g_cable_detect_ops(&g_cable_detect_ops);
	di->scp_ops = g_scp_ops;
	di->scp_ps_ops = g_scp_ps_ops;
	di->ls_ops = g_lvc_ops;
	di->bi_ops = g_bi_lvc_ops;
	di->direct_charge_cable_detect = g_cable_detect_ops;
	di->sysfs_enable_charger = 1;
	di->scp_stage = SCP_STAGE_DEFAULT;
	di->sysfs_iin_thermal = di->orig_volt_para_p[0].volt_info[0].cur_th_high;
	di->max_adaptor_iset = di->orig_volt_para_p[0].volt_info[0].cur_th_high;
	di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_ADAPTOR_DETECT;
	di->scp_stop_charging_complete_flag = 1;
	di->dc_err_report_flag = FALSE;
	if (INVALID == is_direct_charge_ops_valid(di))
	{
		hwlog_err("direct charge ops is	NULL!\n");
		ret = -EINVAL;
		goto fail_0;
	}

	di->sc_conv_ocp_count = 0;
	direct_charge_or_set_local_mode(LVC_MODE);

	platform_set_drvdata(pdev, di);

	di->direct_charge_wq = create_singlethread_workqueue("direct_charge_wq");
	di->direct_charge_watchdog_wq = create_singlethread_workqueue("direct_charge_watchdog_wq");

	wake_lock_init(&di->direct_charge_lock, WAKE_LOCK_SUSPEND, "direct_charge_wakelock");
	INIT_WORK(&di->threshold_caculation_work, threshold_caculation_work);
	INIT_WORK(&di->charge_control_work, charge_control_work);
	INIT_WORK(&di->fault_work, direct_charge_fault_work);
	INIT_WORK(&di->kick_watchdog_work, kick_watchdog_work);


	hrtimer_init(&di->threshold_caculation_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->threshold_caculation_timer.function	= threshold_caculation_timer_func;

	hrtimer_init(&di->charge_control_timer,	CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->charge_control_timer.function = charge_control_timer_func;

	hrtimer_init(&di->kick_watchdog_timer,	CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->kick_watchdog_timer.function = kick_watchdog_timer_func;

	ret = direct_charge_sysfs_create_group(di);
	if (ret)
		hwlog_err("can't create	lvc charge sysfs entries\n");
	power_class = hw_power_get_class();
	if (power_class)
	{
		if (charge_dev == NULL)
			charge_dev = device_create(power_class,	NULL, 0, NULL, "charger");
		ret = sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "direct_charger");
		if (ret)
		{
			hwlog_err("create link to direct_charger_lvc fail.\n");
			goto free_sysfs_group;
		}
	}
	g_di = di;
	direct_charge_set_di(di);

	di->fault_nb.notifier_call = direct_charge_fault_notifier_call;
	ret = atomic_notifier_chain_register(&direct_charge_lvc_fault_notifier_list, &di->fault_nb);
	if (ret < 0)
	{
		hwlog_err("direct_charge_fault_register_notifier failed\n");
		goto free_sysfs_group;
	}
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	ret = di->ls_ops->ls_init();
	di->ls_id = di->ls_ops->get_ls_id();
	if (di->ls_id < 0 || di->ls_id >= LOADSWITCH_TOTAL)
	{
		hwlog_err("error loadswitch id info\n");
		di->ls_id = LOADSWITCH_TOTAL;
	}
	di->ls_name = loadswitch_name[di->ls_id];
	hwlog_info("loadswitch id = %d(%s)\n", di->ls_id, di->ls_name);
	ret |= di->ls_ops->ls_exit();
	if (ret)
	{
		hwlog_err("dev_check for loadswitch fail.\n");
	}
	else
	{
		set_hw_dev_flag(DEV_I2C_LOADSWITCH);
		hwlog_info("dev_check for loadswitch succ.\n");
	}
#endif

	if (power_if_ops_register(&lvc_power_if_ops)) {
		hwlog_err("register power_if_ops_register failed!\n");
	}

	hwlog_info("direct charger lvc probe ok!\n");

	return	0;

free_sysfs_group:
	direct_charge_sysfs_remove_group(di);
	wake_lock_destroy(&di->direct_charge_lock);
fail_0:
	devm_kfree(&pdev->dev, di);
	di = NULL;
	return	ret;
}

/**********************************************************
*	Function: direct_charge_remove
*	Description: direct_charge module remove
*	Parameters: pdev:platform_device
*	return value: NULL
**********************************************************/
static int direct_charge_lvc_remove(struct platform_device *pdev)
{
	struct direct_charge_device *di = platform_get_drvdata(pdev);

	if (NULL == di)
	{
		hwlog_err("[%s]di is NULL!\n", __func__);
		return -ENODEV;
	}
	wake_lock_destroy(&di->direct_charge_lock);
	return 0;
}
/**********************************************************
*	Function: direct_charge_shutdown
*	Description: direct_charge module shutdown
*	Parameters: pdev:platform_device
*	return value: NULL
**********************************************************/
static void direct_charge_lvc_shutdown(struct platform_device *pdev)
{
	struct direct_charge_device *di = platform_get_drvdata(pdev);

	if (NULL == di)
	{
		hwlog_err("[%s]di is NULL!\n", __func__);
		return;
	}

	return;
}

#ifdef	CONFIG_PM
/**********************************************************
*	Function: direct_charge_suspend
*	Description: direct charge module suspend
*	Parameters: pdev:platform_device
*		    state:unused
*	return	value: 0-sucess	or others-fail
**********************************************************/
static int direct_charge_lvc_suspend(struct	platform_device	*pdev, pm_message_t state)
{
	return	0;
}

/**********************************************************
*	Function: direct charge_resume
*	Description: direct charge module resume
*	Parameters: pdev:platform_device
*	return	value: 0-sucess	or others-fail
**********************************************************/
static int direct_charge_lvc_resume(struct	platform_device	*pdev)
{
	return	0;
}
#endif	/* CONFIG_PM */

static struct of_device_id direct_charge_lvc_match_table[] = {
	{
		.compatible = "direct_charger",
		.data = NULL,
	},
	{
	},
};

static struct platform_driver direct_charge_lvc_driver = {
	.probe = direct_charge_lvc_probe,
	.remove	= direct_charge_lvc_remove,
#ifdef	CONFIG_PM
	.suspend = direct_charge_lvc_suspend,
	.resume	= direct_charge_lvc_resume,
#endif
	.shutdown = direct_charge_lvc_shutdown,
	.driver	= {
		.name = "direct_charger",
		.owner = THIS_MODULE,
		.of_match_table	= of_match_ptr(direct_charge_lvc_match_table),
	},
};

/**********************************************************
*	Function: direct_charge_init
*	Description: direct charge module initialization
*	Parameters: NULL
*	return value: 0-sucess or others-fail
**********************************************************/
static int __init direct_charge_lvc_init(void)
{
	return platform_driver_register(&direct_charge_lvc_driver);
}

/**********************************************************
*	Function: direct_charge_exit
*	Description: direct charge module exit
*	Parameters: NULL
*	return	value:	NULL
**********************************************************/
static void __exit direct_charge_lvc_exit(void)
{
	platform_driver_unregister(&direct_charge_lvc_driver);
}
/*lint -restore*/

late_initcall(direct_charge_lvc_init);
module_exit(direct_charge_lvc_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("direct charger lvc module driver");
MODULE_AUTHOR("HUAWEI Inc");

