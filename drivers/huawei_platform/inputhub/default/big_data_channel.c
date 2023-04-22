

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/slab.h>
#include "contexthub_route.h"
#include "sensor_config.h"
#include "contexthub_debug.h"
#include "contexthub_recovery.h"
#include "contexthub_pm.h"
#include "sensor_sysfs.h"
#include "contexthub_ext_log.h"
#include "sensor_detect.h"
#include <huawei_platform/log/hwlog_kernel.h>
#include <huawei_platform/log/imonitor.h>
#include "big_data_channel.h"


/*********** 1.event parameter setting***********/
static big_data_param_detail_t event_motion_type_param[] = {
	{"Pickup", INT_PARAM},
	{"Flip", INT_PARAM},
	{"Proximity", INT_PARAM},
	{"Shake", INT_PARAM},
	{"TiltLr", INT_PARAM},
	{"Pocket", INT_PARAM},
	{"Rotation", INT_PARAM},
	{"Activity", INT_PARAM},
	{"TakeOff", INT_PARAM},
	{"HeadDown", INT_PARAM},
	{"PutDown", INT_PARAM},
	{"Sidegrip", INT_PARAM}
};

static big_data_param_detail_t event_ddr_info_param[] = {
	{"Times", INT_PARAM},
	{"Duration", INT_PARAM}
};

static big_data_param_detail_t event_tof_phonecall_param[] = {
	{"Closest", INT_PARAM},
	{"Farthest", INT_PARAM}
};

/*********** 2.register event param here {EVENT_ID, param_num, param_detail_struct}*****************/
static big_data_event_detail_t big_data_event[] = {
	{BIG_DATA_EVENT_MOTION_TYPE, 12, event_motion_type_param},
	{BIG_DATA_EVENT_DDR_INFO, 2, event_ddr_info_param},
	{BIG_DATA_EVENT_TOF_PHONECALL, 2, event_tof_phonecall_param},
};

/********** 3. (optional)map tag to str**************/
static char *big_data_str_map[] = {
	[BIG_DATA_STR] = "BIG_DATA_STR",
};


static uint64_t iomcu_big_data_fetch(uint32_t event_id)
{
	write_info_t pkg_ap;
	read_info_t pkg_mcu;
	uint64_t *data;
	uint64_t res = 0;
	int ret= -1;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	pkg_ap.tag = TAG_BIG_DATA;
	pkg_ap.cmd = CMD_BIG_DATA_REQUEST_DATA;
	pkg_ap.wr_buf = &event_id;
	pkg_ap.wr_len = sizeof(event_id);

	ret = write_customize_cmd(&pkg_ap, &pkg_mcu, true);

	if (ret != 0)
	{
		hwlog_err("send big data fetch req type = %d fail \n", event_id);
	}else if (pkg_mcu.errno != 0)
	{
		hwlog_err("big data fetch req to mcu fail errno = %d \n", pkg_mcu.errno);
	}else{
		data = (uint64_t*)&pkg_mcu.data;
		hwlog_info("big data fetch get response: type = %d, data = hi: %d , low: %d\n", (uint32_t)event_id, (uint32_t) (*data >> 32), (uint32_t) (*data));
		res = *data;
	}
	return res;
}

uint64_t iomcu_dubai_log_fetch(uint8_t event_type)
{
	uint64_t res = 0;
	if(event_type > DUBAI_EVENT_END || event_type < DUBAI_EVENT_NULL){
		hwlog_err("dubai log fetch event_type: %d illegal!\n", event_type);
		return res;
	}
	res = iomcu_big_data_fetch((uint32_t)event_type);
	return res;
}

static int process_big_data(uint32_t event_id, void* data)
{
	struct imonitor_eventobj* obj = NULL;
	int i = 0;
	int ret = 0;
	uint32_t *raw_data = (uint32_t *)data;
	big_data_event_detail_t event_detail;
	big_data_param_detail_t param_detail;

	if (!data) {
		hwlog_err("%s para error\n", __func__);
		return -1;
	}

	obj = imonitor_create_eventobj(event_id);

	if (!obj) {
		hwlog_err("%s imonitor_create_eventobj failed\n", __func__);
		return -1;
	}
	for (i = 0; i < sizeof(big_data_event) / sizeof(big_data_event[0]); ++i) {
		if (big_data_event[i].event_id == event_id) {
			memcpy(&event_detail, &big_data_event[i], sizeof(event_detail));
			break;
		}
	}

	for (i = 0; i < event_detail.param_num; ++i) {
		memcpy(&param_detail, &event_detail.param_data[i], sizeof(param_detail));
		if (INT_PARAM == param_detail.param_type) {
			ret += imonitor_set_param_integer_v2(obj, param_detail.param_name, raw_data[i]);
		}else if (STR_PARAM == param_detail.param_type) {
			if (big_data_str_map[raw_data[i]] != NULL) {
				ret += imonitor_set_param_string_v2(obj, param_detail.param_name, big_data_str_map[raw_data[i]]);
			}
		}
	}

	if (ret) {
		imonitor_destroy_eventobj(obj);
		hwlog_err("%s imonitor_set_param failed, ret %d\n", __func__, ret);
		return ret;
	}

	ret = imonitor_send_event(obj);
	hwlog_info("big data imonitor send evnet id: %d success\n", (uint32_t)event_id);
	if (ret < 0) {
		hwlog_err("%s imonitor_send_event failed, ret %d\n", __func__, ret);
	}

	imonitor_destroy_eventobj(obj);
	return ret;
}

static int iomcu_big_data_process(const pkt_header_t *head)
{
	uint32_t *data;
	pkt_big_data_report_t* report_t = NULL;
	if(!head)
		return -1;

	report_t = (pkt_big_data_report_t *)head;
	data = (uint32_t *)&report_t[1];
	switch(report_t->event_id){
		case DUBAI_EVENT_AOD_PICKUP:
			hwlog_info("DUBAI_EVENT_AOD_PICKUP: %d\n", data[0]);
			HWDUBAI_LOGE("DUBAI_TAG_TP_AOD", "type=%d data=%d", DUBAI_EVENT_AOD_PICKUP, data[0]);
			break;
		case DUBAI_EVENT_AOD_PICKUP_NO_FINGERDOWN:
			hwlog_info("DUBAI_EVENT_AOD_PICKUP_NO_FINGERDOWN: %d\n", data[0]);
			HWDUBAI_LOGE("DUBAI_TAG_TP_AOD", "type=%d data=%d", DUBAI_EVENT_AOD_PICKUP_NO_FINGERDOWN, data[0]);
			break;
		case BIG_DATA_EVENT_MOTION_TYPE:
		case BIG_DATA_EVENT_DDR_INFO:
		case BIG_DATA_EVENT_TOF_PHONECALL:
			process_big_data(report_t->event_id, data);
			break;

		default:
			hwlog_info("iomcu_big_data_process no matched event id:%d \n",report_t->event_id);
			break;
	}

	return 0;
}

static int iomcu_big_data_init(void)
{
	register_mcu_event_notifier(TAG_BIG_DATA, CMD_BIG_DATA_SEND_TO_AP_RESP, iomcu_big_data_process);
	hwlog_info("iomcu_big_data_init success\n");
	return 0;
}

late_initcall_sync(iomcu_big_data_init);

MODULE_AUTHOR("SensorHub <smartphone@huawei.com>");
MODULE_DESCRIPTION("SensorHub big data channel");
MODULE_LICENSE("GPL");
