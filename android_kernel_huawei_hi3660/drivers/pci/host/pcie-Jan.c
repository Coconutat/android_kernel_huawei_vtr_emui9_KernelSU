#include "pcie-kirin.h"
/*lint -e438 -e550 -e715 -e750 -esym(438,*) -esym(550,*) -esym(715,*) -esym(750,*) */

/* SYSCTRL register */
#define MTCMOS_CTRL_BIT    0x10
#define HW_AUTO_CF_BIT		((0x1 << 20) | (0x1 << 14))
#define IO_HARD_CTRL_DEBOUNCE_BYPASS (0x1 << 28)
#define IO_OE_EN_HARD_BYPASS (0x1 << 29)

#define SCTRL_SCPERCLKEN3	0x1a4
#define SCTRL_SCPWRDIS	0x64
#define SCTRL_SCPWREN	0x60
#define SCTRL_SCPEREN2	0x190
#define SCTRL_SCPERCLKDIS2	0x194

static void kirin_pcie_oe_ctrl(struct kirin_pcie *pcie, int en_flag)
{
	u32 val;

	val = readl(pcie->sctrl_base + SCTRL_SCPERCLKEN3);//lint !e732

	/* set phy_debounce in&out time: 300ns*/
	val |= 0xF0F000;

	/*select oe_gt_mode */
	val |= 0x400;

	/*IO bypass */
	val &= ~IO_HARD_CTRL_DEBOUNCE_BYPASS;

	if (en_flag)
		val &= ~IO_OE_EN_HARD_BYPASS;
	else
		val |= IO_OE_EN_HARD_BYPASS;

	writel(val, pcie->sctrl_base + SCTRL_SCPERCLKEN3);
}

static void kirin_pcie_mtcmos_ctrl(struct kirin_pcie *pcie, int enable)
{
	if (enable)/* [false alarm]:fortify */
		writel(MTCMOS_CTRL_BIT, pcie->sctrl_base + SCTRL_SCPWREN);
	else
		writel(MTCMOS_CTRL_BIT, pcie->sctrl_base + SCTRL_SCPWRDIS);
}

static int kirin_pcie_clk_ctrl(struct kirin_pcie *pcie, int clk_on)
{
	int ret = 0;
	unsigned long ref_clk_rate = REF_CLK_FREQ;

	if (clk_on) {
		ret = clk_set_rate(pcie->phy_ref_clk, ref_clk_rate);
		if (ret) {
			PCIE_PR_ERR("Failed to set rate");
			return ret;
		}

		ret = clk_prepare_enable(pcie->phy_ref_clk);
		if (ret) {
			PCIE_PR_ERR("phy_ref_clk fail");
			return ret;
		}

		ret = clk_prepare_enable(pcie->apb_sys_clk);
		if (ret) {
			PCIE_PR_ERR("apb_sys_clk fail");
			goto REF_CLK;
		}

		ret = clk_prepare_enable(pcie->apb_phy_clk);
		if (ret) {
			PCIE_PR_ERR("apb_phy_clk fail");
			goto APB_SYS_CLK;
		}

		ret = clk_prepare_enable(pcie->pcie_aclk);
		if (ret) {
			PCIE_PR_ERR("pcie_aclk fail");
			goto APB_PHY_CLK;
		}

		ret = clk_prepare_enable(pcie->pcie_aux_clk);
		if (ret) {
			PCIE_PR_ERR("pcie_aux_clk fail");
			goto ACLK;
		}
		goto END;
	}

	clk_disable_unprepare(pcie->pcie_aux_clk);
ACLK:
	clk_disable_unprepare(pcie->pcie_aclk);
APB_PHY_CLK:
	clk_disable_unprepare(pcie->apb_phy_clk);
APB_SYS_CLK:
	clk_disable_unprepare(pcie->apb_sys_clk);
REF_CLK:
	clk_disable_unprepare(pcie->phy_ref_clk);
END:
	return ret;
}

static int kirin_pcie_phy_init(struct kirin_pcie *pcie);

