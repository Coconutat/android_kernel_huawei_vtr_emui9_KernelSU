/*
 *  drivers/misc/inputhub/inputhub_bridge.c
 *  Sensor Hub Channel Bridge
 *
 *  Copyright (C) 2013 Huawei, Inc.
 *  Author: huangjisong
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <soc_pmctrl_interface.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/hisi/hisi_syscounter.h>
#include "contexthub_route.h"
#include "contexthub_boot.h"
#include "contexthub_recovery.h"
#include "sensor_detect.h"
#include "sensor_config.h"
#include "contexthub_pm.h"
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
#include <linux/hisi/hisi_idle_sleep.h>
#endif

/* CONFIG_USE_CAMERA3_ARCH : the camera module build config
* du to the sensor_1V8 by camera power chip */
#ifdef CONFIG_USE_CAMERA3_ARCH
#include <media/huawei/hw_extern_pmic.h>
#define SENSOR_1V8_POWER
#endif

int (*api_inputhub_mcu_recv) (const char* buf, unsigned int length) = 0;
int (*send_func) (int) = NULL;

extern uint32_t need_reset_io_power;
extern uint32_t need_set_3v_io_power;
extern atomic_t iom3_rec_state;

static int isSensorMcuMode;	/*mcu power mode: 0 power off;  1 power on */
static struct notifier_block nb;
static int g_boot_iom3 = STARTUP_IOM3_CMD;

struct completion iom3_reboot;
struct CONFIG_ON_DDR* pConfigOnDDr = NULL;
struct regulator *sensorhub_vddio;
int sensor_power_pmic_flag = 0;
int sensor_power_init_finish = 0;
u8 tplcd_manufacture;
unsigned long sensor_jiffies;
rproc_id_t ipc_ap_to_iom_mbx = HISI_RPROC_IOM3_MBX10;
rproc_id_t ipc_ap_to_lpm_mbx = HISI_RPROC_LPM3_MBX17;
rproc_id_t ipc_iom_to_ap_mbx = HISI_RPROC_IOM3_MBX4;
int sensorhub_reboot_reason_flag = SENSOR_POWER_DO_RESET;

/*lint -e550 -e715 -e773 */
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
#define PERI_USED_TIMEOUT (jiffies + HZ/100)
#define LOWER_LIMIT 0
#define UPPER_LIMIT 255
#define BORDERLINE_UPPER_PROTECT(a,b)		(a ==b) ? a:(a+1)
#define BORDERLINE_LOWER_PROTECT(a,b)	( a == b) ? a:(a-1)
struct timer_list peri_timer;
static unsigned int peri_used_t;
static unsigned int peri_used;
spinlock_t peri_lock;

static void peri_used_timeout(unsigned long data)
{
	unsigned long flags;
	spin_lock_irqsave(&peri_lock, flags);
	pr_debug("[%s]used[%d],t[%d]\n", __func__ ,peri_used,peri_used_t);
	if(0 == peri_used) {
		int ret = hisi_idle_sleep_vote(ID_IOMCU, 0);
		if (ret)pr_err("[%s]hisi_idle_sleep_vote err\n", __func__);
		peri_used_t = 0;
	}
	spin_unlock_irqrestore(&peri_lock, flags);
}
#endif

static void peri_used_init(void)
{
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
	spin_lock_init(&peri_lock);
	setup_timer(&peri_timer, peri_used_timeout, 0);
#endif
}

void peri_used_request(void)
{
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
	unsigned long flags;
	del_timer_sync(&peri_timer);
	spin_lock_irqsave(&peri_lock, flags);
	pr_debug("[%s]used[%d],t[%d]\n", __func__ ,peri_used,peri_used_t);
	if (0 != peri_used) {
		peri_used = BORDERLINE_UPPER_PROTECT(peri_used, UPPER_LIMIT);
		spin_unlock_irqrestore(&peri_lock, flags);
		return;
	}

	if (0 == peri_used_t) {
		int ret = hisi_idle_sleep_vote(ID_IOMCU, 1);
		if (ret)pr_err("[%s]hisi_idle_sleep_vote err\n", __func__);
	}else{/*just for pclist*/}

	peri_used = 1;
	peri_used_t = 1;

	spin_unlock_irqrestore(&peri_lock, flags);
#endif
}

