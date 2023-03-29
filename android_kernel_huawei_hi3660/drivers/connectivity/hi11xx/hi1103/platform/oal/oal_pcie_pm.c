
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "oal_util.h"

#include "oal_net.h"
#include "oal_ext_if.h"

#include "oal_pcie_pm.h"
#include "oal_pcie_host.h"


//#undef  THIS_FILE_ID
//#define THIS_FILE_ID OAM_FILE_ID_OAL_PCIE_PM_C
#ifdef CONFIG_ARCH_SD56XX

/******************************************************************
                    ASPM Test code
*******************************************************************/
oal_void oal_pcie_aspm_enable(oal_pci_dev_stru *pst_dev, oal_pcie_aspm_enum mode)
{
    oal_uint32 pos;
    oal_uint16 reg16, tmp;

    pos = oal_pci_pcie_cap(pst_dev);
    if(pos)
    {
        oal_pci_read_config_word(pst_dev, pos + PCI_EXP_LNKCTL, &reg16);
        switch(mode)
        {
            case PCIE_ASPM_CAP_L0S:
            reg16|=BIT0;
            break;
            case PCIE_ASPM_CAP_L1:
            reg16|=BIT1;
            break;
            case PCIE_ASPM_CAP_L0S_AND_L1:
            reg16|=BIT0;
            reg16|=BIT1;
            break;
        }
        oal_pci_write_config_word(pst_dev, pos + PCI_EXP_LNKCTL, reg16);
        oal_pci_read_config_word(pst_dev, pos + PCI_EXP_LNKCTL,&tmp);
        OAL_IO_PRINT("read pos:0x%x\n", pos + PCI_EXP_LNKCTL);
        oal_pcie_print_bits((oal_void*)&tmp, 2);
    }
    else
    {
        OAL_IO_PRINT("oal_pcie_aspm_enable fail to get PCI_CAP_ID_EXP offset\r\n");
    }
}


oal_void oal_pcie_aspm_disable(oal_pci_dev_stru *pst_dev, oal_pcie_aspm_enum mode)
{
    oal_uint32 pos;
    oal_uint16 reg16;

    pos = oal_pci_pcie_cap(pst_dev);
    if(pos)
    {
        oal_pci_read_config_word(pst_dev, pos + PCI_EXP_LNKCTL, &reg16);
        switch(mode)
        {
            case PCIE_ASPM_CAP_L0S:
            reg16 &= ~BIT0;
            break;
            case PCIE_ASPM_CAP_L1:
            reg16 &= ~BIT1;
            break;
            case PCIE_ASPM_CAP_L0S_AND_L1:
            reg16 &= ~BIT0;
            reg16 &= ~BIT1;
            break;
        }
        oal_pci_write_config_word(pst_dev, pos + PCI_EXP_LNKCTL, reg16);

    }
    else
    {
        OAL_IO_PRINT("oal_pcie_aspm_disable fail to get PCI_CAP_ID_EXP offset\r\n");
    }
}


oal_void oal_pcie_clkpm_set(oal_pci_dev_stru *pst_dev, oal_uint32 enable)
{
    oal_uint32 pos,reg32=0;
    oal_uint16 reg16;

    pos = oal_pci_pcie_cap(pst_dev);
    if (!pos)
    {
        return;
    }
    oal_pci_read_config_dword(pst_dev, pos + PCI_EXP_LNKCAP, &reg32);
    if(!(reg32 & PCI_EXP_LNKCAP_CLKPM))
    {
        OAL_IO_PRINT("oal_pcie_clkpm_set fail:device do not support clk pm \r\n");
        return;
    }

    oal_pci_read_config_word(pst_dev, pos + PCI_EXP_LNKCTL, &reg16);
    if (enable)
    {
        reg16 |= PCI_EXP_LNKCTL_CLKREQ_EN;
    }
    else
    {
        reg16 &= ~PCI_EXP_LNKCTL_CLKREQ_EN;
    }
    oal_pci_write_config_word(pst_dev, pos + PCI_EXP_LNKCTL, reg16);

    return;

}

