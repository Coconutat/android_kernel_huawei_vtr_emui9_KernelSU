



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_spec.h"
#include "mac_board.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_BOARD_ROM_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/



/*****************************************************************************
DFX公用全局变量定义
*****************************************************************************/
#ifdef _PRE_WLAN_DFT_STAT
dfx_performance_log_switch_enum_uint8 g_auc_dfx_performance_log_switch_etc[DFX_PERFORMANCE_LOG_BUTT] = {0};
#endif
hisi_device_board_enum_uint8 g_en_chip_type = BOARD_VERSION;

/*****************************************************************************
  3 函数实现
*****************************************************************************/


/*lint +e19*/
/*****************************************************************************
DFX公用函数实现
*****************************************************************************/
#ifdef _PRE_WLAN_DFT_STAT

oal_uint32 dfx_get_performance_log_switch_enable_etc(dfx_performance_log_switch_enum_uint8 uc_performance_log_switch_type)
{
    return g_auc_dfx_performance_log_switch_etc[uc_performance_log_switch_type];
}

oal_void dfx_set_performance_log_switch_enable_etc(dfx_performance_log_switch_enum_uint8 uc_performance_log_switch_type, oal_uint8 uc_value)
{
    if(DFX_PERFORMANCE_LOG_BUTT <= uc_performance_log_switch_type)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "dfx_set_performance_log_switch_enable_etc::log_switch type:%d.", uc_performance_log_switch_type);
        return;
    }
    g_auc_dfx_performance_log_switch_etc[uc_performance_log_switch_type] = uc_value;
}
#endif


/*lint -e19*/
oal_module_symbol(g_en_chip_type);
/*lint +e19*/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif



