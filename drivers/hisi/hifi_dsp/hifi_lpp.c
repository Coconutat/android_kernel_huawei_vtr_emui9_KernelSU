/*
 * hifi misc driver.
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/wakelock.h>
#include <linux/errno.h>
#include <linux/of_address.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/completion.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <linux/delay.h>

#include <asm/memory.h>
/*lint -e451*/
#include <asm/types.h>
/*lint +e451*/
#include <asm/io.h>

#include <dsm_audio/dsm_audio.h>

#include <linux/compat.h>

#include "hifi_lpp.h"
#include "audio_hifi.h"
#include "drv_mailbox_msg.h"
#include "bsp_drv_ipc.h"
#include "hifi_om.h"
#include <dsm/dsm_pub.h>
#include "usbaudio_ioctl.h"
#include "soundtrigger_socdsp_mailbox.h"

/*lint -e1058*/
#define DTS_COMP_HIFIDSP_NAME "hisilicon,k3hifidsp"
#define FILE_PROC_DIRECTORY "hifidsp"
#define SEND_MSG_TO_HIFI mailbox_send_msg
#define RETRY_COUNT	3
#define FPGA_TIMEOUT_LEN_MS 10000
#define ASIC_TIMEOUT_LEN_MS 2000

static DEFINE_SEMAPHORE(s_misc_sem);/*lint !e64 !e570 !e651*/

LIST_HEAD(recv_sync_work_queue_head);
LIST_HEAD(recv_proc_work_queue_head);


/* 3mic add for reset hi6402 audio dp clk */
struct multi_mic {
	struct workqueue_struct *reset_audio_dp_clk_wq;
	struct work_struct reset_audio_dp_clk_work;
	unsigned int audio_clk_state;
	struct list_head cmd_queue;
	spinlock_t cmd_lock;  /* protects cmd queue */
};

struct misc_msg_info {
	unsigned short msg_id;
	const char * const info;
};

struct hifi_misc_priv {
	spinlock_t	recv_sync_lock;
	spinlock_t	recv_proc_lock;
	spinlock_t	pcm_read_lock;

	struct mutex ioctl_mutex;
	struct mutex proc_read_mutex;

	struct completion	completion;
	wait_queue_head_t	proc_waitq;
	wait_queue_head_t	pcm_read_waitq;

	int	wait_flag;
	int	pcm_read_wait_flag;
	unsigned int	sn;

	struct wake_lock	hifi_misc_wakelock;
	struct wake_lock	update_buff_wakelock;

	unsigned char	*hifi_priv_base_virt;
	unsigned char	*hifi_priv_base_phy;

	struct device	*dev;

	struct multi_mic multi_mic_ctrl;

	enum hifi_dsp_platform_type platform_type;
};
static struct hifi_misc_priv s_misc_data;

static struct notifier_block s_hifi_sr_nb;
static struct notifier_block s_hifi_reboot_nb;
static atomic_t volatile s_hifi_in_suspend = ATOMIC_INIT(0);
static atomic_t volatile s_hifi_in_saving = ATOMIC_INIT(0);


static struct misc_msg_info msg_info[] = {
{ID_AP_AUDIO_SET_DTS_ENABLE_CMD, "ID_AP_AUDIO_SET_DTS_ENABLE_CMD"},
{ID_AP_AUDIO_SET_DTS_DEV_CMD, "ID_AP_AUDIO_SET_DTS_DEV_CMD"},
{ID_AP_AUDIO_SET_DTS_GEQ_CMD, "ID_AP_AUDIO_SET_DTS_GEQ_CMD"},
{ID_AP_AUDIO_SET_DTS_GEQ_ENABLE_CMD, "ID_AP_AUDIO_SET_DTS_GEQ_ENABLE_CMD"},
{ID_AP_HIFI_VOICE_RECORD_START_CMD, "ID_AP_HIFI_VOICE_RECORD_START_CMD"},
{ID_AP_HIFI_VOICE_RECORD_STOP_CMD, "ID_AP_HIFI_VOICE_RECORD_STOP_CMD"},
{ID_AP_VOICEPP_START_REQ, "ID_AP_VOICEPP_START_REQ"},
{ID_VOICEPP_AP_START_CNF, "ID_VOICEPP_AP_START_CNF"},
{ID_AP_VOICEPP_STOP_REQ, "ID_AP_VOICEPP_STOP_REQ"},
{ID_VOICEPP_AP_STOP_CNF, "ID_VOICEPP_AP_STOP_CNF"},
{ID_AP_AUDIO_PLAY_START_REQ, "ID_AP_AUDIO_PLAY_START_REQ"},
{ID_AUDIO_AP_PLAY_START_CNF, "ID_AUDIO_AP_PLAY_START_CNF"},
{ID_AP_AUDIO_PLAY_PAUSE_REQ, "ID_AP_AUDIO_PLAY_PAUSE_REQ"},
{ID_AUDIO_AP_PLAY_PAUSE_CNF, " ID_AUDIO_AP_PLAY_PAUSE_CNF"},
{ID_AUDIO_AP_PLAY_DONE_IND, "ID_AUDIO_AP_PLAY_DONE_IND"},
{ID_AP_AUDIO_PLAY_UPDATE_BUF_CMD, "ID_AP_AUDIO_PLAY_UPDATE_BUF_CMD"},
{ID_AP_AUDIO_PLAY_QUERY_TIME_REQ, "ID_AP_AUDIO_PLAY_QUERY_TIME_REQ"},
{ID_AP_AUDIO_PLAY_WAKEUPTHREAD_REQ, "ID_AP_AUDIO_PLAY_WAKEUPTHREAD_REQ"},
{ID_AUDIO_AP_PLAY_QUERY_TIME_CNF, "ID_AUDIO_AP_PLAY_QUERY_TIME_CNF"},
{ID_AP_AUDIO_PLAY_QUERY_STATUS_REQ, "ID_AP_AUDIO_PLAY_QUERY_STATUS_REQ"},
{ID_AUDIO_AP_PLAY_QUERY_STATUS_CNF, "ID_AUDIO_AP_PLAY_QUERY_STATUS_CNF"},
{ID_AP_AUDIO_PLAY_SEEK_REQ, "ID_AP_AUDIO_PLAY_SEEK_REQ"},
{ID_AUDIO_AP_PLAY_SEEK_CNF, "ID_AUDIO_AP_PLAY_SEEK_CNF"},
{ID_AP_AUDIO_PLAY_SET_VOL_CMD, "ID_AP_AUDIO_PLAY_SET_VOL_CMD"},
{ID_AP_AUDIO_RECORD_PCM_HOOK_CMD, "ID_AP_AUDIO_RECORD_PCM_HOOK_CMD"},
{ID_AP_HIFI_ENHANCE_START_REQ, "ID_AP_HIFI_ENHANCE_START_REQ"},
{ID_HIFI_AP_ENHANCE_START_CNF, "ID_HIFI_AP_ENHANCE_START_CNF"},
{ID_AP_HIFI_ENHANCE_STOP_REQ, "ID_AP_HIFI_ENHANCE_STOP_REQ"},
{ID_HIFI_AP_ENHANCE_STOP_CNF, "ID_HIFI_AP_ENHANCE_STOP_CNF"},
{ID_AP_HIFI_ENHANCE_SET_DEVICE_REQ, "ID_AP_HIFI_ENHANCE_SET_DEVICE_REQ"},
{ID_HIFI_AP_ENHANCE_SET_DEVICE_CNF, "ID_HIFI_AP_ENHANCE_SET_DEVICE_CNF"},
{ID_AP_AUDIO_ENHANCE_SET_DEVICE_IND, "ID_AP_AUDIO_ENHANCE_SET_DEVICE_IND"},
{ID_AP_AUDIO_MLIB_SET_PARA_IND, "ID_AP_AUDIO_MLIB_SET_PARA_IND"},
{ID_AP_AUDIO_CMD_SET_SOURCE_CMD, "ID_AP_AUDIO_CMD_SET_SOURCE_CMD"},
{ID_AP_AUDIO_CMD_SET_DEVICE_CMD, "ID_AP_AUDIO_CMD_SET_DEVICE_CMD"},
{ID_AP_AUDIO_CMD_SET_MODE_CMD, "ID_AP_AUDIO_CMD_SET_MODE_CMD"},
{ID_AP_AUDIO_CMD_SET_ANGLE_CMD, "ID_AP_AUDIO_CMD_SET_ANGLE_CMD"},
{ID_AP_AUDIO_ROUTING_COMPLETE_REQ, "ID_AP_AUDIO_ROUTING_COMPLETE_REQ"},
{ID_AUDIO_AP_OM_DUMP_CMD, "ID_AUDIO_AP_OM_DUMP_CMD"},
{ID_AUDIO_AP_FADE_OUT_REQ, "ID_AUDIO_AP_FADE_OUT_REQ"},
{ID_AP_ENABLE_MODEM_LOOP_REQ, "ID_AP_ENABLE_MODEM_LOOP_REQ"},
{ID_AP_ENABLE_AT_DSP_LOOP_REQ, "ID_AP_ENABLE_AT_DSP_LOOP_REQ"},
{ID_AP_AUDIO_DYN_EFFECT_GET_PARAM, "ID_AP_AUDIO_DYN_EFFECT_GET_PARAM"},
{ID_AP_AUDIO_DYN_EFFECT_GET_PARAM_CNF, "ID_AP_AUDIO_DYN_EFFECT_GET_PARAM_CNF"},
{ID_AP_AUDIO_DYN_EFFECT_TRIGGER, "ID_AP_AUDIO_DYN_EFFECT_TRIGGER"},
{ID_AP_HIFI_REQUEST_SET_PARA_CMD, "ID_AP_HIFI_REQUEST_SET_PARA_CMD"},
{ID_AP_HIFI_REQUEST_GET_PARA_CMD, "ID_AP_HIFI_REQUEST_GET_PARA_CMD"},
{ID_AP_HIFI_REQUEST_GET_PARA_CNF, "ID_AP_HIFI_REQUEST_GET_PARA_CNF"},
};
extern int hisi_dptx_set_aparam(unsigned int channel_num, unsigned int data_width, unsigned int sample_rate);
extern int hisi_dptx_get_spec(void *data, unsigned int size, unsigned int *ext_acount);

