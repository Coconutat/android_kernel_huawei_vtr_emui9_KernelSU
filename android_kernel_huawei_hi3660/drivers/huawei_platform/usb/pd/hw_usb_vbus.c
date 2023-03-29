#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#ifdef CONFIG_CONTEXTHUB_PD
#include <linux/hisi/contexthub/tca.h>
#endif
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/mfd/hisi_pmic.h>
#include <pmic_interface.h>
#include <huawei_platform/log/log_jank.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/direct_charger.h>
#include <huawei_platform/power/huawei_charger.h>
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
int support_pd = 0;
#endif
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_charger.h>
#endif
#ifdef CONFIG_SUPERSWITCH_FSC
bool FUSB3601_in_factory_mode(void);
#endif
#ifndef HWLOG_TAG
#define HWLOG_TAG huawei_usb_vbus
HWLOG_REGIST();
#endif
static int pmic_vbus_attach_enable = 1;
static int hw_vbus_connect_irq, hw_vbus_disconnect_irq;
static bool cc_change = false;
static bool cc_exist = false;
static bool wait_finish = true; /*initial value should be true*/
static bool direct_charge_flag = false;
struct delayed_work g_disconnect_work;
struct delayed_work g_connect_work;
extern struct completion pd_get_typec_state_completion;
#ifdef CONFIG_CONTEXTHUB_PD
extern bool hisi_dptx_ready(void);
static int support_dp = 1;
#endif

static int g_typec_complete_type = NOT_COMPLETE;
static struct wake_lock hwusb_lock;
extern struct mutex typec_state_lock;
extern struct mutex typec_wait_lock;
extern int g_cur_usb_event;
extern int pd_dpm_vbus_notifier_call(struct pd_dpm_info *di, unsigned long event, void *data);
extern struct pd_dpm_info *g_pd_di;
int pmic_vbus_irq_is_enabled(void)
{
	static int need_update_from_dt = 1;
	struct device_node *dn;

	if (!need_update_from_dt)
		return pmic_vbus_attach_enable;

	dn = of_find_compatible_node(NULL, NULL, "huawei,usbvbus");
	if (dn) {
		if(of_property_read_u32(dn, "pmic_vbus_attach_enable", &pmic_vbus_attach_enable))
		{
			hwlog_err("get pmic_vbus_attach_enable fail!\n");
		}

		hwlog_info("pmic_vbus_attach_enable = %d \n",pmic_vbus_attach_enable);
	} else {
		hwlog_err("get device_node fail!\n");
	}
	need_update_from_dt = 0;
	return pmic_vbus_attach_enable;
}
static void hwusb_wake_lock(void)
{
	if (!wake_lock_active(&hwusb_lock)) {
		wake_lock(&hwusb_lock);
		hwlog_info("hwusb wake lock\n");
	}
}
static void hwusb_wake_unlock(void)
{
	if (wake_lock_active(&hwusb_lock)) {
		wake_unlock(&hwusb_lock);
		hwlog_info("hwusb wake unlock\n");
	}
}
#ifdef CONFIG_CONTEXTHUB_PD
void hw_pd_wait_dptx_ready(void)
{
	int count = 10;

	do {
		if(true == hisi_dptx_ready() || !support_dp)
			break;
		msleep(100);
		count--;
	} while(count);

	return;
}
#endif
void reinit_typec_completion(void)
{
	hwlog_info("%s ++\n", __func__);
	mutex_lock(&typec_wait_lock);
	mutex_lock(&typec_state_lock);
	reinit_completion(&pd_get_typec_state_completion);
	g_typec_complete_type = NOT_COMPLETE;
	mutex_unlock(&typec_state_lock);
	mutex_unlock(&typec_wait_lock);
	hwlog_info("%s --\n", __func__);
}
void typec_complete(enum pd_wait_typec_complete typec_completion)
{
	hwlog_info("%s ++\n", __func__);
	mutex_lock(&typec_wait_lock);
	mutex_lock(&typec_state_lock);
	g_typec_complete_type = typec_completion;
	complete(&pd_get_typec_state_completion);
	mutex_unlock(&typec_state_lock);
	mutex_unlock(&typec_wait_lock);
	hwlog_info("%s --\n", __func__);
}
static void send_charger_connect_event(void)
{
#ifdef CONFIG_CONTEXTHUB_PD
	struct pd_dpm_combphy_event event;
	event.dev_type = TCA_CHARGER_CONNECT_EVENT;
	event.irq_type = TCA_IRQ_HPD_IN;
	event.mode_type = TCPC_USB31_CONNECTED;
	event.typec_orien = pd_dpm_get_cc_orientation();

	pd_dpm_handle_combphy_event(event);
#else
	hisi_usb_otg_event(CHARGER_CONNECT_EVENT);
#endif
}

