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
#include <linux/delay.h>
#include <linux/version.h>
#include <media/camera/jpeg/jpeg_base.h>
#include "hisifb_ion.h"
#include "hisi_jpu.h"
#include <linux/device.h>

#define MAX_INPUT_DATA_LEN (8192*8192*4)

/******************************************************************************
**
*/
uint32_t jpu_set_bits32(uint32_t old_val, uint32_t val, uint8_t bw, uint8_t bs)
{
	uint32_t mask = (uint32_t)((1UL << bw) - 1UL);
	uint32_t tmp = old_val;

	tmp &= ~(mask << bs);

	return (tmp | ((val & mask) << bs));
}

/*lint -save -e438 -e550 -e715 -e747 -e774 -e778*/
void hisijpu_set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs)
{
	uint32_t mask = (uint32_t)((1UL << bw) - 1UL);
	uint32_t tmp;

	tmp = (uint32_t)inp32(addr);
	tmp &= ~(mask << bs);

	outp32(addr, tmp | ((val & mask) << bs));
}

void jpu_get_timestamp(struct timeval *tv)
{
	struct timespec ts;

	if (!tv)
		return;

	ktime_get_ts(&ts);
	tv->tv_sec = ts.tv_sec;
	tv->tv_usec = ts.tv_nsec / NSEC_PER_USEC;
}

long jpu_timestamp_diff(struct timeval *lasttime, struct timeval *curtime)
{
	if (!lasttime || !curtime)
		return 0;

	return (curtime->tv_usec >= lasttime->tv_usec) ?
		curtime->tv_usec - lasttime->tv_usec:
		1000000 - (lasttime->tv_usec - curtime->tv_usec);
}


/*******************************************************************************
**
*/
static int hisi_sample_size_hal2jpu(int val)
{
	int ret = 0;

	switch (val) {
	case HISI_JPEG_DECODE_SAMPLE_SIZE_1:
		ret = JPU_FREQ_SCALE_1;
		break;
	case HISI_JPEG_DECODE_SAMPLE_SIZE_2:
		ret = JPU_FREQ_SCALE_2;
		break;
	case HISI_JPEG_DECODE_SAMPLE_SIZE_4:
		ret = JPU_FREQ_SCALE_4;
		break;
	case HISI_JPEG_DECODE_SAMPLE_SIZE_8:
		ret = JPU_FREQ_SCALE_8;
		break;

	default:
		HISI_JPU_ERR("not support sample size(%d)!\n", val);
		ret = -1;
		break;
	}
	HISI_JPU_INFO("sample size(%d)!\n", val);

	return ret;
}

static bool isRGB_out(uint32_t format)
{
	switch (format) {
	case HISI_JPEG_DECODE_OUT_RGBA4444:
	case HISI_JPEG_DECODE_OUT_BGRA4444:
	case HISI_JPEG_DECODE_OUT_RGB565:
	case HISI_JPEG_DECODE_OUT_BGR565:
	case HISI_JPEG_DECODE_OUT_RGBA8888:
	case HISI_JPEG_DECODE_OUT_BGRA8888:
		return true;

	default:
		return false;
	}
}

bool is_original_format(jpu_raw_format in_format, jpu_color_space out_format) {
	if (((HISI_JPEG_DECODE_RAW_YUV444 == in_format) && (HISI_JPEG_DECODE_OUT_YUV444 == out_format))
		|| ((HISI_JPEG_DECODE_RAW_YUV422_H2V1 == in_format) &&( HISI_JPEG_DECODE_OUT_YUV422_H2V1 == out_format))
		|| ((HISI_JPEG_DECODE_RAW_YUV422_H1V2 == in_format) && ( HISI_JPEG_DECODE_OUT_YUV422_H1V2 == out_format))
		|| (( HISI_JPEG_DECODE_RAW_YUV420 == in_format) && (HISI_JPEG_DECODE_OUT_YUV420 == out_format))
		|| (( HISI_JPEG_DECODE_RAW_YUV400 == in_format) && (HISI_JPEG_DECODE_OUT_YUV400 == out_format))){
		return true;
	}

	return false;
}

jpu_output_format hisi_out_format_hal2jpu(struct hisi_jpu_data_type *hisijd)
{
	jpu_data_t *jpu_req = NULL;
	jpu_output_format format  = JPU_OUTPUT_UNSUPPORT;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return format;
	}

	jpu_req = &(hisijd->jpu_req);

	if (is_original_format(jpu_req->in_img_format, jpu_req->out_color_format)) {
		return JPU_OUTPUT_YUV_ORIGINAL;
	}

	/* YUV400 can't decode to yuv420 */
	if ((HISI_JPEG_DECODE_RAW_YUV400 == jpu_req->in_img_format)
		&& (HISI_JPEG_DECODE_OUT_YUV420 == jpu_req->out_color_format)) {
		return format;
	}

	switch (jpu_req->out_color_format) {
	case HISI_JPEG_DECODE_OUT_YUV420:
		format =  JPU_OUTPUT_YUV420;
		break;
	case HISI_JPEG_DECODE_OUT_RGBA4444:
		format =  JPU_OUTPUT_RGBA4444;
		break;
	case HISI_JPEG_DECODE_OUT_BGRA4444:
		format =  JPU_OUTPUT_BGRA4444;
		break;
	case HISI_JPEG_DECODE_OUT_RGB565:
		format =  JPU_OUTPUT_RGB565;
		break;
	case HISI_JPEG_DECODE_OUT_BGR565:
		format =  JPU_OUTPUT_BGR565;
		break;
	case HISI_JPEG_DECODE_OUT_RGBA8888:
		format =  JPU_OUTPUT_RGBA8888;
		break;

	case HISI_JPEG_DECODE_OUT_BGRA8888:
		format =  JPU_OUTPUT_BGRA8888;
		break;
	default:
		break;
	}

	return format;
}

static int hisijpu_set_dht(jpu_data_t *jpu_req, char __iomem *jpu_dec_base)
{
	int i = 0;

	if (!jpu_req) {
		HISI_JPU_ERR("pjpu_req is NULL!\n");
		return -EINVAL;
	}

	if (!jpu_dec_base) {
		HISI_JPU_ERR("jpu_dec_base is NULL!\n");
		return -EINVAL;
	}

	for (i = 0; i < HDC_TABLE_NUM; i++) {
		outp32(jpu_dec_base + JPGD_REG_HDCTABLE + 4 * i, jpu_req->hdc_tab[i]);
		HISI_JPU_DEBUG("hdc_tab[%d]%d\n", i, jpu_req->hdc_tab[i]);
	}

	for (i = 0; i < HAC_TABLE_NUM; i++) {
		outp32(jpu_dec_base + JPGD_REG_HACMINTABLE + 4 * i,  jpu_req->hac_min_tab[i] );
		outp32(jpu_dec_base + JPGD_REG_HACBASETABLE + 4 * i,  jpu_req->hac_base_tab[i]);
		HISI_JPU_DEBUG(" hac_min_tab[%d] %ul\n", i, jpu_req->hac_min_tab[i]);
		HISI_JPU_DEBUG(" hac_base_tab[%d] %ul\n", i, jpu_req->hac_base_tab[i]);
	}

	for (i = 0; i < HAC_SYMBOL_TABLE_NUM; i++) {
		outp32(jpu_dec_base + JPGD_REG_HACSYMTABLE + 4 * i,  jpu_req->hac_symbol_tab[i]);
		HISI_JPU_DEBUG(" gs_hac_symbol_tab[%d] %ul\n", i, jpu_req->hac_symbol_tab[i]);
	}

	return 0;
}

static void hisijpu_set_dqt(jpu_data_t *jpu_req, char __iomem *jpu_dec_base)
{
	uint32_t i = 0;

	if (!jpu_req) {
		HISI_JPU_ERR("pjpu_req is NULL!\n");
		return;
	}

	if (!jpu_dec_base) {
		HISI_JPU_ERR("jpu_dec_base is NULL!\n");
		return;
	}

	for (i = 0; i < MAX_DCT_SIZE; i++) {
		outp32(jpu_dec_base + JPGD_REG_QUANT + 4 * i,  jpu_req->qvalue[i]);
	}

	return;
}

static int hisijpu_set_crop(struct hisi_jpu_data_type *hisijd)
{
	jpu_data_t *jpu_req = NULL;
	jpu_dec_reg_t *pjpu_dec_reg = NULL;

	if (!hisijd) {
		HISI_JPU_ERR("pjpu_req or hisijd null");
		return -EINVAL;
	}

	jpu_req = &(hisijd->jpu_req);
	pjpu_dec_reg = &(hisijd->jpu_dec_reg);

	/* for full decode */
	if (HISI_JPEG_DECODE_MODE_FULL_SUB >= jpu_req->decode_mode) {
		pjpu_dec_reg->crop_horizontal = jpu_set_bits32(pjpu_dec_reg->crop_horizontal,
			(jpu_req->inwidth - 1) << 16, 32, 0);
		pjpu_dec_reg->crop_vertical = jpu_set_bits32(pjpu_dec_reg->crop_vertical,
			(jpu_req->inheight- 1) << 16, 32, 0);
	} else {
		if ((jpu_req->region_info.right < MIN_INPUT_WIDTH) ||
			(jpu_req->region_info.bottom < MIN_INPUT_WIDTH)) {
			return -EINVAL;
		}
		/*lint -save -e737*/
		pjpu_dec_reg->crop_horizontal = jpu_set_bits32(pjpu_dec_reg->crop_horizontal,
			(((uint32_t)(jpu_req->region_info.right - 1)) << 16) | (jpu_req->region_info.left ), 32, 0);
		pjpu_dec_reg->crop_vertical = jpu_set_bits32(pjpu_dec_reg->crop_vertical,
			(((uint32_t)(jpu_req->region_info.bottom - 1)) << 16) | (jpu_req->region_info.top), 32, 0);
		/*lint -restore*/
	}

	return 0;
}


/*******************************************************************************
**
*/
int hisijpu_irq_request(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	/*lint -save -e747*/
	if (hisijd->jpu_done_irq) {
		ret = request_irq(hisijd->jpu_done_irq, hisi_jpu_dec_done_isr,
			0, IRQ_JPU_DEC_DONE_NAME, (void *)hisijd);
		if (ret != 0) {
			HISI_JPU_ERR("request_irq failed, irq_no=%d error=%d!\n",
				hisijd->jpu_done_irq, ret);
			return ret;
		} else {
			HISI_JPU_INFO("request_irq OK disable, irq_no=%d error=%d!\n",
				hisijd->jpu_done_irq, ret);
			disable_irq(hisijd->jpu_done_irq);
		}
	}

	if (hisijd->jpu_err_irq) {
		ret = request_irq(hisijd->jpu_err_irq, hisi_jpu_dec_err_isr,
			0, IRQ_JPU_DEC_ERR_NAME, (void *)hisijd);
		if (ret != 0) {
			HISI_JPU_ERR("request_irq failed, irq_no=%d error=%d!\n",
				hisijd->jpu_err_irq, ret);
			return ret;
		} else {
			disable_irq(hisijd->jpu_err_irq);
		}
	}

	if (hisijd->jpu_other_irq) {
		ret = request_irq(hisijd->jpu_other_irq, hisi_jpu_dec_other_isr,
			0, IRQ_JPU_DEC_OTHER_NAME, (void *)hisijd);
		if (ret != 0) {
			HISI_JPU_ERR("request_irq failed, irq_no=%d error=%d!\n",
				hisijd->jpu_other_irq, ret);
			return ret;
		} else {
			disable_irq(hisijd->jpu_other_irq);
		}
	}
	/*lint -restore*/

	return ret;
}

