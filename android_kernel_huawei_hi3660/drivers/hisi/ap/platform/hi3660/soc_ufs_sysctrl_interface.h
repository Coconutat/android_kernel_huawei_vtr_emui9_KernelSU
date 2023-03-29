#ifndef __SOC_UFS_SYSCTRL_INTERFACE_H__
#define __SOC_UFS_SYSCTRL_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_UFS_Sysctrl_MEMORY_CTRL_ADDR(base) ((base) + (0x000))
#define SOC_UFS_Sysctrl_PSW_POWER_CTRL_ADDR(base) ((base) + (0x004))
#define SOC_UFS_Sysctrl_PHY_ISO_EN_ADDR(base) ((base) + (0x0008))
#define SOC_UFS_Sysctrl_HC_LP_CTRL_ADDR(base) ((base) + (0x000C))
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_ADDR(base) ((base) + (0x0010))
#define SOC_UFS_Sysctrl_PSW_CLK_CTRL_ADDR(base) ((base) + (0x0014))
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_ADDR(base) ((base) + (0x0018))
#define SOC_UFS_Sysctrl_RESET_CTRL_EN_ADDR(base) ((base) + (0x001C))
#define SOC_UFS_Sysctrl_PHY_RESET_STATUS_ADDR(base) ((base) + (0x0028))
#define SOC_UFS_Sysctrl_HC_DEBUG_ADDR(base) ((base) + (0x002C))
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_ADDR(base) ((base) + (0x0030))
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_ADDR(base) ((base) + (0x0034))
#define SOC_UFS_Sysctrl_PHY_DTB_OUT_ADDR(base) ((base) + (0x0038))
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_HH_ADDR(base) ((base) + (0x003C))
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_H_ADDR(base) ((base) + (0x0040))
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_L_ADDR(base) ((base) + (0x0044))
#define SOC_UFS_Sysctrl_DEBUG_MONITORUP_H_ADDR(base) ((base) + (0x0048))
#define SOC_UFS_Sysctrl_DEBUG_MONITORUP_L_ADDR(base) ((base) + (0x004C))
#define SOC_UFS_Sysctrl_MK2_CTRL_ADDR(base) ((base) + (0x0050))
#define SOC_UFS_Sysctrl_UFS_SYSCTRL_ADDR(base) ((base) + (0x005C))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ufs_mem_ctrl_d1w2r : 16;
        unsigned int reserved : 16;
    } reg;
} SOC_UFS_Sysctrl_MEMORY_CTRL_UNION;
#endif
#define SOC_UFS_Sysctrl_MEMORY_CTRL_ufs_mem_ctrl_d1w2r_START (0)
#define SOC_UFS_Sysctrl_MEMORY_CTRL_ufs_mem_ctrl_d1w2r_END (15)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ufs_psw_mtcmos_en : 1;
        unsigned int ufs_psw_mtcmos_ack : 1;
        unsigned int reserved_0 : 14;
        unsigned int ufs_psw_iso_ctrl : 1;
        unsigned int reserved_1 : 15;
    } reg;
} SOC_UFS_Sysctrl_PSW_POWER_CTRL_UNION;
#endif
#define SOC_UFS_Sysctrl_PSW_POWER_CTRL_ufs_psw_mtcmos_en_START (0)
#define SOC_UFS_Sysctrl_PSW_POWER_CTRL_ufs_psw_mtcmos_en_END (0)
#define SOC_UFS_Sysctrl_PSW_POWER_CTRL_ufs_psw_mtcmos_ack_START (1)
#define SOC_UFS_Sysctrl_PSW_POWER_CTRL_ufs_psw_mtcmos_ack_END (1)
#define SOC_UFS_Sysctrl_PSW_POWER_CTRL_ufs_psw_iso_ctrl_START (16)
#define SOC_UFS_Sysctrl_PSW_POWER_CTRL_ufs_psw_iso_ctrl_END (16)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ufs_phy_iso_ctrl : 1;
        unsigned int reserved_0 : 15;
        unsigned int ufs_refclk_iso_en : 1;
        unsigned int reserved_1 : 15;
    } reg;
} SOC_UFS_Sysctrl_PHY_ISO_EN_UNION;
#endif
#define SOC_UFS_Sysctrl_PHY_ISO_EN_ufs_phy_iso_ctrl_START (0)
#define SOC_UFS_Sysctrl_PHY_ISO_EN_ufs_phy_iso_ctrl_END (0)
#define SOC_UFS_Sysctrl_PHY_ISO_EN_ufs_refclk_iso_en_START (16)
#define SOC_UFS_Sysctrl_PHY_ISO_EN_ufs_refclk_iso_en_END (16)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_lp_pwr_gate : 1;
        unsigned int reserved_0 : 7;
        unsigned int sysctrl_pwr_ready : 1;
        unsigned int reserved_1 : 7;
        unsigned int sysctrl_lp_isol_en : 1;
        unsigned int reserved_2 : 15;
    } reg;
} SOC_UFS_Sysctrl_HC_LP_CTRL_UNION;
#endif
#define SOC_UFS_Sysctrl_HC_LP_CTRL_sysctrl_lp_pwr_gate_START (0)
#define SOC_UFS_Sysctrl_HC_LP_CTRL_sysctrl_lp_pwr_gate_END (0)
#define SOC_UFS_Sysctrl_HC_LP_CTRL_sysctrl_pwr_ready_START (8)
#define SOC_UFS_Sysctrl_HC_LP_CTRL_sysctrl_pwr_ready_END (8)
#define SOC_UFS_Sysctrl_HC_LP_CTRL_sysctrl_lp_isol_en_START (16)
#define SOC_UFS_Sysctrl_HC_LP_CTRL_sysctrl_lp_isol_en_END (16)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_cfg_clock_freq : 8;
        unsigned int sysctrl_ref_clock_sel : 2;
        unsigned int reserved_0 : 6;
        unsigned int sysctrl_ref_use_pad : 1;
        unsigned int reserved_1 : 7;
        unsigned int sysctrl_ref_clock_en : 1;
        unsigned int reserved_2 : 7;
    } reg;
} SOC_UFS_Sysctrl_PHY_CLK_CTRL_UNION;
#endif
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_sysctrl_cfg_clock_freq_START (0)
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_sysctrl_cfg_clock_freq_END (7)
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_sysctrl_ref_clock_sel_START (8)
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_sysctrl_ref_clock_sel_END (9)
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_sysctrl_ref_use_pad_START (16)
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_sysctrl_ref_use_pad_END (16)
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_sysctrl_ref_clock_en_START (24)
#define SOC_UFS_Sysctrl_PHY_CLK_CTRL_sysctrl_ref_clock_en_END (24)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_cfg_clock_en : 1;
        unsigned int reserved_0 : 3;
        unsigned int sysctrl_psw_clk_en : 1;
        unsigned int reserved_1 : 27;
    } reg;
} SOC_UFS_Sysctrl_PSW_CLK_CTRL_UNION;
#endif
#define SOC_UFS_Sysctrl_PSW_CLK_CTRL_sysctrl_cfg_clock_en_START (0)
#define SOC_UFS_Sysctrl_PSW_CLK_CTRL_sysctrl_cfg_clock_en_END (0)
#define SOC_UFS_Sysctrl_PSW_CLK_CTRL_sysctrl_psw_clk_en_START (4)
#define SOC_UFS_Sysctrl_PSW_CLK_CTRL_sysctrl_psw_clk_en_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_cgsysclk_bypass : 1;
        unsigned int sysctrl_hclk_bypass : 1;
        unsigned int sysctrl_lp_clkt_bypass : 1;
        unsigned int sysctrl_clkt_bypass : 1;
        unsigned int sysctrl_pclk_bypass : 1;
        unsigned int sysctrl_txsymbol_bypass : 1;
        unsigned int reserved : 26;
    } reg;
} SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_UNION;
#endif
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_cgsysclk_bypass_START (0)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_cgsysclk_bypass_END (0)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_hclk_bypass_START (1)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_hclk_bypass_END (1)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_lp_clkt_bypass_START (2)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_lp_clkt_bypass_END (2)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_clkt_bypass_START (3)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_clkt_bypass_END (3)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_pclk_bypass_START (4)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_pclk_bypass_END (4)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_txsymbol_bypass_START (5)
#define SOC_UFS_Sysctrl_CLOCK_GATE_BYPASS_sysctrl_txsymbol_bypass_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_lp_reset_n : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_UFS_Sysctrl_RESET_CTRL_EN_UNION;
#endif
#define SOC_UFS_Sysctrl_RESET_CTRL_EN_sysctrl_lp_reset_n_START (0)
#define SOC_UFS_Sysctrl_RESET_CTRL_EN_sysctrl_lp_reset_n_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_lp_resetcomplete : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_UFS_Sysctrl_PHY_RESET_STATUS_UNION;
#endif
#define SOC_UFS_Sysctrl_PHY_RESET_STATUS_sysctrl_lp_resetcomplete_START (0)
#define SOC_UFS_Sysctrl_PHY_RESET_STATUS_sysctrl_lp_resetcomplete_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_ufshc_fe_interrupt : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_UFS_Sysctrl_HC_DEBUG_UNION;
#endif
#define SOC_UFS_Sysctrl_HC_DEBUG_sysctrl_dbg_ufshc_fe_interrupt_START (0)
#define SOC_UFS_Sysctrl_HC_DEBUG_sysctrl_dbg_ufshc_fe_interrupt_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_rx_tst_rtcontrol_0 : 8;
        unsigned int sysctrl_dbg_rx_tst_rtcontrol_1 : 8;
        unsigned int sysctrl_dbg_tx_tst_rtcontrol_0 : 8;
        unsigned int sysctrl_dbg_tx_tst_rtcontrol_1 : 8;
    } reg;
} SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_UNION;
#endif
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_sysctrl_dbg_rx_tst_rtcontrol_0_START (0)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_sysctrl_dbg_rx_tst_rtcontrol_0_END (7)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_sysctrl_dbg_rx_tst_rtcontrol_1_START (8)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_sysctrl_dbg_rx_tst_rtcontrol_1_END (15)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_sysctrl_dbg_tx_tst_rtcontrol_0_START (16)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_sysctrl_dbg_tx_tst_rtcontrol_0_END (23)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_sysctrl_dbg_tx_tst_rtcontrol_1_START (24)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_CTRL_sysctrl_dbg_tx_tst_rtcontrol_1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_rx_tst_rtobserve_0 : 8;
        unsigned int sysctrl_dbg_rx_tst_rtobserve_1 : 8;
        unsigned int sysctrl_dbg_tx_tst_rtobserve_0 : 8;
        unsigned int sysctrl_dbg_tx_tst_rtobserve_1 : 8;
    } reg;
} SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_UNION;
#endif
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_sysctrl_dbg_rx_tst_rtobserve_0_START (0)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_sysctrl_dbg_rx_tst_rtobserve_0_END (7)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_sysctrl_dbg_rx_tst_rtobserve_1_START (8)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_sysctrl_dbg_rx_tst_rtobserve_1_END (15)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_sysctrl_dbg_tx_tst_rtobserve_0_START (16)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_sysctrl_dbg_tx_tst_rtobserve_0_END (23)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_sysctrl_dbg_tx_tst_rtobserve_1_START (24)
#define SOC_UFS_Sysctrl_PHY_MPX_TEST_OBSV_sysctrl_dbg_tx_tst_rtobserve_1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_phy_dtb_out : 2;
        unsigned int reserved : 30;
    } reg;
} SOC_UFS_Sysctrl_PHY_DTB_OUT_UNION;
#endif
#define SOC_UFS_Sysctrl_PHY_DTB_OUT_sysctrl_dbg_phy_dtb_out_START (0)
#define SOC_UFS_Sysctrl_PHY_DTB_OUT_sysctrl_dbg_phy_dtb_out_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_monitor_hh : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_UFS_Sysctrl_DEBUG_MONITOR_HH_UNION;
#endif
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_HH_sysctrl_dbg_monitor_hh_START (0)
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_HH_sysctrl_dbg_monitor_hh_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_monitor_h : 32;
    } reg;
} SOC_UFS_Sysctrl_DEBUG_MONITOR_H_UNION;
#endif
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_H_sysctrl_dbg_monitor_h_START (0)
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_H_sysctrl_dbg_monitor_h_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_monitor_l : 32;
    } reg;
} SOC_UFS_Sysctrl_DEBUG_MONITOR_L_UNION;
#endif
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_L_sysctrl_dbg_monitor_l_START (0)
#define SOC_UFS_Sysctrl_DEBUG_MONITOR_L_sysctrl_dbg_monitor_l_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_monitorup_h : 32;
    } reg;
} SOC_UFS_Sysctrl_DEBUG_MONITORUP_H_UNION;
#endif
#define SOC_UFS_Sysctrl_DEBUG_MONITORUP_H_sysctrl_dbg_monitorup_h_START (0)
#define SOC_UFS_Sysctrl_DEBUG_MONITORUP_H_sysctrl_dbg_monitorup_h_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_dbg_monitorup_l : 32;
    } reg;
} SOC_UFS_Sysctrl_DEBUG_MONITORUP_L_UNION;
#endif
#define SOC_UFS_Sysctrl_DEBUG_MONITORUP_L_sysctrl_dbg_monitorup_l_START (0)
#define SOC_UFS_Sysctrl_DEBUG_MONITORUP_L_sysctrl_dbg_monitorup_l_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sysctrl_txX_mk2extensionen : 1;
        unsigned int sysctrl_rx_mk2deepexit : 1;
        unsigned int reserved_0 : 6;
        unsigned int reserved_1 : 24;
    } reg;
} SOC_UFS_Sysctrl_MK2_CTRL_UNION;
#endif
#define SOC_UFS_Sysctrl_MK2_CTRL_sysctrl_txX_mk2extensionen_START (0)
#define SOC_UFS_Sysctrl_MK2_CTRL_sysctrl_txX_mk2extensionen_END (0)
#define SOC_UFS_Sysctrl_MK2_CTRL_sysctrl_rx_mk2deepexit_START (1)
#define SOC_UFS_Sysctrl_MK2_CTRL_sysctrl_rx_mk2deepexit_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ufs_refclk_src_sel : 1;
        unsigned int ufs_sysctrl_bakup : 31;
    } reg;
} SOC_UFS_Sysctrl_UFS_SYSCTRL_UNION;
#endif
#define SOC_UFS_Sysctrl_UFS_SYSCTRL_ufs_refclk_src_sel_START (0)
#define SOC_UFS_Sysctrl_UFS_SYSCTRL_ufs_refclk_src_sel_END (0)
#define SOC_UFS_Sysctrl_UFS_SYSCTRL_ufs_sysctrl_bakup_START (1)
#define SOC_UFS_Sysctrl_UFS_SYSCTRL_ufs_sysctrl_bakup_END (31)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
