#include "usbaudio_mailbox.h"
#include <linux/usb.h>
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/wakelock.h>
#include "../hifi/usbaudio_setinterface.h"
#include "../hifi/usbaudio_ioctl.h"
#ifdef CONFIG_HIFI_MAILBOX
#include "drv_mailbox_cfg.h"
#endif
#ifdef CONFIG_HISI_DEBUG_FS
#include <linux/debugfs.h>
#endif
struct completion probe_msg_complete;
struct completion disconnect_msg_complete;
struct completion nv_check;
struct wake_lock rcv_wake_lock;
static atomic_t nv_check_ref = ATOMIC_INIT(0);

#define HIFI_USBAUDIO_MSG_TIMEOUT (2 * HZ)
#define HIFI_USBAUDIO_NV_CHECK_TIMEOUT ((1 * HZ) / 20)

struct interface_set_mesg
{
	unsigned int dir;
	unsigned int running;
	unsigned int rate;
	struct list_head node;
};

struct usbaudio_msg_proc
{
	struct task_struct *kthread;
	struct interface_set_mesg interface_msg_list;
	struct semaphore proc_sema;
	struct wake_lock msg_proc_wake_lock;
	bool kthread_msg_proc_should_stop;
};

static struct usbaudio_msg_proc msg_proc;

static void nv_check_work(struct work_struct *wk)
{
	usbaudio_ctrl_nv_check();
}
static DECLARE_WORK(nv_check_work_queue, nv_check_work);

int usbaudio_mailbox_send_data(void *pmsg_body, unsigned int msg_len, unsigned int msg_priority)
{
	unsigned int ret = 0;

	ret = DRV_MAILBOX_SENDMAIL(MAILBOX_MAILCODE_ACPU_TO_HIFI_USBAUDIO, pmsg_body, msg_len);
	if (MAILBOX_OK != ret) {
		pr_err("usbaudio channel send mail failed, ret=%d\n", ret);
	}

	return (int)ret;
}

static void usbaudio_mailbox_msg_add(unsigned int dir,
				unsigned int running, unsigned int rate)
{
	struct interface_set_mesg *interface_msg;/*lint -e429 */
	interface_msg = kzalloc(sizeof(*interface_msg), GFP_ATOMIC);
	if (interface_msg) {
		interface_msg->running = running;
		interface_msg->dir = dir;
		interface_msg->rate = rate;
		list_add_tail(&interface_msg->node, &msg_proc.interface_msg_list.node);
		up(&msg_proc.proc_sema);
	} else {
		pr_err("%s:malloc fail \n", __FUNCTION__);
	}
}

static irq_rt_t usbaudio_mailbox_recv_isr(void *usr_para, void *mail_handle, unsigned int mail_len)
{
	struct usbaudio_rcv_msg rcv_msg;
	unsigned int ret = MAILBOX_OK;
	unsigned int mail_size = mail_len;
	memset(&rcv_msg, 0, sizeof(struct usbaudio_rcv_msg));/* unsafe_function_ignore: memset */

	ret = DRV_MAILBOX_READMAILDATA(mail_handle, (unsigned char*)&rcv_msg, &mail_size);
	if ((ret != MAILBOX_OK)
		|| (mail_size == 0)
		|| (mail_size > sizeof(struct usbaudio_rcv_msg))) {
		pr_err("Empty point or data length error! size: %d  ret:%d sizeof(struct usbaudio_mailbox_received_msg):%lu\n",
						mail_size, ret, sizeof(struct usbaudio_rcv_msg));
		return IRQ_NH_MB;
	}

	wake_lock_timeout(&rcv_wake_lock, msecs_to_jiffies(1000));
	switch(rcv_msg.msg_type) {
		case USBAUDIO_CHN_MSG_PROBE_RCV:
			pr_info("receive message: probe succ.\n");
			complete(&probe_msg_complete);
			break;
		case USBAUDIO_CHN_MSG_DISCONNECT_RCV:
			pr_info("receive message: disconnect succ.\n");
			complete(&disconnect_msg_complete);
			break;
		case USBAUDIO_CHN_MSG_NV_CHECK_RCV:
			pr_info("receive message: nv check succ.\n");
			atomic_inc(&nv_check_ref);
			schedule_work(&nv_check_work_queue);
			complete(&nv_check);
			break;
		case USBAUDIO_CHN_MSG_PIPEOUTINTERFACE_ON_RCV:
			pr_info("receive message: pipout on \n");
			usbaudio_mailbox_msg_add(DOWNLINK_STREAM, START_STREAM, rcv_msg.ret_val);
			break;
		case USBAUDIO_CHN_MSG_PIPEOUTINTERFACE_OFF_RCV:
			pr_info("receive message: pipout off \n");
			usbaudio_mailbox_msg_add(DOWNLINK_STREAM, STOP_STREAM, rcv_msg.ret_val);
			break;
		case USBAUDIO_CHN_MSG_PIPEININTERFACE_ON_RCV:
			pr_info("receive message: pipein on.\n");
			usbaudio_mailbox_msg_add(UPLINK_STREAM, START_STREAM, rcv_msg.ret_val);
			break;
		case USBAUDIO_CHN_MSG_PIPEININTERFACE_OFF_RCV:
			pr_info("receive message: pipein off.\n");
			usbaudio_mailbox_msg_add(UPLINK_STREAM, STOP_STREAM, rcv_msg.ret_val);
			break;
		default:
			pr_err("msg_type 0x%x.\n", rcv_msg.msg_type);
			break;
	}

	return IRQ_HDD;
}