int hisijpu_irq_free(struct hisi_jpu_data_type *hisijd)
{
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (hisijd->jpu_done_irq) {
		free_irq(hisijd->jpu_done_irq, 0);
		hisijd->jpu_done_irq = 0;
	}

	if (hisijd->jpu_err_irq) {
		free_irq(hisijd->jpu_err_irq, 0);
		hisijd->jpu_err_irq = 0;
	}

	if (hisijd->jpu_other_irq) {
		free_irq(hisijd->jpu_other_irq, 0);
		hisijd->jpu_other_irq = 0;
	}

	return 0;
}

int hisijpu_irq_enable(struct hisi_jpu_data_type *hisijd)
{
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (hisijd->jpu_done_irq)
		enable_irq(hisijd->jpu_done_irq);

	if (hisijd->jpu_err_irq)
		enable_irq(hisijd->jpu_err_irq);

	if (hisijd->jpu_other_irq)
		enable_irq(hisijd->jpu_other_irq);

	return 0;
}

int hisijpu_irq_disable(struct hisi_jpu_data_type *hisijd)
{
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (hisijd->jpu_err_irq)
		disable_irq(hisijd->jpu_err_irq);

	if (hisijd->jpu_done_irq)
		disable_irq(hisijd->jpu_done_irq);

	if (hisijd->jpu_other_irq)
		disable_irq(hisijd->jpu_other_irq);

	return 0;
}

int hisijpu_irq_disable_nosync(struct hisi_jpu_data_type *hisijd)
{
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (hisijd->jpu_err_irq)
		disable_irq_nosync(hisijd->jpu_err_irq);

	if (hisijd->jpu_done_irq)
		disable_irq_nosync(hisijd->jpu_done_irq);

	if (hisijd->jpu_other_irq)
		disable_irq_nosync(hisijd->jpu_other_irq);

	return 0;
}

/* lint -save -e838 */
int hisijpu_media1_regulator_enable(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	HISI_JPU_DEBUG("+.\n");

	ret = regulator_enable(hisijd->media1_regulator);
	if (ret) {
		HISI_JPU_ERR("jpu media1_regulator failed, error=%d!\n", ret);
	}

	HISI_JPU_DEBUG("-.\n");

	return ret;
}

/* lint -save -e838 */
int hisijpu_jpu_regulator_enable(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	HISI_JPU_DEBUG("+.\n");

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	ret = regulator_enable(hisijd->jpu_regulator);
	if (ret) {
		HISI_JPU_ERR("jpu regulator_enable failed, error=%d!\n", ret);
	}

	HISI_JPU_DEBUG("-.\n");

	return ret;
}

int hisijpu_jpu_regulator_disable(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	HISI_JPU_DEBUG("+.\n");

	ret = regulator_disable(hisijd->jpu_regulator);
	if (ret != 0) {
		HISI_JPU_ERR("jpu regulator_disable failed, error=%d!\n", ret);
	}

	HISI_JPU_DEBUG("-.\n");

	return ret;
}
/* lint -restore */

int hisijpu_media1_regulator_disable(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	HISI_JPU_DEBUG("+.\n");

	ret = regulator_disable(hisijd->media1_regulator);
	if (ret != 0) {
		HISI_JPU_ERR("jpu regulator_disable failed, error=%d!\n", ret);
	}

	HISI_JPU_DEBUG("-.\n");

	return ret;
}
/* lint -restore */

int hisijpu_clk_enable(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	HISI_JPU_DEBUG("+.\n");

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!hisijd->jpg_func_clk) {
		HISI_JPU_ERR("jpg_func_clk is NULL!\n");
		return -EINVAL;
	}

	if (hisijd->jpu_support_platform == HISI_KIRIN_970) {
		HISI_JPU_DEBUG("jpg func clk default rate is: %ld.\n", JPG_FUNC_CLK_DEFAULT_RATE);
		ret = jpeg_dec_set_rate(hisijd->jpg_func_clk, JPG_FUNC_CLK_DEFAULT_RATE);
	}else if (hisijd->jpu_support_platform == HISI_DSS_V500
		|| hisijd->jpu_support_platform == HISI_DSS_V501) {
		HISI_JPU_DEBUG("jpg func clk default rate is: %ld.\n", JPG_FUNC_CLK_DEFAULT_RATE_V501);
		ret = jpeg_dec_set_rate(hisijd->jpg_func_clk, JPG_FUNC_CLK_DEFAULT_RATE_V501);
	}else {
		HISI_JPU_ERR("jpg_func_clk set clk failed, unsupport platform!\n");
		return -EINVAL;
	}

	if (ret) {
		HISI_JPU_ERR("jpg_func_clk set clk failed, error=%d!\n", ret);
		goto TRY_LOW_FREQ;
	}

	ret = jpeg_dec_clk_prepare_enable(hisijd->jpg_func_clk);
	if (ret) {
		HISI_JPU_ERR("jpg_func_clk clk_prepare failed, error=%d!\n", ret);
		goto TRY_LOW_FREQ;
	}

	HISI_JPU_DEBUG("-.\n");

	return 0;

TRY_LOW_FREQ:


	if (hisijd->jpu_support_platform == HISI_DSS_V501) {
		HISI_JPU_INFO("jpg func clk low freq rate is: %ld.\n", JPG_FUNC_CLK_LOW_TEMP_V501);
		ret = jpeg_dec_set_rate(hisijd->jpg_func_clk, JPG_FUNC_CLK_LOW_TEMP_V501);
	} else {
		HISI_JPU_INFO("jpg func clk low freq rate is: %ld.\n", JPG_FUNC_CLK_LOW_TEMP);
		ret = jpeg_dec_set_rate(hisijd->jpg_func_clk, JPG_FUNC_CLK_LOW_TEMP);
	}

	if (ret) {
		HISI_JPU_ERR("jpg_func_clk set clk failed, error=%d!\n", ret);
		return -EINVAL;
	}

	ret = jpeg_dec_clk_prepare_enable(hisijd->jpg_func_clk);
	if (ret) {
		HISI_JPU_ERR("jpg_func_clk clk_prepare failed, error=%d!\n", ret);
		return -EINVAL;
	}

	HISI_JPU_DEBUG("-.\n");

	return 0;
}

int hisijpu_clk_disable(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!hisijd->jpg_func_clk) {
		HISI_JPU_ERR("jpg_func_clk is NULL!\n");
		return -EINVAL;
	}

	HISI_JPU_DEBUG("+.\n");

	jpeg_dec_clk_disable_unprepare(hisijd->jpg_func_clk);

	ret = jpeg_dec_set_rate(hisijd->jpg_func_clk, JPG_FUNC_CLK_PD_RATE);

	if (ret != 0) {
	    HISI_JPU_ERR("fail to set power down rate, ret=%d!\n", ret);
	    return -EINVAL;
	}

	HISI_JPU_DEBUG("-.\n");

	return 0;
}

static int hisijpu_enable_iommu(struct hisi_jpu_data_type *hisijd)
{
	struct iommu_domain *domain = NULL;

	if(!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	/* create iommu domain */
	domain = hisi_ion_enable_iommu(NULL);
	if (!domain) {
		HISI_JPU_ERR("failed to hisi_ion_enable_iommu!\n");
		return -EINVAL;
	}

	hisijd->jpu_domain = domain;

	return 0;
}

static int hisi_jpu_lb_alloc(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;
	size_t lb_size = 0;

	if (!hisijd || !(hisijd->pdev)) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!hisijd->ion_client) {
		HISI_JPU_ERR("ion_client is NULL!\n");
		return -EINVAL;
	}

	lb_size = JPU_LB_SIZE;

	/*alloc jpu dec lb buffer, start end shoulf 32KB align*/
	hisijd->lb_ion_handle = ion_alloc(hisijd->ion_client, lb_size,
		0, ION_HEAP(ION_SYSTEM_HEAP_ID), 0);
	if (IS_ERR_OR_NULL(hisijd->lb_ion_handle)) {
		HISI_JPU_ERR("failed to ion alloc lb_ion_handle!");
		ret = -ENOMEM;
		goto err_ion_handle;
	}

	if (!ion_map_kernel(hisijd->ion_client, hisijd->lb_ion_handle)) {
		HISI_JPU_ERR("failed to ion_map_kernel!");
		ret = -ENOMEM;
		goto err_map_kernel;
	}

	if(ion_map_iommu(hisijd->ion_client, hisijd->lb_ion_handle, &(hisijd->iommu_format))) {
		ret = -ENOMEM;
		HISI_JPU_ERR("failed to ion_map_iommu!");
		goto err_map_iommu;
	}
	hisijd->lb_addr = (uint32_t)(hisijd->iommu_format.iova_start >> 4);

	/* start address for line buffer, unit is 16 byte, must align to 128 byte */
	if (hisijd->lb_addr & (JPU_LB_ADDR_ALIGN - 1)) {
		HISI_JPU_ERR("lb_addr(0x%x) is not %d bytes aligned!\n",
			hisijd->lb_addr, JPU_LB_ADDR_ALIGN - 1);
		ret = -EINVAL;
		goto err_addr;
	}

	HISI_JPU_INFO("lb_size=%zu, hisijd->lb_addr 0x%x\n",lb_size, hisijd->lb_addr);
	return 0;

err_addr:
	ion_unmap_iommu(hisijd->ion_client, hisijd->lb_ion_handle);

err_map_iommu:
	if (hisijd->lb_ion_handle) {
		ion_unmap_kernel(hisijd->ion_client, hisijd->lb_ion_handle);
	}

err_map_kernel:
	if (hisijd->lb_ion_handle) {
		ion_free(hisijd->ion_client, hisijd->lb_ion_handle);
		hisijd->lb_ion_handle = NULL;
	}

err_ion_handle:
	return ret;
}

static void hisi_jpu_lb_free(struct hisi_jpu_data_type *hisijd)
{
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return ;
	}

	if (hisijd->ion_client && hisijd->lb_ion_handle) {
		ion_unmap_iommu(hisijd->ion_client, hisijd->lb_ion_handle);
		ion_unmap_kernel(hisijd->ion_client, hisijd->lb_ion_handle);
		ion_free(hisijd->ion_client, hisijd->lb_ion_handle);
		hisijd->lb_ion_handle = NULL;
	}
}

