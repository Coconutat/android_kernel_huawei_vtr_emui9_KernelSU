#ifndef __SOC_SCHARGER_INTERFACE_H__
#define __SOC_SCHARGER_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_SCHARGER_CHIP_VERSION_0_ADDR(base) ((base) + (0x00))
#define SOC_SCHARGER_CHIP_VERSION_1_ADDR(base) ((base) + (0x01))
#define SOC_SCHARGER_CHIP_VERSION_2_ADDR(base) ((base) + (0x02))
#define SOC_SCHARGER_CHIP_VERSION_3_ADDR(base) ((base) + (0x03))
#define SOC_SCHARGER_CHIP_VERSION_4_ADDR(base) ((base) + (0x04))
#define SOC_SCHARGER_CHIP_VERSION_5_ADDR(base) ((base) + (0x05))
#define SOC_SCHARGER_STATUS0_ADDR(base) ((base) + (0x06))
#define SOC_SCHARGER_STATUS1_ADDR(base) ((base) + (0x07))
#define SOC_SCHARGER_STATUS2_ADDR(base) ((base) + (0x08))
#define SOC_SCHARGER_IRQ_STATUS0_ADDR(base) ((base) + (0x09))
#define SOC_SCHARGER_IRQ_STATUS1_ADDR(base) ((base) + (0x0A))
#define SOC_SCHARGER_IRQ_STATUS2_ADDR(base) ((base) + (0x0B))
#define SOC_SCHARGER_IRQ0_ADDR(base) ((base) + (0x0C))
#define SOC_SCHARGER_IRQ1_ADDR(base) ((base) + (0x0D))
#define SOC_SCHARGER_IRQ2_ADDR(base) ((base) + (0x0E))
#define SOC_SCHARGER_WATCHDOG_IRQ_ADDR(base) ((base) + (0x0F))
#define SOC_SCHARGER_IRQ0_MASK_ADDR(base) ((base) + (0x10))
#define SOC_SCHARGER_IRQ1_MASK_ADDR(base) ((base) + (0x11))
#define SOC_SCHARGER_IRQ2_MASK_ADDR(base) ((base) + (0x12))
#define SOC_SCHARGER_WATCHDOG_IRQ_MASK_ADDR(base) ((base) + (0x13))
#define SOC_SCHARGER_STATUIS_ADDR(base) ((base) + (0x20))
#define SOC_SCHARGER_CNTL_ADDR(base) ((base) + (0x21))
#define SOC_SCHARGER_CMD_ADDR(base) ((base) + (0x24))
#define SOC_SCHARGER_ADDR_ADDR(base) ((base) + (0x27))
#define SOC_SCHARGER_DATA0_ADDR(base) ((base) + (0x28))
#define SOC_SCHARGER_ISR1_ADDR(base) ((base) + (0x39))
#define SOC_SCHARGER_ISR2_ADDR(base) ((base) + (0x3A))
#define SOC_SCHARGER_IMR1_ADDR(base) ((base) + (0x3B))
#define SOC_SCHARGER_IMR2_ADDR(base) ((base) + (0x3C))
#define SOC_SCHARGER_FCP_IRQ3_ADDR(base) ((base) + (0x3D))
#define SOC_SCHARGER_FCP_IRQ4_ADDR(base) ((base) + (0x3E))
#define SOC_SCHARGER_FCP_IRQ5_ADDR(base) ((base) + (0x3F))
#define SOC_SCHARGER_FCP_IRQ3_MASK_ADDR(base) ((base) + (0x40))
#define SOC_SCHARGER_FCP_IRQ4_MASK_ADDR(base) ((base) + (0x41))
#define SOC_SCHARGER_FCP_IRQ5_MASK_ADDR(base) ((base) + (0x42))
#define SOC_SCHARGER_FCP_CTRL_ADDR(base) ((base) + (0x43))
#define SOC_SCHARGER_FCP_MODE2_SET_ADDR(base) ((base) + (0x44))
#define SOC_SCHARGER_FCP_ADAP_CTRL_ADDR(base) ((base) + (0x45))
#define SOC_SCHARGER_MSTATE_ADDR(base) ((base) + (0x46))
#define SOC_SCHARGER_FCP_RDATA_ADDR(base) ((base) + (0x47))
#define SOC_SCHARGER_RDATA_READY_ADDR(base) ((base) + (0x48))
#define SOC_SCHARGER_CRC_ENABLE_ADDR(base) ((base) + (0x49))
#define SOC_SCHARGER_CRC_START_VALUE_ADDR(base) ((base) + (0x4A))
#define SOC_SCHARGER_SAMPLE_CNT_ADJ_ADDR(base) ((base) + (0x4B))
#define SOC_SCHARGER_RX_PING_WIDTH_MIN_H_ADDR(base) ((base) + (0x4C))
#define SOC_SCHARGER_RX_PING_WIDTH_MIN_L_ADDR(base) ((base) + (0x4D))
#define SOC_SCHARGER_RX_PING_WIDTH_MAX_H_ADDR(base) ((base) + (0x4E))
#define SOC_SCHARGER_RX_PING_WIDTH_MAX_L_ADDR(base) ((base) + (0x4F))
#define SOC_SCHARGER_DATA_WAIT_TIME_ADDR(base) ((base) + (0x50))
#define SOC_SCHARGER_RETRY_CNT_ADDR(base) ((base) + (0x51))
#define SOC_SCHARGER_FCP_SOFT_RST_CTRL_ADDR(base) ((base) + (0x52))
#define SOC_SCHARGER_FCP_RO_RESERVE_ADDR(base) ((base) + (0x53))
#define SOC_SCHARGER_FCP_DEBUG_REG0_ADDR(base) ((base) + (0x54))
#define SOC_SCHARGER_FCP_DEBUG_REG1_ADDR(base) ((base) + (0x55))
#define SOC_SCHARGER_FCP_DEBUG_REG2_ADDR(base) ((base) + (0x56))
#define SOC_SCHARGER_BUCK_REG0_ADDR(base) ((base) + (0x60))
#define SOC_SCHARGER_BUCK_REG1_ADDR(base) ((base) + (0x61))
#define SOC_SCHARGER_BUCK_REG2_ADDR(base) ((base) + (0x62))
#define SOC_SCHARGER_BUCK_REG3_ADDR(base) ((base) + (0x63))
#define SOC_SCHARGER_BUCK_REG4_ADDR(base) ((base) + (0x64))
#define SOC_SCHARGER_BUCK_REG5_ADDR(base) ((base) + (0x65))
#define SOC_SCHARGER_BUCK_REG6_ADDR(base) ((base) + (0x66))
#define SOC_SCHARGER_BUCK_REG7_ADDR(base) ((base) + (0x67))
#define SOC_SCHARGER_BUCK_REG8_ADDR(base) ((base) + (0x68))
#define SOC_SCHARGER_BUCK_REG9_ADDR(base) ((base) + (0x69))
#define SOC_SCHARGER_BUCK_REG10_ADDR(base) ((base) + (0x6A))
#define SOC_SCHARGER_BUCK_REG11_ADDR(base) ((base) + (0x6B))
#define SOC_SCHARGER_BUCK_REG12_ADDR(base) ((base) + (0x6C))
#define SOC_SCHARGER_BUCK_REG13_ADDR(base) ((base) + (0x6D))
#define SOC_SCHARGER_BUCK_REG14_ADDR(base) ((base) + (0x6E))
#define SOC_SCHARGER_BUCK_REG15_ADDR(base) ((base) + (0x6F))
#define SOC_SCHARGER_BUCK_REG16_ADDR(base) ((base) + (0x70))
#define SOC_SCHARGER_BUCK_RES_SEL0_ADDR(base) ((base) + (0x71))
#define SOC_SCHARGER_BUCK_RES_SEL1_ADDR(base) ((base) + (0x72))
#define SOC_SCHARGER_BUCK_RES_SEL2_ADDR(base) ((base) + (0x73))
#define SOC_SCHARGER_BUCK_RES_SEL3_ADDR(base) ((base) + (0x74))
#define SOC_SCHARGER_BUCK_CAP_SEL0_ADDR(base) ((base) + (0x75))
#define SOC_SCHARGER_BUCK_CAP_SEL1_ADDR(base) ((base) + (0x76))
#define SOC_SCHARGER_BUCK_RESERVE0_ADDR(base) ((base) + (0x77))
#define SOC_SCHARGER_BUCK_RESERVE1_ADDR(base) ((base) + (0x78))
#define SOC_SCHARGER_BUCK_RESERVE2_ADDR(base) ((base) + (0x79))
#define SOC_SCHARGER_BUCK_RESERVE3_ADDR(base) ((base) + (0x7A))
#define SOC_SCHARGER_BUCK_RESERVE0_STATE_ADDR(base) ((base) + (0x7B))
#define SOC_SCHARGER_BUCK_RESERVE1_STATE_ADDR(base) ((base) + (0x7C))
#define SOC_SCHARGER_OTG_REG0_ADDR(base) ((base) + (0x80))
#define SOC_SCHARGER_OTG_REG1_ADDR(base) ((base) + (0x81))
#define SOC_SCHARGER_OTG_REG2_ADDR(base) ((base) + (0x82))
#define SOC_SCHARGER_OTG_REG3_ADDR(base) ((base) + (0x83))
#define SOC_SCHARGER_OTG_REG4_ADDR(base) ((base) + (0x84))
#define SOC_SCHARGER_OTG_REG5_ADDR(base) ((base) + (0x85))
#define SOC_SCHARGER_OTG_REG6_ADDR(base) ((base) + (0x86))
#define SOC_SCHARGER_OTG_REG7_ADDR(base) ((base) + (0x87))
#define SOC_SCHARGER_OTG_REG8_ADDR(base) ((base) + (0x88))
#define SOC_SCHARGER_OTG_REG9_ADDR(base) ((base) + (0x89))
#define SOC_SCHARGER_OTG_TRIM1_ADDR(base) ((base) + (0x8A))
#define SOC_SCHARGER_OTG_TRIM2_ADDR(base) ((base) + (0x8B))
#define SOC_SCHARGER_OTG_RESERVE_ADDR(base) ((base) + (0x8C))
#define SOC_SCHARGER_OTG_RESERVE1_ADDR(base) ((base) + (0x8D))
#define SOC_SCHARGER_OTG_RESERVE2_ADDR(base) ((base) + (0x8E))
#define SOC_SCHARGER_CHG_REG0_ADDR(base) ((base) + (0x90))
#define SOC_SCHARGER_CHG_REG1_ADDR(base) ((base) + (0x91))
#define SOC_SCHARGER_CHG_REG2_ADDR(base) ((base) + (0x92))
#define SOC_SCHARGER_CHG_REG3_ADDR(base) ((base) + (0x93))
#define SOC_SCHARGER_CHG_REG4_ADDR(base) ((base) + (0x94))
#define SOC_SCHARGER_CHG_REG5_ADDR(base) ((base) + (0x95))
#define SOC_SCHARGER_CHG_VRES_SEL_ADDR(base) ((base) + (0x96))
#define SOC_SCHARGER_CHG_CAP_SEL_ADDR(base) ((base) + (0x97))
#define SOC_SCHARGER_CHG_REG6_ADDR(base) ((base) + (0x98))
#define SOC_SCHARGER_CHG_REG7_ADDR(base) ((base) + (0x99))
#define SOC_SCHARGER_CHG_REG8_ADDR(base) ((base) + (0x9A))
#define SOC_SCHARGER_CHG_REG9_ADDR(base) ((base) + (0x9B))
#define SOC_SCHARGER_CHG_RESVI1_ADDR(base) ((base) + (0x9C))
#define SOC_SCHARGER_CHG_RESVI2_ADDR(base) ((base) + (0x9D))
#define SOC_SCHARGER_CHG_RESVO1_ADDR(base) ((base) + (0x9E))
#define SOC_SCHARGER_CHG_RESVO2_ADDR(base) ((base) + (0x9F))
#define SOC_SCHARGER_DET_TOP_REG0_ADDR(base) ((base) + (0xA0))
#define SOC_SCHARGER_DET_TOP_REG1_ADDR(base) ((base) + (0xA1))
#define SOC_SCHARGER_THSD_ADJ_ADDR(base) ((base) + (0xA2))
#define SOC_SCHARGER_SCHG_LOGIC_CTRL_ADDR(base) ((base) + (0xA3))
#define SOC_SCHARGER_BLOCK_CTRL_ADDR(base) ((base) + (0xA4))
#define SOC_SCHARGER_REF_TOP_CTRL_ADDR(base) ((base) + (0xA5))
#define SOC_SCHARGER_ADC_CTRL_ADDR(base) ((base) + (0xB0))
#define SOC_SCHARGER_ADC_START_ADDR(base) ((base) + (0xB1))
#define SOC_SCHARGER_ADC_CONV_STATUS_ADDR(base) ((base) + (0xB2))
#define SOC_SCHARGER_ADC_DATA1_ADDR(base) ((base) + (0xB3))
#define SOC_SCHARGER_ADC_DATA0_ADDR(base) ((base) + (0xB4))
#define SOC_SCHARGER_ADC_IBIAS_SEL_ADDR(base) ((base) + (0xB5))
#define SOC_SCHARGER_EFUSE_REG0_ADDR(base) ((base) + (0xC0))
#define SOC_SCHARGER_EFUSE_REG1_ADDR(base) ((base) + (0xC1))
#define SOC_SCHARGER_EFUSE_WE0_ADDR(base) ((base) + (0xC2))
#define SOC_SCHARGER_EFUSE_WE1_ADDR(base) ((base) + (0xC3))
#define SOC_SCHARGER_EFUSE_WE2_ADDR(base) ((base) + (0xC4))
#define SOC_SCHARGER_EFUSE_WE3_ADDR(base) ((base) + (0xC5))
#define SOC_SCHARGER_EFUSE_WE4_ADDR(base) ((base) + (0xC6))
#define SOC_SCHARGER_EFUSE_WE5_ADDR(base) ((base) + (0xC7))
#define SOC_SCHARGER_EFUSE_WE6_ADDR(base) ((base) + (0xC8))
#define SOC_SCHARGER_EFUSE_WE7_ADDR(base) ((base) + (0xC9))
#define SOC_SCHARGER_EFUSE_PDOB0_ADDR(base) ((base) + (0xCA))
#define SOC_SCHARGER_EFUSE_PDOB1_ADDR(base) ((base) + (0xCB))
#define SOC_SCHARGER_EFUSE_PDOB2_ADDR(base) ((base) + (0xCC))
#define SOC_SCHARGER_EFUSE_PDOB3_ADDR(base) ((base) + (0xCD))
#define SOC_SCHARGER_EFUSE_PDOB4_ADDR(base) ((base) + (0xCE))
#define SOC_SCHARGER_EFUSE_PDOB5_ADDR(base) ((base) + (0xCF))
#define SOC_SCHARGER_EFUSE_PDOB6_ADDR(base) ((base) + (0xD0))
#define SOC_SCHARGER_EFUSE_PDOB7_ADDR(base) ((base) + (0xD1))
#define SOC_SCHARGER_EFUSE_SOFT_RST_CTRL_ADDR(base) ((base) + (0xD2))
#define SOC_SCHARGER_SYS_RESVO1_ADDR(base) ((base) + (0xE0))
#define SOC_SCHARGER_SYS_RESVO2_ADDR(base) ((base) + (0xE1))
#define SOC_SCHARGER_SYS_RESVO3_ADDR(base) ((base) + (0xE2))
#define SOC_SCHARGER_SYS_RESVO4_ADDR(base) ((base) + (0xE3))
#define SOC_SCHARGER_SYS_RESVI1_ADDR(base) ((base) + (0xE4))
#define SOC_SCHARGER_SYS_RESVI2_ADDR(base) ((base) + (0xE5))
#define SOC_SCHARGER_OSC_FCP_ADDR(base) ((base) + (0xE6))
#define SOC_SCHARGER_GLB_SOFT_RST_CTRL_ADDR(base) ((base) + (0xE7))
#define SOC_SCHARGER_WATCHDOG_SOFT_RST_ADDR(base) ((base) + (0xE8))
#define SOC_SCHARGER_WATCHDOG_CTRL_ADDR(base) ((base) + (0xE9))
#define SOC_SCHARGER_CLK_GATE_ADDR(base) ((base) + (0xEA))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chip_id2 : 8;
    } reg;
} SOC_SCHARGER_CHIP_VERSION_0_UNION;
#endif
#define SOC_SCHARGER_CHIP_VERSION_0_chip_id2_START (0)
#define SOC_SCHARGER_CHIP_VERSION_0_chip_id2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chip_id3 : 8;
    } reg;
} SOC_SCHARGER_CHIP_VERSION_1_UNION;
#endif
#define SOC_SCHARGER_CHIP_VERSION_1_chip_id3_START (0)
#define SOC_SCHARGER_CHIP_VERSION_1_chip_id3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chip_id4 : 8;
    } reg;
} SOC_SCHARGER_CHIP_VERSION_2_UNION;
#endif
#define SOC_SCHARGER_CHIP_VERSION_2_chip_id4_START (0)
#define SOC_SCHARGER_CHIP_VERSION_2_chip_id4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chip_id5 : 8;
    } reg;
} SOC_SCHARGER_CHIP_VERSION_3_UNION;
#endif
#define SOC_SCHARGER_CHIP_VERSION_3_chip_id5_START (0)
#define SOC_SCHARGER_CHIP_VERSION_3_chip_id5_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chip_id0 : 8;
    } reg;
} SOC_SCHARGER_CHIP_VERSION_4_UNION;
#endif
#define SOC_SCHARGER_CHIP_VERSION_4_chip_id0_START (0)
#define SOC_SCHARGER_CHIP_VERSION_4_chip_id0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chip_id1 : 8;
    } reg;
} SOC_SCHARGER_CHIP_VERSION_5_UNION;
#endif
#define SOC_SCHARGER_CHIP_VERSION_5_chip_id1_START (0)
#define SOC_SCHARGER_CHIP_VERSION_5_chip_id1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_rechg_state : 1;
        unsigned char chg_therm_state : 1;
        unsigned char chg_dpm_state : 1;
        unsigned char chg_acl_state : 1;
        unsigned char chg_chgstate : 2;
        unsigned char otg_on : 1;
        unsigned char buck_ok : 1;
    } reg;
} SOC_SCHARGER_STATUS0_UNION;
#endif
#define SOC_SCHARGER_STATUS0_chg_rechg_state_START (0)
#define SOC_SCHARGER_STATUS0_chg_rechg_state_END (0)
#define SOC_SCHARGER_STATUS0_chg_therm_state_START (1)
#define SOC_SCHARGER_STATUS0_chg_therm_state_END (1)
#define SOC_SCHARGER_STATUS0_chg_dpm_state_START (2)
#define SOC_SCHARGER_STATUS0_chg_dpm_state_END (2)
#define SOC_SCHARGER_STATUS0_chg_acl_state_START (3)
#define SOC_SCHARGER_STATUS0_chg_acl_state_END (3)
#define SOC_SCHARGER_STATUS0_chg_chgstate_START (4)
#define SOC_SCHARGER_STATUS0_chg_chgstate_END (5)
#define SOC_SCHARGER_STATUS0_otg_on_START (6)
#define SOC_SCHARGER_STATUS0_otg_on_END (6)
#define SOC_SCHARGER_STATUS0_buck_ok_START (7)
#define SOC_SCHARGER_STATUS0_buck_ok_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char batfet_ctrl : 1;
        unsigned char wdt_time_out_n : 1;
        unsigned char otg_en_in : 1;
        unsigned char chg_batfet_ocp : 1;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_STATUS1_UNION;
