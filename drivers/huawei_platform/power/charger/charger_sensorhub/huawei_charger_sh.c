/*
 * drivers/power/huawei_charger_sh.c
 *
 *huawei charger sensorhub driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wakelock.h>
#include <linux/usb/otg.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/usb/switch/switch_ap/switch_usb_class.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <linux/raid/pq.h>
#ifdef CONFIG_HISI_BCI_BATTERY
#include <linux/power/hisi/hisi_bci_battery.h>
#endif
#include <huawei_platform/power/huawei_charger_sh.h>
#ifdef CONFIG_HISI_COUL
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#endif
#include <huawei_platform/power/charging_core_sh.h>
#include <linux/workqueue.h>
#include <inputhub_route.h>
#include <protocol.h>
#include <inputhub_bridge.h>
#include <sensor_info.h>

struct charge_sysfs_lock sysfs_lock;
struct mutex charge_usb_notify_lock;
extern struct charge_device_ops *g_ops;
static struct device huawei_charger_dev;
struct charge_device_info_sh *g_di_sh;

#define CHARGER_BASE_ADDR_SH 512
#define REG_NUM_SH 21
#define TIMEOUT_SEC 5
struct hisi_charger_bootloader_info {
	bool info_vaild;
	int ibus;
	char reg[REG_NUM_SH];
};

static char reg_value[CHARGELOG_SIZE];
static char reg_head[CHARGELOG_SIZE];
static char bootloader_info[CHARGELOG_SIZE];
static int ico_enable;
static struct hisi_charger_bootloader_info hisi_charger_info = { 0 };
static struct wake_lock charge_lock;
static struct wake_lock wlock;
static enum charging_stat_t charging_status = CHARGING_CLOSE;

extern struct CONFIG_ON_DDR *pConfigOnDDr;
extern int g_iom3_state;
extern atomic_t iom3_rec_state;
extern sys_status_t iom3_sr_status;
extern struct charge_core_info_sh *g_core_info_sh;
extern struct charger_platform_data charger_dts_data;
extern struct fcp_adapter_device_ops sh_fcp_hi6523_ops;
extern struct fcp_adapter_device_ops sh_fcp_fsa9688_ops;

int notify_sensorhub(uint8_t stype, uint8_t rw);
static int charge_send_notify_to_sensorhub(struct charge_device_info_sh *di);
static void fcp_reg_dump(char* pstr);
extern char *get_charger_info_p(void);
extern int coul_get_battery_capacity(void);
extern int coul_get_battery_fcc (void);
extern unsigned int get_pd_charge_flag(void);
extern int coul_battery_unfiltered_capacity(void);
extern int coul_get_battery_rm(void);
extern int coul_get_battery_uuc (void);
extern int coul_get_battery_cc (void);
extern int coul_get_battery_delta_rc (void);
extern int coul_get_battery_ocv (void);
extern int coul_get_battery_resistance (void);


static int dcp_sh_set_enable_charger(unsigned int val)
{
	enum charge_status_event events = VCHRG_POWER_NONE_EVENT;

	if (NULL == pConfigOnDDr) {
		hwlog_err("%s pConfigOnDDr is NULL\n", __func__);
		return -1;
	}

	if ((val < 0) || (val > 1)) {
		return -1;
	}

	pConfigOnDDr->g_di.sysfs_data.charge_enable = val;
	pConfigOnDDr->g_di.sysfs_data.charge_limit = TRUE;
	/*why should send events in this command?
		   because it will get the /sys/class/power_supply/Battery/status immediately
		   to check if the enable/disable command set successfully or not in some product line station
	*/
	if (pConfigOnDDr->g_di.sysfs_data.charge_enable)
			events = VCHRG_START_CHARGING_EVENT;
	else
			events = VCHRG_NOT_CHARGING_EVENT;
	g_ops->set_charge_enable(pConfigOnDDr->g_di.sysfs_data.charge_enable);
	hisi_coul_charger_event_rcv(events);
	notify_sensorhub(CHARGE_SYSFS_ENABLE_CHARGER, 0);
	hwlog_info("RUNNINGTEST set charge enable = %d\n", pConfigOnDDr->g_di.sysfs_data.charge_enable);

	return 0;
}

static int dcp_sh_get_enable_charger(unsigned int *val)
{
	if (NULL == pConfigOnDDr) {
		hwlog_err("%s pConfigOnDDr is NULL\n", __func__);
		return -1;
	}

	notify_sensorhub(CHARGE_SYSFS_ENABLE_CHARGER, 1);
	*val = pConfigOnDDr->g_di.sysfs_data.charge_enable;

	return 0;
}

/* define public power interface */
static struct power_if_ops dcp_sh_power_if_ops = {
	.set_enable_charger = dcp_sh_set_enable_charger,
	.get_enable_charger = dcp_sh_get_enable_charger,
	.type_name = "dcp_sh",
};


/*lint -save -e* */
struct sensorhub_scene g_scens = {
	.stemps[0] = {
		.stype = 0,
		.min_temp = 0,
		.step = 5,
		.mNumItem = 4,
		.items = {{47,45,900,0,0},{50,48,470,0,0},{65,60,470,0,0},{75,73,0,0,10000},{0,}},
	},
	.stemps[1] = {
		.stype = 1,
		.min_temp = 0,
		.step = 5,
		.mNumItem = 2,
		.items = {{48,46,470,0,0},{75,73,0,0,10000},{0,}},
	},
	.stemps[2] = {
		.stype = 2,
		.min_temp = 0,
		.step = 5,
		.mNumItem = 5,
		.items = {{40,38,0,0,0},{42,41,0,0,0},{46,43,900,0,0},{51,47,470,0,0},{75,73,0,0,10000},{0,}},
	},
	.stemps[3] = {
		.stype = 9,
		.min_temp = -20,
		.step = 5,
		.mNumItem = 1,
		.items = {{70,68,0,0,10000},{0,}},
	},
};
/*lint -restore*/

#if defined CONFIG_HUAWEI_DSM
static struct charger_dsm {
	int error_no;
	bool notify_enable;
	void (*dump)(char*);
	char buf[ERR_NO_STRING_SIZE];
};
static struct charger_dsm err_count[] = {
	{ERROR_FCP_VOL_OVER_HIGH, true, .dump = fcp_reg_dump, "fcp vbus is high "},
	{ERROR_FCP_DETECT, true, .dump = fcp_reg_dump, "fcp detect fail "},
	{ERROR_FCP_OUTPUT, true, .dump = fcp_reg_dump, "fcp voltage output fail "},
	{ERROR_SWITCH_ATTACH, true, .dump = fcp_reg_dump, "fcp adapter connect fail "},
	{ERROR_ADAPTER_OVLT, true, .dump = fcp_reg_dump, "fcp adapter voltage over high "},
	{ERROR_ADAPTER_OCCURRENT, true, .dump = fcp_reg_dump, "fcp adapter current over high "},
	{ERROR_ADAPTER_OTEMP, true, .dump = fcp_reg_dump, "fcp adapter temp over high "},
	{ERROR_SAFE_PLOICY_LEARN, true, NULL, "battery safe ploicy learn"},
	{ERROR_SAFE_PLOICY_LEARN1, true, NULL, "safe ploicy learn 1"},
	{ERROR_SAFE_PLOICY_LEARN2, true, NULL, "safe ploicy learn 2"},
	{ERROR_SAFE_PLOICY_LEARN3, true, NULL, "safe ploicy learn 3"},
	{ERROR_BOOST_OCP, true, NULL, "otg ocp"},
	{ERROR_CHARGE_VBAT_OVP, true, NULL, "vbat ovp"},
	{SHB_ERR_FCP_VOL_OVER_HIGH, true, .dump = fcp_reg_dump, "sensorhub fcp vbus is high "},
	{SHB_ERR_FCP_DETECT, true, .dump = fcp_reg_dump, "sensorhub fcp detect fail "},
	{SHB_ERR_FCP_OUTPUT, true, .dump = fcp_reg_dump, "sensorhub fcp voltage output fail "},
	{SHB_ERR_SWITCH_ATTACH, true, .dump = fcp_reg_dump, "sensorhub fcp adapter connect fail "},
	{SHB_ERR_ADAPTER_OVLT, true, .dump = fcp_reg_dump, "sensorhub fcp adapter voltage over high "},
	{SHB_ERR_ADAPTER_OCCURRENT, true, .dump = fcp_reg_dump, "sensorhub fcp adapter current over high "},
	{SHB_ERR_ADAPTER_OTEMP, true, .dump = fcp_reg_dump, "sensorhub fcp adapter temp over high "},
	{SHB_ERR_BOOST_OCP, true, NULL, "sensorhub otg ocp"},
	{SHB_ERR_VBUS_HIGH, true, NULL, "sensorhub vbus over high"},
	{SHB_ERR_VBAT_OVP, true, NULL, "sensorhub vbat over high"},
};
#endif