static int usbaudio_mailbox_isr_register(irq_hdl_t pisr)
{
	int ret = 0;
	unsigned int mailbox_ret = MAILBOX_OK;

	if (NULL == pisr) {
		pr_err("pisr==NULL!\n");
		ret = ERROR;
	} else {
		mailbox_ret = DRV_MAILBOX_REGISTERRECVFUNC(MAILBOX_MAILCODE_HIFI_TO_ACPU_USBAUDIO, (void *)pisr, NULL);/*lint !e611 */
		if (MAILBOX_OK != mailbox_ret) {
			ret = ERROR;
			pr_err("register isr for usbaudio channel failed, ret : %d,0x%x\n", ret, MAILBOX_MAILCODE_HIFI_TO_ACPU_USBAUDIO);
		}
	}

	return ret;
}

int usbaudio_setinterface_complete_msg(int dir,int val, int retval, unsigned int rate)
{
	int ret;
	struct usbaudio_setinterface_complete_msg complete_msg;
	complete_msg.dir = dir;
	complete_msg.val = val;
	complete_msg.rate = rate;
	complete_msg.ret = retval;
	complete_msg.msg_type = USBAUDIO_CHN_MSG_SETINTERFACE_COMPLETE;
	ret = usbaudio_mailbox_send_data(&complete_msg, sizeof(complete_msg), 0);
	return ret;
}

int usbaudio_nv_is_ready(void)
{
	int ret = 1;
	if (atomic_read(&nv_check_ref) >= 1)
		ret = 0;

	return ret;
}

void usbaudio_set_nv_ready(void)
{
	pr_info("nv_check_set \n");
	atomic_set(&nv_check_ref, 0);
	atomic_inc(&nv_check_ref);
}

int usbaudio_nv_check(void)
{
	int ret;
	unsigned long retval;
	struct usbaudio_nv_check_msg nv_check_msg;

	if (atomic_read(&nv_check_ref) >= 1)
		return 0;

	pr_info("nv_check_msg \n");
	init_completion(&nv_check);

	nv_check_msg.msg_type = (unsigned short)USBAUDIO_CHN_MSG_NV_CHECK;

	ret = usbaudio_mailbox_send_data(&nv_check_msg, sizeof(nv_check_msg), 0);
	retval = wait_for_completion_timeout(&nv_check, HIFI_USBAUDIO_NV_CHECK_TIMEOUT);
	if (retval == 0) {
		pr_err("usbaudio nv check timeout\n");
		return -ETIME;
	} else {
		pr_err("usbaudio nv check intime\n");
		ret = INVAILD;
	}

	return ret;
}

