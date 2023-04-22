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
#ifndef __HI64XX_DSP_DEBUG_H__
#define __HI64XX_DSP_DEBUG_H__

extern unsigned long hi64xx_dsp_debug_level;

#define HI64XX_DSP_ERROR(msg, ...)	  \
	do {						  \
		if (hi64xx_dsp_debug_level > 0)  \
			printk(KERN_ERR "hi64xx-dsp[E]%s: "msg, __func__, ## __VA_ARGS__); \
	   } while (0)

#define HI64XX_DSP_WARNING(msg, ...)	\
	do {						  \
		if (hi64xx_dsp_debug_level > 1)  \
			printk(KERN_WARNING "hi64xx-dsp[W]%s: "msg, __func__, ## __VA_ARGS__); \
	   } while (0)

#define HI64XX_DSP_INFO(msg, ...)	 \
	do {						  \
		if (hi64xx_dsp_debug_level > 2)  \
			printk(KERN_INFO "hi64xx-dsp[I]%s: "msg, __func__, ## __VA_ARGS__); \
	   } while (0)

#define HI64XX_DSP_DEBUG(msg, ...)	  \
	do {						  \
		if (hi64xx_dsp_debug_level > 3)  \
			printk(KERN_INFO "hi64xx-dsp[D]%s: "msg, __func__, ## __VA_ARGS__); \
	   } while (0)

#define IN_FUNCTION  HI64XX_DSP_DEBUG("++\n");
#define OUT_FUNCTION HI64XX_DSP_DEBUG("--\n");

enum HI64XX_HIFI_DEBUG {
	HI64XX_DSP_IMG_DOWNLOAD = 0,     /* 0*/
	HI64XX_DSP_INIT,                 /* 1*/
	HI64XX_DSP_DEINIT,               /* 2*/
	HI64XX_DSP_RUNSTALL_PULL_DOWN,   /* 3*/
	HI64XX_DSP_RUNSTALL_PULL_UP,     /* 4*/
	HI64XX_DSP_JTAG_ENABLE,          /* 5*/
	HI64XX_DSP_UART_ENABLE,          /* 6*/
	HI64XX_DSP_HIPLL_ENABLE,         /* 7*/
	HI64XX_DSP_HIPLL_DISABLE,        /* 8*/
	HI64XX_DSP_LOWPLL_ENABLE,        /* 9*/
	HI64XX_DSP_LOWPLL_DISABLE,       /*10*/
	HI64XX_DSP_FPGA_CODEC_RESET,     /*11*/
	HI64XX_DSP_FPGA_PWRON_TEST,      /*12*/
	HI64XX_DSP_FPGA_OM_TEST,         /*13*/
	HI64XX_DSP_FACTORY_TEST,         /*14*/
	HI64XX_DSP_DUMP_LOG,             /*15*/
	HI64XX_DSP_OCRAM_TCM_MEM_CHECK,  /*16*/
	HI64XX_DSP_DUMP_ALL_RAM,         /*17*/

	HI64XX_DSP_POWER_TEST_MAD_PLL_WFI_EN,
	HI64XX_DSP_POWER_TEST_MAD_PLL_WFI_DIS,
	HI64XX_DSP_POWER_TEST_MAIN_PLL_WFI_EN,
	HI64XX_DSP_POWER_TEST_MAIN_PLL_WFI_DIS,

	/* the follows add for dsp debug */
	HI64XX_DSP_EXCEPTION_POINTER_INJECT = 100,
	HI64XX_DSP_ENDLESS_LOOP_INJECT,
	HI64XX_DSP_SOFT_IRQ_INJECT,
	HI64XX_DSP_CALL_EXIT_INJECT,
	HI64XX_DSP_INTERRUPT_STORM_INJECT,
	HI64XX_DSP_PERFORMANCE_LEAK_INJECT,
	HI64XX_DSP_MEM_LEAK_INJECT,
	HI64XX_DSP_MEM_OVERLAP_INJECT,
	HI64XX_DSP_PA_DISORGANIZED_DMA_INJECT,
	HI64XX_DSP_MSG_STORM_INJECT,
	HI64XX_DSP_DSP_BYPASS,
	HI64XX_DSP_SMARTPA_ALG_BYPASS,
	HI64XX_DSP_PCM_TEST_START,
	HI64XX_DSP_PCM_TEST_STOP,
	HI64XX_DSP_TO_AP_MSG,
	HI64XX_DSP_DATA_STATISTIC,
	HI64XX_DSP_HISI_WAKEUP_OM_SWITCH,

	HI64XX_DEBUG_EXIT = 200,
};

#endif
