/*
 * asp_cfg.h -- asp cfg driver
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASP_CFG_H__
#define __ASP_CFG_H__


/******************************************************************************/
/*                      ASP asp_cfg                         */
/******************************************************************************/
#define ASP_CFG_R_RST_CTRLEN_REG        (0x0)
#define ASP_CFG_R_RST_CTRLDIS_REG       (0x4)
#define ASP_CFG_R_RST_CTRLSTAT_REG      (0x8)
#define ASP_CFG_R_GATE_EN_REG           (0xC)
#define ASP_CFG_R_GATE_DIS_REG          (0x10)
#define ASP_CFG_R_GATE_CLKEN_REG        (0x14)
#define ASP_CFG_R_GATE_CLKSTAT_REG      (0x18)
#define ASP_CFG_R_GATE_CLKDIV_EN_REG    (0x1C)
#define ASP_CFG_R_CLK1_DIV_REG          (0x20)
#define ASP_CFG_R_CLK2_DIV_REG          (0x24)
#define ASP_CFG_R_CLK3_DIV_REG          (0x28)
#define ASP_CFG_R_CLK4_DIV_REG          (0x2C)
#define ASP_CFG_R_CLK5_DIV_REG          (0x30)
#define ASP_CFG_R_CLK6_DIV_REG          (0x34)
#define ASP_CFG_R_CLK_SEL_REG           (0x38)
#define ASP_CFG_R_DSP_NMI_REG           (0x3C)
#define ASP_CFG_R_DSP_PRID_REG          (0x40)
#define ASP_CFG_R_DSP_RUNSTALL_REG      (0x44)
#define ASP_CFG_R_DSP_STATVECTORSEL_REG  (0x48)
#define ASP_CFG_R_DSP_OCDHALTONRESET_REG  (0x4C)
#define ASP_CFG_R_DSP_STATUS_REG	(0x50)
#define ASP_CFG_R_DMAC_SEL_REG          (0x54)
#define ASP_CFG_R_BUS_PRIORITY_REG      (0x58)
#define ASP_CFG_R_CG_EN_REG             (0x5C)
#define ASP_CFG_R_OCRAM_RET_REG         (0x60)
#define ASP_CFG_R_INTR_NS_INI_REG       (0x64)
#define ASP_CFG_R_INTR_NS_EN_REG        (0x68)
#define ASP_CFG_R_INTR_NS_MASK_REG      (0x6c)
#define ASP_CFG_R_DBG_SET_AHB2AXI_REG   (0x70)
#define ASP_CFG_R_DBG_STATUS_AHB2AXI_REG (0x74)
#define ASP_CFG_R_DLOCK_BP_REG          (0x78)
#define ASP_CFG_R_TZ_SECURE_N_REG       (0x100)
#define ASP_CFG_R_OCRAM_R0SIZE_REG      (0x104)
#define ASP_CFG_R_SIO_LOOP_SECURE_N_REG (0x108)
#define ASP_CFG_R_INTR_S_INI_REG        (0x10C)
#define ASP_CFG_R_INTR_S_EN_REG         (0x110)
#define ASP_CFG_R_INTR_S_MASK_REG       (0x114)
#define ASP_CFG_R_DSP_REMAPPING_EN_REG  (0x118)
#define ASP_CFG_R_DSP_REMAPPING_REG     (0x11c)
#define ASP_CFG_R_SLIMBUS_ID            (0x01b8)
#define ASP_CFG_R_SLIM_DAT_FMT_CFG      (0x1bc)
#define ASP_CFG_R_SLIM_DAT_CHNL_CFG     (0x1c0)
#define ASP_CFG_R_HDMI_CLK_SEL_REG       (0x1D8)
#define ASP_CFG_R_DIV_SPDIF_CLKSEL_REG       (0x1E4)
//******************************************************************************
//REG FIELD
//******************************************************************************
#define ASP_CFG_RST_TIMER1_N_LEN    1
#define ASP_CFG_RST_TIMER1_N_OFFSET 18
#define ASP_CFG_RST_TIMER0_N_LEN    1
#define ASP_CFG_RST_TIMER0_N_OFFSET 17
#define ASP_CFG_RST_GPIO_N_LEN    1
#define ASP_CFG_RST_GPIO_N_OFFSET 16
#define ASP_CFG_RST_ASP_H2X_N_LEN    1
#define ASP_CFG_RST_ASP_H2X_N_OFFSET 15
#define ASP_CFG_RST_SLIMBUS_N_LEN    1
#define ASP_CFG_RST_SLIMBUS_N_OFFSET 14
#define ASP_CFG_RST_SLIMBUS_BASE_N_LEN    1
#define ASP_CFG_RST_SLIMBUS_BASE_N_OFFSET 13
#define ASP_CFG_RST_WATCHDOG_N_LEN    1
#define ASP_CFG_RST_WATCHDOG_N_OFFSET 12
#define ASP_CFG_RST_OCRAM_N_LEN    1
#define ASP_CFG_RST_OCRAM_N_OFFSET 11
#define ASP_CFG_RST_SRC_DOWM_N_LEN    1
#define ASP_CFG_RST_SRC_DOWM_N_OFFSET 10
#define ASP_CFG_RST_SRC_UP_N_LEN    1
#define ASP_CFG_RST_SRC_UP_N_OFFSET 9
#define ASP_CFG_RST_DMAC_N_LEN    1
#define ASP_CFG_RST_DMAC_N_OFFSET 8
#define ASP_CFG_RST_ASP_HDMI_N_LEN    1
#define ASP_CFG_RST_ASP_HDMI_N_OFFSET 7
#define ASP_CFG_RST_IPC_N_LEN    1
#define ASP_CFG_RST_IPC_N_OFFSET 6
#define ASP_CFG_RST_DSP_DEBUG_N_LEN    1
#define ASP_CFG_RST_DSP_DEBUG_N_OFFSET 5
#define ASP_CFG_RST_DSP_N_LEN    1
#define ASP_CFG_RST_DSP_N_OFFSET 4
#define ASP_CFG_RST_SIO_MODEM_LEN    1
#define ASP_CFG_RST_SIO_MODEM_OFFSET 3
#define ASP_CFG_RST_SIO_BT_LEN    1
#define ASP_CFG_RST_SIO_BT_OFFSET 2
#define ASP_CFG_RST_SIO_VOICE_LEN    1
#define ASP_CFG_RST_SIO_VOICE_OFFSET 1
#define ASP_CFG_RST_SIO_AUDIO_LEN    1
#define ASP_CFG_RST_SIO_AUDIO_OFFSET 0

