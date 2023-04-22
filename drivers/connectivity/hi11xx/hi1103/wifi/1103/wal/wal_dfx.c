


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_types.h"
#include "wal_dfx.h"
#include "oal_net.h"
#include "oal_cfg80211.h"
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "dmac_ext_if.h"
#include "hmac_ext_if.h"
#include "hmac_device.h"
#include "hmac_resource.h"
#include "hmac_ext_if.h"
#include "hmac_vap.h"
#include "hmac_p2p.h"
#include "wal_ext_if.h"
#include "wal_linux_cfg80211.h"
#include "wal_linux_scan.h"
#include "wal_linux_event.h"
#include "wal_ext_if.h"
#include "wal_config.h"
#include "wal_regdb.h"
#include "wal_linux_ioctl.h"
#include "mac_board.h"
#include "oam_ext_if.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_DFX_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_DFR

#define DFR_WAIT_PLAT_FINISH_TIME   (25000) /* 等待平台完成dfr工作的等待时间 */

oal_int8 *g_auc_dfr_error_type_etc[] = {   \
            "AP",    \
            "STA",   \
            "P2P0",  \
            "GO",  \
            "CLIENT",  \
            "DFR UNKOWN ERR TYPE!!"};

/* 此枚举为g_auc_dfr_error_type字符串的indx集合 */
typedef enum
{
    DFR_ERR_TYPE_AP = 0,
    DFR_ERR_TYPE_STA,
    DFR_ERR_TYPE_P2P,
    DFR_ERR_TYPE_GO,
    DFR_ERR_TYPE_CLIENT,

    DFR_ERR_TYPE_BUTT
} wal_dfr_error_type;
typedef oal_uint8 wal_dfr_error_type_enum_uint8;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern struct st_exception_info *g_pst_exception_info_etc;
#else
struct st_exception_info
{
    /* wifi异常触发 */
    oal_work_stru               wifi_excp_worker;
    oal_workqueue_stru         *wifi_exception_workqueue;
	oal_work_stru               wifi_excp_recovery_worker;
    oal_uint32                  wifi_excp_type;
}g_st_exception_info;
struct st_exception_info *g_pst_exception_info_etc = &g_st_exception_info;

struct st_wifi_dfr_callback
{
    void  (*wifi_recovery_complete)(void);
    void  (*notify_wifi_to_recovery)(void);
};
#endif

struct st_wifi_dfr_callback *g_st_wifi_callback_etc = OAL_PTR_NULL;

oal_void wal_dfr_init_param_etc(oal_void);

extern hmac_dfr_info_stru g_st_dfr_info_etc;

#endif //_PRE_WLAN_FEATURE_DFR
/*****************************************************************************
  3 函数实现
*****************************************************************************/

#ifdef _PRE_WLAN_FEATURE_DFR

OAL_STATIC oal_int32  wal_dfr_kick_all_user(hmac_vap_stru *pst_hmac_vap)
{
    wal_msg_write_stru              st_write_msg;
    wal_msg_stru                   *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32                      ul_err_code;
    oal_int32                       l_ret;
    mac_cfg_kick_user_param_stru   *pst_kick_user_param;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_KICK_USER, OAL_SIZEOF(mac_cfg_kick_user_param_stru));

    /* 设置配置命令参数 */
    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)(st_write_msg.auc_value);
    oal_set_mac_addr(pst_kick_user_param->auc_mac_addr, BROADCAST_MACADDR);

    /* 填写去关联reason code */
    pst_kick_user_param->us_reason_code = MAC_UNSPEC_REASON;
    if (OAL_PTR_NULL == pst_hmac_vap->pst_net_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_dfr_kick_all_user::pst_net_device is null!}");
        return OAL_SUCC;
    }

    l_ret = wal_send_cfg_event_etc(pst_hmac_vap->pst_net_device,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(mac_cfg_kick_user_param_stru),
                               (oal_uint8 *)&st_write_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_dfr_kick_all_user::return err code [%d]!}\r\n", l_ret);
        return l_ret;
    }


    /* 处理返回消息 */
    ul_err_code = wal_check_and_release_msg_resp_etc(pst_rsp_msg);
    if(OAL_SUCC != ul_err_code)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_dfr_kick_all_user::hmac start vap fail,err code[%u]!}\r\n", ul_err_code);
        return -OAL_EINVAL;
    }

    return OAL_SUCC;

}


