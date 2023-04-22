/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2013 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/sched/rt.h>
#include <linux/fb.h>
#include <linux/workqueue.h>
#include <linux/vmalloc.h>
#include <lcdkit_tp.h>
#include <linux/notifier.h>
#if defined(CONFIG_FB)
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include "huawei_ts_kit.h"
#include "huawei_ts_kit_misc_dev.h"
//#ifdef CONFIG_HUAWEI_HW_DEV_DCT
//#include <huawei_platform/devdetect/hw_dev_dec.h>
//#endif
//#include <linux/mfd/hisi_hi6xxx_pmic.h>
//#include <linux/hisi/hi6xxx-lcd_type.h>
#include "tpkit_platform_adapter.h"
#include "huawei_ts_kit_api.h"
#include "huawei_ts_kit_algo.h"
#include <linux/hwspinlock.h>
#include "hwspinlock_internal.h"

#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif

#if defined (CONFIG_HISI_BCI_BATTERY)
#include <linux/power/hisi/hisi_bci_battery.h>
#endif

#include "hostprocessing/huawei_thp_attr.h"

#define SCHEDULE_DELAY_MILLiSECOND      200
#define PROJECT_ID_LEN  10
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>

#define LDO17_PHYS_ADDR		(0X93)
#define LSW50_PHYS_ADDR	(0xAC)

static struct dsm_dev dsm_tp = {
	.name = "dsm_tp",
	.device_name = "TP",
	.ic_name = "syn",	/*just for testing, waiting for the module owner revised*/
	.module_name = "NNN",
	.fops = NULL,
	.buff_size = 1024,
};

struct dsm_client *ts_dclient =NULL;
EXPORT_SYMBOL(ts_dclient);
#endif
#define	EDGE_WIDTH_DEFAULT	10
struct ts_kit_platform_data g_ts_kit_platform_data;
EXPORT_SYMBOL(g_ts_kit_platform_data);
#if defined (CONFIG_TEE_TUI)
struct ts_tui_data  tee_tui_data;
EXPORT_SYMBOL(tee_tui_data);
#endif
u8 g_ts_kit_log_cfg = 0;

static struct ts_cmd_node ping_cmd_buff;
static struct ts_cmd_node pang_cmd_buff;
static struct work_struct tp_init_work;
struct mutex  ts_kit_easy_wake_guesure_lock;
/*external variable declare*/
extern const struct attribute_group ts_attr_group;
extern atomic_t g_ts_kit_data_report_over;
//extern int (*get_tp_gpio_num)(void);
/*global variable declare*/
volatile int  not_get_special_tp_node  =0;
#ifdef CONFIG_HUAWEI_THP
extern int thp_project_id_provider(char* project_id);
#endif

bool tp_get_prox_status(void)
{
	if (!g_ts_kit_platform_data.node) {
#ifdef CONFIG_HUAWEI_THP
		return thp_get_prox_switch_status();
#else
		return 0;
#endif
	} else {
		TS_LOG_INFO("[Proximity_feature] %s: It's tskit1.0 driver, not support proximity feature!\n",
			__func__);
		return 0;
	}
}

static int tskit_get_project_id(char* project_id)
{
	if (TS_REGISTER_DONE != atomic_read(&g_ts_kit_platform_data.register_flag)) {
		TS_LOG_ERR("%s not registered, return!!\n", __func__);
		return -EINVAL;
	}
	if(! g_ts_kit_platform_data.chip_data) {
		TS_LOG_ERR("%s  chip data is NULL\n", __func__);
		return -EBUSY;
	}
	if (g_ts_kit_platform_data.chip_data->project_id[0] == 0) {
		TS_LOG_ERR("%s project_id not initialed, return!!\n", __func__);
		return -EIO;
	}

	memcpy(project_id, g_ts_kit_platform_data.chip_data->project_id, PROJECT_ID_LEN);

	TS_LOG_INFO("%s, project id: %s.\n", __func__, g_ts_kit_platform_data.chip_data->project_id);
	return 0;
}

int tp_project_id_provider(char* project_id, uint8_t len)
{
	if (NULL == project_id) {
		TS_LOG_ERR("%s null pointer error!!\n", __func__);
		return -EINVAL;
	}

	if (len < PROJECT_ID_LEN) {
		TS_LOG_ERR("%s len is too small!!\n", __func__);
		return -EINVAL;
	}

	if( g_ts_kit_platform_data.node) {
		TS_LOG_INFO("%s is tskit project\n", __func__);
		return tskit_get_project_id(project_id);
	} else {
#ifdef CONFIG_HUAWEI_THP
		return thp_project_id_provider(project_id) ;
#else
		return -EIO;
#endif
	}
	TS_LOG_INFO("%s get project_id fail!!\n", __func__);
	return 0;
}
EXPORT_SYMBOL(tp_project_id_provider);

int ts_kit_get_esd_status(void)
{
	int ret = 0;

	ret = atomic_read(&g_ts_kit_platform_data.ts_esd_state);

	return ret;
}
EXPORT_SYMBOL(ts_kit_get_esd_status);

void ts_kit_clear_esd_status(void)
{
	atomic_set(&g_ts_kit_platform_data.ts_esd_state, TS_NO_ESD);
}
EXPORT_SYMBOL(ts_kit_clear_esd_status);
static void tp_init_work_fn(struct work_struct *work);
int ts_kit_proc_command_directly(struct ts_cmd_node *cmd);
static int ts_get_brightness_info_cmd(void);
void lcd_huawei_ts_kit_register(struct tp_kit_device_ops *tp_kit_device_ops);

struct tp_kit_device_ops ts_kit_ops = {
    .tp_thread_stop_notify = ts_kit_thread_stop_notify,
};
/* The following is a stub function. For hisilicon platform, it will be redefined in sensorhub module.
For qualcomm platform, it has not been implemented. Thus the stub function can avoid compilation errors.*/
__attribute__((weak)) int tpmodule_notifier_call_chain(unsigned long val, void *v)
{
	TS_LOG_INFO("No provide panel_id for sensor \n");
	return 0;
}
static void ts_panel_id_work_fn(struct work_struct *work)
{
	u8 panel_id = 0xF;

	panel_id = g_ts_kit_platform_data.panel_id;
	tpmodule_notifier_call_chain(panel_id, NULL);
	TS_LOG_INFO("%s : panel_id[%d] call_back exit \n", __func__, panel_id);
}
static DECLARE_WORK(ts_panel_id_work,ts_panel_id_work_fn);

static void ts_touch_switch_cmd(void)
{
	struct ts_kit_device_data *dev
		= g_ts_kit_platform_data.chip_data;

	TS_LOG_DEBUG("+\n");
	if (dev
		&& dev->ops
		&& dev->ops->chip_touch_switch) {
		TS_LOG_INFO("chip set touch switch\n");
		dev->ops->chip_touch_switch();
	}
	TS_LOG_DEBUG("-\n");
	return ;
}