#endif
#define SOC_SCHARGER_STATUS1_batfet_ctrl_START (0)
#define SOC_SCHARGER_STATUS1_batfet_ctrl_END (0)
#define SOC_SCHARGER_STATUS1_wdt_time_out_n_START (1)
#define SOC_SCHARGER_STATUS1_wdt_time_out_n_END (1)
#define SOC_SCHARGER_STATUS1_otg_en_in_START (2)
#define SOC_SCHARGER_STATUS1_otg_en_in_END (2)
#define SOC_SCHARGER_STATUS1_chg_batfet_ocp_START (3)
#define SOC_SCHARGER_STATUS1_chg_batfet_ocp_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_state : 8;
    } reg;
} SOC_SCHARGER_STATUS2_UNION;
#endif
#define SOC_SCHARGER_STATUS2_otg_state_START (0)
#define SOC_SCHARGER_STATUS2_otg_state_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otmp_140_r : 1;
        unsigned char otg_ovp_r : 1;
        unsigned char otg_uvp_r : 1;
        unsigned char otg_ocp_r : 1;
        unsigned char otg_scp_r : 1;
        unsigned char otmp_jreg_r : 1;
        unsigned char buck_ocp_r : 1;
        unsigned char buck_scp_r : 1;
    } reg;
} SOC_SCHARGER_IRQ_STATUS0_UNION;
#endif
#define SOC_SCHARGER_IRQ_STATUS0_otmp_140_r_START (0)
#define SOC_SCHARGER_IRQ_STATUS0_otmp_140_r_END (0)
#define SOC_SCHARGER_IRQ_STATUS0_otg_ovp_r_START (1)
#define SOC_SCHARGER_IRQ_STATUS0_otg_ovp_r_END (1)
#define SOC_SCHARGER_IRQ_STATUS0_otg_uvp_r_START (2)
#define SOC_SCHARGER_IRQ_STATUS0_otg_uvp_r_END (2)
#define SOC_SCHARGER_IRQ_STATUS0_otg_ocp_r_START (3)
#define SOC_SCHARGER_IRQ_STATUS0_otg_ocp_r_END (3)
#define SOC_SCHARGER_IRQ_STATUS0_otg_scp_r_START (4)
#define SOC_SCHARGER_IRQ_STATUS0_otg_scp_r_END (4)
#define SOC_SCHARGER_IRQ_STATUS0_otmp_jreg_r_START (5)
#define SOC_SCHARGER_IRQ_STATUS0_otmp_jreg_r_END (5)
#define SOC_SCHARGER_IRQ_STATUS0_buck_ocp_r_START (6)
#define SOC_SCHARGER_IRQ_STATUS0_buck_ocp_r_END (6)
#define SOC_SCHARGER_IRQ_STATUS0_buck_scp_r_START (7)
#define SOC_SCHARGER_IRQ_STATUS0_buck_scp_r_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char regn_ocp_r : 1;
        unsigned char vbat_ovp_r : 1;
        unsigned char vbus_uvp_r : 1;
        unsigned char vbus_ovp_r : 1;
        unsigned char sys_ocp_r : 1;
        unsigned char chg_fast_fault_r : 1;
        unsigned char chg_pre_fault_r : 1;
        unsigned char chg_tri_fault_r : 1;
    } reg;
} SOC_SCHARGER_IRQ_STATUS1_UNION;
#endif
#define SOC_SCHARGER_IRQ_STATUS1_regn_ocp_r_START (0)
#define SOC_SCHARGER_IRQ_STATUS1_regn_ocp_r_END (0)
#define SOC_SCHARGER_IRQ_STATUS1_vbat_ovp_r_START (1)
#define SOC_SCHARGER_IRQ_STATUS1_vbat_ovp_r_END (1)
#define SOC_SCHARGER_IRQ_STATUS1_vbus_uvp_r_START (2)
#define SOC_SCHARGER_IRQ_STATUS1_vbus_uvp_r_END (2)
#define SOC_SCHARGER_IRQ_STATUS1_vbus_ovp_r_START (3)
#define SOC_SCHARGER_IRQ_STATUS1_vbus_ovp_r_END (3)
#define SOC_SCHARGER_IRQ_STATUS1_sys_ocp_r_START (4)
#define SOC_SCHARGER_IRQ_STATUS1_sys_ocp_r_END (4)
#define SOC_SCHARGER_IRQ_STATUS1_chg_fast_fault_r_START (5)
#define SOC_SCHARGER_IRQ_STATUS1_chg_fast_fault_r_END (5)
#define SOC_SCHARGER_IRQ_STATUS1_chg_pre_fault_r_START (6)
#define SOC_SCHARGER_IRQ_STATUS1_chg_pre_fault_r_END (6)
#define SOC_SCHARGER_IRQ_STATUS1_chg_tri_fault_r_START (7)
#define SOC_SCHARGER_IRQ_STATUS1_chg_tri_fault_r_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char bat_ltmp_r : 1;
        unsigned char bat_htmp_r : 1;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_IRQ_STATUS2_UNION;
#endif
#define SOC_SCHARGER_IRQ_STATUS2_bat_ltmp_r_START (0)
#define SOC_SCHARGER_IRQ_STATUS2_bat_ltmp_r_END (0)
#define SOC_SCHARGER_IRQ_STATUS2_bat_htmp_r_START (1)
#define SOC_SCHARGER_IRQ_STATUS2_bat_htmp_r_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otmp_140_r : 1;
        unsigned char otg_ovp_r : 1;
        unsigned char otg_uvp_r : 1;
        unsigned char otg_ocp_r : 1;
        unsigned char otg_scp_r : 1;
        unsigned char otmp_jreg_r : 1;
        unsigned char buck_ocp_r : 1;
        unsigned char buck_scp_r : 1;
    } reg;
} SOC_SCHARGER_IRQ0_UNION;
#endif
#define SOC_SCHARGER_IRQ0_otmp_140_r_START (0)
#define SOC_SCHARGER_IRQ0_otmp_140_r_END (0)
#define SOC_SCHARGER_IRQ0_otg_ovp_r_START (1)
#define SOC_SCHARGER_IRQ0_otg_ovp_r_END (1)
#define SOC_SCHARGER_IRQ0_otg_uvp_r_START (2)
#define SOC_SCHARGER_IRQ0_otg_uvp_r_END (2)
#define SOC_SCHARGER_IRQ0_otg_ocp_r_START (3)
#define SOC_SCHARGER_IRQ0_otg_ocp_r_END (3)
#define SOC_SCHARGER_IRQ0_otg_scp_r_START (4)
#define SOC_SCHARGER_IRQ0_otg_scp_r_END (4)
#define SOC_SCHARGER_IRQ0_otmp_jreg_r_START (5)
#define SOC_SCHARGER_IRQ0_otmp_jreg_r_END (5)
#define SOC_SCHARGER_IRQ0_buck_ocp_r_START (6)
#define SOC_SCHARGER_IRQ0_buck_ocp_r_END (6)
#define SOC_SCHARGER_IRQ0_buck_scp_r_START (7)
#define SOC_SCHARGER_IRQ0_buck_scp_r_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char regn_ocp_r : 1;
        unsigned char vbat_ovp_r : 1;
        unsigned char vbus_uvp_r : 1;
        unsigned char vbus_ovp_r : 1;
        unsigned char sys_ocp_r : 1;
        unsigned char chg_fast_fault_r : 1;
        unsigned char chg_pre_fault_r : 1;
        unsigned char chg_tri_fault_r : 1;
    } reg;
} SOC_SCHARGER_IRQ1_UNION;
#endif
#define SOC_SCHARGER_IRQ1_regn_ocp_r_START (0)
#define SOC_SCHARGER_IRQ1_regn_ocp_r_END (0)
#define SOC_SCHARGER_IRQ1_vbat_ovp_r_START (1)
#define SOC_SCHARGER_IRQ1_vbat_ovp_r_END (1)
#define SOC_SCHARGER_IRQ1_vbus_uvp_r_START (2)
#define SOC_SCHARGER_IRQ1_vbus_uvp_r_END (2)
#define SOC_SCHARGER_IRQ1_vbus_ovp_r_START (3)
#define SOC_SCHARGER_IRQ1_vbus_ovp_r_END (3)
#define SOC_SCHARGER_IRQ1_sys_ocp_r_START (4)
#define SOC_SCHARGER_IRQ1_sys_ocp_r_END (4)
#define SOC_SCHARGER_IRQ1_chg_fast_fault_r_START (5)
#define SOC_SCHARGER_IRQ1_chg_fast_fault_r_END (5)
#define SOC_SCHARGER_IRQ1_chg_pre_fault_r_START (6)
#define SOC_SCHARGER_IRQ1_chg_pre_fault_r_END (6)
#define SOC_SCHARGER_IRQ1_chg_tri_fault_r_START (7)
#define SOC_SCHARGER_IRQ1_chg_tri_fault_r_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char bat_ltmp_r : 1;
        unsigned char bat_htmp_r : 1;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_IRQ2_UNION;
