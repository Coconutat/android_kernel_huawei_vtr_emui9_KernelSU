/*
 * Header file for device driver Hi6421 PMIC
 *
 * Copyright (c) 2013 Linaro Ltd.
 * Copyright (C) 2011 Hisilicon.

 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef __HISI_PMIC_MNTN_H
#define __HISI_PMIC_MNTN_H

#define PMIC_OCP_LDO_NAME        (16)
#define PMIC_EVENT_RESERVED        (240)

typedef  enum {
	HISI_PMIC_OCP_EVENT = 0,
	HISI_PMIC_EVENT_MAX,
}PMIC_MNTN_EVENT_TYPE;

typedef struct{
	char ldo_num[PMIC_OCP_LDO_NAME];
	char reserved[PMIC_EVENT_RESERVED];
}PMIC_MNTN_EXCP_INFO;

typedef enum {
	PMIC_HRESET_COLD = 0,
	PMIC_HRESET_HOT,
}PMIC_HREST_TYPE;

#if defined(CONFIG_HISI_PMIC_MNTN) || defined(CONFIG_HISI_PMIC_MNTN_SPMI)
//打开关闭smpl功能
//SYS_CTRL1,0x0DC, np_smpl_open_en(bit 0), SMPL功能使能位。
//enable:true, support smpl
extern int hisi_pmic_mntn_config_smpl(bool enable);

//config vsys_pwroff_abs_pd
//'SYS_CTRL0, 0x0DB,vsys_pwroff_abs_pd_mask(bit 0), vsys小于2.3v时是否自动关机控制位。
//enable:true, 1：不自动关机。
extern int hisi_pmic_mntn_config_vsys_pwroff_abs_pd(bool enable);

extern int hisi_pmic_mntn_vsys_pwroff_abs_pd_state(void);
#else
static inline int hisi_pmic_mntn_config_smpl(bool enable){ return 0; }
static inline int hisi_pmic_mntn_config_vsys_pwroff_abs_pd(bool enable){ return 0; }
static inline int hisi_pmic_mntn_vsys_pwroff_abs_pd_state(void){ return 0; }
#endif

#ifdef CONFIG_HISI_PMIC_MNTN_SPMI
extern int hisi_pmic_set_cold_reset(unsigned char status);
/*pmic mntn notifier function*/
int hisi_pmic_mntn_register_notifier(struct notifier_block *nb);
int hisi_pmic_mntn_unregister_notifier(struct notifier_block *nb);
#else
static inline int hisi_pmic_set_cold_reset(unsigned char status){ return 0;}
static inline int hisi_pmic_mntn_register_notifier(struct notifier_block *nb){return 0;}
static inline int hisi_pmic_mntn_unregister_notifier(struct notifier_block *nb){return 0;}
#endif
#endif /*__HISI_PMIC_MNTN_H*/
