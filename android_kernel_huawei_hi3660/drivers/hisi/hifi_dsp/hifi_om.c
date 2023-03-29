/*
 * hifi om.
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/syscalls.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <asm/memory.h>
/*lint -e451*/
#include <asm/types.h>
/*lint +e451*/
#include <asm/io.h>

#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>

#include "audio_hifi.h"
#include "hifi_lpp.h"
#include "hifi_om.h"
#include "drv_mailbox_msg.h"
#include "hisi_rproc.h"
#include "huawei_platform/log/imonitor.h"
#include "huawei_platform/log/imonitor_keys.h"

#include <dsm_audio/dsm_audio.h>
#include <linux/hisi/rdr_pub.h>

/*lint -e773*/
#define HI_DECLARE_SEMAPHORE(name) \
	struct semaphore name = __SEMAPHORE_INITIALIZER(name, 0)
HI_DECLARE_SEMAPHORE(hifi_log_sema);/*lint !e64 !e570 !e651*/
/*lint +e773*/
struct hifi_om_s g_om_data;

#define MAX_LEVEL_STR_LEN 32
#define UNCONFIRM_ADDR (0)
#define UNUSED_PARAMETER(x) (void)(x)

#define BLKMIC_SECONE (17)
#define BLKMIC_SECTWO (32)
#define BLKMIC_SECTHR (64)
#define BLKMIC_SECFOUR (128)
#define HIFI_AP_MESG_CNT (127)
#define IMONITOR_UPDATIME (96400) /*seconds number of one day*/
#define BIGDATA_VOICE_HSEVENTID (916200001)
#define BIGDATA_VOICE_HFEVENTID (916200002)
#define BIGDATA_VOICE_HESEVENTID (916200003)
#define BIGDATA_VOICE_BTEVENTID (916200004)

static struct hifi_dsp_dump_info s_dsp_dump_info[] = {
	{DSP_NORMAL, DUMP_DSP_LOG, FILE_NAME_DUMP_DSP_LOG, UNCONFIRM_ADDR, (DRV_DSP_UART_TO_MEM_SIZE-DRV_DSP_UART_TO_MEM_RESERVE_SIZE)},
	{DSP_NORMAL, DUMP_DSP_BIN, FILE_NAME_DUMP_DSP_BIN, UNCONFIRM_ADDR, HIFI_DUMP_BIN_SIZE},
	{DSP_PANIC,  DUMP_DSP_LOG, FILE_NAME_DUMP_DSP_PANIC_LOG, UNCONFIRM_ADDR, (DRV_DSP_UART_TO_MEM_SIZE-DRV_DSP_UART_TO_MEM_RESERVE_SIZE)},
	{DSP_PANIC,  DUMP_DSP_BIN, FILE_NAME_DUMP_DSP_PANIC_BIN, UNCONFIRM_ADDR, HIFI_DUMP_BIN_SIZE},
};

static struct hifi_effect_info_stru effect_algo[] = {
	{ID_EFFECT_ALGO_FORMATER, "FORMATER"},

	{ID_EFFECT_ALGO_FORTE_VOICE_SPKOUT, "FORTE_VOICE_SPKOUT"},
	{ID_EFFECT_ALGO_FORTE_VOICE_MICIN, "FORTE_VOICE_MICIN"},
	{ID_EFFECT_ALGO_FORTE_VOICE_SPKOUT_BWE, "FORTE_VOICE_SPKOUT_BWE"},

	{ID_EFFECT_ALGO_FORTE_VOIP_MICIN, "FORTE_VOIP_MICIN"},
	{ID_EFFECT_ALGO_FORTE_VOIP_SPKOUT, "FORTE_VOIP_SPKOUT"},

	{ID_EFFECT_ALGO_IN_CONVERT_I2S_GENERAL, "IN_CONVERT_I2S_GENERAL"},
	{ID_EFFECT_ALGO_IN_CONVERT_I2S_HI363X, "IN_CONVERT_I2S_HI363X"},

	{ID_EFFECT_ALGO_INTERLACE, "INTERLACE"},

	{ID_EFFECT_ALGO_OUT_CONVERT_I2S_GENERAL, "OUT_CONVERT_I2S_GENERAL"},
	{ID_EFFECT_ALGO_OUT_CONVERT_I2S_HI363X, "OUT_CONVERT_I2S_HI363X"},

	{ID_EFFECT_ALGO_SWAP, "SWAP"},

	{ID_EFFECT_ALGO_IMEDIA_WNR_MICIN, "IMEDIA_WNR_MICIN"},
	{ID_EFFECT_ALGO_IMEDIA_WNR_SPKOUT, "IMEDIA_WNR_SPKOUT"},

	{ID_EFFECT_ALGO_SWS_INTERFACE, "SWS_INTERFACE"},
	{ID_EFFECT_ALGO_DTS, "DTS"},
	{ID_EFFECT_ALGO_DRE, "DRE"},
	{ID_EFFECT_ALGO_CHC, "CHC"},
	{ID_EFFECT_ALGO_SRC, "SRC"},
	{ID_EFFECT_ALGO_TTY, "TTY"},

	{ID_EFFECT_ALGO_KARAOKE_RECORD, "KARAOKE_RECORD"},
	{ID_EFFECT_ALGO_KARAOKE_PLAY, "KARAOKE_PLAY"},

	{ID_EFFECT_ALGO_MLIB_CS_VOICE_CALL_MICIN, "MLIB_CS_VOICE_CALL_MICIN"},
	{ID_EFFECT_ALGO_MLIB_CS_VOICE_CALL_SPKOUT, "MLIB_CS_VOICE_CALL_SPKOUT"},
	{ID_EFFECT_ALGO_MLIB_VOIP_CALL_MICIN, "MLIB_VOIP_CALL_MICIN"},
	{ID_EFFECT_ALGO_MLIB_VOIP_CALL_SPKOUT, "MLIB_VOIP_CALL_MICIN"},
	{ID_EFFECT_ALGO_MLIB_AUDIO_PLAY, "MLIB_AUDIO_PLAY"},
	{ID_EFFECT_ALGO_MLIB_AUDIO_RECORD, "MLIB_AUDIO_RECORD"},
	{ID_EFFECT_ALGO_MLIB_SIRI_MICIN, "MLIB_SIRI_MICIN"},
	{ID_EFFECT_ALGO_MLIB_SIRI_SPKOUT, "MLIB_SIRI_SPKOUT"},

	{ID_EFFECT_ALGO_EQ, "EQ"},
	{ID_EFFECT_ALGO_MBDRC6402, "MBDRC6402"},