int usbaudio_probe_msg(struct usbaudio_pcms *pcms)
{
	int ret;
	int i;
	unsigned long retval;
	struct usbaudio_probe_msg probe_msg;

	memset(&probe_msg, 0, sizeof(struct usbaudio_probe_msg));/* unsafe_function_ignore: memset */
	pr_info("usbaudio_probe_msg, len %lu \n", sizeof(probe_msg));
	init_completion(&probe_msg_complete);

	probe_msg.msg_type = (unsigned short)USBAUDIO_CHN_MSG_PROBE;
	for (i=0; i<USBAUDIO_PCM_NUM; i++) {
		probe_msg.pcms.fmts[i].formats = pcms->fmts[i].formats;
		probe_msg.pcms.fmts[i].channels = pcms->fmts[i].channels;
		probe_msg.pcms.fmts[i].fmt_type = pcms->fmts[i].fmt_type;
		probe_msg.pcms.fmts[i].frame_size = pcms->fmts[i].frame_size;
		probe_msg.pcms.fmts[i].iface = pcms->fmts[i].iface;
		probe_msg.pcms.fmts[i].altsetting = pcms->fmts[i].altsetting;
		probe_msg.pcms.fmts[i].altset_idx = pcms->fmts[i].altset_idx;
		probe_msg.pcms.fmts[i].attributes = pcms->fmts[i].attributes;
		probe_msg.pcms.fmts[i].endpoint = pcms->fmts[i].endpoint;
		probe_msg.pcms.fmts[i].ep_attr = pcms->fmts[i].ep_attr;
		probe_msg.pcms.fmts[i].datainterval = pcms->fmts[i].datainterval;
		probe_msg.pcms.fmts[i].protocol = pcms->fmts[i].protocol;
		probe_msg.pcms.fmts[i].maxpacksize = pcms->fmts[i].maxpacksize;
		probe_msg.pcms.fmts[i].rates = pcms->fmts[i].rates;
		memcpy(probe_msg.pcms.fmts[i].rate_table, pcms->fmts[i].rate_table, sizeof(pcms->fmts[i].rate_table));
		probe_msg.pcms.fmts[i].clock = pcms->fmts[i].clock;

		probe_msg.pcms.ifdesc[i].bLength = pcms->ifdesc[i].bLength;
		probe_msg.pcms.ifdesc[i].bDescriptorType = pcms->ifdesc[i].bDescriptorType;
		probe_msg.pcms.ifdesc[i].bInterfaceNumber = pcms->ifdesc[i].bInterfaceNumber;
		probe_msg.pcms.ifdesc[i].bAlternateSetting = pcms->ifdesc[i].bAlternateSetting;
		probe_msg.pcms.ifdesc[i].bNumEndpoints = pcms->ifdesc[i].bNumEndpoints;
		probe_msg.pcms.ifdesc[i].bInterfaceClass = pcms->ifdesc[i].bInterfaceClass;
		probe_msg.pcms.ifdesc[i].bInterfaceSubClass = pcms->ifdesc[i].bInterfaceSubClass;
		probe_msg.pcms.ifdesc[i].bInterfaceProtocol = pcms->ifdesc[i].bInterfaceProtocol;
		probe_msg.pcms.ifdesc[i].iInterface = pcms->ifdesc[i].iInterface;

		probe_msg.pcms.epdesc[i].bLength = pcms->epdesc[i].bLength;
		probe_msg.pcms.epdesc[i].bDescriptorType = pcms->epdesc[i].bDescriptorType;
		probe_msg.pcms.epdesc[i].bEndpointAddress = pcms->epdesc[i].bEndpointAddress;
		probe_msg.pcms.epdesc[i].bmAttributes = pcms->epdesc[i].bmAttributes;
		probe_msg.pcms.epdesc[i].wMaxPacketSize = pcms->epdesc[i].wMaxPacketSize;
		probe_msg.pcms.epdesc[i].bInterval = pcms->epdesc[i].bInterval;
		probe_msg.pcms.epdesc[i].bRefresh = pcms->epdesc[i].bRefresh;
		probe_msg.pcms.epdesc[i].bSynchAddress = pcms->epdesc[i].bSynchAddress;
	}
	probe_msg.pcms.customsized = pcms->customsized;
	ret = usbaudio_mailbox_send_data(&probe_msg, sizeof(struct usbaudio_probe_msg), 0);

	retval = wait_for_completion_timeout(&probe_msg_complete, HIFI_USBAUDIO_MSG_TIMEOUT);
	if (retval == 0) {
		pr_err("usbaudio probe msg send timeout\n");
		return -ETIME;
	}

	return ret;
}

