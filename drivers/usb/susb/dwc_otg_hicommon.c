#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>

#include <linux/hisi/usb/hisi_usb.h>
#include <linux/pm_runtime.h>

#include "lm.h"
#include "dwc_otg_pcd.h"
#include "dwc_otg_pcd_if.h"
#include "dwc_otg_core_if.h"
#include "dwc_otg_cil.h"
#include "dwc_otg_driver.h"
#include "dwc_otg_hcd.h"

#include "dwc_otg_hicommon.h"
#include "hisi_usb_otg_type.h"
#include <linux/mfd/hisi_pmic.h>
#include <pmic_interface.h>

atomic_t is_event_work_ready = ATOMIC_INIT(0);
atomic_t is_power_on = ATOMIC_INIT(0);

struct otg_dev *otg_dev_p;

#define ENABLE_USB_OTG_TEST 1
#if ENABLE_USB_OTG_TEST

static ssize_t plugusb_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	char *s;
	switch (otg_dev_p->status) {
	case OTG_DEV_OFF:
		s = "OTG_DEV_OFF";
		break;
	case OTG_DEV_DEVICE:
		s = "OTG_DEV_DEVICE";
		break;
	case OTG_DEV_HOST:
		s = "OTG_DEV_HOST";
		break;
	default:
		s = "unknown";
		break;
	}
	return scnprintf(buf, PAGE_SIZE, "usb otg status: %s\n"
			 "write hoston/hostoff/deviceon/deviceoff to change the state.\n", s);
}
#ifdef HISI_PLUGUSB_TEST
static ssize_t plugusb_store(struct device *pdev,
			     struct device_attribute *attr,
			     const char *buf, size_t size)
{
	if (!strncmp(buf, "hoston", 6)) {
		hisi_usb_otg_event(ID_FALL_EVENT);
		usb_err("hoston\n");
	} else if (!strncmp(buf, "hostoff", 7)) {
		hisi_usb_otg_event(ID_RISE_EVENT);
		usb_err("hostoff\n");
	} else if (!strncmp(buf, "deviceon", 8)) {
		hisi_usb_otg_event(CHARGER_CONNECT_EVENT);
		usb_err("deviceon\n");
	} else if (!strncmp(buf, "deviceoff", 9)) {
		hisi_usb_otg_event(CHARGER_DISCONNECT_EVENT);
		usb_err("deviceoff\n");
	} else {
		printk(KERN_ERR "I dot know that!\n");
	}

	return size;
}
#else
/*lint -save -e715 */
static ssize_t plugusb_store(struct device *pdev,
			     struct device_attribute *attr,
			     const char *buf, size_t size)
{
        usb_err("ERROR:[USB.plugusb] not for user verison\n");

        return size;
}
/*lint -restore */
#endif
DEVICE_ATTR(plugusb, (S_IRUSR | S_IRGRP | S_IWUSR), plugusb_show, plugusb_store);

static const char *charger_type_array[] = {
	[CHARGER_TYPE_SDP]     = "sdp",       /* Standard Downstreame Port */
	[CHARGER_TYPE_CDP]     = "cdp",       /* Charging Downstreame Port */
	[CHARGER_TYPE_DCP]     = "dcp",       /* Dedicate Charging Port */
	[CHARGER_TYPE_UNKNOWN] = "unknown",   /* non-standard */
	[CHARGER_TYPE_NONE]    = "none",      /* not connected */
	[PLEASE_PROVIDE_POWER] = "provide"   /* host mode, provide power */
};

static enum hisi_charger_type get_charger_type_from_str(const char *buf, size_t size)
{
	int i = 0;
	enum hisi_charger_type ret = CHARGER_TYPE_NONE;

	for (i = 0; i < sizeof(charger_type_array) / sizeof(charger_type_array[0]); i++) {
		if (!strncmp(buf, charger_type_array[i], size - 1)) {
			ret = (enum hisi_charger_type)i;
			break;
		}
	}

	return ret;
}

ssize_t hiusb_do_charger_show(void *dev_data, char *buf, size_t len)
{
	struct otg_dev *dev_p = (struct otg_dev *)dev_data;
	enum hisi_charger_type charger_type = CHARGER_TYPE_NONE;

	if (!dev_p) {
		printk(KERN_ERR "platform_get_drvdata return null\n");
		return scnprintf(buf, len, "platform_get_drvdata return null\n");
	}

	charger_type = dev_p->charger_type;

	return scnprintf(buf, len, "[(%d):Charger type = %s]\n"
			"----------------------------------------------------------------\n"
			"usage: echo {str} > chargertest\n"
			"       sdp:        Standard Downstreame Port\n"
			"       cdp:        Charging Downstreame Port\n"
			"       dcp:        Dedicate Charging Port\n"
			"       unknown:    non-standard\n"
			"       none:       not connected\n"
			"       provide:    host mode, provide power\n"
			, charger_type, charger_type_array[charger_type]);
}

ssize_t hiusb_do_charger_store(void *dev_data, const char *buf, size_t size)
{
	struct otg_dev *dev_p = (struct otg_dev *)dev_data;
	enum hisi_charger_type charger_type = get_charger_type_from_str(buf, size);

	if (!dev_p) {
		printk(KERN_ERR "platform_get_drvdata return null\n");
		return size;
	}

	dev_p->charger_type = charger_type;
	notify_charger_type(dev_p);

	return size;
}

static ssize_t fakecharger_show(void *dev_data, char *buf, size_t size)
{
	struct otg_dev *dev_p = (struct otg_dev *)dev_data;

	if (!dev_p) {
		printk(KERN_ERR "platform_get_drvdata return null\n");
		return scnprintf(buf, size, "platform_get_drvdata return null\n");
	}

	return scnprintf(buf, size, "[fake charger type: %s]\n",
					 dev_p->fake_charger_type == CHARGER_TYPE_NONE ? "not fake" :
					 charger_type_array[dev_p->fake_charger_type]);
}

static ssize_t fakecharger_store(void *dev_data, const char *buf, size_t size)
{
	struct otg_dev *dev_p = (struct otg_dev *)dev_data;
	enum hisi_charger_type charger_type = get_charger_type_from_str(buf, size);

	if (!dev_p) {
		printk(KERN_ERR "platform_get_drvdata return null\n");
		return size;
	}

	mutex_lock(&dev_p->lock);
	dev_p->fake_charger_type = charger_type;
	mutex_unlock(&dev_p->lock);

	return size;
}

int hiusb_do_eventmask_show(void *dev_data)
{
	struct otg_dev *dev_p = (struct otg_dev *)dev_data;

	return dev_p->eventmask;
}

void hiusb_do_eventmask_store(void *dev_data, int eventmask)
{
	struct otg_dev *dev_p = (struct otg_dev *)dev_data;

	dev_p->eventmask = eventmask;
}

void hiusb_set_eyepattern_param(void *dev_data, unsigned int eye_diagram_param)
{
	struct otg_dev *dev_p = (struct otg_dev *)dev_data;

	dev_p->eyePattern = eye_diagram_param;
	dev_p->host_eyePattern = eye_diagram_param;
}

void hiusb_get_eyepattern_param(void *dev_data, unsigned int *eye_diagram_param,
		unsigned int *eye_diagram_host_param)
{
	struct otg_dev *dev_p = (struct otg_dev *)dev_data;

	*eye_diagram_param = dev_p->eyePattern;
	*eye_diagram_host_param = dev_p->host_eyePattern;
}

static struct device_attribute *hisi_usb_attributes[] = {
	&dev_attr_plugusb
};

