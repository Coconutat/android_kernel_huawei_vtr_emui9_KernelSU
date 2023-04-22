#ifndef __PMIC_INTERFACE_H__
#define __PMIC_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define PMIC_VERSION0_ADDR(base) ((base) + (0x000))
#define PMIC_VERSION1_ADDR(base) ((base) + (0x001))
#define PMIC_VERSION2_ADDR(base) ((base) + (0x002))
#define PMIC_VERSION3_ADDR(base) ((base) + (0x003))
#define PMIC_VERSION4_ADDR(base) ((base) + (0x004))
#define PMIC_VERSION5_ADDR(base) ((base) + (0x005))
#define PMIC_STATUS0_ADDR(base) ((base) + (0x006))
#define PMIC_STATUS1_ADDR(base) ((base) + (0x007))
#define PMIC_D2A_RES0_ADDR(base) ((base) + (0x008))
#define PMIC_D2A_RES1_ADDR(base) ((base) + (0x009))
#define PMIC_D2A_RES2_ADDR(base) ((base) + (0x00A))
#define PMIC_A2D_RES0_ADDR(base) ((base) + (0x00B))
#define PMIC_A2D_RES1_ADDR(base) ((base) + (0x00C))
#define PMIC_BUCK0_CTRL0_ADDR(base) ((base) + (0x00D))
#define PMIC_BUCK0_CTRL1_ADDR(base) ((base) + (0x00E))
#define PMIC_BUCK0_CTRL2_ADDR(base) ((base) + (0x00F))
#define PMIC_BUCK0_CTRL3_ADDR(base) ((base) + (0x010))
#define PMIC_BUCK0_CTRL4_ADDR(base) ((base) + (0x011))
#define PMIC_BUCK0_CTRL5_ADDR(base) ((base) + (0x012))
#define PMIC_BUCK0_CTRL6_ADDR(base) ((base) + (0x013))
#define PMIC_BUCK0_CTRL7_ADDR(base) ((base) + (0x014))
#define PMIC_BUCK0_CTRL8_ADDR(base) ((base) + (0x015))
#define PMIC_BUCK0_CTRL9_ADDR(base) ((base) + (0x016))
#define PMIC_BUCK0_CTRL10_ADDR(base) ((base) + (0x017))
#define PMIC_BUCK0_CTRL11_ADDR(base) ((base) + (0x018))
#define PMIC_BUCK1_CTRL0_ADDR(base) ((base) + (0x019))
#define PMIC_BUCK1_CTRL1_ADDR(base) ((base) + (0x01A))
#define PMIC_BUCK1_CTRL2_ADDR(base) ((base) + (0x01B))
#define PMIC_BUCK1_CTRL3_ADDR(base) ((base) + (0x01C))
#define PMIC_BUCK1_CTRL4_ADDR(base) ((base) + (0x01D))
#define PMIC_BUCK1_CTRL5_ADDR(base) ((base) + (0x01E))
#define PMIC_BUCK1_CTRL6_ADDR(base) ((base) + (0x01F))
#define PMIC_BUCK1_CTRL7_ADDR(base) ((base) + (0x020))
#define PMIC_BUCK1_CTRL8_ADDR(base) ((base) + (0x021))
#define PMIC_BUCK1_CTRL9_ADDR(base) ((base) + (0x022))
#define PMIC_BUCK1_CTRL10_ADDR(base) ((base) + (0x023))
#define PMIC_BUCK1_CTRL11_ADDR(base) ((base) + (0x024))
#define PMIC_BUCK2_CTRL0_ADDR(base) ((base) + (0x025))
#define PMIC_BUCK2_CTRL1_ADDR(base) ((base) + (0x026))
#define PMIC_BUCK2_CTRL2_ADDR(base) ((base) + (0x027))
#define PMIC_BUCK2_CTRL3_ADDR(base) ((base) + (0x028))
#define PMIC_BUCK2_CTRL4_ADDR(base) ((base) + (0x029))
#define PMIC_BUCK2_CTRL5_ADDR(base) ((base) + (0x02A))
#define PMIC_BUCK2_CTRL6_ADDR(base) ((base) + (0x02B))
#define PMIC_BUCK2_CTRL7_ADDR(base) ((base) + (0x02C))
#define PMIC_BUCK2_CTRL8_ADDR(base) ((base) + (0x02D))
#define PMIC_BUCK2_CTRL9_ADDR(base) ((base) + (0x02E))
#define PMIC_BUCK2_CTRL10_ADDR(base) ((base) + (0x02F))
#define PMIC_BUCK2_CTRL11_ADDR(base) ((base) + (0x030))
#define PMIC_BUCK3_CTRL0_ADDR(base) ((base) + (0x031))
#define PMIC_BUCK3_CTRL1_ADDR(base) ((base) + (0x032))
#define PMIC_BUCK3_CTRL2_ADDR(base) ((base) + (0x033))
#define PMIC_BUCK3_CTRL3_ADDR(base) ((base) + (0x034))
#define PMIC_BUCK3_CTRL4_ADDR(base) ((base) + (0x035))
#define PMIC_BUCK3_CTRL5_ADDR(base) ((base) + (0x036))
#define PMIC_BUCK3_CTRL6_ADDR(base) ((base) + (0x037))
#define PMIC_BUCK3_CTRL7_ADDR(base) ((base) + (0x038))
#define PMIC_BUCK3_CTRL8_ADDR(base) ((base) + (0x039))
#define PMIC_BUCK3_CTRL9_ADDR(base) ((base) + (0x03A))
#define PMIC_BUCK3_CTRL10_ADDR(base) ((base) + (0x03B))
#define PMIC_BUCK3_CTRL11_ADDR(base) ((base) + (0x03C))
#define PMIC_BUCK4_CTRL0_ADDR(base) ((base) + (0x03D))
#define PMIC_BUCK4_CTRL1_ADDR(base) ((base) + (0x03E))
#define PMIC_BUCK4_CTRL2_ADDR(base) ((base) + (0x03F))
#define PMIC_BUCK4_CTRL3_ADDR(base) ((base) + (0x040))
#define PMIC_BUCK4_CTRL4_ADDR(base) ((base) + (0x041))
#define PMIC_BUCK4_CTRL5_ADDR(base) ((base) + (0x042))
#define PMIC_BUCK4_CTRL6_ADDR(base) ((base) + (0x043))
#define PMIC_BUCK4_CTRL7_ADDR(base) ((base) + (0x044))
#define PMIC_BUCK4_CTRL8_ADDR(base) ((base) + (0x045))
#define PMIC_BUCK4_CTRL9_ADDR(base) ((base) + (0x046))
#define PMIC_BUCK4_CTRL10_ADDR(base) ((base) + (0x047))
#define PMIC_BUCK4_CTRL11_ADDR(base) ((base) + (0x048))
#define PMIC_LDO_1_CTRL_ADDR(base) ((base) + (0x049))
#define PMIC_LD2_3_CTRL_ADDR(base) ((base) + (0x04A))
#define PMIC_LDO4_5_CTRL_ADDR(base) ((base) + (0x04B))
#define PMIC_LDO7_8_CTRL_ADDR(base) ((base) + (0x04C))
#define PMIC_LDO9_10_CTRL_ADDR(base) ((base) + (0x04D))
#define PMIC_LD11_12_CTRL_ADDR(base) ((base) + (0x04E))
#define PMIC_LDO13_14_CTRL_ADDR(base) ((base) + (0x04F))
#define PMIC_LDO15_16_CTRL_ADDR(base) ((base) + (0x050))
#define PMIC_LDO17_19_CTRL_ADDR(base) ((base) + (0x051))
#define PMIC_LDO20_21_CTRL_ADDR(base) ((base) + (0x052))
#define PMIC_LDO22_23_CTRL_ADDR(base) ((base) + (0x053))
#define PMIC_LDO24_25_CTRL_ADDR(base) ((base) + (0x054))
#define PMIC_LDO26_27_CTRL_ADDR(base) ((base) + (0x055))
#define PMIC_LDO28_29_CTRL_ADDR(base) ((base) + (0x056))
#define PMIC_LDO30_31_CTRL_ADDR(base) ((base) + (0x057))
#define PMIC_LDO32_CTRL_ADDR(base) ((base) + (0x058))
#define PMIC_LDO0_1_ONOFF_ADDR(base) ((base) + (0x059))
#define PMIC_LDO0_2_ONOFF_ECO_ADDR(base) ((base) + (0x05A))
#define PMIC_LDO0_2_VSET_ADDR(base) ((base) + (0x05B))
#define PMIC_LDO1_ONOFF_ECO_ADDR(base) ((base) + (0x05C))
#define PMIC_LDO1_VSET_ADDR(base) ((base) + (0x05D))
#define PMIC_LDO2_ONOFF_ECO_ADDR(base) ((base) + (0x05E))
#define PMIC_LDO2_VSET_ADDR(base) ((base) + (0x05F))
#define PMIC_LDO3_ONOFF_ECO_ADDR(base) ((base) + (0x060))
#define PMIC_LDO3_VSET_ADDR(base) ((base) + (0x061))
#define PMIC_LDO4_ONOFF_ECO_ADDR(base) ((base) + (0x062))
#define PMIC_LDO4_VSET_ADDR(base) ((base) + (0x063))
#define PMIC_LDO5_ONOFF_ECO_ADDR(base) ((base) + (0x064))
#define PMIC_LDO5_VSET_ADDR(base) ((base) + (0x065))
#define PMIC_LDO7_ONOFF_ECO_ADDR(base) ((base) + (0x066))
#define PMIC_LDO7_VSET_ADDR(base) ((base) + (0x067))
#define PMIC_LDO8_ONOFF_ECO_ADDR(base) ((base) + (0x068))
#define PMIC_LDO8_VSET_ADDR(base) ((base) + (0x069))
#define PMIC_LDO9_ONOFF_ECO_ADDR(base) ((base) + (0x06A))
#define PMIC_LDO9_VSET_ADDR(base) ((base) + (0x06B))
#define PMIC_LDO10_ONOFF_ECO_ADDR(base) ((base) + (0x06C))
#define PMIC_LDO10_VSET_ADDR(base) ((base) + (0x06D))
#define PMIC_LDO11_ONOFF_ECO_ADDR(base) ((base) + (0x06E))
#define PMIC_LDO11_VSET_ADDR(base) ((base) + (0x06F))
#define PMIC_LDO12_ONOFF_ECO_ADDR(base) ((base) + (0x070))
#define PMIC_LDO12_VSET_ADDR(base) ((base) + (0x071))
#define PMIC_LDO13_ONOFF_ECO_ADDR(base) ((base) + (0x072))
#define PMIC_LDO13_VSET_ADDR(base) ((base) + (0x073))
#define PMIC_LDO14_ONOFF_ECO_ADDR(base) ((base) + (0x074))
#define PMIC_LDO14_VSET_ADDR(base) ((base) + (0x075))
#define PMIC_LDO15_ONOFF_ECO_ADDR(base) ((base) + (0x076))
#define PMIC_LDO15_VSET_ADDR(base) ((base) + (0x077))
#define PMIC_LDO16_ONOFF_ECO_ADDR(base) ((base) + (0x078))
#define PMIC_LDO16_VSET_ADDR(base) ((base) + (0x079))
#define PMIC_LDO17_ONOFF_ECO_ADDR(base) ((base) + (0x07A))
#define PMIC_LDO17_VSET_ADDR(base) ((base) + (0x07B))
#define PMIC_LDO19_ONOFF_ECO_ADDR(base) ((base) + (0x07C))
#define PMIC_LDO19_VSET1_ADDR(base) ((base) + (0x07D))
#define PMIC_LDO20_ONOFF_ECO_ADDR(base) ((base) + (0x07E))
#define PMIC_LDO20_VSET_ADDR(base) ((base) + (0x07F))
#define PMIC_LDO21_ONOFF_ECO_ADDR(base) ((base) + (0x080))
#define PMIC_LDO21_VSET_ADDR(base) ((base) + (0x081))
#define PMIC_LDO22_ONOFF_ECO_ADDR(base) ((base) + (0x082))
#define PMIC_LDO22_VSET_ADDR(base) ((base) + (0x083))
#define PMIC_LDO23_ONOFF_ECO_ADDR(base) ((base) + (0x084))
#define PMIC_LDO23_VSET_ADDR(base) ((base) + (0x085))
#define PMIC_LDO24_ONOFF_ECO_ADDR(base) ((base) + (0x086))
#define PMIC_LDO24_VSET_ADDR(base) ((base) + (0x087))
#define PMIC_LDO25_ONOFF_ECO_ADDR(base) ((base) + (0x088))
#define PMIC_LDO25_VSET_ADDR(base) ((base) + (0x089))
#define PMIC_LDO26_ONOFF_ECO_ADDR(base) ((base) + (0x08A))
#define PMIC_LDO26_VSET_ADDR(base) ((base) + (0x08B))
#define PMIC_LDO27_ONOFF_ECO_ADDR(base) ((base) + (0x08C))
#define PMIC_LDO27_VSET_ADDR(base) ((base) + (0x08D))
#define PMIC_LDO28_ONOFF_ECO_ADDR(base) ((base) + (0x08E))
#define PMIC_LDO28_VSET_ADDR(base) ((base) + (0x08F))
#define PMIC_LDO29_ONOFF_ECO_ADDR(base) ((base) + (0x090))
#define PMIC_LDO29_VSET_ADDR(base) ((base) + (0x091))
#define PMIC_LDO30_ONOFF_ECO_ADDR(base) ((base) + (0x092))
#define PMIC_LDO30_VSET_ADDR(base) ((base) + (0x093))
#define PMIC_LDO31_ONOFF_ECO_ADDR(base) ((base) + (0x094))
#define PMIC_LDO31_VSET_ADDR(base) ((base) + (0x095))
#define PMIC_LDO32_ONOFF_ECO_ADDR(base) ((base) + (0x096))
#define PMIC_LDO32_VSET_ADDR(base) ((base) + (0x097))
#define PMIC_BUCK0_ONOFF_ECO_ADDR(base) ((base) + (0x098))
#define PMIC_BUCK0_VSET_ADDR(base) ((base) + (0x099))
#define PMIC_BUCK1_ONOFF_ECO_ADDR(base) ((base) + (0x09A))
#define PMIC_BUCK1_VSET_ADDR(base) ((base) + (0x09B))
#define PMIC_BUCK2_ONOFF_ECO_ADDR(base) ((base) + (0x09C))
#define PMIC_BUCK2_VSET_ADDR(base) ((base) + (0x09D))
#define PMIC_BUCK3_ONOFF_ECO_ADDR(base) ((base) + (0x09E))
#define PMIC_BUCK3_VSET_ADDR(base) ((base) + (0x09F))
#define PMIC_BUCK4_ONOFF_ECO_ADDR(base) ((base) + (0x0A0))
#define PMIC_BUCK4_VSET_ADDR(base) ((base) + (0x0A1))
#define PMIC_LDO_PMUA_ECO_ADDR(base) ((base) + (0x0A2))
#define PMIC_LDO_PMUA_VSET_ADDR(base) ((base) + (0x0A3))
#define PMIC_BST_MODE_EN_ADDR(base) ((base) + (0x0A4))
#define PMIC_NOPWR_CTRL_ADDR(base) ((base) + (0x0A5))
#define PMIC_CLASSD_CTRL0_ADDR(base) ((base) + (0x0A6))
#define PMIC_CLASSD_CTRL1_ADDR(base) ((base) + (0x0A7))
#define PMIC_CLASSD_CTRL2_ADDR(base) ((base) + (0x0A8))
#define PMIC_CLASSD_CTRL3_ADDR(base) ((base) + (0x0A9))
#define PMIC_TH_CTRL_ADDR(base) ((base) + (0x0AA))
#define PMIC_BG_TEST_ADDR(base) ((base) + (0x0AB))
#define PMIC_DR_EN_MODE_345_ADDR(base) ((base) + (0x0AC))
#define PMIC_DR_EN_MODE_12_ADDR(base) ((base) + (0x0AD))
#define PMIC_FLASH_PERIOD_DR12_ADDR(base) ((base) + (0x0AE))
#define PMIC_FLASH_ON_DR12_ADDR(base) ((base) + (0x0AF))
#define PMIC_FLASH_PERIOD_DR345_ADDR(base) ((base) + (0x0B0))
#define PMIC_FLASH_ON_DR345_ADDR(base) ((base) + (0x0B1))
#define PMIC_DR_MODE_SEL_ADDR(base) ((base) + (0x0B2))
#define PMIC_DR_BRE_CTRL_ADDR(base) ((base) + (0x0B3))
#define PMIC_DR12_TIM_CONF0_ADDR(base) ((base) + (0x0B4))
#define PMIC_DR12_TIM_CONF1_ADDR(base) ((base) + (0x0B5))
#define PMIC_DR1_ISET_ADDR(base) ((base) + (0x0B6))
#define PMIC_DR2_ISET_ADDR(base) ((base) + (0x0B7))
#define PMIC_DR_LED_CTRL_ADDR(base) ((base) + (0x0B8))
#define PMIC_DR_OUT_CTRL_ADDR(base) ((base) + (0x0B9))
#define PMIC_DR3_ISET_ADDR(base) ((base) + (0x0BA))
#define PMIC_DR3_START_DEL_ADDR(base) ((base) + (0x0BB))
#define PMIC_DR4_ISET_ADDR(base) ((base) + (0x0BC))
#define PMIC_DR4_START_DEL_ADDR(base) ((base) + (0x0BD))
#define PMIC_DR5_ISET_ADDR(base) ((base) + (0x0BE))
#define PMIC_DR5_START_DEL_ADDR(base) ((base) + (0x0BF))
#define PMIC_DR334_TIM_CONF0_ADDR(base) ((base) + (0x0C0))
#define PMIC_DR345_TIM_CONF1_ADDR(base) ((base) + (0x0C1))
#define PMIC_VSYS_LOW_SET0_ADDR(base) ((base) + (0x0C2))
#define PMIC_VSYS_LOW_SET1_ADDR(base) ((base) + (0x0C3))
#define PMIC_SYS_CTRL_RESERVE_ADDR(base) ((base) + (0x0C4))
#define PMIC_HARDWIRE_CTRL0_ADDR(base) ((base) + (0x0C5))
#define PMIC_HARDWIRE_CTRL1_ADDR(base) ((base) + (0x0C6))
#define PMIC_PERI_CTRL0_ADDR(base) ((base) + (0x0C7))
#define PMIC_PERI_CTRL1_ADDR(base) ((base) + (0x0C8))
#define PMIC_PERI_VSET_CTRL_ADDR(base) ((base) + (0x0C9))
#define PMIC_PERI_TIME__CTRL_ADDR(base) ((base) + (0x0CA))
#define PMIC_HRESET_PWRDOWN_CTRL_ADDR(base) ((base) + (0x0CB))
#define PMIC_OSC32K_ONOFF_CTRL_ADDR(base) ((base) + (0x0CC))
#define PMIC_OCP_DEB_CTRL_ADDR(base) ((base) + (0x0CD))
#define PMIC_OCP_SCP_ONOFF_ADDR(base) ((base) + (0x0CE))
#define PMIC_UV_VSYS_DEB_CTRL_ADDR(base) ((base) + (0x0CF))
#define PMIC_BUCK0_3_OCP_CTRL_ADDR(base) ((base) + (0x0D0))
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_ADDR(base) ((base) + (0x0D1))
#define PMIC_LDO3_7_OCP_CTRL_ADDR(base) ((base) + (0x0D2))
#define PMIC_LDO8_11_OCP_CTRL_ADDR(base) ((base) + (0x0D3))
#define PMIC_LDO12_15_OCP_CTRL_ADDR(base) ((base) + (0x0D4))
#define PMIC_LDO16_20_OCP_CTRL_ADDR(base) ((base) + (0x0D5))
#define PMIC_LDO21_24_OCP_CTRL_ADDR(base) ((base) + (0x0D6))
#define PMIC_LDO25_28_OCP_CTRL_ADDR(base) ((base) + (0x0D7))
#define PMIC_LDO29_32_OCP_CTRL_ADDR(base) ((base) + (0x0D8))
#define PMIC_CLASS_BUCK0_SCP_CTRL_ADDR(base) ((base) + (0x0D9))
#define PMIC_BUCK1_4_SCP_CTRL_ADDR(base) ((base) + (0x0DA))
#define PMIC_SYS_CTRL0_ADDR(base) ((base) + (0x0DB))
#define PMIC_SYS_CTRL1_ADDR(base) ((base) + (0x0DC))
#define PMIC_COUL_ECO_MASK_ADDR(base) ((base) + (0x0DD))
#define PMIC_SIM_CTRL_ADDR(base) ((base) + (0x0DE))
#define PMIC_SIM_DEB_CTRL_ADDR(base) ((base) + (0x0DF))
#define PMIC_AUX_IBIAS_CFG_ADDR(base) ((base) + (0x0E0))
#define PMIC_IRQ_MASK_0_ADDR(base) ((base) + (0x0E1))
#define PMIC_IRQ_MASK_1_ADDR(base) ((base) + (0x0E2))
#define PMIC_IRQ_MASK_2_ADDR(base) ((base) + (0x0E3))
#define PMIC_IRQ_MASK_3_ADDR(base) ((base) + (0x0E4))
#define PMIC_IRQ_MASK_4_ADDR(base) ((base) + (0x0E5))
#define PMIC_IRQ_MASK_5_ADDR(base) ((base) + (0x0E6))
#define PMIC_IRQ_MASK_6_ADDR(base) ((base) + (0x0E7))
#define PMIC_IRQ_MASK_7_ADDR(base) ((base) + (0x0E8))
#define PMIC_IRQ_MASK_8_ADDR(base) ((base) + (0x0E9))
#define PMIC_IRQ_MASK_9_ADDR(base) ((base) + (0x0EA))
#define PMIC_OTP0_0_ADDR(base) ((base) + (0x0EB))
#define PMIC_OTP0_1_ADDR(base) ((base) + (0x0EC))
#define PMIC_OTP0_2_ADDR(base) ((base) + (0x0ED))
#define PMIC_OTP0_3_ADDR(base) ((base) + (0x0EE))
#define PMIC_OTP0_CTRL_0_ADDR(base) ((base) + (0x0EF))
#define PMIC_OTP0_CTRL_1_ADDR(base) ((base) + (0x0F0))
#define PMIC_OTP0_WDATA_ADDR(base) ((base) + (0x0F1))
#define PMIC_OTP0_0_W_ADDR(base) ((base) + (0x0F2))
#define PMIC_OTP0_1_W_ADDR(base) ((base) + (0x0F3))
#define PMIC_OTP0_2_W_ADDR(base) ((base) + (0x0F4))
#define PMIC_OTP0_3_W_ADDR(base) ((base) + (0x0F5))
#define PMIC_OTP1_0_ADDR(base) ((base) + (0x0F6))
#define PMIC_OTP1_1_ADDR(base) ((base) + (0x0F7))
#define PMIC_OTP1_2_ADDR(base) ((base) + (0x0F8))
#define PMIC_OTP1_3_ADDR(base) ((base) + (0x0F9))
#define PMIC_OTP1_CTRL_0_ADDR(base) ((base) + (0x0FA))
#define PMIC_OTP1_CTRL_1_ADDR(base) ((base) + (0x0FB))
#define PMIC_OTP1_WDATA_ADDR(base) ((base) + (0x0FC))
#define PMIC_OTP1_0_W_ADDR(base) ((base) + (0x0FD))
#define PMIC_OTP1_1_W_ADDR(base) ((base) + (0x0FE))
#define PMIC_OTP1_2_W_ADDR(base) ((base) + (0x0FF))
#define PMIC_OTP1_3_W_ADDR(base) ((base) + (0x100))
#define PMIC_HRST_REG0_ADDR(base) ((base) + (0x101))
#define PMIC_HRST_REG1_ADDR(base) ((base) + (0x102))
#define PMIC_HRST_REG2_ADDR(base) ((base) + (0x103))
#define PMIC_SOFT_RST_REG_ADDR(base) ((base) + (0x104))
#define PMIC_CLK_TOP_CTRL0_ADDR(base) ((base) + (0x105))
#define PMIC_CLK_TOP_CTRL1_ADDR(base) ((base) + (0x106))
#define PMIC_CLK_TOP_CTRL2_ADDR(base) ((base) + (0x107))
#define PMIC_CLK_TOP_CTRL3_ADDR(base) ((base) + (0x108))
#define PMIC_CLK_TOP_CTRL4_ADDR(base) ((base) + (0x109))
#define PMIC_CLK_TOP_CTRL5_ADDR(base) ((base) + (0x10A))
#define PMIC_CLK_TOP_CTRL6_ADDR(base) ((base) + (0x10B))
#define PMIC_CLK_TOP_CTRL7_ADDR(base) ((base) + (0x10C))
#define PMIC_CLK_256K_CTRL0_ADDR(base) ((base) + (0x10D))
#define PMIC_CLK_256K_CTRL1_ADDR(base) ((base) + (0x10E))
#define PMIC_CLK_TOP_RESERVE0_ADDR(base) ((base) + (0x10F))
#define PMIC_CLK_TOP_RESERVE1_ADDR(base) ((base) + (0x110))
#define PMIC_SYS_DEBUG0_ADDR(base) ((base) + (0x111))
#define PMIC_SYS_DEBUG1_ADDR(base) ((base) + (0x112))
#define PMIC_DAC0_DIN_MSB_ADDR(base) ((base) + (0x113))
#define PMIC_DAC0_DIN_LSB_ADDR(base) ((base) + (0x114))
#define PMIC_DAC1_DIN_MSB_ADDR(base) ((base) + (0x115))
#define PMIC_DAC1_DIN_LSB_ADDR(base) ((base) + (0x116))
#define PMIC_CORE_LDO_ECO_LOCK_ADDR(base) ((base) + (0x117))
#define PMIC_NP_OCP0_ADDR(base) ((base) + (0x11A))
#define PMIC_NP_OCP1_ADDR(base) ((base) + (0x11B))
#define PMIC_NP_OCP2_ADDR(base) ((base) + (0x11C))
#define PMIC_NP_OCP3_ADDR(base) ((base) + (0x11D))
#define PMIC_NP_OCP4_ADDR(base) ((base) + (0x11E))
#define PMIC_NP_SCP_ADDR(base) ((base) + (0x11F))
#define PMIC_IRQ0_ADDR(base) ((base) + (0x120))
#define PMIC_IRQ1_ADDR(base) ((base) + (0x121))
#define PMIC_IRQ2_ADDR(base) ((base) + (0x122))
#define PMIC_IRQ3_ADDR(base) ((base) + (0x123))
#define PMIC_OCP_IRQ0_ADDR(base) ((base) + (0x124))
#define PMIC_OCP_IRQ1_ADDR(base) ((base) + (0x125))
#define PMIC_OCP_IRQ2_ADDR(base) ((base) + (0x126))
#define PMIC_OCP_IRQ3_ADDR(base) ((base) + (0x127))
#define PMIC_OCP_IRQ4_ADDR(base) ((base) + (0x128))
#define PMIC_OCP_IRQ5_ADDR(base) ((base) + (0x129))
#define PMIC_NP_RECORD0_ADDR(base) ((base) + (0x12A))
#define PMIC_NP_RECORD1_ADDR(base) ((base) + (0x12B))
#define PMIC_RTCDR0_ADDR(base) ((base) + (0x12C))
#define PMIC_RTCDR1_ADDR(base) ((base) + (0x12D))
#define PMIC_RTCDR2_ADDR(base) ((base) + (0x12E))
#define PMIC_RTCDR3_ADDR(base) ((base) + (0x12F))
#define PMIC_RTCMR0_ADDR(base) ((base) + (0x130))
#define PMIC_RTCMR1_ADDR(base) ((base) + (0x131))
#define PMIC_RTCMR2_ADDR(base) ((base) + (0x132))
#define PMIC_RTCMR3_ADDR(base) ((base) + (0x133))
#define PMIC_RTCLR0_ADDR(base) ((base) + (0x134))
#define PMIC_RTCLR1_ADDR(base) ((base) + (0x135))
#define PMIC_RTCLR2_ADDR(base) ((base) + (0x136))
#define PMIC_RTCLR3_ADDR(base) ((base) + (0x137))
#define PMIC_RTCCTRL_ADDR(base) ((base) + (0x138))
#define PMIC_XO_THRESOLD0_ADDR(base) ((base) + (0x139))
#define PMIC_XO_THRESOLD1_ADDR(base) ((base) + (0x13A))
#define PMIC_CRC_VAULE0_ADDR(base) ((base) + (0x13B))
#define PMIC_CRC_VAULE1_ADDR(base) ((base) + (0x13C))
#define PMIC_CRC_VAULE2_ADDR(base) ((base) + (0x13D))
#define PMIC_RTC_PWRUP_TIMER0_ADDR(base) ((base) + (0x13E))
#define PMIC_RTC_PWRUP_TIMER1_ADDR(base) ((base) + (0x13F))
#define PMIC_RTC_PWRUP_TIMER2_ADDR(base) ((base) + (0x140))
#define PMIC_RTC_PWRUP_TIMER3_ADDR(base) ((base) + (0x141))
#define PMIC_RTC_PWRDOWN_TIMER0_ADDR(base) ((base) + (0x142))
#define PMIC_RTC_PWRDOWN_TIMER1_ADDR(base) ((base) + (0x143))
#define PMIC_RTC_PWRDOWN_TIMER2_ADDR(base) ((base) + (0x144))
#define PMIC_RTC_PWRDOWN_TIMER3_ADDR(base) ((base) + (0x145))
#define PMIC_COUL_IRQ_ADDR(base) ((base) + (0x14B))
#define PMIC_COUL_IRQ_MASK_ADDR(base) ((base) + (0x14C))
#define PMIC_CURRENT_0_ADDR(base) ((base) + (0x14D))
#define PMIC_CURRENT_1_ADDR(base) ((base) + (0x14E))
#define PMIC_V_OUT_0_ADDR(base) ((base) + (0x14F))
#define PMIC_V_OUT_1_ADDR(base) ((base) + (0x150))
#define PMIC_CLJ_CTRL_REG_ADDR(base) ((base) + (0x151))
#define PMIC_ECO_REFALSH_TIME_ADDR(base) ((base) + (0x152))
#define PMIC_CL_OUT0_ADDR(base) ((base) + (0x153))
#define PMIC_CL_OUT1_ADDR(base) ((base) + (0x154))
#define PMIC_CL_OUT2_ADDR(base) ((base) + (0x155))
#define PMIC_CL_OUT3_ADDR(base) ((base) + (0x156))
#define PMIC_CL_IN0_ADDR(base) ((base) + (0x157))
#define PMIC_CL_IN1_ADDR(base) ((base) + (0x158))
#define PMIC_CL_IN2_ADDR(base) ((base) + (0x159))
#define PMIC_CL_IN3_ADDR(base) ((base) + (0x15A))
#define PMIC_CHG_TIMER0_ADDR(base) ((base) + (0x15B))
#define PMIC_CHG_TIMER1_ADDR(base) ((base) + (0x15C))
#define PMIC_CHG_TIMER2_ADDR(base) ((base) + (0x15D))
#define PMIC_CHG_TIMER3_ADDR(base) ((base) + (0x15E))
#define PMIC_LOAD_TIMER0_ADDR(base) ((base) + (0x15F))
#define PMIC_LOAD_TIMER1_ADDR(base) ((base) + (0x160))
#define PMIC_LOAD_TIMER2_ADDR(base) ((base) + (0x161))
#define PMIC_LOAD_TIMER3_ADDR(base) ((base) + (0x162))
#define PMIC_CL_INT0_ADDR(base) ((base) + (0x163))
#define PMIC_CL_INT1_ADDR(base) ((base) + (0x164))
#define PMIC_CL_INT2_ADDR(base) ((base) + (0x165))
#define PMIC_CL_INT3_ADDR(base) ((base) + (0x166))
#define PMIC_V_INT0_ADDR(base) ((base) + (0x167))
#define PMIC_V_INT1_ADDR(base) ((base) + (0x168))
#define PMIC_OFFSET_CURRENT0_ADDR(base) ((base) + (0x169))
#define PMIC_OFFSET_CURRENT1_ADDR(base) ((base) + (0x16A))
#define PMIC_OFFSET_VOLTAGE0_ADDR(base) ((base) + (0x16B))
#define PMIC_OFFSET_VOLTAGE1_ADDR(base) ((base) + (0x16C))
#define PMIC_OCV_VOLTAGE0_ADDR(base) ((base) + (0x16D))
#define PMIC_OCV_VOLTAGE1_ADDR(base) ((base) + (0x16E))
#define PMIC_OCV_CURRENT0_ADDR(base) ((base) + (0x16F))
#define PMIC_OCV_CURRENT1_ADDR(base) ((base) + (0x170))
#define PMIC_ECO_OUT_CLIN_0_ADDR(base) ((base) + (0x171))
#define PMIC_ECO_OUT_CLIN_1_ADDR(base) ((base) + (0x172))
#define PMIC_ECO_OUT_CLIN_2_ADDR(base) ((base) + (0x173))
#define PMIC_ECO_OUT_CLIN_3_ADDR(base) ((base) + (0x174))
#define PMIC_ECO_OUT_CLOUT_0_ADDR(base) ((base) + (0x175))
#define PMIC_ECO_OUT_CLOUT_1_ADDR(base) ((base) + (0x176))
#define PMIC_ECO_OUT_CLOUT_2_ADDR(base) ((base) + (0x177))
#define PMIC_ECO_OUT_CLOUT_3_ADDR(base) ((base) + (0x178))
#define PMIC_V_OUT0_PRE0_ADDR(base) ((base) + (0x179))
#define PMIC_V_OUT1_PRE0_ADDR(base) ((base) + (0x17A))
#define PMIC_V_OUT0_PRE1_ADDR(base) ((base) + (0x17B))
#define PMIC_V_OUT1_PRE1_ADDR(base) ((base) + (0x17C))
#define PMIC_V_OUT0_PRE2_ADDR(base) ((base) + (0x17D))
#define PMIC_V_OUT1_PRE2_ADDR(base) ((base) + (0x17E))
#define PMIC_V_OUT0_PRE3_ADDR(base) ((base) + (0x17F))
#define PMIC_V_OUT1_PRE3_ADDR(base) ((base) + (0x180))
#define PMIC_V_OUT0_PRE4_ADDR(base) ((base) + (0x181))
#define PMIC_V_OUT1_PRE4_ADDR(base) ((base) + (0x182))
#define PMIC_V_OUT0_PRE5_ADDR(base) ((base) + (0x183))
#define PMIC_V_OUT1_PRE5_ADDR(base) ((base) + (0x184))
#define PMIC_V_OUT0_PRE6_ADDR(base) ((base) + (0x185))
#define PMIC_V_OUT1_PRE6_ADDR(base) ((base) + (0x186))
#define PMIC_V_OUT0_PRE7_ADDR(base) ((base) + (0x187))
#define PMIC_V_OUT1_PRE7_ADDR(base) ((base) + (0x188))
#define PMIC_V_OUT0_PRE8_ADDR(base) ((base) + (0x189))
#define PMIC_V_OUT1_PRE8_ADDR(base) ((base) + (0x18A))
#define PMIC_V_OUT0_PRE9_ADDR(base) ((base) + (0x18B))
#define PMIC_V_OUT1_PRE9_ADDR(base) ((base) + (0x18C))
#define PMIC_CURRENT0_PRE0_ADDR(base) ((base) + (0x18D))
#define PMIC_CURRENT1_PRE0_ADDR(base) ((base) + (0x18E))
#define PMIC_CURRENT0_PRE1_ADDR(base) ((base) + (0x18F))
#define PMIC_CURRENT1_PRE1_ADDR(base) ((base) + (0x190))
#define PMIC_CURRENT0_PRE2_ADDR(base) ((base) + (0x191))
#define PMIC_CURRENT1_PRE2_ADDR(base) ((base) + (0x192))
#define PMIC_CURRENT0_PRE3_ADDR(base) ((base) + (0x193))
#define PMIC_CURRENT1_PRE3_ADDR(base) ((base) + (0x194))
#define PMIC_CURRENT0_PRE4_ADDR(base) ((base) + (0x195))
#define PMIC_CURRENT1_PRE4_ADDR(base) ((base) + (0x196))
#define PMIC_CURRENT0_PRE5_ADDR(base) ((base) + (0x197))
#define PMIC_CURRENT1_PRE5_ADDR(base) ((base) + (0x198))
#define PMIC_CURRENT0_PRE6_ADDR(base) ((base) + (0x199))
#define PMIC_CURRENT1_PRE6_ADDR(base) ((base) + (0x19A))
#define PMIC_CURRENT0_PRE7_ADDR(base) ((base) + (0x19B))
#define PMIC_CURRENT1_PRE7_ADDR(base) ((base) + (0x19C))
#define PMIC_CURRENT0_PRE8_ADDR(base) ((base) + (0x19D))
#define PMIC_CURRENT1_PRE8_ADDR(base) ((base) + (0x19E))
#define PMIC_CURRENT0_PRE9_ADDR(base) ((base) + (0x19F))
#define PMIC_CURRENT1_PRE9_ADDR(base) ((base) + (0x1A0))
#define PMIC_OFFSET_CURRENT_MOD_0_ADDR(base) ((base) + (0x1A1))
#define PMIC_OFFSET_CURRENT_MOD_1_ADDR(base) ((base) + (0x1A2))
#define PMIC_OFFSET_VOLTAGE_MOD_0_ADDR(base) ((base) + (0x1A3))
#define PMIC_OFFSET_VOLTAGE_MOD_1_ADDR(base) ((base) + (0x1A4))
#define PMIC_CLJ_RESERVED1_ADDR(base) ((base) + (0x1A5))
#define PMIC_CLJ_RESERVED2_ADDR(base) ((base) + (0x1A6))
#define PMIC_CLJ_RESERVED3_ADDR(base) ((base) + (0x1A7))
#define PMIC_CLJ_RESERVED4_ADDR(base) ((base) + (0x1A8))
#define PMIC_CLJ_RESERVED5_ADDR(base) ((base) + (0x1A9))
#define PMIC_CLJ_RESERVED6_ADDR(base) ((base) + (0x1AA))
#define PMIC_CLJ_RESERVED7_ADDR(base) ((base) + (0x1AB))
#define PMIC_PMU_SOFT_RST_ADDR(base) ((base) + (0x1AC))
#define PMIC_CLJ_DEBUG_ADDR(base) ((base) + (0x1AD))
#define PMIC_CLJ_DEBUG_2_ADDR(base) ((base) + (0x1AE))
#define PMIC_STATE_TEST_ADDR(base) ((base) + (0x1AF))
#define PMIC_COUL_RESERVE_ADDR(base) ((base) + (0x1B0))
#define PMIC_SOFT_RESERE0_ADDR(base) ((base) + (0x1B1))
#define PMIC_SOFT_RESERE1_ADDR(base) ((base) + (0x1B2))
#define PMIC_SOFT_RESERE2_ADDR(base) ((base) + (0x1B3))
#define PMIC_SOFT_RESERE3_ADDR(base) ((base) + (0x1B4))
#define PMIC_SOFT_RESERE4_ADDR(base) ((base) + (0x1B5))
#define PMIC_SOFT_RESERE5_ADDR(base) ((base) + (0x1B6))
#define PMIC_SOFT_RESERE6_ADDR(base) ((base) + (0x1B7))
#define PMIC_SOFT_RESERE7_ADDR(base) ((base) + (0x1B8))
#define PMIC_ADC_CTRL_ADDR(base) ((base) + (0x00))
#define PMIC_ADC_START_ADDR(base) ((base) + (0x01))
#define PMIC_CONV_STATUS_ADDR(base) ((base) + (0x02))
#define PMIC_ADC_DATA1_ADDR(base) ((base) + (0x03))
#define PMIC_ADC_DATA0_ADDR(base) ((base) + (0x04))
#define PMIC_ADC_CONV_ADDR(base) ((base) + (0x05))
#define PMIC_ADC_CURRENT_ADDR(base) ((base) + (0x06))
#define PMIC_ADC_CALI_CTRL_ADDR(base) ((base) + (0x07))
#define PMIC_ADC_CALI_VALUE_ADDR(base) ((base) + (0x08))
#define PMIC_ADC_CALI_CFG_ADDR(base) ((base) + (0x09))
#define PMIC_ADC_RSV0_ADDR(base) ((base) + (0x0A))
#define PMIC_ADC_RSV1_ADDR(base) ((base) + (0x0B))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char project_num0 : 8;
    } reg;
} PMIC_VERSION0_UNION;
#endif
#define PMIC_VERSION0_project_num0_START (0)
#define PMIC_VERSION0_project_num0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char project_num1 : 8;
    } reg;
} PMIC_VERSION1_UNION;
#endif
#define PMIC_VERSION1_project_num1_START (0)
#define PMIC_VERSION1_project_num1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char project_num2 : 8;
    } reg;
} PMIC_VERSION2_UNION;
#endif
#define PMIC_VERSION2_project_num2_START (0)
#define PMIC_VERSION2_project_num2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char project_num3 : 8;
    } reg;
} PMIC_VERSION3_UNION;
#endif
#define PMIC_VERSION3_project_num3_START (0)
#define PMIC_VERSION3_project_num3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char version0 : 8;
    } reg;
} PMIC_VERSION4_UNION;
#endif
#define PMIC_VERSION4_version0_START (0)
#define PMIC_VERSION4_version0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chip_id : 8;
    } reg;
} PMIC_VERSION5_UNION;
#endif
#define PMIC_VERSION5_chip_id_START (0)
#define PMIC_VERSION5_chip_id_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char vsys_ov_d200ur : 1;
        unsigned char vsys_uv_d10mr : 1;
        unsigned char vsys_pwroff_abs : 1;
        unsigned char vsys_pwroff_deb_d80mr : 1;
        unsigned char vsys_pwron_d60ur : 1;
        unsigned char thsd_otmp140_d1mr : 1;
        unsigned char thsd_otmp125_d1mr : 1;
        unsigned char pwron_d20m : 1;
    } reg;
} PMIC_STATUS0_UNION;
#endif
#define PMIC_STATUS0_vsys_ov_d200ur_START (0)
#define PMIC_STATUS0_vsys_ov_d200ur_END (0)
#define PMIC_STATUS0_vsys_uv_d10mr_START (1)
#define PMIC_STATUS0_vsys_uv_d10mr_END (1)
#define PMIC_STATUS0_vsys_pwroff_abs_START (2)
#define PMIC_STATUS0_vsys_pwroff_abs_END (2)
#define PMIC_STATUS0_vsys_pwroff_deb_d80mr_START (3)
#define PMIC_STATUS0_vsys_pwroff_deb_d80mr_END (3)
#define PMIC_STATUS0_vsys_pwron_d60ur_START (4)
#define PMIC_STATUS0_vsys_pwron_d60ur_END (4)
#define PMIC_STATUS0_thsd_otmp140_d1mr_START (5)
#define PMIC_STATUS0_thsd_otmp140_d1mr_END (5)
#define PMIC_STATUS0_thsd_otmp125_d1mr_START (6)
#define PMIC_STATUS0_thsd_otmp125_d1mr_END (6)
#define PMIC_STATUS0_pwron_d20m_START (7)
#define PMIC_STATUS0_pwron_d20m_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dcxo_clk_sel : 1;
        unsigned char tcxo_clk_sel : 1;
        unsigned char xo32k_mode_otp : 1;
        unsigned char buck1_vol_sel : 1;
        unsigned char sim0_hpd_d540u : 1;
        unsigned char sim1_hpd_d540u : 1;
        unsigned char alarm_on : 1;
        unsigned char vbus_det_insert_d20m : 1;
    } reg;
} PMIC_STATUS1_UNION;
#endif
#define PMIC_STATUS1_dcxo_clk_sel_START (0)
#define PMIC_STATUS1_dcxo_clk_sel_END (0)
#define PMIC_STATUS1_tcxo_clk_sel_START (1)
#define PMIC_STATUS1_tcxo_clk_sel_END (1)
#define PMIC_STATUS1_xo32k_mode_otp_START (2)
#define PMIC_STATUS1_xo32k_mode_otp_END (2)
#define PMIC_STATUS1_buck1_vol_sel_START (3)
#define PMIC_STATUS1_buck1_vol_sel_END (3)
#define PMIC_STATUS1_sim0_hpd_d540u_START (4)
#define PMIC_STATUS1_sim0_hpd_d540u_END (4)
#define PMIC_STATUS1_sim1_hpd_d540u_START (5)
#define PMIC_STATUS1_sim1_hpd_d540u_END (5)
#define PMIC_STATUS1_alarm_on_START (6)
#define PMIC_STATUS1_alarm_on_END (6)
#define PMIC_STATUS1_vbus_det_insert_d20m_START (7)
#define PMIC_STATUS1_vbus_det_insert_d20m_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_cali_thresold_0_3 : 4;
        unsigned char d2a_reserve0 : 4;
    } reg;
} PMIC_D2A_RES0_UNION;
#endif
#define PMIC_D2A_RES0_xo_cali_thresold_0_3_START (0)
#define PMIC_D2A_RES0_xo_cali_thresold_0_3_END (3)
#define PMIC_D2A_RES0_d2a_reserve0_START (4)
#define PMIC_D2A_RES0_d2a_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_cali_thresold_4_7 : 4;
        unsigned char d2a_reserve1 : 4;
    } reg;
} PMIC_D2A_RES1_UNION;
#endif
#define PMIC_D2A_RES1_xo_cali_thresold_4_7_START (0)
#define PMIC_D2A_RES1_xo_cali_thresold_4_7_END (3)
#define PMIC_D2A_RES1_d2a_reserve1_START (4)
#define PMIC_D2A_RES1_d2a_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_cali_thresold_8_15 : 8;
    } reg;
} PMIC_D2A_RES2_UNION;
#endif
#define PMIC_D2A_RES2_xo_cali_thresold_8_15_START (0)
#define PMIC_D2A_RES2_xo_cali_thresold_8_15_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_19m2_sel : 1;
        unsigned char xo_19m2_abnor_n : 1;
        unsigned char a2d_reserve0 : 6;
    } reg;
} PMIC_A2D_RES0_UNION;
#endif
#define PMIC_A2D_RES0_xo_19m2_sel_START (0)
#define PMIC_A2D_RES0_xo_19m2_sel_END (0)
#define PMIC_A2D_RES0_xo_19m2_abnor_n_START (1)
#define PMIC_A2D_RES0_xo_19m2_abnor_n_END (1)
#define PMIC_A2D_RES0_a2d_reserve0_START (2)
#define PMIC_A2D_RES0_a2d_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char a2d_reserve1 : 8;
    } reg;
} PMIC_A2D_RES1_UNION;
#endif
#define PMIC_A2D_RES1_a2d_reserve1_START (0)
#define PMIC_A2D_RES1_a2d_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_adj_clx : 4;
        unsigned char buck0_adj_rlx : 4;
    } reg;
} PMIC_BUCK0_CTRL0_UNION;
#endif
#define PMIC_BUCK0_CTRL0_buck0_adj_clx_START (0)
#define PMIC_BUCK0_CTRL0_buck0_adj_clx_END (3)
#define PMIC_BUCK0_CTRL0_buck0_adj_rlx_START (4)
#define PMIC_BUCK0_CTRL0_buck0_adj_rlx_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_ng_dt_sel : 1;
        unsigned char buck0_pg_dt_sel : 1;
        unsigned char buck0_sft_sel : 1;
        unsigned char reserved : 1;
        unsigned char buck0_dt_sel : 2;
        unsigned char buck0_ocp_sel : 2;
    } reg;
} PMIC_BUCK0_CTRL1_UNION;
#endif
#define PMIC_BUCK0_CTRL1_buck0_ng_dt_sel_START (0)
#define PMIC_BUCK0_CTRL1_buck0_ng_dt_sel_END (0)
#define PMIC_BUCK0_CTRL1_buck0_pg_dt_sel_START (1)
#define PMIC_BUCK0_CTRL1_buck0_pg_dt_sel_END (1)
#define PMIC_BUCK0_CTRL1_buck0_sft_sel_START (2)
#define PMIC_BUCK0_CTRL1_buck0_sft_sel_END (2)
#define PMIC_BUCK0_CTRL1_buck0_dt_sel_START (4)
#define PMIC_BUCK0_CTRL1_buck0_dt_sel_END (5)
#define PMIC_BUCK0_CTRL1_buck0_ocp_sel_START (6)
#define PMIC_BUCK0_CTRL1_buck0_ocp_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_ng_n_sel : 2;
        unsigned char buck0_ng_p_sel : 2;
        unsigned char buck0_pg_n_sel : 2;
        unsigned char buck0_pg_p_sel : 2;
    } reg;
} PMIC_BUCK0_CTRL2_UNION;
#endif
#define PMIC_BUCK0_CTRL2_buck0_ng_n_sel_START (0)
#define PMIC_BUCK0_CTRL2_buck0_ng_n_sel_END (1)
#define PMIC_BUCK0_CTRL2_buck0_ng_p_sel_START (2)
#define PMIC_BUCK0_CTRL2_buck0_ng_p_sel_END (3)
#define PMIC_BUCK0_CTRL2_buck0_pg_n_sel_START (4)
#define PMIC_BUCK0_CTRL2_buck0_pg_n_sel_END (5)
#define PMIC_BUCK0_CTRL2_buck0_pg_p_sel_START (6)
#define PMIC_BUCK0_CTRL2_buck0_pg_p_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_reg_r : 2;
        unsigned char reserved : 2;
        unsigned char buck0_reg_en : 1;
        unsigned char buck0_dbias : 2;
        unsigned char buck0_ocp_d : 1;
    } reg;
} PMIC_BUCK0_CTRL3_UNION;
#endif
#define PMIC_BUCK0_CTRL3_buck0_reg_r_START (0)
#define PMIC_BUCK0_CTRL3_buck0_reg_r_END (1)
#define PMIC_BUCK0_CTRL3_buck0_reg_en_START (4)
#define PMIC_BUCK0_CTRL3_buck0_reg_en_END (4)
#define PMIC_BUCK0_CTRL3_buck0_dbias_START (5)
#define PMIC_BUCK0_CTRL3_buck0_dbias_END (6)
#define PMIC_BUCK0_CTRL3_buck0_ocp_d_START (7)
#define PMIC_BUCK0_CTRL3_buck0_ocp_d_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_short_pdp : 1;
        unsigned char buck0_reg_ss_d : 1;
        unsigned char buck0_regop_c : 1;
        unsigned char buck0_reg_idr : 2;
        unsigned char buck0_reg_dr : 3;
    } reg;
} PMIC_BUCK0_CTRL4_UNION;
#endif
#define PMIC_BUCK0_CTRL4_buck0_short_pdp_START (0)
#define PMIC_BUCK0_CTRL4_buck0_short_pdp_END (0)
#define PMIC_BUCK0_CTRL4_buck0_reg_ss_d_START (1)
#define PMIC_BUCK0_CTRL4_buck0_reg_ss_d_END (1)
#define PMIC_BUCK0_CTRL4_buck0_regop_c_START (2)
#define PMIC_BUCK0_CTRL4_buck0_regop_c_END (2)
#define PMIC_BUCK0_CTRL4_buck0_reg_idr_START (3)
#define PMIC_BUCK0_CTRL4_buck0_reg_idr_END (4)
#define PMIC_BUCK0_CTRL4_buck0_reg_dr_START (5)
#define PMIC_BUCK0_CTRL4_buck0_reg_dr_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_ton : 2;
        unsigned char buck0_nmos_off : 1;
        unsigned char buck0_reg_c : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_BUCK0_CTRL5_UNION;
