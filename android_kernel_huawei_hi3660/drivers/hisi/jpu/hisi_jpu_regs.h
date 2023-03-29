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
#ifndef HISI_JPU_REGS_H
#define HISI_JPU_REGS_H

#include "hisi_jpu.h"


#define MAX_INPUT_WIDTH	(8192)
#define MAX_INPUT_HEIGHT	(8192)
#define MIN_INPUT_WIDTH	(16)
#define MIN_INPUT_HEIGHT	(16)

/* start address for planar Y or RGB, must align to 64 byte (YUV format) or 128 byte (RGB format), unit is 16 byte */
#define JPU_OUT_RGB_ADDR_ALIGN	(8)
#define JPU_OUT_YUV_ADDR_ALIGN	(4)
#define JPU_OUT_STRIDE_ALIGN	(8)
#define JPU_MIN_STRIDE	(1)
#define JPU_MAX_STRIDE	(2048)
#define JPU_MAX_ADDR	(0xfffffff)

#define JPU_LB_ADDR_ALIGN	(128)
/*******************************************************************************
** JPEG CVDR
*/
#define JPGDEC_CVDR_AXI_JPEG_CVDR_CFG	(0x0)
#define JPGDEC_CVDR_AXI_JPEG_CVDR_WR_QOS_CFG	(0xc)
#define JPGDEC_CVDR_AXI_JPEG_CVDR_RD_QOS_CFG	(0x10)
#define JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1	(0xA10)
#define JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_1	(0xA18)
#define JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_2	(0xA20)
#define JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_2	(0xA28)
#define JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0	(0x900)
#define JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1	(0x910)
#define JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_WR_0	(0x908)
#define JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_WR_1	(0x918)

/*
** JPEG Decode: atlant fpga cs
*/
#define JPGDEC_CVDR_AXI_JPEG_CVDR_CFG_CS			(0x0)
#define JPGDEC_CVDR_AXI_JPEG_CVDR_WR_QOS_CFG_CS		(0xc)
#define JPGDEC_CVDR_AXI_JPEG_CVDR_RD_QOS_CFG_CS		(0x10)
#define JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS			(0x15B0)
#define JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_8_CS		(0x15B8)
#define JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_9_CS			(0x15C0)
#define JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_9_CS		(0x15C8)
#define JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_8_CS			(0x13B0)
#define JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_9_CS			(0x13C0)
#define JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_WR_8_CS		(0x13B8)
#define JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_WR_9_CS		(0x13C8)


/*******************************************************************************
** JPEG TOP
*/
#define JPGDEC_RO_STATE (0xC)
#define JPGDEC_CRG_CFG0	(0x20)
#define JPGDEC_CRG_CFG1	(0x24)
#define JPGDEC_MEM_CFG	(0x28)
#define JPGDEC_IRQ_REG0	(0x2C)
#define JPGDEC_IRQ_REG1	(0x30)
#define JPGDEC_IRQ_REG2	(0x34)

/*
** JPGDEC_IRQ_REG1: mask jpg decoder IRQ state
** 0: not mask the irq; 1: mask the irq;
*/
#define BIT_JPGDEC_INT_OVER_TIME	BIT(3)
#define BIT_JPGDEC_INT_DEC_ERR	BIT(2)
#define BIT_JPGDEC_INT_BS_RES	BIT(1)
#define BIT_JPGDEC_INT_DEC_FINISH	BIT(0)


/*******************************************************************************
** JPEG Decode
*/
#define JPEG_DEC_START	(0x0)       //once start,the decoder info will set to default value
#define JPEG_DEC_PREFTCH_CTRL	(0x4)
#define JPEG_DEC_FRAME_SIZE	(0x8)
#define JPEG_DEC_CROP_HORIZONTAL	(0xC)
#define JPEG_DEC_CROP_VERTICAL	(0x10)
#define JPEG_DEC_BITSTREAMS_START	(0x14)
#define JPEG_DEC_BITSTREAMS_END	(0x18)
#define JPEG_DEC_FRAME_START_Y	(0x1C)
#define JPEG_DEC_FRAME_STRIDE_Y	(0x20)
#define JPEG_DEC_FRAME_START_C	(0x24)
#define JPEG_DEC_FRAME_STRIDE_C	(0x28)
#define JPEG_DEC_LBUF_START_ADDR	(0x2C)
#define JPEG_DEC_OUTPUT_TYPE	(0x30)
#define JPEG_DEC_FREQ_SCALE	(0x34)
#define JPEG_DEC_MIDDLE_FILTER	(0x38)
#define JPEG_DEC_SAMPLING_FACTOR	(0x3C)
#define JPEG_DEC_DRI	(0x40)
#define JPEG_DEC_OVER_TIME_THD	(0x4C)

