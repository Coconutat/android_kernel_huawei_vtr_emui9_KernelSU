#include <linux/proc_fs.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/wakelock.h>
#include <linux/errno.h>
#include "../hi64xx_dsp/hi64xx_algo_interface.h"
#include "soundtrigger_socdsp_mailbox.h"
#include "hifi_lpp.h"
#include "soundtrigger_socdsp_pcm.h"
#include "soundtrigger_event.h"

#include "drv_mailbox_cfg.h"

struct completion start_msg_complete;
struct completion stop_msg_complete;
struct completion parameter_set_msg_complete;
struct completion parameter_get_msg_complete;
struct wake_lock soundtrigger_rcv_wake_lock;

#define SOCDSP_WAKEUP_MSG_TIMEOUT (2 * HZ)
#define NORMAL_BUFFER_LEN (640) //16K*1ch*2byte*20ms

struct soundtrigger_rcv_msg {
	WAKEUP_CHN_COMMON
	union {
		int socdsp_handle;
		unsigned int fast_len;
		struct wakeup_period_elapsed elapsed;
	};
	char *elapsed_buffer;
};

static int g_socdsp_handle = 0;
char *g_model_buf = NULL;

static void soundtrigger_hotword_detected_work(struct work_struct *wk)
{
	hw_soundtrigger_event_uevent(0);
}
static DECLARE_WORK(hotword_detected_queue, soundtrigger_hotword_detected_work);

int soundtrigger_mailbox_send_data(void *pmsg_body, unsigned int msg_len, unsigned int msg_priority)
{
	unsigned int ret = 0;

	ret = DRV_MAILBOX_SENDMAIL(MAILBOX_MAILCODE_ACPU_TO_HIFI_WAKEUP, pmsg_body, msg_len);
	if (MAILBOX_OK != ret) {
		pr_err("soundtrigger channel send mail failed, ret=%d\n", ret);
	}

	return (int)ret;
}

static irq_rt_t soundtrigger_mailbox_recv_isr(void *usr_para, void *mail_handle, unsigned int mail_len)
{
	struct soundtrigger_rcv_msg rcv_msg;
	unsigned int ret = MAILBOX_OK;
	unsigned int mail_size = mail_len;
	memset(&rcv_msg, 0, sizeof(rcv_msg));/* unsafe_function_ignore: memset */

	ret = DRV_MAILBOX_READMAILDATA(mail_handle, (unsigned char*)&rcv_msg, &mail_size);
	if ((ret != MAILBOX_OK)
		|| (mail_size == 0)
		|| (mail_size > sizeof(rcv_msg))) {
		pr_err("Empty point or data length error! size: %d  ret:%d sizeof(struct soundtrigger_rcv_msg):%lu\n",
						mail_size, ret, sizeof(rcv_msg));
		return IRQ_NH_MB;
	}

	wake_lock_timeout(&soundtrigger_rcv_wake_lock, msecs_to_jiffies(1000));
	switch(rcv_msg.msg_type) {
	case WAKEUP_CHN_MSG_START_ACK:
		pr_info("receive message: start succ.\n");
		complete(&start_msg_complete);
		break;
	case WAKEUP_CHN_MSG_STOP_ACK:
		pr_info("receive message: stop succ.\n");
		complete(&stop_msg_complete);
		break;
	case WAKEUP_CHN_MSG_PARAMETER_SET_ACK:
		pr_info("receive message: parameter set succ.\n");
		complete(&parameter_set_msg_complete);
		break;
	case WAKEUP_CHN_MSG_PARAMETER_GET_ACK:
		pr_info("receive message: parameter get succ. \n");
		g_socdsp_handle = rcv_msg.socdsp_handle;
		complete(&parameter_get_msg_complete);
		break;
	case WAKEUP_CHN_MSG_HOTWORD_DETECT_RCV:
		pr_info("receive message: hotword detected \n");
		soundtrigger_socdsp_pcm_flag_init();
		soundtrigger_socdsp_pcm_fastbuffer_filled(rcv_msg.fast_len);
		schedule_work(&hotword_detected_queue);
		break;
	case WAKEUP_CHN_MSG_ELAPSED_RCV:
		soundtrigger_socdsp_pcm_elapsed(rcv_msg.elapsed.start, rcv_msg.elapsed.len);
		break;
	default:
		pr_err("msg_type 0x%x.\n", rcv_msg.msg_type);
		break;
	}

	return IRQ_HDD;
}

static int soundtrigger_mailbox_isr_register(irq_hdl_t pisr)
{
	int ret = 0;
	unsigned int mailbox_ret = MAILBOX_OK;

	if (NULL == pisr) {
		pr_err("pisr==NULL!\n");
		ret = ERROR;
	} else {
		mailbox_ret = DRV_MAILBOX_REGISTERRECVFUNC(MAILBOX_MAILCODE_HIFI_TO_ACPU_WAKEUP, (void *)pisr, NULL);/*lint !e611 */
		if (MAILBOX_OK != mailbox_ret) {
			ret = ERROR;
			pr_err("register isr for soundtrigger channel failed, ret : %d,0x%x\n", ret, MAILBOX_MAILCODE_HIFI_TO_ACPU_WAKEUP);
		}
	}

	return ret;
}