static int seq_print_freq(struct seq_file *m, char *buf, int tx_num, int rx_num)
{
	char ii, jj;
	unsigned char *head;
	unsigned char *report_data_8;

	head = (unsigned char*)buf;

	seq_printf(m, "Calibration Image - Coarse & Fine\n");
	report_data_8 = (unsigned char*)buf;
	report_data_8++;	//point to second byte of F54 report data
	for (ii = 0; ii < rx_num; ii++)
	{
		for (jj = 0; jj < tx_num; jj++)
		{
			seq_printf(m, "%02x ", *report_data_8);

			report_data_8 +=2;

		}
		seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Image - Detail\n");
	report_data_8 = head;
	for (ii = 0; ii < rx_num; ii++)
	{
		for (jj = 0; jj < tx_num; jj++)
		{
			seq_printf(m, "%02x ", *report_data_8);

			report_data_8 +=2;

		}
		seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Noise - Coarse & Fine\n");
	report_data_8 = (unsigned char*)buf;	//point to first byte of data
	report_data_8 += (tx_num * rx_num *2 + 1);
	for (ii = 0; ii < rx_num * 2 ; ii++)
	{
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 +=2;

		if ((ii+1) % tx_num == 0 )
			seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Noise - Detail\n");
	report_data_8 = (unsigned char*)buf;
	report_data_8 += (tx_num * rx_num *2);
	for (ii = 0; ii < rx_num * 2; ii++)
	{
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 +=2;

		if ((ii+1) % tx_num == 0 )
			seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration button - Coarse & Fine\n");
	report_data_8 = (unsigned char*)buf;
	report_data_8 += (tx_num * rx_num *2 + rx_num *4 + 1);
	for (ii = 0; ii < 4; ii++)
	{
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 +=2;
	}

	seq_printf(m, "\nCalibration button - Detail\n");
	report_data_8 = (unsigned char*)buf;
	report_data_8 += (tx_num * rx_num *2 + rx_num *4);
	for (ii = 0; ii < 4; ii++)
	{
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 +=2;
	}

	return 0;
}

static int calibration_proc_show(struct seq_file *m, void *v)
{
	struct ts_calibration_data_info *info = NULL;
	struct ts_cmd_node *cmd = NULL;
	int error = NO_ERR;

	if (!g_ts_kit_platform_data.chip_data->should_check_tp_calibration_info) {
		TS_LOG_ERR("No calibration data.\n");
		error = NO_ERR;
		goto out;
	}

	cmd = (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node), GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info = (struct ts_calibration_data_info *)kzalloc(sizeof(struct
				ts_calibration_data_info), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out_free_cmd;
	}

	cmd->command = TS_READ_CALIBRATION_DATA;
	cmd->cmd_param.prv_params = (void *)info;

	if( g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) {
		error = ts_kit_proc_command_directly(cmd);
	}else{
		error = ts_kit_put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out_free_cmd;
	}

	seq_write(m, info->data, info->used_size);

	seq_printf(m, "\n\nCollect data for freq: 0\n\n");
	seq_print_freq(m, info->data, info->tx_num, info->rx_num);

	seq_printf(m, "\n\nCollect data for freq: 1\n\n");
	seq_print_freq(m,
			info->data
			+ info->tx_num * info->rx_num * 2	/* shift 2D */
			+ info->rx_num * 2 * 2	/* shift noise */
			+ 4 * 2,		/* shift 0D */
			info->tx_num, info->rx_num);

	seq_printf(m, "\n\nCollect data for freq: 2\n\n");
	seq_print_freq(m,
			info->data
			+ (info->tx_num * info->rx_num * 2	/* shift 2D */
			+ info->rx_num * 2 * 2	/* shift noise */
			+ 4 * 2) * 2,		/* shift 0D */
			info->tx_num, info->rx_num);

	seq_printf(m, "\n\nCollect data for interval scan\n\n");
	seq_print_freq(m, info->data+2048, info->tx_num, info->rx_num);

	kfree(info);
out_free_cmd:
	kfree(cmd);
out:
	return 0;
}

void ts_kit_rotate_rawdata_abcd2cbad(int row, int column, int *data_start, int rotate_type)
{
	int *rawdatabuf_temp = NULL;
	int row_index, column_index;
	int row_size = 0;
	int column_size = 0;
	int i = 0;

	TS_LOG_INFO("\n");
	rawdatabuf_temp =
	    (int *)kzalloc(row * column * sizeof(int), GFP_KERNEL);
	if (!rawdatabuf_temp) {
		TS_LOG_ERR("Failed to alloc buffer for rawdatabuf_temp\n");
		return;
	}

	memcpy(rawdatabuf_temp, data_start, row * column * sizeof(int));
	switch (rotate_type) {
		case TS_RAWDATA_TRANS_NONE:
			break;
		case TS_RAWDATA_TRANS_ABCD2CBAD:
			/* src column to dst row*/
			row_size = column;
			column_size = row;
			for (column_index = column_size - 1; column_index >= 0; column_index--) {
				for (row_index = row_size - 1; row_index >= 0; row_index--) {
					data_start[i++] =
					    rawdatabuf_temp[row_index * column_size + column_index];
				}
			}
			break;
		case TS_RAWDATA_TRANS_ABCD2ADCB:
			/* src column to dst row*/
			row_size = column;
			column_size = row;
			for (column_index = 0; column_index < column_size; column_index++) {
				for (row_index = 0; row_index < row_size; row_index++) {
					data_start[i++] =
					    rawdatabuf_temp[row_index * column_size +
							    column_index];
				}
			}
			break;
		default:
			break;
	}
	if (rawdatabuf_temp) {
		kfree(rawdatabuf_temp);
		rawdatabuf_temp = NULL;
	}
	return;
}
EXPORT_SYMBOL(ts_kit_rotate_rawdata_abcd2cbad);

static void rawdata_proc_printf(struct seq_file *m, struct ts_rawdata_info *info,
					int range_size, int row_size)
{
	int index = 0;
	int index1 = 0;
	int index2 = 0;
	int rx_num = info->hybrid_buff[0];
	int tx_num = info->hybrid_buff[1];

	if((0 == range_size) || (0 == row_size)) {
		TS_LOG_ERR("%s  range_size OR row_size is 0\n", __func__);
		return;
	}
	for (index = 0; row_size * index + 2 < info->used_size; index++) {
		if (0 == index) {
			seq_printf(m, "rawdata begin\n");	/*print the title */
		}
		for (index1 = 0; index1 < row_size; index1++) {
			seq_printf(m, "%d,", info->buff[2 + row_size * index + index1]);	/*print oneline */
		}
		/*index1 = 0;*/
		seq_printf(m, "\n ");

		if ((range_size - 1) == index) {
			seq_printf(m, "rawdata end\n");
			seq_printf(m, "noisedata begin\n");
		}
	}
	seq_printf(m, "noisedata end\n");


	index1 =tx_num+rx_num+2;

	/*********hybrid_buff   |rx_num|tx_num|noize_rx|noize_tx|raw_rx|raw_tx|****/
	if((index1 <= TS_RAWDATA_RESULT_MAX/2)
		&& g_ts_kit_platform_data.chip_data->self_cap_test){
		seq_printf(m, "selfcap rawdata begin\n");	/*print the title */
		seq_printf(m, "rx:\n");
		for (index = 0;  index < rx_num; index++) {
			seq_printf(m, "%d,", info->hybrid_buff[index1 + index]);	/*print  rx oneline */
		}
		seq_printf(m, "\ntx:\n");
		for (index = 0; index < tx_num; index++) {
			seq_printf(m, "%d,", info->hybrid_buff[index1 + index+rx_num]);	/*print tx oneline */
		}
		seq_printf(m, "\nselfcap rawdata end\n");

		seq_printf(m, "selfcap noisedata begin\n");
		seq_printf(m, "rx:\n");
		for (index =0;  index < rx_num; index++) {
			seq_printf(m, "%d,", info->hybrid_buff[2 + index]);	/*print oneline */
		}
		seq_printf(m, "\ntx:\n");
		for (index =0; index < tx_num; index++) {
			seq_printf(m, "%d,", info->hybrid_buff[2 + index + rx_num]); /*print oneline */
		}
		seq_printf(m, "\nselfcap noisedata end\n");
	}

	if(g_ts_kit_platform_data.chip_data->forcekey_test_support)
	{
		if(info->hybrid_buff_used_size > (index1 + tx_num + rx_num))
		{
			index2 = info->hybrid_buff_used_size - (index1 + tx_num + rx_num);
			seq_printf(m, "\nforcekey value : ");
			for (index = 0; index < index2; index++){
				seq_printf(m, "%d, ", info->hybrid_buff[index + index1 + tx_num + rx_num ]);
			}
			seq_printf(m, "\n");
		}
	}

	if(info->used_sharp_selcap_single_ended_delta_size) {
		seq_printf(m, "selfcap touchdelta begin\n");
		for (index = 0; index < info->used_sharp_selcap_touch_delta_size; index++) {
			seq_printf(m, "%d,", info->buff[info->used_size + index]);
		}
		seq_printf(m, "\n ");
		seq_printf(m, "selfcap touchdelta end\n");
		seq_printf(m, "selfcap singleenddelta begin\n");
		for (index = 0; index < info->used_sharp_selcap_single_ended_delta_size; index++) {
			seq_printf(m, "%d,", info->buff[info->used_size +
					info->used_sharp_selcap_touch_delta_size + index]);
		}
		seq_printf(m, "\n ");
		seq_printf(m, "selfcap singleenddelta end\n");
	}
	if (g_ts_kit_platform_data.chip_data->trx_delta_test_support) {
			seq_printf(m, "%s\n", info->tx_delta_buf);
			seq_printf(m, "%s\n", info->rx_delta_buf);
	}
	if (g_ts_kit_platform_data.chip_data->td43xx_ee_short_test_support) {
			seq_printf(m, "%s\n", info->td43xx_rt95_part_one);
			seq_printf(m, "%s\n", info->td43xx_rt95_part_two);
	}
	return;
}

static int rawdata_proc_parade_printf(struct seq_file *m, struct ts_rawdata_info *info,
					int range_size, int row_size)
{
	int rdIndex = RAWDATA_SIZE_LIMIT;
	int index = 0;
	int index1 = 0;

	seq_printf(m, "cm data begin\n");	/*print the title */
	for(index = 0; index < range_size; index++){
		for (index1 = 0; index1 < row_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
	}
	seq_printf(m, "cm data end\n");	/*print the title */
	seq_printf(m, "mutual noise data begin\n");	/*print the title */
	for(index = 0; index < range_size; index++){
		for (index1 = 0; index1 < row_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
	}
	seq_printf(m, "mutual noise data end\n");	/*print the title */
	seq_printf(m, "self noise data begin\n");	/*print the title */
	seq_printf(m, "-rx:,");
	for (index1 = 0; index1 < row_size; index1++){
		if(rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else{
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "-tx:,");
	for (index1 = 0; index1 < range_size; index1++){
		if(rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else{
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "self noise data end\n");	/*print the title */
	seq_printf(m, "cm gradient(10x real value) begin\n");	/*print the title */
	seq_printf(m, "-rx:,");
	for (index1 = 0; index1 < row_size; index1++){
		if(rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else{
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "-tx:,");
	for (index1 = 0; index1 < range_size; index1++){
		if(rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else{
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "cm gradient end\n");	/*print the title */
	seq_printf(m, "cp begin\n");	/*print the title */
	seq_printf(m, "-rx:,");
	for (index1 = 0; index1 < row_size; index1++){
		if(rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else{
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "-tx:,");
	for (index1 = 0; index1 < range_size; index1++){
		if(rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else{
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "cp end\n");	/*print the title */
	seq_printf(m, "cp delta begin\n");	/*print the title */
	seq_printf(m, "-rx:,");
	for (index1 = 0; index1 < row_size; index1++){
		if(rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else{
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "-tx:,");
	for (index1 = 0; index1 < range_size; index1++){
		if(rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else{
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "cp detlat end\n");	/*print the title */
	seq_printf(m, "*************end data*************\n");

	return NO_ERR;
out:
	return RESULT_ERR;
}

static void rawdata_proc_3d_func_printf(struct seq_file *m, struct ts_rawdata_info *info)
{
	int index = 0;
	int index1 = 0;
	int row_size = 0;
	int range_size = 0;

	TS_LOG_INFO("print 3d data\n");
	row_size = info->buff_3d[0];
	range_size = info->buff_3d[1];

	if(0 == row_size) {
		TS_LOG_ERR("%s, row_size = %d\n", __func__,row_size);
		return;
	}

	seq_printf(m, "rx: %d, tx : %d(3d)\n", row_size, range_size);

	for (index=0; row_size*index+2 < info->used_size_3d; index++) {
		if (0 == index) {
			seq_printf(m, "rawdata begin(3d)\n");							/*print the title*/
		}
		for (index1=0; index1 < row_size; index1++) {
			seq_printf(m, "%d,", info->buff_3d[2+row_size*index+index1]);		/*print oneline*/
		}
		//index1 = 0;
		seq_printf(m, "\n ");

		if ((range_size -1) == index) {
			seq_printf(m, "rawdata end(3d)\n");
			seq_printf(m, "noisedata begin(3d)\n");
		}
	}
	seq_printf(m, "noisedata end(3d)\n");
	return;
}

static void rawdata_proc_newformat_printf(struct seq_file *m, struct ts_rawdata_info_new *info)
{
	struct ts_rawdata_newnodeinfo * rawdatanode = NULL;
	int index = 0;
	int index1 = 0;
	int row_size = info->tx;
	int tx_n = 0, rx_n = 0;
	int rawtest_size = info->tx * info->rx;
	char pfstatus[RAW_DATA_END]={0};//0-NA,'P' pass,'F' false
	char resulttemp[TS_RAWDATA_RESULT_CODE_LEN]={0};

	TS_LOG_INFO("%s : devinfo:%s,nodenum:%d,alllength:%d\n",__func__,info->deviceinfo,info->listnodenum);

	/* i2c info */
	seq_printf(m, "%s",info->i2cinfo);
	seq_printf(m, "%s","-");

	/* row data p or f */
	list_for_each_entry(rawdatanode, &info->rawdata_head, node){
	    if (rawdatanode->typeindex < RAW_DATA_END){
			if (pfstatus[rawdatanode->typeindex] == 0 || pfstatus[rawdatanode->typeindex]=='P'){
				pfstatus[rawdatanode->typeindex] = rawdatanode->testresult;
			}
    	}
	}
	list_for_each_entry(rawdatanode, &info->rawdata_head, node){
		if (rawdatanode->typeindex < RAW_DATA_END){
			if(pfstatus[rawdatanode->typeindex] != 0){
				resulttemp[0] = rawdatanode->typeindex + '0';
				resulttemp[1] = pfstatus[rawdatanode->typeindex]; //default result_code is failed
				resulttemp[2] = '\0';
				seq_printf(m, "%s",resulttemp);
				seq_printf(m, "%s","-");
				pfstatus[rawdatanode->typeindex] = 0;
			}
		}
	}
	/* statistics_data info */
	list_for_each_entry(rawdatanode, &info->rawdata_head, node){
	    if (strlen(rawdatanode->statistics_data) > 0){
			seq_printf(m, "%s",rawdatanode->statistics_data);
    	}
	}

	/* result info */
	if (strlen(info->i2cerrinfo)>0){
		resulttemp[0] = RAW_DATA_TYPE_IC + '0';
		resulttemp[1] = ':';
		resulttemp[2] = '\0';
		seq_printf(m, "%s",resulttemp);
		seq_printf(m, "%s",info->i2cerrinfo);
		seq_printf(m, "%s","-");
	}
	list_for_each_entry(rawdatanode, &info->rawdata_head, node){
		if (strlen(rawdatanode->tptestfailedreason)>0){
			resulttemp[0] = rawdatanode->typeindex + '0';
			resulttemp[1] = ':';
			resulttemp[2] = '\0';
			seq_printf(m, "%s",resulttemp);
			seq_printf(m, "%s",rawdatanode->tptestfailedreason);
			seq_printf(m, "%s","-");
		}
	}

	/* dev info */
	seq_printf(m, "%s",info->deviceinfo);
	seq_printf(m, "\n");
	seq_printf(m, "*************touch data*************\n");
	seq_printf(m, "tx: %d, rx : %d\n", info->tx, info->rx);
    list_for_each_entry(rawdatanode, &info->rawdata_head, node){
		if(rawdatanode->size > 0)
        seq_printf(m, "%s begin\n",rawdatanode->test_name);
		if(rawdatanode->typeindex == RAW_DATA_TYPE_TrxDelta && rawdatanode->size > 0){
			seq_printf(m, "RX:\n");
			for(index = 0; index < (rawtest_size - info->tx); index++) {

				seq_printf(m, "%d,", rawdatanode->values[index]);
				tx_n++;
				if(tx_n == info->rx-1){
					seq_printf(m, "\n");
					tx_n = 0;
				}
			}
			seq_printf(m, "\nTX:\n");
			for(index = 0; index < (rawtest_size - info->rx); index++) {
				seq_printf(m, "%d,", rawdatanode->values[rawtest_size + index]);
				rx_n++;
				if(rx_n == info->rx){
					seq_printf(m, "\n");
					rx_n = 0;
				}
			}
			seq_printf(m, "\n");
		}
		else if (rawdatanode->typeindex == RAW_DATA_TYPE_SelfCap && rawdatanode->size > 0){
			seq_printf(m, "rx:\n");
			for (index = 0;  index < info->rx; index++) {
				seq_printf(m, "%d,", rawdatanode->values[index]);	/*print  rx oneline */
			}
			seq_printf(m, "\ntx:\n");
			for (index = 0; index < info->tx; index++) {
				seq_printf(m, "%d,", rawdatanode->values[index + info->rx]);	/*print tx oneline */
			}
			seq_printf(m, "\n");
		} else if (rawdatanode->typeindex == RAW_DATA_TYPE_SelfNoisetest && rawdatanode->size > 0){
			seq_printf(m, "rx:\n");
			for (index =0;	index < info->rx; index++) {
				seq_printf(m, "%d,", rawdatanode->values[index]); /*print oneline */
			}
			seq_printf(m, "\ntx:\n");
			for (index =0; index < info->tx; index++) {
				seq_printf(m, "%d,", rawdatanode->values[index + info->rx]); /*print oneline */
			}
			seq_printf(m, "\n");
		} else {
			for (index = 0; row_size * index < rawdatanode->size; index++) {
				for (index1 = 0; (index1 < row_size)&&((row_size * index + index1)<rawdatanode->size); index1++) {
					seq_printf(m, "%d,", rawdatanode->values[row_size * index + index1]);	/*print oneline */
				}
				/*index1 = 0;*/
				seq_printf(m, "\n");
			}
		}
		if(rawdatanode->size > 0)
		seq_printf(m, "%s end\n",rawdatanode->test_name);
    }
	return;
}
void rawdata_proc_freehook(void * infotemp){
	struct ts_rawdata_info_new *info = infotemp;
	struct list_head *pos, *n;
	struct ts_rawdata_newnodeinfo * rawdatanode = NULL;

	if (info) {

		list_for_each_safe(pos, n, &info->rawdata_head) {
			rawdatanode = list_entry(pos, struct ts_rawdata_newnodeinfo, node);
			if (rawdatanode->values){
				kfree(rawdatanode->values);
	            rawdatanode->values = NULL;
			}
			list_del(pos);
			kfree(rawdatanode);
			rawdatanode = NULL;
		}
		kfree(info);
		info = NULL;
		TS_LOG_DEBUG("%s, free deal ok\n", __func__);
	}
	return;
}
/*lint -save -e* */
static int rawdata_proc_for_newformat(struct seq_file *m, void *v)
{
	struct ts_cmd_node *cmd = NULL;
	struct ts_rawdata_info_new *info = NULL;
	int error = NO_ERR;
	int error2 = NO_ERR;
	
	TS_LOG_INFO("rawdata_proc_for_newformat, buffer size = %ld\n", m->size);
	if(m->size <= RAW_DATA_SIZE) {
		m->count = m->size;
		return 0;
	}
	
	cmd = (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node), GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	info = (struct ts_rawdata_info_new *)kzalloc(sizeof(struct ts_rawdata_info_new), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	INIT_LIST_HEAD(&info->rawdata_head);
	info->status = TS_ACTION_UNDEF;
	cmd->command = TS_READ_RAW_DATA;
	cmd->cmd_param.prv_params = (void *)info;

	if(g_ts_kit_platform_data.chip_data->is_direct_proc_cmd){
		error = ts_kit_proc_command_directly(cmd);
	}
	else{
		if (g_ts_kit_platform_data.chip_data->rawdata_get_timeout)
			error = ts_kit_put_one_cmd(cmd, g_ts_kit_platform_data.chip_data->rawdata_get_timeout);
		else
			error = ts_kit_put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}
	/*
	 If the error code is -EBUSY, said timeout, info release in ts_thread
	*/
	if (error == -EBUSY) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		goto out;
	}
	
	if (info->status != TS_ACTION_SUCCESS 
		  || error) {
		TS_LOG_ERR("read action failed\n");
		error = -EIO;
		goto out;
	}
	
	/*
	 *1.Start writing data to the node, 
	 */	
	rawdata_proc_newformat_printf(m, info);	
out:	
	if (info) {		
		cmd->command = TS_FREEBUFF;
		cmd->cmd_param.prv_params = (void *)info;
		cmd->cmd_param.ts_cmd_freehook = rawdata_proc_freehook;
		error2 = ts_kit_put_one_cmd(cmd, NO_SYNC_TIMEOUT);
		if (error2 != NO_ERR) {
			TS_LOG_ERR("put free cmd error :%d\n", error2);
		}
		info = NULL;
	}
	if (cmd) {
		kfree(cmd);
		cmd = NULL;
	}
	return NO_ERR;
}
/*lint -restore*/
/*lint -save -e* */
static int rawdata_proc_show(struct seq_file *m, void *v)
{
	short row_size = 0;
	int range_size = 0;
	int error = NO_ERR;
	int tx_rx_delta_size = 0;
	int trx_delta_test_support = g_ts_kit_platform_data.chip_data->trx_delta_test_support;
	int td43xx_ee_short_test_support = g_ts_kit_platform_data.chip_data->td43xx_ee_short_test_support;
	struct ts_cmd_node *cmd = NULL;
	struct ts_rawdata_info *info = NULL;

	/**********************************************/
	/* Rawdata rectification, if dts configured   */
	/* with a new mark, take a new process        */
	/**********************************************/
    if (g_ts_kit_platform_data.chip_data->rawdata_newformatflag == TS_RAWDATA_NEWFORMAT){
		return rawdata_proc_for_newformat(m,v);		
	}
	TS_LOG_INFO("rawdata_proc_show, buffer size = %ld\n", m->size);
	if(m->size <= RAW_DATA_SIZE) {
		m->count = m->size;
		return 0;
	}

	cmd = (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node), GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info = (struct ts_rawdata_info *)vmalloc(sizeof(struct ts_rawdata_info));
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	memset(info, 0,sizeof(struct ts_rawdata_info) );
	if (!g_ts_kit_platform_data.chip_data->tx_num || !g_ts_kit_platform_data.chip_data->rx_num) {
		tx_rx_delta_size = TX_RX_BUF_MAX;
	} else {
		tx_rx_delta_size = g_ts_kit_platform_data.chip_data->tx_num
			* g_ts_kit_platform_data.chip_data->rx_num
			* MAX_CAP_DATA_SIZE;
	}

	TS_LOG_INFO("tx:%d, rx:%d, tx_rx_delta_size:%d\n",
		g_ts_kit_platform_data.chip_data->tx_num,
		g_ts_kit_platform_data.chip_data->rx_num,
		tx_rx_delta_size);

	if (trx_delta_test_support) {
		info->tx_delta_buf = (int *)kzalloc(tx_rx_delta_size, GFP_KERNEL);
		if (!info->tx_delta_buf) {
			TS_LOG_ERR("malloc failed\n");
			error = -ENOMEM;
			goto out;
		}
		info->rx_delta_buf = (int *)kzalloc(tx_rx_delta_size, GFP_KERNEL);
		if (!info->rx_delta_buf) {
			TS_LOG_ERR("malloc failed\n");
			error = -ENOMEM;
			goto out;
		}
	}

	if (td43xx_ee_short_test_support) {
		info->td43xx_rt95_part_one = (signed int *)kzalloc(tx_rx_delta_size, GFP_KERNEL);
		if (!info->td43xx_rt95_part_one) {
			TS_LOG_ERR("malloc td43xx_rt95_part_one failed\n");
			error = -ENOMEM;
			goto out;
		}
		info->td43xx_rt95_part_two = (signed int *)kzalloc(tx_rx_delta_size, GFP_KERNEL);
		if (!info->td43xx_rt95_part_two) {
			TS_LOG_ERR("malloc td43xx_rt95_part_two failed\n");
			error = -ENOMEM;
			goto out;
		}
	}

	info->used_size = 0;
	info->used_sharp_selcap_single_ended_delta_size = 0;
	info->used_sharp_selcap_touch_delta_size = 0;
	info->status = TS_ACTION_UNDEF;
	cmd->command = TS_READ_RAW_DATA;
	cmd->cmd_param.prv_params = (void *)info;

	if(g_ts_kit_platform_data.chip_data->is_direct_proc_cmd){
		error = ts_kit_proc_command_directly(cmd);
	}
	else{
		if (g_ts_kit_platform_data.chip_data->rawdata_get_timeout)
			error = ts_kit_put_one_cmd(cmd, g_ts_kit_platform_data.chip_data->rawdata_get_timeout);
		else
			error = ts_kit_put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}
	if(!g_ts_kit_platform_data.chip_data->is_parade_solution){
		if (error) {
			TS_LOG_ERR("put cmd error :%d\n", error);
			goto free_cmd;
		}
	}
	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("read action failed\n");
		error = -EIO;
		goto out;
	}
	seq_printf(m, "%s\n", info->result);
	seq_printf(m, "*************touch data*************\n");

	if (g_ts_kit_platform_data.chip_data->rawdata_arrange_swap) {
		row_size = info->buff[1];
		range_size = info->buff[0];
	}else{
		row_size = info->buff[0];
		range_size = info->buff[1];
	}

	if (g_ts_kit_platform_data.chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2CBAD
		|| g_ts_kit_platform_data.chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2ADCB) {
		ts_kit_rotate_rawdata_abcd2cbad(row_size, range_size, info->buff + 2,
				g_ts_kit_platform_data.chip_data->rawdata_arrange_type);
		ts_kit_rotate_rawdata_abcd2cbad(row_size, range_size,
				info->buff + 2 + row_size * range_size, g_ts_kit_platform_data.chip_data->rawdata_arrange_type);
		row_size = info->buff[1];
		range_size = info->buff[0];
	}
	seq_printf(m, "rx: %d, tx : %d\n", row_size, range_size);
	if(g_ts_kit_platform_data.chip_data->is_parade_solution == 0){//Not Parade Solution, use default
		if(g_ts_kit_platform_data.chip_data->is_ic_rawdata_proc_printf == 1){
			if (!g_ts_kit_platform_data.chip_data->ops) {
				TS_LOG_ERR("ops is NULL\n");
				error = -ENOMEM;
				goto out;
			}
			if(g_ts_kit_platform_data.chip_data->ops->chip_special_rawdata_proc_printf){
				g_ts_kit_platform_data.chip_data->ops->chip_special_rawdata_proc_printf(m, info, range_size, row_size);
			}
		}else{
			rawdata_proc_printf(m, info, range_size, row_size);
		}
	} else {
		if (rawdata_proc_parade_printf(m, info, range_size, row_size) < 0) {
			goto out;
		}
	}

	if(g_ts_kit_platform_data.chip_data->support_3d_func) {
		rawdata_proc_3d_func_printf(m, info);
	}

	error = NO_ERR;

out:
	if (info) {
		if (trx_delta_test_support) {
			if (info->rx_delta_buf) {
				kfree(info->rx_delta_buf);
				info->rx_delta_buf = NULL;
			}
			if (info->tx_delta_buf) {
				kfree(info->tx_delta_buf);
				info->tx_delta_buf = NULL;
			}
		}
		if (td43xx_ee_short_test_support) {
			if (info->td43xx_rt95_part_one) {
				kfree(info->td43xx_rt95_part_one);
				info->td43xx_rt95_part_one = NULL;
			}
			if (info->td43xx_rt95_part_two) {
				kfree(info->td43xx_rt95_part_two);
				info->td43xx_rt95_part_two = NULL;
			}
		}
		vfree(info);
		info = NULL;
	}

free_cmd:
	if (cmd) {
		kfree(cmd);
		cmd = NULL;
	}

	return error;
}
/*lint -restore*/
static int calibration_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, calibration_proc_show, NULL);
}

static const struct file_operations calibration_proc_fops = {
	.open		= calibration_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
static int rawdata_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rawdata_proc_show, NULL);
}
static const struct file_operations rawdata_proc_fops =
{
    .open     = rawdata_proc_open,
    .read     = seq_read,
    .llseek       = seq_lseek,
    .release  = single_release,
};
/*external function declare*/
extern int i2c_check_addr_busy(struct i2c_adapter* adapter, int addr);

#if defined (CONFIG_TEE_TUI)
extern int i2c_init_secos(struct i2c_adapter *adap);
extern int i2c_exit_secos(struct i2c_adapter *adap);
extern int spi_exit_secos(unsigned int spi_bus_id);
extern int spi_init_secos(unsigned int spi_bus_id);
#endif

void ts_stop_wd_timer(struct ts_kit_platform_data* cd);
void ts_kit_thread_stop_notify(void);

int ts_i2c_write(u8* buf, u16 length);
int ts_spi_write(u8* buf, u16 length);
int ts_i2c_read(u8* reg_addr, u16 reg_len, u8* buf, u16 len);
int ts_spi_read(u8* reg_addr, u16 reg_len, u8* buf, u16 len);



static struct ts_bus_info ts_bus_i2c_info =
{
    .btype      = TS_BUS_I2C,
    .bus_write  = ts_i2c_write,
    .bus_read   = ts_i2c_read,
};

static struct ts_bus_info ts_bus_spi_info =
{
    .btype      = TS_BUS_SPI,
    .bus_write  = ts_spi_write,
    .bus_read   = ts_spi_read,
};

#if defined (CONFIG_HUAWEI_DSM)
void ts_i2c_error_dmd_report(u8* reg_addr)
{
    if(atomic_read(&g_ts_kit_platform_data.power_state) == TS_UNINIT){
        TS_LOG_INFO("ts_dirver_don't_init,no need report dmd\n");
        return;
    }
    if ((atomic_read(&g_ts_kit_platform_data.power_state) == TS_SLEEP)
    ||(atomic_read(&g_ts_kit_platform_data.power_state) == TS_WORK_IN_SLEEP)
	|| (atomic_read(&g_ts_kit_platform_data.power_state) == TS_GOTO_SLEEP)){
        if(reg_addr == NULL){
            ts_dmd_report(DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO, "irq_gpio:%d;value:%d;reset_gpio:%d;value:%d;I2C_status:%d.\n",
                g_ts_kit_platform_data.irq_gpio, gpio_get_value(g_ts_kit_platform_data.irq_gpio),\
                g_ts_kit_platform_data.reset_gpio, gpio_get_value(g_ts_kit_platform_data.reset_gpio), \
                g_ts_kit_platform_data.dsm_info.constraints_I2C_status);
        }else{
            ts_dmd_report(DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO, "irq_gpio:%d;value:%d;reset_gpio:%d;value:%d;I2C_status:%d;addr:%d.\n",
                g_ts_kit_platform_data.irq_gpio, gpio_get_value(g_ts_kit_platform_data.irq_gpio),\
                g_ts_kit_platform_data.reset_gpio, gpio_get_value(g_ts_kit_platform_data.reset_gpio), \
                g_ts_kit_platform_data.dsm_info.constraints_I2C_status,*reg_addr);
          }
    }else{
        if(reg_addr == NULL){
            ts_dmd_report(DSM_TP_I2C_RW_ERROR_NO, "irq_gpio:%d;value:%d;reset_gpio:%d;value:%d;I2C_status:%d.\n",
                g_ts_kit_platform_data.irq_gpio, gpio_get_value(g_ts_kit_platform_data.irq_gpio),\
                g_ts_kit_platform_data.reset_gpio, gpio_get_value(g_ts_kit_platform_data.reset_gpio), \
                g_ts_kit_platform_data.dsm_info.constraints_I2C_status);\
       }else{
            ts_dmd_report(DSM_TP_I2C_RW_ERROR_NO, "irq_gpio:%d value:%d reset_gpio:%d  value:%d. I2C_status:%d;addr:%d.\n",
                g_ts_kit_platform_data.irq_gpio, gpio_get_value(g_ts_kit_platform_data.irq_gpio),\
                g_ts_kit_platform_data.reset_gpio, gpio_get_value(g_ts_kit_platform_data.reset_gpio), \
                g_ts_kit_platform_data.dsm_info.constraints_I2C_status,*reg_addr);
       }
    }
}
#endif

#define GET_HWLOCK_FAIL   0

static int tp_i2c_get_hwlock(void)
{
	int ret = 0;
	unsigned long time = 0;
	unsigned long timeout = 0;
	struct hwspinlock *hwlock = NULL;

	hwlock = g_ts_kit_platform_data.i2c_hwlock.hwspin_lock;
	timeout = jiffies + msecs_to_jiffies(GET_HARDWARE_TIMEOUT);

	do{
		ret = hwlock->bank->ops->trylock(hwlock);
		if (GET_HWLOCK_FAIL == ret) {
			time = jiffies;
			if (time_after(time, timeout)) {
				TS_LOG_ERR(" i2c get hardware_mutex for completion timeout \n");
				return -ETIME;
			}
		}
	}while(GET_HWLOCK_FAIL == ret);

	return 0;
}

static void  tp_i2c_release_hwlock(void)
{
	struct hwspinlock *hwlock = NULL;
	hwlock = g_ts_kit_platform_data.i2c_hwlock.hwspin_lock;
       hwlock->bank->ops->unlock(hwlock);
	return;
}
int ts_i2c_write(u8* buf, u16 length)
{
    int count = 0;
    int ret;

#if defined (CONFIG_TEE_TUI)
	if (g_ts_kit_platform_data.chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif
	if(g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag){
		ret = tp_i2c_get_hwlock();
		if(ret){
			TS_LOG_ERR("i2c get hardware mutex failure\n");
			return -EAGAIN;
		}
	}
    do
    {
        ret = i2c_master_send(g_ts_kit_platform_data.client, (const char *)buf, length);
        if (ret == length)
        {
		if(g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag){
			 tp_i2c_release_hwlock();
		}
            return NO_ERR;
        }
#if defined (CONFIG_HUAWEI_DSM)
	else
		g_ts_kit_platform_data.dsm_info.constraints_I2C_status = ret;
#endif
        msleep(I2C_WAIT_TIME);
    }
    while (++count < I2C_RW_TRIES);

	if(g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag){
			 tp_i2c_release_hwlock();
	}
#if defined (CONFIG_HUAWEI_DSM)
    ts_i2c_error_dmd_report(&buf[0]);
#endif

    TS_LOG_ERR("ts_i2c_write failed\n");
    return -EIO;
}

int ts_change_spi_mode(struct spi_device *spi, u16 mode)
{
	int ret = 0;

	if (spi->mode != mode) {
		spi->mode = mode;
		ret = spi_setup(spi);
		if (ret){
			TS_LOG_ERR("%s setup spi failed.\n");
			return ret;
		}
	}

	return 0;
}

int ts_spi_write(u8* buf, u16 length)
{
    return NO_ERR;
}

int ts_i2c_read(u8* reg_addr, u16 reg_len, u8* buf, u16 len)
{
    int count = 0;
    int ret = 0;
    int msg_len = 0;
    struct i2c_msg *msg_addr = NULL;
    struct i2c_msg xfer[2];

#if defined (CONFIG_TEE_TUI)
	if (g_ts_kit_platform_data.chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif
	if(g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag){
		ret = tp_i2c_get_hwlock();
		if(ret){
			TS_LOG_ERR("i2c get hardware mutex failure\n");
			return -EAGAIN;
		}
	}
	if (g_ts_kit_platform_data.chip_data->is_i2c_one_byte) {
		/* Read data */
		xfer[0].addr = g_ts_kit_platform_data.client->addr;
		xfer[0].flags = I2C_M_RD;
		xfer[0].len = len;
		xfer[0].buf = buf;
		do {
			ret = i2c_transfer(g_ts_kit_platform_data.client->adapter, xfer, 1);
			if (ret == 1) {
				if(g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag){
					 tp_i2c_release_hwlock();
				}
				return NO_ERR;
			}
#if defined (CONFIG_HUAWEI_DSM)
			else
				g_ts_kit_platform_data.dsm_info.constraints_I2C_status = ret;
#endif

			msleep(I2C_WAIT_TIME);
		} while (++count < I2C_RW_TRIES);
	} else {
    /*register addr*/
    xfer[0].addr = g_ts_kit_platform_data.client->addr;
    xfer[0].flags = 0;
    xfer[0].len = reg_len;
    xfer[0].buf = reg_addr;

    /* Read data */
    xfer[1].addr = g_ts_kit_platform_data.client->addr;
    xfer[1].flags = I2C_M_RD;
    xfer[1].len = len;
    xfer[1].buf = buf;

    if (reg_len > 0) {
    	msg_len = 2;
    	msg_addr = &xfer[0];
    } else {
    	msg_len = 1;
    	msg_addr = &xfer[1];
    }
    do
    {
        ret = i2c_transfer(g_ts_kit_platform_data.client->adapter, msg_addr, msg_len);
        if (ret == msg_len)
        {
		if(g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag){
			 tp_i2c_release_hwlock();
		}
            return NO_ERR;
        }
#if defined (CONFIG_HUAWEI_DSM)
	else
		g_ts_kit_platform_data.dsm_info.constraints_I2C_status = ret;
#endif
			msleep(I2C_WAIT_TIME);
		} while (++count < I2C_RW_TRIES);
    }
	if(g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag){
		tp_i2c_release_hwlock();
	}
#if defined (CONFIG_HUAWEI_DSM)
    ts_i2c_error_dmd_report(reg_addr);
#endif
    TS_LOG_ERR("ts_i2c_read failed\n");
    return -EIO;
}

int ts_spi_read(u8* reg_addr, u16 reg_len, u8* buf, u16 len)
{
    return NO_ERR;
}
static irqreturn_t ts_irq_handler(int irq, void* dev_id)
{
    int error = NO_ERR;
    struct ts_cmd_node cmd;

    wake_lock_timeout(&g_ts_kit_platform_data.ts_wake_lock, HZ);

    if (g_ts_kit_platform_data.chip_data->ops->chip_irq_top_half)
    { error = g_ts_kit_platform_data.chip_data->ops->chip_irq_top_half(&cmd); }

    if (error)//unexpected error happen, put err cmd to ts thread
    { cmd.command = TS_INT_ERR_OCCUR; }
    else
    { cmd.command = TS_INT_PROCESS; }

    if (strncmp(g_ts_kit_platform_data.product_name,"ares",sizeof("ares")) ||
         strncmp(g_ts_kit_platform_data.chip_data->chip_name,"parade",sizeof("parade")))
    {
        disable_irq_nosync(g_ts_kit_platform_data.irq_id);
    }

    if (ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT) && (TS_UNINIT != atomic_read(&g_ts_kit_platform_data.state)))
    { enable_irq(g_ts_kit_platform_data.irq_id); }

    return IRQ_HANDLED;
}
#if defined (CONFIG_TEE_TUI)
void ts_kit_tui_secos_init(void)
{
	unsigned char ts_state = 0;
	int times = 0;
	int ret = 0;

	while (times < TS_FB_LOOP_COUNTS) {
		ts_state = atomic_read(&g_ts_kit_platform_data.state);
		if ((TS_SLEEP == ts_state) || (TS_WORK_IN_SLEEP == ts_state)) {
			mdelay(TS_FB_WAIT_TIME);
			times++;
		} else {
			break;
		}
	}

	if (!g_ts_kit_platform_data.chip_data->report_tui_enable) {
		ts_stop_wd_timer(&g_ts_kit_platform_data);
		disable_irq(g_ts_kit_platform_data.irq_id);
		times = 0;
		while (times < TS_FB_LOOP_COUNTS) {
			if (!atomic_read(&g_ts_kit_data_report_over)) {
				mdelay(TS_FB_WAIT_TIME);
				times++;
			} else {
				break;
			}
		}
		if (TS_BUS_I2C == g_ts_kit_platform_data.bops->btype) {
			i2c_init_secos(g_ts_kit_platform_data.client->adapter);
		} else {
			ret = spi_init_secos(g_ts_kit_platform_data.bops->bus_id);
			if (ret) {
				TS_LOG_ERR("%s spi_init_secos failed ret: %d\n", __func__, ret);
			}
		}

		g_ts_kit_platform_data.chip_data->report_tui_enable = true;
		TS_LOG_INFO("[tui] ts_kit_tui_secos_init: report_tui_enable is %d\n",
				g_ts_kit_platform_data.chip_data->report_tui_enable);
	}
}

void ts_kit_tui_secos_exit(void)
{
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	int ret = 0;

	if (g_ts_kit_platform_data.chip_data->report_tui_enable) {
		ts_start_wd_timer(&g_ts_kit_platform_data);
		if (TS_BUS_I2C == g_ts_kit_platform_data.bops->btype) {
			i2c_exit_secos(g_ts_kit_platform_data.client->adapter);
		} else {
			ret = spi_exit_secos(g_ts_kit_platform_data.bops->bus_id);
			if (ret) {
				TS_LOG_ERR("%s spi_exit_secos failed ret: %d\n", __func__, ret);
			}
		}
		if (dev->ops->chip_reset)
			dev->ops->chip_reset();

		enable_irq(g_ts_kit_platform_data.irq_id);
		g_ts_kit_platform_data.chip_data->report_tui_enable = false;
		TS_LOG_INFO("ts_kit_tui_secos_exit: tui_set_flag is %d\n",
			    g_ts_kit_platform_data.chip_data->tui_set_flag);

		if (g_ts_kit_platform_data.chip_data->tui_set_flag & 0x1) {
			TS_LOG_INFO("TUI exit, do before suspend\n");
			ret = ts_kit_power_control_notify(TS_BEFORE_SUSPEND,
						SHORT_SYNC_TIMEOUT);
			if (ret) {
				TS_LOG_ERR("ts beforce suspend device err\n");
			}
		}

		if (g_ts_kit_platform_data.chip_data->tui_set_flag & 0x2) {
			TS_LOG_INFO("TUI exit, do suspend\n");
			ret = ts_kit_power_control_notify(TS_SUSPEND_DEVICE,
						NO_SYNC_TIMEOUT);
			if (ret) {
				TS_LOG_ERR("ts suspend device err\n");
			}
		}

		g_ts_kit_platform_data.chip_data->tui_set_flag = 0;
		TS_LOG_INFO("ts_kit_tui_secos_exit: report_tui_enable is %d\n",
			    g_ts_kit_platform_data.chip_data->report_tui_enable);
	}
	return ;
}

static int tui_tp_init(void *data, int secure)
{
	if (secure) {
		ts_kit_tui_secos_init();
	} else
		ts_kit_tui_secos_exit();
	return 0;
}

int ts_kit_tui_report_input(void *finger_data)
{
	int error = NO_ERR;
	struct ts_fingers *finger = (struct ts_fingers *)finger_data;
	struct input_dev *input_dev = g_ts_kit_platform_data.input_dev;
	int id;

	int finger_num = 0;
	TS_LOG_DEBUG("ts_tui_report_input\n");

	for (id = 0; id < TS_MAX_FINGER; id++) {

		if (finger->fingers[id].status == 0) {
			TS_LOG_DEBUG("never touch before: id is %d\n", id);
			continue;
		}

		if (finger->fingers[id].status == TS_FINGER_PRESS) {
			TS_LOG_DEBUG
			    ("down: id is %d, finger->fingers[id].pressure = %d, finger->fingers[id].x = %d, finger->fingers[id].y = %d\n",
			     id, finger->fingers[id].pressure,
			     finger->fingers[id].x, finger->fingers[id].y);
			finger_num++;

			input_report_abs(input_dev, ABS_MT_PRESSURE,
					 finger->fingers[id].pressure);
			input_report_abs(input_dev, ABS_MT_POSITION_X,
					 finger->fingers[id].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
					 finger->fingers[id].y);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
			input_mt_sync(input_dev);

		} else if (finger->fingers[id].status == TS_FINGER_RELEASE) {
			TS_LOG_DEBUG("up: id is %d, status = %d\n", id,
				     finger->fingers[id].status);
			input_mt_sync(input_dev);
		}
	}

	input_report_key(input_dev, BTN_TOUCH, finger_num);
	input_sync(input_dev);

	return error;
}
#endif
//struct anti_false_touch_param *g_anti_false_touch_param = NULL;
void ts_kit_anti_false_touch_param_achieve(struct ts_kit_device_data *chip_data){
	int retval  = NO_ERR;
	unsigned int value = 0;
	struct anti_false_touch_param *local_param = NULL;
	struct device_node *root = g_ts_kit_platform_data.node;
	struct device_node *device = NULL;
	bool found = false;

	TS_LOG_INFO("%s +\n", __func__);
	if (NULL == chip_data){
		TS_LOG_ERR("%s anti false touch get chip data NULL\n", __func__);
		return ;
	}
	local_param = &(chip_data->anti_false_touch_param_data);
	//g_anti_false_touch_param = local_param;
	memset(local_param, 0, sizeof(struct anti_false_touch_param));

	TS_LOG_INFO("%s chip_name:%s\n", __func__, chip_data->chip_name);
	for_each_child_of_node(root, device) {	/*find the chip node*/
		if (of_device_is_compatible(device, chip_data->chip_name)) {
			found = true;
			break;
		}
	}

	if (false == found){
		TS_LOG_ERR("%s anti false touch find dts node fail\n", __func__);
		return ;
	}

	//feature_all
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_ALL, &value);
	if (retval) {
		local_param->feature_all = 0;
		TS_LOG_ERR("get device feature_all failed\n");
	}else{
		local_param->feature_all = value;
	}

	if (!local_param->feature_all){
		TS_LOG_INFO("product do not suppurt avert misoper, set all param to 0\n");
		return ;
	}

	//feature_resend_point
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_RESEND_POINT, &value);
	if (retval) {
		local_param->feature_resend_point = 0;
		TS_LOG_ERR("get device feature_resend_point failed\n");
	}else{
		local_param->feature_resend_point = value;
	}

	//feature_orit_support
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_ORIT_SUPPORT, &value);
	if (retval) {
		local_param->feature_orit_support = 0;
		TS_LOG_ERR("get device feature_orit_support failed\n");
	}else{
		local_param->feature_orit_support = value;
	}

	//feature_reback_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_REBACK_BT, &value);
	if (retval) {
		local_param->feature_reback_bt = 0;
		TS_LOG_ERR("get device feature_reback_bt failed\n");
	}else{
		local_param->feature_reback_bt = value;
	}

	//lcd_width
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_LCD_WIDTH, &value);
	if (retval) {
		local_param->lcd_width = 0;
		TS_LOG_ERR("get device lcd_width failed\n");
	}else{
		local_param->lcd_width = value;
	}

	//lcd_height
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_LCD_HEIGHT, &value);
	if (retval) {
		local_param->lcd_height = 0;
		TS_LOG_ERR("get device lcd_height failed\n");
	}else{
		local_param->lcd_height = value;
	}

	//click_time_limit
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_CLICK_TIME_LIMIT, &value);
	if (retval) {
		local_param->click_time_limit = 0;
		TS_LOG_ERR("get device click_time_limit failed\n");
	}else{
		local_param->click_time_limit = value;
	}

	//click_time_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_CLICK_TIME_BT, &value);
	if (retval) {
		local_param->click_time_bt = 0;
		TS_LOG_ERR("get device click_time_bt failed\n");
	}else{
		local_param->click_time_bt = value;
	}

	//edge_position
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_POISION, &value);
	if (retval) {
		local_param->edge_position = 0;
		TS_LOG_ERR("get device edge_position failed\n");
	}else{
		local_param->edge_position = value;
	}

	//edge_postion_secondline
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_POISION_SECONDLINE, &value);
	if (retval) {
		local_param->edge_postion_secondline = 0;
		TS_LOG_ERR("get device edge_postion_secondline failed\n");
	}else{
		local_param->edge_postion_secondline = value;
	}

	//bt_edge_x
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_BT_EDGE_X, &value);
	if (retval) {
		local_param->bt_edge_x = 0;
		TS_LOG_ERR("get device bt_edge_x failed\n");
	}else{
		local_param->bt_edge_x = value;
	}

	//bt_edge_y
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_BT_EDGE_Y, &value);
	if (retval) {
		local_param->bt_edge_y = 0;
		TS_LOG_ERR("get device bt_edge_y failed\n");
	}else{
		local_param->bt_edge_y = value;
	}

	//move_limit_x
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_X, &value);
	if (retval) {
		local_param->move_limit_x = 0;
		TS_LOG_ERR("get device move_limit_x failed\n");
	}else{
		local_param->move_limit_x = value;
	}

	//move_limit_y
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y, &value);
	if (retval) {
		local_param->move_limit_y = 0;
		TS_LOG_ERR("get device move_limit_y failed\n");
	}else{
		local_param->move_limit_y = value;
	}

	//move_limit_x_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_X_T, &value);
	if (retval) {
		local_param->move_limit_x_t = 0;
		TS_LOG_ERR("get device move_limit_x_t failed\n");
	}else{
		local_param->move_limit_x_t = value;
	}

	//move_limit_y_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_T, &value);
	if (retval) {
		local_param->move_limit_y_t = 0;
		TS_LOG_ERR("get device move_limit_y_t failed\n");
	}else{
		local_param->move_limit_y_t = value;
	}

	//move_limit_x_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_X_BT, &value);
	if (retval) {
		local_param->move_limit_x_bt = 0;
		TS_LOG_ERR("get device move_limit_x_bt failed\n");
	}else{
		local_param->move_limit_x_bt = value;
	}

	//move_limit_y_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_BT, &value);
	if (retval) {
		local_param->move_limit_y_bt = 0;
		TS_LOG_ERR("get device move_limit_y_bt failed\n");
	}else{
		local_param->move_limit_y_bt = value;
	}

	//edge_y_confirm_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_Y_CONFIRM_T, &value);
	if (retval) {
		local_param->edge_y_confirm_t = 0;
		TS_LOG_ERR("get device edge_y_confirm_t failed\n");
	}else{
		local_param->edge_y_confirm_t = value;
	}

	//edge_y_dubious_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_Y_DUBIOUS_T, &value);
	if (retval) {
		local_param->edge_y_dubious_t = 0;
		TS_LOG_ERR("get device edge_y_dubious_t failed\n");
	}else{
		local_param->edge_y_dubious_t = value;
	}

	//edge_y_avg_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_Y_AVG_BT, &value);
	if (retval) {
		local_param->edge_y_avg_bt = 0;
		TS_LOG_ERR("get device edge_y_avg_bt failed\n");
	}else{
		local_param->edge_y_avg_bt = value;
	}

	//edge_xy_down_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_XY_DOWN_BT, &value);
	if (retval) {
		local_param->edge_xy_down_bt = 0;
		TS_LOG_ERR("get device edge_xy_down_bt failed\n");
	}else{
		local_param->edge_xy_down_bt = value;
	}

	//edge_xy_confirm_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_XY_CONFIRM_T, &value);
	if (retval) {
		local_param->edge_xy_confirm_t = 0;
		TS_LOG_ERR("get device edge_xy_confirm_t failed\n");
	}else{
		local_param->edge_xy_confirm_t = value;
	}

	//max_points_bak_num
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MAX_POINTS_BAK_NUM, &value);
	if (retval) {
		local_param->max_points_bak_num = 0;
		TS_LOG_ERR("get device max_points_bak_num failed\n");
	}else{
		local_param->max_points_bak_num = value;
	}

	//drv_stop_width
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_DRV_STOP_WIDTH, &value);
	if (retval) {
		local_param->drv_stop_width = 0;
		TS_LOG_ERR("get device drv_stop_width failed\n");
	}else{
		local_param->drv_stop_width = value;
	}

	//sensor_x_width
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_SENSOR_X_WIDTH, &value);
	if (retval) {
		local_param->sensor_x_width = 0;
		TS_LOG_ERR("get device sensor_x_width failed\n");
	}else{
		local_param->sensor_x_width = value;
	}

	//sensor_y_width
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_SENSOR_Y_WIDTH, &value);
	if (retval) {
		local_param->sensor_y_width = 0;
		TS_LOG_ERR("get device sensor_y_width failed\n");
	}else{
		local_param->sensor_y_width = value;
	}
	/* emui5.1 support */
	//feature_sg
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_SG, &value);
	if (retval) {
		local_param->feature_sg = 0;
		TS_LOG_ERR("get device feature_sg failed\n");
	}else{
		local_param->feature_sg = value;
	}

	//sg_min_value
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_SG_MIN_VALUE, &value);
	if (retval) {
		local_param->sg_min_value = 0;
		TS_LOG_ERR("get device sg_min_value failed\n");
	}else{
		local_param->sg_min_value = value;
	}

	//feature_support_list
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_SUPPORT_LIST, &value);
	if (retval) {
		local_param->feature_support_list = 0;
		TS_LOG_ERR("get device feature_support_list failed\n");
	}else{
		local_param->feature_support_list = value;
	}

	//max_distance_dt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MAX_DISTANCE_DT, &value);
	if (retval) {
		local_param->max_distance_dt = 0;
		TS_LOG_ERR("get device max_distance_dt failed\n");
	}else{
		local_param->max_distance_dt = value;
	}

	//feature_big_data
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_BIG_DATA, &value);
	if (retval) {
		local_param->feature_big_data = 0;
		TS_LOG_ERR("get device feature_big_data failed\n");
	}else{
		local_param->feature_big_data = value;
	}

	//feature_click_inhibition
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_CLICK_INHIBITION, &value);
	if (retval) {
		local_param->feature_click_inhibition = 0;
		TS_LOG_ERR("get device feature_click_inhibition failed\n");
	}else{
		local_param->feature_click_inhibition = value;
	}

	//min_click_time
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MIN_CLICK_TIME, &value);
	if (retval) {
		local_param->min_click_time = 0;
		TS_LOG_ERR("get device min_click_time failed\n");
	}else{
		local_param->min_click_time = value;
	}
	TS_LOG_INFO("%s:"
		" feature_all:%d, feature_resend_point:%d, feature_orit_support:%d, feature_reback_bt:%d, lcd_width:%d, lcd_height:%d,"
		" click_time_limit:%d, click_time_bt:%d, edge_position:%d, edge_postion_secondline:%d, bt_edge_x:%d, bt_edge_y:%d,"
		" move_limit_x:%d, move_limit_y:%d, move_limit_x_t:%d, move_limit_y_t:%d, move_limit_x_bt:%d,"
		" move_limit_y_bt:%d, edge_y_confirm_t:%d, edge_y_dubious_t:%d, edge_y_avg_bt:%d, edge_xy_down_bt:%d, edge_xy_confirm_t:%d, max_points_bak_num:%d,"
		" drv_stop_width:%d sensor_x_width:%d, sensor_y_width:%d,"
		" feature_sg:%d, sg_min_value:%d, feature_support_list:%d, max_distance_dt:%d, feature_big_data:%d, feature_click_inhibition:%d, min_click_time:%d\n",
		__func__,
		local_param->feature_all,
		local_param->feature_resend_point,
		local_param->feature_orit_support,
		local_param->feature_reback_bt,
		local_param->lcd_width,
		local_param->lcd_height,

		local_param->click_time_limit,
		local_param->click_time_bt,
		local_param->edge_position,
		local_param->edge_postion_secondline,
		local_param->bt_edge_x,
		local_param->bt_edge_y,

		local_param->move_limit_x,
		local_param->move_limit_y,
		local_param->move_limit_x_t,
		local_param->move_limit_y_t,
		local_param->move_limit_x_bt,

		local_param->move_limit_y_bt,
		local_param->edge_y_confirm_t,
		local_param->edge_y_dubious_t,
		local_param->edge_y_avg_bt,
		local_param->edge_xy_down_bt,
		local_param->edge_xy_confirm_t,
		local_param->max_points_bak_num,

		local_param->drv_stop_width,
		local_param->sensor_x_width,
		local_param->sensor_y_width,

		local_param->feature_sg,
		local_param->sg_min_value,
		local_param->feature_support_list,
		local_param->max_distance_dt,
		local_param->feature_big_data,
		local_param->feature_click_inhibition,
		local_param->min_click_time);
}
EXPORT_SYMBOL(ts_kit_anti_false_touch_param_achieve);


