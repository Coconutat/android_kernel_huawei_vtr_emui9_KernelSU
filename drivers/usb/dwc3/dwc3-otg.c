#include <linux/kernel.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>

#include "core.h"
#include "io.h"
#include "dwc3-otg.h"
#include "dwc3-hisi.h"

#define DBG(format, arg...) printk(KERN_INFO "[%s]" format, __func__, ##arg)

struct dwc3_otg *dwc_otg_handler;

#ifdef DWC3_OTG_FORCE_RESET_GCTL
static void dwc3_otg_force_reset_core(struct dwc3 *dwc)
{
	u32 reg;

	/* step 1: reset usb controller */
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg |= (DWC3_GCTL_CORESOFTRESET);
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	reg &= ~(DWC3_GCTL_CORESOFTRESET);
	dwc3_writel(dwc->regs, DWC3_GCTL, reg);
}

static void dwc3_otg_split_disable(struct dwc3 *dwc)
{
	u32 reg;

	/* step 2: set SPLIT boundary disable */
	reg = dwc3_readl(dwc->regs, DWC3_GUCTL3);
	printk(KERN_ERR "dwc3_otg_force_reset_core: FIRST [%x = 0x%x]\n", DWC3_GUCTL3, reg);
	reg |= (DWC3_GUCTL3_SPLITDISABLE);
	dwc3_writel(dwc->regs, DWC3_GUCTL3, reg);

	printk(KERN_ERR "SECOND [%x = 0x%x]\n", DWC3_GUCTL3, reg);
}
#endif

#if IS_ENABLED(CONFIG_USB_DWC3_HOST) || IS_ENABLED(CONFIG_USB_DWC3_DUAL_ROLE)
static int dwc3_otg_start_host(struct dwc3 *dwc)
{
	unsigned long flags;
	int ret;
	u32 reg;

	DBG("+\n");

	spin_lock_irqsave(&dwc->lock, flags);
	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	pr_debug("%s: GCTL value 0x%x\n", __func__, reg);

#ifdef DWC3_OTG_FORCE_RESET_GCTL
	dwc3_otg_force_reset_core(dwc);
#endif /* DWC3_OTG_FORCE_RESET_GCTL */

	hisi_dwc3_cpmode_enable();
	dwc3_lscdtimer_set();

	spin_unlock_irqrestore(&dwc->lock, flags);

#ifdef CONFIG_HISI_USB_DWC3_MASK_IRQ_WORKAROUND
	if (dwc->irq_state == 0) {
		enable_irq(dwc->irq_gadget);
		dwc->irq_state = 1;
		DBG("enable irq\n");
	}
#endif

	ret = dwc3_host_init(dwc);
	if (ret) {
		pr_err("%s: failed to register xHCI device\n", __func__);
		return ret;
	}

#ifdef DWC3_OTG_FORCE_RESET_GCTL
	dwc3_otg_split_disable(dwc);
#endif /* DWC3_OTG_FORCE_RESET_GCTL */

	hisi_dwc3_platform_host_quirks();

	DBG("-\n");

	return ret;
}

static void dwc3_otg_stop_host(struct dwc3 *dwc)
{
	DBG("+\n");
	dwc3_host_exit(dwc);
	DBG("-\n");
}
#else
static inline int dwc3_otg_start_host(struct dwc3 *dwc)
{ return 0; }
static inline void dwc3_otg_stop_host(struct dwc3 *dwc)
{ }
#endif

static int dwc3_otg_start_peripheral(struct dwc3 *dwc)
{
	int ret;
	unsigned long flags;
	u32 reg;

	DBG("+\n");

	spin_lock_irqsave(&dwc->lock, flags);
	dwc3_lscdtimer_set();

	hisi_dwc3_platform_device_quirks();

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	pr_debug("%s: GCTL value 0x%x\n", __func__, reg);
	spin_unlock_irqrestore(&dwc->lock, flags);

	ret = dwc3_gadget_init(dwc);
	DBG("-\n");

	return ret;
}

static void dwc3_otg_stop_peripheral(struct dwc3 *dwc)
{
	DBG("+\n");
	dwc3_gadget_exit(dwc);
	dwc3_event_buffers_cleanup(dwc);
	tasklet_kill(&dwc->bh);
	DBG("-\n");
}

