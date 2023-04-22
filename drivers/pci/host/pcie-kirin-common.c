#include "pcie-kirin-common.h"

/*lint -e618 -e647 -e679 -e826 -e648 -e438 -e550 -e713 -e732 -e737 -e774 -e838 -esym(618,*) -esym(647,*) -esym(679,*) -esym(826,*) -esym(648,*) -esym(438,*) -esym(550,*) -esym(713,*) -esym(732,*) -esym(737,*) -esym(774,*) -esym(838,*) */

/**
 * config_enable_dbi - make it possible to access the rc configuration registers in the CDM,
 * or the ep configuration registers.
 * @flag: If flag equals 0, you can access the ep configuration registers in the CDM;
 *  If not, you can access the rc configuration registers in the CDM.
 */

int kirin_pcie_valid(u32 rc_id)
{
	if (rc_id >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_id);
		return 0;
	}

	if (!atomic_read(&(g_kirin_pcie[rc_id].is_power_on))) {
		PCIE_PR_ERR("PCIe%d is power off ", rc_id);
		return 0;
	}

	return -1;
}

int config_enable_dbi(u32 rc_id, int flag)
{
	u32 ret1;
	u32 ret2;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	ret1 = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL0_ADDR);
	ret2 = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL1_ADDR);
	if (flag) {
		ret1 = ret1 | PCIE_ELBI_SLV_DBI_ENABLE;
		ret2 = ret2 | PCIE_ELBI_SLV_DBI_ENABLE;
	} else {
		ret1 = ret1 & (~PCIE_ELBI_SLV_DBI_ENABLE);
		ret2 = ret2 & (~PCIE_ELBI_SLV_DBI_ENABLE);
	}
	kirin_elb_writel(pcie, ret1, SOC_PCIECTRL_CTRL0_ADDR);
	kirin_elb_writel(pcie, ret2, SOC_PCIECTRL_CTRL1_ADDR);

	udelay(10);//lint !e778  !e774  !e516 !e747
	ret1 = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL0_ADDR);
	ret2 = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL1_ADDR);

	PCIE_PR_INFO("PCIeCTRL apb register 0x0=[0x%x], 0x4=[0x%x]", ret1, ret2);

	return 0;
}
/**
 * set_bme - enable bus master or not.
 * @flag: If flag equals 0, bus master is disabled. If not, bus master is enabled.
 */
int set_bme(u32 rc_id, int flag)
{
	int ret;
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	pp = &(pcie->pp);

	config_enable_dbi(rc_id, ENABLE);

	ret = readl(pp->dbi_base + PCI_COMMAND);
	if (flag) {
		PCIE_PR_INFO("Enable Bus master!!!");
		ret |= PCI_COMMAND_MASTER;
	} else {
		PCIE_PR_INFO("Disable Bus master!!!");
		ret &= ~PCI_COMMAND_MASTER;/* [false alarm]:fortify */
	}
	writel(ret, pp->dbi_base + PCI_COMMAND);

	udelay(5);//lint !e778  !e774  !e516 !e747
	ret = readl(pp->dbi_base + PCI_COMMAND);
	PCIE_PR_INFO("Register[0x4] value is [0x%x]", ret);

	config_enable_dbi(rc_id, DISABLE);

	return 0;
}

/**
 * set_mse - enable mem space or not.
 * @flag: If flag equals 0, mem space is disabled. If not, mem space is enabled.
 */
int set_mse(u32 rc_id, int flag)
{
	int ret;
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	pp = &(pcie->pp);

	config_enable_dbi(rc_id, ENABLE);

	ret = readl(pp->dbi_base + PCI_COMMAND);
	if (flag) {
		PCIE_PR_INFO("Enable MEM space!!!");
		ret |= PCI_COMMAND_MEMORY;
	} else {
		PCIE_PR_INFO("Disable MEM space!!!");
		ret &= ~PCI_COMMAND_MEMORY;/* [false alarm]:fortify */
	}
	writel(ret, pp->dbi_base + PCI_COMMAND);

	udelay(5);//lint !e778  !e774  !e516 !e747
	ret = readl(pp->dbi_base + PCI_COMMAND);
	PCIE_PR_INFO("Register[0x4] value is [0x%x]", ret);

	config_enable_dbi(rc_id, DISABLE);

	return 0;
}