#endif
#define PMIC_BUCK0_CTRL5_buck0_ton_START (0)
#define PMIC_BUCK0_CTRL5_buck0_ton_END (1)
#define PMIC_BUCK0_CTRL5_buck0_nmos_off_START (2)
#define PMIC_BUCK0_CTRL5_buck0_nmos_off_END (2)
#define PMIC_BUCK0_CTRL5_buck0_reg_c_START (3)
#define PMIC_BUCK0_CTRL5_buck0_reg_c_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_shield_i : 2;
        unsigned char buck0_new_dmd_sel : 5;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK0_CTRL6_UNION;
#endif
#define PMIC_BUCK0_CTRL6_buck0_shield_i_START (0)
#define PMIC_BUCK0_CTRL6_buck0_shield_i_END (1)
#define PMIC_BUCK0_CTRL6_buck0_new_dmd_sel_START (2)
#define PMIC_BUCK0_CTRL6_buck0_new_dmd_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_dmd_sel : 3;
        unsigned char buck0_mos_sel : 2;
        unsigned char reserved : 3;
    } reg;
} PMIC_BUCK0_CTRL7_UNION;
#endif
#define PMIC_BUCK0_CTRL7_buck0_dmd_sel_START (0)
#define PMIC_BUCK0_CTRL7_buck0_dmd_sel_END (2)
#define PMIC_BUCK0_CTRL7_buck0_mos_sel_START (3)
#define PMIC_BUCK0_CTRL7_buck0_mos_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_reserve0 : 8;
    } reg;
} PMIC_BUCK0_CTRL8_UNION;
#endif
#define PMIC_BUCK0_CTRL8_buck0_reserve0_START (0)
#define PMIC_BUCK0_CTRL8_buck0_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_reserve1 : 8;
    } reg;
} PMIC_BUCK0_CTRL9_UNION;
#endif
#define PMIC_BUCK0_CTRL9_buck0_reserve1_START (0)
#define PMIC_BUCK0_CTRL9_buck0_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_reserve2 : 8;
    } reg;
} PMIC_BUCK0_CTRL10_UNION;
#endif
#define PMIC_BUCK0_CTRL10_buck0_reserve2_START (0)
#define PMIC_BUCK0_CTRL10_buck0_reserve2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_pdn_lx_det : 1;
        unsigned char buck0_eco_dmd : 1;
        unsigned char buck0_dmd_shield_n : 1;
        unsigned char buck0_ocp_delay_sel : 1;
        unsigned char buck0_dmd_clamp : 1;
        unsigned char buck0_en_regop_clamp : 1;
        unsigned char buck0_en_ss_sel : 1;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK0_CTRL11_UNION;
#endif
#define PMIC_BUCK0_CTRL11_buck0_pdn_lx_det_START (0)
#define PMIC_BUCK0_CTRL11_buck0_pdn_lx_det_END (0)
#define PMIC_BUCK0_CTRL11_buck0_eco_dmd_START (1)
#define PMIC_BUCK0_CTRL11_buck0_eco_dmd_END (1)
#define PMIC_BUCK0_CTRL11_buck0_dmd_shield_n_START (2)
#define PMIC_BUCK0_CTRL11_buck0_dmd_shield_n_END (2)
#define PMIC_BUCK0_CTRL11_buck0_ocp_delay_sel_START (3)
#define PMIC_BUCK0_CTRL11_buck0_ocp_delay_sel_END (3)
#define PMIC_BUCK0_CTRL11_buck0_dmd_clamp_START (4)
#define PMIC_BUCK0_CTRL11_buck0_dmd_clamp_END (4)
#define PMIC_BUCK0_CTRL11_buck0_en_regop_clamp_START (5)
#define PMIC_BUCK0_CTRL11_buck0_en_regop_clamp_END (5)
#define PMIC_BUCK0_CTRL11_buck0_en_ss_sel_START (6)
#define PMIC_BUCK0_CTRL11_buck0_en_ss_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_adj_clx : 4;
        unsigned char buck1_adj_rlx : 4;
    } reg;
} PMIC_BUCK1_CTRL0_UNION;
#endif
#define PMIC_BUCK1_CTRL0_buck1_adj_clx_START (0)
#define PMIC_BUCK1_CTRL0_buck1_adj_clx_END (3)
#define PMIC_BUCK1_CTRL0_buck1_adj_rlx_START (4)
#define PMIC_BUCK1_CTRL0_buck1_adj_rlx_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_ng_dt_sel : 1;
        unsigned char buck1_pg_dt_sel : 1;
        unsigned char buck1_sft_sel : 1;
        unsigned char reserved : 1;
        unsigned char buck1_dt_sel : 2;
        unsigned char buck1_ocp_sel : 2;
    } reg;
} PMIC_BUCK1_CTRL1_UNION;
#endif
#define PMIC_BUCK1_CTRL1_buck1_ng_dt_sel_START (0)
#define PMIC_BUCK1_CTRL1_buck1_ng_dt_sel_END (0)
#define PMIC_BUCK1_CTRL1_buck1_pg_dt_sel_START (1)
#define PMIC_BUCK1_CTRL1_buck1_pg_dt_sel_END (1)
#define PMIC_BUCK1_CTRL1_buck1_sft_sel_START (2)
#define PMIC_BUCK1_CTRL1_buck1_sft_sel_END (2)
#define PMIC_BUCK1_CTRL1_buck1_dt_sel_START (4)
#define PMIC_BUCK1_CTRL1_buck1_dt_sel_END (5)
#define PMIC_BUCK1_CTRL1_buck1_ocp_sel_START (6)
#define PMIC_BUCK1_CTRL1_buck1_ocp_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_ng_n_sel : 2;
        unsigned char buck1_ng_p_sel : 2;
        unsigned char buck1_pg_n_sel : 2;
        unsigned char buck1_pg_p_sel : 2;
    } reg;
} PMIC_BUCK1_CTRL2_UNION;
#endif
#define PMIC_BUCK1_CTRL2_buck1_ng_n_sel_START (0)
#define PMIC_BUCK1_CTRL2_buck1_ng_n_sel_END (1)
#define PMIC_BUCK1_CTRL2_buck1_ng_p_sel_START (2)
#define PMIC_BUCK1_CTRL2_buck1_ng_p_sel_END (3)
#define PMIC_BUCK1_CTRL2_buck1_pg_n_sel_START (4)
#define PMIC_BUCK1_CTRL2_buck1_pg_n_sel_END (5)
#define PMIC_BUCK1_CTRL2_buck1_pg_p_sel_START (6)
#define PMIC_BUCK1_CTRL2_buck1_pg_p_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_reg_r : 2;
        unsigned char reserved : 2;
        unsigned char buck1_reg_en : 1;
        unsigned char buck1_dbias : 2;
        unsigned char buck1_ocp_d : 1;
    } reg;
} PMIC_BUCK1_CTRL3_UNION;
#endif
#define PMIC_BUCK1_CTRL3_buck1_reg_r_START (0)
#define PMIC_BUCK1_CTRL3_buck1_reg_r_END (1)
#define PMIC_BUCK1_CTRL3_buck1_reg_en_START (4)
#define PMIC_BUCK1_CTRL3_buck1_reg_en_END (4)
#define PMIC_BUCK1_CTRL3_buck1_dbias_START (5)
#define PMIC_BUCK1_CTRL3_buck1_dbias_END (6)
#define PMIC_BUCK1_CTRL3_buck1_ocp_d_START (7)
#define PMIC_BUCK1_CTRL3_buck1_ocp_d_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_short_pdp : 1;
        unsigned char buck1_reg_ss_d : 1;
        unsigned char buck1_regop_c : 1;
        unsigned char buck1_reg_idr : 2;
        unsigned char buck1_reg_dr : 3;
    } reg;
} PMIC_BUCK1_CTRL4_UNION;
#endif
#define PMIC_BUCK1_CTRL4_buck1_short_pdp_START (0)
#define PMIC_BUCK1_CTRL4_buck1_short_pdp_END (0)
#define PMIC_BUCK1_CTRL4_buck1_reg_ss_d_START (1)
#define PMIC_BUCK1_CTRL4_buck1_reg_ss_d_END (1)
#define PMIC_BUCK1_CTRL4_buck1_regop_c_START (2)
#define PMIC_BUCK1_CTRL4_buck1_regop_c_END (2)
#define PMIC_BUCK1_CTRL4_buck1_reg_idr_START (3)
#define PMIC_BUCK1_CTRL4_buck1_reg_idr_END (4)
#define PMIC_BUCK1_CTRL4_buck1_reg_dr_START (5)
#define PMIC_BUCK1_CTRL4_buck1_reg_dr_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_ton : 2;
        unsigned char buck1_nmos_off : 1;
        unsigned char buck1_reg_c : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_BUCK1_CTRL5_UNION;
#endif
#define PMIC_BUCK1_CTRL5_buck1_ton_START (0)
#define PMIC_BUCK1_CTRL5_buck1_ton_END (1)
#define PMIC_BUCK1_CTRL5_buck1_nmos_off_START (2)
#define PMIC_BUCK1_CTRL5_buck1_nmos_off_END (2)
#define PMIC_BUCK1_CTRL5_buck1_reg_c_START (3)
#define PMIC_BUCK1_CTRL5_buck1_reg_c_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_shield_i : 2;
        unsigned char buck1_new_dmd_sel : 5;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK1_CTRL6_UNION;
#endif
#define PMIC_BUCK1_CTRL6_buck1_shield_i_START (0)
#define PMIC_BUCK1_CTRL6_buck1_shield_i_END (1)
#define PMIC_BUCK1_CTRL6_buck1_new_dmd_sel_START (2)
#define PMIC_BUCK1_CTRL6_buck1_new_dmd_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_dmd_sel : 3;
        unsigned char buck1_mos_sel : 2;
        unsigned char reserved : 3;
    } reg;
} PMIC_BUCK1_CTRL7_UNION;
#endif
#define PMIC_BUCK1_CTRL7_buck1_dmd_sel_START (0)
#define PMIC_BUCK1_CTRL7_buck1_dmd_sel_END (2)
#define PMIC_BUCK1_CTRL7_buck1_mos_sel_START (3)
#define PMIC_BUCK1_CTRL7_buck1_mos_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_reserve0 : 8;
    } reg;
} PMIC_BUCK1_CTRL8_UNION;
#endif
#define PMIC_BUCK1_CTRL8_buck1_reserve0_START (0)
#define PMIC_BUCK1_CTRL8_buck1_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_reserve1 : 8;
    } reg;
} PMIC_BUCK1_CTRL9_UNION;
#endif
#define PMIC_BUCK1_CTRL9_buck1_reserve1_START (0)
#define PMIC_BUCK1_CTRL9_buck1_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_reserve2 : 8;
    } reg;
} PMIC_BUCK1_CTRL10_UNION;
#endif
#define PMIC_BUCK1_CTRL10_buck1_reserve2_START (0)
#define PMIC_BUCK1_CTRL10_buck1_reserve2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_pdn_lx_det : 1;
        unsigned char buck1_eco_dmd : 1;
        unsigned char buck1_dmd_shield_n : 1;
        unsigned char buck1_ocp_delay_sel : 1;
        unsigned char buck1_dmd_clamp : 1;
        unsigned char buck1_en_regop_clamp : 1;
        unsigned char buck1_en_ss_sel : 1;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK1_CTRL11_UNION;