int hisi_jpu_register(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd || !hisijd->pdev) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	hisijd->jpu_dec_done_flag = 0;
	init_waitqueue_head(&hisijd->jpu_dec_done_wq);

	sema_init(&hisijd->blank_sem, 1);
	hisijd->power_on = false;
	hisijd->jpu_res_initialized = false;
	hisijd->ion_client = hisi_ion_client_create(HISI_JPU_ION_CLIENT_NAME);

	if (IS_ERR_OR_NULL(hisijd->ion_client)) {
		dev_err(&hisijd->pdev->dev, "failed to create ion client!\n");
		return -ENOMEM;
	}

	ret = hisijpu_enable_iommu(hisijd);
	if (ret != 0) {
		dev_err(&hisijd->pdev->dev, "hisijpu_enable_iommu failed!\n");
		return ret;
	}

	ret = hisi_jpu_lb_alloc(hisijd);
	if (ret != 0) {
		dev_err(&hisijd->pdev->dev, "hisi_jpu_lb_alloc failed!\n");
		return ret;
	}

	ret = hisijpu_irq_request(hisijd);
	if (ret) {
		dev_err(&hisijd->pdev->dev, "hisijpu_irq_request failed!");
		return ret;
	}

	return 0;
}

int hisi_jpu_unregister(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}
	if (!hisijd->pdev) {
		return -EINVAL;
	}
	ret = hisijpu_irq_disable(hisijd);
	if (ret) {
		dev_err(&hisijd->pdev->dev, "hisijpu_irq_disable failed!\n");
		return -EINVAL;
	}

	ret = hisijpu_irq_free(hisijd);
	if (ret) {
		dev_err(&hisijd->pdev->dev, "hisijpu_irq_free failed!\n");
		return -EINVAL;
	}

	hisi_jpu_lb_free(hisijd);

	if (hisijd->ion_client) {
		ion_client_destroy(hisijd->ion_client);
		hisijd->ion_client = NULL;
	}

	return 0;
}


/*******************************************************************************
**
*/
static int hisijpu_check_reg_state(char __iomem *addr, uint32_t val)
{
	uint32_t tmp = 0;
	int delay_count = 0;
	bool is_timeout = true;

	if (!addr) {
		return -1;
	}

	while (1) {
		tmp = (uint32_t)inp32(addr);
		if(((tmp & val) == val) || (delay_count > 10)) {
			is_timeout = (delay_count > 10) ? true : false;
			udelay(10);
			break;
		} else {
			udelay(10);
			++delay_count;
		}
	}

	if (is_timeout) {
		HISI_JPU_ERR("fail to wait reg!\n");
		return -1;
	}

	return 0;
}
/*lint -restore*/

void hisi_jpu_dec_normal_reset(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return;
	}

	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS, 0x1, 1, 25);
		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS, 0x01000000);
		if (ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS!\n");
		}

		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS, 0x0, 1, 25);
	} else {
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1, 0x1, 1, 25);
		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1, 0x01000000);
		if (ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1!\n");
		}

		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1, 0x0, 1, 25);
	}
}

void hisi_jpu_dec_error_reset(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return;
	}

	/* step1 */
	hisijpu_set_reg(hisijd->jpu_top_base + JPGDEC_CRG_CFG1, 0x00010000, 32, 0);
	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS, 0x1, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_9_CS, 0x1, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_8_CS, 0x1, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_9_CS, 0x1, 1, 25);
	} else {
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1, 0x1, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_2, 0x1, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0, 0x1, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1, 0x1, 1, 25);
	}

	/* step2 */
	ret = hisijpu_check_reg_state(hisijd->jpu_top_base + JPGDEC_RO_STATE, 0x2);
	if(ret) {
		HISI_JPU_ERR("fail to wait JPGDEC_RO_STATE!\n");
	}
	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS, 0x01000000);
		if(ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS!\n");
		}

		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_9_CS, 0x01000000);
		if(ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_9_CS!\n");
		}

		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_8_CS, 0x01000000);
		if(ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_8_CS!\n");
		}

		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_9_CS, 0x01000000);
		if(ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_9_CS!\n");
		}
	} else {
		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1, 0x01000000);
		if(ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1!\n");
		}

		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_2, 0x01000000);
		if(ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_2!\n");
		}

		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0, 0x01000000);
		if(ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0!\n");
		}

		ret = hisijpu_check_reg_state(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1, 0x01000000);
		if(ret) {
			HISI_JPU_ERR("fail to wait JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1!\n");
		}
	}
	/* step3,read bit0 is 1*/
	hisijpu_set_reg(hisijd->jpu_top_base + JPGDEC_CRG_CFG1, 1, 1, 0);
	ret = hisijpu_check_reg_state(hisijd->jpu_top_base + JPGDEC_CRG_CFG1, 0x1);
	if(ret) {
		HISI_JPU_ERR("fail to wait JPGDEC_CRG_CFG1!\n");
	}

	/* step4 */
	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS, 0x0, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_9_CS, 0x0, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_8_CS, 0x0, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_9_CS, 0x0, 1, 25);
	} else {
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1, 0x0, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_2, 0x0, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0, 0x0, 1, 25);
		hisijpu_set_reg(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1, 0x0, 1, 25);
	}
	hisijpu_set_reg(hisijd->jpu_top_base + JPGDEC_CRG_CFG1, 0x0, 32, 0);
}

void hisijpu_smmu_on(struct hisi_jpu_data_type *hisijd)
{
	uint32_t phy_pgd_base = 0;
	struct iommu_domain_data *domain_data = NULL;
	uint32_t fama_ptw_msb = 0;

	if(!hisijd){
		HISI_JPU_ERR("hisijd is NULL!\n");
		return;
	}

	HISI_JPU_DEBUG("+.\n");

	HISI_JPU_DEBUG("CONFIG_JPU_SMMU_ENABLE.\n");
	/*
	** Set global mode:
	** SMMU_SCR_S.glb_nscfg = 0x3
	** SMMU_SCR_P.glb_prot_cfg = 0x0
	** SMMU_SCR.glb_bypass = 0x0
	*/
	/*set_reg(smmu_base + SMMU_SCR_S, 0x3, 2, 0);*/
	/*set_reg(smmu_base + SMMU_SCR_P, 0x0, 1, 0);*/
	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_SCR, 0x0, 1, 0);

	/* for performance Ptw_mid: 0x1d, Ptw_pf: 0x1 */
	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_SCR, 0x1, 4, 16);

	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_SCR, 0x1d, 6, 20);

	/*
	** Set interrupt mode:
	** Clear all interrupt state: SMMU_INCLR_NS = 0xFF
	** Enable interrupt: SMMU_INTMASK_NS = 0x0
	*/
	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_INTRAW_NS, 0xff, 32, 0);
	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_INTMASK_NS, 0x0, 32, 0);

	/*
	** Set non-secure pagetable addr:
	** SMMU_CB_TTBR0 = non-secure pagetable address
	** Set non-secure pagetable type:
	** SMMU_CB_TTBCR.cb_ttbcr_des= 0x1 (long descriptor)
	*/
	if(!hisijd->jpu_domain) {
		HISI_JPU_ERR("jpu_domain null.\n");
		return;
	}
	domain_data = (struct iommu_domain_data *)(hisijd->jpu_domain->priv);

	phy_pgd_base = (uint32_t)(domain_data->phy_pgd_base);
	fama_ptw_msb = (domain_data->phy_pgd_base >> 32) & 0x0000007F;

	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_CB_TTBR0, phy_pgd_base, 32, 0);
	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_CB_TTBCR, 0x1, 1, 0);

	/* FAMA configuration */
	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_FAMA_CTRL0, 0x80, 14, 0);
	hisijpu_set_reg(hisijd->jpu_smmu_base + SMMU_FAMA_CTRL1, fama_ptw_msb, 7, 0);
	HISI_JPU_DEBUG("-.\n");

}

int hisi_jpu_on(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if(hisijd->power_on) {
		HISI_JPU_DEBUG("hisijd has already power_on!\n");
		return ret;
	}

	/* step 1 mediasubsys enable */
	ret = hisijpu_media1_regulator_enable(hisijd);
	if (ret) {
		HISI_JPU_ERR("media1_regulator_enable fail!\n");
		return ret;
	}

	/* step 2 clk_enable */
	ret = hisijpu_clk_enable(hisijd);
	if(ret) {
		HISI_JPU_ERR("hisijpu_clk_enable fail!!\n");
		hisijpu_media1_regulator_disable(hisijd);
		return ret;
	}

	/* step 3 regulator_enable ispsubsys*/
	ret = hisijpu_jpu_regulator_enable(hisijd);
	if(ret) {
		HISI_JPU_ERR("jpu_regulator_enable fail!\n");
		hisijpu_clk_disable(hisijd);
		hisijpu_media1_regulator_disable(hisijd);
		return ret;
	}

	/* step 4 jpeg decoder inside clock enable */
	outp32(hisijd->jpu_top_base + JPGDEC_CRG_CFG0, 0x1);

	if (hisijd->jpu_support_platform == HISI_KIRIN_970) {
		outp32(hisijd->jpu_top_base + JPGDEC_MEM_CFG, 0x02605550);
	}

	hisijpu_smmu_on(hisijd);

	hisi_jpu_dec_interrupt_mask(hisijd);
	hisi_jpu_dec_interrupt_clear(hisijd);

	ret = hisijpu_irq_enable(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisijpu_irq_enable failed!\n");
		return -EINVAL;
	}

	hisi_jpu_dec_interrupt_unmask(hisijd);

	hisijd->power_on = true;

	return ret;
}

int hisi_jpu_off(struct hisi_jpu_data_type *hisijd)
{
	int ret = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if(!hisijd->power_on) {
		HISI_JPU_DEBUG("hisijd has already power off!\n");
		return ret;
	}

	hisi_jpu_dec_interrupt_mask(hisijd);
	ret = hisijpu_irq_disable(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisijpu_irq_disable failed!\n");
		return -EINVAL;
	}

	/* jpeg decoder inside clock disable */
	outp32(hisijd->jpu_top_base + JPGDEC_CRG_CFG0, 0x0);

	/* ispsubsys regulator disable */
	ret = hisijpu_jpu_regulator_disable(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisijpu_jpu_regulator_disable failed!\n");
		return -EINVAL;
	}

	/* clk disable*/
	ret = hisijpu_clk_disable(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisijpu_clk_disable failed!\n");
		return -EINVAL;
	}

	/* media disable */
	ret = hisijpu_media1_regulator_disable(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisijpu_media1_regulator_disable failed!\n");
		return -EINVAL;
	}

	hisijd->power_on = false;

	return ret;
}


/*******************************************************************************
**
*/

