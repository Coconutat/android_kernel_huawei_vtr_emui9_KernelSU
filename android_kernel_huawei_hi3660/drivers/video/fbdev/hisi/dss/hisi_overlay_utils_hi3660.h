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
#ifndef _HISI_OVERLAY_UTILS_PLATFORM_H_
#define _HISI_OVERLAY_UTILS_PLATFORM_H_

#include "hisi_display_effect_hi3660.h"
#define CONFIG_DSS_LP_USED
#define HISI_DSS_VERSION_V400

//GPIO
#define GPIO_LCD_POWER_1V2  (54)     //GPIO_6_6
#define GPIO_LCD_STANDBY    (67)     //GPIO_8_3
#define GPIO_LCD_RESETN     (65)     //GPIO_8_1
#define GPIO_LCD_GATING     (60)     //GPIO_7_4
#define GPIO_LCD_PCLK_GATING (58)    //GPIO_7_2
#define GPIO_LCD_REFCLK_GATING (59)  //GPIO_7_3
#define GPIO_LCD_SPICS         (168) //GPIO_21_0
#define GPIO_LCD_DRV_EN        (73)  //GPIO_9_1

#define GPIO_PG_SEL_A (72)    //GPIO_9_0
#define GPIO_TX_RX_A (74)    //GPIO_9_2
#define GPIO_PG_SEL_B (76)    //GPIO_9_4
#define GPIO_TX_RX_B (78)    //GPIO_9_6

/*******************************************************************************
**
*/
#define CRGPERI_PLL0_CLK_RATE	(1600000000UL)
#define CRGPERI_PLL2_CLK_RATE	(960000000UL)

#define DEFAULT_DSS_CORE_CLK_08V_RATE	(535000000UL)
#define DEFAULT_DSS_CORE_CLK_07V_RATE	(400000000UL)
#define DEFAULT_PCLK_DSS_RATE	(114000000UL)
#define DEFAULT_PCLK_PCTRL_RATE	(80000000UL)
#define DSS_MAX_PXL0_CLK_288M (288000000UL)

#define OVL_LAYER_NUM_MAX (7)
#define OVL_PATTERN_RATIO (1)

//288 KB
#define MMBUF_SIZE_MAX	(288 * 1024)
#define MMBUF_SIZE_MDC_MAX (0)
#define HISI_DSS_CMDLIST_MAX	(16)
#define HISI_DSS_CMDLIST_IDXS_MAX (0xFFFF)   //16 cmdlist, 16bit, 1111,1111,1111,1111=0xFFFF
#define HISI_DSS_COPYBIT_CMDLIST_IDXS	 (0xC000)  //bit14, bit15
#define HISI_DSS_DPP_MAX_SUPPORT_BIT (0x7ff) //10 dpp module, 10bit, contrast to enmu dpp_module_idx
#define HISIFB_DSS_PLATFORM_TYPE  (FB_ACCEL_HI366x | FB_ACCEL_PLATFORM_TYPE_ASIC)

#define DSS_MIF_SMMU_SMRX_IDX_STEP (16)

//PERI REG
#define CRG_PERI_DIS3_DEFAULT_VAL     (0x0002F000)

//scl
#define SCF_LINE_BUF	(2560)

//DSS global
#define DSS_GLB_MODULE_CLK_SEL_DEFAULT_VAL  (0xF0000008)


//LDI0 clk sel
#define DSS_LDI_CLK_SEL_DEFAULT_VAL    (0x00000004)

//DBuf mem ctrl
#define DSS_DBUF_MEM_CTRL_DEFAULT_VAL  (0x00000008)

//SMMU
#define DSS_SMMU_RLD_EN0_DEFAULT_VAL    (0xffffffff)
#define DSS_SMMU_RLD_EN1_DEFAULT_VAL    (0xffffff8f)
#define DSS_SMMU_OUTSTANDING_VAL		(0xf)

//MIF
#define DSS_MIF_CTRL2_INVAL_SEL3_STRIDE_MASK		(0xc)

//AFBCE
#define DSS_AFBCE_ENC_OS_CFG_DEFAULT_VAL			(0x7)

#define TUI_SEC_RCH			(DSS_RCHN_V0)

#define DSS_CHN_MAX_DEFINE (DSS_COPYBIT_MAX)

#define SMMU_SID_NUM	(64)

/*******************************************************************************
** dss module reg
*/
typedef struct dss_mctl_ch_base {
	char __iomem *chn_mutex_base;
	char __iomem *chn_flush_en_base;
	char __iomem *chn_ov_en_base;
	char __iomem *chn_starty_base;
	char __iomem *chn_mod_dbg_base;
} dss_mctl_ch_base_t;

