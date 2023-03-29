#include <huawei_platform/power/wireless_direct_charger.h>
#include <huawei_platform/power/wired_channel_switch.h>
#include <huawei_platform/power/wireless_direct_charger.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <huawei_platform/power/wired_channel_switch.h>
#ifdef CONFIG_BOOST_5V
#include <huawei_platform/power/boost_5v.h>
#endif
#include <huawei_platform/power/wireless_charger.h>
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif

#define HWLOG_TAG wireless_dc
HWLOG_REGIST();

static struct wldc_device_info *g_wldc_di = NULL;
static char g_wldc_stage_char[WLDC_STAGE_TOTAL][WIRELESS_STAGE_STR_LEN] = {
	{"STAGE_DEFAULT"}, {"STAGE_CHECK"}, {"STAGE_SUCCESS"},
	{"STAGE_CHARGING"}, {"STAGE_CHARGE_DONE"},{"STAGE_STOP_CHARGING"}
};

void wldc_set_di(struct wldc_device_info *di)
{
	if (!di) {
		hwlog_err("%s: di null!\n", __func__);
	} else {
		g_wldc_di = di;
	}
}
void wldc_set_charge_stage(enum wldc_stage wldc_stage)
{
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	if (wldc_stage < WLDC_STAGE_TOTAL && di->wldc_stage != wldc_stage) {
		di->wldc_stage = wldc_stage;
		hwlog_info("%s set charge stage to %s\n", __func__, g_wldc_stage_char[wldc_stage]);
	}
}
static int wldc_msleep(int sleep_ms)
{
	int i;
	int interval = 25;  //ms
	int cnt = sleep_ms/interval;
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s: di null!\n", __func__);
		return -1;
	}
	for(i = 0; i < cnt; i++) {
		msleep(interval);
		if (di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("%s: wireless tx disconnect, stop msleep\n", __func__);
			return -1;
		}
	}
	return 0;
}
static int wldc_get_bat_voltage(void)
{
	int btb_vol = 0;
	int package_vol = 0;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}

	btb_vol = di->bi_ops->get_bat_btb_voltage();
	package_vol = di->bi_ops->get_bat_package_voltage();
	if (btb_vol < 0 && package_vol < 0) {
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: error\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "get bi->vbatt fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		return -1;
	}

	return btb_vol > package_vol ? btb_vol : package_vol;
}
static int wldc_get_bat_current(void)
{
	int ret;
	int bat_curr = 0;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}
	ret = di->bi_ops->get_bat_current(&bat_curr);
	if (ret) {
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: error\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "get bi->ibatt fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
	}
	return bat_curr;
}
static int wldc_get_ls_vbus(void)
{
	int ret;
	int vbus = 0;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}
	ret = di->bi_ops->get_vbus_voltage(&vbus);
	if (ret < 0) {
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: error\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "get bi->vbus fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
	}
	return vbus;
}
static int wldc_get_ls_ibus(void)
{
	int ret;
	int ibus = 0;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return -1;
	}
	ret = di->bi_ops->get_ls_ibus(&ibus);
	if (ret < 0) {
		di->wldc_stop_flag_error = 1;
		hwlog_err("%s: error\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "get bi->ls_ibus fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
	}
	return ibus;
}
int wldc_get_error_cnt(void)
{
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_debug("%s: di null\n", __func__);
		return -1;
	}
	return di->error_cnt;
}
int wldc_get_warning_cnt(void)
{
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_debug("%s: di null\n", __func__);
		return -1;
	}
	return di->warning_cnt;
}
bool wldc_is_stop_charging_complete(void)
{
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s g_wldc_di is null\n", __func__);
		return true;
	}

	return (true == di->wldc_stop_charging_complete_flag);
}
int can_vbatt_do_wldc_charge(struct wldc_device_info *di)
{
	int vbatt = hisi_battery_voltage();
	if (vbatt < di->vbatt_min || vbatt > di->vbatt_max) {
		hwlog_err("%s: vbatt(%dmV) exceed [%d, %d]mV\n",
			__func__, vbatt, di->vbatt_min, di->vbatt_max);
		return -1;
	}
	return 0;
}
int can_tbatt_do_wldc_charge(void)
{
	int tbatt = hisi_battery_temperature();
	if (tbatt <= WLDC_TBATT_MIN || tbatt >= WLDC_TBATT_MAX) {
		hwlog_err("%s: tbatt(%d) exceed (%d, %d)\n",
			__func__, tbatt, WLDC_TBATT_MIN, WLDC_TBATT_MAX);
		return -1;
	}
	return 0;
}
int wldc_retore_normal_charge_path(void)
{
	return charge_set_hiz_enable(HIZ_MODE_DISABLE);
}
int wldc_cutt_off_normal_charge_path(void)
{
	return charge_set_hiz_enable(HIZ_MODE_ENABLE);
}
int wldc_check_ops_valid(struct wldc_device_info *di)
{
	if (NULL == di->ls_ops || NULL == di->ls_ops->ls_init ||
		NULL == di->ls_ops->ls_enable || NULL == di->ls_ops->is_ls_close ||
		NULL == di->ls_ops->ls_exit) {
		hwlog_err("%s: di->ls_ops is null!\n", __func__);
		return -1;
	}
	if (NULL == di->bi_ops || NULL == di->bi_ops->init ||
		NULL == di->bi_ops->get_ls_ibus || NULL == di->bi_ops->get_bat_btb_voltage ||
		NULL == di->bi_ops->get_bat_package_voltage || NULL == di->bi_ops->exit) {
		hwlog_err("%s: di->bi_ops is null!\n", __func__);
		return -1;
	}
	return 0;
}
int wldc_set_rx_init_vout(struct wldc_device_info *di)
{
	int ret;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	ret = wireless_charge_set_tx_vout(di->rx_init_vout);
	if (ret) {
		hwlog_err("%s: set tx init vout fail\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "set tx init vout fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		return -1;
	}
	ret = wireless_charge_set_rx_vout(di->rx_init_vout);
	if (ret) {
		hwlog_err("%s: set rx init vout fail\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "set rx init vout fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		return -1;
	}
	di->rx_vout_set = di->rx_init_vout;
	return 0;
}
int wldc_chip_init(struct wldc_device_info *di)
{
	int ret;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	ret = di->ls_ops->ls_init();
	if (ret) {
		hwlog_err("%s: ls init fail!\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "ls init fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		return -1;
	}
	ret = di->bi_ops->init();
	if (ret) {
		hwlog_err("%s: bi init fail!\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "batt_info init fail\n");
		strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		return -1;
	}
	hwlog_info("[%s] succ!\n", __func__);
	return 0;
}
static int wldc_open_direct_charge_path(struct wldc_device_info *di)
{
	int ret, i;
	int ls_vbatt, ls_ibus, rx_vrect, rx_vout, rx_vout_set;
	int soc;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	soc = hisi_battery_capacity();
	ls_vbatt = wldc_get_bat_voltage();
	if (ls_vbatt < 0) {
		hwlog_err("%s: get ls_vbatt fail!\n", __func__);
		return -1;
	}
	hwlog_info("ls_vbatt = %dmV\n", ls_vbatt);
	rx_vout_set = ls_vbatt * di->volt_ratio + di->rx_init_delt_vout;
	ret = wireless_charge_set_rx_vout(rx_vout_set);
	if (ret) {
		hwlog_err("%s: set rx vout fail\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "open wldc: set rx vout fail\n");
		goto FuncEnd;
	}
	di->rx_vout_set = rx_vout_set;
	ret = di->ls_ops->ls_enable(1);
	if (ret) {
		hwlog_err("%s: ls enable fail!\n", __func__);
		snprintf(tmp_buf, sizeof(tmp_buf), "open wldc: ls enable fail\n");
		goto FuncEnd;
	}
	for (i = 0; i < WLDC_OPEN_PATH_CNT; i++) {
		ret = wldc_msleep(300);  //only used here
		if (ret) {
			return -1;
		}
		ls_ibus = wldc_get_ls_ibus();
		if (ls_ibus > WLDC_OPEN_PATH_IOUT_MIN) {
			hwlog_info("[%s] succ\n", __func__);
			return 0;
		}
		ls_vbatt = wldc_get_bat_voltage();
		if (ls_vbatt > di->vbatt_max) {
			hwlog_err("%s: ls_vbatt(%dmV) too high, ls_ibus = %d, try next loop\n",
				__func__, ls_vbatt, ls_ibus);
			snprintf(tmp_buf, sizeof(tmp_buf),
				"open wldc: ls_vbatt(%dmV) too high, ls_ibus = %dmA\n", ls_vbatt, ls_ibus);
			goto FuncEnd;
		}
		rx_vout_set += di->vstep;
		ret = wireless_charge_set_rx_vout(rx_vout_set);
		if (ret) {
			hwlog_err("%s: set rx vout fail\n", __func__);
			snprintf(tmp_buf, sizeof(tmp_buf), "open wldc: set rx vout fail\n");
			goto FuncEnd;
		}
		di->rx_vout_set = rx_vout_set;
		rx_vrect = wireless_charge_get_rx_vrect();
		rx_vout = wireless_charge_get_rx_vout();
		hwlog_info("[%s] ls_ibus = %dmA, vrect = %dmV, vout = %dmV\n",
			__func__, ls_ibus, rx_vrect, rx_vout);
		di->ls_ops->ls_enable(1);
		if (di->ls_ops->is_ls_close()) {
			hwlog_err("%s: ls is close\n", __func__);
		}
		if (di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("%s: wireless tx disconnect, stop check\n", __func__);
			return -1;
		}
	}
	if (i >= WLDC_OPEN_PATH_CNT) {
		hwlog_info("%s: try too many times, ls_ibus = %d\n", __func__, ls_ibus);
		snprintf(tmp_buf, sizeof(tmp_buf), "open wldc: try too many times, ls_ibus = %d\n", ls_ibus);
		goto FuncEnd;
	}

	return 0;

FuncEnd:
	strncat(di->wldc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
	if (soc >= BATTERY_CAPACITY_HIGH_TH) {
		di->dc_open_retry_cnt ++;
		hwlog_info("[%s] fail, may because of high battery capacity(d%%%)\n", __func__, soc);
	}
	return -1;
}
static int wldc_leak_current_check(struct wldc_device_info *di)
{
	int i, iout;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	char dsm_buf[CHARGE_DMDLOG_SIZE] = { 0 };

	for (i = 0; i < WLDC_LEAK_CURRENT_CHECK_CNT; i++) {
		iout = wireless_charge_get_rx_iout();
		hwlog_info("%s: leak_current = %dmA\n", __func__, iout);
		snprintf(tmp_buf, sizeof(tmp_buf), "leak_current = %dmA\n", iout);
		strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
		if (iout > WLDC_LEAK_CURRENT_MIN) {
			strncat(di->wldc_err_dsm_buff, dsm_buf, strlen(dsm_buf));
			return -1;
		}
		if (di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("%s: wireless tx disconnect, stop check\n", __func__);
			return -1;
		}
		wldc_msleep(50); //only used here
	}
	return 0;
}
static int wldc_vrect_vout_diff_value_check(struct wldc_device_info *di)
{
	int i, vrect, vout;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	char dsm_buf[CHARGE_DMDLOG_SIZE] = { 0 };

	for (i = 0; i < WLDC_DIFF_VOLT_CHECK_CNT; i++) {
		vrect = wireless_charge_get_rx_vrect();
		vout = wireless_charge_get_rx_vout();
		hwlog_info("%s: vrect = %dmV, vout = %dmV, delta_th = %dmV\n",
			__func__, vrect, vout, WLDC_DIFF_VOLT_CHECK_TH);
		snprintf(tmp_buf, sizeof(tmp_buf), "check vdiff, vrect = %dmV, vout = %dmV\n", vrect, vout);
		strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
		if (vrect - vout > WLDC_DIFF_VOLT_CHECK_TH) {
			return 0;
		}
		if (di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("%s: wireless tx disconnect, stop check\n", __func__);
			return -1;
		}
		wldc_msleep(100); //only used here
	}
	strncat(di->wldc_err_dsm_buff, dsm_buf, strlen(dsm_buf));
	return -1;
}
static int wldc_vout_accuracy_check(struct wldc_device_info *di)
{
	int ret = 0;
	int i;
	int rx_vout, rx_iout;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	char dsm_buf[CHARGE_DMDLOG_SIZE] = { 0 };

	ret = wldc_msleep(300); //only used here, delay for stable vout
	if (ret) {
		return -1;
	}

	for (i = 0; i < WLDC_VOUT_ACCURACY_CHECK_CNT; i++) {
		rx_vout = wireless_charge_get_rx_vout();
		rx_iout = wireless_charge_get_rx_iout();
		snprintf(tmp_buf, sizeof(tmp_buf),"check vout_acc, "
			"vout_set = %dmV, vout = %dmV, iout = %dmA\n", di->rx_init_vout, rx_vout, rx_iout);
		strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
		hwlog_info("[%s] rx_vout_set = %dmV, rx_vout = %dmV, err_th = %dmV, rx_iout = %dmA\n",
				__func__, di->rx_init_vout, rx_vout, di->rx_vout_err_th, rx_iout);
		if (abs(rx_vout - di->rx_init_vout) > di->rx_vout_err_th) {
			strncat(di->wldc_err_dsm_buff, dsm_buf, strlen(dsm_buf));
			return -1;
		}
		if (di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
			hwlog_err("%s: wireless tx disconnect, stop check\n", __func__);
			return -1;
		}
		wldc_msleep(100); //only used here
	}
	hwlog_err("[%s] succ\n", __func__);
	return 0;
}
int wldc_security_check(struct wldc_device_info *di)
{
	int ret;
	ret = wldc_vout_accuracy_check(di);
	if (ret) {
		hwlog_err("%s: voltage_accuracy_check fail, try next loop!\n", __func__);
		return -1;
	}
	ret = wldc_leak_current_check(di);
	if (ret) {
		hwlog_err("%s: leak_current_check fail, try next loop!\n", __func__);
		return -1;
	}
	ret = wldc_vrect_vout_diff_value_check(di);
	if (ret) {
		hwlog_err("%s: vrect_vout_delta_check fail, try next loop!\n", __func__);
		return -1;
	}
	ret = wldc_open_direct_charge_path(di);
	if (ret) {
		hwlog_err("%s: open_direct_charg_path fail, try next loop!\n", __func__);
		return -1;
	}
	return 0;
}
int wireless_direct_charge_check(void)
{
	int ret;
	struct wldc_device_info *di = NULL;
	if (true) {//tmp_code
		ret = wireless_sc_get_di(&di);
		if (!ret) {
			wldc_set_di(di);
			return wireless_sc_charge_check();
		}
		return -1;
	}
	return -1;
}
static void wldc_charge_regulation(struct wldc_device_info *di)
{
	int ret;
	int vrect, vout, iout, iout_avg;
	int ls_vbus, ls_ibus, ls_vbatt, ls_ibatt;

	vrect = wireless_charge_get_rx_vrect();
	vout = wireless_charge_get_rx_vout();
	iout = wireless_charge_get_rx_iout();
	iout_avg = wireless_charge_get_rx_avg_iout();
	ls_vbatt = wldc_get_bat_voltage();
	ls_ibatt = wldc_get_bat_current();
	ls_vbus = wldc_get_ls_vbus();
	ls_ibus = wldc_get_ls_ibus();
	hwlog_info("cur_stage = %d, ls_vbus = %dmV, ls_ibus = %dmA, ls_vbatt = %dmV, ls_ibatt = %dmA, "
		"vrect = %dmV, vout = %dmV, iout = %dmA, iout_avg = %dmA, "
		"vbat_th = %dmV, iout_th_high = %dmA, iout_th_low = %dmA, iout_err_th = %dmA\n",
		di->cur_stage, ls_vbus, ls_ibus, ls_vbatt, ls_ibatt, vrect, vout, iout, iout_avg,
		di->cur_vbat_th, di->cur_iout_th_high, di->cur_iout_th_low, di->rx_iout_err_th);
	if (iout > WLDC_IOUT_MAX) {
		di->rx_vout_set -= di->vstep;
		hwlog_info("[%s] iout(%dmA) > %dmA, decrease rx_vout!\n",
			__func__, iout, WLDC_IOUT_MAX);
		goto FuncEnd;
	}
	if (iout_avg > di->cur_iout_th_high + di->rx_iout_err_th) {
		di->rx_vout_set -= di->vstep;
		hwlog_info("[%s] iout_avg(%dmA) > %dmA, decrease rx_vout!\n",
			__func__, iout_avg, di->cur_iout_th_high + di->rx_iout_err_th);
		goto FuncEnd;
	}
	if (ls_vbatt > di->cur_vbat_th) {
		di->rx_vout_set -= di->volt_ratio * (ls_vbatt - di->cur_vbat_th);
		hwlog_info("[%s] ls_vbatt(%dmV) > %dmV, decrease rx_vout!\n",
			__func__, ls_vbatt, di->cur_vbat_th);
		goto FuncEnd;
	}
	if (iout_avg > di->cur_iout_th_high - di->rx_iout_err_th) {
		hwlog_debug("[%s] iout_avg(%dmA) > %dmA, do nothing!\n",
			__func__, iout_avg, di->cur_iout_th_high - di->rx_iout_err_th);
		return;
	}
	if (iout > di->cur_iout_th_high - di->rx_iout_err_th) {
		hwlog_debug("[%s] iout(%dmA) > %dmA, do nothing!\n",
			__func__, iout, di->cur_iout_th_high - di->rx_iout_err_th);
		return;
	}
	if (iout_avg < di->cur_iout_th_high - di->rx_iout_err_th) {
		di->rx_vout_set += di->vstep;
		hwlog_info("[%s] iout_avg(%dmA) < %dmA, increase rx_vout!\n",
			__func__, iout_avg, di->cur_iout_th_high - di->rx_iout_err_th);
		goto FuncEnd;
	}
FuncEnd:
	ret = wireless_charge_set_rx_vout(di->rx_vout_set);
	if (ret)
		hwlog_err("%s: set_rx_vout fail\n", __func__);
}
static void wldc_select_charge_stage(struct wldc_device_info *di)
{
	int vbatt, ibatt, iout;
	int i,cur_stage, vbat_th;

	vbatt = wldc_get_bat_voltage();
	ibatt = wldc_get_bat_current();
	iout = wireless_charge_get_rx_iout();
	di->pre_stage = di->cur_stage;

	for (i = di->stage_size - 1; i >= 0; --i) {
		vbat_th = di->volt_para[i].vbatt_th;
		if (vbatt >= vbat_th && iout <= di->volt_para[i].iout_th_low) {
			cur_stage = 2*i +2;
			break;
		} else if (vbatt >= vbat_th) {
			cur_stage = 2*i +1;
			break;
		}
	}
	if (i < 0) {
		cur_stage = 0;
	}
	if (cur_stage >= di->pre_stage) {
		di->cur_stage = cur_stage;  ////jump_stage_if_need
	}
}
static void wldc_check_exit_condition(struct wldc_device_info *di)
{
	int ret;
	int pmode_index = wireless_charge_get_power_mode();
	ret = can_tbatt_do_wldc_charge();
	if (ret) {
		di->wldc_stop_flag_info = 1;
		return;
	}
	wireless_charge_update_max_vout_and_iout(false);
	if (!wireless_charge_mode_judge_criterion(pmode_index, WIRELESS_MODE_FINAL_JUDGE_CRIT)) {
		di->wldc_stop_flag_info = 1;
		return;
	}
	if (di->ls_ops->is_ls_close()) {
		hwlog_err("%s: ls is close\n", __func__);
		di->wldc_stop_flag_warning = 1;
		return;
	}
}
static void wldc_select_charge_param(struct wldc_device_info *di)
{
	di->cur_vbat_th = di->volt_para[di->cur_stage/2].vbatt_th;
	di->cur_iout_th_high = di->volt_para[di->cur_stage/2].iout_th_high > di->sysfs_data.iin_thermal ?
		di->sysfs_data.iin_thermal : di->volt_para[di->cur_stage/2].iout_th_high;
	di->cur_iout_th_low = di->volt_para[di->cur_stage/2].iout_th_low;
}
void wldc_stop_charging(struct wldc_device_info *di)
{
	int ret;
	int vout;

	hwlog_info("%s: ++\n", __func__);
	if (di->wldc_stop_flag_error) {
		di->error_cnt += 1;
	}
	if (di->wldc_stop_flag_warning) {
		di->warning_cnt += 1;
	}
	if (WIRED_CHANNEL_ON == wireless_charge_get_wired_channel_state() ||
		di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
		wldc_set_charge_stage(WLDC_STAGE_STOP_CHARGING);
	} else if (di->wldc_stop_flag_error || di->wldc_stop_flag_info ||
		di->wldc_stop_flag_warning || !di->sysfs_data.enable_charger) {
		wldc_set_charge_stage(WLDC_STAGE_DEFAULT);
	} else {
		wldc_set_charge_stage(WLDC_STAGE_CHARGE_DONE);
	}
	ret = di->ls_ops->ls_enable(0);
	if (ret) {
		hwlog_err("%s: ls enable fail!\n", __func__);
	}
	msleep(50);  //only used here
	ret = wldc_retore_normal_charge_path();
	if (ret) {
		hwlog_err("%s: retore_normal_charge fail!\n", __func__);
	}
	ret = wired_chsw_set_wired_reverse_channel(WIRED_REVERSE_CHANNEL_CUTOFF);
	if (ret) {
		hwlog_err("%s: close wired_reverse_channel fail!\n", __func__);
	}
	cancel_delayed_work_sync(&di->wldc_calc_work);
	if (di->wldc_stage != WLDC_STAGE_STOP_CHARGING) {
		vout = wireless_charge_get_rx_vout();
		if (vout >=  RX_HIGH_VOUT) {
			ret = wireless_charge_set_rx_vout(di->rx_init_vout);
			if (ret) {
				hwlog_err("%s: set tx/rx vout fail\n", __func__);
			}
		}
		wireless_charge_restart_charging(WIRELESS_STAGE_REGULATION);
	}
	ret = di->ls_ops->ls_exit();
	if (ret) {
		hwlog_err("[%s]: ls exit fail!\n", __func__);
	}
	ret = di->bi_ops->exit();
	if (ret) {
		hwlog_err("[%s]: bi exit fail!\n", __func__);
	}
#ifdef CONFIG_TCPC_CLASS
	pd_dpm_ignore_vbus_only_event(false);
#endif
#ifdef CONFIG_BOOST_5V
	boost_5v_enable(BOOST_5V_DISABLE, BOOST_CTRL_WLDC);
#endif
	di->wldc_stop_flag_error = 0;
	di->wldc_stop_flag_warning = 0;
	di->wldc_stop_flag_info = 0;
	di->cur_stage = 0;
	di->wldc_stop_charging_complete_flag = true;
	hwlog_info("%s: --\n", __func__);
}
void wldc_update_basp_para(struct wldc_device_info *di)
{
	int ret, i, cur_level;
	static int last_level = BASP_PARA_LEVEL;
	AGING_SAFE_POLICY_TYPE basp = {0};

	if (!di) {
		hwlog_err("%s direct_charge_device is NULL\n", __func__);
		return ;
	}

	ret = hisi_battery_aging_safe_policy(&basp);
	if (ret) {
		hwlog_err("%s: get basp policy fail!\n", __func__);
		return;
	}

	cur_level = basp.level;
	if (cur_level != last_level && di->stage_size >= 1) {
		/*vout policy*/
		di->volt_para[di->stage_size -1].vbatt_th =
			di->orig_volt_para[di->stage_size -1].vbatt_th - basp.dc_volt_dec;
		for (i = 0; i < di->stage_size -1; i++) {
			di->volt_para[i].vbatt_th = di->orig_volt_para[i].vbatt_th < di->volt_para[di->stage_size -1].vbatt_th\
								? di->orig_volt_para[i].vbatt_th : di->volt_para[di->stage_size -1].vbatt_th;
		}
		/*iout policy*/
		di->volt_para[di->stage_size -1].iout_th_high =
						di->orig_volt_para[di->stage_size -1].iout_th_high * basp.cur_ratio/BASP_PARA_SCALE;
		di->volt_para[di->stage_size -1].iout_th_low =
						di->orig_volt_para[di->stage_size -1].iout_th_low;
		for (i = 0; i < di->stage_size -1; i++) {
			di->volt_para[i].iout_th_high = di->orig_volt_para[i].iout_th_high * basp.cur_ratio /BASP_PARA_SCALE;
			di->volt_para[i].iout_th_low = di->orig_volt_para[i].iout_th_low * basp.cur_ratio /BASP_PARA_SCALE;
		}

		last_level = cur_level;
		hwlog_info(BASP_TAG"cur_level = %d\n", cur_level);
		for (i = 0; i < di->stage_size; i++) {
			hwlog_info(BASP_TAG"volt_para[%d], vbatt_th:%d, iout_th_high:%d, cur_th_low:%d\n",
				i, di->volt_para[i].vbatt_th, di->volt_para[i].iout_th_high, di->volt_para[i].iout_th_low);
		}
	}
}
void wldc_start_charging(struct wldc_device_info *di)
{
	wldc_set_charge_stage(WLDC_STAGE_CHARGING);
	mod_delayed_work(system_wq, &di->wldc_ctrl_work, msecs_to_jiffies(di->ctrl_interval));
	mod_delayed_work(system_wq, &di->wldc_calc_work, msecs_to_jiffies(di->calc_interval));
}
static bool wldc_need_stop_work(struct wldc_device_info *di)
{
	if (di->wldc_stop_flag_error || di->wldc_stop_flag_info ||
		di->wldc_stop_flag_warning || !di->sysfs_data.enable_charger) {
		hwlog_info("%s: stop charging!\n", __func__);
		return true;
	}
	return false;
}
void wldc_calc_work(struct work_struct *work)
{
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	if (wldc_need_stop_work(di)) {
		hwlog_err("%s: stop calc work!\n", __func__);
		return;
	}
	wldc_check_exit_condition(di);
	wldc_update_basp_para(di);
	wldc_select_charge_stage(di);
	wldc_select_charge_param(di);
	if (2*di->stage_size == di->cur_stage) {
		hwlog_info("%s: direct charge done, stop threshold_calculation!\n", __func__);
		return;
	}
	if (di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
		hwlog_info("%s: tx disconnect, stop threshold_calculation!\n", __func__);
		return;
	}
	mod_delayed_work(system_wq, &di->wldc_calc_work, msecs_to_jiffies(di->calc_interval));
}
void wldc_control_work(struct work_struct *work)
{
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}

	if (wldc_need_stop_work(di)) {
		wldc_stop_charging(di);
		return;
	}
	if (di->wldc_stage == WLDC_STAGE_STOP_CHARGING) {
		hwlog_info("%s: tx disconnect, stop direct_charge!\n", __func__);
		di->wldc_stop_flag_info = 1;
		wldc_stop_charging(di);
		return;
	}
	if (2*di->stage_size == di->cur_stage) {
		hwlog_info("%s: wireless direct charge done!\n", __func__);
		wldc_stop_charging(di);
		return;
	}

	wldc_charge_regulation(di);

	mod_delayed_work(system_wq, &di->wldc_ctrl_work, msecs_to_jiffies(di->ctrl_interval));
}
void wldc_tx_disconnect_handler(void)
{
	struct wldc_device_info *di = g_wldc_di;
	if (!di) {
		hwlog_debug("%s: di null\n", __func__);
		return;
	}
	wldc_set_charge_stage(WLDC_STAGE_STOP_CHARGING);
	di->wldc_err_report_flag = FALSE;
	di->error_cnt = 0;
	di->warning_cnt = 0;
	di->dc_open_retry_cnt = 0;
	di->cur_stage = 0;
}
static int wldc_parse_volt_para(struct device_node* np, struct wldc_device_info* di)
{
	int ret = 0;
	int i = 0;
	int array_len = 0;
	u32 temp_para[WLDC_PARA_TOTAL * WLDC_VOLT_LEVEL] = {0};

	array_len = of_property_count_u32_elems(np, "volt_para");
	if ((array_len <= 0) || (array_len % WLDC_PARA_TOTAL != 0)) {
		di->stage_size = 0;
		hwlog_err("%s: para is invaild, please check!\n", __func__);
		return -1;
	} else if (array_len > WLDC_VOLT_LEVEL * WLDC_VOLT_LEVEL) {
		di->stage_size = 0;
		hwlog_err("%s: para is too long, please check!!\n", __func__);
		return -1;
	} else {
		ret = of_property_read_u32_array(np, "volt_para", temp_para, array_len);
		if (ret) {
			di->stage_size = 0;
			hwlog_err("%s: get para fail!\n", __func__);
			return -1;
		} else {
			di->stage_size = array_len / WLDC_PARA_TOTAL;
			for (i = 0; i < di->stage_size; i++) {
				di->orig_volt_para[i].vbatt_th = temp_para[(int)(WLDC_PARA_VBATT_TH + WLDC_PARA_TOTAL * i)];
				di->orig_volt_para[i].iout_th_high = temp_para[(int)(WLDC_PARA_IOUT_TH_HIGH + WLDC_PARA_TOTAL * i)];
				di->orig_volt_para[i].iout_th_low = temp_para[(int)(WLDC_PARA_IOUT_TH_LOW + WLDC_PARA_TOTAL * i)];
				hwlog_info("orig_volt_para[%d], vbatt_th: %d iout_th_high: %d iout_th_low: %d\n",
					i, di->orig_volt_para[i].vbatt_th, di->orig_volt_para[i].iout_th_high, di->orig_volt_para[i].iout_th_low);
			}
		}
	}
	return 0;
}

int wldc_parse_dts(struct device_node *np, struct wldc_device_info *di)
{
	int ret;
	ret = of_property_read_u32(np, "vbatt_max", &di->vbatt_max);
	if (ret) {
		hwlog_err("%s: get vbatt_max failed, use default config!!\n", __func__);
		di->vbatt_max = WLDC_DEFAULT_VBATT_MAX;
	}
	hwlog_info("%s: vbatt_max  = %dmV\n", __func__, di->vbatt_max);
	ret = of_property_read_u32(np, "vbatt_min", &di->vbatt_min);
	if (ret) {
		hwlog_err("%s: get vbatt_min failed, use default config!!\n", __func__);
		di->vbatt_min = WLDC_DEFAULT_VBATT_MIN;
	}
	hwlog_info("%s: vbatt_min  = %dmV\n", __func__, di->vbatt_min);
	ret = of_property_read_u32(np, "rx_init_vout", &di->rx_init_vout);
	if (ret) {
		hwlog_err("%s: get rx_init_vout failed, use default config!!\n", __func__);
		di->rx_init_vout = ADAPTER_9V*MVOLT_PER_VOLT;
	}
	hwlog_info("%s: rx_init_vout  = %dmV\n", __func__, di->rx_init_vout);
	ret = of_property_read_u32(np, "rx_vout_err_th", &di->rx_vout_err_th);
	if (ret) {
		hwlog_err("%s: get rx_vout_err_th failed, use default config!!\n", __func__);
		di->rx_vout_err_th = WLDC_DEFAULT_VOUT_ERR_TH;
	}
	hwlog_info("%s: rx_vout_err_th  = %dmV\n", __func__, di->rx_vout_err_th);
	ret = of_property_read_u32(np, "rx_iout_err_th", &di->rx_iout_err_th);
	if (ret) {
		hwlog_err("%s: get rx_iout_err_th failed, use default config!!\n", __func__);
		di->rx_iout_err_th = WLDC_DEFAULT_IOUT_ERR_TH;
	}
	hwlog_info("%s: rx_iout_err_th  = %dmA\n", __func__, di->rx_iout_err_th);
	ret = of_property_read_u32(np, "rx_init_delt_vout", &di->rx_init_delt_vout);
	if (ret) {
		hwlog_err("%s: get rx_init_delt_vout failed, use default config!!\n", __func__);
		di->rx_init_delt_vout = WLDC_INIT_DELT_VOUT;
	}
	hwlog_info("%s: rx_init_delt_vout  = %dmV\n", __func__, di->rx_init_delt_vout);
	ret = of_property_read_u32(np, "volt_ratio", &di->volt_ratio);
	if (ret) {
		hwlog_err("%s: get sc_volt_ratio failed, use default config!!\n", __func__);
		di->volt_ratio = SC_DEFAULT_VOLT_RATIO;
	}
	hwlog_info("%s: sc_volt_ratio  = %d\n", __func__, di->volt_ratio);
	ret = of_property_read_u32(np, "ctrl_interval", &di->ctrl_interval);
	if (ret) {
		hwlog_err("%s: get ctrl_interval failed, use default config!!\n", __func__);
		di->ctrl_interval = WLDC_DEFAULT_CTRL_INTERVAL;
	}
	hwlog_info("%s: ctrl_interval  = %dms\n", __func__, di->ctrl_interval);
	ret = of_property_read_u32(np, "calc_interval", &di->calc_interval);
	if (ret) {
		hwlog_err("%s: get calc_interval failed, use default config!!\n", __func__);
		di->calc_interval = WLDC_DEFAULT_CALC_INTERVAL;
	}
	hwlog_info("%s: calc_interval  = %dms\n", __func__, di->calc_interval);
	ret = of_property_read_u32(np, "vstep", &di->vstep);
	if (ret) {
		hwlog_err("%s: get vstep failed, use default config!!\n", __func__);
		di->vstep = WLDC_DEFAULT_VSTEP;
	}
	hwlog_info("%s: vstep  = %dmV\n", __func__, di->vstep);

	ret = wldc_parse_volt_para(np,di);
	if (ret) {
		hwlog_err("%s: parse_volt_para fail\n", __func__);
		return -1;
	}
	return 0;
}