#define JPEG_DEC_HOR_PHASE0_COEF01	(0x80)
#define JPEG_DEC_HOR_PHASE0_COEF23	(0x84)
#define JPEG_DEC_HOR_PHASE0_COEF45	(0x88)
#define JPEG_DEC_HOR_PHASE0_COEF67	(0x8C)
#define JPEG_DEC_HOR_PHASE2_COEF01	(0x90)
#define JPEG_DEC_HOR_PHASE2_COEF23	(0x94)
#define JPEG_DEC_HOR_PHASE2_COEF45	(0x98)
#define JPEG_DEC_HOR_PHASE2_COEF67	(0x9C)

#define JPEG_DEC_VER_PHASE0_COEF01	(0xA0)
#define JPEG_DEC_VER_PHASE0_COEF23	(0xA4)
#define JPEG_DEC_VER_PHASE2_COEF01	(0xA8)
#define JPEG_DEC_VER_PHASE2_COEF23	(0xAC)

#define JPEG_DEC_CSC_IN_DC_COEF	(0xB0)
#define JPEG_DEC_CSC_OUT_DC_COEF	(0xB4)
#define JPEG_DEC_CSC_TRANS_COEF0	(0xB8)
#define JPEG_DEC_CSC_TRANS_COEF1	(0xBC)
#define JPEG_DEC_CSC_TRANS_COEF2	(0xC0)
#define JPEG_DEC_CSC_TRANS_COEF3	(0xC4)
#define JPEG_DEC_CSC_TRANS_COEF4	(0xC8)

#define JPGD_REG_QUANT	(0x200)
#define JPGD_REG_HDCTABLE	(0x300)
#define JPGD_REG_HACMINTABLE	(0x340)
#define JPGD_REG_HACBASETABLE	(0x380)
#define JPGD_REG_HACSYMTABLE	(0x400)
#define JPGD_REG_DEBUG_BASE	(0x800)
#define JPGD_REG_DEBUG_RANGE	(16)

/*
**#define JPEG_DEC_QUANT_TABLE	0x200+0x4*qt_range
**#define JPEG_DEC_HDC_TABLE	0x300+0x4*hdc_range
**#define JPEG_DEC_HAC_MIN_TABLE	0x340+0x4*hac_range
**#define JPEG_DEC_HAC_BASE_TABLE	0x380+0x4*hac_range
**#define JPEG_DEC_HAC_SYMBOL_TABLE	0x400+0x4*rs_range
*/

/*
** JPEG Decode: atlant fpga cs
*/
#define JPEG_DEC_START_CS  (0x0)       //once start,the decoder info will set to default value
#define JPEG_DEC_PREFTCH_CTRL_CS   (0x4)
#define JPEG_DEC_FRAME_SIZE_CS (0x10)
#define JPEG_DEC_CROP_HORIZONTAL_CS    (0x14)
#define JPEG_DEC_CROP_VERTICAL_CS  (0x18)
#define JPEG_DEC_BITSTREAMS_START_H_CS   (0x20)
#define JPEG_DEC_BITSTREAMS_START_L_CS   (0x24)
#define JPEG_DEC_BITSTREAMS_END_H_CS (0x28)
#define JPEG_DEC_BITSTREAMS_END_L_CS (0x2C)
#define JPEG_DEC_FRAME_START_Y_CS  (0x30)
#define JPEG_DEC_FRAME_STRIDE_Y_CS (0x34)
#define JPEG_DEC_FRAME_START_C_CS  (0x38)
#define JPEG_DEC_FRAME_STRIDE_C_CS (0x3C)
#define JPEG_DEC_LBUF_START_ADDR_CS    (0x40)
#define JPEG_DEC_OUTPUT_TYPE_CS    (0x50)
#define JPEG_DEC_FREQ_SCALE_CS (0x54)
#define JPEG_DEC_MIDDLE_FILTER_CS  (0x58)
#define JPEG_DEC_SAMPLING_FACTOR_CS    (0x5C)
#define JPEG_DEC_DRI_CS    (0x60)
#define JPEG_DEC_OVER_TIME_THD_CS  (0x70)

#define JPEG_DEC_HOR_PHASE0_COEF01_CS  (0x80)
#define JPEG_DEC_HOR_PHASE0_COEF23_CS  (0x84)
#define JPEG_DEC_HOR_PHASE0_COEF45_CS  (0x88)
#define JPEG_DEC_HOR_PHASE0_COEF67_CS  (0x8C)
#define JPEG_DEC_HOR_PHASE2_COEF01_CS  (0x90)
#define JPEG_DEC_HOR_PHASE2_COEF23_CS  (0x94)
#define JPEG_DEC_HOR_PHASE2_COEF45_CS  (0x98)
#define JPEG_DEC_HOR_PHASE2_COEF67_CS  (0x9C)