int ltssm_enable(u32 rc_id, int yes)
{
	u32 val;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	if (yes) {
		val = kirin_elb_readl(pcie,  SOC_PCIECTRL_CTRL7_ADDR);
		val |= PCIE_LTSSM_ENABLE_BIT;
		kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL7_ADDR);
	} else {
		val = kirin_elb_readl(pcie,  SOC_PCIECTRL_CTRL7_ADDR);
		val &= ~PCIE_LTSSM_ENABLE_BIT;
		kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL7_ADDR);
	}

	return 0;
}

void kirin_pcie_reset_phy(struct kirin_pcie *pcie)
{
	u32 reg_val;

	reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL1_ADDR);
	reg_val &= ~(0x1 << 17);//phy_reset_sel
	reg_val |= (0x1 << 16);//phy_reset
	kirin_apb_phy_writel(pcie, reg_val, SOC_PCIEPHY_CTRL1_ADDR);
	udelay(10);//lint !e778  !e774
	reg_val &= ~(0x1 << 16);//phy_dereset
	kirin_apb_phy_writel(pcie, reg_val, SOC_PCIEPHY_CTRL1_ADDR);
}

void kirin_pcie_config_l0sl1(u32 rc_id, enum link_aspm_state aspm_state)
{
	struct kirin_pcie *pcie;
	u16 reg_val;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];

	if (!pcie->rc_dev || !pcie->ep_dev) {
		PCIE_PR_ERR("Failed to get dev ");
		return;
	}

	if (aspm_state == ASPM_CLOSE) {
		pcie_capability_clear_and_set_word(pcie->ep_dev, PCI_EXP_LNKCTL,
						   PCI_EXP_LNKCTL_ASPMC, aspm_state);

		pcie_capability_clear_and_set_word(pcie->rc_dev, PCI_EXP_LNKCTL,
					PCI_EXP_LNKCTL_ASPMC, aspm_state);
		pcie_capability_read_word(pcie->ep_dev, PCI_EXP_LNKCTL, &reg_val);
		PCIE_PR_DEBUG("EP LNKCTL: 0x%x", reg_val);
	} else {
		pcie_capability_clear_and_set_word(pcie->rc_dev, PCI_EXP_LNKCTL,
						   PCI_EXP_LNKCTL_ASPMC, aspm_state);

		pcie_capability_clear_and_set_word(pcie->ep_dev, PCI_EXP_LNKCTL,
					PCI_EXP_LNKCTL_ASPMC, aspm_state);
	}
}

void enable_req_clk(struct kirin_pcie *pcie, u32 enable_flag)
{
	u32 val;

	val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL1_ADDR);

	if (enable_flag)
		val &= ~PCIE_APB_CLK_REQ;
	else
		val |= PCIE_APB_CLK_REQ;

	kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL1_ADDR);
}