void peri_used_release(void)
{
#ifdef CONFIG_CONTEXTHUB_IDLE_32K
	unsigned long flags;
	spin_lock_irqsave(&peri_lock, flags);
	pr_debug("[%s]used[%d]\n", __func__ ,peri_used);
	peri_used = BORDERLINE_LOWER_PROTECT(peri_used, LOWER_LIMIT);
	mod_timer(&peri_timer, PERI_USED_TIMEOUT);
	spin_unlock_irqrestore(&peri_lock, flags);
#endif
}

#ifdef CONFIG_HUAWEI_DSM
struct dsm_client* shb_dclient;
#endif

#ifdef SENSOR_1V8_POWER
extern int hw_extern_pmic_query_state(int index, int *state);
#else
static int hw_extern_pmic_query_state(int index, int *state)
{
	hwlog_err("the camera power cfg donot define\n");
	return 1;
}
#endif

int getSensorMcuMode(void)
{
    return isSensorMcuMode;
}
EXPORT_SYMBOL(getSensorMcuMode);

static void setSensorMcuMode(int mode)
{
    isSensorMcuMode = mode;
}

int is_sensorhub_disabled(void)
{
	int len = 0;
	struct device_node* sh_node = NULL;
	const char* sh_status = NULL;
	static int ret = 0, once = 0;

	if(once)
		return ret;

	sh_node = of_find_compatible_node(NULL, NULL, "huawei,sensorhub_status");
	if (!sh_node) {
	    hwlog_err("%s, can not find node  sensorhub_status n", __func__);
	    return -1;
	}

	sh_status = of_get_property(sh_node, "status", &len);
	if (!sh_status) {
	    hwlog_err("%s, can't find property status\n", __func__);
	    return -1;
	}

	if (strstr(sh_status, "ok")) {
	    hwlog_info("%s, sensorhub enabled!\n", __func__);
	    ret = 0;
	} else {
	    hwlog_info("%s, sensorhub disabled!\n", __func__);
	    ret = -1;
	}
	once = 1;
	return ret;
}

#ifdef SENSOR_1V8_POWER
static int check_sensor_1V8_from_pmic(void)
{
	int len = 0, ret = 0, i = 0;
	int sensor_1v8_flag = 0, state = 0;
	int sensor_1v8_ldo = 0;
	struct device_node *sensorhub_node = NULL;
	const char *sensor_1v8_from_pmic = NULL;
	sensorhub_node = of_find_compatible_node(NULL, NULL, "huawei,sensorhub");
	if (!sensorhub_node) {
		hwlog_err("%s, can't find node sensorhub\n", __func__);
		return 0;
	}
	sensor_1v8_from_pmic = of_get_property(sensorhub_node, "sensor_1v8_from_pmic", &len);
	if (!sensor_1v8_from_pmic) {
		hwlog_info("%s, can't find property sensor_1v8_from_pmic\n", __func__);
		return 1;
	}
	sensor_power_pmic_flag = 1;
	if (strstr(sensor_1v8_from_pmic, "yes")) {
		hwlog_info("%s, sensor_1v8 from pmic\n",__func__);
		if (of_property_read_u32(sensorhub_node, "sensor_1v8_ldo", &sensor_1v8_ldo)){
			hwlog_err("%s,read sensor_1v8_ldo fail\n",__func__);
			return 0;
		} else {
			hwlog_info("%s,read sensor_1v8_ldo(%d) succ\n",__func__,sensor_1v8_ldo);
			sensor_1v8_flag = 1;
		}
	} else {
		hwlog_info("%s, sensor_1v8 not from pmic\n", __func__);
		return 1;
	}

	if(sensor_1v8_flag){
		for(i = 0; i < 10; i++){
			ret = hw_extern_pmic_query_state(sensor_1v8_ldo, &state);
			if(state){
				hwlog_info("sensor_1V8 set high succ!\n");
				break;
			}
			msleep(200);
		}
		if(i==10 && state == 0){
			hwlog_err("sensor_1V8 set high fail,ret:%d!\n",ret);
			return 0;
		}
	}
	sensor_power_init_finish = 1;
	return 1;
}
#endif