static int __dwc3_set_mode(struct dwc3 *dwc)
{
	unsigned long flags;
	int ret;

	if (dwc->desired_dr_role == dwc->current_dr_role)
		return 0;

	if (dwc->dr_mode != USB_DR_MODE_OTG)
		return -EINVAL;

	switch (dwc->current_dr_role) {
		case USB_DR_MODE_HOST:
			dwc3_otg_stop_host(dwc);
			break;
		case USB_DR_MODE_PERIPHERAL:
			dwc3_otg_stop_peripheral(dwc);
			break;
		default:
			break;
	}

	if (!dwc->desired_dr_role)
		return 0;

	spin_lock_irqsave(&dwc->lock, flags);

	ret = dwc3_core_init(dwc);
	if (ret) {
		dev_err(dwc->dev, "failed to initialize core\n");
		spin_unlock_irqrestore(&dwc->lock, flags);
		return ret;
	}

	dwc3_set_prtcap(dwc, dwc->desired_dr_role);

	spin_unlock_irqrestore(&dwc->lock, flags);

	switch (dwc->desired_dr_role) {
		case USB_DR_MODE_HOST:
			ret = dwc3_otg_start_host(dwc);
			if (ret)
				dev_err(dwc->dev, "failed to initialize host\n");
			break;
		case USB_DR_MODE_PERIPHERAL:
			ret = dwc3_otg_start_peripheral(dwc);
			if (ret)
				dev_err(dwc->dev, "failed to initialize peripheral\n");
			break;
		default:
			break;
	}

	return ret;
}

static int dwc3_set_mode(struct dwc3 *dwc, u32 mode)
{
	unsigned long flags;
	int ret;

	pr_info("%s: set mode %d, current mode %d\n", __func__, mode, dwc->current_dr_role);
	spin_lock_irqsave(&dwc->lock, flags);
	dwc->desired_dr_role = mode;
	spin_unlock_irqrestore(&dwc->lock, flags);

	ret = __dwc3_set_mode(dwc);
	if (!ret)
		dwc->current_dr_role = dwc->desired_dr_role;

	return ret;
}

int dwc3_otg_work(struct dwc3_otg *dwc_otg, int evt)
{
	int ret = 0;

	DBG("+\n");

	/* if otg is not enabled, do nothing */
	if (!dwc_otg) {
		DBG("dwc3 is not otg mode!\n");
		return 0;
	}

	mutex_lock(&dwc_otg->lock);

	switch (evt) {
	case DWC3_OTG_EVT_ID_SET:
		ret = dwc3_set_mode(dwc_otg->dwc, USB_DR_MODE_UNKNOWN);
		break;
	case DWC3_OTG_EVT_ID_CLEAR:
		ret = dwc3_set_mode(dwc_otg->dwc, USB_DR_MODE_HOST);
		break;
	case DWC3_OTG_EVT_VBUS_SET:
		ret = dwc3_set_mode(dwc_otg->dwc, USB_DR_MODE_PERIPHERAL);
		break;
	case DWC3_OTG_EVT_VBUS_CLEAR:
		ret = dwc3_set_mode(dwc_otg->dwc, USB_DR_MODE_UNKNOWN);
		break;
	default:
		break;
	}

	mutex_unlock(&dwc_otg->lock);
	DBG("-\n");

	return ret;
}

int dwc3_otg_init(struct dwc3 *dwc)
{
	struct dwc3_otg *dwc_otg;
	u32 reg;

	DBG("+\n");

	dwc_otg = devm_kzalloc(dwc->dev, sizeof(struct dwc3_otg), GFP_KERNEL);
	if (!dwc_otg) {
		dev_err(dwc->dev, "unable to allocate dwc3_otg\n");
		return -ENOMEM;
	}

	mutex_init(&dwc_otg->lock);

	dwc_otg->dwc = dwc;
	dwc->dwc_otg = dwc_otg;

	dwc_otg_handler = dwc_otg;

	reg = dwc3_readl(dwc->regs, DWC3_GCTL);
	pr_debug("%s: GCTL value 0x%x\n", __func__, reg);

	/* default device mode */
	dwc3_set_prtcap(dwc, DWC3_GCTL_PRTCAP_DEVICE);

	DBG("-\n");

	return 0;
}

void dwc3_otg_exit(struct dwc3 *dwc)
{
	DBG("+\n");
	dwc_otg_handler = NULL;
	dwc->dwc_otg->dwc = NULL;
	dwc->dwc_otg = NULL;
	DBG("-\n");
	return;
}