	{ID_EFFECT_ALGO_IMEDIA_VOIP_MICIN, "IMEDIA_VOIP_MICIN"},
	{ID_EFFECT_ALGO_IMEDIA_VOIP_SPKOUT, "IMEDIA_VOIP_SPKOUT"},
	{ID_EFFECT_ALGO_IMEDIA_VOICE_CALL_MICIN, "IMEDIA_VOICE_CALL_MICIN"},
	{ID_EFFECT_ALGO_IMEDIA_VOICE_CALL_SPKOUT, "IMEDIA_VOICE_CALL_SPKOUT"},
	{ID_EFFECT_ALGO_IMEDIA_VOICE_CALL_SPKOUT_BWE, "IMEDIA_VOICE_CALL_SPKOUT_BWE"},
};

static  void hifi_om_voice_bsd_work_handler(struct work_struct *work);
static  void hifi_om_show_audio_detect_info(struct work_struct *work);
static  void hifi_om_show_voice_3a_info(struct work_struct *work);
static  void hifi_om_voice_bigdata_handler(struct work_struct *work);
static  void voice_bigdata_decode(char arrayblock[2][20], char arraybigdata[4][64], unsigned char *data);
static  void voice_bigdata_update_imonitor_inc_blkmic(unsigned int eventID, unsigned short paramid, imedia_voice_bigdata_to_imonitor *voice_bigdata_buff, char *blockmic);
static  void voice_bigdata_update_imonitor(unsigned int eventID, unsigned short paramid, imedia_voice_bigdata_to_imonitor *voice_bigdata_buff);
static struct hifi_om_work_info work_info[] = {
	{HIFI_OM_WORK_VOICE_BSD, "hifi_om_work_voice_bsd", hifi_om_voice_bsd_work_handler, {0}},
	{HIFI_OM_WORK_AUDIO_OM_DETECTION, "hifi_om_work_audio_om_detect", hifi_om_show_audio_detect_info, {0}},
	{HIFI_OM_WORK_VOICE_3A, "hifi_om_work_voice_3a", hifi_om_show_voice_3a_info, {0}},
	{HIFI_OM_WORK_VOICE_BIGDATA, "hifi_om_work_voice_bigdata", hifi_om_voice_bigdata_handler, {0}},
};

static unsigned int dsm_notify_limit = 0x10;

static void hifi_get_time_stamp(char *timestamp_buf, unsigned int len)
{
	struct timeval tv = {0};
	struct rtc_time tm = {0};

	BUG_ON(NULL == timestamp_buf);

	memset(&tv, 0, sizeof(struct timeval));/* unsafe_function_ignore: memset */
	memset(&tm, 0, sizeof(struct rtc_time));/* unsafe_function_ignore: memset */

	do_gettimeofday(&tv);
	tv.tv_sec -= (long)sys_tz.tz_minuteswest * 60;
	rtc_time_to_tm(tv.tv_sec, &tm);

	snprintf(timestamp_buf, len, "%04d%02d%02d%02d%02d%02d",/* unsafe_function_ignore: snprintf */
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);

	return;
}

static int hifi_chown(char *path, uid_t user, gid_t group)
{
	int ret = 0;
	mm_segment_t old_fs;

	if (NULL == path)
		return -1;

	old_fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/

	ret = (int)sys_chown((const char __user *)path, user, group);
	if (ret) {
		loge("chown %s uid [%d] gid [%d] failed error [%d]!\n", path, user, group, ret);
	}

	set_fs(old_fs);

	return ret;
}

static int hifi_create_dir(char *path)
{
	int fd = -1;
	mm_segment_t old_fs = 0;

	if (!path) {
		loge("path(%pK) is invailed\n", path);
		return -1;
	}

	old_fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/

	fd = sys_access(path, 0);
	if (0 != fd) {
		logi("need create dir %s.\n", path);
		fd = sys_mkdir(path, HIFI_OM_DIR_LIMIT);
		if (fd < 0) {
			set_fs(old_fs);
			loge("create dir %s fail, ret: %d.\n", path, fd);
			return fd;
		}
		logi("create dir %s successed, fd: %d.\n", path, fd);

		/* hifi_log dir limit root-system */
		if (hifi_chown(path, ROOT_UID, SYSTEM_GID)) {
			loge("chown %s failed!\n", path);
		}
	}

	set_fs(old_fs);

	return 0;
}

static int hifi_om_create_log_dir(char *path)
{
	char cur_path[HIFI_DUMP_FILE_NAME_MAX_LEN];
	int index = 0;

	if (!path || (strlen(path) + 1) > HIFI_DUMP_FILE_NAME_MAX_LEN) {
		loge("path(%pK) is invailed\n", path);
		return -1;
	}

	if (0 == sys_access(path, 0))
		return 0;

	memset(cur_path, 0, HIFI_DUMP_FILE_NAME_MAX_LEN);/* unsafe_function_ignore: memset */

	if (*path != '/')
		return -1;

	cur_path[index++] = *path++;

	while (*path != '\0') {
		if (*path == '/') {
			if (hifi_create_dir(cur_path)) {
				loge("create dir %s failed\n", cur_path);
				return -1;
			}
		}
		cur_path[index++] = *path++;
	}

	return 0;

}

int hifi_om_get_voice_bsd_param(void __user * uaddr)
{
	int work_id = HIFI_OM_WORK_VOICE_BSD;
	struct hifi_om_work *om_work = NULL;
	unsigned char data[MAIL_LEN_MAX] = {'\0'};
	unsigned int data_len = 0;
	struct voice_bsd_param_hsm param;

	memset(&param, 0, sizeof(param));/* unsafe_function_ignore: memset */
	if (try_copy_from_user(&param, uaddr, sizeof(param))) {
		loge("copy_from_user failed\n");
		return -EFAULT;
	}

	if (!param.pdata) {
		loge("user buffer is null\n");
		return -EINVAL;
	}

	spin_lock_bh(&work_info[work_id].ctl.lock);
	if (!list_empty(&work_info[work_id].ctl.list)) {
		om_work = list_entry(work_info[work_id].ctl.list.next, struct hifi_om_work, om_node);

		data_len = om_work->data_len;
		memcpy(data, om_work->data, om_work->data_len);/* unsafe_function_ignore: memcpy */

		list_del(&om_work->om_node);
		kzfree(om_work);
	} else {
		spin_unlock_bh(&work_info[work_id].ctl.lock);
		return -EAGAIN;
	}
	spin_unlock_bh(&work_info[work_id].ctl.lock);

	if (param.data_len < data_len) {
		loge("userspace len(%u) is less than data_len(%u)\n", param.data_len, data_len);
		return -EINVAL;
	}

	if (try_copy_to_user((void __user *)param.pdata, data, data_len)) {
		loge("copy_to_user failed\n");
		return -EFAULT;
	}
	logd("size(%u)copy to user success\n", data_len);

	return 0;
}