oal_uint32  wal_process_p2p_excp_etc(hmac_vap_stru *pst_hmac_vap)
{
    mac_vap_stru     *pst_mac_vap;
    hmac_device_stru *pst_hmac_dev;

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_DFR,
                     "{hmac_process_sta_excp::Now begin P2P exception recovery program,del user[num:%d] when P2P state is P2P0[%d]/CL[%d]/GO[%d] .}",
                     pst_mac_vap->us_user_nums,
                     IS_P2P_DEV(pst_mac_vap),
                     IS_P2P_CL(pst_mac_vap),
                     IS_P2P_GO(pst_mac_vap));

    /* 删除用户 */
    wal_dfr_kick_all_user(pst_hmac_vap);

    /* AP模式还是STA模式 */
    if (IS_AP(pst_mac_vap))
    {
        /* vap信息初始化 */
        //hmac_dfr_reinit_ap(pst_hmac_vap);
    }
    else if (IS_STA(pst_mac_vap))
    {
        pst_hmac_dev = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
        if (OAL_PTR_NULL == pst_hmac_dev)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFR,
                             "{hmac_process_p2p_excp::pst_hmac_device is null, dev_id[%d].}",
                             pst_mac_vap->uc_device_id);

            return OAL_ERR_CODE_MAC_DEVICE_NULL;
        }
        /* 删除扫描信息列表，停止扫描 */
        if (pst_hmac_dev->st_scan_mgmt.st_scan_record_mgmt.uc_vap_id == pst_mac_vap->uc_vap_id)
        {
            pst_hmac_dev->st_scan_mgmt.st_scan_record_mgmt.p_fn_cb = OAL_PTR_NULL;
            pst_hmac_dev->st_scan_mgmt.en_is_scanning = OAL_FALSE;
        }
    }

    return OAL_SUCC;
}

oal_uint32  wal_process_ap_excp_etc(hmac_vap_stru *pst_hmac_vap)
{
    mac_vap_stru     *pst_mac_vap;

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFR,
                     "{hmac_process_sta_excp::Now begin AP exception recovery program, when AP have [%d] USERs.}",
                     pst_mac_vap->us_user_nums);

    /* 删除用户 */
    wal_dfr_kick_all_user(pst_hmac_vap);
    return OAL_SUCC;
}