static void fcp_reg_dump(char* pstr)
{
	hwlog_info("fcp_reg_dump sensorhub.\n");
	if (charger_dts_data.fcp_support) {
		if (!sh_fcp_hi6523_ops.reg_dump) {
			hwlog_err("%s ops is null\n", __func__);
		} else {
			sh_fcp_hi6523_ops.reg_dump(pstr);
		}
	} else if (sh_fcp_fsa9688_ops.is_support_fcp && !sh_fcp_fsa9688_ops.is_support_fcp()) {
		if (!sh_fcp_fsa9688_ops.reg_dump) {
			hwlog_err("%s ops is null\n", __func__);
		} else {
			sh_fcp_fsa9688_ops.reg_dump(pstr);
		}
	} else
		hwlog_err("%s nothing support fcp.\n", __func__);
}

/**********************************************************
*  Function:       sensorhub_charger_dsm_report
*  Description:    charger dsm report
*  Parameters:   err_no val
*  return value:  0:succ ;-1 :fail
**********************************************************/
int sensorhub_charger_dsm_report(int err_no, int *val)
{
	int ret = -1;
#if defined CONFIG_HUAWEI_DSM
	char dsm_buff[CHARGE_DMDLOG_SIZE] = { 0 };
	char buf[ERR_NO_STRING_SIZE] = { 0 };
	int i;
	int err_count_size = sizeof(err_count)/sizeof(struct charger_dsm);

	for (i = 0; i < err_count_size; i++) {
		if ((err_no == err_count[i].error_no) &&
			(true == err_count[i].notify_enable)) {/*every err_no report one times */
			strncat(dsm_buff, err_count[i].buf, ERR_NO_STRING_SIZE - 1);
			if (val) {  /*need report reg */
				snprintf(buf, sizeof(buf), "val= %d\n", *val);
				strncat(dsm_buff, buf, strlen(buf));
			}
			if (err_count[i].dump)
				err_count[i].dump(dsm_buff);
			if (!power_dsm_dmd_report(POWER_DSM_BATTERY, err_no, dsm_buff)) {
				/*when it be set 1,it will not report */
				err_count[i].notify_enable = false;
				ret = 0;
				break;
			}
		}
	}
#endif
	return ret;
}

/*lint -save -e* */
static int dump_bootloader_info(char *reg_value)
{
	char buff[26] = { 0 };
	int i = 0;

	memset(reg_value, 0, CHARGELOG_SIZE);
	snprintf(buff, 26, "%-8.2d", hisi_charger_info.ibus);
	strncat(reg_value, buff, strlen(buff));
	for (i = 0; i < REG_NUM_SH; i++) {
		snprintf(buff, 26, "0x%-8.2x", hisi_charger_info.reg[i]);
		strncat(reg_value, buff, strlen(buff));
	}
	return 0;
}
/*lint -restore*/

static int copy_bootloader_charger_info(void)
{
	char *p = NULL;
	p = get_charger_info_p();

	if (NULL == p) {
		hwlog_err("bootloader pointer NULL!\n");
		return -1;
	}

	memcpy(&hisi_charger_info, p + CHARGER_BASE_ADDR_SH,
	       sizeof(hisi_charger_info));
	hwlog_info("bootloader ibus %d\n", hisi_charger_info.ibus);

	return 0;
}

/**********************************************************
*  Function:       charge_wake_lock
*  Description:   apply charge wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void charge_wake_lock(void)
{
	if (!wake_lock_active(&charge_lock)) {
		wake_lock(&charge_lock);
		hwlog_info("sensorhub charge wake lock\n");
	}
}

/**********************************************************
*  Function:       charge_wake_unlock
*  Description:   release charge wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
void charge_wake_unlock(void)
{
	if (wake_lock_active(&charge_lock)) {
		wake_unlock(&charge_lock);
		hwlog_info("sensorhub charge wake unlock\n");
	}
}

static int notify_sensorhub_fcp(uint8_t stype, uint8_t rw)
{
	pkt_fcp_t pkt;
	pkt_fcp_resp_t resp_pkt;

	memset(&pkt, 0, sizeof(pkt));
	memset(&resp_pkt, 0, sizeof(resp_pkt));

	pkt.hd.tag = TAG_CHARGER;
	pkt.hd.cmd = CMD_CMN_CONFIG_REQ;
	pkt.hd.resp = RESP;
	pkt.hd.length = sizeof(pkt.sub_cmd);
	pkt.sub_cmd = CHARGE_SYSFS_FCP_SUPPORT;
	if (0 == WAIT_FOR_MCU_RESP_DATA_AFTER_SEND(&pkt,
						   inputhub_mcu_write_cmd
						   (&pkt, sizeof(pkt)),
						   5000, &resp_pkt,
						   sizeof(resp_pkt))) {
		hwlog_err("wait for notify sensorhub fcp timeout\n");
		return -1;
	} else {
		if (resp_pkt.hd.errno != 0) {
			hwlog_err("notify_sensorhub_fcp fail.\n");
			return -1;
		} else {
			hwlog_info("notify_sensorhub_fcp success, result %d.\n", resp_pkt.wr);
			return resp_pkt.wr;
		}
	}
}

int notify_sensorhub(uint8_t stype, uint8_t rw)
{
	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	uint8_t sub_cmd[2] = {0,};
	int ret = 0;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	sub_cmd[0] = stype;
	sub_cmd[1] = rw;
	pkg_ap.tag = TAG_CHARGER;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.wr_buf = sub_cmd;
	pkg_ap.wr_len = sizeof(sub_cmd);

	ret = write_customize_cmd(&pkg_ap,  &pkg_mcu);
	if(ret) {
		hwlog_err("send sysfs event notify to sensorhub fail, ret is %d, event is %d, rw is %d!\n", ret, stype, rw);
		return -1;
	} else {
		if(pkg_mcu.errno!=0) {
			hwlog_err("send sysfs event notify to sensorhub fail, errno is %d, event is %d, rw is %d!\n", pkg_mcu.errno, stype, rw);
			return -1;
		} else {
			hwlog_info( "send sysfs event notify to sensorhub success, event is %d, rw is %d!\n", stype, rw);
			return 0;
		}
	}
}

/**********************************************************
*  Function:       charge_rename_charger_type
*  Description:    rename the charger_type from USB PHY to charger
*  Parameters:   type:charger type from USB PHY
*                      di:charge_device_info
*                      update_flag
*  return value:  true:notify work /false:not notify work
**********************************************************/
static int charge_rename_charger_type(enum hisi_charger_type type,
				       struct charge_device_info_sh *di , enum update_type update_flag)
{
	bool ret = TRUE;

