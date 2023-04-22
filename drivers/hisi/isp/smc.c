/*
 * hisilicon ISP driver, qos.c
 *
 * Copyright (c) 2018 Hisilicon Technologies CO., Ltd.
 *
 */

#include <asm/uaccess.h>
#include <asm/compiler.h>
#include "libhwsecurec/securec.h"

#define ISP_FN_SET_PARAMS       (0xC500AB00)
#define ISP_FN_PARAMS_DUMP      (0xC500AB01)
#define ISP_FN_ISPCPU_NS_DUMP   (0xC500AB02)
#define ISP_FN_ISP_INIT         (0xC500AB11)
#define ISP_FN_ISP_EXIT         (0xC500AB12)
#define ISP_FN_ISPCPU_INIT      (0xC500AB22)
#define ISP_FN_ISPCPU_EXIT      (0xC500AB23)
#define ISP_FN_ISPCPU_MAP       (0xC500AB24)
#define ISP_FN_ISPCPU_UNMAP     (0xC500AB25)
#define ISP_FN_SET_NONSEC       (0xC500AB26)
#define ISP_FN_DISRESET_ISPCPU  (0xC500AB27)
#define ISP_FN_ISPSMMU_NS_INIT  (0xC500AB28)
#define ISP_FN_GET_ISPCPU_IDLE  (0xC500AB29)
#define ISP_FN_ISPTOP_PU        (0xC500AB30)
#define ISP_FN_ISPTOP_PD        (0xC500AB31)

noinline int atfd_hisi_service_isp_smc(u64 _funcid, u64 _arg0, u64 _arg1, u64 _arg2)
{
       register u64 funcid asm("x0") = _funcid;
       register u64 arg0 asm("x1") = _arg0;
       register u64 arg1 asm("x2") = _arg1;
       register u64 arg2 asm("x3") = _arg2;
       asm volatile (
            __asmeq("%0", "x0")
            __asmeq("%1", "x1")
            __asmeq("%2", "x2")
            __asmeq("%3", "x3")
            "smc    #0\n"
        : "+r" (funcid)
        : "r" (arg0), "r" (arg1), "r" (arg2));

    return (int)funcid;
}

int atfisp_setparams(u64 shrd_paddr)
{
    atfd_hisi_service_isp_smc(ISP_FN_SET_PARAMS, shrd_paddr, 0, 0);
    return 0;
}

int atfisp_params_dump(u64 boot_pa_addr)
{
    atfd_hisi_service_isp_smc(ISP_FN_PARAMS_DUMP, boot_pa_addr, 0, 0);
    return 0;
}

int atfisp_cpuinfo_nsec_dump(u64 param_pa_addr)
{
    atfd_hisi_service_isp_smc(ISP_FN_ISPCPU_NS_DUMP, param_pa_addr, 0, 0);
    return 0;
}

void atfisp_isp_init(u64 pgd_base)
{
    atfd_hisi_service_isp_smc(ISP_FN_ISP_INIT, pgd_base, 0, 0);
}

void atfisp_isp_exit(void)
{
    atfd_hisi_service_isp_smc(ISP_FN_ISP_EXIT, 0, 0, 0);
}

int atfisp_ispcpu_init(void)
{
    return atfd_hisi_service_isp_smc(ISP_FN_ISPCPU_INIT, 0, 0, 0);
}

void atfisp_ispcpu_exit(void)
{
    atfd_hisi_service_isp_smc(ISP_FN_ISPCPU_EXIT, 1, 0, 0);
}

int atfisp_ispcpu_map(void)
{
    return atfd_hisi_service_isp_smc(ISP_FN_ISPCPU_MAP, 0, 0, 0);
}

void atfisp_ispcpu_unmap(void)
{
    atfd_hisi_service_isp_smc(ISP_FN_ISPCPU_UNMAP, 0, 0, 0);
}

void atfisp_set_nonsec(void)
{
    atfd_hisi_service_isp_smc(ISP_FN_SET_NONSEC, 0, 0, 0);
}

int atfisp_disreset_ispcpu(void)
{
    return atfd_hisi_service_isp_smc(ISP_FN_DISRESET_ISPCPU, 0, 0, 0);
}

void atfisp_ispsmmu_ns_init(u64 pgt_addr)
{
    atfd_hisi_service_isp_smc(ISP_FN_ISPSMMU_NS_INIT, pgt_addr, 0, 0);
}

int atfisp_get_ispcpu_idle(void)
{
    return atfd_hisi_service_isp_smc(ISP_FN_GET_ISPCPU_IDLE, 0, 0, 0);
}

int atfisp_isptop_power_up(void)
{
    return atfd_hisi_service_isp_smc(ISP_FN_ISPTOP_PU, 0, 0, 0);
}

int atfisp_isptop_power_down(void)
{
    return atfd_hisi_service_isp_smc(ISP_FN_ISPTOP_PD, 0, 0, 0) ;
}

