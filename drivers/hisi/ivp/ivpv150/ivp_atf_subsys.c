#include <asm/compiler.h>
#include <linux/compiler.h>
#include "../ivp_log.h"
#include "ivp_atf_subsys.h"


noinline int atfd_hisi_service_ivp_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
    asm volatile(
        __asmeq("%0", "x0")
        __asmeq("%1", "x1")
        __asmeq("%2", "x2")
        __asmeq("%3", "x3")
        "smc    #0\n"
    : "+r" (function_id)
    : "r" (arg0), "r" (arg1), "r" (arg2));

    return (int)function_id;
}

int ivpatf_change_slv_secmod(unsigned int mode)
{
    ivp_dbg("change slv mode to %d", mode);
    atfd_hisi_service_ivp_smc(IVP_SLV_SECMODE, mode, 0, 0);
    return 0;
}

int ivpatf_change_mst_secmod(unsigned int mode)
{
    ivp_dbg("change mst mode to %d", mode);
    atfd_hisi_service_ivp_smc(IVP_MST_SECMODE, mode, 0, 0);
    return 0;
}