	if(CHARGER_TYPE_DCP == type && CHARGER_TYPE_NON_STANDARD == pConfigOnDDr->g_di.charger_type){
		hwlog_info("update charger type!\n");
		wake_lock_timeout(&wlock, TIMEOUT_SEC*HZ);
		notify_sensorhub(CHARGE_UPDATE_CHARGER_TYPE,0);
		charge_wake_unlock();
	}
	switch (type) {
	case CHARGER_TYPE_SDP:
		if(CHARGER_REMOVED == di->charger_type || CHARGER_TYPE_NON_STANDARD == di->charger_type
			|| CHARGER_TYPE_USB == di->charger_type || update_flag == TYPE_SENSORHUB_RECOVERY)
		{
			di->charger_type = CHARGER_TYPE_USB;
			di->charger_source = POWER_SUPPLY_TYPE_USB;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_CDP:
		if(CHARGER_REMOVED == di->charger_type || CHARGER_TYPE_NON_STANDARD == di->charger_type
			|| update_flag == TYPE_SENSORHUB_RECOVERY)
		{
			di->charger_type = CHARGER_TYPE_BC_USB;
			di->charger_source = POWER_SUPPLY_TYPE_USB;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_DCP:
		if(CHARGER_REMOVED == di->charger_type || update_flag == TYPE_SWITCH_UPDATE || update_flag == TYPE_SENSORHUB_RECOVERY)
		{
			di->charger_type = CHARGER_TYPE_STANDARD;
			di->charger_source = POWER_SUPPLY_TYPE_MAINS;
			if (1 == di->support_standard_ico)
				ico_enable = 1;
			else
				ico_enable = 0;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_UNKNOWN:
		if(CHARGER_REMOVED == di->charger_type || update_flag == TYPE_SENSORHUB_RECOVERY)
		{
			di->charger_type = CHARGER_TYPE_NON_STANDARD;
			di->charger_source = POWER_SUPPLY_TYPE_MAINS;
			ret = FALSE;
		}
		break;
	case CHARGER_TYPE_NONE:
		if(CHARGER_REMOVED != di->charger_type)
		{
			di->charger_type = CHARGER_REMOVED;
			di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
			ret = FALSE;
		}
		break;
	case PLEASE_PROVIDE_POWER:
		if(CHARGER_REMOVED == di->charger_type || update_flag == TYPE_SENSORHUB_RECOVERY)
		{
			di->charger_type = USB_EVENT_OTG_ID;
			di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
			ret = FALSE;
		}
		break;
	default:
		di->charger_type = CHARGER_REMOVED;
		di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
		ret = FALSE;
		break;
	}
	return ret;
}

/**********************************************************
*  Function:       charge_send_uevent
*  Discription:    send charge uevent immediately after charger type is recognized
*  Parameters:   di:charge_device_info_sh
*  return value:  NULL
**********************************************************/
static void charge_send_uevent(struct charge_device_info_sh *di)
{
	/*send events */
	enum charge_status_event events;
	if (di == NULL) {
		hwlog_err("[%s]di is NULL!\n", __func__);
		return;
	}

	if (di->charger_source == POWER_SUPPLY_TYPE_MAINS) {
		events = VCHRG_START_AC_CHARGING_EVENT;
		hisi_coul_charger_event_rcv(events);
	} else if (di->charger_source == POWER_SUPPLY_TYPE_USB) {
		events = VCHRG_START_USB_CHARGING_EVENT;
		hisi_coul_charger_event_rcv(events);
	} else if (di->charger_source == POWER_SUPPLY_TYPE_BATTERY) {
		events = VCHRG_STOP_CHARGING_EVENT;
		hisi_coul_charger_event_rcv(events);
	} else {
		hwlog_err("[%s]error charger source!\n", __func__);
		/*do nothing*/
	}
}

/*lint -save -e* */
static enum fcp_check_stage_type fcp_get_stage(void)
{
	if (pConfigOnDDr == NULL) {
		hwlog_err("[%s]pConfigOnDDr is not init!\n", __func__);
		return FCP_STAGE_DEFAUTL;
	} else if ((IOM3_RECOVERY_UNINIT != atomic_read(&iom3_rec_state)) && (iom3_sr_status != ST_SLEEP)) {
		notify_sensorhub(CHARGE_GET_FCP_STAGE, 1);
		return pConfigOnDDr->g_di.fcp_stage;
	} else
		return pConfigOnDDr->g_di.fcp_stage;
}
/*lint -restore*/


static enum usb_charger_type huawei_get_charger_type(void)
{
	if (NULL == pConfigOnDDr) {
		hwlog_err("[%s]pConfigOnDDr is not init!\n", __func__);
		return CHARGER_REMOVED;
	}
	return pConfigOnDDr->g_di.charger_type;
}

/**********************************************************
*  Function:       set_charge_state
*  Description:    set charge stop or enable
*  Parameters:     state:0 stop 1 enable
*  return value:   old state
**********************************************************/
static int set_charge_state(int state)
{
	int old_state = 1;
	int chg_en = 0;

	if (((state != 0) && (state != 1))
	    || NULL == g_ops->get_charge_enable_status ){
	    return -1;
	}

	old_state = g_ops->get_charge_enable_status();
	chg_en = state;
	g_ops->set_charge_enable(chg_en);

	return old_state;
}
extern void hisi_usb_otg_bc_again(void);
/**********************************************************
*  Function:       charge_type_dcp_detected_notify_sh
*  Description:    check high voltage charge once dcp detected
*  Parameters:     NULL
*  return value:   NULL
**********************************************************/
void charge_type_dcp_detected_notify_sh(void)
{
	if(NULL == pConfigOnDDr)
	{
		return ;
	}
	if (CHARGER_TYPE_NON_STANDARD == pConfigOnDDr->g_di.charger_type) {
		hisi_usb_otg_bc_again();
		hwlog_info(" stop phy enter! \n");
		wake_lock_timeout(&wlock, TIMEOUT_SEC*HZ);
		notify_sensorhub(CHARGE_UPDATE_CHARGER_TYPE,0);
	}
}

/**********************************************************
*  Function:       charge_process_vr_charge_event
*  Description:    deal with vr charge events
*  Parameters:     charge_device_info_sh * di
*  return value:   NULL
**********************************************************/
static void charge_process_vr_charge_event(struct charge_device_info_sh *di)
{
	enum usb_charger_type type_bak;
	enum power_supply_type supply_bak;

	type_bak = di->charger_type;
	supply_bak = di->charger_source;
	di->charger_type = CHARGER_REMOVED;
	di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
	charge_send_notify_to_sensorhub(di);
	di->charger_type = type_bak;
	di->charger_source = supply_bak;

	switch (di->sysfs_data.vr_charger_type) {
	case CHARGER_TYPE_SDP:
		di->charger_type = CHARGER_TYPE_USB;
		di->charger_source = POWER_SUPPLY_TYPE_USB;
		charge_send_uevent(di);
        	charge_send_notify_to_sensorhub(di);
		break;
	case CHARGER_TYPE_CDP:
		di->charger_type = CHARGER_TYPE_BC_USB;
		di->charger_source = POWER_SUPPLY_TYPE_USB;
		charge_send_uevent(di);
		charge_send_notify_to_sensorhub(di);
		break;
	case CHARGER_TYPE_DCP:
		di->charger_type = CHARGER_TYPE_VR;
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		charge_send_uevent(di);
		charge_send_notify_to_sensorhub(di);
		break;
	case CHARGER_TYPE_UNKNOWN:
		di->charger_type = CHARGER_TYPE_NON_STANDARD;
		di->charger_source = POWER_SUPPLY_TYPE_MAINS;
		charge_send_uevent(di);
		charge_send_notify_to_sensorhub(di);
		break;
	case CHARGER_TYPE_NONE:
		di->charger_type = USB_EVENT_OTG_ID;
		di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
		charge_send_uevent(di);
		charge_send_notify_to_sensorhub(di);
		break;
	default:
		hwlog_info("Invalid vr charger type! vr_charge_type = %d\n",
			   di->sysfs_data.vr_charger_type);
		break;
	}
}

/*lint -save -e* */
void update_charging_info(struct charge_device_info_sh *di)
{
	di->battery_fcc = coul_get_battery_fcc();
	di->battery_capacity = coul_get_battery_capacity();
	di->typec_current_mode = typec_current_mode_detect();
	di->ufcapacity = coul_battery_unfiltered_capacity();
	di->afcapacity = hisi_bci_show_capacity();
	di->rm = coul_get_battery_rm();
	di->uuc = coul_get_battery_uuc();
	di->cc = coul_get_battery_cc();
	di->delta_rc = coul_get_battery_delta_rc();
	di->ocv = coul_get_battery_ocv();
	di->rbatt = coul_get_battery_resistance();
	di->pd_charge = get_pd_charge_flag();
	hwlog_info("full charge capacity is 0x%x, current capacity is 0x%x.\n", di->battery_fcc, di->battery_capacity);
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_send_notify_to_sensorhub
*  Description:    notify sensorhub when charger insert
*  Parameters:   di: charger device info
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int charge_send_notify_to_sensorhub(struct charge_device_info_sh *di)
{
	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	pkt_cmn_interval_req_t pkt_charge_info;
	int ret = 0;

	if(NULL == di)
	{
		return -1;
	}
	memset(&pkt_charge_info, 0, sizeof(pkt_charge_info));
	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	switch(di->charger_type) {
		case CHARGER_TYPE_USB:	/*SDP*/
		case CHARGER_TYPE_BC_USB:	/*CDP*/
		case CHARGER_TYPE_NON_STANDARD:	/*UNKNOW*/
		case CHARGER_TYPE_STANDARD:	/*DCP*/
		case USB_EVENT_OTG_ID:
		case CHARGER_TYPE_VR:
		case CHARGER_TYPE_PD:
			pkt_charge_info.param.reserved[0] = (uint8_t)di->charger_type;
			pkt_charge_info.param.reserved[1] = (uint8_t)di->charger_source;
			if (charging_status == CHARGING_OPEN && g_iom3_state == IOM3_ST_NORMAL) {
				hwlog_warn("charger has already opend.\n");
				return 0;
			} else if ((IOM3_RECOVERY_UNINIT == atomic_read(&iom3_rec_state)) || (IOM3_ST_RECOVERY == g_iom3_state)) {
				di->charger_type = CHARGER_REMOVED;
				hwlog_warn("sensorhub is in initial stage.\n");
				return 0;
			} else
				charging_status = CHARGING_OPEN;
			update_charging_info(di);
			pkg_ap.tag = TAG_CHARGER;
			pkg_ap.cmd = CMD_CMN_INTERVAL_REQ;
			pkg_ap.wr_buf = &pkt_charge_info.param;
			pkg_ap.wr_len = sizeof(pkt_charge_info) - sizeof(pkt_charge_info.hd);
			if (g_iom3_state == IOM3_ST_NORMAL)
				ret=write_customize_cmd(&pkg_ap,  NULL);
			else if (g_iom3_state == IOM3_ST_REPEAT)
				ret=write_customize_cmd_noresp(pkg_ap.tag, pkg_ap.cmd,  pkg_ap.wr_buf, pkg_ap.wr_len);
			else {
				hwlog_warn("open notify can not send to sensorhub. sensorhub maybe in recovery mode.\n");
				return 0;
			}
			if(ret) {
				hwlog_err("send start notify to sensorhub fail, ret is %d, charger type is %d\n", ret, di->charger_type);
				return -1;
			} else {
				hwlog_info( "send start notify to sensorhub success, charger type is %d, type %d, source %d.\n",
					di->charger_type, pkt_charge_info.param.reserved[0], pkt_charge_info.param.reserved[1]);
				return 0;
			}

		case CHARGER_REMOVED:	/*not connected*/
			if (charging_status == CHARGING_CLOSE)
				return 0;
			else
				charging_status = CHARGING_CLOSE;
			pkg_ap.tag = TAG_CHARGER;
			pkg_ap.cmd = CMD_CMN_CLOSE_REQ;
			pkg_ap.wr_buf = NULL;
			pkg_ap.wr_len = 0;
			if (g_iom3_state == IOM3_ST_NORMAL) {
				ret = write_customize_cmd(&pkg_ap,  &pkg_mcu);
			} else if (g_iom3_state == IOM3_ST_REPEAT)
				ret = write_customize_cmd_noresp(pkg_ap.tag, pkg_ap.cmd, pkg_ap.wr_buf, pkg_ap.wr_len);
			else {
				hwlog_warn("close notify can not send to sensorhub. sensorhub maybe in recovery mode.\n");
				return 0;
			}
			if(ret) {
				hwlog_err("send close notify to sensorhub fail, ret is %d, charger type is %d\n", ret, di->charger_type);
				return -1;
			} else {
				if(pkg_mcu.errno!=0) {
					hwlog_err("send close notify to sensorhub fail, errno is %d, charger type is %d\n", pkg_mcu.errno, di->charger_type);
					return -1;
				} else {
					hwlog_info( "send close notify to sensorhub success, charger type is %d\n", di->charger_type);
					return 0;
				}
			}

		default:
			hwlog_err("charge_send_notify_to_sensorhub unsupport charger type %d\n", di->charger_type);
			break;
		}
	return 0;
}

/**********************************************************
*  Function:       charge_usb_notifier_call_sh
*  Description:    respond the charger_type events from USB PHY
*  Parameters:   usb_nb:usb notifier_block
*                      event:charger type event name
*                      data:unused
*  return value:  NOTIFY_OK-success or others
**********************************************************/
int charge_usb_notifier_call_sh(struct notifier_block *usb_nb,
				    unsigned long event, void *data)
{
	bool filter_flag = TRUE;
	enum hisi_charger_type charge_type = (enum hisi_charger_type)event;

	if (NULL == pConfigOnDDr) {
		hwlog_err("charge_usb_notifier_call_sh pConfigOnDDr is null.\n");
		return NOTIFY_OK;
	}
	mutex_lock(&charge_usb_notify_lock);
	hwlog_info("charge_usb_notifier_call_sh called in!event is %ld. charger type %d.\n", event, pConfigOnDDr->g_di.charger_type);
	//charger_type_ever_notify = true;
	if (charge_type == CHARGER_TYPE_UNKNOWN)
		charge_wake_lock();
	else if (charge_type == CHARGER_TYPE_NONE)
		charge_wake_unlock();

	if(g_iom3_state == IOM3_ST_NORMAL) {
		filter_flag = charge_rename_charger_type((enum hisi_charger_type)event, &pConfigOnDDr->g_di, TYPE_NONE);
	} else {
		filter_flag = charge_rename_charger_type((enum hisi_charger_type)event, &pConfigOnDDr->g_di, TYPE_SENSORHUB_RECOVERY);
	}
	if(filter_flag) {
		hwlog_info("not use work,filter_flag=%d\n",filter_flag);
		mutex_unlock(&charge_usb_notify_lock);
		return NOTIFY_OK;
	}

	charge_send_uevent(&pConfigOnDDr->g_di);
        hwlog_info("charge_usb_notifier_call_sh called!event is %ld. charger type %d.\n", event, pConfigOnDDr->g_di.charger_type);
        charge_send_notify_to_sensorhub(&pConfigOnDDr->g_di);
	mutex_unlock(&charge_usb_notify_lock);
	return NOTIFY_OK;
}

#ifdef CONFIG_SYSFS
#define CHARGE_SYSFS_FIELD(_name, n, m, store)                \
{                                                   \
    .attr = __ATTR(_name, m, charge_sysfs_show, store),    \
    .name = CHARGE_SYSFS_##n,          \
}

#define CHARGE_SYSFS_FIELD_RW(_name, n)               \
	CHARGE_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, charge_sysfs_store)

#define CHARGE_SYSFS_FIELD_RO(_name, n)               \
	CHARGE_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t charge_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf);
static ssize_t charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

/*lint -save -e* */
static struct charge_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static struct charge_sysfs_field_info charge_sysfs_field_tbl[] = {
	CHARGE_SYSFS_FIELD_RW(adc_conv_rate, ADC_CONV_RATE),
	CHARGE_SYSFS_FIELD_RW(iin_thermal, IIN_THERMAL),
	CHARGE_SYSFS_FIELD_RW(ichg_thermal, ICHG_THERMAL),
	CHARGE_SYSFS_FIELD_RW(iin_thermal_aux, IIN_THERMAL_AUX),
	CHARGE_SYSFS_FIELD_RW(ichg_thermal_aux, ICHG_THERMAL_AUX),
	/*iin_runningtest will be used for running test and audio test */
	CHARGE_SYSFS_FIELD_RW(iin_runningtest, IIN_RUNNINGTEST),
	CHARGE_SYSFS_FIELD_RW(ichg_runningtest, ICHG_RUNNINGTEST),
	CHARGE_SYSFS_FIELD_RW(enable_charger, ENABLE_CHARGER),
	CHARGE_SYSFS_FIELD_RW(limit_charging, LIMIT_CHARGING),
	CHARGE_SYSFS_FIELD_RW(regulation_voltage, REGULATION_VOLTAGE),
	CHARGE_SYSFS_FIELD_RW(shutdown_q4, BATFET_DISABLE),
	CHARGE_SYSFS_FIELD_RW(shutdown_watchdog, WATCHDOG_DISABLE),
	CHARGE_SYSFS_FIELD_RO(chargelog, CHARGELOG),
	CHARGE_SYSFS_FIELD_RO(chargelog_head, CHARGELOG_HEAD),
	CHARGE_SYSFS_FIELD_RO(Ibus, IBUS),
	CHARGE_SYSFS_FIELD_RO(Vbus, VBUS),
	CHARGE_SYSFS_FIELD_RW(enable_hiz, HIZ),
	CHARGE_SYSFS_FIELD_RO(chargerType, CHARGE_TYPE),
	CHARGE_SYSFS_FIELD_RO(charge_done_status, CHARGE_DONE_STATUS),
	CHARGE_SYSFS_FIELD_RW(charge_done_sleep_status, CHARGE_DONE_SLEEP_STATUS),
	CHARGE_SYSFS_FIELD_RW(inputcurrent, INPUTCURRENT),
	CHARGE_SYSFS_FIELD_RO(voltage_sys, VOLTAGE_SYS),
	CHARGE_SYSFS_FIELD_RO(bootloader_charger_info, BOOTLOADER_CHARGER_INFO),
	CHARGE_SYSFS_FIELD_RO(ichg_reg, ICHG_REG),
	CHARGE_SYSFS_FIELD_RO(ichg_adc, ICHG_ADC),
	CHARGE_SYSFS_FIELD_RO(ichg_reg_aux, ICHG_REG_AUX),
	CHARGE_SYSFS_FIELD_RO(ichg_adc_aux, ICHG_ADC_AUX),
	CHARGE_SYSFS_FIELD_RW(vr_charger_type, VR_CHARGER_TYPE),
	CHARGE_SYSFS_FIELD_RW(support_ico,    SUPPORT_ICO),
	CHARGE_SYSFS_FIELD_RW(water_intrused,    WATER_INTRUSED),
	CHARGE_SYSFS_FIELD_RO(charge_term_volt_design, CHARGE_TERM_VOLT_DESIGN),
	CHARGE_SYSFS_FIELD_RO(charge_term_curr_design, CHARGE_TERM_CURR_DESIGN),
	CHARGE_SYSFS_FIELD_RO(charge_term_volt_setting, CHARGE_TERM_VOLT_SETTING),
	CHARGE_SYSFS_FIELD_RO(charge_term_curr_setting, CHARGE_TERM_CURR_SETTING),
	CHARGE_SYSFS_FIELD_RO(fcp_support, FCP_SUPPORT),
	CHARGE_SYSFS_FIELD_RW(sensorhub_thermal_input, THERMAL_INPUT),
};

static struct attribute *charge_sysfs_attrs[ARRAY_SIZE(charge_sysfs_field_tbl) + 1];

static const struct attribute_group charge_sysfs_attr_group = {
	.attrs = charge_sysfs_attrs,
};
/*lint -restore*/

/**********************************************************
*  Function:       charge_sysfs_init_attrs
*  Description:    initialize charge_sysfs_attrs[] for charge attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static void charge_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		charge_sysfs_attrs[i] = &charge_sysfs_field_tbl[i].attr.attr;
	}
	charge_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_sysfs_field_lookup
*  Description:    get the current device_attribute from charge_sysfs_field_tbl by attr's name
*  Parameters:   name:device attribute name
*  return value:  charge_sysfs_field_tbl[]
**********************************************************/

/*lint -save -e* */
static struct charge_sysfs_field_info *charge_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp
		    (name, charge_sysfs_field_tbl[i].attr.attr.name,
		     strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &charge_sysfs_field_tbl[i];
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_sysfs_show
*  Description:    show the value for all charge device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static ssize_t charge_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct charge_sysfs_field_info *info = NULL;
	int ret;

	info = charge_sysfs_field_lookup(attr->attr.name);
	if (!info || !pConfigOnDDr)
		return -EINVAL;

	switch (info->name) {
	case CHARGE_SYSFS_ADC_CONV_RATE:
		notify_sensorhub(CHARGE_SYSFS_ADC_CONV_RATE, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.adc_conv_rate);
	case CHARGE_SYSFS_IIN_THERMAL:
		notify_sensorhub(CHARGE_SYSFS_IIN_THERMAL, 1);
		if (!pConfigOnDDr->g_di.is_dual_charger)
			return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.iin_thl);
		else
			return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.iin_thl_main);
	case CHARGE_SYSFS_ICHG_THERMAL:
		notify_sensorhub(CHARGE_SYSFS_ICHG_THERMAL, 1);
		if (!pConfigOnDDr->g_di.is_dual_charger)
			return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.ichg_thl);
		else
			return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.ichg_thl_main);
	case CHARGE_SYSFS_IIN_THERMAL_AUX:
		notify_sensorhub(CHARGE_SYSFS_IIN_THERMAL_AUX, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.iin_thl_aux);
	case CHARGE_SYSFS_ICHG_THERMAL_AUX:
		notify_sensorhub(CHARGE_SYSFS_ICHG_THERMAL_AUX, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux);
	case CHARGE_SYSFS_ICHG_REG:
		if (!pConfigOnDDr->g_di.is_dual_charger)
			return -EINVAL;
		return snprintf(buf, PAGE_SIZE, "%d\n", g_ops->get_ichg_reg(MAIN_CHARGER));
	case CHARGE_SYSFS_ICHG_ADC:
		if (!pConfigOnDDr->g_di.is_dual_charger)
			return -EINVAL;
		return snprintf(buf, PAGE_SIZE, "%d\n", g_ops->get_ichg_adc(MAIN_CHARGER));
	case CHARGE_SYSFS_ICHG_REG_AUX:
		if (!pConfigOnDDr->g_di.is_dual_charger)
			return -EINVAL;
		return snprintf(buf, PAGE_SIZE, "%d\n", g_ops->get_ichg_reg(AUX_CHARGER));
	case CHARGE_SYSFS_ICHG_ADC_AUX:
		if (!pConfigOnDDr->g_di.is_dual_charger)
			return -EINVAL;
		return snprintf(buf, PAGE_SIZE, "%d\n", g_ops->get_ichg_adc(AUX_CHARGER));
	case CHARGE_SYSFS_IIN_RUNNINGTEST:
		notify_sensorhub(CHARGE_SYSFS_IIN_RUNNINGTEST, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.iin_rt);
	case CHARGE_SYSFS_ICHG_RUNNINGTEST:
		notify_sensorhub(CHARGE_SYSFS_ICHG_RUNNINGTEST, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.ichg_rt);
	case CHARGE_SYSFS_ENABLE_CHARGER:
		notify_sensorhub(CHARGE_SYSFS_ENABLE_CHARGER, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.charge_enable);
	case CHARGE_SYSFS_LIMIT_CHARGING:
		notify_sensorhub(CHARGE_SYSFS_LIMIT_CHARGING ,1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.charge_limit);
	case CHARGE_SYSFS_REGULATION_VOLTAGE:
		notify_sensorhub(CHARGE_SYSFS_REGULATION_VOLTAGE, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.vterm_rt);
	case CHARGE_SYSFS_BATFET_DISABLE:
		notify_sensorhub(CHARGE_SYSFS_BATFET_DISABLE, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.batfet_disable);
	case CHARGE_SYSFS_WATCHDOG_DISABLE:
		notify_sensorhub(CHARGE_SYSFS_WATCHDOG_DISABLE, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.wdt_disable);
	case CHARGE_SYSFS_CHARGELOG:
		if (BAT_BOARD_UDP == pConfigOnDDr->g_di.is_board_type) {
			ret = snprintf(buf, PAGE_SIZE, "%s\n", "");
			return ret;
		}
		mutex_lock(&sysfs_lock.dump_reg_lock);
		g_ops->dump_register(reg_value);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", reg_value);
		mutex_unlock(&sysfs_lock.dump_reg_lock);
		return ret;
	case CHARGE_SYSFS_CHARGELOG_HEAD:
		mutex_lock(&sysfs_lock.dump_reg_head_lock);
		g_ops->get_register_head(reg_head);
		ret = snprintf(buf, PAGE_SIZE, "%s\n", reg_head);
		mutex_unlock(&sysfs_lock.dump_reg_head_lock);
		return ret;
	case CHARGE_SYSFS_IBUS:
		pConfigOnDDr->g_di.sysfs_data.ibus = 0;
		if (g_ops->get_ibus)	/*this is an optional interface for charger*/
			pConfigOnDDr->g_di.sysfs_data.ibus = g_ops->get_ibus();
		hwlog_info("ibus is %d.\n", pConfigOnDDr->g_di.sysfs_data.ibus);
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.ibus);
	case CHARGE_SYSFS_VBUS:
		pConfigOnDDr->g_di.sysfs_data.vbus = 0;
		if (g_ops->get_vbus)	 { /*this is an optional interface for charger*/
			ret = g_ops->get_vbus(&pConfigOnDDr->g_di.sysfs_data.vbus);
			if (ret)
				hwlog_err("[%s]vbus read failed \n", __func__);
		}
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.vbus);
	case CHARGE_SYSFS_HIZ:
		notify_sensorhub(CHARGE_SYSFS_HIZ, 1);
		return snprintf(buf, PAGE_SIZE, "%u\n", pConfigOnDDr->g_di.sysfs_data.hiz_enable);
	case CHARGE_SYSFS_CHARGE_TYPE:
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.charger_type);
	case CHARGE_SYSFS_CHARGE_DONE_STATUS:
		notify_sensorhub(CHARGE_SYSFS_CHARGE_DONE_STATUS, 1);
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.charge_done_status);
	case CHARGE_SYSFS_CHARGE_DONE_SLEEP_STATUS:
		notify_sensorhub(CHARGE_SYSFS_CHARGE_DONE_SLEEP_STATUS, 1);
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.charge_done_sleep_status);
		break;
	case CHARGE_SYSFS_INPUTCURRENT:
		notify_sensorhub(CHARGE_SYSFS_INPUTCURRENT, 1);
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.inputcurrent);
		break;
	case CHARGE_SYSFS_SUPPORT_ICO:
		notify_sensorhub(CHARGE_SYSFS_SUPPORT_ICO, 1);
		return snprintf(buf,PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.support_ico);
	case CHARGE_SYSFS_WATER_INTRUSED:
		notify_sensorhub(CHARGE_SYSFS_WATER_INTRUSED, 1);
		ret = snprintf(buf,PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.water_intrused);
		if(1 == pConfigOnDDr->g_di.sysfs_data.water_intrused)
			pConfigOnDDr->g_di.sysfs_data.water_intrused = 0;
		return ret;
	case CHARGE_SYSFS_VOLTAGE_SYS:
		pConfigOnDDr->g_di.sysfs_data.voltage_sys = 0;
		if (g_ops->get_vbat_sys)
			pConfigOnDDr->g_di.sysfs_data.voltage_sys = g_ops->get_vbat_sys();
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.voltage_sys);
		break;
	case CHARGE_SYSFS_BOOTLOADER_CHARGER_INFO:
		mutex_lock(&sysfs_lock.bootloader_info_lock);
		if (hisi_charger_info.info_vaild) {
			dump_bootloader_info(bootloader_info);
			ret =
			    snprintf(buf, PAGE_SIZE, "%s\n", bootloader_info);
		} else {
			ret = snprintf(buf, PAGE_SIZE, "\n");
		}
		mutex_unlock(&sysfs_lock.bootloader_info_lock);
		return ret;
	case CHARGE_SYSFS_VR_CHARGER_TYPE:
		//notify_sensorhub(CHARGE_SYSFS_VR_CHARGER_TYPE, 1);
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.vr_charger_type);
	case CHARGE_SYSFS_CHARGE_TERM_VOLT_DESIGN:
		notify_sensorhub(CHARGE_SYSFS_CHARGE_TERM_VOLT_DESIGN, 1);
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_core_info.data.vterm);
	case CHARGE_SYSFS_CHARGE_TERM_CURR_DESIGN:
		notify_sensorhub(CHARGE_SYSFS_CHARGE_TERM_CURR_DESIGN, 1);
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_core_info.data.iterm);
	case CHARGE_SYSFS_CHARGE_TERM_VOLT_SETTING:
		notify_sensorhub(CHARGE_SYSFS_CHARGE_TERM_VOLT_SETTING, 1);
		return snprintf(buf, PAGE_SIZE, "%d\n",
			((pConfigOnDDr->g_core_info.data.vterm < pConfigOnDDr->g_di.sysfs_data.vterm_rt) ? pConfigOnDDr->g_core_info.data.vterm : pConfigOnDDr->g_di.sysfs_data.vterm_rt));
	case CHARGE_SYSFS_CHARGE_TERM_CURR_SETTING:
		notify_sensorhub(CHARGE_SYSFS_CHARGE_TERM_CURR_SETTING, 1);
		return snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_core_info.data.iterm);
	case CHARGE_SYSFS_FCP_SUPPORT:
		mutex_lock(&sysfs_lock.fcp_support_lock);
		pConfigOnDDr->g_di.sysfs_data.fcp_support = notify_sensorhub_fcp(CHARGE_SYSFS_FCP_SUPPORT, 1);
		ret = snprintf(buf, PAGE_SIZE, "%d\n", pConfigOnDDr->g_di.sysfs_data.fcp_support);
		mutex_unlock(&sysfs_lock.fcp_support_lock);
		return ret;
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return 0;
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_sysfs_store
*  Description:    set the value for charge_data's node which is can be written
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static ssize_t charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct charge_sysfs_field_info *info = NULL;
	long val = 0;
	enum charge_status_event events = VCHRG_POWER_NONE_EVENT;

	info = charge_sysfs_field_lookup(attr->attr.name);
	if (!info || !pConfigOnDDr)
		return -EINVAL;

	switch (info->name) {
		/*NOTICE:
		   it will be charging with default current when the current node has been set to 0/1,
		   include iin_thermal/ichg_thermal/iin_runningtest/ichg_runningtest node
		 */
	case CHARGE_SYSFS_ADC_CONV_RATE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.adc_conv_rate = val;
		if (g_ops->set_adc_conv_rate)
			g_ops->set_adc_conv_rate(pConfigOnDDr->g_di.sysfs_data.adc_conv_rate);
		notify_sensorhub(CHARGE_SYSFS_ADC_CONV_RATE, 0);
		hwlog_info("set adc conversion rate mode = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.adc_conv_rate);
		break;
	case CHARGE_SYSFS_IIN_THERMAL:
#ifndef CONFIG_HLTHERM_RUNTEST
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
			return -EINVAL;
		if (!pConfigOnDDr->g_di.is_dual_charger) {
			if ((val == 0) || (val == 1))
				pConfigOnDDr->g_di.sysfs_data.iin_thl = pConfigOnDDr->g_core_info.data.iin_max;
			else if ((val > 1) && (val <= 100))
				pConfigOnDDr->g_di.sysfs_data.iin_thl = 100;
			else
				pConfigOnDDr->g_di.sysfs_data.iin_thl = val;

			if (pConfigOnDDr->g_di.input_current > pConfigOnDDr->g_di.sysfs_data.iin_thl)
				g_ops->set_input_current(pConfigOnDDr->g_di.sysfs_data.
							   iin_thl);
			hwlog_info("THERMAL set input current = %d\n",
				   pConfigOnDDr->g_di.sysfs_data.iin_thl);
		} else {
			if ((val == 0) || (val == 1))
				pConfigOnDDr->g_di.sysfs_data.iin_thl_main = pConfigOnDDr->g_core_info.data.iin_max / 2;
			else if ((val > 1) && (val <= 100))
				pConfigOnDDr->g_di.sysfs_data.iin_thl_main = 100;
			else
				pConfigOnDDr->g_di.sysfs_data.iin_thl_main = val;

			pConfigOnDDr->g_di.sysfs_data.iin_thl =
				pConfigOnDDr->g_di.sysfs_data.iin_thl_main + pConfigOnDDr->g_di.sysfs_data.iin_thl_aux;

			g_ops->set_input_current_thermal(pConfigOnDDr->g_di.sysfs_data.iin_thl_main,
							   pConfigOnDDr->g_di.sysfs_data.iin_thl_aux);
			hwlog_info("THERMAL set input current main = %d\n",
				   pConfigOnDDr->g_di.sysfs_data.iin_thl_main);
		}
		notify_sensorhub(CHARGE_SYSFS_IIN_THERMAL, 0);
#endif
		break;
	case CHARGE_SYSFS_ICHG_THERMAL:
#ifndef CONFIG_HLTHERM_RUNTEST
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
			return -EINVAL;
		if (!pConfigOnDDr->g_di.is_dual_charger) {
			if ((val == 0) || (val == 1))
				pConfigOnDDr->g_di.sysfs_data.ichg_thl = pConfigOnDDr->g_core_info.data.ichg_max;
			else if ((val > 1) && (val <= 500)) {
				hwlog_info
				    ("THERMAL set charge current = %ld is less than 500mA\n", val);
				pConfigOnDDr->g_di.sysfs_data.ichg_thl = 500;
			} else
				pConfigOnDDr->g_di.sysfs_data.ichg_thl = val;

			if (pConfigOnDDr->g_di.charge_current > pConfigOnDDr->g_di.sysfs_data.ichg_thl)
				g_ops->set_charge_current(pConfigOnDDr->g_di.sysfs_data.ichg_thl);
			hwlog_info("THERMAL set charge current = %d\n",
				   pConfigOnDDr->g_di.sysfs_data.ichg_thl);
		} else {
			if ((val == 0) || (val == 1))
				pConfigOnDDr->g_di.sysfs_data.ichg_thl_main =
				    pConfigOnDDr->g_core_info.data.ichg_max;
			else
				pConfigOnDDr->g_di.sysfs_data.ichg_thl_main = val;

			pConfigOnDDr->g_di.sysfs_data.ichg_thl =
				pConfigOnDDr->g_di.sysfs_data.ichg_thl_main + pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux;
			if (pConfigOnDDr->g_di.sysfs_data.ichg_thl <= 500) {
				hwlog_info
				    ("THERMAL set charge current = %u is less than 500mA, main = %u, aux = %u\n",
				     pConfigOnDDr->g_di.sysfs_data.ichg_thl,
				     pConfigOnDDr->g_di.sysfs_data.ichg_thl_main,
				     pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux);
				pConfigOnDDr->g_di.sysfs_data.ichg_thl = 500;
			}
			g_ops->set_charge_current_thermal(pConfigOnDDr->g_di.sysfs_data.ichg_thl_main,
							    pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux);
			hwlog_info("THERMAL set charge current main = %d\n",
				   pConfigOnDDr->g_di.sysfs_data.ichg_thl_main);
		}
		notify_sensorhub(CHARGE_SYSFS_ICHG_THERMAL, 0);
#endif
		break;
	case CHARGE_SYSFS_IIN_THERMAL_AUX:
#ifndef CONFIG_HLTHERM_RUNTEST
		if (!pConfigOnDDr->g_di.is_dual_charger)
			break;
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
			return -EINVAL;
		if ((val == 0) || (val == 1))
			pConfigOnDDr->g_di.sysfs_data.iin_thl_aux = pConfigOnDDr->g_core_info.data.iin_max / 2;
		else if ((val > 1) && (val <= 100))
			pConfigOnDDr->g_di.sysfs_data.iin_thl_aux = 100;
		else
			pConfigOnDDr->g_di.sysfs_data.iin_thl_aux = val;

		pConfigOnDDr->g_di.sysfs_data.iin_thl =
		    pConfigOnDDr->g_di.sysfs_data.iin_thl_main + pConfigOnDDr->g_di.sysfs_data.iin_thl_aux;

		g_ops->set_input_current_thermal(pConfigOnDDr->g_di.sysfs_data.iin_thl_main,
						   pConfigOnDDr->g_di.sysfs_data.iin_thl_aux);
		notify_sensorhub(CHARGE_SYSFS_IIN_THERMAL_AUX, 0);
		hwlog_info("THERMAL set input current aux = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.iin_thl_aux);
#endif
		break;
	case CHARGE_SYSFS_ICHG_THERMAL_AUX:
#ifndef CONFIG_HLTHERM_RUNTEST
		if (!pConfigOnDDr->g_di.is_dual_charger)
			break;
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
			return -EINVAL;
		if ((val == 0) || (val == 1))
			pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux = pConfigOnDDr->g_core_info.data.ichg_max;
		else
			pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux = val;

		pConfigOnDDr->g_di.sysfs_data.ichg_thl =
		    pConfigOnDDr->g_di.sysfs_data.ichg_thl_main + pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux;
		if (pConfigOnDDr->g_di.sysfs_data.ichg_thl <= 500) {
			hwlog_info
			    ("THERMAL set charge current = %u is less than 500mA, main = %u, aux = %u\n",
			     pConfigOnDDr->g_di.sysfs_data.ichg_thl,
			     pConfigOnDDr->g_di.sysfs_data.ichg_thl_main,
			     pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux);
			pConfigOnDDr->g_di.sysfs_data.ichg_thl = 500;
		}
		g_ops->set_charge_current_thermal(pConfigOnDDr->g_di.sysfs_data.ichg_thl_main,
						    pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux);
		notify_sensorhub(CHARGE_SYSFS_ICHG_THERMAL_AUX, 0);
		hwlog_info("THERMAL set charge current aux = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.ichg_thl_aux);
#endif
		break;
	case CHARGE_SYSFS_IIN_RUNNINGTEST:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
			return -EINVAL;
		if ((val == 0) || (val == 1))
			pConfigOnDDr->g_di.sysfs_data.iin_rt = pConfigOnDDr->g_core_info.data.iin_max;
		else if ((val > 1) && (val <= 100))
			pConfigOnDDr->g_di.sysfs_data.iin_rt = 100;
		else
			pConfigOnDDr->g_di.sysfs_data.iin_rt = val;

		if (pConfigOnDDr->g_di.input_current > pConfigOnDDr->g_di.sysfs_data.iin_rt)
			g_ops->set_input_current(pConfigOnDDr->g_di.sysfs_data.iin_rt);
		notify_sensorhub(CHARGE_SYSFS_IIN_RUNNINGTEST, 0);
		hwlog_info("RUNNINGTEST set input current = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.iin_rt);
		break;
	case CHARGE_SYSFS_ICHG_RUNNINGTEST:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000))
			return -EINVAL;
		if ((val == 0) || (val == 1))
			pConfigOnDDr->g_di.sysfs_data.ichg_rt = pConfigOnDDr->g_core_info.data.ichg_max;
		else if ((val > 1) && (val <= 205))
			pConfigOnDDr->g_di.sysfs_data.ichg_rt = 205;
		else
			pConfigOnDDr->g_di.sysfs_data.ichg_rt = val;

		if (pConfigOnDDr->g_di.charge_current > pConfigOnDDr->g_di.sysfs_data.ichg_rt)
			g_ops->set_charge_current(pConfigOnDDr->g_di.sysfs_data.ichg_rt);
		notify_sensorhub(CHARGE_SYSFS_ICHG_RUNNINGTEST, 0);
		hwlog_info("RUNNINGTEST set charge current = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.ichg_rt);
		break;
	case CHARGE_SYSFS_ENABLE_CHARGER:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.charge_enable = val;
		pConfigOnDDr->g_di.sysfs_data.charge_limit = TRUE;
		/*why should send events in this command?
		   because it will get the /sys/class/power_supply/Battery/status immediately
		   to check if the enable/disable command set successfully or not in some product line station
		 */
		if (pConfigOnDDr->g_di.sysfs_data.charge_enable)
			events = VCHRG_START_CHARGING_EVENT;
		else
			events = VCHRG_NOT_CHARGING_EVENT;
		g_ops->set_charge_enable(pConfigOnDDr->g_di.sysfs_data.charge_enable);
		hisi_coul_charger_event_rcv(events);
		notify_sensorhub(CHARGE_SYSFS_ENABLE_CHARGER, 0);
		hwlog_info("RUNNINGTEST set charge enable = %d\n", pConfigOnDDr->g_di.sysfs_data.charge_enable);
		break;
	case CHARGE_SYSFS_LIMIT_CHARGING:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.charge_limit = val;
		notify_sensorhub(CHARGE_SYSFS_LIMIT_CHARGING, 0);
		hwlog_info("PROJECTMUNE set limit charge enable = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.charge_limit);
		break;
	case CHARGE_SYSFS_REGULATION_VOLTAGE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 3200) || (val > 4400))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.vterm_rt = val;
		g_ops->set_terminal_voltage(pConfigOnDDr->g_di.sysfs_data.vterm_rt);
		notify_sensorhub(CHARGE_SYSFS_REGULATION_VOLTAGE, 0);
		hwlog_info("RUNNINGTEST set terminal voltage = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.vterm_rt);
		break;
	case CHARGE_SYSFS_BATFET_DISABLE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.batfet_disable = val;
		if (1 == val)
			g_ops->set_input_current(CHARGE_CURRENT_2000_MA);
		g_ops->set_batfet_disable(pConfigOnDDr->g_di.sysfs_data.batfet_disable);
		notify_sensorhub(CHARGE_SYSFS_BATFET_DISABLE, 0);
		hwlog_info("RUNNINGTEST set batfet disable = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.batfet_disable);
		break;
	case CHARGE_SYSFS_WATCHDOG_DISABLE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.wdt_disable = val;
		if (val == 1)
			g_ops->set_watchdog_timer(WATCHDOG_TIMER_DISABLE);
		notify_sensorhub(CHARGE_SYSFS_WATCHDOG_DISABLE, 0);
		hwlog_info("RUNNINGTEST set wdt disable = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.wdt_disable);
		break;
	case CHARGE_SYSFS_HIZ:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.hiz_enable = val;
		if (g_ops->set_charger_hiz)
			g_ops->set_charger_hiz(pConfigOnDDr->g_di.sysfs_data.hiz_enable);
		notify_sensorhub(CHARGE_SYSFS_HIZ, 0);
		hwlog_info("RUNNINGTEST set hiz enable = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.hiz_enable);
		break;
	case CHARGE_SYSFS_CHARGE_DONE_STATUS:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 10))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.charge_done_status = val;
		notify_sensorhub(CHARGE_SYSFS_CHARGE_DONE_STATUS, 0);
		break;
	case CHARGE_SYSFS_CHARGE_DONE_SLEEP_STATUS:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 10))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.charge_done_sleep_status = val;
		notify_sensorhub(CHARGE_SYSFS_CHARGE_DONE_SLEEP_STATUS, 0);
		break;
	case CHARGE_SYSFS_INPUTCURRENT:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 100) || (val > 2000))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.inputcurrent = val;
		g_ops->set_input_current(pConfigOnDDr->g_di.sysfs_data.inputcurrent);
		notify_sensorhub(CHARGE_SYSFS_INPUTCURRENT, 0);
		hwlog_info("set input currrent is: %ld\n", val);
		break;
	case CHARGE_SYSFS_SUPPORT_ICO:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.support_ico = val;
		notify_sensorhub(CHARGE_SYSFS_SUPPORT_ICO, 0);
		hwlog_info("SUPPORT_ICO = %d\n", pConfigOnDDr->g_di.sysfs_data.support_ico);
		break;
	case CHARGE_SYSFS_WATER_INTRUSED:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.water_intrused = val;
		notify_sensorhub(CHARGE_SYSFS_WATER_INTRUSED, 0);
		hwlog_info("set water_intrused = %d\n", pConfigOnDDr->g_di.sysfs_data.water_intrused);
		break;
	case CHARGE_SYSFS_VR_CHARGER_TYPE:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 4))
			return -EINVAL;
		pConfigOnDDr->g_di.sysfs_data.vr_charger_type = val;
		hwlog_info("Set vr_charger_type = %d\n",
			   pConfigOnDDr->g_di.sysfs_data.vr_charger_type);
		//charge_process_vr_charge_event(&pConfigOnDDr->g_di);//////////??????
		//notify_sensorhub(CHARGE_SYSFS_VR_CHARGER_TYPE, 0);
		break;
	case CHARGE_SYSFS_THERMAL_INPUT:
		if(!buf || sizeof(struct sensorhub_scene)!= count) {
			hwlog_err("size is %d, struct is %d.\n", (int)count, (int)sizeof(struct sensorhub_scene));
        		return -EINVAL;
    		} else {
			hwlog_info("store sensorhub thermal info success.\n");
			memcpy(&pConfigOnDDr->scenes, buf, count);
    		}
    		if(notify_sensorhub(CHARGE_THERMAL_INFO, 0)) {
			hwlog_err("notify sensorhub CHARGE_THERMAL_INFO error.\n");
        		return -EINVAL;
    		} else {
        		hwlog_info("notify sensorhub CHARGE_THERMAL_INFO success.\n");
    		}
    		break;
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return count;
}
/*lint -restore*/