static int try_update_firmware(void)
{
    char joint_chr = '_';
    int error = NO_ERR;
    char* fw_name;
    struct ts_cmd_node cmd;

    memset(&cmd, 0, sizeof(struct ts_cmd_node));
    cmd.command = TS_FW_UPDATE_BOOT;
    fw_name = cmd.cmd_param.pub_params.firmware_info.fw_name;

    /*firmware name [product_name][ic_name][module][vendor]*/
    strncat(fw_name, g_ts_kit_platform_data.product_name, MAX_STR_LEN);
    strncat(fw_name, &joint_chr, 1);
    strncat(fw_name, g_ts_kit_platform_data.chip_data->chip_name, MAX_STR_LEN);
    strncat(fw_name, &joint_chr, 1);

    error = ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT);

    return error;
}
static void ts_watchdog_work(struct work_struct* work)
{
    int error = NO_ERR;
    struct ts_cmd_node cmd;

    TS_LOG_DEBUG("ts_watchdog_work\n");
    cmd.command = TS_CHECK_STATUS;

    if (g_ts_kit_platform_data.chip_data->is_parade_solution){
		error = ts_kit_proc_command_directly(&cmd);
    }else {
		error = ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
    }
    if (error)
    {
        TS_LOG_ERR("put TS_CHECK_STATUS cmd error :%d\n", error);
    }
    return;
}
static void ts_watchdog_timer(unsigned long data)
{
    struct ts_kit_platform_data*  cd = (struct ts_kit_platform_data*)data;

    TS_LOG_DEBUG("Timer triggered\n");

    if (!cd)
    { return; }

    if (!work_pending(&cd->watchdog_work))
    { schedule_work(&cd->watchdog_work); }

    return;
}
void ts_kit_thread_stop_notify(void)
{
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	TS_LOG_INFO("ts thread stop called by lcd only shutdown\n");
	if(TS_UNINIT == atomic_read(&g_ts_kit_platform_data.state)) {
		TS_LOG_INFO("ts is not init");
		return;
	}
	if(TS_UNREGISTER == atomic_read(&g_ts_kit_platform_data.register_flag)) {
		TS_LOG_ERR("ts is not register\n");
		return;
	}

	atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
       atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);
	disable_irq(g_ts_kit_platform_data.irq_id);
	ts_stop_wd_timer(&g_ts_kit_platform_data);
	if (dev && dev->ops && dev->ops->chip_shutdown)
		dev->ops->chip_shutdown();
	/*there is something wrong about system, now abandon the kthread_stop to avoid unkown bug*/
	//kthread_stop(g_ts_kit_platform_data.ts_task);
}