/*lint -save -e774 -e438 -e550 -e732*/
static int hisi_jpu_dec_reg_default(struct hisi_jpu_data_type *hisijd)
{
	jpu_dec_reg_t *jpu_dec_reg = NULL;
	char __iomem *jpu_dec_base = NULL;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!hisijd->jpu_dec_base) {
		HISI_JPU_ERR("jpu_dec_base is NULL!\n");
		return -EINVAL;
	}

	jpu_dec_base = hisijd->jpu_dec_base;
	jpu_dec_reg = &(hisijd->jpu_dec_reg_default);

	/* cppcheck-suppress * */
	memset(jpu_dec_reg, 0, sizeof(jpu_dec_reg_t));

	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		jpu_dec_reg->dec_start = inp32(jpu_dec_base + JPEG_DEC_START_CS);
		jpu_dec_reg->preftch_ctrl = inp32(jpu_dec_base + JPEG_DEC_PREFTCH_CTRL_CS);
		jpu_dec_reg->frame_size = inp32(jpu_dec_base + JPEG_DEC_FRAME_SIZE_CS);
		jpu_dec_reg->crop_horizontal = inp32(jpu_dec_base + JPEG_DEC_CROP_HORIZONTAL_CS);
		jpu_dec_reg->crop_vertical = inp32(jpu_dec_base + JPEG_DEC_CROP_VERTICAL_CS);
		jpu_dec_reg->bitstreams_start_h = inp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_START_H_CS);
		jpu_dec_reg->bitstreams_start = inp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_START_L_CS);
		jpu_dec_reg->bitstreams_end_h = inp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_END_H_CS);
		jpu_dec_reg->bitstreams_end = inp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_END_L_CS);
		jpu_dec_reg->frame_start_y = inp32(jpu_dec_base + JPEG_DEC_FRAME_START_Y_CS);
		jpu_dec_reg->frame_stride_y = inp32(jpu_dec_base + JPEG_DEC_FRAME_STRIDE_Y_CS);
		jpu_dec_reg->frame_start_c = inp32(jpu_dec_base + JPEG_DEC_FRAME_START_C_CS);
		jpu_dec_reg->frame_stride_c = inp32(jpu_dec_base + JPEG_DEC_FRAME_STRIDE_C_CS);
		jpu_dec_reg->lbuf_start_addr = inp32(jpu_dec_base + JPEG_DEC_LBUF_START_ADDR_CS);
		jpu_dec_reg->output_type = inp32(jpu_dec_base + JPEG_DEC_OUTPUT_TYPE_CS);
		jpu_dec_reg->freq_scale = inp32(jpu_dec_base + JPEG_DEC_FREQ_SCALE_CS);
		jpu_dec_reg->middle_filter = inp32(jpu_dec_base + JPEG_DEC_MIDDLE_FILTER_CS);
		jpu_dec_reg->sampling_factor = inp32(jpu_dec_base + JPEG_DEC_SAMPLING_FACTOR_CS);
		jpu_dec_reg->dri = inp32(jpu_dec_base + JPEG_DEC_DRI_CS);
		jpu_dec_reg->over_time_thd = inp32(jpu_dec_base + JPEG_DEC_OVER_TIME_THD_CS);
		jpu_dec_reg->hor_phase0_coef01 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF01_CS);
		jpu_dec_reg->hor_phase0_coef23 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF23_CS);
		jpu_dec_reg->hor_phase0_coef45 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF45_CS);
		jpu_dec_reg->hor_phase0_coef67 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF67_CS);
		jpu_dec_reg->hor_phase2_coef01 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF01_CS);
		jpu_dec_reg->hor_phase2_coef23 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF23_CS);
		jpu_dec_reg->hor_phase2_coef45 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF45_CS);
		jpu_dec_reg->hor_phase2_coef67 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF67_CS);
		jpu_dec_reg->ver_phase0_coef01 = inp32(jpu_dec_base + JPEG_DEC_VER_PHASE0_COEF01_CS);
		jpu_dec_reg->ver_phase0_coef23 = inp32(jpu_dec_base + JPEG_DEC_VER_PHASE0_COEF23_CS);
		jpu_dec_reg->ver_phase2_coef01 = inp32(jpu_dec_base + JPEG_DEC_VER_PHASE2_COEF01_CS);
		jpu_dec_reg->ver_phase2_coef23 = inp32(jpu_dec_base + JPEG_DEC_VER_PHASE2_COEF23_CS);
		jpu_dec_reg->csc_in_dc_coef = inp32(jpu_dec_base + JPEG_DEC_CSC_IN_DC_COEF_CS);
		jpu_dec_reg->csc_out_dc_coef = inp32(jpu_dec_base + JPEG_DEC_CSC_OUT_DC_COEF_CS);
		jpu_dec_reg->csc_trans_coef0 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF0_CS);
		jpu_dec_reg->csc_trans_coef1 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF1_CS);
		jpu_dec_reg->csc_trans_coef2 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF2_CS);
		jpu_dec_reg->csc_trans_coef3 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF3_CS);
		jpu_dec_reg->csc_trans_coef4 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF4_CS);

	} else {
		jpu_dec_reg->dec_start = inp32(jpu_dec_base + JPEG_DEC_START);
		jpu_dec_reg->preftch_ctrl = inp32(jpu_dec_base + JPEG_DEC_PREFTCH_CTRL);
		jpu_dec_reg->frame_size = inp32(jpu_dec_base + JPEG_DEC_FRAME_SIZE);
		jpu_dec_reg->crop_horizontal = inp32(jpu_dec_base + JPEG_DEC_CROP_HORIZONTAL);
		jpu_dec_reg->crop_vertical = inp32(jpu_dec_base + JPEG_DEC_CROP_VERTICAL);
		jpu_dec_reg->bitstreams_start = inp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_START);
		jpu_dec_reg->bitstreams_end = inp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_END);
		jpu_dec_reg->frame_start_y = inp32(jpu_dec_base + JPEG_DEC_FRAME_START_Y);
		jpu_dec_reg->frame_stride_y = inp32(jpu_dec_base + JPEG_DEC_FRAME_STRIDE_Y);
		jpu_dec_reg->frame_start_c = inp32(jpu_dec_base + JPEG_DEC_FRAME_START_C);
		jpu_dec_reg->frame_stride_c = inp32(jpu_dec_base + JPEG_DEC_FRAME_STRIDE_C);
		jpu_dec_reg->lbuf_start_addr = inp32(jpu_dec_base + JPEG_DEC_LBUF_START_ADDR);
		jpu_dec_reg->output_type = inp32(jpu_dec_base + JPEG_DEC_OUTPUT_TYPE);
		jpu_dec_reg->freq_scale = inp32(jpu_dec_base + JPEG_DEC_FREQ_SCALE);
		jpu_dec_reg->middle_filter = inp32(jpu_dec_base + JPEG_DEC_MIDDLE_FILTER);
		jpu_dec_reg->sampling_factor = inp32(jpu_dec_base + JPEG_DEC_SAMPLING_FACTOR);
		jpu_dec_reg->dri = inp32(jpu_dec_base + JPEG_DEC_DRI);
		jpu_dec_reg->over_time_thd = inp32(jpu_dec_base + JPEG_DEC_OVER_TIME_THD);
		jpu_dec_reg->hor_phase0_coef01 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF01);
		jpu_dec_reg->hor_phase0_coef23 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF23);
		jpu_dec_reg->hor_phase0_coef45 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF45);
		jpu_dec_reg->hor_phase0_coef67 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF67);
		jpu_dec_reg->hor_phase2_coef01 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF01);
		jpu_dec_reg->hor_phase2_coef23 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF23);
		jpu_dec_reg->hor_phase2_coef45 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF45);
		jpu_dec_reg->hor_phase2_coef67 = inp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF67);
		jpu_dec_reg->ver_phase0_coef01 = inp32(jpu_dec_base + JPEG_DEC_VER_PHASE0_COEF01);
		jpu_dec_reg->ver_phase0_coef23 = inp32(jpu_dec_base + JPEG_DEC_VER_PHASE0_COEF23);
		jpu_dec_reg->ver_phase2_coef01 = inp32(jpu_dec_base + JPEG_DEC_VER_PHASE2_COEF01);
		jpu_dec_reg->ver_phase2_coef23 = inp32(jpu_dec_base + JPEG_DEC_VER_PHASE2_COEF23);
		jpu_dec_reg->csc_in_dc_coef = inp32(jpu_dec_base + JPEG_DEC_CSC_IN_DC_COEF);
		jpu_dec_reg->csc_out_dc_coef = inp32(jpu_dec_base + JPEG_DEC_CSC_OUT_DC_COEF);
		jpu_dec_reg->csc_trans_coef0 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF0);
		jpu_dec_reg->csc_trans_coef1 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF1);
		jpu_dec_reg->csc_trans_coef2 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF2);
		jpu_dec_reg->csc_trans_coef3 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF3);
		jpu_dec_reg->csc_trans_coef4 = inp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF4);

	}

	return 0;
}

static int hisi_jpu_dec_reg_init(struct hisi_jpu_data_type *hisijd)
{
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	// cppcheck-suppress *
	memcpy(&(hisijd->jpu_dec_reg), &(hisijd->jpu_dec_reg_default), sizeof(jpu_dec_reg_t));

	return 0;
}

static int hisi_jpu_dec_set_cvdr(struct hisi_jpu_data_type *hisijd)
{
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!hisijd->jpu_cvdr_base) {
		HISI_JPU_ERR("jpu_cvdr_base is NULL!\n");
		return -EINVAL;
	}
	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_CVDR_CFG_CS, 0x070f2000);

		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_8_CS, 0x80060000);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_9_CS, 0x80060000);

	if (hisijd->jpu_support_platform == HISI_KIRIN_970) {
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0, 0x80000000);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1, 0x80000000);
	}else if (hisijd->jpu_support_platform == HISI_DSS_V501) {
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0, 0x80060000);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1, 0x80060000);
	}
		/* Wr_qos_max:0x1;wr_qos_threshold_01_start:0x1;wr_qos_threshold_01_stop:0x1,WR_QOS&RD_QOS encode will also set this */
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_CVDR_WR_QOS_CFG_CS, 0x10333311);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_CVDR_RD_QOS_CFG_CS, 0x10333311);

		/* NRRD1 */
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_8_CS, 0xf002222);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_8_CS, 0x80060080);

		/* NRRD2 */
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_9_CS, 0xf003333);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_9_CS, 0x80060080);
	} else {
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_CVDR_CFG, 0x070f2000);

		if (hisijd->jpu_support_platform == HISI_KIRIN_970) {
			outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0, 0x80000000);
			outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1, 0x80000000);
		}else if (hisijd->jpu_support_platform == HISI_DSS_V500) {
			outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_0, 0x80060000);
			outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_WR_CFG_1, 0x80060000);
		}

		/* Wr_qos_max:0x1;wr_qos_threshold_01_start:0x1;wr_qos_threshold_01_stop:0x1,WR_QOS&RD_QOS encode will also set this */
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_CVDR_WR_QOS_CFG, 0x10333311);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_CVDR_RD_QOS_CFG, 0x10333311);

		/* NRRD1 */
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_1, 0xf002222);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_1, 0x80060080);

		/* NRRD2 */
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_LIMITER_NR_RD_2, 0xf003333);
		outp32(hisijd->jpu_cvdr_base + JPGDEC_CVDR_AXI_JPEG_NR_RD_CFG_2, 0x80060080);
	}

	return 0;
}

static int hisi_jpu_dec_set_unreset(struct hisi_jpu_data_type *hisijd)
{
	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!hisijd->jpu_top_base) {
		HISI_JPU_ERR("jpu_top_base is NULL!\n");
		return -EINVAL;
	}

	/* module reset */
	outp32(hisijd->jpu_top_base + JPGDEC_CRG_CFG1, 0x1);
	outp32(hisijd->jpu_top_base + JPGDEC_CRG_CFG1, 0x0);

	return 0;
}