#endif
#define PMIC_BUCK1_CTRL11_buck1_pdn_lx_det_START (0)
#define PMIC_BUCK1_CTRL11_buck1_pdn_lx_det_END (0)
#define PMIC_BUCK1_CTRL11_buck1_eco_dmd_START (1)
#define PMIC_BUCK1_CTRL11_buck1_eco_dmd_END (1)
#define PMIC_BUCK1_CTRL11_buck1_dmd_shield_n_START (2)
#define PMIC_BUCK1_CTRL11_buck1_dmd_shield_n_END (2)
#define PMIC_BUCK1_CTRL11_buck1_ocp_delay_sel_START (3)
#define PMIC_BUCK1_CTRL11_buck1_ocp_delay_sel_END (3)
#define PMIC_BUCK1_CTRL11_buck1_dmd_clamp_START (4)
#define PMIC_BUCK1_CTRL11_buck1_dmd_clamp_END (4)
#define PMIC_BUCK1_CTRL11_buck1_en_regop_clamp_START (5)
#define PMIC_BUCK1_CTRL11_buck1_en_regop_clamp_END (5)
#define PMIC_BUCK1_CTRL11_buck1_en_ss_sel_START (6)
#define PMIC_BUCK1_CTRL11_buck1_en_ss_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_adj_clx : 4;
        unsigned char buck2_adj_rlx : 4;
    } reg;
} PMIC_BUCK2_CTRL0_UNION;
#endif
#define PMIC_BUCK2_CTRL0_buck2_adj_clx_START (0)
#define PMIC_BUCK2_CTRL0_buck2_adj_clx_END (3)
#define PMIC_BUCK2_CTRL0_buck2_adj_rlx_START (4)
#define PMIC_BUCK2_CTRL0_buck2_adj_rlx_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_ng_dt_sel : 1;
        unsigned char buck2_pg_dt_sel : 1;
        unsigned char buck2_sft_sel : 1;
        unsigned char reserved : 1;
        unsigned char buck2_dt_sel : 2;
        unsigned char buck2_ocp_sel : 2;
    } reg;
} PMIC_BUCK2_CTRL1_UNION;
#endif
#define PMIC_BUCK2_CTRL1_buck2_ng_dt_sel_START (0)
#define PMIC_BUCK2_CTRL1_buck2_ng_dt_sel_END (0)
#define PMIC_BUCK2_CTRL1_buck2_pg_dt_sel_START (1)
#define PMIC_BUCK2_CTRL1_buck2_pg_dt_sel_END (1)
#define PMIC_BUCK2_CTRL1_buck2_sft_sel_START (2)
#define PMIC_BUCK2_CTRL1_buck2_sft_sel_END (2)
#define PMIC_BUCK2_CTRL1_buck2_dt_sel_START (4)
#define PMIC_BUCK2_CTRL1_buck2_dt_sel_END (5)
#define PMIC_BUCK2_CTRL1_buck2_ocp_sel_START (6)
#define PMIC_BUCK2_CTRL1_buck2_ocp_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_ng_n_sel : 2;
        unsigned char buck2_ng_p_sel : 2;
        unsigned char buck2_pg_n_sel : 2;
        unsigned char buck2_pg_p_sel : 2;
    } reg;
} PMIC_BUCK2_CTRL2_UNION;
#endif
#define PMIC_BUCK2_CTRL2_buck2_ng_n_sel_START (0)
#define PMIC_BUCK2_CTRL2_buck2_ng_n_sel_END (1)
#define PMIC_BUCK2_CTRL2_buck2_ng_p_sel_START (2)
#define PMIC_BUCK2_CTRL2_buck2_ng_p_sel_END (3)
#define PMIC_BUCK2_CTRL2_buck2_pg_n_sel_START (4)
#define PMIC_BUCK2_CTRL2_buck2_pg_n_sel_END (5)
#define PMIC_BUCK2_CTRL2_buck2_pg_p_sel_START (6)
#define PMIC_BUCK2_CTRL2_buck2_pg_p_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_reg_r : 2;
        unsigned char reserved : 2;
        unsigned char buck2_reg_en : 1;
        unsigned char buck2_dbias : 2;
        unsigned char buck2_ocp_d : 1;
    } reg;
} PMIC_BUCK2_CTRL3_UNION;
#endif
#define PMIC_BUCK2_CTRL3_buck2_reg_r_START (0)
#define PMIC_BUCK2_CTRL3_buck2_reg_r_END (1)
#define PMIC_BUCK2_CTRL3_buck2_reg_en_START (4)
#define PMIC_BUCK2_CTRL3_buck2_reg_en_END (4)
#define PMIC_BUCK2_CTRL3_buck2_dbias_START (5)
#define PMIC_BUCK2_CTRL3_buck2_dbias_END (6)
#define PMIC_BUCK2_CTRL3_buck2_ocp_d_START (7)
#define PMIC_BUCK2_CTRL3_buck2_ocp_d_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_short_pdp : 1;
        unsigned char buck2_reg_ss_d : 1;
        unsigned char buck2_regop_c : 1;
        unsigned char buck2_reg_idr : 2;
        unsigned char buck2_reg_dr : 3;
    } reg;
} PMIC_BUCK2_CTRL4_UNION;
#endif
#define PMIC_BUCK2_CTRL4_buck2_short_pdp_START (0)
#define PMIC_BUCK2_CTRL4_buck2_short_pdp_END (0)
#define PMIC_BUCK2_CTRL4_buck2_reg_ss_d_START (1)
#define PMIC_BUCK2_CTRL4_buck2_reg_ss_d_END (1)
#define PMIC_BUCK2_CTRL4_buck2_regop_c_START (2)
#define PMIC_BUCK2_CTRL4_buck2_regop_c_END (2)
#define PMIC_BUCK2_CTRL4_buck2_reg_idr_START (3)
#define PMIC_BUCK2_CTRL4_buck2_reg_idr_END (4)
#define PMIC_BUCK2_CTRL4_buck2_reg_dr_START (5)
#define PMIC_BUCK2_CTRL4_buck2_reg_dr_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_ton : 2;
        unsigned char buck2_nmos_off : 1;
        unsigned char buck2_reg_c : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_BUCK2_CTRL5_UNION;
#endif
#define PMIC_BUCK2_CTRL5_buck2_ton_START (0)
#define PMIC_BUCK2_CTRL5_buck2_ton_END (1)
#define PMIC_BUCK2_CTRL5_buck2_nmos_off_START (2)
#define PMIC_BUCK2_CTRL5_buck2_nmos_off_END (2)
#define PMIC_BUCK2_CTRL5_buck2_reg_c_START (3)
#define PMIC_BUCK2_CTRL5_buck2_reg_c_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_shield_i : 2;
        unsigned char buck2_new_dmd_sel : 5;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK2_CTRL6_UNION;
#endif
#define PMIC_BUCK2_CTRL6_buck2_shield_i_START (0)
#define PMIC_BUCK2_CTRL6_buck2_shield_i_END (1)
#define PMIC_BUCK2_CTRL6_buck2_new_dmd_sel_START (2)
#define PMIC_BUCK2_CTRL6_buck2_new_dmd_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_dmd_sel : 3;
        unsigned char buck2_mos_sel : 2;
        unsigned char reserved : 3;
    } reg;
} PMIC_BUCK2_CTRL7_UNION;
#endif
#define PMIC_BUCK2_CTRL7_buck2_dmd_sel_START (0)
#define PMIC_BUCK2_CTRL7_buck2_dmd_sel_END (2)
#define PMIC_BUCK2_CTRL7_buck2_mos_sel_START (3)
#define PMIC_BUCK2_CTRL7_buck2_mos_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_reserve0 : 8;
    } reg;
} PMIC_BUCK2_CTRL8_UNION;
#endif
#define PMIC_BUCK2_CTRL8_buck2_reserve0_START (0)
#define PMIC_BUCK2_CTRL8_buck2_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_reserve1 : 8;
    } reg;
} PMIC_BUCK2_CTRL9_UNION;
#endif
#define PMIC_BUCK2_CTRL9_buck2_reserve1_START (0)
#define PMIC_BUCK2_CTRL9_buck2_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_reserve2 : 8;
    } reg;
} PMIC_BUCK2_CTRL10_UNION;
#endif
#define PMIC_BUCK2_CTRL10_buck2_reserve2_START (0)
#define PMIC_BUCK2_CTRL10_buck2_reserve2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_pdn_lx_det : 1;
        unsigned char buck2_eco_dmd : 1;
        unsigned char buck2_dmd_shield_n : 1;
        unsigned char buck2_ocp_delay_sel : 1;
        unsigned char buck2_dmd_clamp : 1;
        unsigned char buck2_en_regop_clamp : 1;
        unsigned char buck2_en_ss_sel : 1;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK2_CTRL11_UNION;
#endif
#define PMIC_BUCK2_CTRL11_buck2_pdn_lx_det_START (0)
#define PMIC_BUCK2_CTRL11_buck2_pdn_lx_det_END (0)
#define PMIC_BUCK2_CTRL11_buck2_eco_dmd_START (1)
#define PMIC_BUCK2_CTRL11_buck2_eco_dmd_END (1)
#define PMIC_BUCK2_CTRL11_buck2_dmd_shield_n_START (2)
#define PMIC_BUCK2_CTRL11_buck2_dmd_shield_n_END (2)
#define PMIC_BUCK2_CTRL11_buck2_ocp_delay_sel_START (3)
#define PMIC_BUCK2_CTRL11_buck2_ocp_delay_sel_END (3)
#define PMIC_BUCK2_CTRL11_buck2_dmd_clamp_START (4)
#define PMIC_BUCK2_CTRL11_buck2_dmd_clamp_END (4)
#define PMIC_BUCK2_CTRL11_buck2_en_regop_clamp_START (5)
#define PMIC_BUCK2_CTRL11_buck2_en_regop_clamp_END (5)
#define PMIC_BUCK2_CTRL11_buck2_en_ss_sel_START (6)
#define PMIC_BUCK2_CTRL11_buck2_en_ss_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_adj_clx : 4;
        unsigned char buck3_adj_rlx : 4;
    } reg;
} PMIC_BUCK3_CTRL0_UNION;
#endif
#define PMIC_BUCK3_CTRL0_buck3_adj_clx_START (0)
#define PMIC_BUCK3_CTRL0_buck3_adj_clx_END (3)
#define PMIC_BUCK3_CTRL0_buck3_adj_rlx_START (4)
#define PMIC_BUCK3_CTRL0_buck3_adj_rlx_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_ng_dt_sel : 1;
        unsigned char buck3_pg_dt_sel : 1;
        unsigned char buck3_sft_sel : 1;
        unsigned char reserved : 1;
        unsigned char buck3_dt_sel : 2;
        unsigned char buck3_ocp_sel : 2;
    } reg;
} PMIC_BUCK3_CTRL1_UNION;
#endif
#define PMIC_BUCK3_CTRL1_buck3_ng_dt_sel_START (0)
#define PMIC_BUCK3_CTRL1_buck3_ng_dt_sel_END (0)
#define PMIC_BUCK3_CTRL1_buck3_pg_dt_sel_START (1)
#define PMIC_BUCK3_CTRL1_buck3_pg_dt_sel_END (1)
#define PMIC_BUCK3_CTRL1_buck3_sft_sel_START (2)
#define PMIC_BUCK3_CTRL1_buck3_sft_sel_END (2)
#define PMIC_BUCK3_CTRL1_buck3_dt_sel_START (4)
#define PMIC_BUCK3_CTRL1_buck3_dt_sel_END (5)
#define PMIC_BUCK3_CTRL1_buck3_ocp_sel_START (6)
#define PMIC_BUCK3_CTRL1_buck3_ocp_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_ng_n_sel : 2;
        unsigned char buck3_ng_p_sel : 2;
        unsigned char buck3_pg_n_sel : 2;
        unsigned char buck3_pg_p_sel : 2;
    } reg;
} PMIC_BUCK3_CTRL2_UNION;
#endif
#define PMIC_BUCK3_CTRL2_buck3_ng_n_sel_START (0)
#define PMIC_BUCK3_CTRL2_buck3_ng_n_sel_END (1)
#define PMIC_BUCK3_CTRL2_buck3_ng_p_sel_START (2)
#define PMIC_BUCK3_CTRL2_buck3_ng_p_sel_END (3)
#define PMIC_BUCK3_CTRL2_buck3_pg_n_sel_START (4)
#define PMIC_BUCK3_CTRL2_buck3_pg_n_sel_END (5)
#define PMIC_BUCK3_CTRL2_buck3_pg_p_sel_START (6)
#define PMIC_BUCK3_CTRL2_buck3_pg_p_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_reg_r : 2;
        unsigned char reserved : 2;
        unsigned char buck3_reg_en : 1;
        unsigned char buck3_dbias : 2;
        unsigned char buck3_ocp_d : 1;
    } reg;
} PMIC_BUCK3_CTRL3_UNION;
#endif
#define PMIC_BUCK3_CTRL3_buck3_reg_r_START (0)
#define PMIC_BUCK3_CTRL3_buck3_reg_r_END (1)
#define PMIC_BUCK3_CTRL3_buck3_reg_en_START (4)
#define PMIC_BUCK3_CTRL3_buck3_reg_en_END (4)
#define PMIC_BUCK3_CTRL3_buck3_dbias_START (5)
#define PMIC_BUCK3_CTRL3_buck3_dbias_END (6)
#define PMIC_BUCK3_CTRL3_buck3_ocp_d_START (7)
#define PMIC_BUCK3_CTRL3_buck3_ocp_d_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_short_pdp : 1;
        unsigned char buck3_reg_ss_d : 1;
        unsigned char buck3_regop_c : 1;
        unsigned char buck3_reg_idr : 2;
        unsigned char buck3_reg_dr : 3;
    } reg;
} PMIC_BUCK3_CTRL4_UNION;
#endif
#define PMIC_BUCK3_CTRL4_buck3_short_pdp_START (0)
#define PMIC_BUCK3_CTRL4_buck3_short_pdp_END (0)
#define PMIC_BUCK3_CTRL4_buck3_reg_ss_d_START (1)
#define PMIC_BUCK3_CTRL4_buck3_reg_ss_d_END (1)
#define PMIC_BUCK3_CTRL4_buck3_regop_c_START (2)
#define PMIC_BUCK3_CTRL4_buck3_regop_c_END (2)
#define PMIC_BUCK3_CTRL4_buck3_reg_idr_START (3)
#define PMIC_BUCK3_CTRL4_buck3_reg_idr_END (4)
#define PMIC_BUCK3_CTRL4_buck3_reg_dr_START (5)
#define PMIC_BUCK3_CTRL4_buck3_reg_dr_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_ton : 2;
        unsigned char buck3_nmos_off : 1;
        unsigned char buck3_reg_c : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_BUCK3_CTRL5_UNION;
#endif
#define PMIC_BUCK3_CTRL5_buck3_ton_START (0)
#define PMIC_BUCK3_CTRL5_buck3_ton_END (1)
#define PMIC_BUCK3_CTRL5_buck3_nmos_off_START (2)
#define PMIC_BUCK3_CTRL5_buck3_nmos_off_END (2)
#define PMIC_BUCK3_CTRL5_buck3_reg_c_START (3)
#define PMIC_BUCK3_CTRL5_buck3_reg_c_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_shield_i : 2;
        unsigned char buck3_new_dmd_sel : 5;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK3_CTRL6_UNION;
#endif
#define PMIC_BUCK3_CTRL6_buck3_shield_i_START (0)
#define PMIC_BUCK3_CTRL6_buck3_shield_i_END (1)
#define PMIC_BUCK3_CTRL6_buck3_new_dmd_sel_START (2)
#define PMIC_BUCK3_CTRL6_buck3_new_dmd_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_dmd_sel : 3;
        unsigned char buck3_mos_sel : 2;
        unsigned char reserved : 3;
    } reg;
} PMIC_BUCK3_CTRL7_UNION;
#endif
#define PMIC_BUCK3_CTRL7_buck3_dmd_sel_START (0)
#define PMIC_BUCK3_CTRL7_buck3_dmd_sel_END (2)
#define PMIC_BUCK3_CTRL7_buck3_mos_sel_START (3)
#define PMIC_BUCK3_CTRL7_buck3_mos_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_reserve0 : 8;
    } reg;
} PMIC_BUCK3_CTRL8_UNION;
#endif
#define PMIC_BUCK3_CTRL8_buck3_reserve0_START (0)
#define PMIC_BUCK3_CTRL8_buck3_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_reserve1 : 8;
    } reg;
} PMIC_BUCK3_CTRL9_UNION;
#endif
#define PMIC_BUCK3_CTRL9_buck3_reserve1_START (0)
#define PMIC_BUCK3_CTRL9_buck3_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_reserve2 : 8;
    } reg;
} PMIC_BUCK3_CTRL10_UNION;
#endif
#define PMIC_BUCK3_CTRL10_buck3_reserve2_START (0)
#define PMIC_BUCK3_CTRL10_buck3_reserve2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_pdn_lx_det : 1;
        unsigned char buck3_eco_dmd : 1;
        unsigned char buck3_dmd_shield_n : 1;
        unsigned char buck3_ocp_delay_sel : 1;
        unsigned char buck3_dmd_clamp : 1;
        unsigned char buck3_en_regop_clamp : 1;
        unsigned char buck3_en_ss_sel : 1;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK3_CTRL11_UNION;
#endif
#define PMIC_BUCK3_CTRL11_buck3_pdn_lx_det_START (0)
#define PMIC_BUCK3_CTRL11_buck3_pdn_lx_det_END (0)
#define PMIC_BUCK3_CTRL11_buck3_eco_dmd_START (1)
#define PMIC_BUCK3_CTRL11_buck3_eco_dmd_END (1)
#define PMIC_BUCK3_CTRL11_buck3_dmd_shield_n_START (2)
#define PMIC_BUCK3_CTRL11_buck3_dmd_shield_n_END (2)
#define PMIC_BUCK3_CTRL11_buck3_ocp_delay_sel_START (3)
#define PMIC_BUCK3_CTRL11_buck3_ocp_delay_sel_END (3)
#define PMIC_BUCK3_CTRL11_buck3_dmd_clamp_START (4)
#define PMIC_BUCK3_CTRL11_buck3_dmd_clamp_END (4)
#define PMIC_BUCK3_CTRL11_buck3_en_regop_clamp_START (5)
#define PMIC_BUCK3_CTRL11_buck3_en_regop_clamp_END (5)
#define PMIC_BUCK3_CTRL11_buck3_en_ss_sel_START (6)
#define PMIC_BUCK3_CTRL11_buck3_en_ss_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_adj_clx : 4;
        unsigned char buck4_adj_rlx : 4;
    } reg;
} PMIC_BUCK4_CTRL0_UNION;
#endif
#define PMIC_BUCK4_CTRL0_buck4_adj_clx_START (0)
#define PMIC_BUCK4_CTRL0_buck4_adj_clx_END (3)
#define PMIC_BUCK4_CTRL0_buck4_adj_rlx_START (4)
#define PMIC_BUCK4_CTRL0_buck4_adj_rlx_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_ng_dt_sel : 1;
        unsigned char buck4_pg_dt_sel : 1;
        unsigned char buck4_sft_sel : 1;
        unsigned char reserved : 1;
        unsigned char buck4_dt_sel : 2;
        unsigned char buck4_ocp_sel : 2;
    } reg;
} PMIC_BUCK4_CTRL1_UNION;
#endif
#define PMIC_BUCK4_CTRL1_buck4_ng_dt_sel_START (0)
#define PMIC_BUCK4_CTRL1_buck4_ng_dt_sel_END (0)
#define PMIC_BUCK4_CTRL1_buck4_pg_dt_sel_START (1)
#define PMIC_BUCK4_CTRL1_buck4_pg_dt_sel_END (1)
#define PMIC_BUCK4_CTRL1_buck4_sft_sel_START (2)
#define PMIC_BUCK4_CTRL1_buck4_sft_sel_END (2)
#define PMIC_BUCK4_CTRL1_buck4_dt_sel_START (4)
#define PMIC_BUCK4_CTRL1_buck4_dt_sel_END (5)
#define PMIC_BUCK4_CTRL1_buck4_ocp_sel_START (6)
#define PMIC_BUCK4_CTRL1_buck4_ocp_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_ng_n_sel : 2;
        unsigned char buck4_ng_p_sel : 2;
        unsigned char buck4_pg_n_sel : 2;
        unsigned char buck4_pg_p_sel : 2;
    } reg;
} PMIC_BUCK4_CTRL2_UNION;
#endif
#define PMIC_BUCK4_CTRL2_buck4_ng_n_sel_START (0)
#define PMIC_BUCK4_CTRL2_buck4_ng_n_sel_END (1)
#define PMIC_BUCK4_CTRL2_buck4_ng_p_sel_START (2)
#define PMIC_BUCK4_CTRL2_buck4_ng_p_sel_END (3)
#define PMIC_BUCK4_CTRL2_buck4_pg_n_sel_START (4)
#define PMIC_BUCK4_CTRL2_buck4_pg_n_sel_END (5)
#define PMIC_BUCK4_CTRL2_buck4_pg_p_sel_START (6)
#define PMIC_BUCK4_CTRL2_buck4_pg_p_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_reg_r : 2;
        unsigned char reserved : 2;
        unsigned char buck4_reg_en : 1;
        unsigned char buck4_dbias : 2;
        unsigned char buck4_ocp_d : 1;
    } reg;
} PMIC_BUCK4_CTRL3_UNION;
#endif
#define PMIC_BUCK4_CTRL3_buck4_reg_r_START (0)
#define PMIC_BUCK4_CTRL3_buck4_reg_r_END (1)
#define PMIC_BUCK4_CTRL3_buck4_reg_en_START (4)
#define PMIC_BUCK4_CTRL3_buck4_reg_en_END (4)
#define PMIC_BUCK4_CTRL3_buck4_dbias_START (5)
#define PMIC_BUCK4_CTRL3_buck4_dbias_END (6)
#define PMIC_BUCK4_CTRL3_buck4_ocp_d_START (7)
#define PMIC_BUCK4_CTRL3_buck4_ocp_d_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_short_pdp : 1;
        unsigned char buck4_reg_ss_d : 1;
        unsigned char buck4_regop_c : 1;
        unsigned char buck4_reg_idr : 2;
        unsigned char buck4_reg_dr : 3;
    } reg;
} PMIC_BUCK4_CTRL4_UNION;
#endif
#define PMIC_BUCK4_CTRL4_buck4_short_pdp_START (0)
#define PMIC_BUCK4_CTRL4_buck4_short_pdp_END (0)
#define PMIC_BUCK4_CTRL4_buck4_reg_ss_d_START (1)
#define PMIC_BUCK4_CTRL4_buck4_reg_ss_d_END (1)
#define PMIC_BUCK4_CTRL4_buck4_regop_c_START (2)
#define PMIC_BUCK4_CTRL4_buck4_regop_c_END (2)
#define PMIC_BUCK4_CTRL4_buck4_reg_idr_START (3)
#define PMIC_BUCK4_CTRL4_buck4_reg_idr_END (4)
#define PMIC_BUCK4_CTRL4_buck4_reg_dr_START (5)
#define PMIC_BUCK4_CTRL4_buck4_reg_dr_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_ton : 2;
        unsigned char buck4_nmos_off : 1;
        unsigned char buck4_reg_c : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_BUCK4_CTRL5_UNION;
#endif
#define PMIC_BUCK4_CTRL5_buck4_ton_START (0)
#define PMIC_BUCK4_CTRL5_buck4_ton_END (1)
#define PMIC_BUCK4_CTRL5_buck4_nmos_off_START (2)
#define PMIC_BUCK4_CTRL5_buck4_nmos_off_END (2)
#define PMIC_BUCK4_CTRL5_buck4_reg_c_START (3)
#define PMIC_BUCK4_CTRL5_buck4_reg_c_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_shield_i : 2;
        unsigned char buck4_new_dmd_sel : 5;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK4_CTRL6_UNION;
#endif
#define PMIC_BUCK4_CTRL6_buck4_shield_i_START (0)
#define PMIC_BUCK4_CTRL6_buck4_shield_i_END (1)
#define PMIC_BUCK4_CTRL6_buck4_new_dmd_sel_START (2)
#define PMIC_BUCK4_CTRL6_buck4_new_dmd_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_dmd_sel : 3;
        unsigned char buck4_mos_sel : 2;
        unsigned char reserved : 3;
    } reg;
} PMIC_BUCK4_CTRL7_UNION;
#endif
#define PMIC_BUCK4_CTRL7_buck4_dmd_sel_START (0)
#define PMIC_BUCK4_CTRL7_buck4_dmd_sel_END (2)
#define PMIC_BUCK4_CTRL7_buck4_mos_sel_START (3)
#define PMIC_BUCK4_CTRL7_buck4_mos_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_reserve0 : 8;
    } reg;
} PMIC_BUCK4_CTRL8_UNION;
#endif
#define PMIC_BUCK4_CTRL8_buck4_reserve0_START (0)
#define PMIC_BUCK4_CTRL8_buck4_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_reserve1 : 8;
    } reg;
} PMIC_BUCK4_CTRL9_UNION;
#endif
#define PMIC_BUCK4_CTRL9_buck4_reserve1_START (0)
#define PMIC_BUCK4_CTRL9_buck4_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_reserve2 : 8;
    } reg;
} PMIC_BUCK4_CTRL10_UNION;
#endif
#define PMIC_BUCK4_CTRL10_buck4_reserve2_START (0)
#define PMIC_BUCK4_CTRL10_buck4_reserve2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_pdn_lx_det : 1;
        unsigned char buck4_eco_dmd : 1;
        unsigned char buck4_dmd_shield_n : 1;
        unsigned char buck4_ocp_delay_sel : 1;
        unsigned char buck4_dmd_clamp : 1;
        unsigned char buck4_en_regop_clamp : 1;
        unsigned char buck4_en_ss_sel : 1;
        unsigned char reserved : 1;
    } reg;
} PMIC_BUCK4_CTRL11_UNION;
#endif
#define PMIC_BUCK4_CTRL11_buck4_pdn_lx_det_START (0)
#define PMIC_BUCK4_CTRL11_buck4_pdn_lx_det_END (0)
#define PMIC_BUCK4_CTRL11_buck4_eco_dmd_START (1)
#define PMIC_BUCK4_CTRL11_buck4_eco_dmd_END (1)
#define PMIC_BUCK4_CTRL11_buck4_dmd_shield_n_START (2)
#define PMIC_BUCK4_CTRL11_buck4_dmd_shield_n_END (2)
#define PMIC_BUCK4_CTRL11_buck4_ocp_delay_sel_START (3)
#define PMIC_BUCK4_CTRL11_buck4_ocp_delay_sel_END (3)
#define PMIC_BUCK4_CTRL11_buck4_dmd_clamp_START (4)
#define PMIC_BUCK4_CTRL11_buck4_dmd_clamp_END (4)
#define PMIC_BUCK4_CTRL11_buck4_en_regop_clamp_START (5)
#define PMIC_BUCK4_CTRL11_buck4_en_regop_clamp_END (5)
#define PMIC_BUCK4_CTRL11_buck4_en_ss_sel_START (6)
#define PMIC_BUCK4_CTRL11_buck4_en_ss_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo1_vrset : 3;
        unsigned char ldo1_ocp_enn : 1;
        unsigned char ldo0_2_vrset : 3;
        unsigned char ldo0_2_ocp_enn : 1;
    } reg;
} PMIC_LDO_1_CTRL_UNION;
#endif
#define PMIC_LDO_1_CTRL_ldo1_vrset_START (0)
#define PMIC_LDO_1_CTRL_ldo1_vrset_END (2)
#define PMIC_LDO_1_CTRL_ldo1_ocp_enn_START (3)
#define PMIC_LDO_1_CTRL_ldo1_ocp_enn_END (3)
#define PMIC_LDO_1_CTRL_ldo0_2_vrset_START (4)
#define PMIC_LDO_1_CTRL_ldo0_2_vrset_END (6)
#define PMIC_LDO_1_CTRL_ldo0_2_ocp_enn_START (7)
#define PMIC_LDO_1_CTRL_ldo0_2_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo3_vrset : 3;
        unsigned char ldo3_ocp_enn : 1;
        unsigned char ldo2_vrset : 3;
        unsigned char ldo2_ocp_enn : 1;
    } reg;
} PMIC_LD2_3_CTRL_UNION;
#endif
#define PMIC_LD2_3_CTRL_ldo3_vrset_START (0)
#define PMIC_LD2_3_CTRL_ldo3_vrset_END (2)
#define PMIC_LD2_3_CTRL_ldo3_ocp_enn_START (3)
#define PMIC_LD2_3_CTRL_ldo3_ocp_enn_END (3)
#define PMIC_LD2_3_CTRL_ldo2_vrset_START (4)
#define PMIC_LD2_3_CTRL_ldo2_vrset_END (6)
#define PMIC_LD2_3_CTRL_ldo2_ocp_enn_START (7)
#define PMIC_LD2_3_CTRL_ldo2_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo5_vrset : 3;
        unsigned char ldo5_ocp_enn : 1;
        unsigned char ldo4_vrset : 3;
        unsigned char ldo4_ocp_enn : 1;
    } reg;
} PMIC_LDO4_5_CTRL_UNION;
#endif
#define PMIC_LDO4_5_CTRL_ldo5_vrset_START (0)
#define PMIC_LDO4_5_CTRL_ldo5_vrset_END (2)
#define PMIC_LDO4_5_CTRL_ldo5_ocp_enn_START (3)
#define PMIC_LDO4_5_CTRL_ldo5_ocp_enn_END (3)
#define PMIC_LDO4_5_CTRL_ldo4_vrset_START (4)
#define PMIC_LDO4_5_CTRL_ldo4_vrset_END (6)
#define PMIC_LDO4_5_CTRL_ldo4_ocp_enn_START (7)
#define PMIC_LDO4_5_CTRL_ldo4_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo8_vrset : 3;
        unsigned char ldo8_ocp_enn : 1;
        unsigned char ldo7_vrset : 3;
        unsigned char ldo7_ocp_enn : 1;
    } reg;
} PMIC_LDO7_8_CTRL_UNION;
#endif
#define PMIC_LDO7_8_CTRL_ldo8_vrset_START (0)
#define PMIC_LDO7_8_CTRL_ldo8_vrset_END (2)
#define PMIC_LDO7_8_CTRL_ldo8_ocp_enn_START (3)
#define PMIC_LDO7_8_CTRL_ldo8_ocp_enn_END (3)
#define PMIC_LDO7_8_CTRL_ldo7_vrset_START (4)
#define PMIC_LDO7_8_CTRL_ldo7_vrset_END (6)
#define PMIC_LDO7_8_CTRL_ldo7_ocp_enn_START (7)
#define PMIC_LDO7_8_CTRL_ldo7_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo10_vrset : 3;
        unsigned char ldo10_ocp_enn : 1;
        unsigned char ldo9_vrset : 3;
        unsigned char ldo9_ocp_enn : 1;
    } reg;
} PMIC_LDO9_10_CTRL_UNION;
#endif
#define PMIC_LDO9_10_CTRL_ldo10_vrset_START (0)
#define PMIC_LDO9_10_CTRL_ldo10_vrset_END (2)
#define PMIC_LDO9_10_CTRL_ldo10_ocp_enn_START (3)
#define PMIC_LDO9_10_CTRL_ldo10_ocp_enn_END (3)
#define PMIC_LDO9_10_CTRL_ldo9_vrset_START (4)
#define PMIC_LDO9_10_CTRL_ldo9_vrset_END (6)
#define PMIC_LDO9_10_CTRL_ldo9_ocp_enn_START (7)
#define PMIC_LDO9_10_CTRL_ldo9_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo12_vrset : 3;
        unsigned char ldo12_ocp_enn : 1;
        unsigned char ldo11_vrset : 3;
        unsigned char ldo11_ocp_enn : 1;
    } reg;
} PMIC_LD11_12_CTRL_UNION;
#endif
#define PMIC_LD11_12_CTRL_ldo12_vrset_START (0)
#define PMIC_LD11_12_CTRL_ldo12_vrset_END (2)
#define PMIC_LD11_12_CTRL_ldo12_ocp_enn_START (3)
#define PMIC_LD11_12_CTRL_ldo12_ocp_enn_END (3)
#define PMIC_LD11_12_CTRL_ldo11_vrset_START (4)
#define PMIC_LD11_12_CTRL_ldo11_vrset_END (6)
#define PMIC_LD11_12_CTRL_ldo11_ocp_enn_START (7)
#define PMIC_LD11_12_CTRL_ldo11_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo14_vrset : 3;
        unsigned char ldo14_ocp_enn : 1;
        unsigned char ldo13_vrset : 3;
        unsigned char ldo13_ocp_enn : 1;
    } reg;
} PMIC_LDO13_14_CTRL_UNION;
#endif
#define PMIC_LDO13_14_CTRL_ldo14_vrset_START (0)
#define PMIC_LDO13_14_CTRL_ldo14_vrset_END (2)
#define PMIC_LDO13_14_CTRL_ldo14_ocp_enn_START (3)
#define PMIC_LDO13_14_CTRL_ldo14_ocp_enn_END (3)
#define PMIC_LDO13_14_CTRL_ldo13_vrset_START (4)
#define PMIC_LDO13_14_CTRL_ldo13_vrset_END (6)
#define PMIC_LDO13_14_CTRL_ldo13_ocp_enn_START (7)
#define PMIC_LDO13_14_CTRL_ldo13_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo16_vrset : 3;
        unsigned char ldo16_ocp_enn : 1;
        unsigned char ldo15_vrset : 3;
        unsigned char ldo15_ocp_enn : 1;
    } reg;
} PMIC_LDO15_16_CTRL_UNION;
#endif
#define PMIC_LDO15_16_CTRL_ldo16_vrset_START (0)
#define PMIC_LDO15_16_CTRL_ldo16_vrset_END (2)
#define PMIC_LDO15_16_CTRL_ldo16_ocp_enn_START (3)
#define PMIC_LDO15_16_CTRL_ldo16_ocp_enn_END (3)
#define PMIC_LDO15_16_CTRL_ldo15_vrset_START (4)
#define PMIC_LDO15_16_CTRL_ldo15_vrset_END (6)
#define PMIC_LDO15_16_CTRL_ldo15_ocp_enn_START (7)
#define PMIC_LDO15_16_CTRL_ldo15_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo19_vrset : 3;
        unsigned char ldo19_ocp_enn : 1;
        unsigned char ldo17_vrset : 3;
        unsigned char ldo17_ocp_enn : 1;
    } reg;
} PMIC_LDO17_19_CTRL_UNION;
#endif
#define PMIC_LDO17_19_CTRL_ldo19_vrset_START (0)
#define PMIC_LDO17_19_CTRL_ldo19_vrset_END (2)
#define PMIC_LDO17_19_CTRL_ldo19_ocp_enn_START (3)
#define PMIC_LDO17_19_CTRL_ldo19_ocp_enn_END (3)
#define PMIC_LDO17_19_CTRL_ldo17_vrset_START (4)
#define PMIC_LDO17_19_CTRL_ldo17_vrset_END (6)
#define PMIC_LDO17_19_CTRL_ldo17_ocp_enn_START (7)
#define PMIC_LDO17_19_CTRL_ldo17_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo21_vrset : 3;
        unsigned char ldo21_ocp_enn : 1;
        unsigned char ldo20_vrset : 3;
        unsigned char ldo20_ocp_enn : 1;
    } reg;
} PMIC_LDO20_21_CTRL_UNION;
#endif
#define PMIC_LDO20_21_CTRL_ldo21_vrset_START (0)
#define PMIC_LDO20_21_CTRL_ldo21_vrset_END (2)
#define PMIC_LDO20_21_CTRL_ldo21_ocp_enn_START (3)
#define PMIC_LDO20_21_CTRL_ldo21_ocp_enn_END (3)
#define PMIC_LDO20_21_CTRL_ldo20_vrset_START (4)
#define PMIC_LDO20_21_CTRL_ldo20_vrset_END (6)
#define PMIC_LDO20_21_CTRL_ldo20_ocp_enn_START (7)
#define PMIC_LDO20_21_CTRL_ldo20_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo23_vrset : 3;
        unsigned char ldo23_ocp_enn : 1;
        unsigned char ldo22_vrset : 3;
        unsigned char ldo22_ocp_enn : 1;
    } reg;
} PMIC_LDO22_23_CTRL_UNION;
#endif
#define PMIC_LDO22_23_CTRL_ldo23_vrset_START (0)
#define PMIC_LDO22_23_CTRL_ldo23_vrset_END (2)
#define PMIC_LDO22_23_CTRL_ldo23_ocp_enn_START (3)
#define PMIC_LDO22_23_CTRL_ldo23_ocp_enn_END (3)
#define PMIC_LDO22_23_CTRL_ldo22_vrset_START (4)
#define PMIC_LDO22_23_CTRL_ldo22_vrset_END (6)
#define PMIC_LDO22_23_CTRL_ldo22_ocp_enn_START (7)
#define PMIC_LDO22_23_CTRL_ldo22_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo25_vrset : 3;
        unsigned char ldo25_ocp_enn : 1;
        unsigned char ldo24_vrset : 3;
        unsigned char ldo24_ocp_enn : 1;
    } reg;
} PMIC_LDO24_25_CTRL_UNION;
#endif
#define PMIC_LDO24_25_CTRL_ldo25_vrset_START (0)
#define PMIC_LDO24_25_CTRL_ldo25_vrset_END (2)
#define PMIC_LDO24_25_CTRL_ldo25_ocp_enn_START (3)
#define PMIC_LDO24_25_CTRL_ldo25_ocp_enn_END (3)
#define PMIC_LDO24_25_CTRL_ldo24_vrset_START (4)
#define PMIC_LDO24_25_CTRL_ldo24_vrset_END (6)
#define PMIC_LDO24_25_CTRL_ldo24_ocp_enn_START (7)
#define PMIC_LDO24_25_CTRL_ldo24_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo27_vrset : 3;
        unsigned char ldo27_ocp_enn : 1;
        unsigned char ldo26_vrset : 3;
        unsigned char ldo26_ocp_enn : 1;
    } reg;
} PMIC_LDO26_27_CTRL_UNION;
#endif
#define PMIC_LDO26_27_CTRL_ldo27_vrset_START (0)
#define PMIC_LDO26_27_CTRL_ldo27_vrset_END (2)
#define PMIC_LDO26_27_CTRL_ldo27_ocp_enn_START (3)
#define PMIC_LDO26_27_CTRL_ldo27_ocp_enn_END (3)
#define PMIC_LDO26_27_CTRL_ldo26_vrset_START (4)
#define PMIC_LDO26_27_CTRL_ldo26_vrset_END (6)
#define PMIC_LDO26_27_CTRL_ldo26_ocp_enn_START (7)
#define PMIC_LDO26_27_CTRL_ldo26_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo29_vrset : 3;
        unsigned char ldo29_ocp_enn : 1;
        unsigned char ldo28_vrset : 3;
        unsigned char ldo28_ocp_enn : 1;
    } reg;
} PMIC_LDO28_29_CTRL_UNION;
#endif
#define PMIC_LDO28_29_CTRL_ldo29_vrset_START (0)
#define PMIC_LDO28_29_CTRL_ldo29_vrset_END (2)
#define PMIC_LDO28_29_CTRL_ldo29_ocp_enn_START (3)
#define PMIC_LDO28_29_CTRL_ldo29_ocp_enn_END (3)
#define PMIC_LDO28_29_CTRL_ldo28_vrset_START (4)
#define PMIC_LDO28_29_CTRL_ldo28_vrset_END (6)
#define PMIC_LDO28_29_CTRL_ldo28_ocp_enn_START (7)
#define PMIC_LDO28_29_CTRL_ldo28_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo31_vrset : 3;
        unsigned char ldo31_ocp_enn : 1;
        unsigned char ldo30_vrset : 3;
        unsigned char ldo30_ocp_enn : 1;
    } reg;
} PMIC_LDO30_31_CTRL_UNION;
#endif
#define PMIC_LDO30_31_CTRL_ldo31_vrset_START (0)
#define PMIC_LDO30_31_CTRL_ldo31_vrset_END (2)
#define PMIC_LDO30_31_CTRL_ldo31_ocp_enn_START (3)
#define PMIC_LDO30_31_CTRL_ldo31_ocp_enn_END (3)
#define PMIC_LDO30_31_CTRL_ldo30_vrset_START (4)
#define PMIC_LDO30_31_CTRL_ldo30_vrset_END (6)
#define PMIC_LDO30_31_CTRL_ldo30_ocp_enn_START (7)
#define PMIC_LDO30_31_CTRL_ldo30_ocp_enn_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo32_vrset : 3;
        unsigned char ldo32_ocp_enn : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO32_CTRL_UNION;