int sensor_pmic_power_check(void)
{
	int ret = 0;
	int state = 0;
	int result = SENSOR_POWER_STATE_OK;

	if (!sensor_power_init_finish || is_sensorhub_disabled()) {
		result = SENSOR_POWER_STATE_INIT_NOT_READY;
		goto out;
	}
	if (!sensor_power_pmic_flag) {
		result =  SENSOR_POWER_STATE_NOT_PMIC;
		goto out;
	}

	ret = hw_extern_pmic_query_state(1, &state);
	if (ret) {
		result = SENSOR_POWER_STATE_CHECK_ACTION_FAILED;
		goto out;
	}
	if (!state)
		result = SENSOR_POWER_STATE_CHECK_RESULT_FAILED;
out:
	hwlog_info("sensor check result:%d\n", result);
	return result;
}

static lcd_module lcd_info[] = {
	{DTS_COMP_JDI_NT35695_CUT3_1, JDI_TPLCD},
	{DTS_COMP_LG_ER69006A, LG_TPLCD},
	{DTS_COMP_JDI_NT35695_CUT2_5, JDI_TPLCD},
	{DTS_COMP_LG_ER69007, KNIGHT_BIEL_TPLCD},
	{DTS_COMP_SHARP_NT35597, KNIGHT_BIEL_TPLCD},
	{DTS_COMP_SHARP_NT35695, KNIGHT_BIEL_TPLCD},
	{DTS_COMP_LG_ER69006_FHD, KNIGHT_BIEL_TPLCD},
	{DTS_COMP_MIPI_BOE_ER69006, KNIGHT_LENS_TPLCD},
	{DTS_COMP_BOE_OTM1906C, BOE_TPLCD},
	{DTS_COMP_INX_OTM1906C, INX_TPLCD},
	{DTS_COMP_EBBG_OTM1906C, EBBG_TPLCD},
	{DTS_COMP_JDI_NT35695, JDI_TPLCD},
	{DTS_COMP_LG_R69006, LG_TPLCD},
	{DTS_COMP_SAMSUNG_S6E3HA3X02, SAMSUNG_TPLCD},
	{DTS_COMP_LG_R69006_5P2, LG_TPLCD},
	{DTS_COMP_SHARP_NT35695_5P2, SHARP_TPLCD},
	{DTS_COMP_JDI_R63452_5P2, JDI_TPLCD},
	{DTS_COMP_SAM_WQ_5P5, BIEL_TPLCD},
	{DTS_COMP_SAM_FHD_5P5, VITAL_TPLCD},
	{DTS_COMP_JDI_R63450_5P7, JDI_TPLCD},
	{DTS_COMP_SHARP_DUKE_NT35597, SHARP_TPLCD},
};

static lcd_model lcd_model_info[] = {
	{DTS_COMP_AUO_OTM1901A_5P2, AUO_TPLCD},
	{DTS_COMP_AUO_TD4310_5P2, AUO_TPLCD},
	{DTS_COMP_TM_FT8716_5P2, TM_TPLCD},
	{DTS_COMP_EBBG_NT35596S_5P2, EBBG_TPLCD},
	{DTS_COMP_JDI_ILI7807E_5P2, JDI_TPLCD},
};

static int8_t get_lcd_info(uint8_t index)
{
	struct device_node *np = NULL;
	int ret = -1;

	np = of_find_compatible_node(NULL, NULL, lcd_info[index].dts_comp_mipi);
	ret = of_device_is_available(np);
	if (np && ret) {
		//hwlog_info("%s is present\n", lcd_info[index].dts_comp_mipi);
		return lcd_info[index].tplcd;
	} else
		return -1;
}

static int8_t get_lcd_model(const char *lcd_model, uint8_t index)
{
	if(!strncmp(lcd_model, lcd_model_info[index].dts_comp_lcd_model, strlen(lcd_model_info[index].dts_comp_lcd_model))){
		//hwlog_info("%s is present\n", lcd_model_info[index].dts_comp_lcd_model);
		return lcd_model_info[index].tplcd;
	} else
		return -1;
}