static int hisi_jpu_dec_set_reg(struct hisi_jpu_data_type *hisijd, jpu_dec_reg_t *jpu_dec_reg)
{
	char __iomem *jpu_dec_base = NULL;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!jpu_dec_reg) {
		HISI_JPU_ERR("jpu_dec_reg is NULL!\n");
		return -EINVAL;
	}

	jpu_dec_base = hisijd->jpu_dec_base;
	if (!jpu_dec_base) {
		HISI_JPU_ERR("jpu_dec_base is NULL!\n");
		return -EINVAL;
	}

	if (g_debug_jpu_dec) {
		HISI_JPU_INFO("jpu reg val:\n"
		"dec_start=%d\n"
		"preftch_ctrl=%d\n"
		"frame_size=%d\n"
		"crop_horizontal=%d\n"
		"crop_vertical=%d\n"
		"bitstreams_start_h=%d\n"
		"bitstreams_start=%d\n"
		"bitstreams_end_h=%d\n"
		"bitstreams_end=%d\n"
		"frame_start_y=%d\n"
		"frame_stride_y=%d\n"
		"frame_start_c=%d\n"
		"frame_stride_c=%d\n"
		"lbuf_start_addr=%d\n"
		"output_type=%d\n"
		"freq_scale=%d\n"
		"middle_filter=%d\n"
		"sampling_factor=%d\n"
		"dri=%d\n"
		"over_time_thd=%d\n"
		"hor_phase0_coef01=%d\n"
		"hor_phase0_coef23=%d\n"
		"hor_phase0_coef45=%d\n"
		"hor_phase0_coef67=%d\n"
		"hor_phase2_coef01=%d\n"
		"hor_phase2_coef23=%d\n"
		"hor_phase2_coef45=%d\n"
		"hor_phase2_coef67=%d\n"
		"ver_phase0_coef01=%d\n"
		"ver_phase0_coef23=%d\n"
		"ver_phase2_coef01=%d\n"
		"ver_phase2_coef23=%d\n"
		"csc_in_dc_coef=%d\n"
		"csc_out_dc_coef=%d\n"
		"csc_trans_coef0=%d\n"
		"csc_trans_coef1=%d\n"
		"csc_trans_coef2=%d\n"
		"csc_trans_coef3=%d\n"
		"csc_trans_coef4=%d\n",
		jpu_dec_reg->dec_start,
		jpu_dec_reg->preftch_ctrl,
		jpu_dec_reg->frame_size,
		jpu_dec_reg->crop_horizontal,
		jpu_dec_reg->crop_vertical,
		jpu_dec_reg->bitstreams_start_h,
		jpu_dec_reg->bitstreams_start,
		jpu_dec_reg->bitstreams_end_h,
		jpu_dec_reg->bitstreams_end,
		jpu_dec_reg->frame_start_y,
		jpu_dec_reg->frame_stride_y,
		jpu_dec_reg->frame_start_c,
		jpu_dec_reg->frame_stride_c,
		jpu_dec_reg->lbuf_start_addr,
		jpu_dec_reg->output_type,
		jpu_dec_reg->freq_scale,
		jpu_dec_reg->middle_filter,
		jpu_dec_reg->sampling_factor,
		jpu_dec_reg->dri,
		jpu_dec_reg->over_time_thd,
		jpu_dec_reg->hor_phase0_coef01,
		jpu_dec_reg->hor_phase0_coef23,
		jpu_dec_reg->hor_phase0_coef45,
		jpu_dec_reg->hor_phase0_coef67,
		jpu_dec_reg->hor_phase2_coef01,
		jpu_dec_reg->hor_phase2_coef23,
		jpu_dec_reg->hor_phase2_coef45,
		jpu_dec_reg->hor_phase2_coef67,
		jpu_dec_reg->ver_phase0_coef01,
		jpu_dec_reg->ver_phase0_coef23,
		jpu_dec_reg->ver_phase2_coef01,
		jpu_dec_reg->ver_phase2_coef23,
		jpu_dec_reg->csc_in_dc_coef,
		jpu_dec_reg->csc_out_dc_coef,
		jpu_dec_reg->csc_trans_coef0,
		jpu_dec_reg->csc_trans_coef1,
		jpu_dec_reg->csc_trans_coef2,
		jpu_dec_reg->csc_trans_coef3,
		jpu_dec_reg->csc_trans_coef4);
	}

	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		outp32(jpu_dec_base + JPEG_DEC_PREFTCH_CTRL_CS, jpu_dec_reg->preftch_ctrl);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_SIZE_CS, jpu_dec_reg->frame_size);
		outp32(jpu_dec_base + JPEG_DEC_CROP_HORIZONTAL_CS, jpu_dec_reg->crop_horizontal);
		outp32(jpu_dec_base + JPEG_DEC_CROP_VERTICAL_CS, jpu_dec_reg->crop_vertical);
		outp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_START_H_CS, jpu_dec_reg->bitstreams_start_h);
		outp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_START_L_CS, jpu_dec_reg->bitstreams_start);
		outp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_END_H_CS, jpu_dec_reg->bitstreams_end_h);
		outp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_END_L_CS, jpu_dec_reg->bitstreams_end);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_START_Y_CS, jpu_dec_reg->frame_start_y);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_STRIDE_Y_CS, jpu_dec_reg->frame_stride_y);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_START_C_CS, jpu_dec_reg->frame_start_c);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_STRIDE_C_CS, jpu_dec_reg->frame_stride_c);
		outp32(jpu_dec_base + JPEG_DEC_LBUF_START_ADDR_CS, jpu_dec_reg->lbuf_start_addr);
		outp32(jpu_dec_base + JPEG_DEC_OUTPUT_TYPE_CS, jpu_dec_reg->output_type);
		outp32(jpu_dec_base + JPEG_DEC_FREQ_SCALE_CS, jpu_dec_reg->freq_scale);
		outp32(jpu_dec_base + JPEG_DEC_MIDDLE_FILTER_CS, jpu_dec_reg->middle_filter);
		outp32(jpu_dec_base + JPEG_DEC_SAMPLING_FACTOR_CS, jpu_dec_reg->sampling_factor);
		outp32(jpu_dec_base + JPEG_DEC_DRI_CS, jpu_dec_reg->dri);
		outp32(jpu_dec_base + JPEG_DEC_OVER_TIME_THD_CS, jpu_dec_reg->over_time_thd);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF01_CS, jpu_dec_reg->hor_phase0_coef01);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF23_CS, jpu_dec_reg->hor_phase0_coef23);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF45_CS, jpu_dec_reg->hor_phase0_coef45);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF67_CS, jpu_dec_reg->hor_phase0_coef67);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF01_CS, jpu_dec_reg->hor_phase2_coef01);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF23_CS, jpu_dec_reg->hor_phase2_coef23);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF45_CS, jpu_dec_reg->hor_phase2_coef45);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF67_CS, jpu_dec_reg->hor_phase2_coef67);
		outp32(jpu_dec_base + JPEG_DEC_VER_PHASE0_COEF01_CS, jpu_dec_reg->ver_phase0_coef01);
		outp32(jpu_dec_base + JPEG_DEC_VER_PHASE0_COEF23_CS, jpu_dec_reg->ver_phase0_coef23);
		outp32(jpu_dec_base + JPEG_DEC_VER_PHASE2_COEF01_CS, jpu_dec_reg->ver_phase2_coef01);
		outp32(jpu_dec_base + JPEG_DEC_VER_PHASE2_COEF23_CS, jpu_dec_reg->ver_phase2_coef23);
		outp32(jpu_dec_base + JPEG_DEC_CSC_IN_DC_COEF_CS, jpu_dec_reg->csc_in_dc_coef);
		outp32(jpu_dec_base + JPEG_DEC_CSC_OUT_DC_COEF_CS, jpu_dec_reg->csc_out_dc_coef);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF0_CS, jpu_dec_reg->csc_trans_coef0);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF1_CS, jpu_dec_reg->csc_trans_coef1);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF2_CS, jpu_dec_reg->csc_trans_coef2);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF3_CS, jpu_dec_reg->csc_trans_coef3);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF4_CS, jpu_dec_reg->csc_trans_coef4);

		/* to make sure start reg set at last */
		outp32(jpu_dec_base + JPEG_DEC_START_CS, 0x80000001);
	} else {
		outp32(jpu_dec_base + JPEG_DEC_PREFTCH_CTRL, jpu_dec_reg->preftch_ctrl);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_SIZE, jpu_dec_reg->frame_size);
		outp32(jpu_dec_base + JPEG_DEC_CROP_HORIZONTAL, jpu_dec_reg->crop_horizontal);
		outp32(jpu_dec_base + JPEG_DEC_CROP_VERTICAL, jpu_dec_reg->crop_vertical);
		outp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_START, jpu_dec_reg->bitstreams_start);
		outp32(jpu_dec_base + JPEG_DEC_BITSTREAMS_END, jpu_dec_reg->bitstreams_end);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_START_Y, jpu_dec_reg->frame_start_y);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_STRIDE_Y, jpu_dec_reg->frame_stride_y);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_START_C, jpu_dec_reg->frame_start_c);
		outp32(jpu_dec_base + JPEG_DEC_FRAME_STRIDE_C, jpu_dec_reg->frame_stride_c);
		outp32(jpu_dec_base + JPEG_DEC_LBUF_START_ADDR, jpu_dec_reg->lbuf_start_addr);
		outp32(jpu_dec_base + JPEG_DEC_OUTPUT_TYPE, jpu_dec_reg->output_type);
		outp32(jpu_dec_base + JPEG_DEC_FREQ_SCALE, jpu_dec_reg->freq_scale);
		outp32(jpu_dec_base + JPEG_DEC_MIDDLE_FILTER, jpu_dec_reg->middle_filter);
		outp32(jpu_dec_base + JPEG_DEC_SAMPLING_FACTOR, jpu_dec_reg->sampling_factor);
		outp32(jpu_dec_base + JPEG_DEC_DRI, jpu_dec_reg->dri);
		outp32(jpu_dec_base + JPEG_DEC_OVER_TIME_THD, jpu_dec_reg->over_time_thd);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF01, jpu_dec_reg->hor_phase0_coef01);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF23, jpu_dec_reg->hor_phase0_coef23);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF45, jpu_dec_reg->hor_phase0_coef45);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE0_COEF67, jpu_dec_reg->hor_phase0_coef67);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF01, jpu_dec_reg->hor_phase2_coef01);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF23, jpu_dec_reg->hor_phase2_coef23);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF45, jpu_dec_reg->hor_phase2_coef45);
		outp32(jpu_dec_base + JPEG_DEC_HOR_PHASE2_COEF67, jpu_dec_reg->hor_phase2_coef67);
		outp32(jpu_dec_base + JPEG_DEC_VER_PHASE0_COEF01, jpu_dec_reg->ver_phase0_coef01);
		outp32(jpu_dec_base + JPEG_DEC_VER_PHASE0_COEF23, jpu_dec_reg->ver_phase0_coef23);
		outp32(jpu_dec_base + JPEG_DEC_VER_PHASE2_COEF01, jpu_dec_reg->ver_phase2_coef01);
		outp32(jpu_dec_base + JPEG_DEC_VER_PHASE2_COEF23, jpu_dec_reg->ver_phase2_coef23);
		outp32(jpu_dec_base + JPEG_DEC_CSC_IN_DC_COEF, jpu_dec_reg->csc_in_dc_coef);
		outp32(jpu_dec_base + JPEG_DEC_CSC_OUT_DC_COEF, jpu_dec_reg->csc_out_dc_coef);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF0, jpu_dec_reg->csc_trans_coef0);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF1, jpu_dec_reg->csc_trans_coef1);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF2, jpu_dec_reg->csc_trans_coef2);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF3, jpu_dec_reg->csc_trans_coef3);
		outp32(jpu_dec_base + JPEG_DEC_CSC_TRANS_COEF4, jpu_dec_reg->csc_trans_coef4);

		/* to make sure start reg set at last */
		outp32(jpu_dec_base + JPEG_DEC_START, 0x80000001);

	}

	return 0;
}
/*lint -restore*/