#define ASP_CFG_GT_SPDIF_LEN    1
#define ASP_CFG_GT_SPDIF_OFFSET 26
#define ASP_CFG_GT_AUDIO_PLL_LEN    1
#define ASP_CFG_GT_AUDIO_PLL_OFFSET 25
#define ASP_CFG_GT_TIMER1HCLK_LEN    1
#define ASP_CFG_GT_TIMER1HCLK_OFFSET 23
#define ASP_CFG_GT_TIMER0HCLK_LEN    1
#define ASP_CFG_GT_TIMER0HCLK_OFFSET 22
#define ASP_CFG_GT_GPIOHCLK_LEN    1
#define ASP_CFG_GT_GPIOHCLK_OFFSET 21
#define ASP_CFG_GT_HDMIBCLK_LEN    1
#define ASP_CFG_GT_HDMIBCLK_OFFSET 20
#define ASP_CFG_GT_SPDIFCLK_LEN    1
#define ASP_CFG_GT_SPDIFCLK_OFFSET 19
#define ASP_CFG_GT_ASP_H2X_LEN    1
#define ASP_CFG_GT_ASP_H2X_OFFSET 18
#define ASP_CFG_GT_SLIMBUS_BASE_CLK_LEN    1
#define ASP_CFG_GT_SLIMBUS_BASE_CLK_OFFSET 17
#define ASP_CFG_GT_SLIMBUSHCLK_LEN    1
#define ASP_CFG_GT_SLIMBUSHCLK_OFFSET 16
#define ASP_CFG_GT_WDHCLK_LEN    1
#define ASP_CFG_GT_WDHCLK_OFFSET 15
#define ASP_CFG_GT_OCRAMHCLK_LEN    1
#define ASP_CFG_GT_OCRAMHCLK_OFFSET 14
#define ASP_CFG_GT_SRCDOWNHCLK_LEN    1
#define ASP_CFG_GT_SRCDOWNHCLK_OFFSET 13
#define ASP_CFG_GT_SRCUPHCLK_LEN    1
#define ASP_CFG_GT_SRCUPHCLK_OFFSET 12
#define ASP_CFG_GT_DMACHCLK_LEN    1
#define ASP_CFG_GT_DMACHCLK_OFFSET 11
#define ASP_CFG_GT_HDMIHCLK_LEN    1
#define ASP_CFG_GT_HDMIHCLK_OFFSET 10
#define ASP_CFG_GT_IPCHCLK_LEN    1
#define ASP_CFG_GT_IPCHCLK_OFFSET 9
#define ASP_CFG_GT_SIOMODEM_LEN    1
#define ASP_CFG_GT_SIOMODEM_OFFSET 8
#define ASP_CFG_GT_MODEMBCLK_OUT_LEN    1
#define ASP_CFG_GT_MODEMBCLK_OUT_OFFSET 7
#define ASP_CFG_GT_SIOBT_LEN    1
#define ASP_CFG_GT_SIOBT_OFFSET 6
#define ASP_CFG_GT_BTBCLK_OUT_LEN    1
#define ASP_CFG_GT_BTBCLK_OUT_OFFSET 5
#define ASP_CFG_GT_SIOVOICE_LEN    1
#define ASP_CFG_GT_SIOVOICE_OFFSET 4
#define ASP_CFG_GT_VOICEBCLK_OUT_LEN    1
#define ASP_CFG_GT_VOICEBCLK_OUT_OFFSET 3
#define ASP_CFG_GT_SIOAUDIO_LEN    1
#define ASP_CFG_GT_SIOAUDIO_OFFSET 2
#define ASP_CFG_GT_AUDIOBCLK_OUT_LEN    1
#define ASP_CFG_GT_AUDIOBCLK_OUT_OFFSET 1
#define ASP_CFG_GT_HIFIDSPCLK_LEN    1
#define ASP_CFG_GT_HIFIDSPCLK_OFFSET 0