oal_void oal_pcie_common_clk_set(oal_pci_dev_stru *pst_dev, oal_uint32 nfts)
{
    oal_uint32 pos,same_clock = 1;
    oal_uint16 reg16=0;
    oal_uint32 reg32=0;

    pos = oal_pci_pcie_cap(pst_dev);
    if (!pos)
    {
        return;
    }

    oal_pci_read_config_word(pst_dev, pos + PCI_EXP_LNKSTA, &reg16);
	if (!(reg16 & PCI_EXP_LNKSTA_SLC))
	{
		same_clock = 0;
	}

    OAL_IO_PRINT("oal_pcie_common_clk_set :device support[%d]\r\n",same_clock);
    oal_pci_read_config_word(pst_dev, pos + PCI_EXP_LNKCTL, &reg16);
    if (same_clock)
    {
        reg16 |= PCI_EXP_LNKCTL_CCC;
    }
    else
    {
        reg16 &= ~PCI_EXP_LNKCTL_CCC;
    }
    oal_pci_write_config_word(pst_dev, pos + PCI_EXP_LNKCTL, reg16);
    oal_pcie_print_bits(&reg16, 2);
    /*N-FTS个数配置*/
    oal_pci_read_config_dword(pst_dev, PCIE_PL_ASPM_CTRL_OFFSET, &reg32);
    OAL_IO_PRINT("0x70c:old ");
    oal_pcie_print_bits(&reg32, 4);
    REG32_SETBITS(reg32,COMMON_CLK_NFTS_BIT_OFFSET,8,nfts);
    REG32_SETBITS(reg32,ACK_NFTS_BIT_OFFSET,8,nfts);
    oal_pci_write_config_word(pst_dev, PCIE_PL_ASPM_CTRL_OFFSET, reg32);
    OAL_IO_PRINT("0x70c: ");
    oal_pcie_print_bits(&reg32, 4);
    oal_pci_read_config_dword(pst_dev, PCIE_PL_ASPM_CTRL_OFFSET, &reg32);
    OAL_IO_PRINT("0x70c: read:0x%x", reg32);
    oal_pci_read_config_dword(pst_dev, 0x80c, &reg32);
    REG32_SETBITS(reg32,0,8,nfts);
    oal_pci_write_config_word(pst_dev, 0x80c, reg32);
    OAL_IO_PRINT("0x80c: ");
    oal_pcie_print_bits(&reg32, 4);

    return;

}

oal_uint32 oal_pcie_l11_set(oal_pci_dev_stru *pst_dev,oal_uint32 enable)
{
    oal_uint32 pos,reg_cap,reg_ctrl1;

    pos = oal_pci_find_ext_capability(pst_dev,PCI_EXT_CAP_ID_L1SS);
    if(pos)
    {
        oal_pci_read_config_dword(pst_dev, pos + PCI_L1SS_CAP, &reg_cap);
        reg_cap &= (PCI_L1SS_L11_PCIPM_EN | PCI_L1SS_L11_ASPM_EN);
        oal_pci_read_config_dword(pst_dev, pos + PCI_L1SS_CTRL1, &reg_ctrl1);
        if(enable)
        {
            oal_pci_write_config_word(pst_dev, pos + PCI_L1SS_CTRL1, (reg_cap|reg_ctrl1));
        }
        else
        {
            oal_pci_write_config_word(pst_dev, pos + PCI_L1SS_CTRL1, (reg_cap|0x00));
        }
        OAL_IO_PRINT("pcie L1.1 enabled\n");
        return OAL_SUCC;
    }
    else
    {
        OAL_IO_PRINT("oal_pcie_l1ss_set FAIL find PCI_EXT_CAP_ID_L1SS");
        return OAL_FAIL;

    }
}