void hisijpu_dump_info(struct jpu_data*  jpu_info)
{
	uint32_t i = 0;
	int j = 0;

	if (!jpu_info) {
		HISI_JPU_ERR("jpu_info is NULL!\n");
		return;
	}

	HISI_JPU_INFO(" informat %d,outformat %d \n",
		jpu_info->in_img_format,jpu_info->out_color_format);

	HISI_JPU_INFO("input buffer: [w:%d h:%d align_w:%d align_h:%d\n"
		"num_compents:%d\n"
		"sample_rate:%d\n",
		jpu_info->inwidth, jpu_info->inheight, jpu_info->pix_width, jpu_info->pix_height,
		jpu_info->num_components,
		jpu_info->sample_rate);

	HISI_JPU_INFO("stride_y :%d\n"
		"stride_c:%d\n"
		"restart_interval:%d\n",
		jpu_info->stride_y,
		jpu_info->stride_c,
		jpu_info->restart_interval);

	HISI_JPU_INFO("region[ %d %d %d %d]\n",
		jpu_info->region_info.left,jpu_info->region_info.right,
		jpu_info->region_info.top, jpu_info->region_info.bottom);

	for (i = 0; i < jpu_info->num_components; i++) {
		HISI_JPU_INFO("i=%u\n"
			"s32ComponentId:%d\n"
			"s32ComponentIndex:%d\n"
			"s32QuantTblNo:%d\n"
			"s32DcTblNo:%d\n"
			"s32AcTblNo:%d\n"
			"u8HorSampleFac:%d\n"
			"u8VerSampleFac:%d\n",
			i,
			jpu_info->component_info[i].s32ComponentId,
			jpu_info->component_info[i].s32ComponentIndex,
			jpu_info->component_info[i].s32QuantTblNo,
			jpu_info->component_info[i].s32DcTblNo,
			jpu_info->component_info[i].s32AcTblNo,
			jpu_info->component_info[i].u8HorSampleFac,
			jpu_info->component_info[i].u8VerSampleFac);
	}

	for (j = 0; j < HDC_TABLE_NUM; j++) {
		HISI_JPU_DEBUG("hdc_tab %ul\n", jpu_info->hdc_tab[j]);
	}

	for (i = 0; i < HAC_TABLE_NUM; i++) {
		HISI_JPU_DEBUG("i:%d hac_min_tab:%ul\n", i, jpu_info->hac_min_tab[i]);
		HISI_JPU_DEBUG("i:%d hac_base_tab:%ul\n", i, jpu_info->hac_base_tab[i]);
	}

	for (j = 0; j < HAC_SYMBOL_TABLE_NUM; j++) {
		HISI_JPU_DEBUG(":hdc_tab %ul\n", jpu_info->hac_symbol_tab[j]);
	}

	for (j = 0; j < MAX_DCT_SIZE; j++) {
		HISI_JPU_DEBUG("qvalue %d\n", jpu_info->qvalue[j]);
	}
}

static int hisijpu_check_format(jpu_data_t *jpu_req)
{
	if (!jpu_req) {
		HISI_JPU_ERR("jpu_req is NULL!\n");
		return -EINVAL;
	}

	if (jpu_req->out_color_format == HISI_JPEG_DECODE_OUT_YUV420) {
		if ((HISI_JPEG_DECODE_RAW_YUV422_H2V1 == jpu_req->in_img_format)
			|| (HISI_JPEG_DECODE_RAW_YUV444 == jpu_req->in_img_format)) {
			if (HISI_JPEG_DECODE_SAMPLE_SIZE_8 == jpu_req->sample_rate) {
				HISI_JPU_INFO("sample_rate %d, in_img_format %d\n",
					jpu_req->sample_rate, jpu_req->in_img_format);
				return -1;
			}
		}
	}

	return 0;
}
static int hisi_jpu_check_inbuff_par(jpu_data_t *jpu_req)
{
	if (!jpu_req) {
		HISI_JPU_ERR("jpu_req is NULL!\n");
		return -EINVAL;
	}

	if ((jpu_req->in_img_format >= HISI_JPEG_DECODE_RAW_MAX) ||
		(jpu_req->in_img_format <= HISI_JPEG_DECODE_RAW_YUV_UNSUPPORT)) {
		HISI_JPU_ERR("in_img_format(%d) is out of range!\n",
			jpu_req->in_img_format);
		return -EINVAL;
	}

	if ((jpu_req->sample_rate >= HISI_JPEG_DECODE_SAMPLE_SIZE_MAX)
		|| (jpu_req->sample_rate < HISI_JPEG_DECODE_SAMPLE_SIZE_1)) {
		HISI_JPU_ERR("sample_rate(%d) is out of range!\n",
			jpu_req->sample_rate);
		return -EINVAL;
	}

	if (((jpu_req->inwidth / jpu_req->sample_rate) < MIN_INPUT_WIDTH)
		|| (jpu_req->inwidth > MAX_INPUT_WIDTH)
		||((jpu_req->inheight / jpu_req->sample_rate) < MIN_INPUT_HEIGHT)
		|| (jpu_req->inheight > MAX_INPUT_HEIGHT)) {
		HISI_JPU_ERR("image inwidth=%d, inheight=%d ,sample_rate %d is out of range!\n",
			jpu_req->inwidth, jpu_req->inheight, jpu_req->sample_rate);
		return -EINVAL;
	}

	if (((jpu_req->pix_width / jpu_req->sample_rate) < MIN_INPUT_WIDTH)
		|| (jpu_req->pix_width > MAX_INPUT_WIDTH)
		||((jpu_req->pix_height / jpu_req->sample_rate) < MIN_INPUT_HEIGHT)
		|| (jpu_req->pix_height > MAX_INPUT_HEIGHT)) {
		HISI_JPU_ERR("image inwidth=%d, inheight=%d ,sample_rate %d is out of range!\n",
			jpu_req->pix_width, jpu_req->pix_height, jpu_req->sample_rate);
		return -EINVAL;
	}

	return 0;
}

static long hisi_jpu_check_inbuff_addr(struct hisi_jpu_data_type *hisijd, jpu_data_t *jpu_req)
{
	struct ion_handle *inhnd = NULL;
	struct iommu_map_format iommu_in_format;
	size_t buf_len = 0;
	unsigned long buf_addr = 0;
	bool succ = true;

	if (!hisijd || !(hisijd->pdev)) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!hisijd->ion_client) {
		HISI_JPU_ERR("ion_client is NULL!\n");
		return -EINVAL;
	}

	if (!jpu_req) {
		HISI_JPU_ERR("jpu_req is NULL!\n");
		return -EINVAL;
	}

	if (jpu_req->in_sharefd < 0) {
		HISI_JPU_ERR("in_sharefd=%d invalid!\n", jpu_req->in_sharefd);
		return -EINVAL;
	}

	if (jpu_req->end_addr <= jpu_req->start_addr) {
		HISI_JPU_ERR("end_addr invalid!\n");
		return -EINVAL;
	}

	if (jpu_req->addr_offset > jpu_req->start_addr || jpu_req->addr_offset > MAX_INPUT_DATA_LEN) {
		HISI_JPU_ERR("addr offset invalid!\n");
		return -EINVAL;
	}

	memset(&iommu_in_format, 0, sizeof(struct iommu_map_format));

	//check input buffer addr
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	inhnd = ion_import_dma_buf_fd(hisijd->ion_client, jpu_req->in_sharefd);
#else
	inhnd = ion_import_dma_buf(hisijd->ion_client, jpu_req->in_sharefd);
#endif
	if (IS_ERR(inhnd)) {
		HISI_JPU_ERR("inhnd ion_import_dma_buf fail ! \n");
		inhnd = NULL;
		return -EINVAL;
	}

	if (jpu_req->smmu_enable == 1) {
		if (ion_map_iommu(hisijd->ion_client, inhnd, &iommu_in_format)) {
			HISI_JPU_ERR("inhnd ion_map_iommu fail ! \n");
			succ = false;
		} else {
			if ((jpu_req->start_addr - jpu_req->addr_offset) != iommu_in_format.iova_start) {
				HISI_JPU_ERR("inbuffer ion_map_iommu check fail ! \n");
				succ = false;
			}
			ion_unmap_iommu(hisijd->ion_client, inhnd);
		}
	} else {
		if (hisifb_ion_phys(hisijd->ion_client, inhnd, &(hisijd->pdev->dev), &buf_addr, &buf_len)) {
			HISI_JPU_ERR("inhnd ion_phys fail ! \n");
			succ = false;
		} else {
			if (buf_addr != (jpu_req->start_addr - jpu_req->addr_offset)) {
				HISI_JPU_ERR("inbuffer ion_phys check fail ! \n");
				succ = false;
			}
		}
	}

	ion_free(hisijd->ion_client, inhnd);

	if (!succ)
		return -EINVAL;
	else
		return 0;
}