static void send_charger_disconnect_event(void)
{
#ifdef CONFIG_CONTEXTHUB_PD
	struct pd_dpm_combphy_event event;
	event.dev_type = TCA_CHARGER_DISCONNECT_EVENT;
	event.irq_type = TCA_IRQ_HPD_OUT;
	event.mode_type = TCPC_NC;
	event.typec_orien = pd_dpm_get_cc_orientation();
	pd_dpm_handle_combphy_event(event);

#else
	hisi_usb_otg_event(CHARGER_DISCONNECT_EVENT);
#endif
}
static irqreturn_t charger_connect_interrupt(int irq, void *p)
{
	hwlog_info("%s: start\n", __func__);
#ifdef CONFIG_TCPC_CLASS
	/*bugfix for digital headset issue*/
	if (support_pd && pd_dpm_ignore_vbuson_event()) {
		hwlog_info("%s ignore_vbus_on_event\n", __func__);
		pd_dpm_set_ignore_vbuson_event(false);
		hwlog_info("%s: end\n", __func__);
		return IRQ_HANDLED;
	}
	/*ignore vbus event when pd pwr swap happen*/
	if (support_pd && pd_dpm_get_pd_finish_flag()) {
		hwlog_info("%s ignore vbus connect event when pd contract is established\n", __func__);
		hwlog_info("%s: end\n", __func__);
		return IRQ_HANDLED;
	}
#endif
	LOG_JANK_D(JLID_USBCHARGING_START, "JL_USBCHARGING_START");
	schedule_delayed_work(&g_connect_work, msecs_to_jiffies(0));
	hwlog_info("%s: end\n", __func__);
	return IRQ_HANDLED;
}

static void vbus_connect_work(struct work_struct *w)
{
	unsigned long timeout;
	int typec_state = PD_DPM_USB_TYPEC_DETACHED;

	hwlog_info("%s: start\n", __func__);
	hwusb_wake_lock();
#ifdef CONFIG_WIRELESS_CHARGER
	wireless_charger_pmic_vbus_handler(true);
#endif
#ifdef CONFIG_CONTEXTHUB_PD
	hw_pd_wait_dptx_ready();
#endif
	if (!pmic_vbus_attach_enable) {
#ifdef CONFIG_SUPERSWITCH_FSC
		if (!FUSB3601_in_factory_mode()) {
			hwlog_info("%s: pmic_vbus not enabled, end\n", __func__);
		} else {
			hwlog_info("%s: superswitch in factory mode\n", __func__);
			send_charger_connect_event();
			charger_source_sink_event(START_SINK);
		}
#else
		hwlog_info("%s: pmic_vbus not enabled, end\n", __func__);
#endif
		hwusb_wake_unlock();
		return;
	}
	if (direct_charge_flag) {
		cc_change = false;
		cc_exist = false;
		send_charger_connect_event();
		direct_charge_flag = false;
		hwlog_info("%s: in direct charging\n", __func__);
		hwlog_info("%s: end\n", __func__);
		hwusb_wake_unlock();
		return;
	}
	cc_change = false;
	cc_exist = false;
	wait_finish = false;
	timeout = wait_for_completion_timeout(&pd_get_typec_state_completion, msecs_to_jiffies(200));
	mutex_lock(&typec_wait_lock);
	wait_finish = true;
	if (pd_dpm_get_cur_usb_event() == PD_DPM_TYPEC_ATTACHED_AUDIO) {
		g_cur_usb_event = PD_DPM_TYPEC_ATTACHED_AUDIO;
		send_charger_connect_event();
		mutex_unlock(&typec_wait_lock);
		hwusb_wake_unlock();
		hwlog_info("%s: in audioacc charging\n", __func__);
		return;
	}
	if (COMPLETE_FROM_VBUS_DISCONNECT == g_typec_complete_type) {
		hwlog_info("%s: vbus remove while waiting, exit\n", __func__);
		cc_change = false;
		cc_exist = false;
		send_charger_connect_event();
		send_charger_disconnect_event();
		mutex_unlock(&typec_wait_lock);
		reinit_typec_completion();
		return;
	} else 	if (COMPLETE_FROM_TYPEC_CHANGE == g_typec_complete_type || timeout) {
		hwlog_info("%s: cc change, exit,complete_type = %d, timeout is %u\n", __func__, g_typec_complete_type, timeout);
		cc_change = true;
		cc_exist = true;
		hwusb_wake_unlock();
		mutex_unlock(&typec_wait_lock);
		return;
	} else {
		cc_change = false;
		pd_dpm_get_typec_state(&typec_state);
		if (PD_DPM_USB_TYPEC_DETACHED == typec_state) {
			cc_exist = false; /* cable without cc, need to send connect event*/
		} else {
			cc_exist = true; /*cable with cc, 1.direct_charge,2.pwr swap,3.hardreset*/
		}
		send_charger_connect_event();
		mutex_unlock(&typec_wait_lock);
		hwlog_info("%s: end\n", __func__);
		hwusb_wake_unlock();
	}
}