#endif
#define SOC_SCHARGER_IRQ2_bat_ltmp_r_START (0)
#define SOC_SCHARGER_IRQ2_bat_ltmp_r_END (0)
#define SOC_SCHARGER_IRQ2_bat_htmp_r_START (1)
#define SOC_SCHARGER_IRQ2_bat_htmp_r_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char wdt_irq : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_WATCHDOG_IRQ_UNION;
#endif
#define SOC_SCHARGER_WATCHDOG_IRQ_wdt_irq_START (0)
#define SOC_SCHARGER_WATCHDOG_IRQ_wdt_irq_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otmp_140_mk : 1;
        unsigned char otg_ovp_mk : 1;
        unsigned char otg_uvp_mk : 1;
        unsigned char otg_ocp_mk : 1;
        unsigned char otg_scp_mk : 1;
        unsigned char otmp_jreg_mk : 1;
        unsigned char buck_ocp_mk : 1;
        unsigned char buck_scp_mk : 1;
    } reg;
} SOC_SCHARGER_IRQ0_MASK_UNION;
#endif
#define SOC_SCHARGER_IRQ0_MASK_otmp_140_mk_START (0)
#define SOC_SCHARGER_IRQ0_MASK_otmp_140_mk_END (0)
#define SOC_SCHARGER_IRQ0_MASK_otg_ovp_mk_START (1)
#define SOC_SCHARGER_IRQ0_MASK_otg_ovp_mk_END (1)
#define SOC_SCHARGER_IRQ0_MASK_otg_uvp_mk_START (2)
#define SOC_SCHARGER_IRQ0_MASK_otg_uvp_mk_END (2)
#define SOC_SCHARGER_IRQ0_MASK_otg_ocp_mk_START (3)
#define SOC_SCHARGER_IRQ0_MASK_otg_ocp_mk_END (3)
#define SOC_SCHARGER_IRQ0_MASK_otg_scp_mk_START (4)
#define SOC_SCHARGER_IRQ0_MASK_otg_scp_mk_END (4)
#define SOC_SCHARGER_IRQ0_MASK_otmp_jreg_mk_START (5)
#define SOC_SCHARGER_IRQ0_MASK_otmp_jreg_mk_END (5)
#define SOC_SCHARGER_IRQ0_MASK_buck_ocp_mk_START (6)
#define SOC_SCHARGER_IRQ0_MASK_buck_ocp_mk_END (6)
#define SOC_SCHARGER_IRQ0_MASK_buck_scp_mk_START (7)
#define SOC_SCHARGER_IRQ0_MASK_buck_scp_mk_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char regn_ocp_mk : 1;
        unsigned char vbat_ovp_mk : 1;
        unsigned char vbus_uvp_mk : 1;
        unsigned char vbus_ovp_mk : 1;
        unsigned char sys_ocp_mk : 1;
        unsigned char chg_fast_fault_mk : 1;
        unsigned char chg_pre_fault_mk : 1;
        unsigned char chg_tri_fault_mk : 1;
    } reg;
} SOC_SCHARGER_IRQ1_MASK_UNION;
#endif
#define SOC_SCHARGER_IRQ1_MASK_regn_ocp_mk_START (0)
#define SOC_SCHARGER_IRQ1_MASK_regn_ocp_mk_END (0)
#define SOC_SCHARGER_IRQ1_MASK_vbat_ovp_mk_START (1)
#define SOC_SCHARGER_IRQ1_MASK_vbat_ovp_mk_END (1)
#define SOC_SCHARGER_IRQ1_MASK_vbus_uvp_mk_START (2)
#define SOC_SCHARGER_IRQ1_MASK_vbus_uvp_mk_END (2)
#define SOC_SCHARGER_IRQ1_MASK_vbus_ovp_mk_START (3)
#define SOC_SCHARGER_IRQ1_MASK_vbus_ovp_mk_END (3)
#define SOC_SCHARGER_IRQ1_MASK_sys_ocp_mk_START (4)
#define SOC_SCHARGER_IRQ1_MASK_sys_ocp_mk_END (4)
#define SOC_SCHARGER_IRQ1_MASK_chg_fast_fault_mk_START (5)
#define SOC_SCHARGER_IRQ1_MASK_chg_fast_fault_mk_END (5)
#define SOC_SCHARGER_IRQ1_MASK_chg_pre_fault_mk_START (6)
#define SOC_SCHARGER_IRQ1_MASK_chg_pre_fault_mk_END (6)
#define SOC_SCHARGER_IRQ1_MASK_chg_tri_fault_mk_START (7)
#define SOC_SCHARGER_IRQ1_MASK_chg_tri_fault_mk_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char bat_ltmp_mk : 1;
        unsigned char bat_htmp_mk : 1;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_IRQ2_MASK_UNION;
#endif
#define SOC_SCHARGER_IRQ2_MASK_bat_ltmp_mk_START (0)
#define SOC_SCHARGER_IRQ2_MASK_bat_ltmp_mk_END (0)
#define SOC_SCHARGER_IRQ2_MASK_bat_htmp_mk_START (1)
#define SOC_SCHARGER_IRQ2_MASK_bat_htmp_mk_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char wdt_irq_mk : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_WATCHDOG_IRQ_MASK_UNION;
#endif
#define SOC_SCHARGER_WATCHDOG_IRQ_MASK_wdt_irq_mk_START (0)
#define SOC_SCHARGER_WATCHDOG_IRQ_MASK_wdt_irq_mk_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char attach : 1;
        unsigned char reserved : 5;
        unsigned char dvc : 2;
    } reg;
} SOC_SCHARGER_STATUIS_UNION;
#endif
#define SOC_SCHARGER_STATUIS_attach_START (0)
#define SOC_SCHARGER_STATUIS_attach_END (0)
#define SOC_SCHARGER_STATUIS_dvc_START (6)
#define SOC_SCHARGER_STATUIS_dvc_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sndcmd : 1;
        unsigned char reserved_0: 1;
        unsigned char mstr_rst : 1;
        unsigned char enable : 1;
        unsigned char reserved_1: 4;
    } reg;
} SOC_SCHARGER_CNTL_UNION;
#endif
#define SOC_SCHARGER_CNTL_sndcmd_START (0)
#define SOC_SCHARGER_CNTL_sndcmd_END (0)
#define SOC_SCHARGER_CNTL_mstr_rst_START (2)
#define SOC_SCHARGER_CNTL_mstr_rst_END (2)
#define SOC_SCHARGER_CNTL_enable_START (3)
#define SOC_SCHARGER_CNTL_enable_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cmd : 8;
    } reg;
} SOC_SCHARGER_CMD_UNION;
#endif
#define SOC_SCHARGER_CMD_cmd_START (0)
#define SOC_SCHARGER_CMD_cmd_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char addr : 8;
    } reg;
} SOC_SCHARGER_ADDR_UNION;
#endif
#define SOC_SCHARGER_ADDR_addr_START (0)
#define SOC_SCHARGER_ADDR_addr_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char data0 : 8;
    } reg;
} SOC_SCHARGER_DATA0_UNION;
#endif
#define SOC_SCHARGER_DATA0_data0_START (0)
#define SOC_SCHARGER_DATA0_data0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reserved_0: 3;
        unsigned char crcpar : 1;
        unsigned char nack : 1;
        unsigned char reserved_1: 1;
        unsigned char ack : 1;
        unsigned char cmdcpl : 1;
    } reg;
} SOC_SCHARGER_ISR1_UNION;
#endif
#define SOC_SCHARGER_ISR1_crcpar_START (3)
#define SOC_SCHARGER_ISR1_crcpar_END (3)
#define SOC_SCHARGER_ISR1_nack_START (4)
#define SOC_SCHARGER_ISR1_nack_END (4)
#define SOC_SCHARGER_ISR1_ack_START (6)
#define SOC_SCHARGER_ISR1_ack_END (6)
#define SOC_SCHARGER_ISR1_cmdcpl_START (7)
#define SOC_SCHARGER_ISR1_cmdcpl_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reserved_0: 1;
        unsigned char protstat : 1;
        unsigned char reserved_1: 1;
        unsigned char parrx : 1;
        unsigned char crcrx : 1;
        unsigned char reserved_2: 3;
    } reg;
} SOC_SCHARGER_ISR2_UNION;
#endif
#define SOC_SCHARGER_ISR2_protstat_START (1)
#define SOC_SCHARGER_ISR2_protstat_END (1)
#define SOC_SCHARGER_ISR2_parrx_START (3)
#define SOC_SCHARGER_ISR2_parrx_END (3)
#define SOC_SCHARGER_ISR2_crcrx_START (4)
#define SOC_SCHARGER_ISR2_crcrx_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reserved_0: 3;
        unsigned char crcpar_mk : 1;
        unsigned char nack_mk : 1;
        unsigned char reserved_1: 1;
        unsigned char ack_mk : 1;
        unsigned char cmdcpl_mk : 1;
    } reg;
} SOC_SCHARGER_IMR1_UNION;
#endif
#define SOC_SCHARGER_IMR1_crcpar_mk_START (3)
#define SOC_SCHARGER_IMR1_crcpar_mk_END (3)
#define SOC_SCHARGER_IMR1_nack_mk_START (4)
#define SOC_SCHARGER_IMR1_nack_mk_END (4)
#define SOC_SCHARGER_IMR1_ack_mk_START (6)
#define SOC_SCHARGER_IMR1_ack_mk_END (6)
#define SOC_SCHARGER_IMR1_cmdcpl_mk_START (7)
#define SOC_SCHARGER_IMR1_cmdcpl_mk_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reserved_0 : 1;
        unsigned char protstat_mk : 1;
        unsigned char reserved_1 : 1;
        unsigned char parrx_mk : 1;
        unsigned char crcrx_mk : 1;
        unsigned char reserved_2 : 3;
    } reg;
} SOC_SCHARGER_IMR2_UNION;
#endif
#define SOC_SCHARGER_IMR2_protstat_mk_START (1)
#define SOC_SCHARGER_IMR2_protstat_mk_END (1)
#define SOC_SCHARGER_IMR2_parrx_mk_START (3)
#define SOC_SCHARGER_IMR2_parrx_mk_END (3)
#define SOC_SCHARGER_IMR2_crcrx_mk_START (4)
#define SOC_SCHARGER_IMR2_crcrx_mk_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char last_hand_fail_irq : 1;
        unsigned char tail_hand_fail_irq : 1;
        unsigned char trans_hand_fail_irq : 1;
        unsigned char init_hand_fail_irq : 1;
        unsigned char rx_data_det_irq : 1;
        unsigned char rx_head_det_irq : 1;
        unsigned char cmd_err_irq : 1;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_FCP_IRQ3_UNION;
#endif
#define SOC_SCHARGER_FCP_IRQ3_last_hand_fail_irq_START (0)
#define SOC_SCHARGER_FCP_IRQ3_last_hand_fail_irq_END (0)
#define SOC_SCHARGER_FCP_IRQ3_tail_hand_fail_irq_START (1)
#define SOC_SCHARGER_FCP_IRQ3_tail_hand_fail_irq_END (1)
#define SOC_SCHARGER_FCP_IRQ3_trans_hand_fail_irq_START (2)
#define SOC_SCHARGER_FCP_IRQ3_trans_hand_fail_irq_END (2)
#define SOC_SCHARGER_FCP_IRQ3_init_hand_fail_irq_START (3)
#define SOC_SCHARGER_FCP_IRQ3_init_hand_fail_irq_END (3)
#define SOC_SCHARGER_FCP_IRQ3_rx_data_det_irq_START (4)
#define SOC_SCHARGER_FCP_IRQ3_rx_data_det_irq_END (4)
#define SOC_SCHARGER_FCP_IRQ3_rx_head_det_irq_START (5)
#define SOC_SCHARGER_FCP_IRQ3_rx_head_det_irq_END (5)
#define SOC_SCHARGER_FCP_IRQ3_cmd_err_irq_START (6)
#define SOC_SCHARGER_FCP_IRQ3_cmd_err_irq_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char last_hand_no_respond_irq : 1;
        unsigned char tail_hand_no_respond_irq : 1;
        unsigned char trans_hand_no_respond_irq : 1;
        unsigned char init_hand_no_respond_irq : 1;
        unsigned char enable_hand_fail_irq : 1;
        unsigned char enable_hand_no_respond_irq : 1;
        unsigned char reserved : 2;
    } reg;
} SOC_SCHARGER_FCP_IRQ4_UNION;
#endif
#define SOC_SCHARGER_FCP_IRQ4_last_hand_no_respond_irq_START (0)
#define SOC_SCHARGER_FCP_IRQ4_last_hand_no_respond_irq_END (0)
#define SOC_SCHARGER_FCP_IRQ4_tail_hand_no_respond_irq_START (1)
#define SOC_SCHARGER_FCP_IRQ4_tail_hand_no_respond_irq_END (1)
#define SOC_SCHARGER_FCP_IRQ4_trans_hand_no_respond_irq_START (2)
#define SOC_SCHARGER_FCP_IRQ4_trans_hand_no_respond_irq_END (2)
#define SOC_SCHARGER_FCP_IRQ4_init_hand_no_respond_irq_START (3)
#define SOC_SCHARGER_FCP_IRQ4_init_hand_no_respond_irq_END (3)
#define SOC_SCHARGER_FCP_IRQ4_enable_hand_fail_irq_START (4)
#define SOC_SCHARGER_FCP_IRQ4_enable_hand_fail_irq_END (4)
#define SOC_SCHARGER_FCP_IRQ4_enable_hand_no_respond_irq_START (5)
#define SOC_SCHARGER_FCP_IRQ4_enable_hand_no_respond_irq_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_set_d60m_r : 1;
        unsigned char fcp_en_det_int : 1;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_FCP_IRQ5_UNION;
