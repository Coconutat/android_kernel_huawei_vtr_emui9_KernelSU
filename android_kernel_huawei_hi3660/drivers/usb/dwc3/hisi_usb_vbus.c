#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/log/log_jank.h>
#include "hisi_usb_vbus.h"

static irqreturn_t charger_connect_interrupt(int irq, void *p)
{
	LOG_JANK_D(JLID_USBCHARGING_START, "JL_USBCHARGING_START");
	hisi_usb_otg_event(CHARGER_CONNECT_EVENT);
	return IRQ_HANDLED;
}

static irqreturn_t charger_disconnect_interrupt(int irq, void *p)
{
	LOG_JANK_D(JLID_USBCHARGING_END, "JL_USBCHARGING_END");
	hisi_usb_otg_event(CHARGER_DISCONNECT_EVENT);
	return IRQ_HANDLED;
}

static inline int usb_vbus_is_error_ops(const struct usb_vbus_ops *vbus_ops)
{
	return (!vbus_ops || !vbus_ops->get_irq_byname);
}

static int vbus_connect_irq, vbus_disconnect_irq;

int hisi_usb_vbus_request_irq(void *pdev, const struct usb_vbus_ops *vbus_ops)
{
	int ret = 0;
	pr_info("[%s]+\n", __func__);

	if (usb_vbus_is_error_ops(vbus_ops)) {
		pr_err("bad usb vbus ops\n");
		return -EINVAL;
	}

	vbus_connect_irq = vbus_ops->get_irq_byname(pdev, "connect");
	if (0 == vbus_connect_irq) {
		pr_err("failed to get connect irq\n");
		return -ENOENT;
	}
	vbus_disconnect_irq = vbus_ops->get_irq_byname(pdev, "disconnect");
	if (0 == vbus_disconnect_irq) {
		pr_err("failed to get disconnect irq\n");
		return -ENOENT;
	}

	pr_info("vbus_connect_irq: %d, vbus_disconnect_irq: %d\n",
			vbus_connect_irq, vbus_disconnect_irq);

	ret = request_irq(vbus_connect_irq, charger_connect_interrupt,
					  IRQF_NO_SUSPEND, "hiusb_in_interrupt", pdev);
	if (ret) {
		pr_err("request charger connect irq failed, irq: %d!\n", vbus_connect_irq);
		return ret;
	}

	ret = request_irq(vbus_disconnect_irq, charger_disconnect_interrupt,
					  IRQF_NO_SUSPEND, "hiusb_in_interrupt", pdev);
	if (ret) {
		free_irq(vbus_disconnect_irq, pdev);
		pr_err("request charger connect irq failed, irq: %d!\n", vbus_disconnect_irq);
		return ret;
	}

	/* avoid lose intrrupt */
	if (hisi_usb_vbus_value()) {
		pr_info("%s: vbus high, issue a charger connect event\n", __func__);
		hisi_usb_otg_event(CHARGER_CONNECT_EVENT);
	} else {
		pr_info("%s: vbus low, issue a charger disconnect event\n", __func__);
		hisi_usb_otg_event(CHARGER_DISCONNECT_EVENT);
	}

	pr_info("[%s]-\n", __func__);

	return ret;
}

int hisi_usb_vbus_free_irq(void *pdev)
{
	free_irq(vbus_connect_irq, pdev);
	free_irq(vbus_disconnect_irq, pdev);
	return 0;
}