#endif
#define PMIC_LDO32_CTRL_ldo32_vrset_START (0)
#define PMIC_LDO32_CTRL_ldo32_vrset_END (2)
#define PMIC_LDO32_CTRL_ldo32_ocp_enn_START (3)
#define PMIC_LDO32_CTRL_ldo32_ocp_enn_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reserved_0 : 4;
        unsigned char reg_ldo0_1_en : 1;
        unsigned char reserved_1 : 3;
    } reg;
} PMIC_LDO0_1_ONOFF_UNION;
#endif
#define PMIC_LDO0_1_ONOFF_reg_ldo0_1_en_START (4)
#define PMIC_LDO0_1_ONOFF_reg_ldo0_1_en_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo0_2_eco_en : 1;
        unsigned char reg_ldo0_2_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO0_2_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO0_2_ONOFF_ECO_reg_ldo0_2_eco_en_START (0)
#define PMIC_LDO0_2_ONOFF_ECO_reg_ldo0_2_eco_en_END (0)
#define PMIC_LDO0_2_ONOFF_ECO_reg_ldo0_2_en_START (1)
#define PMIC_LDO0_2_ONOFF_ECO_reg_ldo0_2_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo0_2_vset : 3;
        unsigned char reserved_0 : 1;
        unsigned char ldo0_2_vset_adj : 3;
        unsigned char reserved_1 : 1;
    } reg;
} PMIC_LDO0_2_VSET_UNION;
#endif
#define PMIC_LDO0_2_VSET_ldo0_2_vset_START (0)
#define PMIC_LDO0_2_VSET_ldo0_2_vset_END (2)
#define PMIC_LDO0_2_VSET_ldo0_2_vset_adj_START (4)
#define PMIC_LDO0_2_VSET_ldo0_2_vset_adj_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo1_eco_en : 1;
        unsigned char reg_ldo1_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO1_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO1_ONOFF_ECO_reg_ldo1_eco_en_START (0)
#define PMIC_LDO1_ONOFF_ECO_reg_ldo1_eco_en_END (0)
#define PMIC_LDO1_ONOFF_ECO_reg_ldo1_en_START (1)
#define PMIC_LDO1_ONOFF_ECO_reg_ldo1_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo1_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO1_VSET_UNION;
#endif
#define PMIC_LDO1_VSET_ldo1_vset_START (0)
#define PMIC_LDO1_VSET_ldo1_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo2_eco_en : 1;
        unsigned char reg_ldo2_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO2_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO2_ONOFF_ECO_reg_ldo2_eco_en_START (0)
#define PMIC_LDO2_ONOFF_ECO_reg_ldo2_eco_en_END (0)
#define PMIC_LDO2_ONOFF_ECO_reg_ldo2_en_START (1)
#define PMIC_LDO2_ONOFF_ECO_reg_ldo2_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo2_vset : 3;
        unsigned char reserved_0 : 1;
        unsigned char ldo2_vset_adj : 3;
        unsigned char reserved_1 : 1;
    } reg;
} PMIC_LDO2_VSET_UNION;
#endif
#define PMIC_LDO2_VSET_ldo2_vset_START (0)
#define PMIC_LDO2_VSET_ldo2_vset_END (2)
#define PMIC_LDO2_VSET_ldo2_vset_adj_START (4)
#define PMIC_LDO2_VSET_ldo2_vset_adj_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo3_eco_en : 1;
        unsigned char reg_ldo3_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO3_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO3_ONOFF_ECO_reg_ldo3_eco_en_START (0)
#define PMIC_LDO3_ONOFF_ECO_reg_ldo3_eco_en_END (0)
#define PMIC_LDO3_ONOFF_ECO_reg_ldo3_en_START (1)
#define PMIC_LDO3_ONOFF_ECO_reg_ldo3_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo3_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO3_VSET_UNION;
#endif
#define PMIC_LDO3_VSET_ldo3_vset_START (0)
#define PMIC_LDO3_VSET_ldo3_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo4_eco_en : 1;
        unsigned char reg_ldo4_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO4_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO4_ONOFF_ECO_reg_ldo4_eco_en_START (0)
#define PMIC_LDO4_ONOFF_ECO_reg_ldo4_eco_en_END (0)
#define PMIC_LDO4_ONOFF_ECO_reg_ldo4_en_START (1)
#define PMIC_LDO4_ONOFF_ECO_reg_ldo4_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo4_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO4_VSET_UNION;
#endif
#define PMIC_LDO4_VSET_ldo4_vset_START (0)
#define PMIC_LDO4_VSET_ldo4_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo5_eco_en : 1;
        unsigned char reg_ldo5_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO5_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO5_ONOFF_ECO_reg_ldo5_eco_en_START (0)
#define PMIC_LDO5_ONOFF_ECO_reg_ldo5_eco_en_END (0)
#define PMIC_LDO5_ONOFF_ECO_reg_ldo5_en_START (1)
#define PMIC_LDO5_ONOFF_ECO_reg_ldo5_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo5_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO5_VSET_UNION;
#endif
#define PMIC_LDO5_VSET_ldo5_vset_START (0)
#define PMIC_LDO5_VSET_ldo5_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo7_eco_en : 1;
        unsigned char reg_ldo7_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO7_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO7_ONOFF_ECO_reg_ldo7_eco_en_START (0)
#define PMIC_LDO7_ONOFF_ECO_reg_ldo7_eco_en_END (0)
#define PMIC_LDO7_ONOFF_ECO_reg_ldo7_en_START (1)
#define PMIC_LDO7_ONOFF_ECO_reg_ldo7_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo7_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO7_VSET_UNION;
#endif
#define PMIC_LDO7_VSET_ldo7_vset_START (0)
#define PMIC_LDO7_VSET_ldo7_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo8_eco_en : 1;
        unsigned char reg_ldo8_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO8_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO8_ONOFF_ECO_reg_ldo8_eco_en_START (0)
#define PMIC_LDO8_ONOFF_ECO_reg_ldo8_eco_en_END (0)
#define PMIC_LDO8_ONOFF_ECO_reg_ldo8_en_START (1)
#define PMIC_LDO8_ONOFF_ECO_reg_ldo8_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo8_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO8_VSET_UNION;
#endif
#define PMIC_LDO8_VSET_ldo8_vset_START (0)
#define PMIC_LDO8_VSET_ldo8_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo9_eco_en : 1;
        unsigned char reg_ldo9_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO9_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO9_ONOFF_ECO_reg_ldo9_eco_en_START (0)
#define PMIC_LDO9_ONOFF_ECO_reg_ldo9_eco_en_END (0)
#define PMIC_LDO9_ONOFF_ECO_reg_ldo9_en_START (1)
#define PMIC_LDO9_ONOFF_ECO_reg_ldo9_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo9_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO9_VSET_UNION;
#endif
#define PMIC_LDO9_VSET_ldo9_vset_START (0)
#define PMIC_LDO9_VSET_ldo9_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo10_eco_en : 1;
        unsigned char reg_ldo10_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO10_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO10_ONOFF_ECO_reg_ldo10_eco_en_START (0)
#define PMIC_LDO10_ONOFF_ECO_reg_ldo10_eco_en_END (0)
#define PMIC_LDO10_ONOFF_ECO_reg_ldo10_en_START (1)
#define PMIC_LDO10_ONOFF_ECO_reg_ldo10_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo10_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO10_VSET_UNION;
#endif
#define PMIC_LDO10_VSET_ldo10_vset_START (0)
#define PMIC_LDO10_VSET_ldo10_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo11_eco_en : 1;
        unsigned char reg_ldo11_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO11_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO11_ONOFF_ECO_reg_ldo11_eco_en_START (0)
#define PMIC_LDO11_ONOFF_ECO_reg_ldo11_eco_en_END (0)
#define PMIC_LDO11_ONOFF_ECO_reg_ldo11_en_START (1)
#define PMIC_LDO11_ONOFF_ECO_reg_ldo11_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo11_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO11_VSET_UNION;
#endif
#define PMIC_LDO11_VSET_ldo11_vset_START (0)
#define PMIC_LDO11_VSET_ldo11_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo12_eco_en : 1;
        unsigned char reg_ldo12_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO12_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO12_ONOFF_ECO_reg_ldo12_eco_en_START (0)
#define PMIC_LDO12_ONOFF_ECO_reg_ldo12_eco_en_END (0)
#define PMIC_LDO12_ONOFF_ECO_reg_ldo12_en_START (1)
#define PMIC_LDO12_ONOFF_ECO_reg_ldo12_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo12_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO12_VSET_UNION;
#endif
#define PMIC_LDO12_VSET_ldo12_vset_START (0)
#define PMIC_LDO12_VSET_ldo12_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo13_eco_en : 1;
        unsigned char reg_ldo13_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO13_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO13_ONOFF_ECO_reg_ldo13_eco_en_START (0)
#define PMIC_LDO13_ONOFF_ECO_reg_ldo13_eco_en_END (0)
#define PMIC_LDO13_ONOFF_ECO_reg_ldo13_en_START (1)
#define PMIC_LDO13_ONOFF_ECO_reg_ldo13_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo13_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO13_VSET_UNION;
#endif
#define PMIC_LDO13_VSET_ldo13_vset_START (0)
#define PMIC_LDO13_VSET_ldo13_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo14_eco_en : 1;
        unsigned char reg_ldo14_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO14_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO14_ONOFF_ECO_reg_ldo14_eco_en_START (0)
#define PMIC_LDO14_ONOFF_ECO_reg_ldo14_eco_en_END (0)
#define PMIC_LDO14_ONOFF_ECO_reg_ldo14_en_START (1)
#define PMIC_LDO14_ONOFF_ECO_reg_ldo14_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo14_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO14_VSET_UNION;
#endif
#define PMIC_LDO14_VSET_ldo14_vset_START (0)
#define PMIC_LDO14_VSET_ldo14_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo15_eco_en : 1;
        unsigned char reg_ldo15_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO15_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO15_ONOFF_ECO_reg_ldo15_eco_en_START (0)
#define PMIC_LDO15_ONOFF_ECO_reg_ldo15_eco_en_END (0)
#define PMIC_LDO15_ONOFF_ECO_reg_ldo15_en_START (1)
#define PMIC_LDO15_ONOFF_ECO_reg_ldo15_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo15_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO15_VSET_UNION;
#endif
#define PMIC_LDO15_VSET_ldo15_vset_START (0)
#define PMIC_LDO15_VSET_ldo15_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo16_eco_en : 1;
        unsigned char reg_ldo16_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO16_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO16_ONOFF_ECO_reg_ldo16_eco_en_START (0)
#define PMIC_LDO16_ONOFF_ECO_reg_ldo16_eco_en_END (0)
#define PMIC_LDO16_ONOFF_ECO_reg_ldo16_en_START (1)
#define PMIC_LDO16_ONOFF_ECO_reg_ldo16_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo16_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO16_VSET_UNION;
#endif
#define PMIC_LDO16_VSET_ldo16_vset_START (0)
#define PMIC_LDO16_VSET_ldo16_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo17_eco_en : 1;
        unsigned char reg_ldo17_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO17_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO17_ONOFF_ECO_reg_ldo17_eco_en_START (0)
#define PMIC_LDO17_ONOFF_ECO_reg_ldo17_eco_en_END (0)
#define PMIC_LDO17_ONOFF_ECO_reg_ldo17_en_START (1)
#define PMIC_LDO17_ONOFF_ECO_reg_ldo17_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo17_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO17_VSET_UNION;
#endif
#define PMIC_LDO17_VSET_ldo17_vset_START (0)
#define PMIC_LDO17_VSET_ldo17_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo19_eco_en : 1;
        unsigned char reg_ldo19_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO19_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO19_ONOFF_ECO_reg_ldo19_eco_en_START (0)
#define PMIC_LDO19_ONOFF_ECO_reg_ldo19_eco_en_END (0)
#define PMIC_LDO19_ONOFF_ECO_reg_ldo19_en_START (1)
#define PMIC_LDO19_ONOFF_ECO_reg_ldo19_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo19_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO19_VSET1_UNION;
#endif
#define PMIC_LDO19_VSET1_ldo19_vset_START (0)
#define PMIC_LDO19_VSET1_ldo19_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo20_eco_en : 1;
        unsigned char reg_ldo20_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO20_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO20_ONOFF_ECO_reg_ldo20_eco_en_START (0)
#define PMIC_LDO20_ONOFF_ECO_reg_ldo20_eco_en_END (0)
#define PMIC_LDO20_ONOFF_ECO_reg_ldo20_en_START (1)
#define PMIC_LDO20_ONOFF_ECO_reg_ldo20_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo20_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO20_VSET_UNION;
#endif
#define PMIC_LDO20_VSET_ldo20_vset_START (0)
#define PMIC_LDO20_VSET_ldo20_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo21_eco_en : 1;
        unsigned char reg_ldo21_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO21_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO21_ONOFF_ECO_reg_ldo21_eco_en_START (0)
#define PMIC_LDO21_ONOFF_ECO_reg_ldo21_eco_en_END (0)
#define PMIC_LDO21_ONOFF_ECO_reg_ldo21_en_START (1)
#define PMIC_LDO21_ONOFF_ECO_reg_ldo21_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo21_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO21_VSET_UNION;
#endif
#define PMIC_LDO21_VSET_ldo21_vset_START (0)
#define PMIC_LDO21_VSET_ldo21_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo22_eco_en : 1;
        unsigned char reg_ldo22_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO22_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO22_ONOFF_ECO_reg_ldo22_eco_en_START (0)
#define PMIC_LDO22_ONOFF_ECO_reg_ldo22_eco_en_END (0)
#define PMIC_LDO22_ONOFF_ECO_reg_ldo22_en_START (1)
#define PMIC_LDO22_ONOFF_ECO_reg_ldo22_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo22_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO22_VSET_UNION;
#endif
#define PMIC_LDO22_VSET_ldo22_vset_START (0)
#define PMIC_LDO22_VSET_ldo22_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo23_eco_en : 1;
        unsigned char reg_ldo23_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO23_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO23_ONOFF_ECO_reg_ldo23_eco_en_START (0)
#define PMIC_LDO23_ONOFF_ECO_reg_ldo23_eco_en_END (0)
#define PMIC_LDO23_ONOFF_ECO_reg_ldo23_en_START (1)
#define PMIC_LDO23_ONOFF_ECO_reg_ldo23_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo23_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO23_VSET_UNION;
#endif
#define PMIC_LDO23_VSET_ldo23_vset_START (0)
#define PMIC_LDO23_VSET_ldo23_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo24_eco_en : 1;
        unsigned char reg_ldo24_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO24_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO24_ONOFF_ECO_reg_ldo24_eco_en_START (0)
#define PMIC_LDO24_ONOFF_ECO_reg_ldo24_eco_en_END (0)
#define PMIC_LDO24_ONOFF_ECO_reg_ldo24_en_START (1)
#define PMIC_LDO24_ONOFF_ECO_reg_ldo24_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo24_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO24_VSET_UNION;
#endif
#define PMIC_LDO24_VSET_ldo24_vset_START (0)
#define PMIC_LDO24_VSET_ldo24_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo25_eco_en : 1;
        unsigned char reg_ldo25_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO25_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO25_ONOFF_ECO_reg_ldo25_eco_en_START (0)
#define PMIC_LDO25_ONOFF_ECO_reg_ldo25_eco_en_END (0)
#define PMIC_LDO25_ONOFF_ECO_reg_ldo25_en_START (1)
#define PMIC_LDO25_ONOFF_ECO_reg_ldo25_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo25_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO25_VSET_UNION;
#endif
#define PMIC_LDO25_VSET_ldo25_vset_START (0)
#define PMIC_LDO25_VSET_ldo25_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo26_eco_en : 1;
        unsigned char reg_ldo26_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO26_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO26_ONOFF_ECO_reg_ldo26_eco_en_START (0)
#define PMIC_LDO26_ONOFF_ECO_reg_ldo26_eco_en_END (0)
#define PMIC_LDO26_ONOFF_ECO_reg_ldo26_en_START (1)
#define PMIC_LDO26_ONOFF_ECO_reg_ldo26_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo26_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO26_VSET_UNION;
#endif
#define PMIC_LDO26_VSET_ldo26_vset_START (0)
#define PMIC_LDO26_VSET_ldo26_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo27_eco_en : 1;
        unsigned char reg_ldo27_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO27_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO27_ONOFF_ECO_reg_ldo27_eco_en_START (0)
#define PMIC_LDO27_ONOFF_ECO_reg_ldo27_eco_en_END (0)
#define PMIC_LDO27_ONOFF_ECO_reg_ldo27_en_START (1)
#define PMIC_LDO27_ONOFF_ECO_reg_ldo27_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo27_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO27_VSET_UNION;
#endif
#define PMIC_LDO27_VSET_ldo27_vset_START (0)
#define PMIC_LDO27_VSET_ldo27_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo28_eco_en : 1;
        unsigned char reg_ldo28_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO28_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO28_ONOFF_ECO_reg_ldo28_eco_en_START (0)
#define PMIC_LDO28_ONOFF_ECO_reg_ldo28_eco_en_END (0)
#define PMIC_LDO28_ONOFF_ECO_reg_ldo28_en_START (1)
#define PMIC_LDO28_ONOFF_ECO_reg_ldo28_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo28_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO28_VSET_UNION;
#endif
#define PMIC_LDO28_VSET_ldo28_vset_START (0)
#define PMIC_LDO28_VSET_ldo28_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo29_eco_en : 1;
        unsigned char reg_ldo29_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO29_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO29_ONOFF_ECO_reg_ldo29_eco_en_START (0)
#define PMIC_LDO29_ONOFF_ECO_reg_ldo29_eco_en_END (0)
#define PMIC_LDO29_ONOFF_ECO_reg_ldo29_en_START (1)
#define PMIC_LDO29_ONOFF_ECO_reg_ldo29_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo29_vset : 3;
        unsigned char reserved_0 : 1;
        unsigned char ldo29_vset_adj : 3;
        unsigned char reserved_1 : 1;
    } reg;
} PMIC_LDO29_VSET_UNION;
#endif
#define PMIC_LDO29_VSET_ldo29_vset_START (0)
#define PMIC_LDO29_VSET_ldo29_vset_END (2)
#define PMIC_LDO29_VSET_ldo29_vset_adj_START (4)
#define PMIC_LDO29_VSET_ldo29_vset_adj_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo30_eco_en : 1;
        unsigned char reg_ldo30_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO30_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO30_ONOFF_ECO_reg_ldo30_eco_en_START (0)
#define PMIC_LDO30_ONOFF_ECO_reg_ldo30_eco_en_END (0)
#define PMIC_LDO30_ONOFF_ECO_reg_ldo30_en_START (1)
#define PMIC_LDO30_ONOFF_ECO_reg_ldo30_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo30_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO30_VSET_UNION;
#endif
#define PMIC_LDO30_VSET_ldo30_vset_START (0)
#define PMIC_LDO30_VSET_ldo30_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo31_eco_en : 1;
        unsigned char reg_ldo31_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO31_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO31_ONOFF_ECO_reg_ldo31_eco_en_START (0)
#define PMIC_LDO31_ONOFF_ECO_reg_ldo31_eco_en_END (0)
#define PMIC_LDO31_ONOFF_ECO_reg_ldo31_en_START (1)
#define PMIC_LDO31_ONOFF_ECO_reg_ldo31_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo31_vset : 3;
        unsigned char ldo_reserve : 5;
    } reg;
} PMIC_LDO31_VSET_UNION;
#endif
#define PMIC_LDO31_VSET_ldo31_vset_START (0)
#define PMIC_LDO31_VSET_ldo31_vset_END (2)
#define PMIC_LDO31_VSET_ldo_reserve_START (3)
#define PMIC_LDO31_VSET_ldo_reserve_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo32_eco_en : 1;
        unsigned char reg_ldo32_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_LDO32_ONOFF_ECO_UNION;
#endif
#define PMIC_LDO32_ONOFF_ECO_reg_ldo32_eco_en_START (0)
#define PMIC_LDO32_ONOFF_ECO_reg_ldo32_eco_en_END (0)
#define PMIC_LDO32_ONOFF_ECO_reg_ldo32_en_START (1)
#define PMIC_LDO32_ONOFF_ECO_reg_ldo32_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo32_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_LDO32_VSET_UNION;
#endif
#define PMIC_LDO32_VSET_ldo32_vset_START (0)
#define PMIC_LDO32_VSET_ldo32_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_buck0_eco_en : 1;
        unsigned char reg_buck0_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_BUCK0_ONOFF_ECO_UNION;
#endif
#define PMIC_BUCK0_ONOFF_ECO_reg_buck0_eco_en_START (0)
#define PMIC_BUCK0_ONOFF_ECO_reg_buck0_eco_en_END (0)
#define PMIC_BUCK0_ONOFF_ECO_reg_buck0_en_START (1)
#define PMIC_BUCK0_ONOFF_ECO_reg_buck0_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_BUCK0_VSET_UNION;
#endif
#define PMIC_BUCK0_VSET_buck0_vset_START (0)
#define PMIC_BUCK0_VSET_buck0_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_buck1_eco_en : 1;
        unsigned char reg_buck1_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_BUCK1_ONOFF_ECO_UNION;
#endif
#define PMIC_BUCK1_ONOFF_ECO_reg_buck1_eco_en_START (0)
#define PMIC_BUCK1_ONOFF_ECO_reg_buck1_eco_en_END (0)
#define PMIC_BUCK1_ONOFF_ECO_reg_buck1_en_START (1)
#define PMIC_BUCK1_ONOFF_ECO_reg_buck1_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck1_vset : 4;
        unsigned char buck1_vset_adj : 4;
    } reg;
} PMIC_BUCK1_VSET_UNION;
#endif
#define PMIC_BUCK1_VSET_buck1_vset_START (0)
#define PMIC_BUCK1_VSET_buck1_vset_END (3)
#define PMIC_BUCK1_VSET_buck1_vset_adj_START (4)
#define PMIC_BUCK1_VSET_buck1_vset_adj_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_buck2_eco_en : 1;
        unsigned char reg_buck2_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_BUCK2_ONOFF_ECO_UNION;
#endif
#define PMIC_BUCK2_ONOFF_ECO_reg_buck2_eco_en_START (0)
#define PMIC_BUCK2_ONOFF_ECO_reg_buck2_eco_en_END (0)
#define PMIC_BUCK2_ONOFF_ECO_reg_buck2_en_START (1)
#define PMIC_BUCK2_ONOFF_ECO_reg_buck2_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck2_vset : 4;
        unsigned char buck2_vset_adj : 4;
    } reg;
} PMIC_BUCK2_VSET_UNION;
#endif
#define PMIC_BUCK2_VSET_buck2_vset_START (0)
#define PMIC_BUCK2_VSET_buck2_vset_END (3)
#define PMIC_BUCK2_VSET_buck2_vset_adj_START (4)
#define PMIC_BUCK2_VSET_buck2_vset_adj_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_buck3_eco_en : 1;
        unsigned char reg_buck3_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_BUCK3_ONOFF_ECO_UNION;
#endif
#define PMIC_BUCK3_ONOFF_ECO_reg_buck3_eco_en_START (0)
#define PMIC_BUCK3_ONOFF_ECO_reg_buck3_eco_en_END (0)
#define PMIC_BUCK3_ONOFF_ECO_reg_buck3_en_START (1)
#define PMIC_BUCK3_ONOFF_ECO_reg_buck3_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_vset : 4;
        unsigned char buck3_vset_adj : 4;
    } reg;
} PMIC_BUCK3_VSET_UNION;
#endif
#define PMIC_BUCK3_VSET_buck3_vset_START (0)
#define PMIC_BUCK3_VSET_buck3_vset_END (3)
#define PMIC_BUCK3_VSET_buck3_vset_adj_START (4)
#define PMIC_BUCK3_VSET_buck3_vset_adj_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_buck4_eco_en : 1;
        unsigned char reg_buck4_en : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_BUCK4_ONOFF_ECO_UNION;
#endif
#define PMIC_BUCK4_ONOFF_ECO_reg_buck4_eco_en_START (0)
#define PMIC_BUCK4_ONOFF_ECO_reg_buck4_eco_en_END (0)
#define PMIC_BUCK4_ONOFF_ECO_reg_buck4_en_START (1)
#define PMIC_BUCK4_ONOFF_ECO_reg_buck4_en_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_vset : 4;
        unsigned char reserved : 4;
    } reg;
} PMIC_BUCK4_VSET_UNION;
#endif
#define PMIC_BUCK4_VSET_buck4_vset_START (0)
#define PMIC_BUCK4_VSET_buck4_vset_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char pmua_eco_en : 1;
        unsigned char reserved : 7;
    } reg;
} PMIC_LDO_PMUA_ECO_UNION;
#endif
#define PMIC_LDO_PMUA_ECO_pmua_eco_en_START (0)
#define PMIC_LDO_PMUA_ECO_pmua_eco_en_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char pmua_vset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_LDO_PMUA_VSET_UNION;
#endif
#define PMIC_LDO_PMUA_VSET_pmua_vset_START (0)
#define PMIC_LDO_PMUA_VSET_pmua_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_buck_boost_en_d : 1;
        unsigned char buck_boost_en_mode : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_BST_MODE_EN_UNION;