oal_uint32 oal_pcie_l12_set(oal_pci_dev_stru *pst_dev,oal_uint32 enable)
{
    oal_uint32 pos,reg_cap,reg_ctrl1, reg32;

    pos = oal_pci_find_ext_capability(pst_dev,PCI_EXT_CAP_ID_L1SS);
    if(pos)
    {
        oal_pci_read_config_dword(pst_dev, pos + PCI_L1SS_CAP, &reg_cap);
        reg_cap &= (PCI_L1SS_L12_PCIPM_EN | PCI_L1SS_L12_ASPM_EN);
        oal_pci_read_config_dword(pst_dev, pos + PCI_L1SS_CTRL1, &reg_ctrl1);
        if(enable)
        {
            oal_pci_write_config_word(pst_dev, pos + PCI_L1SS_CTRL1, (reg_cap|reg_ctrl1));
        }
        else
        {
            oal_pci_write_config_word(pst_dev, pos + PCI_L1SS_CTRL1, (reg_cap|0x00));
        }

        pos = oal_pci_pcie_cap(pst_dev);
        if(!pos)
        {
            OAL_IO_PRINT("get pcie cap failed!\n");
            return OAL_FAIL;
        }

        /*PCI_EXP_DEVCTL2_LTR_EN 必须要在PCI_L1SS_L12_PCIPM_EN/PCI_L1SS_L12_ASPM_EN 使能后配置，否者不生效!!*/
        oal_pci_read_config_dword(pst_dev, pos + PCI_EXP_DEVCTL2, &reg32);
        reg32 |= PCI_EXP_DEVCTL2_LTR_EN;
        oal_pci_write_config_dword(pst_dev, pos + PCI_EXP_DEVCTL2, reg32);
        oal_pci_read_config_dword(pst_dev, pos + PCI_EXP_DEVCTL2, &reg32);
        OAL_IO_PRINT("PCI_EXP_DEVCTL2:0x%x\n", reg32);
        OAL_IO_PRINT("pcie L1.2 enabled\n");
        return OAL_SUCC;
    }
    else
    {
        OAL_IO_PRINT("oal_pcie_l1ss_set FAIL find PCI_EXT_CAP_ID_L1SS");
        return OAL_FAIL;

    }
}

oal_uint32 oal_pcie_l1ss_set(oal_pci_dev_stru *pst_dev,oal_uint32 enable)
{
    oal_uint32 ret;
    ret = oal_pcie_l11_set(pst_dev, enable);
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    ret = oal_pcie_l12_set(pst_dev, enable);
    if(OAL_SUCC != ret)
    {
        return ret;
    }

    return OAL_SUCC;
#if 0
    oal_uint32 pos,reg_cap,reg_ctrl1;

    pos = oal_pci_find_ext_capability(pst_dev,PCI_EXT_CAP_ID_L1SS);
    if(pos)
    {
        oal_pci_read_config_dword(pst_dev, pos + PCI_L1SS_CAP, &reg_cap);
        reg_cap &= 0x0F;
        oal_pci_read_config_dword(pst_dev, pos + PCI_L1SS_CTRL1, &reg_ctrl1);
        if(enable)
        {
            oal_pci_write_config_word(pst_dev, pos + PCI_L1SS_CTRL1, (reg_cap|reg_ctrl1));
        }
        else
        {
            oal_pci_write_config_word(pst_dev, pos + PCI_L1SS_CTRL1, (reg_cap|0x00));
        }
        return OAL_SUCC;
    }
    else
    {
        OAL_IO_PRINT("oal_pcie_l1ss_set FAIL find PCI_EXT_CAP_ID_L1SS");
        return OAL_FAIL;

    }
#endif
}

oal_void oal_pcie_l0s_entry_latency_set(oal_pci_dev_stru *pst_dev,oal_pcie_L0s_ent_enum value)
{
    oal_uint32 reg32=0;

    /*bit[24:26]*/
    oal_pci_read_config_dword(pst_dev, PCIE_PL_ASPM_CTRL_OFFSET, &reg32);
    REG32_SETBITS(reg32,24,3,value);
    oal_pci_write_config_word(pst_dev,PCIE_PL_ASPM_CTRL_OFFSET, reg32);

}

oal_void oal_pcie_l1_entry_latency_set(oal_pci_dev_stru *pst_dev,oal_pcie_L1_ent_enum value)
{
    oal_uint32 reg32=0;

    /*bit[27:29]*/
    oal_pci_read_config_dword(pst_dev, PCIE_PL_ASPM_CTRL_OFFSET, &reg32);
    REG32_SETBITS(reg32,27,3,value);
    oal_pci_write_config_word(pst_dev,PCIE_PL_ASPM_CTRL_OFFSET, reg32);

}

oal_void oal_pcie_complete_timeout_set(oal_pci_dev_stru *pst_dev,oal_pcie_comp_timeout_enum value)
{
    oal_uint32 pos;
    oal_uint16 reg16=0;

    pos = oal_pci_pcie_cap(pst_dev);
    if(pos)
    {
        oal_pci_read_config_word(pst_dev, pos + PCI_EXP_DEVCTL2, &reg16);
        REG16_SETBITS(reg16,0,4,value);
        REG16_SETBITS(reg16,10,1,0x1);
        oal_pci_write_config_word(pst_dev, pos + PCI_EXP_DEVCTL2, reg16);

    }
    else
    {
        OAL_IO_PRINT("oal_pcie_aspm_enable fail to get PCI_CAP_ID_EXP offset\r\n");
    }

}