#if defined (CONFIG_HISI_BCI_BATTERY)
static int ts_charger_detect_cmd(enum ts_charger_state charger_state)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_charger_info *info = NULL;

	TS_LOG_INFO("%s called, charger type: %d, [0 out, 1 in], supported: %d\n",
	     __func__, charger_state,
	     g_ts_kit_platform_data.feature_info.charger_info.charger_supported);

	if (g_ts_kit_platform_data.feature_info.charger_info.charger_supported == 0) {
		TS_LOG_DEBUG("%s, do nothing cause charger_supported is zero\n", __func__);
		goto out;
	}

	info = &g_ts_kit_platform_data.feature_info.charger_info;
	info->op_action = TS_ACTION_WRITE;
	if (USB_PIUG_OUT == charger_state) {	/*usb plug out*/
		if (info->charger_switch == 0) {
			TS_LOG_ERR ("%s, there is no need to send cmd repeated\n",__func__);
			error = -EINVAL;
			goto out;
		}
		info->charger_switch = 0;
	} else {		/*usb plug in*/
		if (info->charger_switch == 1) {
			TS_LOG_ERR("%s, there is no need to send repeated\n",__func__);
			error = -EINVAL;
			goto out;
		}
		info->charger_switch = 1;
	}

	if (TS_WORK != atomic_read(&g_ts_kit_platform_data.state)) {
		TS_LOG_ERR("%s, can not send cmd when TP is not working in normal mode\n",__func__);
		error = -EINVAL;
		goto out;
	}

	cmd = (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	cmd->command = TS_CHARGER_SWITCH;
	cmd->cmd_param.prv_params = (void *)info;
	if (NO_ERR != ts_kit_put_one_cmd(cmd, NO_SYNC_TIMEOUT)) {
		TS_LOG_ERR("%s, put_one_cmd failed\n", __func__);
		error = -EINVAL;
		goto out;
	}

out:
	if (cmd != NULL)
		kfree(cmd);
	return error;
}

static int charger_detect_notifier_callback(struct notifier_block *self,
					    unsigned long event, void *data)
{
	enum  ts_charger_state charger_state  = USB_PIUG_OUT;

	if (!g_ts_kit_platform_data.feature_info.charger_info.charger_supported) {
		return 0;
	}

	switch (event) {
	case VCHRG_START_USB_CHARGING_EVENT :
	case VCHRG_START_AC_CHARGING_EVENT:
	case VCHRG_START_CHARGING_EVENT:
		charger_state  = USB_PIUG_IN;
		break;
	case VCHRG_STOP_CHARGING_EVENT:
		charger_state  = USB_PIUG_OUT;
		break;
	default:
		break;
	}

	if (charger_state != g_ts_kit_platform_data.feature_info.charger_info.status) {
		TS_LOG_INFO("%s, charger event:%ld, status=%d. \n",  __func__, event, charger_state);
		ts_charger_detect_cmd(charger_state);
	}

	g_ts_kit_platform_data.feature_info.charger_info.status = charger_state;

	return NO_ERR;
}

static void ts_kit_charger_notifier_register(void)
{
	int error;
	g_ts_kit_platform_data.charger_detect_notify.notifier_call = charger_detect_notifier_callback;
	error = hisi_register_notifier(&g_ts_kit_platform_data.charger_detect_notify, 1);
	if (error < 0) {
		TS_LOG_ERR("%s, charger_type_notifier_register failed\n",__func__);
		g_ts_kit_platform_data.charger_detect_notify.notifier_call = NULL;
	} else {
		TS_LOG_INFO ("%s, charger notify register succ\n",__func__);
	}

}
#endif

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block* self, unsigned long event, void* data)
{
    int error = NO_ERR;
    int i;
    struct fb_event* fb_event = data;
    int* blank;
    unsigned char ts_state = 0;
    int times = 0;

    TS_LOG_INFO("tpkit fb_callback called,ic_type is %d,pt_flag is %ld,event is %d\n",g_tskit_ic_type,g_tskit_pt_station_flag,event);
    /* only ONCELL and AMOLED ic need use fb_notifier_callbac */
    if (!(g_tskit_ic_type == ONCELL || g_tskit_ic_type == AMOLED))
    { 
    	 TS_LOG_INFO("fb_notifier_callback do not need to do, return\n");
        return NO_ERR; 
    }

    /* only need process event  FB_EARLY_EVENT_BLANK\FB_EVENT_BLANK  */
    if (!(event == FB_EARLY_EVENT_BLANK || event == FB_EVENT_BLANK)) {
        TS_LOG_DEBUG("event(%d) do not need process\n", event);
        return NO_ERR;
    }

    if (fb_event->data == NULL)
    {
        TS_LOG_DEBUG("event = %d, blank is NULL, not do fb_notifier_callback\n", event);
        return NO_ERR;
    }
    blank = fb_event->data;

    for (i = 0 ; i < FB_MAX; i++)
    {
        if (registered_fb[i] == fb_event->info)
        {
            if (i == 0)
            {
                TS_LOG_DEBUG("Ts-index:%d, go on !\n", i);
                break;
            }
            else
            {
                TS_LOG_INFO("Ts-index:%d, exit !\n", i);
                return error;
            }
        }
    }
    TS_LOG_INFO("fb_notifier_callback, blank: %d, event:%lu, state: %d\n", *blank, event, atomic_read(&g_ts_kit_platform_data.state));
    switch (*blank)
    {
        case FB_BLANK_UNBLANK:
            /*resume device*/
            switch (event)
            {

                case FB_EARLY_EVENT_BLANK:
                    TS_LOG_DEBUG("resume: event = %lu, not care\n", event);
                    break;
                case FB_EVENT_BLANK:
                    while (1)
                    {
                        ts_state = atomic_read(&g_ts_kit_platform_data.state);
                        if ((TS_SLEEP == ts_state) || (TS_WORK_IN_SLEEP == ts_state))
                        {
                            error = ts_kit_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);      /*touch power on*/
                            if (error)
                            { TS_LOG_ERR("ts resume device err\n"); }
                            error = ts_kit_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);  /*enable irq*/
                            if (error)
                            { TS_LOG_ERR("ts after resume err\n"); }
                            break;
                        }
                        else if(TS_WORK == ts_state)
                        {
                            TS_LOG_INFO("ts has already in work status,do nothing\n");
                            break;
                        }
                        else
                        {
                            msleep(TS_FB_WAIT_TIME);
                            if (times++ > TS_FB_LOOP_COUNTS)
                            {
                                times = 0;
                                TS_LOG_ERR("no resume, blank: %d, event:%lu, state: %d\n", *blank, event, ts_state);
                                break;
                            }
                        }
                    }
                    break;
                default:
                    TS_LOG_DEBUG("resume: event = %lu, not care\n", event);
                    break;
            }
            break;
        case FB_BLANK_VSYNC_SUSPEND:
        case FB_BLANK_HSYNC_SUSPEND:
        case FB_BLANK_NORMAL:
        case FB_BLANK_POWERDOWN:
        default:
            /*suspend device*/
            switch (event)
            {
                case FB_EARLY_EVENT_BLANK:
                    while (1)
                    {
                        ts_state = atomic_read(&g_ts_kit_platform_data.state);
                        if ((TS_WORK == ts_state) || (TS_WORK_IN_SLEEP == ts_state))
                        {
                            error = ts_kit_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT); /*disable irq*/
                            if (error)
                            { TS_LOG_ERR("ts suspend device err\n"); }
                            break;
                        }
                        else
                        {
                            msleep(TS_FB_WAIT_TIME);
                            if (times++ > TS_FB_LOOP_COUNTS)
                            {
                                times = 0;
                                TS_LOG_ERR("no early suspend, blank: %d, event:%lu, state: %d\n", *blank, event, ts_state);
                                break;
                            }
                        }
                    }
                    break;
                case FB_EVENT_BLANK:
                    while (1)
                    {
                        ts_state = atomic_read(&g_ts_kit_platform_data.state);
                        if ((TS_WORK == ts_state) || (TS_WORK_IN_SLEEP == ts_state))
                        {
                            error = ts_kit_power_control_notify(TS_SUSPEND_DEVICE, NO_SYNC_TIMEOUT);    /*touch power off*/
                            if (error)
                            { TS_LOG_ERR("ts before suspend err\n"); }
                            break;
                        }
                        else
                        {
                            msleep(TS_FB_WAIT_TIME);
                            if (times++ > TS_FB_LOOP_COUNTS)
                            {
                                times = 0;
                                TS_LOG_ERR("no suspend, blank: %d, event:%lu, state: %d\n", *blank, event, ts_state);
                                break;
                            }
                        }
                    }
                    break;
                default:
                    TS_LOG_DEBUG("suspend: event = %lu, not care\n", event);
                    break;
            }
            break;
    }

    return NO_ERR;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ts_early_suspend(struct early_suspend* h)
{
    bool is_in_cell = g_ts_kit_platform_data.chip_data->is_in_cell;
    int error = NO_ERR;

    TS_LOG_INFO("ts early suspend, %s\n", (is_in_cell == false) ? "need suspend" : "no need suspend");

    /*for the in-cell, ts_suspend_notify called by lcd, not here*/
    if (false == is_in_cell)
    {
        error = ts_kit_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);
        if (error)
        { TS_LOG_ERR("ts before suspend err\n"); }
        error = ts_kit_power_control_notify(TS_SUSPEND_DEVICE, SHORT_SYNC_TIMEOUT);
        if (error)
        { TS_LOG_ERR("ts suspend device err\n"); }
    }

    TS_LOG_INFO("ts_early_suspend done\n");
}
static void ts_late_resume(struct early_suspend* h)
{
    bool is_in_cell = g_ts_kit_platform_data.chip_data->is_in_cell;
    int error = NO_ERR;
    TS_LOG_INFO("ts late resume, %s\n", (is_in_cell == false) ? "need resume" : "no need resume");

    /*for the in-cell, ts_resume_notify called by lcd, not here*/
    if (false == is_in_cell)
    {
        error = ts_kit_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);
        if (error)
        { TS_LOG_ERR("ts resume device err\n"); }
        error = ts_kit_power_control_notify(TS_AFTER_RESUME, SHORT_SYNC_TIMEOUT);
        if (error)
        { TS_LOG_ERR("ts after resume err\n"); }
    }
	if(g_ts_kit_platform_data.chip_data->is_direct_proc_cmd == 0)
		ts_send_holster_cmd();

    TS_LOG_INFO("ts_late_resume done\n");
}
#endif

