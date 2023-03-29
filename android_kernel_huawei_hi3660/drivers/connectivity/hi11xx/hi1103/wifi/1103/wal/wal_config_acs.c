

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_SUPPORT_ACS

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "wal_main.h"
#include "oam_linux_netlink.h"
#include "wal_config_acs.h"
#include "wal_ext_if.h"
#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_device.h"
#include "hmac_ext_if.h"
#include "dmac_acs.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_CONFIG_ACS_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
extern oal_void  oam_netlink_ops_register_etc(oam_nl_cmd_enum_uint8 en_type, oal_uint32 (*p_func)(oal_uint8 *puc_data, oal_uint32 ul_len));
extern oal_void  oam_netlink_ops_unregister_etc(oam_nl_cmd_enum_uint8 en_type);
extern oal_int32  oam_netlink_kernel_send_etc(oal_uint8 *puc_data, oal_uint32 ul_data_len, oam_nl_cmd_enum_uint8 en_type);

/*****************************************************************************
  3 函数实现
*****************************************************************************/
#ifdef _PRE_SUPPORT_ACS

oal_uint32  wal_acs_netlink_recv_handle(frw_event_mem_stru *pst_event_mem)
{
    oal_uint32              ul_device_num;
    oal_uint32              ul_ret;
    mac_device_stru        *pst_mac_dev;
    mac_vap_stru           *pst_mac_vap;
    mac_acs_cmd_stru       *pst_acs_cmd_hdr;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_acs_netlink_recv_handle::pst_event_mem null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_acs_cmd_hdr = (mac_acs_cmd_stru *)frw_get_event_payload(pst_event_mem);

    /* 向所有DEVICE广播一份 */
    for (ul_device_num = 0; ul_device_num < MAC_RES_MAX_DEV_NUM; ul_device_num++)
    {
        pst_mac_dev = mac_res_get_dev_etc(ul_device_num);

        /* 设备不存在 */
        if (OAL_PTR_NULL == pst_mac_dev)
        {
            continue;
        }

        /* 设备未初始化 */
        if (OAL_FALSE == pst_mac_dev->en_device_state)
        {
            continue;
        }

        /* ACS未使能 */
        if (OAL_PTR_NULL == pst_mac_dev->pst_acs)
        {
            continue;
        }

        // note:假如没有任何业务VAP，则驱动收不到应用层的请求。
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_dev->auc_vap_id[0]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            continue;
        }

        ul_ret = hmac_config_set_acs_cmd(pst_mac_vap, OAL_SIZEOF(mac_acs_cmd_stru), (oal_uint8 *)pst_acs_cmd_hdr);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_acs_netlink_recv::app send cmd failed:cmd=%d seq=%d dev=%d}\r\n",
                    pst_acs_cmd_hdr->uc_cmd, pst_acs_cmd_hdr->ul_cmd_cnt, ul_device_num);
        }
    }

    return OAL_SUCC;
}


oal_uint32  wal_acs_netlink_recv(oal_uint8 *puc_data, oal_uint32 ul_len)
{
    mac_device_stru        *pst_mac_dev;
    mac_vap_stru           *pst_mac_vap;
    frw_event_mem_stru     *pst_event_mem;
    frw_event_stru         *pst_event;

    //随便获取一个dev
    pst_mac_dev = mac_res_get_dev_etc(0);

    /* 设备不存在 */
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_acs_netlink_recv:: pst_mac_dev NULL}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设备未初始化 */
    if (OAL_FALSE == pst_mac_dev->en_device_state)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_acs_netlink_recv:: mac_dev not init}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    // note:假如没有任何业务VAP，则驱动收不到应用层的请求。
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_dev->auc_vap_id[0]);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_acs_netlink_recv:: pst_mac_vap NULL}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(mac_acs_cmd_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{wal_acs_netlink_recv::pst_event_mem null.}");
        return OAL_FAIL;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CRX,
                       WAL_HOST_CRX_SUBTYPE_CFG_ACS,
                       OAL_SIZEOF(mac_acs_cmd_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    oal_memcopy(frw_get_event_payload(pst_event_mem), puc_data, ul_len);

    OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_acs_netlink_recv:: send acs msg to wal}\r\n");

    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}
#endif


oal_uint32 wal_acs_response_event_handler(frw_event_mem_stru *pst_event_mem)
{
    mac_acs_response_hdr_stru *pst_acs_resp_hdr;
    frw_event_stru            *pst_event;

    pst_event        = frw_get_event_stru(pst_event_mem);
    pst_acs_resp_hdr = (mac_acs_response_hdr_stru *)pst_event->auc_event_data;

    if (pst_acs_resp_hdr->uc_cmd == DMAC_ACS_CMD_DO_SCAN)
    {
        oal_uint32  puc_real_dat = *(oal_uint32 *)(pst_acs_resp_hdr + 1);

        pst_acs_resp_hdr = (mac_acs_response_hdr_stru *)puc_real_dat;
        oam_netlink_kernel_send_etc((oal_uint8 *)pst_acs_resp_hdr, pst_acs_resp_hdr->ul_len, OAM_NL_CMD_ACS);
        OAL_MEM_FREE((oal_void *)puc_real_dat, OAL_TRUE);
    }
    else
    {
        oam_netlink_kernel_send_etc((oal_uint8 *)pst_acs_resp_hdr, pst_acs_resp_hdr->ul_len, OAM_NL_CMD_ACS);
    }

    return OAL_SUCC;
}

oal_uint32  wal_acs_init(oal_void)
{
    oam_netlink_ops_register_etc(OAM_NL_CMD_ACS, wal_acs_netlink_recv);

    return OAL_SUCC;
}


oal_uint32 wal_acs_exit(oal_void)
{
    oam_netlink_ops_unregister_etc(OAM_NL_CMD_ACS);

    return OAL_SUCC;
}

#endif /* #ifdef _PRE_SUPPORT_ACS */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