typedef struct dss_smmu {
	uint32_t smmu_scr;
	uint32_t smmu_memctrl;
	uint32_t smmu_lp_ctrl;
	uint32_t smmu_press_remap;
	uint32_t smmu_intmask_ns;
	uint32_t smmu_intraw_ns;
	uint32_t smmu_intstat_ns;
	uint32_t smmu_intclr_ns;
	uint32_t smmu_smrx_ns[SMMU_SID_NUM];
	uint32_t smmu_rld_en0_ns;
	uint32_t smmu_rld_en1_ns;
	uint32_t smmu_rld_en2_ns;
	uint32_t smmu_cb_sctrl;
	uint32_t smmu_cb_ttbr0;
	uint32_t smmu_cb_ttbr1;
	uint32_t smmu_cb_ttbcr;
	uint32_t smmu_offset_addr_ns;
	uint32_t smmu_scachei_all;
	uint32_t smmu_scachei_l1;
	uint32_t smmu_scachei_l2l3;
	uint32_t smmu_fama_ctrl0_ns;
	uint32_t smmu_fama_ctrl1_ns;
	uint32_t smmu_addr_msb;
	uint32_t smmu_err_rdaddr;
	uint32_t smmu_err_wraddr;
	uint32_t smmu_fault_addr_tcu;
	uint32_t smmu_fault_id_tcu;
	uint32_t smmu_fault_addr_tbux;
	uint32_t smmu_fault_id_tbux;
	uint32_t smmu_fault_infox;
	uint32_t smmu_dbgrptr_tlb;
	uint32_t smmu_dbgrdata_tlb;
	uint32_t smmu_dbgrptr_cache;
	uint32_t smmu_dbgrdata0_cache;
	uint32_t smmu_dbgrdata1_cache;
	uint32_t smmu_dbgaxi_ctrl;
	uint32_t smmu_ova_addr;
	uint32_t smmu_opa_addr;
	uint32_t smmu_ova_ctrl;
	uint32_t smmu_opref_addr;
	uint32_t smmu_opref_ctrl;
	uint32_t smmu_opref_cnt;
	uint32_t smmu_smrx_s[SMMU_SID_NUM];
	uint32_t smmu_rld_en0_s;
	uint32_t smmu_rld_en1_s;
	uint32_t smmu_rld_en2_s;
	uint32_t smmu_intmas_s;
	uint32_t smmu_intraw_s;
	uint32_t smmu_intstat_s;
	uint32_t smmu_intclr_s;
	uint32_t smmu_scr_s;
	uint32_t smmu_scb_sctrl;
	uint32_t smmu_scb_ttbr;
	uint32_t smmu_scb_ttbcr;
	uint32_t smmu_offset_addr_s;

	uint8_t smmu_smrx_ns_used[DSS_CHN_MAX_DEFINE];
} dss_smmu_t;

typedef struct dss_arsr2p {
	uint32_t arsr_input_width_height;
	uint32_t arsr_output_width_height;
	uint32_t ihleft;
	uint32_t ihright;
	uint32_t ivtop;
	uint32_t ivbottom;
	uint32_t ihinc;
	uint32_t ivinc;
	uint32_t offset;
	uint32_t mode;
	struct arsr2p_info arsr2p_effect;
	struct arsr2p_info arsr2p_effect_scale_up;
	struct arsr2p_info arsr2p_effect_scale_down;
	uint32_t ihleft1;
	uint32_t ihright1;
	uint32_t ivbottom1;
} dss_arsr2p_t;

typedef struct dirty_region_updt {
	uint32_t dbuf_frm_size;
	uint32_t dbuf_frm_hsize;
	uint32_t dpp_img_size_bef_sr;
	uint32_t dpp_img_size_aft_sr;
	uint32_t ldi_dpi0_hrz_ctrl0;
	uint32_t ldi_dpi0_hrz_ctrl1;
	uint32_t ldi_dpi0_hrz_ctrl2;
	uint32_t ldi_dpi1_hrz_ctrl0;
	uint32_t ldi_dpi1_hrz_ctrl1;
	uint32_t ldi_dpi1_hrz_ctrl2;
	uint32_t ldi_vrt_ctrl0;
	uint32_t ldi_vrt_ctrl1;
	uint32_t ldi_vrt_ctrl2;
	uint32_t ldi_ctrl;
	uint32_t ifbc_size;
	uint32_t edpi_cmd_size;
	dss_arsr1p_t s_arsr1p;
}dirty_region_updt_t;

