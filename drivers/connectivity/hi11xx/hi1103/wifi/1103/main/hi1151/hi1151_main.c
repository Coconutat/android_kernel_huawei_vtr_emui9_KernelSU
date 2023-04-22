
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "frw_ext_if.h"

#include "wlan_spec.h"
#include "hal_ext_if.h"
#include "dmac_ext_if.h"
#include "dmac_alg.h"
#include "alg_ext_if.h"

#include "hmac_ext_if.h"
#include "wal_ext_if.h"
#include "sdt_drv.h"
#include "hisi_customize_wifi.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_BLD_RAW_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_int32  hi1151_main_init(oal_void)
{
    oal_int32 ul_return = OAL_FAIL;

    ul_return = oal_main_init_etc();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("oal_main_init_etc failed, ret = %d", ul_return);
        return ul_return;
    }

    ul_return = oam_main_init_etc();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("oam_main_init_etc failed, ret = %d", ul_return);
        return ul_return;
    }

#if (_PRE_PRODUCT_ID_HI1151==_PRE_PRODUCT_ID)
    ul_return = plat_main_init();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("plat_main_init failed, ret = %d", ul_return);
        return ul_return;
    }
#endif

    ul_return = frw_main_init_etc();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("frw_main_init_etc failed, ret = %d", ul_return);
        return ul_return;
    }

    ul_return = hal_main_init();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("hal_main_init failed, ret = %d", ul_return);
        return ul_return;
    }

    ul_return = dmac_main_init();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("dmac_main_init failed, ret = %d", ul_return);
        return ul_return;
    }

    ul_return = hmac_main_init_etc();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("hmac_main_init_etc failed, ret = %d", ul_return);
        return ul_return;
    }

    ul_return = wal_main_init_etc();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("wal_main_init_etc failed, ret = %d", ul_return);
        return ul_return;
    }

    ul_return = alg_main_init();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("alg_main_init failed, ret = %d", ul_return);
        return ul_return;
    }

    ul_return = sdt_drv_main_init_etc();
    if (OAL_SUCC != ul_return)
    {
        OAL_IO_PRINT("sdt_main_init failed, ret = %d", ul_return);
        return ul_return;
    }

    return OAL_SUCC;
}



oal_void  hi1151_main_exit(oal_void)
{
    sdt_drv_main_exit_etc();
    alg_main_exit();
    wal_main_exit_etc();
    hmac_main_exit_etc();
    dmac_main_exit();
    hal_main_exit();
    frw_main_exit_etc();
#if (_PRE_PRODUCT_ID_HI1151==_PRE_PRODUCT_ID)
    plat_main_exit();
#endif
    oam_main_exit_etc();
    oal_main_exit_etc();

    return ;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
module_init(hi1151_main_init);
module_exit(hi1151_main_exit);
#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