#define ASP_CFG_GT_SPDIF_BCLK_DIV_LEN    1
#define ASP_CFG_GT_SPDIF_BCLK_DIV_OFFSET 10
#define ASP_CFG_GT_HDMIADWS_BCLK_DIV_LEN    1
#define ASP_CFG_GT_HDMIADWS_CLK_DIV_OFFSET 9
#define ASP_CFG_GT_SIO_MODEM_BCLK_DIV_LEN    1
#define ASP_CFG_GT_SIO_MODEM_BCLK_DIV_OFFSET 6
#define ASP_CFG_GT_SIO_BT_BCLK_DIV_LEN    1
#define ASP_CFG_GT_SIO_BT_BCLK_DIV_OFFSET 5
#define ASP_CFG_GT_SIO_VOICE_BCLK_DIV_LEN    1
#define ASP_CFG_GT_SIO_VOICE_BCLK_DIV_OFFSET 4
#define ASP_CFG_GT_SIO_AUDIO_BCLK_DIV_LEN    1
#define ASP_CFG_GT_SIO_AUDIO_BCLK_DIV_OFFSET 3
#define ASP_CFG_GT_SIOBCLK_DIV_LEN    1
#define ASP_CFG_GT_SIOBCLK_DIV_OFFSET 2
#define ASP_CFG_GT_HDMIREF_DIV_LEN    1
#define ASP_CFG_GT_HDMIREF_DIV_OFFSET 1
#define ASP_CFG_GT_HDMIMCLK_DIV_LEN    1
#define ASP_CFG_GT_HDMIMCLK_DIV_OFFSET 0