#endif
#define PMIC_BST_MODE_EN_reg_buck_boost_en_d_START (0)
#define PMIC_BST_MODE_EN_reg_buck_boost_en_d_END (0)
#define PMIC_BST_MODE_EN_buck_boost_en_mode_START (1)
#define PMIC_BST_MODE_EN_buck_boost_en_mode_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_chg_en : 1;
        unsigned char np_chg_vset : 2;
        unsigned char reserved : 5;
    } reg;
} PMIC_NOPWR_CTRL_UNION;
#endif
#define PMIC_NOPWR_CTRL_np_chg_en_START (0)
#define PMIC_NOPWR_CTRL_np_chg_en_END (0)
#define PMIC_NOPWR_CTRL_np_chg_vset_START (1)
#define PMIC_NOPWR_CTRL_np_chg_vset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_classd_en : 1;
        unsigned char classd_mute : 1;
        unsigned char classd_mute_sel : 1;
        unsigned char classd_drv_en : 1;
        unsigned char classd_i_ocp : 2;
        unsigned char classd_gain : 2;
    } reg;
} PMIC_CLASSD_CTRL0_UNION;
#endif
#define PMIC_CLASSD_CTRL0_reg_classd_en_START (0)
#define PMIC_CLASSD_CTRL0_reg_classd_en_END (0)
#define PMIC_CLASSD_CTRL0_classd_mute_START (1)
#define PMIC_CLASSD_CTRL0_classd_mute_END (1)
#define PMIC_CLASSD_CTRL0_classd_mute_sel_START (2)
#define PMIC_CLASSD_CTRL0_classd_mute_sel_END (2)
#define PMIC_CLASSD_CTRL0_classd_drv_en_START (3)
#define PMIC_CLASSD_CTRL0_classd_drv_en_END (3)
#define PMIC_CLASSD_CTRL0_classd_i_ocp_START (4)
#define PMIC_CLASSD_CTRL0_classd_i_ocp_END (5)
#define PMIC_CLASSD_CTRL0_classd_gain_START (6)
#define PMIC_CLASSD_CTRL0_classd_gain_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char classd_i_pump : 2;
        unsigned char classd_i_ramp : 2;
        unsigned char classd_p_sel : 2;
        unsigned char classd_n_sel : 2;
    } reg;
} PMIC_CLASSD_CTRL1_UNION;
#endif
#define PMIC_CLASSD_CTRL1_classd_i_pump_START (0)
#define PMIC_CLASSD_CTRL1_classd_i_pump_END (1)
#define PMIC_CLASSD_CTRL1_classd_i_ramp_START (2)
#define PMIC_CLASSD_CTRL1_classd_i_ramp_END (3)
#define PMIC_CLASSD_CTRL1_classd_p_sel_START (4)
#define PMIC_CLASSD_CTRL1_classd_p_sel_END (5)
#define PMIC_CLASSD_CTRL1_classd_n_sel_START (6)
#define PMIC_CLASSD_CTRL1_classd_n_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char classd_pls_byp : 1;
        unsigned char classd_dt_sel : 1;
        unsigned char classd_fx_bps : 1;
        unsigned char classd_ocp_bps : 1;
        unsigned char classd_reserve0 : 4;
    } reg;
} PMIC_CLASSD_CTRL2_UNION;
#endif
#define PMIC_CLASSD_CTRL2_classd_pls_byp_START (0)
#define PMIC_CLASSD_CTRL2_classd_pls_byp_END (0)
#define PMIC_CLASSD_CTRL2_classd_dt_sel_START (1)
#define PMIC_CLASSD_CTRL2_classd_dt_sel_END (1)
#define PMIC_CLASSD_CTRL2_classd_fx_bps_START (2)
#define PMIC_CLASSD_CTRL2_classd_fx_bps_END (2)
#define PMIC_CLASSD_CTRL2_classd_ocp_bps_START (3)
#define PMIC_CLASSD_CTRL2_classd_ocp_bps_END (3)
#define PMIC_CLASSD_CTRL2_classd_reserve0_START (4)
#define PMIC_CLASSD_CTRL2_classd_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char classd_reserve1 : 8;
    } reg;
} PMIC_CLASSD_CTRL3_UNION;
#endif
#define PMIC_CLASSD_CTRL3_classd_reserve1_START (0)
#define PMIC_CLASSD_CTRL3_classd_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ref_eco_en : 1;
        unsigned char reg_thsd_en : 1;
        unsigned char thsd_eco_en : 1;
        unsigned char reserved_0 : 1;
        unsigned char thsd_tmp_set : 2;
        unsigned char reserved_1 : 2;
    } reg;
} PMIC_TH_CTRL_UNION;
#endif
#define PMIC_TH_CTRL_ref_eco_en_START (0)
#define PMIC_TH_CTRL_ref_eco_en_END (0)
#define PMIC_TH_CTRL_reg_thsd_en_START (1)
#define PMIC_TH_CTRL_reg_thsd_en_END (1)
#define PMIC_TH_CTRL_thsd_eco_en_START (2)
#define PMIC_TH_CTRL_thsd_eco_en_END (2)
#define PMIC_TH_CTRL_thsd_tmp_set_START (4)
#define PMIC_TH_CTRL_thsd_tmp_set_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ref_test : 8;
    } reg;
} PMIC_BG_TEST_UNION;
#endif
#define PMIC_BG_TEST_ref_test_START (0)
#define PMIC_BG_TEST_ref_test_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char en_dr3_int : 1;
        unsigned char dr3_mode : 1;
        unsigned char en_dr4_int : 1;
        unsigned char dr4_mode : 1;
        unsigned char en_dr5_int : 1;
        unsigned char dr5_mode : 1;
        unsigned char reserved : 2;
    } reg;
} PMIC_DR_EN_MODE_345_UNION;
#endif
#define PMIC_DR_EN_MODE_345_en_dr3_int_START (0)
#define PMIC_DR_EN_MODE_345_en_dr3_int_END (0)
#define PMIC_DR_EN_MODE_345_dr3_mode_START (1)
#define PMIC_DR_EN_MODE_345_dr3_mode_END (1)
#define PMIC_DR_EN_MODE_345_en_dr4_int_START (2)
#define PMIC_DR_EN_MODE_345_en_dr4_int_END (2)
#define PMIC_DR_EN_MODE_345_dr4_mode_START (3)
#define PMIC_DR_EN_MODE_345_dr4_mode_END (3)
#define PMIC_DR_EN_MODE_345_en_dr5_int_START (4)
#define PMIC_DR_EN_MODE_345_en_dr5_int_END (4)
#define PMIC_DR_EN_MODE_345_dr5_mode_START (5)
#define PMIC_DR_EN_MODE_345_dr5_mode_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char en_dr1_int : 1;
        unsigned char dr1_mode : 1;
        unsigned char en_dr2_int : 1;
        unsigned char dr2_mode : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_DR_EN_MODE_12_UNION;
#endif
#define PMIC_DR_EN_MODE_12_en_dr1_int_START (0)
#define PMIC_DR_EN_MODE_12_en_dr1_int_END (0)
#define PMIC_DR_EN_MODE_12_dr1_mode_START (1)
#define PMIC_DR_EN_MODE_12_dr1_mode_END (1)
#define PMIC_DR_EN_MODE_12_en_dr2_int_START (2)
#define PMIC_DR_EN_MODE_12_en_dr2_int_END (2)
#define PMIC_DR_EN_MODE_12_dr2_mode_START (3)
#define PMIC_DR_EN_MODE_12_dr2_mode_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char flash_period_dr12 : 8;
    } reg;
} PMIC_FLASH_PERIOD_DR12_UNION;
#endif
#define PMIC_FLASH_PERIOD_DR12_flash_period_dr12_START (0)
#define PMIC_FLASH_PERIOD_DR12_flash_period_dr12_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char flash_on_dr12 : 8;
    } reg;
} PMIC_FLASH_ON_DR12_UNION;
#endif
#define PMIC_FLASH_ON_DR12_flash_on_dr12_START (0)
#define PMIC_FLASH_ON_DR12_flash_on_dr12_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char flash_period_dr345 : 8;
    } reg;
} PMIC_FLASH_PERIOD_DR345_UNION;
#endif
#define PMIC_FLASH_PERIOD_DR345_flash_period_dr345_START (0)
#define PMIC_FLASH_PERIOD_DR345_flash_period_dr345_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char flash_on_dr345 : 8;
    } reg;
} PMIC_FLASH_ON_DR345_UNION;
#endif
#define PMIC_FLASH_ON_DR345_flash_on_dr345_START (0)
#define PMIC_FLASH_ON_DR345_flash_on_dr345_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr1_mode_sel : 1;
        unsigned char dr2_mode_sel : 1;
        unsigned char dr3_mode_sel : 1;
        unsigned char dr4_mode_sel : 1;
        unsigned char dr5_mode_sel : 1;
        unsigned char reserved : 3;
    } reg;
} PMIC_DR_MODE_SEL_UNION;
#endif
#define PMIC_DR_MODE_SEL_dr1_mode_sel_START (0)
#define PMIC_DR_MODE_SEL_dr1_mode_sel_END (0)
#define PMIC_DR_MODE_SEL_dr2_mode_sel_START (1)
#define PMIC_DR_MODE_SEL_dr2_mode_sel_END (1)
#define PMIC_DR_MODE_SEL_dr3_mode_sel_START (2)
#define PMIC_DR_MODE_SEL_dr3_mode_sel_END (2)
#define PMIC_DR_MODE_SEL_dr4_mode_sel_START (3)
#define PMIC_DR_MODE_SEL_dr4_mode_sel_END (3)
#define PMIC_DR_MODE_SEL_dr5_mode_sel_START (4)
#define PMIC_DR_MODE_SEL_dr5_mode_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_dr1_en : 1;
        unsigned char dr1_flash_en : 1;
        unsigned char reserved_0 : 2;
        unsigned char reg_dr2_en : 1;
        unsigned char dr2_flash_en : 1;
        unsigned char reserved_1 : 2;
    } reg;
} PMIC_DR_BRE_CTRL_UNION;
#endif
#define PMIC_DR_BRE_CTRL_reg_dr1_en_START (0)
#define PMIC_DR_BRE_CTRL_reg_dr1_en_END (0)
#define PMIC_DR_BRE_CTRL_dr1_flash_en_START (1)
#define PMIC_DR_BRE_CTRL_dr1_flash_en_END (1)
#define PMIC_DR_BRE_CTRL_reg_dr2_en_START (4)
#define PMIC_DR_BRE_CTRL_reg_dr2_en_END (4)
#define PMIC_DR_BRE_CTRL_dr2_flash_en_START (5)
#define PMIC_DR_BRE_CTRL_dr2_flash_en_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr12_t_off : 3;
        unsigned char reserved_0 : 1;
        unsigned char dr12_t_on : 3;
        unsigned char reserved_1 : 1;
    } reg;
} PMIC_DR12_TIM_CONF0_UNION;
#endif
#define PMIC_DR12_TIM_CONF0_dr12_t_off_START (0)
#define PMIC_DR12_TIM_CONF0_dr12_t_off_END (2)
#define PMIC_DR12_TIM_CONF0_dr12_t_on_START (4)
#define PMIC_DR12_TIM_CONF0_dr12_t_on_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr12_t_rise : 3;
        unsigned char reserved_0 : 1;
        unsigned char dr12_t_fall : 3;
        unsigned char reserved_1 : 1;
    } reg;
} PMIC_DR12_TIM_CONF1_UNION;
#endif
#define PMIC_DR12_TIM_CONF1_dr12_t_rise_START (0)
#define PMIC_DR12_TIM_CONF1_dr12_t_rise_END (2)
#define PMIC_DR12_TIM_CONF1_dr12_t_fall_START (4)
#define PMIC_DR12_TIM_CONF1_dr12_t_fall_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr1_iset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_DR1_ISET_UNION;
#endif
#define PMIC_DR1_ISET_dr1_iset_START (0)
#define PMIC_DR1_ISET_dr1_iset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr2_iset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_DR2_ISET_UNION;
#endif
#define PMIC_DR2_ISET_dr2_iset_START (0)
#define PMIC_DR2_ISET_dr2_iset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_dr3_en : 1;
        unsigned char reg_dr4_en : 1;
        unsigned char reg_dr5_en : 1;
        unsigned char reserved : 5;
    } reg;
} PMIC_DR_LED_CTRL_UNION;
#endif
#define PMIC_DR_LED_CTRL_reg_dr3_en_START (0)
#define PMIC_DR_LED_CTRL_reg_dr3_en_END (0)
#define PMIC_DR_LED_CTRL_reg_dr4_en_START (1)
#define PMIC_DR_LED_CTRL_reg_dr4_en_END (1)
#define PMIC_DR_LED_CTRL_reg_dr5_en_START (2)
#define PMIC_DR_LED_CTRL_reg_dr5_en_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr3_out_ctrl : 2;
        unsigned char dr4_out_ctrl : 2;
        unsigned char dr5_out_ctrl : 2;
        unsigned char reserved : 2;
    } reg;
} PMIC_DR_OUT_CTRL_UNION;
#endif
#define PMIC_DR_OUT_CTRL_dr3_out_ctrl_START (0)
#define PMIC_DR_OUT_CTRL_dr3_out_ctrl_END (1)
#define PMIC_DR_OUT_CTRL_dr4_out_ctrl_START (2)
#define PMIC_DR_OUT_CTRL_dr4_out_ctrl_END (3)
#define PMIC_DR_OUT_CTRL_dr5_out_ctrl_START (4)
#define PMIC_DR_OUT_CTRL_dr5_out_ctrl_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr3_iset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_DR3_ISET_UNION;
#endif
#define PMIC_DR3_ISET_dr3_iset_START (0)
#define PMIC_DR3_ISET_dr3_iset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr3_start_delay : 8;
    } reg;
} PMIC_DR3_START_DEL_UNION;
#endif
#define PMIC_DR3_START_DEL_dr3_start_delay_START (0)
#define PMIC_DR3_START_DEL_dr3_start_delay_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr4_iset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_DR4_ISET_UNION;
#endif
#define PMIC_DR4_ISET_dr4_iset_START (0)
#define PMIC_DR4_ISET_dr4_iset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr4_start_delay : 8;
    } reg;
} PMIC_DR4_START_DEL_UNION;
#endif
#define PMIC_DR4_START_DEL_dr4_start_delay_START (0)
#define PMIC_DR4_START_DEL_dr4_start_delay_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr5_iset : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_DR5_ISET_UNION;
#endif
#define PMIC_DR5_ISET_dr5_iset_START (0)
#define PMIC_DR5_ISET_dr5_iset_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr5_start_delay : 8;
    } reg;
} PMIC_DR5_START_DEL_UNION;
#endif
#define PMIC_DR5_START_DEL_dr5_start_delay_START (0)
#define PMIC_DR5_START_DEL_dr5_start_delay_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr345_t_off : 4;
        unsigned char dr345_t_on : 4;
    } reg;
} PMIC_DR334_TIM_CONF0_UNION;
#endif
#define PMIC_DR334_TIM_CONF0_dr345_t_off_START (0)
#define PMIC_DR334_TIM_CONF0_dr345_t_off_END (3)
#define PMIC_DR334_TIM_CONF0_dr345_t_on_START (4)
#define PMIC_DR334_TIM_CONF0_dr345_t_on_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dr345_t_rise : 3;
        unsigned char reserved_0 : 1;
        unsigned char dr345_t_fall : 3;
        unsigned char reserved_1 : 1;
    } reg;
} PMIC_DR345_TIM_CONF1_UNION;
#endif
#define PMIC_DR345_TIM_CONF1_dr345_t_rise_START (0)
#define PMIC_DR345_TIM_CONF1_dr345_t_rise_END (2)
#define PMIC_DR345_TIM_CONF1_dr345_t_fall_START (4)
#define PMIC_DR345_TIM_CONF1_dr345_t_fall_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char vsys_uv_set : 4;
        unsigned char vsys_pwroff_deb_set : 3;
        unsigned char vsys_pwroff_abs_set : 1;
    } reg;
} PMIC_VSYS_LOW_SET0_UNION;
#endif
#define PMIC_VSYS_LOW_SET0_vsys_uv_set_START (0)
#define PMIC_VSYS_LOW_SET0_vsys_uv_set_END (3)
#define PMIC_VSYS_LOW_SET0_vsys_pwroff_deb_set_START (4)
#define PMIC_VSYS_LOW_SET0_vsys_pwroff_deb_set_END (6)
#define PMIC_VSYS_LOW_SET0_vsys_pwroff_abs_set_START (7)
#define PMIC_VSYS_LOW_SET0_vsys_pwroff_abs_set_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char vsys_pwron_set : 3;
        unsigned char reserved_0 : 1;
        unsigned char nfc_on_d2a : 1;
        unsigned char reserved_1 : 3;
    } reg;
} PMIC_VSYS_LOW_SET1_UNION;
#endif
#define PMIC_VSYS_LOW_SET1_vsys_pwron_set_START (0)
#define PMIC_VSYS_LOW_SET1_vsys_pwron_set_END (2)
#define PMIC_VSYS_LOW_SET1_nfc_on_d2a_START (4)
#define PMIC_VSYS_LOW_SET1_nfc_on_d2a_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_ctrl_reserve : 8;
    } reg;
} PMIC_SYS_CTRL_RESERVE_UNION;
#endif
#define PMIC_SYS_CTRL_RESERVE_sys_ctrl_reserve_START (0)
#define PMIC_SYS_CTRL_RESERVE_sys_ctrl_reserve_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_eco_in_hd_mask : 1;
        unsigned char reg_xo_core_hd_mask : 1;
        unsigned char reg_xo_ldo_hd_mask : 1;
        unsigned char reg_sys_clk_hd_mask : 1;
        unsigned char reg_abb_clk_hd_mask : 1;
        unsigned char reg_wifi_clk_hd_mask : 1;
        unsigned char reserved : 2;
    } reg;
} PMIC_HARDWIRE_CTRL0_UNION;
#endif
#define PMIC_HARDWIRE_CTRL0_reg_eco_in_hd_mask_START (0)
#define PMIC_HARDWIRE_CTRL0_reg_eco_in_hd_mask_END (0)
#define PMIC_HARDWIRE_CTRL0_reg_xo_core_hd_mask_START (1)
#define PMIC_HARDWIRE_CTRL0_reg_xo_core_hd_mask_END (1)
#define PMIC_HARDWIRE_CTRL0_reg_xo_ldo_hd_mask_START (2)
#define PMIC_HARDWIRE_CTRL0_reg_xo_ldo_hd_mask_END (2)
#define PMIC_HARDWIRE_CTRL0_reg_sys_clk_hd_mask_START (3)
#define PMIC_HARDWIRE_CTRL0_reg_sys_clk_hd_mask_END (3)
#define PMIC_HARDWIRE_CTRL0_reg_abb_clk_hd_mask_START (4)
#define PMIC_HARDWIRE_CTRL0_reg_abb_clk_hd_mask_END (4)
#define PMIC_HARDWIRE_CTRL0_reg_wifi_clk_hd_mask_START (5)
#define PMIC_HARDWIRE_CTRL0_reg_wifi_clk_hd_mask_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_ldo27_hd_mask : 1;
        unsigned char reg_ldo26_hd_mask : 2;
        unsigned char reg_ldo14_hd_mask : 1;
        unsigned char reg_ldo13_hd_mask : 1;
        unsigned char reserved : 3;
    } reg;
} PMIC_HARDWIRE_CTRL1_UNION;
#endif
#define PMIC_HARDWIRE_CTRL1_reg_ldo27_hd_mask_START (0)
#define PMIC_HARDWIRE_CTRL1_reg_ldo27_hd_mask_END (0)
#define PMIC_HARDWIRE_CTRL1_reg_ldo26_hd_mask_START (1)
#define PMIC_HARDWIRE_CTRL1_reg_ldo26_hd_mask_END (2)
#define PMIC_HARDWIRE_CTRL1_reg_ldo14_hd_mask_START (3)
#define PMIC_HARDWIRE_CTRL1_reg_ldo14_hd_mask_END (3)
#define PMIC_HARDWIRE_CTRL1_reg_ldo13_hd_mask_START (4)
#define PMIC_HARDWIRE_CTRL1_reg_ldo13_hd_mask_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char peri_en_ldo5_on : 1;
        unsigned char peri_en_ldo2_eco : 1;
        unsigned char peri_en_ldo0_2_eco : 1;
        unsigned char peri_en_buck4_on : 1;
        unsigned char peri_en_buck3_eco : 1;
        unsigned char peri_en_buck2_eco : 1;
        unsigned char peri_en_buck1_eco : 1;
        unsigned char peri_en_buck0_on : 1;
    } reg;
} PMIC_PERI_CTRL0_UNION;
#endif
#define PMIC_PERI_CTRL0_peri_en_ldo5_on_START (0)
#define PMIC_PERI_CTRL0_peri_en_ldo5_on_END (0)
#define PMIC_PERI_CTRL0_peri_en_ldo2_eco_START (1)
#define PMIC_PERI_CTRL0_peri_en_ldo2_eco_END (1)
#define PMIC_PERI_CTRL0_peri_en_ldo0_2_eco_START (2)
#define PMIC_PERI_CTRL0_peri_en_ldo0_2_eco_END (2)
#define PMIC_PERI_CTRL0_peri_en_buck4_on_START (3)
#define PMIC_PERI_CTRL0_peri_en_buck4_on_END (3)
#define PMIC_PERI_CTRL0_peri_en_buck3_eco_START (4)
#define PMIC_PERI_CTRL0_peri_en_buck3_eco_END (4)
#define PMIC_PERI_CTRL0_peri_en_buck2_eco_START (5)
#define PMIC_PERI_CTRL0_peri_en_buck2_eco_END (5)
#define PMIC_PERI_CTRL0_peri_en_buck1_eco_START (6)
#define PMIC_PERI_CTRL0_peri_en_buck1_eco_END (6)
#define PMIC_PERI_CTRL0_peri_en_buck0_on_START (7)
#define PMIC_PERI_CTRL0_peri_en_buck0_on_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char peri_en_ldo30_on : 1;
        unsigned char peri_en_ldo29_eco : 1;
        unsigned char peri_en_ldo23_on : 1;
        unsigned char peri_en_ldo12_eco : 1;
        unsigned char peri_en_ldo11_eco : 1;
        unsigned char peri_en_ldo10_on : 1;
        unsigned char peri_en_ldo8_on : 1;
        unsigned char peri_en_ldo7_on : 1;
    } reg;
} PMIC_PERI_CTRL1_UNION;
#endif
#define PMIC_PERI_CTRL1_peri_en_ldo30_on_START (0)
#define PMIC_PERI_CTRL1_peri_en_ldo30_on_END (0)
#define PMIC_PERI_CTRL1_peri_en_ldo29_eco_START (1)
#define PMIC_PERI_CTRL1_peri_en_ldo29_eco_END (1)
#define PMIC_PERI_CTRL1_peri_en_ldo23_on_START (2)
#define PMIC_PERI_CTRL1_peri_en_ldo23_on_END (2)
#define PMIC_PERI_CTRL1_peri_en_ldo12_eco_START (3)
#define PMIC_PERI_CTRL1_peri_en_ldo12_eco_END (3)
#define PMIC_PERI_CTRL1_peri_en_ldo11_eco_START (4)
#define PMIC_PERI_CTRL1_peri_en_ldo11_eco_END (4)
#define PMIC_PERI_CTRL1_peri_en_ldo10_on_START (5)
#define PMIC_PERI_CTRL1_peri_en_ldo10_on_END (5)
#define PMIC_PERI_CTRL1_peri_en_ldo8_on_START (6)
#define PMIC_PERI_CTRL1_peri_en_ldo8_on_END (6)
#define PMIC_PERI_CTRL1_peri_en_ldo7_on_START (7)
#define PMIC_PERI_CTRL1_peri_en_ldo7_on_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char peri_en_buck3_vset : 1;
        unsigned char peri_en_buck2_vset : 1;
        unsigned char peri_en_buck1_vset : 1;
        unsigned char peri_en_ldo29_vset : 1;
        unsigned char peri_en_ldo2_vset : 1;
        unsigned char peri_en_ldo0_2_vset : 1;
        unsigned char reserved : 2;
    } reg;
} PMIC_PERI_VSET_CTRL_UNION;
#endif
#define PMIC_PERI_VSET_CTRL_peri_en_buck3_vset_START (0)
#define PMIC_PERI_VSET_CTRL_peri_en_buck3_vset_END (0)
#define PMIC_PERI_VSET_CTRL_peri_en_buck2_vset_START (1)
#define PMIC_PERI_VSET_CTRL_peri_en_buck2_vset_END (1)
#define PMIC_PERI_VSET_CTRL_peri_en_buck1_vset_START (2)
#define PMIC_PERI_VSET_CTRL_peri_en_buck1_vset_END (2)
#define PMIC_PERI_VSET_CTRL_peri_en_ldo29_vset_START (3)
#define PMIC_PERI_VSET_CTRL_peri_en_ldo29_vset_END (3)
#define PMIC_PERI_VSET_CTRL_peri_en_ldo2_vset_START (4)
#define PMIC_PERI_VSET_CTRL_peri_en_ldo2_vset_END (4)
#define PMIC_PERI_VSET_CTRL_peri_en_ldo0_2_vset_START (5)
#define PMIC_PERI_VSET_CTRL_peri_en_ldo0_2_vset_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reserved_0 : 1;
        unsigned char ldo8_on_sel : 1;
        unsigned char ldo5_on_sel : 1;
        unsigned char ldo7_on_sel : 1;
        unsigned char buck4_on_sel : 1;
        unsigned char reserved_1 : 3;
    } reg;
} PMIC_PERI_TIME__CTRL_UNION;
#endif
#define PMIC_PERI_TIME__CTRL_ldo8_on_sel_START (1)
#define PMIC_PERI_TIME__CTRL_ldo8_on_sel_END (1)
#define PMIC_PERI_TIME__CTRL_ldo5_on_sel_START (2)
#define PMIC_PERI_TIME__CTRL_ldo5_on_sel_END (2)
#define PMIC_PERI_TIME__CTRL_ldo7_on_sel_START (3)
#define PMIC_PERI_TIME__CTRL_ldo7_on_sel_END (3)
#define PMIC_PERI_TIME__CTRL_buck4_on_sel_START (4)
#define PMIC_PERI_TIME__CTRL_buck4_on_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hreset_mode : 1;
        unsigned char reserved : 7;
    } reg;
} PMIC_HRESET_PWRDOWN_CTRL_UNION;
#endif
#define PMIC_HRESET_PWRDOWN_CTRL_hreset_mode_START (0)
#define PMIC_HRESET_PWRDOWN_CTRL_hreset_mode_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char en_32k_sys : 1;
        unsigned char en_32k_bt : 1;
        unsigned char en_32k_gps : 1;
        unsigned char reserved : 5;
    } reg;
} PMIC_OSC32K_ONOFF_CTRL_UNION;
#endif
#define PMIC_OSC32K_ONOFF_CTRL_en_32k_sys_START (0)
#define PMIC_OSC32K_ONOFF_CTRL_en_32k_sys_END (0)
#define PMIC_OSC32K_ONOFF_CTRL_en_32k_bt_START (1)
#define PMIC_OSC32K_ONOFF_CTRL_en_32k_bt_END (1)
#define PMIC_OSC32K_ONOFF_CTRL_en_32k_gps_START (2)
#define PMIC_OSC32K_ONOFF_CTRL_en_32k_gps_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char scp_ocp_deb_sel2 : 3;
        unsigned char reserved_0 : 1;
        unsigned char scp_ocp_deb_sel1 : 3;
        unsigned char reserved_1 : 1;
    } reg;
} PMIC_OCP_DEB_CTRL_UNION;
#endif
#define PMIC_OCP_DEB_CTRL_scp_ocp_deb_sel2_START (0)
#define PMIC_OCP_DEB_CTRL_scp_ocp_deb_sel2_END (2)
#define PMIC_OCP_DEB_CTRL_scp_ocp_deb_sel1_START (4)
#define PMIC_OCP_DEB_CTRL_scp_ocp_deb_sel1_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char en_buck_scp_deb : 1;
        unsigned char en_buck_ocp_deb : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_OCP_SCP_ONOFF_UNION;
#endif
#define PMIC_OCP_SCP_ONOFF_en_buck_scp_deb_START (0)
#define PMIC_OCP_SCP_ONOFF_en_buck_scp_deb_END (0)
#define PMIC_OCP_SCP_ONOFF_en_buck_ocp_deb_START (1)
#define PMIC_OCP_SCP_ONOFF_en_buck_ocp_deb_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char vsys_pwroff_deb_sel : 3;
        unsigned char reserved : 1;
        unsigned char vsys_uv_deb_sel : 4;
    } reg;
} PMIC_UV_VSYS_DEB_CTRL_UNION;
#endif
#define PMIC_UV_VSYS_DEB_CTRL_vsys_pwroff_deb_sel_START (0)
#define PMIC_UV_VSYS_DEB_CTRL_vsys_pwroff_deb_sel_END (2)
#define PMIC_UV_VSYS_DEB_CTRL_vsys_uv_deb_sel_START (4)
#define PMIC_UV_VSYS_DEB_CTRL_vsys_uv_deb_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck3_ocp_auto_stop : 2;
        unsigned char buck2_ocp_auto_stop : 2;
        unsigned char buck1_ocp_auto_stop : 2;
        unsigned char buck0_ocp_auto_stop : 2;
    } reg;
} PMIC_BUCK0_3_OCP_CTRL_UNION;
#endif
#define PMIC_BUCK0_3_OCP_CTRL_buck3_ocp_auto_stop_START (0)
#define PMIC_BUCK0_3_OCP_CTRL_buck3_ocp_auto_stop_END (1)
#define PMIC_BUCK0_3_OCP_CTRL_buck2_ocp_auto_stop_START (2)
#define PMIC_BUCK0_3_OCP_CTRL_buck2_ocp_auto_stop_END (3)
#define PMIC_BUCK0_3_OCP_CTRL_buck1_ocp_auto_stop_START (4)
#define PMIC_BUCK0_3_OCP_CTRL_buck1_ocp_auto_stop_END (5)
#define PMIC_BUCK0_3_OCP_CTRL_buck0_ocp_auto_stop_START (6)
#define PMIC_BUCK0_3_OCP_CTRL_buck0_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo2_ocp_auto_stop : 2;
        unsigned char ldo1_ocp_auto_stop : 2;
        unsigned char ldo0_ocp_auto_stop : 2;
        unsigned char buck4_ocp_auto_stop : 2;
    } reg;
} PMIC_BUCK4_LDO0_1_OCP_CTRL_UNION;
#endif
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_ldo2_ocp_auto_stop_START (0)
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_ldo2_ocp_auto_stop_END (1)
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_ldo1_ocp_auto_stop_START (2)
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_ldo1_ocp_auto_stop_END (3)
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_ldo0_ocp_auto_stop_START (4)
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_ldo0_ocp_auto_stop_END (5)
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_buck4_ocp_auto_stop_START (6)
#define PMIC_BUCK4_LDO0_1_OCP_CTRL_buck4_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo7_ocp_auto_stop : 2;
        unsigned char ldo5_ocp_auto_stop : 2;
        unsigned char ldo4_ocp_auto_stop : 2;
        unsigned char ldo3_ocp_auto_stop : 2;
    } reg;
} PMIC_LDO3_7_OCP_CTRL_UNION;
#endif
#define PMIC_LDO3_7_OCP_CTRL_ldo7_ocp_auto_stop_START (0)
#define PMIC_LDO3_7_OCP_CTRL_ldo7_ocp_auto_stop_END (1)
#define PMIC_LDO3_7_OCP_CTRL_ldo5_ocp_auto_stop_START (2)
#define PMIC_LDO3_7_OCP_CTRL_ldo5_ocp_auto_stop_END (3)
#define PMIC_LDO3_7_OCP_CTRL_ldo4_ocp_auto_stop_START (4)
#define PMIC_LDO3_7_OCP_CTRL_ldo4_ocp_auto_stop_END (5)
#define PMIC_LDO3_7_OCP_CTRL_ldo3_ocp_auto_stop_START (6)
#define PMIC_LDO3_7_OCP_CTRL_ldo3_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo11_ocp_auto_stop : 2;
        unsigned char ldo10_ocp_auto_stop : 2;
        unsigned char ldo9_ocp_auto_stop : 2;
        unsigned char ldo8_ocp_auto_stop : 2;
    } reg;
} PMIC_LDO8_11_OCP_CTRL_UNION;
#endif
#define PMIC_LDO8_11_OCP_CTRL_ldo11_ocp_auto_stop_START (0)
#define PMIC_LDO8_11_OCP_CTRL_ldo11_ocp_auto_stop_END (1)
#define PMIC_LDO8_11_OCP_CTRL_ldo10_ocp_auto_stop_START (2)
#define PMIC_LDO8_11_OCP_CTRL_ldo10_ocp_auto_stop_END (3)
#define PMIC_LDO8_11_OCP_CTRL_ldo9_ocp_auto_stop_START (4)
#define PMIC_LDO8_11_OCP_CTRL_ldo9_ocp_auto_stop_END (5)
#define PMIC_LDO8_11_OCP_CTRL_ldo8_ocp_auto_stop_START (6)
#define PMIC_LDO8_11_OCP_CTRL_ldo8_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo15_ocp_auto_stop : 2;
        unsigned char ldo14_ocp_auto_stop : 2;
        unsigned char ldo13_ocp_auto_stop : 2;
        unsigned char ldo12_ocp_auto_stop : 2;
    } reg;
} PMIC_LDO12_15_OCP_CTRL_UNION;
#endif
#define PMIC_LDO12_15_OCP_CTRL_ldo15_ocp_auto_stop_START (0)
#define PMIC_LDO12_15_OCP_CTRL_ldo15_ocp_auto_stop_END (1)
#define PMIC_LDO12_15_OCP_CTRL_ldo14_ocp_auto_stop_START (2)
#define PMIC_LDO12_15_OCP_CTRL_ldo14_ocp_auto_stop_END (3)
#define PMIC_LDO12_15_OCP_CTRL_ldo13_ocp_auto_stop_START (4)
#define PMIC_LDO12_15_OCP_CTRL_ldo13_ocp_auto_stop_END (5)
#define PMIC_LDO12_15_OCP_CTRL_ldo12_ocp_auto_stop_START (6)
#define PMIC_LDO12_15_OCP_CTRL_ldo12_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo20_ocp_auto_stop : 2;
        unsigned char ldo19_ocp_auto_stop : 2;
        unsigned char ldo17_ocp_auto_stop : 2;
        unsigned char ldo16_ocp_auto_stop : 2;
    } reg;
} PMIC_LDO16_20_OCP_CTRL_UNION;
#endif
#define PMIC_LDO16_20_OCP_CTRL_ldo20_ocp_auto_stop_START (0)
#define PMIC_LDO16_20_OCP_CTRL_ldo20_ocp_auto_stop_END (1)
#define PMIC_LDO16_20_OCP_CTRL_ldo19_ocp_auto_stop_START (2)
#define PMIC_LDO16_20_OCP_CTRL_ldo19_ocp_auto_stop_END (3)
#define PMIC_LDO16_20_OCP_CTRL_ldo17_ocp_auto_stop_START (4)
#define PMIC_LDO16_20_OCP_CTRL_ldo17_ocp_auto_stop_END (5)
#define PMIC_LDO16_20_OCP_CTRL_ldo16_ocp_auto_stop_START (6)
#define PMIC_LDO16_20_OCP_CTRL_ldo16_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo24_ocp_auto_stop : 2;
        unsigned char ldo23_ocp_auto_stop : 2;
        unsigned char ldo22_ocp_auto_stop : 2;
        unsigned char ldo21_ocp_auto_stop : 2;
    } reg;
} PMIC_LDO21_24_OCP_CTRL_UNION;
#endif
#define PMIC_LDO21_24_OCP_CTRL_ldo24_ocp_auto_stop_START (0)
#define PMIC_LDO21_24_OCP_CTRL_ldo24_ocp_auto_stop_END (1)
#define PMIC_LDO21_24_OCP_CTRL_ldo23_ocp_auto_stop_START (2)
#define PMIC_LDO21_24_OCP_CTRL_ldo23_ocp_auto_stop_END (3)
#define PMIC_LDO21_24_OCP_CTRL_ldo22_ocp_auto_stop_START (4)
#define PMIC_LDO21_24_OCP_CTRL_ldo22_ocp_auto_stop_END (5)
#define PMIC_LDO21_24_OCP_CTRL_ldo21_ocp_auto_stop_START (6)
#define PMIC_LDO21_24_OCP_CTRL_ldo21_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo28_ocp_auto_stop : 2;
        unsigned char ldo27_ocp_auto_stop : 2;
        unsigned char ldo26_ocp_auto_stop : 2;
        unsigned char ldo25_ocp_auto_stop : 2;
    } reg;
} PMIC_LDO25_28_OCP_CTRL_UNION;
#endif
#define PMIC_LDO25_28_OCP_CTRL_ldo28_ocp_auto_stop_START (0)
#define PMIC_LDO25_28_OCP_CTRL_ldo28_ocp_auto_stop_END (1)
#define PMIC_LDO25_28_OCP_CTRL_ldo27_ocp_auto_stop_START (2)
#define PMIC_LDO25_28_OCP_CTRL_ldo27_ocp_auto_stop_END (3)
#define PMIC_LDO25_28_OCP_CTRL_ldo26_ocp_auto_stop_START (4)
#define PMIC_LDO25_28_OCP_CTRL_ldo26_ocp_auto_stop_END (5)
#define PMIC_LDO25_28_OCP_CTRL_ldo25_ocp_auto_stop_START (6)
#define PMIC_LDO25_28_OCP_CTRL_ldo25_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ldo32_ocp_auto_stop : 2;
        unsigned char ldo31_ocp_auto_stop : 2;
        unsigned char ldo30_ocp_auto_stop : 2;
        unsigned char ldo29_ocp_auto_stop : 2;
    } reg;
} PMIC_LDO29_32_OCP_CTRL_UNION;
#endif
#define PMIC_LDO29_32_OCP_CTRL_ldo32_ocp_auto_stop_START (0)
#define PMIC_LDO29_32_OCP_CTRL_ldo32_ocp_auto_stop_END (1)
#define PMIC_LDO29_32_OCP_CTRL_ldo31_ocp_auto_stop_START (2)
#define PMIC_LDO29_32_OCP_CTRL_ldo31_ocp_auto_stop_END (3)
#define PMIC_LDO29_32_OCP_CTRL_ldo30_ocp_auto_stop_START (4)
#define PMIC_LDO29_32_OCP_CTRL_ldo30_ocp_auto_stop_END (5)
#define PMIC_LDO29_32_OCP_CTRL_ldo29_ocp_auto_stop_START (6)
#define PMIC_LDO29_32_OCP_CTRL_ldo29_ocp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck0_scp_auto_stop : 2;
        unsigned char classd_ocp_auto_stop : 2;
        unsigned char reserved : 4;
    } reg;
} PMIC_CLASS_BUCK0_SCP_CTRL_UNION;
#endif
#define PMIC_CLASS_BUCK0_SCP_CTRL_buck0_scp_auto_stop_START (0)
#define PMIC_CLASS_BUCK0_SCP_CTRL_buck0_scp_auto_stop_END (1)
#define PMIC_CLASS_BUCK0_SCP_CTRL_classd_ocp_auto_stop_START (2)
#define PMIC_CLASS_BUCK0_SCP_CTRL_classd_ocp_auto_stop_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_scp_auto_stop : 2;
        unsigned char buck3_scp_auto_stop : 2;
        unsigned char buck2_scp_auto_stop : 2;
        unsigned char buck1_scp_auto_stop : 2;
    } reg;
} PMIC_BUCK1_4_SCP_CTRL_UNION;
#endif
#define PMIC_BUCK1_4_SCP_CTRL_buck4_scp_auto_stop_START (0)
#define PMIC_BUCK1_4_SCP_CTRL_buck4_scp_auto_stop_END (1)
#define PMIC_BUCK1_4_SCP_CTRL_buck3_scp_auto_stop_START (2)
#define PMIC_BUCK1_4_SCP_CTRL_buck3_scp_auto_stop_END (3)
#define PMIC_BUCK1_4_SCP_CTRL_buck2_scp_auto_stop_START (4)
#define PMIC_BUCK1_4_SCP_CTRL_buck2_scp_auto_stop_END (5)
#define PMIC_BUCK1_4_SCP_CTRL_buck1_scp_auto_stop_START (6)
#define PMIC_BUCK1_4_SCP_CTRL_buck1_scp_auto_stop_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char vsys_pwroff_abs_pd_mask : 1;
        unsigned char vsys_pwroff_deb_pd_mask : 1;
        unsigned char thsd_otmp140_pd_mask : 1;
        unsigned char vsys_ov_pd_mask : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_SYS_CTRL0_UNION;