static int get_lcd_module(void)
{
	uint8_t index = 0;
	int8_t tplcd;
	struct device_node *np = NULL;
	const char *lcd_model = NULL;

	for(index=0; index<ARRAY_SIZE(lcd_info); index++) {
		tplcd = get_lcd_info(index);
		if(tplcd>0)
			return tplcd;
	}

	np = of_find_compatible_node(NULL,NULL,"huawei,lcd_panel_type");
	if(!np){
		hwlog_err("not find lcd_panel_type node\n");
		return -1;
	}
	if(of_property_read_string(np,"lcd_panel_type",&lcd_model)){
		hwlog_err("not find lcd_model in dts\n");
		return -1;
	}
	hwlog_info("find lcd_panel_type suc in dts!!\n");

	for(index=0; index<ARRAY_SIZE(lcd_model_info); index++) {
		tplcd = get_lcd_model(lcd_model, index);
		if(tplcd>0)
			return tplcd;
	}

	hwlog_warn("sensor kernel failed to get lcd module\n");
	return -1;
}

static int inputhub_mcu_recv(const char* buf, unsigned int length)
{
    if (IOM3_RECOVERY_START == atomic_read(&iom3_rec_state))
    {
        hwlog_err("iom3 under recovery mode, ignore all recv data\n");
        return 0;
    }

    if (api_inputhub_mcu_recv != NULL) {
        return api_inputhub_mcu_recv(buf, length);
    } else {
        hwlog_err("---->error: api_inputhub_mcu_recv == NULL\n");
        return -1;
    }
}

/*received data from mcu.*/
static int mbox_recv_notifier(struct notifier_block* nb, unsigned long len, void* msg)
{
    inputhub_mcu_recv(msg, len * sizeof(int));	/*convert to bytes*/
    return 0;
}

static int inputhub_mcu_connect(void)
{
	int ret = 0;
	/*connect to inputhub_route*/
	api_inputhub_mcu_recv = inputhub_route_recv_mcu_data;

	hwlog_info("----%s--->\n", __func__);

	nb.next = NULL;
	nb.notifier_call = mbox_recv_notifier;

	/* register the rx notify callback */
	ret = RPROC_MONITOR_REGISTER(ipc_iom_to_ap_mbx, &nb);
	if (ret)
		hwlog_info("%s:RPROC_MONITOR_REGISTER failed", __func__);
	return 0;
}

/*extern void hisi_rdr_nmi_notify_iom3(void);*/
static int sensorhub_img_dump(int type, void* buff, int size)
{
    return 0;
}

#ifdef CONFIG_HUAWEI_DSM
struct dsm_client_ops sensorhub_ops =
{
    .poll_state = NULL,
    .dump_func = sensorhub_img_dump,
};

struct dsm_dev dsm_sensorhub =
{
    .name = "dsm_sensorhub",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = &sensorhub_ops,
    .buff_size = 1024,
};
#endif

static int boot_iom3(void)
{
	int ret = 0;
	peri_used_request();
	ret = RPROC_ASYNC_SEND(ipc_ap_to_iom_mbx, (mbox_msg_t*)&g_boot_iom3, 1);
	peri_used_release();
	if (ret) {
		hwlog_err("RPROC_ASYNC_SEND error in %s\n", __func__);
	}
	return ret;
}

void write_timestamp_base_to_sharemem(void)
{
	u64 syscnt;
	u64 kernel_ns;
	struct timespec64 ts;

	get_monotonic_boottime64(&ts);
#ifdef CONFIG_HISI_SYSCOUNTER
	syscnt = hisi_get_syscount();
#endif
	kernel_ns = (u64)(ts.tv_sec * NSEC_PER_SEC) + (u64)ts.tv_nsec;

	pConfigOnDDr->timestamp_base.syscnt = syscnt;
	pConfigOnDDr->timestamp_base.kernel_ns = kernel_ns;

	return;
}