static void hifi_om_voice_bsd_work_handler(struct work_struct *work)
{
	int retval = 0;
	char *envp[2] = {"hifi_voice_bsd_param", NULL};

	retval = kobject_uevent_env(&g_om_data.dev->kobj, KOBJ_CHANGE, envp);
	if (retval) {
		loge("send uevent failed, retval: %d\n", retval);
		return;
	}
	logi("report uevent success\n");

	return;
}
/*lint -e429*/
void hifi_om_rev_data_handle(int work_id, const unsigned char *addr, unsigned int len)
{
	struct hifi_om_work *work = NULL;

	if (!addr || 0 == len || len > MAIL_LEN_MAX) {
		loge("addr is null or len is invaled, len: %u", len);
		return;
	}

	work = kzalloc(sizeof(*work) + len, GFP_ATOMIC);
	if (!work) {
		loge("malloc size %zu failed\n", sizeof(*work) + len);
		return;
	}
	memcpy(work->data, addr, len);/* unsafe_function_ignore: memcpy */
	work->data_len = len;

	spin_lock_bh(&work_info[work_id].ctl.lock);
	list_add_tail(&work->om_node, &work_info[work_id].ctl.list);
	spin_unlock_bh(&work_info[work_id].ctl.lock);

	if (!queue_work(work_info[work_id].ctl.wq, &work_info[work_id].ctl.work)) {
		loge("work_id: %d, This work was already on the queue\n", work_id);
	}

	return;
}
/*lint +e429*/
static void hifi_om_show_audio_detect_info(struct work_struct *work)
{
	int work_id = HIFI_OM_WORK_AUDIO_OM_DETECTION;
	struct hifi_om_work *om_work = NULL;
	unsigned char data[MAIL_LEN_MAX] = {'\0'};
	unsigned int data_len = 0;
	unsigned int hifi_msg_type = 0;
	struct hifi_om_load_info_stru hifi_om_info;
	struct hifi_om_effect_mcps_stru mcps_info;
	struct hifi_om_update_buff_delay_info update_buff_delay_info;

	spin_lock_bh(&work_info[work_id].ctl.lock);
	if (!list_empty(&work_info[work_id].ctl.list)) {
		om_work = list_entry(work_info[work_id].ctl.list.next, struct hifi_om_work, om_node);

		data_len = om_work->data_len;
		memcpy(data, om_work->data, om_work->data_len);/* unsafe_function_ignore: memcpy */

		list_del(&om_work->om_node);
		kzfree(om_work);
	}
	spin_unlock_bh(&work_info[work_id].ctl.lock);

	memset(&hifi_om_info, 0, sizeof(hifi_om_info));/* unsafe_function_ignore: memset */
	memset(&mcps_info, 0, sizeof(mcps_info));/* unsafe_function_ignore: memset */
	memset(&update_buff_delay_info, 0, sizeof(update_buff_delay_info));/* unsafe_function_ignore: memset */

	hifi_msg_type = *(unsigned int *)data;

	switch (hifi_msg_type) {
	case HIFI_CPU_OM_LOAD_INFO:
		if ((sizeof(hifi_om_info)) != data_len) {
			logw("unavailable data from hifi, data_len: %u\n", data_len);
			return;
		}
		memcpy(&hifi_om_info, data, sizeof(hifi_om_info));/* unsafe_function_ignore: memcpy */

		hifi_om_cpu_load_info_show(&hifi_om_info);
		break;
	case HIFI_CPU_OM_ALGO_MCPS_INFO:
		if ((sizeof(mcps_info)) != data_len) {
			logw("unavailable data from hifi, data_len: %u\n", data_len);
			return;
		}
		memcpy(&mcps_info, data, sizeof(mcps_info));/* unsafe_function_ignore: memcpy */

		hifi_om_effect_mcps_info_show(&mcps_info);
		break;
	case HIFI_CPU_OM_UPDATE_BUFF_DELAY_INFO:
		if ((sizeof(update_buff_delay_info)) != data_len) {
			logw("unavailable data from hifi, data_len: %u\n", data_len);
			return;
		}
		memcpy(&update_buff_delay_info, data, sizeof(update_buff_delay_info));/* unsafe_function_ignore: memcpy */

		hifi_om_update_buff_delay_info_show(&update_buff_delay_info);
		break;
	default:
		logi("type(%d), not support\n", hifi_msg_type);
		break;
	}

	return;
}

static void hifi_om_show_voice_3a_info(struct work_struct *work)
{
	int work_id = HIFI_OM_WORK_VOICE_3A;
	struct hifi_om_work *om_work = NULL;
	unsigned char data[MAIL_LEN_MAX] = {'\0'};
	unsigned int data_len = 0;
	unsigned int hifi_msg_type = 0;
	struct voice_3a_om_stru  voice_3a_om_info;
	UNUSED_PARAMETER(work);

	spin_lock_bh(&work_info[work_id].ctl.lock);
	if (!list_empty(&work_info[work_id].ctl.list)) {
		om_work = list_entry(work_info[work_id].ctl.list.next, struct hifi_om_work, om_node);/*lint !e826*/

		data_len = om_work->data_len;
		memcpy(data, om_work->data, om_work->data_len);/*lint !e747*/ /* unsafe_function_ignore: memcpy */

		list_del(&om_work->om_node);
		kzfree(om_work);
	}
	spin_unlock_bh(&work_info[work_id].ctl.lock);

	hifi_msg_type = *(unsigned int *)data;/*lint !e838*/

	switch (hifi_msg_type) {
	case HIFI_3A_INFO_MSG:
		if ((sizeof(voice_3a_om_info)) != data_len) {
			logw("unavailable data from hifi, data_len: %u\n", data_len);
			return;
		}
		memcpy(&voice_3a_om_info, data, sizeof(voice_3a_om_info));/* unsafe_function_ignore: memcpy */
		audio_dsm_report_info(AUDIO_CODEC, DSM_SOC_HIFI_3A_ERROR, "3a error type:%d error:%d\n", hifi_msg_type, voice_3a_om_info.recv_msg);
		break;
	default:
		logi("type(%d), not support\n", hifi_msg_type);
		break;
	}

	return;
}