oal_void *g_pst_pci_rc_sys_ctl = OAL_PTR_NULL;
oal_void *g_pst_pci_dbi_0 = OAL_PTR_NULL;
oal_void *g_pst_pci_dbi_1 = OAL_PTR_NULL;

oal_uint32  oal_pcie_rc_mem_map(oal_void)
{
    g_pst_pci_rc_sys_ctl = oal_ioremap_nocache(OAL_PCIE_RC_SYS_BASE, 0x1000);
    if (OAL_PTR_NULL == g_pst_pci_rc_sys_ctl)
    {
        OAL_IO_PRINT("oal_pci_rc_map, Cannot map system controller register base!\n");

        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_IO_PRINT("oal_pci_rc_map,OAL_PCIE_SYS_BASE = 0x%x,map g_pst_pci_rc_sys_ctl = 0x%lx\n",OAL_PCIE_RC_SYS_BASE,(oal_ulong)g_pst_pci_rc_sys_ctl);

    g_pst_pci_dbi_0 = oal_ioremap_nocache(OAL_DBI_RC_BASE_ADDR_0, 0x1000);
    if (OAL_PTR_NULL == g_pst_pci_dbi_0)
    {
        OAL_IO_PRINT("oal_pci_rc_map, Cannot map dbi 0 register base!\n");

        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_IO_PRINT("oal_pci_rc_map,OAL_DBI_BASE_ADDR_0=0x%x,map g_pst_pci_dbi_0 = 0x%lx\n",(unsigned int)OAL_DBI_RC_BASE_ADDR_0,(oal_ulong)g_pst_pci_dbi_0);

    return OAL_SUCC;
}

oal_void  oal_pcie_rc_mem_unmap(oal_void)
{
    if (g_pst_pci_rc_sys_ctl)
    {
        oal_iounmap(g_pst_pci_rc_sys_ctl);
    }

    if (g_pst_pci_dbi_0)
    {
        oal_iounmap(g_pst_pci_dbi_0);
    }
}


oal_void oal_pcie_rc_dbi_enable(oal_uint32 id)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_uint32 ul_val;

    if (0 == id)
    {
        ul_val = oal_readl(g_pst_pci_rc_sys_ctl + OAL_PERI_R_PCIE0);
        ul_val |= BIT21;
        oal_writel(ul_val, g_pst_pci_rc_sys_ctl + OAL_PERI_R_PCIE0);

        ul_val = oal_readl(g_pst_pci_rc_sys_ctl + OAL_PERI_W_PCIE0);
        ul_val |= BIT21;
        oal_writel(ul_val, g_pst_pci_rc_sys_ctl + OAL_PERI_W_PCIE0);
    }
    else
    {
        ul_val = oal_readl(g_pst_pci_rc_sys_ctl + OAL_PERI_R_PCIE1);
        ul_val |= BIT21;
        oal_writel(ul_val, g_pst_pci_rc_sys_ctl + OAL_PERI_R_PCIE1);

        ul_val = oal_readl(g_pst_pci_rc_sys_ctl + OAL_PERI_W_PCIE1);
        ul_val |= BIT21;
        oal_writel(ul_val, g_pst_pci_rc_sys_ctl + OAL_PERI_W_PCIE1);
    }
#endif
}


oal_void oal_pcie_rc_dbi_disable(oal_uint32 id)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* 配置工作模式，恢复读写wifi侧 */
     oal_uint32 ul_val;

     if (0 == id)
     {
        ul_val = oal_readl(g_pst_pci_rc_sys_ctl + OAL_PERI_R_PCIE0);
        ul_val &= (~BIT21);
        oal_writel(ul_val, g_pst_pci_rc_sys_ctl + OAL_PERI_R_PCIE0);

        ul_val = oal_readl(g_pst_pci_rc_sys_ctl + OAL_PERI_W_PCIE0);
        ul_val &= (~BIT21);
        oal_writel(ul_val, g_pst_pci_rc_sys_ctl + OAL_PERI_W_PCIE0);
      }
      else
      {
        ul_val = oal_readl(g_pst_pci_rc_sys_ctl + OAL_PERI_R_PCIE1);
        ul_val &= (~BIT21);
        oal_writel(ul_val, g_pst_pci_rc_sys_ctl + OAL_PERI_R_PCIE1);

        ul_val = oal_readl(g_pst_pci_rc_sys_ctl + OAL_PERI_W_PCIE1);
        ul_val &= (~BIT21);
        oal_writel(ul_val, g_pst_pci_rc_sys_ctl + OAL_PERI_W_PCIE1);
      }
#endif
}