#endif
#define PMIC_SYS_CTRL0_vsys_pwroff_abs_pd_mask_START (0)
#define PMIC_SYS_CTRL0_vsys_pwroff_abs_pd_mask_END (0)
#define PMIC_SYS_CTRL0_vsys_pwroff_deb_pd_mask_START (1)
#define PMIC_SYS_CTRL0_vsys_pwroff_deb_pd_mask_END (1)
#define PMIC_SYS_CTRL0_thsd_otmp140_pd_mask_START (2)
#define PMIC_SYS_CTRL0_thsd_otmp140_pd_mask_END (2)
#define PMIC_SYS_CTRL0_vsys_ov_pd_mask_START (3)
#define PMIC_SYS_CTRL0_vsys_ov_pd_mask_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_smpl_open_en : 1;
        unsigned char np_smpl_time_sel : 2;
        unsigned char reserved_0 : 1;
        unsigned char np_pwron_8s_sel : 1;
        unsigned char reserved_1 : 3;
    } reg;
} PMIC_SYS_CTRL1_UNION;
#endif
#define PMIC_SYS_CTRL1_np_smpl_open_en_START (0)
#define PMIC_SYS_CTRL1_np_smpl_open_en_END (0)
#define PMIC_SYS_CTRL1_np_smpl_time_sel_START (1)
#define PMIC_SYS_CTRL1_np_smpl_time_sel_END (2)
#define PMIC_SYS_CTRL1_np_pwron_8s_sel_START (4)
#define PMIC_SYS_CTRL1_np_pwron_8s_sel_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char coul_codec_clk_en_mask : 1;
        unsigned char coul_wifi_clk_en_mask : 1;
        unsigned char coul_sys_clk_en_mask : 1;
        unsigned char reserved : 5;
    } reg;
} PMIC_COUL_ECO_MASK_UNION;
#endif
#define PMIC_COUL_ECO_MASK_coul_codec_clk_en_mask_START (0)
#define PMIC_COUL_ECO_MASK_coul_codec_clk_en_mask_END (0)
#define PMIC_COUL_ECO_MASK_coul_wifi_clk_en_mask_START (1)
#define PMIC_COUL_ECO_MASK_coul_wifi_clk_en_mask_END (1)
#define PMIC_COUL_ECO_MASK_coul_sys_clk_en_mask_START (2)
#define PMIC_COUL_ECO_MASK_coul_sys_clk_en_mask_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sim0_hpd_r_pd_en : 1;
        unsigned char sim0_hpd_f_pd_en : 1;
        unsigned char sim0_hpd_h_pd_en : 1;
        unsigned char sim0_hpd_l_pd_en : 1;
        unsigned char sim1_hpd_r_pd_en : 1;
        unsigned char sim1_hpd_f_pd_en : 1;
        unsigned char sim1_hpd_h_pd_en : 1;
        unsigned char sim1_hpd_l_pd_en : 1;
    } reg;
} PMIC_SIM_CTRL_UNION;
#endif
#define PMIC_SIM_CTRL_sim0_hpd_r_pd_en_START (0)
#define PMIC_SIM_CTRL_sim0_hpd_r_pd_en_END (0)
#define PMIC_SIM_CTRL_sim0_hpd_f_pd_en_START (1)
#define PMIC_SIM_CTRL_sim0_hpd_f_pd_en_END (1)
#define PMIC_SIM_CTRL_sim0_hpd_h_pd_en_START (2)
#define PMIC_SIM_CTRL_sim0_hpd_h_pd_en_END (2)
#define PMIC_SIM_CTRL_sim0_hpd_l_pd_en_START (3)
#define PMIC_SIM_CTRL_sim0_hpd_l_pd_en_END (3)
#define PMIC_SIM_CTRL_sim1_hpd_r_pd_en_START (4)
#define PMIC_SIM_CTRL_sim1_hpd_r_pd_en_END (4)
#define PMIC_SIM_CTRL_sim1_hpd_f_pd_en_START (5)
#define PMIC_SIM_CTRL_sim1_hpd_f_pd_en_END (5)
#define PMIC_SIM_CTRL_sim1_hpd_h_pd_en_START (6)
#define PMIC_SIM_CTRL_sim1_hpd_h_pd_en_END (6)
#define PMIC_SIM_CTRL_sim1_hpd_l_pd_en_START (7)
#define PMIC_SIM_CTRL_sim1_hpd_l_pd_en_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sim_hpd_deb_sel : 5;
        unsigned char sim_del_sel : 3;
    } reg;
} PMIC_SIM_DEB_CTRL_UNION;
#endif
#define PMIC_SIM_DEB_CTRL_sim_hpd_deb_sel_START (0)
#define PMIC_SIM_DEB_CTRL_sim_hpd_deb_sel_END (4)
#define PMIC_SIM_DEB_CTRL_sim_del_sel_START (5)
#define PMIC_SIM_DEB_CTRL_sim_del_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char aux_offset_cfg : 2;
        unsigned char aux_ibias_cfg : 2;
        unsigned char reserved : 4;
    } reg;
} PMIC_AUX_IBIAS_CFG_UNION;
#endif
#define PMIC_AUX_IBIAS_CFG_aux_offset_cfg_START (0)
#define PMIC_AUX_IBIAS_CFG_aux_offset_cfg_END (1)
#define PMIC_AUX_IBIAS_CFG_aux_ibias_cfg_START (2)
#define PMIC_AUX_IBIAS_CFG_aux_ibias_cfg_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_0 : 8;
    } reg;
} PMIC_IRQ_MASK_0_UNION;
#endif
#define PMIC_IRQ_MASK_0_irq_mask_0_START (0)
#define PMIC_IRQ_MASK_0_irq_mask_0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_1 : 8;
    } reg;
} PMIC_IRQ_MASK_1_UNION;
#endif
#define PMIC_IRQ_MASK_1_irq_mask_1_START (0)
#define PMIC_IRQ_MASK_1_irq_mask_1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_2 : 8;
    } reg;
} PMIC_IRQ_MASK_2_UNION;
#endif
#define PMIC_IRQ_MASK_2_irq_mask_2_START (0)
#define PMIC_IRQ_MASK_2_irq_mask_2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_3 : 8;
    } reg;
} PMIC_IRQ_MASK_3_UNION;
#endif
#define PMIC_IRQ_MASK_3_irq_mask_3_START (0)
#define PMIC_IRQ_MASK_3_irq_mask_3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_4 : 8;
    } reg;
} PMIC_IRQ_MASK_4_UNION;
#endif
#define PMIC_IRQ_MASK_4_irq_mask_4_START (0)
#define PMIC_IRQ_MASK_4_irq_mask_4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_5 : 8;
    } reg;
} PMIC_IRQ_MASK_5_UNION;
#endif
#define PMIC_IRQ_MASK_5_irq_mask_5_START (0)
#define PMIC_IRQ_MASK_5_irq_mask_5_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_6 : 8;
    } reg;
} PMIC_IRQ_MASK_6_UNION;
#endif
#define PMIC_IRQ_MASK_6_irq_mask_6_START (0)
#define PMIC_IRQ_MASK_6_irq_mask_6_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_7 : 8;
    } reg;
} PMIC_IRQ_MASK_7_UNION;
#endif
#define PMIC_IRQ_MASK_7_irq_mask_7_START (0)
#define PMIC_IRQ_MASK_7_irq_mask_7_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_8 : 8;
    } reg;
} PMIC_IRQ_MASK_8_UNION;
#endif
#define PMIC_IRQ_MASK_8_irq_mask_8_START (0)
#define PMIC_IRQ_MASK_8_irq_mask_8_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char irq_mask_9 : 8;
    } reg;
} PMIC_IRQ_MASK_9_UNION;
#endif
#define PMIC_IRQ_MASK_9_irq_mask_9_START (0)
#define PMIC_IRQ_MASK_9_irq_mask_9_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdob0 : 8;
    } reg;
} PMIC_OTP0_0_UNION;
#endif
#define PMIC_OTP0_0_otp0_pdob0_START (0)
#define PMIC_OTP0_0_otp0_pdob0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdob1 : 8;
    } reg;
} PMIC_OTP0_1_UNION;
#endif
#define PMIC_OTP0_1_otp0_pdob1_START (0)
#define PMIC_OTP0_1_otp0_pdob1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdob2 : 8;
    } reg;
} PMIC_OTP0_2_UNION;
#endif
#define PMIC_OTP0_2_otp0_pdob2_START (0)
#define PMIC_OTP0_2_otp0_pdob2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdob3 : 8;
    } reg;
} PMIC_OTP0_3_UNION;
#endif
#define PMIC_OTP0_3_otp0_pdob3_START (0)
#define PMIC_OTP0_3_otp0_pdob3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pwe_int : 1;
        unsigned char otp0_pwe_pulse : 1;
        unsigned char otp0_por_int : 1;
        unsigned char otp0_por_pulse : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_OTP0_CTRL_0_UNION;
#endif
#define PMIC_OTP0_CTRL_0_otp0_pwe_int_START (0)
#define PMIC_OTP0_CTRL_0_otp0_pwe_int_END (0)
#define PMIC_OTP0_CTRL_0_otp0_pwe_pulse_START (1)
#define PMIC_OTP0_CTRL_0_otp0_pwe_pulse_END (1)
#define PMIC_OTP0_CTRL_0_otp0_por_int_START (2)
#define PMIC_OTP0_CTRL_0_otp0_por_int_END (2)
#define PMIC_OTP0_CTRL_0_otp0_por_pulse_START (3)
#define PMIC_OTP0_CTRL_0_otp0_por_pulse_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pa : 2;
        unsigned char otp0_ptm : 2;
        unsigned char otp01_pprog : 1;
        unsigned char reserved : 3;
    } reg;
} PMIC_OTP0_CTRL_1_UNION;
#endif
#define PMIC_OTP0_CTRL_1_otp0_pa_START (0)
#define PMIC_OTP0_CTRL_1_otp0_pa_END (1)
#define PMIC_OTP0_CTRL_1_otp0_ptm_START (2)
#define PMIC_OTP0_CTRL_1_otp0_ptm_END (3)
#define PMIC_OTP0_CTRL_1_otp01_pprog_START (4)
#define PMIC_OTP0_CTRL_1_otp01_pprog_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdin : 8;
    } reg;
} PMIC_OTP0_WDATA_UNION;
#endif
#define PMIC_OTP0_WDATA_otp0_pdin_START (0)
#define PMIC_OTP0_WDATA_otp0_pdin_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdob0_w : 8;
    } reg;
} PMIC_OTP0_0_W_UNION;
#endif
#define PMIC_OTP0_0_W_otp0_pdob0_w_START (0)
#define PMIC_OTP0_0_W_otp0_pdob0_w_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdob1_w : 8;
    } reg;
} PMIC_OTP0_1_W_UNION;
#endif
#define PMIC_OTP0_1_W_otp0_pdob1_w_START (0)
#define PMIC_OTP0_1_W_otp0_pdob1_w_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdob2_w : 8;
    } reg;
} PMIC_OTP0_2_W_UNION;
#endif
#define PMIC_OTP0_2_W_otp0_pdob2_w_START (0)
#define PMIC_OTP0_2_W_otp0_pdob2_w_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp0_pdob3_w : 8;
    } reg;
} PMIC_OTP0_3_W_UNION;
#endif
#define PMIC_OTP0_3_W_otp0_pdob3_w_START (0)
#define PMIC_OTP0_3_W_otp0_pdob3_w_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdob0 : 8;
    } reg;
} PMIC_OTP1_0_UNION;
#endif
#define PMIC_OTP1_0_otp1_pdob0_START (0)
#define PMIC_OTP1_0_otp1_pdob0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdob1 : 8;
    } reg;
} PMIC_OTP1_1_UNION;
#endif
#define PMIC_OTP1_1_otp1_pdob1_START (0)
#define PMIC_OTP1_1_otp1_pdob1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdob2 : 8;
    } reg;
} PMIC_OTP1_2_UNION;
#endif
#define PMIC_OTP1_2_otp1_pdob2_START (0)
#define PMIC_OTP1_2_otp1_pdob2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdob3 : 8;
    } reg;
} PMIC_OTP1_3_UNION;
#endif
#define PMIC_OTP1_3_otp1_pdob3_START (0)
#define PMIC_OTP1_3_otp1_pdob3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pwe_int : 1;
        unsigned char otp1_pwe_pulse : 1;
        unsigned char otp1_por_int : 1;
        unsigned char otp1_por_pulse : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_OTP1_CTRL_0_UNION;
#endif
#define PMIC_OTP1_CTRL_0_otp1_pwe_int_START (0)
#define PMIC_OTP1_CTRL_0_otp1_pwe_int_END (0)
#define PMIC_OTP1_CTRL_0_otp1_pwe_pulse_START (1)
#define PMIC_OTP1_CTRL_0_otp1_pwe_pulse_END (1)
#define PMIC_OTP1_CTRL_0_otp1_por_int_START (2)
#define PMIC_OTP1_CTRL_0_otp1_por_int_END (2)
#define PMIC_OTP1_CTRL_0_otp1_por_pulse_START (3)
#define PMIC_OTP1_CTRL_0_otp1_por_pulse_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pa : 2;
        unsigned char otp1_ptm : 2;
        unsigned char reserved : 4;
    } reg;
} PMIC_OTP1_CTRL_1_UNION;
#endif
#define PMIC_OTP1_CTRL_1_otp1_pa_START (0)
#define PMIC_OTP1_CTRL_1_otp1_pa_END (1)
#define PMIC_OTP1_CTRL_1_otp1_ptm_START (2)
#define PMIC_OTP1_CTRL_1_otp1_ptm_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdin : 8;
    } reg;
} PMIC_OTP1_WDATA_UNION;
#endif
#define PMIC_OTP1_WDATA_otp1_pdin_START (0)
#define PMIC_OTP1_WDATA_otp1_pdin_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdob0_w : 8;
    } reg;
} PMIC_OTP1_0_W_UNION;
#endif
#define PMIC_OTP1_0_W_otp1_pdob0_w_START (0)
#define PMIC_OTP1_0_W_otp1_pdob0_w_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdob1_w : 8;
    } reg;
} PMIC_OTP1_1_W_UNION;
#endif
#define PMIC_OTP1_1_W_otp1_pdob1_w_START (0)
#define PMIC_OTP1_1_W_otp1_pdob1_w_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdob2_w : 8;
    } reg;
} PMIC_OTP1_2_W_UNION;
#endif
#define PMIC_OTP1_2_W_otp1_pdob2_w_START (0)
#define PMIC_OTP1_2_W_otp1_pdob2_w_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char otp1_pdob3_w : 8;
    } reg;
} PMIC_OTP1_3_W_UNION;
#endif
#define PMIC_OTP1_3_W_otp1_pdob3_w_START (0)
#define PMIC_OTP1_3_W_otp1_pdob3_w_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hrst_reg0 : 8;
    } reg;
} PMIC_HRST_REG0_UNION;
#endif
#define PMIC_HRST_REG0_hrst_reg0_START (0)
#define PMIC_HRST_REG0_hrst_reg0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hrst_reg1 : 8;
    } reg;
} PMIC_HRST_REG1_UNION;
#endif
#define PMIC_HRST_REG1_hrst_reg1_START (0)
#define PMIC_HRST_REG1_hrst_reg1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hrst_reg2 : 8;
    } reg;
} PMIC_HRST_REG2_UNION;
#endif
#define PMIC_HRST_REG2_hrst_reg2_START (0)
#define PMIC_HRST_REG2_hrst_reg2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_rst_cfg : 8;
    } reg;
} PMIC_SOFT_RST_REG_UNION;
#endif
#define PMIC_SOFT_RST_REG_soft_rst_cfg_START (0)
#define PMIC_SOFT_RST_REG_soft_rst_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_reg_otp : 2;
        unsigned char xo_ldo_curr : 2;
        unsigned char xo_ldo_res : 3;
        unsigned char xo_ldo_reg_res : 1;
    } reg;
} PMIC_CLK_TOP_CTRL0_UNION;
#endif
#define PMIC_CLK_TOP_CTRL0_xo_reg_otp_START (0)
#define PMIC_CLK_TOP_CTRL0_xo_reg_otp_END (1)
#define PMIC_CLK_TOP_CTRL0_xo_ldo_curr_START (2)
#define PMIC_CLK_TOP_CTRL0_xo_ldo_curr_END (3)
#define PMIC_CLK_TOP_CTRL0_xo_ldo_res_START (4)
#define PMIC_CLK_TOP_CTRL0_xo_ldo_res_END (6)
#define PMIC_CLK_TOP_CTRL0_xo_ldo_reg_res_START (7)
#define PMIC_CLK_TOP_CTRL0_xo_ldo_reg_res_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_xo_c1fix : 8;
    } reg;
} PMIC_CLK_TOP_CTRL1_UNION;
#endif
#define PMIC_CLK_TOP_CTRL1_np_xo_c1fix_START (0)
#define PMIC_CLK_TOP_CTRL1_np_xo_c1fix_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_xo_c2fix : 4;
        unsigned char xo_buff_reg_curr : 1;
        unsigned char xo_otp_reg_sel1 : 1;
        unsigned char xo_otp_reg_sel0 : 1;
        unsigned char xo_c1_cap : 1;
    } reg;
} PMIC_CLK_TOP_CTRL2_UNION;
#endif
#define PMIC_CLK_TOP_CTRL2_np_xo_c2fix_START (0)
#define PMIC_CLK_TOP_CTRL2_np_xo_c2fix_END (3)
#define PMIC_CLK_TOP_CTRL2_xo_buff_reg_curr_START (4)
#define PMIC_CLK_TOP_CTRL2_xo_buff_reg_curr_END (4)
#define PMIC_CLK_TOP_CTRL2_xo_otp_reg_sel1_START (5)
#define PMIC_CLK_TOP_CTRL2_xo_otp_reg_sel1_END (5)
#define PMIC_CLK_TOP_CTRL2_xo_otp_reg_sel0_START (6)
#define PMIC_CLK_TOP_CTRL2_xo_otp_reg_sel0_END (6)
#define PMIC_CLK_TOP_CTRL2_xo_c1_cap_START (7)
#define PMIC_CLK_TOP_CTRL2_xo_c1_cap_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_freq_rf1 : 2;
        unsigned char xo_freq_rf0 : 2;
        unsigned char xo_sys_phase : 1;
        unsigned char xo_tri_cap : 3;
    } reg;
} PMIC_CLK_TOP_CTRL3_UNION;
#endif
#define PMIC_CLK_TOP_CTRL3_xo_freq_rf1_START (0)
#define PMIC_CLK_TOP_CTRL3_xo_freq_rf1_END (1)
#define PMIC_CLK_TOP_CTRL3_xo_freq_rf0_START (2)
#define PMIC_CLK_TOP_CTRL3_xo_freq_rf0_END (3)
#define PMIC_CLK_TOP_CTRL3_xo_sys_phase_START (4)
#define PMIC_CLK_TOP_CTRL3_xo_sys_phase_END (4)
#define PMIC_CLK_TOP_CTRL3_xo_tri_cap_START (5)
#define PMIC_CLK_TOP_CTRL3_xo_tri_cap_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_freq_codec : 2;
        unsigned char xo_freq_sys : 2;
        unsigned char xo_freq_wifibt : 2;
        unsigned char xo_freq_abb : 2;
    } reg;
} PMIC_CLK_TOP_CTRL4_UNION;
#endif
#define PMIC_CLK_TOP_CTRL4_xo_freq_codec_START (0)
#define PMIC_CLK_TOP_CTRL4_xo_freq_codec_END (1)
#define PMIC_CLK_TOP_CTRL4_xo_freq_sys_START (2)
#define PMIC_CLK_TOP_CTRL4_xo_freq_sys_END (3)
#define PMIC_CLK_TOP_CTRL4_xo_freq_wifibt_START (4)
#define PMIC_CLK_TOP_CTRL4_xo_freq_wifibt_END (5)
#define PMIC_CLK_TOP_CTRL4_xo_freq_abb_START (6)
#define PMIC_CLK_TOP_CTRL4_xo_freq_abb_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_xo_codec_en : 1;
        unsigned char reg_xo_sys_en : 1;
        unsigned char reg_xo_wifi_en : 1;
        unsigned char reg_xo_abb_en : 1;
        unsigned char xo_rf1_en : 1;
        unsigned char xo_rf0_en : 1;
        unsigned char reserved : 2;
    } reg;
} PMIC_CLK_TOP_CTRL5_UNION;
#endif
#define PMIC_CLK_TOP_CTRL5_reg_xo_codec_en_START (0)
#define PMIC_CLK_TOP_CTRL5_reg_xo_codec_en_END (0)
#define PMIC_CLK_TOP_CTRL5_reg_xo_sys_en_START (1)
#define PMIC_CLK_TOP_CTRL5_reg_xo_sys_en_END (1)
#define PMIC_CLK_TOP_CTRL5_reg_xo_wifi_en_START (2)
#define PMIC_CLK_TOP_CTRL5_reg_xo_wifi_en_END (2)
#define PMIC_CLK_TOP_CTRL5_reg_xo_abb_en_START (3)
#define PMIC_CLK_TOP_CTRL5_reg_xo_abb_en_END (3)
#define PMIC_CLK_TOP_CTRL5_xo_rf1_en_START (4)
#define PMIC_CLK_TOP_CTRL5_xo_rf1_en_END (4)
#define PMIC_CLK_TOP_CTRL5_xo_rf0_en_START (5)
#define PMIC_CLK_TOP_CTRL5_xo_rf0_en_END (5)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_eco_in_n : 1;
        unsigned char reg_xo_ldo_en : 1;
        unsigned char reg_xo_core_en : 1;
        unsigned char reg_rc_debug : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_CLK_TOP_CTRL6_UNION;
#endif
#define PMIC_CLK_TOP_CTRL6_reg_eco_in_n_START (0)
#define PMIC_CLK_TOP_CTRL6_reg_eco_in_n_END (0)
#define PMIC_CLK_TOP_CTRL6_reg_xo_ldo_en_START (1)
#define PMIC_CLK_TOP_CTRL6_reg_xo_ldo_en_END (1)
#define PMIC_CLK_TOP_CTRL6_reg_xo_core_en_START (2)
#define PMIC_CLK_TOP_CTRL6_reg_xo_core_en_END (2)
#define PMIC_CLK_TOP_CTRL6_reg_rc_debug_START (3)
#define PMIC_CLK_TOP_CTRL6_reg_rc_debug_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_xo_wifibt_dig : 1;
        unsigned char np_xo_abb_dig : 1;
        unsigned char np_xo_rf1_dig : 1;
        unsigned char np_xo_rf0_dig : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_CLK_TOP_CTRL7_UNION;
