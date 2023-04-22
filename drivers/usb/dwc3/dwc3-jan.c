#include <linux/module.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <pmic_interface.h>
#include <linux/mfd/hisi_pmic.h>
#include "dwc3-hisi.h"
#include <huawei_platform/power/huawei_charger.h>

/*lint -e750 -esym(750,*)*/
/* clk module will round to 228M */
#define USB3OTG_ACLK_FREQ		229000000

#define SCTRL_SCDEEPSLEEPED				0x08
#define USB_REFCLK_ISO_EN               (1 << 25)
#define PCTRL_PERI_CTRL3                0x10
#define USB_TCXO_EN						(1 << 1)
#define PERI_CTRL3_MSK_START            (16)
#define SC_CLK_USB3PHY_3MUX1_SEL        (1 << 25)

#define SC_SEL_ABB_BACKUP               (1 << 8)
#define CLKDIV_MASK_START               (16)

#define PERI_CRG_CLKDIV21               0xFC

#define GT_CLK_ABB_BACKUP               (1 << 22)
#define PERI_CRG_CLK_DIS5               0x54

#define PMC_PPLL3CTRL0                  0x048
#define PPLL3_FBDIV_START               (8)
#define PPLL3_EN                        (1 << 0)
#define PPLL3_BP                        (1 << 1)
#define PPLL3_LOCK                      (1 << 26)

#define PMC_PPLL3CTRL1                  0x04C
#define PPLL3_INT_MOD                   (1 << 24)
#define GT_CLK_PPLL3                    (1 << 26)

#define PERI_CRG_CLK_EN5                0x50

#define SC_USB3PHY_ABB_GT_EN            (1 << 15)
#define REF_SSP_EN                      (1 << 16)

#define GT_CLK_USB3OTG_REF				(1 << 0)
#define GT_ACLK_USB3OTG					(1 << 1)
/*lint -e750 +esym(750,*)*/

/*
 * hisi dwc3 phy registers
 */
#define DWC3_PHY_RX_OVRD_IN_HI	0x1006
#define DWC3_PHY_RX_SCOPE_VDCC	0x1026

/* DWC3_PHY_RX_SCOPE_VDCC */
#define RX_SCOPE_LFPS_EN	(1 << 0)

/* USB3PHY_CR_STS */
#define USB3OTG_PHY_CR_DATA_OUT(x)	(((x) >> 1) & 0xffff)
#define USB3OTG_PHY_CR_ACK		(1 << 0)

/* USB3PHY_CR_CTRL */
#define USB3OTG_PHY_CR_DATA_IN(x)	(((x) << 4) & (0xffff << 4))
#define USB3OTG_PHY_CR_WRITE		(1 << 3)
#define USB3OTG_PHY_CR_READ		(1 << 2)
#define USB3OTG_PHY_CR_CAP_DATA		(1 << 1)
#define USB3OTG_PHY_CR_CAP_ADDR		(1 << 0)

#define IP_RST_USB3OTG_MUX				(1 << 8)
#define IP_RST_USB3OTG_AHBIF				(1 << 7)
#define IP_RST_USB3OTG_32K				(1 << 6)
#define IP_RST_USB3OTG					(1 << 5)
#define IP_RST_USB3OTGPHY_POR				(1 << 3)

extern struct hisi_dwc3_device *hisi_dwc3_dev;

static void phy_cr_wait_ack(void __iomem *otg_bc_base)
{
	int i = 1000;

	while (1) {
		if ((readl(otg_bc_base + USB3PHY_CR_STS) & USB3OTG_PHY_CR_ACK) == 1)
			break;
		udelay(50);
		if (i-- < 0) {
			usb_err("wait phy_cr_ack timeout!\n");
			break;
		}
	}
}

static void phy_cr_set_addr(void __iomem *otg_bc_base, u32 addr)
{
	u32 reg;

	/* set addr */
	reg = USB3OTG_PHY_CR_DATA_IN(addr);
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	udelay(100);

	/* cap addr */
	reg = readl(otg_bc_base + USB3PHY_CR_CTRL);
	reg |= USB3OTG_PHY_CR_CAP_ADDR;
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	phy_cr_wait_ack(otg_bc_base);

	/* clear ctrl reg */
	writel(0, otg_bc_base + USB3PHY_CR_CTRL);
}