oal_void oal_pcie_rc_aspm_enable(oal_pcie_aspm_enum mode)
{
    oal_uint32 reg32;

    oal_pcie_rc_dbi_enable(0);
    switch(mode)
    {
        case PCIE_ASPM_CAP_L0S:
        reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        reg32 |= BIT0;
        oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        break;
        case PCIE_ASPM_CAP_L1:
        reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        reg32 |= BIT1;
        oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        break;
        case PCIE_ASPM_CAP_L0S_AND_L1:
        reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        reg32 |= BIT0;
        reg32 |= BIT1;
        oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        break;
    }

    oal_pcie_rc_dbi_disable(0);

}

oal_void oal_pcie_rc_reg_print(oal_uint32 reg)
{
    oal_uint32 reg32;

    oal_pcie_rc_dbi_enable(0);
    reg32 = oal_readl(g_pst_pci_dbi_0 + reg);
    OAL_IO_PRINT("reg 0x%x ", reg);
    oal_pcie_print_bits(&reg32, sizeof(reg32));
    oal_pcie_rc_dbi_disable(0);
}

oal_void oal_pcie_rc_aspm_disable(oal_pcie_aspm_enum mode)
{
    oal_uint32 reg32;

    oal_pcie_rc_dbi_enable(0);
    switch(mode)
    {
        case PCIE_ASPM_CAP_L0S:
        reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        reg32 &= ~BIT0;
        oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        break;
        case PCIE_ASPM_CAP_L1:
        reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        reg32 &= ~BIT1;
        oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        break;
        case PCIE_ASPM_CAP_L0S_AND_L1:
        reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        reg32 &= ~BIT0;
        reg32 &= ~BIT1;
        oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
        break;
    }

    oal_pcie_rc_dbi_disable(0);

}

oal_void oal_pcie_rc_clkpm_set(int enable)
{
    oal_uint32 reg32;

    oal_pcie_rc_dbi_enable(0);

    reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCAP);
    if(!(reg32 & PCI_EXP_LNKCAP_CLKPM))
    {
        OAL_IO_PRINT("oal_pcie_clkpm_set fail:device do not support clk pm \r\n");
        return;
    }

    reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
    if (enable)
    {
        reg32 |= PCI_EXP_LNKCTL_CLKREQ_EN;
    }
    else
    {
        reg32 &= ~PCI_EXP_LNKCTL_CLKREQ_EN;
    }
    oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);

    oal_pcie_rc_dbi_disable(0);

    return;

}

oal_void oal_pcie_rc_common_clk_set(oal_uint32 nfts)
{
    oal_uint32 same_clock = 1;
    oal_uint32 reg32;

    oal_pcie_rc_dbi_enable(0);

    reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKSTA);
	if (!(reg32 & PCI_EXP_LNKSTA_SLC))
	{
		same_clock = 0;
	}
    OAL_IO_PRINT("oal_pcie_rc_common_clk_set :rc support[%d]\r\n",same_clock);

    reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
    if (same_clock)
    {
        reg32 |= PCI_EXP_LNKCTL_CCC;
    }
    else
    {
        reg32 &= ~PCI_EXP_LNKCTL_CCC;
    }
    oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
    oal_pcie_print_bits(&reg32, 4);

    /*N-FTS个数配置*/
    reg32 = oal_readl(g_pst_pci_dbi_0+PCIE_PL_ASPM_CTRL_OFFSET);
    OAL_IO_PRINT("rc 0x70c: old");
    oal_pcie_print_bits(&reg32, 4);
    REG32_SETBITS(reg32,COMMON_CLK_NFTS_BIT_OFFSET,8,nfts);
    REG32_SETBITS(reg32,ACK_NFTS_BIT_OFFSET,8,nfts);
    oal_writel(reg32, g_pst_pci_dbi_0+PCIE_PL_ASPM_CTRL_OFFSET);
    OAL_IO_PRINT("rc 0x70c: ");
    oal_pcie_print_bits(&reg32, 4);

    reg32 = oal_readl(g_pst_pci_dbi_0+0x80c);
    OAL_IO_PRINT("rc 0x80c: old");
    oal_pcie_print_bits(&reg32, 4);
    REG32_SETBITS(reg32,0,8,nfts);
    oal_writel(reg32, g_pst_pci_dbi_0+0x80c);
    OAL_IO_PRINT("rc 0x80c: ");
    oal_pcie_print_bits(&reg32, 4);

    reg32 = oal_readl(g_pst_pci_dbi_0+0x80c);
    REG32_SETBITS(reg32,0,8,nfts);
    oal_writel(reg32, g_pst_pci_dbi_0+0x80c);
    OAL_IO_PRINT("rc 0x80c: ");
    oal_pcie_print_bits(&reg32, 4);

    oal_pcie_rc_dbi_disable(0);

    return;

}