int create_attr_file(struct device *dev)
{
	int ret = 0;
	int i = 0, attr_count = 0;
	struct class *hisi_usb_class;
	struct device *hisi_usb_dev;

	attr_count = sizeof(hisi_usb_attributes) / sizeof(*hisi_usb_attributes);
	for (i = 0; i < attr_count; i++) {
		ret = device_create_file(dev, hisi_usb_attributes[i]);
		if (ret) {
			dev_err(dev, "create attr file [%d] error!\n", i);
			break;
		}
	}

	while (i < attr_count && 0 < i) {
		dev_err(dev, "need remove attr file:%d!\n", i);
		i--;
		device_remove_file(dev, hisi_usb_attributes[i]);
	}

	hisi_usb_class = class_create(THIS_MODULE, "hisi_usb_class");
	if (IS_ERR(hisi_usb_class)) {
		usb_dbg("create hisi_usb_class error!\n");
	} else {
		hisi_usb_dev = device_create(hisi_usb_class, NULL, 0, NULL, "hisi_usb_dev");
		if (IS_ERR(hisi_usb_dev)) {
			usb_dbg("create hisi_usb_dev error!\n");
		} else {
			ret |= sysfs_create_link(&hisi_usb_dev->kobj, &dev->kobj, "interface");
		}
	}

	if (ret) {
		usb_dbg("create attr file error!\n");
	}

#ifdef CONFIG_USB_F_BALONG_ACM
#ifdef CONFIG_HISI_DEBUG_FS
	hiusb_debug_quick_register("fakecharger",
		platform_get_drvdata(to_platform_device(dev)),
		(hiusb_debug_show_ops)fakecharger_show,
		(hiusb_debug_store_ops)fakecharger_store);
	hiusb_debug_init(dev_get_drvdata(dev));
#endif
#endif

	return ret;
}
#else
static inline int create_attr_file(struct device *dev) {return 0; }
#endif

int get_resource(struct otg_dev *dev)
{
	int ret;
	struct device_node *np = NULL;
	struct device *base_dev = &(dev->pdev->dev);

	/*
	 * get registers base address
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,usb-otg-ahbif");
	if (np) {
		dev->usb_ahbif_base = of_iomap(np, 0);
	}

	/*
	 * get pctrl base address
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,pctrl");
	if (np) {
		dev->pctrl_reg_base = of_iomap(np, 0);
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
	if (np) {
		dev->pericrg_base = of_iomap(np, 0);
	}

	/*
	 * get host eye pattern param.
	 */
	ret = of_property_read_u32(base_dev->of_node, "host_eye_diagram_param", &(dev->host_eyePattern));
	if (ret) {
		usb_err("get host eye_diagram_param failed !!! \n");
		dev->host_eyePattern = 0x059066DB;
	}

	/*
	 * get device eye pattern param.
	 */
	ret = of_property_read_u32(base_dev->of_node, "eye_diagram_param", &(dev->eyePattern));
	if (ret) {
		usb_err("get eye_diagram_param failed !!! \n");
		dev->eyePattern = 0x059066DB;
		ret = 0;
	}

	/*
	 * get device usb support voltage check
	 */
	if(of_property_read_u32(base_dev->of_node, "usb_support_check_voltage", &(dev->check_voltage))){
		usb_dbg("usb driver not usb_support_check_voltage!!!\n");
		dev->check_voltage = 0;
	}

	if (!dev->usb_ahbif_base || !dev->pericrg_base || !dev->pctrl_reg_base) {
		usb_err("get registers base address failed![a:%pK],[peri:%pK],[pctrl:%pK]\n", dev->usb_ahbif_base, dev->pericrg_base, dev->pctrl_reg_base);
		return -ENXIO;
	}

	/*
	 * get ldo handlers
	 */
	dev->otgdebugsubsys_regu.supply = "otgdebugsubsys";
	ret = devm_regulator_bulk_get(base_dev, 1, &dev->otgdebugsubsys_regu);
	if (ret) {
		usb_err("couldn't get regulator otgdebugsubsys\n");
	}

	/*
	 * get abb clk handler
	 */
	dev->clk = devm_clk_get(base_dev, "clk_abb_192");
	if (IS_ERR_OR_NULL(dev->clk)) {
		usb_err("get usb clk failed\n");
		return -EINVAL;
	}

	/*
	 * get hclk handler
	 */
	dev->hclk_usb2otg = devm_clk_get(base_dev, "hclk_usb2otg");
	if (IS_ERR_OR_NULL(dev->hclk_usb2otg)) {
		usb_err("get aclk_usb3otg failed\n");
		return -EINVAL;
	}

	if (of_property_read_u32(base_dev->of_node, "fpga_flag", &(dev->fpga_flag))) {
		dev->fpga_flag = 0;
		dev->fpga_usb_mode_gpio = -1;
		usb_dbg("in asic mode!\n");
	} else {
		dev->fpga_usb_mode_gpio = of_get_named_gpio(base_dev->of_node, "fpga_usb_mode_gpio", 0);
		if (dev->fpga_usb_mode_gpio > 0) {
			usb_dbg("in fpga mode:%d!\n", dev->fpga_usb_mode_gpio);
		} else {
			usb_err("[fpga mode]:get gpio_number failure errno:%d!\n", dev->fpga_usb_mode_gpio);
		}
	}

	usb_dbg("get resource %s!\n", ret ? "error" : "done");
	return ret;
}

static void put_resource(struct otg_dev *dev)
{
	usb_dbg("put resource done.\n");
	return;
}

int open_ldo(struct otg_dev *otg_device)
{
	int ret = 0;

	if (0 != otg_device->is_regu_on) {
		usb_dbg("ldo already opened!\n");
		return 0;
	}

	ret = regulator_bulk_enable(1, &otg_device->otgdebugsubsys_regu);
	if (ret) {
		usb_err("enable regulator otgdebugsubsys failed\n");
	}

	otg_device->is_regu_on = 1;

	return ret;
}

void close_ldo(struct otg_dev *otg_device)
{
	int ret;

	if (0 == otg_device->is_regu_on) {
		usb_dbg("ldo already closed!\n");
		return;
	}

	ret = regulator_bulk_disable(1, &otg_device->otgdebugsubsys_regu);

	if (ret != 0) {
		usb_err("%s(%u) : regulator_bulk_disable fail err = %d\n", __func__, __LINE__, ret);
	}

	otg_device->is_regu_on = 0;
}

void init_usb_otg_phy(struct otg_dev *otg_device, int is_host_mode)
{
	struct usb_phy_ops *usb_phy_ops = get_usb_phy_ops(otg_device);
	if (usb_phy_ops) {
		usb_phy_ops->init(otg_device, is_host_mode);
	} else {
		printk(KERN_ERR "[%s]:get phy ops error!\n", __func__);
	}
}

void close_usb_otg_phy(struct otg_dev *otg_device)
{
	struct usb_phy_ops *usb_phy_ops = get_usb_phy_ops(otg_device);
	if (usb_phy_ops) {
		usb_phy_ops->close(otg_device);
	} else {
		printk(KERN_ERR "[%s]:get phy ops error!\n", __func__);
	}
}

static void show_pericrg_status(void)
{
	void __iomem *pericrg_base = otg_dev_p->pericrg_base;
	printk(KERN_INFO "PERI_CRG_RSTEN4: 0x%08x, PERI_CRG_RSTDIS4: 0x%08x,"
		" PERI_CRG_RSTSTAT4: 0x%08x\n",
		readl(pericrg_base + PERI_CRG_RSTEN4),
		readl(pericrg_base + PERI_CRG_RSTDIS4),
		readl(pericrg_base + PERI_CRG_RSTSTAT4));
}

static int hw_setup(struct otg_dev *otg_device, int is_host_mode)
{
	int ret;
	struct usb_phy_ops *usb_phy_ops = get_usb_phy_ops(otg_device);

	ret = open_ldo(otg_device);
	if (ret) {
		usb_err("open ldo error!");
		return ret;
	}

	ret = clk_set_rate(otg_device->hclk_usb2otg, USB2OTG_HCLK_FREQ);
	if (ret) {
		usb_err("clk_set_rate failed\n");
		return ret;
	}

	/* enable clk */
	if (usb_phy_ops) {
		usb_phy_ops->enable_clk(otg_device);
	} else {
		usb_err("[USB2.0]:Can't register enable clk!\n");
	}

	init_usb_otg_phy(otg_device, is_host_mode);
	show_pericrg_status();
	atomic_set(&is_power_on, 1);

	otg_device->hcd_status = HCD_ON;

	if (otg_device->dwc_otg_irq_enabled == false) {
		enable_irq(otg_device->lm_dev->irq);
		otg_device->dwc_otg_irq_enabled = true;
	}
	return 0;
}