#endif
#define SOC_SCHARGER_FCP_IRQ5_fcp_set_d60m_r_START (0)
#define SOC_SCHARGER_FCP_IRQ5_fcp_set_d60m_r_END (0)
#define SOC_SCHARGER_FCP_IRQ5_fcp_en_det_int_START (1)
#define SOC_SCHARGER_FCP_IRQ5_fcp_en_det_int_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char last_hand_fail_irq_mk : 1;
        unsigned char tail_hand_fail_irq_mk : 1;
        unsigned char trans_hand_fail_irq_mk : 1;
        unsigned char init_hand_fail_irq_mk : 1;
        unsigned char rx_data_det_irq_mk : 1;
        unsigned char rx_head_det_irq_mk : 1;
        unsigned char cmd_err_irq_mk : 1;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_FCP_IRQ3_MASK_UNION;
#endif
#define SOC_SCHARGER_FCP_IRQ3_MASK_last_hand_fail_irq_mk_START (0)
#define SOC_SCHARGER_FCP_IRQ3_MASK_last_hand_fail_irq_mk_END (0)
#define SOC_SCHARGER_FCP_IRQ3_MASK_tail_hand_fail_irq_mk_START (1)
#define SOC_SCHARGER_FCP_IRQ3_MASK_tail_hand_fail_irq_mk_END (1)
#define SOC_SCHARGER_FCP_IRQ3_MASK_trans_hand_fail_irq_mk_START (2)
#define SOC_SCHARGER_FCP_IRQ3_MASK_trans_hand_fail_irq_mk_END (2)
#define SOC_SCHARGER_FCP_IRQ3_MASK_init_hand_fail_irq_mk_START (3)
#define SOC_SCHARGER_FCP_IRQ3_MASK_init_hand_fail_irq_mk_END (3)
#define SOC_SCHARGER_FCP_IRQ3_MASK_rx_data_det_irq_mk_START (4)
#define SOC_SCHARGER_FCP_IRQ3_MASK_rx_data_det_irq_mk_END (4)
#define SOC_SCHARGER_FCP_IRQ3_MASK_rx_head_det_irq_mk_START (5)
#define SOC_SCHARGER_FCP_IRQ3_MASK_rx_head_det_irq_mk_END (5)
#define SOC_SCHARGER_FCP_IRQ3_MASK_cmd_err_irq_mk_START (6)
#define SOC_SCHARGER_FCP_IRQ3_MASK_cmd_err_irq_mk_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char last_hand_no_respond_irq_mk : 1;
        unsigned char tail_hand_no_respond_irq_mk : 1;
        unsigned char trans_hand_no_respond_irq_mk : 1;
        unsigned char init_hand_no_respond_irq_mk : 1;
        unsigned char enable_hand_fail_irq_mk : 1;
        unsigned char enable_hand_no_respond_irq_mk : 1;
        unsigned char reserved : 2;
    } reg;
} SOC_SCHARGER_FCP_IRQ4_MASK_UNION;
#endif
#define SOC_SCHARGER_FCP_IRQ4_MASK_last_hand_no_respond_irq_mk_START (0)
#define SOC_SCHARGER_FCP_IRQ4_MASK_last_hand_no_respond_irq_mk_END (0)
#define SOC_SCHARGER_FCP_IRQ4_MASK_tail_hand_no_respond_irq_mk_START (1)
#define SOC_SCHARGER_FCP_IRQ4_MASK_tail_hand_no_respond_irq_mk_END (1)
#define SOC_SCHARGER_FCP_IRQ4_MASK_trans_hand_no_respond_irq_mk_START (2)
#define SOC_SCHARGER_FCP_IRQ4_MASK_trans_hand_no_respond_irq_mk_END (2)
#define SOC_SCHARGER_FCP_IRQ4_MASK_init_hand_no_respond_irq_mk_START (3)
#define SOC_SCHARGER_FCP_IRQ4_MASK_init_hand_no_respond_irq_mk_END (3)
#define SOC_SCHARGER_FCP_IRQ4_MASK_enable_hand_fail_irq_mk_START (4)
#define SOC_SCHARGER_FCP_IRQ4_MASK_enable_hand_fail_irq_mk_END (4)
#define SOC_SCHARGER_FCP_IRQ4_MASK_enable_hand_no_respond_irq_mk_START (5)
#define SOC_SCHARGER_FCP_IRQ4_MASK_enable_hand_no_respond_irq_mk_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_set_d60m_r_mk : 1;
        unsigned char fcp_en_det_int_mk : 1;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_FCP_IRQ5_MASK_UNION;
#endif
#define SOC_SCHARGER_FCP_IRQ5_MASK_fcp_set_d60m_r_mk_START (0)
#define SOC_SCHARGER_FCP_IRQ5_MASK_fcp_set_d60m_r_mk_END (0)
#define SOC_SCHARGER_FCP_IRQ5_MASK_fcp_en_det_int_mk_START (1)
#define SOC_SCHARGER_FCP_IRQ5_MASK_fcp_en_det_int_mk_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_clk_test : 1;
        unsigned char fcp_mode : 1;
        unsigned char fcp_cmp_en : 1;
        unsigned char fcp_det_en : 1;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_FCP_CTRL_UNION;
#endif
#define SOC_SCHARGER_FCP_CTRL_fcp_clk_test_START (0)
#define SOC_SCHARGER_FCP_CTRL_fcp_clk_test_END (0)
#define SOC_SCHARGER_FCP_CTRL_fcp_mode_START (1)
#define SOC_SCHARGER_FCP_CTRL_fcp_mode_END (1)
#define SOC_SCHARGER_FCP_CTRL_fcp_cmp_en_START (2)
#define SOC_SCHARGER_FCP_CTRL_fcp_cmp_en_END (2)
#define SOC_SCHARGER_FCP_CTRL_fcp_det_en_START (3)
#define SOC_SCHARGER_FCP_CTRL_fcp_det_en_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_mod2_set : 2;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_FCP_MODE2_SET_UNION;
#endif
#define SOC_SCHARGER_FCP_MODE2_SET_fcp_mod2_set_START (0)
#define SOC_SCHARGER_FCP_MODE2_SET_fcp_mod2_set_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_set_d60m_r : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_FCP_ADAP_CTRL_UNION;
#endif
#define SOC_SCHARGER_FCP_ADAP_CTRL_fcp_set_d60m_r_START (0)
#define SOC_SCHARGER_FCP_ADAP_CTRL_fcp_set_d60m_r_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char mstate : 4;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_MSTATE_UNION;
#endif
#define SOC_SCHARGER_MSTATE_mstate_START (0)
#define SOC_SCHARGER_MSTATE_mstate_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_rdata : 8;
    } reg;
} SOC_SCHARGER_FCP_RDATA_UNION;
#endif
#define SOC_SCHARGER_FCP_RDATA_fcp_rdata_START (0)
#define SOC_SCHARGER_FCP_RDATA_fcp_rdata_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_rdata_ready : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_RDATA_READY_UNION;
#endif
#define SOC_SCHARGER_RDATA_READY_fcp_rdata_ready_START (0)
#define SOC_SCHARGER_RDATA_READY_fcp_rdata_ready_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char crc_en : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_CRC_ENABLE_UNION;
#endif
#define SOC_SCHARGER_CRC_ENABLE_crc_en_START (0)
#define SOC_SCHARGER_CRC_ENABLE_crc_en_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char crc_start_val : 8;
    } reg;
} SOC_SCHARGER_CRC_START_VALUE_UNION;
#endif
#define SOC_SCHARGER_CRC_START_VALUE_crc_start_val_START (0)
#define SOC_SCHARGER_CRC_START_VALUE_crc_start_val_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sample_cnt_adjust : 5;
        unsigned char reserved : 3;
    } reg;
} SOC_SCHARGER_SAMPLE_CNT_ADJ_UNION;
#endif
#define SOC_SCHARGER_SAMPLE_CNT_ADJ_sample_cnt_adjust_START (0)
#define SOC_SCHARGER_SAMPLE_CNT_ADJ_sample_cnt_adjust_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rx_ping_width_min_h : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_RX_PING_WIDTH_MIN_H_UNION;
#endif
#define SOC_SCHARGER_RX_PING_WIDTH_MIN_H_rx_ping_width_min_h_START (0)
#define SOC_SCHARGER_RX_PING_WIDTH_MIN_H_rx_ping_width_min_h_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rx_ping_width_min_l : 8;
    } reg;
} SOC_SCHARGER_RX_PING_WIDTH_MIN_L_UNION;
#endif
#define SOC_SCHARGER_RX_PING_WIDTH_MIN_L_rx_ping_width_min_l_START (0)
#define SOC_SCHARGER_RX_PING_WIDTH_MIN_L_rx_ping_width_min_l_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rx_ping_width_max_h : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_RX_PING_WIDTH_MAX_H_UNION;
#endif
#define SOC_SCHARGER_RX_PING_WIDTH_MAX_H_rx_ping_width_max_h_START (0)
#define SOC_SCHARGER_RX_PING_WIDTH_MAX_H_rx_ping_width_max_h_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rx_ping_width_max_l : 8;
    } reg;
} SOC_SCHARGER_RX_PING_WIDTH_MAX_L_UNION;
#endif
#define SOC_SCHARGER_RX_PING_WIDTH_MAX_L_rx_ping_width_max_l_START (0)
#define SOC_SCHARGER_RX_PING_WIDTH_MAX_L_rx_ping_width_max_l_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char data_wait_time : 7;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_DATA_WAIT_TIME_UNION;
#endif
#define SOC_SCHARGER_DATA_WAIT_TIME_data_wait_time_START (0)
#define SOC_SCHARGER_DATA_WAIT_TIME_data_wait_time_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cmd_retry_config : 4;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_RETRY_CNT_UNION;
#endif
#define SOC_SCHARGER_RETRY_CNT_cmd_retry_config_START (0)
#define SOC_SCHARGER_RETRY_CNT_cmd_retry_config_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_soft_rst_cfg : 8;
    } reg;
} SOC_SCHARGER_FCP_SOFT_RST_CTRL_UNION;
#endif
#define SOC_SCHARGER_FCP_SOFT_RST_CTRL_fcp_soft_rst_cfg_START (0)
#define SOC_SCHARGER_FCP_SOFT_RST_CTRL_fcp_soft_rst_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char fcp_ro_reserve : 8;
    } reg;
} SOC_SCHARGER_FCP_RO_RESERVE_UNION;
#endif
#define SOC_SCHARGER_FCP_RO_RESERVE_fcp_ro_reserve_START (0)
#define SOC_SCHARGER_FCP_RO_RESERVE_fcp_ro_reserve_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char slv_crc_err : 1;
        unsigned char resp_ack_parity_err : 1;
        unsigned char rdata_parity_err : 1;
        unsigned char slv_crc_parity_err : 1;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_FCP_DEBUG_REG0_UNION;
#endif
#define SOC_SCHARGER_FCP_DEBUG_REG0_slv_crc_err_START (0)
#define SOC_SCHARGER_FCP_DEBUG_REG0_slv_crc_err_END (0)
#define SOC_SCHARGER_FCP_DEBUG_REG0_resp_ack_parity_err_START (1)
#define SOC_SCHARGER_FCP_DEBUG_REG0_resp_ack_parity_err_END (1)
#define SOC_SCHARGER_FCP_DEBUG_REG0_rdata_parity_err_START (2)
#define SOC_SCHARGER_FCP_DEBUG_REG0_rdata_parity_err_END (2)
#define SOC_SCHARGER_FCP_DEBUG_REG0_slv_crc_parity_err_START (3)
#define SOC_SCHARGER_FCP_DEBUG_REG0_slv_crc_parity_err_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char resp_ack : 8;
    } reg;
} SOC_SCHARGER_FCP_DEBUG_REG1_UNION;
#endif
#define SOC_SCHARGER_FCP_DEBUG_REG1_resp_ack_START (0)
#define SOC_SCHARGER_FCP_DEBUG_REG1_resp_ack_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char slv_crc : 8;
    } reg;
} SOC_SCHARGER_FCP_DEBUG_REG2_UNION;
#endif
#define SOC_SCHARGER_FCP_DEBUG_REG2_slv_crc_START (0)
#define SOC_SCHARGER_FCP_DEBUG_REG2_slv_crc_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_scp_vset : 2;
        unsigned char buck_ilimit : 5;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_BUCK_REG0_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG0_buck_scp_vset_START (0)
#define SOC_SCHARGER_BUCK_REG0_buck_scp_vset_END (1)
#define SOC_SCHARGER_BUCK_REG0_buck_ilimit_START (2)
#define SOC_SCHARGER_BUCK_REG0_buck_ilimit_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_ocp_en_n : 1;
        unsigned char buck_pfm_en : 1;
        unsigned char buck_scp_dis : 1;
        unsigned char reserved : 5;
    } reg;
} SOC_SCHARGER_BUCK_REG1_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG1_buck_ocp_en_n_START (0)
#define SOC_SCHARGER_BUCK_REG1_buck_ocp_en_n_END (0)
#define SOC_SCHARGER_BUCK_REG1_buck_pfm_en_START (1)
#define SOC_SCHARGER_BUCK_REG1_buck_pfm_en_END (1)
#define SOC_SCHARGER_BUCK_REG1_buck_scp_dis_START (2)
#define SOC_SCHARGER_BUCK_REG1_buck_scp_dis_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_ocp_sel : 2;
        unsigned char buck_dpm_sel : 3;
        unsigned char buck_sysmin_sel : 2;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_BUCK_REG2_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG2_buck_ocp_sel_START (0)
#define SOC_SCHARGER_BUCK_REG2_buck_ocp_sel_END (1)
#define SOC_SCHARGER_BUCK_REG2_buck_dpm_sel_START (2)
#define SOC_SCHARGER_BUCK_REG2_buck_dpm_sel_END (4)
#define SOC_SCHARGER_BUCK_REG2_buck_sysmin_sel_START (5)
#define SOC_SCHARGER_BUCK_REG2_buck_sysmin_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_gap : 3;
        unsigned char buck_ilimit_k : 5;
    } reg;
} SOC_SCHARGER_BUCK_REG3_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG3_buck_gap_START (0)
#define SOC_SCHARGER_BUCK_REG3_buck_gap_END (2)
#define SOC_SCHARGER_BUCK_REG3_buck_ilimit_k_START (3)
#define SOC_SCHARGER_BUCK_REG3_buck_ilimit_k_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_osc_otp_reg : 3;
        unsigned char buck_osc_frq : 4;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_BUCK_REG4_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG4_buck_osc_otp_reg_START (0)