static int mcu_sys_ready_callback(const pkt_header_t *head)
{
	int ret = 0;
	unsigned int time_of_vddio_power_reset = 0;
	unsigned long new_sensor_jiffies = 0;
#ifdef SENSOR_1V8_POWER
	int result = 0;
#endif
	if (ST_MINSYSREADY == ((pkt_sys_statuschange_req_t *) head)->status) {
		hwlog_info("sys ready mini!\n");
		tplcd_manufacture = get_lcd_module();
		hwlog_info("sensor kernel get_lcd_module tplcd_manufacture=%d\n", tplcd_manufacture);
	#ifdef SENSOR_1V8_POWER
		result = check_sensor_1V8_from_pmic();
		if(!result)
			hwlog_err("check sensor_1V8 from pmic fail\n");
	#endif
		hwlog_info("need_reset_io_power:%d reboot_reason_flag:%d\n", need_reset_io_power, sensorhub_reboot_reason_flag);
		if(need_reset_io_power && (SENSOR_POWER_DO_RESET == sensorhub_reboot_reason_flag)) {
			new_sensor_jiffies = jiffies- sensor_jiffies;
			time_of_vddio_power_reset = jiffies_to_msecs(new_sensor_jiffies);
			if (time_of_vddio_power_reset < SENSOR_MAX_RESET_TIME_MS)
				msleep(SENSOR_MAX_RESET_TIME_MS - time_of_vddio_power_reset);

			if (need_set_3v_io_power) {
				ret = regulator_set_voltage(sensorhub_vddio, SENSOR_VOLTAGE_3V, SENSOR_VOLTAGE_3V);
				if (ret < 0)
					hwlog_err("failed to set sensorhub_vddio voltage to 3V\n");
			}

			hwlog_info("time_of_vddio_power_reset %u\n", time_of_vddio_power_reset);
			ret = regulator_enable(sensorhub_vddio);
			if (ret < 0)
				hwlog_err("sensor vddio enable 2.85V\n");

			msleep(SENSOR_DETECT_AFTER_POWERON_TIME_MS);
		}
		ret = init_sensors_cfg_data_from_dts();
		if (ret)
			hwlog_err("get sensors cfg data from dts fail,ret=%d, use default config data!\n", ret);
		else
			hwlog_info("get sensors cfg data from dts success!\n");
	} else if (ST_MCUREADY == ((pkt_sys_statuschange_req_t *) head)->status) {
		hwlog_info("mcu all ready!\n");
		ret = sensor_set_cfg_data();
		if (ret < 0)
			hwlog_err("sensor_chip_detect ret=%d\n", ret);
		ret = sensor_set_fw_load();
		if (ret < 0) {
			hwlog_err("sensor fw dload err ret=%d\n", ret);
		}
		unregister_mcu_event_notifier(TAG_SYS, CMD_SYS_STATUSCHANGE_REQ, mcu_sys_ready_callback);
		atomic_set(&iom3_rec_state, IOM3_RECOVERY_IDLE);
	} else {
		hwlog_info("other status\n");
	}
	return 0;
}

static void set_notifier(void)
{
	register_mcu_event_notifier(TAG_SYS, CMD_SYS_STATUSCHANGE_REQ, mcu_sys_ready_callback);
	register_mcu_event_notifier(TAG_SYS, CMD_SYS_STATUSCHANGE_REQ, iom3_rec_sys_callback);
	set_pm_notifier();
}

static void read_reboot_reason_cmdline(void)
{
	char reboot_reason_buf[SENSOR_REBOOT_REASON_MAX_LEN] = { 0 };
	char *pstr, *dstr;

	pstr = strstr(saved_command_line, "reboot_reason=");
	if (!pstr) {
		pr_err("No fastboot reboot_reason info\n");
		return;
	}
	pstr += strlen("reboot_reason=");
	dstr = strstr(pstr, " ");
	if (!dstr) {
		pr_err("No find the reboot_reason end\n");
		return;
	}
	memcpy(reboot_reason_buf, pstr, (unsigned long)(dstr - pstr));
	reboot_reason_buf[dstr - pstr] = '\0';

	if (!strcasecmp(reboot_reason_buf, "AP_S_COLDBOOT")) {
		/* reboot flag */
		sensorhub_reboot_reason_flag = SENSOR_POWER_NO_RESET;
	} else {
		/* others */
		sensorhub_reboot_reason_flag = SENSOR_POWER_DO_RESET;
	}
	hwlog_info("sensorhub get reboot reason:%s length:%d flag:%d\n",	reboot_reason_buf, (int)strlen(reboot_reason_buf), sensorhub_reboot_reason_flag);
}

