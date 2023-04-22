#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <linux/usb/ch9.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/usb/audio.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#include "dwc3-hisi.h"
#include "core.h"
#include "dwc3-otg.h"
#include "hisi_usb_vbus.h"
#include "dwc3-hifi-usb.h"

struct hisi_dwc3_device *hisi_dwc3_dev;
atomic_t hisi_dwc3_power_on = ATOMIC_INIT(0);
atomic_t hisi_dwc3_cptest_flag = ATOMIC_INIT(0);
#ifdef CONFIG_HISI_DEBUG_FS
struct usb3_hisi_debug_node hisi_debug_data = {
	.hisi_dwc3_linkstate_flag = ATOMIC_INIT(0),
	.hisi_dwc3_noc_flag = ATOMIC_INIT(0),
	.usb_test_noc_addr = 0,
	.hisi_dwc3_lbintpll_flag = ATOMIC_INIT(0)
};
#endif

static void schedule_bc_again(struct hisi_dwc3_device *hisi_dwc);
static void cancel_bc_again(struct hisi_dwc3_device *hisi_dwc, int sync);
static inline int hisi_dwc3_phy_init(struct hisi_dwc3_device *hisi_dwc);
static inline int hisi_dwc3_phy_shutdown(struct hisi_dwc3_device *hisi_dwc);
static int hisi_dwc3_usb20_phy_init(struct hisi_dwc3_device *hisi_dwc3, unsigned int combophy_flag);
static int hisi_dwc3_usb20_phy_shutdown(struct hisi_dwc3_device *hisi_dwc,
		unsigned int combophy_flag, unsigned int keep_power);
#ifdef CONFIG_SND
extern int usbaudio_nv_is_ready(void);
#else
static inline int usbaudio_nv_is_ready(void){return 1;}
#endif

int hisi_dwc3_is_fpga(void)
{
	if (!hisi_dwc3_dev) {
		usb_err("usb driver not probed!\n");
		return 0;
	}

	return !!hisi_dwc3_dev->fpga_flag;
}

void hisi_usb_unreset_phy_if_fpga(void)
{
	unsigned gpio;

	if (!hisi_dwc3_dev || hisi_dwc3_dev->fpga_phy_reset_gpio < 0)
		return;

	gpio = (unsigned)hisi_dwc3_dev->fpga_phy_reset_gpio;

	gpio_direction_output(gpio, 1);
	udelay(100);

	gpio_direction_output(gpio, 0);
	udelay(100);
}

void hisi_usb_switch_sharedphy_if_fpga(int to_hifi)
{
	if (!hisi_dwc3_dev || hisi_dwc3_dev->fpga_phy_switch_gpio < 0)
		return;

	gpio_direction_output((unsigned)hisi_dwc3_dev->fpga_phy_switch_gpio,
			!!to_hifi);
	udelay(100);
}


int hisi_dwc3_clean_cptest(void)
{
	atomic_set(&hisi_dwc3_cptest_flag, 0);

	return atomic_read(&hisi_dwc3_cptest_flag);
}

#ifdef CONFIG_HISI_DEBUG_FS
static ssize_t hisi_dwc3_cptest_store(void *dev_data, char *buf, size_t size)
{
	usb_err("[USB.CPTEST] Start usb CP test!\n");

	atomic_set(&hisi_dwc3_cptest_flag, 1);
	return size;
}

static ssize_t hisi_dwc3_cptest_show(void *dev_data, char *buf, size_t size)
{
	int val = 0;

	val = atomic_read(&hisi_dwc3_cptest_flag);

	return scnprintf(buf, size, "[cptest mode: %d]\n", val);
}


/*
 * dump linke state.
 */
static ssize_t hisi_dwc3_linkstate_store(void *dev_data, char *buf, size_t size)
{
	usb_err("[USB.LINKSTATE] usb linkstate dump enable!\n");

	atomic_set(&hisi_debug_data.hisi_dwc3_linkstate_flag, 1);
	return size;
}

static ssize_t hisi_dwc3_linkstate_show(void *dev_data, char *buf, size_t size)
{
	int val = 0;

	val = atomic_read(&hisi_debug_data.hisi_dwc3_linkstate_flag);

	return scnprintf(buf, size, "[linkstate dump mode: %d]\n", val);
}

int hisi_dwc3_is_linkstate_dump(void)
{
	return atomic_read(&hisi_debug_data.hisi_dwc3_linkstate_flag);
}

/*
 * noc activity test.
 */
static ssize_t hisi_dwc3_noc_store(void *dev_data, char *buf, size_t size)
{
	usb_err("[USB.NOC] usb noc activity enable!\n");

	if (sscanf(buf, "0x%x", &hisi_debug_data.usb_test_noc_addr) != 1) {
		usb_err("inject addr!\n");
		return size;
	}

	usb_err("[USB.NOC] get noc addr:0x%x\n", hisi_debug_data.usb_test_noc_addr);

	atomic_set(&hisi_debug_data.hisi_dwc3_noc_flag, 1);
	return size;
}

static ssize_t hisi_dwc3_noc_show(void *dev_data, char *buf, size_t size)
{
	int val = 0;

	val = atomic_read(&hisi_debug_data.hisi_dwc3_noc_flag);

	return scnprintf(buf, size, "[noc activity test mode: %d]\n", val);
}

int hisi_dwc3_is_test_noc_addr(void)
{
	return atomic_read(&hisi_debug_data.hisi_dwc3_noc_flag);
}

uint32_t hisi_dwc3_get_noc_addr(uint32_t addr)
{
	if (!addr) {
		usb_err("[USB.NOC.TEST] get input addr:0x%x\n", addr);
	}

	return hisi_debug_data.usb_test_noc_addr;
}

/*
 * lbintpll clk select.
 */
static ssize_t hisi_dwc3_lbintpll_store(void *dev_data, char *buf, size_t size)
{
	usb_err("[USB.NOC] usb lbintpll clk select!\n");

	atomic_set(&hisi_debug_data.hisi_dwc3_lbintpll_flag, 1);
	return size;
}

static ssize_t hisi_dwc3_lbintpll_show(void *dev_data, char *buf, size_t size)
{
	int val = 0;

	val = atomic_read(&hisi_debug_data.hisi_dwc3_lbintpll_flag);

	return scnprintf(buf, size, "[lbintpll clk flag: %d]\n", val);
}

int hisi_dwc3_select_lbintpll_clk(void)
{
	return atomic_read(&hisi_debug_data.hisi_dwc3_lbintpll_flag);
}

static ssize_t hisi_dwc3_vboost_store(void *dev_data, char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;
	uint32_t vboost;

	if (sscanf(buf, "0x%x", &vboost) != 1) {
		usb_err("inject vboost!\n");
		return size;
	}

	if (vboost > 7) {
		usb_err("bad vboost input!\n");
		return size;
	}

	hisi_dwc->usb3_phy_tx_vboost_lvl = vboost;
	return size;
}

static ssize_t hisi_dwc3_vboost_show(void *dev_data, char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	return scnprintf(buf, size, "[vboost: 0x%x]\n", hisi_dwc->usb3_phy_tx_vboost_lvl);
}
#endif

static const char *const speed_names[] = {
	[USB_SPEED_UNKNOWN] = "UNKNOWN",
	[USB_SPEED_LOW] = "low-speed",
	[USB_SPEED_FULL] = "full-speed",
	[USB_SPEED_HIGH] = "high-speed",
	[USB_SPEED_WIRELESS] = "wireless",
	[USB_SPEED_SUPER] = "super-speed",
	[USB_SPEED_SUPER_PLUS] = "super-speed-plus",
};

enum usb_device_speed hisi_dwc3_get_dt_host_maxspeed(void)
{
	enum usb_device_speed speed = USB_SPEED_SUPER;
	struct device *dev;
	const char *maximum_speed = NULL;
	unsigned int i;
	int err;

	if (!hisi_dwc3_dev) {
		usb_err("hisi_dwc3_dev is null\n");
		return speed;
	}

	dev = &hisi_dwc3_dev->pdev->dev;

	err = device_property_read_string(dev, "host-maximum-speed", &maximum_speed);
	if (err < 0)
		return speed;

	for (i = 0; i < ARRAY_SIZE(speed_names); i++) {
		if (strncmp(maximum_speed, speed_names[i], strlen(speed_names[i])) == 0) {
			speed = (enum usb_device_speed)i;
			break;
		}
	}

	return speed;
}

void set_hisi_dwc3_power_flag(int val)
{
	unsigned long flags;
	struct dwc3 *dwc = NULL;

	if (dwc_otg_handler && dwc_otg_handler->dwc) {
		dwc = dwc_otg_handler->dwc;
		spin_lock_irqsave(&dwc->lock, flags);
		usb_dbg("get dwc3 lock\n");
	}

	atomic_set(&hisi_dwc3_power_on, val);
	usb_dbg("set hisi_dwc3_power_flag %d\n", val);

	if (dwc) {
		spin_unlock_irqrestore(&dwc->lock, flags);/*lint !e644 */
		usb_dbg("put dwc3 lock\n");
	}
}

int get_hisi_dwc3_power_flag(void)
{
	int val = 0;

	val = atomic_read(&hisi_dwc3_power_on);

	return val;
}

#define ENABLE_USB_TEST_PORT
#ifdef ENABLE_USB_TEST_PORT

static const char *const hisi_usb_state_names[] = {
	[USB_STATE_UNKNOWN] = "USB_STATE_UNKNOWN",
	[USB_STATE_OFF] = "USB_STATE_OFF",
	[USB_STATE_DEVICE] = "USB_STATE_DEVICE",
	[USB_STATE_HOST] = "USB_STATE_HOST",
	[USB_STATE_HIFI_USB] = "USB_STATE_HIFI_USB",
	[USB_STATE_HIFI_USB_HIBERNATE] = "USB_STATE_HIFI_USB_HIBERNATE",
	[USB_STATE_AP_USE_HIFIUSB] = "USB_STATE_AP_USE_HIFIUSB",
};

const char *hisi_usb_state_string(enum usb_state state)
{
	if (state < USB_STATE_UNKNOWN || state >= USB_STATE_ILLEGAL)
		return "illegal state";

	return hisi_usb_state_names[state];
}

static ssize_t plugusb_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct hisi_dwc3_device *hisi_dwc3 = platform_get_drvdata(pdev);

	if (!hisi_dwc3) {
		usb_err("hisi_dwc3 NULL\n");
		return scnprintf(buf, PAGE_SIZE, "hisi_dwc3 NULL\n");
	}
	return scnprintf(buf, PAGE_SIZE, "current state: %s\n" "usage: %s\n",
			hisi_usb_state_string(hisi_dwc3->state),
			"echo hoston/hostoff/deviceon/deviceoff > plugusb\n");
}
#ifdef HISI_PLUGUSB_TEST
static ssize_t plugusb_dp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
#ifdef CONFIG_CONTEXTHUB_PD
	struct pd_dpm_combphy_event event;
	int lunch = 1;

#ifdef CONFIG_TCPC_CLASS
	event.typec_orien = (TYPEC_PLUG_ORIEN_E)pd_dpm_get_cc_orientation();
#endif
	if (!strncmp(buf, "hoston", strlen("hoston"))) {
		event.dev_type = TCA_ID_FALL_EVENT;
		event.irq_type = TCA_IRQ_HPD_IN;
		event.mode_type = TCPC_USB31_CONNECTED;

		usb_err("hoston\n");
	} else if (!strncmp(buf, "hostoff", strlen("hostoff"))) {
		event.dev_type = TCA_ID_RISE_EVENT;
		event.irq_type = TCA_IRQ_HPD_OUT;
		event.mode_type = TCPC_NC;

		usb_err("hostoff\n");
	} else if (!strncmp(buf, "deviceon", strlen("deviceon"))) {
		event.dev_type = TCA_CHARGER_CONNECT_EVENT;
		event.irq_type = TCA_IRQ_HPD_IN;
		event.mode_type = TCPC_USB31_CONNECTED;

		usb_err("deviceon\n");
	} else if (!strncmp(buf, "deviceoff", strlen("deviceoff"))) {
		event.dev_type = TCA_CHARGER_DISCONNECT_EVENT;
		event.irq_type = TCA_IRQ_HPD_OUT;
		event.mode_type = TCPC_NC;

		usb_err("deviceoff\n");
	} else {
		lunch = 0;
	}

	if (lunch) {
#ifdef CONFIG_TCPC_CLASS
		pd_dpm_handle_combphy_event(event);
#endif
		return size;
	}
#endif

	if (!strncmp(buf, "hifiusbon", strlen("hifiusbon"))) {
		hisi_usb_otg_event(START_HIFI_USB);
		usb_err("hifiusbon\n");
	} else if (!strncmp(buf, "hifiusboff", strlen("hifiusboff"))) {
		hisi_usb_otg_event(STOP_HIFI_USB);
		usb_err("hifiusboff\n");
	} else {
		usb_err("input state is illegal!\n");
	}

	return size;
}

static ssize_t plugusb_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	if (!hisi_dwc3_dev) {
		usb_err("usb module not ready\n");
		return size;
	}

	if (hisi_dwc3_dev->support_dp != 0) {
		return  plugusb_dp_store(dev, attr, buf, size);
	}

	if (!strncmp(buf, "hoston", strlen("hoston"))) {
		hisi_usb_otg_event(ID_FALL_EVENT);
		usb_err("hoston\n");
	} else if (!strncmp(buf, "hostoff", strlen("hostoff"))) {
		hisi_usb_otg_event(ID_RISE_EVENT);
		usb_err("hostoff\n");
	} else if (!strncmp(buf, "deviceon", strlen("deviceon"))) {
		hisi_usb_otg_event(CHARGER_CONNECT_EVENT);
		usb_err("deviceon\n");
	} else if (!strncmp(buf, "deviceoff", strlen("deviceoff"))) {
		hisi_usb_otg_event(CHARGER_DISCONNECT_EVENT);
		usb_err("deviceoff\n");
	} else if (!strncmp(buf, "hifiusbon", strlen("hifiusbon"))) {
		hisi_usb_otg_event(START_HIFI_USB);
		usb_err("hifiusbon\n");
	} else if (!strncmp(buf, "hifiusboff", strlen("hifiusboff"))) {
		hisi_usb_otg_event(STOP_HIFI_USB);
		usb_err("hifiusboff\n");
	} else
		usb_err("input state is illegal!\n");

	return size;
}
#else
/*lint -save -e715 */
static ssize_t plugusb_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	usb_err("ERROR:[USB.plugusb] not for user verison\n");

	return size;
}
/*lint -restore */
#endif
/*lint -save -e750 */
DEVICE_ATTR(plugusb, (S_IRUGO | S_IWUSR), plugusb_show, plugusb_store);
/*lint -restore */


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
	unsigned int i = 0;
	enum hisi_charger_type ret = CHARGER_TYPE_NONE;

	for (i = 0; i < sizeof(charger_type_array) / sizeof(charger_type_array[0]); i++) {/*lint !e574*/
		if (!strncmp(buf, charger_type_array[i], size - 1)) {
			ret = (enum hisi_charger_type)i;
			break;
		}
	}

	return ret;
}