#define SOC_SCHARGER_BUCK_REG4_buck_osc_otp_reg_END (2)
#define SOC_SCHARGER_BUCK_REG4_buck_osc_frq_START (3)
#define SOC_SCHARGER_BUCK_REG4_buck_osc_frq_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_fullduty_en : 1;
        unsigned char buck_block_ctrl : 3;
        unsigned char buck_saw_peak_adj : 2;
        unsigned char buck_saw_vally_adj : 1;
        unsigned char buck_syn_en : 1;
    } reg;
} SOC_SCHARGER_BUCK_REG5_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG5_buck_fullduty_en_START (0)
#define SOC_SCHARGER_BUCK_REG5_buck_fullduty_en_END (0)
#define SOC_SCHARGER_BUCK_REG5_buck_block_ctrl_START (1)
#define SOC_SCHARGER_BUCK_REG5_buck_block_ctrl_END (3)
#define SOC_SCHARGER_BUCK_REG5_buck_saw_peak_adj_START (4)
#define SOC_SCHARGER_BUCK_REG5_buck_saw_peak_adj_END (5)
#define SOC_SCHARGER_BUCK_REG5_buck_saw_vally_adj_START (6)
#define SOC_SCHARGER_BUCK_REG5_buck_saw_vally_adj_END (6)
#define SOC_SCHARGER_BUCK_REG5_buck_syn_en_START (7)
#define SOC_SCHARGER_BUCK_REG5_buck_syn_en_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_fullduty_offtime : 2;
        unsigned char buck_min_offtime_sel : 1;
        unsigned char buck_min_ontime_sel : 1;
        unsigned char buck_min_offtime : 2;
        unsigned char buck_min_ontime : 2;
    } reg;
} SOC_SCHARGER_BUCK_REG6_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG6_buck_fullduty_offtime_START (0)
#define SOC_SCHARGER_BUCK_REG6_buck_fullduty_offtime_END (1)
#define SOC_SCHARGER_BUCK_REG6_buck_min_offtime_sel_START (2)
#define SOC_SCHARGER_BUCK_REG6_buck_min_offtime_sel_END (2)
#define SOC_SCHARGER_BUCK_REG6_buck_min_ontime_sel_START (3)
#define SOC_SCHARGER_BUCK_REG6_buck_min_ontime_sel_END (3)
#define SOC_SCHARGER_BUCK_REG6_buck_min_offtime_START (4)
#define SOC_SCHARGER_BUCK_REG6_buck_min_offtime_END (5)
#define SOC_SCHARGER_BUCK_REG6_buck_min_ontime_START (6)
#define SOC_SCHARGER_BUCK_REG6_buck_min_ontime_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_pfm_in : 2;
        unsigned char buck_pfm_ibias_adj : 2;
        unsigned char buck_fullduty_delaytime : 4;
    } reg;
} SOC_SCHARGER_BUCK_REG7_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG7_buck_pfm_in_START (0)
#define SOC_SCHARGER_BUCK_REG7_buck_pfm_in_END (1)
#define SOC_SCHARGER_BUCK_REG7_buck_pfm_ibias_adj_START (2)
#define SOC_SCHARGER_BUCK_REG7_buck_pfm_ibias_adj_END (3)
#define SOC_SCHARGER_BUCK_REG7_buck_fullduty_delaytime_START (4)
#define SOC_SCHARGER_BUCK_REG7_buck_fullduty_delaytime_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_offtime_judge_en : 1;
        unsigned char buck_offtime_judge_delay : 3;
        unsigned char buck_offtime_judge : 2;
        unsigned char buck_pfm_out : 2;
    } reg;
} SOC_SCHARGER_BUCK_REG8_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG8_buck_offtime_judge_en_START (0)
#define SOC_SCHARGER_BUCK_REG8_buck_offtime_judge_en_END (0)
#define SOC_SCHARGER_BUCK_REG8_buck_offtime_judge_delay_START (1)
#define SOC_SCHARGER_BUCK_REG8_buck_offtime_judge_delay_END (3)
#define SOC_SCHARGER_BUCK_REG8_buck_offtime_judge_START (4)
#define SOC_SCHARGER_BUCK_REG8_buck_offtime_judge_END (5)
#define SOC_SCHARGER_BUCK_REG8_buck_pfm_out_START (6)
#define SOC_SCHARGER_BUCK_REG8_buck_pfm_out_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_cmp_ibias_adj : 1;
        unsigned char buck_sft_maxduty_en : 1;
        unsigned char buck_9v_maxduty_adj : 2;
        unsigned char buck_12v_maxduty_adj : 2;
        unsigned char buck_9v_maxduty_en : 1;
        unsigned char buck_12v_maxduty_en : 1;
    } reg;
} SOC_SCHARGER_BUCK_REG9_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG9_buck_cmp_ibias_adj_START (0)
#define SOC_SCHARGER_BUCK_REG9_buck_cmp_ibias_adj_END (0)
#define SOC_SCHARGER_BUCK_REG9_buck_sft_maxduty_en_START (1)
#define SOC_SCHARGER_BUCK_REG9_buck_sft_maxduty_en_END (1)
#define SOC_SCHARGER_BUCK_REG9_buck_9v_maxduty_adj_START (2)
#define SOC_SCHARGER_BUCK_REG9_buck_9v_maxduty_adj_END (3)
#define SOC_SCHARGER_BUCK_REG9_buck_12v_maxduty_adj_START (4)
#define SOC_SCHARGER_BUCK_REG9_buck_12v_maxduty_adj_END (5)
#define SOC_SCHARGER_BUCK_REG9_buck_9v_maxduty_en_START (6)
#define SOC_SCHARGER_BUCK_REG9_buck_9v_maxduty_en_END (6)
#define SOC_SCHARGER_BUCK_REG9_buck_12v_maxduty_en_START (7)
#define SOC_SCHARGER_BUCK_REG9_buck_12v_maxduty_en_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_hmos_fall : 3;
        unsigned char buck_hmos_rise : 3;
        unsigned char buck_sftstart_adj : 2;
    } reg;
} SOC_SCHARGER_BUCK_REG10_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG10_buck_hmos_fall_START (0)
#define SOC_SCHARGER_BUCK_REG10_buck_hmos_fall_END (2)
#define SOC_SCHARGER_BUCK_REG10_buck_hmos_rise_START (3)
#define SOC_SCHARGER_BUCK_REG10_buck_hmos_rise_END (5)
#define SOC_SCHARGER_BUCK_REG10_buck_sftstart_adj_START (6)
#define SOC_SCHARGER_BUCK_REG10_buck_sftstart_adj_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_dt_lshs_delay : 2;
        unsigned char buck_lmos_fall : 3;
        unsigned char buck_lmos_rise : 3;
    } reg;
} SOC_SCHARGER_BUCK_REG11_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG11_buck_dt_lshs_delay_START (0)
#define SOC_SCHARGER_BUCK_REG11_buck_dt_lshs_delay_END (1)
#define SOC_SCHARGER_BUCK_REG11_buck_lmos_fall_START (2)
#define SOC_SCHARGER_BUCK_REG11_buck_lmos_fall_END (4)
#define SOC_SCHARGER_BUCK_REG11_buck_lmos_rise_START (5)
#define SOC_SCHARGER_BUCK_REG11_buck_lmos_rise_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_dmd_ibias : 2;
        unsigned char buck_dmd_dis : 1;
        unsigned char buck_dmd_clamp : 1;
        unsigned char buck_ss_init_dis : 1;
        unsigned char buck_ssmode_en : 1;
        unsigned char buck_dt_hsls : 1;
        unsigned char buck_dt_lshs : 1;
    } reg;
} SOC_SCHARGER_BUCK_REG12_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG12_buck_dmd_ibias_START (0)
#define SOC_SCHARGER_BUCK_REG12_buck_dmd_ibias_END (1)
#define SOC_SCHARGER_BUCK_REG12_buck_dmd_dis_START (2)
#define SOC_SCHARGER_BUCK_REG12_buck_dmd_dis_END (2)
#define SOC_SCHARGER_BUCK_REG12_buck_dmd_clamp_START (3)
#define SOC_SCHARGER_BUCK_REG12_buck_dmd_clamp_END (3)
#define SOC_SCHARGER_BUCK_REG12_buck_ss_init_dis_START (4)
#define SOC_SCHARGER_BUCK_REG12_buck_ss_init_dis_END (4)
#define SOC_SCHARGER_BUCK_REG12_buck_ssmode_en_START (5)
#define SOC_SCHARGER_BUCK_REG12_buck_ssmode_en_END (5)
#define SOC_SCHARGER_BUCK_REG12_buck_dt_hsls_START (6)
#define SOC_SCHARGER_BUCK_REG12_buck_dt_hsls_END (6)
#define SOC_SCHARGER_BUCK_REG12_buck_dt_lshs_START (7)
#define SOC_SCHARGER_BUCK_REG12_buck_dt_lshs_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_q1ocp_adj : 2;
        unsigned char buck_sft_scp_en_n : 1;
        unsigned char buck_dmd_sel : 4;
        unsigned char buck_dmd_delay : 1;
    } reg;
} SOC_SCHARGER_BUCK_REG13_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG13_buck_q1ocp_adj_START (0)
#define SOC_SCHARGER_BUCK_REG13_buck_q1ocp_adj_END (1)
#define SOC_SCHARGER_BUCK_REG13_buck_sft_scp_en_n_START (2)
#define SOC_SCHARGER_BUCK_REG13_buck_sft_scp_en_n_END (2)
#define SOC_SCHARGER_BUCK_REG13_buck_dmd_sel_START (3)
#define SOC_SCHARGER_BUCK_REG13_buck_dmd_sel_END (6)
#define SOC_SCHARGER_BUCK_REG13_buck_dmd_delay_START (7)
#define SOC_SCHARGER_BUCK_REG13_buck_dmd_delay_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_q1ocp_lpf_adj : 2;
        unsigned char buck_q3ocp_lpf_adj : 2;
        unsigned char buck_ocp_vally : 2;
        unsigned char buck_ocp_peak : 2;
    } reg;
} SOC_SCHARGER_BUCK_REG14_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG14_buck_q1ocp_lpf_adj_START (0)
#define SOC_SCHARGER_BUCK_REG14_buck_q1ocp_lpf_adj_END (1)
#define SOC_SCHARGER_BUCK_REG14_buck_q3ocp_lpf_adj_START (2)
#define SOC_SCHARGER_BUCK_REG14_buck_q3ocp_lpf_adj_END (3)
#define SOC_SCHARGER_BUCK_REG14_buck_ocp_vally_START (4)
#define SOC_SCHARGER_BUCK_REG14_buck_ocp_vally_END (5)
#define SOC_SCHARGER_BUCK_REG14_buck_ocp_peak_START (6)
#define SOC_SCHARGER_BUCK_REG14_buck_ocp_peak_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_ocp_mode_sel : 1;
        unsigned char buck_ocp_sense_cap_adj : 1;
        unsigned char buck_ocp_delay : 2;
        unsigned char buck_sft_ocp_en : 1;
        unsigned char reserved : 3;
    } reg;
} SOC_SCHARGER_BUCK_REG15_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG15_buck_ocp_mode_sel_START (0)
#define SOC_SCHARGER_BUCK_REG15_buck_ocp_mode_sel_END (0)
#define SOC_SCHARGER_BUCK_REG15_buck_ocp_sense_cap_adj_START (1)
#define SOC_SCHARGER_BUCK_REG15_buck_ocp_sense_cap_adj_END (1)
#define SOC_SCHARGER_BUCK_REG15_buck_ocp_delay_START (2)
#define SOC_SCHARGER_BUCK_REG15_buck_ocp_delay_END (3)
#define SOC_SCHARGER_BUCK_REG15_buck_sft_ocp_en_START (4)
#define SOC_SCHARGER_BUCK_REG15_buck_sft_ocp_en_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_sd_sel : 2;
        unsigned char buck_st_sel : 2;
        unsigned char buck_balance_sel : 1;
        unsigned char reserved : 3;
    } reg;
} SOC_SCHARGER_BUCK_REG16_UNION;
#endif
#define SOC_SCHARGER_BUCK_REG16_buck_sd_sel_START (0)
#define SOC_SCHARGER_BUCK_REG16_buck_sd_sel_END (1)
#define SOC_SCHARGER_BUCK_REG16_buck_st_sel_START (2)
#define SOC_SCHARGER_BUCK_REG16_buck_st_sel_END (3)
#define SOC_SCHARGER_BUCK_REG16_buck_balance_sel_START (4)
#define SOC_SCHARGER_BUCK_REG16_buck_balance_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_res4_sel : 2;
        unsigned char buck_res3_sel : 2;
        unsigned char buck_res2_sel : 2;
        unsigned char buck_res1_sel : 2;
    } reg;
} SOC_SCHARGER_BUCK_RES_SEL0_UNION;
#endif
#define SOC_SCHARGER_BUCK_RES_SEL0_buck_res4_sel_START (0)
#define SOC_SCHARGER_BUCK_RES_SEL0_buck_res4_sel_END (1)
#define SOC_SCHARGER_BUCK_RES_SEL0_buck_res3_sel_START (2)
#define SOC_SCHARGER_BUCK_RES_SEL0_buck_res3_sel_END (3)
#define SOC_SCHARGER_BUCK_RES_SEL0_buck_res2_sel_START (4)
#define SOC_SCHARGER_BUCK_RES_SEL0_buck_res2_sel_END (5)
#define SOC_SCHARGER_BUCK_RES_SEL0_buck_res1_sel_START (6)
#define SOC_SCHARGER_BUCK_RES_SEL0_buck_res1_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_res8_sel : 2;
        unsigned char buck_res7_sel : 2;
        unsigned char buck_res6_sel : 2;
        unsigned char buck_res5_sel : 2;
    } reg;
} SOC_SCHARGER_BUCK_RES_SEL1_UNION;
#endif
#define SOC_SCHARGER_BUCK_RES_SEL1_buck_res8_sel_START (0)
#define SOC_SCHARGER_BUCK_RES_SEL1_buck_res8_sel_END (1)
#define SOC_SCHARGER_BUCK_RES_SEL1_buck_res7_sel_START (2)
#define SOC_SCHARGER_BUCK_RES_SEL1_buck_res7_sel_END (3)
#define SOC_SCHARGER_BUCK_RES_SEL1_buck_res6_sel_START (4)
#define SOC_SCHARGER_BUCK_RES_SEL1_buck_res6_sel_END (5)
#define SOC_SCHARGER_BUCK_RES_SEL1_buck_res5_sel_START (6)
#define SOC_SCHARGER_BUCK_RES_SEL1_buck_res5_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_res12_sel : 2;
        unsigned char buck_res11_sel : 2;
        unsigned char buck_res10_sel : 2;
        unsigned char buck_res9_sel : 2;
    } reg;
} SOC_SCHARGER_BUCK_RES_SEL2_UNION;
#endif
#define SOC_SCHARGER_BUCK_RES_SEL2_buck_res12_sel_START (0)
#define SOC_SCHARGER_BUCK_RES_SEL2_buck_res12_sel_END (1)
#define SOC_SCHARGER_BUCK_RES_SEL2_buck_res11_sel_START (2)
#define SOC_SCHARGER_BUCK_RES_SEL2_buck_res11_sel_END (3)
#define SOC_SCHARGER_BUCK_RES_SEL2_buck_res10_sel_START (4)
#define SOC_SCHARGER_BUCK_RES_SEL2_buck_res10_sel_END (5)
#define SOC_SCHARGER_BUCK_RES_SEL2_buck_res9_sel_START (6)
#define SOC_SCHARGER_BUCK_RES_SEL2_buck_res9_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_res14_sel : 2;
        unsigned char buck_res13_sel : 2;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_BUCK_RES_SEL3_UNION;