#define PCI_CFG_SPACE_SIZE	256
#define PCI_CFG_SPACE_EXP_SIZE	4096
oal_uint32 oal_pcie_rc_l1ss_set(oal_uint32 enable)
{
    oal_uint32 pos,current_id;
    oal_uint32 reg_hdr,reg_cap,reg_ctrl1;

    oal_int32 ttl = (PCI_CFG_SPACE_EXP_SIZE - PCI_CFG_SPACE_SIZE) / 8;

    oal_pcie_rc_dbi_enable(0);

    current_id = 0;

    /* PCI Express Extended Capability */
    pos = PCI_CFG_SPACE_SIZE;
    reg_hdr = oal_readl(g_pst_pci_dbi_0 + pos);
    while ((ttl--) > 0) {
        current_id = PCI_EXT_CAP_ID(reg_hdr);

        if (current_id == PCI_EXT_CAP_ID_L1SS)
        {
            break;
        }
        pos = PCI_EXT_CAP_NEXT(reg_hdr);

        reg_hdr = oal_readl(g_pst_pci_dbi_0 + pos);
    }

   if (current_id != PCI_EXT_CAP_ID_L1SS)
   {

        OAL_IO_PRINT("oal_pcie_rc_l1ss_set FAIL find PCI_EXT_CAP_ID_L1SS");
        return OAL_FAIL;
    }


    reg_cap  = oal_readl(g_pst_pci_dbi_0 + reg_hdr + PCI_L1SS_CAP);
    reg_cap &= 0x0F;
    reg_ctrl1 = oal_readl(g_pst_pci_dbi_0 + reg_hdr + PCI_L1SS_CTRL1);
    if(enable)
    {
        oal_writel((reg_cap|reg_ctrl1),g_pst_pci_dbi_0 + reg_hdr + PCI_L1SS_CTRL1);
    }
    else
    {
        oal_writel((reg_cap|0x00), g_pst_pci_dbi_0 + reg_hdr + PCI_L1SS_CTRL1);
    }

    oal_pcie_rc_dbi_disable(0);
    return OAL_SUCC;
}

oal_void oal_pcie_rc_l0s_entry_latency_set(oal_pcie_L0s_ent_enum value)
{
    oal_uint32 reg32=0;

    oal_pcie_rc_dbi_enable(0);

    /*bit[24:26]*/
    reg32 = oal_readl(g_pst_pci_dbi_0+PCIE_PL_ASPM_CTRL_OFFSET);
    REG32_SETBITS(reg32,24,3,value);
    oal_writel(reg32,(g_pst_pci_dbi_0+PCIE_PL_ASPM_CTRL_OFFSET));

    oal_pcie_rc_dbi_disable(0);

}

oal_void oal_pcie_rc_l1_entry_latency_set(oal_pcie_L1_ent_enum value)
{
    oal_uint32 reg32=0;

    oal_pcie_rc_dbi_enable(0);

    /*bit[27:29]*/
    reg32 = oal_readl(g_pst_pci_dbi_0+PCIE_PL_ASPM_CTRL_OFFSET);
    REG32_SETBITS(reg32,27,3,value);
    oal_writel(reg32,g_pst_pci_dbi_0+PCIE_PL_ASPM_CTRL_OFFSET);

    oal_pcie_rc_dbi_disable(0);

}

