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

#ifndef HISI_DPE_UTILS_H
#define HISI_DPE_UTILS_H

#include "hisi_fb.h"

#define COMFORM_MAX	80
#define CHANGE_MAX	100
#define DISCOUNT_COEFFICIENT(value)  (CHANGE_MAX - value) / CHANGE_MAX
#define CSC_VALUE_MAX	32768
#define CSC_VALUE_NUM	9

struct dss_vote_cmd * get_dss_vote_cmd(struct hisi_fb_data_type *hisifd);
int set_dss_vote_cmd(struct hisi_fb_data_type *hisifd, dss_vote_cmd_t dss_vote_cmd);
int dpe_set_clk_rate(struct platform_device *pdev);
int dpe_get_voltage_value(dss_vote_cmd_t *vote_cmd);
int dpe_get_voltage_level(int votage_value);
int hisifb_set_mmbuf_clk_rate(struct hisi_fb_data_type *hisifd);

int dpe_set_pixel_clk_rate_on_pll0(struct hisi_fb_data_type *hisifd);
int dpe_set_common_clk_rate_on_pll0(struct hisi_fb_data_type *hisifd);

void init_post_scf(struct hisi_fb_data_type *hisifd);
void init_dbuf(struct hisi_fb_data_type *hisifd);
void deinit_dbuf(struct hisi_fb_data_type *hisifd);
void init_dpp(struct hisi_fb_data_type *hisifd);
void init_sbl(struct hisi_fb_data_type *hisifd);
void init_acm(struct hisi_fb_data_type *hisifd);
void init_igm_gmp_xcc_gm(struct hisi_fb_data_type *hisifd);
void init_dither(struct hisi_fb_data_type *hisifd);
void init_ifbc(struct hisi_fb_data_type *hisifd);
void acm_set_lut(char __iomem *address, uint32_t table[], uint32_t size);
void acm_set_lut_hue(char __iomem *address, uint32_t table[], uint32_t size);

void hisifb_display_post_process_chn_init(struct hisi_fb_data_type *hisifd);
void init_ldi(struct hisi_fb_data_type *hisifd, bool fastboot_enable);
void deinit_ldi(struct hisi_fb_data_type *hisifd);
void enable_ldi(struct hisi_fb_data_type *hisifd);
void disable_ldi(struct hisi_fb_data_type *hisifd);
void ldi_frame_update(struct hisi_fb_data_type *hisifd, bool update);
void single_frame_update(struct hisi_fb_data_type *hisifd);
void ldi_data_gate(struct hisi_fb_data_type *hisifd, bool enble);

#if !defined(CONFIG_HISI_FB_3650) || !defined (CONFIG_HISI_FB_6250)
int dpe_recover_pxl_clock(struct hisi_fb_data_type *hisifd);
void init_dpp_csc(struct hisi_fb_data_type *hisifd);
#endif

void dpe_store_ct_cscValue(struct hisi_fb_data_type *hisifd, unsigned int csc_value[]);
int dpe_set_ct_cscValue(struct hisi_fb_data_type *hisifd);
ssize_t dpe_show_ct_cscValue(struct hisi_fb_data_type *hisifd, char *buf);
int dpe_set_xcc_cscValue(struct hisi_fb_data_type *hisifd);
/* isr */
irqreturn_t dss_dsi0_isr(int irq, void *ptr);
irqreturn_t dss_dsi1_isr(int irq, void *ptr);
irqreturn_t dss_sdp_isr_mipi_panel(int irq, void *ptr);
irqreturn_t dss_sdp_isr_dp(int irq, void *ptr);
irqreturn_t dss_pdp_isr(int irq, void *ptr);
irqreturn_t dss_sdp_isr(int irq, void *ptr);
irqreturn_t dss_adp_isr(int irq, void *ptr);
irqreturn_t dss_mdc_isr(int irq, void *ptr);

void dpe_interrupt_clear(struct hisi_fb_data_type *hisifd);
void dpe_interrupt_unmask(struct hisi_fb_data_type *hisifd);
void dpe_interrupt_mask(struct hisi_fb_data_type *hisifd);
void mdc_regulator_enable(struct hisi_fb_data_type *hisifd);
void mdc_regulator_disable(struct hisi_fb_data_type *hisifd);
int mediacrg_regulator_enable(struct hisi_fb_data_type *hisifd);
int mediacrg_regulator_disable(struct hisi_fb_data_type *hisifd);
int dpe_regulator_enable(struct hisi_fb_data_type *hisifd);
int dpe_regulator_disable(struct hisi_fb_data_type *hisifd);
int dpe_common_clk_enable(struct hisi_fb_data_type *hisifd);
int dpe_inner_clk_enable(struct hisi_fb_data_type *hisifd);
int dpe_common_clk_disable(struct hisi_fb_data_type *hisifd);
int dpe_inner_clk_disable(struct hisi_fb_data_type *hisifd);
void hisifb_pipe_clk_set_underflow_flag(struct hisi_fb_data_type *hisifd, bool underflow);
void dss_inner_clk_common_enable(struct hisi_fb_data_type *hisifd, bool fastboot_enable);

void dss_inner_clk_common_disable(struct hisi_fb_data_type *hisifd);

void dss_inner_clk_pdp_enable(struct hisi_fb_data_type *hisifd, bool fastboot_enable);
void dss_inner_clk_pdp_disable(struct hisi_fb_data_type *hisifd);

void dss_inner_clk_sdp_enable(struct hisi_fb_data_type *hisifd);
void dss_inner_clk_sdp_disable(struct hisi_fb_data_type *hisifd);

void dpe_update_g_comform_discount(unsigned int value);
int dpe_set_comform_ct_cscValue(struct hisi_fb_data_type *hisifd);
ssize_t dpe_show_comform_ct_cscValue(struct hisi_fb_data_type *hisifd, char *buf);

void dpe_init_led_rg_ct_cscValue(void);
void dpe_store_led_rg_ct_cscValue(unsigned int csc_value[]);
int dpe_set_led_rg_ct_cscValue(struct hisi_fb_data_type *hisifd);
ssize_t dpe_show_led_rg_ct_cscValue(char *buf);

ssize_t dpe_show_cinema_value(struct hisi_fb_data_type *hisifd, char *buf);

void dpe_set_cinema_acm(struct hisi_fb_data_type *hisifd, unsigned int value);

int dpe_set_cinema(struct hisi_fb_data_type *hisifd, unsigned int value);
void dpe_update_g_acm_state(unsigned int value);
void dpe_set_acm_state(struct hisi_fb_data_type *hisifd);
ssize_t dpe_show_acm_state(char *buf);
void dpe_update_g_gmp_state(unsigned int value);
void dpe_set_gmp_state(struct hisi_fb_data_type *hisifd);
ssize_t dpe_show_gmp_state(char *buf);
void dpe_sbl_set_al_bl(struct hisi_fb_data_type *hisifd);
#endif
