#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <huawei_platform/power/huawei_charger.h>
#include <huawei_platform/power/wireless_transmitter.h>
#include <huawei_platform/power/wired_channel_switch.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <linux/jiffies.h>
#include <../charging_core.h>
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif

#define HWLOG_TAG wireless_tx
HWLOG_REGIST();

static struct wake_lock wireless_tx_wakelock;
static struct wireless_tx_device_info *g_wireless_tx_di;
static struct wireless_tx_device_ops *g_wireless_tx_ops;
static enum wireless_tx_stage tx_stage = WL_TX_STAGE_DEFAULT;
static enum wireless_tx_status_type tx_status = WL_TX_STATUS_DEFAULT;
static char tx_stage_str[WL_TX_STAGE_TOTAL][WL_TX_STR_LEN_32] = {
	{"STAGE_DEFAULT"}, {"STAGE_POWER_SUPPLY"},{"STAGE_CHIP_INIT"},{"STAGE_PING_RX"}, {"STAGE_REGULATION"}
};
static unsigned int tx_iin_samples[WL_TX_IIN_SAMPLE_LEN];
static bool tx_open_flag = false; // record the UI operation state
static int tx_iin_limit[WI_TX_CHARGER_TYPE_MAX] = {0};
static int g_init_tbatt;