oal_uint32  wal_process_sta_excp_etc(hmac_vap_stru *pst_hmac_vap)
{
    mac_vap_stru     *pst_mac_vap;
    hmac_device_stru *pst_hmac_dev;

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_hmac_dev = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFR,
                         "{hmac_process_sta_excp::pst_hmac_device is null, dev_id[%d].}",
                         pst_mac_vap->uc_device_id);

        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFR,
                     "{hmac_process_sta_excp::Now begin STA exception recovery program, when sta have [%d] users.}",
                     pst_mac_vap->us_user_nums);

    /* 关联状态下上报关联失败，删除用户 */
    wal_dfr_kick_all_user(pst_hmac_vap);

    /* 删除扫描信息列表，停止扫描 */
    if (pst_hmac_dev->st_scan_mgmt.st_scan_record_mgmt.uc_vap_id == pst_mac_vap->uc_vap_id)
    {
        pst_hmac_dev->st_scan_mgmt.st_scan_record_mgmt.p_fn_cb = OAL_PTR_NULL;
        pst_hmac_dev->st_scan_mgmt.en_is_scanning = OAL_FALSE;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_int32  wal_dfr_destroy_vap(oal_net_device_stru *pst_netdev)
{
    wal_msg_write_stru           st_write_msg;
    wal_msg_stru                *pst_rsp_msg = OAL_PTR_NULL;
    oal_uint32                  ul_err_code;

    oal_int32                    l_ret;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DESTROY_VAP, OAL_SIZEOF(oal_int32));
    l_ret = wal_send_cfg_event_etc(pst_netdev,
                        WAL_MSG_TYPE_WRITE,
                        WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_int32),
                        (oal_uint8 *)&st_write_msg,
                         OAL_TRUE,
                        &pst_rsp_msg);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAL_IO_PRINT("DFR DESTROY_VAP[name:%s] fail, return[%d]!", pst_netdev->name, l_ret);
        OAM_WARNING_LOG2(0, OAM_SF_DFR, "{wal_dfr_destroy_vap::DESTROY_VAP return err code [%d], iftype[%d]!}\r\n",
                        l_ret,
                        pst_netdev->ieee80211_ptr->iftype);

        return l_ret;
    }

    /* 读取返回的错误码 */
    ul_err_code = wal_check_and_release_msg_resp_etc(pst_rsp_msg);
    if(OAL_SUCC != ul_err_code)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_dfr_destroy_vap::hmac add vap fail, err code[%u]!}\r\n", ul_err_code);
        return ul_err_code;
    }

    OAL_NET_DEV_PRIV(pst_netdev) = OAL_PTR_NULL;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_dfr_recovery_env(void)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32                    ul_ret;
    oal_int32                     l_ret;
    oal_net_device_stru          *pst_netdev;
    wal_dfr_error_type_enum_uint8 en_err_type = DFR_ERR_TYPE_BUTT;
    oal_uint32                    ul_timeleft;
    oal_wireless_dev_stru        *pst_wireless_dev;

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_RECV_LASTWORD);

    if (OAL_TRUE != g_st_dfr_info_etc.bit_ready_to_recovery_flag)
    {
        return OAL_SUCC;
    }

    ul_timeleft = oal_wait_for_completion_timeout(&g_st_dfr_info_etc.st_plat_process_comp, OAL_MSECS_TO_JIFFIES(DFR_WAIT_PLAT_FINISH_TIME));
    if (!ul_timeleft)
    {
        OAM_ERROR_LOG1(0 , OAM_SF_DFR, "wal_dfr_recovery_env:wait dev reset timeout[%d]", DFR_WAIT_PLAT_FINISH_TIME);
        //return OAL_FAIL;
    }

    OAM_WARNING_LOG2(0, OAM_SF_ANY, "wal_dfr_recovery_env: get plat_process_comp signal after[%u]ms, netdev_num=%d!",
        (oal_uint32)OAL_JIFFIES_TO_MSECS(OAL_MSECS_TO_JIFFIES(DFR_WAIT_PLAT_FINISH_TIME) - ul_timeleft), g_st_dfr_info_etc.ul_netdev_num);

    /* 恢复vap, 上报异常给上层 */
    for (; (g_st_dfr_info_etc.ul_netdev_num > 0 && g_st_dfr_info_etc.ul_netdev_num<WLAN_VAP_SUPPORT_MAX_NUM_LIMIT); g_st_dfr_info_etc.ul_netdev_num--)
    {
        ul_ret = OAL_SUCC;
        pst_netdev = g_st_dfr_info_etc.past_netdev[g_st_dfr_info_etc.ul_netdev_num - 1];

        if (NL80211_IFTYPE_AP == pst_netdev->ieee80211_ptr->iftype)
        {

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
            wal_custom_cali_etc();
            hwifi_config_init_force_etc();
#endif
            l_ret = wal_cfg_vap_h2d_event_etc(pst_netdev);
            if(OAL_SUCC != l_ret)
            {
                OAM_ERROR_LOG1(0 , OAM_SF_DFR, "wal_dfr_recovery_env:DFR Device cfg_vap creat false[%d]!", l_ret);
                return OAL_FAIL;
            }

            /* host device_stru初始化*/
            l_ret = wal_host_dev_init_etc(pst_netdev);
            if(OAL_SUCC != l_ret)
            {
                OAM_ERROR_LOG1(0 , OAM_SF_DFR, "wal_dfr_recovery_env::DFR wal_host_dev_init_etc FAIL %d ", l_ret);
            }

            ul_ret = wal_setup_ap_etc(pst_netdev);
            en_err_type = DFR_ERR_TYPE_AP;
            oal_net_device_open(pst_netdev);


        }
        else if ((NL80211_IFTYPE_STATION == pst_netdev->ieee80211_ptr->iftype)||
                (NL80211_IFTYPE_P2P_DEVICE == pst_netdev->ieee80211_ptr->iftype))
        {
            l_ret = wal_netdev_open_etc(pst_netdev,OAL_FALSE);
            en_err_type = (!OAL_STRCMP(pst_netdev->name , "p2p0"))? DFR_ERR_TYPE_P2P : DFR_ERR_TYPE_STA;
        }
        else
        {
            pst_wireless_dev = OAL_NETDEVICE_WDEV(pst_netdev);

            /* 去注册netdev */
            oal_net_unregister_netdev(pst_netdev);
            OAL_MEM_FREE(pst_wireless_dev, OAL_TRUE);

            continue;
        }

        if (OAL_UNLIKELY(OAL_SUCC != l_ret) || OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAL_IO_PRINT("DFR BOOT_VAP[name:%s] fail! error_code[%d]", pst_netdev->name, ((oal_uint8)l_ret | (oal_uint8)ul_ret));
            OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_dfr_recovery_env:: Boot vap Failure, vap_iftype[%d], error_code[%d]!}\r\n",
                            pst_netdev->ieee80211_ptr->iftype,
                            ((oal_uint8)l_ret | (oal_uint8)ul_ret));
            continue;
        }


        /* 上报异常 */
        oal_cfg80211_rx_exception_etc(pst_netdev,
                                (oal_uint8 *)g_auc_dfr_error_type_etc[en_err_type],
                                OAL_STRLEN(g_auc_dfr_error_type_etc[en_err_type]));

    }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wlan_pm_enable_etc();