static u16 phy_cr_read(void __iomem *otg_bc_base, u32 addr)
{
	u32 reg;
	int i = 1000;

	phy_cr_set_addr(otg_bc_base, addr);

	/* read cap */
	writel(USB3OTG_PHY_CR_READ, otg_bc_base + USB3PHY_CR_CTRL);

	udelay(100);

	while (1) {
		reg = readl(otg_bc_base + USB3PHY_CR_STS);
		if ((reg & USB3OTG_PHY_CR_ACK) == 1) {
			break;
		}
		udelay(50);
		if (i-- < 0) {
			usb_err("wait phy_cr_ack timeout!\n");
			break;
		}
	}

	/* clear ctrl reg */
	writel(0, otg_bc_base + USB3PHY_CR_CTRL);

	return (u16)USB3OTG_PHY_CR_DATA_OUT(reg);
}

static void phy_cr_write(void __iomem *otg_bc_base, u32 addr, u32 value)
{
	u32 reg;

	phy_cr_set_addr(otg_bc_base, addr);

	reg = USB3OTG_PHY_CR_DATA_IN(value);
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	/* cap data */
	reg = readl(otg_bc_base + USB3PHY_CR_CTRL);
	reg |= USB3OTG_PHY_CR_CAP_DATA;
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	/* wait ack */
	phy_cr_wait_ack(otg_bc_base);

	/* clear ctrl reg */
	writel(0, otg_bc_base + USB3PHY_CR_CTRL);

	reg = USB3OTG_PHY_CR_WRITE;
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	/* wait ack */
	phy_cr_wait_ack(otg_bc_base);
}

static void config_femtophy_param(struct hisi_dwc3_device *hisi_dwc)
{
	uint32_t reg;
	void __iomem *otg_bc_base = hisi_dwc->otg_bc_reg_base;

	if (hisi_dwc->fpga_flag != 0)
		return;

	/* set high speed phy parameter */
	if (hisi_dwc->host_flag) {
		writel(hisi_dwc->eye_diagram_host_param, otg_bc_base + USBOTG3_CTRL4);
		usb_dbg("set hs phy param 0x%x for host\n",
				readl(otg_bc_base + USBOTG3_CTRL4));
	} else {
		writel(hisi_dwc->eye_diagram_param, otg_bc_base + USBOTG3_CTRL4);
		usb_dbg("set hs phy param 0x%x for device\n",
				readl(otg_bc_base + USBOTG3_CTRL4));
	}

	/* set usb3 phy cr config for usb3.0 */

	if (hisi_dwc->host_flag) {
		phy_cr_write(otg_bc_base, DWC3_PHY_RX_OVRD_IN_HI,
				hisi_dwc->usb3_phy_host_cr_param);
	} else {
		phy_cr_write(otg_bc_base, DWC3_PHY_RX_OVRD_IN_HI,
				hisi_dwc->usb3_phy_cr_param);
	}

	usb_dbg("set ss phy rx equalization 0x%x\n",
			phy_cr_read(otg_bc_base, DWC3_PHY_RX_OVRD_IN_HI));

	/* enable RX_SCOPE_LFPS_EN for usb3.0 */
	reg = phy_cr_read(otg_bc_base, DWC3_PHY_RX_SCOPE_VDCC);
	reg |= RX_SCOPE_LFPS_EN;
	phy_cr_write(otg_bc_base, DWC3_PHY_RX_SCOPE_VDCC, reg);

	usb_dbg("set ss RX_SCOPE_VDCC 0x%x\n",
			phy_cr_read(otg_bc_base, DWC3_PHY_RX_SCOPE_VDCC));

	reg = readl(otg_bc_base + USBOTG3_CTRL6);
	reg &= ~TX_VBOOST_LVL_MASK;
	reg |= TX_VBOOST_LVL(hisi_dwc->usb3_phy_tx_vboost_lvl);
	writel(reg, otg_bc_base + USBOTG3_CTRL6);
	usb_dbg("set ss phy tx vboost lvl 0x%x\n", readl(otg_bc_base + USBOTG3_CTRL6));
}