BLOCKING_NOTIFIER_HEAD(tx_event_nh);
int wireless_tx_ops_register(struct wireless_tx_device_ops *tx_ops)
{
	int ret = 0;

	if (tx_ops != NULL) {
		g_wireless_tx_ops = tx_ops;
	} else {
		hwlog_err("wireless tx ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}
int wireless_tx_get_tx_status(void)
{
	return tx_status;
}
bool wireless_tx_get_tx_open_flag(void)
{
	return tx_open_flag;
}
static void wireless_tx_wake_lock(void)
{
	if (!wake_lock_active(&wireless_tx_wakelock)) {
		wake_lock(&wireless_tx_wakelock);
		hwlog_info("wireless_tx wake lock\n");
	}
}
static void wireless_tx_wake_unlock(void)
{
	if (wake_lock_active(&wireless_tx_wakelock)) {
		wake_unlock(&wireless_tx_wakelock);
		hwlog_info("wireless_tx wake unlock\n");
	}
}
static void wireless_tx_set_tx_open_flag(bool enable)
{
	tx_open_flag = enable;
	hwlog_info("[%s] set tx_open_flag = %d\n", __func__, tx_open_flag);
#ifdef CONFIG_DIRECT_CHARGER
	if (enable == false) {
		set_direct_charger_disable_flags(DIRECT_CHARGER_CLEAR_DISABLE_FLAGS, DIRECT_CHARGER_WIRELESS_TX);
	}
#endif
}
static void wireless_tx_set_stage(enum wireless_tx_stage stage)
{
	tx_stage = stage;
	hwlog_info("[%s] %s\n",__func__, tx_stage_str[tx_stage]);
}
static enum wireless_tx_stage wireless_tx_get_stage(void)
{
	return tx_stage;
}
static void wireless_tx_set_tx_status(enum wireless_tx_status_type event)
{
	tx_status = event;
	hwlog_info("[%s] 0x%02x\n", __func__, tx_status);
}
int wireless_tx_get_tx_iin_limit(enum usb_charger_type charger_type)
{
	if (!tx_open_flag || wireless_tx_get_stage() == WL_TX_STAGE_DEFAULT
		|| charger_type >= WI_TX_CHARGER_TYPE_MAX || charger_type < 0)
		return 0;
	return tx_iin_limit[charger_type];
}
static void wireless_tx_calc_tx_iin_avg(struct wireless_tx_device_info *di, unsigned int tx_iin)
{
	static int index = 0;
	int iin_sum = 0;
	int i;

	tx_iin_samples[index] = tx_iin;
	index = (index + 1) % WL_TX_IIN_SAMPLE_LEN;
	for (i = 0; i < WL_TX_IIN_SAMPLE_LEN; i++) {
		iin_sum += tx_iin_samples[i];
	}
	di->tx_iin_avg = iin_sum/WL_TX_IIN_SAMPLE_LEN;
}
static void wireless_tx_reset_avg_iout(struct wireless_tx_device_info *di)
{
	int i;
	for (i = 0; i < WL_TX_IIN_SAMPLE_LEN; i++) {
		tx_iin_samples[i] = 0;
	}
	di->tx_iin_avg = 0;
}
static int wireless_tx_chip_reset(struct wireless_tx_device_info *di)
{
	return di->tx_ops->chip_reset();
}
static void wireless_tx_set_max_fop(struct wireless_tx_device_info *di, u16 fop)
{
	di->tx_ops->set_tx_max_fop(fop);
}
static void wireless_tx_get_max_fop(struct wireless_tx_device_info *di, u16 *fop)
{
	di->tx_ops->get_tx_max_fop(fop);
}
static void wireless_tx_set_min_fop(struct wireless_tx_device_info *di, u16 fop)
{
	di->tx_ops->set_tx_min_fop(fop);
}
static void wireless_tx_get_min_fop(struct wireless_tx_device_info *di, u16 *fop)
{
	di->tx_ops->get_tx_min_fop(fop);
}
static void wireless_tx_get_fop(struct wireless_tx_device_info *di, u16 *fop)
{
	di->tx_ops->get_tx_fop(fop);
}
static void wireless_tx_get_digital_ping_frequency(struct wireless_tx_device_info *di, u16 *dping_freq)
{
	di->tx_ops->get_tx_ping_frequency(dping_freq);
}
static void wireless_tx_set_digital_ping_frequency(struct wireless_tx_device_info *di, u16 dping_freq)
{
	di->tx_ops->set_tx_ping_frequency(dping_freq);
}
static void wireless_tx_get_digital_ping_interval(struct wireless_tx_device_info *di, u16 *dping_interval)
{
	di->tx_ops->get_tx_ping_interval(dping_interval);
}
static void wireless_tx_set_digital_ping_interval(struct wireless_tx_device_info *di, u16 dping_interval)
{
	di->tx_ops->set_tx_ping_interval(dping_interval);
}
static void wireless_tx_check_fwupdate(struct wireless_tx_device_info *di)
{
	di->tx_ops->check_fwupdate(WIRELESS_TX_MODE);
}
static void wireless_tx_dsm_dump(struct wireless_tx_device_info *di, char* dsm_buff)
{
	int ret, i;
	char buff[ERR_NO_STRING_SIZE] = {0};
	int tx_iin, tx_vin, tx_vrect, chip_temp = 0;
	int soc = hisi_battery_capacity();
	int tbatt = hisi_battery_temperature();
	int charger_vbus = charge_get_vbus();
	ret = di->tx_ops->get_tx_iin(&tx_iin);
	ret |= di->tx_ops->get_tx_vin(&tx_vin);
	ret |= di->tx_ops->get_tx_vrect(&tx_vrect);
	ret |= di->tx_ops->get_chip_temp(&chip_temp);
	if (ret) {
		hwlog_err("%s: get tx vin/iin/vrect... fail", __func__);
	}
	snprintf(buff, sizeof(buff),
		"soc = %d, tbatt = %d, init_tbatt = %d, charger_vbus = %dmV, tx_vrect = %dmV, tx_vin = %dmV, tx_iin = %dmA, tx_iin_avg = %dmA, chip_temp = %d\n",
		soc, tbatt, g_init_tbatt, charger_vbus, tx_vrect, tx_vin, tx_iin, di->tx_iin_avg, chip_temp);
	strncat(dsm_buff, buff, strlen(buff));
	snprintf(buff, ERR_NO_STRING_SIZE, "tx_iin(mA): ");
	strncat(dsm_buff, buff, strlen(buff));
	for (i = 0; i < WL_TX_IIN_SAMPLE_LEN; i++) {
		snprintf(buff, ERR_NO_STRING_SIZE, "%d ", tx_iin_samples[i]);
		strncat(dsm_buff, buff, strlen(buff));
	}
}

static void wireless_tx_dsm_report(int err_no, char* dsm_buff)
{
	struct wireless_tx_device_info *di = g_wireless_tx_di;
	if (di) {
		wireless_tx_dsm_dump(di, dsm_buff);
		power_dsm_dmd_report(POWER_DSM_BATTERY, err_no, dsm_buff);
	}
}

static void wireless_tx_check_rx_disconnect(struct wireless_tx_device_info *di)
{
	bool rx_disconnect = di->tx_ops->check_rx_disconnect();
	if (rx_disconnect) {
		hwlog_info("[%s] rx disconnect!\n",__func__);
		wireless_tx_set_tx_status(WL_TX_STATUS_RX_DISCONNECT);
		di->stop_reverse_charge = true;
	}
}
static void wireless_tx_check_in_tx_mode(struct wireless_tx_device_info *di)
{
	bool in_tx_mode = di->tx_ops->in_tx_mode();
	if (!in_tx_mode) {
		if (++di->tx_mode_err_cnt >= WL_TX_MODE_ERR_CNT) {
			hwlog_info("[%s] not in tx mode!\n",__func__);
			wireless_tx_set_tx_status(WL_TX_STATUS_TX_CLOSE);
			di->stop_reverse_charge = true;
		}
	} else {
		di->tx_mode_err_cnt = 0;
	}
}
static void wireless_tx_en_enable(struct wireless_tx_device_info *di, bool enable)
{
	di->tx_ops->rx_enable(enable);
}
static void wireless_tx_enable_tx_mode(struct wireless_tx_device_info *di, bool enable)
{
	if (WIRED_CHANNEL_OFF == wireless_charge_get_wired_channel_state()) {
		di->tx_ops->enable_tx_mode(enable);
		hwlog_info("[%s] enable = %d, wired channel state:off\n",__func__, enable);
	} else {
		//If enable equals false in wired charging, chip should be closed.
		wireless_tx_en_enable(di, enable);
		if (enable) {
			di->tx_ops->enable_tx_mode(enable);
		}
		hwlog_info("[%s] enable = %d, wired channel state:on\n",__func__, enable);
	}
}
static void wireless_tx_enable_power(bool enable)
{
	/* enable OTG or set vbus to 5V/9V */
	if (WIRED_CHANNEL_CUTOFF == wired_chsw_get_wired_channel()) {
		charge_otg_mode_enable(enable, OTG_CTRL_WIRELESS_TX);
	} else {
		if (enable) {
			charge_set_adapter_voltage(ADAPTER_5V, RESET_ADAPTER_WIRELESS_TX, 0);
		} else {
			charge_set_adapter_voltage(ADAPTER_9V, RESET_ADAPTER_WIRELESS_TX, 0);
		}
	}
}
static int wireless_tx_power_supply(struct wireless_tx_device_info *di)
{
	int count = 0;
	int tx_vin = 0;
	char dsm_buff[CHARGE_DMDLOG_SIZE] = {0};

	do {
		msleep(WL_TX_VIN_SLEEP_TIME);
		if (!tx_open_flag || di->stop_reverse_charge) {
			hwlog_err("%s: Tx mode has already stop! tx_open_flag = %d, stop_reverse_charge_flag = %d\n",
				__func__ , tx_open_flag, di->stop_reverse_charge);
			return WL_TX_FAIL;
		}
	} while (++count < WL_TX_VIN_RETRY_CNT);
	count = 0;
	wireless_tx_enable_power(true);
	do {
		tx_vin = charge_get_vbus();
		if(tx_vin >= WL_TX_VIN_MIN && tx_vin <= WL_TX_VIN_MAX) {
			hwlog_info("[%s] tx_vin = %dmV, power supply succ!\n", __func__, tx_vin);
			return WL_TX_SUCC;
		}
		msleep(WL_TX_VIN_SLEEP_TIME);
		if (!tx_open_flag || di->stop_reverse_charge) {
			hwlog_err("%s: Tx mode has already stop! tx_open_flag = %d, stop_reverse_charge_flag = %d\n",
				__func__ , tx_open_flag, di->stop_reverse_charge);
			return WL_TX_FAIL;
		}
		count++;
		hwlog_info("[%s] tx_vin = %dmV, retry times = %d!\n", __func__, tx_vin, count);
	} while (count < WL_TX_VIN_RETRY_CNT);

	hwlog_err("%s: power supply for TX fail!\n", __func__);
	wireless_tx_dsm_report(ERROR_WIRELESS_TX_POWER_SUPPLY_FAIL, dsm_buff);
	return WL_TX_FAIL;
}

static void wireless_tx_para_init(struct wireless_tx_device_info *di)
{
	di->stop_reverse_charge = false;
	di->i2c_err_cnt = 0;
	di->tx_iin_low_cnt = 0;
	di->tx_mode_err_cnt = 0;
	di->standard_rx = false;
	di->monitor_interval = WL_TX_MONITOR_INTERVAL;
	wireless_tx_reset_avg_iout(di);
}
static int wireless_tx_chip_init(struct wireless_tx_device_info *di)
{
	int ret = 0;
	if (!tx_open_flag || di->stop_reverse_charge) {
		hwlog_err("%s: Tx mode has already stop! tx_open_flag = %d, stop_reverse_charge_flag = %d\n",
				__func__ , tx_open_flag, di->stop_reverse_charge);
		return WL_TX_FAIL;
	}

	wireless_tx_en_enable(di, true);
	wireless_tx_check_fwupdate(di);
	ret = di->tx_ops->tx_chip_init();
	if (ret) {
		hwlog_err("%s: TX chip init fail!\n", __func__);
		return WL_TX_FAIL;
	}
	hwlog_info("%s: TX chip init succ!\n", __func__);
	return WL_TX_SUCC;
}
static int wireless_tx_ping_rx(struct wireless_tx_device_info *di)
{
	u16 tx_vin = 0;
	int ret;
	bool tx_vin_uvp_flag = false;
	int tx_vin_uvp_cnt = 0;
	int tx_vin_ovp_cnt = 0;
	struct timespec64 ts64_timeout;
	struct timespec64 ts64_interval;
	struct timespec64 ts64_now;
	ts64_now = current_kernel_time64();
	ts64_interval.tv_sec = di->ping_timeout;
	ts64_interval.tv_nsec = 0;
	char dsm_buff[CHARGE_DMDLOG_SIZE] = {0};

	if (di->ping_timeout == WL_TX_PING_TIMEOUT_2) {
		ret = wireless_tx_chip_reset(di);
		if (ret) {
			hwlog_err("%s: chip_reset fail\n", __func__);
		}
		msleep(150);  //only used here
		ret = wireless_tx_chip_init(di);
		if (ret) {
			hwlog_err("%s: chip_init fail\n", __func__);
		}
	}

	wireless_tx_enable_tx_mode(di, true);

	ts64_timeout = timespec64_add_safe(ts64_now, ts64_interval);
	if (ts64_timeout.tv_sec == TIME_T_MAX) {
		hwlog_err("%s: time overflow happend, TX ping RX fail!\n", __func__);
		return WL_TX_FAIL;
	}

	while (timespec64_compare(&ts64_now, &ts64_timeout) < 0) {
		/*wait for config packet interrupt */
		if (WL_TX_STATUS_PING_SUCC == wireless_tx_get_tx_status()) {
			return WL_TX_SUCC;
		}
		ret = di->tx_ops->get_tx_vin(&tx_vin);
		if (ret) {
			hwlog_err("[%s] get tx_vin fail\n", __func__);
			tx_vin = 0;
		} else if (tx_vin < WL_TX_VIN_MIN || tx_vin >= WL_TX_VIN_MAX) {
			hwlog_err("%s: tx_vin = %umV\n", __func__, tx_vin);
		}
		/**to solve the problem of tx reset when power_supply ocp/scp
		  * in case of putting tx on the metal or something like this
		  */
		if (tx_vin < WL_TX_VIN_MIN) {
			tx_vin_uvp_flag = true;
		} else if (tx_vin >= WL_TX_VIN_MIN) {
			if (tx_vin_uvp_flag && ++tx_vin_uvp_cnt <= WL_TX_PING_VIN_UVP_CNT) {
				hwlog_err("%s: tx vin uvp cnt = %d\n", __func__, tx_vin_uvp_cnt);
				ret = wireless_tx_chip_init(di);
				if (ret) {
					hwlog_err("%s: tx_chip_init fail\n", __func__);
					wireless_tx_set_tx_status(WL_TX_STATUS_TX_CLOSE);
					return WL_TX_FAIL;
				}
				wireless_tx_enable_tx_mode(di, true);
			}
			tx_vin_uvp_flag = false;
		}
		if (tx_vin_uvp_cnt >= WL_TX_PING_VIN_UVP_CNT) {
			hwlog_err("%s: tx_vin_uvp over %d times, tx ping fail\n", __func__, WL_TX_PING_VIN_UVP_CNT);
			wireless_tx_set_tx_status(WL_TX_STATUS_TX_CLOSE);
			return WL_TX_FAIL;
		}

		/*to enter RX mode when put TRX(phone) on TX in case of TRX TX mode on*/
		if (WIRED_CHANNEL_CUTOFF == wired_chsw_get_wired_channel() && //power supply by otg
			tx_vin >= WL_TX_VIN_MAX) {
			if (++tx_vin_ovp_cnt >= WL_TX_PING_VIN_OVP_CNT) {
				hwlog_err("%s: tx_vin over %dmV for %dms, stop TX power supply to reset TRX chip\n",
					__func__, WL_TX_VIN_MAX, tx_vin_ovp_cnt*WL_TX_PING_CHECK_INTERVAL);
				wireless_tx_enable_power(false);
				wireless_tx_set_tx_status(WL_TX_STATUS_TX_CLOSE);
				return WL_TX_FAIL;
			}
		} else {
			tx_vin_ovp_cnt = 0;
		}
		msleep(WL_TX_PING_CHECK_INTERVAL);
		if (WIRED_CHANNEL_OFF == wireless_charge_get_wired_channel_state()) {
			ts64_now = current_kernel_time64();
		}

		if (!tx_open_flag || di->stop_reverse_charge) {
			hwlog_err("%s: Tx mode has already stop! tx_open_flag = %d, stop_reverse_charge_flag = %d\n",
				__func__ , tx_open_flag, di->stop_reverse_charge);
			return WL_TX_FAIL;
		}
	}
	wireless_tx_set_tx_open_flag(false);
	wireless_tx_set_tx_status(WL_TX_STATUS_PING_TIMEOUT);
	hwlog_err("[%s] TX ping RX timeout!!\n",__func__);
	if (di->ping_timeout == WL_TX_PING_TIMEOUT_1) {
		snprintf(dsm_buff, sizeof(dsm_buff), "TX ping RX timeout\n");
		power_dsm_dmd_report(POWER_DSM_BATTERY, ERROR_WIRELESS_TX_STATISTICS, dsm_buff);
	}
	return WL_TX_FAIL;
}
static int wireless_tx_can_do_reverse_charging(void)
{
	int batt_temp = hisi_battery_temperature();
	int soc = hisi_bci_show_capacity();
	char dsm_buff[CHARGE_DMDLOG_SIZE] = {0};

	if (batt_temp <= WL_TX_BATT_TEMP_MIN) {
		hwlog_info("[%s] battery temperature(%d) is too low(th: %d)\n",
			__func__, batt_temp, WL_TX_BATT_TEMP_MIN);
		wireless_tx_set_tx_status(WL_TX_STATUS_TBATT_LOW);
		return WL_TX_FAIL;
	}
	if (batt_temp >= WL_TX_BATT_TEMP_MAX) {
		hwlog_info("[%s] battery temperature(%d) is too high(th: %d)\n",
			__func__, batt_temp, WL_TX_BATT_TEMP_MAX);
		wireless_tx_set_tx_status(WL_TX_STATUS_TBATT_HIGH);
		if (batt_temp - g_init_tbatt > WL_TX_TBATT_DELTA_TH) {
			wireless_tx_dsm_report(ERROR_WIRELESS_TX_BATTERY_OVERHEAT, dsm_buff);
		}
		return WL_TX_FAIL;
	}
	if (soc <= WL_TX_SOC_MIN &&
		WIRED_CHANNEL_OFF == wireless_charge_get_wired_channel_state()) {
		hwlog_info("[%s] capacity is out of range\n",__func__);
		wireless_tx_set_tx_status(WL_TX_STATUS_SOC_ERROR);
		return WL_TX_FAIL;
	}

	if (wireless_charge_check_tx_exist()) {
		hwlog_info("[%s] in wireless charging, can not enable tx mode\n",__func__);
		wireless_tx_set_tx_status(WL_TX_STATUS_IN_WL_CHARGING);
		return WL_TX_FAIL;
	}
	return WL_TX_SUCC;
}
static void wireless_tx_fault_event_handler(struct wireless_tx_device_info *di)
{
	wireless_tx_wake_lock();
	hisi_coul_charger_event_rcv(WIRELESS_TX_STATUS_CHANGED);
	hwlog_info("[%s] tx_status = 0x%02x\n", __func__, tx_status);

	switch(tx_status) {
		case WL_TX_STATUS_RX_DISCONNECT:
			di->ping_timeout = WL_TX_PING_TIMEOUT_2;
			wireless_tx_set_stage(WL_TX_STAGE_PING_RX);
			schedule_work(&di->wireless_tx_check_work);
			break;
		case WL_TX_STATUS_TX_CLOSE:
		case WL_TX_STATUS_SOC_ERROR:
		case WL_TX_STATUS_TBATT_HIGH:
		case WL_TX_STATUS_TBATT_LOW:
		case WL_TX_STATUS_CHARGE_DONE:
			wireless_tx_set_tx_open_flag(false);
			wireless_tx_enable_tx_mode(di, false);
			wireless_tx_enable_power(false);
			wireless_tx_wake_unlock();
			break;
		default:
			wireless_tx_wake_unlock();
			hwlog_err("%s: has no this tx_status(%d)\n", __func__, tx_status);
			break;
	}
}
void wireless_tx_cancel_work(void)
{
	struct wireless_tx_device_info *di = g_wireless_tx_di;
	if (di && tx_open_flag) {
		hwlog_info("[%s] -->start\n", __func__);
		di->stop_reverse_charge = true;
		cancel_work_sync(&di->wireless_tx_check_work);
		cancel_delayed_work_sync(&di->wireless_tx_monitor_work);
		wireless_tx_enable_tx_mode(di, false);
		wireless_tx_enable_power(false);
		hwlog_info("[%s] -->end\n", __func__);
	}
}
void wireless_tx_start_check(void)
{
	struct wireless_tx_device_info *di = g_wireless_tx_di;
	if (!di) {
		return;
	}
	if (tx_open_flag) {
		hwlog_info("[%s] -->begin\n", __func__);
		wireless_tx_set_stage(WL_TX_STAGE_DEFAULT);
		schedule_work(&di->wireless_tx_check_work);
	}
}
static void wireless_tx_iout_control(struct wireless_tx_device_info *di)
{
	int ret = 0;
	u16 tx_iin = 0;
	u16 tx_vin = 0;
	u16 tx_vrect = 0;
	u8 chip_temp = 0;
	static int log_cnt = 0;
	ret = di->tx_ops->get_tx_iin(&tx_iin);
	ret |= di->tx_ops->get_tx_vin(&tx_vin);
	ret |= di->tx_ops->get_tx_vrect(&tx_vrect);
	ret |= di->tx_ops->get_chip_temp(&chip_temp);
	if (ret) {
		di->i2c_err_cnt++;
		hwlog_err("%s: get tx vin/iin fail", __func__);
	}
	if (di->standard_rx == false && tx_iin <= WL_TX_IIN_LOW &&
		WIRED_CHANNEL_OFF == wireless_charge_get_wired_channel_state()) {
		if (++di->tx_iin_low_cnt >= WL_TX_IIN_LOW_CNT/di->monitor_interval) {
			di->tx_iin_low_cnt = WL_TX_IIN_LOW_CNT;
			wireless_tx_set_tx_status(WL_TX_STATUS_CHARGE_DONE);
			di->stop_reverse_charge = true;
			hwlog_info("[%s] tx_iin below for %ds, set tx_status to charge_done\n",
				__func__, WL_TX_IIN_LOW_CNT/MSEC_PER_SEC);
		}
	} else {
		di->tx_iin_low_cnt = 0;
	}
	wireless_tx_calc_tx_iin_avg(di, tx_iin);
	if (log_cnt++ == WL_TX_MONITOR_LOG_INTERVAL/di->monitor_interval) {
		hwlog_info("[%s] tx_iin_avg = %dmA, tx_iin = %dmA, tx_vin = %dmV, tx_vrect = %dmV, chip_temp = %d \n",
			__func__, di->tx_iin_avg, tx_iin, tx_vin, tx_vrect, chip_temp);
		log_cnt = 0;
	}
}
static void wireless_tx_monitor_work(struct work_struct *work)
{
	struct wireless_tx_device_info *di = g_wireless_tx_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}

	if (WL_TX_FAIL == wireless_tx_can_do_reverse_charging())
		goto FuncEnd;

	wireless_tx_check_rx_disconnect(di);
	wireless_tx_check_in_tx_mode(di);

	if (di->stop_reverse_charge) {
		hwlog_info("[%s] stop monitor work\n",__func__);
		goto FuncEnd;
	}

	if (!tx_open_flag || di->i2c_err_cnt > WL_TX_I2C_ERR_CNT) {
		hwlog_info("[%s] TX chip is fault or TX closed, stop monitor work\n",__func__);
		wireless_tx_set_tx_status(WL_TX_STATUS_TX_CLOSE);
		goto FuncEnd;
	}

	wireless_tx_iout_control(di);
	schedule_delayed_work(&di->wireless_tx_monitor_work, msecs_to_jiffies(di->monitor_interval));
	return;

FuncEnd:
	wireless_tx_set_stage(WL_TX_STAGE_DEFAULT);
	wireless_tx_fault_event_handler(di);
}
static void wireless_tx_start_check_work(struct work_struct *work)
{
	int ret = 0;
	struct wireless_tx_device_info *di = g_wireless_tx_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}

	wireless_tx_wake_lock();
	wireless_tx_para_init(di);
	g_init_tbatt = hisi_battery_temperature();

	if (WL_TX_STAGE_DEFAULT == wireless_tx_get_stage()) {
		ret = wireless_tx_can_do_reverse_charging();
		if (ret) {
			goto FuncEnd;
		}
		wireless_tx_set_stage(WL_TX_STAGE_POWER_SUPPLY);
	}
	if (WL_TX_STAGE_POWER_SUPPLY == wireless_tx_get_stage()) {
		ret = wireless_tx_power_supply(di);
		if (ret) {
			goto FuncEnd;
		}
		wireless_tx_set_stage(WL_TX_STAGE_CHIP_INIT);
	}
	if (WL_TX_STAGE_CHIP_INIT == wireless_tx_get_stage()) {
		ret = wireless_tx_chip_init(di);
		if (ret) {
			goto FuncEnd;
		}
		wireless_tx_set_stage(WL_TX_STAGE_PING_RX);
	}
	if (WL_TX_STAGE_PING_RX == wireless_tx_get_stage()) {
		ret = wireless_tx_ping_rx(di);
		if (ret) {
			goto FuncEnd;
		}
		wireless_tx_set_stage(WL_TX_STAGE_REGULATION);
	}

	hwlog_info("[%s] start wireless reverse charging!\n",__func__);
	wireless_tx_set_tx_status(WL_TX_STATUS_IN_CHARGING);
	hisi_coul_charger_event_rcv(WIRELESS_TX_STATUS_CHANGED);
	mod_delayed_work(system_wq, &di->wireless_tx_monitor_work, msecs_to_jiffies(0));
	return;

FuncEnd:
	hisi_coul_charger_event_rcv(WIRELESS_TX_STATUS_CHANGED);
	if (wireless_tx_get_stage() == WL_TX_STAGE_DEFAULT) {
		wireless_tx_set_tx_open_flag(false);
	}
	if (wireless_tx_get_stage() >= WL_TX_STAGE_PING_RX) {
		wireless_tx_enable_tx_mode(di, false);
	}
	if (wireless_tx_get_stage() >= WL_TX_STAGE_POWER_SUPPLY) {
		wireless_tx_enable_power(false);
	}
	wireless_tx_set_stage(WL_TX_STAGE_DEFAULT);
	wireless_tx_wake_unlock();
}
static void wireless_tx_event_work(struct work_struct *work)
{
	struct wireless_tx_device_info *di = g_wireless_tx_di;
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}

	switch (di->tx_event_type) {
		case WL_TX_EVENT_GET_CFG:
			//get configure packet, ping succ
			wireless_tx_set_tx_status(WL_TX_STATUS_PING_SUCC);
			break;
		case WL_TX_EVENT_HANDSHAKE_SUCC:
			//0x8866 handshake, security authentic succ
			di->standard_rx = true;
			break;
		case WL_TX_EVENT_CHARGEDONE:
			if (WIRED_CHANNEL_OFF == wireless_charge_get_wired_channel_state()) {
				wireless_tx_set_tx_status(WL_TX_STATUS_CHARGE_DONE);
				di->stop_reverse_charge = true;
			} else {
				hwlog_err("%s: wired vbus on, ignore rx charge done event\n", __func__);
			}
			break;
		case WL_TX_EVENT_CEP_TIMEOUT:
			wireless_tx_set_tx_status(WL_TX_STATUS_RX_DISCONNECT);
			di->stop_reverse_charge = true;
			break;
		case WL_TX_EVENT_EPT_CMD:
			wireless_tx_set_tx_status(WL_TX_STATUS_TX_CLOSE);
			di->stop_reverse_charge = true;
			break;
		case WL_TX_EVENT_OVP:
			wireless_tx_set_tx_status(WL_TX_STATUS_TX_CLOSE);
			di->stop_reverse_charge = true;
			break;
		case WL_TX_EVENT_OCP:
			wireless_tx_set_tx_status(WL_TX_STATUS_TX_CLOSE);
			di->stop_reverse_charge = true;
			break;
		default:
			hwlog_err("%s: has no this event_type(%d)\n", __func__, di->tx_event_type);
			break;
	}
}
static int wireless_tx_event_notifier_call(struct notifier_block *tx_event_nb, unsigned int event, void *data)
{
	struct wireless_tx_device_info *di =
	    container_of(tx_event_nb, struct wireless_tx_device_info, tx_event_nb);
	u8 *tx_notify_data = NULL;
	if (!di) {
		hwlog_err("%s: di NULL\n", __func__);
		return NOTIFY_OK;
	}
	if (data) {
		tx_notify_data = (u8 *)data;
		di->tx_event_data = *tx_notify_data;
	}
	di->tx_event_type = (enum wireless_tx_status_type)event;
	schedule_work(&di->wireless_tx_evt_work);
	return NOTIFY_OK;
}
static void wireless_tx_parse_dts(struct device_node *np, struct wireless_tx_device_info *di)
{
	int ret, i;
	// charger type : USB/BC_USB/NON_STANDARD/STANDARD/FCP/REMOVED/OTG_ID/VR/TYPEC/PD/SCP/WIRELESS
	ret = of_property_read_u32_array(np, "tx_iin_limit", tx_iin_limit, WI_TX_CHARGER_TYPE_MAX);
	if (ret) {
		hwlog_err("%s: get tx_iin_limit para fail!\n", __func__);
	}
	for (i = 0; i < WI_TX_CHARGER_TYPE_MAX; i++) {
		hwlog_info("[%s] tx_iin_limit[%d] = %d\n", __func__, i, tx_iin_limit[i]);
	}
}
static int wireless_tx_ops_check(struct wireless_tx_device_info *di)
{
	int ret = 0;
	if ((di == NULL) || (di->tx_ops == NULL)
		|| (di->tx_ops->chip_reset == NULL)
		|| (di->tx_ops->rx_enable == NULL)
		|| (di->tx_ops->rx_sleep_enable == NULL)
		|| (di->tx_ops->enable_tx_mode == NULL)
		|| (di->tx_ops->tx_chip_init == NULL)
		|| (di->tx_ops->tx_stop_config == NULL)
		|| (di->tx_ops->check_fwupdate == NULL)
		|| (di->tx_ops->kick_watchdog == NULL)
		|| (di->tx_ops->get_tx_iin == NULL)
		|| (di->tx_ops->get_tx_vrect == NULL)
		|| (di->tx_ops->get_tx_vin == NULL)
		|| (di->tx_ops->get_chip_temp == NULL)
		|| (di->tx_ops->get_tx_fop == NULL)
		|| (di->tx_ops->set_tx_max_fop == NULL)
		|| (di->tx_ops->get_tx_max_fop == NULL)
		|| (di->tx_ops->set_tx_min_fop == NULL)
		|| (di->tx_ops->get_tx_min_fop == NULL)
		|| (di->tx_ops->set_tx_ping_frequency == NULL)
		|| (di->tx_ops->get_tx_ping_frequency == NULL)
		|| (di->tx_ops->set_tx_ping_interval == NULL)
		|| (di->tx_ops->get_tx_ping_interval == NULL)
		|| (di->tx_ops->check_rx_disconnect == NULL)
		|| (di->tx_ops->in_tx_mode == NULL))
	{
		hwlog_err("wireless_tx ops is NULL!\n");
		ret = -EINVAL;
	}
	return ret;
}