void kirin_pcie_config_l1ss(u32 rc_id, enum l1ss_ctrl_state enable)
{
	u32 reg_val;
	int rc_l1ss_pm, ep_l1ss_pm, ep_ltr_pm;
	struct kirin_pcie *pcie;
	struct kirin_pcie_dtsinfo *dtsinfo;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];

	dtsinfo = &pcie->dtsinfo;

	if ((pcie->rc_dev == NULL) || (pcie->ep_dev == NULL)) {
		PCIE_PR_ERR("Failed to get RC_dev or EP_dev ");
		return;
	}

	PCIE_PR_DEBUG("Get RC PCI_EXT_L1SS_CAP_ID");
	rc_l1ss_pm = pci_find_ext_capability(pcie->rc_dev, PCI_EXT_L1SS_CAP_ID);
	if (!rc_l1ss_pm) {
		PCIE_PR_ERR("Failed to get RC PCI_EXT_L1SS_CAP_ID");
		return;
	}

	PCIE_PR_DEBUG("Get EP PCI_EXT_L1SS_CAP_ID");
	ep_l1ss_pm = pci_find_ext_capability(pcie->ep_dev, PCI_EXT_L1SS_CAP_ID);
	if (!ep_l1ss_pm) {
		PCIE_PR_ERR("Failed to get EP PCI_EXT_L1SS_CAP_ID");
		return;
	}

	pcie_capability_read_dword(pcie->ep_dev, PCI_EXP_DEVCTL2, &reg_val);
	reg_val &= ~(0x1 << 10);
	pcie_capability_write_dword(pcie->ep_dev, PCI_EXP_DEVCTL2, reg_val);

	pcie_capability_read_dword(pcie->rc_dev, PCI_EXP_DEVCTL2, &reg_val);
	reg_val &= ~(0x1 << 10);
	pcie_capability_write_dword(pcie->rc_dev, PCI_EXP_DEVCTL2, reg_val);


	/*disble L1ss*/
	pci_read_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL1, &reg_val);
	reg_val &= ~L1SS_PM_ASPM_ALL;
	pci_write_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL1, reg_val);

	pci_read_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL1, &reg_val);
	reg_val &= ~L1SS_PM_ASPM_ALL;
	pci_write_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL1, reg_val);

	if (enable) {
		enable_req_clk(pcie, DISABLE);

		/*RC: Power On Value & Scale*/
		if (dtsinfo->ep_l1ss_ctrl2) {
			pci_read_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL2, &reg_val);
			reg_val &= ~0xFF;
			reg_val |= dtsinfo->ep_l1ss_ctrl2;
			pci_write_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL2, reg_val);
		}

		pcie_capability_read_dword(pcie->rc_dev, PCI_EXP_DEVCTL2, &reg_val);
		reg_val |= (0x1 << 10);
		pcie_capability_write_dword(pcie->rc_dev, PCI_EXP_DEVCTL2, reg_val);

		/*EP: Power On Value & Scale*/
		if (dtsinfo->ep_l1ss_ctrl2) {
			pci_read_config_dword(pcie->ep_dev, ep_l1ss_pm+ PCI_EXT_L1SS_CTRL2, &reg_val);
			reg_val &= ~0xFF;
			reg_val |= dtsinfo->ep_l1ss_ctrl2;
			pci_write_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL2, reg_val);
		}

		/*EP: LTR Latency*/
		if (dtsinfo->ep_ltr_latency) {
			ep_ltr_pm= pci_find_ext_capability(pcie->ep_dev, PCI_EXT_LTR_CAP_ID);
			if (ep_ltr_pm)
				pci_write_config_dword(pcie->ep_dev, ep_ltr_pm + LTR_MAX_SNOOP_LATENCY, dtsinfo->ep_ltr_latency);
		}

		pcie_capability_read_dword(pcie->ep_dev, PCI_EXP_DEVCTL2, &reg_val);
		reg_val |= (0x1 << 10);
		pcie_capability_write_dword(pcie->ep_dev, PCI_EXP_DEVCTL2, reg_val);

		/*Enable*/
		pci_read_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL1, &reg_val);
		reg_val = dtsinfo->l1ss_ctrl1;
		reg_val &= 0xFFFFFFF0;
		reg_val |= enable;
		pci_write_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL1, reg_val);

		pci_read_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL1, &reg_val);
		reg_val = dtsinfo->l1ss_ctrl1;
		reg_val &= 0xFFFFFFF0;
		reg_val |= enable;
		pci_write_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL1, reg_val);

	} else {
		enable_req_clk(pcie, ENABLE);
	}

}

static void set_atu_addr(struct pcie_port *pp, int type, char *dbi_base,
			u64 src_addr, u64 dst_addr, u32 size)
{
	u32 iatu_offset = lower_32_bits(dbi_base - (char *)(pp->dbi_base));
	kirin_pcie_writel_rc(pp, iatu_offset + PCIE_ATU_LOWER_BASE, lower_32_bits(src_addr));
	kirin_pcie_writel_rc(pp, iatu_offset + PCIE_ATU_UPPER_BASE, upper_32_bits(src_addr));
	kirin_pcie_writel_rc(pp, iatu_offset + PCIE_ATU_LIMIT, lower_32_bits(src_addr + size - 1));
	kirin_pcie_writel_rc(pp, iatu_offset + PCIE_ATU_LOWER_TARGET, lower_32_bits(dst_addr));
	kirin_pcie_writel_rc(pp, iatu_offset + PCIE_ATU_UPPER_TARGET, upper_32_bits(dst_addr));
	kirin_pcie_writel_rc(pp, iatu_offset + PCIE_ATU_CR1, type);
	kirin_pcie_writel_rc(pp, iatu_offset + PCIE_ATU_CR2, (u32)PCIE_ATU_ENABLE);//lint !e648
}