ssize_t hiusb_do_charger_show(void *dev_data, char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;
	enum hisi_charger_type charger_type;

	if (!hisi_dwc) {
		usb_err("platform_get_drvdata return null\n");
		return scnprintf(buf, size, "platform_get_drvdata return null\n");
	}

	mutex_lock(&hisi_dwc->lock);
	charger_type = hisi_dwc->charger_type;
	mutex_unlock(&hisi_dwc->lock);

	return scnprintf(buf, size, "[(%d):Charger type = %s]\n"
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
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;
	enum hisi_charger_type charger_type = get_charger_type_from_str(buf, size);

	if (!hisi_dwc) {
		usb_err("platform_get_drvdata return null\n");
		return size;
	}

	mutex_lock(&hisi_dwc->lock);
	hisi_dwc->charger_type = charger_type;
	notify_charger_type(hisi_dwc);
	mutex_unlock(&hisi_dwc->lock);

	return size;
}

void hiusb_get_eyepattern_param(void *dev_data, unsigned int *eye_diagram_param,
		unsigned int *eye_diagram_host_param)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	*eye_diagram_param = hisi_dwc->eye_diagram_param;
	*eye_diagram_host_param = hisi_dwc->eye_diagram_host_param;
}

void hiusb_set_eyepattern_param(void *dev_data, unsigned int eye_diagram_param)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	hisi_dwc->eye_diagram_param = eye_diagram_param;
	hisi_dwc->eye_diagram_host_param = eye_diagram_param;
}

static ssize_t fakecharger_show(void *dev_data, char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	usb_err("+\n");
	if (!hisi_dwc) {
		usb_err("platform_get_drvdata return null\n");
		return scnprintf(buf, size, "platform_get_drvdata return null\n");
	}

	return scnprintf(buf, size, "[fake charger type: %s]\n",
					 hisi_dwc->fake_charger_type == CHARGER_TYPE_NONE ? "not fake" :
					 charger_type_array[hisi_dwc->fake_charger_type]);
}

static ssize_t fakecharger_store(void *dev_data, const char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;
	enum hisi_charger_type charger_type = get_charger_type_from_str(buf, size);

	usb_err("+\n");
	if (!hisi_dwc) {
		usb_err("platform_get_drvdata return null\n");
		return size;
	}

	mutex_lock(&hisi_dwc->lock);
	hisi_dwc->fake_charger_type = charger_type;
	mutex_unlock(&hisi_dwc->lock);

	return size;
}

static ssize_t hifi_ip_first_show(void *dev_data, char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	if (!hisi_dwc)
		return scnprintf(buf, size, "unknown\n");
	return scnprintf(buf, size, "%d\n", hisi_dwc->hifi_ip_first);
}

static ssize_t hifi_ip_first_store(void *dev_data, const char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	if (!hisi_dwc) {
		pr_err("%s: hisi_dwc NULL\n", __func__);
		return size;
	}

	mutex_lock(&hisi_dwc->lock);
	if (buf[0] == '1')
		hisi_dwc->hifi_ip_first = 1;
	else if (buf[0] == '0')
		hisi_dwc->hifi_ip_first = 0;
	mutex_unlock(&hisi_dwc->lock);

	return size;
}

static ssize_t hifi_usb_perf_show(void *dev_data, char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	if (!hisi_dwc)
		return scnprintf(buf, size, "unknown\n");

	return scnprintf(buf, size, "connect_time: %dms\n",
		jiffies_to_msecs(hisi_dwc->hifi_usb_setconfig_time_stamp
			- hisi_dwc->start_host_time_stamp));
}

static ssize_t hifi_usb_perf_store(void *dev_data, const char *buf, size_t size)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	if (!hisi_dwc) {
		pr_err("%s: hisi_dwc NULL\n", __func__);
		return size;
	}
	return size;
}

int hiusb_do_eventmask_show(void *dev_data)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	return hisi_dwc->eventmask;
}

void hiusb_do_eventmask_store(void *dev_data, int eventmask)
{
	struct hisi_dwc3_device *hisi_dwc = (struct hisi_dwc3_device *)dev_data;

	hisi_dwc->eventmask = eventmask;
}

static struct device_attribute *hisi_dwc3_attributes[] = {
	&dev_attr_plugusb,
	NULL
};