static  void hifi_om_voice_bigdata_handler(struct work_struct *work)
{
	int work_id = HIFI_OM_WORK_VOICE_BIGDATA;
	struct hifi_om_work *om_work = NULL;
	unsigned char data[MAIL_LEN_MAX] = {'\0'};
	unsigned int data_len = 0;
	static char voice_bigdata[4][64] = {{0},{0},{0},{0}};
	static char voice_bigdata_miccheck[2][20] = {{0},{0}};
	static struct timespec bigdata_time1 = {0, 0};
	static struct timespec bigdata_time2 = {0, 0};
	static int hifi_ap_count = 0;

	spin_lock_bh(&work_info[work_id].ctl.lock);
	if (!list_empty(&work_info[work_id].ctl.list)) {
		om_work = list_entry(work_info[work_id].ctl.list.next, struct hifi_om_work, om_node);/*lint !e826*/
		data_len = om_work->data_len;
		memcpy(data, om_work->data, om_work->data_len);/*lint !e747*/ /* unsafe_function_ignore: memcpy */
		list_del(&om_work->om_node);
		kzfree(om_work);
	}
	spin_unlock_bh(&work_info[work_id].ctl.lock);

	if ((sizeof(imedia_voice_bigdata)) != data_len) {
		logw("unavailable data from hifi, data_len: %u\n", data_len);
		return;
	}

	hifi_ap_count++;
	bigdata_time2 = current_kernel_time();
	if (hifi_ap_count < HIFI_AP_MESG_CNT) {
		voice_bigdata_decode(voice_bigdata_miccheck, voice_bigdata, data);
	}

	if (IMONITOR_UPDATIME < (bigdata_time2.tv_sec - bigdata_time1.tv_sec)) {
		imedia_voice_bigdata_to_imonitor *voice_bigdata_buff = NULL;
		char *blockmic = NULL;

		/*carry data from kernel to imonitor,handset mode*/
		voice_bigdata_buff = (imedia_voice_bigdata_to_imonitor*)voice_bigdata[0];
		blockmic = (char*)voice_bigdata_miccheck[0];
		voice_bigdata_update_imonitor_inc_blkmic(BIGDATA_VOICE_HSEVENTID, E916200001_NOISECNT0_TINYINT, voice_bigdata_buff, blockmic);

		/*carry data from kernel to imonitor,handfree mode*/
		voice_bigdata_buff = (imedia_voice_bigdata_to_imonitor*)voice_bigdata[1];
		blockmic = (char*)voice_bigdata_miccheck[1];
		voice_bigdata_update_imonitor_inc_blkmic(BIGDATA_VOICE_HFEVENTID, E916200002_NOISECNT0_TINYINT, voice_bigdata_buff, blockmic);

		/*carry data from kernel to imonitor,headset mode*/
		voice_bigdata_buff = (imedia_voice_bigdata_to_imonitor*)voice_bigdata[2];
		voice_bigdata_update_imonitor(BIGDATA_VOICE_HESEVENTID, E916200003_NOISECNT0_TINYINT, voice_bigdata_buff);

		/*carry data from kernel to imonitor,bluetooth mode*/
		voice_bigdata_buff = (imedia_voice_bigdata_to_imonitor*)voice_bigdata[3];
		voice_bigdata_update_imonitor(BIGDATA_VOICE_BTEVENTID, E916200004_NOISECNT0_TINYINT, voice_bigdata_buff);

		memset(voice_bigdata, 0, sizeof(voice_bigdata));/* unsafe_function_ignore: memset */
		memset(voice_bigdata_miccheck, 0, sizeof(voice_bigdata_miccheck));/* unsafe_function_ignore: memset */
		bigdata_time1 = current_kernel_time();
		hifi_ap_count = 0;
	}
}

/*carry data from kernel to imonitor,for handset mode and handfree mode*/
static  void voice_bigdata_update_imonitor_inc_blkmic(unsigned int eventID, unsigned short paramid, imedia_voice_bigdata_to_imonitor *voice_bigdata_buff, char *blockmic)
{
	int i, j;
	int blockmic_size = E916200001_BLOCKMICCNT19_TINYINT - E916200001_BLOCKMICCNT0_TINYINT + 1;
	struct imonitor_eventobj *voice_bigdata_obj;

	if ((NULL == voice_bigdata_buff) || (NULL == blockmic) || ((E916200001_NOISECNT0_TINYINT != paramid) && (E916200002_NOISECNT0_TINYINT != paramid))) {
		logw(" imonitor data from kernel is empty \n");
		return;
	}

	/*creat imonitor obj*/
	voice_bigdata_obj = imonitor_create_eventobj(eventID);

	if (NULL == voice_bigdata_obj) {
		logw(" imonitor obj create fail \n");
		return;
	}

	/*carry noise and voice data from kerenl to imonitor*/
	for (i = paramid; i < VOICE_BIGDATA_NOISESIZE; i++) {
		imonitor_set_param_integer(voice_bigdata_obj, i, voice_bigdata_buff->noise[i]);
		imonitor_set_param_integer(voice_bigdata_obj, i + VOICE_BIGDATA_NOISESIZE, voice_bigdata_buff->voice[i]);
	}

	/*carry blockmic data from kerenl to imonitor*/
	for (i = paramid + (VOICE_BIGDATA_NOISESIZE + VOICE_BIGDATA_VOICESIZE); i < (VOICE_BIGDATA_NOISESIZE + VOICE_BIGDATA_VOICESIZE + blockmic_size); i++) {
		imonitor_set_param_integer(voice_bigdata_obj, i, *blockmic);
		blockmic++;
	}

	/*carry charact data from kerenl to imonitor*/
	for (i = 0; i < VOICE_BIGDATA_CHARACTSIZE; i++) {
		j = (VOICE_BIGDATA_CHARACTSIZE - 1) - i;
		imonitor_set_param_integer(voice_bigdata_obj, VOICE_BIGDATA_NOISESIZE + VOICE_BIGDATA_VOICESIZE + blockmic_size + i, voice_bigdata_buff->charact[j]);
	}

	imonitor_send_event(voice_bigdata_obj);
	imonitor_destroy_eventobj(voice_bigdata_obj);
}

/*carry data from kernel to imonitor,for headset mode and bluetooth mode*/
static  void voice_bigdata_update_imonitor(unsigned int eventID, unsigned short paramid, imedia_voice_bigdata_to_imonitor *voice_bigdata_buff)
{
	int i, j;
	struct imonitor_eventobj *voice_bigdata_obj;

	if ((NULL == voice_bigdata_buff) || ((E916200003_NOISECNT0_TINYINT != paramid) && (E916200004_NOISECNT0_TINYINT != paramid))) {
		logw(" imonitor data from kernel is empty \n");
		return;
	}

	/*creat imonitor obj*/
	voice_bigdata_obj = imonitor_create_eventobj(eventID);

	if (NULL == voice_bigdata_obj) {
		logw(" imonitor obj create fail \n");
		return;
	}

	/*carry noise and voice data from kerenl to imonitor*/
	for (i = paramid; i < VOICE_BIGDATA_NOISESIZE; i++) {
		imonitor_set_param_integer(voice_bigdata_obj, i, voice_bigdata_buff->noise[i]);
		imonitor_set_param_integer(voice_bigdata_obj, i + VOICE_BIGDATA_NOISESIZE, voice_bigdata_buff->voice[i]);
	}

	/*carry charact data from kerenl to imonitor*/
	for (i = 0; i < VOICE_BIGDATA_CHARACTSIZE; i++) {
		j = (VOICE_BIGDATA_CHARACTSIZE - 1) - i;
		imonitor_set_param_integer(voice_bigdata_obj, VOICE_BIGDATA_NOISESIZE + VOICE_BIGDATA_VOICESIZE + i, voice_bigdata_buff->charact[j]);
	}
	imonitor_send_event(voice_bigdata_obj);
	imonitor_destroy_eventobj(voice_bigdata_obj);
}