static void kirin_pcie_atu_cfg(struct pcie_port *pp, u32 index, int direct,
		int type, u64 src_addr, u64 dst_addr, u32 size)
{
	struct kirin_pcie *pcie;
	char *dbi_base;
	unsigned int base_addr;

	pcie = to_kirin_pcie(pp);//lint !e826
	if (!atomic_read(&(pcie->is_power_on))) {
		PCIE_PR_ERR("PCIe is power off");
		return;
	}

	dbi_base = pp->dbi_base;
	base_addr = pcie->dtsinfo.iatu_base_offset;

	if (base_addr != PCIE_ATU_VIEWPORT) {
		base_addr += index * 0x200;
		if (direct == (int)PCIE_ATU_REGION_INBOUND)//lint !e648
			base_addr += INBOUNT_OFFSET;
	} else

	kirin_pcie_writel_rc(pp, PCIE_ATU_VIEWPORT, (index | direct));//lint !e648

	dbi_base += base_addr;

	set_atu_addr(pp, type, dbi_base, src_addr, dst_addr, size);
}

void kirin_pcie_generate_msg(u32 rc_id, int index, u32 iatu_offset, int msg_type, u32 msg_code)
{
	u32 val;
	struct kirin_pcie *pcie;
	struct pcie_port *pp;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];
	pp = &pcie->pp;

	kirin_pcie_outbound_atu(rc_id, index, msg_type,
				pcie->dtsinfo.dbi_base + MSG_CPU_ADDR, 0x0, MSG_CPU_ADDR_SIZE);

	val = kirin_pcie_readl_rc(pp, iatu_offset + PCIE_ATU_CR2);
	val |= (msg_code | INHIBIT_PAYLOAD);
	kirin_pcie_writel_rc(pp, iatu_offset + PCIE_ATU_CR2, val);

	writel(0x0, pcie->pme_base);

}

int kirin_pcie_power_ctrl(struct pcie_port *pp, enum rc_power_status on_flag)
{
	int ret;
	u32 val;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);

	PCIE_PR_INFO("++");

	/*power on*/
	if (on_flag == RC_POWER_ON || on_flag == RC_POWER_RESUME) {
		spin_lock(&pcie->ep_ltssm_lock);
		pcie->ep_link_status = DEVICE_LINK_UP;
		spin_unlock(&pcie->ep_ltssm_lock);

		ret = pcie->plat_ops->plat_on(pp, on_flag);
		if (ret < 0)
			return ret;

		if (pcie->dtsinfo.board_type == BOARD_FPGA) {
			val = kirin_pcie_readl_rc(pp, KIRIN_PCIE_LNKCTL2);
			val &= ~SPEED_MASK;
			val |= SPEED_GEN1;
			kirin_pcie_writel_rc(pp, KIRIN_PCIE_LNKCTL2, val);
		}

	} else if (on_flag == RC_POWER_OFF || on_flag == RC_POWER_SUSPEND) {
		ret = pcie->plat_ops->plat_off(pp, on_flag);
	} else {
		PCIE_PR_ERR("Invalid Param");
		ret = -1;
	}

	PCIE_PR_INFO("--");
	return ret;
}

bool is_pipe_clk_stable(struct kirin_pcie *pcie)
{
	u32 reg_val;
	u32 time = 100;
	u32 pipe_clk_stable = 0x1 << 19;

	if (pcie->dtsinfo.board_type != BOARD_FPGA) {
		reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_STATE0_ADDR);
		while (reg_val & pipe_clk_stable) {
			mdelay(1);
			if (time == 0) {
				return false;
			}
			time--;
			reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_STATE0_ADDR);
		}
	}

	return true;
}
/* ECO Function for PHY Debug */
#define ECO				1
#define ECO_TEST		2
#define SRAM_TEST_SIZE	16
#define ECO_BYPASS_ADDR	SOC_PCIEPHY_CTRL40_ADDR