#endif
#define PMIC_CLK_TOP_CTRL7_np_xo_wifibt_dig_START (0)
#define PMIC_CLK_TOP_CTRL7_np_xo_wifibt_dig_END (0)
#define PMIC_CLK_TOP_CTRL7_np_xo_abb_dig_START (1)
#define PMIC_CLK_TOP_CTRL7_np_xo_abb_dig_END (1)
#define PMIC_CLK_TOP_CTRL7_np_xo_rf1_dig_START (2)
#define PMIC_CLK_TOP_CTRL7_np_xo_rf1_dig_END (2)
#define PMIC_CLK_TOP_CTRL7_np_xo_rf0_dig_START (3)
#define PMIC_CLK_TOP_CTRL7_np_xo_rf0_dig_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_256k_en0 : 8;
    } reg;
} PMIC_CLK_256K_CTRL0_UNION;
#endif
#define PMIC_CLK_256K_CTRL0_reg_256k_en0_START (0)
#define PMIC_CLK_256K_CTRL0_reg_256k_en0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reg_256k_en1 : 8;
    } reg;
} PMIC_CLK_256K_CTRL1_UNION;
#endif
#define PMIC_CLK_256K_CTRL1_reg_256k_en1_START (0)
#define PMIC_CLK_256K_CTRL1_reg_256k_en1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_reserve0 : 8;
    } reg;
} PMIC_CLK_TOP_RESERVE0_UNION;
#endif
#define PMIC_CLK_TOP_RESERVE0_xo_reserve0_START (0)
#define PMIC_CLK_TOP_RESERVE0_xo_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_reserve1 : 8;
    } reg;
} PMIC_CLK_TOP_RESERVE1_UNION;
#endif
#define PMIC_CLK_TOP_RESERVE1_xo_reserve1_START (0)
#define PMIC_CLK_TOP_RESERVE1_xo_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_debug0 : 8;
    } reg;
} PMIC_SYS_DEBUG0_UNION;
#endif
#define PMIC_SYS_DEBUG0_sys_debug0_START (0)
#define PMIC_SYS_DEBUG0_sys_debug0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sys_debug1 : 8;
    } reg;
} PMIC_SYS_DEBUG1_UNION;
#endif
#define PMIC_SYS_DEBUG1_sys_debug1_START (0)
#define PMIC_SYS_DEBUG1_sys_debug1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dac0_din_msb : 2;
        unsigned char reserved : 6;
    } reg;
} PMIC_DAC0_DIN_MSB_UNION;
#endif
#define PMIC_DAC0_DIN_MSB_dac0_din_msb_START (0)
#define PMIC_DAC0_DIN_MSB_dac0_din_msb_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dac0_din_lsb : 8;
    } reg;
} PMIC_DAC0_DIN_LSB_UNION;
#endif
#define PMIC_DAC0_DIN_LSB_dac0_din_lsb_START (0)
#define PMIC_DAC0_DIN_LSB_dac0_din_lsb_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dac1_din_msb : 2;
        unsigned char reserved : 6;
    } reg;
} PMIC_DAC1_DIN_MSB_UNION;
#endif
#define PMIC_DAC1_DIN_MSB_dac1_din_msb_START (0)
#define PMIC_DAC1_DIN_MSB_dac1_din_msb_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char dac1_din_lsb : 8;
    } reg;
} PMIC_DAC1_DIN_LSB_UNION;
#endif
#define PMIC_DAC1_DIN_LSB_dac1_din_lsb_START (0)
#define PMIC_DAC1_DIN_LSB_dac1_din_lsb_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char core_ldo_eco_lock : 8;
    } reg;
} PMIC_CORE_LDO_ECO_LOCK_UNION;
#endif
#define PMIC_CORE_LDO_ECO_LOCK_core_ldo_eco_lock_START (0)
#define PMIC_CORE_LDO_ECO_LOCK_core_ldo_eco_lock_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_ocpldo2 : 1;
        unsigned char np_ocpldo1 : 1;
        unsigned char np_ocpldo0_2 : 1;
        unsigned char np_ocpbuck4 : 1;
        unsigned char np_ocpbuck3 : 1;
        unsigned char np_ocpbuck2 : 1;
        unsigned char np_ocpbuck1 : 1;
        unsigned char np_ocpbuck0 : 1;
    } reg;
} PMIC_NP_OCP0_UNION;
#endif
#define PMIC_NP_OCP0_np_ocpldo2_START (0)
#define PMIC_NP_OCP0_np_ocpldo2_END (0)
#define PMIC_NP_OCP0_np_ocpldo1_START (1)
#define PMIC_NP_OCP0_np_ocpldo1_END (1)
#define PMIC_NP_OCP0_np_ocpldo0_2_START (2)
#define PMIC_NP_OCP0_np_ocpldo0_2_END (2)
#define PMIC_NP_OCP0_np_ocpbuck4_START (3)
#define PMIC_NP_OCP0_np_ocpbuck4_END (3)
#define PMIC_NP_OCP0_np_ocpbuck3_START (4)
#define PMIC_NP_OCP0_np_ocpbuck3_END (4)
#define PMIC_NP_OCP0_np_ocpbuck2_START (5)
#define PMIC_NP_OCP0_np_ocpbuck2_END (5)
#define PMIC_NP_OCP0_np_ocpbuck1_START (6)
#define PMIC_NP_OCP0_np_ocpbuck1_END (6)
#define PMIC_NP_OCP0_np_ocpbuck0_START (7)
#define PMIC_NP_OCP0_np_ocpbuck0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_ocpldo11 : 1;
        unsigned char np_ocpldo10 : 1;
        unsigned char np_ocpldo9 : 1;
        unsigned char np_ocpldo8 : 1;
        unsigned char np_ocpldo7 : 1;
        unsigned char np_ocpldo5 : 1;
        unsigned char np_ocpldo4 : 1;
        unsigned char np_ocpldo3 : 1;
    } reg;
} PMIC_NP_OCP1_UNION;
#endif
#define PMIC_NP_OCP1_np_ocpldo11_START (0)
#define PMIC_NP_OCP1_np_ocpldo11_END (0)
#define PMIC_NP_OCP1_np_ocpldo10_START (1)
#define PMIC_NP_OCP1_np_ocpldo10_END (1)
#define PMIC_NP_OCP1_np_ocpldo9_START (2)
#define PMIC_NP_OCP1_np_ocpldo9_END (2)
#define PMIC_NP_OCP1_np_ocpldo8_START (3)
#define PMIC_NP_OCP1_np_ocpldo8_END (3)
#define PMIC_NP_OCP1_np_ocpldo7_START (4)
#define PMIC_NP_OCP1_np_ocpldo7_END (4)
#define PMIC_NP_OCP1_np_ocpldo5_START (5)
#define PMIC_NP_OCP1_np_ocpldo5_END (5)
#define PMIC_NP_OCP1_np_ocpldo4_START (6)
#define PMIC_NP_OCP1_np_ocpldo4_END (6)
#define PMIC_NP_OCP1_np_ocpldo3_START (7)
#define PMIC_NP_OCP1_np_ocpldo3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_ocpldo20 : 1;
        unsigned char np_ocpldo19 : 1;
        unsigned char np_ocpldo17 : 1;
        unsigned char np_ocpldo16 : 1;
        unsigned char np_ocpldo15 : 1;
        unsigned char np_ocpldo14 : 1;
        unsigned char np_ocpldo13 : 1;
        unsigned char np_ocpldo12 : 1;
    } reg;
} PMIC_NP_OCP2_UNION;
#endif
#define PMIC_NP_OCP2_np_ocpldo20_START (0)
#define PMIC_NP_OCP2_np_ocpldo20_END (0)
#define PMIC_NP_OCP2_np_ocpldo19_START (1)
#define PMIC_NP_OCP2_np_ocpldo19_END (1)
#define PMIC_NP_OCP2_np_ocpldo17_START (2)
#define PMIC_NP_OCP2_np_ocpldo17_END (2)
#define PMIC_NP_OCP2_np_ocpldo16_START (3)
#define PMIC_NP_OCP2_np_ocpldo16_END (3)
#define PMIC_NP_OCP2_np_ocpldo15_START (4)
#define PMIC_NP_OCP2_np_ocpldo15_END (4)
#define PMIC_NP_OCP2_np_ocpldo14_START (5)
#define PMIC_NP_OCP2_np_ocpldo14_END (5)
#define PMIC_NP_OCP2_np_ocpldo13_START (6)
#define PMIC_NP_OCP2_np_ocpldo13_END (6)
#define PMIC_NP_OCP2_np_ocpldo12_START (7)
#define PMIC_NP_OCP2_np_ocpldo12_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_ocpldo28 : 1;
        unsigned char np_ocpldo27 : 1;
        unsigned char np_ocpldo26 : 1;
        unsigned char np_ocpldo25 : 1;
        unsigned char np_ocpldo24 : 1;
        unsigned char np_ocpldo23 : 1;
        unsigned char np_ocpldo22 : 1;
        unsigned char np_ocpldo21 : 1;
    } reg;
} PMIC_NP_OCP3_UNION;
#endif
#define PMIC_NP_OCP3_np_ocpldo28_START (0)
#define PMIC_NP_OCP3_np_ocpldo28_END (0)
#define PMIC_NP_OCP3_np_ocpldo27_START (1)
#define PMIC_NP_OCP3_np_ocpldo27_END (1)
#define PMIC_NP_OCP3_np_ocpldo26_START (2)
#define PMIC_NP_OCP3_np_ocpldo26_END (2)
#define PMIC_NP_OCP3_np_ocpldo25_START (3)
#define PMIC_NP_OCP3_np_ocpldo25_END (3)
#define PMIC_NP_OCP3_np_ocpldo24_START (4)
#define PMIC_NP_OCP3_np_ocpldo24_END (4)
#define PMIC_NP_OCP3_np_ocpldo23_START (5)
#define PMIC_NP_OCP3_np_ocpldo23_END (5)
#define PMIC_NP_OCP3_np_ocpldo22_START (6)
#define PMIC_NP_OCP3_np_ocpldo22_END (6)
#define PMIC_NP_OCP3_np_ocpldo21_START (7)
#define PMIC_NP_OCP3_np_ocpldo21_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_classd_ocp : 1;
        unsigned char reserved : 3;
        unsigned char np_ocpldo32 : 1;
        unsigned char np_ocpldo31 : 1;
        unsigned char np_ocpldo30 : 1;
        unsigned char np_ocpldo29 : 1;
    } reg;
} PMIC_NP_OCP4_UNION;
#endif
#define PMIC_NP_OCP4_np_classd_ocp_START (0)
#define PMIC_NP_OCP4_np_classd_ocp_END (0)
#define PMIC_NP_OCP4_np_ocpldo32_START (4)
#define PMIC_NP_OCP4_np_ocpldo32_END (4)
#define PMIC_NP_OCP4_np_ocpldo31_START (5)
#define PMIC_NP_OCP4_np_ocpldo31_END (5)
#define PMIC_NP_OCP4_np_ocpldo30_START (6)
#define PMIC_NP_OCP4_np_ocpldo30_END (6)
#define PMIC_NP_OCP4_np_ocpldo29_START (7)
#define PMIC_NP_OCP4_np_ocpldo29_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_buck4_scp : 1;
        unsigned char np_buck3_scp : 1;
        unsigned char np_buck2_scp : 1;
        unsigned char np_buck1_scp : 1;
        unsigned char np_buck0_scp : 1;
        unsigned char reserved : 3;
    } reg;
} PMIC_NP_SCP_UNION;
#endif
#define PMIC_NP_SCP_np_buck4_scp_START (0)
#define PMIC_NP_SCP_np_buck4_scp_END (0)
#define PMIC_NP_SCP_np_buck3_scp_START (1)
#define PMIC_NP_SCP_np_buck3_scp_END (1)
#define PMIC_NP_SCP_np_buck2_scp_START (2)
#define PMIC_NP_SCP_np_buck2_scp_END (2)
#define PMIC_NP_SCP_np_buck1_scp_START (3)
#define PMIC_NP_SCP_np_buck1_scp_END (3)
#define PMIC_NP_SCP_np_buck0_scp_START (4)
#define PMIC_NP_SCP_np_buck0_scp_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char por_d45mr : 1;
        unsigned char vbus_det_insert_d20mr : 1;
        unsigned char vbus_det_insert_d20mf : 1;
        unsigned char alarmon_r : 1;
        unsigned char pwronn_d6sf : 1;
        unsigned char pwronn_d1sf : 1;
        unsigned char pwronn_d20mr : 1;
        unsigned char pwronn_d20mf : 1;
    } reg;
} PMIC_IRQ0_UNION;
#endif
#define PMIC_IRQ0_por_d45mr_START (0)
#define PMIC_IRQ0_por_d45mr_END (0)
#define PMIC_IRQ0_vbus_det_insert_d20mr_START (1)
#define PMIC_IRQ0_vbus_det_insert_d20mr_END (1)
#define PMIC_IRQ0_vbus_det_insert_d20mf_START (2)
#define PMIC_IRQ0_vbus_det_insert_d20mf_END (2)
#define PMIC_IRQ0_alarmon_r_START (3)
#define PMIC_IRQ0_alarmon_r_END (3)
#define PMIC_IRQ0_pwronn_d6sf_START (4)
#define PMIC_IRQ0_pwronn_d6sf_END (4)
#define PMIC_IRQ0_pwronn_d1sf_START (5)
#define PMIC_IRQ0_pwronn_d1sf_END (5)
#define PMIC_IRQ0_pwronn_d20mr_START (6)
#define PMIC_IRQ0_pwronn_d20mr_END (6)
#define PMIC_IRQ0_pwronn_d20mf_START (7)
#define PMIC_IRQ0_pwronn_d20mf_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ocp_r : 1;
        unsigned char coul_r : 1;
        unsigned char vsys_ov_d200ur : 1;
        unsigned char vsys_uv_d10mr : 1;
        unsigned char vsys_pwroff_abs_2d : 1;
        unsigned char vsys_pwroff_deb_d80mr : 1;
        unsigned char thsd_otmp140_d1mr : 1;
        unsigned char thsd_otmp125_d1mr : 1;
    } reg;
} PMIC_IRQ1_UNION;
#endif
#define PMIC_IRQ1_ocp_r_START (0)
#define PMIC_IRQ1_ocp_r_END (0)
#define PMIC_IRQ1_coul_r_START (1)
#define PMIC_IRQ1_coul_r_END (1)
#define PMIC_IRQ1_vsys_ov_d200ur_START (2)
#define PMIC_IRQ1_vsys_ov_d200ur_END (2)
#define PMIC_IRQ1_vsys_uv_d10mr_START (3)
#define PMIC_IRQ1_vsys_uv_d10mr_END (3)
#define PMIC_IRQ1_vsys_pwroff_abs_2d_START (4)
#define PMIC_IRQ1_vsys_pwroff_abs_2d_END (4)
#define PMIC_IRQ1_vsys_pwroff_deb_d80mr_START (5)
#define PMIC_IRQ1_vsys_pwroff_deb_d80mr_END (5)
#define PMIC_IRQ1_thsd_otmp140_d1mr_START (6)
#define PMIC_IRQ1_thsd_otmp140_d1mr_END (6)
#define PMIC_IRQ1_thsd_otmp125_d1mr_START (7)
#define PMIC_IRQ1_thsd_otmp125_d1mr_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hresetn_d90uf : 1;
        unsigned char reserved : 7;
    } reg;
} PMIC_IRQ2_UNION;
#endif
#define PMIC_IRQ2_hresetn_d90uf_START (0)
#define PMIC_IRQ2_hresetn_d90uf_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char sim0_hpd_r : 1;
        unsigned char sim0_hpd_f : 1;
        unsigned char sim0_hpd_h : 1;
        unsigned char sim0_hpd_l : 1;
        unsigned char sim1_hpd_r : 1;
        unsigned char sim1_hpd_f : 1;
        unsigned char sim1_hpd_h : 1;
        unsigned char sim1_hpd_l : 1;
    } reg;
} PMIC_IRQ3_UNION;
#endif
#define PMIC_IRQ3_sim0_hpd_r_START (0)
#define PMIC_IRQ3_sim0_hpd_r_END (0)
#define PMIC_IRQ3_sim0_hpd_f_START (1)
#define PMIC_IRQ3_sim0_hpd_f_END (1)
#define PMIC_IRQ3_sim0_hpd_h_START (2)
#define PMIC_IRQ3_sim0_hpd_h_END (2)
#define PMIC_IRQ3_sim0_hpd_l_START (3)
#define PMIC_IRQ3_sim0_hpd_l_END (3)
#define PMIC_IRQ3_sim1_hpd_r_START (4)
#define PMIC_IRQ3_sim1_hpd_r_END (4)
#define PMIC_IRQ3_sim1_hpd_f_START (5)
#define PMIC_IRQ3_sim1_hpd_f_END (5)
#define PMIC_IRQ3_sim1_hpd_h_START (6)
#define PMIC_IRQ3_sim1_hpd_h_END (6)
#define PMIC_IRQ3_sim1_hpd_l_START (7)
#define PMIC_IRQ3_sim1_hpd_l_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ocpldo2 : 1;
        unsigned char ocpldo1 : 1;
        unsigned char ocpldo0_2 : 1;
        unsigned char ocpbuck4 : 1;
        unsigned char ocpbuck3 : 1;
        unsigned char ocpbuck2 : 1;
        unsigned char ocpbuck1 : 1;
        unsigned char ocpbuck0 : 1;
    } reg;
} PMIC_OCP_IRQ0_UNION;
#endif
#define PMIC_OCP_IRQ0_ocpldo2_START (0)
#define PMIC_OCP_IRQ0_ocpldo2_END (0)
#define PMIC_OCP_IRQ0_ocpldo1_START (1)
#define PMIC_OCP_IRQ0_ocpldo1_END (1)
#define PMIC_OCP_IRQ0_ocpldo0_2_START (2)
#define PMIC_OCP_IRQ0_ocpldo0_2_END (2)
#define PMIC_OCP_IRQ0_ocpbuck4_START (3)
#define PMIC_OCP_IRQ0_ocpbuck4_END (3)
#define PMIC_OCP_IRQ0_ocpbuck3_START (4)
#define PMIC_OCP_IRQ0_ocpbuck3_END (4)
#define PMIC_OCP_IRQ0_ocpbuck2_START (5)
#define PMIC_OCP_IRQ0_ocpbuck2_END (5)
#define PMIC_OCP_IRQ0_ocpbuck1_START (6)
#define PMIC_OCP_IRQ0_ocpbuck1_END (6)
#define PMIC_OCP_IRQ0_ocpbuck0_START (7)
#define PMIC_OCP_IRQ0_ocpbuck0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ocpldo11 : 1;
        unsigned char ocpldo10 : 1;
        unsigned char ocpldo9 : 1;
        unsigned char ocpldo8 : 1;
        unsigned char ocpldo7 : 1;
        unsigned char ocpldo5 : 1;
        unsigned char ocpldo4 : 1;
        unsigned char ocpldo3 : 1;
    } reg;
} PMIC_OCP_IRQ1_UNION;
#endif
#define PMIC_OCP_IRQ1_ocpldo11_START (0)
#define PMIC_OCP_IRQ1_ocpldo11_END (0)
#define PMIC_OCP_IRQ1_ocpldo10_START (1)
#define PMIC_OCP_IRQ1_ocpldo10_END (1)
#define PMIC_OCP_IRQ1_ocpldo9_START (2)
#define PMIC_OCP_IRQ1_ocpldo9_END (2)
#define PMIC_OCP_IRQ1_ocpldo8_START (3)
#define PMIC_OCP_IRQ1_ocpldo8_END (3)
#define PMIC_OCP_IRQ1_ocpldo7_START (4)
#define PMIC_OCP_IRQ1_ocpldo7_END (4)
#define PMIC_OCP_IRQ1_ocpldo5_START (5)
#define PMIC_OCP_IRQ1_ocpldo5_END (5)
#define PMIC_OCP_IRQ1_ocpldo4_START (6)
#define PMIC_OCP_IRQ1_ocpldo4_END (6)
#define PMIC_OCP_IRQ1_ocpldo3_START (7)
#define PMIC_OCP_IRQ1_ocpldo3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ocpldo20 : 1;
        unsigned char ocpldo19 : 1;
        unsigned char ocpldo17 : 1;
        unsigned char ocpldo16 : 1;
        unsigned char ocpldo15 : 1;
        unsigned char ocpldo14 : 1;
        unsigned char ocpldo13 : 1;
        unsigned char ocpldo12 : 1;
    } reg;
} PMIC_OCP_IRQ2_UNION;
#endif
#define PMIC_OCP_IRQ2_ocpldo20_START (0)
#define PMIC_OCP_IRQ2_ocpldo20_END (0)
#define PMIC_OCP_IRQ2_ocpldo19_START (1)
#define PMIC_OCP_IRQ2_ocpldo19_END (1)
#define PMIC_OCP_IRQ2_ocpldo17_START (2)
#define PMIC_OCP_IRQ2_ocpldo17_END (2)
#define PMIC_OCP_IRQ2_ocpldo16_START (3)
#define PMIC_OCP_IRQ2_ocpldo16_END (3)
#define PMIC_OCP_IRQ2_ocpldo15_START (4)
#define PMIC_OCP_IRQ2_ocpldo15_END (4)
#define PMIC_OCP_IRQ2_ocpldo14_START (5)
#define PMIC_OCP_IRQ2_ocpldo14_END (5)
#define PMIC_OCP_IRQ2_ocpldo13_START (6)
#define PMIC_OCP_IRQ2_ocpldo13_END (6)
#define PMIC_OCP_IRQ2_ocpldo12_START (7)
#define PMIC_OCP_IRQ2_ocpldo12_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char ocpldo28 : 1;
        unsigned char ocpldo27 : 1;
        unsigned char ocpldo26 : 1;
        unsigned char ocpldo25 : 1;
        unsigned char ocpldo24 : 1;
        unsigned char ocpldo23 : 1;
        unsigned char ocpldo22 : 1;
        unsigned char ocpldo21 : 1;
    } reg;
} PMIC_OCP_IRQ3_UNION;
#endif
#define PMIC_OCP_IRQ3_ocpldo28_START (0)
#define PMIC_OCP_IRQ3_ocpldo28_END (0)
#define PMIC_OCP_IRQ3_ocpldo27_START (1)
#define PMIC_OCP_IRQ3_ocpldo27_END (1)
#define PMIC_OCP_IRQ3_ocpldo26_START (2)
#define PMIC_OCP_IRQ3_ocpldo26_END (2)
#define PMIC_OCP_IRQ3_ocpldo25_START (3)
#define PMIC_OCP_IRQ3_ocpldo25_END (3)
#define PMIC_OCP_IRQ3_ocpldo24_START (4)
#define PMIC_OCP_IRQ3_ocpldo24_END (4)
#define PMIC_OCP_IRQ3_ocpldo23_START (5)
#define PMIC_OCP_IRQ3_ocpldo23_END (5)
#define PMIC_OCP_IRQ3_ocpldo22_START (6)
#define PMIC_OCP_IRQ3_ocpldo22_END (6)
#define PMIC_OCP_IRQ3_ocpldo21_START (7)
#define PMIC_OCP_IRQ3_ocpldo21_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char classd_ocp : 1;
        unsigned char reserved : 3;
        unsigned char ocpldo32 : 1;
        unsigned char ocpldo31 : 1;
        unsigned char ocpldo30 : 1;
        unsigned char ocpldo29 : 1;
    } reg;
} PMIC_OCP_IRQ4_UNION;
#endif
#define PMIC_OCP_IRQ4_classd_ocp_START (0)
#define PMIC_OCP_IRQ4_classd_ocp_END (0)
#define PMIC_OCP_IRQ4_ocpldo32_START (4)
#define PMIC_OCP_IRQ4_ocpldo32_END (4)
#define PMIC_OCP_IRQ4_ocpldo31_START (5)
#define PMIC_OCP_IRQ4_ocpldo31_END (5)
#define PMIC_OCP_IRQ4_ocpldo30_START (6)
#define PMIC_OCP_IRQ4_ocpldo30_END (6)
#define PMIC_OCP_IRQ4_ocpldo29_START (7)
#define PMIC_OCP_IRQ4_ocpldo29_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char buck4_scp : 1;
        unsigned char buck3_scp : 1;
        unsigned char buck2_scp : 1;
        unsigned char buck1_scp : 1;
        unsigned char buck0_scp : 1;
        unsigned char reserved : 3;
    } reg;
} PMIC_OCP_IRQ5_UNION;
#endif
#define PMIC_OCP_IRQ5_buck4_scp_START (0)
#define PMIC_OCP_IRQ5_buck4_scp_END (0)
#define PMIC_OCP_IRQ5_buck3_scp_START (1)
#define PMIC_OCP_IRQ5_buck3_scp_END (1)
#define PMIC_OCP_IRQ5_buck2_scp_START (2)
#define PMIC_OCP_IRQ5_buck2_scp_END (2)
#define PMIC_OCP_IRQ5_buck1_scp_START (3)
#define PMIC_OCP_IRQ5_buck1_scp_END (3)
#define PMIC_OCP_IRQ5_buck0_scp_START (4)
#define PMIC_OCP_IRQ5_buck0_scp_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_pwronn_restart : 1;
        unsigned char np_pwrhold_shutdown : 1;
        unsigned char np_pwronn_shutdown : 1;
        unsigned char np_pwrhold_pwrup : 1;
        unsigned char np_alarmon_pwrup : 1;
        unsigned char np_vbus_pwrup : 1;
        unsigned char np_pwronn_pwrup : 1;
        unsigned char np_fast_pwrup : 1;
    } reg;
} PMIC_NP_RECORD0_UNION;
#endif
#define PMIC_NP_RECORD0_np_pwronn_restart_START (0)
#define PMIC_NP_RECORD0_np_pwronn_restart_END (0)
#define PMIC_NP_RECORD0_np_pwrhold_shutdown_START (1)
#define PMIC_NP_RECORD0_np_pwrhold_shutdown_END (1)
#define PMIC_NP_RECORD0_np_pwronn_shutdown_START (2)
#define PMIC_NP_RECORD0_np_pwronn_shutdown_END (2)
#define PMIC_NP_RECORD0_np_pwrhold_pwrup_START (3)
#define PMIC_NP_RECORD0_np_pwrhold_pwrup_END (3)
#define PMIC_NP_RECORD0_np_alarmon_pwrup_START (4)
#define PMIC_NP_RECORD0_np_alarmon_pwrup_END (4)
#define PMIC_NP_RECORD0_np_vbus_pwrup_START (5)
#define PMIC_NP_RECORD0_np_vbus_pwrup_END (5)
#define PMIC_NP_RECORD0_np_pwronn_pwrup_START (6)
#define PMIC_NP_RECORD0_np_pwronn_pwrup_END (6)
#define PMIC_NP_RECORD0_np_fast_pwrup_START (7)
#define PMIC_NP_RECORD0_np_fast_pwrup_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char np_tcxo_clk_sel_r : 1;
        unsigned char np_dcxo_clk_sel_r : 1;
        unsigned char np_dcxo_clk_sel_f : 1;
        unsigned char np_vsys_vcoin_sel : 1;
        unsigned char np_smpl : 1;
        unsigned char np_core_io_vld_f : 1;
        unsigned char reserved : 1;
        unsigned char np_pwrhold_1s : 1;
    } reg;
} PMIC_NP_RECORD1_UNION;
#endif
#define PMIC_NP_RECORD1_np_tcxo_clk_sel_r_START (0)
#define PMIC_NP_RECORD1_np_tcxo_clk_sel_r_END (0)
#define PMIC_NP_RECORD1_np_dcxo_clk_sel_r_START (1)
#define PMIC_NP_RECORD1_np_dcxo_clk_sel_r_END (1)
#define PMIC_NP_RECORD1_np_dcxo_clk_sel_f_START (2)
#define PMIC_NP_RECORD1_np_dcxo_clk_sel_f_END (2)
#define PMIC_NP_RECORD1_np_vsys_vcoin_sel_START (3)
#define PMIC_NP_RECORD1_np_vsys_vcoin_sel_END (3)
#define PMIC_NP_RECORD1_np_smpl_START (4)
#define PMIC_NP_RECORD1_np_smpl_END (4)
#define PMIC_NP_RECORD1_np_core_io_vld_f_START (5)
#define PMIC_NP_RECORD1_np_core_io_vld_f_END (5)
#define PMIC_NP_RECORD1_np_pwrhold_1s_START (7)
#define PMIC_NP_RECORD1_np_pwrhold_1s_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcdr0 : 8;
    } reg;
} PMIC_RTCDR0_UNION;
#endif
#define PMIC_RTCDR0_rtcdr0_START (0)
#define PMIC_RTCDR0_rtcdr0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcdr1 : 8;
    } reg;
} PMIC_RTCDR1_UNION;
#endif
#define PMIC_RTCDR1_rtcdr1_START (0)
#define PMIC_RTCDR1_rtcdr1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcdr2 : 8;
    } reg;
} PMIC_RTCDR2_UNION;
#endif
#define PMIC_RTCDR2_rtcdr2_START (0)
#define PMIC_RTCDR2_rtcdr2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcdr3 : 8;
    } reg;
} PMIC_RTCDR3_UNION;
#endif
#define PMIC_RTCDR3_rtcdr3_START (0)
#define PMIC_RTCDR3_rtcdr3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcmr0 : 8;
    } reg;
} PMIC_RTCMR0_UNION;
#endif
#define PMIC_RTCMR0_rtcmr0_START (0)
#define PMIC_RTCMR0_rtcmr0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcmr1 : 8;
    } reg;
} PMIC_RTCMR1_UNION;
#endif
#define PMIC_RTCMR1_rtcmr1_START (0)
#define PMIC_RTCMR1_rtcmr1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcmr2 : 8;
    } reg;
} PMIC_RTCMR2_UNION;
#endif
#define PMIC_RTCMR2_rtcmr2_START (0)
#define PMIC_RTCMR2_rtcmr2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcmr3 : 8;
    } reg;
} PMIC_RTCMR3_UNION;
#endif
#define PMIC_RTCMR3_rtcmr3_START (0)
#define PMIC_RTCMR3_rtcmr3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcclr0 : 8;
    } reg;
} PMIC_RTCLR0_UNION;
#endif
#define PMIC_RTCLR0_rtcclr0_START (0)
#define PMIC_RTCLR0_rtcclr0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcclr1 : 8;
    } reg;
} PMIC_RTCLR1_UNION;
#endif
#define PMIC_RTCLR1_rtcclr1_START (0)
#define PMIC_RTCLR1_rtcclr1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcclr2 : 8;
    } reg;
} PMIC_RTCLR2_UNION;
#endif
#define PMIC_RTCLR2_rtcclr2_START (0)
#define PMIC_RTCLR2_rtcclr2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtcclr3 : 8;
    } reg;
} PMIC_RTCLR3_UNION;
#endif
#define PMIC_RTCLR3_rtcclr3_START (0)
#define PMIC_RTCLR3_rtcclr3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtccr : 1;
        unsigned char ref_res_sel_int : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_RTCCTRL_UNION;
#endif
#define PMIC_RTCCTRL_rtccr_START (0)
#define PMIC_RTCCTRL_rtccr_END (0)
#define PMIC_RTCCTRL_ref_res_sel_int_START (1)
#define PMIC_RTCCTRL_ref_res_sel_int_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_cali_thresold_low : 8;
    } reg;
} PMIC_XO_THRESOLD0_UNION;
#endif
#define PMIC_XO_THRESOLD0_xo_cali_thresold_low_START (0)
#define PMIC_XO_THRESOLD0_xo_cali_thresold_low_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char xo_cali_thresold1_high : 8;
    } reg;
} PMIC_XO_THRESOLD1_UNION;
#endif
#define PMIC_XO_THRESOLD1_xo_cali_thresold1_high_START (0)
#define PMIC_XO_THRESOLD1_xo_cali_thresold1_high_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char crc_value0 : 8;
    } reg;
} PMIC_CRC_VAULE0_UNION;
#endif
#define PMIC_CRC_VAULE0_crc_value0_START (0)
#define PMIC_CRC_VAULE0_crc_value0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char crc_value1 : 8;
    } reg;
} PMIC_CRC_VAULE1_UNION;
#endif
#define PMIC_CRC_VAULE1_crc_value1_START (0)
#define PMIC_CRC_VAULE1_crc_value1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char crc_value2 : 5;
        unsigned char reserved : 3;
    } reg;
} PMIC_CRC_VAULE2_UNION;
#endif
#define PMIC_CRC_VAULE2_crc_value2_START (0)
#define PMIC_CRC_VAULE2_crc_value2_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtc_pwrup_timer0 : 8;
    } reg;
} PMIC_RTC_PWRUP_TIMER0_UNION;
#endif
#define PMIC_RTC_PWRUP_TIMER0_rtc_pwrup_timer0_START (0)
#define PMIC_RTC_PWRUP_TIMER0_rtc_pwrup_timer0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtc_pwrup_timer1 : 8;
    } reg;
} PMIC_RTC_PWRUP_TIMER1_UNION;
#endif
#define PMIC_RTC_PWRUP_TIMER1_rtc_pwrup_timer1_START (0)
#define PMIC_RTC_PWRUP_TIMER1_rtc_pwrup_timer1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtc_pwrup_timer2 : 8;
    } reg;
} PMIC_RTC_PWRUP_TIMER2_UNION;
#endif
#define PMIC_RTC_PWRUP_TIMER2_rtc_pwrup_timer2_START (0)
#define PMIC_RTC_PWRUP_TIMER2_rtc_pwrup_timer2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtc_pwrup_timer3 : 8;
    } reg;
} PMIC_RTC_PWRUP_TIMER3_UNION;
#endif
#define PMIC_RTC_PWRUP_TIMER3_rtc_pwrup_timer3_START (0)
#define PMIC_RTC_PWRUP_TIMER3_rtc_pwrup_timer3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtc_pwrdown_timer0 : 8;
    } reg;
} PMIC_RTC_PWRDOWN_TIMER0_UNION;
#endif
#define PMIC_RTC_PWRDOWN_TIMER0_rtc_pwrdown_timer0_START (0)
#define PMIC_RTC_PWRDOWN_TIMER0_rtc_pwrdown_timer0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtc_pwrdown_timer1 : 8;
    } reg;
} PMIC_RTC_PWRDOWN_TIMER1_UNION;
#endif
#define PMIC_RTC_PWRDOWN_TIMER1_rtc_pwrdown_timer1_START (0)
#define PMIC_RTC_PWRDOWN_TIMER1_rtc_pwrdown_timer1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtc_pwrdown_timer2 : 8;
    } reg;
} PMIC_RTC_PWRDOWN_TIMER2_UNION;
#endif
#define PMIC_RTC_PWRDOWN_TIMER2_rtc_pwrdown_timer2_START (0)
#define PMIC_RTC_PWRDOWN_TIMER2_rtc_pwrdown_timer2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char rtc_pwrdown_timer3 : 8;
    } reg;
} PMIC_RTC_PWRDOWN_TIMER3_UNION;
#endif
#define PMIC_RTC_PWRDOWN_TIMER3_rtc_pwrdown_timer3_START (0)
#define PMIC_RTC_PWRDOWN_TIMER3_rtc_pwrdown_timer3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_int_i : 1;
        unsigned char cl_out_i : 1;
        unsigned char cl_in_i : 1;
        unsigned char vbat_int_i : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_COUL_IRQ_UNION;
#endif
#define PMIC_COUL_IRQ_cl_int_i_START (0)
#define PMIC_COUL_IRQ_cl_int_i_END (0)
#define PMIC_COUL_IRQ_cl_out_i_START (1)
#define PMIC_COUL_IRQ_cl_out_i_END (1)
#define PMIC_COUL_IRQ_cl_in_i_START (2)
#define PMIC_COUL_IRQ_cl_in_i_END (2)
#define PMIC_COUL_IRQ_vbat_int_i_START (3)
#define PMIC_COUL_IRQ_vbat_int_i_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_int_i_mk : 1;
        unsigned char cl_out_i_mk : 1;
        unsigned char cl_in_i_mk : 1;
        unsigned char vbat_int_i_mk : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_COUL_IRQ_MASK_UNION;
