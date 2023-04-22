

#ifndef __OAL_EXT_IF_H__
#define __OAL_EXT_IF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_types.h"
#include "oal_util.h"
#include "oal_hardware.h"
#include "oal_schedule.h"
#include "oal_bus_if.h"
#include "oal_mem.h"
#include "oal_net.h"
#include "oal_list.h"
#include "oal_queue.h"
#include "oal_workqueue.h"
#include "arch/oal_ext_if.h"
#include "oal_thread.h"

//#include "oal_gpio.h"
#if (!defined(_PRE_PRODUCT_ID_HI110X_DEV))
#include "oal_aes.h"
#include "oal_gpio.h"
#endif

/*infusion检查对预编译宏支持不好，在此定义宏支持infusion检查，正式代码不需要*/
#ifdef _PRE_INFUSION_CHECK
#include "oal_infusion.h"
#endif
/*end infusion*/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_exception_rst.h"
#endif
#include "oal_fsm.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    OAL_TRACE_ENTER_FUNC,
    OAL_TRACE_EXIT_FUNC,

    OAL_TRACE_DIRECT_BUTT
}oal_trace_direction_enum;
typedef oal_uint8 oal_trace_direction_enum_uint8;

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
/* 黑名单模式 */
typedef enum
{
    CS_BLACKLIST_MODE_NONE,            /* 关闭         */
    CS_BLACKLIST_MODE_BLACK,           /* 黑名单       */
    CS_BLACKLIST_MODE_WHITE,           /* 白名单       */

    CS_BLACKLIST_MODE_BUTT
}cs_blacklist_mode_enum;
typedef oal_uint8 cs_blacklist_mode_enum_uint8;
#endif

#ifdef _PRE_WLAN_REPORT_WIFI_ABNORMAL
//上报驱动异常状态，并期望上层解决
typedef enum
{
    OAL_ABNORMAL_FRW_TIMER_BROKEN   = 0,    //frw定时器断链
    OAL_ABNORMAL_OTHER              = 1,    //其他异常

    OAL_ABNORMAL_BUTT
}oal_wifi_abnormal_reason_enum;

typedef enum
{
    OAL_ACTION_RESTART_VAP          = 0,    //重启vap，上层暂未实现
    OAL_ACTION_REBOOT               = 1,    //重启板子

    OAL_ACTION_BUTT
}oal_product_action_enum;

#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
//extern 上层函数，5v2 vendorID 固定为5200
extern void (*g_pf_abnormal_action)(int vendorID, int reason, int action, void *arg,  int size);
//arg 参数收地址，size 参数空间大小(字节数); 需要调用者保证参数和大小对应
#define OAL_REPORT_WIFI_ABNORMAL(_l_reason, _l_action, _p_arg, _l_size)            g_pf_abnormal_action(5200, _l_reason, _l_action, _p_arg,  _l_size)
#else
#define OAL_REPORT_WIFI_ABNORMAL(_l_reason, _l_action, _p_arg, _l_size)
#endif

#endif

#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
//下面定义需要和ont定义一致
typedef enum
{
    OAL_WIFI_STA_LEAVE          = 21,    //STA 离开
    OAL_WIFI_STA_JOIN           = 22,    //STA 加入

    OAL_WIFI_BUTT
}oal_wifi_sta_action_report_enum;
//sta上下线通知
extern void (*g_pf_wifi_asynchronous_event_report)(oal_uint32 uiIfIndex,   oal_uint32 uiEvent, void* pValue, oal_uint32 uiValueLen);
#define OAL_WIFI_REPORT_STA_ACTION(_ul_ifindex, _ul_eventID, _p_arg, _l_size)            g_pf_wifi_asynchronous_event_report(_ul_ifindex, _ul_eventID, _p_arg, _l_size)
#else
typedef enum
{
    OAL_WIFI_STA_LEAVE          = 0,    //STA 离开
    OAL_WIFI_STA_JOIN           = 1,    //STA 加入

    OAL_WIFI_BUTT
}oal_wifi_sta_action_report_enum;
#define OAL_WIFI_REPORT_STA_ACTION(_ul_ifindex, _ul_eventID, _p_arg, _l_size)
#endif


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
extern oal_void                       *g_pst_5115_sys_ctl;
#ifdef _PRE_WLAN_FEATURE_PCIE_ADAPT_5116
extern oal_void                       *g_pst_5115_sys_ctl1;
#endif
extern oal_void                       *g_pst_5610_mode;
extern oal_void                       *g_pst_5610_gpio;
#if(_PRE_PRODUCT_ID_HI1151==_PRE_PRODUCT_ID)
#if(_PRE_TARGET_PRODUCT_TYPE_5610DMB == _PRE_CONFIG_TARGET_PRODUCT)
extern oal_void                       *g_pst_5610_gpio1;
extern oal_void                       *g_pst_5610_gpio2;
#endif
#endif