void set_usb3_phy_cr_param(u32 addr, u32 value)
{
	if (!hisi_dwc3_dev) {
		pr_err("hisi dwc3 device not ready!\n");
		return;
	}

	phy_cr_write(hisi_dwc3_dev->otg_bc_reg_base, addr, value);
}
EXPORT_SYMBOL_GPL(set_usb3_phy_cr_param);

void read_usb3_phy_cr_param(u32 addr)
{
	if (!hisi_dwc3_dev) {
		pr_err("hisi dwc3 device not ready!\n");
		return;
	}

	usb_dbg("read usb3 phy cr param 0x%x\n",
		phy_cr_read(hisi_dwc3_dev->otg_bc_reg_base, addr));
}
EXPORT_SYMBOL_GPL(read_usb3_phy_cr_param);


static int usb3_regu_init(struct hisi_dwc3_device *hisi_dwc3)
{
	int ret;

	if (0 != hisi_dwc3->is_regu_on) {
		usb_dbg("ldo already opened!\n");
		return 0;
	}

	if (0 == hisi_dwc3->fpga_flag) {
		ret = regulator_set_mode(hisi_dwc3->usb_regu, REGULATOR_MODE_NORMAL);
		if (ret) {
			usb_err("set usb regu to normal mode failed (ret %d)\n", ret);
			return ret;
		}
	}

	hisi_dwc3->is_regu_on = 1;

	return 0;
}

static int usb3_regu_shutdown(struct hisi_dwc3_device *hisi_dwc3)
{
	int ret;

	if (0 == hisi_dwc3->is_regu_on) {
		usb_dbg("regu already closed!\n");
		return 0;
	}

	if (0 == hisi_dwc3->fpga_flag) {
		ret = regulator_set_mode(hisi_dwc3->usb_regu, REGULATOR_MODE_IDLE);
		if (ret) {
			usb_err("set usb regu to idle mode failed (ret %d)\n", ret);
			return ret;
		}
	}

	hisi_dwc3->is_regu_on = 0;

	return 0;
}

static int usb3_clk_init(struct hisi_dwc3_device *hisi_dwc3)
{
	int ret;
	uint32_t temp;
	void __iomem *pctrl_base = hisi_dwc3->pctrl_reg_base;
	void __iomem *pericfg_base = hisi_dwc3->pericfg_reg_base;

	/* set usb aclk 240MHz to improve performance */
	ret = clk_set_rate(hisi_dwc3->gt_aclk_usb3otg, USB3OTG_ACLK_FREQ);
	if (ret) {
		usb_err("usb aclk set rate failed\n");
	}

	ret = clk_prepare_enable(hisi_dwc3->gt_aclk_usb3otg);
	if (ret) {
		usb_err("clk_prepare_enable gt_aclk_usb3otg failed\n");
		return ret;
	}

	/* usb refclk iso enable */
	writel(USB_REFCLK_ISO_EN, pericfg_base + PERI_CRG_ISODIS);

	/* enable usb_tcxo_en */
	writel(USB_TCXO_EN | (USB_TCXO_EN << PERI_CTRL3_MSK_START),
			pctrl_base + PCTRL_PERI_CTRL3);

	/* select usbphy clk from abb */
	temp = readl(pctrl_base + PCTRL_PERI_CTRL24);
	temp &= ~SC_CLK_USB3PHY_3MUX1_SEL;
	writel(temp, pctrl_base + PCTRL_PERI_CTRL24);

	/* open clk gate */
	writel(GT_CLK_USB3OTG_REF | GT_ACLK_USB3OTG,
			pericfg_base + PERI_CRG_CLK_EN4);


	ret = clk_prepare_enable(hisi_dwc3->clk);
	if (ret) {
		usb_err("clk_prepare_enable clk failed\n");
		return ret;
	}

	return 0;
}