#define RST_ASP_H2X_BIT		(1 << ASP_CFG_RST_ASP_H2X_N_OFFSET)

#define RST_ASP_AUDIO_PLL_BIT		(1 << ASP_CFG_GT_AUDIO_PLL_OFFSET)
#define RST_HDMIBCLK_BIT				(1 << ASP_CFG_GT_HDMIBCLK_OFFSET)
#define RST_HDMIBCLK_BIT				(1 << ASP_CFG_GT_HDMIBCLK_OFFSET)
#define RST_SIO_AUDIO_BIT			(1 << ASP_CFG_RST_SIO_AUDIO_N_OFFSET)
#define RST_SIO_VOICE_BIT			(1 << ASP_CFG_RST_SIO_VOICE_N_OFFSET)
#define RST_SIO_BT_BIT				(1 << ASP_CFG_RST_SIO_BT_N_OFFSET)
#define RST_SIO_MODEM_BIT			(1 << ASP_CFG_RST_SIO_MODEM_N_OFFSET)
#define RST_ASP_HDMI_BIT				(1 << ASP_CFG_RST_ASP_HDMI_N_OFFSET)
#define RST_SRCUP_BIT				(1 << ASP_CFG_RST_SRC_UP_N_OFFSET)
#define RST_SRCDOWN_BIT				(1 << ASP_CFG_RST_SRC_DOWN_N_OFFSET)
#define RST_DMAC_BIT					(1 << ASP_CFG_RST_DMAC_N_OFFSET)
#define RST_SLIMBUS_BIT     			(1 << ASP_CFG_RST_SLIMBUS_N_OFFSET)
#define RST_SLIMBUS_BASE_BIT  		(1 << ASP_CFG_RST_SLIMBUS_BASE_N_OFFSET)

#define CLK_AUDIO_PLL_BIT  		(1 << ASP_CFG_GT_AUDIO_PLL_OFFSET)
#define CLK_HDMI_HCLK_BIT  		(1 << ASP_CFG_GT_HDMIHCLK_OFFSET)
#define CLK_HDMI_BCLK_BIT  		(1 << ASP_CFG_GT_HDMIBCLK_OFFSET)
#define CLK_HDMI_MCLK_BIT  		(1 << ASP_CFG_GT_ASP_H2X_OFFSET)
#define CLK_SLIMBUSH_BIT            	(1 << ASP_CFG_GT_SLIMBUSHCLK_OFFSET)
#define CLK_SLIMBUS_BASE_BIT        (1 << ASP_CFG_GT_SLIMBUS_BASE_CLK_OFFSET)
#define CLK_SPDIF_HCLK_BIT			(1 << ASP_CFG_GT_SPDIFCLK_OFFSET)
#define CLK_GT_SPDIF_BIT  		(1 << ASP_CFG_GT_SPDIF_OFFSET)

#define ASP_CFG_ASP_HDMI_INT_OFFSET 0

void asp_cfg_hdmi_clk_sel(unsigned int value);
void asp_cfg_div_clk(unsigned int value);
void asp_cfg_enable_hdmi_interrupeter(void);
void asp_cfg_disable_hdmi_interrupeter(void);
void asp_cfg_hdmi_module_enable(void);
void asp_cfg_hdmi_module_disable(void);
void asp_cfg_dp_module_enable(void);
void asp_cfg_dp_module_disable(void);
unsigned int asp_cfg_get_irq_value(void);
#endif