/*lint -save -e* */
static bool charge_check_ts(void)
{
	if (pConfigOnDDr == NULL) {
		hwlog_err("[%s]pConfigOnDDr is not init!\n", __func__);
		return FALSE;
	} else if ((IOM3_RECOVERY_UNINIT != atomic_read(&iom3_rec_state)) && (iom3_sr_status != ST_SLEEP)) {
		notify_sensorhub(CHARGE_CHECK_CHARGER_TS, 1);
		return pConfigOnDDr->g_di.ts_flag;
	} else
		return pConfigOnDDr->g_di.ts_flag;
}

static int huawei_get_charge_current_max(void)
{
	if (NULL == pConfigOnDDr) {
		hwlog_err("[%s]pConfigOnDDr is not init!\n", __func__);
		return 0;
	}
	return pConfigOnDDr->g_core_info.data.ichg_max;
}

static bool charge_check_otg_state(void)
{
	if (pConfigOnDDr == NULL) {
		hwlog_err("[%s]pConfigOnDDr is not init!\n", __func__);
		return FALSE;
	} else if ((IOM3_RECOVERY_UNINIT != atomic_read(&iom3_rec_state)) && (iom3_sr_status != ST_SLEEP)) {
		notify_sensorhub(CHARGE_CHECK_CHARGER_OTG_STATE, 1);
		return pConfigOnDDr->g_di.otg_flag;
	} else
		return pConfigOnDDr->g_di.otg_flag;
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_sysfs_create_group
*  Description:    create the charge device sysfs group
*  Parameters:   di:charge_device_info_sh
*  return value:  0-sucess or others-fail
**********************************************************/
static int charge_sysfs_create_group(struct device *dev)
{
	charge_sysfs_init_attrs();
	return sysfs_create_group(&dev->kobj, &charge_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Description:    remove the charge device sysfs group
*  Parameters:   di:charge_device_info_sh
*  return value:  NULL
**********************************************************/
static inline void charge_sysfs_remove_group(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &charge_sysfs_attr_group);
}
#else
static int charge_sysfs_create_group(struct charge_device_info_sh *di)
{
	return 0;
}

static inline void charge_sysfs_remove_group(struct charge_device_info_sh *di)
{
}
#endif

/**********************************************************
*  Function:       charge_parse_dts
*  Description:    parse dts
*  Parameters:   charge_device_info_sh di
*  return value:  NULL
**********************************************************/
static void charge_parse_dts(struct charge_device_info_sh *di)
{
	int ret = 0;
	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,dual_charger"),
			"is_dual_charger", &di->is_dual_charger);
	if (ret){
		hwlog_err("get is_dual_charger fail!\n");
		di->is_dual_charger = 0;
	}

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger_sensorhub"),
			"water_check_enabled", &di->water_check_enabled);
	if (ret)
		hwlog_err("get water_check_enabled fail!\n");

	di->charge_done_sleep_dts =
		of_property_read_bool(of_find_compatible_node(NULL, NULL, "huawei,charger_sensorhub"),
			"charge_done_sleep_enabled");
	hwlog_info("charge_done_sleep_dts = %d\n", di->charge_done_sleep_dts);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,hisi_bci_battery"),
			"battery_board_type", &di->is_board_type);
	if (ret) {
		hwlog_err("get battery_board_type fail!\n");
		di->is_board_type = BAT_BOARD_ASIC;
	}

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger_sensorhub"), \
			"support_standard_ico", &di->support_standard_ico);
	if (ret) {
		hwlog_err("get support_standard_ico fail!\n");
		di->support_standard_ico = 0;
	}
	hwlog_info("support_standard_ico = %d\n", di->support_standard_ico);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger_sensorhub"), \
			"ico_all_the_way", &di->ico_all_the_way);
	if (ret) {
		hwlog_err("get ico_all_the_way fail!\n");
		di->ico_all_the_way = 0;
	}
	hwlog_info("ico_all_the_way = %d\n", di->ico_all_the_way);

	ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "huawei,charger_sensorhub"), \
			"fcp_vindpm", &di->fcp_vindpm);
	if (ret) {
		hwlog_err("get fcp_vindpm fail!\n");
		di->fcp_vindpm = CHARGE_VOLTAGE_4600_MV;
	}
	hwlog_info("fcp_vindpm = %d\n", di->fcp_vindpm);
}