#endif
#define PMIC_COUL_IRQ_MASK_cl_int_i_mk_START (0)
#define PMIC_COUL_IRQ_MASK_cl_int_i_mk_END (0)
#define PMIC_COUL_IRQ_MASK_cl_out_i_mk_START (1)
#define PMIC_COUL_IRQ_MASK_cl_out_i_mk_END (1)
#define PMIC_COUL_IRQ_MASK_cl_in_i_mk_START (2)
#define PMIC_COUL_IRQ_MASK_cl_in_i_mk_END (2)
#define PMIC_COUL_IRQ_MASK_vbat_int_i_mk_START (3)
#define PMIC_COUL_IRQ_MASK_vbat_int_i_mk_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current_0 : 8;
    } reg;
} PMIC_CURRENT_0_UNION;
#endif
#define PMIC_CURRENT_0_current_0_START (0)
#define PMIC_CURRENT_0_current_0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current_1 : 8;
    } reg;
} PMIC_CURRENT_1_UNION;
#endif
#define PMIC_CURRENT_1_current_1_START (0)
#define PMIC_CURRENT_1_current_1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out_0 : 8;
    } reg;
} PMIC_V_OUT_0_UNION;
#endif
#define PMIC_V_OUT_0_v_out_0_START (0)
#define PMIC_V_OUT_0_v_out_0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out_1 : 8;
    } reg;
} PMIC_V_OUT_1_UNION;
#endif
#define PMIC_V_OUT_1_v_out_1_START (0)
#define PMIC_V_OUT_1_v_out_1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_ctrl : 3;
        unsigned char reflash_value_ctrl : 1;
        unsigned char eco_filter_time : 2;
        unsigned char calibration_ctrl : 1;
        unsigned char coul_ctrl_onoff_reg : 1;
    } reg;
} PMIC_CLJ_CTRL_REG_UNION;
#endif
#define PMIC_CLJ_CTRL_REG_eco_ctrl_START (0)
#define PMIC_CLJ_CTRL_REG_eco_ctrl_END (2)
#define PMIC_CLJ_CTRL_REG_reflash_value_ctrl_START (3)
#define PMIC_CLJ_CTRL_REG_reflash_value_ctrl_END (3)
#define PMIC_CLJ_CTRL_REG_eco_filter_time_START (4)
#define PMIC_CLJ_CTRL_REG_eco_filter_time_END (5)
#define PMIC_CLJ_CTRL_REG_calibration_ctrl_START (6)
#define PMIC_CLJ_CTRL_REG_calibration_ctrl_END (6)
#define PMIC_CLJ_CTRL_REG_coul_ctrl_onoff_reg_START (7)
#define PMIC_CLJ_CTRL_REG_coul_ctrl_onoff_reg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_reflash_time : 8;
    } reg;
} PMIC_ECO_REFALSH_TIME_UNION;
#endif
#define PMIC_ECO_REFALSH_TIME_eco_reflash_time_START (0)
#define PMIC_ECO_REFALSH_TIME_eco_reflash_time_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_out0 : 8;
    } reg;
} PMIC_CL_OUT0_UNION;
#endif
#define PMIC_CL_OUT0_cl_out0_START (0)
#define PMIC_CL_OUT0_cl_out0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_out1 : 8;
    } reg;
} PMIC_CL_OUT1_UNION;
#endif
#define PMIC_CL_OUT1_cl_out1_START (0)
#define PMIC_CL_OUT1_cl_out1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_out2 : 8;
    } reg;
} PMIC_CL_OUT2_UNION;
#endif
#define PMIC_CL_OUT2_cl_out2_START (0)
#define PMIC_CL_OUT2_cl_out2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_out3 : 8;
    } reg;
} PMIC_CL_OUT3_UNION;
#endif
#define PMIC_CL_OUT3_cl_out3_START (0)
#define PMIC_CL_OUT3_cl_out3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_in0 : 8;
    } reg;
} PMIC_CL_IN0_UNION;
#endif
#define PMIC_CL_IN0_cl_in0_START (0)
#define PMIC_CL_IN0_cl_in0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_in1 : 8;
    } reg;
} PMIC_CL_IN1_UNION;
#endif
#define PMIC_CL_IN1_cl_in1_START (0)
#define PMIC_CL_IN1_cl_in1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_in2 : 8;
    } reg;
} PMIC_CL_IN2_UNION;
#endif
#define PMIC_CL_IN2_cl_in2_START (0)
#define PMIC_CL_IN2_cl_in2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_in3 : 8;
    } reg;
} PMIC_CL_IN3_UNION;
#endif
#define PMIC_CL_IN3_cl_in3_START (0)
#define PMIC_CL_IN3_cl_in3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_timer0 : 8;
    } reg;
} PMIC_CHG_TIMER0_UNION;
#endif
#define PMIC_CHG_TIMER0_chg_timer0_START (0)
#define PMIC_CHG_TIMER0_chg_timer0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_timer1 : 8;
    } reg;
} PMIC_CHG_TIMER1_UNION;
#endif
#define PMIC_CHG_TIMER1_chg_timer1_START (0)
#define PMIC_CHG_TIMER1_chg_timer1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_timer2 : 8;
    } reg;
} PMIC_CHG_TIMER2_UNION;
#endif
#define PMIC_CHG_TIMER2_chg_timer2_START (0)
#define PMIC_CHG_TIMER2_chg_timer2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char chg_timer3 : 8;
    } reg;
} PMIC_CHG_TIMER3_UNION;
#endif
#define PMIC_CHG_TIMER3_chg_timer3_START (0)
#define PMIC_CHG_TIMER3_chg_timer3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char load_timer0 : 8;
    } reg;
} PMIC_LOAD_TIMER0_UNION;
#endif
#define PMIC_LOAD_TIMER0_load_timer0_START (0)
#define PMIC_LOAD_TIMER0_load_timer0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char load_timer1 : 8;
    } reg;
} PMIC_LOAD_TIMER1_UNION;
#endif
#define PMIC_LOAD_TIMER1_load_timer1_START (0)
#define PMIC_LOAD_TIMER1_load_timer1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char load_timer2 : 8;
    } reg;
} PMIC_LOAD_TIMER2_UNION;
#endif
#define PMIC_LOAD_TIMER2_load_timer2_START (0)
#define PMIC_LOAD_TIMER2_load_timer2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char load_timer3 : 8;
    } reg;
} PMIC_LOAD_TIMER3_UNION;
#endif
#define PMIC_LOAD_TIMER3_load_timer3_START (0)
#define PMIC_LOAD_TIMER3_load_timer3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_int0 : 8;
    } reg;
} PMIC_CL_INT0_UNION;
#endif
#define PMIC_CL_INT0_cl_int0_START (0)
#define PMIC_CL_INT0_cl_int0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_int1 : 8;
    } reg;
} PMIC_CL_INT1_UNION;
#endif
#define PMIC_CL_INT1_cl_int1_START (0)
#define PMIC_CL_INT1_cl_int1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_int2 : 8;
    } reg;
} PMIC_CL_INT2_UNION;
#endif
#define PMIC_CL_INT2_cl_int2_START (0)
#define PMIC_CL_INT2_cl_int2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cl_int3 : 8;
    } reg;
} PMIC_CL_INT3_UNION;
#endif
#define PMIC_CL_INT3_cl_int3_START (0)
#define PMIC_CL_INT3_cl_int3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_int0 : 8;
    } reg;
} PMIC_V_INT0_UNION;
#endif
#define PMIC_V_INT0_v_int0_START (0)
#define PMIC_V_INT0_v_int0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_int1 : 8;
    } reg;
} PMIC_V_INT1_UNION;
#endif
#define PMIC_V_INT1_v_int1_START (0)
#define PMIC_V_INT1_v_int1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char offset_current0 : 8;
    } reg;
} PMIC_OFFSET_CURRENT0_UNION;
#endif
#define PMIC_OFFSET_CURRENT0_offset_current0_START (0)
#define PMIC_OFFSET_CURRENT0_offset_current0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char offset_current1 : 8;
    } reg;
} PMIC_OFFSET_CURRENT1_UNION;
#endif
#define PMIC_OFFSET_CURRENT1_offset_current1_START (0)
#define PMIC_OFFSET_CURRENT1_offset_current1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char offset_voltage0 : 8;
    } reg;
} PMIC_OFFSET_VOLTAGE0_UNION;
#endif
#define PMIC_OFFSET_VOLTAGE0_offset_voltage0_START (0)
#define PMIC_OFFSET_VOLTAGE0_offset_voltage0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char offset_voltage1 : 8;
    } reg;
} PMIC_OFFSET_VOLTAGE1_UNION;
#endif
#define PMIC_OFFSET_VOLTAGE1_offset_voltage1_START (0)
#define PMIC_OFFSET_VOLTAGE1_offset_voltage1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_ocv_data0 : 8;
    } reg;
} PMIC_OCV_VOLTAGE0_UNION;
#endif
#define PMIC_OCV_VOLTAGE0_v_ocv_data0_START (0)
#define PMIC_OCV_VOLTAGE0_v_ocv_data0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_ocv_data1 : 8;
    } reg;
} PMIC_OCV_VOLTAGE1_UNION;
#endif
#define PMIC_OCV_VOLTAGE1_v_ocv_data1_START (0)
#define PMIC_OCV_VOLTAGE1_v_ocv_data1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char i_ocv_data0 : 8;
    } reg;
} PMIC_OCV_CURRENT0_UNION;
#endif
#define PMIC_OCV_CURRENT0_i_ocv_data0_START (0)
#define PMIC_OCV_CURRENT0_i_ocv_data0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char i_ocv_data1 : 8;
    } reg;
} PMIC_OCV_CURRENT1_UNION;
#endif
#define PMIC_OCV_CURRENT1_i_ocv_data1_START (0)
#define PMIC_OCV_CURRENT1_i_ocv_data1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_out_clin0 : 8;
    } reg;
} PMIC_ECO_OUT_CLIN_0_UNION;
#endif
#define PMIC_ECO_OUT_CLIN_0_eco_out_clin0_START (0)
#define PMIC_ECO_OUT_CLIN_0_eco_out_clin0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_out_clin1 : 8;
    } reg;
} PMIC_ECO_OUT_CLIN_1_UNION;
#endif
#define PMIC_ECO_OUT_CLIN_1_eco_out_clin1_START (0)
#define PMIC_ECO_OUT_CLIN_1_eco_out_clin1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_out_clin2 : 8;
    } reg;
} PMIC_ECO_OUT_CLIN_2_UNION;
#endif
#define PMIC_ECO_OUT_CLIN_2_eco_out_clin2_START (0)
#define PMIC_ECO_OUT_CLIN_2_eco_out_clin2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_out_clin3 : 8;
    } reg;
} PMIC_ECO_OUT_CLIN_3_UNION;
#endif
#define PMIC_ECO_OUT_CLIN_3_eco_out_clin3_START (0)
#define PMIC_ECO_OUT_CLIN_3_eco_out_clin3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_out_clout0 : 8;
    } reg;
} PMIC_ECO_OUT_CLOUT_0_UNION;
#endif
#define PMIC_ECO_OUT_CLOUT_0_eco_out_clout0_START (0)
#define PMIC_ECO_OUT_CLOUT_0_eco_out_clout0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_out_clout1 : 8;
    } reg;
} PMIC_ECO_OUT_CLOUT_1_UNION;
#endif
#define PMIC_ECO_OUT_CLOUT_1_eco_out_clout1_START (0)
#define PMIC_ECO_OUT_CLOUT_1_eco_out_clout1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_out_clout2 : 8;
    } reg;
} PMIC_ECO_OUT_CLOUT_2_UNION;
#endif
#define PMIC_ECO_OUT_CLOUT_2_eco_out_clout2_START (0)
#define PMIC_ECO_OUT_CLOUT_2_eco_out_clout2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char eco_out_clout3 : 8;
    } reg;
} PMIC_ECO_OUT_CLOUT_3_UNION;
#endif
#define PMIC_ECO_OUT_CLOUT_3_eco_out_clout3_START (0)
#define PMIC_ECO_OUT_CLOUT_3_eco_out_clout3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre0 : 8;
    } reg;
} PMIC_V_OUT0_PRE0_UNION;
#endif
#define PMIC_V_OUT0_PRE0_v_out0_pre0_START (0)
#define PMIC_V_OUT0_PRE0_v_out0_pre0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre0 : 8;
    } reg;
} PMIC_V_OUT1_PRE0_UNION;
#endif
#define PMIC_V_OUT1_PRE0_v_out1_pre0_START (0)
#define PMIC_V_OUT1_PRE0_v_out1_pre0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre1 : 8;
    } reg;
} PMIC_V_OUT0_PRE1_UNION;
#endif
#define PMIC_V_OUT0_PRE1_v_out0_pre1_START (0)
#define PMIC_V_OUT0_PRE1_v_out0_pre1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre1 : 8;
    } reg;
} PMIC_V_OUT1_PRE1_UNION;
#endif
#define PMIC_V_OUT1_PRE1_v_out1_pre1_START (0)
#define PMIC_V_OUT1_PRE1_v_out1_pre1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre2 : 8;
    } reg;
} PMIC_V_OUT0_PRE2_UNION;
#endif
#define PMIC_V_OUT0_PRE2_v_out0_pre2_START (0)
#define PMIC_V_OUT0_PRE2_v_out0_pre2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre2 : 8;
    } reg;
} PMIC_V_OUT1_PRE2_UNION;
#endif
#define PMIC_V_OUT1_PRE2_v_out1_pre2_START (0)
#define PMIC_V_OUT1_PRE2_v_out1_pre2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre3 : 8;
    } reg;
} PMIC_V_OUT0_PRE3_UNION;
#endif
#define PMIC_V_OUT0_PRE3_v_out0_pre3_START (0)
#define PMIC_V_OUT0_PRE3_v_out0_pre3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre3 : 8;
    } reg;
} PMIC_V_OUT1_PRE3_UNION;
#endif
#define PMIC_V_OUT1_PRE3_v_out1_pre3_START (0)
#define PMIC_V_OUT1_PRE3_v_out1_pre3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre4 : 8;
    } reg;
} PMIC_V_OUT0_PRE4_UNION;
#endif
#define PMIC_V_OUT0_PRE4_v_out0_pre4_START (0)
#define PMIC_V_OUT0_PRE4_v_out0_pre4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre4 : 8;
    } reg;
} PMIC_V_OUT1_PRE4_UNION;
#endif
#define PMIC_V_OUT1_PRE4_v_out1_pre4_START (0)
#define PMIC_V_OUT1_PRE4_v_out1_pre4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre5 : 8;
    } reg;
} PMIC_V_OUT0_PRE5_UNION;
#endif
#define PMIC_V_OUT0_PRE5_v_out0_pre5_START (0)
#define PMIC_V_OUT0_PRE5_v_out0_pre5_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre5 : 8;
    } reg;
} PMIC_V_OUT1_PRE5_UNION;
#endif
#define PMIC_V_OUT1_PRE5_v_out1_pre5_START (0)
#define PMIC_V_OUT1_PRE5_v_out1_pre5_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre6 : 8;
    } reg;
} PMIC_V_OUT0_PRE6_UNION;
#endif
#define PMIC_V_OUT0_PRE6_v_out0_pre6_START (0)
#define PMIC_V_OUT0_PRE6_v_out0_pre6_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre6 : 8;
    } reg;
} PMIC_V_OUT1_PRE6_UNION;
#endif
#define PMIC_V_OUT1_PRE6_v_out1_pre6_START (0)
#define PMIC_V_OUT1_PRE6_v_out1_pre6_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre7 : 8;
    } reg;
} PMIC_V_OUT0_PRE7_UNION;
#endif
#define PMIC_V_OUT0_PRE7_v_out0_pre7_START (0)
#define PMIC_V_OUT0_PRE7_v_out0_pre7_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre7 : 8;
    } reg;
} PMIC_V_OUT1_PRE7_UNION;
#endif
#define PMIC_V_OUT1_PRE7_v_out1_pre7_START (0)
#define PMIC_V_OUT1_PRE7_v_out1_pre7_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre8 : 8;
    } reg;
} PMIC_V_OUT0_PRE8_UNION;
#endif
#define PMIC_V_OUT0_PRE8_v_out0_pre8_START (0)
#define PMIC_V_OUT0_PRE8_v_out0_pre8_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre8 : 8;
    } reg;
} PMIC_V_OUT1_PRE8_UNION;
#endif
#define PMIC_V_OUT1_PRE8_v_out1_pre8_START (0)
#define PMIC_V_OUT1_PRE8_v_out1_pre8_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out0_pre9 : 8;
    } reg;
} PMIC_V_OUT0_PRE9_UNION;
#endif
#define PMIC_V_OUT0_PRE9_v_out0_pre9_START (0)
#define PMIC_V_OUT0_PRE9_v_out0_pre9_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_out1_pre9 : 8;
    } reg;
} PMIC_V_OUT1_PRE9_UNION;
#endif
#define PMIC_V_OUT1_PRE9_v_out1_pre9_START (0)
#define PMIC_V_OUT1_PRE9_v_out1_pre9_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre0 : 8;
    } reg;
} PMIC_CURRENT0_PRE0_UNION;
#endif
#define PMIC_CURRENT0_PRE0_current0_pre0_START (0)
#define PMIC_CURRENT0_PRE0_current0_pre0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre0 : 8;
    } reg;
} PMIC_CURRENT1_PRE0_UNION;
#endif
#define PMIC_CURRENT1_PRE0_current1_pre0_START (0)
#define PMIC_CURRENT1_PRE0_current1_pre0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre1 : 8;
    } reg;
} PMIC_CURRENT0_PRE1_UNION;
#endif
#define PMIC_CURRENT0_PRE1_current0_pre1_START (0)
#define PMIC_CURRENT0_PRE1_current0_pre1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre1 : 8;
    } reg;
} PMIC_CURRENT1_PRE1_UNION;
#endif
#define PMIC_CURRENT1_PRE1_current1_pre1_START (0)
#define PMIC_CURRENT1_PRE1_current1_pre1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre2 : 8;
    } reg;
} PMIC_CURRENT0_PRE2_UNION;
#endif
#define PMIC_CURRENT0_PRE2_current0_pre2_START (0)
#define PMIC_CURRENT0_PRE2_current0_pre2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre2 : 8;
    } reg;
} PMIC_CURRENT1_PRE2_UNION;
#endif
#define PMIC_CURRENT1_PRE2_current1_pre2_START (0)
#define PMIC_CURRENT1_PRE2_current1_pre2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre3 : 8;
    } reg;
} PMIC_CURRENT0_PRE3_UNION;
#endif
#define PMIC_CURRENT0_PRE3_current0_pre3_START (0)
#define PMIC_CURRENT0_PRE3_current0_pre3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre3 : 8;
    } reg;
} PMIC_CURRENT1_PRE3_UNION;
#endif
#define PMIC_CURRENT1_PRE3_current1_pre3_START (0)
#define PMIC_CURRENT1_PRE3_current1_pre3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre4 : 8;
    } reg;
} PMIC_CURRENT0_PRE4_UNION;
#endif
#define PMIC_CURRENT0_PRE4_current0_pre4_START (0)
#define PMIC_CURRENT0_PRE4_current0_pre4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre4 : 8;
    } reg;
} PMIC_CURRENT1_PRE4_UNION;
#endif
#define PMIC_CURRENT1_PRE4_current1_pre4_START (0)
#define PMIC_CURRENT1_PRE4_current1_pre4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre5 : 8;
    } reg;
} PMIC_CURRENT0_PRE5_UNION;
#endif
#define PMIC_CURRENT0_PRE5_current0_pre5_START (0)
#define PMIC_CURRENT0_PRE5_current0_pre5_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre5 : 8;
    } reg;
} PMIC_CURRENT1_PRE5_UNION;
#endif
#define PMIC_CURRENT1_PRE5_current1_pre5_START (0)
#define PMIC_CURRENT1_PRE5_current1_pre5_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre6 : 8;
    } reg;
} PMIC_CURRENT0_PRE6_UNION;
#endif
#define PMIC_CURRENT0_PRE6_current0_pre6_START (0)
#define PMIC_CURRENT0_PRE6_current0_pre6_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre6 : 8;
    } reg;
} PMIC_CURRENT1_PRE6_UNION;
#endif
#define PMIC_CURRENT1_PRE6_current1_pre6_START (0)
#define PMIC_CURRENT1_PRE6_current1_pre6_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre7 : 8;
    } reg;
} PMIC_CURRENT0_PRE7_UNION;
#endif
#define PMIC_CURRENT0_PRE7_current0_pre7_START (0)
#define PMIC_CURRENT0_PRE7_current0_pre7_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre7 : 8;
    } reg;
} PMIC_CURRENT1_PRE7_UNION;
#endif
#define PMIC_CURRENT1_PRE7_current1_pre7_START (0)
#define PMIC_CURRENT1_PRE7_current1_pre7_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre8 : 8;
    } reg;
} PMIC_CURRENT0_PRE8_UNION;
#endif
#define PMIC_CURRENT0_PRE8_current0_pre8_START (0)
#define PMIC_CURRENT0_PRE8_current0_pre8_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre8 : 8;
    } reg;
} PMIC_CURRENT1_PRE8_UNION;
#endif
#define PMIC_CURRENT1_PRE8_current1_pre8_START (0)
#define PMIC_CURRENT1_PRE8_current1_pre8_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current0_pre9 : 8;
    } reg;
} PMIC_CURRENT0_PRE9_UNION;
#endif
#define PMIC_CURRENT0_PRE9_current0_pre9_START (0)
#define PMIC_CURRENT0_PRE9_current0_pre9_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current1_pre9 : 8;
    } reg;
} PMIC_CURRENT1_PRE9_UNION;
#endif
#define PMIC_CURRENT1_PRE9_current1_pre9_START (0)
#define PMIC_CURRENT1_PRE9_current1_pre9_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char offset_current_mod_0 : 8;
    } reg;
} PMIC_OFFSET_CURRENT_MOD_0_UNION;
#endif
#define PMIC_OFFSET_CURRENT_MOD_0_offset_current_mod_0_START (0)
#define PMIC_OFFSET_CURRENT_MOD_0_offset_current_mod_0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char offset_current_mod_1 : 8;
    } reg;
} PMIC_OFFSET_CURRENT_MOD_1_UNION;
#endif
#define PMIC_OFFSET_CURRENT_MOD_1_offset_current_mod_1_START (0)
#define PMIC_OFFSET_CURRENT_MOD_1_offset_current_mod_1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char offset_voltage_mod_0 : 8;
    } reg;
} PMIC_OFFSET_VOLTAGE_MOD_0_UNION;
#endif
#define PMIC_OFFSET_VOLTAGE_MOD_0_offset_voltage_mod_0_START (0)
#define PMIC_OFFSET_VOLTAGE_MOD_0_offset_voltage_mod_0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char offset_voltage_mod_1 : 8;
    } reg;
} PMIC_OFFSET_VOLTAGE_MOD_1_UNION;
#endif
#define PMIC_OFFSET_VOLTAGE_MOD_1_offset_voltage_mod_1_START (0)
#define PMIC_OFFSET_VOLTAGE_MOD_1_offset_voltage_mod_1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char clj_rw_inf1 : 8;
    } reg;
} PMIC_CLJ_RESERVED1_UNION;
#endif
#define PMIC_CLJ_RESERVED1_clj_rw_inf1_START (0)
#define PMIC_CLJ_RESERVED1_clj_rw_inf1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char clj_rw_inf2 : 8;
    } reg;
} PMIC_CLJ_RESERVED2_UNION;
#endif
#define PMIC_CLJ_RESERVED2_clj_rw_inf2_START (0)
#define PMIC_CLJ_RESERVED2_clj_rw_inf2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char clj_rw_inf3 : 8;
    } reg;
} PMIC_CLJ_RESERVED3_UNION;
#endif
#define PMIC_CLJ_RESERVED3_clj_rw_inf3_START (0)
#define PMIC_CLJ_RESERVED3_clj_rw_inf3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char clj_rw_inf4 : 8;
    } reg;
} PMIC_CLJ_RESERVED4_UNION;
#endif
#define PMIC_CLJ_RESERVED4_clj_rw_inf4_START (0)
#define PMIC_CLJ_RESERVED4_clj_rw_inf4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char i_reserve_1 : 8;
    } reg;
} PMIC_CLJ_RESERVED5_UNION;
#endif
#define PMIC_CLJ_RESERVED5_i_reserve_1_START (0)
#define PMIC_CLJ_RESERVED5_i_reserve_1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char i_reserve_2 : 8;
    } reg;
} PMIC_CLJ_RESERVED6_UNION;
#endif
#define PMIC_CLJ_RESERVED6_i_reserve_2_START (0)
#define PMIC_CLJ_RESERVED6_i_reserve_2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char v_reserve_1 : 8;
    } reg;
} PMIC_CLJ_RESERVED7_UNION;
#endif
#define PMIC_CLJ_RESERVED7_v_reserve_1_START (0)
#define PMIC_CLJ_RESERVED7_v_reserve_1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_rst_n : 8;
    } reg;
} PMIC_PMU_SOFT_RST_UNION;
#endif
#define PMIC_PMU_SOFT_RST_soft_rst_n_START (0)
#define PMIC_PMU_SOFT_RST_soft_rst_n_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char cic_clk_inv_i : 1;
        unsigned char cic_clk_inv_v : 1;
        unsigned char adc_ana_v_output : 1;
        unsigned char adc_ana_i_output : 1;
        unsigned char cali_en_i : 1;
        unsigned char cali_en_i_force : 1;
        unsigned char cali_en_v_force : 1;
        unsigned char cali_en_v : 1;
    } reg;
} PMIC_CLJ_DEBUG_UNION;
#endif
#define PMIC_CLJ_DEBUG_cic_clk_inv_i_START (0)
#define PMIC_CLJ_DEBUG_cic_clk_inv_i_END (0)
#define PMIC_CLJ_DEBUG_cic_clk_inv_v_START (1)
#define PMIC_CLJ_DEBUG_cic_clk_inv_v_END (1)
#define PMIC_CLJ_DEBUG_adc_ana_v_output_START (2)
#define PMIC_CLJ_DEBUG_adc_ana_v_output_END (2)
#define PMIC_CLJ_DEBUG_adc_ana_i_output_START (3)
#define PMIC_CLJ_DEBUG_adc_ana_i_output_END (3)
#define PMIC_CLJ_DEBUG_cali_en_i_START (4)
#define PMIC_CLJ_DEBUG_cali_en_i_END (4)
#define PMIC_CLJ_DEBUG_cali_en_i_force_START (5)
#define PMIC_CLJ_DEBUG_cali_en_i_force_END (5)
#define PMIC_CLJ_DEBUG_cali_en_v_force_START (6)
#define PMIC_CLJ_DEBUG_cali_en_v_force_END (6)
#define PMIC_CLJ_DEBUG_cali_en_v_START (7)
#define PMIC_CLJ_DEBUG_cali_en_v_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char current_coul_always_off : 1;
        unsigned char voltage_coul_always_off : 1;
        unsigned char coul_gate_clk_en : 1;
        unsigned char reg_data_clr : 1;
        unsigned char reserved : 4;
    } reg;
} PMIC_CLJ_DEBUG_2_UNION;
#endif
#define PMIC_CLJ_DEBUG_2_current_coul_always_off_START (0)
#define PMIC_CLJ_DEBUG_2_current_coul_always_off_END (0)
#define PMIC_CLJ_DEBUG_2_voltage_coul_always_off_START (1)
#define PMIC_CLJ_DEBUG_2_voltage_coul_always_off_END (1)
#define PMIC_CLJ_DEBUG_2_coul_gate_clk_en_START (2)
#define PMIC_CLJ_DEBUG_2_coul_gate_clk_en_END (2)
#define PMIC_CLJ_DEBUG_2_reg_data_clr_START (3)
#define PMIC_CLJ_DEBUG_2_reg_data_clr_END (3)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char state_test : 3;
        unsigned char reserved : 5;
    } reg;
} PMIC_STATE_TEST_UNION;
#endif
#define PMIC_STATE_TEST_state_test_START (0)
#define PMIC_STATE_TEST_state_test_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char coul_reserve : 8;
    } reg;
} PMIC_COUL_RESERVE_UNION;
#endif
#define PMIC_COUL_RESERVE_coul_reserve_START (0)
#define PMIC_COUL_RESERVE_coul_reserve_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_reserve0 : 8;
    } reg;
} PMIC_SOFT_RESERE0_UNION;
#endif
#define PMIC_SOFT_RESERE0_soft_reserve0_START (0)
#define PMIC_SOFT_RESERE0_soft_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_reserve1 : 8;
    } reg;
} PMIC_SOFT_RESERE1_UNION;
#endif
#define PMIC_SOFT_RESERE1_soft_reserve1_START (0)
#define PMIC_SOFT_RESERE1_soft_reserve1_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_reserve2 : 8;
    } reg;
} PMIC_SOFT_RESERE2_UNION;
#endif
#define PMIC_SOFT_RESERE2_soft_reserve2_START (0)
#define PMIC_SOFT_RESERE2_soft_reserve2_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_reserve3 : 8;
    } reg;
} PMIC_SOFT_RESERE3_UNION;
#endif
#define PMIC_SOFT_RESERE3_soft_reserve3_START (0)
#define PMIC_SOFT_RESERE3_soft_reserve3_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_reserve4 : 8;
    } reg;
} PMIC_SOFT_RESERE4_UNION;
#endif
#define PMIC_SOFT_RESERE4_soft_reserve4_START (0)
#define PMIC_SOFT_RESERE4_soft_reserve4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_reserve5 : 8;
    } reg;
} PMIC_SOFT_RESERE5_UNION;
#endif
#define PMIC_SOFT_RESERE5_soft_reserve5_START (0)
#define PMIC_SOFT_RESERE5_soft_reserve5_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_reserve6 : 8;
    } reg;
} PMIC_SOFT_RESERE6_UNION;
#endif
#define PMIC_SOFT_RESERE6_soft_reserve6_START (0)
#define PMIC_SOFT_RESERE6_soft_reserve6_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char soft_reserve7 : 8;
    } reg;
} PMIC_SOFT_RESERE7_UNION;
#endif
#define PMIC_SOFT_RESERE7_soft_reserve7_START (0)
#define PMIC_SOFT_RESERE7_soft_reserve7_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_chanl_sel : 5;
        unsigned char hkadc_fre_sel : 2;
        unsigned char hkadc_bapass : 1;
    } reg;
} PMIC_ADC_CTRL_UNION;
#endif
#define PMIC_ADC_CTRL_hkadc_chanl_sel_START (0)
#define PMIC_ADC_CTRL_hkadc_chanl_sel_END (4)
#define PMIC_ADC_CTRL_hkadc_fre_sel_START (5)
#define PMIC_ADC_CTRL_hkadc_fre_sel_END (6)
#define PMIC_ADC_CTRL_hkadc_bapass_START (7)
#define PMIC_ADC_CTRL_hkadc_bapass_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_start : 1;
        unsigned char reserved : 7;
    } reg;
} PMIC_ADC_START_UNION;
#endif
#define PMIC_ADC_START_hkadc_start_START (0)
#define PMIC_ADC_START_hkadc_start_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_valid : 1;
        unsigned char reserved : 7;
    } reg;
} PMIC_CONV_STATUS_UNION;
#endif
#define PMIC_CONV_STATUS_hkadc_valid_START (0)
#define PMIC_CONV_STATUS_hkadc_valid_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_data11_4 : 8;
    } reg;
} PMIC_ADC_DATA1_UNION;
#endif
#define PMIC_ADC_DATA1_hkadc_data11_4_START (0)
#define PMIC_ADC_DATA1_hkadc_data11_4_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char reserved : 4;
        unsigned char hkadc_data3_0 : 4;
    } reg;
} PMIC_ADC_DATA0_UNION;
#endif
#define PMIC_ADC_DATA0_hkadc_data3_0_START (4)
#define PMIC_ADC_DATA0_hkadc_data3_0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_buffer_sel : 1;
        unsigned char hkadc_config : 6;
        unsigned char reserved : 1;
    } reg;
} PMIC_ADC_CONV_UNION;
#endif
#define PMIC_ADC_CONV_hkadc_buffer_sel_START (0)
#define PMIC_ADC_CONV_hkadc_buffer_sel_END (0)
#define PMIC_ADC_CONV_hkadc_config_START (1)
#define PMIC_ADC_CONV_hkadc_config_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_ibias_sel : 8;
    } reg;
} PMIC_ADC_CURRENT_UNION;
#endif
#define PMIC_ADC_CURRENT_hkadc_ibias_sel_START (0)
#define PMIC_ADC_CURRENT_hkadc_ibias_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_cali_en : 1;
        unsigned char hkadc_cali_sel : 1;
        unsigned char reserved : 6;
    } reg;
} PMIC_ADC_CALI_CTRL_UNION;
#endif
#define PMIC_ADC_CALI_CTRL_hkadc_cali_en_START (0)
#define PMIC_ADC_CALI_CTRL_hkadc_cali_en_END (0)
#define PMIC_ADC_CALI_CTRL_hkadc_cali_sel_START (1)
#define PMIC_ADC_CALI_CTRL_hkadc_cali_sel_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_cali_data : 8;
    } reg;
} PMIC_ADC_CALI_VALUE_UNION;
#endif
#define PMIC_ADC_CALI_VALUE_hkadc_cali_data_START (0)
#define PMIC_ADC_CALI_VALUE_hkadc_cali_data_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_cali_cfg : 8;
    } reg;
} PMIC_ADC_CALI_CFG_UNION;
#endif
#define PMIC_ADC_CALI_CFG_hkadc_cali_cfg_START (0)
#define PMIC_ADC_CALI_CFG_hkadc_cali_cfg_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_reserve0 : 8;
    } reg;
} PMIC_ADC_RSV0_UNION;
#endif
#define PMIC_ADC_RSV0_hkadc_reserve0_START (0)
#define PMIC_ADC_RSV0_hkadc_reserve0_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned char value;
    struct
    {
        unsigned char hkadc_reserve1 : 8;
    } reg;
} PMIC_ADC_RSV1_UNION;
#endif
#define PMIC_ADC_RSV1_hkadc_reserve1_START (0)
#define PMIC_ADC_RSV1_hkadc_reserve1_END (7)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