static void hw_shutdown(struct otg_dev *otg_device)
{
	dwc_otg_device_t *dwc_otg_dev;
	struct usb_phy_ops *usb_phy_ops;
	unsigned long flags;

	if ((!otg_device) || (!otg_device->lm_dev))
		return;

	usb_phy_ops = get_usb_phy_ops(otg_device);
	dwc_otg_dev = lm_get_drvdata(otg_device->lm_dev);

	if (!dwc_otg_dev)
		return;

	/* get spin lock, wait for irq handler finish */
	DWC_SPINLOCK_IRQSAVE(dwc_otg_dev->core_if->lock, &flags);
	atomic_set(&is_power_on, 0);
	DWC_SPINUNLOCK_IRQRESTORE(dwc_otg_dev->core_if->lock, flags);

	/* work or timer may enable global interrupts, close it now */
	dwc_otg_disable_global_interrupts(dwc_otg_dev->core_if);

	close_usb_otg_phy(otg_device);

	/* disable clk */
	if (usb_phy_ops) {
		usb_phy_ops->disable_clk(otg_device);
	} else {
		usb_err("[USB2.0]:Can't register disable clk!\n");
	}

	close_ldo(otg_device);
	show_pericrg_status();

	otg_device->hcd_status = HCD_OFF;

	if (otg_device->dwc_otg_irq_enabled == true) {
		disable_irq(otg_device->lm_dev->irq);
		otg_device->dwc_otg_irq_enabled = false;
	}

	return;
}

int dwc_otg_hcd_setup(void)
{
	struct otg_dev *p = otg_dev_p;
	mutex_lock(&p->lock);

	if ((OTG_DEV_OFF == p->status) && (HCD_OFF == p->hcd_status)) {
		usb_dbg("p->status: %d, hcd_status: %d\n", p->status, p->hcd_status);
		if (open_ldo(p)) {
			usb_err("open ldo failed!\n");
			mutex_unlock(&p->lock);
			return -1;
		}
		init_usb_otg_phy(p, 1);
		p->hcd_status = HCD_ON;;
	}

	mutex_unlock(&p->lock);
	return 0;
}

void dwc_otg_hcd_shutdown(void)
{
	struct otg_dev *p = otg_dev_p;
	mutex_lock(&p->lock);

	if ((OTG_DEV_OFF == p->status) && (HCD_ON == p->hcd_status)) {
		usb_dbg("p->status: %d, hcd_status: %d\n", p->status, p->hcd_status);
		close_usb_otg_phy(p);
		close_ldo(p);
		p->hcd_status = HCD_OFF;
	}

	mutex_unlock(&p->lock);
}

static char *charger_type_string(enum hisi_charger_type type)
{
	char *s = NULL;
	if (type == CHARGER_TYPE_SDP)
		s = "SDP";
	else if (type == CHARGER_TYPE_CDP)
		s = "CDP";
	else if (type == CHARGER_TYPE_DCP)
		s = "DCP";
	else if (type == CHARGER_TYPE_UNKNOWN)
		s = "UNKNOWN";
	else if (type == CHARGER_TYPE_NONE)
		s = "NONE";
	else
		s = "ilegal charger";
	return s;
}

static char *event_type_string(enum otg_dev_event_type event)
{
	char *s = NULL;
	if (event == CHARGER_CONNECT_EVENT)
		s = "CHARGER_CONNECT";
	else if (event == CHARGER_DISCONNECT_EVENT)
		s = "CHARGER_DISCONNECT";
	else if (event == ID_FALL_EVENT)
		s = "OTG_CONNECT";
	else if (event == ID_RISE_EVENT)
		s = "OTG_DISCONNECT";
	else if (event == NONE_EVENT)
		s = "NONE";
	else
		s = "ilegal event";
	return s;
}

void dump_bc_reg(struct otg_dev *dev_p)
{
	/* for debug */
	struct usb_ahbif_registers *ahbif = (struct usb_ahbif_registers *)otg_dev_p->usb_ahbif_base;

	usb_err("bc_ctrl0:[%x]\n", ahbif->bc_ctrl0.reg);
	usb_err("bc_ctrl1:[%x]\n", ahbif->bc_ctrl1.reg);
	usb_err("bc_ctrl2:[%x]\n", ahbif->bc_ctrl2.reg);
	usb_err("bc_ctrl3:[%x]\n", ahbif->bc_ctrl3.reg);
	usb_err("bc_ctrl4:[%x]\n", ahbif->bc_ctrl4.reg);
	usb_err("bc_ctrl5:[%x]\n", ahbif->bc_ctrl5.reg);
	usb_err("bc_ctrl6:[%x]\n", ahbif->bc_ctrl6.reg);
	usb_err("bc_ctrl7:[%x]\n", ahbif->bc_ctrl7.reg);
	usb_err("bc_ctrl8:[%x]\n", ahbif->bc_ctrl8.reg);
	usb_err("bc_sts0:[%x]\n", ahbif->bc_sts0.reg);
	usb_err("bc_sts1:[%x]\n", ahbif->bc_sts1.reg);
	usb_err("bc_sts2:[%x]\n", ahbif->bc_sts2.reg);
	usb_err("bc_sts3:[%x]\n", ahbif->bc_sts3.reg);
	usb_err("bc_sts4:[%x]\n", ahbif->bc_sts4.reg);
}

/* BC1.2 Spec:
 * If a PD detects that D+ is greater than VDAT_REF, it knows that it is
 * attached to a DCP. It is then required to enable VDP_SRC or pull D+
 * to VDP_UP through RDP_UP */
static void disable_vdp_src(struct otg_dev *dev_p)
{
	struct usb_ahbif_registers *ahbif
		= (struct usb_ahbif_registers *)otg_dev_p->usb_ahbif_base;
	union bc_ctrl4 bc_ctrl4;
	union bc_ctrl5 bc_ctrl5;

	usb_dbg("+\n");
	if (dev_p->vdp_src_enable == 0)
		return;
	dev_p->vdp_src_enable = 0;

	bc_ctrl5.reg = readl(&ahbif->bc_ctrl5);
	bc_ctrl5.bits.bc_chrg_sel = 0;
	bc_ctrl5.bits.bc_vdat_src_en = 0;
	bc_ctrl5.bits.bc_vdat_det_en = 0;
	writel(bc_ctrl5.reg, &ahbif->bc_ctrl5);

	/* bc_suspend = 1, nomal mode */
	bc_ctrl4.reg = readl(&ahbif->bc_ctrl4);
	bc_ctrl4.bits.bc_suspendm = 1;
	writel(bc_ctrl4.reg, &ahbif->bc_ctrl4);

	/* disable BC */
	writel(0, &ahbif->bc_ctrl3);
	usb_dbg("-\n");
}

static void enable_vdp_src(struct otg_dev *dev_p)
{
	struct usb_ahbif_registers *ahbif
		= (struct usb_ahbif_registers *)otg_dev_p->usb_ahbif_base;
	union bc_ctrl5 bc_ctrl5;

	usb_dbg("+\n");
	if (dev_p->vdp_src_enable == 1)
		return;
	dev_p->vdp_src_enable = 1;

	bc_ctrl5.reg = readl(&ahbif->bc_ctrl5);
	bc_ctrl5.bits.bc_chrg_sel = 0;
	bc_ctrl5.bits.bc_vdat_src_en = 1;
	bc_ctrl5.bits.bc_vdat_det_en = 1;
	writel(bc_ctrl5.reg, &ahbif->bc_ctrl5);
	usb_dbg("-\n");
}