#endif

    g_st_dfr_info_etc.bit_device_reset_process_flag = OAL_FALSE;
    g_st_dfr_info_etc.bit_ready_to_recovery_flag    = OAL_FALSE;
    g_st_dfr_info_etc.ul_netdev_num                 = 0;

#endif

    return OAL_SUCC;

}



extern oal_int32 plat_exception_handler_etc(oal_uint32 subsys_type, oal_uint32 thread_type, oal_uint32 exception_type);
oal_uint32  wal_dfr_excp_process_etc(mac_device_stru *pst_mac_device, oal_uint32 ul_exception_type)
{
    hmac_vap_stru               *pst_hmac_vap;
    mac_vap_stru                *pst_mac_vap;
    oal_uint8                    uc_vap_idx;
    oal_int32                    l_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_int32                    l_plat_ret = OAL_SUCC;
#endif
    oal_net_device_stru          *pst_netdev = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_net_device_stru          *pst_p2p0_netdev = OAL_PTR_NULL;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{wal_dfr_excp_process_etc::pst_mac_device is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    //wlan_pm_disable_etc();
    wlan_pm_disable_check_wakeup_etc(OAL_FALSE);
#endif

    for (uc_vap_idx = pst_mac_device->uc_vap_num; uc_vap_idx > 0; uc_vap_idx--)
    {
        /* 获取最右边一位为1的位数，此值即为vap的数组下标 */
        pst_hmac_vap    = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->auc_vap_id[uc_vap_idx-1]);
        if (OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_dfr_excp_process_etc::mac_res_get_hmac_vap fail!vap_idx = %u}\r",pst_mac_device->auc_vap_id[uc_vap_idx-1]);
            continue;
        }
        pst_mac_vap     = &(pst_hmac_vap->st_vap_base_info);
        pst_netdev      = pst_hmac_vap->pst_net_device;

#ifdef _PRE_WLAN_FEATURE_P2P
        if (IS_P2P_DEV(pst_mac_vap))
        {
            pst_netdev = pst_hmac_vap->pst_p2p0_net_device;
        }
        else if (IS_P2P_CL(pst_mac_vap))
        {
            pst_p2p0_netdev = pst_hmac_vap->pst_p2p0_net_device;
            if (OAL_PTR_NULL != pst_p2p0_netdev)
            {
                g_st_dfr_info_etc.past_netdev[g_st_dfr_info_etc.ul_netdev_num]  = pst_p2p0_netdev;
                g_st_dfr_info_etc.ul_netdev_num ++;
            }
        }
#endif
        if (OAL_PTR_NULL == pst_netdev)
        {
            OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_dfr_excp_process_etc::pst_netdev NULL pointer !vap_idx = %u}\r",pst_mac_device->auc_vap_id[uc_vap_idx-1]);
            continue;
        }
        else if (OAL_PTR_NULL == pst_netdev->ieee80211_ptr)
        {
            OAM_WARNING_LOG1(0, OAM_SF_DFR, "{wal_dfr_excp_process_etc::ieee80211_ptr NULL pointer !vap_idx = %u}\r",pst_mac_device->auc_vap_id[uc_vap_idx-1]);
            continue;
        }

        g_st_dfr_info_etc.past_netdev[g_st_dfr_info_etc.ul_netdev_num]  = pst_netdev;
        g_st_dfr_info_etc.ul_netdev_num ++;

        OAM_WARNING_LOG4(0 , OAM_SF_DFR, "wal_dfr_excp_process_etc:vap_iftype [%d], vap_id[%d], vap_idx[%d], netdev_num[%d]",
                    pst_netdev->ieee80211_ptr->iftype,
                    pst_mac_vap->uc_vap_id,
                    uc_vap_idx,
                    g_st_dfr_info_etc.ul_netdev_num);

        wal_force_scan_complete_etc(pst_netdev, OAL_TRUE);
        wal_stop_sched_scan_etc(pst_netdev);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        mutex_lock(&g_st_dfr_info_etc.wifi_excp_mutex);
