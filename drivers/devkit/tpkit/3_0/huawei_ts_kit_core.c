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
#include <linux/notifier.h>
#ifdef CONFIG_HUAWEI_DEVKIT_MTK_3_0
#include <linux/of_irq.h>
#endif

#define CREATE_TRACE_POINTS
#include "trace-events-touch.h"

#include <huawei_ts_kit.h>
#include <huawei_ts_kit_platform.h>
#include <huawei_ts_kit_misc.h>
//#ifdef CONFIG_HUAWEI_HW_DEV_DCT
//#include <huawei_platform/devdetect/hw_dev_dec.h>
//#endif
//#include <linux/mfd/hisi_hi6xxx_pmic.h>
//#include <linux/hisi/hi6xxx-lcd_type.h>
#include <tpkit_platform_adapter.h>
#include <huawei_ts_kit_api.h>
#include <huawei_ts_kit_algo.h>
#include <linux/hwspinlock.h>
#include "hwspinlock_internal.h"

#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif

#if defined (CONFIG_HISI_BCI_BATTERY)
#include <linux/power/hisi/hisi_bci_battery.h>
#endif

#define SCHEDULE_DELAY_MILLiSECOND      200
#define PROJECT_ID_LEN  10
#define RAWDATA_NUM_OF_TRX_MAX 100
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>

#define LDO17_PHYS_ADDR		(0X93)
#define LSW50_PHYS_ADDR	(0xAC)
static struct dsm_dev dsm_tp = {
	.name = "dsm_tp",
	.device_name = "TP",
	.ic_name = "syn",	/*just for testing, waiting for the module owner revised */
	.module_name = "NNN",
	.fops = NULL,
	.buff_size = 1024,
};

struct dsm_client *ts_dclient = NULL;
EXPORT_SYMBOL(ts_dclient);
#endif
#define	EDGE_WIDTH_DEFAULT	10
struct ts_kit_platform_data g_ts_kit_platform_data;
EXPORT_SYMBOL(g_ts_kit_platform_data);
#if defined (CONFIG_TEE_TUI)
struct ts_tui_data tee_tui_data;
EXPORT_SYMBOL(tee_tui_data);
#endif
u8 g_ts_kit_log_cfg = 0;

static struct ts_cmd_node ping_cmd_buff;
static struct ts_cmd_node pang_cmd_buff;
static struct work_struct tp_init_work;
struct mutex ts_kit_easy_wake_guesure_lock;
/*external variable declare*/
extern const struct attribute_group ts_attr_group;
extern atomic_t g_ts_kit_data_report_over;
/*global variable declare*/
#ifdef CONFIG_HUAWEI_THP
extern int thp_project_id_provider(char *project_id);
#endif
int ts_kit_ops_register(struct ts_kit_ops *ops);
int ts_kit_get_gesture_mode(void);
int tskit_get_status_by_type(int type, int *status);

struct ts_kit_ops ts_kit_ops = {
	.ts_power_notify = ts_kit_power_control_notify,
	.get_tp_status_by_type = tskit_get_status_by_type,
};