static int hisijpu_check_outbuffer_par(jpu_data_t *jpu_req)
{
	uint32_t out_addr_align = 0;
	uint32_t out_format = 0;
	uint32_t stride_align_mask = 0;

	if (!jpu_req) {
		HISI_JPU_ERR("jpu_req is NULL!\n");
		return -EINVAL;
	}

	if ((jpu_req->out_color_format >= HISI_JPEG_DECODE_OUT_FORMAT_MAX) ||
		(jpu_req->out_color_format <= HISI_JPEG_DECODE_OUT_UNKNOWN)) {
		HISI_JPU_ERR("out_color_format(%d) is out of range!\n",
			jpu_req->out_color_format);
		return -EINVAL;
	}

	if (jpu_req->out_color_format < 0) {
		HISI_JPU_ERR("jpu_req out_color_format is %d not support!\n", jpu_req->out_color_format);
		return -EINVAL;
	}

	/* jpu_req->pix_width; aligned to mcu width */
	/* jpu_req->pix_height; aligned to mcu height */
	/*lint -save -e571*/
	out_format = (uint32_t)jpu_req->out_color_format;
	/*lint -restore*/

	if (isRGB_out(out_format)) {
		out_addr_align = JPU_OUT_RGB_ADDR_ALIGN;
		stride_align_mask = JPU_OUT_STRIDE_ALIGN - 1;
	} else {
		out_addr_align = JPU_OUT_YUV_ADDR_ALIGN;
		stride_align_mask = JPU_OUT_STRIDE_ALIGN/2 - 1;
	}

	/*
	** start address for planar Y or RGB, unit is 16 byte, must align to 64 byte (YUV format) or 128 byte (RGB format)
	** stride for planar Y or RGB, unit is 16 byte, must align to 64 byte (YUV format) or 128 byte (RGB format)
	*/
	if ((jpu_req->start_addr_y > JPU_MAX_ADDR)
		|| (jpu_req->start_addr_y & (out_addr_align - 1))) {
		HISI_JPU_ERR("start_addr_y is not %d bytes aligned!\n",
			out_addr_align - 1);
		return -EINVAL;
	}

	if ((jpu_req->stride_y < JPU_MIN_STRIDE)
		||(jpu_req->stride_y > JPU_MAX_STRIDE)
		/*|| (jpu_req->stride_y & stride_align_mask)*/) {
		HISI_JPU_ERR("stride_y(%d) is not %d bytes aligned!\n",
			jpu_req->stride_y, stride_align_mask);
		return -EINVAL;
	}

	/*
	** start address for planar UV, unit is 16 byte, must align to 64 byte
	** stride for planar UV, unit is 16 byte, must align to 64 byte
	*/
	if ((jpu_req->start_addr_c > JPU_MAX_ADDR)
		||(jpu_req->start_addr_c & (out_addr_align - 1))) {
		HISI_JPU_ERR("start_addr_c is not %d bytes aligned!\n",
			out_addr_align - 1);
		return -EINVAL;
	}

	if ((jpu_req->stride_c > JPU_MAX_STRIDE/2)
		/*|| (jpu_req->stride_c & stride_align_mask)*/) {
		HISI_JPU_ERR("stride_c(%d) is not %d bytes aligned!\n",
			jpu_req->stride_c, stride_align_mask);
		return -EINVAL;
	}

	return 0;
}

static long hisi_jpu_check_outbuff_addr(struct hisi_jpu_data_type *hisijd, jpu_data_t *jpu_req)
{
	struct ion_handle *outhnd = NULL;
	struct iommu_map_format iommu_out_format;
	size_t buf_len = 0;
	unsigned long buf_addr = 0;
	bool succ = true;

	if (!hisijd || !(hisijd->pdev)) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (!hisijd->ion_client) {
		HISI_JPU_ERR("ion_client is NULL!\n");
		return -EINVAL;
	}

	if (!jpu_req) {
		HISI_JPU_ERR("jpu_req is NULL!\n");
		return -EINVAL;
	}

	if (jpu_req->out_sharefd < 0) {
		HISI_JPU_ERR("out_sharefd=%d invalid!\n", jpu_req->out_sharefd);
		return -EINVAL;
	}

	//start_addr unit is 16 byte, page unit is 32KB, so start_addr need to devide 2048
	if ( (jpu_req->last_page_y < ( jpu_req->start_addr_y / 2048 )) ||
		(jpu_req->last_page_c < ( jpu_req->start_addr_c / 2048 )) ) {
		HISI_JPU_ERR("last_page_y invalid!\n");
		return -EINVAL;
	}

	/*uint32_t restart_interval;*/

	memset(&iommu_out_format, 0, sizeof(struct iommu_map_format));

	//check output buffer addr
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
	outhnd = ion_import_dma_buf_fd(hisijd->ion_client, jpu_req->out_sharefd);
#else
	outhnd = ion_import_dma_buf(hisijd->ion_client, jpu_req->out_sharefd);
#endif
	if (IS_ERR(outhnd)) {
		HISI_JPU_ERR("outhnd ion_import_dma_buf fail ! \n");
		outhnd = NULL;
		return -EINVAL;
	}

	if (jpu_req->smmu_enable == 1) {
		if (ion_map_iommu(hisijd->ion_client, outhnd, &iommu_out_format)) {
			HISI_JPU_ERR("outhnd ion_map_iommu fail ! \n");
			succ = false;
		} else {
			if (jpu_req->start_addr_y != (iommu_out_format.iova_start / 16)) {
				HISI_JPU_ERR("outbuffer ion_map_iommu check fail !\n");
				succ = false;
			}
			ion_unmap_iommu(hisijd->ion_client, outhnd);
		}
	} else {
		if (hisifb_ion_phys(hisijd->ion_client, outhnd, &(hisijd->pdev->dev), &buf_addr, &buf_len)) {
			HISI_JPU_ERR("outhnd ion_phys fail ! \n");
			succ = false;
		} else {
			if ((buf_addr / 16) != jpu_req->start_addr_y) {
				HISI_JPU_ERR("outbuffer ion_phys check fail ! \n");
				succ = false;
			}
		}
	}

	ion_free(hisijd->ion_client, outhnd);

	if (!succ)
		return -EINVAL;
	else
		return 0;
}

static int hisijpu_check_region_decode_info(jpu_data_t *jpu_req)
{
	if (!jpu_req) {
		HISI_JPU_ERR("jpu_req is NULL!\n");
		return -EINVAL;
	}

	if (jpu_req->decode_mode < HISI_JPEG_DECODE_MODE_REGION) {
		HISI_JPU_INFO("in full decode, decode mode %d \n", jpu_req->decode_mode);
		return 0;
	}

	if ((jpu_req->region_info.left >= jpu_req->region_info.right)
		|| (jpu_req->region_info.top >= jpu_req->region_info.bottom)
		|| ((((jpu_req->region_info.right - jpu_req->region_info.left) + 1) /jpu_req->sample_rate) < MIN_INPUT_WIDTH)
		|| ((((jpu_req->region_info.bottom- jpu_req->region_info.top) + 1) /jpu_req->sample_rate) < MIN_INPUT_HEIGHT)) {
		HISI_JPU_ERR("region[%d %d %d %d] invalid sample_rate %d!\n",
			jpu_req->region_info.left, jpu_req->region_info.right,
			jpu_req->region_info.top, jpu_req->region_info.bottom,
			jpu_req->sample_rate);
		return -EINVAL;
	}

	if (HISI_JPEG_DECODE_OUT_YUV422_H2V1 == jpu_req->out_color_format) {
		if ((((jpu_req->region_info.right - jpu_req->region_info.left) + 1) / jpu_req->sample_rate) % 2) {
			HISI_JPU_ERR("region[%d %d] width invalid!\n",
				jpu_req->region_info.left, jpu_req->region_info.right);
			return -EINVAL;
		}
	}

	if ((HISI_JPEG_DECODE_OUT_YUV420 == jpu_req->out_color_format)
		|| (HISI_JPEG_DECODE_OUT_YUV444 == jpu_req->out_color_format)) {
		if (((((jpu_req->region_info.right - jpu_req->region_info.left) + 1) / jpu_req->sample_rate) % 2)
			|| ((((jpu_req->region_info.bottom - jpu_req->region_info.top) + 1) / jpu_req->sample_rate) % 2)) {
			HISI_JPU_ERR("region[%d %d %d %d] width or height invalid, jpu_req->sample_rate %d!\n",
				jpu_req->region_info.left, jpu_req->region_info.right,
				jpu_req->region_info.top, jpu_req->region_info.bottom,
				jpu_req->sample_rate);
			return -EINVAL;
		}
	}

	/* 440 height should be even num*/
	if (HISI_JPEG_DECODE_OUT_YUV422_H1V2 == jpu_req->out_color_format) {
		if ((((jpu_req->region_info.bottom - jpu_req->region_info.top) + 1) / jpu_req->sample_rate) % 2) {
			HISI_JPU_ERR("region[%d %d %d %d] height invalid, sample_rate %d!\n",
				jpu_req->region_info.left, jpu_req->region_info.right,
				jpu_req->region_info.top, jpu_req->region_info.bottom,
				jpu_req->sample_rate);
			return -EINVAL;
		}
	}

	return 0;
}

static int hisijpu_check_full_decode_info(jpu_data_t *jpu_req)
{
	if (!jpu_req) {
		HISI_JPU_ERR("jpu_req is NULL!\n");
		return -EINVAL;
	}

	if (jpu_req->decode_mode >= HISI_JPEG_DECODE_MODE_MAX) {
		HISI_JPU_ERR("decode_mode(%d) is out of range!\n",
			jpu_req->decode_mode);
		return -EINVAL;
	}

	if (HISI_JPEG_DECODE_MODE_FULL_SUB < jpu_req->decode_mode) {
		HISI_JPU_INFO("in region decode, decode_mode %d\n", jpu_req->decode_mode);
		return 0;
	}

	if (HISI_JPEG_DECODE_OUT_YUV422_H2V1 == jpu_req->out_color_format) {
		if ((jpu_req->pix_width / jpu_req->sample_rate) % 2) {
			HISI_JPU_ERR("out_color_format %d, jpu_req->pix_width %d invalid!\n",
				jpu_req->out_color_format, jpu_req->pix_width);
			return -EINVAL;
		}
	}

	if (HISI_JPEG_DECODE_OUT_YUV420 == jpu_req->out_color_format) {
		if (((jpu_req->pix_width / jpu_req->sample_rate) % 2) ||
			((jpu_req->pix_height / jpu_req->sample_rate) % 2)) {
			HISI_JPU_ERR("out_color_format %d, jpu_req->pix_width %d invalid!\n",
				jpu_req->out_color_format, jpu_req->pix_width);
			return -EINVAL;
		}
	}

	if (HISI_JPEG_DECODE_OUT_YUV422_H1V2 == jpu_req->out_color_format) {
		if ((jpu_req->pix_height / jpu_req->sample_rate) % 2) {
			HISI_JPU_ERR("out_color_format %d, jpu_req->pix_height %d invalid!\n",
				jpu_req->out_color_format, jpu_req->pix_height);
			return -EINVAL;
		}
	}

	return 0;
}

static int hisijpu_check_buffers(struct hisi_jpu_data_type *hisijd, jpu_data_t *jpu_req)
{
	int ret = 0;

	if ((!hisijd) || (!jpu_req)) {
		HISI_JPU_ERR("hisijd or jpu_req is NULL!\n");
		return -EINVAL;
	}

	ret = hisi_jpu_check_inbuff_par(jpu_req);
	if(ret) {
		HISI_JPU_ERR("check input buffer par error!\n");
		return -EINVAL;
	}

	ret = hisi_jpu_check_inbuff_addr(hisijd, jpu_req);
	if (ret) {
		HISI_JPU_ERR("check inputbuff addr error\n");
		return -EINVAL;
	}

	ret = hisijpu_check_outbuffer_par(jpu_req);
	if (ret) {
		HISI_JPU_ERR("check outbuffer par error\n");
		return -EINVAL;
	}

	ret = hisi_jpu_check_outbuff_addr(hisijd, jpu_req);
	if (ret) {
		HISI_JPU_ERR("check outputbuff addr error\n");
		return -EINVAL;
	}

	return 0;
}

