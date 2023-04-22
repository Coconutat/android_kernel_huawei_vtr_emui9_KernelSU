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
#ifndef __HISI_JPU_COMMON_H__
#define __HISI_JPU_COMMON_H__


#include <linux/types.h>

#define HISIJPU_IOCTL_MAGIC 'J'

#define HISIJPU_JOB_EXEC _IOW(HISIJPU_IOCTL_MAGIC, 0x21, struct jpu_data)


#define JPU_MCU_8ALIGN	(8)
#define JPU_MCU_16ALIGN	(16)

/*
** Maximum number of data components
*/
#define PIXEL_COMPONENT_NUM	(3)
#define NUM_COMPS_IN_SCAN	(4)

#define MAX_NUM_HUFF_TBLS	(4)
#define MAX_NUM_QUANT_TBLS	(4)
#define MAX_DCT_SIZE	(64)
#define HDC_TABLE_NUM	(12)
#define HAC_TABLE_NUM	(8)
#define HAC_SYMBOL_TABLE_NUM	(176)

#ifndef BIT
#define BIT(x)  (1<<(x))
#endif

typedef struct {
	uint32_t left;
	uint32_t right;
	uint32_t top;
	uint32_t bottom;
	uint32_t flag;
	uint32_t rsv;
} jpu_region_info;

/*
** input format
*/
typedef enum {
	HISI_JPEG_DECODE_RAW_YUV_UNSUPPORT = -1,
	HISI_JPEG_DECODE_RAW_YUV444 = 0,
	HISI_JPEG_DECODE_RAW_YUV422_H2V1 = 1, /* YUV422 */
	HISI_JPEG_DECODE_RAW_YUV422_H1V2 = 2, /* YUV440 */
	HISI_JPEG_DECODE_RAW_YUV420 = 3,
	HISI_JPEG_DECODE_RAW_YUV400 = 4,
	HISI_JPEG_DECODE_RAW_MAX,
} jpu_raw_format;

typedef enum {
	HISI_JPEG_DECODE_OUT_UNKNOWN = -1,
	HISI_JPEG_DECODE_OUT_YUV444 = 0, /* Y/Cb/Cr (also known as YUV) */
	HISI_JPEG_DECODE_OUT_YUV422_H2V1 = 1, /* YUV422 */
	HISI_JPEG_DECODE_OUT_YUV422_H1V2 = 2, /* YUV440 */
	HISI_JPEG_DECODE_OUT_YUV420 = 3,
	HISI_JPEG_DECODE_OUT_YUV400 = 4,
	HISI_JPEG_DECODE_OUT_RGBA4444 = 5,
	HISI_JPEG_DECODE_OUT_BGRA4444 = 6,
	HISI_JPEG_DECODE_OUT_RGB565 = 7,
	HISI_JPEG_DECODE_OUT_BGR565 = 8,
	HISI_JPEG_DECODE_OUT_RGBA8888 = 9,
	HISI_JPEG_DECODE_OUT_BGRA8888 = 10,
	HISI_JPEG_DECODE_OUT_FORMAT_MAX,
} jpu_color_space;

/*
** jpu decode sample scale
*/
typedef enum {
	HISI_JPEG_DECODE_SAMPLE_SIZE_1 = 1,
	HISI_JPEG_DECODE_SAMPLE_SIZE_2 = 2,
	HISI_JPEG_DECODE_SAMPLE_SIZE_4 = 4,
	HISI_JPEG_DECODE_SAMPLE_SIZE_8 = 8,
	HISI_JPEG_DECODE_SAMPLE_SIZE_MAX,
} hisi_jpeg_decode_sample_size;

/*
** jpu decode mode
*/
typedef enum {
	HISI_JPEG_DECODE_MODE_FULL = 0,
	HISI_JPEG_DECODE_MODE_FULL_SUB = 1,
	HISI_JPEG_DECODE_MODE_REGION = 2,
	HISI_JPEG_DECODE_MODE_REGION_SUB = 3,
	HISI_JPEG_DECODE_MODE_MAX,
} jpu_decode_mode;

typedef struct component_info {
	uint32_t s32ComponentId;
	uint32_t s32ComponentIndex;
	uint32_t s32QuantTblNo;
	uint32_t s32DcTblNo;
	uint32_t s32AcTblNo;
	uint8_t u8HorSampleFac;
	uint8_t u8VerSampleFac;
	uint16_t reserved0;
} component_info_t;


typedef struct jpu_data {
	uint32_t inwidth; /* full decode input width */
	uint32_t inheight; /* full decode input height */
	uint32_t pix_width; /* aligned to mcu width */
	uint32_t pix_height; /* aligned to mcu height */
	uint32_t num_components;
	hisi_jpeg_decode_sample_size sample_rate; /* 1, 1/2, 1/4, 1/8 */
	uint64_t start_addr; /* stream start addr */
	uint64_t end_addr; /* stream end addr */

	uint64_t start_addr_y; /* start address for planar Y or RGB of output buffer */
	uint64_t last_page_y; /* last 32KB page for planar Y or RGB of output buffer */
	uint64_t start_addr_c; /* start address for planar UV or RGB of output buffer */
	uint64_t last_page_c; /* last 32KB page for planar UV or RGB of output buffer */
	uint32_t stride_y; /* stride for planar Y or RGBoutput buffer */
	uint32_t stride_c; /* stride for planar c or RGBoutput buffer */
	uint32_t restart_interval;
	uint32_t addr_offset; /* start of scan data offset based start_addr*/

	bool arith_code; /* TRUE=arithmetic coding, FALSE=Huffman */
	bool progressive_mode;
	bool smmu_enable;

	jpu_decode_mode decode_mode;
	jpu_color_space out_color_format;
	jpu_raw_format in_img_format;
	jpu_region_info region_info;

	component_info_t component_info[PIXEL_COMPONENT_NUM];
	int32_t qvalue[MAX_DCT_SIZE];
	uint32_t hdc_tab[HDC_TABLE_NUM];
	uint32_t hac_min_tab[HAC_TABLE_NUM];
	uint32_t hac_base_tab[HAC_TABLE_NUM];
	uint32_t hac_symbol_tab[HAC_SYMBOL_TABLE_NUM];
	int in_sharefd;
	int out_sharefd;
} jpu_data_t;


#endif /* __HISI_JPU_COMMON_H__ */