static int parse_spi_config(void)
{
    int retval = 0;

    retval = of_property_read_u32(g_ts_kit_platform_data.node, "spi-max-frequency",
                &g_ts_kit_platform_data.spi_max_frequency);
    if (retval) {
        TS_LOG_ERR("%s: get spi_max_frequency failed\n",__func__);
        goto  err_out;
    }

    retval = of_property_read_u32(g_ts_kit_platform_data.node, "spi-mode",
		&g_ts_kit_platform_data.spi_mode);
    if (retval) {
        TS_LOG_ERR("%s: get spi mode failed\n",__func__);
        goto  err_out;
    }

    retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,interface",
                &g_ts_kit_platform_data.spidev0_chip_info.iface);
    if (retval) {
        TS_LOG_ERR("%s: get iface failed\n",__func__);
        goto  err_out;
    }
    retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,com-mode",
                &g_ts_kit_platform_data.spidev0_chip_info.com_mode);
    if (retval) {
        TS_LOG_ERR("%s: get com_mode failed\n",__func__);
        goto  err_out;
    }
    retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,rx-level-trig",
                &g_ts_kit_platform_data.spidev0_chip_info.rx_lev_trig);
    if (retval) {
        TS_LOG_ERR("%s: get rx_lev_trig failed\n",__func__);
        goto  err_out;
    }
    retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,tx-level-trig",
                &g_ts_kit_platform_data.spidev0_chip_info.tx_lev_trig);
    if (retval) {
        TS_LOG_ERR( "%s: get tx_lev_trig failed\n",__func__);
        goto  err_out;
    }
    retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,ctrl-len",
                &g_ts_kit_platform_data.spidev0_chip_info.ctrl_len);
    if (retval) {
        TS_LOG_ERR( "%s: get ctrl_len failed\n",__func__);
        goto  err_out;
    }
    retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,wait-state",
                &g_ts_kit_platform_data.spidev0_chip_info.wait_state);
    if (retval) {
        TS_LOG_ERR( "%s: get wait_state failed\n",__func__);
        goto  err_out;
    }
    retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,duplex",
        &g_ts_kit_platform_data.spidev0_chip_info.duplex);
    if (retval) {
        TS_LOG_ERR("%s: get duplex failed\n",__func__);
        goto  err_out;
    }
    retval = of_property_read_u32(g_ts_kit_platform_data.node, "cs_reset_low_delay",
        &g_ts_kit_platform_data.cs_reset_low_delay);
    if (retval) {
        TS_LOG_ERR("%s: get duplex failed\n",__func__);
        goto  err_out;
    }
    retval = of_property_read_u32(g_ts_kit_platform_data.node, "cs_reset_high_delay",
        &g_ts_kit_platform_data.cs_reset_high_delay);
    if (retval) {
        TS_LOG_ERR("%s: get duplex failed\n",__func__);
        goto  err_out;
    }
    g_ts_kit_platform_data.cs_gpio = of_get_named_gpio(g_ts_kit_platform_data.node , "cs_gpio", 0);
    if (!gpio_is_valid(g_ts_kit_platform_data.cs_gpio))
    {
        g_ts_kit_platform_data.cs_gpio = 0;
        TS_LOG_ERR(" ts_kit cs gpio is not valid\n");
    }

    TS_LOG_INFO("%s: spi-max-frequency = %d  spi_mode = %d pl022,interface =%d pl022,com-mode = %d pl022,rx-level-trig = %d"
        "pl022,tx-level-trig = %d pl022,ctrl-len = %d pl022,wait_state = %d pl022,duplex = %d,cs_reset_low_delay=%d cs_reset_high_delay = %d cs_gpio = %d\n",
        __func__,g_ts_kit_platform_data.spi_max_frequency,g_ts_kit_platform_data.spi_mode,g_ts_kit_platform_data.spidev0_chip_info.iface,g_ts_kit_platform_data.spidev0_chip_info.com_mode,
        g_ts_kit_platform_data.spidev0_chip_info.rx_lev_trig,g_ts_kit_platform_data.spidev0_chip_info.tx_lev_trig,g_ts_kit_platform_data.spidev0_chip_info.ctrl_len,g_ts_kit_platform_data.spidev0_chip_info.wait_state,
        g_ts_kit_platform_data.spidev0_chip_info.duplex,g_ts_kit_platform_data.cs_reset_low_delay,g_ts_kit_platform_data.cs_reset_high_delay,g_ts_kit_platform_data.cs_gpio);
    return 0;
err_out:
    return retval;
}

static int get_ts_board_info(void)
{
    const char* bus_type;
    int rc;
    int error = NO_ERR;
    u32 bus_id = 0;
    u32 hide_plain_id = 0;
    u32 touch_switch_need_process = 0;
    u32 fp_tp_enable = 0;
    u32 register_charger_notifier = 0;
    g_ts_kit_platform_data.node = NULL;

    g_ts_kit_platform_data.node = of_find_compatible_node(NULL, NULL, TS_DEV_NAME);
    if (!g_ts_kit_platform_data.node)
    {
        TS_LOG_ERR("can't find ts module node\n");
        error = -EINVAL;
        goto out;
    }

    rc = of_property_read_string(g_ts_kit_platform_data.node, "bus_type", &bus_type);
    if (rc)
    {
        TS_LOG_ERR("bus type read failed:%d\n", rc);
        error = -EINVAL;
        goto out;
    }
    TS_LOG_INFO("bus type is:%s\n", bus_type);
	
    if (!strcmp (bus_type, "i2c"))
    {
        g_ts_kit_platform_data.bops = &ts_bus_i2c_info;
    }
    else if (!strcmp (bus_type, "spi"))
    {
        rc = parse_spi_config();
        if(rc) {
            TS_LOG_ERR("parse_spi_config fail\n");
            error = -EINVAL;
            goto out;
    }
        g_ts_kit_platform_data.bops = &ts_bus_spi_info;
    }
    else
    {
        TS_LOG_ERR("bus type invaild:%s\n", bus_type);
        error = -EINVAL;
	    goto out;
    }

    rc = of_property_read_u32(g_ts_kit_platform_data.node, "bus_id", &bus_id);
    if (rc)
    {
        TS_LOG_ERR("bus id read failed\n");
        error = -EINVAL;
        goto out;
    }
    g_ts_kit_platform_data.bops->bus_id = bus_id;
    TS_LOG_INFO("bus id :%d\n", bus_id);

    rc = of_property_read_u32(g_ts_kit_platform_data.node, "need_i2c_hwlock", &g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag);
    if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag)
    {
		g_ts_kit_platform_data.i2c_hwlock.hwspin_lock = hwspin_lock_request_specific(TP_I2C_HWSPIN_LOCK_CODE);
		if(!g_ts_kit_platform_data.i2c_hwlock.hwspin_lock)
		{
			TS_LOG_INFO("get i2c hwlock failed.\n");
			error = -EINVAL;
			 goto out;
		}
		TS_LOG_INFO("get i2c hwlock success.\n");
    }

    rc = of_property_read_u32(g_ts_kit_platform_data.node, "aft_enable", &g_ts_kit_platform_data.aft_param.aft_enable_flag);
    if (g_ts_kit_platform_data.aft_param.aft_enable_flag)
    {
	  of_property_read_u32(g_ts_kit_platform_data.node, "drv_stop_width", &g_ts_kit_platform_data.aft_param.drv_stop_width);
	  of_property_read_u32(g_ts_kit_platform_data.node, "lcd_width", &g_ts_kit_platform_data.aft_param.lcd_width);
	  of_property_read_u32(g_ts_kit_platform_data.node, "lcd_height", &g_ts_kit_platform_data.aft_param.lcd_height);
        TS_LOG_INFO("aft enable,drv_stop_width is %d,lcd_width is %d, lcd_height is %d\n",
			g_ts_kit_platform_data.aft_param.drv_stop_width,g_ts_kit_platform_data.aft_param.lcd_width,g_ts_kit_platform_data.aft_param.lcd_height);
    }
    else
    {
        TS_LOG_INFO("aft disable\n");
    }

    g_ts_kit_platform_data.reset_gpio = of_get_named_gpio(g_ts_kit_platform_data.node , "reset_gpio", 0);
    if (!gpio_is_valid(g_ts_kit_platform_data.reset_gpio))
    {
        g_ts_kit_platform_data.reset_gpio = 0;
        TS_LOG_ERR(" ts_kit reset gpio is not valid\n");
    }
    g_ts_kit_platform_data.irq_gpio = of_get_named_gpio(g_ts_kit_platform_data.node , "irq_gpio", 0);
    if (!gpio_is_valid(g_ts_kit_platform_data.irq_gpio ))
    {
        TS_LOG_ERR(" ts_kit irq_gpio is not valid\n");
	    error = -EINVAL;
		goto out;
    }

    rc = of_property_read_u32(g_ts_kit_platform_data.node , "fp_tp_enable", &fp_tp_enable);
    if (rc) {
        TS_LOG_ERR(" ts_kit fp_tp_enable is not valid\n");
        g_ts_kit_platform_data.fp_tp_enable = 0;
    } else {
        g_ts_kit_platform_data.fp_tp_enable = fp_tp_enable;
    }
    rc = of_property_read_u32(g_ts_kit_platform_data.node , "register_charger_notifier", &register_charger_notifier);
    if (rc) {
        TS_LOG_ERR(" ts_kit register_charger_notifier is not config, use default enable\n");
        g_ts_kit_platform_data.register_charger_notifier = true;
    } else {
        g_ts_kit_platform_data.register_charger_notifier = register_charger_notifier;
        TS_LOG_INFO("ts_kit register_charger_notifier = %d\n", register_charger_notifier);
    }

    rc = of_property_read_u32(g_ts_kit_platform_data.node, "hide_plain_id", &hide_plain_id);
    if (rc) {
	    g_ts_kit_platform_data.hide_plain_id = 0;
	    TS_LOG_INFO("hide_plain_id not exsit\n");
    }
    g_ts_kit_platform_data.hide_plain_id = hide_plain_id;
	rc = of_property_read_u32(g_ts_kit_platform_data.node, "touch_switch_need_process", &touch_switch_need_process);
    if (rc) {
	    g_ts_kit_platform_data.touch_switch_need_process = 0;
	    TS_LOG_INFO("touch_switch_need_process not exsit\n");
    } else {
        g_ts_kit_platform_data.touch_switch_need_process = touch_switch_need_process;
    }

	TS_LOG_INFO("bus id :%d ts_kit reset gpio is = %d ts_kit irq gpio is = %d hide_plain_id = %d touch_switch_need_process = %d.\n",
		g_ts_kit_platform_data.bops->bus_id, g_ts_kit_platform_data.reset_gpio,
		g_ts_kit_platform_data.irq_gpio, g_ts_kit_platform_data.hide_plain_id, g_ts_kit_platform_data.touch_switch_need_process);

out:
    return error;
}

static void ts_spi_cs_set(u32 control)
{
    int ret = 0;

    if (SSP_CHIP_SELECT == control) {
        ret = gpio_direction_output(g_ts_kit_platform_data.cs_gpio, control);
        /* cs steup time at least 10ns */
        ndelay(g_ts_kit_platform_data.cs_reset_low_delay);
    }else{
        /* cs hold time at least 4*40ns(@25MHz) */
        ret = gpio_direction_output(g_ts_kit_platform_data.cs_gpio, control);
        ndelay(g_ts_kit_platform_data.cs_reset_high_delay);
    }

    if (ret < 0) {
        TS_LOG_ERR("%s: fail to set gpio cs, result = %d.\n",
                __func__, ret);
    }
}

static int ts_gpio_request(void)
{
    int error = NO_ERR;

    if (g_ts_kit_platform_data.reset_gpio) {
        error = gpio_request(g_ts_kit_platform_data.reset_gpio, "ts_kit_reset_gpio");
        if (error < 0)
        {
            TS_LOG_ERR("Fail request gpio:%d, ret=%d\n", g_ts_kit_platform_data.reset_gpio, error);
        return error;
        }
    }
    error = gpio_request(g_ts_kit_platform_data.irq_gpio, "ts_kit_irq_gpio");
    if (error < 0)
    {
        TS_LOG_ERR("Fail request gpio:%d, ret=%d\n", g_ts_kit_platform_data.irq_gpio, error);
        return error;
    }
    TS_LOG_INFO("reset_gpio :%d ,irq_gpio :%d\n", g_ts_kit_platform_data.reset_gpio,g_ts_kit_platform_data.irq_gpio);
    return error;
}
static int ts_creat_i2c_client(void)
{
    struct i2c_adapter* adapter = NULL;
    struct i2c_client* client = NULL;
    struct i2c_board_info board_info;

    memset(&board_info, 0, sizeof(struct i2c_board_info));
    strncpy(board_info.type, TS_DEV_NAME, I2C_NAME_SIZE - 1);
    board_info.addr = I2C_DEFAULT_ADDR;
    board_info.flags = true;

    adapter = i2c_get_adapter(g_ts_kit_platform_data.bops->bus_id);
    if (!adapter)
    {
        TS_LOG_ERR("i2c_get_adapter failed\n");
        return -EIO;
    }

    client = i2c_new_device(adapter, &board_info);
    if (!client)
    {
        TS_LOG_ERR("i2c_new_device failed\n");
        return -EIO;
    }
    g_ts_kit_platform_data.client = client;
    i2c_set_clientdata(client, &g_ts_kit_platform_data);
	
#if defined (CONFIG_HUAWEI_DSM)
	if (!ts_dclient) {
		ts_dclient = dsm_register_client(&dsm_tp);
	}
#endif
    return NO_ERR;
}

static int ts_destory_i2c_client(void)
{
    TS_LOG_ERR("destory i2c device\n");
    i2c_unregister_device(g_ts_kit_platform_data.client);
    return NO_ERR;
}

static int ts_creat_spi_client(void)
{
    struct spi_master *spi_master = NULL;
    struct spi_device *spi_device = NULL;
    struct spi_board_info board_info;
    int error = NO_ERR;

    TS_LOG_INFO("ts_creat_spi_client called\n");
    spi_master = spi_busnum_to_master(g_ts_kit_platform_data.bops->bus_id);
    if (NULL == spi_master) {
        TS_LOG_ERR("spi_busnum_to_master(%d) return NULL\n", \
            g_ts_kit_platform_data.bops->bus_id);
        return -ENODEV;
    }

    memset(&board_info, 0, sizeof(struct spi_board_info));
    board_info.bus_num = g_ts_kit_platform_data.bops->bus_id;
    board_info.max_speed_hz = g_ts_kit_platform_data.spi_max_frequency;
    board_info.mode = g_ts_kit_platform_data.spi_mode;

    g_ts_kit_platform_data.spidev0_chip_info.hierarchy = SSP_MASTER;
    g_ts_kit_platform_data.spidev0_chip_info.cs_control = ts_spi_cs_set;
    board_info.controller_data = &g_ts_kit_platform_data.spidev0_chip_info;

    error = gpio_request(g_ts_kit_platform_data.cs_gpio, "tpkit_cs");
    if (error) {
        TS_LOG_ERR("%s:gpio_request(%d) failed\n", __func__,g_ts_kit_platform_data.cs_gpio);
        return -ENODEV;
    }
    gpio_direction_output(g_ts_kit_platform_data.cs_gpio, GPIO_HIGH);

    spi_device = spi_new_device(spi_master, &board_info);
    if (NULL == spi_device) {
        gpio_free(g_ts_kit_platform_data.cs_gpio);
        TS_LOG_ERR("spi_new_device fail\n");
        return -ENODEV;
    }
    g_ts_kit_platform_data.spi = spi_device;
    spi_set_drvdata(spi_device, &g_ts_kit_platform_data);
#if defined (CONFIG_HUAWEI_DSM)
    if (!ts_dclient) {
        ts_dclient = dsm_register_client(&dsm_tp);
    }
#endif
    TS_LOG_INFO("ts_creat_spi_client sucessful\n");
    return NO_ERR;
}

static int ts_destory_spi_client(void)
{
    TS_LOG_ERR("destory i2c device\n");
    spi_unregister_device(g_ts_kit_platform_data.spi);
    return NO_ERR;
}

static int ts_create_client(void)
{
    int error = -EINVAL;

    switch (g_ts_kit_platform_data.bops->btype)
    {
        case TS_BUS_I2C:
            TS_LOG_DEBUG("create ts's i2c device\n");
            error = ts_creat_i2c_client();
            break;
        case TS_BUS_SPI:
            TS_LOG_DEBUG("create ts's spi device\n");
            error = ts_creat_spi_client();
            break;
        default:
            TS_LOG_ERR("unknown ts's device\n");
            break;
    }

    return error;
}

static int ts_destory_client(void)
{
    TS_LOG_ERR("destory touchscreen device\n");

    switch (g_ts_kit_platform_data.bops->btype)
    {
        case TS_BUS_I2C:
            TS_LOG_DEBUG("destory ts's i2c device\n");
            ts_destory_i2c_client();
            break;
        case TS_BUS_SPI:
            ts_destory_spi_client();
            TS_LOG_DEBUG("destory ts's spi device\n");
            break;
        default:
            TS_LOG_ERR("unknown ts's device\n");
            break;
    }
    return NO_ERR;
}

static int ts_kit_parse_config(void)
{
    int error = NO_ERR;
    int rc = 0;
    int index = 0;
    char* tmp_buff = NULL;
     int tmp_buff_len= 0;

    if (g_ts_kit_platform_data.node)
    {
        rc = of_property_read_string(g_ts_kit_platform_data.node, "product", (const char**)&tmp_buff);
        if (rc)
        {
            TS_LOG_ERR("product read failed:%d\n", rc);
            error = -EINVAL;
            goto out;
        }
    }

    if (!tmp_buff)
    {
        TS_LOG_ERR("tmp_buff failed\n");
        error = -EINVAL;
        goto out;
    }

    memset(g_ts_kit_platform_data.product_name, 0, MAX_STR_LEN);
    tmp_buff_len = (int)strlen(tmp_buff);
    for (index = 0;  index < tmp_buff_len && index < (MAX_STR_LEN - 1); index++) //exchange name to lower
    { g_ts_kit_platform_data.product_name[index] = tolower(tmp_buff[index]); }

    TS_LOG_INFO("parse product name :%s\n", g_ts_kit_platform_data.product_name);

out:
    return error;
}
static void procfs_create(void)
{
    if (!proc_mkdir("touchscreen", NULL))
    { return; }
    proc_create("touchscreen/tp_capacitance_data", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, NULL, &rawdata_proc_fops);
    proc_create("touchscreen/tp_calibration_data", S_IRUSR | S_IWUSR |S_IRGRP | S_IROTH, NULL, &calibration_proc_fops);
    return;
}

static int ts_kit_create_sysfs(void)
{
    int error = NO_ERR;

    TS_LOG_INFO("ts_kit_create_sysfs enter\n");
    error = sysfs_create_group(&g_ts_kit_platform_data.ts_dev->dev.kobj, &ts_attr_group);
    if (error)
    {
        TS_LOG_ERR("can't create ts's sysfs\n");
        goto err_del_platform_dev;
    }
    TS_LOG_INFO("sysfs_create_group success\n");
    procfs_create();
    TS_LOG_INFO("procfs_create success\n");
    error = sysfs_create_link(NULL, &g_ts_kit_platform_data.ts_dev->dev.kobj, "touchscreen");
    if (error)
    {
        TS_LOG_ERR("%s: Fail create link error = %d\n", __func__, error);
        goto err_free_sysfs;
    }
    goto err_out;

err_free_sysfs:
    sysfs_remove_group(&g_ts_kit_platform_data.ts_dev->dev.kobj, &ts_attr_group);
err_del_platform_dev:
    platform_device_del(g_ts_kit_platform_data.ts_dev);
    platform_device_put(g_ts_kit_platform_data.ts_dev);
err_out:
    return error;
}


static int ts_kit_chip_init(void)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	
	TS_LOG_INFO("ts_chip_init called\n");
	mutex_init(&ts_kit_easy_wake_guesure_lock);
	if(g_ts_kit_platform_data.chip_data->is_direct_proc_cmd == 0){
		if (dev->ops->chip_init) {
			error = dev->ops->chip_init();
		}
	}
	if (error) {
		TS_LOG_ERR("chip init failed\n");
	}
#ifdef CONFIG_HUAWEI_DSM
	else {
		if (dev->chip_name && DSM_MAX_IC_NAME_LEN > strlen(dev->chip_name) &&
				dev->module_name && DSM_MAX_MODULE_NAME_LEN > strlen(dev->module_name)) {
			dsm_tp.ic_name = dev->chip_name;
			dsm_tp.module_name = dev->module_name;
			if (dsm_update_client_vendor_info(&dsm_tp)) {
				TS_LOG_ERR("dsm update client_vendor_info is failed\n");
			}
		} else {
				TS_LOG_ERR("ic_name, module_name is invalid\n");
		}
	}
#endif
	return error;
}

static int ts_register_algo(void)
{
	int error = NO_ERR;
	struct ts_kit_device_data*dev = g_ts_kit_platform_data.chip_data;

	TS_LOG_INFO("register algo called\n");
	dev->algo_size = 0;
       INIT_LIST_HEAD(&dev->algo_head);
	error = ts_kit_register_algo_func(dev);

	return error;
}