unsigned long try_copy_to_user(void __user *to, const void *from, unsigned long n)
{
	int try_times = 0;
	unsigned long ret = 0;

	while (try_times < RETRY_COUNT) {
		ret = copy_to_user(to, from, n);
		if (0 == ret)
			break;

		try_times++;
		logw("copy to user fail ret %lu, retry %d times\n", ret, try_times);
	}

	return ret;
}

unsigned long try_copy_from_user(void *to, const void __user *from, unsigned long n)
{
	int try_times = 0;
	unsigned long ret = 0;

	while (try_times < RETRY_COUNT) {
		ret = copy_from_user(to, from, n);
		if (0 == ret)
			break;

		try_times++;
		logw("copy from user fail ret %lu, retry %d times\n", ret, try_times);
	}

	return ret;
}

void sochifi_watchdog_send_event(void)
{
	int retval = 0;
	char *envp[2] = {"hifi_watchdog", NULL};

	can_reset_system();

	logi("soc hifi watchdog coming, now reset mediaserver \n");
	retval = kobject_uevent_env(&s_misc_data.dev->kobj, KOBJ_CHANGE, envp);
	if (retval) {
		loge("send uevent failed, retval: %d\n", retval);
	}

	return;
}

static void hifi_misc_msg_info(unsigned short msg_id)
{
	int i;
	int size;

	size = sizeof(msg_info)/sizeof(msg_info[0]);

	for (i = 0; i < size; i++) {
		if (msg_info[i].msg_id == msg_id) {
			if (msg_id != ID_AP_AUDIO_PLAY_QUERY_TIME_REQ
				&& msg_id != ID_AUDIO_AP_PLAY_QUERY_TIME_CNF)
				logi("MSG: %s.\n", msg_info[i].info);
			break;
		}
	}

	if (i == size) {
		logw("MSG: Not defined msg id: 0x%x.\n", msg_id);
	}

	return;
}


static int hifi_misc_async_write(unsigned char *arg, unsigned int len)
{
	int ret = OK;

	IN_FUNCTION;

	if (NULL == arg) {
		loge("input arg is NULL.\n");
		ret = ERROR;
		goto END;
	}

	/*调用核间通信接口发送数据*/
	ret = SEND_MSG_TO_HIFI(MAILBOX_MAILCODE_ACPU_TO_HIFI_MISC, arg, len);
	if (OK != ret) {
		loge("msg send to hifi fail,ret is %d.\n", ret);
		ret = ERROR;
		goto END;
	}

END:
	OUT_FUNCTION;
	return ret;
}