#ifdef CONFIG_SYSFS
#define WIRELESS_TX_SYSFS_FIELD(_name, n, m, store)	\
{					\
	.attr = __ATTR(_name, m, wireless_tx_sysfs_show, store),	\
	.name = WL_TX_SYSFS_##n,		\
}
#define WIRELESS_TX_SYSFS_FIELD_RW(_name, n)               \
	WIRELESS_TX_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, wireless_tx_sysfs_store)
#define WIRELESS_TX_SYSFS_FIELD_RO(_name, n)               \
	WIRELESS_TX_SYSFS_FIELD(_name, n, S_IRUGO, NULL)
static ssize_t wireless_tx_sysfs_show(struct device *dev,
				struct device_attribute *attr, char *buf);
static ssize_t wireless_tx_sysfs_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count);
struct wireless_tx_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};
static struct wireless_tx_sysfs_field_info wireless_tx_sysfs_field_tbl[] = {
	WIRELESS_TX_SYSFS_FIELD_RW(tx_open, TX_OPEN),
	WIRELESS_TX_SYSFS_FIELD_RO(tx_status, TX_STATUS),
	WIRELESS_TX_SYSFS_FIELD_RO(tx_iin_avg, TX_IIN_AVG),
	WIRELESS_TX_SYSFS_FIELD_RW(dping_freq, DPING_FREQ),
	WIRELESS_TX_SYSFS_FIELD_RW(dping_interval, DPING_INTERVAL),
	WIRELESS_TX_SYSFS_FIELD_RW(max_fop, MAX_FOP),
	WIRELESS_TX_SYSFS_FIELD_RW(min_fop, MIN_FOP),
	WIRELESS_TX_SYSFS_FIELD_RO(tx_fop, TX_FOP),
};
static struct attribute *wireless_tx_sysfs_attrs[ARRAY_SIZE(wireless_tx_sysfs_field_tbl) + 1];
static const struct attribute_group wireless_tx_sysfs_attr_group = {
	.attrs = wireless_tx_sysfs_attrs,
};
/**********************************************************
*  Function:       wireless_tx_sysfs_init_attrs
*  Discription:    initialize wireless_tx_sysfs_attrs[] for wireless_tx attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void wireless_tx_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(wireless_tx_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		wireless_tx_sysfs_attrs[i] = &wireless_tx_sysfs_field_tbl[i].attr.attr;

	wireless_tx_sysfs_attrs[limit] = NULL;
}
/**********************************************************
*  Function:       wireless_tx_sysfs_field_lookup
*  Discription:    get the current device_attribute from wireless_tx_sysfs_field_tbl by attr's name
*  Parameters:   name:evice attribute name
*  return value:  wireless_tx_sysfs_field_tbl[]
**********************************************************/
static struct wireless_tx_sysfs_field_info *wireless_tx_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(wireless_tx_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp(name, wireless_tx_sysfs_field_tbl[i].attr.attr.name, strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &wireless_tx_sysfs_field_tbl[i];
}
/**********************************************************
*  Function:       wireless_tx_sysfs_create_group
*  Discription:    create the wireless_tx device sysfs group
*  Parameters:   di:wireless_tx_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int wireless_tx_sysfs_create_group(struct wireless_tx_device_info *di)
{
	wireless_tx_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &wireless_tx_sysfs_attr_group);
}
/**********************************************************
*  Function:       wireless_tx_sysfs_remove_group
*  Discription:    remove the wireless_tx device sysfs group
*  Parameters:   di:wireless_tx_device_info
*  return value:  NULL
**********************************************************/
static void wireless_tx_sysfs_remove_group(struct wireless_tx_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &wireless_tx_sysfs_attr_group);
}
#else
static int wireless_tx_sysfs_create_group(struct wireless_tx_device_info *di)
{
	return 0;
}
static void wireless_tx_sysfs_remove_group(struct wireless_tx_device_info *di)
{
}
#endif