static long ts_ioctl_get_fingers_info(unsigned long arg)
{
    int ret = 0;
    void __user* argp = (void __user*)arg;
    struct ts_fingers data;
    u32 frame_size;

    //TS_LOG_ERR("[MUTI_AFT] ts_ioctl_get_fingers_info enter \n");
    if (arg == 0)
    {
        TS_LOG_ERR("arg == 0.\n");
        return -EINVAL;
    }
    /* wait event */
    atomic_set(&g_ts_kit_platform_data.fingers_waitq_flag, AFT_WAITQ_WAIT);
	down_interruptible(&g_ts_kit_platform_data.fingers_aft_send);
	if(atomic_read(&g_ts_kit_platform_data.fingers_waitq_flag) == AFT_WAITQ_WAIT)
	{
                atomic_set(&g_ts_kit_platform_data.fingers_waitq_flag, AFT_WAITQ_IGNORE);
                return -EINVAL;
	}

	if(atomic_read(&g_ts_kit_platform_data.fingers_waitq_flag) == AFT_WAITQ_WAKEUP)
	{
		if (copy_to_user(argp, &g_ts_kit_platform_data.fingers_send_aft_info,
				sizeof(struct ts_fingers)))
		{
			TS_LOG_ERR("ts_ioctl_get_fingers_info Failed to copy_from_user().\n");
			return -EFAULT;
		}
	}

	return ret;
}
static long ts_ioctl_get_aft_param_info(unsigned long arg)
{
    if (arg == 0)
    {
        TS_LOG_ERR("arg == 0.\n");
        return -EINVAL;
    }
    if (copy_to_user(arg, &g_ts_kit_platform_data.aft_param,
                     sizeof(struct ts_aft_algo_param)))
    {

        TS_LOG_ERR("ts_ioctl_get_aft_param_info Failed to copy_to_user().\n");
        return -EFAULT;
    }
    return 0;
}
static long ts_ioctl_set_coordinates(unsigned long arg)
{
    struct ts_fingers data;
    void __user* argp = (void __user*)arg;
    struct ts_fingers* finger = NULL;
    struct input_dev* input_dev = g_ts_kit_platform_data.input_dev;
    struct anti_false_touch_param *local_param = NULL;
    int finger_num = 0;
    int id = 0;
    unsigned long flags;
	
	if(!input_dev){
		TS_LOG_ERR("The command node or input device is not exist!\n");
		return -EINVAL;
	}

//TS_LOG_ERR("[MUTI_AFT] ts_ioctl_set_coordinates enter\n");
    if (arg == 0)
    {
        TS_LOG_ERR("arg == 0.\n");
        return -EINVAL;
    }
    if (copy_from_user(&data, argp,
                       sizeof(struct ts_fingers)))
    {
        TS_LOG_ERR("ts_ioctl_set_coordinates Failed to copy_from_user().\n");
        return -EFAULT;
    }
    //memcpy(&g_ts_kit_platform_data.fingers_recv_aft_info,&data,sizeof(struct ts_fingers));
	finger = &data;

#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
	struct aft_abs_param_major aft_abs_major;
	int major = 0;
	int minor = 0;
#else
	int x_y_distance = 0;
	short tmp_distance = 0;
	char *p = NULL;
#endif

	if (g_ts_kit_platform_data.chip_data){
		local_param = &(g_ts_kit_platform_data.chip_data->anti_false_touch_param_data);
	}else{
		local_param = NULL;
	}
    //TS_LOG_ERR("[MUTI_AFT] ts_report_input\n");
    ts_check_touch_window(finger);

    for (id = 0; id < TS_MAX_FINGER; id++)
    {
        if (finger->fingers[id].status == 0)
        {
            //TS_LOG_ERR("[MUTI_AFT] never touch before: id is %d\n", id);
            continue;
        }
        if (finger->fingers[id].status == TS_FINGER_PRESS)
        {
            if (lcdkit_fps_support_query() && lcdkit_fps_tscall_support_query())
                lcdkit_fps_ts_callback();

            finger_num++;
            input_report_abs(input_dev, ABS_MT_PRESSURE, finger->fingers[id].pressure);
            input_report_abs(input_dev, ABS_MT_POSITION_X, finger->fingers[id].x);
            input_report_abs(input_dev, ABS_MT_POSITION_Y, finger->fingers[id].y);
            if (g_ts_kit_platform_data.fp_tp_enable) {
                input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, finger->fingers[id].major);
                input_report_abs(input_dev, ABS_MT_TOUCH_MINOR, finger->fingers[id].minor);
            }
            input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
			if (local_param && local_param->feature_all){
				if (local_param->sensor_x_width && local_param->sensor_y_width){
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
					if ((finger->fingers[id].major || finger->fingers[id].minor)
						&& (!g_ts_kit_platform_data.feature_info.holster_info.holster_switch)){
						major = 0; minor = 1;
						memset(&aft_abs_major, 0, sizeof(struct aft_abs_param_major));
						aft_abs_major.edgex = finger->fingers[id].major * local_param->sensor_x_width;
						aft_abs_major.edgey = finger->fingers[id].minor * local_param->sensor_y_width;
						if(local_param->feature_sg)
							aft_abs_major.orientation = finger->fingers[id].orientation;
						else
							aft_abs_major.orientation = 0;

						aft_abs_major.version = 0x01; /*Version number*/
						memcpy(&major, &aft_abs_major, sizeof(int));
						input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, major);
						input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, minor);
					}else{
						major = 0; minor = 0;
						input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, major);
						input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, minor);
					}
#else
					x_y_distance = 0;
					p = (char *)&x_y_distance;
					tmp_distance = finger->fingers[id].major * local_param->sensor_x_width;
					memcpy(p, (char *)&tmp_distance, sizeof(short));
					tmp_distance = finger->fingers[id].minor * local_param->sensor_y_width;
					memcpy(p+sizeof(short), (char *)&tmp_distance, sizeof(short));
					input_report_abs(input_dev, ABS_MT_DISTANCE, x_y_distance);
#endif
				}else{
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
					input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, 0);
					input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, 0);
#else
					input_report_abs(input_dev, ABS_MT_DISTANCE, 0);
#endif
				}
			}
            input_mt_sync(input_dev);				//modfiy by mengkun
        }
        else if (finger->fingers[id].status == TS_FINGER_RELEASE)
        {
            //TS_LOG_ERR("[MUTI_AFT] up: id is %d, status = %d\n", id, finger->fingers[id].status);
            input_mt_sync(input_dev);	//modfiy by mengkun
        }
    }

    input_report_key(input_dev, BTN_TOUCH, finger_num);
    input_sync(input_dev);

    ts_film_touchplus(finger, finger_num, input_dev);
    if (((g_ts_kit_platform_data.chip_data->easy_wakeup_info.sleep_mode == TS_GESTURE_MODE) ||
         (g_ts_kit_platform_data.chip_data->easy_wakeup_info.palm_cover_flag == true)) &&
        (g_ts_kit_platform_data.feature_info.holster_info.holster_switch == 0))
    {
        input_report_key (input_dev, finger->gesture_wakeup_value, 1);
        input_sync(input_dev);
        input_report_key (input_dev, finger->gesture_wakeup_value, 0);
        input_sync (input_dev);
    }
    if ((g_ts_kit_platform_data.aft_param.aft_enable_flag) && (finger->add_release_flag))
	{
		finger->add_release_flag = 0;	
		input_report_key(input_dev, BTN_TOUCH, 0);
		input_mt_sync(input_dev);
		input_sync(input_dev);
		//TS_LOG_ERR("[MUTI_AFT] report the added release event\n");
	}
    //TS_LOG_ERR("[MUTI_AFT] ts_report_input done, finger_num = %d\n", finger_num);


    atomic_set(&g_ts_kit_data_report_over, 1);
    return 0;
}
static int aft_get_info_misc_open(struct inode* inode, struct file* filp)
{
    return 0;
}

static int aft_get_info_misc_release(struct inode* inode, struct file* filp)
{
    /*if(g_ts_kit_platform_data.aft_param.aft_enable_flag)
    { 
		g_ts_kit_platform_data.fingers_waitq_flag = AFT_WAITQ_WAKEUP;
		wake_up_interruptible(&(g_ts_kit_platform_data.fingers_waitq));
    }*/
    return 0;
}

static long aft_get_info_misc_ioctl(struct file* filp, unsigned int cmd,
                                   unsigned long arg)
{
    long ret;
//TS_LOG_ERR("[MUTI_AFT] aft_get_info_misc_ioctl enter cmd:%d\n",cmd);
    switch (cmd)
    {
        case INPUT_AFT_IOCTL_CMD_GET_TS_FINGERS_INFO:
            ret = ts_ioctl_get_fingers_info(arg);
            break;
        case INPUT_AFT_IOCTL_CMD_GET_ALGO_PARAM_INFO:
            ret = ts_ioctl_get_aft_param_info(arg);
            break;
        default:
            TS_LOG_ERR("cmd unkown.\n");
            ret = -EINVAL;
    }

    return ret;
}

static const struct file_operations g_aft_get_info_misc_fops =
{
    .owner = THIS_MODULE,
    .open = aft_get_info_misc_open,
    .release = aft_get_info_misc_release,
    .unlocked_ioctl = aft_get_info_misc_ioctl,
};
static struct miscdevice g_aft_get_info_misc_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_AFT_GET_INFO,
    .fops = &g_aft_get_info_misc_fops,
};
static int aft_set_info_misc_open(struct inode* inode, struct file* filp)
{
    return 0;
}

static int aft_set_info_misc_release(struct inode* inode, struct file* filp)
{
    /*if(g_ts_kit_platform_data.aft_param.aft_enable_flag)
    { 
		g_ts_kit_platform_data.fingers_waitq_flag = AFT_WAITQ_WAKEUP;
		wake_up_interruptible(&(g_ts_kit_platform_data.fingers_waitq));
    }*/
    return 0;
}

static long aft_set_info_misc_ioctl(struct file* filp, unsigned int cmd,
                                   unsigned long arg)
{
    long ret;
//TS_LOG_ERR("[MUTI_AFT] aft_Set_info_misc_ioctl enter cmd:%d\n",cmd);
    switch (cmd)
    {
        case INPUT_AFT_IOCTL_CMD_SET_COORDINATES:
            ret = ts_ioctl_set_coordinates(arg);
            break;
        default:
            TS_LOG_ERR("cmd unkown.\n");
            ret = -EINVAL;
    }

    return ret;
}

static const struct file_operations g_aft_set_info_misc_fops =
{
    .owner = THIS_MODULE,
    .open = aft_set_info_misc_open,
    .release = aft_set_info_misc_release,
    .unlocked_ioctl = aft_set_info_misc_ioctl,
};
static struct miscdevice g_aft_set_info_misc_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_AFT_SET_INFO,
    .fops = &g_aft_set_info_misc_fops,
};
static int ts_input_open(struct input_dev* dev)
{

    TS_LOG_DEBUG("input_open called:%d\n", dev->users);
    return NO_ERR;
}

static void ts_input_close(struct input_dev* dev)
{
    TS_LOG_DEBUG("input_close called:%d\n", dev->users);
}
static int ts_kit_input_tp_device_register(struct input_dev *dev)
{
    int error = NO_ERR;
    struct input_dev* input_dev = NULL;

    input_dev = input_allocate_device();
    if (!input_dev)
    {
        TS_LOG_ERR("failed to allocate memory for input tp dev\n");
        error = -ENOMEM;
        goto err_out;
    }

    input_dev->name = TS_DEV_NAME;
    if (g_ts_kit_platform_data.bops->btype == TS_BUS_I2C)
    { input_dev->id.bustype = BUS_I2C; }
    else if (g_ts_kit_platform_data.bops->btype == TS_BUS_SPI)
    { input_dev->id.bustype = BUS_SPI; }
    input_dev->dev.parent = &g_ts_kit_platform_data.ts_dev->dev;
    input_dev->open = ts_input_open;
    input_dev->close = ts_input_close;
    g_ts_kit_platform_data.input_dev = input_dev;

    if (g_ts_kit_platform_data.chip_data->ops->chip_input_config) //config input for diff chip
    {
        error = g_ts_kit_platform_data.chip_data->ops->chip_input_config(g_ts_kit_platform_data.input_dev);
    }
    if (error)
    {
        goto err_free_dev;
    }

    input_set_drvdata(input_dev, &g_ts_kit_platform_data);

    error = input_register_device(input_dev);
    if (error)
    {
        TS_LOG_ERR("input dev register failed : %d\n", error);
        goto err_free_dev;
    }

    TS_LOG_INFO("%s exit, %d, error is %d\n", __func__, __LINE__, error);

    return error;

err_free_dev:
    input_free_device(input_dev);
err_out:
    return error;
}

static int ts_kit_input_pen_device_register(struct input_dev *dev)
{
    int error = NO_ERR;
    struct input_dev* pen_dev = NULL;

    pen_dev = input_allocate_device();
    if (!pen_dev)
    {
        TS_LOG_ERR("failed to allocate memory for input pen dev\n");
        error = -ENOMEM;
        goto err_out;
    }

    pen_dev->name = TS_PEN_DEV_NAME;
    g_ts_kit_platform_data.pen_dev = pen_dev;

    if (g_ts_kit_platform_data.chip_data && g_ts_kit_platform_data.chip_data->ops && g_ts_kit_platform_data.chip_data->ops->chip_input_pen_config) {
       error = g_ts_kit_platform_data.chip_data->ops->chip_input_pen_config(g_ts_kit_platform_data.pen_dev);
        if (error) {
            goto err_free_dev;
        }
    }
    input_set_drvdata(pen_dev, &g_ts_kit_platform_data);

    error = input_register_device(pen_dev);
    if (error)
    {
        TS_LOG_ERR("input dev register failed : %d\n", error);
        goto err_free_dev;
    }
    return error;

err_free_dev:
    input_free_device(pen_dev);
err_out:
    return error;
}

static int ts_kit_input_device_register(struct input_dev *dev)
{
    int error = NO_ERR;

    //dev is null, no use
    error = ts_kit_input_tp_device_register(dev);
    if (error)
    {
        TS_LOG_ERR("failed to register for input tp dev\n");
        error = -ENOMEM;
        goto err_out;
    }

    if(g_ts_kit_platform_data.feature_info.pen_info.pen_supported)
    {
        error = ts_kit_input_pen_device_register(dev);
        if (error)
        {
            TS_LOG_ERR("failed to register for input pen dev\n");
            error = -ENOMEM;
            goto err_free_dev;
        }
    }

    return error;

err_free_dev:
    input_free_device(g_ts_kit_platform_data.input_dev);
err_out:
    return error;
}

static int ts_kit_pm_init(void)
{
    int error = NO_ERR;

#if defined(CONFIG_FB)
    if (!g_ts_kit_platform_data.chip_data->use_lcdkit_power_notify) {
		g_ts_kit_platform_data.fb_notify.notifier_call = fb_notifier_callback;
		error = fb_register_client(&g_ts_kit_platform_data.fb_notify);
		if (error) {
			TS_LOG_ERR("unable to register fb_notifier: %d\n", error);
			goto err_out;
		}
	}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    g_ts_kit_platform_data.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + TS_SUSPEND_LEVEL;
    g_ts_kit_platform_data.early_suspend.suspend = ts_early_suspend;
    g_ts_kit_platform_data.early_suspend.resume = ts_late_resume;
    register_early_suspend(&g_ts_kit_platform_data.early_suspend);
#endif

    /* ONCELL and AMOLED not need lcdkit power notify */
    if (!(g_tskit_ic_type == ONCELL || g_tskit_ic_type == AMOLED) ||
            g_ts_kit_platform_data.chip_data->use_lcdkit_power_notify) {
        g_ts_kit_platform_data.lcdkit_notify.notifier_call =
                        ts_kit_power_notify_callback;
        error = lcdkit_register_notifier(&g_ts_kit_platform_data.lcdkit_notify);
        if (error) {
            TS_LOG_ERR("unable to register lcdkit_notify: %d\n", error);
            goto err_out;
        }
        TS_LOG_INFO("register lcdkit_notify done\n");
    }
    TS_LOG_INFO("ts_kit_pm_init success\n");
err_out:
    return error;
}

static int ts_kit_irq_init(void)
{
    int error = NO_ERR;
    unsigned int irq_flags;

    g_ts_kit_platform_data.irq_id = gpio_to_irq(g_ts_kit_platform_data.irq_gpio);

    switch (g_ts_kit_platform_data.chip_data->irq_config)
    {
        case TS_IRQ_LOW_LEVEL:
            irq_flags = IRQF_TRIGGER_LOW;
            break;
        case TS_IRQ_HIGH_LEVEL:
            irq_flags = IRQF_TRIGGER_HIGH;
            break;
        case TS_IRQ_RAISE_EDGE:
            irq_flags = IRQF_TRIGGER_RISING;
            break;
        case TS_IRQ_FALL_EDGE:
            irq_flags = IRQF_TRIGGER_FALLING;
            break;
        default:
            TS_LOG_ERR("ts irq_config invaild\n");
            goto err_out;
    }

    atomic_set(&g_ts_kit_platform_data.state, TS_WORK);//avoid 1st irq unable to handler
    atomic_set(&g_ts_kit_platform_data.power_state, TS_WORK);
    error = request_irq(g_ts_kit_platform_data.irq_id, ts_irq_handler, irq_flags | IRQF_NO_SUSPEND, "ts", &g_ts_kit_platform_data);
    if (error)
    {
        TS_LOG_ERR("ts request_irq failed\n");
		goto err_out;
    }
    TS_LOG_INFO("ts_kit_irq_init success\n");
err_out:
    return error;
}

static void ts_get_brightness_info(void)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	g_ts_kit_lcd_brightness_info = 0;

	TS_LOG_INFO("ts_get_brightness_info called\n");
	if(g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) {
		struct ts_cmd_node cmd;
		memset(&cmd, 0, sizeof(struct ts_cmd_node));
		cmd.command = TS_READ_BRIGHTNESS_INFO;
		error = ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
	} else {
		if (dev->ops->chip_get_brightness_info) {
			g_ts_kit_lcd_brightness_info = dev->ops->chip_get_brightness_info();
			TS_LOG_INFO("ts_get_brightness_info  brightness data:%d\n", g_ts_kit_lcd_brightness_info);
		}
	}
	return;
}

static int ts_get_brightness_info_cmd(void)
{
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	int rc = NO_ERR;
	g_ts_kit_lcd_brightness_info = 0;
	TS_LOG_INFO("ts_get_brightness_info_cmd called\n");

	if (dev->ops->chip_get_brightness_info) {
		if(g_ts_kit_platform_data.chip_data->is_direct_proc_cmd){
			rc = dev->ops->chip_get_brightness_info();
			if(rc > 0){
				g_ts_kit_lcd_brightness_info = rc;
				rc = NO_ERR;
			}
		} else {
			g_ts_kit_lcd_brightness_info = dev->ops->chip_get_brightness_info();
		}
		TS_LOG_INFO("ts_get_brightness_info_cmd brightness data:%d\n", g_ts_kit_lcd_brightness_info);
	}
	return rc;
}
static int ts_kit_update_firmware(void)
{
	int error = NO_ERR;
	unsigned int touch_recovery_mode;
	unsigned int charge_flag;

	/*get_boot_into_recovery_flag need to be added later*/
	touch_recovery_mode = get_into_recovery_flag_adapter();
	charge_flag = get_pd_charge_flag_adapter();

	/*do not do boot fw update on recovery mode*/
	TS_LOG_INFO("touch_recovery_mode is %d, charge_flag:%u\n",
		touch_recovery_mode, charge_flag);
	if ((!touch_recovery_mode) && (!charge_flag)) {
		error = try_update_firmware();
		if (error) {
			TS_LOG_ERR("return fail : %d\n", error);
			goto err_out;
		}
	} else if (charge_flag &&
		g_ts_kit_platform_data.chip_data->download_fw_incharger) {
		error = try_update_firmware();
		if (error) {
			TS_LOG_ERR("return fail : %d\n", error);
			goto err_out;
		}
	}
err_out:
	return error;

}