static  void voice_bigdata_blockmic (char array[2][20], unsigned char device, unsigned char data)
{
	unsigned char micdata;
	micdata = data;
	if (device > 1) {
		logw("unavailable bigdata blockmic data\n");
		return;
	}
	if ((micdata > 0) && (micdata < BLKMIC_SECONE)) {
		array[device][micdata-1]++;
	}else if ((micdata >= BLKMIC_SECONE) && (micdata < BLKMIC_SECTWO)) {
		array[device][16]++;
	}else if ((micdata >= BLKMIC_SECTWO) && (micdata < BLKMIC_SECTHR)) {
		array[device][17]++;
	}else if ((micdata >= BLKMIC_SECTHR) && (micdata < BLKMIC_SECFOUR)) {
		array[device][18]++;
	}else if (micdata >= BLKMIC_SECFOUR) {
		array[device][19]++;
	}else {
		;
	}
}

static  void voice_bigdata_voicecharact(char array[4][64], unsigned char device, unsigned int data)
{
	unsigned int shi_var;
	unsigned int datatran;
	unsigned int index;
	int i;
	if (device > 3) {
		logw("unavailable bigdata voicecharact data\n");
		return;
	}
	datatran = data;
	shi_var = 0x80000000;
	for (i = 32; i < 64;i++) {
		index = !!(datatran&shi_var);
		array[device][i] = array[device][i] + index;
		shi_var = shi_var>>1;
	}
}

static  void voice_bigdata_decode(char arrayblock[2][20], char arraybigdata[4][64], unsigned char * data)
{
	imedia_voice_bigdata * bigdata_mesg = NULL;

	if (NULL == data) {
		logw(" data from hifi is empty \n");
		return;
	}
	bigdata_mesg = (imedia_voice_bigdata*)data;
	switch (bigdata_mesg->iMedia_Voice_bigdata_device) {
		case MLIB_DEVICE_HANDSET:
			arraybigdata[0][bigdata_mesg->iMedia_Voice_bigdata_noise]++;
			arraybigdata[0][bigdata_mesg->iMedia_Voice_bigdata_voice+16]++;
			voice_bigdata_voicecharact(arraybigdata, bigdata_mesg->iMedia_Voice_bigdata_device, bigdata_mesg->iMedia_Voice_bigdata_charact);
			voice_bigdata_blockmic (arrayblock, bigdata_mesg->iMedia_Voice_bigdata_device, bigdata_mesg->iMedia_Voice_bigdata_miccheck);
			break;
		case MLIB_DEVICE_HANDFREE:
			arraybigdata[1][bigdata_mesg->iMedia_Voice_bigdata_noise]++;
			arraybigdata[1][bigdata_mesg->iMedia_Voice_bigdata_voice+16]++;
			voice_bigdata_voicecharact(arraybigdata, bigdata_mesg->iMedia_Voice_bigdata_device, bigdata_mesg->iMedia_Voice_bigdata_charact);
			voice_bigdata_blockmic (arrayblock, bigdata_mesg->iMedia_Voice_bigdata_device, bigdata_mesg->iMedia_Voice_bigdata_miccheck);
			break;
		case MLIB_DEVICE_USBHEADSET:
		case MLIB_DEVICE_HEADSET:
			bigdata_mesg->iMedia_Voice_bigdata_device = MLIB_DEVICE_HEADSET;
			arraybigdata[2][bigdata_mesg->iMedia_Voice_bigdata_noise]++;
			arraybigdata[2][bigdata_mesg->iMedia_Voice_bigdata_voice+16]++;
			bigdata_mesg->iMedia_Voice_bigdata_device = bigdata_mesg->iMedia_Voice_bigdata_device - 1;
			voice_bigdata_voicecharact(arraybigdata, bigdata_mesg->iMedia_Voice_bigdata_device, bigdata_mesg->iMedia_Voice_bigdata_charact);
			break;
		case MLIB_DEVICE_BLUETOOTH:
			arraybigdata[3][bigdata_mesg->iMedia_Voice_bigdata_noise]++;
			arraybigdata[3][bigdata_mesg->iMedia_Voice_bigdata_voice+16]++;
			bigdata_mesg->iMedia_Voice_bigdata_device = bigdata_mesg->iMedia_Voice_bigdata_device - 1;
			voice_bigdata_voicecharact(arraybigdata, bigdata_mesg->iMedia_Voice_bigdata_device, bigdata_mesg->iMedia_Voice_bigdata_charact);
			break;
		default:
			break;
		}
}
static void hifi_dump_dsp(DUMP_DSP_INDEX index)
{
	int ret = 0;

	mm_segment_t fs = 0;
	struct file *fp = NULL;
	int file_flag = O_RDWR;
	struct kstat file_stat;
	int write_size = 0;
	unsigned int err_no = 0xFFFFFFFF;

	char tmp_buf[64] = {0};
	unsigned long tmp_len = 0;
	struct rtc_time cur_tm;
	struct timespec now;

	char  path_name[HIFI_DUMP_FILE_NAME_MAX_LEN] = {0};
	char* file_name		= s_dsp_dump_info[index].file_name;
	char* data_addr		= NULL;
	unsigned int data_len = s_dsp_dump_info[index].data_len;

	char* is_panic		= "i'm panic.\n";
	char* is_exception	= "i'm exception.\n";
	char* not_panic		= "i'm ok.\n";

	memset(path_name, 0, HIFI_DUMP_FILE_NAME_MAX_LEN);/* unsafe_function_ignore: memset */

	if (down_interruptible(&g_om_data.dsp_dump_sema) < 0) {
		loge("acquire the semaphore error.\n");
		return;
	}

	IN_FUNCTION;

	hifi_get_log_signal();

	s_dsp_dump_info[NORMAL_LOG].data_addr = g_om_data.dsp_log_addr + DRV_DSP_UART_TO_MEM_RESERVE_SIZE;
	s_dsp_dump_info[PANIC_LOG].data_addr  = g_om_data.dsp_log_addr + DRV_DSP_UART_TO_MEM_RESERVE_SIZE;

	if (NULL == s_dsp_dump_info[index].data_addr) {
		loge("dsp log ioremap fail.\n");
		goto END;
	}

	data_addr = s_dsp_dump_info[index].data_addr;

	fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/

	ret = hifi_om_create_log_dir(LOG_PATH_HIFI_LOG);
	if (0 != ret) {
		goto END;
	}

	snprintf(path_name, HIFI_DUMP_FILE_NAME_MAX_LEN, "%s%s", LOG_PATH_HIFI_LOG, file_name);/* unsafe_function_ignore: snprintf */

	ret = vfs_stat(path_name, &file_stat);
	if (ret < 0) {
		logi("there isn't a dsp log file:%s, and need to create.\n", path_name);
		file_flag |= O_CREAT;
	}

	fp = filp_open(path_name, file_flag, HIFI_OM_FILE_LIMIT);
	if (IS_ERR(fp)) {
		loge("open file fail: %s.\n", path_name);
		fp = NULL;
		goto END;
	}

	/*write from file start*/
	vfs_llseek(fp, 0, SEEK_SET);

	/*write file head*/
	if (DUMP_DSP_LOG == s_dsp_dump_info[index].dump_type) {
		/*write dump log time*/
		now = current_kernel_time();
		rtc_time_to_tm(now.tv_sec, &cur_tm);

		memset(tmp_buf, 0, sizeof(tmp_buf));/* unsafe_function_ignore: memset */
		tmp_len = snprintf(tmp_buf, sizeof(tmp_buf), "%04d-%02d-%02d %02d:%02d:%02d.\n",/* unsafe_function_ignore: snprintf */
								cur_tm.tm_year+1900, cur_tm.tm_mon+1,
								cur_tm.tm_mday, cur_tm.tm_hour,
								cur_tm.tm_min, cur_tm.tm_sec);
		vfs_write(fp, tmp_buf, tmp_len, &fp->f_pos);/*lint !e613*/

		/*write exception no*/
		memset(tmp_buf, 0, sizeof(tmp_buf));/* unsafe_function_ignore: memset */
		err_no = (unsigned int)(*(g_om_data.dsp_exception_no));
		if (err_no != 0xFFFFFFFF) {
			tmp_len = snprintf(tmp_buf, sizeof(tmp_buf), "the exception no: %u.\n", err_no);/* unsafe_function_ignore: snprintf */
		} else {
			tmp_len = snprintf(tmp_buf, sizeof(tmp_buf), "%s", "hifi is fine, just dump log.\n");/* unsafe_function_ignore: snprintf */
		}

		vfs_write(fp, tmp_buf, tmp_len, &fp->f_pos);/*lint !e613*/

		/*write error type*/
		if (0xdeadbeaf == *g_om_data.dsp_panic_mark) {
			vfs_write(fp, is_panic, strlen(is_panic), &fp->f_pos);/*lint !e613*/
		} else if(0xbeafdead == *g_om_data.dsp_panic_mark){
			vfs_write(fp, is_exception, strlen(is_exception), &fp->f_pos);/*lint !e613*/
		} else {
			vfs_write(fp, not_panic, strlen(not_panic), &fp->f_pos);/*lint !e613*/
		}
	}

	/*write dsp info*/
	if((write_size = vfs_write(fp, data_addr, data_len, &fp->f_pos)) < 0) {/*lint !e613*/
		loge("write file fail.\n");
	}

	/* hifi.log file limit root-system */
	if (hifi_chown(path_name, ROOT_UID, SYSTEM_GID)) {
		loge("chown %s failed!\n", path_name);
	}

	logi("write file size: %d.\n", write_size);

END:
	if (fp) {
		filp_close(fp, 0);
	}
	set_fs(fs);

	hifi_release_log_signal();

	up(&g_om_data.dsp_dump_sema);
	OUT_FUNCTION;

	return;
}

