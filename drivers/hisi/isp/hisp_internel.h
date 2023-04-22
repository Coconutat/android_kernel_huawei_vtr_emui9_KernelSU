/*
 * hisilicon ISP driver, hisi_isp_rproc.c
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 */

#ifndef _HISP_INTERNEL_H
#define _HISP_INTERNEL_H

#define TIMEOUT        20
#define SMMU_CLK_ENABLE         0x400000
#define HISI_KIRIN970_CS        0x00000065

/* regs offset*/
#define CRG_C88_PERIPHISP_SEC_RSTSTAT       0xC88
#define CRG_C90_PERIPHISP_ISPA7_CTRL_BASE   0xC90

/* regs bit mask*/
#define ISP_A7_REMAP_ENABLE (1 << 31)
#define ISPA7_DBGPWRDUP     (1 << 2)
#define ISPA7_VINITHI_HIGH  (1 << 1)
#define BIT_RST_ISP         (1 << 0)
#define ISP_SUB_CTRL_ISP_A7_CTRL_0          (0x40)

#ifdef CONFIG_ES_ISP_LOW_FREQ
#define CLOCKVALUE "clock-value-low"
#else
#define CLOCKVALUE "clock-value"
#endif

extern int dis_rststat_poll(void __iomem *addr, unsigned int value);

int atfisp_setparams(u64 shrd_paddr);
int atfisp_params_dump(u64 boot_pa_addr);
int atfisp_cpuinfo_nsec_dump(u64 param_pa_addr);
void atfisp_isp_init(u64 pgd_base);
void atfisp_isp_exit(void);
int atfisp_ispcpu_init(void);
void atfisp_ispcpu_exit(void);
int atfisp_ispcpu_map(void);
void atfisp_ispcpu_unmap(void);
void atfisp_set_nonsec(void);
int atfisp_disreset_ispcpu(void);
void atfisp_ispsmmu_ns_init(u64 pgt_addr);
int atfisp_get_ispcpu_idle(void);
int atfisp_isptop_power_up(void);
int atfisp_isptop_power_down(void);
int hisp_get_pgd_base(u64 *pgd_base);
int ispmmu_init(void);
int ispmmu_exit(void);
int ispcvdr_init(void);
int hisi_isp_set_pgd(void);
int hisi_isp_cvdr_getdts(struct device_node *np);
int get_isp_mdc_flag(void);
int hisi_isp_mdc_getdts(struct device_node *np);

#endif /* _HISI_ISP_HISI_ISP_RPROC_H */