static int hifi_misc_sync_write(unsigned char  *buff, unsigned int len)
{
	int ret = OK;
	unsigned long wait_reult = 0;
	static unsigned int wait_count = 0;

	IN_FUNCTION;

	if (NULL == buff) {
		loge("input arg is NULL.\n");
		ret = ERROR;
		goto END;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
	reinit_completion(&s_misc_data.completion);
#else
	INIT_COMPLETION(s_misc_data.completion);
#endif

	/*调用核间通信接口发送数据，得到返回值ret*/
	ret = SEND_MSG_TO_HIFI(MAILBOX_MAILCODE_ACPU_TO_HIFI_MISC, buff, len);
	if (OK != ret) {
		loge("msg send to hifi fail,ret is %d.\n", ret);
		ret = ERROR;
		goto END;
	}

	if (hifi_misc_get_platform_type() == HIFI_DSP_PLATFORM_FPGA)
		wait_reult = wait_for_completion_timeout(&s_misc_data.completion, msecs_to_jiffies(FPGA_TIMEOUT_LEN_MS));
	else
		wait_reult = wait_for_completion_timeout(&s_misc_data.completion, msecs_to_jiffies(ASIC_TIMEOUT_LEN_MS));

	s_misc_data.sn++;
	if (unlikely(s_misc_data.sn & 0x10000000)){
		s_misc_data.sn = 0;
	}

	if (!wait_reult) {
		++wait_count;

		if (wait_count > 1) {
			loge("wait completion timeout\n");
			audio_dsm_report_info(AUDIO_CODEC, DSM_SOC_HIFI_SYNC_TIMEOUT, "soc hifi sync message timeout");
			hifi_dump_panic_log();
		}

		ret = ERROR;
	} else {
		ret = OK;
	}

END:
	OUT_FUNCTION;
	return ret;
}

static bool hifi_misc_local_process(unsigned short _msg_id)
{
	bool ret = false;
	HIFI_MSG_ID msg_id =  (HIFI_MSG_ID)_msg_id;

	switch(msg_id) {
	case ID_AUDIO_AP_OM_DUMP_CMD:
	case ID_AUDIO_AP_FADE_OUT_REQ:
	case ID_AUDIO_AP_DP_CLK_EN_IND:
	case ID_AUDIO_AP_VOICE_BSD_PARAM_CMD:
	case ID_AUDIO_AP_OM_CMD:
	case ID_AUDIO_AP_3A_CMD:
	case ID_HIFI_AP_BIGDATA_CMD:
		ret = true;
		break;
	default:
		break;
	}

	return ret;
}
/*lint -e429*/
static void hifi_misc_mesg_process(void *cmd)
{
	unsigned int cmd_id = 0;
	struct common_hifi_cmd * common_cmd = NULL;
	struct hifi_om_ap_data *hifi_om_rev_data = NULL;

	BUG_ON(NULL == cmd);

	hifi_om_rev_data = (struct hifi_om_ap_data *)cmd;
	common_cmd =(struct common_hifi_cmd *)cmd;
	cmd_id = *(unsigned short*)cmd;

	switch (cmd_id) {
	case ID_AUDIO_AP_OM_CMD:
		hifi_om_rev_data_handle(HIFI_OM_WORK_AUDIO_OM_DETECTION, hifi_om_rev_data->data,
			hifi_om_rev_data->data_len);
		break;
	case ID_AUDIO_AP_3A_CMD:
		hifi_om_rev_data_handle(HIFI_OM_WORK_VOICE_3A, hifi_om_rev_data->data,
			hifi_om_rev_data->data_len);
		break;
	case ID_AUDIO_AP_VOICE_BSD_PARAM_CMD:
		hifi_om_rev_data_handle(HIFI_OM_WORK_VOICE_BSD, hifi_om_rev_data->data,
			hifi_om_rev_data->data_len);
		break;
	case ID_AUDIO_AP_OM_DUMP_CMD:
		logi("hifi notify to dump hifi log, hifi errtype: %d.\n", common_cmd->value);
		break;
	case ID_AUDIO_AP_DP_CLK_EN_IND:
	case ID_AUDIO_AP_FADE_OUT_REQ:
		{
			struct dp_clk_request *dp_clk_cmd = NULL;

			dp_clk_cmd = (struct dp_clk_request *)kmalloc(sizeof(struct dp_clk_request), GFP_ATOMIC);
			if (!dp_clk_cmd) {
				loge("malloc fail\n");
				break;
			}
			memset(dp_clk_cmd, 0, sizeof(struct dp_clk_request));/* unsafe_function_ignore: memset */

			logi("multi mic cmd: 0x%x.\n", common_cmd->msg_id);
			memcpy(&(dp_clk_cmd->dp_clk_msg), common_cmd, sizeof(struct common_hifi_cmd));/* unsafe_function_ignore: memcpy */

			spin_lock_bh(&(s_misc_data.multi_mic_ctrl.cmd_lock));
			list_add_tail(&dp_clk_cmd->dp_clk_node, &(s_misc_data.multi_mic_ctrl.cmd_queue));
			spin_unlock_bh(&(s_misc_data.multi_mic_ctrl.cmd_lock));
			wake_lock_timeout(&s_misc_data.hifi_misc_wakelock, HZ/2);
			if (queue_work(s_misc_data.multi_mic_ctrl.reset_audio_dp_clk_wq,
					&s_misc_data.multi_mic_ctrl.reset_audio_dp_clk_work))
				logw("cmd 0x%x no trigger queue work\n", common_cmd->msg_id);
			break;
		}
	case ID_HIFI_AP_BIGDATA_CMD:
		hifi_om_rev_data_handle(HIFI_OM_WORK_VOICE_BIGDATA, hifi_om_rev_data->data,
			hifi_om_rev_data->data_len);
		break;
	default:
		break;
	}

	return;
}
/*lint +e429*/

static void hifi_misc_handle_mail(void *usr_para, void *mail_handle, unsigned int mail_len)
{
	unsigned int ret_mail			= 0;
	struct recv_request *recv = NULL;
	HIFI_CHN_CMD *cmd_para = NULL;
	void *recmsg = NULL;

	IN_FUNCTION;

	if (NULL == mail_handle) {
		loge("mail_handle is NULL.\n");
		goto END;
	}

	if (mail_len >= MAIL_LEN_MAX || mail_len <= (SIZE_CMD_ID + sizeof(unsigned short))) {
		loge("mail_len is invalid: %u(>= 512 or <= 8)\n", mail_len);
		goto END;
	}

	recv = (struct recv_request *)kmalloc(sizeof(struct recv_request), GFP_ATOMIC);
	if (NULL == recv)
	{
		loge("recv kmalloc failed.\n");
		goto ERR;
	}
	memset(recv, 0, sizeof(struct recv_request));/* unsafe_function_ignore: memset */

	/* 设定SIZE */
	recv->rev_msg.mail_buff_len = mail_len;
	/* 分配总的空间 */
	recv->rev_msg.mail_buff = (unsigned char *)kmalloc(mail_len, GFP_ATOMIC);
	if (NULL == recv->rev_msg.mail_buff)
	{
		loge("recv->rev_msg.mail_buff kmalloc failed.\n");
		goto ERR;
	}
	memset(recv->rev_msg.mail_buff, 0, mail_len);/* unsafe_function_ignore: memset */

	/* 将剩余内容copy透传到buff中 */
	ret_mail = mailbox_read_msg_data(mail_handle, (char*)(recv->rev_msg.mail_buff), (unsigned int *)(&(recv->rev_msg.mail_buff_len)));

	if ((ret_mail != MAILBOX_OK) || (recv->rev_msg.mail_buff_len == 0)) {
		loge("Empty point or data length error! ret=0x%x, mail_size: %d.\n", (unsigned int)ret_mail, recv->rev_msg.mail_buff_len);
		goto ERR;
	}

	logd("ret_mail=%d, mail_buff_len=%d, msgID=0x%x.\n", ret_mail, recv->rev_msg.mail_buff_len,
		 *((unsigned int *)(recv->rev_msg.mail_buff + mail_len - SIZE_CMD_ID)));

	/* 约定，前4个字节是cmd_id */
	cmd_para   = (HIFI_CHN_CMD *)(recv->rev_msg.mail_buff + mail_len - SIZE_CMD_ID);
	recmsg = (void*)recv->rev_msg.mail_buff;
	/* 赋予不同的接收指针，由接收者释放分配空间 */
	if (HIFI_CHN_SYNC_CMD == cmd_para->cmd_type) {
		if (s_misc_data.sn == cmd_para->sn) {
			spin_lock_bh(&s_misc_data.recv_sync_lock);
			list_add_tail(&recv->recv_node, &recv_sync_work_queue_head);
			spin_unlock_bh(&s_misc_data.recv_sync_lock);
			complete(&s_misc_data.completion);
			goto END;
		} else {
			loge("s_misc_data.sn !== cmd_para->sn: %d, %d.\n", s_misc_data.sn, cmd_para->sn);
			goto ERR;
		}
	} else if ((HIFI_CHN_READNOTICE_CMD == cmd_para->cmd_type) && (ACPU_TO_HIFI_ASYNC_CMD == cmd_para->sn)) {
		if (ID_AUDIO_AP_PLAY_DONE_IND == *((unsigned short *)recmsg)) {
			logi("receive msg: ID_AUDIO_AP_PLAY_DONE_IND\n");
			/* only mesg ID_AUDIO_AP_PLAY_DONE_IND lock 5s */
			wake_lock_timeout(&s_misc_data.update_buff_wakelock, 5*HZ);

			spin_lock_bh(&s_misc_data.recv_proc_lock);
			list_add_tail(&recv->recv_node, &recv_proc_work_queue_head);
			s_misc_data.wait_flag++;
			spin_unlock_bh(&s_misc_data.recv_proc_lock);
			wake_up(&s_misc_data.proc_waitq);
			goto END;
		}

		if (ID_AUDIO_AP_UPDATE_PCM_BUFF_CMD == *((unsigned short *)recmsg)) {
			spin_lock_bh(&s_misc_data.pcm_read_lock);
			s_misc_data.pcm_read_wait_flag = true;
			spin_unlock_bh(&s_misc_data.pcm_read_lock);
			wake_up(&s_misc_data.pcm_read_waitq);
		}

		if(hifi_misc_local_process(*(unsigned short*)recmsg)){
			hifi_misc_mesg_process(recmsg);
		}
	} else {
		loge("unknown msg comed from hifi .\n");
	}

ERR:
	if (recv) {
		if (recv->rev_msg.mail_buff) {
			kfree(recv->rev_msg.mail_buff);
		}
		kfree(recv);
	}

END:
	OUT_FUNCTION;

	return;/*lint !e593*/
}


static int hifi_dsp_get_input_param(unsigned int usr_para_size, void *usr_para_addr,
									unsigned int *krn_para_size, void **krn_para_addr)
{
	void *para_in = NULL;
	unsigned int para_size_in = 0;

	IN_FUNCTION;

	/* 限制分配空间 */
	if ((usr_para_size == 0) || (usr_para_size > SIZE_LIMIT_PARAM - SIZE_CMD_ID)) {
		loge("usr_para_size(%u) exceed LIMIT(0/%u).\n", usr_para_size, SIZE_LIMIT_PARAM - SIZE_CMD_ID);
		goto ERR;
	}

	/*获取arg入参*/
	para_size_in = usr_para_size + SIZE_CMD_ID;
	para_in = kzalloc(para_size_in, GFP_KERNEL);
	if (NULL == para_in) {
		loge("kzalloc fail.\n");
		goto ERR;
	}

	if (NULL != usr_para_addr) {
		if (try_copy_from_user(para_in , usr_para_addr, usr_para_size)) {
			loge("copy_from_user fail.\n");
			goto ERR;
		}
	} else {
		loge("usr_para_addr is null no user data.\n");
		goto ERR;
	}

	/* 设置出参 */
	*krn_para_size = para_size_in;
	*krn_para_addr = para_in;

	hifi_misc_msg_info(*(unsigned short*)para_in);

	OUT_FUNCTION;
	return OK;

ERR:
	if (para_in != NULL) {
		kfree(para_in);
		para_in = NULL;
	}

	OUT_FUNCTION;
	return ERROR;
}


static void hifi_dsp_get_input_param_free(void **krn_para_addr)
{
	IN_FUNCTION;

	if (*krn_para_addr != NULL) {
		kfree(*krn_para_addr);
		*krn_para_addr = NULL;
	} else {
		loge("krn_para_addr to free is NULL.\n");
	}

	OUT_FUNCTION;
	return;
}


static int hifi_dsp_get_output_param(unsigned int krn_para_size, void *krn_para_addr,
									 unsigned int *usr_para_size, void __user *usr_para_addr)
{
	int ret			= OK;
	void __user *para_to = NULL;
	unsigned int para_n = 0;

	IN_FUNCTION;

	/* 入参判定 */
	if (NULL == krn_para_addr) {
		loge("krn_para_addr is NULL.\n");
		ret = -EINVAL;
		goto END;
	}

	/* 入参判定 */
	if ((NULL == usr_para_addr) || (NULL == usr_para_size)) {
		loge("usr_size_p=0x%pK, usr_addr=0x%pK.\n", usr_para_size, usr_para_addr);
		ret = -EINVAL;
		goto END;
	}

	para_to = usr_para_addr;
	para_n = krn_para_size;
	if ((0 == para_n) || (SIZE_LIMIT_PARAM < para_n)) {
		loge("para_n exceed limit (%d / %d).\n", para_n, SIZE_LIMIT_PARAM);
		ret = -EINVAL;
		goto END;
	}

	if (para_n > *usr_para_size) {
		loge("para_n exceed usr_size(%d / %d).\n", para_n, *usr_para_size);
		ret = -EINVAL;
		goto END;
	}

	/* Copy data from kernel space to user space
		to, from, n */
	if (try_copy_to_user(para_to, krn_para_addr, para_n)) {
		loge("copy_to_user fail\n");
		ret = COPYFAIL;
		goto END;
	}

	*usr_para_size = para_n;
	hifi_misc_msg_info(*(unsigned short*)krn_para_addr);

END:
	OUT_FUNCTION;
	return ret;
}


static int hifi_dsp_async_cmd(unsigned long arg)
{
	int ret = OK;
	struct misc_io_async_param param;
	void *para_krn_in = NULL;
	unsigned int para_krn_size_in = 0;
	HIFI_CHN_CMD *cmd_para = NULL;
	void* para_addr_in = NULL;

	IN_FUNCTION;

	if (try_copy_from_user(&param,(void*) arg, sizeof(struct misc_io_async_param))) {
		loge("copy_from_user fail.\n");
		ret = ERROR;
		goto END;
	}

	para_addr_in = INT_TO_ADDR(param.para_in_l,param.para_in_h);
	/*获取arg入参*/
	ret = hifi_dsp_get_input_param(param.para_size_in, para_addr_in,
								   &para_krn_size_in, &para_krn_in);
	if (OK != ret) {
		loge("get ret=%d.\n", ret);
		goto END;
	}
	/* add cmd id and sn  */
	cmd_para = (HIFI_CHN_CMD *)(para_krn_in+para_krn_size_in-SIZE_CMD_ID);
	cmd_para->cmd_type = HIFI_CHN_SYNC_CMD;
	cmd_para->sn = ACPU_TO_HIFI_ASYNC_CMD;

	/*邮箱发送至HIFI, 异步*/
	ret = hifi_misc_async_write(para_krn_in, para_krn_size_in);
	if (OK != ret) {
		loge("async_write ret=%d.\n", ret);
		goto END;
	}

	if (ID_AP_AUDIO_PLAY_UPDATE_BUF_CMD == *(unsigned short *)para_krn_in) {
		wake_unlock(&s_misc_data.update_buff_wakelock);/*lint !e455*/
	}

END:
	hifi_dsp_get_input_param_free(&para_krn_in);
	OUT_FUNCTION;
	return ret;
}



static int hifi_dsp_sync_cmd(unsigned long arg)
{
	int ret = OK;
	struct misc_io_sync_param param;
	void *para_krn_in = NULL;
	unsigned int para_krn_size_in = 0;
	HIFI_CHN_CMD *cmd_para = NULL;
	void __user *para_addr_in	= NULL;
	void __user *para_addr_out = NULL;
	struct recv_request *recv = NULL;
	unsigned char * mail_buf = NULL;
	unsigned int mail_len = 0;

	IN_FUNCTION;

	if (try_copy_from_user(&param,(void*) arg, sizeof(struct misc_io_sync_param))) {
		loge("copy_from_user fail.\n");
		ret = ERROR;
		goto END;
	}
	logd("para_size_in=%d.\n", param.para_size_in);

	para_addr_in   = INT_TO_ADDR(param.para_in_l ,param.para_in_h);
	para_addr_out  = INT_TO_ADDR(param.para_out_l,param.para_out_h);
	/*获取arg入参*/
	ret = hifi_dsp_get_input_param(param.para_size_in, para_addr_in,
						&para_krn_size_in, &para_krn_in);
	if (OK != ret) {
		loge("hifi_dsp_get_input_param fail: ret=%d.\n", ret);
		goto END;
	}

	/* add cmd id and sn  */
	cmd_para = (HIFI_CHN_CMD *)(para_krn_in+para_krn_size_in-SIZE_CMD_ID);
	cmd_para->cmd_type = HIFI_CHN_SYNC_CMD;

	cmd_para->sn = s_misc_data.sn;

	/*邮箱发送至HIFI, 同步*/
	ret = hifi_misc_sync_write(para_krn_in, para_krn_size_in);
	if (OK != ret) {
		loge("hifi_misc_sync_write ret=%d.\n", ret);
		goto END;
	}

	mail_buf = kzalloc(SIZE_LIMIT_PARAM, GFP_KERNEL);
	if (!mail_buf) {
		loge("alloc mail buffer failed\n");
		ret = -ENOMEM;
		goto END;
	}

	/*将获得的rev_msg信息填充到出参arg*/
	spin_lock_bh(&s_misc_data.recv_sync_lock);

	if (!list_empty(&recv_sync_work_queue_head)) {
		recv = list_entry(recv_sync_work_queue_head.next, struct recv_request, recv_node);
		mail_len = recv->rev_msg.mail_buff_len - SIZE_CMD_ID;
		if (mail_len <= SIZE_LIMIT_PARAM) {
			memcpy(mail_buf, recv->rev_msg.mail_buff, mail_len);/* unsafe_function_ignore: memcpy */
			ret = OK;
		} else {
			ret = -EINVAL;
		}

		list_del(&recv->recv_node);
		kfree(recv->rev_msg.mail_buff);
		kfree(recv);
		recv = NULL;
	}
	spin_unlock_bh(&s_misc_data.recv_sync_lock);

	if (ret != OK) {
		loge("mail buff error buf length:%u\n", mail_len);
		goto END;
	}
	ret = hifi_dsp_get_output_param(mail_len, mail_buf, &param.para_size_out, para_addr_out);
	if (OK != ret) {
		loge("get output param fail, ret=%d.\n", ret);
		goto END;
	}

	if (try_copy_to_user((void*)arg, &param, sizeof(struct misc_io_sync_param))) {
		loge("copy_to_user fail.\n");
		ret = COPYFAIL;
		goto END;
	}

END:
	if (mail_buf)
		kfree(mail_buf);

	/*释放krn入参*/
	hifi_dsp_get_input_param_free(&para_krn_in);

	OUT_FUNCTION;
	return ret;
}


static int hifi_dsp_get_phys_cmd(unsigned long arg)
{
	int ret  =	OK;
	struct misc_io_get_phys_param param;
	unsigned long para_addr_in = 0;

	IN_FUNCTION;

	if (try_copy_from_user(&param,(void*) arg, sizeof(struct misc_io_get_phys_param))) {
		loge("copy_from_user fail.\n");
		OUT_FUNCTION;
		return ERROR;
	}

	switch (param.flag)
	{
		case 0:
			para_addr_in = (unsigned long)(s_misc_data.hifi_priv_base_phy - HIFI_UNSEC_BASE_ADDR);
			param.phys_addr_l = GET_LOW32(para_addr_in);
			param.phys_addr_h = GET_HIG32(para_addr_in);
			logd("para_addr_in = 0x%pK.\n", (void *)para_addr_in);
			break;

		default:
			ret = ERROR;
			loge("invalid flag=%d.\n", param.flag);
			break;
	}

	if (try_copy_to_user((void*)arg, &param, sizeof(struct misc_io_get_phys_param))) {
		loge("copy_to_user fail.\n");
		ret = ERROR;
	}

	OUT_FUNCTION;
	return ret;
}


/*lint -e429*/
static int hifi_dsp_wakeup_read_thread(unsigned long arg)
{
	struct recv_request *recv = NULL;
	struct misc_recmsg_param *recmsg = NULL;
	unsigned int wake_cmd = (unsigned int)arg;
	struct list_head *pos = NULL;
	unsigned int node_count = 0;

	spin_lock_bh(&s_misc_data.recv_proc_lock);
	list_for_each(pos, &recv_proc_work_queue_head) {/*lint !e838*/
		node_count++;
	}
	spin_unlock_bh(&s_misc_data.recv_proc_lock);

	if (node_count >= MAX_NODE_COUNT) {
		loge("too much work left in proc_work_queue, node count:%u\n", node_count);
		return -EBUSY;
	}

	recv = (struct recv_request *)kmalloc(sizeof(struct recv_request), GFP_ATOMIC);
	if (NULL == recv)
	{
		loge("recv kmalloc failed.\n");
		return -ENOMEM;
	}
	memset(recv, 0, sizeof(struct recv_request));/* unsafe_function_ignore: memset */

	wake_lock_timeout(&s_misc_data.hifi_misc_wakelock, HZ);

	/* 设定SIZE */
	recv->rev_msg.mail_buff_len = sizeof(struct misc_recmsg_param) + SIZE_CMD_ID;

	/* 分配总的空间 */
	recv->rev_msg.mail_buff = (unsigned char *)kmalloc(recv->rev_msg.mail_buff_len, GFP_ATOMIC);
	if (NULL == recv->rev_msg.mail_buff)
	{
		kfree(recv);
		loge("recv->rev_msg.mail_buff kmalloc failed.\n");
		return -ENOMEM;
	}
	memset(recv->rev_msg.mail_buff, 0, recv->rev_msg.mail_buff_len);/* unsafe_function_ignore: memset */

	recmsg = (struct misc_recmsg_param *)recv->rev_msg.mail_buff;
	recmsg->msgID = ID_AUDIO_AP_PLAY_DONE_IND;
	recmsg->playStatus = (unsigned short)wake_cmd;

	spin_lock_bh(&s_misc_data.recv_proc_lock);
	list_add_tail(&recv->recv_node, &recv_proc_work_queue_head);
	s_misc_data.wait_flag++;
	spin_unlock_bh(&s_misc_data.recv_proc_lock);

	wake_up(&s_misc_data.proc_waitq);

	return OK;
}
/*lint +e429*/
static int hifi_dsp_wakeup_pcm_read_thread(unsigned long arg)
{
	(void)arg;

	spin_lock_bh(&s_misc_data.pcm_read_lock);
	s_misc_data.pcm_read_wait_flag = true;
	spin_unlock_bh(&s_misc_data.pcm_read_lock);

	wake_up(&s_misc_data.pcm_read_waitq);

	return OK;
}


static int hifi_dsp_write_param(unsigned long arg)
{
	int ret = OK;
	void*		hifi_param_vir_addr = NULL;
	void*		para_addr_in		= NULL;
	void*		para_addr_out		= NULL;
	struct misc_io_sync_param para;

	IN_FUNCTION;

	if (try_copy_from_user(&para, (void*)arg, sizeof(struct misc_io_sync_param))) {
		loge("copy_from_user fail.\n");
		ret = ERROR;
		goto error1;
	}

	para_addr_in  = INT_TO_ADDR(para.para_in_l ,para.para_in_h);
	para_addr_out = INT_TO_ADDR(para.para_out_l,para.para_out_h);

	hifi_param_vir_addr = (unsigned char*)(s_misc_data.hifi_priv_base_virt + (HIFI_AP_NV_DATA_ADDR - HIFI_UNSEC_BASE_ADDR));

	logd("hifi_param_vir_addr = 0x%pK. (*hifi_param_vir_addr) = 0x%x\n",
			hifi_param_vir_addr, (*(int *)hifi_param_vir_addr));

	logd("user addr = 0x%pK, size = %d \n", para_addr_in, para.para_size_in);

	if (para.para_size_in != NVPARAM_TOTAL_SIZE) {
		loge("the para_size_in(%u) is not equal to NVPARAM_TOTAL_SIZE(%u). \n",
			para.para_size_in, (unsigned int)(NVPARAM_TOTAL_SIZE));
		ret = ERROR;
		goto error1;
	}

	if (try_copy_from_user(hifi_param_vir_addr, (void __user *)para_addr_in, para.para_size_in)) {
		loge("copy data to hifi error\n");
		ret = ERROR;
	}

	if (para.para_size_out != sizeof(ret)) {
		loge("the para_size_out(%u) is not equal to sizeof(ret)(%zu) \n", para.para_size_out, sizeof(ret));
		ret = ERROR;
		goto error1;
	}

	if (try_copy_to_user((void __user *)para_addr_out, &ret, sizeof(ret))) {
		loge("copy data to user fail\n");
		ret = ERROR;
	}

error1:
	OUT_FUNCTION;
	return ret;
}


static int hifi_dsp_write_audio_effect_param(unsigned long arg)
{
	int ret = OK;
	unsigned long copy_ret = 0;
	void*		hifi_param_vir_addr = NULL;
	void*		para_addr_in		= NULL;
	void*		para_addr_out		= NULL;
	struct misc_io_sync_param para;

	IN_FUNCTION;

	if (try_copy_from_user(&para, (void*)arg, sizeof(struct misc_io_sync_param))) {
		loge("copy_from_user fail.\n");
		ret = ERROR;
		goto error1;
	}

	if (para.para_size_in > HIFI_AUDIO_EFFECT_PARAM_BUFF_SIZE) {
		loge("the para_size_in(%u) is greater than  HIFI_AUDIO_PARAM_TOTAL_SIZE(%u). \n",
			para.para_size_in, (unsigned int)(HIFI_AUDIO_EFFECT_PARAM_BUFF_SIZE));
		ret = ERROR;
		goto error1;
	}

	if (para.para_size_out != sizeof(ret)) {
		loge("the para_size_out(%u) is not equal to sizeof(ret)(%zu) \n", para.para_size_out, sizeof(ret));
		ret = ERROR;
		goto error1;
	}

	para_addr_in  = INT_TO_ADDR(para.para_in_l ,para.para_in_h);
	para_addr_out = INT_TO_ADDR(para.para_out_l,para.para_out_h);
	hifi_param_vir_addr = (unsigned char*)(s_misc_data.hifi_priv_base_virt + (HIFI_AUDIO_EFFECT_PARAM_ADDR - HIFI_UNSEC_BASE_ADDR));
	logd("hifi_param_vir_addr = 0x%pK. (*hifi_param_vir_addr) = 0x%x\n",
			hifi_param_vir_addr, (*(int *)hifi_param_vir_addr));
	logd("user addr = 0x%pK, size = %d \n", para_addr_in, para.para_size_in);

	copy_ret = try_copy_from_user(hifi_param_vir_addr, (void __user *)para_addr_in, para.para_size_in);/*lint -e747*/
	if (copy_ret != 0) {
		loge("copy data to hifi error! ret = %lu\n", copy_ret);
		ret = ERROR;
	}

	copy_ret = try_copy_to_user((void __user *)para_addr_out, &ret, sizeof(ret));
	if (copy_ret) {
		loge("copy data to user fail! ret = %lu\n", copy_ret);
		ret = ERROR;
	}

error1:
	OUT_FUNCTION;

	return ret;
}

static int hifi_dsp_write_smartpa_param(unsigned long arg)
{
	int ret = OK;
	unsigned long copy_ret = 0;
	void* hifi_param_vir_addr = NULL;
	void* para_addr_in        = NULL;
	void* para_addr_out       = NULL;
	struct misc_io_sync_param para;

	IN_FUNCTION;

	if (try_copy_from_user(&para, (void*)arg, sizeof(struct misc_io_sync_param))) {
		loge("copy_from_user fail.\n");
		ret = ERROR;
		goto error1;
	}

	if (para.para_size_in > HISI_AP_AUDIO_PA_BUFF_SIZE) {
		loge("the para_size_in(%u) is greater than HISI_AP_AUDIO_PA_BUFF_SIZE(%u). \n",
		para.para_size_in, (unsigned int)(HISI_AP_AUDIO_PA_BUFF_SIZE));
		ret = ERROR;
		goto error1;
	}

	if (para.para_size_out != sizeof(ret)) {
		loge("the para_size_out(%u) is not equal to sizeof(ret)(%zu) \n", para.para_size_out, sizeof(ret));
		ret = ERROR;
		goto error1;
	}

	para_addr_in  = INT_TO_ADDR(para.para_in_l ,para.para_in_h);
	para_addr_out = INT_TO_ADDR(para.para_out_l,para.para_out_h);
	hifi_param_vir_addr = (unsigned char*)(s_misc_data.hifi_priv_base_virt + (HISI_AP_AUDIO_PA_ADDR - HIFI_UNSEC_BASE_ADDR));
	logd("hifi_param_vir_addr = 0x%pK. (*hifi_param_vir_addr) = 0x%x\n", hifi_param_vir_addr, (*(int *)hifi_param_vir_addr));
	logd("user addr = 0x%pK, size = %d \n", para_addr_in, para.para_size_in);

	copy_ret = try_copy_from_user(hifi_param_vir_addr, (void __user *)para_addr_in, para.para_size_in);/*lint -e747*/
	if (copy_ret != 0) {
		loge("copy data to hifi error! ret = %lu\n", copy_ret);
		ret = ERROR;
	}

	copy_ret = try_copy_to_user((void __user *)para_addr_out, &ret, sizeof(ret));
	if (copy_ret) {
		loge("copy data to user fail! ret = %lu\n", copy_ret);
		ret = ERROR;
	}

error1:
	OUT_FUNCTION;

	return ret;
}
static int hifi_dsp_usbaudio_cmd(unsigned long arg)
{
	int ret = OK;
	void* para_addr_in = NULL;
	void* para_addr_out = NULL;
	struct usbaudio_ioctl_input input;
	struct usbaudio_info output;
	struct misc_io_sync_param para;

	IN_FUNCTION;

	memset(&input, 0, sizeof(input));/* unsafe_function_ignore: memset */
	memset(&output, 0, sizeof(output));/* unsafe_function_ignore: memset */
	memset(&para, 0, sizeof(para));/* unsafe_function_ignore: memset */

	if (try_copy_from_user(&para, (void*)arg, sizeof(struct misc_io_sync_param))) {
		loge("copy_from_user fail.\n");
		ret = ERROR;
		goto error1;
	}

	if (para.para_size_out != sizeof(output)) {
		loge("wrong ioctl output size.size:%d should be %ld\n", para.para_size_out, sizeof(output));
		ret = ERROR;
		goto error1;
	}

	para_addr_in  = INT_TO_ADDR(para.para_in_l ,para.para_in_h);
	para_addr_out = INT_TO_ADDR(para.para_out_l,para.para_out_h);

	if (para.para_size_in != sizeof(struct usbaudio_ioctl_input)) {
		loge("wrong ioctl input size.size:%d should be %ld\n", para.para_size_in, sizeof(struct usbaudio_ioctl_input));
		ret = ERROR;
		goto error1;
	}

	if (try_copy_from_user(&input, (void __user *)para_addr_in, para.para_size_in)) {
		loge("copy ioctl input data to kernel error.\n");
		ret = ERROR;
		goto error1;
	}

	if ((unsigned int)USBAUDIO_QUERY_INFO == input.msg_type) {
		/* query controller state */
		usbaudio_ctrl_query_info(&output);
	} else if ((unsigned int)USBAUDIO_USB_POWER_RESUME == input.msg_type) {
		output.sr_status = usbaudio_ctrl_usb_resume();
	} else if ((unsigned int)USBAUDIO_NV_ISREADY == input.msg_type) {
		usbaudio_set_nv_ready();
	} else {
		/* do nothing */
		loge("message type error. %x \n", input.msg_type);
	}

	if (try_copy_to_user((void __user *)para_addr_out, &output, sizeof(output))) {
		loge("copy data to user fail\n");
		ret = ERROR;
	}
error1:
	OUT_FUNCTION;
	return ret;
}

static int hifi_dsp_soundtrigger_cmd(unsigned long arg)
{
	int ret = OK;
	void* para_addr_in = NULL;
	void* para_addr_out = NULL;
	struct soundtrigger_sync_msg *input = NULL;
	int output = 0;
	struct misc_io_sync_param para;

	IN_FUNCTION;

	memset(&para, 0, sizeof(para));/* unsafe_function_ignore: memset */

	if (try_copy_from_user(&para, (void*)arg, sizeof(struct misc_io_sync_param))) {
		loge("copy_from_user fail.\n");
		ret = ERROR;
		goto error1;
	}

	if (para.para_size_out != sizeof(output)) {
		loge("wrong ioctl output size.size:%d should be %ld\n", para.para_size_out, sizeof(output));
		ret = ERROR;
		goto error1;
	}

	para_addr_in  = INT_TO_ADDR(para.para_in_l ,para.para_in_h);
	para_addr_out = INT_TO_ADDR(para.para_out_l,para.para_out_h);

	if (para.para_size_in < sizeof(*input) || para.para_size_in > (sizeof(*input) + HISI_AP_AUDIO_WAKEUP_MODEL_SIZE)) {
		loge("wrong ioctl input size.size:%d \n", para.para_size_in);
		ret = ERROR;
		goto error1;
	}

	input = (struct soundtrigger_sync_msg *)kzalloc(para.para_size_in, GFP_ATOMIC);
	if (input == NULL) {
		loge("kzalloc fail\n");
		ret = ERROR;
		goto error1;
	}

	if (try_copy_from_user((void*)input, (void __user *)para_addr_in, para.para_size_in)) {
		loge("copy ioctl input data to kernel error.\n");
		ret = ERROR;
		goto error1;
	}

	switch (input->msg_type) {
	case WAKEUP_CHN_MSG_START:
		output = start_recognition_msg(input->module_id);
		break;
	case WAKEUP_CHN_MSG_PARAMETER_SET:
		output = parameter_set_msg(input->module_id, &input->set_param);
		break;
	case WAKEUP_CHN_MSG_PARAMETER_GET:
		get_handle_msg(&output);
		break;
	case WAKEUP_CHN_MSG_STOP:
		output = stop_recognition_msg(input->module_id);
		break;
	default:
		/* do nothing */
		loge("message type error. %x \n", input->msg_type);
		ret = ERROR;
		break;
	}

	if (try_copy_to_user((void __user *)para_addr_out, &output, sizeof(output))) {
		loge("copy data to user fail\n");
		ret = ERROR;
	}

error1:
	if (input)
		kfree(input);

	OUT_FUNCTION;
	return ret;
}




static int hifi_misc_open(struct inode *finode, struct file *fd)
{
	logi("open device\n");
	return OK;
}


static int hifi_misc_release(struct inode *finode, struct file *fd)
{
	logi("close device\n");
	return OK;
}


static long hifi_misc_ioctl(struct file *fd,
							unsigned int cmd,
							unsigned long arg)
{
	int ret = OK;
	void __user *data32 = (void __user *)arg;

	IN_FUNCTION;

	if (!(void __user *)arg) {
		loge("Input buff is NULL\n");
		OUT_FUNCTION;
		return (long)-EINVAL;
	}

	if (!is_hifi_loaded()) {
		loge("hifi isn't loaded\n");
		return (long)-EINVAL;
	}

	/*cmd命令处理*/
	switch(cmd) {
		case HIFI_MISC_IOCTL_ASYNCMSG/*异步命令*/:
			logd("ioctl: HIFI_MISC_IOCTL_ASYNCMSG\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_async_cmd((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		case HIFI_MISC_IOCTL_SYNCMSG/*同步命令*/:
			logd("ioctl: HIFI_MISC_IOCTL_SYNCMSG\n");
			ret = down_interruptible(&s_misc_sem);
			if (ret != 0)
			{
				loge("SYNCMSG wake up by other irq err:%d\n",ret);
				goto out;
			}
			ret = hifi_dsp_sync_cmd((unsigned long)data32);
			up(&s_misc_sem);
			break;

		case HIFI_MISC_IOCTL_GET_PHYS/*获取*/:
			logd("ioctl: HIFI_MISC_IOCTL_GET_PHYS\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_get_phys_cmd((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		case HIFI_MISC_IOCTL_WRITE_PARAMS : /* write algo param to hifi*/
			logi("ioctl: HIFI_MISC_IOCTL_WRITE_PARAMS\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_write_param((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		case HIFI_MISC_IOCTL_AUDIO_EFFECT_PARAMS :  /* write audio effect param to hifi*//*lint -e30  -e142*/
			logi("ioctl: HIFI_MISC_IOCTL_AUDIO_EFFECT_PARAMS\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_write_audio_effect_param((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		case HIFI_MISC_IOCTL_DUMP_HIFI:
			logi("ioctl: HIFI_MISC_IOCTL_DUMP_HIFI\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_dump_hifi((void __user *)arg);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;


		case HIFI_MISC_IOCTL_GET_VOICE_BSD_PARAM:
			logi("ioctl:HIFI_MISC_IOCTL_GET_VOICE_BSD_PARAM\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_om_get_voice_bsd_param(data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		case HIFI_MISC_IOCTL_WAKEUP_THREAD:
			logi("ioctl: HIFI_MISC_IOCTL_WAKEUP_THREAD\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_wakeup_read_thread((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		case HIFI_MISC_IOCTL_WAKEUP_PCM_READ_THREAD: /*lint !e30 !e142*/
			logi("ioctl: HIFI_MISC_IOCTL_WAKEUP_PCM_READ_THREAD\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_wakeup_pcm_read_thread((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		case HIFI_MISC_IOCTL_USBAUDIO:
			logi("ioctl: HIFI_MISC_IOCTL_USBAUDIO\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_usbaudio_cmd((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		case HIFI_MISC_IOCTL_SMARTPA_PARAMS:  /* write smartpakit param to hifi*//*lint -e30  -e142*/
			logi("ioctl: HIFI_MISC_IOCTL_SMARTPA_PARAMS\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_write_smartpa_param((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;



		case HIFI_MISC_IOCTL_SOUNDTRIGGER:
			logi("ioctl: HIFI_MISC_IOCTL_SOUNDTRIGGER\n");
			mutex_lock(&s_misc_data.ioctl_mutex);
			ret = hifi_dsp_soundtrigger_cmd((unsigned long)data32);
			mutex_unlock(&s_misc_data.ioctl_mutex);
			break;

		default:
			/*打印无该CMD类型*/
			ret = (long)ERROR;
			loge("ioctl: Invalid CMD 0x%x\n", (unsigned int)cmd);
			break;
	}
out:
	OUT_FUNCTION;
	return (long)ret;
}

static long hifi_misc_ioctl32(struct file *fd,
							unsigned int cmd,
							unsigned long arg)
{
	void *user_ptr = compat_ptr(arg);
	return hifi_misc_ioctl(fd, cmd, (unsigned long)user_ptr);
}

static int hifi_misc_mmap(struct file *file, struct vm_area_struct *vma)
{
	int ret = OK;
	unsigned long phys_page_addr = 0;
	unsigned long size = 0;
	IN_FUNCTION;

	if (NULL == (void *)vma) {
		logd("input error: vma is NULL\n");
		return ERROR;
	}

	phys_page_addr = (u64)s_misc_data.hifi_priv_base_phy >> PAGE_SHIFT;
	size = ((unsigned long)vma->vm_end - (unsigned long)vma->vm_start);
	logd("vma 0x%pK.\n", vma);
	logd("size %ld, vma->vm_start %pK, end %pK\n", size,
		 (void *)(unsigned long)vma->vm_start, (void *)(unsigned long)vma->vm_end);
	logd("phys_page_addr 0x%pK\n", (void *)phys_page_addr);

	if (size > HIFI_MUSIC_DATA_SIZE) {
		loge("size error, size:%lu\n", size);
		return ERROR;
	}

	vma->vm_page_prot = pgprot_writecombine(PAGE_SHARED);

	ret = remap_pfn_range(vma,
					vma->vm_start,
					phys_page_addr,
					size,
					vma->vm_page_prot);
	if (ret != 0)
	{
		loge("remap_pfn_range ret %d\n", ret);
		return ERROR;
	}

	OUT_FUNCTION;
	return ret;
}


static ssize_t hifi_misc_proc_read(struct file *file, char __user *buf,
								   size_t count, loff_t *ppos)
{
	static unsigned int len = 0;
	static int retry_cnt = 0;
	static char *to_user_buf = NULL;
	int ret = -EAGAIN;
	struct recv_request *recv = NULL;
	struct misc_recmsg_param *recmsg = NULL;

	IN_FUNCTION;

	if (NULL == buf) {
		loge("input error: buf is NULL\n");
		return -EINVAL;
	}

	if (!is_hifi_loaded()) {
		loge("hifi isn't loaded\n");
		return -ENXIO;
	}

	mutex_lock(&s_misc_data.proc_read_mutex);

	if (to_user_buf)
		goto RETRY;

	if (list_empty(&recv_proc_work_queue_head)) {
		/* wait wake_up() has to be called after change the result of the wait condition. */
		ret = wait_event_interruptible(s_misc_data.proc_waitq, s_misc_data.wait_flag!=0);
		if (ret) {
			logi("wait interrupted by a signal, ret: %d\n", ret);
			mutex_unlock(&s_misc_data.proc_read_mutex);
			return ret;
		}
		logi("wait_event_interruptible success\n");
	}

	/* locked recv_proc_lock */
	spin_lock_bh(&s_misc_data.recv_proc_lock);

	if (likely(s_misc_data.wait_flag > 0)) {
		s_misc_data.wait_flag--;
	}

	if (!list_empty(&recv_proc_work_queue_head)) {
		recv = list_entry(recv_proc_work_queue_head.next, struct recv_request, recv_node);
		len = recv->rev_msg.mail_buff_len;
		recmsg = (struct misc_recmsg_param*)recv->rev_msg.mail_buff;

		if (unlikely(len >= MAIL_LEN_MAX || len <= SIZE_CMD_ID || !recmsg)) {
			loge("buff size is invalid: %u(>= 512 or <= 8) or recmsg is null\n", len);
			ret = -EINVAL;
		} else {
			len -= SIZE_CMD_ID;
			to_user_buf = kzalloc(len, GFP_ATOMIC);
			if (!to_user_buf) {
				loge("kzalloc failed\n");
				ret = -ENOMEM;
				goto ERR;
			}
			memcpy(to_user_buf, recv->rev_msg.mail_buff, len);/* unsafe_function_ignore: memcpy */
		}

		list_del(&recv->recv_node);
		kfree(recv->rev_msg.mail_buff);
		kfree(recv);
		recv = NULL;
	} else {
		loge("recv proc list is empty\n");
	}

	/* release recv_proc_lock */
	spin_unlock_bh(&s_misc_data.recv_proc_lock);

RETRY:
	if (to_user_buf) {
		recmsg = (struct misc_recmsg_param*)to_user_buf;
		logi("msgid: 0x%x, len: %u, "
			"play status : %d (0 - done normal, 1 - done complete, 2 - done abnormal, 3 - reset)\n",
			recmsg->msgID, len, recmsg->playStatus);

		if (len > count) {
			loge("copy len[%u] bigger than count[%zu]\n", len, count);
			kzfree(to_user_buf);
			to_user_buf = NULL;
			mutex_unlock(&s_misc_data.proc_read_mutex);
			return -EINVAL;
		}

		ret = (int)try_copy_to_user(buf, to_user_buf, len);
		if (ret != 0) {
			loge("copy to user fail, ret:%d, len:%u, retry_cnt:%d\n",ret, len, retry_cnt);
			retry_cnt++;
			ret = -EFAULT;
		}

		if (0 == ret || retry_cnt >= RETRY_COUNT) {
			kzfree(to_user_buf);
			to_user_buf = NULL;
			retry_cnt = 0;
			len = 0;
		}
	}

	mutex_unlock(&s_misc_data.proc_read_mutex);
	OUT_FUNCTION;
	return ret;

ERR:
	spin_unlock_bh(&s_misc_data.recv_proc_lock);
	mutex_unlock(&s_misc_data.proc_read_mutex);
	return ret;

}

static ssize_t hifi_misc_pcm_read(struct file *file, char __user *buf,
								   size_t count, loff_t *ppos)
{
	int ret = 0;

	(void)file;
	(void)ppos;

	if (!is_hifi_loaded()) {
		loge("hifi isn't loaded\n");
		return -ENXIO;
	}

	if (!buf) {
		loge("param err, buf is null\n");
		return -EINVAL;
	}

	if (0 == count || count > HIFI_PCM_UPLOAD_BUFFER_SIZE) {
		loge("param err, count = %zu\n", count);
		return -EINVAL;
	}

	/* wait_event_interruptible(wq, condition);
	 * The function will return -ERESTARTSYS if it was interrupted by a signal,
	 * and 0 if condition evaluated to true.
	 */
	ret = wait_event_interruptible(s_misc_data.pcm_read_waitq, s_misc_data.pcm_read_wait_flag); /*lint !e40 !e578 !e712 !e774 !e845*/
	if (ret) {
		loge("pcm read wait interrupted, 0x%x\n", ret);
		return -EBUSY;
	}

	spin_lock_bh(&s_misc_data.pcm_read_lock);

	s_misc_data.pcm_read_wait_flag = false;

	spin_unlock_bh(&s_misc_data.pcm_read_lock);

	if (try_copy_to_user(buf, s_misc_data.hifi_priv_base_virt + (HIFI_PCM_UPLOAD_BUFFER_ADDR - HIFI_UNSEC_BASE_ADDR), count)) {
		loge("pcm read copy_to_user fail\n");
		return -EFAULT;
	}

	return (ssize_t)count;
}

static const struct file_operations hifi_misc_fops = {
	.owner			= THIS_MODULE,
	.open			= hifi_misc_open,
	.release		= hifi_misc_release,
	.unlocked_ioctl = hifi_misc_ioctl,
	.compat_ioctl   = hifi_misc_ioctl32,
	.mmap			= hifi_misc_mmap,
};

static struct miscdevice hifi_misc_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "hifi_misc",
	.fops	= &hifi_misc_fops,
};

static const struct file_operations hifi_proc_fops = {
	.owner = THIS_MODULE,
	.read = hifi_misc_proc_read,
};

static const struct file_operations hifi_pcm_read_fops = {
	.owner = THIS_MODULE, /*lint !e64*/
	.read = hifi_misc_pcm_read,
}; /*lint !e785*/

static void hifi_misc_proc_init( void )
{
	struct proc_dir_entry * hifi_misc_dir;
	struct proc_dir_entry * entry_hifi;
	struct proc_dir_entry * entry_hifi_pcm_read;

	hifi_misc_dir = proc_mkdir(FILE_PROC_DIRECTORY, NULL);
	if (hifi_misc_dir == NULL) {
		loge("Unable to create /proc/hifidsp directory\n");
		return ;
	}

	/* Creating read/write "status" entry */
	entry_hifi = proc_create("hifi", 0440, hifi_misc_dir, &hifi_proc_fops);
	if (!entry_hifi) {
		loge("Unable to create /proc/hifidsp/hifi entry\n");
	}

	entry_hifi_pcm_read = proc_create("hifi_pcm_read", 0440, hifi_misc_dir, &hifi_pcm_read_fops);
	if (!entry_hifi_pcm_read) {
		loge("Unable to create /proc/hifidsp/hifi_pcm_read entry\n");
	}

	if (!entry_hifi && !entry_hifi_pcm_read) {
		/* void remove_proc_entry(const char *name, struct proc_dir_entry *parent);
		 * remove a /proc entry and free it if it's not currently in use.
		 */
		remove_proc_entry(FILE_PROC_DIRECTORY, NULL);
		logi("remove /proc/hifidsp directory\n");
	}

	return;
}

static int hifi_sr_event(struct notifier_block *this,
		unsigned long event, void *ptr) {
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		logi("resume +.\n");
		atomic_set(&s_hifi_in_suspend, 0);/*lint !e446*/
		logi("resume -.\n");
		break;

	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		logi("suspend +.\n");
		atomic_set(&s_hifi_in_suspend, 1);/*lint !e446*/
		while (true) {
			if (atomic_read(&s_hifi_in_saving)) {
				msleep(100);
			} else {
				break;
			}
		}
		logi("suspend -.\n");
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static int hifi_reboot_notifier(struct notifier_block *nb,
		unsigned long foo, void *bar)
{
	logi("reboot +.\n");
	atomic_set(&s_hifi_in_suspend, 1);/*lint !e446*/
	while (true) {
		if (atomic_read(&s_hifi_in_saving)) {
			msleep(100);
		} else {
			break;
		}
	}
	logi("reboot -.\n");

	return 0;
}

void hifi_get_log_signal(void)
{
	while (true) {
		if (atomic_read(&s_hifi_in_suspend)) {
			msleep(100);
		} else {
			atomic_set(&s_hifi_in_saving, 1);/*lint !e446*/
			break;
		}
	}
	return;
}

void hifi_release_log_signal(void)
{
	atomic_set(&s_hifi_in_saving, 0);/*lint !e446*/
}

int hifi_send_msg(unsigned int mailcode, void *data, unsigned int length)
{
	if (is_hifi_loaded()) {
		return (unsigned int)mailbox_send_msg(mailcode, data, length);
	}
	return -1;
}
EXPORT_SYMBOL(hifi_send_msg);

static int hifi_misc_send_hifi_msg_async(struct common_hifi_cmd* cmd)
{
	int ret = OK;
	logi("send msg: 0x%x to hifi !\n", cmd->msg_id);
	ret = (unsigned int)mailbox_send_msg(MAILBOX_MAILCODE_ACPU_TO_HIFI_MISC, cmd, sizeof(struct common_hifi_cmd));
	if (OK != ret) {
		loge("msg: 0x%x send to hifi fail, ret is %d.\n", cmd->msg_id, ret);
	}

	return ret;
}

extern void hi6402_3mic_audio_clk(int mode);
extern void hi6402_3mic_fade_out(void);
/* 3mic add for reset hi6402 audio clk */
void reset_audio_clk_work(struct work_struct *work)
{
	struct common_hifi_cmd cmd_cnf;
	struct common_hifi_cmd tmp_mesg;
	struct common_hifi_cmd* mesg = &(tmp_mesg);
	struct dp_clk_request* dp_clk_cmd = NULL;
	unsigned short msg_id =  0;

	while (!list_empty(&s_misc_data.multi_mic_ctrl.cmd_queue)) {
		memset(&cmd_cnf, 0, sizeof(struct common_hifi_cmd));/* unsafe_function_ignore: memset */
		memset(mesg, 0, sizeof(struct common_hifi_cmd));/* unsafe_function_ignore: memset */

		spin_lock_bh(&s_misc_data.multi_mic_ctrl.cmd_lock);

		if (!list_empty(&s_misc_data.multi_mic_ctrl.cmd_queue)) {
			dp_clk_cmd = list_entry(s_misc_data.multi_mic_ctrl.cmd_queue.next, struct dp_clk_request, dp_clk_node);

			if (NULL == dp_clk_cmd) {
				loge("request is NULL.\n");
				spin_unlock_bh(&s_misc_data.multi_mic_ctrl.cmd_lock);
				return;
			} else {
				memcpy(mesg, &(dp_clk_cmd->dp_clk_msg), sizeof(struct common_hifi_cmd));/* unsafe_function_ignore: memcpy */
			}

			list_del(&dp_clk_cmd->dp_clk_node);
			kfree(dp_clk_cmd);
			dp_clk_cmd = NULL;
		} else {
			logw("list is empty!\n");
			spin_unlock_bh(&s_misc_data.multi_mic_ctrl.cmd_lock);
			return;
		}
		spin_unlock_bh(&s_misc_data.multi_mic_ctrl.cmd_lock);

		msg_id =  mesg->msg_id;

		logi("%s++,mesg[0x%x],value[0x%x],reserve[0x%x]\n", __FUNCTION__, mesg->msg_id, mesg->value, mesg->reserve);

		switch(msg_id) {
			case ID_AUDIO_AP_DP_CLK_EN_IND:
			{
				int audio_clk_state = s_misc_data.multi_mic_ctrl.audio_clk_state;

				/* reset hi6402 audio dp clk */
				if((audio_clk_state & HI6402_DP_CLK_ON) != (mesg->value & HI6402_DP_CLK_ON)) {
					hi6402_3mic_audio_clk(mesg->value);
					s_misc_data.multi_mic_ctrl.audio_clk_state = mesg->value;
				}

				/* send ack to hifi */
				if ((mesg->value & HI6402_DP_CLK_ON) == HI6402_DP_CLK_ON) {
					cmd_cnf.reserve = HI6402_DP_CLK_ON;
				} else {
					cmd_cnf.reserve = HI6402_DP_CLK_OFF;
				}

				cmd_cnf.msg_id = ID_AP_AUDIO_DP_CLK_STATE_IND;
				cmd_cnf.value = mesg->reserve;
				hifi_misc_send_hifi_msg_async(&cmd_cnf);
			}
			break;

			case ID_AUDIO_AP_FADE_OUT_REQ:
			{
				hi6402_3mic_fade_out();
				cmd_cnf.msg_id = ID_AP_AUDIO_FADE_OUT_IND;
				cmd_cnf.value = mesg->value;
				hifi_misc_send_hifi_msg_async(&cmd_cnf);

			}
			break;

			default:
			{
				loge("error msg:0x%x\n", msg_id);
			}
			break;
		}
	}

	if(!list_empty(&s_misc_data.multi_mic_ctrl.cmd_queue)){
		pr_info("%s have other cmd in list\n",__FUNCTION__);
	}

	logi("%s--\n",__FUNCTION__);
}

static void hifi_misc_set_platform_type(struct platform_device *pdev, struct hifi_misc_priv *priv)
{
	const char *platform_type;

	if (!pdev || !priv) {
		printk("param error\n");
		return;
	}

	priv->platform_type = HIFI_DSP_PLATFORM_ASIC;

	if (!of_property_read_string(pdev->dev.of_node, "platform-type", &platform_type)) {
		if (!strncmp(platform_type, "ASIC", strlen("ASIC"))) {
			priv->platform_type = HIFI_DSP_PLATFORM_ASIC;
			printk("platform type is ASIC\n");
		} else if (!strncmp(platform_type, "FPGA", strlen("FPGA"))) {
			priv->platform_type = HIFI_DSP_PLATFORM_FPGA;
			printk("platform type is FPGA\n");
		} else {
			printk("get hifidsp platform type error, set defult type[ASIC]\n");
		}
	} else {
		printk("get hifidsp property fail, set defult type[ASIC]\n");
	}
}

enum hifi_dsp_platform_type hifi_misc_get_platform_type(void)
{
	return s_misc_data.platform_type;
}


static int hifi_misc_probe (struct platform_device *pdev)
{
	int ret = OK;

	IN_FUNCTION;

	printk("hifi pdev name[%s].\n", pdev->name);//can't use logx

	ret = misc_register(&hifi_misc_device);
	if (OK != ret) {
		printk("hifi misc device register fail,ERROR is %d\n", ret);//can't use logx
		return ERROR;
	}

	memset(&s_misc_data, 0, sizeof(struct hifi_misc_priv));/* unsafe_function_ignore: memset */
	s_misc_data.dev = &pdev->dev;

	/* Register to get PM events */
	s_hifi_sr_nb.notifier_call = hifi_sr_event;
	s_hifi_sr_nb.priority = -1;
	(void)register_pm_notifier(&s_hifi_sr_nb);

	s_hifi_reboot_nb.notifier_call = hifi_reboot_notifier;
	s_hifi_reboot_nb.priority = -1;
	(void)register_reboot_notifier(&s_hifi_reboot_nb);

	hifi_misc_set_platform_type(pdev, &s_misc_data);

	s_misc_data.hifi_priv_base_phy = (unsigned char*)HIFI_UNSEC_BASE_ADDR;
	s_misc_data.hifi_priv_base_virt = (unsigned char*)ioremap_wc(HIFI_UNSEC_BASE_ADDR, HIFI_UNSEC_REGION_SIZE);
	if (NULL == s_misc_data.hifi_priv_base_virt) {
		printk("hifi ioremap_wc error\n");//can't use logx
		goto err1;
	}

	hifi_om_init(pdev, s_misc_data.hifi_priv_base_virt, s_misc_data.hifi_priv_base_phy);

	//clear nv data region
	memset(s_misc_data.hifi_priv_base_virt + (HIFI_AP_NV_DATA_ADDR - HIFI_UNSEC_BASE_ADDR), 0, HIFI_AP_NV_DATA_SIZE);/* unsafe_function_ignore: memset */
	memset(s_misc_data.hifi_priv_base_virt + (PCM_PLAY_BUFF_LOCATION - HIFI_UNSEC_BASE_ADDR), 0, PCM_PLAY_BUFF_SIZE);/* unsafe_function_ignore: memset */

	hifi_misc_proc_init();

	/*初始化接受信号量*/
	spin_lock_init(&s_misc_data.recv_sync_lock);
	spin_lock_init(&s_misc_data.recv_proc_lock);
	spin_lock_init(&s_misc_data.pcm_read_lock);

	mutex_init(&s_misc_data.ioctl_mutex);
	mutex_init(&s_misc_data.proc_read_mutex);

	/*初始化同步信号量*/
	init_completion(&s_misc_data.completion);

	/*初始化读文件信号量*/
	init_waitqueue_head(&s_misc_data.proc_waitq);
	s_misc_data.wait_flag = 0;

	init_waitqueue_head(&s_misc_data.pcm_read_waitq);
	s_misc_data.pcm_read_wait_flag = false;

	s_misc_data.sn = 0;

	wake_lock_init(&s_misc_data.hifi_misc_wakelock,WAKE_LOCK_SUSPEND, "hifi_wakelock");
	wake_lock_init(&s_misc_data.update_buff_wakelock, WAKE_LOCK_SUSPEND, "update_buff_wakelock");

	ret = DRV_IPCIntInit();
	if (OK != ret) {
		loge("hifi ipc init fail.\n");
		goto err2;
	}
	ret = (int)mailbox_init();
	if (OK != ret) {
		loge("hifi mailbox init fail.\n");
		goto err2;
	}

	/* init 3mic reset clk workqueue */
	/* 3mic add for reset hi6402 audio dp clk
	 * init 3mic reset clk workqueue
	 */
	s_misc_data.multi_mic_ctrl.reset_audio_dp_clk_wq =
		create_singlethread_workqueue("multi_mic_reset_clk_wq");
	if (!(s_misc_data.multi_mic_ctrl.reset_audio_dp_clk_wq)) {
		pr_err("%s(%u) : workqueue create failed", __FUNCTION__,__LINE__);
		goto err3;
	}
	INIT_WORK(&s_misc_data.multi_mic_ctrl.reset_audio_dp_clk_work, reset_audio_clk_work);
	s_misc_data.multi_mic_ctrl.audio_clk_state = HI6402_DP_CLK_ON;
	INIT_LIST_HEAD(&s_misc_data.multi_mic_ctrl.cmd_queue);
	spin_lock_init(&(s_misc_data.multi_mic_ctrl.cmd_lock));
	/*注册双核通信处理函数*/
	ret = mailbox_reg_msg_cb(MAILBOX_MAILCODE_HIFI_TO_ACPU_MISC, (mb_msg_cb)hifi_misc_handle_mail, NULL);

	if (OK != ret) {
		loge("hifi mailbox handle func register fail.\n");
		goto err3;
	}

	OUT_FUNCTION;
	return OK;

err3:
	if (NULL != s_misc_data.hifi_priv_base_virt) {
		iounmap(s_misc_data.hifi_priv_base_virt);
		s_misc_data.hifi_priv_base_virt = NULL;
	}

	if(s_misc_data.multi_mic_ctrl.reset_audio_dp_clk_wq) {
		flush_workqueue(s_misc_data.multi_mic_ctrl.reset_audio_dp_clk_wq);
		destroy_workqueue(s_misc_data.multi_mic_ctrl.reset_audio_dp_clk_wq);
	}

err2:
	wake_lock_destroy(&s_misc_data.hifi_misc_wakelock);
	wake_lock_destroy(&s_misc_data.update_buff_wakelock);
	mutex_destroy(&s_misc_data.ioctl_mutex);
	mutex_destroy(&s_misc_data.proc_read_mutex);

	hifi_om_deinit(pdev);

err1:
	unregister_pm_notifier(&s_hifi_sr_nb);
	unregister_reboot_notifier(&s_hifi_reboot_nb);

	(void)misc_deregister(&hifi_misc_device);

	OUT_FUNCTION;
	return ERROR;
}


static int hifi_misc_remove(struct platform_device *pdev)
{
	IN_FUNCTION;

	hifi_om_deinit(pdev);

	if (NULL != s_misc_data.hifi_priv_base_virt) {
		iounmap(s_misc_data.hifi_priv_base_virt);
		s_misc_data.hifi_priv_base_virt = NULL;
	}

	/* Unregister for PM events */
	unregister_pm_notifier(&s_hifi_sr_nb);
	unregister_reboot_notifier(&s_hifi_reboot_nb);

	/* misc deregister*/
	(void)misc_deregister(&hifi_misc_device);

	/* wake lock destroy */
	wake_lock_destroy(&s_misc_data.hifi_misc_wakelock);
	wake_lock_destroy(&s_misc_data.update_buff_wakelock);
	mutex_destroy(&s_misc_data.ioctl_mutex);
	mutex_destroy(&s_misc_data.proc_read_mutex);

	OUT_FUNCTION;
	return OK;
}

static const struct of_device_id hifi_match_table[] = {
	{
		.compatible = DTS_COMP_HIFIDSP_NAME,
		.data = NULL,
	},
	{}
};
MODULE_DEVICE_TABLE(of, hifi_match_table);

static struct platform_driver hifi_misc_driver = {
	.driver =
	{
		.name  = "hifi_dsp_misc",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hifi_match_table),
	},
	.probe	= hifi_misc_probe,
	.remove = hifi_misc_remove,
};

static int __init hifi_misc_init(void)
{
	return platform_driver_register(&hifi_misc_driver);
}

static void __exit hifi_misc_exit(void)
{
	platform_driver_unregister(&hifi_misc_driver);
}

fs_initcall_sync(hifi_misc_init);
module_exit(hifi_misc_exit);

MODULE_DESCRIPTION("hifi driver");
MODULE_LICENSE("GPL");