oal_void oal_pcie_rc_complete_timeout_set(oal_pcie_comp_timeout_enum value)
{
    oal_uint16 reg16=0;

    oal_pcie_rc_dbi_enable(0);
    reg16  = readw(g_pst_pci_dbi_0 + PCI_EXP_DEVCTL2);
    REG16_SETBITS(reg16,0,4,value);
    REG16_SETBITS(reg16,10,1,0x1);
    writew(reg16, g_pst_pci_dbi_0 + PCI_EXP_DEVCTL2);

    oal_pcie_rc_dbi_disable(0);

}


#define LINK_RETRAIN_TIMEOUT HZ
oal_uint32 oal_pcie_link_retrain(oal_void)
{
    oal_uint32 reg32;
    oal_ulong  start_jiffies;

    OAL_IO_PRINT("oal_pcie_link_retrain\n");
    oal_pcie_rc_dbi_enable(0);

    /* Retrain link */
    reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);
	reg32 |= PCI_EXP_LNKCTL_RL;
	oal_writel(reg32,g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKCTL);

    oal_pcie_rc_dbi_disable(0);

	/* Wait for link training end. Break out after waiting for timeout */
	start_jiffies = jiffies;
	for (;;) {
	    oal_pcie_rc_dbi_enable(0);
	    reg32 = oal_readl(g_pst_pci_dbi_0 + OAL_PCIE_CAP_POS + PCI_EXP_LNKSTA);
		oal_pcie_rc_dbi_disable(0);

		if (!(reg32 & PCI_EXP_LNKSTA_LT))
		{
			break;
	    }
		if (oal_time_after(jiffies, start_jiffies + LINK_RETRAIN_TIMEOUT))
		{
			break;
		}

		oal_msleep(1);
	}
	if (!(reg32 & PCI_EXP_LNKSTA_LT))
	{
		return OAL_SUCC;
	}
	else
	{
        OAL_IO_PRINT("oal_pcie_rc_retrain_link fail\r\n");
	    return OAL_FAIL;
	}
}


oal_void oal_pcie_link_aspm_enable(oal_pci_dev_stru *pst_dev, oal_pcie_aspm_enum mode)
{
    OAL_IO_PRINT("aspm enable mode:%d\n", mode);
    oal_pcie_rc_aspm_enable(mode);

    oal_pcie_aspm_enable(pst_dev,mode);
}

oal_void oal_pcie_link_aspm_disable(oal_pci_dev_stru *pst_dev, oal_pcie_aspm_enum mode)
{
    oal_pcie_rc_aspm_disable(mode);

    oal_pcie_aspm_disable(pst_dev,mode);
}

oal_void oal_pcie_link_clkpm_set(oal_pci_dev_stru *pst_dev, oal_uint32 enable)
{
    oal_pcie_clkpm_set(pst_dev, enable);

    oal_pcie_rc_clkpm_set(enable);
}

oal_void oal_pcie_link_common_clk_set(oal_pci_dev_stru *pst_dev)
{
    oal_uint32 nfts = 0x3a;
//    oal_uint32 nfts = 0x10;/*change nfts to triiger l0s recovery*/

    OAL_IO_PRINT("pcie nfts :0x%x common clk set\n", nfts);
    oal_pcie_common_clk_set(pst_dev, nfts);

    oal_pcie_rc_common_clk_set(nfts);

    //oal_pcie_link_retrain();
}

oal_void oal_pcie_link_l1ss_set(oal_pci_dev_stru *pst_dev, oal_uint32 enable)
{
    oal_pcie_l1ss_set(pst_dev,enable);

    oal_pcie_rc_l1ss_set(enable);

}

oal_void oal_pcie_link_l0s_entry_latency_set(oal_pci_dev_stru *pst_dev,oal_pcie_L0s_ent_enum value)
{
    oal_pcie_rc_l0s_entry_latency_set(value);

    oal_pcie_l0s_entry_latency_set(pst_dev,value);

}

oal_void oal_pcie_link_l1_entry_latency_set(oal_pci_dev_stru *pst_dev,oal_pcie_L1_ent_enum value)
{
    oal_pcie_rc_l1_entry_latency_set(value);

    oal_pcie_l1_entry_latency_set(pst_dev,value);

}

oal_void oal_pcie_link_complete_timeout_set(oal_pci_dev_stru *pst_dev,oal_pcie_comp_timeout_enum value)
{
    oal_pcie_rc_complete_timeout_set(value);

    oal_pcie_complete_timeout_set(pst_dev,value);

}
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