/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_int32   oal_main_init_etc(oal_void);
extern oal_void    oal_main_exit_etc(oal_void);
extern oal_uint32 oal_chip_get_version_etc(oal_void);
extern oal_uint8 oal_chip_get_device_num_etc(oal_uint32 ul_chip_ver);
extern oal_uint8 oal_board_get_service_vap_start_id(oal_void);
#ifdef HAVE_HISI_NFC
extern oal_void save_nfc_lowpower_log_etc(oal_void);
extern oal_void hi_wlan_power_off_etc(void);
#endif

#ifdef _PRE_DEBUG_PROFILING
typedef int (*cyg_check_hook_t)(long this_func, long call_func, long direction);
extern void __cyg_profile_func_register(cyg_check_hook_t hook);
#endif

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    #define WITP_BUILD_VERSION "Hi1151_ut"
#elif !defined(WITP_BUILD_VERSION)
    #if _PRE_PRODUCT_ID_HI1151 == _PRE_PRODUCT_ID
        #if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
            #define WITP_BUILD_VERSION "5V2C01B530"
        #else
            #define WITP_BUILD_VERSION "5V2C01B530"
        #endif
    #else
        #define WITP_BUILD_VERSION "not Hi1151"
    #endif
#endif

#ifndef WITP_BUILD_ENV
#define WITP_BUILD_ENV "NDF"
#endif

#define WITP_BUILD_VERSION_ENV WITP_BUILD_VERSION "@" WITP_BUILD_ENV

#if (_PRE_PRODUCT_ID_HI1151 == _PRE_PRODUCT_ID) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && !defined(_PRE_WLAN_AIO)
#define DEFINE_GET_BUILD_VERSION_FUNC(mod)\
const oal_int8 * mod ## _get_build_version(oal_void) \
{ \
    const oal_int8 * puc_ ## mod ## _version = (oal_int8 *)WITP_BUILD_VERSION_ENV; \
    return puc_ ## mod ## _version; \
} \
oal_module_symbol(mod ## _get_build_version);

#define GET_BUILD_VERSION_FUNC(mod) \
({ \
    extern const oal_int8 * mod ## _get_build_version(oal_void); \
    mod ## _get_build_version(); \
})

#ifdef _PRE_WLAN_VERSION_CHECK
#define OAL_RET_ON_MISMATCH(_mod, ret) \
do \
{ \
    const oal_int8 *pc_mod_verison = (const oal_int8 *)GET_BUILD_VERSION_FUNC(_mod); \
    const oal_int8 *pc_oal_version = (const oal_int8 *)GET_BUILD_VERSION_FUNC(oal); \
    if (oal_strcmp(pc_mod_verison, pc_oal_version)) \
    { \
        OAL_IO_PRINT("\nERROR:module version mismatch:\n%s:%s\n%s:%s\n", \
                #_mod,  pc_mod_verison, "oal", pc_oal_version); \
        return ret; \
    } \
}while(0)
#else
#define OAL_RET_ON_MISMATCH(_mod, ret)
#endif
#else
#define DEFINE_GET_BUILD_VERSION_FUNC(mod)
#define GET_BUILD_VERSION_FUNC(mod)
#define OAL_RET_ON_MISMATCH(_mod, ret)
#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oal_ext_if.h */