#endif
#define SOC_SCHARGER_BUCK_RES_SEL3_buck_res14_sel_START (0)
#define SOC_SCHARGER_BUCK_RES_SEL3_buck_res14_sel_END (1)
#define SOC_SCHARGER_BUCK_RES_SEL3_buck_res13_sel_START (2)
#define SOC_SCHARGER_BUCK_RES_SEL3_buck_res13_sel_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_cap4_sel : 2;
        unsigned char buck_cap3_sel : 2;
        unsigned char buck_cap2_sel : 2;
        unsigned char buck_cap1_sel : 2;
    } reg;
} SOC_SCHARGER_BUCK_CAP_SEL0_UNION;
#endif
#define SOC_SCHARGER_BUCK_CAP_SEL0_buck_cap4_sel_START (0)
#define SOC_SCHARGER_BUCK_CAP_SEL0_buck_cap4_sel_END (1)
#define SOC_SCHARGER_BUCK_CAP_SEL0_buck_cap3_sel_START (2)
#define SOC_SCHARGER_BUCK_CAP_SEL0_buck_cap3_sel_END (3)
#define SOC_SCHARGER_BUCK_CAP_SEL0_buck_cap2_sel_START (4)
#define SOC_SCHARGER_BUCK_CAP_SEL0_buck_cap2_sel_END (5)
#define SOC_SCHARGER_BUCK_CAP_SEL0_buck_cap1_sel_START (6)
#define SOC_SCHARGER_BUCK_CAP_SEL0_buck_cap1_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_cap7_sel : 2;
        unsigned char buck_cap6_sel : 2;
        unsigned char buck_cap5_sel : 2;
        unsigned char reserved : 2;
    } reg;
} SOC_SCHARGER_BUCK_CAP_SEL1_UNION;
#endif
#define SOC_SCHARGER_BUCK_CAP_SEL1_buck_cap7_sel_START (0)
#define SOC_SCHARGER_BUCK_CAP_SEL1_buck_cap7_sel_END (1)
#define SOC_SCHARGER_BUCK_CAP_SEL1_buck_cap6_sel_START (2)
#define SOC_SCHARGER_BUCK_CAP_SEL1_buck_cap6_sel_END (3)
#define SOC_SCHARGER_BUCK_CAP_SEL1_buck_cap5_sel_START (4)
#define SOC_SCHARGER_BUCK_CAP_SEL1_buck_cap5_sel_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_reserve0 : 8;
    } reg;
} SOC_SCHARGER_BUCK_RESERVE0_UNION;
#endif
#define SOC_SCHARGER_BUCK_RESERVE0_buck_reserve0_START (0)
#define SOC_SCHARGER_BUCK_RESERVE0_buck_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_reserve1 : 8;
    } reg;
} SOC_SCHARGER_BUCK_RESERVE1_UNION;
#endif
#define SOC_SCHARGER_BUCK_RESERVE1_buck_reserve1_START (0)
#define SOC_SCHARGER_BUCK_RESERVE1_buck_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_reserve2 : 8;
    } reg;
} SOC_SCHARGER_BUCK_RESERVE2_UNION;
#endif
#define SOC_SCHARGER_BUCK_RESERVE2_buck_reserve2_START (0)
#define SOC_SCHARGER_BUCK_RESERVE2_buck_reserve2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_reserve3 : 8;
    } reg;
} SOC_SCHARGER_BUCK_RESERVE3_UNION;
#endif
#define SOC_SCHARGER_BUCK_RESERVE3_buck_reserve3_START (0)
#define SOC_SCHARGER_BUCK_RESERVE3_buck_reserve3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_reserve0_state : 8;
    } reg;
} SOC_SCHARGER_BUCK_RESERVE0_STATE_UNION;
#endif
#define SOC_SCHARGER_BUCK_RESERVE0_STATE_buck_reserve0_state_START (0)
#define SOC_SCHARGER_BUCK_RESERVE0_STATE_buck_reserve0_state_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_reserve1_state : 8;
    } reg;
} SOC_SCHARGER_BUCK_RESERVE1_STATE_UNION;
#endif
#define SOC_SCHARGER_BUCK_RESERVE1_STATE_buck_reserve1_state_START (0)
#define SOC_SCHARGER_BUCK_RESERVE1_STATE_buck_reserve1_state_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_scp_en : 1;
        unsigned char otg_ocp_sys : 1;
        unsigned char otg_ocp_en : 1;
        unsigned char otg_lim_set : 2;
        unsigned char otg_lim_en : 1;
        unsigned char otg_en_int : 1;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_OTG_REG0_UNION;
#endif
#define SOC_SCHARGER_OTG_REG0_otg_scp_en_START (0)
#define SOC_SCHARGER_OTG_REG0_otg_scp_en_END (0)
#define SOC_SCHARGER_OTG_REG0_otg_ocp_sys_START (1)
#define SOC_SCHARGER_OTG_REG0_otg_ocp_sys_END (1)
#define SOC_SCHARGER_OTG_REG0_otg_ocp_en_START (2)
#define SOC_SCHARGER_OTG_REG0_otg_ocp_en_END (2)
#define SOC_SCHARGER_OTG_REG0_otg_lim_set_START (3)
#define SOC_SCHARGER_OTG_REG0_otg_lim_set_END (4)
#define SOC_SCHARGER_OTG_REG0_otg_lim_en_START (5)
#define SOC_SCHARGER_OTG_REG0_otg_lim_en_END (5)
#define SOC_SCHARGER_OTG_REG0_otg_en_int_START (6)
#define SOC_SCHARGER_OTG_REG0_otg_en_int_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_vo : 2;
        unsigned char otg_pfm_v_en : 1;
        unsigned char otg_ovp_en : 1;
        unsigned char otg_uvp_set_v : 2;
        unsigned char otg_uvp_en : 1;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_OTG_REG1_UNION;
#endif
#define SOC_SCHARGER_OTG_REG1_otg_vo_START (0)
#define SOC_SCHARGER_OTG_REG1_otg_vo_END (1)
#define SOC_SCHARGER_OTG_REG1_otg_pfm_v_en_START (2)
#define SOC_SCHARGER_OTG_REG1_otg_pfm_v_en_END (2)
#define SOC_SCHARGER_OTG_REG1_otg_ovp_en_START (3)
#define SOC_SCHARGER_OTG_REG1_otg_ovp_en_END (3)
#define SOC_SCHARGER_OTG_REG1_otg_uvp_set_v_START (4)
#define SOC_SCHARGER_OTG_REG1_otg_uvp_set_v_END (5)
#define SOC_SCHARGER_OTG_REG1_otg_uvp_en_START (6)
#define SOC_SCHARGER_OTG_REG1_otg_uvp_en_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_slop_en : 1;
        unsigned char otg_dmd : 1;
        unsigned char otg_skip_set : 1;
        unsigned char otg_ovp_set_t : 2;
        unsigned char otg_uvp_set_t : 2;
        unsigned char otg_scp_time : 1;
    } reg;
} SOC_SCHARGER_OTG_REG2_UNION;
#endif
#define SOC_SCHARGER_OTG_REG2_otg_slop_en_START (0)
#define SOC_SCHARGER_OTG_REG2_otg_slop_en_END (0)
#define SOC_SCHARGER_OTG_REG2_otg_dmd_START (1)
#define SOC_SCHARGER_OTG_REG2_otg_dmd_END (1)
#define SOC_SCHARGER_OTG_REG2_otg_skip_set_START (2)
#define SOC_SCHARGER_OTG_REG2_otg_skip_set_END (2)
#define SOC_SCHARGER_OTG_REG2_otg_ovp_set_t_START (3)
#define SOC_SCHARGER_OTG_REG2_otg_ovp_set_t_END (4)
#define SOC_SCHARGER_OTG_REG2_otg_uvp_set_t_START (5)
#define SOC_SCHARGER_OTG_REG2_otg_uvp_set_t_END (6)
#define SOC_SCHARGER_OTG_REG2_otg_scp_time_START (7)
#define SOC_SCHARGER_OTG_REG2_otg_scp_time_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_clp_l_iav_set : 2;
        unsigned char otg_clp_l_iav_en : 1;
        unsigned char otg_clp_h_iav_en : 1;
        unsigned char otg_clp_l_en : 1;
        unsigned char otg_clp_h_en : 1;
        unsigned char otg_slop_set : 2;
    } reg;
} SOC_SCHARGER_OTG_REG3_UNION;
#endif
#define SOC_SCHARGER_OTG_REG3_otg_clp_l_iav_set_START (0)
#define SOC_SCHARGER_OTG_REG3_otg_clp_l_iav_set_END (1)
#define SOC_SCHARGER_OTG_REG3_otg_clp_l_iav_en_START (2)
#define SOC_SCHARGER_OTG_REG3_otg_clp_l_iav_en_END (2)
#define SOC_SCHARGER_OTG_REG3_otg_clp_h_iav_en_START (3)
#define SOC_SCHARGER_OTG_REG3_otg_clp_h_iav_en_END (3)
#define SOC_SCHARGER_OTG_REG3_otg_clp_l_en_START (4)
#define SOC_SCHARGER_OTG_REG3_otg_clp_l_en_END (4)
#define SOC_SCHARGER_OTG_REG3_otg_clp_h_en_START (5)
#define SOC_SCHARGER_OTG_REG3_otg_clp_h_en_END (5)
#define SOC_SCHARGER_OTG_REG3_otg_slop_set_START (6)
#define SOC_SCHARGER_OTG_REG3_otg_slop_set_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_ccom2_cmp : 2;
        unsigned char otg_ccom1_cmp : 2;
        unsigned char otg_rcom_cmp : 2;
        unsigned char otg_clp_l_set : 1;
        unsigned char otg_pfm_iav_en : 1;
    } reg;
} SOC_SCHARGER_OTG_REG4_UNION;
#endif
#define SOC_SCHARGER_OTG_REG4_otg_ccom2_cmp_START (0)
#define SOC_SCHARGER_OTG_REG4_otg_ccom2_cmp_END (1)
#define SOC_SCHARGER_OTG_REG4_otg_ccom1_cmp_START (2)
#define SOC_SCHARGER_OTG_REG4_otg_ccom1_cmp_END (3)
#define SOC_SCHARGER_OTG_REG4_otg_rcom_cmp_START (4)
#define SOC_SCHARGER_OTG_REG4_otg_rcom_cmp_END (5)
#define SOC_SCHARGER_OTG_REG4_otg_clp_l_set_START (6)
#define SOC_SCHARGER_OTG_REG4_otg_clp_l_set_END (6)
#define SOC_SCHARGER_OTG_REG4_otg_pfm_iav_en_START (7)
#define SOC_SCHARGER_OTG_REG4_otg_pfm_iav_en_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_osc_ckmax : 2;
        unsigned char otg_ccom_iav : 2;
        unsigned char otg_rcom2_iav : 2;
        unsigned char otg_rcom1_iav : 2;
    } reg;
} SOC_SCHARGER_OTG_REG5_UNION;
#endif
#define SOC_SCHARGER_OTG_REG5_otg_osc_ckmax_START (0)
#define SOC_SCHARGER_OTG_REG5_otg_osc_ckmax_END (1)
#define SOC_SCHARGER_OTG_REG5_otg_ccom_iav_START (2)
#define SOC_SCHARGER_OTG_REG5_otg_ccom_iav_END (3)
#define SOC_SCHARGER_OTG_REG5_otg_rcom2_iav_START (4)
#define SOC_SCHARGER_OTG_REG5_otg_rcom2_iav_END (5)
#define SOC_SCHARGER_OTG_REG5_otg_rcom1_iav_START (6)
#define SOC_SCHARGER_OTG_REG5_otg_rcom1_iav_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_osc : 4;
        unsigned char otg_gm : 3;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_OTG_REG6_UNION;