static void detect_charger_type(struct otg_dev *dev_p)
{
	struct usb_ahbif_registers *ahbif
		= (struct usb_ahbif_registers *)otg_dev_p->usb_ahbif_base;
	enum hisi_charger_type type = CHARGER_TYPE_NONE;
	union bc_ctrl4 bc_ctrl4;
	union bc_ctrl5 bc_ctrl5;
	union bc_sts2 bc_sts2;
	unsigned long jiffies_expire;
	int i = 0;

	usb_dbg("+\n");

	if (dev_p->fake_charger_type != CHARGER_TYPE_NONE) {
		usb_dbg("fake type: %d\n", dev_p->fake_charger_type);
		dev_p->charger_type = dev_p->fake_charger_type;
		return;
	}

	/* enable BC */
	writel(1, &ahbif->bc_ctrl3);

	/* phy suspend */
	bc_ctrl4.reg = readl(&ahbif->bc_ctrl4);
	bc_ctrl4.bits.bc_suspendm = 0;
	bc_ctrl4.bits.bc_dmpulldown = 1;
	writel(bc_ctrl4.reg, &ahbif->bc_ctrl4);

	/* enable DCD */
	bc_ctrl5.reg = readl(&ahbif->bc_ctrl5);
	bc_ctrl5.bits.bc_dcd_en = 1;
	writel(bc_ctrl5.reg, &ahbif->bc_ctrl5);

	jiffies_expire = jiffies + msecs_to_jiffies(900);
	msleep(50);
	while (i < 10) {
		union bc_sts2 bc_sts2;
		bc_sts2.reg = readl(&ahbif->bc_sts2);
		if (bc_sts2.bits.bc_fs_vplus == 0) {
			i++;
		} else if (i) {
			usb_dbg("USB D+ D- not connected!\n");
			i = 0;
		}

		msleep(1);

		if (time_after(jiffies, jiffies_expire)) {
			usb_dbg("DCD timeout!\n");
			type = CHARGER_TYPE_UNKNOWN;
			break;
		}
	}

	/* disable DCD */
	bc_ctrl5.reg = readl(&ahbif->bc_ctrl5);
	bc_ctrl5.bits.bc_dcd_en = 0;
	writel(bc_ctrl5.reg, &ahbif->bc_ctrl5);

	usb_dbg("DCD done\n");

	if (type == CHARGER_TYPE_NONE) {
		/* enable vdect */
		bc_ctrl5.reg = readl(&ahbif->bc_ctrl5);
		bc_ctrl5.bits.bc_chrg_sel = 0;
		bc_ctrl5.bits.bc_vdat_src_en = 1;
		bc_ctrl5.bits.bc_vdat_det_en = 1;
		writel(bc_ctrl5.reg, &ahbif->bc_ctrl5);

		msleep(40);

		/* we can detect sdp or cdp dcp */
		bc_sts2.reg = readl(&ahbif->bc_sts2);
		if (bc_sts2.bits.bc_chg_det == 0) {
			type = CHARGER_TYPE_SDP;
		}

		/* disable vdect */
		bc_ctrl5.reg = readl(&ahbif->bc_ctrl5);
		bc_ctrl5.bits.bc_vdat_src_en = 0;
		bc_ctrl5.bits.bc_vdat_det_en = 0;
		writel(bc_ctrl5.reg, &ahbif->bc_ctrl5);
	}

	usb_dbg("Primary Detection done\n");

	if (type == CHARGER_TYPE_NONE) {
		/* enable vdect */
		bc_ctrl5.reg = readl(&ahbif->bc_ctrl5);
		bc_ctrl5.bits.bc_chrg_sel = 1;
		bc_ctrl5.bits.bc_vdat_src_en = 1;
		bc_ctrl5.bits.bc_vdat_det_en = 1;
		writel(bc_ctrl5.reg, &ahbif->bc_ctrl5);

		msleep(40);

		/* we can detect sdp or cdp dcp */
		bc_sts2.reg = readl(&ahbif->bc_sts2);
		if (bc_sts2.bits.bc_chg_det == 0) {
			type = CHARGER_TYPE_CDP;
		} else {
			type = CHARGER_TYPE_DCP;
		}

		/* disable vdect */
		bc_ctrl5.reg = readl(&ahbif->bc_ctrl5);
		bc_ctrl5.bits.bc_chrg_sel = 0;
		bc_ctrl5.bits.bc_vdat_src_en = 0;
		bc_ctrl5.bits.bc_vdat_det_en = 0;
		writel(bc_ctrl5.reg, &ahbif->bc_ctrl5);
	}

	usb_dbg("Secondary Detection done\n");

	/* If a PD detects that D+ is greater than VDAT_REF, it knows that it is
	 * attached to a DCP. It is then required to enable VDP_SRC or pull D+
	 * to VDP_UP through RDP_UP */
	if (type == CHARGER_TYPE_DCP) {
		usb_dbg("charger is DCP, enable VDP_SRC\n");
		enable_vdp_src(otg_dev_p);
	} else {
		/* bc_suspend = 1, nomal mode */
		bc_ctrl4.reg = readl(&ahbif->bc_ctrl4);
		bc_ctrl4.bits.bc_suspendm = 1;
		writel(bc_ctrl4.reg, &ahbif->bc_ctrl4);

		msleep(10);

		/* disable BC */
		writel(0, &ahbif->bc_ctrl3);
	}

	otg_dev_p->charger_type = type;
	usb_dbg("charger type: %s\n", charger_type_string(type));
	usb_dbg("-\n");
	return;
}

int hisi_charger_type_notifier_register(struct notifier_block *nb)
{
	int ret = -1;
	if (otg_dev_p)
		ret = blocking_notifier_chain_register(
			  &otg_dev_p->charger_type_notifier, nb);
	else
		printk(KERN_ERR "Notifier head not yet init.\n");
	return ret;
}

int hisi_charger_type_notifier_unregister(struct notifier_block *nb)
{
	int ret = -1;
	if (otg_dev_p)
		ret = blocking_notifier_chain_unregister(
			  &otg_dev_p->charger_type_notifier, nb);
	else
		printk(KERN_ERR "Notifier head not yet init.\n");
	return ret;
}