static int create_attr_file(struct device *dev)
{
	struct device_attribute **attrs = hisi_dwc3_attributes;
	struct device_attribute *attr;
	struct class *hisi_usb_class;
	struct device *hisi_usb_dev;
	int i;
	int ret = 0;

	usb_dbg("+\n");
	for (i = 0; attrs[i] != NULL; i++) {
		attr = attrs[i];
		ret = device_create_file(dev, attr);
		if (ret) {
			dev_err(dev, "create attr file error!\n");
			goto err;
		}
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
	hiusb_debug_quick_register( "fakecharger",
			platform_get_drvdata(to_platform_device(dev)),
			(hiusb_debug_show_ops)fakecharger_show,
			(hiusb_debug_store_ops)fakecharger_store);

	hiusb_debug_quick_register( "cptest",
			platform_get_drvdata(to_platform_device(dev)),
			(hiusb_debug_show_ops)hisi_dwc3_cptest_show,
			(hiusb_debug_store_ops)hisi_dwc3_cptest_store);

	hiusb_debug_quick_register( "hifi_ip_first",
			platform_get_drvdata(to_platform_device(dev)),
			(hiusb_debug_show_ops)hifi_ip_first_show,
			(hiusb_debug_store_ops)hifi_ip_first_store);

	hiusb_debug_quick_register( "perf",
			platform_get_drvdata(to_platform_device(dev)),
			(hiusb_debug_show_ops)hifi_usb_perf_show,
			(hiusb_debug_store_ops)hifi_usb_perf_store);

	hiusb_debug_quick_register( "linkstate",
			platform_get_drvdata(to_platform_device(dev)),
			(hiusb_debug_show_ops)hisi_dwc3_linkstate_show,
			(hiusb_debug_store_ops)hisi_dwc3_linkstate_store);

	hiusb_debug_quick_register( "noctest",
			platform_get_drvdata(to_platform_device(dev)),
			(hiusb_debug_show_ops)hisi_dwc3_noc_show,
			(hiusb_debug_store_ops)hisi_dwc3_noc_store);

	hiusb_debug_quick_register( "lbintpll-clk",
			platform_get_drvdata(to_platform_device(dev)),
			(hiusb_debug_show_ops)hisi_dwc3_lbintpll_show,
			(hiusb_debug_store_ops)hisi_dwc3_lbintpll_store);

	hiusb_debug_quick_register( "vboost",
			platform_get_drvdata(to_platform_device(dev)),
			(hiusb_debug_show_ops)hisi_dwc3_vboost_show,
			(hiusb_debug_store_ops)hisi_dwc3_vboost_store);

	hiusb_debug_init(platform_get_drvdata(to_platform_device(dev)));
#endif
#endif

	usb_dbg("-\n");
	return 0;

err:
	for (i-- ; i >= 0; i--) {
		attr = attrs[i];
		device_remove_file(dev, attr);
	}

	return ret;
}

static void remove_attr_file(struct device *dev)
{
	struct device_attribute **attrs = hisi_dwc3_attributes;
	struct device_attribute *attr;

	while ((attr = *attrs++)) {
		device_remove_file(dev, attr);
	}

#ifdef CONFIG_USB_F_BALONG_ACM
#ifdef CONFIG_HISI_DEBUG_FS
	hiusb_debugfs_destory(platform_get_drvdata(to_platform_device(dev)));
#endif /* config hisi debugfs */
#endif
}
#else
static inline int create_attr_file(struct device *dev)
{
	return 0;
}
static inline void remove_attr_file(struct device *dev) {}
#endif

int hisi_charger_type_notifier_register(struct notifier_block *nb)
{
	if (!hisi_dwc3_dev) {
		pr_err("hisi dwc3 device not ready!\n");
		return -EBUSY;
	}
	if (!nb)
		return -EINVAL;
	return blocking_notifier_chain_register(&hisi_dwc3_dev->charger_type_notifier, nb);
}
EXPORT_SYMBOL_GPL(hisi_charger_type_notifier_register);

int hisi_charger_type_notifier_unregister(struct notifier_block *nb)
{
	if (!hisi_dwc3_dev) {
		pr_err("hisi dwc3 device not ready!\n");
		return -EBUSY;
	}
	if (!nb)
		return -EINVAL;
	return blocking_notifier_chain_unregister(&hisi_dwc3_dev->charger_type_notifier, nb);
}
EXPORT_SYMBOL_GPL(hisi_charger_type_notifier_unregister);


const char *charger_type_string(enum hisi_charger_type type)
{
	if (type < CHARGER_TYPE_SDP || type >= CHARGER_TYPE_ILLEGAL)
		return "illegal charger";

	return charger_type_array[type];
}

static const char *event_type_string(enum otg_dev_event_type event)
{
	static const char *const hisi_usb_event_strings[] = {
		[CHARGER_CONNECT_EVENT]		= "CHARGER_CONNECT",
		[CHARGER_DISCONNECT_EVENT]	= "CHARGER_DISCONNECT",
		[ID_FALL_EVENT]			= "OTG_CONNECT",
		[ID_RISE_EVENT]			= "OTG_DISCONNECT",
		[START_HIFI_USB]		= "START_HIFI_USB",
		[START_HIFI_USB_RESET_VBUS]	= "START_HIFI_USB_RESET_VBUS",
		[STOP_HIFI_USB]			= "STOP_HIFI_USB",
		[STOP_HIFI_USB_RESET_VBUS]	= "STOP_HIFI_USB_RESET_VBUS",
		[START_AP_USE_HIFIUSB]		= "START_AP_USE_HIFIUSB",
		[STOP_AP_USE_HIFIUSB]		= "STOP_AP_USE_HIFIUSB",
		[HIFI_USB_HIBERNATE]		= "HIFI_USB_HIBERNATE",
		[HIFI_USB_WAKEUP]		= "HIFI_USB_WAKEUP",
		[DISABLE_USB3_PORT]		= "DISABLE_USB3_PORT",
		[NONE_EVENT]			= "NONE",
	};

	if (event < CHARGER_CONNECT_EVENT || event > NONE_EVENT)
		return "illegal event";

	return hisi_usb_event_strings[event];
}

int hisi_dwc3_is_es(void)
{
	struct hisi_dwc3_device *hisi_dwc3 = hisi_dwc3_dev;

	if (!hisi_dwc3) {
		usb_err("need probe usb driver!\n");
		return 0;
	}

	return hisi_dwc3->es_firmware;
}

enum hisi_charger_type hisi_get_charger_type(void)
{
	if (!hisi_dwc3_dev) {
		pr_err("[%s]hisi_dwc3 not yet probed!\n", __func__);
		return CHARGER_TYPE_NONE;
	}

	pr_info("[%s]type: %s\n", __func__, charger_type_string(hisi_dwc3_dev->charger_type));
	return hisi_dwc3_dev->charger_type;
}
EXPORT_SYMBOL_GPL(hisi_get_charger_type);

void notify_charger_type(struct hisi_dwc3_device *hisi_dwc3)
{
	usb_dbg("+\n");
	blocking_notifier_call_chain(&hisi_dwc3->charger_type_notifier,
				hisi_dwc3->charger_type, hisi_dwc3);

	if (hisi_dwc3->charger_type == CHARGER_TYPE_DCP)
		if (hisi_dwc3->phy_ops->set_hi_impedance)
			hisi_dwc3->phy_ops->set_hi_impedance(hisi_dwc3);
	usb_dbg("-\n");
}

static void set_vbus_power(struct hisi_dwc3_device *hisi_dwc3, unsigned int is_on)
{
	enum hisi_charger_type new;

	if (0 == is_on)
		new = CHARGER_TYPE_NONE;
	else
		new = PLEASE_PROVIDE_POWER;

	if (hisi_dwc3->charger_type != new) {
		usb_dbg("set port power %d\n", is_on);
		hisi_dwc3->charger_type = new;
		notify_charger_type(hisi_dwc3);
	}

	if (hisi_dwc3->fpga_otg_drv_vbus_gpio > 0) {
		gpio_direction_output(hisi_dwc3->fpga_otg_drv_vbus_gpio, !!is_on);
		usb_dbg("turn %s drvvbus for fpga\n", is_on ? "on" : "off");
	}
}

/*lint -save -e454 -e455 -e456 */
static void hisi_dwc3_wake_lock(struct hisi_dwc3_device *hisi_dwc3)
{
	if (!wake_lock_active(&hisi_dwc3->wake_lock)) {
		usb_dbg("usb otg wake lock\n");
		wake_lock(&hisi_dwc3->wake_lock);
	}
}

static void hisi_dwc3_wake_unlock(struct hisi_dwc3_device *hisi_dwc3)
{
	if (wake_lock_active(&hisi_dwc3->wake_lock)) {
		usb_dbg("usb otg wake unlock\n");
		wake_unlock(&hisi_dwc3->wake_lock);
	}
}
/*lint -restore */

static inline bool enumerate_allowed(struct hisi_dwc3_device *hisi_dwc)
{
	if (hisi_dwc->bc_again_delay_time == BC_AGAIN_DELAY_TIME_1)
		return false;

	/* do not start peripheral if real charger connected */
	return ((hisi_dwc->charger_type == CHARGER_TYPE_SDP)
			|| (hisi_dwc->charger_type == CHARGER_TYPE_CDP)
			|| (hisi_dwc->charger_type == CHARGER_TYPE_UNKNOWN));
}

static inline bool sleep_allowed(struct hisi_dwc3_device *hisi_dwc)
{
	return ((hisi_dwc->charger_type == CHARGER_TYPE_DCP)
			|| (hisi_dwc->charger_type == CHARGER_TYPE_UNKNOWN));
}

static inline bool bc_again_allowed(struct hisi_dwc3_device *hisi_dwc)
{
	if (hisi_dwc->bc_unknown_again_flag)
		return ((hisi_dwc->charger_type == CHARGER_TYPE_SDP)
				|| (hisi_dwc->charger_type == CHARGER_TYPE_UNKNOWN)
				|| (hisi_dwc->charger_type == CHARGER_TYPE_CDP));
	else
		return ((hisi_dwc->charger_type == CHARGER_TYPE_SDP)
				|| (hisi_dwc->charger_type == CHARGER_TYPE_CDP));
}

/*
 * create event queue
 * event_queue: event queue handle
 * count: set the queue max node
 */
int event_queue_creat(struct hiusb_event_queue *event_queue, unsigned int count)
{
	if (!event_queue) {
		usb_err("[event_queue_creat]bad argument (0x%pK)\n", event_queue);
		return -EINVAL;
	}

	count = (count >= MAX_EVENT_COUNT ? MAX_EVENT_COUNT : count);
	event_queue->max_event = count;
	event_queue->num_event = (count >= EVENT_QUEUE_UNIT ? EVENT_QUEUE_UNIT : count);

	event_queue->event = kzalloc(event_queue->num_event * sizeof(enum otg_dev_event_type), GFP_KERNEL);
	if (!event_queue->event) {
		usb_err("[event_queue_creat]:Can't alloc space:%d!\n", event_queue->num_event);
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
		usb_err("event queue full!\n");
		return -ENOSPC;
	}

	if (event_queue->overlay) {
		if (event_queue->overlay_index == event_queue->enpos) {
			event_queue->enpos = ((event_queue->enpos + 1) % event_queue->num_event);
		}

		if (event_queue_isempty(event_queue)) {
			usb_err("overlay and queue isempty? just enqueue!\n");
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

static inline int dwc3_event_work_is_sync(struct hisi_dwc3_device *hisi_dwc)
{
	return hisi_dwc->is_hanle_event_sync;
}

static int start_device(struct hisi_dwc3_device *hisi_dwc)
{
	int ret;

	hisi_dwc->host_flag = 0;

	/* due to detect charger type, must resume hisi_dwc */
	ret = hisi_dwc3_phy_init(hisi_dwc);
	if (ret) {
		usb_err("hisi_dwc3_phy_init failed (ret %d)\n", ret);
		return ret;
	}

	/*if the platform support,it need check voltage*/
	if (hisi_dwc->phy_ops->check_voltage)
		hisi_dwc->phy_ops->check_voltage(hisi_dwc);

	/* detect charger type */
	hisi_dwc->charger_type = detect_charger_type(hisi_dwc);
	notify_charger_type(hisi_dwc);

	/* In some cases, DCP is detected as SDP wrongly. To avoid this,
	 * start bc_again delay work to detect charger type once more.
	 * If later the enum process is executed, then it's a real SDP, so
	 * the work will be canceled.
	 */
	if (bc_again_allowed(hisi_dwc))
		schedule_bc_again(hisi_dwc);

	/* do not start peripheral if real charger connected */
	if (enumerate_allowed(hisi_dwc)) {
		if (hisi_dwc->fpga_usb_mode_gpio > 0) {
			gpio_direction_output((unsigned)hisi_dwc->fpga_usb_mode_gpio, 0);
			usb_dbg("switch to device mode\n");
		}

		/* start peripheral */
		ret = dwc3_otg_work(dwc_otg_handler,
			    DWC3_OTG_EVT_VBUS_SET);
		if (ret) {
			usb_err("start peripheral error\n");
			return ret;
		}
	} else {
		usb_dbg("it need notify USB_CONNECT_DCP while a real charger connected\n");
		hisi_dwc->speed = USB_CONNECT_DCP;
		if (!queue_work(system_power_efficient_wq,
						&hisi_dwc->speed_change_work)) {
			usb_err("schedule speed_change_work wait:%d\n", hisi_dwc->speed);
		}
	}

	hisi_dwc->state = USB_STATE_DEVICE;

	if (sleep_allowed(hisi_dwc))
		hisi_dwc3_wake_unlock(hisi_dwc);
	else
		hisi_dwc3_wake_lock(hisi_dwc);

	usb_dbg("hisi usb status: OFF -> DEVICE\n");

	return 0;
}

static void stop_device(struct hisi_dwc3_device *hisi_dwc)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
	hisi_bc_disable_vdp_src(hisi_dwc);
	spin_unlock_irqrestore(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/

	/* peripheral not started, if real charger connected */
	if (enumerate_allowed(hisi_dwc)) {
		/* stop peripheral */
		ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_VBUS_CLEAR);
		if (ret) {
			usb_err("stop peripheral error\n");
			return;
		}
	}

	cancel_bc_again(hisi_dwc, 0);

	/* usb cable disconnect, notify no charger */
	hisi_dwc->charger_type = CHARGER_TYPE_NONE;
	notify_charger_type(hisi_dwc);

	hisi_dwc->state = USB_STATE_OFF;
	hisi_dwc3_wake_unlock(hisi_dwc);

	ret = hisi_dwc3_phy_shutdown(hisi_dwc);
	if (ret)
		usb_err("hisi_dwc3_phy_shutdown failed (ret %d)\n", ret);

	usb_dbg("hisi usb status: DEVICE -> OFF\n");
}

static int start_host(struct hisi_dwc3_device *hisi_dwc)
{
	int ret;

	/* Set host_flag for eye_diagram_param set in hisi_dwc3_phy_init */
	hisi_dwc->host_flag = 1;

	ret = hisi_dwc3_phy_init(hisi_dwc);
	if (ret) {
		usb_err("hisi_dwc3_phy_init failed (ret %d)\n", ret);
		return ret;
	}

	if (hisi_dwc->fpga_usb_mode_gpio > 0) {
		gpio_direction_output((unsigned)hisi_dwc->fpga_usb_mode_gpio, 1);
		usb_dbg("switch to host mode\n");
	}

	/* start host */
	ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_ID_CLEAR);
	if (ret) {
		usb_err("start host error\n");
		return ret;
	}

	return 0;
}

static int stop_host(struct hisi_dwc3_device *hisi_dwc)
{
	int ret;

	/* stop host */
	ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_ID_SET);
	if (ret) {
		usb_err("stop host error\n");
		return ret;
	}

	ret = hisi_dwc3_phy_shutdown(hisi_dwc);
	if (ret)
		usb_err("hisi_dwc3_phy_shutdown failed (ret %d)\n", ret);

	return ret;
}

static int start_audio_usb(struct hisi_dwc3_device *hisi_dwc, unsigned int combophy_flag)
{
	if (hisi_dwc3_usb20_phy_init(hisi_dwc, combophy_flag)) {
		usb_err("audio usb phy init failed\n");
		return -EBUSY;
	}

	if (start_hifi_usb()) {
		if (hisi_dwc3_usb20_phy_shutdown(hisi_dwc, combophy_flag, 0))
			WARN_ON(1);
		return -EBUSY;
	}

	return 0;
}

static void stop_audio_usb(struct hisi_dwc3_device *hisi_dwc, unsigned int combophy_flag)
{
	stop_hifi_usb();
	if (hisi_dwc3_usb20_phy_shutdown(hisi_dwc, combophy_flag, 0))
		WARN_ON(1);
}

static void handle_start_hifi_usb_event(struct hisi_dwc3_device *hisi_dwc,
					int reset_vbus)
{
	switch (hisi_dwc->state) {
	case USB_STATE_OFF:
		set_vbus_power(hisi_dwc, 1);
		if (start_audio_usb(hisi_dwc, 1)) {
			usb_err("start_audio_usb failed\n");
			if (start_host(hisi_dwc)) {
				usb_err("start_host failed\n");
			} else {
				hisi_dwc->state = USB_STATE_HOST;
				hisi_dwc3_wake_lock(hisi_dwc);
				usb_dbg("hisi usb_status: OFF -> HOST\n");
			}
			return;
		}

		hisi_dwc->state = USB_STATE_HIFI_USB;
		hisi_dwc3_wake_unlock(hisi_dwc);
		pd_dpm_wakelock_ctrl(PD_WAKE_UNLOCK);
		usb_dbg("hisi usb state: OFF -> HIFI_USB\n");
		usb_dbg("hisi usb start hifi usb time: %d ms\n",
				jiffies_to_msecs(jiffies
				- hisi_dwc->start_host_time_stamp));
		break;

	case USB_STATE_HOST:
		if (reset_vbus) {
			set_vbus_power(hisi_dwc, 0);
			pd_dpm_vbus_ctrl(CHARGER_TYPE_NONE);
		}

		if (stop_host(hisi_dwc)) {
			usb_err("stop_host failed\n");
			return;
		}

		if (reset_vbus) {
			set_vbus_power(hisi_dwc, 1);
			pd_dpm_vbus_ctrl(PLEASE_PROVIDE_POWER);
		}

		if (start_audio_usb(hisi_dwc, 1)) {
			usb_err("start_audio_usb failed\n");
			if (start_host(hisi_dwc))
				usb_err("start_host failed\n");
			return;
		}

		hisi_dwc->state = USB_STATE_HIFI_USB;
		hisi_dwc3_wake_unlock(hisi_dwc);
		pd_dpm_wakelock_ctrl(PD_WAKE_UNLOCK);
		usb_dbg("hisi usb state: HOST -> HIFI_USB\n");

		usb_dbg("hisi usb start hifi usb time: %d ms\n",
				jiffies_to_msecs(jiffies
				- hisi_dwc->start_hifi_usb_time_stamp));
		break;
	default:
		usb_dbg("event %d in state %d\n", START_HIFI_USB, hisi_dwc->state);
		break;
	}
}

static void handle_stop_hifi_usb_event(struct hisi_dwc3_device *hisi_dwc,
					int reset_vbus)
{
	int ret;

	switch (hisi_dwc->state) {
	case USB_STATE_HIFI_USB:
		stop_audio_usb(hisi_dwc, 1);

		if (reset_vbus) {
			set_vbus_power(hisi_dwc, 0);
			pd_dpm_vbus_ctrl(CHARGER_TYPE_NONE);
		}

		if (!hisi_dwc->hifi_ip_first)
			msleep(1500);
		else if (!reset_vbus)
			msleep(300);

		if (start_host(hisi_dwc)) {
			usb_err("start_host failed\n");
			return;
		}

		if (reset_vbus) {
			set_vbus_power(hisi_dwc, 1);
			pd_dpm_vbus_ctrl(PLEASE_PROVIDE_POWER);
		}

		hisi_dwc->state = USB_STATE_HOST;
		hisi_dwc3_wake_lock(hisi_dwc);
		pd_dpm_wakelock_ctrl(PD_WAKE_LOCK);
		usb_dbg("hisi usb state: HIFI_USB -> HOST\n");

		break;
	case USB_STATE_HIFI_USB_HIBERNATE:
		/* phy was closed in this state */
		stop_hifi_usb();

		msleep(1500);

		ret = hisi_dwc3_usb20_phy_init(hisi_dwc, 0);
		if (ret)
			usb_err("shared_phy_init error ret %d\n", ret);
		ret = hisi_dwc3_usb20_phy_shutdown(hisi_dwc, 1, 0);
		if (ret)
			usb_err("shared_phy_shutdown error ret %d\n", ret);

		if (start_host(hisi_dwc)) {
			usb_err("start_host failed\n");
			return;
		}

		hisi_dwc->state = USB_STATE_HOST;
		hisi_dwc3_wake_lock(hisi_dwc);
		pd_dpm_wakelock_ctrl(PD_WAKE_LOCK);
		usb_dbg("hisi usb state: HIFI_USB_HIBERNATE -> HOST\n");

		break;
	default:
		usb_dbg("event %d in state %d\n", STOP_HIFI_USB, hisi_dwc->state);
		break;
	}
}

static void handle_start_hifi_usb_hibernate(struct hisi_dwc3_device *hisi_dwc)
{
	switch (hisi_dwc->state) {
	case USB_STATE_HIFI_USB:
		if (hifi_usb_hibernate()) {
			WARN_ON(1);
			return;
		}

		if (hisi_dwc3_usb20_phy_shutdown(hisi_dwc, 0, 1)) {
			WARN_ON(1);
			return;
		}

		hisi_dwc->state = USB_STATE_HIFI_USB_HIBERNATE;
		usb_dbg("hisi usb state: HIFI_USB -> HIFI_USB_HIBERNATE\n");
		msleep(50); /* debounce of suspend state */
		break;
	default:
		usb_dbg("event %d in state %d\n", HIFI_USB_HIBERNATE, hisi_dwc->state);
		break;
	}
}

static void handle_start_hifi_usb_wakeup(struct hisi_dwc3_device *hisi_dwc)
{
	switch (hisi_dwc->state) {
	case USB_STATE_HIFI_USB_HIBERNATE:
		if (hisi_dwc3_usb20_phy_init(hisi_dwc, 0)) {
			WARN_ON(1);
			return;
		}

		if (hifi_usb_revive()) {
			WARN_ON(1);
			return;
		}

		hisi_dwc->state = USB_STATE_HIFI_USB;
		usb_dbg("hisi usb state: HIFI_USB_HIBERNATE -> HIFI_USB\n");
		break;
	default:
		usb_dbg("event %d in state %d\n", HIFI_USB_WAKEUP, hisi_dwc->state);
		break;
	}
}

/* Caution: this function must be called in "hisi_dwc3->lock"
 * Currently, this function called only by hisi_usb_resume */
int hisi_usb_wakeup_hifi_usb(void)
{
	struct hisi_dwc3_device *dev = hisi_dwc3_dev;
	if (!dev)
		return -ENOENT;

	handle_start_hifi_usb_wakeup(dev);
	return 0;
}

static void handle_charger_connect_event(struct hisi_dwc3_device *hisi_dwc)
{
	if (USB_STATE_DEVICE == hisi_dwc->state) {
		usb_dbg("Already in device mode, do nothing\n");
	} else if (USB_STATE_OFF == hisi_dwc->state) {
		if (start_device(hisi_dwc))
			usb_err("start_device error\n");
	} else if (USB_STATE_HOST == hisi_dwc->state) {
		usb_dbg("Charger connect intrrupt in HOST mode\n");
	} else if (USB_STATE_HIFI_USB == hisi_dwc->state) {
		usb_dbg("vbus power in hifi usb state\n");
	} else {
		usb_dbg("can not handle_charger_connect_event in mode %s\n",
				hisi_usb_state_string(hisi_dwc->state));
	}
}

static void handle_charger_disconnect_event(struct hisi_dwc3_device *hisi_dwc)
{
	if (USB_STATE_OFF == hisi_dwc->state) {
		usb_dbg("Already in off mode, do nothing\n");
	} else if (USB_STATE_DEVICE == hisi_dwc->state) {
		stop_device(hisi_dwc);
	} else if (USB_STATE_HOST == hisi_dwc->state) {
		usb_dbg("Charger disconnect intrrupt in HOST mode\n");
	} else if (USB_STATE_HIFI_USB == hisi_dwc->state) {
		/* lose power ??? */
		usb_dbg("vbus disconnect event in hifi usb state\n");
	} else {
		usb_dbg("can not handle_charger_disconnect_event in mode %s\n",
				hisi_usb_state_string(hisi_dwc->state));
	}
}

static void handle_start_arm_usb_event(struct hisi_dwc3_device *hisi_dwc)
{
	switch (hisi_dwc->state) {
	case USB_STATE_OFF:
		set_vbus_power(hisi_dwc, 1);
		if (start_host(hisi_dwc))
			set_vbus_power(hisi_dwc, 0);

		hisi_dwc->state = USB_STATE_HOST;
		hisi_dwc3_wake_lock(hisi_dwc);
		usb_dbg("hisi usb_status: OFF -> HOST\n");
		break;
	default:
		usb_dbg("event %d in state %d\n", ID_FALL_EVENT, hisi_dwc->state);
		break;
	}
}


static void handle_id_fall_event(struct hisi_dwc3_device *hisi_dwc)
{
	/*
	 * 1. hifi ip first feature controlled by device tree.
	 * 2. hifi usb need to wait for usbauddio nv.
	 */
	if (hisi_usb_otg_use_hifi_ip_first()
		&& (TCPC_USB31_CONNECTED == hisi_dwc->mode_type)) {
		usb_dbg("use hifi ip first\n");
		handle_start_hifi_usb_event(hisi_dwc, 0);

	} else {
		usb_dbg("use arm ip first\n");
		handle_start_arm_usb_event(hisi_dwc);
	}

}

static void handle_id_rise_event(struct hisi_dwc3_device *hisi_dwc)
{
	switch (hisi_dwc->state) {
	case USB_STATE_HOST:
		set_vbus_power(hisi_dwc, 0);
		if (stop_host(hisi_dwc))
			usb_err("stop_host failed\n");

		hisi_dwc->state = USB_STATE_OFF;
		hisi_dwc3_wake_unlock(hisi_dwc);
		usb_dbg("hiusb_status: HOST -> OFF\n");

		reset_hifi_usb();
		break;
	case USB_STATE_HIFI_USB:
		set_vbus_power(hisi_dwc, 0);
		stop_audio_usb(hisi_dwc, 0);

		hisi_dwc->state = USB_STATE_OFF;
		hisi_dwc3_wake_unlock(hisi_dwc);
		usb_dbg("hisi usb state: HIFI_USB -> OFF\n");

		reset_hifi_usb();

		usb_dbg("hisi usb stop hifi usb time: %d ms\n",
				jiffies_to_msecs(jiffies
					- hisi_dwc->stop_host_time_stamp));
		break;

	case USB_STATE_HIFI_USB_HIBERNATE:
		set_vbus_power(hisi_dwc, 0);
		/* phy was closed in this state */
		stop_hifi_usb();

		hisi_dwc->state = USB_STATE_OFF;
		hisi_dwc3_wake_unlock(hisi_dwc);
		usb_dbg("hisi usb state: HIFI_USB_HIBERNATE -> OFF\n");

		reset_hifi_usb();

		usb_dbg("hisi usb stop hifi usb time: %d ms\n",
				jiffies_to_msecs(jiffies
				- hisi_dwc->stop_host_time_stamp));
		break;

	case USB_STATE_AP_USE_HIFIUSB:
		set_vbus_power(hisi_dwc, 0);
		ap_stop_use_hifiusb();

		if (hisi_dwc3_usb20_phy_shutdown(hisi_dwc, 0, 0))
			WARN_ON(1);

		hisi_dwc->state = USB_STATE_OFF;
		hisi_dwc3_wake_unlock(hisi_dwc);
		usb_dbg("hisi usb state: AP_USE_HIFI_USB -> OFF\n");

		break;
	default:
		usb_dbg("event %d in state %d\n", ID_RISE_EVENT, hisi_dwc->state);
		break;
	}
}

static int dwc3_tcpc_is_usb_only(struct hisi_dwc3_device *hisi_dwc)
{
	if (hisi_dwc->phy_ops && hisi_dwc->phy_ops->tcpc_is_usb_only)
		return hisi_dwc->phy_ops->tcpc_is_usb_only();

	return 0;
}

static void handle_start_ap_use_hifiusb(struct hisi_dwc3_device *hisi_dwc)
{
	int is_usb_only;

	switch (hisi_dwc->state) {
	case USB_STATE_HOST:
		if (stop_host(hisi_dwc)) {
			usb_err("stop_host failed\n");
			return;
		}

		is_usb_only = dwc3_tcpc_is_usb_only(hisi_dwc);
		if (is_usb_only) {
			usb_dbg("combophy is in usb only mode, do vbus reset\n");
			set_vbus_power(hisi_dwc, 0);
			pd_dpm_vbus_ctrl(CHARGER_TYPE_NONE);
			msleep(30);
		}

		if (hisi_dwc3_usb20_phy_init(hisi_dwc, 0)) {
			usb_err("audio usb phy init failed\n");
		} else if (ap_start_use_hifiusb()) {
			usb_err("start ap use hifiusb failed");
			if (!hisi_dwc3_usb20_phy_shutdown(hisi_dwc, 0, 0)) {
				if (start_host(hisi_dwc))
					usb_err("start_host failed\n");
			} else
				usb_err("shard phy shutdown failed\n");
		} else {
			hisi_dwc->state = USB_STATE_AP_USE_HIFIUSB;
			usb_dbg("hisi usb state: HOST -> AP_USE_HIFI_USB\n");
		}

		if (is_usb_only) {
			msleep(30);
			set_vbus_power(hisi_dwc, 1);
			pd_dpm_vbus_ctrl(PLEASE_PROVIDE_POWER);
		}

		break;
	default:
		usb_dbg("event %d in state %d\n", START_AP_USE_HIFIUSB,
				hisi_dwc->state);
		break;
	}
}

static void handle_stop_ap_use_hifiusb(struct hisi_dwc3_device *hisi_dwc)
{
	switch (hisi_dwc->state) {
	case USB_STATE_AP_USE_HIFIUSB:
		ap_stop_use_hifiusb();

		if (hisi_dwc3_usb20_phy_shutdown(hisi_dwc, 0, 0))
			WARN_ON(1);

		if (start_host(hisi_dwc)) {
			usb_err("start_host failed\n");
			return;
		}

		hisi_dwc->state = USB_STATE_HOST;
		usb_dbg("hisi usb state: AP_USE_HIFI_USB -> HOST\n");

		break;
	default:
		usb_dbg("event %d in state %d\n", STOP_AP_USE_HIFIUSB,
				hisi_dwc->state);
		break;
	}
}

static void hisi_usb_disable_usb3(struct hisi_dwc3_device *hisi_dwc)
{
	if (hisi_dwc->phy_ops->disable_usb3)
		hisi_dwc->phy_ops->disable_usb3();
}

static void handle_disable_usb3(struct hisi_dwc3_device *hisi_dwc)
{
	int ret;

	if (hisi_dwc->state != USB_STATE_HOST) {
		usb_dbg("event %d in state %d\n", DISABLE_USB3_PORT,
				hisi_dwc->state);
		return;
	}

	if (!hisi_dwc->phy_ops->disable_usb3)
		return;

	/* stop host */
	ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_ID_SET);
	if (ret) {
		usb_err("stop host error\n");
		return;
	}

	hisi_usb_disable_usb3(hisi_dwc);

	/* start host */
	ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_ID_CLEAR);
	if (ret) {
		usb_err("start host error\n");
		return;
	}
}