static int kirin_pcie_turn_on(struct pcie_port *pp, enum rc_power_status on_flag)
{
	struct kirin_pcie *pcie = to_kirin_pcie(pp);//lint !e826
	int ret = 0;

	PCIE_PR_DEBUG("+");

	mutex_lock(&pcie->power_lock);

	if (atomic_read(&(pcie->is_power_on))) {
		PCIE_PR_INFO("PCIe%d already power on", pcie->rc_id);
		goto MUTEX_UNLOCK;
	}

	if (on_flag == RC_POWER_ON) {
		kirin_pcie_mtcmos_ctrl(pcie, ENABLE);
		udelay((unsigned long)100);//lint !e778  !e774  !e516
	}

	kirin_pcie_oe_ctrl(pcie, ENABLE);

		/*clk enable*/
	ret = kirin_pcie_clk_ctrl(pcie, ENABLE);
	udelay((unsigned long)1);//lint !e778  !e774  !e516
	if (ret)
		goto MTCMOS_CLOSE;

	/*clk disable*/
	kirin_pcie_clk_ctrl(pcie, DISABLE);
	udelay((unsigned long)1);//lint !e778  !e774  !e516

	/*ISO disable*/
	if (on_flag == RC_POWER_ON)
		kirin_pcie_iso_ctrl(pcie, DISABLE);

	/*unset module*/
	kirin_pcie_reset_ctrl(pcie, RST_DISABLE);

	/*clk on*/
	ret = kirin_pcie_clk_ctrl(pcie, ENABLE);
	if (ret)
		goto RESET_PCIE;
	PCIE_PR_DEBUG("clk on Done ");

	/*Enable 100MHz gate*/
	writel(HW_AUTO_CF_BIT, pcie->sctrl_base + SCTRL_SCPEREN2);
	ret = kirin_pcie_phy_init(pcie);
	if (ret) {
		PCIE_PR_ERR("Phy Init Failed ");
		goto CLK_CLOSE;
	}

	atomic_set(&(pcie->is_power_on), 1);

	mdelay((unsigned long)22);//lint !e778  !e774  !e516  !e747  !e845
	ret = gpio_direction_output((unsigned int)pcie->gpio_id_reset, 1);
	if (ret) {
		PCIE_PR_ERR("perst deassert fail");
		goto CLK_CLOSE;
	}
	mdelay((unsigned long)7);//lint !e778  !e774  !e516  !e747  !e845

	PCIE_PR_DEBUG("-");
	goto MUTEX_UNLOCK;

CLK_CLOSE:
	kirin_pcie_clk_ctrl(pcie, DISABLE);
RESET_PCIE:
	kirin_pcie_reset_ctrl(pcie, RST_ENABLE);
	kirin_pcie_iso_ctrl(pcie, ENABLE);
MTCMOS_CLOSE:
	kirin_pcie_mtcmos_ctrl(pcie, DISABLE);
MUTEX_UNLOCK:
	mutex_unlock(&pcie->power_lock);
	return ret;
}

static int kirin_pcie_turn_off(struct pcie_port *pp, enum rc_power_status on_flag)
{
	struct kirin_pcie *pcie = to_kirin_pcie(pp);//lint !e826
	int ret = 0;

	PCIE_PR_DEBUG("+");
	mutex_lock(&pcie->power_lock);

	if (!atomic_read(&(pcie->is_power_on))) {
		PCIE_PR_INFO("PCIe%d already power off", pcie->rc_id);
		goto MUTEX_UNLOCK;
	}

	atomic_set(&(pcie->is_power_on), 0);

	/*perst assert*/
	ret = gpio_direction_output((unsigned int)pcie->gpio_id_reset, 0); 
	if (ret) {
		PCIE_PR_ERR("perst assert fail");
		goto MUTEX_UNLOCK;
	}
	mdelay((unsigned long)10);//lint !e778  !e774  !e516  !e747  !e845

	kirin_pcie_reset_ctrl(pcie, RST_ENABLE);

	kirin_pcie_clk_ctrl(pcie, DISABLE);

	if (on_flag == RC_POWER_OFF) {
		kirin_pcie_iso_ctrl(pcie, ENABLE);
		kirin_pcie_mtcmos_ctrl(pcie, DISABLE);
	}

	kirin_pcie_oe_ctrl(pcie, DISABLE);
	/*Close 100MHz Gate*/
	writel(HW_AUTO_CF_BIT, pcie->sctrl_base + SCTRL_SCPERCLKDIS2);

	PCIE_PR_DEBUG("-");

MUTEX_UNLOCK:
	mutex_unlock(&pcie->power_lock);
	return ret;
}

static int kirin_pcie_phy_init(struct kirin_pcie *pcie)
{
	u32 reg_val;

	/*choose 100MHz clk src: Bit[8]==1 pll, Bit[8]==0 Board */
	reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL1_ADDR);
	reg_val &= ~(0x1 << 8);
	kirin_apb_phy_writel(pcie, reg_val, SOC_PCIEPHY_CTRL1_ADDR);

	/*pull down phy_test_powerdown signal */
	reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL0_ADDR);
	reg_val &= ~(0x1 << 22);
	kirin_apb_phy_writel(pcie, reg_val, SOC_PCIEPHY_CTRL0_ADDR);
	udelay((unsigned long)10);//lint !e778  !e774  !e516

	/*derst PHY */
	reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL1_ADDR);
	reg_val &= ~(0x1 << 16);
	kirin_apb_phy_writel(pcie, reg_val, SOC_PCIEPHY_CTRL1_ADDR);

	if (!is_pipe_clk_stable(pcie)) {
		PCIE_PR_INFO("PIPE clk is not stable");
		return -EINVAL;
	}

	return 0;
}

struct pcie_platform_ops plat_ops = {
	.sram_ext_load = NULL,
	.cal_alg_adjust = NULL,
	.plat_on = kirin_pcie_turn_on,
	.plat_off = kirin_pcie_turn_off,
};

int pcie_plat_init(struct platform_device *pdev, struct kirin_pcie *pcie)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
	if (!np) {
		PCIE_PR_ERR("Failed to get crgctrl node ");
		return -1;
	}
	pcie->crg_base = of_iomap(np, 0);
	if (!pcie->crg_base) {
		PCIE_PR_ERR("Failed to iomap crg_base iomap");
		return -1;
	}

	pcie->phy_ref_clk = devm_clk_get(&pdev->dev, "pcie_phy_ref");
	if (IS_ERR(pcie->phy_ref_clk)) {
		PCIE_PR_ERR("Failed to get pcie_phy_ref clock");
		return PTR_ERR(pcie->phy_ref_clk);
	}

	pcie->plat_ops = &plat_ops;

	return 0;
}

/*lint -e438 -e550 -e715 -e750 +esym(438,*) +esym(550,*) +esym(715,*) +esym(750,*) */