static irqreturn_t charger_disconnect_interrupt(int irq, void *p)
{
	hwlog_info("%s: start\n", __func__);
#ifdef CONFIG_TCPC_CLASS
	/*bugfix for digital headset issue*/
	if (support_pd && pd_dpm_ignore_vbusoff_event()) {
		hwlog_info("%s ignore_vbus_off_event\n", __func__);
		pd_dpm_set_ignore_vbusoff_event(false);
		hwlog_info("%s: end\n", __func__);
		return IRQ_HANDLED;
	}
	/*ignore vbus event when pd pwr swap happen*/
	if (support_pd && pd_dpm_get_pd_finish_flag()) {
		hwlog_info("%s ignore vbus disconnect event when pd contract is established\n", __func__);
		hwlog_info("%s: end\n", __func__);
		return IRQ_HANDLED;
	}
#endif

	schedule_delayed_work(&g_disconnect_work, msecs_to_jiffies(0));
	hwlog_info("%s: end\n", __func__);
	return IRQ_HANDLED;
}

static void vbus_disconnect_work(struct work_struct *w)
{
	hwusb_wake_lock();
	hwlog_info("%s: start\n", __func__);
#ifdef CONFIG_CONTEXTHUB_PD
	hw_pd_wait_dptx_ready();
#endif
	if (!pmic_vbus_attach_enable) {
#ifdef CONFIG_SUPERSWITCH_FSC
		if (!FUSB3601_in_factory_mode()) {
			hwlog_info("%s: pmic_vbus not enabled, end\n", __func__);
		} else {
			hwlog_info("%s: superswitch in factory mode\n", __func__);
			send_charger_disconnect_event();
			charger_source_sink_event(STOP_SINK);
		}
#else
		hwlog_info("%s: pmic_vbus not enabled, end\n", __func__);
#endif
		hwusb_wake_unlock();
		return;
	}
	if (direct_charge_get_cutoff_normal_flag()) {
		send_charger_disconnect_event();
		direct_charge_flag = true;
		hwlog_info("%s: in direct charging\n", __func__);
		hwlog_info("%s: end\n", __func__);
		hwusb_wake_unlock();
		return;
	}
	if (g_cur_usb_event == PD_DPM_TYPEC_ATTACHED_AUDIO) {
		g_cur_usb_event = PD_DPM_USB_TYPEC_NONE;
		send_charger_disconnect_event();
		hwusb_wake_unlock();
		hwlog_info("%s: in audioacc charging\n", __func__);
		return;
	}
	if (cc_change) {
		hwlog_info("%s: cc change, exit\n", __func__);
		hwlog_info("%s: end\n", __func__);
		hwusb_wake_unlock();
		return;
	}
	mutex_lock(&typec_wait_lock);
	if (!wait_finish) {
		hwlog_info("%s: in waiting process, exit\n", __func__);
		mutex_unlock(&typec_wait_lock);
		typec_complete(COMPLETE_FROM_VBUS_DISCONNECT);
		hwusb_wake_unlock();
		return;
	}
	if (cc_exist) {
		hwlog_info("%s: cc exist, exit\n", __func__);
		mutex_unlock(&typec_wait_lock);
		hwusb_wake_unlock();
		return;
	}
	LOG_JANK_D(JLID_USBCHARGING_END, "JL_USBCHARGING_END");
	send_charger_disconnect_event();
	mutex_unlock(&typec_wait_lock);
	hwlog_info("%s: end\n", __func__);
	hwusb_wake_unlock();
}

