#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/bitops.h>

#include "dwc3-hisi.h"
extern struct hisi_dwc3_device *hisi_dwc3_dev;

void hisi_bc_dplus_pulldown(struct hisi_dwc3_device *hisi_dwc)
{
	void __iomem *base = hisi_dwc->otg_bc_reg_base;
	volatile u32 reg;

	usb_dbg("+\n");

	/* enable BC */
	writel(BC_CTRL1_BC_MODE, base + BC_CTRL1);
	reg = readl(base + BC_CTRL0);
	reg |= ((1u << 7) | (1u << 8));
	writel(reg, base + BC_CTRL0);

	usb_dbg("-\n");
}

void hisi_bc_dplus_pullup(struct hisi_dwc3_device *hisi_dwc)
{
	void __iomem *base = hisi_dwc->otg_bc_reg_base;
	volatile u32 reg;

	usb_dbg("+\n");

	reg = readl(base + BC_CTRL0);
	reg &= (~((1u << 7) | (1u << 8)));
	writel(reg, base + BC_CTRL0);
	/* disable BC */
	writel((readl(base + BC_CTRL1) & ~BC_CTRL1_BC_MODE), base + BC_CTRL1);

	usb_dbg("-\n");
}

/* BC1.2 Spec:
 * If a PD detects that D+ is greater than VDAT_REF, it knows that it is
 * attached to a DCP. It is then required to enable VDP_SRC or pull D+
 * to VDP_UP through RDP_UP */
void hisi_bc_disable_vdp_src(struct hisi_dwc3_device *hisi_dwc3)
{
	void __iomem *base;
	void __iomem *bc_ctrl2;

	uint32_t reg;

	usb_dbg("+\n");

	if (hisi_dwc3->vdp_src_enable == 0)
		return;
	hisi_dwc3->vdp_src_enable = 0;

	base = hisi_dwc3->otg_bc_reg_base;
	bc_ctrl2 = hisi_dwc3->bc_ctrl_reg;
	usb_dbg("diaable VDP_SRC\n");

	reg = readl(bc_ctrl2);
	reg &= ~(BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB);
	writel(reg, bc_ctrl2);

	reg = readl(base + BC_CTRL0);
	reg |= BC_CTRL0_BC_SUSPEND_N;
	writel(reg, base + BC_CTRL0);

	writel((readl(base + BC_CTRL1) & ~BC_CTRL1_BC_MODE), base + BC_CTRL1);
	usb_dbg("-\n");
}

void hisi_bc_enable_vdp_src(struct hisi_dwc3_device *hisi_dwc3)
{
	void __iomem *bc_ctrl2;
	uint32_t reg;

	usb_dbg("+\n");

	if (hisi_dwc3->vdp_src_enable != 0)
		return;
	hisi_dwc3->vdp_src_enable = 1;

	bc_ctrl2 = hisi_dwc3->bc_ctrl_reg;
	usb_dbg("enable VDP_SRC\n");
	reg = readl(bc_ctrl2);
	reg &= ~BC_CTRL2_BC_PHY_CHRGSEL;
	reg |= (BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB);
	writel(reg, bc_ctrl2);
	usb_dbg("-\n");
}

static int is_dcd_timeout(void __iomem *base)
{
	unsigned long jiffies_expire;
	uint32_t reg;
	int ret = 0;
	int i = 0;

	jiffies_expire = jiffies + msecs_to_jiffies(900);
	msleep(50);
	while (1) {
		reg = readl(base + BC_STS0);
		if ((reg & BC_STS0_BC_PHY_FSVPLUS) == 0) {
			i++;
			if (i >= 10)
				break;
		} else {
			i = 0;
		}

		msleep(10);

		if (time_after(jiffies, jiffies_expire)) {
			usb_dbg("DCD timeout!\n");
			ret = -1;
			break;
		}
	}

	return ret;
}

enum hisi_charger_type detect_charger_type(struct hisi_dwc3_device *hisi_dwc3)
{
	enum hisi_charger_type type = CHARGER_TYPE_NONE;
	void __iomem *base = hisi_dwc3->otg_bc_reg_base;
	void __iomem *bc_ctrl2;
	uint32_t reg;
	unsigned long flags;

	usb_dbg("+\n");

	if (hisi_dwc3->fpga_flag) {
		usb_dbg("this is fpga platform, charger is SDP\n");
		return CHARGER_TYPE_SDP;
	}