static void hifi_set_dsp_debug_level(unsigned int level)
{
	*(unsigned int*)g_om_data.dsp_debug_level_addr = level;
}


static void hifi_create_procfs(void)
{
}

static void hifi_remove_procfs(void)
{
}

static int hifi_dump_dsp_thread(void *p)
{
	#define HIFI_TIME_STAMP_1S	  32768
	#define HIFI_DUMPLOG_TIMESPAN (10 * HIFI_TIME_STAMP_1S)

	unsigned int exception_no = 0;
	unsigned int time_now = 0;
	unsigned int time_diff = 0;
	unsigned int* hifi_info_addr = NULL;
	unsigned int hifi_stack_addr = 0;
	unsigned int i;

	IN_FUNCTION;

	while (!kthread_should_stop()) {
		if (down_interruptible(&hifi_log_sema) != 0) {
			loge("hifi_dump_dsp_thread wake up err.\n");
		}
		/*Do not create the /data/hisi_logs/running_trace/hifi_log/ folder*/
		/*and files within when not in internal beta phase*/
		if (EDITION_INTERNAL_BETA != bbox_check_edition()) {
			loge("Not beta, Do not dump hifi\n");
			continue;
		}
		time_now = (unsigned int)readl(g_om_data.dsp_time_stamp);
		time_diff = time_now - g_om_data.pre_dsp_dump_timestamp;
		g_om_data.pre_dsp_dump_timestamp = time_now;
		hifi_info_addr = g_om_data.dsp_stack_addr;

		exception_no = *(unsigned int*)(hifi_info_addr + 3);
		hifi_stack_addr = *(unsigned int*)(hifi_info_addr + 4);
		logi("errno:%x pre_errno:%x is_first:%d is_force:%d time_diff:%d ms.\n", exception_no, g_om_data.pre_exception_no, g_om_data.first_dump_log, g_om_data.force_dump_log, (time_diff * 1000) / HIFI_TIME_STAMP_1S);

		hifi_get_time_stamp(g_om_data.cur_dump_time, HIFI_DUMP_FILE_NAME_MAX_LEN);

		if (exception_no < 40 && (exception_no != g_om_data.pre_exception_no)) {
			logi("panic addr:0x%pK, cur_pc:0x%pK, pre_pc:0x%pK, cause:0x%x\n", (void *)(unsigned long)(*hifi_info_addr), (void *)(unsigned long)(*(hifi_info_addr+1)), (void *)(unsigned long)(*(hifi_info_addr+2)), *(unsigned int*)(unsigned long)(hifi_info_addr+3));
			for( i = 0; i < (DRV_DSP_STACK_TO_MEM_SIZE/2)/sizeof(int)/4; i+=4){
				logi("0x%pK: 0x%pK 0x%pK 0x%pK 0x%pK\n", (void *)(long)(unsigned)(hifi_stack_addr+i*4), (void *)(unsigned long)(*(hifi_info_addr+i)),(void *)(unsigned long)(*(hifi_info_addr+1+i)),(void *)(unsigned long)(*(hifi_info_addr+2+i)),(void *)(unsigned long)(*(hifi_info_addr+i+3)));
			}

			hifi_dump_dsp(PANIC_LOG);
			hifi_dump_dsp(PANIC_BIN);

			g_om_data.pre_exception_no = exception_no;
		} else if (g_om_data.first_dump_log || g_om_data.force_dump_log || time_diff > HIFI_DUMPLOG_TIMESPAN) {
			hifi_dump_dsp(NORMAL_LOG);
			if (DSP_LOG_BUF_FULL != g_om_data.dsp_error_type) {/*needn't dump bin when hifi log buffer full*/
				hifi_dump_dsp(NORMAL_BIN);
			}
			g_om_data.first_dump_log = false;
		}

		hifi_info_addr = NULL;
	}
	OUT_FUNCTION;
	return 0;
}