#endif

        oal_net_device_close(pst_netdev);
        l_ret = wal_dfr_destroy_vap(pst_netdev);
        if (OAL_UNLIKELY(OAL_SUCC != l_ret))
        {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
            mutex_unlock(&g_st_dfr_info_etc.wifi_excp_mutex);
#endif
            g_st_dfr_info_etc.ul_netdev_num--;
            continue;
        }

#ifdef _PRE_WLAN_FEATURE_P2P
        if (OAL_PTR_NULL != pst_p2p0_netdev)
        {
            l_ret = wal_dfr_destroy_vap(pst_p2p0_netdev);
            if (OAL_UNLIKELY(OAL_SUCC != l_ret))
            {
                OAM_WARNING_LOG0(0, OAM_SF_DFR, "{wal_dfr_excp_process_etc::DESTROY_P2P0 return err code [%d]!}\r\n");
                oal_net_unregister_netdev(pst_netdev);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
                mutex_unlock(&g_st_dfr_info_etc.wifi_excp_mutex);
#endif
                g_st_dfr_info_etc.ul_netdev_num--;
                continue;
            }
            pst_p2p0_netdev = OAL_PTR_NULL;
        }
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        mutex_unlock(&g_st_dfr_info_etc.wifi_excp_mutex);
#endif
    }

    /* device close& open */
    OAL_INIT_COMPLETION(&g_st_dfr_info_etc.st_plat_process_comp);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    l_plat_ret = plat_exception_handler_etc(SUBSYS_WIFI, THREAD_WIFI, ul_exception_type);
#endif

    //开始dfr恢复动作: wal_dfr_recovery_env();
    g_st_dfr_info_etc.bit_ready_to_recovery_flag = OAL_TRUE;
    oal_queue_work(g_pst_exception_info_etc->wifi_exception_workqueue, &g_pst_exception_info_etc->wifi_excp_recovery_worker);

    return OAL_SUCC;

}

oal_void wal_dfr_signal_complete_etc(oal_void)
{
    OAL_COMPLETE(&g_st_dfr_info_etc.st_plat_process_comp);
}



oal_uint32 wal_dfr_excp_rx_etc(oal_uint8 uc_device_id, oal_uint32 ul_exception_type)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint8                     uc_vap_idx;
    hmac_vap_stru                *pst_hmac_vap;
    mac_vap_stru                 *pst_mac_vap;
    mac_device_stru              *pst_mac_dev;

    pst_mac_dev = mac_res_get_dev_etc(uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(0 , OAM_SF_DFR, "wal_dfr_excp_rx_etc:ERROR dev_ID[%d] in DFR process!", uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 03暂作异常现场保留，不复位 */
    /*  异常复位开关是否开启 */
    if ((!g_st_dfr_info_etc.bit_device_reset_enable) || g_st_dfr_info_etc.bit_device_reset_process_flag)
    {
        OAM_ERROR_LOG1(0 , OAM_SF_DFR, "wal_dfr_excp_rx_etc:ERROR now in DFR process! bit_device_reset_process_flag=%d",
            g_st_dfr_info_etc.bit_device_reset_process_flag);
        return OAL_SUCC;
    }

    g_st_dfr_info_etc.bit_device_reset_process_flag = OAL_TRUE;
    g_st_dfr_info_etc.bit_user_disconnect_flag      = OAL_TRUE;
    g_st_dfr_info_etc.ul_dfr_num++;

    /* log现在进入异常处理流程 */
    OAM_WARNING_LOG3(0, OAM_SF_DFR, "{wal_dfr_excp_rx_etc:: Enter the exception processing[%d], type[%d] dev_ID[%d].}",
        g_st_dfr_info_etc.ul_dfr_num, ul_exception_type, uc_device_id);

    /* 按照每个vap模式进行异常处理 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_dev->uc_vap_num; uc_vap_idx++)
    {

        /* 获取最右边一位为1的位数，此值即为vap的数组下标 */
        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_dev->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_WARNING_LOG2(0, OAM_SF_DFR, "{wal_dfr_excp_rx_etc::mac_res_get_hmac_vap fail!vap_idx = %u dev_ID[%d]}\r",pst_mac_dev->auc_vap_id[uc_vap_idx], uc_device_id);
            continue;
        }

        pst_mac_vap  = &(pst_hmac_vap->st_vap_base_info);
        if (!IS_LEGACY_VAP(pst_mac_vap))
        {
            wal_process_p2p_excp_etc(pst_hmac_vap);
        }
        else if (IS_AP(pst_mac_vap))
        {
            wal_process_ap_excp_etc(pst_hmac_vap);
        }
        else if (IS_STA(pst_mac_vap))
        {
            wal_process_sta_excp_etc(pst_hmac_vap);
        }
        else
        {
            continue;
        }

    }

    CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_PLAT, CHR_PLAT_DRV_ERROR_WIFI_RECOVERY);
    
    return wal_dfr_excp_process_etc(pst_mac_dev, ul_exception_type);