static int hisijpu_check_userdata(struct hisi_jpu_data_type *hisijd, jpu_data_t *jpu_req)
{
	int ret = 0;

	if ((!hisijd) || (!jpu_req)) {
		HISI_JPU_ERR("hisijd or jpu_req is NULL!\n");
		return -EINVAL;
	}

	if ((jpu_req->num_components == 0) ||
		(jpu_req->num_components > PIXEL_COMPONENT_NUM)) {
		HISI_JPU_ERR("the num_components %d is out of range!\n",
			jpu_req->num_components);
		return -EINVAL;
	}

	if (jpu_req->decode_mode >= HISI_JPEG_DECODE_MODE_MAX) {
		HISI_JPU_ERR("the image decode_mode=%d is out of range!\n",
			jpu_req->decode_mode);
		return -EINVAL;
	}

	if (jpu_req->progressive_mode || jpu_req->arith_code) {
		HISI_JPU_ERR("not support progressive mode and arith_code mode!\n");
		return -EINVAL;
	}

	if(hisijd->jpu_support_platform != HISI_DSS_V501) {
		/* is format can handle for chip limit*/
		ret = hisijpu_check_format(jpu_req);
		if (ret) {
			HISI_JPU_ERR("this format not support 8 sample\n");
			return -EINVAL;
		}
	}

	ret = hisijpu_check_buffers(hisijd, jpu_req);
	if (ret) {
		HISI_JPU_ERR("hisijpu check_buffers fail!\n");
		return -EINVAL;
	}

	ret = hisijpu_check_region_decode_info(jpu_req);
	if (ret) {
		HISI_JPU_ERR("check region decode info error\n");
		return -EINVAL;
	}

	ret = hisijpu_check_full_decode_info(jpu_req);
	if (ret) {
		HISI_JPU_ERR("check full decode info error\n");
		return -EINVAL;
	}

	if (g_debug_jpu_dec) {
		hisijpu_dump_info(jpu_req);
	}

	HISI_JPU_INFO("data check ok, input format %d, output format %d, sample size %d \n",
		jpu_req->in_img_format, jpu_req->out_color_format, jpu_req->sample_rate);

	return 0;
}

void hisi_jpu_dump_reg(struct hisi_jpu_data_type *hisijd)
{
	int i = 0;
	HISI_JPU_INFO("dump debug_reg: \n");
	for (i = 0; i < JPGD_REG_DEBUG_RANGE; i++) {
		HISI_JPU_INFO("JPEG_DEC_DEBUG_INFO offset @ %d, val:0x%x\n", 0x4*i, inp32(hisijd->jpu_dec_base + JPGD_REG_DEBUG_BASE + 0x4*i));
	}
}

int hisijpu_job_exec(struct hisi_jpu_data_type *hisijd, void __user *argp)
{
	int ret = 0;
	unsigned long ret1 = 0;
	jpu_data_t *jpu_req = NULL;
	jpu_dec_reg_t *pjpu_dec_reg = NULL;
	struct timeval tv0 = {0};
	struct timeval tv1 = {0};
	long timediff = 0;

	uint8_t yfac = 0;
	uint8_t ufac = 0;
	uint8_t vfac = 0;
	int freq_scale = 0;
	int out_format = 0;

	if (!hisijd) {
		HISI_JPU_ERR("hisijd is NULL!\n");
		return -EINVAL;
	}

	if (NULL == argp) {
		HISI_JPU_ERR("argp is NULL!\n");
		return -EINVAL;
	}

	HISI_JPU_DEBUG("+.\n");

	jpu_req = &(hisijd->jpu_req);
	pjpu_dec_reg = &(hisijd->jpu_dec_reg);

	down(&hisijd->blank_sem);
	ret = hisi_jpu_on(hisijd);
	if(ret) {
		HISI_JPU_ERR("hisi_jpu_on failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	ret1 = copy_from_user(jpu_req, argp, sizeof(jpu_data_t));
	if (ret1) {
		HISI_JPU_ERR("copy_from_user failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	ret = hisijpu_check_userdata(hisijd, jpu_req);
	if (ret) {
		HISI_JPU_ERR("hisijpu_check_userdata failed!\n");
		ret = -EINVAL;
		goto err_out;
	}
	if (!hisijd->jpu_res_initialized) {
		ret = hisi_jpu_dec_reg_default(hisijd);
		if (ret) {
			HISI_JPU_ERR("hisi_jpu_dec_reg_default failed!\n");
			ret = -EINVAL;
			goto err_out;
		}
		hisijd->jpu_res_initialized = true;
	}

	ret = hisi_jpu_dec_reg_init(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisi_jpu_dec_reg_init failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	ret = hisi_jpu_dec_set_cvdr(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisi_jpu_dec_set_cvdr failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	ret = hisi_jpu_dec_set_unreset(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisi_jpu_dec_set_unreset failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	/* prefetch control */
	pjpu_dec_reg->preftch_ctrl = jpu_set_bits32(pjpu_dec_reg->preftch_ctrl,
		jpu_req->smmu_enable ? 0x0 : 0x1, 32, 0);
	/* define reset interval */
	pjpu_dec_reg->dri = jpu_set_bits32(pjpu_dec_reg->dri,
		jpu_req->restart_interval, 32, 0);

	/* frame size */
	pjpu_dec_reg->frame_size = jpu_set_bits32(pjpu_dec_reg->frame_size,
		((jpu_req->pix_width - 1) | ((jpu_req->pix_height - 1) << 16)), 32, 0);

	/* input bitstreams addr */
	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		pjpu_dec_reg->bitstreams_start_h = (jpu_req->start_addr >> 32) & 0x3;
	}
	pjpu_dec_reg->bitstreams_start = jpu_set_bits32(pjpu_dec_reg->bitstreams_start,
		(uint32_t)jpu_req->start_addr, 32, 0);
	if(hisijd->jpu_support_platform == HISI_DSS_V501) {
		pjpu_dec_reg->bitstreams_end_h = (jpu_req->end_addr >> 32) & 0x3;
	}
	pjpu_dec_reg->bitstreams_end = jpu_set_bits32(pjpu_dec_reg->bitstreams_end,
		(uint32_t)jpu_req->end_addr, 32, 0);

	/* output buffer addr */
	pjpu_dec_reg->frame_start_y = jpu_set_bits32(pjpu_dec_reg->frame_start_y,
		(uint32_t)jpu_req->start_addr_y, 28, 0);
	pjpu_dec_reg->frame_stride_y = jpu_set_bits32(pjpu_dec_reg->frame_stride_y,
		jpu_req->stride_y | ((uint32_t)(jpu_req->last_page_y << 12)), 29, 0);
	pjpu_dec_reg->frame_start_c = jpu_set_bits32(pjpu_dec_reg->frame_start_c,
		(uint32_t)jpu_req->start_addr_c, 28, 0);
	pjpu_dec_reg->frame_stride_c = jpu_set_bits32(pjpu_dec_reg->frame_stride_c,
		jpu_req->stride_c | ((uint32_t)(jpu_req->last_page_c << 12)), 29, 0);

	/* start address for line buffer,unit is 16 byte, must align to 128 byte */
	pjpu_dec_reg->lbuf_start_addr = jpu_set_bits32(pjpu_dec_reg->lbuf_start_addr,
		hisijd->lb_addr, 28, 0);

	/* output type , should compare with input format*/
	out_format = hisi_out_format_hal2jpu(hisijd);
	if (out_format < 0) {
		HISI_JPU_ERR("hisi_out_format_hal2jpu failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	//set alpha value as 0xff
	if (jpu_req->out_color_format >= HISI_JPEG_DECODE_OUT_RGBA4444) {
		pjpu_dec_reg->output_type = jpu_set_bits32(pjpu_dec_reg->output_type,
			(uint32_t)out_format | (0xff << 8), 16, 0);
	}else {
		pjpu_dec_reg->output_type = jpu_set_bits32(pjpu_dec_reg->output_type,
			(uint32_t)out_format, 16, 0);
	}

	/* frequence scale */
	freq_scale = hisi_sample_size_hal2jpu(jpu_req->sample_rate);
	if (freq_scale < 0) {
		HISI_JPU_ERR("hisi_sample_size_hal2jpu failed!\n");
		ret = -EINVAL;
		goto err_out;
	}
	pjpu_dec_reg->freq_scale = jpu_set_bits32(pjpu_dec_reg->freq_scale,
		(uint32_t)freq_scale, 2, 0);

	/* for  decode region*/
	ret = hisijpu_set_crop(hisijd);
	if (ret) {
		HISI_JPU_ERR("hisijpu_set_crop ret = %d\n", ret);
		goto err_out;
	}

	/* MIDDLE FITER can use default value */

	/* sampling factor */
	yfac = (((jpu_req->component_info[0].u8HorSampleFac << 4) |
		jpu_req->component_info[0].u8VerSampleFac) & 0xff);
	ufac = (((jpu_req->component_info[1].u8HorSampleFac << 4) |
		jpu_req->component_info[1].u8VerSampleFac) & 0xff);
	vfac = (((jpu_req->component_info[2].u8HorSampleFac << 4) |
		jpu_req->component_info[2].u8VerSampleFac) & 0xff);
	pjpu_dec_reg->sampling_factor = jpu_set_bits32(pjpu_dec_reg->sampling_factor,
		(vfac | (ufac << 8) | (yfac << 16)), 24, 0);

	/* set dqt table */
	hisijpu_set_dqt(jpu_req, hisijd->jpu_dec_base);

	/* set dht table */
	ret = hisijpu_set_dht(jpu_req, hisijd->jpu_dec_base);
	if (ret) {
		HISI_JPU_ERR("hisijpu_set_dht failed!\n");
		ret = -EINVAL;
		goto err_out;
	}

	/* set over_time */

	/* start to work */
	pjpu_dec_reg->dec_start = jpu_set_bits32(pjpu_dec_reg->dec_start, 0x1, 1, 0);

	ret = hisi_jpu_dec_set_reg(hisijd, pjpu_dec_reg);
	if (ret) {
		HISI_JPU_ERR("hisi_jpu_dec_set_reg ret = %d\n", ret);
	}

	if (g_debug_jpu_dec_job_timediff)
		jpu_get_timestamp(&tv0);

	ret = hisi_jpu_dec_done_config(hisijd, jpu_req);
	if (ret != 0) {
		HISI_JPU_ERR("hisi_jpu_dec_done_config failed! ret = %d\n", ret);
		hisi_jpu_dump_reg(hisijd);
	}

err_out:
	ret1 = hisi_jpu_off(hisijd);
	if (ret1) {
		HISI_JPU_ERR("hisi_jpu_off failed! \n");
	}

	up(&hisijd->blank_sem);

	if (g_debug_jpu_dec_job_timediff) {
		jpu_get_timestamp(&tv1);
		timediff = jpu_timestamp_diff(&tv0, &tv1);
		HISI_JPU_INFO("jpu job exec timediff is %ld us!", timediff);
	}

	HISI_JPU_DEBUG("-.\n");

	return ret;
}