int parameter_set_msg(int module_id, struct parameter_set *set_val)
{
	int ret;
	unsigned long retval;
	struct parameter_set_msg set_msg;
	init_completion(&parameter_set_msg_complete);

	memset(&set_msg, 0, sizeof(set_msg));/* unsafe_function_ignore: memset */
	pr_info("parameter_set_msg \n");
	set_msg.msg_type = WAKEUP_CHN_MSG_PARAMETER_SET;
	set_msg.module_id = module_id;
	memcpy(&set_msg.para, set_val, sizeof(*set_val));
	if (set_msg.para.key == MLIB_ST_PARA_MODEL) {
		if (set_val->model.length <= HISI_AP_AUDIO_WAKEUP_MODEL_SIZE) {
			if (g_model_buf) {
				memcpy(g_model_buf, set_val->model.piece, set_val->model.length);
			} else {
				pr_err("g_model_buf is NULL\n");
			}
		} else {
			pr_err("model length exceed %u\n", set_val->model.length);
			return -ETIME;
		}
	}
	ret = soundtrigger_mailbox_send_data(&set_msg, sizeof(set_msg), 0);
	if (ret)
	{
		pr_err("parameter_set_msg: soundtrigger_mailbox_send_data error(%d)\n", ret);
		return ret;
	}

	retval = wait_for_completion_timeout(&parameter_set_msg_complete, SOCDSP_WAKEUP_MSG_TIMEOUT);
	if (retval == 0) {
		pr_err("parameter set msg send timeout\n");
		return -ETIME;
	}

	return ret;
}

int start_recognition_msg(int module_id)
{
	int ret;
	unsigned long retval;
	struct wakeup_start_msg start_msg;
	init_completion(&start_msg_complete);

	memset(&start_msg, 0, sizeof(start_msg));/* unsafe_function_ignore: memset */
	pr_info("start_recognition_msg \n");
	start_msg.msg_type = WAKEUP_CHN_MSG_START;
	start_msg.module_id = module_id;

	ret = soundtrigger_mailbox_send_data(&start_msg, sizeof(start_msg), 0);
	if (ret)
	{
		pr_err("start_recognition_msg: soundtrigger_mailbox_send_data error(%d)\n", ret);
		return ret;
	}

	retval = wait_for_completion_timeout(&start_msg_complete, SOCDSP_WAKEUP_MSG_TIMEOUT);
	if (retval == 0) {
		pr_err("start recognition msg send timeout\n");
		return -ETIME;
	}

	return ret;
}

int stop_recognition_msg(int module_id)
{
	int ret;
	unsigned long retval;
	struct wakeup_stop_msg stop_msg;
	init_completion(&stop_msg_complete);

	memset(&stop_msg, 0, sizeof(stop_msg));/* unsafe_function_ignore: memset */
	pr_info("stop_recognition_msg \n");
	stop_msg.msg_type = WAKEUP_CHN_MSG_STOP;
	stop_msg.module_id = module_id;

	ret = soundtrigger_mailbox_send_data(&stop_msg, sizeof(stop_msg), 0);
	if (ret)
	{
		pr_err("stop_recognition_msg: soundtrigger_mailbox_send_data error(%d)\n", ret);
		return ret;
	}

	retval = wait_for_completion_timeout(&stop_msg_complete, SOCDSP_WAKEUP_MSG_TIMEOUT);
	if (retval == 0) {
		pr_err("start recognition msg send timeout\n");
		return -ETIME;
	}

	return ret;
}

int get_handle_msg(int *socdsp_handle)
{
	int ret;
	unsigned long retval;
	struct parameter_get_msg get_msg;
	init_completion(&parameter_get_msg_complete);

	memset(&get_msg, 0, sizeof(get_msg));/* unsafe_function_ignore: memset */
	pr_info("get_handle_msg \n");
	get_msg.msg_type = WAKEUP_CHN_MSG_PARAMETER_GET;
	g_socdsp_handle = 0;
	ret = soundtrigger_mailbox_send_data(&get_msg, sizeof(get_msg), 0);
	if (ret)
	{
		pr_err("get_handle_msg: soundtrigger_mailbox_send_data error(%d)\n", ret);
		return ret;
	}

	retval = wait_for_completion_timeout(&parameter_get_msg_complete, SOCDSP_WAKEUP_MSG_TIMEOUT);
	if (retval == 0) {
		pr_err("get handle msg send timeout\n");
		return -ETIME;
	} else {
		*socdsp_handle = g_socdsp_handle;
	}

	return ret;
}

int soundtrigger_mailbox_init(void)
{
	int ret = 0;
	pr_info("soundtrigger_mailbox_init \n");

	/* register soundtrigger mailbox message isr */
	ret = soundtrigger_mailbox_isr_register((void*)soundtrigger_mailbox_recv_isr);/*lint !e611 */
	if (ret) {
		pr_err("soundtrigger_mailbox_isr_register failed : %d\n", ret);
		return -EIO;
	}

	wake_lock_init(&soundtrigger_rcv_wake_lock, WAKE_LOCK_SUSPEND, "soundtrigger_rcv_msg");

	g_model_buf = ioremap_wc(HISI_AP_AUDIO_WAKEUP_MODEL_ADDR, HISI_AP_AUDIO_WAKEUP_MODEL_SIZE);
	if (g_model_buf == NULL) {
		pr_err("model buffer ioremap err\n");
		DRV_MAILBOX_REGISTERRECVFUNC(MAILBOX_MAILCODE_HIFI_TO_ACPU_WAKEUP, NULL, NULL);
		return -ENOMEM;
	}

	return ret;
}

void soundtrigger_mailbox_deinit(void)
{
	pr_info("soundtrigger_mailbox_deinit \n");

	if (g_model_buf != NULL) {
		iounmap(g_model_buf);
		g_model_buf = NULL;
	}
	DRV_MAILBOX_REGISTERRECVFUNC(MAILBOX_MAILCODE_HIFI_TO_ACPU_WAKEUP, NULL, NULL);
	wake_lock_destroy(&soundtrigger_rcv_wake_lock);
}