#else
    return OAL_SUCC;
#endif
}

oal_uint32 wal_dfr_get_excp_type_etc(oal_void)
{
    return g_pst_exception_info_etc->wifi_excp_type;
}


void  wal_dfr_excp_work_etc(oal_work_stru *work)
{
    oal_uint32 ul_exception_type;
    oal_uint8  uc_device_index;

    ul_exception_type = wal_dfr_get_excp_type_etc();

    /* 暂不支持多chip */
    if (1 != WLAN_CHIP_MAX_NUM_PER_BOARD)
    {
        OAM_ERROR_LOG1(0, OAM_SF_DFR, "DFR Can not support muti_chip[%d].\n", WLAN_CHIP_MAX_NUM_PER_BOARD);
        return;
    }

    
    if (g_st_dfr_info_etc.bit_device_reset_process_flag)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_dfr_excp_work_etc:: dfr_process_status[%d]!}",
            g_st_dfr_info_etc.bit_device_reset_process_flag);
        return;
    }

    g_st_dfr_info_etc.ul_netdev_num = 0;
    OAL_MEMZERO((oal_uint8 *)(g_st_dfr_info_etc.past_netdev), OAL_SIZEOF(g_st_dfr_info_etc.past_netdev[0]) * WLAN_VAP_SUPPORT_MAX_NUM_LIMIT);

    for (uc_device_index = 0; uc_device_index < MAC_RES_MAX_DEV_NUM; uc_device_index++)
    {
        wal_dfr_excp_rx_etc(uc_device_index, ul_exception_type);
    }
}
void  wal_dfr_bfgx_excp_etc(void)
{
    wal_dfr_excp_work_etc(NULL);
}
void  wal_dfr_recovery_work_etc(oal_work_stru *work)
{
    wal_dfr_recovery_env();
}

