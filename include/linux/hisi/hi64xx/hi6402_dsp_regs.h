/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#ifndef __HI6402_DSP_REGS_H__
#define __HI6402_DSP_REGS_H__
#include "hi64xx_dsp_regs.h"
#define HI6402_DSP_I2S_DSPIF_CLK_EN 	   (BASE_ADDR_PAGE_CFG + 0x045)

#define HI6402_DSP_CMD0 				   (BASE_ADDR_PAGE_CFG + 0x070)
/* use for notify DSP PLL status */
#define HI6402_DSP_CMD1 				   (BASE_ADDR_PAGE_CFG + 0x071)
#define HI6402_DSP_CMD2 				   (BASE_ADDR_PAGE_CFG + 0x072)

/* APB CLK */
#define HI6402_APB_CLK_CFG_REG		(BASE_ADDR_PAGE_CFG + 0x053)
#define HI6402_PERI_CLK_SEL_BIT			7
#define HI6402_APB_CLK_EN_BIT			0

#define HI6402_UART_MODE				   (HI6402_DSP_CMD2)
/* use for notify soundtrigger event */
#define HI6402_DSP_CMD3 				   (BASE_ADDR_PAGE_CFG + 0x073)

#define HI6402_OCRAM1_BASE                 0x10046340
#define HI6402_MSG_ADDR                    (HI6402_OCRAM1_BASE + HI64xx_MSG_START_ADDR_OFFSET)
#define HI6402_PARA_ADDR                   (HI6402_OCRAM1_BASE)
#define HI6402_MLIB_TO_AP_MSG_ADDR         (HI6402_OCRAM1_BASE + 0x19874)
#define HI6402_MLIB_TO_AP_MSG_SIZE         (588)

#define HI6402_CFG_SW_ENTER 1
#define HI6402_CFG_SW_EXIT 0

#define HI6402_DUMP_OCRAM_ADDR				(0x1005FE00)
#define HI6402_DUMP_OCRAM_SIZE				(0x200)
#define HI6402_DUMP_LOG_ADDR				(0x10047340)
#define HI6402_DUMP_LOG_SIZE				(0x9000)

/* hi6402 debug addr */
#define HI6402_DSP_WFI_STATE_ADDR 0x100503C4
#define HI6402_DSP_MSG_STATE_ADDR (HI6402_DUMP_OCRAM_ADDR + 0x18)
#define HI6402_OCRAM_START_ADDR    (0x10000000)
#define HI6402_OCRAM_SIZE          (0x60000)
#define HI6402_ITCM_START_ADDR     (0x08000000)
#define HI6402_ITCM_SIZE           (0x5000)
#define HI6402_DTCM_START_ADDR     (0x08010000)
#define HI6402_DTCM_SIZE           (0x20000)

#define HI6402_DSP_S2_CTRL_L     (HI64xx_AUDIO_SUB_BASE + 0xB0)

#define HI6402_DSP_S3_CTRL_L     (HI64xx_AUDIO_SUB_BASE + 0xB2)

#define HI6402_SLIM_CTRL_3     (HI64xx_AUDIO_SUB_BASE + 0xC6)
#define HI6402_SLIM_CTRL_5     (HI64xx_AUDIO_SUB_BASE + 0xC8)

#define HI6402_SLIM_UP_EN     (HI64xx_AUDIO_SUB_BASE + 0x1FE)

#define HI6402_SC_S1_SRC_LR_CTRL_M		   (HI64xx_AUDIO_SUB_BASE + 0x37)
#define HI6402_SC_S2_SRC_LR_CTRL_M		   (HI64xx_AUDIO_SUB_BASE + 0x49)
#define HI6402_SC_S3_SRC_LR_CTRL_M		   (HI64xx_AUDIO_SUB_BASE + 0x58)
#define HI6402_SC_S4_SRC_LR_CTRL_M		   (HI64xx_AUDIO_SUB_BASE + 0x67)
#define HI6402_SC_MISC_SRC_CTRL_H		   (HI64xx_AUDIO_SUB_BASE + 0x88)

#endif