static struct charge_extra_ops huawei_charge_extra_ops = {
	.check_ts = charge_check_ts,
	.check_otg_state = charge_check_otg_state,
	.get_stage = fcp_get_stage,
	.get_charger_type = huawei_get_charger_type,
	.set_state = set_charge_state,
	.get_charge_current = huawei_get_charge_current_max,
};

struct notifier_block usb_nb;
/**********************************************************
*  Function:       charge_probe
*  Description:    chargre module probe
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int charge_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct charge_device_info_sh *di;
	struct class *power_class = NULL;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("alloc di failed\n");
		return -ENOMEM;
	}

	huawei_charger_dev = pdev->dev;

	hwlog_info("Register usb notifier\n");
	usb_nb.notifier_call = charge_usb_notifier_call_sh;
	ret = hisi_charger_type_notifier_register(&usb_nb);
	if (ret < 0) {
		hwlog_err("hisi_charger_type_notifier_register failed\n");
		goto charge_fail_0;
	}

	di->sysfs_data.adc_conv_rate = 0;
	di->sysfs_data.iin_thl = g_core_info_sh->data.iin_max;
	di->sysfs_data.ichg_thl = g_core_info_sh->data.ichg_max;
	di->sysfs_data.iin_thl_main = g_core_info_sh->data.iin_max / 2;
	di->sysfs_data.ichg_thl_main = g_core_info_sh->data.ichg_max;
	di->sysfs_data.iin_thl_aux = g_core_info_sh->data.iin_max / 2;
	di->sysfs_data.ichg_thl_aux = g_core_info_sh->data.ichg_max;
	di->sysfs_data.iin_rt = g_core_info_sh->data.iin_max;
	di->sysfs_data.ichg_rt = g_core_info_sh->data.ichg_max;
	di->sysfs_data.vterm_rt = hisi_battery_vbat_max();
	di->sysfs_data.charge_enable = TRUE;
	di->sysfs_data.batfet_disable = FALSE;
	di->sysfs_data.wdt_disable = FALSE;
	di->sysfs_data.charge_limit = TRUE;
	di->sysfs_data.hiz_enable = FALSE;
	di->sysfs_data.charge_done_status = CHARGE_DONE_NON;
	di->sysfs_data.charge_done_sleep_status = CHARGE_DONE_SLEEP_DISABLED;
	di->sysfs_data.vr_charger_type = CHARGER_TYPE_NONE;//hisi_usb.h
	di->sysfs_data.support_ico = 1;
	di->charger_type = CHARGER_REMOVED;
	di->charger_source = POWER_SUPPLY_TYPE_BATTERY;
	di->ts_flag = FALSE;
	di->otg_flag = FALSE;
	di->fcp_stage = FCP_STAGE_DEFAUTL;
	di->typec_current_mode = TYPEC_DEV_CURRENT_DEFAULT;
	mutex_init(&sysfs_lock.dump_reg_lock);
	mutex_init(&sysfs_lock.dump_reg_head_lock);
	mutex_init(&sysfs_lock.bootloader_info_lock);
	mutex_init(&sysfs_lock.fcp_support_lock);
	mutex_init(&charge_usb_notify_lock);

	wake_lock_init(&charge_lock, WAKE_LOCK_SUSPEND, "charge_wakelock");
	wake_lock_init(&wlock, WAKE_LOCK_SUSPEND, "sensorhub_wakelock");

#ifndef CONFIG_HLTHERM_RUNTEST
	di->is_hltherm_runtest_mode = 0;
#else
	di->is_hltherm_runtest_mode = 1;
#endif
	di->charge_fault = CHARGE_FAULT_NON;
	di->check_full_count = 0;
	charge_parse_dts(di);

	if (strstr(saved_command_line, "androidboot.swtype=factory")) {
		di->is_factory_mode = 1;
	} else {
		di->is_factory_mode = 0;
	}

	if (strstr(saved_command_line, "androidboot.mode=charger")) {
		di->is_charger_mode = 1;
	} else {
		di->is_charger_mode = 0;
	}

	di->vbatt_max = hisi_battery_vbat_max();//for charge_core_safe_policy_handler
	di->sensorhub_stat = 0;

	ret = charge_sysfs_create_group(&huawei_charger_dev);
	if (ret)
		hwlog_err("can't create charge sysfs entries\n");
	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL)
			charge_dev = device_create(power_class, NULL, 0, NULL, "charger");
		ret = sysfs_create_link(&charge_dev->kobj, &huawei_charger_dev.kobj, "charge_data");
		if (ret) {
			hwlog_err("create link to charge_data fail.\n");
			goto charge_fail_1;
		}
	}
	g_di_sh = di;

	ret = charge_extra_ops_register(&huawei_charge_extra_ops);
	if (ret) {
		hwlog_err("register extra charge ops failed!\n");
	}

	if (power_if_ops_register(&dcp_sh_power_if_ops)) {
		hwlog_err("register power_if_ops_register failed!\n");
	}

	copy_bootloader_charger_info();
	update_charging_info(g_di_sh);
	hwlog_info("huawei sensorhub charger probe ok!\n");
	return 0;

charge_fail_1:
	charge_sysfs_remove_group(&huawei_charger_dev);
	wake_lock_destroy(&charge_lock);
	wake_lock_destroy(&wlock);
charge_fail_0:
	return ret;
}
/*lint -restore*/