int kirin_pcie_cfg_eco(struct kirin_pcie *pcie)
{
	u32 reg_val;
	struct kirin_pcie_dtsinfo *dtsinfo;
	u32 sram_init_done = 0x1 << 0;
	u32 time = 10;

#ifdef CONFIG_KIRIN_PCIE_TEST
	void __iomem *checkdata;
	void __iomem *sramdata = pcie->phy_base + pcie->sram_phy_offset;
#endif

	PCIE_PR_INFO("+");
	dtsinfo = &(pcie->dtsinfo);

	reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_STATE39_ADDR);
	while (!(reg_val & sram_init_done)) {
		udelay(100);//lint !e778  !e774  !e516 !e747
		if (time == 0) {
			PCIE_PR_ERR("phy0_sram_init_done fail");
			return -1;
		}
		time--;
		reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_STATE39_ADDR);
	}

	if (pcie->plat_ops->sram_ext_load
		&& pcie->plat_ops->sram_ext_load((void *)pcie)) {
		PCIE_PR_ERR("Sram extra load Failed");
		return -1;
	}

#ifdef CONFIG_KIRIN_PCIE_TEST
	if (dtsinfo->eco == ECO_TEST) {
		/*lint -e124 -esym(124,*)  */
		checkdata = (char *)kmalloc(SRAM_TEST_SIZE * 2, GFP_KERNEL);
		if (!checkdata) {
			PCIE_PR_ERR("Failed to alloc checkdata");
			return -ENOMEM;
		}
		memset(checkdata, 0x0, SRAM_TEST_SIZE * 2);
		memcpy_fromio(checkdata, sramdata, SRAM_TEST_SIZE);
		memset_io(sramdata, 0x0, SRAM_TEST_SIZE);
		if (memcmp(sramdata, checkdata + SRAM_TEST_SIZE, SRAM_TEST_SIZE) != 0) {
			PCIE_PR_ERR("sram data clear fail");
			goto ECO_TEST_FAIL;
		}
		memcpy_toio(sramdata, checkdata, SRAM_TEST_SIZE);
		if (memcmp(sramdata, checkdata + SRAM_TEST_SIZE, SRAM_TEST_SIZE) == 0) {
			PCIE_PR_ERR("sram data all is 0, write eco data fail");
			goto ECO_TEST_FAIL;
		}
		PCIE_PR_ERR("ECO TEST SUCCESS!");
		kfree(checkdata);
		/*lint -e124 +esym(124,*) */
	}
#endif

	/*pull up phy0_sram_ext_ld_done signal, not choose ECO */
	reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL40_ADDR);
	reg_val |= (0x1 << 4);
	kirin_apb_phy_writel(pcie, reg_val, SOC_PCIEPHY_CTRL40_ADDR);

	PCIE_PR_INFO("-");
	return 0;

#ifdef CONFIG_KIRIN_PCIE_TEST
ECO_TEST_FAIL:
	kfree(checkdata);
#endif
	return -1;
}

/*Set Bit0=0'b: pull down phy0_sram_bypass signal, choose ECO */
/*Set Bit0=1'b: pull up phy0_sram_bypass signal, not choose ECO */
void kirin_pcie_select_eco(struct kirin_pcie *pcie)
{
	u32 reg_val;
	reg_val = kirin_apb_phy_readl(pcie, ECO_BYPASS_ADDR);
	if (pcie->dtsinfo.eco)
		reg_val &= ~(0x1 << 0);
	else
		reg_val |= (0x1 << 0);
	kirin_apb_phy_writel(pcie, reg_val, ECO_BYPASS_ADDR);
}

int kirin_pcie_noc_power(struct kirin_pcie *pcie, int enable)
{
	u32 time = 100;
	u32 val = NOC_PW_MASK;
	int rst;

	if (enable)
		val = NOC_PW_MASK | NOC_PW_SET_BIT;
	else
		val = NOC_PW_MASK;
	rst = enable ? 1 : 0;

	writel(val, pcie->pmctrl_base + NOC_POWER_IDLEREQ_1);

	time = 100;
	val = readl(pcie->pmctrl_base + NOC_POWER_IDLE_1);
	while((val & NOC_PW_SET_BIT) != rst) {
		udelay(10);
		if (!time) {
			PCIE_PR_ERR("Failed to reverse noc power-status");
			return -1;
		}
		time--;
		val = readl(pcie->pmctrl_base + NOC_POWER_IDLE_1);
	}

	return 0;
}