void hifi_dump_panic_log(void)
{
	if (!g_om_data.dsp_loaded) {
		loge("hifi isn't loaded, errno: 0x%x .\n" , g_om_data.dsp_loaded_sign);
		return;
	}
	up(&hifi_log_sema);
	return;
}

static bool hifi_check_img_loaded(void)
{
	bool dsp_loaded = false;
	g_om_data.dsp_loaded_sign = *(g_om_data.dsp_loaded_indicate_addr);

	if (0xA5A55A5A == g_om_data.dsp_loaded_sign) {
		loge("hifi img is not be loaded.\n");
	} else if (g_om_data.dsp_loaded_sign > 0) {
		loge("hifi img is loaded fail: 0x%x.\n", g_om_data.dsp_loaded_sign);
	} else {
		logi("hifi img be loaded.\n");
		dsp_loaded = true;
	}

	return dsp_loaded;
}

bool is_hifi_loaded(void)
{
	if (!g_om_data.dsp_loaded) {
		loge("hifi isn't load, errno is 0x%x.\n", g_om_data.dsp_loaded_sign);
	}

	return g_om_data.dsp_loaded;
}

int hifi_dsp_dump_hifi(void __user *arg)
{
	unsigned int err_type = 0;

	if (!arg) {
		loge("arg is null\n");
		return -1;
	}

	if (try_copy_from_user(&err_type, arg, sizeof(err_type))) {
		loge("copy_from_user fail, don't dump log\n");
		return -1;
	}
	g_om_data.dsp_error_type = err_type;/*lint !e64*/
	g_om_data.force_dump_log = true;
	up(&hifi_log_sema);

	return 0;
}

void hifi_om_init(struct platform_device *pdev, unsigned char* hifi_priv_base_virt, unsigned char* hifi_priv_base_phy)
{
	unsigned int i = 0;
	BUG_ON(NULL == pdev);

	BUG_ON(NULL == hifi_priv_base_virt);
	BUG_ON(NULL == hifi_priv_base_phy);

	memset(&g_om_data, 0, sizeof(struct hifi_om_s));/* unsafe_function_ignore: memset */

	g_om_data.dev = &pdev->dev;

	if (hifi_misc_get_platform_type() == HIFI_DSP_PLATFORM_FPGA)
		g_om_data.debug_level = 0; /*err level*/
	else
		g_om_data.debug_level = 2; /*info level*/

	g_om_data.reset_system = false;

	g_om_data.dsp_time_stamp = (unsigned int*)ioremap(SYS_TIME_STAMP_REG, 0x4);
	if (NULL == g_om_data.dsp_time_stamp) {
		printk("time stamp reg ioremap Error.\n");//can't use logx
		return;
	}

	IN_FUNCTION;

	g_om_data.dsp_debug_level = 2; /*info level*/
	g_om_data.first_dump_log = true;

	g_om_data.dsp_panic_mark = (unsigned int*)(hifi_priv_base_virt + (DRV_DSP_PANIC_MARK - HIFI_UNSEC_BASE_ADDR));
	g_om_data.dsp_bin_addr = (char*)(hifi_priv_base_virt + (HIFI_DUMP_BIN_ADDR - HIFI_UNSEC_BASE_ADDR));
	g_om_data.dsp_exception_no = (unsigned int*)(hifi_priv_base_virt + (DRV_DSP_EXCEPTION_NO - HIFI_UNSEC_BASE_ADDR));
	g_om_data.dsp_log_cur_addr = (unsigned int*)(hifi_priv_base_virt + (DRV_DSP_UART_TO_MEM_CUR_ADDR - HIFI_UNSEC_BASE_ADDR));
	g_om_data.dsp_log_addr = (char *)(hifi_priv_base_virt + (DRV_DSP_UART_TO_MEM - HIFI_UNSEC_BASE_ADDR));
	memset(g_om_data.dsp_log_addr, 0, DRV_DSP_UART_TO_MEM_SIZE);/* unsafe_function_ignore: memset */
	*g_om_data.dsp_log_cur_addr = DRV_DSP_UART_TO_MEM_RESERVE_SIZE;

	g_om_data.dsp_debug_level_addr = (unsigned int*)(hifi_priv_base_virt + (DRV_DSP_UART_LOG_LEVEL - HIFI_UNSEC_BASE_ADDR));
	g_om_data.dsp_debug_kill_addr = (unsigned int*)(hifi_priv_base_virt + (DRV_DSP_KILLME_ADDR - HIFI_UNSEC_BASE_ADDR));
	g_om_data.dsp_fama_config = (struct drv_fama_config *)(hifi_priv_base_virt + (DRV_DSP_SOCP_FAMA_CONFIG_ADDR - HIFI_UNSEC_BASE_ADDR));
	g_om_data.dsp_stack_addr = (unsigned int*)(hifi_priv_base_virt + (DRV_DSP_STACK_TO_MEM - HIFI_UNSEC_BASE_ADDR));
	g_om_data.dsp_loaded_indicate_addr = (unsigned int*)(hifi_priv_base_virt + (DRV_DSP_LOADED_INDICATE - HIFI_UNSEC_BASE_ADDR));


	*(g_om_data.dsp_exception_no) = ~0;
	g_om_data.pre_exception_no = ~0;
	g_om_data.dsp_fama_config->head_magic = DRV_DSP_SOCP_FAMA_HEAD_MAGIC;
	g_om_data.dsp_fama_config->flag = DRV_DSP_FAMA_OFF;
	g_om_data.dsp_fama_config->rear_magic = DRV_DSP_SOCP_FAMA_REAR_MAGIC;

	s_dsp_dump_info[NORMAL_BIN].data_addr = g_om_data.dsp_bin_addr;
	s_dsp_dump_info[PANIC_BIN].data_addr  = g_om_data.dsp_bin_addr;

	g_om_data.dsp_loaded = hifi_check_img_loaded();
	hifi_set_dsp_debug_level(g_om_data.dsp_debug_level);

	sema_init(&g_om_data.dsp_dump_sema, 1);

	g_om_data.kdumpdsp_task = kthread_create(hifi_dump_dsp_thread, 0, "dspdumplog");
	if (IS_ERR(g_om_data.kdumpdsp_task)) {
		loge("creat hifi dump log thread fail.\n");
	} else {
		wake_up_process(g_om_data.kdumpdsp_task);
	}

	hifi_create_procfs();

	for (i = 0; i < ARRAY_SIZE(work_info); i++) {
		work_info[i].ctl.wq = create_singlethread_workqueue(work_info[i].work_name);
		if (!work_info[i].ctl.wq) {
			pr_err("%s(%u):workqueue create failed!\n", __FUNCTION__, __LINE__);
		} else {
			INIT_WORK(&work_info[i].ctl.work, work_info[i].func);
			spin_lock_init(&work_info[i].ctl.lock);
			INIT_LIST_HEAD(&work_info[i].ctl.list);
		}
	}

	OUT_FUNCTION;
	return;
}