int usbaudio_disconnect_msg(unsigned int dsp_reset_flag)
{
	int ret;
	unsigned long retval;
	struct usbaudio_disconnect_msg disconnect_msg;

	pr_info("usbaudio_disconnect_msg, reset_flag %d , len %lu \n",dsp_reset_flag, sizeof(disconnect_msg));
	init_completion(&disconnect_msg_complete);
	disconnect_msg.msg_type = (unsigned short)USBAUDIO_CHN_MSG_DISCONNECT;
	disconnect_msg.dsp_reset_flag = dsp_reset_flag;
	ret = usbaudio_mailbox_send_data(&disconnect_msg, sizeof(disconnect_msg), 0);

	retval = wait_for_completion_timeout(&disconnect_msg_complete, HIFI_USBAUDIO_MSG_TIMEOUT);
	if (retval == 0) {
		pr_err("usbaudio disconnect msg send timeout\n");
		return -ETIME;
	}

	return ret;
}

static int interface_msg_proc_thread(void *p)
{
	int ret;
	struct interface_set_mesg *set_mesg = NULL;

	while (!msg_proc.kthread_msg_proc_should_stop) {
		ret = down_interruptible(&msg_proc.proc_sema);
		if (ret == -ETIME) {
			pr_err("proc sema down_int err -ETIME .\n");
		}
		wake_lock(&msg_proc.msg_proc_wake_lock);
		if (list_empty(&msg_proc.interface_msg_list.node)) {
			pr_err("interface_msg_list is empty!\n");
		} else {
			set_mesg = list_entry(msg_proc.interface_msg_list.node.next, struct interface_set_mesg, node);
			if (set_mesg) {
				pr_err("[0:out 1:in]%d [0:start 1:stop]%d \n", set_mesg->dir, set_mesg->running);
				if (set_mesg->dir == 0)
					usbaudio_ctrl_set_pipeout_interface(set_mesg->running, set_mesg->rate);
				else
					usbaudio_ctrl_set_pipein_interface(set_mesg->running, set_mesg->rate);
				list_del(&set_mesg->node);
				kfree(set_mesg);
			} else {
				pr_err("set_mesg is null \n");
			}
		}
		wake_unlock(&msg_proc.msg_proc_wake_lock);
	}

	return 0;
}

int usbaudio_mailbox_init(void)
{
	struct sched_param param;
	int ret = 0;
	pr_info("usbaudio_mailbox_init \n");

	atomic_set(&nv_check_ref, 0); /*lint !e1058 */
	/* register usbaudio mailbox message isr */
	ret = usbaudio_mailbox_isr_register((void*)usbaudio_mailbox_recv_isr);/*lint !e611 */
	if (ret) {
		pr_err("usbaudio_mailbox_isr_register failed : %d\n", ret);
	}

	INIT_LIST_HEAD(&msg_proc.interface_msg_list.node);
	sema_init(&msg_proc.proc_sema, 0);
	wake_lock_init(&msg_proc.msg_proc_wake_lock, WAKE_LOCK_SUSPEND, "usbaudio_msg_proc");
	wake_lock_init(&rcv_wake_lock, WAKE_LOCK_SUSPEND, "usbaudio_rcv_msg");
	msg_proc.kthread_msg_proc_should_stop = false;
	msg_proc.kthread = kthread_create(interface_msg_proc_thread, 0, "interface_msg_proc_thread");

	if (IS_ERR(msg_proc.kthread)) {
		pr_err("create interface_msg_proc_thread fail\n");
		return -ENOMEM;
	}

	/* set high prio */
	memset(&param, 0, sizeof(struct sched_param));
	param.sched_priority = MAX_RT_PRIO - 20;
	ret = sched_setscheduler(msg_proc.kthread, SCHED_RR, &param);
	if (ret)
		pr_err("set thread schedule priorty failed:%d.\n", param.sched_priority);

	wake_up_process(msg_proc.kthread);
	return ret;
}

void usbaudio_mailbox_deinit(void)
{
	struct interface_set_mesg *set_mesg, *n;
	pr_info("usbaudio_mailbox_deinit \n");

	if (msg_proc.kthread) {
		msg_proc.kthread_msg_proc_should_stop = true;
		up(&msg_proc.proc_sema);
		kthread_stop(msg_proc.kthread);
	}

	list_for_each_entry_safe(set_mesg, n, &msg_proc.interface_msg_list.node, node) {
		list_del(&set_mesg->node);
		kfree(set_mesg);
	}

	wake_lock_destroy(&msg_proc.msg_proc_wake_lock);
	wake_lock_destroy(&rcv_wake_lock);
}