static int write_defualt_config_info_to_sharemem(void)
{
    if (!pConfigOnDDr) {
        pConfigOnDDr = (struct CONFIG_ON_DDR*)ioremap_wc(IOMCU_CONFIG_START, IOMCU_CONFIG_SIZE);
    }

    if (pConfigOnDDr == NULL) {
        hwlog_err("ioremap (%x) failed in %s!\n", IOMCU_CONFIG_START, __func__);
        return -1;
    }

    memset(pConfigOnDDr, 0, sizeof(struct CONFIG_ON_DDR));
    pConfigOnDDr->LogBuffCBBackup.mutex = 0;
    pConfigOnDDr->log_level = INFO_LEVEL;
    return 0;
}

static void check_ipc_mbx_from_dts(void)
{
	struct device_node* sh_node;
	uint32_t u32_temp = 0;
	int ret;

	sh_node = of_find_compatible_node(NULL, NULL, "huawei,sensorhub_ipc_cfg");
	if (!sh_node) {
		hwlog_err("%s, can not find node huawei,sensorhub_ipc_cfg", __func__);
		return;
	}

	ret = of_property_read_u32(sh_node, "AP_IOM_MBX_NUM", &u32_temp);
	if (ret) {
		hwlog_warn("%s, can't find property AP_IOM_MBX_NUM\n", __func__);
	} else {
		ipc_ap_to_iom_mbx = (rproc_id_t)u32_temp;
		hwlog_info("ipc_ap_to_iom_mbx is %d;\n", ipc_ap_to_iom_mbx);
	}

	ret = of_property_read_u32(sh_node, "IOM_AP_MBX_NUM", &u32_temp);
	if (ret) {
		hwlog_warn("%s, can't find property IOM_AP_MBX_NUM\n", __func__);
	} else {
		ipc_iom_to_ap_mbx = (rproc_id_t)u32_temp;
		hwlog_info("ipc_iom_to_ap_mbx is %d;\n", ipc_iom_to_ap_mbx);
	}

	ret = of_property_read_u32(sh_node, "AP_LPM_MBX_NUM", &u32_temp);
	if (ret) {
		hwlog_warn("%s, can't find property AP_LPM_MBX_NUM\n", __func__);
	} else {
		ipc_ap_to_lpm_mbx = (rproc_id_t)u32_temp;
		hwlog_info("ipc_ap_to_lpm_mbx is %d;\n", ipc_ap_to_lpm_mbx);
	}
}

static int inputhub_mcu_init(void)
{
	int ret = 0;

	if (is_sensorhub_disabled())
		return -1;

	peri_used_init();
	check_ipc_mbx_from_dts();
	if (write_defualt_config_info_to_sharemem())
		return -1;
	write_timestamp_base_to_sharemem();
	read_tp_color_cmdline();
	read_reboot_reason_cmdline();
	sensorhub_io_driver_init();

#ifdef CONFIG_HUAWEI_DSM
	shb_dclient = dsm_register_client(&dsm_sensorhub);
#endif
	init_completion(&iom3_reboot);
	recovery_init();
	sensor_redetect_init();
	inputhub_route_init();
	set_notifier();
	inputhub_mcu_connect();
	ret = boot_iom3();
	if (ret) {
		hwlog_err("%s boot sensorhub fail!ret %d.\n", __func__, ret);
	}
	setSensorMcuMode(1);
	hwlog_info("----%s--->\n", __func__);
	return ret;
}

static void __exit inputhub_mcu_exit(void)
{
    inputhub_route_exit();
    RPROC_PUT(ipc_ap_to_iom_mbx);
    RPROC_PUT(ipc_iom_to_ap_mbx);
}

late_initcall(inputhub_mcu_init);
module_exit(inputhub_mcu_exit);

MODULE_AUTHOR("Input Hub <smartphone@huawei.com>");
MODULE_DESCRIPTION("input hub bridge");
MODULE_LICENSE("GPL");