int tskit_get_status_by_type(int type, int *status)
{
	int ret = 0;

	if (status == NULL) {
		TS_LOG_ERR("status is null\n");
		return -EINVAL;
	}
	switch (type) {
	case TS_GESTURE_FUNCTION:
		*status = ts_kit_gesture_func;
		break;
	default:
		TS_LOG_ERR("not support type\n");
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int tskit_get_project_id(char *project_id)
{
	if (TS_REGISTER_DONE !=
	    atomic_read(&g_ts_kit_platform_data.register_flag)) {
		TS_LOG_ERR("%s not registered, return!!\n", __func__);
		return -EINVAL;
	}
	if (!g_ts_kit_platform_data.chip_data) {
		TS_LOG_ERR("%s  chip data is NULL\n", __func__);
		return -EBUSY;
	}
	if (g_ts_kit_platform_data.chip_data->project_id[0] == 0) {
		TS_LOG_ERR("%s project_id not initialed, return!!\n", __func__);
		return -EIO;
	}

	memcpy(project_id, g_ts_kit_platform_data.chip_data->project_id,
	       PROJECT_ID_LEN);

	TS_LOG_INFO("%s, project id: %s.\n", __func__,
		    g_ts_kit_platform_data.chip_data->project_id);
	return 0;
}

int tp_project_id_provider(char *project_id, uint8_t len)
{
	if (NULL == project_id) {
		TS_LOG_ERR("%s null pointer error!!\n", __func__);
		return -EINVAL;
	}

	if (len < PROJECT_ID_LEN) {
		TS_LOG_ERR("%s len is too small!!\n", __func__);
		return -EINVAL;
	}

	if (g_ts_kit_platform_data.node) {
		TS_LOG_INFO("%s is tskit project\n", __func__);
		return tskit_get_project_id(project_id);
	} else {
#ifdef CONFIG_HUAWEI_THP
		return thp_project_id_provider(project_id);
#else
		return -EIO;
#endif
	}
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

#ifdef CONFIG_HUAWEI_THP
extern const char *thp_get_vendor_name(void);
#endif
const char* ts_kit_get_vendor_name(void)
{
	return g_ts_kit_platform_data.chip_data ?
		g_ts_kit_platform_data.chip_data->vendor_name :
#ifdef CONFIG_HUAWEI_THP
		thp_get_vendor_name();
#else
		"null";
#endif
}
EXPORT_SYMBOL(ts_kit_get_vendor_name);

static void tp_init_work_fn(struct work_struct *work);
int ts_kit_proc_command_directly(struct ts_cmd_node *cmd);
static int ts_get_brightness_info_cmd(void);


/* The following is a stub function. For hisilicon platform, it will be redefined in sensorhub module.
For qualcomm platform, it has not been implemented. Thus the stub function can avoid compilation errors.*/
__attribute__ ((weak))
int tpmodule_notifier_call_chain(unsigned long val, void *v)
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

static DECLARE_WORK(ts_panel_id_work, ts_panel_id_work_fn);

static void ts_touch_switch_cmd(void)
{
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	TS_LOG_DEBUG("+\n");
	if (dev && dev->ops && dev->ops->chip_touch_switch) {
		TS_LOG_INFO("chip set touch switch\n");
		dev->ops->chip_touch_switch();
	}
	TS_LOG_DEBUG("-\n");
	return;
}

static void ts_set_sensibility(struct ts_cmd_node* in_cmd, struct ts_cmd_node* out_cmd)
{
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;

	if (dev && dev->ops && dev->ops->chip_set_sensibility_cfg) {
		dev->ops->chip_set_sensibility_cfg(in_cmd->cmd_param.pub_params.sensibility_cfg);
	}

	return ;
}

static int seq_print_freq(struct seq_file *m, char *buf, int tx_num, int rx_num)
{
	char ii, jj;
	unsigned char *head;
	unsigned char *report_data_8;

	head = (unsigned char *)buf;

	seq_printf(m, "Calibration Image - Coarse & Fine\n");
	report_data_8 = (unsigned char *)buf;
	report_data_8++;	//point to second byte of F54 report data
	for (ii = 0; ii < rx_num; ii++) {
		for (jj = 0; jj < tx_num; jj++) {
			seq_printf(m, "%02x ", *report_data_8);

			report_data_8 += 2;

		}
		seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Image - Detail\n");
	report_data_8 = head;
	for (ii = 0; ii < rx_num; ii++) {
		for (jj = 0; jj < tx_num; jj++) {
			seq_printf(m, "%02x ", *report_data_8);

			report_data_8 += 2;

		}
		seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Noise - Coarse & Fine\n");
	report_data_8 = (unsigned char *)buf;	//point to first byte of data
	report_data_8 += (tx_num * rx_num * 2 + 1);
	for (ii = 0; ii < rx_num * 2; ii++) {
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 += 2;

		if ((ii + 1) % tx_num == 0)
			seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Noise - Detail\n");
	report_data_8 = (unsigned char *)buf;
	report_data_8 += (tx_num * rx_num * 2);
	for (ii = 0; ii < rx_num * 2; ii++) {
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 += 2;

		if ((ii + 1) % tx_num == 0)
			seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration button - Coarse & Fine\n");
	report_data_8 = (unsigned char *)buf;
	report_data_8 += (tx_num * rx_num * 2 + rx_num * 4 + 1);
	for (ii = 0; ii < 4; ii++) {
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 += 2;
	}

	seq_printf(m, "\nCalibration button - Detail\n");
	report_data_8 = (unsigned char *)buf;
	report_data_8 += (tx_num * rx_num * 2 + rx_num * 4);
	for (ii = 0; ii < 4; ii++) {
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 += 2;
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

	info = (struct ts_calibration_data_info *)kzalloc(sizeof(struct ts_calibration_data_info),
							  GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out_free_cmd;
	}

	cmd->command = TS_READ_CALIBRATION_DATA;
	cmd->cmd_param.prv_params = (void *)info;

	if (g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) {
		error = ts_kit_proc_command_directly(cmd);
	} else {
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
	seq_print_freq(m, info->data + info->tx_num * info->rx_num * 2	/* shift 2D */
		       + info->rx_num * 2 * 2	/* shift noise */
		       + 4 * 2,	/* shift 0D */
		       info->tx_num, info->rx_num);

	seq_printf(m, "\n\nCollect data for freq: 2\n\n");
	seq_print_freq(m, info->data + (info->tx_num * info->rx_num * 2	/* shift 2D */
					+ info->rx_num * 2 * 2	/* shift noise */
					+ 4 * 2) * 2,	/* shift 0D */
		       info->tx_num, info->rx_num);

	seq_printf(m, "\n\nCollect data for interval scan\n\n");
	seq_print_freq(m, info->data + 2048, info->tx_num, info->rx_num);

	kfree(info);
 out_free_cmd:
	kfree(cmd);
 out:
	return 0;
}

void ts_kit_rotate_rawdata_abcd2cbad(int row, int column, int *data_start,
				     int rotate_type)
{
	int *rawdatabuf_temp = NULL;
	int row_index, column_index;
	int row_size = 0;
	int column_size = 0;
	int i = 0;

	TS_LOG_INFO("\n");
	rawdatabuf_temp = (int *)kzalloc(row * column * sizeof(int), GFP_KERNEL);
	if (!rawdatabuf_temp) {
		TS_LOG_ERR("Failed to alloc buffer for rawdatabuf_temp\n");
		return;
	}

	memcpy(rawdatabuf_temp, data_start, row * column * sizeof(int));
	switch (rotate_type) {
	case TS_RAWDATA_TRANS_NONE:
		break;
	case TS_RAWDATA_TRANS_ABCD2CBAD:
		/* src column to dst row */
		row_size = column;
		column_size = row;
		for (column_index = column_size - 1; column_index >= 0; column_index--) {
			for (row_index = row_size - 1; row_index >= 0; row_index--) {
				data_start[i++] = rawdatabuf_temp[row_index * column_size + column_index];
			}
		}
		break;
	case TS_RAWDATA_TRANS_ABCD2ADCB:
		/* src column to dst row */
		row_size = column;
		column_size = row;
		for (column_index = 0; column_index < column_size; column_index++) {
			for (row_index = 0; row_index < row_size; row_index++) {
				data_start[i++] = rawdatabuf_temp[row_index * column_size + column_index];
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

static void rawdata_proc_printf(struct seq_file *m,
				struct ts_rawdata_info *info, int range_size,
				int row_size)
{
	int index = 0;
	int index1 = 0;
	int index2 = 0;
	int rx_num = info->hybrid_buff[0];
	int tx_num = info->hybrid_buff[1];

	if ((0 == range_size) || (0 == row_size)) {
		TS_LOG_ERR("%s  range_size OR row_size is 0\n", __func__);
		return;
	}
	if (rx_num <= 0 || rx_num > RAWDATA_NUM_OF_TRX_MAX || tx_num <= 0
	    || tx_num > RAWDATA_NUM_OF_TRX_MAX) {
		TS_LOG_ERR("%s  rx_num or tx_num is wrong value\n", __func__);
		return;
	}
	for (index = 0; row_size * index + 2 < info->used_size; index++) {
		if (0 == index) {
			seq_printf(m, "rawdata begin\n");	/*print the title */
		}
		for (index1 = 0; index1 < row_size; index1++) {
			seq_printf(m, "%d,", info->buff[2 + row_size * index + index1]);	/*print oneline */
		}
		/*index1 = 0; */
		seq_printf(m, "\n ");

		if ((range_size - 1) == index) {
			seq_printf(m, "rawdata end\n");
			seq_printf(m, "noisedata begin\n");
		}
	}
	seq_printf(m, "noisedata end\n");

	index1 = tx_num + rx_num + 2;

	/*********hybrid_buff   |rx_num|tx_num|noize_rx|noize_tx|raw_rx|raw_tx|****/
	if ((index1 <= TS_RAWDATA_RESULT_MAX / 2)
	    && g_ts_kit_platform_data.chip_data->self_cap_test) {
		seq_printf(m, "selfcap rawdata begin\n");	/*print the title */
		seq_printf(m, "rx:\n");
		if (g_ts_kit_platform_data.chip_data->
		    support_forcekey_cap_value_test) {
			for (index = CAP_TEST_FORCEKEY_VALUE_NUM;
			     index < rx_num + CAP_TEST_FORCEKEY_VALUE_NUM;
			     index++) {
				seq_printf(m, "%d,", info->hybrid_buff[index1 + index]);	/*print  rx oneline */
			}
		} else {
			for (index = 0; index < rx_num; index++) {
				seq_printf(m, "%d,", info->hybrid_buff[index1 + index]);	/*print  rx oneline */
			}
		}
		seq_printf(m, "\ntx:\n");
		if (g_ts_kit_platform_data.chip_data->
		    support_forcekey_cap_value_test) {
			for (index = CAP_TEST_FORCEKEY_VALUE_NUM;
			     index < tx_num + CAP_TEST_FORCEKEY_VALUE_NUM;
			     index++) {
				seq_printf(m, "%d,", info->hybrid_buff[index1 + index + rx_num]);	/*print tx oneline */
			}
		} else {
			for (index = 0; index < tx_num; index++) {
				seq_printf(m, "%d,", info->hybrid_buff[index1 + index + rx_num]);	/*print tx oneline */
			}
		}
		seq_printf(m, "\nselfcap rawdata end\n");

		seq_printf(m, "selfcap noisedata begin\n");
		seq_printf(m, "rx:\n");
		for (index = 0; index < rx_num; index++) {
			seq_printf(m, "%d,", info->hybrid_buff[2 + index]);	/*print oneline */
		}
		seq_printf(m, "\ntx:\n");
		for (index = 0; index < tx_num; index++) {
			seq_printf(m, "%d,", info->hybrid_buff[2 + index + rx_num]);	/*print oneline */
		}
		seq_printf(m, "\nselfcap noisedata end\n");
	}

	if (g_ts_kit_platform_data.chip_data->forcekey_test_support ||
	    g_ts_kit_platform_data.chip_data->support_forcekey_cap_value_test) {
		if (info->hybrid_buff_used_size > (index1 + tx_num + rx_num)) {
			index2 =
			    info->hybrid_buff_used_size - (index1 + tx_num +
							   rx_num);
			seq_printf(m, "\nforcekey value : ");
			if (g_ts_kit_platform_data.chip_data->
			    support_forcekey_cap_value_test)
				index1 = index1 + CAP_TEST_FORCEKEY_VALUE_NUM;
			for (index = 0; index < index2; index++) {
				seq_printf(m, "%d, ",
					   info->hybrid_buff[index + index1 +
							     tx_num + rx_num]);
			}
			seq_printf(m, "\n");
		}
	}

	if (info->used_sharp_selcap_single_ended_delta_size) {
		seq_printf(m, "selfcap touchdelta begin\n");
		for (index = 0;
		     index < info->used_sharp_selcap_touch_delta_size;
		     index++) {
			seq_printf(m, "%d,",
				   info->buff[info->used_size + index]);
		}
		seq_printf(m, "\n ");
		seq_printf(m, "selfcap touchdelta end\n");
		seq_printf(m, "selfcap singleenddelta begin\n");
		for (index = 0; index < info->used_sharp_selcap_single_ended_delta_size; index++) {
			seq_printf(m, "%d,",
				   info->buff[info->used_size + info->used_sharp_selcap_touch_delta_size + index]);
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

static int rawdata_proc_parade_printf(struct seq_file *m,
				      struct ts_rawdata_info *info,
				      int range_size, int row_size)
{
	int rdIndex = RAWDATA_SIZE_LIMIT;
	int index = 0;
	int index1 = 0;

	seq_printf(m, "cm data begin\n");	/*print the title */
	for (index = 0; index < range_size; index++) {
		for (index1 = 0; index1 < row_size; index1++) {
			if (rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else {
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
	}
	seq_printf(m, "cm data end\n");	/*print the title */
	seq_printf(m, "mutual noise data begin\n");	/*print the title */
	for (index = 0; index < range_size; index++) {
		for (index1 = 0; index1 < row_size; index1++) {
			if (rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else {
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
	}
	seq_printf(m, "mutual noise data end\n");	/*print the title */
	seq_printf(m, "self noise data begin\n");	/*print the title */
	seq_printf(m, "-rx:,");
	for (index1 = 0; index1 < row_size; index1++) {
		if (rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else {
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "-tx:,");
	for (index1 = 0; index1 < range_size; index1++) {
		if (rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else {
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "self noise data end\n");	/*print the title */
	seq_printf(m, "cm gradient(10x real value) begin\n");	/*print the title */
	seq_printf(m, "-rx:,");
	for (index1 = 0; index1 < row_size; index1++) {
		if (rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else {
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "-tx:,");
	for (index1 = 0; index1 < range_size; index1++) {
		if (rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else {
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "cm gradient end\n");	/*print the title */
	seq_printf(m, "cp begin\n");	/*print the title */
	seq_printf(m, "-rx:,");
	for (index1 = 0; index1 < row_size; index1++) {
		if (rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else {
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "-tx:,");
	for (index1 = 0; index1 < range_size; index1++) {
		if (rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else {
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "cp end\n");	/*print the title */
	seq_printf(m, "cp delta begin\n");	/*print the title */
	seq_printf(m, "-rx:,");
	for (index1 = 0; index1 < row_size; index1++) {
		if (rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else {
			seq_printf(m, "\n ");
			goto out;
		}
	}
	seq_printf(m, "\n ");
	seq_printf(m, "-tx:,");
	for (index1 = 0; index1 < range_size; index1++) {
		if (rdIndex < info->used_size)
			seq_printf(m, "%d,", info->buff[rdIndex++]);
		else {
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

static void rawdata_proc_3d_func_printf(struct seq_file *m,
					struct ts_rawdata_info *info)
{
	int index = 0;
	int index1 = 0;
	int row_size = 0;
	int range_size = 0;

	TS_LOG_INFO("print 3d data\n");
	row_size = info->buff_3d[0];
	range_size = info->buff_3d[1];

	if (0 == row_size) {
		TS_LOG_ERR("%s, row_size = %d\n", __func__, row_size);
		return;
	}

	seq_printf(m, "rx: %d, tx : %d(3d)\n", row_size, range_size);

	for (index = 0; row_size * index + 2 < info->used_size_3d; index++) {
		if (0 == index) {
			seq_printf(m, "rawdata begin(3d)\n");	/*print the title */
		}
		for (index1 = 0; index1 < row_size; index1++) {
			seq_printf(m, "%d,", info->buff_3d[2 + row_size * index + index1]);	/*print oneline */
		}
		//index1 = 0;
		seq_printf(m, "\n ");

		if ((range_size - 1) == index) {
			seq_printf(m, "rawdata end(3d)\n");
			seq_printf(m, "noisedata begin(3d)\n");
		}
	}
	seq_printf(m, "noisedata end(3d)\n");
	return;
}

static void rawdata_proc_newformat_printf(struct seq_file *m,
					  struct ts_rawdata_info_new *info)
{
	struct ts_rawdata_newnodeinfo *rawdatanode = NULL;
	int index = 0;
	int index1 = 0;
	int tx_n = 0, rx_n = 0;
	int row_size = info->tx;
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
				if(rawdatanode->typeindex < RAW_DATA_TYPE_forcekey){
					resulttemp[0] = rawdatanode->typeindex + '0';
					resulttemp[1] = pfstatus[rawdatanode->typeindex]; //default result_code is failed
					resulttemp[2] = '\0';
					seq_printf(m, "%s",resulttemp);
					seq_printf(m, "%s","-");
					pfstatus[rawdatanode->typeindex] = 0;
				} else {
					resulttemp[0] = (rawdatanode->typeindex % RAWDATA_TESTTMP_NUM) + 1 + '0';
					resulttemp[1] = (rawdatanode->typeindex % RAWDATA_TESTTMP_NUM) + '0';
					resulttemp[2] = pfstatus[rawdatanode->typeindex]; //default result_code is failed
					resulttemp[3] = '\0';
					seq_printf(m, "%s",resulttemp);
					seq_printf(m, "%s","-");
					pfstatus[rawdatanode->typeindex] = 0;
				}
			}
		}
	}
	/* statistics_data info */
	list_for_each_entry(rawdatanode, &info->rawdata_head, node) {
		if (strlen(rawdatanode->statistics_data) > 0) {
			seq_printf(m, "%s", rawdatanode->statistics_data);
		}
	}

	/* result info */
	if (strlen(info->i2cerrinfo) > 0) {
		resulttemp[0] = RAW_DATA_TYPE_IC + '0';
		resulttemp[1] = '-';
		resulttemp[2] = '\0';
		seq_printf(m, "%s", resulttemp);
		seq_printf(m, "%s", info->i2cerrinfo);
		seq_printf(m, "%s", "-");
	}
	list_for_each_entry(rawdatanode, &info->rawdata_head, node) {
		if (strlen(rawdatanode->tptestfailedreason) > 0) {
			resulttemp[0] = rawdatanode->typeindex + '0';
			resulttemp[1] = '-';
			resulttemp[2] = '\0';
			seq_printf(m, "%s", resulttemp);
			seq_printf(m, "%s", rawdatanode->tptestfailedreason);
			seq_printf(m, "%s", "-");
		}
	}

	/* dev info */
	seq_printf(m, "%s", info->deviceinfo);
	seq_printf(m, "\n");

	seq_printf(m, "*************touch data*************\n");
	seq_printf(m, "tx: %d, rx : %d\n", info->tx, info->rx);
    list_for_each_entry(rawdatanode, &info->rawdata_head, node){
		if(rawdatanode->size > 0)
	seq_printf(m, "%s begin\n",rawdatanode->test_name);
		if(rawdatanode->typeindex == RAW_DATA_TYPE_TrxDelta && rawdatanode->size > 0){
			if (g_ts_kit_platform_data.chip_data->print_all_trx_diffdata_for_newformat_flag) {
				seq_puts(m, "RX:\n");
				for (index = 0; index < rawtest_size; index++) {
					seq_printf(m, "%d,", rawdatanode->values[index]);
					tx_n++;
					if (tx_n == info->tx) {
						seq_puts(m, "\n");
						tx_n = 0;
					}
				}
				seq_puts(m, "\nTX:\n");
				for (index = 0; index < rawtest_size; index++) {
					seq_printf(m, "%d,", rawdatanode->values[rawtest_size + index]);
					rx_n++;
					if (rx_n == info->tx) {
						seq_puts(m, "\n");
						rx_n = 0;
					}
				}
				seq_puts(m, "\n");
			} else {
				seq_printf(m, "RX:\n");
				for(index = 0; index < (rawtest_size - info->tx); index++) {
					seq_printf(m, "%d,", rawdatanode->values[index]);
					tx_n++;
					if(tx_n == info->tx){
						seq_printf(m, "\n");
						tx_n = 0;
					}
				}
				seq_printf(m, "\nTX:\n");
				for(index = 0; index < (rawtest_size - info->rx); index++) {
					seq_printf(m, "%d,", rawdatanode->values[rawtest_size + index]);
					rx_n++;
					if(rx_n == info->tx-1){
						seq_printf(m, "\n");
						rx_n = 0;
					}
				}
				seq_printf(m, "\n");
			}
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
		if (rawdatanode->size > 0)
			seq_printf(m, "%s end\n", rawdatanode->test_name);
	}

	return;
}

void rawdata_proc_freehook(void *infotemp)
{
	struct ts_rawdata_info_new *info = infotemp;
	struct list_head *pos, *n;
	struct ts_rawdata_newnodeinfo *rawdatanode = NULL;

	if (info) {
		list_for_each_safe(pos, n, &info->rawdata_head) {
			rawdatanode = list_entry(pos, struct ts_rawdata_newnodeinfo, node);
			if (rawdatanode->values) {
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
	if (m->size <= RAW_DATA_SIZE) {
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

	if (g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) {
		error = ts_kit_proc_command_directly(cmd);
	} else {
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

	if (info->status != TS_ACTION_SUCCESS || error) {
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
		info->rx_delta_buf =  (int *)kzalloc(tx_rx_delta_size, GFP_KERNEL);
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
		info->td43xx_rt95_part_two =
		    (signed int *)kzalloc(tx_rx_delta_size, GFP_KERNEL);
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

	if (g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) {
		error = ts_kit_proc_command_directly(cmd);
	} else {
		if (g_ts_kit_platform_data.chip_data->rawdata_get_timeout)
			error = ts_kit_put_one_cmd(cmd,
						   g_ts_kit_platform_data.
						   chip_data->rawdata_get_timeout);
		else
			error = ts_kit_put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		goto free_cmd;
	}

	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("read action failed\n");
		error = -EIO;
		goto out;
	}
	seq_printf(m, "%s\n", info->result);
	seq_printf(m, "*************touch data*************\n");

	row_size = info->buff[0];
	range_size = info->buff[1];

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

	if (g_ts_kit_platform_data.chip_data->support_3d_func) {
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
	.open = calibration_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int rawdata_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rawdata_proc_show, NULL);
}

static const struct file_operations rawdata_proc_fops = {
	.open = rawdata_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*external function declare*/
extern int i2c_check_addr_busy(struct i2c_adapter *adapter, int addr);

#if defined (CONFIG_TEE_TUI)
extern int i2c_init_secos(struct i2c_adapter *adap);
extern int i2c_exit_secos(struct i2c_adapter *adap);
extern int spi_exit_secos(unsigned int spi_bus_id);
extern int spi_init_secos(unsigned int spi_bus_id);
#endif

void ts_stop_wd_timer(struct ts_kit_platform_data *cd);
void ts_kit_thread_stop_notify(void);

int ts_i2c_write(u8 * buf, u16 length);
int ts_spi_write(u8 * buf, u16 length);
int ts_i2c_read(u8 * reg_addr, u16 reg_len, u8 * buf, u16 len);
int ts_spi_read(u8 * reg_addr, u16 reg_len, u8 * buf, u16 len);

static struct ts_bus_info ts_bus_i2c_info = {
	.btype = TS_BUS_I2C,
	.bus_write = ts_i2c_write,
	.bus_read = ts_i2c_read,
};

static struct ts_bus_info ts_bus_spi_info = {
	.btype = TS_BUS_SPI,
	.bus_write = ts_spi_write,
	.bus_read = ts_spi_read,
};

#if defined (CONFIG_HUAWEI_DSM)
void ts_i2c_error_dmd_report(u8 * reg_addr)
{
	if (atomic_read(&g_ts_kit_platform_data.power_state) == TS_UNINIT) {
		TS_LOG_INFO("ts_dirver_don't_init,no need report dmd\n");
		return;
	}
	if ((atomic_read(&g_ts_kit_platform_data.power_state) == TS_SLEEP)
	    || (atomic_read(&g_ts_kit_platform_data.power_state) ==
		TS_WORK_IN_SLEEP)
	    || (atomic_read(&g_ts_kit_platform_data.power_state) ==
		TS_GOTO_SLEEP)) {
		if (reg_addr == NULL) {
			ts_dmd_report(DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO,
				      "irq_gpio:%d;value:%d;reset_gpio:%d;value:%d;I2C_status:%d.\n",
				      g_ts_kit_platform_data.irq_gpio,
				      gpio_get_value
				      (g_ts_kit_platform_data.irq_gpio),
				      g_ts_kit_platform_data.reset_gpio,
				      gpio_get_value
				      (g_ts_kit_platform_data.reset_gpio),
				      g_ts_kit_platform_data.dsm_info.
				      constraints_I2C_status);
		} else {
			ts_dmd_report(DSM_TP_ABNORMAL_DONT_AFFECT_USE_NO,
				      "irq_gpio:%d;value:%d;reset_gpio:%d;value:%d;I2C_status:%d;addr:%d.\n",
				      g_ts_kit_platform_data.irq_gpio,
				      gpio_get_value
				      (g_ts_kit_platform_data.irq_gpio),
				      g_ts_kit_platform_data.reset_gpio,
				      gpio_get_value
				      (g_ts_kit_platform_data.reset_gpio),
				      g_ts_kit_platform_data.dsm_info.
				      constraints_I2C_status, *reg_addr);
		}
	} else {
		if (reg_addr == NULL) {
			ts_dmd_report(DSM_TP_I2C_RW_ERROR_NO,
				      "irq_gpio:%d;value:%d;reset_gpio:%d;value:%d;I2C_status:%d.\n",
				      g_ts_kit_platform_data.irq_gpio,
				      gpio_get_value
				      (g_ts_kit_platform_data.irq_gpio),
				      g_ts_kit_platform_data.reset_gpio,
				      gpio_get_value
				      (g_ts_kit_platform_data.reset_gpio),
				      g_ts_kit_platform_data.dsm_info.
				      constraints_I2C_status);
		} else {
			ts_dmd_report(DSM_TP_I2C_RW_ERROR_NO,
				      "irq_gpio:%d value:%d reset_gpio:%d  value:%d. I2C_status:%d;addr:%d.\n",
				      g_ts_kit_platform_data.irq_gpio,
				      gpio_get_value
				      (g_ts_kit_platform_data.irq_gpio),
				      g_ts_kit_platform_data.reset_gpio,
				      gpio_get_value
				      (g_ts_kit_platform_data.reset_gpio),
				      g_ts_kit_platform_data.dsm_info.
				      constraints_I2C_status, *reg_addr);
		}
	}
}
#endif

#define GET_HWLOCK_FAIL   0

int tp_i2c_get_hwlock(void)
{
	int ret = 0;
	unsigned long time = 0;
	unsigned long timeout = 0;
	struct hwspinlock *hwlock = NULL;

	hwlock = g_ts_kit_platform_data.i2c_hwlock.hwspin_lock;
	timeout = jiffies + msecs_to_jiffies(GET_HARDWARE_TIMEOUT);

	do {
		ret = hwlock->bank->ops->trylock(hwlock);
		if (GET_HWLOCK_FAIL == ret) {
			time = jiffies;
			if (time_after(time, timeout)) {
				TS_LOG_ERR(" i2c get hardware_mutex for completion timeout \n");
				return -ETIME;
			}
		}
	} while (GET_HWLOCK_FAIL == ret);

	return 0;
}

void tp_i2c_release_hwlock(void)
{
	struct hwspinlock *hwlock = NULL;
	hwlock = g_ts_kit_platform_data.i2c_hwlock.hwspin_lock;
	hwlock->bank->ops->unlock(hwlock);
	return;
}

int ts_i2c_write(u8 * buf, u16 length)
{
	int count = 0;
	int ret;

	trace_touch(TOUCH_TRACE_I2C, TOUCH_TRACE_FUNC_IN, "write");
#if defined (CONFIG_TEE_TUI)
	if (g_ts_kit_platform_data.chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif
	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		ret = tp_i2c_get_hwlock();
		if (ret) {
			TS_LOG_ERR("i2c get hardware mutex failure\n");
			return -EAGAIN;
		}
	}
	do {
		ret =
		    i2c_master_send(g_ts_kit_platform_data.client, (const char *)buf, length);
		if (ret == length) {
			if (g_ts_kit_platform_data.i2c_hwlock. tp_i2c_hwlock_flag) {
				tp_i2c_release_hwlock();
			}
			trace_touch(TOUCH_TRACE_I2C, TOUCH_TRACE_FUNC_OUT, "write");
			return NO_ERR;
		}
#if defined (CONFIG_HUAWEI_DSM)
		else
			g_ts_kit_platform_data.dsm_info.constraints_I2C_status = ret;
#endif
		msleep(I2C_WAIT_TIME);
	}
	while (++count < I2C_RW_TRIES);

	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		tp_i2c_release_hwlock();
	}
#if defined (CONFIG_HUAWEI_DSM)
	ts_i2c_error_dmd_report(&buf[0]);
#endif

	TS_LOG_ERR("ts_i2c_write failed\n");
	trace_touch(TOUCH_TRACE_I2C, TOUCH_TRACE_FUNC_OUT, "write");
	return -EIO;
}

int ts_change_spi_mode(struct spi_device *spi, u16 mode)
{
	int ret = 0;

	if (spi->mode != mode) {
		spi->mode = mode;
		ret = spi_setup(spi);
		if (ret) {
			TS_LOG_ERR("%s setup spi failed.\n");
			return ret;
		}
	}

	return 0;
}

int ts_spi_write(u8 * buf, u16 length)
{
	return NO_ERR;
}

int ts_i2c_read(u8 * reg_addr, u16 reg_len, u8 * buf, u16 len)
{
	int count = 0;
	int ret = 0;
	int msg_len = 0;
	struct i2c_msg *msg_addr = NULL;
	struct i2c_msg xfer[2];

	trace_touch(TOUCH_TRACE_I2C, TOUCH_TRACE_FUNC_IN, "read");

#if defined (CONFIG_TEE_TUI)
	if (g_ts_kit_platform_data.chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif
	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		ret = tp_i2c_get_hwlock();
		if (ret) {
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
				if (g_ts_kit_platform_data.i2c_hwlock. tp_i2c_hwlock_flag) {
					tp_i2c_release_hwlock();
				}
				trace_touch(TOUCH_TRACE_I2C, TOUCH_TRACE_FUNC_OUT, "read");
				return NO_ERR;
			}
#if defined (CONFIG_HUAWEI_DSM)
			else
				g_ts_kit_platform_data.dsm_info.constraints_I2C_status = ret;
#endif

			msleep(I2C_WAIT_TIME);
		} while (++count < I2C_RW_TRIES);
	} else {
		/*register addr */
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
		do {
			ret = i2c_transfer(g_ts_kit_platform_data.client->adapter,
					 msg_addr, msg_len);
			if (ret == msg_len) {
				if (g_ts_kit_platform_data.i2c_hwlock. tp_i2c_hwlock_flag) {
					tp_i2c_release_hwlock();
				}
			trace_touch(TOUCH_TRACE_I2C, TOUCH_TRACE_FUNC_OUT, "read");
				return NO_ERR;
			}
#if defined (CONFIG_HUAWEI_DSM)
	else
		g_ts_kit_platform_data.dsm_info.constraints_I2C_status = ret;
#endif
			msleep(I2C_WAIT_TIME);
		} while (++count < I2C_RW_TRIES);
	}
	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		tp_i2c_release_hwlock();
	}
#if defined (CONFIG_HUAWEI_DSM)
	ts_i2c_error_dmd_report(reg_addr);
#endif
	TS_LOG_ERR("ts_i2c_read failed\n");
	trace_touch(TOUCH_TRACE_I2C, TOUCH_TRACE_FUNC_OUT, "read");
	return -EIO;
}

int ts_spi_read(u8 * reg_addr, u16 reg_len, u8 * buf, u16 len)
{
	return NO_ERR;
}

static irqreturn_t ts_irq_handler(int irq, void *dev_id)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;

	wake_lock_timeout(&g_ts_kit_platform_data.ts_wake_lock, HZ);

	trace_touch(TOUCH_TRACE_IRQ_TOP, TOUCH_TRACE_FUNC_IN, NULL);

	if (g_ts_kit_platform_data.chip_data->ops->chip_irq_top_half) {
		error =
		    g_ts_kit_platform_data.chip_data->ops->
		    chip_irq_top_half(&cmd);
	}

	if (error)		//unexpected error happen, put err cmd to ts thread
	{
		cmd.command = TS_INT_ERR_OCCUR;
	} else {
		cmd.command = TS_INT_PROCESS;
	}

	disable_irq_nosync(g_ts_kit_platform_data.irq_id);

	if (ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT)
	    && (TS_UNINIT != atomic_read(&g_ts_kit_platform_data.state))) {
		enable_irq(g_ts_kit_platform_data.irq_id);
	}

	trace_touch(TOUCH_TRACE_IRQ_TOP, TOUCH_TRACE_FUNC_OUT, NULL);
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
			ts_kit_power_control_notify(TS_BEFORE_SUSPEND,
						SHORT_SYNC_TIMEOUT);
		}

		if (g_ts_kit_platform_data.chip_data->tui_set_flag & 0x2) {
			TS_LOG_INFO("TUI exit, do suspend\n");
			ts_kit_power_control_notify(TS_SUSPEND_DEVICE,
						NO_SYNC_TIMEOUT);
		}

		g_ts_kit_platform_data.chip_data->tui_set_flag = 0;
		TS_LOG_INFO("ts_kit_tui_secos_exit: report_tui_enable is %d\n",
			    g_ts_kit_platform_data.chip_data->report_tui_enable);
	}
}

static int tui_tp_init(void *data, int secure)
{
	if (secure) {
		ts_kit_tui_secos_init();
	} else
		ts_kit_tui_secos_exit();
	return 0;
}

#endif
static int try_update_firmware(void)
{
	char joint_chr = '_';
	int error = NO_ERR;
	char *fw_name;
	struct ts_cmd_node cmd;
	memset(&cmd, 0, sizeof(struct ts_cmd_node));
	cmd.command = TS_FW_UPDATE_BOOT;
	fw_name = cmd.cmd_param.pub_params.firmware_info.fw_name;
	/*firmware name [product_name][ic_name][module][vendor] */
	strncat(fw_name, g_ts_kit_platform_data.product_name, MAX_STR_LEN);
	strncat(fw_name, &joint_chr, 1);
	strncat(fw_name, g_ts_kit_platform_data.chip_data->chip_name,
		MAX_STR_LEN);
	strncat(fw_name, &joint_chr, 1);
	error = ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
	return error;
}

static void ts_watchdog_work(struct work_struct *work)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;
	TS_LOG_DEBUG("ts_watchdog_work\n");
	cmd.command = TS_CHECK_STATUS;
	if (g_ts_kit_platform_data.chip_data->is_parade_solution) {
		error = ts_kit_proc_command_directly(&cmd);
	} else {
		error = ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put TS_CHECK_STATUS cmd error :%d\n", error);
	}
	return;
}

static void ts_watchdog_timer(unsigned long data)
{
	struct ts_kit_platform_data *cd = (struct ts_kit_platform_data *)data;
	TS_LOG_DEBUG("Timer triggered\n");
	if (!cd) {
		return;
	}

	if (!work_pending(&cd->watchdog_work)) {
		schedule_work(&cd->watchdog_work);
	}

	return;
}

void ts_kit_thread_stop_notify(void)
{
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	TS_LOG_INFO("ts thread stop called by lcd only shutdown\n");
	if (TS_UNINIT == atomic_read(&g_ts_kit_platform_data.state)) {
		TS_LOG_INFO("ts is not init");
		return;
	}
	if (TS_UNREGISTER == atomic_read(&g_ts_kit_platform_data.register_flag)) {
		TS_LOG_ERR("ts is not register\n");
		return;
	}

	atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
	atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);
	disable_irq(g_ts_kit_platform_data.irq_id);
	ts_stop_wd_timer(&g_ts_kit_platform_data);
	if (dev && dev->ops && dev->ops->chip_shutdown)
		dev->ops->chip_shutdown();
	/*there is something wrong about system, now abandon the kthread_stop to avoid unkown bug */
	//kthread_stop(g_ts_kit_platform_data.ts_task);
}

#if defined (CONFIG_HISI_BCI_BATTERY)
static int ts_charger_detect_cmd(enum ts_charger_state charger_state)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_charger_info *info = NULL;
	TS_LOG_INFO ("%s called, charger type: %d, [0 out, 1 in], supported: %d\n",
	     __func__, charger_state, g_ts_kit_platform_data.feature_info.charger_info.charger_supported);

	if (g_ts_kit_platform_data.feature_info.charger_info.charger_supported == 0) {
		TS_LOG_DEBUG("%s, do nothing cause charger_supported is zero\n", __func__);
		goto out;
	}

	info = &g_ts_kit_platform_data.feature_info.charger_info;
	info->op_action = TS_ACTION_WRITE;
	if (USB_PIUG_OUT == charger_state) {	/*usb plug out */
		if (info->charger_switch == 0) {
			TS_LOG_ERR ("%s, there is no need to send cmd repeated\n",__func__);
			error = -EINVAL;
			goto out;
		}
		info->charger_switch = 0;
	} else {		/*usb plug in */
		if (info->charger_switch == 1) {
			TS_LOG_ERR("%s, there is no need to send repeated\n", __func__);
			error = -EINVAL;
			goto out;
		}
		info->charger_switch = 1;
	}

	if (TS_WORK != atomic_read(&g_ts_kit_platform_data.state)) {
		TS_LOG_ERR ("%s, can not send cmd when TP is not working in normal mode\n", __func__);
		error = -EINVAL;
		goto out;
	}

	cmd = (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node), GFP_KERNEL);
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
	enum ts_charger_state charger_state = USB_PIUG_OUT;
	if (!g_ts_kit_platform_data.feature_info.charger_info.charger_supported) {
		return 0;
	}

	switch (event) {
	case VCHRG_START_USB_CHARGING_EVENT:
	case VCHRG_START_AC_CHARGING_EVENT:
	case VCHRG_START_CHARGING_EVENT:
	case VCHRG_CHARGE_DONE_EVENT:
	case VCHRG_POWER_SUPPLY_OVERVOLTAGE:
	case VCHRG_POWER_SUPPLY_WEAKSOURCE:
	case VCHRG_CURRENT_FULL_EVENT:
	case VCHRG_NOT_CHARGING_EVENT:
		charger_state = USB_PIUG_IN;
		break;
	case VCHRG_STOP_CHARGING_EVENT:
		charger_state = USB_PIUG_OUT;
		break;
	default:
		break;
	}

	if (charger_state != g_ts_kit_platform_data.feature_info.charger_info.status) {
		TS_LOG_INFO("%s, charger event:%ld, status=%d. \n", __func__, event, charger_state);
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
		TS_LOG_INFO("%s, charger notify register succ\n", __func__);
	}

}
#endif

static int parse_spi_config(void)
{
	int retval = 0;
	retval = of_property_read_u32(g_ts_kit_platform_data.node,
				 "spi-max-frequency",
				 &g_ts_kit_platform_data.spi_max_frequency);
	if (retval) {
		TS_LOG_ERR("%s: get spi_max_frequency failed\n", __func__);
		goto err_out;
	}

	retval = of_property_read_u32(g_ts_kit_platform_data.node, "spi-mode",
				 &g_ts_kit_platform_data.spi_mode);
	if (retval) {
		TS_LOG_ERR("%s: get spi mode failed\n", __func__);
		goto err_out;
	}

	retval = of_property_read_u32(g_ts_kit_platform_data.node,
				 "pl022,interface",
				 &g_ts_kit_platform_data.spidev0_chip_info.
				 iface);
	if (retval) {
		TS_LOG_ERR("%s: get iface failed\n", __func__);
		goto err_out;
	}
	retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,com-mode",
				 &g_ts_kit_platform_data.spidev0_chip_info.
				 com_mode);
	if (retval) {
		TS_LOG_ERR("%s: get com_mode failed\n", __func__);
		goto err_out;
	}
	retval = of_property_read_u32(g_ts_kit_platform_data.node,
				 "pl022,rx-level-trig",
				 &g_ts_kit_platform_data.spidev0_chip_info.
				 rx_lev_trig);
	if (retval) {
		TS_LOG_ERR("%s: get rx_lev_trig failed\n", __func__);
		goto err_out;
	}
	retval = of_property_read_u32(g_ts_kit_platform_data.node,
				 "pl022,tx-level-trig",
				 &g_ts_kit_platform_data.spidev0_chip_info.
				 tx_lev_trig);
	if (retval) {
		TS_LOG_ERR("%s: get tx_lev_trig failed\n", __func__);
		goto err_out;
	}
	retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,ctrl-len",
				 &g_ts_kit_platform_data.spidev0_chip_info.
				 ctrl_len);
	if (retval) {
		TS_LOG_ERR("%s: get ctrl_len failed\n", __func__);
		goto err_out;
	}
	retval =
	    of_property_read_u32(g_ts_kit_platform_data.node,
				 "pl022,wait-state",
				 &g_ts_kit_platform_data.spidev0_chip_info.
				 wait_state);
	if (retval) {
		TS_LOG_ERR("%s: get wait_state failed\n", __func__);
		goto err_out;
	}
	retval = of_property_read_u32(g_ts_kit_platform_data.node, "pl022,duplex",
				 &g_ts_kit_platform_data.spidev0_chip_info.
				 duplex);
	if (retval) {
		TS_LOG_ERR("%s: get duplex failed\n", __func__);
		goto err_out;
	}
	retval = of_property_read_u32(g_ts_kit_platform_data.node,
				 "cs_reset_low_delay",
				 &g_ts_kit_platform_data.cs_reset_low_delay);
	if (retval) {
		TS_LOG_ERR("%s: get duplex failed\n", __func__);
		goto err_out;
	}
	retval = of_property_read_u32(g_ts_kit_platform_data.node,
				 "cs_reset_high_delay",
				 &g_ts_kit_platform_data.cs_reset_high_delay);
	if (retval) {
		TS_LOG_ERR("%s: get duplex failed\n", __func__);
		goto err_out;
	}
	g_ts_kit_platform_data.cs_gpio =
	    of_get_named_gpio(g_ts_kit_platform_data.node, "cs_gpio", 0);
	if (!gpio_is_valid(g_ts_kit_platform_data.cs_gpio)) {
		g_ts_kit_platform_data.cs_gpio = 0;
		TS_LOG_ERR(" ts_kit cs gpio is not valid\n");
	}

	TS_LOG_INFO
	    ("%s: spi-max-frequency = %d  spi_mode = %d pl022,interface =%d pl022,com-mode = %d pl022,rx-level-trig = %d"
	     "pl022,tx-level-trig = %d pl022,ctrl-len = %d pl022,wait_state = %d pl022,duplex = %d,cs_reset_low_delay=%d cs_reset_high_delay = %d cs_gpio = %d\n",
	     __func__, g_ts_kit_platform_data.spi_max_frequency,
	     g_ts_kit_platform_data.spi_mode,
	     g_ts_kit_platform_data.spidev0_chip_info.iface,
	     g_ts_kit_platform_data.spidev0_chip_info.com_mode,
	     g_ts_kit_platform_data.spidev0_chip_info.rx_lev_trig,
	     g_ts_kit_platform_data.spidev0_chip_info.tx_lev_trig,
	     g_ts_kit_platform_data.spidev0_chip_info.ctrl_len,
	     g_ts_kit_platform_data.spidev0_chip_info.wait_state,
	     g_ts_kit_platform_data.spidev0_chip_info.duplex,
	     g_ts_kit_platform_data.cs_reset_low_delay,
	     g_ts_kit_platform_data.cs_reset_high_delay,
	     g_ts_kit_platform_data.cs_gpio);
	return 0;
 err_out:
	return retval;
}

static int get_ts_board_info(void)
{
	const char *bus_type = NULL;
	int rc = 0;
	int error = NO_ERR;
	u32 bus_id = 0;
	u32 hide_plain_id = 0;
	u32 fp_tp_enable = 0;
	u32 register_charger_notifier = 0;
	g_ts_kit_platform_data.node = NULL;
	g_ts_kit_platform_data.node =
	    of_find_compatible_node(NULL, NULL, TS_DEV_NAME);
	if (!g_ts_kit_platform_data.node) {
		TS_LOG_ERR("can't find ts module node\n");
		error = -EINVAL;
		goto out;
	}

	rc = of_property_read_string(g_ts_kit_platform_data.node, "bus_type",
				     &bus_type);
	if (rc) {
		TS_LOG_ERR("bus type read failed:%d\n", rc);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_INFO("bus type is:%s\n", bus_type);
	if (!strcmp(bus_type, "i2c")) {
		g_ts_kit_platform_data.bops = &ts_bus_i2c_info;
	} else if (!strcmp(bus_type, "spi")) {
		rc = parse_spi_config();
		if (rc) {
			TS_LOG_ERR("parse_spi_config fail\n");
			error = -EINVAL;
			goto out;
		}
		g_ts_kit_platform_data.bops = &ts_bus_spi_info;
	} else {
		TS_LOG_ERR("bus type invaild:%s\n", bus_type);
		error = -EINVAL;
		goto out;
	}

	rc = of_property_read_u32(g_ts_kit_platform_data.node, "bus_id",
				  &bus_id);
	if (rc) {
		TS_LOG_ERR("bus id read failed\n");
		error = -EINVAL;
		goto out;
	}
	g_ts_kit_platform_data.bops->bus_id = bus_id;
	TS_LOG_INFO("bus id :%d\n", bus_id);
	rc = of_property_read_u32(g_ts_kit_platform_data.node,
				  "need_i2c_hwlock",
				  &g_ts_kit_platform_data.i2c_hwlock.
				  tp_i2c_hwlock_flag);
	if (rc) {
		TS_LOG_ERR("i2c_hwlock read error\n");
	}
	if (g_ts_kit_platform_data.i2c_hwlock.tp_i2c_hwlock_flag) {
		g_ts_kit_platform_data.i2c_hwlock.hwspin_lock =
		    hwspin_lock_request_specific(TP_I2C_HWSPIN_LOCK_CODE);
		if (!g_ts_kit_platform_data.i2c_hwlock.hwspin_lock) {
			TS_LOG_INFO("get i2c hwlock failed.\n");
			error = -EINVAL;
			goto out;
		}
		TS_LOG_INFO("get i2c hwlock success.\n");
	}

	rc = of_property_read_u32(g_ts_kit_platform_data.node, "aft_enable",
				  &g_ts_kit_platform_data.aft_param.aft_enable_flag);
	if (rc) {
		TS_LOG_ERR("aft_enable read error\n");
	}
	if (g_ts_kit_platform_data.aft_param.aft_enable_flag) {
		of_property_read_u32(g_ts_kit_platform_data.node, "drv_stop_width",
				     &g_ts_kit_platform_data.aft_param.drv_stop_width);
		of_property_read_u32(g_ts_kit_platform_data.node, "lcd_width",
				     &g_ts_kit_platform_data.aft_param.lcd_width);
		of_property_read_u32(g_ts_kit_platform_data.node, "lcd_height",
				     &g_ts_kit_platform_data.aft_param.lcd_height);
		TS_LOG_INFO  ("aft enable,drv_stop_width is %d,lcd_width is %d, lcd_height is %d\n",
				g_ts_kit_platform_data.aft_param.drv_stop_width,
				g_ts_kit_platform_data.aft_param.lcd_width,
				g_ts_kit_platform_data.aft_param.lcd_height);
	} else {
		TS_LOG_INFO("aft disable\n");
	}
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	g_ts_kit_platform_data.reset_gpio =
	    of_get_named_gpio(g_ts_kit_platform_data.node, "reset_gpio", 0);
	if (!gpio_is_valid(g_ts_kit_platform_data.reset_gpio)) {
		g_ts_kit_platform_data.reset_gpio = 0;
		TS_LOG_ERR(" ts_kit reset gpio is not valid\n");
	}
	g_ts_kit_platform_data.irq_gpio =
	    of_get_named_gpio(g_ts_kit_platform_data.node, "irq_gpio", 0);
	if (!gpio_is_valid(g_ts_kit_platform_data.irq_gpio)) {
		TS_LOG_ERR(" ts_kit irq_gpio is not valid\n");
		error = -EINVAL;
		goto out;
	}
#endif
	rc = of_property_read_u32(g_ts_kit_platform_data.node, "fp_tp_enable",
				  &fp_tp_enable);
	if (rc) {
		TS_LOG_ERR(" ts_kit fp_tp_enable is not valid\n");
		g_ts_kit_platform_data.fp_tp_enable = 0;
	} else {
		g_ts_kit_platform_data.fp_tp_enable = fp_tp_enable;
	}
	rc = of_property_read_u32(g_ts_kit_platform_data.node,
				  "register_charger_notifier",
				  &register_charger_notifier);
	if (rc) {
		TS_LOG_ERR
		    (" ts_kit register_charger_notifier is not config, use default enable\n");
		g_ts_kit_platform_data.register_charger_notifier = true;
	} else {
		g_ts_kit_platform_data.register_charger_notifier =
		    register_charger_notifier;
		TS_LOG_INFO("ts_kit register_charger_notifier = %d\n",
			    register_charger_notifier);
	}

	rc = of_property_read_u32(g_ts_kit_platform_data.node, "hide_plain_id",
				  &hide_plain_id);
	if (rc) {
		g_ts_kit_platform_data.hide_plain_id = 0;
		TS_LOG_INFO("hide_plain_id not exsit\n");
	}
	g_ts_kit_platform_data.hide_plain_id = hide_plain_id;
	TS_LOG_INFO
	    ("bus id :%d ts_kit reset gpio is = %d ts_kit irq gpio is = %d hide_plain_id = %d.\n",
	     g_ts_kit_platform_data.bops->bus_id,
	     g_ts_kit_platform_data.reset_gpio, g_ts_kit_platform_data.irq_gpio,
	     g_ts_kit_platform_data.hide_plain_id);
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
	} else {
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
		if (error < 0) {
			TS_LOG_ERR("Fail request gpio:%d, ret=%d\n", g_ts_kit_platform_data.reset_gpio, error);
			return error;
		}
	}
	error = gpio_request(g_ts_kit_platform_data.irq_gpio, "ts_kit_irq_gpio");
	if (error < 0) {
		TS_LOG_ERR("Fail request gpio:%d, ret=%d\n",
			   g_ts_kit_platform_data.irq_gpio, error);
		return error;
	}
	TS_LOG_INFO("reset_gpio :%d ,irq_gpio :%d\n",
		    g_ts_kit_platform_data.reset_gpio,
		    g_ts_kit_platform_data.irq_gpio);
	return error;
}

static int ts_creat_i2c_client(void)
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_client *client = NULL;
	struct i2c_board_info board_info;
	memset(&board_info, 0, sizeof(struct i2c_board_info));
	strncpy(board_info.type, TS_DEV_NAME, I2C_NAME_SIZE - 1);
	board_info.addr = I2C_DEFAULT_ADDR;
	board_info.flags = true;
	adapter = i2c_get_adapter(g_ts_kit_platform_data.bops->bus_id);
	if (!adapter) {
		TS_LOG_ERR("i2c_get_adapter failed\n");
		return -EIO;
	}

	client = i2c_new_device(adapter, &board_info);
	if (!client) {
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
		TS_LOG_ERR("spi_busnum_to_master(%d) return NULL\n",
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
		TS_LOG_ERR("%s:gpio_request(%d) failed\n", __func__, g_ts_kit_platform_data.cs_gpio);
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
	switch (g_ts_kit_platform_data.bops->btype) {
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
	switch (g_ts_kit_platform_data.bops->btype) {
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
	char *tmp_buff = NULL;
	int tmp_buff_len = 0;
	if (g_ts_kit_platform_data.node) {
		rc = of_property_read_string(g_ts_kit_platform_data.node, "product", (const char **)&tmp_buff);
		if (rc) {
			TS_LOG_ERR("product read failed:%d\n", rc);
			error = -EINVAL;
			goto out;
		}
	}

	if (!tmp_buff) {
		TS_LOG_ERR("tmp_buff failed\n");
		error = -EINVAL;
		goto out;
	}

	memset(g_ts_kit_platform_data.product_name, 0, MAX_STR_LEN);
	tmp_buff_len = (int)strlen(tmp_buff);
	for (index = 0; index < tmp_buff_len && index < (MAX_STR_LEN - 1); index++)	//exchange name to lower
	{
		g_ts_kit_platform_data.product_name[index] = tolower(tmp_buff[index]);
	}

	TS_LOG_INFO("parse product name :%s\n", g_ts_kit_platform_data.product_name);
 out:
	return error;
}

static void procfs_create(void)
{
	if (!proc_mkdir("touchscreen", NULL)) {
		return;
	}
	proc_create("touchscreen/tp_capacitance_data",
		    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, NULL,
		    &rawdata_proc_fops);
	proc_create("touchscreen/tp_calibration_data",
		    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, NULL,
		    &calibration_proc_fops);
	return;
}

static int ts_kit_create_sysfs(void)
{
	int error = NO_ERR;
	TS_LOG_INFO("ts_kit_create_sysfs enter\n");
	error = sysfs_create_group(&g_ts_kit_platform_data.ts_dev->dev.kobj, &ts_attr_group);
	if (error) {
		TS_LOG_ERR("can't create ts's sysfs\n");
		goto err_del_platform_dev;
	}
	TS_LOG_INFO("sysfs_create_group success\n");
	procfs_create();
	TS_LOG_INFO("procfs_create success\n");
	error = sysfs_create_link(NULL, &g_ts_kit_platform_data.ts_dev->dev.kobj, "touchscreen");
	if (error) {
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
static int ts_parse_chip_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data);
static int ts_kit_chip_init(void)
{
	int error = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	TS_LOG_INFO("ts_chip_init called\n");

	if (ts_parse_chip_config(dev->cnode, dev))
		TS_LOG_ERR("%s:parse chip config failed\n", __func__);


	mutex_init(&ts_kit_easy_wake_guesure_lock);
	if (g_ts_kit_platform_data.chip_data->is_direct_proc_cmd == 0) {
		if (dev->ops->chip_init) {
			error = dev->ops->chip_init();
		}
	}
	if (error) {
		TS_LOG_ERR("chip init failed\n");
	}

#ifdef CONFIG_HUAWEI_DSM
	else {
		if (strlen(dev->chip_name) > 0 && DSM_MAX_IC_NAME_LEN > strlen(dev->chip_name) &&
				strlen(dev->module_name) > 0 && DSM_MAX_MODULE_NAME_LEN > strlen(dev->module_name)) {
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
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	TS_LOG_INFO("register algo called\n");
	dev->algo_size = 0;
	INIT_LIST_HEAD(&dev->algo_head);
	error = ts_kit_register_algo_func(dev);
	return error;
}

static int ts_input_open(struct input_dev *dev)
{
	TS_LOG_DEBUG("input_open called:%d\n", dev->users);
	return NO_ERR;
}

static void ts_input_close(struct input_dev *dev)
{
	TS_LOG_DEBUG("input_close called:%d\n", dev->users);
}

static int ts_kit_input_tp_device_register(struct input_dev *dev)
{
	int error = NO_ERR;
	struct input_dev *input_dev = NULL;
	input_dev = input_allocate_device();
	if (!input_dev) {
		TS_LOG_ERR("failed to allocate memory for input tp dev\n");
		error = -ENOMEM;
		goto err_out;
	}

	input_dev->name = TS_DEV_NAME;
	if (g_ts_kit_platform_data.bops->btype == TS_BUS_I2C) {
		input_dev->id.bustype = BUS_I2C;
	} else if (g_ts_kit_platform_data.bops->btype == TS_BUS_SPI) {
		input_dev->id.bustype = BUS_SPI;
	}
	input_dev->dev.parent = &g_ts_kit_platform_data.ts_dev->dev;
	input_dev->open = ts_input_open;
	input_dev->close = ts_input_close;
	g_ts_kit_platform_data.input_dev = input_dev;
	if (g_ts_kit_platform_data.chip_data->ops->chip_input_config)	//config input for diff chip
	{
		error =
		    g_ts_kit_platform_data.chip_data->ops->
		    chip_input_config(g_ts_kit_platform_data.input_dev);
	}
	if (error) {
		goto err_free_dev;
	}

	input_set_drvdata(input_dev, &g_ts_kit_platform_data);
	error = input_register_device(input_dev);
	if (error) {
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
	struct input_dev *pen_dev = NULL;
	pen_dev = input_allocate_device();
	if (!pen_dev) {
		TS_LOG_ERR("failed to allocate memory for input pen dev\n");
		error = -ENOMEM;
		goto err_out;
	}

	pen_dev->name = TS_PEN_DEV_NAME;
	g_ts_kit_platform_data.pen_dev = pen_dev;
	if (g_ts_kit_platform_data.chip_data
	    && g_ts_kit_platform_data.chip_data->ops
	    && g_ts_kit_platform_data.chip_data->ops->chip_input_pen_config) {
		error = g_ts_kit_platform_data.chip_data->ops->chip_input_pen_config(g_ts_kit_platform_data.pen_dev);
		if (error) {
			goto err_free_dev;
		}
	}
	input_set_drvdata(pen_dev, &g_ts_kit_platform_data);
	error = input_register_device(pen_dev);
	if (error) {
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
	if (error) {
		TS_LOG_ERR("failed to register for input tp dev\n");
		error = -ENOMEM;
		goto err_out;
	}

	if (g_ts_kit_platform_data.feature_info.pen_info.pen_supported) {
		error = ts_kit_input_pen_device_register(dev);
		if (error) {
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
	TS_LOG_DEBUG("ts_kit_pm_init success\n");
	return 0;
}

#if defined (CONFIG_HUAWEI_DEVKIT_MTK_3_0)
static void ts_gpio_irq_init_config(void)
{
	int error = NO_ERR;
	u32 ints[2] = { 0, 0 };

	error= of_property_read_u32_array(g_ts_kit_platform_data.node, "debounce", ints, ARRAY_SIZE(ints));
	if (error){
		TS_LOG_ERR("%s: debounce read failed\n", __func__);
		return;
	}

	error = gpio_set_debounce(ints[0], ints[1]);
	if(-ENOTSUPP == error){
		TS_LOG_INFO("Warnning : The control is not support debunce\n");
	}

	return;
}
#endif

static int ts_kit_irq_init(void)
{
	int error = NO_ERR;
	unsigned int irq_flags;

#if defined (CONFIG_HUAWEI_DEVKIT_MTK_3_0)
	ts_gpio_irq_init_config();
	g_ts_kit_platform_data.irq_id = irq_of_parse_and_map(g_ts_kit_platform_data.node, 0);
#else
	g_ts_kit_platform_data.irq_id = gpio_to_irq(g_ts_kit_platform_data.irq_gpio);
#endif

	switch (g_ts_kit_platform_data.chip_data->irq_config) {
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

	atomic_set(&g_ts_kit_platform_data.state, TS_WORK);	//avoid 1st irq unable to handler
	atomic_set(&g_ts_kit_platform_data.power_state, TS_WORK);
	error = request_irq(g_ts_kit_platform_data.irq_id, ts_irq_handler,
			irq_flags | IRQF_NO_SUSPEND, "ts",
			&g_ts_kit_platform_data);
	if (error) {
		TS_LOG_ERR("ts request_irq failed\n");
		goto err_out;
	}
	TS_LOG_INFO("ts_kit_irq_init success\n");
 err_out:
	return error;
}

static void ts_get_brightness_info(void)
{
	return;
}

static int ts_get_brightness_info_cmd(void)
{
	int rc = NO_ERR;
	return rc;
}

static int ts_kit_update_firmware(void)
{
	int error = NO_ERR;
	unsigned int touch_recovery_mode = 0;
	unsigned int charge_flag = 0;
/*get_boot_into_recovery_flag need to be added later*/
	touch_recovery_mode = get_into_recovery_flag_adapter();
	charge_flag = get_pd_charge_flag_adapter();
	/*do not do boot fw update on recovery mode */
	TS_LOG_INFO("touch_recovery_mode is %d, charge_flag:%u\n",
		    touch_recovery_mode, charge_flag);
	if (!touch_recovery_mode && !charge_flag) {
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
	if (g_ts_kit_platform_data.chip_data->need_wd_check_status) {
		TS_LOG_INFO("This chip need watch dog to check status\n");
		INIT_WORK(&(g_ts_kit_platform_data.watchdog_work),
			  ts_watchdog_work);
		setup_timer(&(g_ts_kit_platform_data.watchdog_timer),
			    ts_watchdog_timer,
			    (unsigned long)(&g_ts_kit_platform_data));
	}
	return;
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
	struct ts_cmd_node cmd;

	TS_LOG_INFO("%s Enter\n", __func__);
	if (g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) {
		g_ts_kit_platform_data.chip_data->is_can_device_use_int = true;
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

static int ts_send_boot_detection_cmd(void)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;
	TS_LOG_INFO("%s Enter\n", __func__);
	cmd.command = TS_BOOT_DETECTION;
	error = ts_kit_put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
	}
	return error;
}
static void ts_boot_detection(void)
{
	int value = NO_ERR;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	if(dev->ops->chip_boot_detection && g_ts_kit_platform_data.chip_data->boot_detection_flag) {
		value = dev->ops->chip_boot_detection();
	}
	return;
}

static void proc_init_cmd(void)
{
	schedule_work(&tp_init_work);
	return;
}

static void tp_init_work_fn(struct work_struct *work)
{
	struct ts_cmd_node use_cmd;
	int i = TS_CMD_QUEUE_SIZE;
	struct ts_cmd_queue *q;
	unsigned long flags;
	struct ts_cmd_node *cmd = &use_cmd;
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	int error = NO_ERR;
	q = &g_ts_kit_platform_data.no_int_queue;

	//Call chip init
	g_ts_kit_platform_data.chip_data->isbootupdate_finish = false;
	mutex_lock(&g_ts_kit_platform_data.chip_data->device_call_lock);
	if (dev->ops->chip_init) {
		TS_LOG_INFO("%s, call chip init\n", __func__);
		error = dev->ops->chip_init();
	}
	mutex_unlock(&g_ts_kit_platform_data.chip_data->device_call_lock);
	/*deliver panel_id for sensor, direct_proc_cmd = 1 */
	if (g_ts_kit_platform_data.chip_data->provide_panel_id_support > 0) {
		schedule_work(&ts_panel_id_work);
	}

	g_ts_kit_platform_data.chip_data->isbootupdate_finish = true;
	if (error != NO_ERR) {
		TS_LOG_ERR("%s,chip init fail with error:%d\n", __func__,
			   error);
		return;
	}
	TS_LOG_INFO("%s, chip init done\n", __func__);
	while (i-- > 0) {
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
		if (error != NO_ERR) {
			TS_LOG_INFO("%s process init cmd %d error", __func__, cmd->command);
		}
	}
	if (g_ts_kit_platform_data.chip_data->is_parade_solution) {
		ts_start_wd_timer(&g_ts_kit_platform_data);
	}
}

int ts_kit_put_one_cmd_direct_sync(struct ts_cmd_node *cmd, int timeout)
{
	int error = NO_ERR;
	TS_LOG_INFO("%s Enter\n", __func__);
	if (g_ts_kit_platform_data.chip_data->is_parade_solution == 0) {
		return ts_kit_put_one_cmd(cmd, timeout);
	}

	if ((atomic_read(&g_ts_kit_platform_data.state) == TS_UNINIT)) {
		error = -EIO;
		return error;
	}
	if ((atomic_read(&g_ts_kit_platform_data.state) == TS_SLEEP)
	    || (atomic_read(&g_ts_kit_platform_data.state) == TS_WORK_IN_SLEEP)) {
		TS_LOG_INFO("%s In Sleep State\n", __func__);
		error = -EIO;
		return error;
	}

	return error;
}

int ts_kit_put_one_cmd(struct ts_cmd_node *cmd, int timeout)
{
	int error = -EIO;
	unsigned long flags;
	struct ts_cmd_queue *q;
	struct ts_cmd_sync *sync = NULL;
	if (!cmd) {
		TS_LOG_ERR("find null pointer\n");
		goto out;
	}

	if ((TS_UNINIT == atomic_read(&g_ts_kit_platform_data.state))
	    && (cmd->command != TS_CHIP_DETECT)) {
		TS_LOG_ERR("ts module not initialize\n");
		goto out;
	}

	if (timeout) {
		sync = (struct ts_cmd_sync *)kzalloc(sizeof(struct ts_cmd_sync),
							GFP_KERNEL);
		if (NULL == sync) {
			TS_LOG_ERR("failed to kzalloc completion\n");
			error = -ENOMEM;
			goto out;
		}
		init_completion(&sync->done);
		atomic_set(&sync->timeout_flag, TS_NOT_TIMEOUT);
		cmd->sync = sync;
	} else {
		cmd->sync = NULL;
	}

	if (g_ts_kit_platform_data.chip_data == NULL) {
		q = &g_ts_kit_platform_data.queue;
	} else {
		if ((g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) &&
		    (g_ts_kit_platform_data.chip_data->is_can_device_use_int ==
		     false) && (cmd->command != TS_CHIP_DETECT)) {
			if (cmd->command == TS_INT_PROCESS)
				goto out;	//Not use INT in the init process
			q = &g_ts_kit_platform_data.no_int_queue;
		} else {
			q = &g_ts_kit_platform_data.queue;
		}
	}
	cmd->ts_cmd_check_key = TS_CMD_CHECK_KEY;
	spin_lock_irqsave(&q->spin_lock, flags);
	smp_wmb();
	if (q->cmd_count == q->queue_size) {
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
	wake_up_process(g_ts_kit_platform_data.ts_task);	//wakeup process
	if (timeout
	    && !(wait_for_completion_timeout(&sync->done, abs(timeout) * HZ))) {
		atomic_set(&sync->timeout_flag, TS_TIMEOUT);
		TS_LOG_ERR("wait for cmd respone timeout\n");
		error = -EBUSY;
		goto out;
	}
	smp_wmb();
 free_sync:
	if (sync) {
		kfree(sync);
	}
 out:
	return error;
}

EXPORT_SYMBOL(ts_kit_put_one_cmd);
static int get_one_cmd(struct ts_cmd_node *cmd)
{
	unsigned long flags;
	int error = -EIO;
	struct ts_cmd_queue *q;
	if (unlikely(!cmd)) {
		TS_LOG_ERR("find null pointer\n");
		goto out;
	}

	q = &g_ts_kit_platform_data.queue;
	spin_lock_irqsave(&q->spin_lock, flags);
	smp_wmb();
	if (!q->cmd_count) {
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

static int ts_proc_command(struct ts_cmd_node *cmd)
{
	int error = NO_ERR;
	struct ts_cmd_sync *sync = NULL;
	struct ts_cmd_node *proc_cmd = cmd;
	struct ts_cmd_node *out_cmd = &pang_cmd_buff;
	if (!cmd || cmd->ts_cmd_check_key != TS_CMD_CHECK_KEY) {
		TS_LOG_ERR("invalid cmd, no need to process\n");
		goto out;
	}
	sync = cmd->sync;
	//discard timeout cmd to fix panic
	if (sync && atomic_read(&sync->timeout_flag) == TS_TIMEOUT) {
		kfree(sync);
		goto out;
	}

	if (!ts_cmd_need_process(proc_cmd)) {
		TS_LOG_INFO("no need to process cmd:%d", proc_cmd->command);
		goto out;
	}

 related_proc:
	out_cmd->command = TS_INVAILD_CMD;
	switch (proc_cmd->command) {
	case TS_INT_PROCESS:
		ts_proc_bottom_half(proc_cmd, out_cmd);
		enable_irq(g_ts_kit_platform_data.irq_id);
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
		ts_power_control(g_ts_kit_platform_data.irq_id, proc_cmd,
				 out_cmd);
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
	case TS_CHARGER_SWITCH:
		ts_kit_charger_switch(proc_cmd, out_cmd);
		break;
	case TS_REGS_STORE:
		ts_chip_regs_operate(proc_cmd, out_cmd, sync);
		break;
	case TS_TEST_CMD:
		ts_test_cmd(proc_cmd, out_cmd);
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
		    && proc_cmd->cmd_param.prv_params != NULL) {
			proc_cmd->cmd_param.ts_cmd_freehook(proc_cmd->cmd_param.
							    prv_params);
		}
		break;
	case TS_SET_SENSIBILITY:
		ts_set_sensibility(proc_cmd, out_cmd);
		break;
	case TS_BOOT_DETECTION:
		ts_boot_detection();
		break;
	default:
		break;
	}

	TS_LOG_DEBUG("command :%d process result:%d \n", proc_cmd->command,
		     error);
	if (out_cmd->command != TS_INVAILD_CMD) {
		TS_LOG_DEBUG("related command :%d  need process\n",
			     out_cmd->command);
		swap(proc_cmd, out_cmd);	//ping - pang
		goto related_proc;
	}

	if (sync)		//notify wait threads by completion
	{
		smp_mb();
		TS_LOG_DEBUG("wakeup threads in waitqueue\n");
		if (atomic_read(&sync->timeout_flag) == TS_TIMEOUT) {
			kfree(sync);
		} else {
			complete(&sync->done);
		}
	}

 out:
	return error;
}

int ts_kit_proc_command_directly(struct ts_cmd_node *cmd)
{
	int error = NO_ERR;
	struct ts_cmd_node outcmd;

	TS_LOG_DEBUG("%s Enter\n", __func__);
	/*Do not use cmd->sync in this func, setting it as null */
	cmd->sync = NULL;
	if (!ts_cmd_need_process(cmd)) {
		TS_LOG_INFO("%s, no need to process cmd:%d", __func__,
			    cmd->command);
		error = -EIO;
		goto out;
	}

	mutex_lock(&g_ts_kit_platform_data.chip_data->device_call_lock);
	switch (cmd->command) {
	case TS_INT_PROCESS:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_INPUT_ALGO:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_REPORT_INPUT:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_POWER_CONTROL:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_FW_UPDATE_BOOT:
		error = ts_fw_update_boot(cmd, &outcmd);
		break;
	case TS_FW_UPDATE_SD:
		error = ts_fw_update_sd(cmd, &outcmd);
		break;
	case TS_DEBUG_DATA:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
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
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_DSM_DEBUG:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_GLOVE_SWITCH:
		error = ts_glove_switch(cmd, &outcmd);
		break;
	case TS_TEST_TYPE:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_PALM_SWITCH:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_HAND_DETECT:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_FORCE_RESET:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_INT_ERR_OCCUR:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_ERR_OCCUR:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_CHECK_STATUS:
		ts_check_status(cmd, &outcmd);
		break;
	case TS_WAKEUP_GESTURE_ENABLE:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_HOLSTER_SWITCH:
		error = ts_holster_switch(cmd, &outcmd);
		break;
	case TS_ROI_SWITCH:
		error = ts_roi_switch(cmd, &outcmd);
		break;
	case TS_TOUCH_WINDOW:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_CHARGER_SWITCH:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_REGS_STORE:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_TEST_CMD:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_HARDWARE_TEST:
		TS_LOG_ERR("%s, command %d does not support direct call!",
			   __func__, cmd->command);
		break;
	case TS_READ_BRIGHTNESS_INFO:
		error = ts_get_brightness_info_cmd();
		break;
	case TS_TOUCH_SWITCH:
		ts_touch_switch_cmd();
		break;
	default:
		TS_LOG_ERR("%s, command %d unknown!", __func__, cmd->command);
		break;
	}
	mutex_unlock(&g_ts_kit_platform_data.chip_data->device_call_lock);
	TS_LOG_DEBUG("%s, command :%d process result:%d \n", __func__,
		     cmd->command, error);
 out:
	return error;
}

#if defined (CONFIG_HUAWEI_DSM)
void ts_dmd_report(int dmd_num, const char *pszFormat, ...)
{
	va_list args;
	char input_buf[TS_CHIP_DMD_REPORT_SIZE] = {0};
	char report_buf[TS_CHIP_DMD_REPORT_SIZE] = {0};

	va_start(args, pszFormat);
	vsnprintf(input_buf, sizeof(input_buf) - 1, pszFormat, args);
	va_end(args);
	snprintf(report_buf, sizeof(report_buf), "%stp state:%d chip_name:%s version_name:%s irq_gpio:%d value:%d ",\
		input_buf, atomic_read(&g_ts_kit_platform_data.power_state),g_ts_kit_platform_data.chip_data->chip_name,\
		g_ts_kit_platform_data.chip_data->version_name,g_ts_kit_platform_data.irq_gpio, gpio_get_value(g_ts_kit_platform_data.irq_gpio));

	if (!dsm_client_ocuppy(ts_dclient)) {
		dsm_client_record(ts_dclient, report_buf);
		dsm_client_notify(ts_dclient, dmd_num);
		TS_LOG_INFO("ts_dmd_report %s\n", report_buf);
	}
	return;
}

EXPORT_SYMBOL(ts_dmd_report);
#endif
static int ts_kit_init(void *data)
{
	int error = NO_ERR;
	struct input_dev *input_dev = NULL;
	atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
	atomic_set(&g_ts_kit_platform_data.ts_esd_state, TS_NO_ESD);
	TS_LOG_INFO("ts_kit_init\n");
	g_ts_kit_platform_data.edge_wideth = EDGE_WIDTH_DEFAULT;
	TS_LOG_DEBUG("ts init: cmd queue size : %d\n", TS_CMD_QUEUE_SIZE);
	wake_lock_init(&g_ts_kit_platform_data.ts_wake_lock,
		       WAKE_LOCK_SUSPEND, "ts_wake_lock");
	g_ts_kit_platform_data.panel_id = 0xFF;
	error = ts_kit_parse_config();
	if (error) {
		TS_LOG_ERR("ts init parse config failed : %d\n", error);
		goto err_out;
	}
	TS_LOG_INFO("ts_kit_parse_config success\n");
	error = ts_kit_create_sysfs();
	if (error) {
		TS_LOG_ERR("ts init create sysfs failed : %d\n", error);
		goto err_out;
	}

	error = ts_kit_chip_init();
	if (error) {
		TS_LOG_ERR("chip init failed : %d,  try fw update again\n",
			   error);
	}

	if (g_ts_kit_platform_data.chip_data->is_direct_proc_cmd) {
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
	error = ts_register_algo();
	if (error) {
		TS_LOG_ERR("ts register algo failed : %d\n", error);
		goto err_remove_sysfs;
	}

	error = ts_kit_misc_init(&g_ts_kit_platform_data);
	if (error) {
		TS_LOG_ERR("%s: fail to misc init %d\n", __func__, error);
		goto err_remove_sysfs;
	}

	error = ts_kit_input_device_register(input_dev);
	if (error) {
		TS_LOG_ERR("ts init input device register failed : %d\n",
			   error);
		goto err_remove_sysfs;
	}
	ts_kit_status_check_init();
	error = ts_kit_pm_init();
	if (error) {
		TS_LOG_ERR("ts init pm init failed : %d\n", error);
		goto err_free_input_dev;
	}
#if defined (CONFIG_TEE_TUI)
	g_ts_kit_platform_data.chip_data->tui_data = &tee_tui_data;
	register_tui_driver(tui_tp_init, "tp",
			    g_ts_kit_platform_data.chip_data->tui_data);
#endif
	error = ts_kit_irq_init();
	if (error) {
		TS_LOG_ERR("ts init irq_init failed : %d\n", error);
		goto err_unregister_suspend;
	}
	/*get brightness info */
	ts_get_brightness_info();
	error = ts_kit_update_firmware();
	if (error) {
		TS_LOG_ERR("ts init update_firmware failed : %d\n", error);
		goto err_firmware_update;
	}
	ts_send_roi_cmd(TS_ACTION_READ, NO_SYNC_TIMEOUT);	/*roi function set as default by TP firmware */
	ts_send_init_cmd();	/*Send this cmd to make sure all the cmd in the init is called */
#if defined (CONFIG_HISI_BCI_BATTERY)
	if (g_ts_kit_platform_data.register_charger_notifier) {
		ts_kit_charger_notifier_register();
		TS_LOG_INFO("charger notifier is register\n");
	}
#endif
	if(g_ts_kit_platform_data.chip_data->ts_platform_data->feature_info.pen_info.supported_pen_alg) {
		ts_pen_open_confirm_init();
	}
	ts_kit_status_check_start();
	if(TS_BOOT_DETECTION_SUPPORT == g_ts_kit_platform_data.chip_data->boot_detection_flag) {
		ts_send_boot_detection_cmd();
	}

	error = NO_ERR;
	TS_LOG_INFO("ts_kit_init called out\n");
	goto out;
 err_firmware_update:
	free_irq(g_ts_kit_platform_data.irq_id, &g_ts_kit_platform_data);
 err_unregister_suspend:
 err_free_input_dev:
	input_unregister_device(input_dev);
	input_free_device(input_dev);
	ts_kit_misc_destory();
 err_remove_sysfs:
	sysfs_remove_link(NULL, "touchscreen");
	sysfs_remove_group(&g_ts_kit_platform_data.ts_dev->dev.kobj,
			   &ts_attr_group);
	platform_device_del(g_ts_kit_platform_data.ts_dev);
	platform_device_put(g_ts_kit_platform_data.ts_dev);
 err_out:
	atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
	atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);
	wake_lock_destroy(&g_ts_kit_platform_data.ts_wake_lock);
 out:
	TS_LOG_INFO("ts_init, g_ts_kit_platform_data.state : %d\n",
		    atomic_read(&g_ts_kit_platform_data.state));
	if (error) {
		TS_LOG_ERR("ts_init  failed\n");
#if defined (CONFIG_HUAWEI_DSM)
		ts_dmd_report(DSM_TP_INIT_ERROR_NO,
			      "try to client record 926004032 for tp init error \n");
#endif
	}
	return error;
}

static void ts_ic_shutdown(void)
{
	struct ts_kit_device_data *dev = g_ts_kit_platform_data.chip_data;
	if (dev->ops->chip_shutdown) {
		dev->ops->chip_shutdown();
	}
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

	input_unregister_device(g_ts_kit_platform_data.input_dev);
	input_free_device(g_ts_kit_platform_data.input_dev);
	ts_kit_misc_destory();
	sysfs_remove_link(NULL, "touchscreen");
	sysfs_remove_group(&g_ts_kit_platform_data.ts_dev->dev.kobj,
			   &ts_attr_group);
	wake_lock_destroy(&g_ts_kit_platform_data.ts_wake_lock);
	platform_device_unregister(g_ts_kit_platform_data.ts_dev);
	ts_destory_client();
	if ((NULL != g_ts_kit_platform_data.charger_detect_notify.notifier_call)
	    && g_ts_kit_platform_data.register_charger_notifier) {
		charger_type_notifier_unregister
		    (&g_ts_kit_platform_data.charger_detect_notify);
		TS_LOG_INFO("charger_type_notifier_unregister called\n");
	}
	memset(&g_ts_kit_platform_data, 0, sizeof(struct ts_kit_platform_data));
	TS_LOG_ERR("ts_thread exited\n");
}

static bool ts_task_continue(void)
{
	bool task_continue = true;
	unsigned long flags;
	TS_LOG_DEBUG("prepare enter idle\n");
 repeat:
	if (unlikely(kthread_should_stop())) {
		task_continue = false;
		goto out;
	}
	spin_lock_irqsave(&g_ts_kit_platform_data.queue.spin_lock, flags);
	smp_wmb();
	if (g_ts_kit_platform_data.queue.cmd_count) {
		set_current_state(TASK_RUNNING);
		TS_LOG_DEBUG("ts task state to  TASK_RUNNING\n");
		goto out_unlock;
	} else {
		set_current_state(TASK_INTERRUPTIBLE);
		TS_LOG_DEBUG("ts task state to  TASK_INTERRUPTIBLE\n");
		spin_unlock_irqrestore(&g_ts_kit_platform_data.queue.spin_lock,
				       flags);
		schedule();
		goto repeat;
	}

 out_unlock:
	spin_unlock_irqrestore(&g_ts_kit_platform_data.queue.spin_lock, flags);
 out:
	return task_continue;
}

static int ts_thread(void *p)
{
	static const struct sched_param param = {
		//.sched_priority = MAX_USER_RT_PRIO / 2,
		.sched_priority = 99,
	};
	smp_wmb();
	TS_LOG_INFO("ts_thread\n");
	memset(&ping_cmd_buff, 0, sizeof(struct ts_cmd_node));
	memset(&pang_cmd_buff, 0, sizeof(struct ts_cmd_node));
	smp_mb();
	sched_setscheduler(current, SCHED_RR, &param);
	while (ts_task_continue()) {
		while (!get_one_cmd(&ping_cmd_buff))	//get one command
		{
			ts_proc_command(&ping_cmd_buff);
			memset(&ping_cmd_buff, 0, sizeof(struct ts_cmd_node));
			memset(&pang_cmd_buff, 0, sizeof(struct ts_cmd_node));
		}
	}

	TS_LOG_ERR("ts thread stop\n");
	ts_kit_exit();

	platform_device_unregister(g_ts_kit_platform_data.ts_dev);
	ts_destory_client();
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	if (g_ts_kit_platform_data.reset_gpio) {
		gpio_free(g_ts_kit_platform_data.reset_gpio);
	}
	gpio_free(g_ts_kit_platform_data.irq_gpio);
#endif
	atomic_set(&g_ts_kit_platform_data.state, TS_UNINIT);
	atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);
	return NO_ERR;
}

void ts_kit_chip_detect(struct ts_kit_device_data *chipdata, int timeout)
{
	int error = 0;
	struct ts_cmd_node cmd;
	TS_LOG_INFO("ts_kit_chip_detect called\n");
	memset(&cmd, 0, sizeof(struct ts_cmd_node));
	//atomic_set(&g_ts_kit_platform_data.state, TS_WORK);
	cmd.command = TS_CHIP_DETECT;
	cmd.cmd_param.pub_params.chip_data = chipdata;
	error = ts_kit_put_one_cmd(&cmd, timeout);
	if (error) {
		TS_LOG_ERR("ts_kit_chip_detect, put cmd error :%d\n", error);
	}

	return;
}

static int _ts_of_property_read_u32(struct device_node *np,
				const char *propname,
				int *out_val,
				int quiet)
{
	int ret = 0;
	ret = of_property_read_u32(np, propname, out_val);
	if (quiet)
		return ret;

	if (ret)
		TS_LOG_ERR("%s not configed\n", propname);
	else
		TS_LOG_INFO("%s configed value = %d", propname, *out_val);

	return ret;
}

static int _ts_of_property_read_u16(struct device_node *np,
				const char *propname,
				u16 *out_val,
				int quiet)
{
	int val = 0;
	int ret = 0;

	ret = _ts_of_property_read_u32(np, propname, &val, quiet);
	if (!ret)
		*out_val = (u16)val;

	return ret;
}

static int _ts_of_property_read_u8(struct device_node *np,
				const char *propname,
				u8 *out_val,
				int quiet)
{
	int val = 0;
	int ret = 0;

	ret = _ts_of_property_read_u32(np, propname, &val, quiet);

	if (!ret)
		*out_val = (u8)val;

	return ret;
}

#define ts_of_property_read_u32(...) _ts_of_property_read_u32(__VA_ARGS__, 0)
#define ts_of_property_read_u32_quiet(...) _ts_of_property_read_u32(__VA_ARGS__, 1)
#define ts_of_property_read_u16(...) _ts_of_property_read_u16(__VA_ARGS__, 0)
#define ts_of_property_read_u16_quiet(...) _ts_of_property_read_u16(__VA_ARGS__, 1)
#define ts_of_property_read_u8(...) _ts_of_property_read_u8(__VA_ARGS__, 0)
#define ts_of_property_read_u8_quiet(...) _ts_of_property_read_u8(__VA_ARGS__, 1)

static int ts_of_property_read_u32_default(struct device_node *np,
				const char *propname,
				unsigned int *out_val, int default_val)
{
	int ret = ts_of_property_read_u32(np, propname, out_val);
	if (ret)
		*out_val = default_val;
	return ret;
}

static int ts_parse_hardware_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data)
{
	ts_of_property_read_u32(np, "slave_address", &chip_data->slave_addr);

	ts_of_property_read_u32(np, "irq_config", &chip_data->irq_config);

	return 0;
}

static int ts_parse_captest_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data)
{
	int ret = 0;
	int array_len = 0;
	int index = 0;
	const char *raw_data_dts = NULL;

	ts_of_property_read_u32_quiet(np, "is_ic_rawdata_proc_printf",&chip_data->is_ic_rawdata_proc_printf);
	ts_of_property_read_u8(np, "rawdata_newformatflag",&chip_data->rawdata_newformatflag);
	ts_of_property_read_u8(np, "print_all_trx_diffdata_for_newformat_flag",&chip_data->print_all_trx_diffdata_for_newformat_flag);
	ts_of_property_read_u32_quiet(np, "boot_detection_addr", &chip_data->boot_detection_addr);
	ts_of_property_read_u32_quiet(np, "boot_detection_threshold", &chip_data->boot_detection_threshold);
	ts_of_property_read_u8(np, "boot_detection_flag", &chip_data->boot_detection_flag);

	ts_of_property_read_u32_quiet(np, "unite_cap_test_interface", &chip_data->unite_cap_test_interface);
	ts_of_property_read_u32_quiet(np, "report_rate_test", &chip_data->report_rate_test);
	ts_of_property_read_u32_quiet(np, "huawei,test_capacitance_via_csvfile", &chip_data->test_capacitance_via_csvfile);
	ts_of_property_read_u32_quiet(np, "rawdata_get_timeout", &chip_data->rawdata_get_timeout);
	ts_of_property_read_u32_default(np, "rawdata_report_type", &chip_data->rawdata_report_type, 3);
	ts_of_property_read_u32(np, "rawdata_arrange_type", &chip_data->rawdata_arrange_type);
	ts_of_property_read_u32(np, "capacitance_test_config", &chip_data->capacitance_test_config);
	ts_of_property_read_u32(np, "raw_test_type", &chip_data->raw_test_type);
	ts_of_property_read_u32(np, "self_cap_test", &chip_data->self_cap_test);
	ts_of_property_read_u32(np, "huawei,support_forcekey_cap_value_test",
					&chip_data->support_forcekey_cap_value_test);
	ts_of_property_read_u32(np, "should_check_tp_calibration_info",
					&chip_data->should_check_tp_calibration_info);
	ts_of_property_read_u32_quiet(np, "huawei,csvfile_use_product_system", &chip_data->csvfile_use_product_system);
	ts_of_property_read_u32_quiet(np, "huawei,td43xx_ee_short_test", &chip_data->td43xx_ee_short_test_support);
	ts_of_property_read_u32_quiet(np, "huawei,trx_delta_test_support", &chip_data->trx_delta_test_support);
	ts_of_property_read_u32_quiet(np, "huawei,threshold_tddi_ee_short_partone", &chip_data->tddi_ee_short_test_partone_limit);
	ts_of_property_read_u32_quiet(np, "huawei,threshold_tddi_ee_short_parttwo", &chip_data->tddi_ee_short_test_parttwo_limit);

	ret =  of_property_read_string(np, "tp_test_type",
				 &chip_data->tp_test_type);
	if (ret) {
		TS_LOG_INFO("tp_test_type not configed, use default\n");
		chip_data->tp_test_type = "Normalize_type:judge_different_reslut:17";//17 for 0f 4f
	}

	if (chip_data->unite_cap_test_interface) {
		array_len = of_property_count_strings(np, "raw_data_limit");
		if (array_len <= 0 || array_len > RAWDATA_NUM) {
			TS_LOG_ERR("raw_data_limit length invaild or dts number is larger than:%d\n",
					array_len);
		}

		for (index = 0; index < array_len; index++) {
			ret = of_property_read_string_index(np,
							  "raw_data_limit",
							  index, &raw_data_dts);
			if (ret) {
				TS_LOG_ERR ("%s: fail to read index = %d,raw_data_limit = %s,ret = %d,\n",
						__func__, index, raw_data_dts, ret);
			}

			if (!ret) {
				chip_data->raw_limit_buf[index] =
				    simple_strtol(raw_data_dts, NULL, 10);
				TS_LOG_INFO("rawdatabuf[%d] = %d\n", index,
					    chip_data->raw_limit_buf[index]);
			}
		}
	}
	return 0;
}

static int ts_parse_feature_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data)
{
	ts_of_property_read_u32(np, "parade,is_parade_solution",&chip_data->is_parade_solution);
	ts_of_property_read_u32(np, "parade,direct_proc_cmd", &chip_data->is_direct_proc_cmd);
	ts_of_property_read_u32(np, "parade,sleep_in_mode", &chip_data->sleep_in_mode);
	ts_of_property_read_u32(np, "diff_data_report_supported", &chip_data->diff_data_report_supported);
	ts_of_property_read_u8(np, "roi_supported", &chip_data->ts_platform_data->feature_info.roi_info.roi_supported);
	ts_of_property_read_u8(np, "glove_supported", &chip_data->ts_platform_data->feature_info.glove_info.glove_supported);
	ts_of_property_read_u8(np, "holster_supported", &chip_data->ts_platform_data->feature_info.holster_info.holster_supported);
	ts_of_property_read_u8(np, "pen_support", &chip_data->ts_platform_data->feature_info.pen_info.pen_supported);
	ts_of_property_read_u32(np, "supported_pen_alg", &chip_data->ts_platform_data->feature_info.pen_info.supported_pen_alg);
	ts_of_property_read_u32(np, "supported_pen_mmitest", &chip_data->ts_platform_data->feature_info.pen_info.supported_pen_mmitest);
	ts_of_property_read_u32(np, "support_3d_func", &chip_data->support_3d_func);
	ts_of_property_read_u8(np, "charger_supported", &chip_data->ts_platform_data->feature_info.charger_info.charger_supported);
	ts_of_property_read_u32_quiet(np, "check_bulcked", &chip_data->check_bulcked);
	ts_of_property_read_u32_quiet(np, "support_aft", &chip_data->support_aft);
	ts_of_property_read_u32_quiet(np, "fp_tp_report_touch_minor_event", &chip_data->fp_tp_report_touch_minor_event);
	ts_of_property_read_u32(np, "algo_id", &chip_data->algo_id);
	ts_of_property_read_u32_quiet(np, "need_wd_check_status", &chip_data->need_wd_check_status);
	ts_of_property_read_u32_quiet(np, "check_status_watchdog_timeout", &chip_data->check_status_watchdog_timeout);
	ts_of_property_read_u32_quiet(np, "touch_switch_flag", &chip_data->touch_switch_flag);
	ts_of_property_read_u32_quiet(np, "get_brightness_info_flag", &chip_data->get_brightness_info_flag);
	/* for virtual key */
	ts_of_property_read_u32_quiet(np, "has_virtualkey", &chip_data->has_virtualkey);
	ts_of_property_read_u32_quiet(np, "lcd_full", &chip_data->lcd_full);
	ts_of_property_read_u32_quiet(np, "send_stylus_gesture_switch", &chip_data->send_stylus_gesture_switch);

	ts_of_property_read_u8_quiet(np, "provide_panel_id_support", &chip_data->provide_panel_id_support);
	ts_of_property_read_u32(np, "suspend_no_config",&chip_data->suspend_no_config);
	return 0;
}
static int ts_parse_input_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data)
{
	int retval = 0;

	ts_of_property_read_u32_default(np, "x_max", &chip_data->x_max, 1080);
	ts_of_property_read_u32_default(np, "y_max", &chip_data->y_max, 1920);
	ts_of_property_read_u32_default(np, "x_max_mt", &chip_data->x_max_mt, chip_data->x_max);
	ts_of_property_read_u32_default(np, "y_max_mt", &chip_data->y_max_mt, chip_data->y_max);
	ts_of_property_read_u32_default(np, "flip_x", &chip_data->flip_x, true);
	ts_of_property_read_u32_default(np, "flip_y", &chip_data->flip_y, true);

	TS_LOG_INFO("x_max = %d, y_max = %d, x_mt = %d,y_mt = %d\n",
		chip_data->x_max, chip_data->y_max, chip_data->x_max_mt, chip_data->y_max_mt);
	return 0;
}
static int ts_kit_get_power_config(struct device_node *chip_node,
			struct ts_kit_platform_data *cd);
static int ts_parse_power_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data)
{
	struct ts_kit_platform_data *cd = &g_ts_kit_platform_data;

	ts_kit_get_power_config(np, cd);
	ts_of_property_read_u32(np, "vddio_default_on", &chip_data->vddio_default_on);

	return NO_ERR;
}
/*
 * Parse basic hardware about config
 * before call chip detect
 */
static int ts_parse_chip_base_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data)
{
	if (!np) {
		TS_LOG_ERR("%s:devices node invaild\n", __func__);
		return -EINVAL;
	}

	ts_parse_hardware_config(np,chip_data);
	ts_parse_power_config(np,chip_data);

	ts_of_property_read_u32(np, "ic_type", &chip_data->ic_type);
	ts_of_property_read_u32_default(np, "projectid_len", &chip_data->projectid_len, 10);

	ts_of_property_read_u32_default(np, "is_in_cell", &chip_data->is_in_cell, 0);

	return NO_ERR;
}

/*
 * chip driver call this function to parse some panel specific config
 * e.g  different panel has different glove ctrl address
 * parm: devices -- panel config node
 *       chip_data -- chip driver data
 */
int ts_parse_panel_specific_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data)
{
	int retval = 0;
	int read_val = 0;
	const char *producer = NULL;
	ts_parse_captest_config(np, chip_data);
	/* parse module name */
	retval = of_property_read_string(np, "producer", &producer);
	if (retval) {
		TS_LOG_ERR("read producer error.\n");
	}
	if (producer) {
		TS_LOG_INFO("producer = %s\n", producer);
		strncpy(chip_data->module_name, producer, MAX_STR_LEN - 1);
	}

	ts_of_property_read_u32(np, "supported_func_indicater", &chip_data->supported_func_indicater);
	ts_of_property_read_u32(np, "ic_type", &chip_data->ic_type);
	ts_of_property_read_u16_quiet(np, "charger_switch_bit",
		 &chip_data->ts_platform_data->feature_info.charger_info.charger_switch_bit);
	ts_of_property_read_u16_quiet(np, "charger_switch_addr",
		&chip_data->ts_platform_data->feature_info.charger_info.charger_switch_addr);
	ts_of_property_read_u16_quiet(np, "holster_switch_bit",
		&chip_data->ts_platform_data->feature_info.holster_info.holster_switch_bit);
	ts_of_property_read_u16_quiet(np, "holster_switch_addr",
				 &chip_data->ts_platform_data->feature_info.holster_info.holster_switch_addr);
	ts_of_property_read_u16_quiet(np, "glove_switch_bit",
		&chip_data->ts_platform_data->feature_info.glove_info.glove_switch_bit);
	ts_of_property_read_u16_quiet(np, "glove_switch_addr",
				 &chip_data->ts_platform_data->feature_info.glove_info.glove_switch_addr);
	ts_of_property_read_u16_quiet(np, "roi_control_addr",
		&chip_data->ts_platform_data->feature_info.roi_info.roi_control_addr);
	ts_of_property_read_u8_quiet(np, "roi_control_bit",
		&chip_data->ts_platform_data->feature_info.roi_info.roi_control_bit);
	ts_of_property_read_u16_quiet(np, "roi_data_addr",
		&chip_data->ts_platform_data->feature_info.roi_info.roi_data_addr);
	ts_of_property_read_u32_default(np, "fw_update_logic", &chip_data->fw_update_logic, FW_UPDATE_DIFF);
	ts_of_property_read_u32_quiet(np, "noise_state_reg", &chip_data->noise_state_reg);
	ts_of_property_read_u32_quiet(np, "ic_status_reg", &chip_data->ic_status_reg);

	/* touch switch regs */
	ts_of_property_read_u32_quiet(np, "touch_switch_flag", &chip_data->touch_switch_flag);
	ts_of_property_read_u16_quiet(np, "touch_switch_reg", &chip_data->touch_switch_reg);
	ts_of_property_read_u16_quiet(np, "touch_switch_hold_off_reg", &chip_data->touch_switch_hold_off_reg);
	ts_of_property_read_u16_quiet(np, "touch_game_reg", &chip_data->touch_game_reg);
	ts_of_property_read_u8_quiet(np, "game_control_bit", &chip_data->game_control_bit);
	ts_of_property_read_u16_quiet(np, "touch_scene_reg", &chip_data->touch_scene_reg);

	/*diff data report config*/
	ts_of_property_read_u32(np, "diff_data_len", &chip_data->diff_data_len);
	ts_of_property_read_u32(np, "diff_data_control_addr", &chip_data->diff_data_control_addr);

	if (!ts_of_property_read_u16_quiet(np, "aft_data_addr", &chip_data->aft_data_addr)) {
		chip_data->support_aft = 0;
		TS_LOG_INFO("cannot get chip aft_data_addr, set support_aft disable\n");
	}
	ts_of_property_read_u32_quiet(np, "use_new_oem_structure", &chip_data->is_new_oem_structure);
	ts_of_property_read_u32_quiet(np, "huawei,support_2dbarcode_info",
					&chip_data->support_2dbarcode_info);
	ts_of_property_read_u32_quiet(np, "huawei,read_2dbarcode_oem_type",
					&chip_data->read_2dbarcode_oem_type);

	return 0;
}

/*
 * Parse all configs after chip detect and befor chip init
 * np -- chip driver dts node
 * chip_data -- chip driver data
 */
static int ts_parse_chip_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data)
{
	ts_parse_input_config(np, chip_data);
	ts_parse_feature_config(np,chip_data);
	ts_parse_panel_specific_config(np, chip_data);
	return 0;
}


#define IS_INVAILD_POWER_ID(x) (x >= TS_KIT_POWER_ID_MAX)

static char *ts_kit_power_name[TS_KIT_POWER_ID_MAX] = {
	"ts-kit-iovdd",
	"ts-kit-vcc",
};

static const char* ts_kit_power_id2name(enum ts_kit_power_id id)
{
	return !IS_INVAILD_POWER_ID(id) ? ts_kit_power_name[id] : 0;
}

int ts_kit_power_supply_get(enum ts_kit_power_id power_id)
{
	struct ts_kit_platform_data *cd = &g_ts_kit_platform_data;
	struct ts_kit_power_supply *power;
	int ret = 0;

	if (IS_INVAILD_POWER_ID(power_id)) {
		TS_LOG_ERR("%s: invalid power id %d", __func__, power_id);
		return -EINVAL;
	}

	power = &cd->ts_kit_powers[power_id];
	if (power->type == TS_KIT_POWER_UNUSED) {
		return 0;
	}

	if (power->use_count) {
		power->use_count++;
		return 0;
	}
	switch (power->type) {
	case TS_KIT_POWER_LDO:
		power->regulator = regulator_get(&cd->ts_dev->dev, ts_kit_power_id2name(power_id));
		if (IS_ERR_OR_NULL(power->regulator)) {
			TS_LOG_ERR("%s:fail to get %s\n", __func__, ts_kit_power_id2name(power_id));
			return -ENODEV;
		}

		ret = regulator_set_voltage(power->regulator, power->ldo_value, power->ldo_value);
		if (ret) {
			regulator_put(power->regulator);
			TS_LOG_ERR("%s:fail to set %s valude %d\n", __func__,
					ts_kit_power_id2name(power_id), power->ldo_value);
			return ret;
		}
		break;
	case TS_KIT_POWER_GPIO:
		ret = gpio_request(power->gpio, ts_kit_power_id2name(power_id));
		if (ret) {
			TS_LOG_ERR("%s:request gpio %d for %s failed\n", __func__,
					power->gpio, ts_kit_power_id2name(power_id));
			return ret;
		}
		break;
	default:
		TS_LOG_ERR("%s: invalid power type %d\n", __func__, power->type);
		return -EINVAL;
	}
	power->use_count++;
	return 0;
}

int ts_kit_power_supply_put(enum ts_kit_power_id power_id)
{
	struct ts_kit_platform_data *cd = &g_ts_kit_platform_data;
	struct ts_kit_power_supply *power;

	if (IS_INVAILD_POWER_ID(power_id)) {
		TS_LOG_ERR("%s: invalid power id %d", __func__, power_id);
		return -EINVAL;
	}

	power = &cd->ts_kit_powers[power_id];
	if (power->type == TS_KIT_POWER_UNUSED) {
		return 0;
	}
	if ((--power->use_count) > 0)
		return 0;

	switch (power->type) {
	case TS_KIT_POWER_LDO:
		// regulator_disable(power->regulator);
		regulator_put(power->regulator);
		break;
	case TS_KIT_POWER_GPIO:
		gpio_direction_output(power->gpio, 0);
		gpio_free(power->gpio);
		break;
	default:
		TS_LOG_ERR("%s: invalid power type %d\n", __func__, power->type);
		return -EINVAL;
	}
	return 0;
}

int ts_kit_power_supply_ctrl(enum ts_kit_power_id power_id, int status, unsigned int delay_ms)
{
	struct ts_kit_platform_data *cd = &g_ts_kit_platform_data;
	struct ts_kit_power_supply *power;
	int rc = 0;
	if (IS_INVAILD_POWER_ID(power_id)) {
		TS_LOG_ERR("%s: invalid power id %d", __func__, power_id);
		return -EINVAL;
	}

	power = &cd->ts_kit_powers[power_id];
	if (power->type == TS_KIT_POWER_UNUSED) {
		return 0;
	}
	if (!power->use_count) {
		TS_LOG_ERR("%s:regulator %s not gotten yet\n", __func__,
				ts_kit_power_id2name(power_id));
		return -ENODEV;
	}
	switch (power->type) {
	case TS_KIT_POWER_LDO:
		rc =  status ? regulator_enable(power->regulator) :
			regulator_disable(power->regulator);
		if (rc)
			TS_LOG_ERR("%s:regulator %s %s failed, %d\n", __func__,
				ts_kit_power_id2name(power_id), status ? "enable" : "disable", rc);

		break;
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	case TS_KIT_POWER_GPIO:
		gpio_direction_output(power->gpio, status ? 1 : 0);
		break;
#endif
	default:
		TS_LOG_ERR("%s: invalid power type %d\n", __func__, power->type);
		return -EINVAL;
	}
	mdelay(delay_ms);

	TS_LOG_INFO("%s:regulator %s %s done\n", __func__,
				ts_kit_power_id2name(power_id), status ? "enable" : "disable");
	return rc;
}

#define POWER_CONFIG_NAME_MAX 20
static int ts_kit_parse_one_power(struct device_node *chip_node,
			struct ts_kit_platform_data *cd,
			int power_id)
{
	const char *power_name;
	char config_name[POWER_CONFIG_NAME_MAX] = {0};
	struct ts_kit_power_supply *power;
	int rc;

	power_name = ts_kit_power_id2name(power_id);
	power = &cd->ts_kit_powers[power_id];

	rc = snprintf(config_name, POWER_CONFIG_NAME_MAX - 1, "%s-type", power_name);

	rc = of_property_read_u32(chip_node, config_name, &power->type);
	if (rc || power->type == TS_KIT_POWER_UNUSED) {
		TS_LOG_INFO("%s: power %s type not config or 0, unused\n", __func__, config_name);
		return 0;
	}

	switch (power->type) {
	case TS_KIT_POWER_GPIO:
		snprintf(config_name, POWER_CONFIG_NAME_MAX - 1, "%s-gpio", power_name);
		power->gpio = of_get_named_gpio(chip_node, config_name, 0);
		if (!gpio_is_valid(power->gpio)) {
			TS_LOG_ERR("%s:failed to get %s\n", __func__, config_name);
			return -ENODEV;
		}
		break;
	case TS_KIT_POWER_LDO:
		snprintf(config_name, POWER_CONFIG_NAME_MAX - 1, "%s-value", power_name);
		rc = of_property_read_u32(chip_node, config_name, &power->ldo_value);
		if (rc) {
			TS_LOG_ERR("%s:failed to get %s\n", __func__, config_name);
			return rc;
		}
		break;
	default:
		TS_LOG_ERR("%s: invaild power type %d", __func__, power->type);
		break;
	}

	return 0;
}

static int ts_kit_get_power_config(struct device_node *chip_node,
			struct ts_kit_platform_data *cd)
{
	int rc;
	int i;

	for (i = 0; i < TS_KIT_POWER_ID_MAX; i++) {
		rc = ts_kit_parse_one_power(chip_node, cd, i);
		if (rc)
			return rc;
	}

	return 0;
}

static u8 ts_init_flag = 0;
int huawei_ts_chip_register(struct ts_kit_device_data *chipdata)
{
	int ret = NO_ERR;
	TS_LOG_INFO("huawei_ts_chip_register called here\n");
	if (NULL == chipdata) {
		TS_LOG_ERR("%s chipdata is null\n", __func__);
		return -EINVAL;
	}

	/*
	 * Do chip config parse before call chip detect
	 */
	ret = ts_parse_chip_base_config(chipdata->cnode, chipdata);
	if (ret) {
		TS_LOG_ERR("%s:parse chip config faild\n", __func__);
		return ret;
	}
	if ((ts_init_flag == 1) && (TS_UNREGISTER == atomic_read(&g_ts_kit_platform_data.register_flag))) {
		if (chipdata->ops->chip_detect)
			ts_kit_chip_detect(chipdata, NO_SYNC_TIMEOUT);
	} else {
		return -EPERM;
	}

	return 0;
}

/************** Begin ts event notify block *********************/
static BLOCKING_NOTIFIER_HEAD(ts_event_nh);
int ts_event_notifier_register(struct notifier_block *nb)
{
	TS_LOG_INFO("%s + \n", __func__);
	if (!nb) {
		TS_LOG_ERR("nb == NULL \n");
		return -EINVAL;
	}
	return blocking_notifier_chain_register(&ts_event_nh, nb);
}

EXPORT_SYMBOL_GPL(ts_event_notifier_register);
int ts_event_notifier_unregister(struct notifier_block *nb)
{
	TS_LOG_INFO("%s + \n", __func__);
	if (!nb) {
		TS_LOG_ERR("nb == NULL \n");
		return -EINVAL;
	}
	return blocking_notifier_chain_unregister(&ts_event_nh, nb);
}

EXPORT_SYMBOL_GPL(ts_event_notifier_unregister);
// ret: 0 OK, other fail.
int ts_event_notify(ts_notify_event_type event)	// for panel use to notify event.
{
	return blocking_notifier_call_chain(&ts_event_nh,
					    (unsigned long)event, NULL);
}

/************** End ts event notify block *********************/

int ts_kit_get_pt_station_status(int *status)
{
	struct lcd_kit_ops *lcd_ops = lcd_kit_get_ops();
	int retval;

	if (!status)
		return -EINVAL;

	if((lcd_ops)&&(lcd_ops->get_status_by_type)) {
		retval = lcd_ops->get_status_by_type(PT_STATION_TYPE, status);
		if(retval < 0) {
			TS_LOG_ERR("%s: get pt station status fail\n", __func__);
			return retval;
		}
	}

	return 0;
}

static int __init huawei_ts_module_init(void)
{
	int error = NO_ERR;
	ts_init_flag = 0;
	TS_LOG_INFO("huawei_ts, huawei_ts_module_init called here\n");
	memset(&g_ts_kit_platform_data, 0, sizeof(struct ts_kit_platform_data));
	atomic_set(&g_ts_kit_platform_data.register_flag, TS_UNREGISTER);
	atomic_set(&g_ts_kit_platform_data.power_state, TS_UNINIT);
	error = get_ts_board_info();
	if (error) {
		TS_LOG_ERR("get bus info failed :%d\n", error);
		goto out;
	}
	error = ts_create_client();
	if (error) {
		TS_LOG_ERR("create device failed :%d\n", error);
		goto out;
	}
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	error = ts_gpio_request();
	if (error) {
		TS_LOG_ERR("ts_gpio_request failed :%d\n", error);
		goto err_remove_client;
	}
#endif
	g_ts_kit_platform_data.ts_dev = platform_device_alloc("huawei_touch", -1);
	if (!g_ts_kit_platform_data.ts_dev) {
		TS_LOG_ERR("platform device malloc failed\n");
		error = -ENOMEM;
		goto err_remove_gpio;
	}
	error = platform_device_add(g_ts_kit_platform_data.ts_dev);
	if (error) {
		TS_LOG_ERR("platform device add failed :%d\n", error);
		goto err_put_platform_dev;
	}

	ts_init_flag = 1;

	error = ts_kit_ops_register(&ts_kit_ops);
	if (error) {
		TS_LOG_ERR("ts_kit_ops_register failed :%d\n", error);
		goto err_put_platform_dev;
	}

	g_ts_kit_platform_data.ts_init_task =
	    kthread_create(ts_kit_init, &g_ts_kit_platform_data,
			   "ts_init_thread:%d", 0);
	if (IS_ERR(g_ts_kit_platform_data.ts_init_task)) {
		TS_LOG_ERR("create ts_thread failed\n");
		error = ts_destory_client();
		memset(&g_ts_kit_platform_data, 0,
		       sizeof(struct ts_kit_platform_data));
		error = -EINVAL;
		goto out;
	}
	g_ts_kit_platform_data.ts_task =
	    kthread_create(ts_thread, &g_ts_kit_platform_data, "ts_thread:%d",
			   0);
	if (IS_ERR(g_ts_kit_platform_data.ts_task)) {
		TS_LOG_ERR("create ts_thread failed\n");
		error = ts_destory_client();
		memset(&g_ts_kit_platform_data, 0,
		       sizeof(struct ts_kit_platform_data));
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
	   to conside. We use barrier to make sure data consistently */
	smp_mb();
	wake_up_process(g_ts_kit_platform_data.ts_task);
	TS_LOG_INFO("ts_init called out\n");
	goto out;
 err_put_platform_dev:
	platform_device_put(g_ts_kit_platform_data.ts_dev);
 err_remove_gpio:
#ifndef CONFIG_HUAWEI_DEVKIT_MTK_3_0
	gpio_free(g_ts_kit_platform_data.irq_gpio);
	if (g_ts_kit_platform_data.reset_gpio) {
		gpio_free(g_ts_kit_platform_data.reset_gpio);
	}
#endif
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

module_init(huawei_ts_module_init);
module_exit(huawei_ts_module_exit);
EXPORT_SYMBOL(g_ts_kit_log_cfg);
EXPORT_SYMBOL(huawei_ts_chip_register);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");