static int hisi_usb_vbus_probe(struct platform_device *pdev)
{
	int ret = 0;
#ifdef CONFIG_TCPC_CLASS
	struct device_node *np = NULL;
	struct device* dev;
#endif
	hwlog_info("[%s]+\n", __func__);

#ifdef CONFIG_TCPC_CLASS
	dev = &pdev->dev;
	np = dev->of_node;
	ret = of_property_read_u32(np, "support_pd", &support_pd);
	if (ret) {
		hwlog_err("get support_pd failed\n");
		return -EINVAL;
	}
	hwlog_info("support_pd = %d\n", support_pd);
#endif
        INIT_DELAYED_WORK(&g_disconnect_work, vbus_disconnect_work);
        INIT_DELAYED_WORK(&g_connect_work, vbus_connect_work);

#ifdef CONFIG_CONTEXTHUB_PD
	ret = of_property_read_u32(np, "support_dp", &support_dp);
	if (ret) {
		hwlog_err("get support_dp failed\n");
	}
	hwlog_info("support_dp = %d\n", support_dp);

#endif
	ret = of_property_read_u32(np, "pmic_vbus_attach_enable", &pmic_vbus_attach_enable);
	if (ret) {
		hwlog_err("get pmic_vbus_attach_enable failed\n");
		pmic_vbus_attach_enable = 1;
	}
	hwlog_info("pmic_vbus_attach_enable = %d\n", pmic_vbus_attach_enable);
	wake_lock_init(&hwusb_lock, WAKE_LOCK_SUSPEND, "hwusb_wakelock");
	hw_vbus_connect_irq = hisi_get_pmic_irq_byname(VBUS_CONNECT);
	if (0 == hw_vbus_connect_irq) {
		hwlog_err("failed to get connect irq\n");
		wake_lock_destroy(&hwusb_lock);
		return -ENOENT;
	}
	hw_vbus_disconnect_irq = hisi_get_pmic_irq_byname(VBUS_DISCONNECT);
	if (0 == hw_vbus_disconnect_irq) {
		hwlog_err("failed to get disconnect irq\n");
		wake_lock_destroy(&hwusb_lock);
		return -ENOENT;
	}

	hwlog_info("hw_vbus_connect_irq: %d, hw_vbus_disconnect_irq: %d\n",
			hw_vbus_connect_irq, hw_vbus_disconnect_irq);

	ret = request_irq(hw_vbus_connect_irq, charger_connect_interrupt,
					  IRQF_NO_SUSPEND, "hiusb_in_interrupt", pdev);
	if (ret) {
		hwlog_err("request charger connect irq failed, irq: %d!\n", hw_vbus_connect_irq);
		wake_lock_destroy(&hwusb_lock);
		return ret;
	}

	ret = request_irq(hw_vbus_disconnect_irq, charger_disconnect_interrupt,
					  IRQF_NO_SUSPEND, "hiusb_in_interrupt", pdev);
	if (ret) {
		free_irq(hw_vbus_disconnect_irq, pdev);
		hwlog_err("request charger connect irq failed, irq: %d!\n", hw_vbus_disconnect_irq);
	}


	/* avoid lose intrrupt */
	if (hisi_pmic_get_vbus_status()) {
		hwlog_info("%s: vbus high, issue a charger connect event\n", __func__);
		/*call combphy switch until dp probe finish*/
		#ifdef CONFIG_TCPC_CLASS
		if (!(support_pd && pd_dpm_ignore_vbuson_event()) && !(support_pd && pd_dpm_get_pd_finish_flag())) {
		#endif
			schedule_delayed_work(&g_connect_work, msecs_to_jiffies(0));
		#ifdef CONFIG_TCPC_CLASS
		}
		#endif
	} else {
		if (!g_pd_di->pd_source_vbus) {
			if (pmic_vbus_attach_enable) {
				pd_dpm_vbus_notifier_call(g_pd_di, CHARGER_TYPE_NONE, NULL);
			}
			hwlog_info("%s: vbus low, issue a charger disconnect event\n", __func__);
		}
	}

	hwlog_info("[%s] probe ok -\n", __func__);

	return ret;
}

static int hisi_usb_vbus_remove(struct platform_device *pdev)
{
	free_irq(hw_vbus_connect_irq, pdev);
	free_irq(hw_vbus_disconnect_irq, pdev);
	wake_lock_destroy(&hwusb_lock);
	return 0;
}

static struct of_device_id hisi_usb_vbus_of_match[] = {
	{ .compatible = "huawei,usbvbus", },
	{ },
};

static struct platform_driver hisi_usb_vbus_drv = {
	.probe		= hisi_usb_vbus_probe,
	.remove		= hisi_usb_vbus_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "hw_usb_vbus",
		.of_match_table	= hisi_usb_vbus_of_match,
	},
};

static int __init huawei_usb_vbus_init(void)
{
    return platform_driver_register(&hisi_usb_vbus_drv);
}

static void __exit huawei_usb_vbus_exit(void)
{
    platform_driver_unregister(&hisi_usb_vbus_drv);
}

late_initcall_sync(huawei_usb_vbus_init);
module_exit(huawei_usb_vbus_exit);

MODULE_AUTHOR("huawei");
MODULE_DESCRIPTION("This module detect USB VBUS connection/disconnection");
MODULE_LICENSE("GPL v2");