#define JPEG_DEC_VER_PHASE0_COEF01_CS  (0xA0)
#define JPEG_DEC_VER_PHASE0_COEF23_CS  (0xA4)
#define JPEG_DEC_VER_PHASE2_COEF01_CS  (0xA8)
#define JPEG_DEC_VER_PHASE2_COEF23_CS  (0xAC)

#define JPEG_DEC_CSC_IN_DC_COEF_CS (0xB0)
#define JPEG_DEC_CSC_OUT_DC_COEF_CS    (0xB4)
#define JPEG_DEC_CSC_TRANS_COEF0_CS    (0xB8)
#define JPEG_DEC_CSC_TRANS_COEF1_CS    (0xBC)
#define JPEG_DEC_CSC_TRANS_COEF2_CS    (0xC0)
#define JPEG_DEC_CSC_TRANS_COEF3_CS    (0xC4)
#define JPEG_DEC_CSC_TRANS_COEF4_CS    (0xC8)

#define JPGD_REG_QUANT_CS  (0x200)
#define JPGD_REG_HDCTABLE_CS   (0x300)
#define JPGD_REG_HACMINTABLE_CS    (0x340)
#define JPGD_REG_HACBASETABLE_CS   (0x380)
#define JPGD_REG_HACSYMTABLE_CS    (0x400)

/*
 * **#define JPEG_DEC_QUANT_TABLE_CS   0x200+0x4*qt_range
 * **#define JPEG_DEC_HDC_TABLE_CS 0x300+0x4*hdc_range
 * **#define JPEG_DEC_HAC_MIN_TABLE_CS 0x340+0x4*hac_range
 * **#define JPEG_DEC_HAC_BASE_TABLE_CS    0x380+0x4*hac_range
 * **#define JPEG_DEC_HAC_SYMBOL_TABLE_CS  0x400+0x4*rs_range
 * */

/*
** jpu freq scale
*/
enum jpu_freq_scale {
	JPU_FREQ_SCALE_1 = 0,
	JPU_FREQ_SCALE_2 = 1,
	JPU_FREQ_SCALE_4 = 2,
	JPU_FREQ_SCALE_8 = 3,
};

/*
** output format
*/
typedef enum {
	JPU_OUTPUT_UNSUPPORT = -1,
	JPU_OUTPUT_YUV420 = 0,
	JPU_OUTPUT_YUV_ORIGINAL = 1,
	JPU_OUTPUT_RGBA4444 = 4,
	JPU_OUTPUT_BGRA4444 = 5,
	JPU_OUTPUT_RGB565 = 6,
	JPU_OUTPUT_BGR565 = 7,
	JPU_OUTPUT_RGBA8888 = 8,
	JPU_OUTPUT_BGRA8888 = 9,
} jpu_output_format;

/*
** support platform
*/
typedef enum {
	HISI_KIRIN_970 = 1,
	HISI_DSS_V500,
	HISI_DSS_V501,
	UNSUPPORT_PLATFORM,
}jpeg_dec_platform;

typedef struct jpu_dec_reg {
	uint32_t dec_start;
	uint32_t preftch_ctrl;
	uint32_t frame_size;
	uint32_t crop_horizontal;
	uint32_t crop_vertical;
	uint32_t bitstreams_start_h;
	uint32_t bitstreams_start;
	uint32_t bitstreams_end_h;
	uint32_t bitstreams_end;
	uint32_t frame_start_y;
	uint32_t frame_stride_y;
	uint32_t frame_start_c;
	uint32_t frame_stride_c;
	uint32_t lbuf_start_addr;
	uint32_t output_type;
	uint32_t freq_scale;
	uint32_t middle_filter;
	uint32_t sampling_factor;
	uint32_t dri;
	uint32_t over_time_thd;
	uint32_t hor_phase0_coef01;
	uint32_t hor_phase0_coef23;
	uint32_t hor_phase0_coef45;
	uint32_t hor_phase0_coef67;
	uint32_t hor_phase2_coef01;
	uint32_t hor_phase2_coef23;
	uint32_t hor_phase2_coef45;
	uint32_t hor_phase2_coef67;
	uint32_t ver_phase0_coef01;
	uint32_t ver_phase0_coef23;
	uint32_t ver_phase2_coef01;
	uint32_t ver_phase2_coef23;
	uint32_t csc_in_dc_coef;
	uint32_t csc_out_dc_coef;
	uint32_t csc_trans_coef0;
	uint32_t csc_trans_coef1;
	uint32_t csc_trans_coef2;
	uint32_t csc_trans_coef3;
	uint32_t csc_trans_coef4;
} jpu_dec_reg_t;


#endif  /* HISI_JPU_REGS_H */