#ifndef CONFIG_KIRIN_PCIE_JAN
int kirin_pcie_phy_init(struct kirin_pcie *pcie)
{
	u32 reg_val;

	kirin_pcie_select_eco(pcie);

	/* pull down phy_test_powerdown signal */
	reg_val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL0_ADDR);
	reg_val &= ~PHY_TEST_POWERDOWN;
	kirin_apb_phy_writel(pcie, reg_val, SOC_PCIEPHY_CTRL0_ADDR);

	if (pcie->dtsinfo.eco)
		kirin_pcie_reset_phy(pcie);

	/* deassert controller perst_n */
	reg_val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL12_ADDR);
	if (pcie->dtsinfo.ep_flag)
		reg_val |= PERST_IN_EP;
	else
		reg_val |= PERST_IN_RC;
	kirin_elb_writel(pcie, reg_val, SOC_PCIECTRL_CTRL12_ADDR);
	udelay(10);

	if (pcie->dtsinfo.eco && kirin_pcie_cfg_eco(pcie)) {
		PCIE_PR_ERR("eco init fail");
		return -1;
	}

	return 0;
}
#endif

void kirin_pcie_natural_cfg(struct kirin_pcie *pcie)
{
	u32 val;

	/* pull up sys_aux_pwr_det */
	val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL7_ADDR);
	val |= (0x1 << 10);
	kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL7_ADDR);

	if (pcie->dtsinfo.ep_flag) {
		/* cfg as ep */
		val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL0_ADDR);
		val &= 0xFFFFFFF;
		kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL0_ADDR);
		/* input */
		val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL12_ADDR);
		val |= (0x3 << 2);
		kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL12_ADDR);
	} else {
		/* cfg as rc */
		val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL0_ADDR);
		val &= ~(PCIE_TYPE_MASK << PCIE_TYPE_SHIFT);
		val |= (PCIE_TYPE_RC << PCIE_TYPE_SHIFT);
		kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL0_ADDR);

		/* output, pull down */
		val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL12_ADDR);
		val &= ~(0x3 << 2);
		val |= (0x1 << 1);
		val &= ~(0x1 << 0);
		kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL12_ADDR);
	}

	/* Handle phy_reset and lane0_reset to HW */
	val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL1_ADDR);
	val |= PCIEPHY_RESET_BIT;
	val &= ~PCIEPHY_PIPE_LINE0_RESET_BIT;
	kirin_apb_phy_writel(pcie, val, SOC_PCIEPHY_CTRL1_ADDR);
}

void kirin_pcie_outbound_atu(u32 rc_id, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size)
{
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];
	pp = &(pcie->pp);

	kirin_pcie_atu_cfg(pp, (u32)index, PCIE_ATU_REGION_OUTBOUND,
					type, cpu_addr, pci_addr, size); //lint !e648
}

/* adjust PCIeIO(diffbuf) driver */
void pcie_io_adjust(struct kirin_pcie *pcie)
{
	struct kirin_pcie_dtsinfo *dtsinfo = &pcie->dtsinfo;
	u32 reg_val;

	if (dtsinfo->io_driver[2]) {
		reg_val = kirin_elb_readl(pcie, dtsinfo->io_driver[0]);
		reg_val &= ~dtsinfo->io_driver[1];
		reg_val |= dtsinfo->io_driver[2];
		kirin_elb_writel(pcie, reg_val, dtsinfo->io_driver[0]);
	}
}

void set_phy_eye_param(struct kirin_pcie *pcie)
{
	u32 reg_val;
	u32 i;
	u32 *base;
	struct kirin_pcie_dtsinfo *dtsinfo = &(pcie->dtsinfo);

	if (!dtsinfo->eye_param_nums)
		return;

	for (i = 0; i < dtsinfo->eye_param_nums; i++) {
		base = dtsinfo->eye_param_data + i * TRIPLE_TUPLE;
		if (*(base + 2) != 0xFFFF) {
			reg_val = kirin_natural_phy_readl(pcie, *(base + 0));
			reg_val &= ~(*(base + 1));
			reg_val |= *(base + 2);
			kirin_natural_phy_writel(pcie, reg_val, *(base + 0));
		}
	}
}

void kirin_pcie_inbound_atu(u32 rc_id, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size)
{
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];
	pp = &pcie->pp;

	kirin_pcie_atu_cfg(pp, (u32)index, PCIE_ATU_REGION_INBOUND,
					type, pci_addr, cpu_addr, size);
}