#endif
#define SOC_SCHARGER_OTG_REG6_otg_osc_START (0)
#define SOC_SCHARGER_OTG_REG6_otg_osc_END (3)
#define SOC_SCHARGER_OTG_REG6_otg_gm_START (4)
#define SOC_SCHARGER_OTG_REG6_otg_gm_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_dmd_ofs : 4;
        unsigned char otg_ckmin : 2;
        unsigned char otg_lmos_ocp : 2;
    } reg;
} SOC_SCHARGER_OTG_REG7_UNION;
#endif
#define SOC_SCHARGER_OTG_REG7_otg_dmd_ofs_START (0)
#define SOC_SCHARGER_OTG_REG7_otg_dmd_ofs_END (3)
#define SOC_SCHARGER_OTG_REG7_otg_ckmin_START (4)
#define SOC_SCHARGER_OTG_REG7_otg_ckmin_END (5)
#define SOC_SCHARGER_OTG_REG7_otg_lmos_ocp_START (6)
#define SOC_SCHARGER_OTG_REG7_otg_lmos_ocp_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_down : 2;
        unsigned char otg_hmos : 1;
        unsigned char otg_cmp : 1;
        unsigned char otg_phase : 2;
        unsigned char otg_rf : 2;
    } reg;
} SOC_SCHARGER_OTG_REG8_UNION;
#endif
#define SOC_SCHARGER_OTG_REG8_otg_down_START (0)
#define SOC_SCHARGER_OTG_REG8_otg_down_END (1)
#define SOC_SCHARGER_OTG_REG8_otg_hmos_START (2)
#define SOC_SCHARGER_OTG_REG8_otg_hmos_END (2)
#define SOC_SCHARGER_OTG_REG8_otg_cmp_START (3)
#define SOC_SCHARGER_OTG_REG8_otg_cmp_END (3)
#define SOC_SCHARGER_OTG_REG8_otg_phase_START (4)
#define SOC_SCHARGER_OTG_REG8_otg_phase_END (5)
#define SOC_SCHARGER_OTG_REG8_otg_rf_START (6)
#define SOC_SCHARGER_OTG_REG8_otg_rf_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_delay2 : 2;
        unsigned char otg_delay1 : 2;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_OTG_REG9_UNION;
#endif
#define SOC_SCHARGER_OTG_REG9_otg_delay2_START (0)
#define SOC_SCHARGER_OTG_REG9_otg_delay2_END (1)
#define SOC_SCHARGER_OTG_REG9_otg_delay1_START (2)
#define SOC_SCHARGER_OTG_REG9_otg_delay1_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_trim1 : 8;
    } reg;
} SOC_SCHARGER_OTG_TRIM1_UNION;
#endif
#define SOC_SCHARGER_OTG_TRIM1_otg_trim1_START (0)
#define SOC_SCHARGER_OTG_TRIM1_otg_trim1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_trim2 : 8;
    } reg;
} SOC_SCHARGER_OTG_TRIM2_UNION;
#endif
#define SOC_SCHARGER_OTG_TRIM2_otg_trim2_START (0)
#define SOC_SCHARGER_OTG_TRIM2_otg_trim2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_resved : 8;
    } reg;
} SOC_SCHARGER_OTG_RESERVE_UNION;
#endif
#define SOC_SCHARGER_OTG_RESERVE_otg_resved_START (0)
#define SOC_SCHARGER_OTG_RESERVE_otg_resved_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_resved1 : 8;
    } reg;
} SOC_SCHARGER_OTG_RESERVE1_UNION;
#endif
#define SOC_SCHARGER_OTG_RESERVE1_otg_resved1_START (0)
#define SOC_SCHARGER_OTG_RESERVE1_otg_resved1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_resved2 : 8;
    } reg;
} SOC_SCHARGER_OTG_RESERVE2_UNION;
#endif
#define SOC_SCHARGER_OTG_RESERVE2_otg_resved2_START (0)
#define SOC_SCHARGER_OTG_RESERVE2_otg_resved2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_pre_vchg : 2;
        unsigned char chg_pre_ichg : 2;
        unsigned char batfet_ctrl_cfg : 1;
        unsigned char chg_en : 1;
        unsigned char reserved : 2;
    } reg;
} SOC_SCHARGER_CHG_REG0_UNION;
#endif
#define SOC_SCHARGER_CHG_REG0_chg_pre_vchg_START (0)
#define SOC_SCHARGER_CHG_REG0_chg_pre_vchg_END (1)
#define SOC_SCHARGER_CHG_REG0_chg_pre_ichg_START (2)
#define SOC_SCHARGER_CHG_REG0_chg_pre_ichg_END (3)
#define SOC_SCHARGER_CHG_REG0_batfet_ctrl_cfg_START (4)
#define SOC_SCHARGER_CHG_REG0_batfet_ctrl_cfg_END (4)
#define SOC_SCHARGER_CHG_REG0_chg_en_START (5)
#define SOC_SCHARGER_CHG_REG0_chg_en_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_fast_ichg : 5;
        unsigned char reserved : 3;
    } reg;
} SOC_SCHARGER_CHG_REG1_UNION;
#endif
#define SOC_SCHARGER_CHG_REG1_chg_fast_ichg_START (0)
#define SOC_SCHARGER_CHG_REG1_chg_fast_ichg_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_term_ichg : 3;
        unsigned char chg_fast_vchg : 4;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_CHG_REG2_UNION;
#endif
#define SOC_SCHARGER_CHG_REG2_chg_term_ichg_START (0)
#define SOC_SCHARGER_CHG_REG2_chg_term_ichg_END (2)
#define SOC_SCHARGER_CHG_REG2_chg_fast_vchg_START (3)
#define SOC_SCHARGER_CHG_REG2_chg_fast_vchg_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_gap_set : 2;
        unsigned char chg_fastchg_timer : 2;
        unsigned char chg_prechg_timer : 2;
        unsigned char chg_vrechg_hys : 2;
    } reg;
} SOC_SCHARGER_CHG_REG3_UNION;
#endif
#define SOC_SCHARGER_CHG_REG3_chg_gap_set_START (0)
#define SOC_SCHARGER_CHG_REG3_chg_gap_set_END (1)
#define SOC_SCHARGER_CHG_REG3_chg_fastchg_timer_START (2)
#define SOC_SCHARGER_CHG_REG3_chg_fastchg_timer_END (3)
#define SOC_SCHARGER_CHG_REG3_chg_prechg_timer_START (4)
#define SOC_SCHARGER_CHG_REG3_chg_prechg_timer_END (5)
#define SOC_SCHARGER_CHG_REG3_chg_vrechg_hys_START (6)
#define SOC_SCHARGER_CHG_REG3_chg_vrechg_hys_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_bat_open : 1;
        unsigned char chg_clk_div2_shd : 1;
        unsigned char chg_en_term : 1;
        unsigned char reserved : 5;
    } reg;
} SOC_SCHARGER_CHG_REG4_UNION;
#endif
#define SOC_SCHARGER_CHG_REG4_chg_bat_open_START (0)
#define SOC_SCHARGER_CHG_REG4_chg_bat_open_END (0)
#define SOC_SCHARGER_CHG_REG4_chg_clk_div2_shd_START (1)
#define SOC_SCHARGER_CHG_REG4_chg_clk_div2_shd_END (1)
#define SOC_SCHARGER_CHG_REG4_chg_en_term_START (2)
#define SOC_SCHARGER_CHG_REG4_chg_en_term_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_vclamp_set : 3;
        unsigned char chg_ir_set : 3;
        unsigned char reserved : 2;
    } reg;
} SOC_SCHARGER_CHG_REG5_UNION;
#endif
#define SOC_SCHARGER_CHG_REG5_chg_vclamp_set_START (0)
#define SOC_SCHARGER_CHG_REG5_chg_vclamp_set_END (2)
#define SOC_SCHARGER_CHG_REG5_chg_ir_set_START (3)
#define SOC_SCHARGER_CHG_REG5_chg_ir_set_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_vres2_sel : 2;
        unsigned char chg_vres1_sel : 2;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_CHG_VRES_SEL_UNION;
#endif
#define SOC_SCHARGER_CHG_VRES_SEL_chg_vres2_sel_START (0)
#define SOC_SCHARGER_CHG_VRES_SEL_chg_vres2_sel_END (1)
#define SOC_SCHARGER_CHG_VRES_SEL_chg_vres1_sel_START (2)
#define SOC_SCHARGER_CHG_VRES_SEL_chg_vres1_sel_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_cap4_sel : 2;
        unsigned char chg_cap3_sel : 2;
        unsigned char chg_cap2_sel : 2;
        unsigned char chg_cap1_sel : 2;
    } reg;
} SOC_SCHARGER_CHG_CAP_SEL_UNION;
#endif
#define SOC_SCHARGER_CHG_CAP_SEL_chg_cap4_sel_START (0)
#define SOC_SCHARGER_CHG_CAP_SEL_chg_cap4_sel_END (1)
#define SOC_SCHARGER_CHG_CAP_SEL_chg_cap3_sel_START (2)
#define SOC_SCHARGER_CHG_CAP_SEL_chg_cap3_sel_END (3)
#define SOC_SCHARGER_CHG_CAP_SEL_chg_cap2_sel_START (4)
#define SOC_SCHARGER_CHG_CAP_SEL_chg_cap2_sel_END (5)
#define SOC_SCHARGER_CHG_CAP_SEL_chg_cap1_sel_START (6)
#define SOC_SCHARGER_CHG_CAP_SEL_chg_cap1_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_ios_adj_ir : 5;
        unsigned char chg_rechg_time : 2;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_CHG_REG6_UNION;
#endif
#define SOC_SCHARGER_CHG_REG6_chg_ios_adj_ir_START (0)
#define SOC_SCHARGER_CHG_REG6_chg_ios_adj_ir_END (4)
#define SOC_SCHARGER_CHG_REG6_chg_rechg_time_START (5)
#define SOC_SCHARGER_CHG_REG6_chg_rechg_time_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_iscale_adj_gap : 3;
        unsigned char chg_iscale_adj_ir : 4;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_CHG_REG7_UNION;
#endif
#define SOC_SCHARGER_CHG_REG7_chg_iscale_adj_gap_START (0)
#define SOC_SCHARGER_CHG_REG7_chg_iscale_adj_gap_END (2)
#define SOC_SCHARGER_CHG_REG7_chg_iscale_adj_ir_START (3)
#define SOC_SCHARGER_CHG_REG7_chg_iscale_adj_ir_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_ios_adj_gap : 5;
        unsigned char reserved : 3;
    } reg;
} SOC_SCHARGER_CHG_REG8_UNION;
#endif
#define SOC_SCHARGER_CHG_REG8_chg_ios_adj_gap_START (0)
#define SOC_SCHARGER_CHG_REG8_chg_ios_adj_gap_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_cp_src_sel : 1;
        unsigned char chg_iref_clamp : 2;
        unsigned char chg_ocp_shd : 1;
        unsigned char chg_cv_adj : 4;
    } reg;
} SOC_SCHARGER_CHG_REG9_UNION;
#endif
#define SOC_SCHARGER_CHG_REG9_chg_cp_src_sel_START (0)
#define SOC_SCHARGER_CHG_REG9_chg_cp_src_sel_END (0)
#define SOC_SCHARGER_CHG_REG9_chg_iref_clamp_START (1)
#define SOC_SCHARGER_CHG_REG9_chg_iref_clamp_END (2)
#define SOC_SCHARGER_CHG_REG9_chg_ocp_shd_START (3)
#define SOC_SCHARGER_CHG_REG9_chg_ocp_shd_END (3)
#define SOC_SCHARGER_CHG_REG9_chg_cv_adj_START (4)
#define SOC_SCHARGER_CHG_REG9_chg_cv_adj_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_resvi1 : 8;
    } reg;
} SOC_SCHARGER_CHG_RESVI1_UNION;
#endif
#define SOC_SCHARGER_CHG_RESVI1_chg_resvi1_START (0)
#define SOC_SCHARGER_CHG_RESVI1_chg_resvi1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_resvi2 : 8;
    } reg;
} SOC_SCHARGER_CHG_RESVI2_UNION;
#endif
#define SOC_SCHARGER_CHG_RESVI2_chg_resvi2_START (0)
#define SOC_SCHARGER_CHG_RESVI2_chg_resvi2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_resvo1 : 8;
    } reg;
} SOC_SCHARGER_CHG_RESVO1_UNION;
#endif
#define SOC_SCHARGER_CHG_RESVO1_chg_resvo1_START (0)
#define SOC_SCHARGER_CHG_RESVO1_chg_resvo1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_resvo2 : 8;
    } reg;
} SOC_SCHARGER_CHG_RESVO2_UNION;
#endif
#define SOC_SCHARGER_CHG_RESVO2_chg_resvo2_START (0)
#define SOC_SCHARGER_CHG_RESVO2_chg_resvo2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char bat_gd_shield : 1;
        unsigned char bat_gd_sel : 1;
        unsigned char slp_vset : 1;
        unsigned char vbus_vset : 2;
        unsigned char ts_ctrl : 1;
        unsigned char reserved : 2;
    } reg;
} SOC_SCHARGER_DET_TOP_REG0_UNION;
#endif
#define SOC_SCHARGER_DET_TOP_REG0_bat_gd_shield_START (0)
#define SOC_SCHARGER_DET_TOP_REG0_bat_gd_shield_END (0)
#define SOC_SCHARGER_DET_TOP_REG0_bat_gd_sel_START (1)
#define SOC_SCHARGER_DET_TOP_REG0_bat_gd_sel_END (1)
#define SOC_SCHARGER_DET_TOP_REG0_slp_vset_START (2)
#define SOC_SCHARGER_DET_TOP_REG0_slp_vset_END (2)
#define SOC_SCHARGER_DET_TOP_REG0_vbus_vset_START (3)
#define SOC_SCHARGER_DET_TOP_REG0_vbus_vset_END (4)
#define SOC_SCHARGER_DET_TOP_REG0_ts_ctrl_START (5)
#define SOC_SCHARGER_DET_TOP_REG0_ts_ctrl_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_hot_hsys : 2;
        unsigned char chg_cold_hsys : 2;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_DET_TOP_REG1_UNION;
#endif
#define SOC_SCHARGER_DET_TOP_REG1_chg_hot_hsys_START (0)
#define SOC_SCHARGER_DET_TOP_REG1_chg_hot_hsys_END (1)
#define SOC_SCHARGER_DET_TOP_REG1_chg_cold_hsys_START (2)
#define SOC_SCHARGER_DET_TOP_REG1_chg_cold_hsys_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char thsd_treg_set : 2;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_THSD_ADJ_UNION;
#endif
#define SOC_SCHARGER_THSD_ADJ_thsd_treg_set_START (0)
#define SOC_SCHARGER_THSD_ADJ_thsd_treg_set_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck_enb : 1;
        unsigned char timer_test : 1;
        unsigned char regn_ocp_shield : 1;
        unsigned char reserved : 5;
    } reg;
} SOC_SCHARGER_SCHG_LOGIC_CTRL_UNION;
#endif
#define SOC_SCHARGER_SCHG_LOGIC_CTRL_buck_enb_START (0)
#define SOC_SCHARGER_SCHG_LOGIC_CTRL_buck_enb_END (0)
#define SOC_SCHARGER_SCHG_LOGIC_CTRL_timer_test_START (1)
#define SOC_SCHARGER_SCHG_LOGIC_CTRL_timer_test_END (1)
#define SOC_SCHARGER_SCHG_LOGIC_CTRL_regn_ocp_shield_START (2)
#define SOC_SCHARGER_SCHG_LOGIC_CTRL_regn_ocp_shield_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otg_500ma_mos : 1;
        unsigned char gate_gnd_ctrl : 1;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_BLOCK_CTRL_UNION;