static void handle_event(struct hisi_dwc3_device *hisi_dwc, enum otg_dev_event_type event)
{
	int reset_vbus = 0;

	usb_err("type: %s\n", event_type_string(event));

	if (event == START_HIFI_USB_RESET_VBUS) {
		event = START_HIFI_USB;
		reset_vbus = 1;
	}

	if (event == STOP_HIFI_USB_RESET_VBUS) {
		event = STOP_HIFI_USB;
		reset_vbus = 1;
	}

	switch (event) {
	case CHARGER_CONNECT_EVENT:
		handle_charger_connect_event(hisi_dwc);
		break;

	case CHARGER_DISCONNECT_EVENT:
		handle_charger_disconnect_event(hisi_dwc);
		break;

	case ID_FALL_EVENT:
		handle_id_fall_event(hisi_dwc);
		break;

	case ID_RISE_EVENT:
		handle_id_rise_event(hisi_dwc);
		break;

	case START_HIFI_USB:
		handle_start_hifi_usb_event(hisi_dwc, reset_vbus);
		break;

	case STOP_HIFI_USB:
		handle_stop_hifi_usb_event(hisi_dwc, reset_vbus);
		break;

	case START_AP_USE_HIFIUSB:
		handle_start_ap_use_hifiusb(hisi_dwc);
		break;

	case STOP_AP_USE_HIFIUSB:
		handle_stop_ap_use_hifiusb(hisi_dwc);
		break;

	case HIFI_USB_HIBERNATE:
		handle_start_hifi_usb_hibernate(hisi_dwc);
		break;

	case HIFI_USB_WAKEUP:
		hisi_dwc3_wake_lock(hisi_dwc);
		handle_start_hifi_usb_wakeup(hisi_dwc);
		hisi_dwc3_wake_unlock(hisi_dwc);
		break;

	case DISABLE_USB3_PORT:
		handle_disable_usb3(hisi_dwc);
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

	struct hisi_dwc3_device *hisi_dwc = container_of(work,
				    struct hisi_dwc3_device, event_work);

	usb_err("+\n");
	mutex_lock(&hisi_dwc->lock);

	while (!event_queue_isempty(&hisi_dwc->event_queue)) {
		spin_lock_irqsave(&(hisi_dwc->event_lock), flags);
		event = event_dequeue(&hisi_dwc->event_queue);
		spin_unlock_irqrestore(&(hisi_dwc->event_lock), flags);

		handle_event(hisi_dwc, event);
	}

	event_queue_clear_overlay(&hisi_dwc->event_queue);

	mutex_unlock(&hisi_dwc->lock);

	if (dwc3_event_work_is_sync(hisi_dwc)) {
		usb_err("sync & complete\n");
		complete(&hisi_dwc->event_completion);
	}
	usb_err("-\n");
	return;
}

static void hisi_dwc3_speed_change_work(struct work_struct *work)
{
	struct hisi_dwc3_device *hisi_dwc = container_of(work,
				    struct hisi_dwc3_device, speed_change_work);
	usb_dbg("+\n");
	if (hisi_dwc->fpga_flag) {
		usb_dbg("- fpga platform, don't notify speed\n");
		return ;
	}

	if (hisi_dwc->phy_ops->notify_speed)
		hisi_dwc->phy_ops->notify_speed(hisi_dwc);
	usb_dbg("-\n");
}

void hisi_dwc3_cpmode_enable(void)
{
	usb_dbg("+\n");

	if (!hisi_dwc3_dev) {
		usb_err("USB drv not probe!\n");
		return ;
	}

	if (!atomic_read(&hisi_dwc3_cptest_flag)) {
		usb_dbg("not enable cpmode!\n");
		return ;
	}

	if (hisi_dwc3_dev->phy_ops->cptest_enable)
		hisi_dwc3_dev->phy_ops->cptest_enable(hisi_dwc3_dev);

	atomic_set(&hisi_dwc3_cptest_flag, 0);

	usb_dbg("-\n");
}
EXPORT_SYMBOL_GPL(hisi_dwc3_cpmode_enable);

void dwc3_lscdtimer_set(void)
{
	usb_dbg("+\n");

	if (!hisi_dwc3_dev) {
		usb_err("USB drv not probe!\n");
		return ;
	}

	if (hisi_dwc3_dev->phy_ops->lscdtimer_set)
		hisi_dwc3_dev->phy_ops->lscdtimer_set();

	usb_dbg("-\n");
}
EXPORT_SYMBOL_GPL(dwc3_lscdtimer_set);

#define DWC3_LLUCTL0				0xd024
#define DWC3_LLUCTL0_SUPPORT_P4_PG		(1u << 29)

#define DWC3_GUCTL_USBHSTINIMMRETRYEN		(1u << 14)

#define DWC3_GRXTHRCFG_USBRXPKTCNTSEL		(1u << 26)

#define DWC31_GUCTL2				0xc608
#define DWC31_GUCTL2_SVC_OPP_PER_HS(n)		((n) << 5)
#define DWC31_GUCTL2_SVC_OPP_PER_HS_MASK	DWC31_GUCTL2_SVC_OPP_PER_HS(3)

#define DWC3_GUCTL3_SVC_OPP_PER_HS_SEP(n)	((n) << 9)
#define DWC3_GUCTL3_SVC_OPP_PER_HS_SEP_MASK	DWC3_GUCTL3_SVC_OPP_PER_HS_SEP(0xF)

#define DWC3_GUCTL_DTOUT(n)			((n) << 0)
#define DWC3_GUCTL_DTOUT_MASK			DWC3_GUCTL_DTOUT(0x7FF)

#define DWC3_GUCTL1_HW_LPM_CAP_DISABLE		(1u << 13)
#define DWC3_GUCTL1_HW_LPM_HLE_DISABLE		(1u << 14)

void hisi_dwc3_platform_host_quirks(void)
{
	u32 reg;
	void __iomem *base;

	usb_dbg("+\n");

	if (!hisi_dwc3_dev) {
		usb_err("USB drv not probe!\n");
		return;
	}

	base = hisi_dwc3_dev->usb_core_reg_base;


	/* BugNo: 9001202031 */
	if (hisi_dwc3_dev->quirk_disable_usb2phy_suspend) {
		reg = readl(base + DWC3_GUSB2PHYCFG(0));
		reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;
		writel(reg, base + DWC3_GUSB2PHYCFG(0));
	}

	/* BugNo: 9001205968 */
	if (hisi_dwc3_dev->quirk_clear_svc_opp_per_hs) {
		reg = readl(base + DWC31_GUCTL2);
		reg &= ~DWC31_GUCTL2_SVC_OPP_PER_HS_MASK;
		writel(reg, base + DWC31_GUCTL2);
	}

	/* BugNo: 9001208988 */
	if (hisi_dwc3_dev->quirk_disable_rx_thres_cfg) {
		reg = readl(base + DWC3_GRXTHRCFG);
		reg &= ~DWC3_GRXTHRCFG_USBRXPKTCNTSEL;
		writel(reg, base + DWC3_GRXTHRCFG);
	}

	/* BugNo: 9001212079 */
	if (hisi_dwc3_dev->quirk_set_svc_opp_per_hs_sep) {
		reg = readl(base + DWC3_GUCTL3);
		reg &= ~DWC3_GUCTL3_SVC_OPP_PER_HS_SEP_MASK;
		reg |= DWC3_GUCTL3_SVC_OPP_PER_HS_SEP(3);
		writel(reg, base + DWC3_GUCTL3);
	}

	/* BugNo: 9001227814 */
	if (hisi_dwc3_dev->quirk_adjust_dtout) {
		reg = readl(base + DWC3_GUCTL);
		reg &= ~DWC3_GUCTL_DTOUT_MASK;
		reg |= DWC3_GUCTL_DTOUT(8);
		writel(reg, base + DWC3_GUCTL);
	}

	/* BugNo: 9001238552 */
	if (hisi_dwc3_dev->quirk_force_disable_host_lpm) {
		reg = readl(base + DWC3_GUCTL1);
		reg |= DWC3_GUCTL1_HW_LPM_CAP_DISABLE | DWC3_GUCTL1_HW_LPM_HLE_DISABLE;
		writel(reg, base + DWC3_GUCTL1);
	}

	/* BugNo: 9001162113 */
	if (hisi_dwc3_dev->quirk_enable_hst_imm_retry) {
		reg = readl(base + DWC3_GUCTL);
		reg |= DWC3_GUCTL_USBHSTINIMMRETRYEN;
		writel(reg, base + DWC3_GUCTL);
	}

	usb_dbg("-\n");
}

EXPORT_SYMBOL_GPL(hisi_dwc3_platform_host_quirks);

void hisi_dwc3_platform_device_quirks(void)
{
	u32 reg;
	void __iomem *base;

	usb_dbg("+\n");

	if (!hisi_dwc3_dev) {
		usb_err("USB drv not probe!\n");
		return;
	}

	base = hisi_dwc3_dev->usb_core_reg_base;

	/* BugNo: 9001169999 */
	if (hisi_dwc3_dev->quirk_enable_p4_gate) {
		reg = readl(base + DWC3_LLUCTL0);
		reg |= DWC3_LLUCTL0_SUPPORT_P4_PG;
		writel(reg, base + DWC3_LLUCTL0);
	}

	usb_dbg("-\n");
}
EXPORT_SYMBOL_GPL(hisi_dwc3_platform_device_quirks);

static enum otg_dev_event_type hifi_usb_event_filter(
					enum otg_dev_event_type event)
{
	if (event == START_HIFI_USB_RESET_VBUS)
		return START_HIFI_USB;
	else if (event == STOP_HIFI_USB_RESET_VBUS)
		return STOP_HIFI_USB;
	else
		return event;
}

static int id_rise_event_check(enum otg_dev_event_type last_event)
{
	if ((last_event == ID_FALL_EVENT)
		    || (last_event == START_HIFI_USB)
		    || (last_event == STOP_HIFI_USB)
		    || (last_event == START_AP_USE_HIFIUSB)
		    || (last_event == STOP_AP_USE_HIFIUSB)
		    || (last_event == HIFI_USB_HIBERNATE)
		    || (last_event == HIFI_USB_WAKEUP)
		    || (last_event == DISABLE_USB3_PORT))
		return 1;
	else
		return 0;
}

static int id_fall_event_check(enum otg_dev_event_type last_event)
{
	if ((last_event == CHARGER_DISCONNECT_EVENT)
			|| (last_event == ID_RISE_EVENT))
		return 1;
	else
		return 0;
}

static int start_hifi_usb_event_check(enum otg_dev_event_type last_event)
{
	if ((last_event == ID_FALL_EVENT)
		    || (last_event == STOP_HIFI_USB)
		    || (last_event == START_HIFI_USB)) /* start hifiusb maybe failed, allow retry */
		return 1;
	else
		return 0;
}

static int stop_hifi_usb_event_check(enum otg_dev_event_type last_event)
{
	if ((last_event == START_HIFI_USB)
		    || (last_event == HIFI_USB_WAKEUP)
		    || (last_event == HIFI_USB_HIBERNATE)
		    || (last_event == ID_FALL_EVENT))
		return 1;
	else
		return 0;

}

static int hifi_usb_hibernate_event_check(enum otg_dev_event_type last_event)
{
	if ((last_event == START_HIFI_USB)
		    || (last_event == HIFI_USB_WAKEUP)
		    || (last_event == ID_FALL_EVENT))
		return 1;
	return 0;
}

static int hifi_usb_wakeup_event_check(enum otg_dev_event_type last_event)
{
	if ((last_event == HIFI_USB_HIBERNATE)
			|| (last_event == HIFI_USB_WAKEUP))
		return 1;
	return 0;
}

static int event_check(enum otg_dev_event_type last_event,
		enum otg_dev_event_type new_event)
{
	int ret = 0;

	if (last_event == NONE_EVENT)
		return 1;

	last_event = hifi_usb_event_filter(last_event);
	new_event = hifi_usb_event_filter(new_event);

	switch (new_event) {
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
		ret = id_fall_event_check(last_event);
		break;
	case ID_RISE_EVENT:
		ret = id_rise_event_check(last_event);
		break;
	case START_HIFI_USB:
		ret = start_hifi_usb_event_check(last_event);
		break;
	case STOP_HIFI_USB:
		ret = stop_hifi_usb_event_check(last_event);
		break;
	case HIFI_USB_HIBERNATE:
		ret = hifi_usb_hibernate_event_check(last_event);
		break;
	case HIFI_USB_WAKEUP:
		ret = hifi_usb_wakeup_event_check(last_event);
		break;
	case START_AP_USE_HIFIUSB:
		if ((last_event == ID_FALL_EVENT)
				|| (last_event == STOP_HIFI_USB)
				|| (last_event == STOP_AP_USE_HIFIUSB))
			ret = 1;
		break;
	case STOP_AP_USE_HIFIUSB:
		if (last_event == START_AP_USE_HIFIUSB)
			ret = 1;
		break;
	case DISABLE_USB3_PORT:
		if (last_event == ID_FALL_EVENT ||
				last_event == STOP_HIFI_USB)
			ret = 1;
		break;
	default:
		break;
	}
	return ret;
}

static void time_consuming_check(struct hisi_dwc3_device *hisi_dwc3,
			enum otg_dev_event_type event)
{
	if (event == ID_FALL_EVENT)
		hisi_dwc3->start_host_time_stamp = jiffies;
	else if (event == ID_RISE_EVENT)
		hisi_dwc3->stop_host_time_stamp = jiffies;
	else if ((event == START_HIFI_USB)
			|| (event == START_HIFI_USB_RESET_VBUS)) {
		hisi_dwc3->start_hifi_usb_time_stamp = jiffies;

		usb_dbg("xhci enum audio device time: %d ms\n",
			jiffies_to_msecs(
				hisi_dwc3->start_hifi_usb_time_stamp
				- hisi_dwc3->start_host_time_stamp));
	}
}

int hisi_usb_vbus_value(void)
{
	return hisi_pmic_get_vbus_status();
}

/*
 * return 0 means event was accepted, others means event was rejected.
 */
int hisi_usb_otg_event(enum otg_dev_event_type event)
{
	int ret = 0;
#ifdef CONFIG_USB_DWC3_OTG
	unsigned long flags;
	struct hisi_dwc3_device *hisi_dwc3 = hisi_dwc3_dev;

	if (!hisi_dwc3)
		return -ENODEV;

	if (hisi_dwc3->eventmask) {
		usb_dbg("eventmask enabled, mask all events.\n");
		return -EPERM;
	}

	spin_lock_irqsave(&(hisi_dwc3->event_lock), flags);

	if (event_check(hisi_dwc3->event, event)) {
		usb_dbg("event: %s\n", event_type_string(event));
		hisi_dwc3->event = event;

		time_consuming_check(hisi_dwc3, event);

		if ((CHARGER_CONNECT_EVENT == event)
				|| (CHARGER_DISCONNECT_EVENT == event))
			hisi_dwc3_wake_lock(hisi_dwc3);

		if (!event_enqueue(&hisi_dwc3->event_queue, event)) {
			if (!queue_work(system_power_efficient_wq,
					&hisi_dwc3->event_work)) {
				usb_err("schedule event_work wait:%d\n", event);
			}
		} else {
			usb_err("hisi_usb_otg_event can't enqueue event:%d\n", event);
			ret = -EBUSY;
		}

		if ((ID_RISE_EVENT == event) || (CHARGER_DISCONNECT_EVENT == event)) {
			event_queue_set_overlay(&hisi_dwc3->event_queue);

			usb_dbg("it need notify USB_SPEED_UNKNOWN to app while usb plugout\n");
			hisi_dwc3->speed = USB_SPEED_UNKNOWN;
			if (!queue_work(system_power_efficient_wq,
							&hisi_dwc3->speed_change_work)) {
				usb_err("schedule speed_change_work wait:%d\n", hisi_dwc3->speed);
			}
		}
	} else {
		usb_err("last event: [%s], event [%s] was rejected.\n",
				event_type_string(hisi_dwc3->event), event_type_string(event));
		ret = -EINVAL;
	}

	spin_unlock_irqrestore(&(hisi_dwc3->event_lock), flags);
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(hisi_usb_otg_event);

/*lint -save -e578 */
static inline void dwc3_event_work_set_sync(struct hisi_dwc3_device *hisi_dwc)
{
	/* set sync flag */
	hisi_dwc->is_hanle_event_sync = 1;
}

static inline void dwc3_event_work_clr_sync(struct hisi_dwc3_device *hisi_dwc)
{
	hisi_dwc->is_hanle_event_sync = 0;
}

static DEFINE_MUTEX(sync_event_lock);

int hisi_usb_otg_use_hifi_ip_first()
{
	struct hisi_dwc3_device *hisi_dwc3 = hisi_dwc3_dev;

	/*
	 * just check if usb module probe.
	 */
	if (!hisi_dwc3) {
		usb_err("usb module not probe\n");
		return 0;
	}

	return ((hisi_dwc3->hifi_ip_first)
				&& (0 == get_hifi_usb_retry_count())
				&& (0 == get_never_hifi_usb_value())
				&& (0 == usbaudio_nv_is_ready()));
}
EXPORT_SYMBOL_GPL(hisi_usb_otg_use_hifi_ip_first);

int hisi_usb_otg_get_typec_orien()
{
	struct hisi_dwc3_device *hisi_dwc3 = hisi_dwc3_dev;

	/*
	 * just check if usb module probe.
	 */
	if (!hisi_dwc3) {
		usb_err("usb module not probe\n");
		return 0;
	}

	return hisi_dwc3->plug_orien;
}
EXPORT_SYMBOL_GPL(hisi_usb_otg_get_typec_orien);

int hisi_usb_otg_event_sync(TCPC_MUX_CTRL_TYPE mode_type, enum otg_dev_event_type event, TYPEC_PLUG_ORIEN_E typec_orien)
{
	int ret = -EINVAL;
	struct hisi_dwc3_device *hisi_dwc3 = hisi_dwc3_dev;
	void __iomem *pericfg_base;

	usb_info("+.\n");

	/*
	 * just check if usb module probe.
	 */
	if (!hisi_dwc3) {
		usb_err("usb module not probe\n");
		return -EBUSY;
	}

	pericfg_base = hisi_dwc3->pericfg_reg_base;/*lint !e529 */
	if (!pericfg_base) {
		usb_err("pericfg base if null\n");
		return -EFAULT;
	}

	/*
	 * step 0: check if in interrupt context.
	 */
	if (in_interrupt()) {
		usb_err("Hard interrupt context.\n");
		WARN_ON_ONCE(1);
		return ret;
	}

	/*
	 * step 1: check input event.
	 */
	if (NONE_EVENT < event) { /*lint !e685 */
		usb_err("unknow event:%d.\n", event);
		return ret;
	}

	/*
	 * step 2:check eventmask
	 */
	if (hisi_dwc3->eventmask) {
		usb_err("eventmask enabled, sync mask all events.\n");
		return ret;
	}

	mutex_lock(&sync_event_lock);
	dwc3_event_work_set_sync(hisi_dwc3);

	hisi_dwc3->mode_type = mode_type;
	hisi_dwc3->plug_orien = typec_orien;

	ret = hisi_usb_otg_event(event);
	if (0 <= ret) {
		usb_info("start wait.\n");
		ret = 0;
		/* wait time greater than (XHCI_CMD_DEFAULT_TIMEOUT * 2) */
		if(!wait_for_completion_timeout(&hisi_dwc3->event_completion,
						msecs_to_jiffies(10500))) {
			usb_err("usb task timeout!\n");
			ret = -EAGAIN;
		}
	} else {
		usb_err("no need wait [ret=%d]\n", ret);
	}

	dwc3_event_work_clr_sync(hisi_dwc3);
	mutex_unlock(&sync_event_lock);

	usb_info("-.\n");

	return ret;
}
EXPORT_SYMBOL_GPL(hisi_usb_otg_event_sync);

int hisi_dwc3_is_powerdown(void)
{
	int power_flag = get_hisi_dwc3_power_flag();
	return ((USB_POWER_OFF == power_flag) || (USB_POWER_HOLD == power_flag));
}

#ifdef CONFIG_HISI_DEBUG_FS
void usb_start_dump(void)
{
	if (!queue_work(system_power_efficient_wq, &hisi_dwc3_dev->usb_core_reg_dump_work)) {
		usb_err("[USB.DBG] usb linkstate work not run!\n");
	}
}

static void usb_core_reg_dump_work(struct work_struct *work)
{
	volatile unsigned int reg;
	unsigned long long int count6 = 0;
	int flg6;
	int flg3;
	unsigned long long int count3 = 0;
	unsigned long long int count8 = 0;
	int old = 0;
	struct hisi_dwc3_device *hisi_dwc = container_of(work,
				    struct hisi_dwc3_device, usb_core_reg_dump_work);
	flg6 = 1;
	flg3 = 1;
	while (!hisi_dwc3_is_powerdown()) {
		reg =  readl(hisi_dwc->usb_core_reg_base + 0xd050);
		switch((reg >> 22) & 0xf) {
			case 6:
				count6++;
				if (0 == (count6 % 100000)) {
					printk(KERN_DEBUG "[LINK]:%x, %lld\n", reg, count6);
				}
				if (flg6) {
					printk(KERN_DEBUG "[LINK]:%x, %lld\n", reg, count6);
					count6 = 0;
					flg6 = 0;
					flg3 = 1;
				}
				break;
			case 0:
			case 1:
			case 2:
			case 3:
				count3++;
				if (0 == (count3 % 100000)) {
					printk(KERN_DEBUG "[LINK]:%x, %lld\n", reg, count3);
				}
				if (flg3) {
					printk(KERN_DEBUG "[LINK]:0x%x, %lld\n", reg, count3);
					count3 = 0;
					flg3 = 0;
					flg6 = 1;
				}
				break;
			case 4:
			case 5:
			case 7:
			case 8:
				flg3 = 1;
				flg6 = 1;
				if (((reg >> 18) & 0xf) == old) {
					count8++;
					break;
				}
				old = ((reg >> 18) & 0xf);
				printk(KERN_DEBUG "[USB.DBG.LINK]:0x%x\n", reg);
				break;
			default:
				flg3 = 1;
				flg6 = 1;
				printk(KERN_DEBUG "[USB.DBG.LINK]:0x%x\n", reg);
		}
	}
}
#endif

static void bc_again(struct hisi_dwc3_device *hisi_dwc)
{
	int ret;
	bool schedule = false;
	unsigned int bc_again_delay_time = 0;

	usb_dbg("+\n");

	/*
	 * Check usb controller status.
	 */
	if (hisi_dwc3_is_powerdown()) {
		usb_err("usb controller is reset, just return\n");
		return ;
	}

	/*
	 * STEP 1
	 */
	/* stop peripheral which is started when detected as SDP before */
	if (enumerate_allowed(hisi_dwc)) {
		ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_VBUS_CLEAR);
		if (ret) {
			usb_err("stop peripheral error\n");
			return;
		}
	}

	/*
	 * STEP 2
	 */
	/* if it is CHARGER_TYPE_UNKNOWN, we should pull down d+&d- for 20ms*/
	if (hisi_dwc->charger_type == CHARGER_TYPE_UNKNOWN) {
		hisi_bc_dplus_pulldown(hisi_dwc);
		msleep(20);
		hisi_bc_dplus_pullup(hisi_dwc);
	}

	hisi_dwc->charger_type = detect_charger_type(hisi_dwc);
	notify_charger_type(hisi_dwc);

	if (hisi_dwc->charger_type == CHARGER_TYPE_UNKNOWN) {
		unsigned long flags;

		spin_lock_irqsave(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
		if (hisi_dwc->bc_again_delay_time == BC_AGAIN_DELAY_TIME_1) {
			hisi_dwc->bc_again_delay_time = BC_AGAIN_DELAY_TIME_2;
			schedule = true;
		}

		bc_again_delay_time = hisi_dwc->bc_again_delay_time;
		spin_unlock_irqrestore(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
	} else {
		hisi_dwc->bc_again_delay_time = 0;
	}

	/*
	 * STEP 3
	 */
	/* must recheck enumerate_allowed, because charger_type maybe changed,
	 * and enumerate_allowed according to charger_type */
	if (enumerate_allowed(hisi_dwc)) {
		/* start peripheral */
		ret = dwc3_otg_work(dwc_otg_handler,
				DWC3_OTG_EVT_VBUS_SET);
		if (ret) {
			usb_err("start peripheral error\n");
			return;
		}
	} else {
		usb_dbg("it need notify USB_CONNECT_DCP while a real charger connected\n");
		hisi_dwc->speed = USB_CONNECT_DCP;
		if (!queue_work(system_power_efficient_wq,
						&hisi_dwc->speed_change_work)) {
			usb_err("schedule speed_change_work wait:%d\n", hisi_dwc->speed);
		}
	}

	/* recheck sleep_allowed for charger_type maybe changed */
	if (sleep_allowed(hisi_dwc))
		hisi_dwc3_wake_unlock(hisi_dwc);
	else
		hisi_dwc3_wake_lock(hisi_dwc);

	if (schedule) {
		ret = queue_delayed_work(system_power_efficient_wq,
				&hisi_dwc->bc_again_work,
				msecs_to_jiffies(bc_again_delay_time));
		usb_dbg("schedule ret:%d, run bc_again_work %dms later\n",
			ret, bc_again_delay_time);
	}

	usb_dbg("-\n");
}
/*lint -restore */

void hisi_usb_otg_bc_again(void)
{
	struct hisi_dwc3_device *hisi_dwc = hisi_dwc3_dev;

	usb_dbg("+\n");

	if (!hisi_dwc) {
		usb_err("No usb module, can't call bc again api\n");
		return;
	}

	if ((1 == hisi_dwc->bc_again_flag) && (BC_AGAIN_ONCE == hisi_dwc->bc_unknown_again_flag)) {
		mutex_lock(&hisi_dwc->lock);

		/* we are here because it's detected as SDP before */
		if (hisi_dwc->charger_type == CHARGER_TYPE_UNKNOWN) {
			usb_dbg("charger_type is UNKNOWN, start bc_again_work\n");
			bc_again(hisi_dwc);
		}

		mutex_unlock(&hisi_dwc->lock);
	} else
		usb_dbg("hisi_usb_otg_bc_again do nothing!\n");

	usb_dbg("-\n");
}
EXPORT_SYMBOL_GPL(hisi_usb_otg_bc_again);

static void bc_again_work(struct work_struct *work)
{
	struct hisi_dwc3_device *hisi_dwc = container_of(work,
			struct hisi_dwc3_device, bc_again_work.work);

	usb_dbg("+\n");
	mutex_lock(&hisi_dwc->lock);

	/* we are here because it's detected as SDP before */
	if (bc_again_allowed(hisi_dwc)) {
		usb_dbg("charger_type is not DCP, start bc_again_work\n");
		bc_again(hisi_dwc);
	}

	mutex_unlock(&hisi_dwc->lock);
	usb_dbg("-\n");
}

/**
 * In some cases, DCP is detected as SDP wrongly. To avoid this,
 * start bc_again delay work to detect charger type once more.
 * If later the enum process is executed, then it's a real SDP, so
 * the work will be canceled.
 */
static void schedule_bc_again(struct hisi_dwc3_device *hisi_dwc)
{
	int ret;
	unsigned long flags;
	unsigned int bc_again_delay_time;

	usb_dbg("+\n");

	if (!hisi_dwc->bc_again_flag)
		return;

	spin_lock_irqsave(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
	if ((hisi_dwc->charger_type == CHARGER_TYPE_UNKNOWN)
		&& (BC_AGAIN_TWICE == hisi_dwc->bc_unknown_again_flag))
		hisi_dwc->bc_again_delay_time = BC_AGAIN_DELAY_TIME_1;
	else
		hisi_dwc->bc_again_delay_time = BC_AGAIN_DELAY_TIME_2;

	bc_again_delay_time = hisi_dwc->bc_again_delay_time;
	spin_unlock_irqrestore(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/

	ret = queue_delayed_work(system_power_efficient_wq,
			&hisi_dwc->bc_again_work,
			msecs_to_jiffies(bc_again_delay_time));
	usb_dbg("schedule ret:%d, run bc_again_work %dms later\n",
		ret, bc_again_delay_time);

	usb_dbg("-\n");
}

void hisi_usb_cancel_bc_again(int sync)
{
	if (!hisi_dwc3_dev) {
		usb_err("hisi_dwc3_dev is null\n");
		return;
	}

	cancel_bc_again(hisi_dwc3_dev, sync);
}

static void cancel_bc_again(struct hisi_dwc3_device *hisi_dwc, int sync)
{
	usb_dbg("+\n");
	if (hisi_dwc->bc_again_flag) {
		int ret;
		if (sync)
			ret = cancel_delayed_work_sync(&hisi_dwc->bc_again_work);
		else
			ret = cancel_delayed_work(&hisi_dwc->bc_again_work);
		usb_dbg("cancel_delayed_work(result:%d)\n", ret);
		hisi_dwc->bc_again_delay_time = 0;
	}
	usb_dbg("-\n");
}

static void hisi_dwc3_cmd_tmo_dbg_print(struct hisi_dwc3_device *hisi_dwc)
{
	usb_dbg("+\n");
	if (hisi_dwc->phy_ops->cmd_tmo_dbg_print)
		hisi_dwc->phy_ops->cmd_tmo_dbg_print(hisi_dwc);
	usb_dbg("-\n");
}

static int device_event_notifier_fn(struct notifier_block *nb,
			unsigned long event, void *para)
{
	struct hisi_dwc3_device *hisi_dwc = container_of(nb,
			struct hisi_dwc3_device, event_nb);
	enum usb_device_speed  speed;
	unsigned long flags;
	usb_dbg("+\n");

	switch (event) {
	case DEVICE_EVENT_CONNECT_DONE:
		speed = *(enum usb_device_speed  *)para;
		spin_lock_irqsave(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/

		/*
		 * Keep VDP_SRC if speed is USB_SPEED_SUPER
		 * and charger_type is CHARGER_TYPE_CDP.
		*/
		if (hisi_dwc->charger_type == CHARGER_TYPE_CDP &&
				speed == USB_SPEED_SUPER)
			hisi_bc_enable_vdp_src(hisi_dwc);

		spin_unlock_irqrestore(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
		break;

	case DEVICE_EVENT_RESET:
		/* Disable VDP_SRC for communicaton on D+ */
		spin_lock_irqsave(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
		hisi_bc_disable_vdp_src(hisi_dwc);
		spin_unlock_irqrestore(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
		break;

	case DEVICE_EVENT_CMD_TMO:
		hisi_dwc3_cmd_tmo_dbg_print(hisi_dwc);
		break;

	case DEVICE_EVENT_SETCONFIG:
		speed = *(enum usb_device_speed  *)para;
		spin_lock_irqsave(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
		if (hisi_dwc->charger_type == CHARGER_TYPE_UNKNOWN) {
			hisi_dwc->charger_type = CHARGER_TYPE_SDP;
		}

		hisi_dwc->speed = speed;
		if (!queue_work(system_power_efficient_wq,
						&hisi_dwc->speed_change_work)) {
			usb_err("schedule speed_change_work wait:%d\n", hisi_dwc->speed);
		}

		spin_unlock_irqrestore(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
		break;

	default:
		break;
	}

	usb_dbg("-\n");
	return 0;
}/*lint !e715*/

/*
 * Huawei st310 is a super-speed mass storage device. A call may cause
 * disconnection. Once it disconnected, force USB as high-speed.
 */
static void huawei_st310_quirk(struct usb_device *udev)
{
	int typec_state = PD_DPM_USB_TYPEC_DETACHED;

#define VID_HUAWEI_ST310 0x12D1
#define PID_HUAWEI_ST310 0x3B40

	if (udev->descriptor.idVendor == VID_HUAWEI_ST310 &&
			udev->descriptor.idProduct == PID_HUAWEI_ST310 &&
			udev->speed == USB_SPEED_SUPER) {
#ifdef CONFIG_TCPC_CLASS
		pd_dpm_get_typec_state(&typec_state);
#endif
		if (typec_state != PD_DPM_USB_TYPEC_DETACHED)
			hisi_usb_otg_event(DISABLE_USB3_PORT);
	}
}

static int xhci_notifier_fn(struct notifier_block *nb,
			unsigned long action, void *data)
{
	struct usb_device *udev = (struct usb_device *)data;

	usb_dbg("+\n");

	if (!udev) {
		usb_dbg("udev is null,just return\n");
		return 0;
	}

	if ((action == USB_DEVICE_ADD) && (USB_CLASS_HUB == udev->descriptor.bDeviceClass)) {
		usb_dbg("usb hub don't notify\n");
		return 0;
	}

	if (((action == USB_DEVICE_ADD) || (action == USB_DEVICE_REMOVE))
		&& ((udev->parent != NULL) && (udev->parent->parent == NULL))) {
		usb_dbg("xhci device speed is %d action %s\n", udev->speed,
			(action == USB_DEVICE_ADD)?"USB_DEVICE_ADD":"USB_DEVICE_REMOVE");

		/*only device plug out while phone is host mode,not the usb cable*/
		if (action == USB_DEVICE_REMOVE)
			hisi_dwc3_dev->speed = USB_CONNECT_HOST;
		else
			hisi_dwc3_dev->speed = udev->speed;

		if (action == USB_DEVICE_ADD)
			hisi_dwc3_dev->hifi_usb_setconfig_time_stamp = jiffies;

		if (!queue_work(system_power_efficient_wq,
						&hisi_dwc3_dev->speed_change_work)) {
			usb_err("schedule speed_change_work wait:%d\n", hisi_dwc3_dev->speed);
		}

		/* disable usb3.0 quirk for Huawei ST310-S1 */
		if (action == USB_DEVICE_REMOVE) {
			huawei_st310_quirk(udev);
		}

	}

	usb_dbg("-\n");
	return 0;
}

int hisi_usb_bc_init(struct hisi_dwc3_device *hisi_dwc)
{
	int ret;
	struct device *dev = &hisi_dwc->pdev->dev;

	usb_dbg("+\n");

	spin_lock_init(&hisi_dwc->bc_again_lock);/*lint !e550*/

	if (of_property_read_u32(dev->of_node, "bc_again_flag",
			    &(hisi_dwc->bc_again_flag))) {
		hisi_dwc->bc_again_flag = 0;
	}

	if (hisi_dwc->bc_again_flag) {
		INIT_DELAYED_WORK(&hisi_dwc->bc_again_work, bc_again_work);
		if (of_property_read_u32(dev->of_node, "bc_unknown_again_flag",
			    &(hisi_dwc->bc_unknown_again_flag))) {
			hisi_dwc->bc_unknown_again_flag = 0;
		}
	} else
		hisi_dwc->bc_unknown_again_flag = 0;

	hisi_dwc->event_nb.notifier_call = device_event_notifier_fn;
	ret = dwc3_device_event_notifier_register(&hisi_dwc->event_nb);
	if (ret) {
		usb_err("dwc3_device_event_notifier_register failed\n");
		return ret;
	}


	usb_dbg("-\n");
	return 0;

}

void hisi_usb_bc_exit(struct hisi_dwc3_device *hisi_dwc)
{
	usb_dbg("+\n");

	dwc3_device_event_notifier_unregister(&hisi_dwc->event_nb);
	hisi_dwc->event_nb.notifier_call = NULL;

	usb_dbg("-\n");
}

/**
 * get_usb_state() - get current USB cable state.
 * @hisi_dwc: the instance pointer of struct hisi_dwc3_device
 *
 * return current USB cable state according to VBUS status and ID status.
 */
static enum usb_state get_usb_state(struct hisi_dwc3_device *hisi_dwc)
{
        if (hisi_dwc->fpga_flag) {
                usb_dbg("this is fpga platform, usb is device mode\n");
                return USB_STATE_DEVICE;
        }

        if (hisi_usb_vbus_value() == 0)
                return USB_STATE_OFF;
        else
                return USB_STATE_DEVICE;
}

static void get_phy_param(struct hisi_dwc3_device *hisi_dwc3)
{
	struct device *dev = &hisi_dwc3->pdev->dev;

	/* hs phy param for device mode */
	if (of_property_read_u32(dev->of_node, "eye_diagram_param",
			&(hisi_dwc3->eye_diagram_param))) {
		usb_dbg("get eye diagram param form dt failed, use default value\n");
		hisi_dwc3->eye_diagram_param = 0x1c466e3;
	}
	usb_dbg("eye diagram param: 0x%x\n", hisi_dwc3->eye_diagram_param);

	/* hs phy param for host mode */
	if (of_property_read_u32(dev->of_node, "eye_diagram_host_param",
			&(hisi_dwc3->eye_diagram_host_param))) {
		usb_dbg("get eye diagram host param form dt failed, use default value\n");
		hisi_dwc3->eye_diagram_host_param = 0x1c466e3;
	}
	usb_dbg("eye diagram host param: 0x%x\n", hisi_dwc3->eye_diagram_host_param);

	/* ss phy Rx Equalization */
	if (of_property_read_u32(dev->of_node, "usb3_phy_cr_param",
			&(hisi_dwc3->usb3_phy_cr_param))) {
		usb_dbg("get usb3_phy_cr_param form dt failed, use default value\n");
		hisi_dwc3->usb3_phy_cr_param = (1 << 11) | (3 << 8) | (1 << 7);
	}

	/* ss phy Rx Equalization for host mode */
	if (of_property_read_u32(dev->of_node, "usb3_phy_host_cr_param",
			&(hisi_dwc3->usb3_phy_host_cr_param))) {
		usb_dbg("get usb3_phy_host_cr_param form dt failed, use default value\n");
		hisi_dwc3->usb3_phy_host_cr_param = (1 << 11) | (1 << 8) | (1 << 7);
	}

	usb_dbg("usb3_phy_cr_param: 0x%x\n", hisi_dwc3->usb3_phy_cr_param);
	usb_dbg("usb3_phy_host_cr_param: 0x%x\n", hisi_dwc3->usb3_phy_host_cr_param);

	/* tx_vboost_lvl */
	if (of_property_read_u32(dev->of_node, "usb3_phy_tx_vboost_lvl",
			&(hisi_dwc3->usb3_phy_tx_vboost_lvl))) {
		usb_dbg("get usb3_phy_tx_vboost_lvl form dt failed, use default value\n");
		hisi_dwc3->usb3_phy_tx_vboost_lvl = VBOOST_LVL_DEFAULT_PARAM;
	}
	usb_dbg("usb3_phy_tx_vboost_lvl: %d\n", hisi_dwc3->usb3_phy_tx_vboost_lvl);
}

static inline int hisi_dwc3_phy_init(struct hisi_dwc3_device *hisi_dwc)
{
	if (hisi_dwc->phy_ops->init)
		return hisi_dwc->phy_ops->init(hisi_dwc);

	usb_err("have not found phy init ops\n");
	return -1;
}

static inline int hisi_dwc3_phy_shutdown(struct hisi_dwc3_device *hisi_dwc)
{
	if (hisi_dwc->phy_ops->shutdown)
		return hisi_dwc->phy_ops->shutdown(hisi_dwc);

	usb_err("have not found phy shutdown ops\n");
	return -1;
}


static int hisi_dwc3_usb20_phy_init(struct hisi_dwc3_device *hisi_dwc,
		unsigned int combophy_flag)
{
	int ret = 0;
	usb_dbg("+\n");
	if (hisi_dwc->phy_ops->shared_phy_init)
		return hisi_dwc->phy_ops->shared_phy_init(hisi_dwc, combophy_flag);
	else
		WARN_ON(1);
	usb_dbg("-\n");
	return ret;
}

static int hisi_dwc3_usb20_phy_shutdown(struct hisi_dwc3_device *hisi_dwc,
		unsigned int combophy_flag, unsigned int keep_power)
{
	int ret = 0;

	usb_dbg("+\n");
	if (hisi_dwc->phy_ops->shared_phy_shutdown)
		return hisi_dwc->phy_ops->shared_phy_shutdown(hisi_dwc, combophy_flag, keep_power);
	else
		WARN_ON(1);
	usb_dbg("-\n");
	return ret;
}

static inline int hisi_dwc3_phy_get_resource(struct hisi_dwc3_device *hisi_dwc)
{
	if (hisi_dwc->phy_ops->get_dts_resource) {
		return hisi_dwc->phy_ops->get_dts_resource(hisi_dwc);
	}
	return 0;
}

static void get_resource_for_fpga(struct hisi_dwc3_device *hisi_dwc3)
{
	struct device *dev = &hisi_dwc3->pdev->dev;

	hisi_dwc3->fpga_usb_mode_gpio = -1;
	hisi_dwc3->fpga_otg_drv_vbus_gpio = -1;
	hisi_dwc3->fpga_phy_reset_gpio = -1;
	hisi_dwc3->fpga_phy_switch_gpio = -1;

	if (of_property_read_u32(dev->of_node, "fpga_flag",
			    &(hisi_dwc3->fpga_flag))) {
		hisi_dwc3->fpga_flag = 0;
	}

	if (hisi_dwc3->fpga_flag == 0)
		return;

	usb_dbg("this is fpga platform\n");

	hisi_dwc3->fpga_usb_mode_gpio = of_get_named_gpio(dev->of_node,
			"fpga_usb_mode_gpio", 0);
	hisi_dwc3->fpga_otg_drv_vbus_gpio = of_get_named_gpio(dev->of_node,
			"fpga_otg_drv_vbus_gpio", 0);
	hisi_dwc3->fpga_phy_reset_gpio = of_get_named_gpio(dev->of_node,
			"fpga_phy_reset_gpio", 0);
	hisi_dwc3->fpga_phy_switch_gpio = of_get_named_gpio(dev->of_node,
			"fpga_phy_switch_gpio", 0);

	usb_dbg("fpga usb gpio info:"
		"usb_mode=%d, dr_vbus=%d, phy_reset=%d, phy_switch=%d\n",
			hisi_dwc3->fpga_usb_mode_gpio,
			hisi_dwc3->fpga_otg_drv_vbus_gpio,
			hisi_dwc3->fpga_phy_reset_gpio,
			hisi_dwc3->fpga_phy_switch_gpio);
}

static void get_quirks_dts(struct hisi_dwc3_device *hisi_dwc3)
{
	struct device *dev = &hisi_dwc3->pdev->dev;

	hisi_dwc3->quirk_enable_hst_imm_retry = device_property_read_bool(dev,
				"quirk_enable_hst_imm_retry");
	hisi_dwc3->quirk_disable_rx_thres_cfg = device_property_read_bool(dev,
				"quirk_disable_rx_thres_cfg");
	hisi_dwc3->quirk_disable_usb2phy_suspend = device_property_read_bool(dev,
				"quirk_disable_usb2phy_suspend");
	hisi_dwc3->quirk_clear_svc_opp_per_hs = device_property_read_bool(dev,
				"quirk_clear_svc_opp_per_hs");
	hisi_dwc3->quirk_set_svc_opp_per_hs_sep = device_property_read_bool(dev,
				"quirk_set_svc_opp_per_hs_sep");
	hisi_dwc3->quirk_adjust_dtout = device_property_read_bool(dev,
				"quirk_adjust_dtout");
	hisi_dwc3->quirk_force_disable_host_lpm = device_property_read_bool(dev,
				"quirk_force_disable_host_lpm");
	hisi_dwc3->quirk_enable_p4_gate = device_property_read_bool(dev,
				"quirk_enable_p4_gate");
}

static int get_base_addr_resource(struct hisi_dwc3_device *hisi_dwc3)
{
	struct device *dev = &hisi_dwc3->pdev->dev;
	struct resource *res;
	struct device_node *np;

	/*
	 * map PERI CRG region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
	if (!np) {
		dev_err(dev, "get peri cfg node failed!\n");
		return -EINVAL;
	}
	hisi_dwc3->pericfg_reg_base = of_iomap(np, 0);
	if (!hisi_dwc3->pericfg_reg_base) {
		dev_err(dev, "iomap pericfg_reg_base failed!\n");
		return -EINVAL;
	}

	/*
	 * map PCTRL region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,pctrl");
	if (!np) {
		dev_err(dev, "get pctrl node failed!\n");
		return -EINVAL;
	}
	hisi_dwc3->pctrl_reg_base = of_iomap(np, 0);
	if (!hisi_dwc3->pctrl_reg_base) {
		dev_err(dev, "iomap pctrl_reg_base failed!\n");
		return -EINVAL;
	}

	/*
	 * map SCTRL region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (!np) {
		dev_err(dev, "get sysctrl node failed!\n");
		return -EINVAL;
	}
	hisi_dwc3->sctrl_reg_base = of_iomap(np, 0);
	if (!hisi_dwc3->sctrl_reg_base) {
		dev_err(dev, "iomap sctrl_reg_base failed!\n");
		return -EINVAL;
	}

	/*
	 * map PMCTRL region
	 */
	np = of_find_compatible_node(NULL, NULL, "hisilicon,pmctrl");
	if (!np) {
		dev_err(dev, "get pmctrl node failed!\n");
		return -EINVAL;
	}
	hisi_dwc3->pmctrl_reg_base = of_iomap(np, 0);
	if (!hisi_dwc3->pmctrl_reg_base) {
		dev_err(dev, "iomap pmctrl_reg_base failed!\n");
		return -EINVAL;
	}

	/*
	 * map OTG BC region
	 */
	res = platform_get_resource(hisi_dwc3->pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "missing memory base resource\n");
		return -EINVAL;
	}

	hisi_dwc3->otg_bc_reg_base = devm_ioremap_nocache(dev, res->start, resource_size(res));
	if (IS_ERR_OR_NULL(hisi_dwc3->otg_bc_reg_base)) {
		dev_err(dev, "ioremap res 0 failed\n");
		return -ENOMEM;
	}

	/*
	 * map USB CORE region
	 */
	res = platform_get_resource(hisi_dwc3->pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(dev, "usb missing base resource\n");
		return -EINVAL;
	}

	hisi_dwc3->usb_core_reg_base = devm_ioremap_nocache(dev, res->start, resource_size(res));
	if (IS_ERR_OR_NULL(hisi_dwc3->usb_core_reg_base)) {
		dev_err(dev, "ioremap res 1 failed\n");
		return -ENOMEM;
	}

	return 0;
}


/**
 * get_resource() - prepare resources
 * @hisi_dwc3: the instance pointer of struct hisi_dwc3_device
 *
 * 1. get registers base address and map registers region.
 * 2. get regulator handler.
 */
static int get_resource(struct hisi_dwc3_device *hisi_dwc3)
{
	struct device *dev = &hisi_dwc3->pdev->dev;

	if (get_base_addr_resource(hisi_dwc3))
		return -EINVAL;

	hisi_dwc3->bc_ctrl_reg = hisi_dwc3->otg_bc_reg_base + BC_CTRL2;
	get_phy_param(hisi_dwc3);

	get_resource_for_fpga(hisi_dwc3);

	get_quirks_dts(hisi_dwc3);

	if (hisi_dwc3_phy_get_resource(hisi_dwc3)) {
		dev_err(dev, "get phy resource error!\n");
		return -EINVAL;
	}

	if (of_property_read_u32(dev->of_node, "dma_mask_bit",
			    &(hisi_dwc3->dma_mask_bit))) {
		hisi_dwc3->dma_mask_bit = 32;
	}

	if (of_property_read_u32(dev->of_node, "hifi_ip_first",
			    &(hisi_dwc3->hifi_ip_first))) {
		hisi_dwc3->hifi_ip_first = 0;
	}

	return 0;
}

static void request_gpio(int *gpio)
{
	int ret;

	if (*gpio < 0)
		return;

	ret = gpio_request((unsigned)(*gpio), NULL);
	if (ret) {
		usb_err("request gpio %d failed\n", *gpio);
		*gpio = -1;
	}
}

static void request_gpios_for_fpga(struct hisi_dwc3_device *hisi_dwc)
{
	request_gpio(&hisi_dwc->fpga_usb_mode_gpio);
	request_gpio(&hisi_dwc->fpga_otg_drv_vbus_gpio);
	request_gpio(&hisi_dwc->fpga_phy_reset_gpio);
	request_gpio(&hisi_dwc->fpga_phy_switch_gpio);
}

int hisi_dwc3_probe(struct platform_device *pdev, struct usb3_phy_ops *phy_ops)
{
	int ret;
	struct hisi_dwc3_device *hisi_dwc;
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	enum usb_state init_state;

	usb_dbg("+\n");

	/* [first] check arg & create dwc control struct */
	if (!phy_ops) {
		usb_err("phy_ops is NULL\n");
		return -EINVAL;
	}

	hisi_dwc = devm_kzalloc(dev, sizeof(*hisi_dwc), GFP_KERNEL);
	if (!hisi_dwc) {
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, hisi_dwc);
	hisi_dwc->pdev = pdev;
	hisi_dwc->phy_ops = phy_ops;

	hisi_dwc3_dev = hisi_dwc;

	/*
	 * [next] get resources from dts.
	 */
	ret = get_resource(hisi_dwc);
	if (ret) {
		dev_err(&pdev->dev, "get resource failed!\n");
		goto err_set_dwc3_null;
	}

	request_gpios_for_fpga(hisi_dwc);

	hisi_dwc->dma_mask_bit = hisi_dwc->dma_mask_bit > 64 ?
					64 : hisi_dwc->dma_mask_bit;
	dev->coherent_dma_mask = DMA_BIT_MASK(hisi_dwc->dma_mask_bit);
	dev->dma_mask = &dev->coherent_dma_mask;

	/* create sysfs files. */
	ret = create_attr_file(dev);
	if (ret) {
		dev_err(&pdev->dev, "create_attr_file failed!\n");
		goto err_set_dwc3_null;
	}

	/* initialize */
	hisi_dwc->charger_type = CHARGER_TYPE_SDP;
	hisi_dwc->fake_charger_type = CHARGER_TYPE_NONE;
	hisi_dwc->event = NONE_EVENT;
	hisi_dwc->host_flag = 0;
	hisi_dwc->eventmask = 0;
	hisi_dwc->is_hanle_event_sync = 0;
	hisi_dwc->mode_type = TCPC_NC;
	spin_lock_init(&hisi_dwc->event_lock);
	INIT_WORK(&hisi_dwc->event_work, event_work);
	INIT_WORK(&hisi_dwc->speed_change_work, hisi_dwc3_speed_change_work);
	mutex_init(&hisi_dwc->lock);
	wake_lock_init(&hisi_dwc->wake_lock, WAKE_LOCK_SUSPEND, "usb_wake_lock");
	BLOCKING_INIT_NOTIFIER_HEAD(&hisi_dwc->charger_type_notifier);
	event_queue_creat(&hisi_dwc->event_queue, MAX_EVENT_COUNT);
	init_completion(&hisi_dwc->event_completion);
	hisi_dwc->xhci_nb.notifier_call = xhci_notifier_fn;
	usb_register_notify(&hisi_dwc->xhci_nb);

#ifdef CONFIG_HISI_DEBUG_FS
	INIT_WORK(&hisi_dwc->usb_core_reg_dump_work, usb_core_reg_dump_work);
#endif /* hisi debug */

	/* power on */
	hisi_dwc->is_regu_on = 0;
	ret = hisi_dwc3_phy_init(hisi_dwc);
	if (ret) {
		dev_err(&pdev->dev, "%s: hisi_dwc3_phy_init failed!\n", __func__);
		goto err_remove_attr;
	}


	ret = hisi_usb_bc_init(hisi_dwc);
	if (ret) {
		usb_err("hisi_usb_bc_init failed\n");
		goto err_remove_attr;
	}

	/*
	 * enable runtime pm.
	 */
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		usb_err("hisi_dwc3 pm_runtime_get_sync failed %d\n", ret);
		goto err_bc_exit;
	}

	pm_runtime_forbid(dev);

	/*
	 * probe child deivces
	 */
	ret = of_platform_populate(node, NULL, NULL, dev);
	if (ret) {
		usb_err("%s: register dwc3 failed!\n", __func__);
		goto err_pm_put;
	}

#ifdef CONFIG_USB_DWC3_OTG
	/* default device state  */
	hisi_dwc->state = USB_STATE_DEVICE;

	/* stop peripheral */
	ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_VBUS_CLEAR);
	if (ret) {
		usb_err("stop peripheral error\n");
		goto err_pm_put;
	}

	if (hisi_dwc->fpga_flag != 0) {
                /* if vbus is on, detect charger type */
                if (hisi_usb_vbus_value()) {
                        hisi_dwc->charger_type = detect_charger_type(hisi_dwc);
                        notify_charger_type(hisi_dwc);
                }

                if (sleep_allowed(hisi_dwc))
                        hisi_dwc3_wake_unlock(hisi_dwc);
                else
                        hisi_dwc3_wake_lock(hisi_dwc);

                if (enumerate_allowed(hisi_dwc)) {
                        /* start peripheral */
                        ret = dwc3_otg_work(dwc_otg_handler,
                                        DWC3_OTG_EVT_VBUS_SET);
                        if (ret) {
                                hisi_dwc3_wake_unlock(hisi_dwc);
                                usb_err("start peripheral error\n");
                                goto err_pm_put;
                        }
                }

                hisi_dwc->event = CHARGER_CONNECT_EVENT;

                init_state = get_usb_state(hisi_dwc);
                if (init_state == USB_STATE_OFF) {
                        usb_dbg("init state: OFF\n");
                        hisi_usb_otg_event(CHARGER_DISCONNECT_EVENT);
                }
        } else {
                if (!hisi_usb_vbus_value()) {
                        hisi_dwc->charger_type = CHARGER_TYPE_NONE;
                }
                hisi_dwc->state = USB_STATE_OFF;
                hisi_dwc->event = CHARGER_DISCONNECT_EVENT;

                ret = hisi_dwc3_phy_shutdown(hisi_dwc);
                if (ret)
                        usb_err("hisi_dwc3_phy_shutdown failed (ret %d)\n", ret);

                hisi_dwc3_wake_unlock(hisi_dwc);
        }
#endif

	pm_runtime_allow(dev);

	usb_dbg("-\n");

	return 0;

err_pm_put:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);

err_bc_exit:
	hisi_usb_bc_exit(hisi_dwc);

err_remove_attr:
	event_queue_destroy(&hisi_dwc->event_queue);
	remove_attr_file(dev);
err_set_dwc3_null:
	hisi_dwc3_dev = NULL;

	return ret;
}

static int hisi_dwc3_remove_child(struct device *dev, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	if (data)
		usb_dbg("unused data not NULL!\n");
	platform_device_unregister(pdev);
	return 0;
}

int hisi_dwc3_remove(struct platform_device *pdev)
{
	struct hisi_dwc3_device *hisi_dwc3 = platform_get_drvdata(pdev);
	int ret;

	if (!hisi_dwc3) {
		usb_err("hisi_dwc3 NULL\n");
		return -EBUSY;
	}

	device_for_each_child(&pdev->dev, NULL, hisi_dwc3_remove_child);

	hisi_usb_bc_exit(hisi_dwc3);

	usb_unregister_notify(&hisi_dwc3->xhci_nb);
	ret = hisi_dwc3_phy_shutdown(hisi_dwc3);
	if (ret) {
		usb_err("hisi_dwc3_phy_shutdown error\n");
	}
	hisi_dwc3->phy_ops = NULL;

	event_queue_destroy(&hisi_dwc3->event_queue);

	remove_attr_file(&pdev->dev);

	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

#ifdef CONFIG_PM
#ifdef CONFIG_PM_SLEEP
/*lint -save -e454 -e455 */
static int hisi_dwc3_prepare(struct device *dev)
{
	struct hisi_dwc3_device *hisi_dwc = platform_get_drvdata(
				to_platform_device(dev));
	int ret = 0;

	usb_dbg("+\n");

	if (!hisi_dwc)
		return -ENODEV;

	mutex_lock(&hisi_dwc->lock);

	switch (hisi_dwc->state) {
	case USB_STATE_OFF:
		pr_info("%s: off state.\n", __func__);
		break;
	case USB_STATE_DEVICE:
		pr_info("%s: device state.\n", __func__);

		if (enumerate_allowed(hisi_dwc)) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
			/* stop peripheral */
			ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_VBUS_CLEAR);
			if (ret) {
				usb_err("stop peripheral error\n");
				goto error;
			}
#endif
		} else {
			unsigned long flags;
			usb_dbg("connected is a real charger\n");
			spin_lock_irqsave(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
			hisi_bc_disable_vdp_src(hisi_dwc);
			spin_unlock_irqrestore(&hisi_dwc->bc_again_lock, flags);/*lint !e550*/
		}

		break;
	case USB_STATE_HOST:
		usb_dbg("%s: host mode, should not go to sleep!\n", __func__);
		ret = 0;
		break;
	case USB_STATE_HIFI_USB:
	case USB_STATE_HIFI_USB_HIBERNATE:
		break;
	default:
		pr_err("%s: illegal state!\n", __func__);
		ret = -EFAULT;
		goto error;
	}

	usb_dbg("-\n");
	return ret;
error:
	mutex_unlock(&hisi_dwc->lock);
	return ret;
}

static void hisi_dwc3_complete(struct device *dev)
{
	struct hisi_dwc3_device *hisi_dwc = platform_get_drvdata(
				to_platform_device(dev));
	usb_dbg("+\n");

	if (!hisi_dwc) {
		usb_err("hisi_dwc NULL !\n");
		return;
	}

	switch (hisi_dwc->state) {
	case USB_STATE_OFF:
		usb_dbg("%s: off state.\n", __func__);
		break;
	case USB_STATE_DEVICE:
		usb_dbg("%s: device state.charger_type[%d]\n", __func__, hisi_dwc->charger_type);

		if (sleep_allowed(hisi_dwc))
			hisi_dwc3_wake_unlock(hisi_dwc);
		else
			hisi_dwc3_wake_lock(hisi_dwc);

		/* do not start peripheral if real charger connected */
		if (enumerate_allowed(hisi_dwc)) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
			int ret = 0;

			/* start peripheral */
			ret = dwc3_otg_work(dwc_otg_handler, DWC3_OTG_EVT_VBUS_SET);
			if (ret) {
				usb_err("start peripheral error\n");
				break;
			}
#endif
		} else {
			usb_dbg("a real charger connected\n");
		}

		break;
	case USB_STATE_HOST:
		usb_err("%s: host mode, should not go to sleep!\n", __func__);
		break;
	case USB_STATE_HIFI_USB:
	case USB_STATE_HIFI_USB_HIBERNATE:
		/* keep audio usb power on */
		break;
	default:
		usb_err("%s: illegal state!\n", __func__);
		break;
	}

	mutex_unlock(&hisi_dwc->lock);
	usb_dbg("-\n");
}
/*lint -restore */

static int hisi_dwc3_suspend(struct device *dev)
{
	struct hisi_dwc3_device *hisi_dwc3 = platform_get_drvdata(to_platform_device(dev));
	int ret = 0;

	usb_dbg("+\n");

	if (!hisi_dwc3) {
		usb_err("hisi_dwc3 NULL\n");
		return -EBUSY;
	}

	if (USB_STATE_DEVICE == hisi_dwc3->state) {
		ret = hisi_dwc3_phy_shutdown(hisi_dwc3);
		if (ret)
			usb_err("hisi_dwc3_phy_shutdown failed\n");
	} else {
		usb_dbg("hisi_dwc3 in state %s\n",
				hisi_usb_state_string(hisi_dwc3->state));
	}

	usb_dbg("-\n");

	return ret;
}

static int hisi_dwc3_resume(struct device *dev)
{
	struct hisi_dwc3_device *hisi_dwc3 = platform_get_drvdata(to_platform_device(dev));
	int ret = 0;

	usb_dbg("+\n");

	if (!hisi_dwc3) {
		usb_err("hisi_dwc3 NULL\n");
		return -EBUSY;
	}

	if (USB_STATE_DEVICE == hisi_dwc3->state) {
		ret = hisi_dwc3_phy_init(hisi_dwc3);
		if (ret)
			usb_err("hisi_dwc3_phy_init failed\n");

		pm_runtime_disable(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	} else {
		usb_dbg("hisi_dwc3 in state %s\n",
				hisi_usb_state_string(hisi_dwc3->state));
	}

	usb_dbg("-\n");

	return ret;
}
#endif

static int hisi_dwc3_runtime_suspend(struct device *dev)
{
	usb_dbg("+\n");

	return 0;
}

static int hisi_dwc3_runtime_resume(struct device *dev)
{
	usb_dbg("+\n");

	return 0;
}

static int hisi_dwc3_runtime_idle(struct device *dev)
{
	usb_dbg("+\n");

	return 0;
}

const struct dev_pm_ops hisi_dwc3_dev_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	.prepare	= hisi_dwc3_prepare,
	.complete	= hisi_dwc3_complete,
	SET_SYSTEM_SLEEP_PM_OPS(hisi_dwc3_suspend, hisi_dwc3_resume)
#endif
	SET_RUNTIME_PM_OPS(hisi_dwc3_runtime_suspend, hisi_dwc3_runtime_resume,
			hisi_dwc3_runtime_idle)
};
#endif
