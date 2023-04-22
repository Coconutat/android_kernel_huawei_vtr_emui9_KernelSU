


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_profiling.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "wlan_spec.h"

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
#include "hal_ext_if.h"
#endif

#include "hmac_vap.h"
#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_data.h"
#include "hmac_ext_if.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
#include "hmac_vap.h"
#endif

#include "wal_main.h"
#include "wal_linux_bridge.h"

#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "hmac_device.h"
#include "hmac_resource.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_BRIDGE_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)


oal_net_dev_tx_enum  wal_vap_start_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev)
{
    return hmac_vap_start_xmit(pst_buf, pst_dev);
}

#endif


oal_uint8    g_sk_pacing_shift_etc = 8;
oal_net_dev_tx_enum  wal_bridge_vap_xmit_etc(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)) && defined(_PRE_WLAN_1103_TCP_SMALL_QUEUE_BUGFIX)
    sk_pacing_shift_update(pst_buf->sk, g_sk_pacing_shift_etc);
#endif

#ifdef _PRE_WLAN_FEATURE_GSO
    if (skb_linearize(pst_buf))
    {
        OAM_WARNING_LOG0(0, OAM_SF_RX, "{wal_bridge_vap_xmit::[GSO] failed at skb_linearize");
        dev_kfree_skb_any(pst_buf);
        return NETDEV_TX_OK;
    }
#endif

    return hmac_bridge_vap_xmit_etc(pst_buf, pst_dev);
}

/*lint -e19*/
oal_module_symbol(wal_bridge_vap_xmit_etc);
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