#endif
#define SOC_SCHARGER_BLOCK_CTRL_otg_500ma_mos_START (0)
#define SOC_SCHARGER_BLOCK_CTRL_otg_500ma_mos_END (0)
#define SOC_SCHARGER_BLOCK_CTRL_gate_gnd_ctrl_START (1)
#define SOC_SCHARGER_BLOCK_CTRL_gate_gnd_ctrl_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ref_trim : 7;
        unsigned char ibias_switch_en : 1;
    } reg;
} SOC_SCHARGER_REF_TOP_CTRL_UNION;
#endif
#define SOC_SCHARGER_REF_TOP_CTRL_ref_trim_START (0)
#define SOC_SCHARGER_REF_TOP_CTRL_ref_trim_END (6)
#define SOC_SCHARGER_REF_TOP_CTRL_ibias_switch_en_START (7)
#define SOC_SCHARGER_REF_TOP_CTRL_ibias_switch_en_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_ch_sel : 2;
        unsigned char hkadc_div_freq : 1;
        unsigned char hkadc_reset : 1;
        unsigned char hkadc_en : 1;
        unsigned char reserved : 3;
    } reg;
} SOC_SCHARGER_ADC_CTRL_UNION;
#endif
#define SOC_SCHARGER_ADC_CTRL_hkadc_ch_sel_START (0)
#define SOC_SCHARGER_ADC_CTRL_hkadc_ch_sel_END (1)
#define SOC_SCHARGER_ADC_CTRL_hkadc_div_freq_START (2)
#define SOC_SCHARGER_ADC_CTRL_hkadc_div_freq_END (2)
#define SOC_SCHARGER_ADC_CTRL_hkadc_reset_START (3)
#define SOC_SCHARGER_ADC_CTRL_hkadc_reset_END (3)
#define SOC_SCHARGER_ADC_CTRL_hkadc_en_START (4)
#define SOC_SCHARGER_ADC_CTRL_hkadc_en_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_start : 1;
        unsigned char hkadc_reserved : 7;
    } reg;
} SOC_SCHARGER_ADC_START_UNION;
#endif
#define SOC_SCHARGER_ADC_START_hkadc_start_START (0)
#define SOC_SCHARGER_ADC_START_hkadc_start_END (0)
#define SOC_SCHARGER_ADC_START_hkadc_reserved_START (1)
#define SOC_SCHARGER_ADC_START_hkadc_reserved_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_valid : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_ADC_CONV_STATUS_UNION;
#endif
#define SOC_SCHARGER_ADC_CONV_STATUS_hkadc_valid_START (0)
#define SOC_SCHARGER_ADC_CONV_STATUS_hkadc_valid_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_data11_8 : 4;
        unsigned char reserved : 4;
    } reg;
} SOC_SCHARGER_ADC_DATA1_UNION;
#endif
#define SOC_SCHARGER_ADC_DATA1_hkadc_data11_8_START (0)
#define SOC_SCHARGER_ADC_DATA1_hkadc_data11_8_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_data7_0 : 8;
    } reg;
} SOC_SCHARGER_ADC_DATA0_UNION;
#endif
#define SOC_SCHARGER_ADC_DATA0_hkadc_data7_0_START (0)
#define SOC_SCHARGER_ADC_DATA0_hkadc_data7_0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_ibias_sel : 8;
    } reg;
} SOC_SCHARGER_ADC_IBIAS_SEL_UNION;
#endif
#define SOC_SCHARGER_ADC_IBIAS_SEL_hkadc_ibias_sel_START (0)
#define SOC_SCHARGER_ADC_IBIAS_SEL_hkadc_ibias_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_por_int_ro : 1;
        unsigned char efuse_state : 4;
        unsigned char reserved : 3;
    } reg;
} SOC_SCHARGER_EFUSE_REG0_UNION;
#endif
#define SOC_SCHARGER_EFUSE_REG0_efuse_por_int_ro_START (0)
#define SOC_SCHARGER_EFUSE_REG0_efuse_por_int_ro_END (0)
#define SOC_SCHARGER_EFUSE_REG0_efuse_state_START (1)
#define SOC_SCHARGER_EFUSE_REG0_efuse_state_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_nr_cfg : 1;
        unsigned char efuse_pgenb_cfg : 1;
        unsigned char efuse_strobe_cfg : 1;
        unsigned char efuse_rd_ctrl : 1;
        unsigned char efuse_inctrl_sel : 1;
        unsigned char efuse_prog_sel : 1;
        unsigned char efuse_prog_int : 1;
        unsigned char reserved : 1;
    } reg;
} SOC_SCHARGER_EFUSE_REG1_UNION;
#endif
#define SOC_SCHARGER_EFUSE_REG1_efuse_nr_cfg_START (0)
#define SOC_SCHARGER_EFUSE_REG1_efuse_nr_cfg_END (0)
#define SOC_SCHARGER_EFUSE_REG1_efuse_pgenb_cfg_START (1)
#define SOC_SCHARGER_EFUSE_REG1_efuse_pgenb_cfg_END (1)
#define SOC_SCHARGER_EFUSE_REG1_efuse_strobe_cfg_START (2)
#define SOC_SCHARGER_EFUSE_REG1_efuse_strobe_cfg_END (2)
#define SOC_SCHARGER_EFUSE_REG1_efuse_rd_ctrl_START (3)
#define SOC_SCHARGER_EFUSE_REG1_efuse_rd_ctrl_END (3)
#define SOC_SCHARGER_EFUSE_REG1_efuse_inctrl_sel_START (4)
#define SOC_SCHARGER_EFUSE_REG1_efuse_inctrl_sel_END (4)
#define SOC_SCHARGER_EFUSE_REG1_efuse_prog_sel_START (5)
#define SOC_SCHARGER_EFUSE_REG1_efuse_prog_sel_END (5)
#define SOC_SCHARGER_EFUSE_REG1_efuse_prog_int_START (6)
#define SOC_SCHARGER_EFUSE_REG1_efuse_prog_int_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_we0_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_WE0_UNION;
#endif
#define SOC_SCHARGER_EFUSE_WE0_efuse_we0_cfg_START (0)
#define SOC_SCHARGER_EFUSE_WE0_efuse_we0_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_we1_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_WE1_UNION;
#endif
#define SOC_SCHARGER_EFUSE_WE1_efuse_we1_cfg_START (0)
#define SOC_SCHARGER_EFUSE_WE1_efuse_we1_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_we2_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_WE2_UNION;
#endif
#define SOC_SCHARGER_EFUSE_WE2_efuse_we2_cfg_START (0)
#define SOC_SCHARGER_EFUSE_WE2_efuse_we2_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_we3_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_WE3_UNION;
#endif
#define SOC_SCHARGER_EFUSE_WE3_efuse_we3_cfg_START (0)
#define SOC_SCHARGER_EFUSE_WE3_efuse_we3_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_we4_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_WE4_UNION;
#endif
#define SOC_SCHARGER_EFUSE_WE4_efuse_we4_cfg_START (0)
#define SOC_SCHARGER_EFUSE_WE4_efuse_we4_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_we5_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_WE5_UNION;
#endif
#define SOC_SCHARGER_EFUSE_WE5_efuse_we5_cfg_START (0)
#define SOC_SCHARGER_EFUSE_WE5_efuse_we5_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_we6_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_WE6_UNION;
#endif
#define SOC_SCHARGER_EFUSE_WE6_efuse_we6_cfg_START (0)
#define SOC_SCHARGER_EFUSE_WE6_efuse_we6_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_we7_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_WE7_UNION;
#endif
#define SOC_SCHARGER_EFUSE_WE7_efuse_we7_cfg_START (0)
#define SOC_SCHARGER_EFUSE_WE7_efuse_we7_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_pdob0 : 8;
    } reg;
} SOC_SCHARGER_EFUSE_PDOB0_UNION;
#endif
#define SOC_SCHARGER_EFUSE_PDOB0_efuse_pdob0_START (0)
#define SOC_SCHARGER_EFUSE_PDOB0_efuse_pdob0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_pdob1 : 8;
    } reg;
} SOC_SCHARGER_EFUSE_PDOB1_UNION;
#endif
#define SOC_SCHARGER_EFUSE_PDOB1_efuse_pdob1_START (0)
#define SOC_SCHARGER_EFUSE_PDOB1_efuse_pdob1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_pdob2 : 8;
    } reg;
} SOC_SCHARGER_EFUSE_PDOB2_UNION;
#endif
#define SOC_SCHARGER_EFUSE_PDOB2_efuse_pdob2_START (0)
#define SOC_SCHARGER_EFUSE_PDOB2_efuse_pdob2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_pdob3 : 8;
    } reg;
} SOC_SCHARGER_EFUSE_PDOB3_UNION;
#endif
#define SOC_SCHARGER_EFUSE_PDOB3_efuse_pdob3_START (0)
#define SOC_SCHARGER_EFUSE_PDOB3_efuse_pdob3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_pdob4 : 8;
    } reg;
} SOC_SCHARGER_EFUSE_PDOB4_UNION;
#endif
#define SOC_SCHARGER_EFUSE_PDOB4_efuse_pdob4_START (0)
#define SOC_SCHARGER_EFUSE_PDOB4_efuse_pdob4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_pdob5 : 8;
    } reg;
} SOC_SCHARGER_EFUSE_PDOB5_UNION;
#endif
#define SOC_SCHARGER_EFUSE_PDOB5_efuse_pdob5_START (0)
#define SOC_SCHARGER_EFUSE_PDOB5_efuse_pdob5_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_pdob6 : 8;
    } reg;
} SOC_SCHARGER_EFUSE_PDOB6_UNION;
#endif
#define SOC_SCHARGER_EFUSE_PDOB6_efuse_pdob6_START (0)
#define SOC_SCHARGER_EFUSE_PDOB6_efuse_pdob6_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_pdob7 : 8;
    } reg;
} SOC_SCHARGER_EFUSE_PDOB7_UNION;
#endif
#define SOC_SCHARGER_EFUSE_PDOB7_efuse_pdob7_START (0)
#define SOC_SCHARGER_EFUSE_PDOB7_efuse_pdob7_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char efuse_soft_rst_cfg : 8;
    } reg;
} SOC_SCHARGER_EFUSE_SOFT_RST_CTRL_UNION;
#endif
#define SOC_SCHARGER_EFUSE_SOFT_RST_CTRL_efuse_soft_rst_cfg_START (0)
#define SOC_SCHARGER_EFUSE_SOFT_RST_CTRL_efuse_soft_rst_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_resvo1 : 8;
    } reg;
} SOC_SCHARGER_SYS_RESVO1_UNION;
#endif
#define SOC_SCHARGER_SYS_RESVO1_sys_resvo1_START (0)
#define SOC_SCHARGER_SYS_RESVO1_sys_resvo1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_resvo2 : 8;
    } reg;
} SOC_SCHARGER_SYS_RESVO2_UNION;
#endif
#define SOC_SCHARGER_SYS_RESVO2_sys_resvo2_START (0)
#define SOC_SCHARGER_SYS_RESVO2_sys_resvo2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_resvo3 : 8;
    } reg;
} SOC_SCHARGER_SYS_RESVO3_UNION;
#endif
#define SOC_SCHARGER_SYS_RESVO3_sys_resvo3_START (0)
#define SOC_SCHARGER_SYS_RESVO3_sys_resvo3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_resvo4 : 8;
    } reg;
} SOC_SCHARGER_SYS_RESVO4_UNION;
#endif
#define SOC_SCHARGER_SYS_RESVO4_sys_resvo4_START (0)
#define SOC_SCHARGER_SYS_RESVO4_sys_resvo4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_resvi1 : 8;
    } reg;
} SOC_SCHARGER_SYS_RESVI1_UNION;
#endif
#define SOC_SCHARGER_SYS_RESVI1_sys_resvi1_START (0)
#define SOC_SCHARGER_SYS_RESVI1_sys_resvi1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_resvi2 : 8;
    } reg;
} SOC_SCHARGER_SYS_RESVI2_UNION;
#endif
#define SOC_SCHARGER_SYS_RESVI2_sys_resvi2_START (0)
#define SOC_SCHARGER_SYS_RESVI2_sys_resvi2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char osc_trimcode : 6;
        unsigned char reserved : 2;
    } reg;
} SOC_SCHARGER_OSC_FCP_UNION;
#endif
#define SOC_SCHARGER_OSC_FCP_osc_trimcode_START (0)
#define SOC_SCHARGER_OSC_FCP_osc_trimcode_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_rst_cfg : 8;
    } reg;
} SOC_SCHARGER_GLB_SOFT_RST_CTRL_UNION;
#endif
#define SOC_SCHARGER_GLB_SOFT_RST_CTRL_soft_rst_cfg_START (0)
#define SOC_SCHARGER_GLB_SOFT_RST_CTRL_soft_rst_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char wd_rst_n : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_WATCHDOG_SOFT_RST_UNION;
#endif
#define SOC_SCHARGER_WATCHDOG_SOFT_RST_wd_rst_n_START (0)
#define SOC_SCHARGER_WATCHDOG_SOFT_RST_wd_rst_n_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char watchdog_timer : 2;
        unsigned char reserved : 6;
    } reg;
} SOC_SCHARGER_WATCHDOG_CTRL_UNION;
#endif
#define SOC_SCHARGER_WATCHDOG_CTRL_watchdog_timer_START (0)
#define SOC_SCHARGER_WATCHDOG_CTRL_watchdog_timer_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char red_clk_enable : 1;
        unsigned char reserved : 7;
    } reg;
} SOC_SCHARGER_CLK_GATE_UNION;
#endif
#define SOC_SCHARGER_CLK_GATE_red_clk_enable_START (0)
#define SOC_SCHARGER_CLK_GATE_red_clk_enable_END (0)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