/*
* lookup host ltssm val.
*/
#define LTSSM_STATUE_MASK 0x3F
#define LTSSM_L1SS_MASK 0xC000
u32 show_link_state(u32 rc_id)
{
	unsigned int val;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return LTSSM_PWROFF;

	pcie = &g_kirin_pcie[rc_id];

	val = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE4_ADDR);

	PCIE_PR_INFO("Register 0x410 value [0x%x]", val);

	val = val & LTSSM_STATUE_MASK;

	switch (val) {
	case LTSSM_CPLC:
		PCIE_PR_INFO("L-state: Compliance");
		break;
	case LTSSM_L0:
		PCIE_PR_INFO("L-state: L0");
		break;
	case LTSSM_L0S:
		PCIE_PR_INFO("L-state: L0S");
		break;
	case LTSSM_L1: {
		val = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE5_ADDR);
		PCIE_PR_INFO("Register 0x414 value [0x%x]", val);
		val = val & LTSSM_L1SS_MASK;
		if (val == LTSSM_L1_1)
			PCIE_PR_INFO("L-state: L1.1");
		else if (val == LTSSM_L1_2)
			PCIE_PR_INFO("L-state: L1.2");
		else {
			PCIE_PR_INFO("L-state: L1.0");
			val = LTSSM_L1;
		}
		break;
	}
	case LTSSM_LPBK:
		PCIE_PR_INFO("L-state: LoopBack");
		break;
	default:
		val = LTSSM_OTHERS;
		PCIE_PR_INFO("Other state");
	}
	return val;
}
EXPORT_SYMBOL_GPL(show_link_state);

void pcie_memcpy(ulong dst, ulong src, uint32_t size)
{
	ulong dst_t = dst;
	ulong src_t = src;

	if (IS_ENABLED(CONFIG_KIRIN_PCIE_MAR)) {
		uint dsize;
		uint64_t data_64 = 0;
		uint64_t data_32 = 0;
		uint64_t data_8 = 0;
#if defined(CONFIG_64BIT)
		bool is_64bit_unaligned = (dst_t & 0x7);
#endif
		dsize = sizeof(uint64_t);

		/* Do the transfer(s) */
		while (size) {
			if (size >= sizeof(uint64_t)) {
				if (is_64bit_unaligned) {
					data_32= pcie_rd_32((char *)src_t);
					pcie_wr_32(data_32, (char *)dst_t);
					size -= 4;
					dst_t += 4;
					src_t += 4;
					is_64bit_unaligned = (dst_t & 0x7);
					continue;
				} else {
					data_64= pcie_rd_64((char *)src_t);
					pcie_wr_64(data_64, (char *)dst_t);
				}
			} else {
				dsize = sizeof(uint8_t);
				data_8= pcie_rd_8((char *)src_t);
				pcie_wr_8(data_8, (char *)dst_t);
			}

			/* Adjust for next transfer (if any) */
			if ((size -= dsize)) {
				src_t += dsize;
				dst_t += dsize;
			}
		}
	}else{
		memcpy_s((void *)dst_t, size, (void *)src_t, size);
	}
}

#ifdef CONFIG_KIRIN_PCIE_TEST
int wlan_on(u32 rc_id, int on)
{
	int ret;
	u32 wl_power;
	struct device_node *np;
	struct kirin_pcie *pcie;

	if (rc_id >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_id);
		return -EINVAL;
	}

	pcie = &g_kirin_pcie[rc_id];

	if (!pcie->pp.dev) {
		PCIE_PR_ERR("NO PCIe device");
		return -1;
	}

	np = pcie->pp.dev->of_node;
	if (np) {
		if (!(of_property_read_u32(np, "wl_power", &wl_power))) {
			PCIE_PR_INFO("WL Power On Number is [%d] ", wl_power);
		} else {
			PCIE_PR_INFO("Failed to get wl_power info");
			return -1;
		}
	} else {
		PCIE_PR_INFO("Failed to get kirin_pcie info");
		return -1;
	}

	if (!wl_power) {
		PCIE_PR_ERR("wl_power is invalid");
		return -1;
	}

	ret = gpio_request(wl_power, "wl_on");
	if (ret != 0) {
		PCIE_PR_ERR("Failed to request wl_on gpio");
		return -1;
	}
	if (on) {
		PCIE_PR_INFO("Power on Wlan");
		gpio_direction_output(wl_power, 1);
		mdelay(200);//lint !e778  !e774  !e516  !e747  !e845
	} else {
		PCIE_PR_INFO("Power down Wlan");
		gpio_direction_output(wl_power, 0);
		mdelay(100);//lint !e778  !e774  !e516  !e747  !e845
	}
	gpio_free(wl_power);

	return 0;
}