int hisi_usb_otg_irq_notifier_register(struct notifier_block *nb)
{
	int ret = -1;
	if (otg_dev_p)
		ret = blocking_notifier_chain_register(
			  &otg_dev_p->otg_irq_notifier, nb);
	else
		printk(KERN_ERR "Notifier head not yet init.\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hisi_usb_otg_irq_notifier_register);

int hisi_usb_otg_irq_notifier_unregister(struct notifier_block *nb)
{
	int ret = -1;
	if (otg_dev_p)
		ret = blocking_notifier_chain_unregister(
			  &otg_dev_p->otg_irq_notifier, nb);
	else
		printk(KERN_ERR "Notifier head not yet init.\n");
	return ret;
}
EXPORT_SYMBOL_GPL(hisi_usb_otg_irq_notifier_unregister);

enum hisi_charger_type hisi_get_charger_type(void)
{
	struct otg_dev *dev_p = otg_dev_p;
	enum hisi_charger_type type = CHARGER_TYPE_NONE;

	if (dev_p) {
		type = dev_p->charger_type;
	} else {
		usb_err("usb not probe!\n");
	}
	usb_dbg("charger type: %s\n", charger_type_string(type));
	return type;
}

void notify_charger_type(struct otg_dev *dev_p)
{
	usb_dbg("+\n");
	blocking_notifier_call_chain(&dev_p->charger_type_notifier,
				   dev_p->charger_type, dev_p);
	usb_dbg("-\n");
}

static int is_usb_cable_connected(void)
{
	struct otg_dev *dev = otg_dev_p;
	int ret = 1;

	if (!dev->fpga_flag) {
		ret = hisi_pmic_get_vbus_status();
	}
	return ret;
}

static inline bool enumerate_allowed(struct otg_dev *dev)
{
	if (dev->bc_again_delay_time == BC_AGAIN_DELAY_TIME_1)
		return false;

	/* do not start peripheral if real charger connected */
	return ((dev->charger_type == CHARGER_TYPE_SDP)
			|| (dev->charger_type == CHARGER_TYPE_UNKNOWN)
			|| (dev->charger_type == CHARGER_TYPE_CDP));
}

static inline bool sleep_allowed(struct otg_dev *dev)
{
	return ((dev->charger_type == CHARGER_TYPE_DCP)
			|| (dev->charger_type == CHARGER_TYPE_UNKNOWN));
}

static inline bool bc_again_allowed(struct otg_dev *dev)
{
	if (dev->bc_unknown_again_flag)
		return ((dev->charger_type == CHARGER_TYPE_SDP)
				|| (dev->charger_type == CHARGER_TYPE_UNKNOWN)
				|| (dev->charger_type == CHARGER_TYPE_CDP));
	else
		return ((dev->charger_type == CHARGER_TYPE_SDP)
				|| (dev->charger_type == CHARGER_TYPE_CDP));
}

int dwc_otg_hicommon_is_power_on(void)
{
	return atomic_read(&is_power_on);
}

void dwc_otg_hicommon_set_prtpower(unsigned int val)
{
	struct otg_dev *p = otg_dev_p;
	enum hisi_charger_type new;

	if (0 == val) {
		new = CHARGER_TYPE_NONE;
	} else {
		new = PLEASE_PROVIDE_POWER;
	}
	if (p->charger_type != new) {
		usb_dbg("set port power %d\n", val);
		p->charger_type = new;
		notify_charger_type(p);
	}
}

static int start_peripheral(struct dwc_otg_device *dwc_otg_dev)
{
	struct otg_dev *p = otg_dev_p;
	int ret = 0;

	/* reinit usb core */
	if (!dwc_otg_dev->core_if->core_params->dma_desc_enable) {
		usb_dbg("enable desc dma\n");
		dwc_otg_dev->core_if->core_params->dma_desc_enable = 1;
	}

	dwc_otg_core_init(dwc_otg_dev->core_if);
	if (dwc_otg_is_device_mode(dwc_otg_dev->core_if)) {
		cil_pcd_start(dwc_otg_dev->core_if);
		dwc_otg_dev->core_if->op_state = B_PERIPHERAL;
	} else {
		usb_dbg("error in host mode!\n");
	}

	/* enable usb core interrupt */
	dwc_otg_enable_global_interrupts(dwc_otg_dev->core_if);

	/* do software disconnect and connect for host enum again */
	dwc_otg_pcd_disconnect_us(dwc_otg_dev->pcd, 50);
	udelay(100);

	if (p->gadget_initialized)
		dwc_otg_pcd_connect_us(dwc_otg_dev->pcd, 50);
	else{
		if (CHARGER_TYPE_CDP == p->charger_type) {
				usb_dbg("it need connect over 2ms while CDP \n");
				dwc_otg_pcd_connect_us(dwc_otg_dev->pcd, 2000);
				dwc_otg_pcd_disconnect_us(dwc_otg_dev->pcd, 50);
		}
		usb_dbg("gadget is not initialized!\n");
	}

	return ret;
}

static void stop_peripheral(struct dwc_otg_device *dwc_otg_dev)
{
	/* For send uevent of disconnect. */
	if (dwc_otg_dev->pcd->fops->disconnect)
		dwc_otg_dev->pcd->fops->disconnect(dwc_otg_dev->pcd);

	dwc_otg_disable_global_interrupts(dwc_otg_dev->core_if);
	dwc_otg_mask_all_interrupts(dwc_otg_dev->core_if);
	dwc_otg_clear_all_interrupts(dwc_otg_dev->core_if);

	msleep(100);
}

static int start_host(struct dwc_otg_device *dwc_otg_dev)
{
	int count = 0;

	if (dwc_otg_dev->core_if->core_params->dma_desc_enable) {
		usb_dbg("disable desc dma\n");
		dwc_otg_dev->core_if->core_params->dma_desc_enable = 0;
	}
	dwc_otg_disable_global_interrupts(dwc_otg_dev->core_if);
	while (!dwc_otg_is_host_mode(dwc_otg_dev->core_if)) {
		msleep(10);
		if (++count > 100) {
			printk(KERN_ERR "wait host mode timeout\n");
			return -EBUSY;
		}
	}
	dwc_otg_core_init(dwc_otg_dev->core_if);

	msleep(100);
	if (dwc_otg_is_host_mode(dwc_otg_dev->core_if)) {
		dwc_otg_dev->core_if->op_state = A_HOST;
		usb_dbg("dwc otg is host mode.\n");
	} else {
		usb_dbg("error in device mode!\n");
	}

	cil_hcd_start(dwc_otg_dev->core_if);

	dwc_otg_hicommon_set_prtpower(1);

	return 0;
}

static void stop_host(struct dwc_otg_device *dwc_otg_dev)
{
	extern atomic_t dwc_otg_hcd_bus_state;

	usb_dbg("+\n");
	dwc_otg_hicommon_set_prtpower(0);

	while (0 != atomic_read(&dwc_otg_hcd_bus_state)) {
		msleep(200);
	}
	usb_dbg("-\n");

	dwc_otg_disable_global_interrupts(dwc_otg_dev->core_if);
	dwc_otg_mask_all_interrupts(dwc_otg_dev->core_if);
	dwc_otg_clear_all_interrupts(dwc_otg_dev->core_if);
}

static int off_to_device(struct otg_dev *p)
{
	struct dwc_otg_device *dwc_otg_dev = lm_get_drvdata(p->lm_dev);
	int ret;

	/* open ldo and clock */
	ret = hw_setup(p, 0);
	if (ret) {
		return ret;
	}

	/* disable usb core interrupt */
	dwc_otg_disable_global_interrupts(dwc_otg_dev->core_if);

	/*if the platform support,it need check voltage*/
	if(p->usb_phy_ops->check_voltage)
		p->usb_phy_ops->check_voltage(p);

	/* Get charger type. */
	detect_charger_type(p);

	if (enumerate_allowed(p)) {
		ret = start_peripheral(dwc_otg_dev);
		if (ret) {
			usb_err("start_peripheral failed\n");
			hw_shutdown(p);
			return ret;
		}
	} else
		usb_dbg("[CA.energy] off_to_device do nothing\n");

	return 0;
}

static void device_to_off(struct otg_dev *dev_p)
{
	struct dwc_otg_device *dwc_otg_dev = lm_get_drvdata(dev_p->lm_dev);

	if (enumerate_allowed(dev_p)) {
		stop_peripheral(dwc_otg_dev);
	} else {
		usb_dbg("[CA.energy] device_to_off disable vdp src!\n");
		disable_vdp_src(dev_p);
	}

	hw_shutdown(dev_p);
}

static int off_to_host(struct otg_dev *p)
{
	struct dwc_otg_device *dwc_otg_dev = lm_get_drvdata(p->lm_dev);
	int ret;

	ret = hw_setup(p, 1);
	if (ret) {
		return ret;
	}

	ret = start_host(dwc_otg_dev);
	if (ret) {
		hw_shutdown(p);
		return ret;
	}

	return 0;
}

static void host_to_off(struct otg_dev *p)
{
	struct dwc_otg_device *dwc_otg_dev = lm_get_drvdata(p->lm_dev);

	stop_host(dwc_otg_dev);
	hw_shutdown(p);
}

static void bc_again(struct otg_dev *p)
{
	int ret;
	bool schdule = false;
	unsigned int bc_again_delay_time = 0;
	struct dwc_otg_device *dwc_otg_dev = lm_get_drvdata(p->lm_dev);

	usb_dbg("+\n");
	/*
	 * STEP 1
	 */
	if (enumerate_allowed(p))
		stop_peripheral(dwc_otg_dev);

	/*
	 * STEP 2
	 */
	detect_charger_type(p);
	notify_charger_type(p);
	if (p->charger_type == CHARGER_TYPE_UNKNOWN) {

		if (p->bc_again_delay_time == BC_AGAIN_DELAY_TIME_1) {
			p->bc_again_delay_time = BC_AGAIN_DELAY_TIME_2;
			schdule = true;
		}
		bc_again_delay_time = p->bc_again_delay_time;
	} else {
		p->bc_again_delay_time = 0;
	}

	/*
	 * STEP 3
	 */
	/* must recheck enumerate allowed, because the charger type maybe
	 * changed */
	if (enumerate_allowed(p)) {
		/* start peripheral */
		ret = start_peripheral(dwc_otg_dev);
		if (ret) {
			hw_shutdown(p);
			dwc_otg_hicommon_wake_unlock();
			usb_err("start peripheral error\n");
			return;
		}
	} else {
		usb_dbg("it is not SDP connected,so do nothing\n");
	}

	if (schdule) {
		ret = queue_delayed_work(system_power_efficient_wq,
				&p->bc_again_work,
				msecs_to_jiffies(bc_again_delay_time));
		usb_dbg("schedule ret:%d, run bc_again_work %dms later\n",
			ret, bc_again_delay_time);
	}

	usb_dbg("-\n");
}

void hisi_usb_otg_bc_again(void)
{
	struct otg_dev *p = otg_dev_p;

	usb_dbg("+\n");

	if (!p) {
		usb_err("No usb module, can't call bc again api!\n");
		return;
	}

	if ((1 == p->bc_again_flag) && (BC_AGAIN_ONCE == p->bc_unknown_again_flag)) {
		mutex_lock(&p->lock);
		/* user triggered bc_again allowed only when charger type is unknown */
		if (p->charger_type == CHARGER_TYPE_UNKNOWN) {
			usb_dbg("charger_type is UNKNOWN, start bc_again_work!\n");
			bc_again(p);
		}
		mutex_unlock(&p->lock);
	} else
		usb_dbg("hisi_usb_otg_bc_again do nothing!\n");

	usb_dbg("-\n");
}
EXPORT_SYMBOL_GPL(hisi_usb_otg_bc_again);

static void bc_again_work(struct work_struct *work)
{
	struct otg_dev *p = container_of(work, struct otg_dev,
			bc_again_work.work);

	usb_dbg("+\n");
	mutex_lock(&p->lock);

	/* we are here because it's detected as SDP before */
	if (bc_again_allowed(p)) {
		usb_err("charger_type is not DCP, start bc_again_work\n");
		bc_again(p);
	}

	mutex_unlock(&p->lock);
	usb_dbg("-\n");
}

/**
 * In some cases, DCP is detected as SDP wrongly. To avoid this,
 * start bc_again delay work to detect charger type once more.
 * If later the enum process is executed, then it's a real SDP, so
 * the work will be canceled.
 */
void schdule_bc_again(struct otg_dev *p)
{
	int ret;
	unsigned int bc_again_delay_time;

	usb_dbg("+\n");

	if (!p->bc_again_flag)
		return;

	if ((p->charger_type == CHARGER_TYPE_UNKNOWN)
		&& (BC_AGAIN_TWICE == p->bc_unknown_again_flag))
		p->bc_again_delay_time = BC_AGAIN_DELAY_TIME_1;
	else
		p->bc_again_delay_time = BC_AGAIN_DELAY_TIME_2;

	bc_again_delay_time = p->bc_again_delay_time;

	ret = queue_delayed_work(system_power_efficient_wq,
			&p->bc_again_work,
			msecs_to_jiffies(bc_again_delay_time));
	usb_dbg("schedule ret:%d, run bc_again_work %dms later\n",
		ret, bc_again_delay_time);

	usb_dbg("-\n");
}

void cancel_bc_again(struct otg_dev *p, int sync)
{
	usb_dbg("+\n");
	if (p->bc_again_flag) {
		int ret;
		if (sync)
			ret = cancel_delayed_work_sync(&p->bc_again_work);
		else
			ret = cancel_delayed_work(&p->bc_again_work);

		usb_dbg("cancel_delayed_work(result:%d)\n", ret);
		p->bc_again_delay_time = 0;
	}
	usb_dbg("-\n");
}

void hisi_usb_cancel_bc_again(int sync)
{
	if (!otg_dev_p) {
		usb_err("otg_dev_p is null\n");
		return;
	}
	cancel_bc_again(otg_dev_p, sync);
	if (otg_dev_p->charger_type == CHARGER_TYPE_UNKNOWN) {
		otg_dev_p->charger_type = CHARGER_TYPE_SDP;
	}
}

int hisi_usb_bc_init(struct otg_dev *p)
{
	struct device *dev = &p->pdev->dev;

	usb_dbg("+\n");

	if (of_property_read_u32(dev->of_node, "bc_again_flag",
			    &(p->bc_again_flag))) {
		p->bc_again_flag = 0;
	}

	if (p->bc_again_flag) {
		INIT_DELAYED_WORK(&p->bc_again_work, bc_again_work);
		if (of_property_read_u32(dev->of_node, "bc_unknown_again_flag",
			    &(p->bc_unknown_again_flag))) {
			p->bc_unknown_again_flag = 0;
		}
	} else
		p->bc_unknown_again_flag = 0;

	usb_dbg("-\n");
	return 0;
}

/*
 * create event queue
 * event_queue: event queue handle
 * count: set the queue max node
 */
int event_queue_creat(struct hiusb_event_queue *event_queue, unsigned int count)
{
	if (!event_queue) {
		printk(KERN_ERR "[event_queue_creat]bad argument (0x%pK)\n", event_queue);
		return -EINVAL;
	}

	count = (count >= MAX_EVENT_COUNT ? MAX_EVENT_COUNT : count);
	event_queue->max_event = count;
	event_queue->num_event =
		(count >= EVENT_QUEUE_UNIT ? EVENT_QUEUE_UNIT : count);

	event_queue->event = kzalloc(event_queue->num_event * sizeof(enum otg_dev_event_type), GFP_KERNEL);
	if (!event_queue->event) {
		printk(KERN_ERR "[event_queue_creat]:Can't alloc space:%d!\n", event_queue->num_event);
		return -ENOMEM;
	}

	event_queue->enpos = 0;
	event_queue->depos = 0;
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;

	return 0;
}

void event_queue_destroy(struct hiusb_event_queue *event_queue)
{
	if (!event_queue) {
		return ;
	}

	kfree(event_queue->event);
	event_queue->event = NULL;
	event_queue->enpos = 0;
	event_queue->depos = 0;
	event_queue->num_event = 0;
	event_queue->max_event = 0;
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;
}

/*
 * check if the queue is full
 * return true means full, false is not.
 */
int event_queue_isfull(struct hiusb_event_queue *event_queue)
{
	if (!event_queue) {
		return -EINVAL;
	}

	return (((event_queue->enpos + 1) % event_queue->num_event) == (event_queue->depos));
}

/*
 * check if the queue is full
 * return true means empty, false or not.
 */
int event_queue_isempty(struct hiusb_event_queue *event_queue)
{
	if (!event_queue) {
		return -EINVAL;
	}

	return (event_queue->enpos == event_queue->depos);
}

static inline void event_queue_set_overlay(struct hiusb_event_queue *event_queue)
{
	if (event_queue->overlay) {
		return;
	}
	event_queue->overlay = 1;
	event_queue->overlay_index = event_queue->enpos;
}

static inline void event_queue_clear_overlay(struct hiusb_event_queue *event_queue)
{
	event_queue->overlay = 0;
	event_queue->overlay_index = 0;
}

/*
 * put the new event en queue
 * if the event_queue is full, return -ENOSPC
 */
int event_enqueue(struct hiusb_event_queue *event_queue, enum otg_dev_event_type event)
{
	/* no need verify argument, isfull will check it */
	if (event_queue_isfull(event_queue)) {
		printk(KERN_ERR "event_queue full!\n");
		return -ENOSPC;
	}

	if (event_queue->overlay) {
		if (event_queue->overlay_index == event_queue->enpos) {
			event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
		}

		if (event_queue_isempty(event_queue)) {
			printk(KERN_ERR "overlay and queue isempty? just enqueue!\n");
			event_queue->overlay_index = ((event_queue->overlay_index + 1) % event_queue->num_event);
			event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
			event_queue->overlay = 0;
		}

		event_queue->event[event_queue->overlay_index] = event;
	} else {
		event_queue->event[event_queue->enpos] = event;
		event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
	}

	return 0;
}

/*
 * get event frome event_queue
 * this function never return fail
 * if the event_queue is empty, return NONE_EVENT
 */
enum otg_dev_event_type event_dequeue(struct hiusb_event_queue *event_queue)
{
	enum otg_dev_event_type event;

	/* no need verify argument, isempty will check it */
	if (event_queue_isempty(event_queue)) {
		return NONE_EVENT;
	}

	event = event_queue->event[event_queue->depos];
	event_queue->depos = ((event_queue->depos + 1) % event_queue->num_event);

	return event;
}

static void handle_event(struct otg_dev *p, enum otg_dev_event_type event)
{
	int ret = 0;

	usb_err("[handle_event_type]: %s\n", event_type_string(event));
	switch (event) {
	case CHARGER_CONNECT_EVENT:
		if (OTG_DEV_DEVICE == p->status) {
			usb_dbg("Already in device mode, do nothing.\n");
		} else if (OTG_DEV_OFF == p->status) {
			/*it close otg gpio's irq when the phone's mode changed form off to device*/
			blocking_notifier_call_chain(&p->otg_irq_notifier, (unsigned long)0, p);

			ret = off_to_device(p);
			if (ret)
				return ;

			notify_charger_type(p);

			/* In some cases, DCP is detected as SDP wrongly. To avoid this,
			 * start bc_again delay work to detect charger type once more.
			 * If later the enum process is executed, then it's a real SDP, so
			 * the work will be canceled.
			 */
			if (bc_again_allowed(p))
				schdule_bc_again(p);

			p->status = OTG_DEV_DEVICE;

			/* allow sleep when USB cable connected */
			if (sleep_allowed(p)) {
				usb_dbg("[CA.energy]: detect charger type is %s, unlock wake!\n",
					p->charger_type == CHARGER_TYPE_DCP ? "DCP" : "UNKNOW");
				dwc_otg_hicommon_wake_unlock();
			} else {
				dwc_otg_hicommon_wake_lock();
			}

			usb_dbg("hisi usb status: OFF -> DEVICE\n");
		} else if (OTG_DEV_HOST == p->status) {
			usb_dbg("Charger connect intrrupt in HOST mode \n");
		}

		break;

	case CHARGER_DISCONNECT_EVENT:
		p->need_disable_vdp = 0;
		if (OTG_DEV_OFF == p->status) {
			usb_dbg("Charger disconnect interrupt, but Already in OFF mode.\n");
		} else if (OTG_DEV_DEVICE == p->status) {
			disable_vdp_src(p);

			device_to_off(p);

			cancel_bc_again(p, 0);

			p->charger_type = CHARGER_TYPE_NONE;
			notify_charger_type(p);

			p->status = OTG_DEV_OFF;

			/*it open otg gpio's irq when the phone's mode changed form device to off*/
			blocking_notifier_call_chain(&p->otg_irq_notifier, (unsigned long)1, p);

			/* allow system go to sleep */
			dwc_otg_hicommon_wake_unlock();

			usb_dbg("hisi usb status: DEVICE -> OFF\n");
		} else if (OTG_DEV_HOST == p->status) {
			usb_dbg("Charger disconnect intrrupt in HOST mode\n");
		}
		break;

	case ID_FALL_EVENT:
		dwc_otg_hicommon_wake_lock();
		if (OTG_DEV_OFF == p->status) {
			ret = off_to_host(p);
			if (ret) {
				dwc_otg_hicommon_wake_unlock();
				return ;
			}

			p->status = OTG_DEV_HOST;

			usb_dbg("hisi usb_status: OFF -> HOST\n");
		} else if (OTG_DEV_DEVICE == p->status) {
			usb_dbg("hisi usb_status: DEVICE -> HOST\n");
			p->status = OTG_DEV_HOST;
		} else if (OTG_DEV_HOST == p->status) {
			usb_dbg("ID fall event, already in host mode\n");
		}
		break;

	case ID_RISE_EVENT:
		if (OTG_DEV_HOST == p->status) {
			host_to_off(p);

			p->status = OTG_DEV_OFF;

			usb_dbg("hiusb_status: HOST -> OFF\n");
		} else if (OTG_DEV_DEVICE == p->status) {
			usb_dbg("hiusb_status: HOST -> DEVICE\n");
		} else if (OTG_DEV_OFF == p->status) {
			usb_dbg("ID rise event, Already in OFF mode\n");
		}
		dwc_otg_hicommon_wake_unlock();
		break;

	default:
		usb_dbg("illegal event type!\n");
		break;
	}
}

static void event_work(struct work_struct *work)
{
	unsigned long flags;
	enum otg_dev_event_type event;
	struct otg_dev *p = otg_dev_p;

	mutex_lock(&p->lock);

	usb_err("+\n");

	while (!event_queue_isempty(&p->event_queue)) {
		spin_lock_irqsave(&(p->event_lock), flags);
		event = event_dequeue(&p->event_queue);
		spin_unlock_irqrestore(&(p->event_lock), flags);

		handle_event(p, event);
	}
	event_queue_clear_overlay(&p->event_queue);

	usb_err("-\n");
	mutex_unlock(&p->lock);
	return;
}

static int event_type_check(enum otg_dev_event_type last_event,
			    enum otg_dev_event_type current_event)
{
	int ret = 0;

	if (last_event == NONE_EVENT)
		return 1;

	switch (current_event) {
	case CHARGER_CONNECT_EVENT:
		if ((last_event == CHARGER_DISCONNECT_EVENT)
			|| (last_event == ID_RISE_EVENT))
			ret = 1;
		break;
	case CHARGER_DISCONNECT_EVENT:
		if (last_event == CHARGER_CONNECT_EVENT)
			ret = 1;
		break;
	case ID_FALL_EVENT:
		if ((last_event == CHARGER_DISCONNECT_EVENT)
			|| (last_event == ID_RISE_EVENT))
			ret = 1;
		break;
	case ID_RISE_EVENT:
		if (last_event == ID_FALL_EVENT)
			ret = 1;
		break;
	default:
		break;
	}
	return ret;
}

int hisi_usb_otg_event(enum otg_dev_event_type event)
{
	struct otg_dev *dev_p = otg_dev_p;
	unsigned long flags;
	int ret = 0;

	if (!otg_dev_p) {
		usb_err("No usb module, can't handle event:%d.\n", event);
		return -ENOENT;
	}

	if (dev_p->eventmask) {
		usb_dbg("eventmask enabled, mask all events.\n");
		return ret;
	}

	if (0 == atomic_read(&is_event_work_ready)) {
		return 0;
	}

	spin_lock_irqsave(&(dev_p->event_lock), flags);
	if (event_type_check(dev_p->event, event)) {
		usb_dbg("event: %d\n", event);
		dev_p->event = event;

		if ((CHARGER_CONNECT_EVENT == event)
				|| (CHARGER_DISCONNECT_EVENT == event))
			dwc_otg_hicommon_wake_lock();

		if (!event_enqueue(&dev_p->event_queue, event)) {
			ret = queue_work(system_power_efficient_wq,
					&dev_p->event_work);
			if (!ret) {
				usb_err("schedule event_work wait:%d]\n", event);
			}
		} else {
			usb_err("hisi_usb_otg_event can't enqueue event:%d\n", event);
		}

		if ((ID_RISE_EVENT == event) || (CHARGER_DISCONNECT_EVENT == event)) {
			event_queue_set_overlay(&dev_p->event_queue);
		}
	}
	spin_unlock_irqrestore(&(dev_p->event_lock), flags);
	return ret;
}

void dwc_otg_hicommon_wake_lock(void)
{
	if (!wake_lock_active(&otg_dev_p->wake_lock)) {
		usb_dbg("usb otg wake lock\n");
		wake_lock(&otg_dev_p->wake_lock);
	}
}

void dwc_otg_hicommon_wake_unlock(void)
{
	if (wake_lock_active(&otg_dev_p->wake_lock)) {
		usb_dbg("usb otg wake unlock\n");
		wake_unlock(&otg_dev_p->wake_lock);
	}
}

int dwc_otg_hicommon_pullup(struct usb_gadget *gadget, int is_on)
{
	dwc_otg_device_t *dwc_otg_dev;

	if (gadget == 0) {
		return -ENODEV;
	}

	dwc_otg_dev = lm_get_drvdata(otg_dev_p->lm_dev);

	/* lock */
	mutex_lock(&otg_dev_p->lock);

	if (is_on) {
		if (otg_dev_p->need_disable_vdp) {
			otg_dev_p->need_disable_vdp = 0;
			disable_vdp_src(otg_dev_p);
			udelay(3000);
		}
		otg_dev_p->gadget_initialized = 1;
	} else {
		otg_dev_p->gadget_initialized = 0;
	}

	if ((OTG_DEV_DEVICE == otg_dev_p->status)) {
		if (is_on)
			dwc_otg_pcd_connect_us(dwc_otg_dev->pcd, 50);
		else {
			dwc_otg_pcd_disconnect_us(dwc_otg_dev->pcd, 50);
			msleep(200);
		}
	}

	/* unlock */
	mutex_unlock(&otg_dev_p->lock);
	return 0;
}

extern struct usb_gadget_ops dwc_otg_pcd_ops;

int dwc_otg_hicommon_probe_stage2(void)
{
	struct otg_dev *otg_device = otg_dev_p;
	dwc_otg_device_t *dwc_otg_dev = lm_get_drvdata(otg_device->lm_dev);

	usb_dbg("[%s]+\n", __func__);

	mutex_lock(&otg_device->lock);

	dwc_otg_pcd_ops.pullup = dwc_otg_hicommon_pullup;

	atomic_set(&is_event_work_ready, 1);
	usb_dbg("is_event_work_ready: %d\n", atomic_read(&is_event_work_ready));

	if (dwc_otg_is_host_mode(dwc_otg_dev->core_if)) {
		usb_dbg("host mode\n");

		if (start_host(dwc_otg_dev))
			usb_err("start_host failed\n");

		otg_device->status = OTG_DEV_HOST;
		otg_device->event = ID_FALL_EVENT;
	} else if (0 == is_usb_cable_connected()) {
		usb_dbg("off mode\n");

		stop_peripheral(dwc_otg_dev);
		hw_shutdown(otg_device);

		otg_device->status = OTG_DEV_OFF;
		otg_device->event = CHARGER_DISCONNECT_EVENT;
	} else {
		usb_dbg("device mode\n");

		detect_charger_type(otg_device);
		notify_charger_type(otg_device);

		if (otg_device->charger_type == CHARGER_TYPE_CDP) {
			usb_dbg("it needs enable VDP_SRC while detect CDP!\n");
			otg_device->need_disable_vdp = 1;
			enable_vdp_src(otg_device);
		}

		if (enumerate_allowed(otg_device)){
			if (start_peripheral(dwc_otg_dev))
				usb_err("start_peripheral failed\n");
		} else
			usb_dbg("connected to charger, no start_peripheral\n");

		otg_device->status = OTG_DEV_DEVICE;
		otg_device->event = CHARGER_CONNECT_EVENT;

		/* allow sleep when USB charger connected */
		if (sleep_allowed(otg_device))
			dwc_otg_hicommon_wake_unlock();
		else
			dwc_otg_hicommon_wake_lock();
	}

	mutex_unlock(&otg_device->lock);

	usb_dbg("[%s]-\n", __func__);
	return 0;
}

int dwc_otg_hicommon_probe(struct otg_dev *dev_p)
{
	int ret = 0;
	int irq = 0;
	struct lm_device *lm_dev = NULL;
	struct platform_device *pdev = dev_p->pdev;
	struct resource *res = NULL;

	usb_dbg("[%s]+\n", __func__);

	platform_set_drvdata(pdev, dev_p);

	dev_p->event = NONE_EVENT;
	dev_p->charger_type = CHARGER_TYPE_NONE;
	dev_p->fake_charger_type = CHARGER_TYPE_NONE;
	dev_p->status = OTG_DEV_OFF;
	dev_p->hcd_status = HCD_OFF;
	dev_p->gadget_initialized = 0;
	dev_p->dwc_otg_irq_enabled = true;
	dev_p->is_regu_on = 0;
	dev_p->need_disable_vdp = 0;
	dev_p->eventmask = 0;
	ret = event_queue_creat(&dev_p->event_queue, MAX_EVENT_COUNT);
	if (ret) {
		usb_err("create event queue failure!\n");
		return ret;
	}

	INIT_WORK(&dev_p->event_work, event_work);
	wake_lock_init(&dev_p->wake_lock, WAKE_LOCK_SUSPEND, "usb_wake_lock");
	spin_lock_init(&dev_p->event_lock);
	mutex_init(&dev_p->lock);
	BLOCKING_INIT_NOTIFIER_HEAD(&dev_p->charger_type_notifier);
	BLOCKING_INIT_NOTIFIER_HEAD(&dev_p->otg_irq_notifier);

	/* save dev_p to global for myself */
	otg_dev_p = dev_p;

	ret = get_resource(dev_p);
	if (ret) {
		usb_err("get resource failed 1!\n");
		return ret;
	}

	/* fpga only */
	if (dev_p->fpga_usb_mode_gpio > 0) {
		ret = gpio_request((unsigned)dev_p->fpga_usb_mode_gpio, NULL);
		if (ret) {
			/* request gpio failure! */
			usb_err("request gpio fail.[%d], gpio num:%d!\n",
				ret, dev_p->fpga_usb_mode_gpio);
		}
	}

	ret = hisi_usb_bc_init(dev_p);
	if (ret)
		usb_err("hisi_usb_bc_init failed\n");

	ret = hw_setup(dev_p, 0);
	if (ret) {
		usb_err("hw setup failed!\n");
		put_resource(dev_p);
		return ret;
	}

	dev_p->status = OTG_DEV_DEVICE;

	ret = create_attr_file(&pdev->dev);
	if (ret) {
		usb_err("create_attr_file failed!\n");
		goto error;
	}

	/*
	 * register the dwc otg device
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		usb_err("get resource failed 2!\n");
		ret = -EBUSY;
		goto error;
	}

	usb_dbg("res start: 0x%llx, res end: 0x%llx, res flags: %ld\n",
		res->start, res->end, res->flags);

	irq = platform_get_irq(pdev, 0);
	if (!irq) {
		usb_err("get irq failed!\n");
		ret = -EBUSY;
		goto error;
	}

	lm_dev = devm_kzalloc(&pdev->dev, sizeof(*lm_dev), GFP_KERNEL);
	if (!lm_dev) {
		usb_err("alloc lm_dev failed! not enough memory\n");
		ret = -ENOMEM;
		goto error;
	}

	dev_p->lm_dev = lm_dev;

	lm_dev->resource.start = res->start;
	lm_dev->resource.end = res->end;
	lm_dev->resource.flags = res->flags;
	lm_dev->irq = irq;
	lm_dev->id = -1;
	lm_dev->dev.init_name = "hisi-usb-otg";
	/* lm_dev->dev.parent = &pdev->dev; */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	of_dma_configure(&lm_dev->dev, pdev->dev.of_node);
#endif

	ret = lm_device_register(lm_dev);
	if (ret) {
		usb_err("register dwc otg device failed\n");
		goto error;
	}
	usb_dbg("register dwc otg device done\n");

	pm_runtime_enable(&lm_dev->dev);
	pm_runtime_get_sync(&lm_dev->dev);
	pm_runtime_forbid(&lm_dev->dev);

	dwc_otg_hicommon_probe_stage2();

	pm_runtime_allow(&lm_dev->dev);

	usb_dbg("[%s]-\n", __func__);

	return 0;
error:
	hw_shutdown(dev_p);
	put_resource(dev_p);
	return ret;

}

int dwc_otg_hicommon_remove(struct otg_dev *dev_p)
{
	if ((dev_p->status == OTG_DEV_DEVICE) || (dev_p->status == OTG_DEV_HOST)) {
		hw_shutdown(dev_p);
	}

	event_queue_destroy(&dev_p->event_queue);

	put_resource(dev_p);

	pm_runtime_put_sync(&dev_p->lm_dev->dev);
	pm_runtime_disable(&dev_p->lm_dev->dev);

	return 0;
}

static int dwc_otg_hicommon_suspend(struct device *dev)
{
	struct otg_dev *dev_p = dev_get_drvdata(dev);

	usb_dbg("+\n");

	if (!mutex_trylock(&dev_p->lock)) {
		usb_err("%s: mutex_trylock.\n", __func__);
		return -EAGAIN;
	}

	usb_dbg("status %d\n", dev_p->status);
	if (dev_p->status == OTG_DEV_DEVICE) {
		device_to_off(dev_p);
	} else if (dev_p->status == OTG_DEV_HOST) {
		host_to_off(dev_p);
	}

	usb_dbg("-\n");

	return 0;
}

static int dwc_otg_hicommon_resume(struct device *dev)
{
	struct otg_dev *dev_p = dev_get_drvdata(dev);
	int ret;

	usb_dbg("+\n");

	usb_dbg("status %d\n", dev_p->status);
	if (dev_p->status == OTG_DEV_DEVICE) {
		ret = off_to_device(dev_p);
		if (ret) {
			usb_err("off_to_device failed\n");
			dev_p->status = OTG_DEV_OFF;
		}
	} else if (dev_p->status == OTG_DEV_HOST) {
		ret = off_to_host(dev_p);
		if (ret) {
			usb_err("off_to_host failed\n");
			dev_p->status = OTG_DEV_OFF;
		}
	}

	mutex_unlock(&dev_p->lock);

	usb_dbg("-\n");
	return 0;
}

#ifdef CONFIG_PM
struct dev_pm_ops dwc_otg_dev_pm_ops = {
	.prepare = NULL,
	.complete = NULL,
	.suspend = dwc_otg_hicommon_suspend,
	.resume = dwc_otg_hicommon_resume,

	.runtime_suspend = NULL,
	.runtime_resume = NULL,
	.runtime_idle = NULL,
};
#endif