typedef struct dss_module_reg {
	char __iomem *mif_ch_base[DSS_CHN_MAX_DEFINE];
	char __iomem *aif_ch_base[DSS_CHN_MAX_DEFINE];
	char __iomem *aif1_ch_base[DSS_CHN_MAX_DEFINE];
	dss_mctl_ch_base_t mctl_ch_base[DSS_CHN_MAX_DEFINE];
	char __iomem *dma_base[DSS_CHN_MAX_DEFINE];
	char __iomem *dfc_base[DSS_CHN_MAX_DEFINE];
	char __iomem *scl_base[DSS_CHN_MAX_DEFINE];
	char __iomem *scl_lut_base[DSS_CHN_MAX_DEFINE];
	char __iomem *arsr2p_base[DSS_CHN_MAX_DEFINE];
	char __iomem *arsr2p_lut_base[DSS_CHN_MAX_DEFINE];
	char __iomem *post_clip_base[DSS_CHN_MAX_DEFINE];
	char __iomem *pcsc_base[DSS_CHN_MAX_DEFINE];
	char __iomem *csc_base[DSS_CHN_MAX_DEFINE];

	char __iomem *ov_base[DSS_OVL_IDX_MAX];
	char __iomem *mctl_base[DSS_MCTL_IDX_MAX];
	char __iomem *mctl_sys_base;
	char __iomem *smmu_base;
	char __iomem *post_scf_base;

	dss_mif_t mif[DSS_CHN_MAX_DEFINE];
	dss_aif_t aif[DSS_CHN_MAX_DEFINE];
	dss_aif_t aif1[DSS_CHN_MAX_DEFINE];
	dss_aif_bw_t aif_bw[DSS_CHN_MAX_DEFINE];
	dss_aif_bw_t aif1_bw[DSS_CHN_MAX_DEFINE];
	dss_rdma_t rdma[DSS_CHN_MAX_DEFINE];
	dss_wdma_t wdma[DSS_CHN_MAX_DEFINE];
	dss_dfc_t dfc[DSS_CHN_MAX_DEFINE];
	dss_scl_t scl[DSS_CHN_MAX_DEFINE];
	dss_arsr2p_t arsr2p[DSS_CHN_MAX_DEFINE];
	dss_post_clip_t post_clip[DSS_CHN_MAX_DEFINE];
	dss_csc_t pcsc[DSS_CHN_MAX_DEFINE];
	dss_csc_t csc[DSS_CHN_MAX_DEFINE];
	dss_ovl_t ov[DSS_OVL_IDX_MAX];
	dss_mctl_t mctl[DSS_MCTL_IDX_MAX];
	dss_mctl_ch_t mctl_ch[DSS_CHN_MAX_DEFINE];
	dss_mctl_sys_t mctl_sys;
	dss_smmu_t smmu;
	dirty_region_updt_t dirty_region_updt;
	dss_arsr1p_t post_scf;

	uint8_t mif_used[DSS_CHN_MAX_DEFINE];
	uint8_t aif_ch_used[DSS_CHN_MAX_DEFINE];
	uint8_t aif1_ch_used[DSS_CHN_MAX_DEFINE];
	uint8_t dma_used[DSS_CHN_MAX_DEFINE];
	uint8_t dfc_used[DSS_CHN_MAX_DEFINE];
	uint8_t scl_used[DSS_CHN_MAX_DEFINE];
	uint8_t arsr2p_used[DSS_CHN_MAX_DEFINE];
	uint8_t arsr2p_effect_used[DSS_CHN_MAX_DEFINE];
	uint8_t post_clip_used[DSS_CHN_MAX_DEFINE];
	uint8_t pcsc_used[DSS_CHN_MAX_DEFINE];
	uint8_t csc_used[DSS_CHN_MAX_DEFINE];
	uint8_t ov_used[DSS_OVL_IDX_MAX];
	uint8_t ch_reg_default_used[DSS_CHN_MAX_DEFINE];
	uint8_t mctl_used[DSS_MCTL_IDX_MAX];
	uint8_t mctl_ch_used[DSS_CHN_MAX_DEFINE];
	uint8_t mctl_sys_used;
	uint8_t smmu_used;
	uint8_t dirty_region_updt_used;
	uint8_t post_scf_used;
} dss_module_reg_t;

typedef struct dss_mmbuf_info {
	uint32_t mm_base[DSS_CHN_MAX_DEFINE];
	uint32_t mm_size[DSS_CHN_MAX_DEFINE];

	uint8_t mm_used[DSS_CHN_MAX_DEFINE];
} dss_mmbuf_info_t;


#endif