static void usb3_clk_shutdown(struct hisi_dwc3_device *hisi_dwc3)
{
	uint32_t temp;
	void __iomem *pctrl_base = hisi_dwc3->pctrl_reg_base;
	void __iomem *pericfg_base = hisi_dwc3->pericfg_reg_base;

	writel(GT_CLK_USB3OTG_REF | GT_ACLK_USB3OTG,
			pericfg_base + PERI_CRG_CLK_DIS4);

	temp = readl(pctrl_base + PCTRL_PERI_CTRL24);
	temp &= ~SC_CLK_USB3PHY_3MUX1_SEL;
	writel(temp, pctrl_base + PCTRL_PERI_CTRL24);

	/* disable usb_tcxo_en */
	writel(0 | (USB_TCXO_EN << PERI_CTRL3_MSK_START),
			pctrl_base + PCTRL_PERI_CTRL3);

	clk_disable_unprepare(hisi_dwc3->clk);
	clk_disable_unprepare(hisi_dwc3->gt_aclk_usb3otg);

	msleep(10);
}

static void dwc3_release(struct hisi_dwc3_device *hisi_dwc3)
{
	uint32_t temp;
	void __iomem *pericfg_base = hisi_dwc3->pericfg_reg_base;
	void __iomem *otg_bc_base = hisi_dwc3->otg_bc_reg_base;

	/* dis-reset the module */
	writel(IP_RST_USB3OTG_MUX | IP_RST_USB3OTG_AHBIF | IP_RST_USB3OTG_32K,
			pericfg_base + PERI_CRG_RSTDIS4);

	/* reset phy */
	writel(IP_RST_USB3OTGPHY_POR | IP_RST_USB3OTG, pericfg_base + PERI_CRG_RSTEN4);

	/* enable phy ref clk */
	temp = readl(otg_bc_base + USBOTG3_CTRL0);
	temp |= SC_USB3PHY_ABB_GT_EN;
	writel(temp, otg_bc_base + USBOTG3_CTRL0);

	temp = readl(otg_bc_base + USBOTG3_CTRL7);
	temp |= REF_SSP_EN;
	writel(temp, otg_bc_base + USBOTG3_CTRL7);

	/* exit from IDDQ mode */
	temp = readl(otg_bc_base + USBOTG3_CTRL2);
	temp &= ~(USBOTG3CTRL2_POWERDOWN_HSP | USBOTG3CTRL2_POWERDOWN_SSP);
	writel(temp, otg_bc_base + USBOTG3_CTRL2);

	udelay(100);

	/* dis-reset phy */
	writel(IP_RST_USB3OTGPHY_POR, pericfg_base + PERI_CRG_RSTDIS4);

	/* dis-reset controller */
	writel(IP_RST_USB3OTG, pericfg_base + PERI_CRG_RSTDIS4);

	msleep(10);

	/* fake vbus valid signal */
	temp = readl(otg_bc_base + USBOTG3_CTRL3);
	temp |= (USBOTG3_CTRL3_VBUSVLDEXT | USBOTG3_CTRL3_VBUSVLDEXTSEL);
	writel(temp, otg_bc_base + USBOTG3_CTRL3);

	udelay(100);
}

static void dwc3_reset(struct hisi_dwc3_device *hisi_dwc3)
{
	void __iomem *pericfg_base = hisi_dwc3->pericfg_reg_base;

	writel(IP_RST_USB3OTG, pericfg_base + PERI_CRG_RSTEN4);
	writel(IP_RST_USB3OTGPHY_POR, pericfg_base + PERI_CRG_RSTEN4);
	writel(IP_RST_USB3OTG_MUX | IP_RST_USB3OTG_AHBIF | IP_RST_USB3OTG_32K,
			pericfg_base + PERI_CRG_RSTEN4);
}