int retrain_link(u32 rc_id)
{
	u32 val;
	unsigned long start_jiffies;
	u32 cap_pos;
	struct pcie_port *pp;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pp = &(g_kirin_pcie[rc_id].pp);

	cap_pos = kirin_pcie_find_capability(pp, PCI_CAP_ID_EXP);
	if (!cap_pos)
		return -1;

	kirin_pcie_rd_own_conf(pp, (int)cap_pos + PCI_EXP_LNKCTL, 4, &val);
	/* Retrain link */
	val |= PCI_EXP_LNKCTL_RL;
	kirin_pcie_wr_own_conf(pp, (int)cap_pos + PCI_EXP_LNKCTL, 4, val);

	/* Wait for link training end. Break out after waiting for timeout */
	start_jiffies = jiffies;
	for (;;) {
		kirin_pcie_rd_own_conf(pp, (int)cap_pos + PCI_EXP_LNKSTA, 4, &val);
		val &= 0xffff;
		if (!(val & PCI_EXP_LNKSTA_LT))
			break;
		if (time_after(jiffies, start_jiffies + HZ))
			break;
		msleep(1);
	}
	if (!(val & PCI_EXP_LNKSTA_LT))
		return 0;
	return -1;
}

int set_link_speed(u32 rc_id, enum link_speed gen)
{
	u32 val = 0x1;
	u32 reg_val = 0x0;
	int ret = 0;
	u32 cap_pos = 0x0;
	struct pcie_port *pp;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pp = &(g_kirin_pcie[rc_id].pp);

	switch (gen) {
	case GEN1:
		val = 0x1;
		break;
	case GEN2:
		val = 0x2;
		break;
	case GEN3:
		val = 0x3;
		break;
	default:
		ret = -1;
	}

	cap_pos = kirin_pcie_find_capability(pp, PCI_CAP_ID_EXP);
	if (!cap_pos)
		return -1;

	kirin_pcie_rd_own_conf(pp, (int)cap_pos + PCI_EXP_LNKCTL2, 4, &reg_val);
	reg_val &= ~(0x3 << 0);
	reg_val |= val;
	kirin_pcie_wr_own_conf(pp, (int)cap_pos + PCI_EXP_LNKCTL2, 4, reg_val);

	if (!ret)
		return retrain_link(rc_id);
	return ret;
}

int show_link_speed(u32 rc_id)
{
	unsigned int val;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	val = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE4_ADDR);
	val = val & 0xc0;
	switch (val) {
	case 0x0:
		PCIE_PR_INFO("Link speed: gen1");
		break;
	case 0x40:
		PCIE_PR_INFO("Link speed: gen2");
		break;
	case 0x80:
		PCIE_PR_INFO("Link speed: gen3");
		break;
	default:
		PCIE_PR_INFO("Link speed info unknow");
	}

	return val;
}

int limit_link_speed(struct kirin_pcie *pcie)
{
	return set_link_speed(pcie->rc_id, pcie->speed_limit);
}

u32 kirin_pcie_find_capability(struct pcie_port *pp, int cap)
{
	u8 id;
	int ttl = 0x30;
	u32 pos = 0;
	u32 ent = 0;

	kirin_pcie_rd_own_conf(pp, PCI_CAPABILITY_LIST, 1, &pos);

	while (ttl--) {
		if (pos < 0x40)
			break;
		pos &= ~3;
		kirin_pcie_rd_own_conf(pp, pos, 2, &ent);
		id = ent & 0xff;
		if (id == 0xff)
			break;
		if (id == cap)
			return pos;
		pos = (ent >> 8);
	}
	return 0;
}

#endif
/*lint -e618  -e826 -e648 -e647 -e679 -e438 -e550 -e713 -e732 -e737 -e774 -e838 -esym(618,*) -esym(826,*) -esym(648,*) -esym(847,*) -esym(679,*) -esym(438,*) -esym(550,*) -esym(713,*) -esym(732,*) -esym(737,*) -esym(774,*) -esym(838,*) */