static void ts_kit_status_check_init(void)
{
    if (g_ts_kit_platform_data.chip_data->need_wd_check_status)
    {
        TS_LOG_INFO("This chip need watch dog to check status\n");
        INIT_WORK(&(g_ts_kit_platform_data.watchdog_work), ts_watchdog_work);
        setup_timer(&(g_ts_kit_platform_data.watchdog_timer), ts_watchdog_timer, (unsigned long)(&g_ts_kit_platform_data));
    }
	return ;
}
static void ts_kit_status_check_start(void)
{
    if (g_ts_kit_platform_data.chip_data->need_wd_check_status &&
        !g_ts_kit_platform_data.chip_data->is_parade_solution)
            ts_start_wd_timer(&g_ts_kit_platform_data);
}

static int ts_send_init_cmd(void)
{
	int error = NO_ERR;
	TS_LOG_INFO("%s Enter\n", __func__);
	if(g_ts_kit_platform_data.chip_data->is_direct_proc_cmd){
		g_ts_kit_platform_data.chip_data->is_can_device_use_int = true;
		struct ts_cmd_node cmd;
		cmd.command = TS_TP_INIT;
		error = ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
		if (error) {
			TS_LOG_ERR("put cmd error :%d\n", error);
			error = -EBUSY;
		}
	} else {
		TS_LOG_ERR("%s, nothing to do\n", __func__);
	}
	return error;
}

static void proc_init_cmd(void){
	schedule_work(&tp_init_work);
	return;
}

static void tp_init_work_fn(struct work_struct *work){
	struct ts_cmd_node use_cmd;
	int i = TS_CMD_QUEUE_SIZE;
	struct ts_cmd_queue *q;
	unsigned long flags;
	struct ts_cmd_node *cmd = &use_cmd;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	q = &g_ts_kit_platform_data.no_int_queue;
	int error = NO_ERR;
	//Call chip init
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = false;
	mutex_lock(&g_ts_kit_platform_data.chip_data->device_call_lock);
	if (dev->ops->chip_init) {
		TS_LOG_INFO("%s, call chip init\n",__func__);
		error = dev->ops->chip_init();
	}
	mutex_unlock(&g_ts_kit_platform_data.chip_data->device_call_lock);

	/*deliver panel_id for sensor, direct_proc_cmd = 1*/
	if(g_ts_kit_platform_data.chip_data->provide_panel_id_support > 0){
		schedule_work(&ts_panel_id_work);
	}

	g_ts_kit_platform_data.chip_data->isbootupdate_finish = true;
	if(error != NO_ERR){
		TS_LOG_ERR("%s,chip init fail with error:%d\n",__func__,error);
		return;
	}
	TS_LOG_INFO("%s, chip init done\n",__func__);
	while(i-->0){
		spin_lock_irqsave(&q->spin_lock, flags);
		smp_wmb();
		if (!q->cmd_count) {
			TS_LOG_DEBUG("queue is empty\n");
			spin_unlock_irqrestore(&q->spin_lock, flags);
			break;
		}
		memcpy(cmd, &q->ring_buff[q->rd_index], sizeof(struct ts_cmd_node));
		q->cmd_count--;
		q->rd_index++;
		q->rd_index %= q->queue_size;
		smp_mb();
		spin_unlock_irqrestore(&q->spin_lock, flags);
		error = ts_kit_proc_command_directly(cmd);
		if(error != NO_ERR){
			TS_LOG_INFO("%s process init cmd %d error",__func__, cmd->command);
		}
	}
	if(g_ts_kit_platform_data.chip_data->is_parade_solution){
		ts_start_wd_timer(&g_ts_kit_platform_data);
	}
}

int ts_kit_put_one_cmd_direct_sync(struct ts_cmd_node *cmd, int timeout)
{
	int error = NO_ERR;
	TS_LOG_INFO("%s Enter\n",__func__);
	if(g_ts_kit_platform_data.chip_data->is_parade_solution == 0){
		return ts_kit_put_one_cmd(cmd,timeout);
	}

	if((atomic_read(&g_ts_kit_platform_data.state) == TS_UNINIT)){
		error = -EIO;
		return error;
	}
	if((atomic_read(&g_ts_kit_platform_data.state) == TS_SLEEP)
	     || (atomic_read(&g_ts_kit_platform_data.state) == TS_WORK_IN_SLEEP)){
	     TS_LOG_INFO("%s In Sleep State\n",__func__);
		 error = -EIO;
	     return error;
	}

	return error;
}
int ts_kit_put_one_cmd(struct ts_cmd_node* cmd, int timeout)
{
    int error = -EIO;
    unsigned long flags;
    struct ts_cmd_queue* q;
    struct ts_cmd_sync* sync = NULL;

    if (!cmd)
    {
        TS_LOG_ERR("find null pointer\n");
        goto out;
    }

    if ((TS_UNINIT == atomic_read(&g_ts_kit_platform_data.state)) 
		           && (cmd->command != TS_CHIP_DETECT))
    {
        TS_LOG_ERR("ts module not initialize\n");
        goto out;
    }

    if (timeout)
    {
        sync = (struct ts_cmd_sync*)kzalloc(sizeof(struct ts_cmd_sync), GFP_KERNEL);
        if (NULL == sync)
        {
            TS_LOG_ERR("failed to kzalloc completion\n");
            error = -ENOMEM;
            goto out;
        }
        init_completion(&sync->done);
        atomic_set(&sync->timeout_flag, TS_NOT_TIMEOUT);
        cmd->sync = sync;
    }
    else
    {
        cmd->sync = NULL;
    }

    if(g_ts_kit_platform_data.chip_data == NULL) {
        q = &g_ts_kit_platform_data.queue;
    }else{
        if((g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) &&
            (g_ts_kit_platform_data.chip_data->is_can_device_use_int==false) &&
            (cmd->command != TS_CHIP_DETECT)){
                if(cmd->command == TS_INT_PROCESS)
                    goto out; //Not use INT in the init process
                q = &g_ts_kit_platform_data.no_int_queue;
        } else {
            q = &g_ts_kit_platform_data.queue;
        }
    }
    cmd->ts_cmd_check_key = TS_CMD_CHECK_KEY;
    spin_lock_irqsave(&q->spin_lock, flags);
    smp_wmb();
    if (q->cmd_count == q->queue_size)
    {
        spin_unlock_irqrestore(&q->spin_lock, flags);
        TS_LOG_ERR("queue is full\n");
        WARN_ON(1);
        error = -EIO;
        goto free_sync;
    }
    memcpy(&q->ring_buff[q->wr_index], cmd, sizeof(struct ts_cmd_node));
    q->cmd_count++;
    q->wr_index++;
    q->wr_index %= q->queue_size;
    smp_mb();
    spin_unlock_irqrestore(&q->spin_lock, flags);
    TS_LOG_DEBUG("put one cmd :%d in ring buff\n", cmd->command);
    error = NO_ERR;
    wake_up_process(g_ts_kit_platform_data.ts_task); //wakeup process

    if (timeout && !(wait_for_completion_timeout(&sync->done, abs(timeout)*HZ)))
    {
        atomic_set(&sync->timeout_flag, TS_TIMEOUT);
        TS_LOG_ERR("wait for cmd respone timeout\n");
        error = -EBUSY;
        goto out;
    }
    smp_wmb();

free_sync:
    if (sync)
    {
        kfree(sync);
    }
out:
    return error;
}
EXPORT_SYMBOL(ts_kit_put_one_cmd);
static int get_one_cmd(struct ts_cmd_node* cmd)
{
    unsigned long flags;
    int error = -EIO;
    struct ts_cmd_queue* q;

    if (unlikely(!cmd))
    {
        TS_LOG_ERR("find null pointer\n");
        goto out;
    }

    q = &g_ts_kit_platform_data.queue;

    spin_lock_irqsave(&q->spin_lock, flags);
    smp_wmb();
    if (!q->cmd_count)
    {
        TS_LOG_DEBUG("queue is empty\n");
        spin_unlock_irqrestore(&q->spin_lock, flags);
        goto out;
    }
    memcpy(cmd, &q->ring_buff[q->rd_index], sizeof(struct ts_cmd_node));
    q->cmd_count--;
    q->rd_index++;
    q->rd_index %= q->queue_size;
    smp_mb();
    spin_unlock_irqrestore(&q->spin_lock, flags);
    TS_LOG_DEBUG("get one cmd :%d from ring buff\n", cmd->command);
    error = NO_ERR;

out:
    return error;
}

static int ts_proc_command(struct ts_cmd_node* cmd)
{
    int error = NO_ERR;

    struct ts_cmd_sync* sync = NULL;
    struct ts_cmd_node* proc_cmd = cmd;
    struct ts_cmd_node* out_cmd = &pang_cmd_buff;

    if (!cmd || cmd->ts_cmd_check_key != TS_CMD_CHECK_KEY) {
        TS_LOG_ERR("invalid cmd, no need to process\n");
        goto out;
    }
    sync = cmd->sync;

    //discard timeout cmd to fix panic
    if (sync && atomic_read(&sync->timeout_flag) == TS_TIMEOUT)
    {
        kfree(sync);
        goto out;
    }

    if (!ts_cmd_need_process(proc_cmd))
    {
        TS_LOG_INFO("no need to process cmd:%d", proc_cmd->command);
        goto out;
    }

related_proc:
    out_cmd->command = TS_INVAILD_CMD;

    switch (proc_cmd->command)
    {
        case TS_INT_PROCESS:
            ts_proc_bottom_half(proc_cmd, out_cmd);
            if (strncmp(g_ts_kit_platform_data.product_name,"ares",sizeof("ares")) ||
                  strncmp(g_ts_kit_platform_data.chip_data->chip_name,"parade",sizeof("parade")))
            {
                enable_irq(g_ts_kit_platform_data.irq_id);
            }
            break;
        case TS_INPUT_ALGO:
            ts_algo_calibrate(proc_cmd, out_cmd);
            break;
	case TS_PALM_KEY:
            ts_palm_report(proc_cmd,out_cmd);
            break;
        case TS_REPORT_INPUT:
            ts_report_input(proc_cmd, out_cmd);
            break;
        case TS_REPORT_PEN:
            ts_report_pen(proc_cmd, out_cmd);
            break;
        case TS_POWER_CONTROL:
            ts_power_control(g_ts_kit_platform_data.irq_id, proc_cmd, out_cmd);
            break;
        case TS_FW_UPDATE_BOOT:
            disable_irq(g_ts_kit_platform_data.irq_id);
            ts_fw_update_boot(proc_cmd, out_cmd);
            enable_irq(g_ts_kit_platform_data.irq_id);
            break;
        case TS_FW_UPDATE_SD:
            disable_irq(g_ts_kit_platform_data.irq_id);
            ts_fw_update_sd(proc_cmd, out_cmd);
            enable_irq(g_ts_kit_platform_data.irq_id);
            break;
        case TS_DEBUG_DATA:
            disable_irq(g_ts_kit_platform_data.irq_id);
            ts_read_debug_data(proc_cmd, out_cmd, sync);
            enable_irq(g_ts_kit_platform_data.irq_id);
            break;
        case TS_READ_RAW_DATA:
            disable_irq(g_ts_kit_platform_data.irq_id);
            ts_read_rawdata(proc_cmd, out_cmd, sync);
            enable_irq(g_ts_kit_platform_data.irq_id);
            break;
	case TS_READ_CALIBRATION_DATA:
		disable_irq(g_ts_kit_platform_data.irq_id);
		ts_read_calibration_data(proc_cmd, out_cmd, sync);
		enable_irq(g_ts_kit_platform_data.irq_id);
		break;
	case TS_GET_CALIBRATION_INFO:
		ts_get_calibration_info(proc_cmd, out_cmd, sync);
		break;
	case TS_OEM_INFO_SWITCH:
		ts_oem_info_switch(proc_cmd, out_cmd, sync);
		break;
        case TS_GET_CHIP_INFO:
            ts_get_chip_info(proc_cmd, out_cmd);
            break;
        case TS_SET_INFO_FLAG:
            ts_set_info_flag(proc_cmd, out_cmd);
            break;
        case TS_CALIBRATE_DEVICE:
            ts_calibrate(proc_cmd, out_cmd, sync);
            break;
        case TS_CALIBRATE_DEVICE_LPWG:
            ts_calibrate_wakeup_gesture(proc_cmd, out_cmd, sync);
            break;
        case TS_DSM_DEBUG:
            ts_dsm_debug(proc_cmd, out_cmd);
            break;
        case TS_GLOVE_SWITCH:
            ts_glove_switch(proc_cmd, out_cmd);
            break;
        case TS_TEST_TYPE:
            ts_get_capacitance_test_type(proc_cmd, out_cmd, sync);
            break;
        case TS_PALM_SWITCH:
            ts_palm_switch(proc_cmd, out_cmd, sync);
            break;
        case TS_HAND_DETECT:
            ts_hand_detect(proc_cmd, out_cmd);
            break;
        case TS_FORCE_RESET:
            disable_irq(g_ts_kit_platform_data.irq_id);
            ts_stop_wd_timer(&g_ts_kit_platform_data);
            ts_force_reset(proc_cmd, out_cmd);
            ts_start_wd_timer(&g_ts_kit_platform_data);
            enable_irq(g_ts_kit_platform_data.irq_id);
            break;
        case TS_INT_ERR_OCCUR:
            ts_stop_wd_timer(&g_ts_kit_platform_data);
            ts_int_err_process(proc_cmd, out_cmd);
            enable_irq(g_ts_kit_platform_data.irq_id);
            ts_start_wd_timer(&g_ts_kit_platform_data);
            break;
        case TS_ERR_OCCUR:
            disable_irq(g_ts_kit_platform_data.irq_id);
            ts_stop_wd_timer(&g_ts_kit_platform_data);
            ts_err_process(proc_cmd, out_cmd);
            ts_start_wd_timer(&g_ts_kit_platform_data);
            enable_irq(g_ts_kit_platform_data.irq_id);
            break;
        case TS_CHECK_STATUS:
            ts_check_status(proc_cmd, out_cmd);
            break;
        case TS_WAKEUP_GESTURE_ENABLE:
            ts_wakeup_gesture_enable_switch(proc_cmd, out_cmd);
            break;
        case TS_HOLSTER_SWITCH:
            ts_holster_switch(proc_cmd, out_cmd);
            break;
        case TS_ROI_SWITCH:
            ts_roi_switch(proc_cmd, out_cmd);
            break;
        case TS_TOUCH_WINDOW:
            ts_touch_window(proc_cmd, out_cmd);
            break;
        case TS_CHIP_DETECT:
            ts_chip_detect(proc_cmd, out_cmd);
            break;
#if defined(HUAWEI_CHARGER_FB)
	 case TS_CHARGER_SWITCH:
		ts_kit_charger_switch(proc_cmd, out_cmd);
		break;
#endif
        case TS_REGS_STORE:
            ts_chip_regs_operate(proc_cmd, out_cmd, sync);
            break;
        case TS_TEST_CMD:
            ts_test_cmd(proc_cmd, out_cmd);
            break;
        case TS_HARDWARE_TEST:
            ts_special_hardware_test_switch(proc_cmd, out_cmd);
            break;
	case TS_READ_BRIGHTNESS_INFO:
		ts_get_brightness_info_cmd();
		break;
	case TS_TP_INIT:
		proc_init_cmd();
		break;
	case TS_TOUCH_SWITCH:
		ts_touch_switch_cmd();
		break;
	case TS_FREEBUFF:
		if (proc_cmd->cmd_param.ts_cmd_freehook != NULL 
			  && proc_cmd->cmd_param.prv_params != NULL){
			proc_cmd->cmd_param.ts_cmd_freehook(proc_cmd->cmd_param.prv_params);
		}	
		break;
    default:
            break;
    }

    TS_LOG_DEBUG("command :%d process result:%d \n", proc_cmd->command, error);

    if (out_cmd->command != TS_INVAILD_CMD)
    {
        TS_LOG_DEBUG("related command :%d  need process\n", out_cmd->command);
        swap(proc_cmd, out_cmd);//ping - pang
        goto related_proc;
    }

    if (sync)  //notify wait threads by completion
    {
        smp_mb();
        TS_LOG_DEBUG("wakeup threads in waitqueue\n");
        if (atomic_read(&sync->timeout_flag) == TS_TIMEOUT)
        {
            kfree(sync);
        }
        else
        {
            complete(&sync->done);
        }
    }

out:
    return error;
}

int ts_kit_proc_command_directly(struct ts_cmd_node *cmd)
{
	int error = NO_ERR;
	TS_LOG_DEBUG("%s Enter\n",__func__);
	/*Do not use cmd->sync in this func, setting it as null*/
	cmd->sync = NULL;
	if (!ts_cmd_need_process(cmd)) {
		TS_LOG_INFO("%s, no need to process cmd:%d",__func__, cmd->command);
		error = -EIO;
		goto out;
	}
	struct ts_cmd_node outcmd;
	mutex_lock(&g_ts_kit_platform_data.chip_data->device_call_lock);

	switch (cmd->command) {
		case TS_INT_PROCESS:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_INPUT_ALGO:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_REPORT_INPUT:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_POWER_CONTROL:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_FW_UPDATE_BOOT:
			error = ts_fw_update_boot(cmd, &outcmd);
			break;
		case TS_FW_UPDATE_SD:
			error = ts_fw_update_sd(cmd, &outcmd);
			break;
		case TS_DEBUG_DATA:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_READ_RAW_DATA:
			error = ts_read_rawdata(cmd, &outcmd, cmd->sync);
			break;
		case TS_READ_CALIBRATION_DATA:
			error = ts_read_calibration_data(cmd, &outcmd, cmd->sync);
			break;
		case TS_GET_CALIBRATION_INFO:
			error = ts_get_calibration_info(cmd, &outcmd, cmd->sync);
			break;
		case TS_OEM_INFO_SWITCH:
			error = ts_oem_info_switch(cmd, &outcmd, cmd->sync);
			break;
		case TS_GET_CHIP_INFO:
			error = ts_get_chip_info(cmd, &outcmd);
			break;
		case TS_SET_INFO_FLAG:
			error = ts_set_info_flag(cmd, &outcmd);
			break;
		case TS_CALIBRATE_DEVICE:
			error = ts_calibrate(cmd, &outcmd, cmd->sync);
			break;
		case TS_CALIBRATE_DEVICE_LPWG:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_DSM_DEBUG:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_GLOVE_SWITCH:
			error = ts_glove_switch(cmd, &outcmd);
			break;
		case TS_TEST_TYPE:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_PALM_SWITCH:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_HAND_DETECT:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_FORCE_RESET:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_INT_ERR_OCCUR:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_ERR_OCCUR:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_CHECK_STATUS:
			ts_check_status(cmd, &outcmd);
			break;
		case TS_WAKEUP_GESTURE_ENABLE:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_HOLSTER_SWITCH:
			error = ts_holster_switch(cmd, &outcmd);
			break;
		case TS_ROI_SWITCH:
			error = ts_roi_switch(cmd, &outcmd);
			break;
		case TS_TOUCH_WINDOW:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
#if defined(HUAWEI_CHARGER_FB)
		case TS_CHARGER_SWITCH:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
#endif
		case TS_REGS_STORE:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_TEST_CMD:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_HARDWARE_TEST:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_READ_BRIGHTNESS_INFO:
			error = ts_get_brightness_info_cmd();
			break;
		case TS_TOUCH_SWITCH:
			ts_touch_switch_cmd();
			break;
		default:
			TS_LOG_ERR("%s, command %d unknown!",__func__, cmd->command);
			break;
		}
		mutex_unlock(&g_ts_kit_platform_data.chip_data->device_call_lock);
		TS_LOG_DEBUG("%s, command :%d process result:%d \n",__func__, cmd->command,
				 error);
out:
	return error;
}

