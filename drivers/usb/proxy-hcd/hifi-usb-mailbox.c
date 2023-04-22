#include <linux/errno.h>
#include <linux/usb/hifi-usb-mailbox.h>
#include <linux/hisi/hisi_rproc.h>

#include "hifi-usb.h"
#include "../dwc3/dwc3-hifi-usb.h"

static irq_rt_t usb_notify_recv_isr(void *usr_para, void *mail_handle, unsigned int mail_len)
{
	struct hifi_usb_op_msg mail_buf;
	unsigned int ret = MAILBOX_OK;
	unsigned int mail_size = mail_len;

	if ((mail_len == 0) || (mail_len > sizeof(struct hifi_usb_op_msg)))
		return IRQ_NH_TYPE;

	memset(&mail_buf, 0, sizeof(struct hifi_usb_op_msg));

	ret = DRV_MAILBOX_READMAILDATA(mail_handle, (unsigned char*)&mail_buf, &mail_size);
	if ((ret != MAILBOX_OK)
			|| (mail_size == 0)
			|| (mail_size > sizeof(struct hifi_usb_op_msg))) {
		pr_err("Empty point or data length error! size: %d ret:%d max size:%lu\n",
				mail_size, ret, sizeof(struct hifi_usb_op_msg));
				  return IRQ_NH_MB;
	}

	if (mail_buf.msg_id < ID_AP_HIFI_USB_RUNSTOP &&
			mail_buf.msg_id > ID_AP_USE_HIFI_USB) {
		pr_err("msg_id err %d\n", mail_buf.msg_id);
		return IRQ_NH_TYPE;
	}

	ap_use_hifi_usb_msg_receiver((void *)&mail_buf);

	hifi_usb_msg_receiver(&mail_buf);

	return IRQ_HDD;
}

static int usb_notify_isr_register(irq_hdl_t pisr)
{
	int ret = 0;
	unsigned int mailbox_ret = MAILBOX_OK;

	pr_info("register isr for usb channel !\n");
	if (NULL == pisr) {
		pr_err("pisr==NULL!\n");
		ret = -EINVAL;
	} else {
		/* Micro "MAILBOX_MAILCODE_HIFI_TO_ACPU_USB" defined in
		 * drivers/hisi/hifi_mailbox/mailbox/drv_mailbox_cfg.h */
		mailbox_ret = DRV_MAILBOX_REGISTERRECVFUNC(
				MAILBOX_MAILCODE_HIFI_TO_ACPU_USB,
				(void *)pisr, NULL); /*lint !e611 */
		if (MAILBOX_OK != mailbox_ret) {
			ret = -ENOENT;
			pr_err("register isr for usb channel failed, ret : %d,0x%x\n",
					ret, MAILBOX_MAILCODE_HIFI_TO_ACPU_USB);
		}
	}

	return ret;
}

int hifi_usb_send_mailbox(void *op_msg, unsigned int len)
{
	int count = 100;

	while (RPROC_IS_SUSPEND(HISI_RPROC_HIFI_MBX18) && count) {
		msleep(10);
		pr_err("[%s] mailbox is suspend,need wait\n", __func__);
		count --;
	}

	if (count ==0) {
		pr_err("[%s] mailbox is always suspend\n", __func__);
		return -ESHUTDOWN;
	}

	/* Micro "MAILBOX_MAILCODE_ACPU_TO_HIFI_USB_RT" defined in
	 * drivers/hisi/hifi_mailbox/mailbox/drv_mailbox_cfg.h */
	return mailbox_send_msg(MAILBOX_MAILCODE_ACPU_TO_HIFI_USB_RT,
			op_msg, len);
}

int hifi_usb_mailbox_init(void)
{
	int ret = 0;

	pr_info("hifi_usb_mailbox_init\n");

	/*  register mailbox recv isr */
	ret = usb_notify_isr_register((void*)usb_notify_recv_isr); /*lint !e611 */
	if (ret)
		pr_err("usb_notify_isr_register failed : %d\n", ret);

	return ret;
}

void hifi_usb_mailbox_exit(void)
{
}