/**********************************************************
*  Function:       charge_remove
*  Description:    charge module remove
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int charge_remove(struct platform_device *pdev)
{
	hisi_charger_type_notifier_unregister(&usb_nb);
	charge_sysfs_remove_group(&huawei_charger_dev);
	wake_lock_destroy(&charge_lock);
	wake_lock_destroy(&wlock);
	g_ops = NULL;
	return 0;
}

/**********************************************************
*  Function:       charge_shutdown_sh
*  Description:    charge module shutdown
*  Parameters:   pdev:platform_device
*  return value:  NULL
**********************************************************/
static void charge_shutdown(struct platform_device *pdev)
{
	int ret = 0;

	hwlog_info("%s ++\n", __func__);
	ret = g_ops->set_otg_enable(FALSE);
	if (ret) {
		hwlog_err("[%s]set otg default fail!\n", __func__);
	}

	if (g_ops->set_charger_hiz) {
		ret = g_ops->set_charger_hiz(FALSE);
		if (ret) {
			hwlog_err("[%s]set charger hiz default fail!\n",
				  __func__);
		}
	}

	hisi_charger_type_notifier_unregister(&usb_nb);
	hwlog_info("%s --\n", __func__);

	return;
}

/*lint -save -e* */
static struct of_device_id charge_match_table[] = {
	{
	 .compatible = "huawei,charger_sensorhub",
	 .data = NULL,
	 },
	{
	 },
};
/*lint -restore*/

static struct platform_driver charge_driver = {
	.probe = charge_probe,
	.remove = charge_remove,
	.shutdown = charge_shutdown,
	.driver = {
		   .name = "huawei,charger_sensorhub",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(charge_match_table),
	},
};

/**********************************************************
*  Function:       charge_init
*  Description:    charge module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int __init charge_init(void)
{
	return platform_driver_register(&charge_driver);
}

/**********************************************************
*  Function:       charge_exit
*  Description:    charge module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit charge_exit(void)
{
	platform_driver_unregister(&charge_driver);
}

//late_initcall(charge_init);
device_initcall_sync(charge_init);
module_exit(charge_exit);
/*lint -restore*/

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei charger sensorhub module driver");
MODULE_AUTHOR("HUAWEI Inc");