	if (hisi_dwc3->fake_charger_type != CHARGER_TYPE_NONE) {
		usb_dbg("fake type: %d\n", hisi_dwc3->fake_charger_type);
		return hisi_dwc3->fake_charger_type;
	}

	bc_ctrl2 = hisi_dwc3->bc_ctrl_reg;
	writel(BC_CTRL1_BC_MODE, base + BC_CTRL1);

	/* phy suspend */
	reg = readl(base + BC_CTRL0);
	reg &= ~BC_CTRL0_BC_SUSPEND_N;
	writel(reg, base + BC_CTRL0);

	spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
	/* enable DCD */
	reg = readl(bc_ctrl2);
	reg |= BC_CTRL2_BC_PHY_DCDENB;
	writel(reg, bc_ctrl2);
	spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/

	reg = readl(base + BC_CTRL0);
	reg |= BC_CTRL0_BC_DMPULLDOWN;
	writel(reg, base + BC_CTRL0);

	if (is_dcd_timeout(base)){
		type = CHARGER_TYPE_UNKNOWN;
	}

	reg = readl(base + BC_CTRL0);
	reg &= ~BC_CTRL0_BC_DMPULLDOWN;
	writel(reg, base + BC_CTRL0);

	spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
	/* disable DCD */
	reg = readl(bc_ctrl2);
	reg &= ~BC_CTRL2_BC_PHY_DCDENB;
	writel(reg, bc_ctrl2);

	usb_dbg("DCD done\n");

	if (type == CHARGER_TYPE_NONE) {
		/* enable vdect */
		reg = readl(bc_ctrl2);
		reg &= ~BC_CTRL2_BC_PHY_CHRGSEL;
		reg |= (BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB);
		writel(reg, bc_ctrl2);
		spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/

		msleep(40);

		/* we can detect sdp or cdp dcp */
		reg = readl(base + BC_STS0);
		if ((reg & BC_STS0_BC_PHY_CHGDET) == 0) {
			type = CHARGER_TYPE_SDP;
		}

		spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
		/* disable vdect */
		reg = readl(bc_ctrl2);
		reg &= ~(BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB);
		writel(reg, bc_ctrl2);
	}

	usb_dbg("Primary Detection done\n");

	if (type == CHARGER_TYPE_NONE) {
		/* enable vdect */
		reg = readl(bc_ctrl2);
		reg |= (BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB
				| BC_CTRL2_BC_PHY_CHRGSEL);
		writel(reg, bc_ctrl2);
		spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/

		msleep(40);

		/* we can detect sdp or cdp dcp */
		reg = readl(base + BC_STS0);
		if ((reg & BC_STS0_BC_PHY_CHGDET) == 0)
			type = CHARGER_TYPE_CDP;
		else
			type = CHARGER_TYPE_DCP;

		spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
		/* disable vdect */
		reg = readl(bc_ctrl2);
		reg &= ~(BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB
				| BC_CTRL2_BC_PHY_CHRGSEL);
		writel(reg, bc_ctrl2);
	}
	spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/

	usb_dbg("Secondary Detection done\n");

	/* If a PD detects that D+ is greater than VDAT_REF, it knows that it is
	 * attached to a DCP. It is then required to enable VDP_SRC or pull D+
	 * to VDP_UP through RDP_UP */
	if (type == CHARGER_TYPE_DCP) {
		usb_dbg("charger is DCP, enable VDP_SRC\n");
		spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
		hisi_bc_enable_vdp_src(hisi_dwc3);
		spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
	} else {
		/* bc_suspend = 1, nomal mode */
		reg = readl(base + BC_CTRL0);
		reg |= BC_CTRL0_BC_SUSPEND_N;
		writel(reg, base + BC_CTRL0);

		msleep(10);

		/* disable BC */
		writel((readl(base + BC_CTRL1) & ~BC_CTRL1_BC_MODE), base + BC_CTRL1);
	}

	if (type == CHARGER_TYPE_CDP) {
		usb_dbg("it needs enable VDP_SRC while detect CDP!\n");
		spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
		hisi_bc_enable_vdp_src(hisi_dwc3);
		spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
	}

	usb_dbg("charger type: %s\n", charger_type_string(type));
	usb_dbg("-\n");
	return type;
}