oal_void wal_dfr_init_param_etc(oal_void)
{
    OAL_MEMZERO((oal_uint8 *)&g_st_dfr_info_etc, OAL_SIZEOF(hmac_dfr_info_stru));

    g_st_dfr_info_etc.bit_device_reset_enable        = OAL_TRUE;
    g_st_dfr_info_etc.bit_hw_reset_enable            = OAL_FALSE;
    g_st_dfr_info_etc.bit_soft_watchdog_enable       = OAL_FALSE;
    g_st_dfr_info_etc.bit_device_reset_process_flag  = OAL_FALSE;
    g_st_dfr_info_etc.bit_ready_to_recovery_flag     = OAL_FALSE;
    g_st_dfr_info_etc.bit_user_disconnect_flag       = OAL_FALSE;
    g_st_dfr_info_etc.ul_excp_type                   = 0xffffffff;

    /* use mutex in wal_netdev_open_etc and wal_netdev_stop_etc in case DFR and WiFi on/off conflict,
     * don't use mutex in wal_dfr_excp_work_etc, wal_dfr_recovery_work_etc, and wal_hipriv_test_dfr_start */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mutex_init(&g_st_dfr_info_etc.wifi_excp_mutex);
#endif
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_dfr_excp_init_handler(oal_void)
{
    oal_uint8             uc_device_index;
    hmac_device_stru     *pst_hmac_dev;
    struct st_wifi_dfr_callback *pst_wifi_callback = OAL_PTR_NULL;

    for (uc_device_index = 0; uc_device_index < MAC_RES_MAX_DEV_NUM; uc_device_index++)
    {
        pst_hmac_dev = hmac_res_get_mac_dev_etc(uc_device_index);
        if (OAL_PTR_NULL == pst_hmac_dev)
        {
            OAM_ERROR_LOG1(0 , OAM_SF_DFR, "wal_dfr_excp_init_handler:ERROR hmac dev_ID[%d] in DFR process!", uc_device_index);
            return OAL_ERR_CODE_PTR_NULL;
        }
    }
    /* 初始化dfr开关 */
    wal_dfr_init_param_etc();

    /* 挂接调用钩子 */
    if (OAL_PTR_NULL != g_pst_exception_info_etc)
    {
        OAL_INIT_WORK(&g_pst_exception_info_etc->wifi_excp_worker, wal_dfr_excp_work_etc);
        OAL_INIT_WORK(&g_pst_exception_info_etc->wifi_excp_recovery_worker, wal_dfr_recovery_work_etc);
        g_pst_exception_info_etc->wifi_exception_workqueue= OAL_CREATE_SINGLETHREAD_WORKQUEUE("wifi_exception_queue");
    }

    pst_wifi_callback = (struct st_wifi_dfr_callback *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(struct st_wifi_dfr_callback), OAL_TRUE);
    if (OAL_PTR_NULL == pst_wifi_callback)
    {
        OAM_ERROR_LOG1(0 , OAM_SF_DFR, "wal_init_dev_excp_handler:can not alloc mem,size[%d]!", OAL_SIZEOF(struct st_wifi_dfr_callback));
        g_st_wifi_callback_etc = OAL_PTR_NULL;
        return OAL_ERR_CODE_PTR_NULL;
    }
    g_st_wifi_callback_etc = pst_wifi_callback;
    pst_wifi_callback->wifi_recovery_complete = wal_dfr_signal_complete_etc;
    pst_wifi_callback->notify_wifi_to_recovery = wal_dfr_bfgx_excp_etc;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    plat_wifi_exception_rst_register_etc(pst_wifi_callback);
#endif

    OAM_WARNING_LOG0(0, OAM_SF_DFR, "DFR wal_init_dev_excp_handler init ok.\n");
    return OAL_SUCC;

}


OAL_STATIC oal_void wal_dfr_excp_exit_handler(oal_void)
{
    if (OAL_PTR_NULL != g_pst_exception_info_etc)
    {
        oal_cancel_work_sync(&g_pst_exception_info_etc->wifi_excp_worker);
        oal_cancel_work_sync(&g_pst_exception_info_etc->wifi_excp_recovery_worker);
        oal_destroy_workqueue(g_pst_exception_info_etc->wifi_exception_workqueue);
    }
    OAL_MEM_FREE(g_st_wifi_callback_etc, OAL_TRUE);

    OAM_WARNING_LOG0(0, OAM_SF_DFR, "wal_dfr_excp_exit_handler::DFR dev_excp_handler remove ok.");

}
#endif
#else
oal_uint32 wal_dfr_excp_rx_etc(oal_uint8 uc_device_id, oal_uint32 ul_exception_type)
{
    return OAL_SUCC;
}

#endif //_PRE_WLAN_FEATURE_DFR


oal_uint32 wal_dfx_init_etc(oal_void)
{
    oal_uint32      l_ret = OAL_SUCC;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_DFR
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    //genKernel_init();
    l_ret = init_dev_excp_handler_etc();
    if (OAL_SUCC != l_ret)
    {
        return l_ret;
    }
#endif
    l_ret = wal_dfr_excp_init_handler();
#endif //_PRE_WLAN_FEATURE_DFR
#endif

    return l_ret;
}

oal_void wal_dfx_exit_etc(oal_void)
{

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_DFR
    wal_dfr_excp_exit_handler();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    //genKernel_exit();
    deinit_dev_excp_handler_etc();
#endif
#endif //_PRE_WLAN_FEATURE_DFR
#endif

}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