static int jan_usb3phy_init(struct hisi_dwc3_device *hisi_dwc3)
{
	int ret;

	usb_dbg("+\n");

	ret = usb3_regu_init(hisi_dwc3);
	if (ret)
		return ret;

	ret = usb3_clk_init(hisi_dwc3);
	if (ret)
		return ret;

	dwc3_release(hisi_dwc3);
	config_femtophy_param(hisi_dwc3);

	set_hisi_dwc3_power_flag(1);

	usb_dbg("-\n");

	return 0;
}

static int jan_usb3phy_shutdown(struct hisi_dwc3_device *hisi_dwc3)
{
	int ret;

	usb_dbg("+\n");

	set_hisi_dwc3_power_flag(0);

	dwc3_reset(hisi_dwc3);
	usb3_clk_shutdown(hisi_dwc3);

	ret = usb3_regu_shutdown(hisi_dwc3);
	if (ret)
		return ret;

	usb_dbg("-\n");

	return 0;
}

static int jan_get_dts_resource(struct hisi_dwc3_device *hisi_dwc3)
{
	struct device *dev = &hisi_dwc3->pdev->dev;

	/* get abb clk handler */
	hisi_dwc3->clk = devm_clk_get(dev, "clk_usb3phy_ref");
	if (IS_ERR_OR_NULL(hisi_dwc3->clk)) {
		dev_err(dev, "get usb3phy ref clk failed\n");
		return -EINVAL;
	}

	/* get a clk handler */
	hisi_dwc3->gt_aclk_usb3otg = devm_clk_get(dev, "aclk_usb3otg");
	if (IS_ERR_OR_NULL(hisi_dwc3->gt_aclk_usb3otg)) {
		dev_err(dev, "get aclk_usb3otg failed\n");
		return -EINVAL;
	}

	if (hisi_dwc3->fpga_flag == 0) {
		hisi_dwc3->usb_regu = devm_regulator_get(dev, "usbldo10");
		if (IS_ERR(hisi_dwc3->usb_regu)) {
			dev_err(dev, "couldn't get regulator usb regu\n");
			return -EINVAL;
		}
	}

	if (of_property_read_u32(dev->of_node, "usb_support_check_voltage", &(hisi_dwc3->check_voltage))) {
		dev_err(dev, "usb driver not support check voltage\n");
		hisi_dwc3->check_voltage = 0;
	}

	return 0;
}

static void jan_check_voltage(struct hisi_dwc3_device *hisi_dwc)
{
	usb_dbg("+\n");

	if (hisi_dwc->check_voltage) {
		/*first dplus pulldown*/
		hisi_bc_dplus_pulldown(hisi_dwc);
		/*second call charger's API to check voltage */
		water_detect();
		/*third dplus pullup*/
		hisi_bc_dplus_pullup(hisi_dwc);
	}

	usb_dbg("-\n");
}

static struct usb3_phy_ops jan_phy_ops = {
	.init		= jan_usb3phy_init,
	.shutdown	= jan_usb3phy_shutdown,
	.get_dts_resource = jan_get_dts_resource,
	.check_voltage = jan_check_voltage,
};

static int dwc3_jan_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = hisi_dwc3_probe(pdev, &jan_phy_ops);
	if (ret)
		usb_err("probe failed, ret=[%d]\n", ret);

	return ret;
}

static int dwc3_jan_remove(struct platform_device *pdev)
{
	int ret = 0;

	ret = hisi_dwc3_remove(pdev);
	if (ret)
		usb_err("hisi_dwc3_remove failed, ret=[%d]\n", ret);

	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id dwc3_jan_match[] = {
	{ .compatible = "hisilicon,jan-dwc3" },
	{},
};
MODULE_DEVICE_TABLE(of, dwc3_jan_match);
#else
#define dwc3_jan_match NULL
#endif

static struct platform_driver dwc3_jan_driver = {
	.probe		= dwc3_jan_probe,
	.remove		= dwc3_jan_remove,
	.driver		= {
		.name	= "usb3-jan",
		.of_match_table = of_match_ptr(dwc3_jan_match),
		.pm	= HISI_DWC3_PM_OPS,
	},
};

module_platform_driver(dwc3_jan_driver);
MODULE_LICENSE("GPL");