#if defined (CONFIG_HUAWEI_DSM)
void ts_dmd_report(int dmd_num, const char* pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);

    char input_buf[TS_CHIP_DMD_REPORT_SIZE] = {0};
    char report_buf[TS_CHIP_DMD_REPORT_SIZE] = {0};

    vsnprintf(input_buf, sizeof(input_buf) - 1, pszFormat, args);
    va_end(args);
    snprintf(report_buf, sizeof(report_buf), "%stp state:%d", input_buf, atomic_read(&g_ts_kit_platform_data.power_state));

    if (!dsm_client_ocuppy(ts_dclient)) {
        dsm_client_record(ts_dclient, report_buf);
        dsm_client_notify(ts_dclient, dmd_num);
        TS_LOG_INFO("ts_dmd_report %s\n", report_buf);
    }
    return;
}
EXPORT_SYMBOL(ts_dmd_report);
#endif

static int ts_kit_init(void)
{
    int error = NO_ERR;
    struct input_dev* input_dev = NULL;
    atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
    atomic_set(&g_ts_kit_platform_data.ts_esd_state, TS_NO_ESD);
    TS_LOG_INFO("ts_kit_init\n");
    g_ts_kit_platform_data.edge_wideth = EDGE_WIDTH_DEFAULT;
    TS_LOG_DEBUG("ts init: cmd queue size : %d\n", TS_CMD_QUEUE_SIZE);
    wake_lock_init(&g_ts_kit_platform_data.ts_wake_lock, WAKE_LOCK_SUSPEND, "ts_wake_lock");
	g_ts_kit_platform_data.panel_id = 0xFF;
   
    error = ts_kit_parse_config();
    if (error)
    {
        TS_LOG_ERR("ts init parse config failed : %d\n", error);
        goto err_out;
    }
    TS_LOG_INFO("ts_kit_parse_config success\n");
    error = ts_kit_create_sysfs();
    if (error)
    {
        TS_LOG_ERR("ts init create sysfs failed : %d\n", error);
        goto err_out;
    }
    TS_LOG_INFO("g_tskit_ic_type is %d,g_tskit_pt_station_flag is %d \n",g_tskit_ic_type,g_tskit_pt_station_flag);    
    error = ts_kit_chip_init();
    if (error)
    {
        TS_LOG_ERR("chip init failed : %d,  try fw update again\n", error);
    }

	if(g_ts_kit_platform_data.chip_data->is_direct_proc_cmd){
		g_ts_kit_platform_data.chip_data->is_can_device_use_int = false;
		g_ts_kit_platform_data.no_int_queue.rd_index = 0;
		g_ts_kit_platform_data.no_int_queue.wr_index = 0;
		g_ts_kit_platform_data.no_int_queue.cmd_count = 0;
		g_ts_kit_platform_data.no_int_queue.queue_size = TS_CMD_QUEUE_SIZE;
		spin_lock_init(&g_ts_kit_platform_data.no_int_queue.spin_lock);
		INIT_WORK(&tp_init_work, tp_init_work_fn);
	}

	/*deliver panel_id for sensor, direct_proc_cmd = 0*/
	if(!g_ts_kit_platform_data.chip_data->is_direct_proc_cmd
		&& g_ts_kit_platform_data.chip_data->provide_panel_id_support > 0 ){
		schedule_work(&ts_panel_id_work);
	}

/*     if (not_get_special_tp_node )
    {
        TS_LOG_ERR("get special  TP node failed not_get_special_tp_node = %d \n",not_get_special_tp_node);
	 error =not_get_special_tp_node;
        goto err_out;
    } */

    error = ts_register_algo();
    if (error) 
    {
         TS_LOG_ERR("ts register algo failed : %d\n", error);
	     goto  err_remove_sysfs;
    }
    if(g_ts_kit_platform_data.aft_param.aft_enable_flag)
    {
	    error = misc_register(&g_aft_get_info_misc_device);
	    if (error)
	    {
	        TS_LOG_ERR("Failed to register misc device\n");
	        goto err_remove_sysfs;
	    }
	    error = misc_register(&g_aft_set_info_misc_device);
	    if (error)
	    {
	        TS_LOG_ERR("Failed to register misc device\n");
	        goto err_remove_sysfs;
	    }
	    sema_init(&g_ts_kit_platform_data.fingers_aft_send,0);
        atomic_set(&g_ts_kit_platform_data.fingers_waitq_flag, AFT_WAITQ_IDLE);
    }
    error = ts_kit_input_device_register(input_dev);
    if (error)
    {
        TS_LOG_ERR("ts init input device register failed : %d\n", error);
        goto err_remove_sysfs;
    }
    ts_kit_status_check_init();
    error = ts_kit_pm_init();
    if (error)
    {
        TS_LOG_ERR("ts init pm init failed : %d\n", error);
        goto err_free_input_dev;
    }
#if defined (CONFIG_TEE_TUI)
	g_ts_kit_platform_data.chip_data->tui_data = &tee_tui_data;
	register_tui_driver(tui_tp_init, "tp", g_ts_kit_platform_data.chip_data->tui_data);
#endif
    error = ts_kit_irq_init();
    if (error)
    {
        TS_LOG_ERR("ts init irq_init failed : %d\n", error);
        goto err_unregister_suspend;
    }
    /*get brightness info*/
    ts_get_brightness_info();
    error = ts_kit_update_firmware();
    if (error)
    {
        TS_LOG_ERR("ts init update_firmware failed : %d\n", error);
        goto err_firmware_update;
    }
    ts_send_roi_cmd(TS_ACTION_READ, NO_SYNC_TIMEOUT);	/*roi function set as default by TP firmware */
    ts_send_init_cmd();/*Send this cmd to make sure all the cmd in the init is called*/
#if defined (CONFIG_HISI_BCI_BATTERY)
    if (g_ts_kit_platform_data.register_charger_notifier) {
        ts_kit_charger_notifier_register();
        TS_LOG_INFO("charger notifier is register\n");
    }
#endif

    ts_kit_status_check_start();
    error = NO_ERR;
    TS_LOG_INFO("ts_kit_init called out\n");
    goto out;

err_firmware_update:
    free_irq(g_ts_kit_platform_data.irq_id, &g_ts_kit_platform_data);
err_unregister_suspend:
#if defined(CONFIG_FB)
    if (fb_unregister_client(&g_ts_kit_platform_data.fb_notify))
    { TS_LOG_ERR("error occurred while unregistering fb_notifier.\n"); }
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    unregister_early_suspend(&g_ts_kit_platform_data.early_suspend);
#endif
err_free_input_dev:
    if(NULL != input_dev)
    {
        input_unregister_device(input_dev);
        input_free_device(input_dev);
    }
    misc_deregister(&g_aft_get_info_misc_device);
    misc_deregister(&g_aft_set_info_misc_device);
err_remove_sysfs:
    sysfs_remove_link(NULL, "touchscreen");
    sysfs_remove_group(&g_ts_kit_platform_data.ts_dev->dev.kobj, &ts_attr_group);
    platform_device_del(g_ts_kit_platform_data.ts_dev);
    platform_device_put(g_ts_kit_platform_data.ts_dev);
err_out:
    atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
    atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);
    wake_lock_destroy(&g_ts_kit_platform_data.ts_wake_lock);
out:
    TS_LOG_INFO("ts_init, g_ts_kit_platform_data.state : %d\n", atomic_read(&g_ts_kit_platform_data.state));
    if (error) {
        TS_LOG_ERR("ts_init  failed\n");
#if defined (CONFIG_HUAWEI_DSM)
        ts_dmd_report(DSM_TP_INIT_ERROR_NO, "try to client record 926004032 for tp init error \n");
#endif
    }
    return error;
}
static void ts_ic_shutdown(void)
{
    struct ts_kit_device_data* dev = g_ts_kit_platform_data.chip_data;
    if (dev->ops->chip_shutdown)
    { dev->ops->chip_shutdown(); }
    return;
}
static void ts_kit_exit(void)
{
    atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
    atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);
    disable_irq(g_ts_kit_platform_data.irq_id);
    ts_ic_shutdown();
    free_irq(g_ts_kit_platform_data.irq_id, &g_ts_kit_platform_data);
    //sysfs_remove_group(&g_ts_kit_platform_data.ts_dev->dev.kobj, &ts_attr_group);
#if defined(CONFIG_FB)
    if (fb_unregister_client(&g_ts_kit_platform_data.fb_notify)){ 
    	    TS_LOG_ERR("error occurred while unregistering fb_notifier.\n"); 
	}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    unregister_early_suspend(&g_ts_kit_platform_data.early_suspend);
#endif
    input_unregister_device(g_ts_kit_platform_data.input_dev);
    input_free_device(g_ts_kit_platform_data.input_dev);
    misc_deregister(&g_aft_get_info_misc_device);
    misc_deregister(&g_aft_set_info_misc_device);
    sysfs_remove_link(NULL, "touchscreen");
    sysfs_remove_group(&g_ts_kit_platform_data.ts_dev->dev.kobj, &ts_attr_group);
    wake_lock_destroy(&g_ts_kit_platform_data.ts_wake_lock);
    platform_device_unregister(g_ts_kit_platform_data.ts_dev);
    ts_destory_client();
#if defined(HUAWEI_CHARGER_FB)
	if ((NULL != g_ts_kit_platform_data.charger_detect_notify.notifier_call)
		&& g_ts_kit_platform_data.register_charger_notifier) {
		charger_type_notifier_unregister(&g_ts_kit_platform_data. charger_detect_notify);
		TS_LOG_INFO("charger_type_notifier_unregister called\n");
	}
#endif	
    memset(&g_ts_kit_platform_data, 0, sizeof(struct ts_kit_platform_data));
    TS_LOG_ERR("ts_thread exited\n");
}
static bool ts_task_continue(void)
{
    bool task_continue = true;
    unsigned long flags;
    TS_LOG_DEBUG("prepare enter idle\n");

repeat:
    if (unlikely(kthread_should_stop()))
    {
        task_continue = false;
        goto out;
    }
    spin_lock_irqsave(&g_ts_kit_platform_data.queue.spin_lock, flags);
    smp_wmb();
    if (g_ts_kit_platform_data.queue.cmd_count)
    {
        set_current_state(TASK_RUNNING);
        TS_LOG_DEBUG("ts task state to  TASK_RUNNING\n");
        goto out_unlock;
    }
    else
    {
        set_current_state(TASK_INTERRUPTIBLE);
        TS_LOG_DEBUG("ts task state to  TASK_INTERRUPTIBLE\n");
        spin_unlock_irqrestore(&g_ts_kit_platform_data.queue.spin_lock, flags);
        schedule();
        goto repeat;
    }

out_unlock:
    spin_unlock_irqrestore(&g_ts_kit_platform_data.queue.spin_lock, flags);
out:
    return task_continue;
}
static int ts_thread(void* p)
{
    static const struct sched_param param =
    {
        //.sched_priority = MAX_USER_RT_PRIO / 2,
        .sched_priority = 99,
    };
    smp_wmb();
    TS_LOG_INFO("ts_thread\n");

    memset(&ping_cmd_buff, 0, sizeof(struct ts_cmd_node));
    memset(&pang_cmd_buff, 0, sizeof(struct ts_cmd_node));
    smp_mb();
    sched_setscheduler(current, SCHED_RR, &param);

    while (ts_task_continue())
    {
        while (!get_one_cmd(&ping_cmd_buff)) //get one command
        {
            ts_proc_command(&ping_cmd_buff);
            memset(&ping_cmd_buff, 0, sizeof(struct ts_cmd_node));
            memset(&pang_cmd_buff, 0, sizeof(struct ts_cmd_node));
        }
    }

    TS_LOG_ERR("ts thread stop\n");
    ts_kit_exit();

err_out:
     platform_device_unregister(g_ts_kit_platform_data.ts_dev);
     ts_destory_client();
     if (g_ts_kit_platform_data.reset_gpio) {
         gpio_free(g_ts_kit_platform_data.reset_gpio);
     }
     gpio_free (g_ts_kit_platform_data.irq_gpio);
     atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
     atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);
    return NO_ERR;
}

void ts_kit_chip_detect(struct ts_kit_device_data* chipdata,int timeout)
{
    int error = 0;
    struct ts_cmd_node cmd;
    TS_LOG_INFO("ts_kit_chip_detect called\n");

    memset(&cmd, 0,sizeof(struct ts_cmd_node) );
    //atomic_set(&g_ts_kit_platform_data.state, TS_WORK);
    cmd.command = TS_CHIP_DETECT;
    cmd.cmd_param.pub_params.chip_data = chipdata;

    error = ts_kit_put_one_cmd(&cmd, timeout);
    if (error)
    {
        TS_LOG_ERR("ts_kit_chip_detect, put cmd error :%d\n", error);
    }

    return;
}

static u8 ts_init_flag = 0;
int huawei_ts_chip_register(struct ts_kit_device_data* chipdata)
{
    int error = NO_ERR;
    TS_LOG_INFO("huawei_ts_chip_register called here\n");
    if (NULL == chipdata)
    {
        TS_LOG_ERR("%s chipdata is null\n", __func__);
        return  -EINVAL;
    }
    if ((ts_init_flag == 1)&&(TS_UNREGISTER ==
    atomic_read(&g_ts_kit_platform_data.register_flag)))
    {
        if (chipdata->ops->chip_detect)
        {
            ts_kit_chip_detect(chipdata,NO_SYNC_TIMEOUT);
        }

    }
    else
    {
        error = -EPERM;
    }

out:
    return error;
}

/************** Begin ts event notify block *********************/
static BLOCKING_NOTIFIER_HEAD (ts_event_nh);
int ts_event_notifier_register(struct notifier_block *nb)
{
	TS_LOG_INFO("%s + \n", __func__);
	if (!nb){
		TS_LOG_ERR("nb == NULL \n");
		return -EINVAL;
	}
	return blocking_notifier_chain_register(&ts_event_nh, nb);
}
EXPORT_SYMBOL_GPL(ts_event_notifier_register);

int ts_event_notifier_unregister(struct notifier_block *nb)
{
	TS_LOG_INFO("%s + \n", __func__);
	if (!nb){
		TS_LOG_ERR("nb == NULL \n");
		return -EINVAL;
	}
	return blocking_notifier_chain_unregister(&ts_event_nh, nb);
}
EXPORT_SYMBOL_GPL(ts_event_notifier_unregister);

// ret: 0 OK, other fail.
int ts_event_notify(ts_notify_event_type event)// for panel use to notify event.
{
	return blocking_notifier_call_chain(&ts_event_nh, (unsigned long)event, NULL);
}
/************** End ts event notify block *********************/

static int __init huawei_ts_module_init(void)
{
    int error = NO_ERR;

    ts_init_flag = 0;

    TS_LOG_INFO("huawei_ts, huawei_ts_module_init called here\n");
    memset(&g_ts_kit_platform_data, 0, sizeof(struct ts_kit_platform_data));
    atomic_set(&g_ts_kit_platform_data.register_flag, TS_UNREGISTER);
    atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);

    error = get_ts_board_info();
    if (error)
    {
        TS_LOG_ERR("get bus info failed :%d\n", error);
        goto out;
    }
    error = ts_create_client();
    if (error)
    {
        TS_LOG_ERR("create device failed :%d\n", error);
        goto out;
    }
    error = ts_gpio_request();
    if (error)
    {
        TS_LOG_ERR("ts_gpio_request failed :%d\n", error);
        goto err_remove_client;
    }
    g_ts_kit_platform_data.ts_dev = platform_device_alloc("huawei_touch", -1);
    if (!g_ts_kit_platform_data.ts_dev)
    {
        TS_LOG_ERR("platform device malloc failed\n");
        error = -ENOMEM;
    	goto err_remove_gpio;
    }
    error = platform_device_add(g_ts_kit_platform_data.ts_dev);
    if (error)
    {
        TS_LOG_ERR("platform device add failed :%d\n", error);
	    goto  err_put_platform_dev;
    }

    ts_init_flag = 1;
    lcd_huawei_ts_kit_register(&ts_kit_ops);
    g_ts_kit_platform_data.ts_init_task = kthread_create(ts_kit_init, &g_ts_kit_platform_data, "ts_init_thread:%d", 0);
    if (IS_ERR(g_ts_kit_platform_data.ts_init_task))
    {
        TS_LOG_ERR("create ts_thread failed\n");
        error = ts_destory_client();
        memset(&g_ts_kit_platform_data, 0, sizeof(struct ts_kit_platform_data));
        error = -EINVAL;
        goto out;
    }
    g_ts_kit_platform_data.ts_task = kthread_create(ts_thread, &g_ts_kit_platform_data, "ts_thread:%d", 0);
    if (IS_ERR(g_ts_kit_platform_data.ts_task))
    {
        TS_LOG_ERR("create ts_thread failed\n");
        error = ts_destory_client();
        memset(&g_ts_kit_platform_data, 0, sizeof(struct ts_kit_platform_data));
        error = -EINVAL;
        goto out;
    }
    g_ts_kit_platform_data.queue.rd_index = 0;
    g_ts_kit_platform_data.queue.wr_index = 0;
    g_ts_kit_platform_data.queue.cmd_count = 0;
    g_ts_kit_platform_data.queue.queue_size = TS_CMD_QUEUE_SIZE;
    spin_lock_init(&g_ts_kit_platform_data.queue.spin_lock);
    /* Attention about smp_mb/rmb/wmb
    Add these driver to avoid  data consistency problem
    ts_thread/ts_probe/irq_handler/ts_kit_put_one_cmd/get_one_cmd
    may run in different cpus and L1/L2 cache data consistency need
    to conside. We use barrier to make sure data consistently*/
    smp_mb();
    wake_up_process(g_ts_kit_platform_data.ts_task);

	TS_LOG_INFO("ts_init called out\n");
	goto out;

err_put_platform_dev:
	  platform_device_put(g_ts_kit_platform_data.ts_dev);
err_remove_gpio:
	gpio_free(g_ts_kit_platform_data.irq_gpio);
	if (g_ts_kit_platform_data.reset_gpio) {
		gpio_free(g_ts_kit_platform_data.reset_gpio);
	}
err_remove_client:
	 ts_destory_client();
out:
    return error;
}

static void __exit huawei_ts_module_exit(void)
{
    TS_LOG_INFO("huawei_ts, huawei_ts_module_exit called here\n");
	if (g_ts_kit_platform_data.ts_task)
		kthread_stop(g_ts_kit_platform_data.ts_task);
#if defined (CONFIG_TEE_TUI)
    unregister_tui_driver("tp");
#endif
 //gpio free;
 //destory i2c client
    return;
}

//module_init(huawei_ts_module_init);
module_init(huawei_ts_module_init);
module_exit(huawei_ts_module_exit);
EXPORT_SYMBOL(g_ts_kit_log_cfg);
EXPORT_SYMBOL(huawei_ts_chip_register);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
