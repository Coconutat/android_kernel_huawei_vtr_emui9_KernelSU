


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "wlan_spec.h"
#include "mac_resource.h"
#include "mac_device.h"
#ifdef _PRE_WLAN_FEATURE_11AX
#include "mac_user.h"
#endif
//#include "mac_11i.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_USER_C
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


#ifdef _PRE_WLAN_FEATURE_11AX

oal_void mac_user_set_he_capable(mac_user_stru *pst_mac_user, oal_bool_enum_uint8 en_he_capable)
{
    pst_mac_user->st_he_hdl.en_he_capable = en_he_capable;
}


oal_bool_enum_uint8 mac_user_get_he_capable(mac_user_stru *pst_mac_user)
{
    return pst_mac_user->st_he_hdl.en_he_capable;
}


oal_void mac_user_get_he_hdl(mac_user_stru *pst_mac_user, mac_he_hdl_stru *pst_he_hdl)
{
    oal_memcopy((oal_uint8 *)pst_he_hdl, (oal_uint8 *)(&pst_mac_user->st_he_hdl), OAL_SIZEOF(mac_he_hdl_stru));
}


oal_void mac_user_set_he_hdl(mac_user_stru *pst_mac_user, mac_he_hdl_stru *pst_he_hdl)
{
   oal_memcopy((oal_uint8 *)(&pst_mac_user->st_he_hdl), (oal_uint8 *)pst_he_hdl, OAL_SIZEOF(mac_he_hdl_stru));
}
#endif



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