/**********************************************************
*  Function:       wireless_tx_create_sysfs
*  Discription:    create the wireless_tx device sysfs group
*  Parameters:   di:wireless_tx_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int wireless_tx_create_sysfs(struct wireless_tx_device_info *di)
{
	int ret = 0;
	struct class *power_class = NULL;

	ret = wireless_tx_sysfs_create_group(di);
	if (ret) {
		hwlog_err("%s: create sysfs entries failed!\n", __func__);
		return ret;
	}
	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL)
			charge_dev = device_create(power_class, NULL, 0, NULL, "charger");
		ret = sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "wireless_tx");
		if (ret) {
			hwlog_err("%s: create link to wireless_tx fail!\n", __func__);
			wireless_tx_sysfs_remove_group(di);
			return ret;
		}
	}
	return 0;
}
/**********************************************************
*  Function:       wireless_tx_sysfs_show
*  Discription:    show the value for all wireless tx nodes
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t wireless_tx_sysfs_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct wireless_tx_sysfs_field_info *info = NULL;
	struct wireless_tx_device_info *di = g_wireless_tx_di;
	u16 dping_freq = 0;
	u16 dping_interval = 0;
	u16 max_fop = 0;
	u16 min_fop = 0;
	u16 tx_fop = 0;

	info = wireless_tx_sysfs_field_lookup(attr->attr.name);
	if (!info ||!di)
		return -EINVAL;

	switch (info->name) {
	case WL_TX_SYSFS_TX_OPEN:
		return snprintf(buf, PAGE_SIZE, "%d\n", tx_open_flag);
	case WL_TX_SYSFS_TX_STATUS:
		return snprintf(buf, PAGE_SIZE, "%d\n", tx_status);
	case WL_TX_SYSFS_TX_IIN_AVG:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->tx_iin_avg);
	case WL_TX_SYSFS_DPING_FREQ:
		wireless_tx_get_digital_ping_frequency(di, &dping_freq);
		return snprintf(buf, PAGE_SIZE, "%d\n", dping_freq);
	case WL_TX_SYSFS_DPING_INTERVAL:
		wireless_tx_get_digital_ping_interval(di, &dping_interval);
		return snprintf(buf, PAGE_SIZE, "%d\n", dping_interval);
	case WL_TX_SYSFS_MAX_FOP:
		wireless_tx_get_max_fop(di, &max_fop);
		return snprintf(buf, PAGE_SIZE, "%d\n", max_fop);
	case WL_TX_SYSFS_MIN_FOP:
		wireless_tx_get_min_fop(di, &min_fop);
		return snprintf(buf, PAGE_SIZE, "%d\n", min_fop);
	case WL_TX_SYSFS_TX_FOP:
		wireless_tx_get_fop(di, &tx_fop);
		return snprintf(buf, PAGE_SIZE, "%d\n", tx_fop);
	default:
		hwlog_err("%s: NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return 0;
}
/**********************************************************
*  Function:       wireless_tx_sysfs_store
*  Discription:    set the value for all wireless tx nodes
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t wireless_tx_sysfs_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	struct wireless_tx_sysfs_field_info *info = NULL;
	struct wireless_tx_device_info *di = g_wireless_tx_di;
	long val = 0;

	info = wireless_tx_sysfs_field_lookup(attr->attr.name);
	if (!info ||!di)
		return -EINVAL;

	switch (info->name) {
	case WL_TX_SYSFS_TX_OPEN:
		if (strict_strtol(buf, 10, &val) < 0 || (val < 0) || (val > 1)) {
			hwlog_info("%s: val is not valid!\n", __func__);
			return -EINVAL;
		}
		if (tx_open_flag && val) {
			hwlog_info("[%s] tx mode has already open, ignore!", __func__);
			break;
		}
		wireless_tx_set_tx_open_flag(val);
		if (tx_open_flag) {
			di->ping_timeout = WL_TX_PING_TIMEOUT_1;
			wireless_tx_set_stage(WL_TX_STAGE_DEFAULT);
			schedule_work(&di->wireless_tx_check_work);
		}
		wireless_tx_set_tx_status(WL_TX_STATUS_DEFAULT);
		break;
	case WL_TX_SYSFS_DPING_FREQ:
		if (strict_strtol(buf, 10, &val) < 0) {
			hwlog_info("%s: val is not valid!\n", __func__);
			return -EINVAL;
		}
		wireless_tx_set_digital_ping_frequency(di, val);
		break;
	case WL_TX_SYSFS_DPING_INTERVAL:
		if (strict_strtol(buf, 10, &val) < 0) {
			hwlog_info("%s: val is not valid!\n", __func__);
			return -EINVAL;
		}
		wireless_tx_set_digital_ping_interval(di, val);
		break;
	case WL_TX_SYSFS_MAX_FOP:
		if (strict_strtol(buf, 10, &val) < 0) {
			hwlog_info("%s: val is not valid!\n", __func__);
			return -EINVAL;
		}
		wireless_tx_set_max_fop(di, val);
		break;
	case WL_TX_SYSFS_MIN_FOP:
		if (strict_strtol(buf, 10, &val) < 0) {
			hwlog_info("%s: val is not valid!\n", __func__);
			return -EINVAL;
		}
		wireless_tx_set_min_fop(di, val);
		break;
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return count;
}
static struct wireless_tx_device_info *wireless_tx_device_info_alloc(void)
{
	static struct wireless_tx_device_info *di;
	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("%s: di alloc failed\n", __func__);
	}
	return di;
}
static void wireless_tx_shutdown(struct platform_device *pdev)
{
	struct wireless_tx_device_info *di = platform_get_drvdata(pdev);
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return;
	}
	hwlog_info("[%s]\n", __func__);
	cancel_delayed_work(&di->wireless_tx_monitor_work);
}
static int wireless_tx_remove(struct platform_device *pdev)
{
	struct wireless_tx_device_info *di = platform_get_drvdata(pdev);
	if (!di) {
		hwlog_err("%s: di null\n", __func__);
		return 0;
	}

	wake_lock_destroy(&wireless_tx_wakelock);

	hwlog_info("[%s]\n", __func__);
	return 0;
}
static int wireless_tx_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct wireless_tx_device_info *di = NULL;
	struct device_node *np = NULL;
	di = wireless_tx_device_info_alloc();
	if (!di) {
		hwlog_err("%s:di alloc failed\n",__func__);
		return -ENOMEM;
	}

	g_wireless_tx_di = di;
	di->dev = &pdev->dev;
	np = di->dev->of_node;
	di->tx_ops = g_wireless_tx_ops;

	wake_lock_init(&wireless_tx_wakelock, WAKE_LOCK_SUSPEND, "wireless_tx_wakelock");

	ret = wireless_tx_ops_check(di);
	if (ret) {
		hwlog_err("%s: tx ops null\n", __func__);
		goto wireless_tx_fail_0;
	}
	ret = wireless_tx_create_sysfs(di);
	if (ret) {
		hwlog_err("%s: sysfs creat failed\n", __func__);
		goto wireless_tx_fail_0;
	}
	wireless_tx_parse_dts(np, di);
	INIT_WORK(&di->wireless_tx_check_work, wireless_tx_start_check_work);
	INIT_WORK(&di->wireless_tx_evt_work, wireless_tx_event_work);
	INIT_DELAYED_WORK(&di->wireless_tx_monitor_work, wireless_tx_monitor_work);

	di->tx_event_nb.notifier_call = wireless_tx_event_notifier_call;
	ret = blocking_notifier_chain_register(&tx_event_nh, &di->tx_event_nb);
	if (ret < 0) {
		hwlog_err("register rx_connect notifier failed\n");
		goto  wireless_tx_fail_0;
	}


	hwlog_info("wireless_tx probe ok.\n");
	return 0;

wireless_tx_fail_0:
	wake_lock_destroy(&wireless_tx_wakelock);
	di->tx_ops = NULL;
	kfree(di);
	di = NULL;
	g_wireless_tx_di = NULL;
	np = NULL;
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static struct of_device_id wireless_tx_match_table[] = {
	{
	 .compatible = "huawei,wireless_tx",
	 .data = NULL,
	},
	{},
};

static struct platform_driver wireless_tx_driver = {
	.probe = wireless_tx_probe,
	.remove = wireless_tx_remove,
	.shutdown = wireless_tx_shutdown,
	.driver = {
		.name = "huawei,wireless_tx",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wireless_tx_match_table),
	},
};
/**********************************************************
*  Function:       wireless_tx_init
*  Description:    wireless tx module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init wireless_tx_init(void)
{
	hwlog_info("wireless_tx init ok.\n");

	return platform_driver_register(&wireless_tx_driver);
}
/**********************************************************
*  Function:       wireless_tx_exit
*  Description:    wireless tx module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit wireless_tx_exit(void)
{
	platform_driver_unregister(&wireless_tx_driver);
}

device_initcall_sync(wireless_tx_init);
module_exit(wireless_tx_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("wireless tx module driver");
MODULE_AUTHOR("HUAWEI Inc");