void hifi_om_deinit(struct platform_device *dev)
{
	unsigned int i = 0;

	IN_FUNCTION;

	BUG_ON(NULL == dev);

	up(&g_om_data.dsp_dump_sema);
	kthread_stop(g_om_data.kdumpdsp_task);

	if (NULL != g_om_data.dsp_time_stamp) {
		iounmap(g_om_data.dsp_time_stamp);
		g_om_data.dsp_time_stamp = NULL;
	}

	hifi_remove_procfs();

	for (i = 0; i < ARRAY_SIZE(work_info); i++) {
		if (work_info[i].ctl.wq) {
			flush_workqueue(work_info[i].ctl.wq);
			destroy_workqueue(work_info[i].ctl.wq);
			work_info[i].ctl.wq = NULL;
		}
	}

	OUT_FUNCTION;

	return;
}


void hifi_om_cpu_load_info_show(struct hifi_om_load_info_stru *hifi_om_info)
{
	switch (hifi_om_info->info_type) {
	case HIFI_CPU_LOAD_VOTE_UP:
	case HIFI_CPU_LOAD_VOTE_DOWN:
		logi("CpuUtilization:%d%%, Vote DDR to %dM\n", hifi_om_info->cpu_load_info.cpu_load, hifi_om_info->cpu_load_info.ddr_freq);
		break;

	case HIFI_CPU_LOAD_LACK_PERFORMANCE:
		logw("DDRFreq: %dM, CpuUtilization:%d%%, Lack of performance!!!\n", hifi_om_info->cpu_load_info.ddr_freq,hifi_om_info->cpu_load_info.cpu_load);
		/*upload totally 16 times in every 16 times in case of flushing msg*/
		if (unlikely((dsm_notify_limit % 0x10) == 0)) {/*lint !e730*/
			audio_dsm_report_info(AUDIO_CODEC, DSM_SOC_HIFI_HIGH_CPU, "DSM_SOC_HIFI_HIGH_CPU\n");
		}
		dsm_notify_limit++;
		break;

	default:
		break;
	}
}

void hifi_om_effect_mcps_info_show(struct hifi_om_effect_mcps_stru *hifi_mcps_info)
{
	unsigned int i;

	logw("DDRFreq: %dM, CpuUtilization:%d%%\n",hifi_mcps_info->cpu_load_info.ddr_freq, hifi_mcps_info->cpu_load_info.cpu_load);

	for(i = 0; i < (sizeof(hifi_mcps_info->effect_mcps_info)/sizeof(hifi_effect_mcps_stru)); i++) {
		if (hifi_mcps_info->effect_mcps_info[i].effect_algo_id < ID_EFFECT_ALGO_BUTT && hifi_mcps_info->effect_mcps_info[i].effect_algo_id > ID_EFFECT_ALGO_START) {
			switch (hifi_mcps_info->effect_mcps_info[i].effect_stream_id) {
			case AUDIO_STREAM_PCM_OUTPUT:
				logw("Algorithm: %s, Mcps: %d, Stream: PCM_OUTPUT \n",
					effect_algo[hifi_mcps_info->effect_mcps_info[i].effect_algo_id - 1].effect_name,
					hifi_mcps_info->effect_mcps_info[i].effect_algo_mcps);
				break;

			case AUDIO_STREAM_PLAYER_OUTPUT:
				logw("Algorithm: %s, Mcps: %d, Stream: PLAYER_OUTPUT \n",
					effect_algo[hifi_mcps_info->effect_mcps_info[i].effect_algo_id - 1].effect_name,
					hifi_mcps_info->effect_mcps_info[i].effect_algo_mcps);
				break;

			case AUDIO_STREAM_MIXER_OUTPUT:
				logw("Algorithm: %s, Mcps: %d, Stream: MIXER_OUTPUT \n",
					effect_algo[hifi_mcps_info->effect_mcps_info[i].effect_algo_id - 1].effect_name,
					hifi_mcps_info->effect_mcps_info[i].effect_algo_mcps);
				break;

			case AUDIO_STREAM_VOICE_OUTPUT:
				logw("Algorithm: %s, Mcps: %d, Stream: VOICE_OUTPUT \n",
					effect_algo[hifi_mcps_info->effect_mcps_info[i].effect_algo_id - 1].effect_name,
					hifi_mcps_info->effect_mcps_info[i].effect_algo_mcps);
				break;

			case AUDIO_STREAM_VOICEPP_OUTPUT:
				logw("Algorithm: %s, Mcps: %d, Stream: VOICEPP_OUTPUT \n",
					effect_algo[hifi_mcps_info->effect_mcps_info[i].effect_algo_id- 1].effect_name,
					hifi_mcps_info->effect_mcps_info[i].effect_algo_mcps);
				break;

			case AUDIO_STREAM_PCM_INPUT:
				logw("Algorithm: %s, Mcps: %d, Stream: PCM_INPUT \n",
					effect_algo[hifi_mcps_info->effect_mcps_info[i].effect_algo_id - 1].effect_name,
					hifi_mcps_info->effect_mcps_info[i].effect_algo_mcps);
				break;

			case AUDIO_STREAM_VOICE_INPUT:
				logw("Algorithm: %s, Mcps: %d, Stream: VOICE_INPUT \n",
					effect_algo[hifi_mcps_info->effect_mcps_info[i].effect_algo_id - 1].effect_name,
					hifi_mcps_info->effect_mcps_info[i].effect_algo_mcps);
				break;

			case AUDIO_STREAM_VOICEPP_INPUT:
				logw("Algorithm: %s, Mcps: %d, Stream: VOICEPP_INPUT \n",
					effect_algo[hifi_mcps_info->effect_mcps_info[i].effect_algo_id - 1].effect_name,
					hifi_mcps_info->effect_mcps_info[i].effect_algo_mcps);
				break;

			default:
				break;
			}
		}
	}
}

void hifi_om_update_buff_delay_info_show(struct hifi_om_update_buff_delay_info *info)
{
	logw("hifi continuous update buff delay: mode = %d(0-play, 1-capture), device = %d(0-primary, 1-direct)\n",
		info->pcm_mode, info->pcm_device);
}


