

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "oal_net.h"
#include "oal_cfg80211.h"
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "wal_linux_cfgvendor.h"
#include "wal_linux_ioctl.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_CFGVENDOR_C

#define OUI_GOOGLE  0x001A11
#define OUI_HISI    0x001018

#if (defined(_PRE_PRODUCT_ID_HI110X_HOST) || (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern oal_uint32 band_5g_enabled;

wal_cfgvendor_radio_stat_stru g_st_wifi_radio_stat;


OAL_STATIC oal_void wal_cfgvendor_del_radar_channel(mac_vendor_cmd_channel_list_stru *pst_channel_list)
{
    oal_uint8 uc_loops;
    oal_uint8 uc_idx;
    oal_uint8 uc_channel_num;
    oal_uint8 uc_channel_idx;
    oal_uint8 auc_tmp_channel_list[MAC_CHANNEL_FREQ_5_BUTT];

    if (pst_channel_list->uc_channel_num_5g == 0)
    {
        return;
    }

    uc_channel_idx = 0;
    uc_channel_num = OAL_MIN(MAC_CHANNEL_FREQ_5_BUTT, pst_channel_list->uc_channel_num_5g);

    for (uc_loops = 0; uc_loops < uc_channel_num; uc_loops++)
    {
        if (OAL_SUCC != mac_get_channel_idx_from_num(MAC_RC_START_FREQ_5, pst_channel_list->auc_channel_list_5g[uc_loops], &uc_idx))
        {
            continue;
        }
        if (OAL_TRUE == mac_is_ch_in_radar_band(MAC_RC_START_FREQ_5, uc_idx))
        {
            continue;
        }

        auc_tmp_channel_list[uc_channel_idx++] = pst_channel_list->auc_channel_list_5g[uc_loops];
    }
    pst_channel_list->uc_channel_num_5g = uc_channel_idx;
    oal_memcopy(pst_channel_list->auc_channel_list_5g, auc_tmp_channel_list, uc_channel_idx);
}


OAL_STATIC oal_uint32 wal_cfgvendor_copy_channel_list(mac_vendor_cmd_channel_list_stru *pst_channel_list,
                                                            oal_uint32   band,
                                                            oal_int32  * pl_channel_list,
                                                            oal_uint32 * pul_num_channels)
{
    oal_uint8  *puc_chanel_list;
    oal_uint32  ul_loop;
    oal_uint32  ul_channel_num = 0;

    *pul_num_channels = 0;

    if ((0 == pst_channel_list->uc_channel_num_5g) && (0 == pst_channel_list->uc_channel_num_2g))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_copy_channel_list::get 5G or 2G channel list fail,band = %d",band);
        return OAL_FAIL;
    }

    /* 获取2G,信道列表 */
    if ((band & VENDOR_BG_BAND_MASK) && (0 != pst_channel_list->uc_channel_num_2g))
    {
        puc_chanel_list = pst_channel_list->auc_channel_list_2g;
        for (ul_loop = 0; ul_loop < pst_channel_list->uc_channel_num_2g; ul_loop++)
        {
            pl_channel_list[ul_channel_num++] = oal_ieee80211_channel_to_frequency(puc_chanel_list[ul_loop],IEEE80211_BAND_2GHZ);
        }
    }

    /* 获取5G,信道列表 */
    if ((band & VENDOR_A_BAND_MASK) && (0 != pst_channel_list->uc_channel_num_5g))
    {
        puc_chanel_list = pst_channel_list->auc_channel_list_5g;

        for (ul_loop = 0; ul_loop < pst_channel_list->uc_channel_num_5g; ul_loop++)
        {
            pl_channel_list[ul_channel_num++] = oal_ieee80211_channel_to_frequency(puc_chanel_list[ul_loop],IEEE80211_BAND_5GHZ);
        }
    }

    if (0 == ul_channel_num)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_copy_channel_list::get 5G or 2G channel list fail,ul_channel_num = %d",ul_channel_num);
        return OAL_FAIL;
    }

    *pul_num_channels = ul_channel_num;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_cfgvendor_get_current_channel_list(oal_net_device_stru *pst_netdev,
                                                    oal_uint32   band,
                                                    oal_int32  * pl_channel_list,
                                                    oal_uint32 * pul_num_channels)
{
    oal_int32                     l_ret;
    wal_msg_query_stru            st_query_msg;
    wal_msg_stru                 *pst_rsp_msg = OAL_PTR_NULL;
    wal_msg_rsp_stru             *pst_query_rsp_msg;
    mac_vendor_cmd_channel_list_stru *pst_channel_list = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_netdev || OAL_PTR_NULL == pl_channel_list || OAL_PTR_NULL == pul_num_channels)
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{wal_cfgvendor_get_current_channel_list::channel_list or num_channel is NULL!"
                                        "netdev %p, channel_list %p, num_channels %p.}",
                                        pst_netdev, pl_channel_list, pul_num_channels);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 上层在任何时候都可能下发此命令，需要先判断当前netdev的状态并及时返回 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == OAL_NET_DEV_PRIV(pst_netdev)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_get_current_channel_list::NET_DEV_PRIV is NULL.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 消息参数准备 */
    st_query_msg.en_wid = WLAN_CFGID_VENDOR_CMD_GET_CHANNEL_LIST;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_netdev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_get_current_channel_list:: wal_send_cfg_event return err code %d!}\r\n", l_ret);
        if (OAL_PTR_NULL != pst_rsp_msg)
        {
            oal_free(pst_rsp_msg);
        }
        return (oal_uint32)l_ret;/* [false alarm] */
    }

    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    pst_channel_list  = (mac_vendor_cmd_channel_list_stru *)(pst_query_rsp_msg->auc_value);

    /* 从信道列表中删除DFS 信道不上报 */
    wal_cfgvendor_del_radar_channel(pst_channel_list);

    if (wal_cfgvendor_copy_channel_list(pst_channel_list, band, pl_channel_list, pul_num_channels) != OAL_SUCC)
    {
        oal_free(pst_rsp_msg);
        return OAL_FAIL;
    }

    oal_free(pst_rsp_msg);
    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_cfgvendor_get_channel_list(oal_wiphy_stru *wiphy,
    oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int32  err = 0;
    oal_int32  type;
    oal_int32  al_channel_list[VENDOR_CHANNEL_LIST_ALL + 1] = {0};
    oal_uint32 band = 0; /* 上层下发的band值 */
    oal_uint32 mem_needed;
    oal_uint32 num_channels = 0;
    oal_uint32 reply_len = 0;
    oal_uint32 ul_ret;
    oal_netbuf_stru *skb;

    if (wdev == OAL_PTR_NULL || wdev->netdev == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::wdev or netdev is NULL! wdev %p.}", wdev);
        return -OAL_EFAIL;
    }

    type = oal_nla_type(data);
    if (type == GSCAN_ATTRIBUTE_BAND)
    {
        /* 获取band值 bit0:2G信道列表 bit1:5G信道列表 */
        band = oal_nla_get_u32(data);
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::invalid nla type! type %d.}", type);
        return -OAL_EFAIL;
    }

    ul_ret = wal_cfgvendor_get_current_channel_list(wdev->netdev, band, al_channel_list, &num_channels);
    if (ul_ret != OAL_SUCC)
    {
        OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::fail. ret_value %d, band %d, num_channels %d.}",
                        ul_ret, band, num_channels);
        return -OAL_EFAIL;
    }

    reply_len = OAL_SIZEOF(al_channel_list[0])*(num_channels);
    mem_needed = reply_len + VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);

    /* 申请SKB 上报信道列表 */
    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_channel_list::skb alloc failed.len %d\r\n",  mem_needed);
        return -OAL_ENOMEM;
    }

    oal_nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_CHANNELS, num_channels);
    oal_nla_put(skb, GSCAN_ATTRIBUTE_CHANNEL_LIST, reply_len, al_channel_list);

    err = oal_cfg80211_vendor_cmd_reply(skb);
    if (OAL_UNLIKELY(err))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_channel_list::Vendor Command reply failed ret:%d.\r\n", err);
    }

    OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::ret_value %d, band %d, num_channels %d.}",
                    err, band, num_channels);

    return err;
}


OAL_STATIC oal_int32 wal_cfgvendor_set_country(oal_wiphy_stru *wiphy,
    oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int32 rem;
    oal_int32 type;
    oal_int32 l_ret = -OAL_EFAIL;
    oal_int8  auc_country_code[WLAN_COUNTRY_STR_LEN] = {0};
    OAL_CONST oal_nlattr_stru *iter;

    OAL_NLA_FOR_EACH_ATTR(iter, data, len, rem)
    {
        OAL_MEMZERO(auc_country_code, WLAN_COUNTRY_STR_LEN);
        type = oal_nla_type(iter);
        switch (type)
        {
            case ANDR_WIFI_ATTRIBUTE_COUNTRY:
#ifdef _PRE_WLAN_FEATURE_11D
                oal_memcopy(auc_country_code, oal_nla_data(iter),
                            OAL_MIN(oal_nla_len(iter), OAL_SIZEOF(auc_country_code)));
                OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_cfgvendor_set_country::country code:0x%X 0x%X 0x%X, len = %d!}\r\n",
                                 auc_country_code[0], auc_country_code[1], auc_country_code[2], oal_nla_len(iter));
                /* 设置国家码到wifi 驱动 */
                l_ret = wal_regdomain_update_country_code(wdev->netdev, auc_country_code);
#else
                OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_set_country::_PRE_WLAN_FEATURE_11D is not define!}\r\n");
#endif
                break;
            default:
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_country::unknow type:%d!}\r\n", type);
                break;
        }
    }

    return l_ret;
}

OAL_STATIC oal_int32 wal_cfgvendor_do_get_feature_set(oal_void)
{
    oal_int32 l_feature_set = 0;

#ifdef _PRE_WLAN_FEATURE_HS20
    l_feature_set |= WIFI_FEATURE_HOTSPOT;
#endif

    if (band_5g_enabled)
    {
        l_feature_set |= WIFI_FEATURE_INFRA_5G;
    }

    l_feature_set |= WIFI_FEATURE_LINK_LAYER_STATS; /** 0x10000 Link layer stats collection */

    return l_feature_set;
}

OAL_STATIC oal_int32 wal_cfgvendor_get_feature_set(oal_wiphy_stru *wiphy,
    oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int32 l_err = 0;
    oal_int32 l_reply;
    oal_int32 l_reply_len = OAL_SIZEOF(oal_int32);
    oal_netbuf_stru *skb;

    l_reply = wal_cfgvendor_do_get_feature_set();

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, l_reply_len);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_feature_set::skb alloc failed.len %d\r\n", l_reply_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, l_reply_len, &l_reply);

    l_err = oal_cfg80211_vendor_cmd_reply(skb);
    if (OAL_UNLIKELY(l_err))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_feature_set::Vendor Command reply failed ret:%d.\r\n", l_err);
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_get_feature_set::fset flag:0x%x.\r\n}", l_reply);

    return l_err;
}



OAL_STATIC oal_int32 wal_send_random_mac_oui(oal_net_device_stru *pst_net_dev,
                                             oal_uint8 *auc_random_mac_oui)
{
    wal_msg_write_stru st_write_msg;
    oal_int32          l_ret;

    if(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == auc_random_mac_oui)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_send_random_mac_oui:: null point argument,pst_net_dev:%p, auc_random_mac_oui:%p.}",
                       pst_net_dev, auc_random_mac_oui);
        return -OAL_EFAIL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RANDOM_MAC_OUI, WLAN_RANDOM_MAC_OUI_LEN);
    oal_memcopy(st_write_msg.auc_value, auc_random_mac_oui, WLAN_RANDOM_MAC_OUI_LEN);

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + WLAN_RANDOM_MAC_OUI_LEN,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_send_random_mac_oui::send rand_mac_oui failed, err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return l_ret;
}




OAL_STATIC oal_int32 wal_cfgvendor_set_random_mac_oui(oal_wiphy_stru *pst_wiphy,
                                                                 oal_wireless_dev_stru *pst_wdev,
                                                                 OAL_CONST oal_void  *p_data,
                                                                 oal_int32 l_len)
{
    oal_int32 l_type;
    oal_int32 l_ret;
    oal_uint8 auc_random_mac_oui[WLAN_RANDOM_MAC_OUI_LEN];

    l_type = oal_nla_type(p_data);

    if (ANDR_WIFI_ATTRIBUTE_RANDOM_MAC_OUI == l_type)
    {
        /* 随机mac地址前3字节(mac oui)由Android下发,wps pbc场景和hilink关联场景会将此3字节清0 */
        oal_memcopy(auc_random_mac_oui, oal_nla_data(p_data), WLAN_RANDOM_MAC_OUI_LEN);
        OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_cfgvendor_set_random_mac_oui::mac_ou:0x%.2x:%.2x:%.2x}\r\n",
                         auc_random_mac_oui[0], auc_random_mac_oui[1], auc_random_mac_oui[2]);
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_random_mac_oui:: unknow type:%x}\r\n", l_type);
        return -OAL_EFAIL;
    }

    l_ret = wal_send_random_mac_oui(pst_wdev->netdev, auc_random_mac_oui);
    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_cfgvendor_dbg_get_feature(oal_wiphy_stru *wiphy,
        oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int32 l_ret;
    oal_uint32 supported_features = VENDOR_DBG_MEMORY_DUMP_SUPPORTED;
    oal_netbuf_stru *skb;

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, OAL_SIZEOF(oal_uint32));
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_feature::skb alloc failed. len %d\r\n", OAL_SIZEOF(oal_uint32));
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, OAL_SIZEOF(oal_uint32), &supported_features);

    l_ret = oal_cfg80211_vendor_cmd_reply(skb);
    if (OAL_UNLIKELY(l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_feature::Vendor Command reply failed. ret:%d.\r\n", l_ret);
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_feature::SUCCESS. supported feature:0x%X.\r\n", supported_features);

    return l_ret;
}


OAL_STATIC oal_int32 wal_cfgvendor_dbg_get_version(oal_wiphy_stru *wiphy,
        oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int8 auc_driver_version[] = "Hisi Host Driver, version Hi1102 V100";
    oal_int32 l_ret;
    oal_int32 l_buf_len = OAL_SIZEOF(auc_driver_version);
    oal_netbuf_stru *skb;

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, l_buf_len);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::skb alloc failed.len %d\r\n", l_buf_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, l_buf_len, auc_driver_version);

    l_ret = oal_cfg80211_vendor_cmd_reply(skb);
    if (OAL_UNLIKELY(l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::Vendor Command reply failed. ret:%d.\r\n", l_ret);
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::SUCCESS. driver version len %d\r\n", l_buf_len);
    return l_ret;
}


oal_int32 wal_cfgvendor_dbg_get_ring_status(oal_wiphy_stru *wiphy,
        oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int32 l_ret,l_buf_len;
    debug_ring_status_st debug_ring_status = {{VENDOR_FW_EVENT_RING_NAME},0,FW_EVENT_RING_ID,
                                                VENDOR_FW_EVENT_RING_SIZE,0,0,0,0};
    oal_netbuf_stru *skb;
    l_buf_len = OAL_SIZEOF(debug_ring_status_st);

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, l_buf_len+100);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_status::skb alloc failed.len %d\r\n", l_buf_len+100);
        return -OAL_ENOMEM;
    }

    oal_nla_put_u32(skb, DEBUG_ATTRIBUTE_RING_NUM, 1);
    oal_nla_put(skb, DEBUG_ATTRIBUTE_RING_STATUS, l_buf_len,&debug_ring_status);

    l_ret = oal_cfg80211_vendor_cmd_reply(skb);
    if (OAL_UNLIKELY(l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_status::Vendor Command reply failed. ret:%d.\r\n", l_ret);
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_status::SUCCESS. driver version len %d\r\n", l_buf_len);
    return l_ret;
}


oal_int32 wal_cfgvendor_dbg_trigger_mem_dump(oal_wiphy_stru *wiphy,
        oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_trigger_mem_dump::ENTER");
    return 0;
}


OAL_STATIC oal_int32 wal_cfgvendor_dbg_start_logging(oal_wiphy_stru *wiphy,
        oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_start_logging::ENTER");
    return 0;
}


OAL_STATIC oal_int32 wal_cfgvendor_dbg_get_ring_data(oal_wiphy_stru *wiphy,
        oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_data::ENTER");
    return 0;
}


OAL_STATIC oal_int32 wal_cfgvendor_lstats_get_info(oal_wiphy_stru *pst_wiphy,
        oal_wireless_dev_stru *pst_wdev, OAL_CONST oal_void  *p_data, oal_int32 l_len)
{
    oal_int32            l_err = 0;
    oal_uint32           ul_reply_len;
    oal_void            *p_out_data;
    oal_netbuf_stru     *pst_skb;
    wal_wifi_radio_stat_stru *pst_radio_stat;
    wal_wifi_iface_stat_stru *pst_iface_stat;

    if (pst_wiphy == OAL_PTR_NULL || pst_wdev == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY,
                        "{wal_cfgvendor_lstats_get_info:wiphy or wdev or data is null. %p, %p}",
                        pst_wiphy,
                        pst_wdev);
        return -OAL_EFAUL;
    }

    ul_reply_len = OAL_SIZEOF(*pst_radio_stat) + OAL_SIZEOF(*pst_iface_stat) ;
    p_out_data = (oal_void *)oal_memalloc(ul_reply_len);
    if (p_out_data == OAL_PTR_NULL)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info:alloc memory fail.[%d]}", ul_reply_len);
        return -OAL_ENOMEM;
    }
    OAL_MEMZERO(p_out_data, ul_reply_len);

    /* 获取radio 统计 */
    pst_radio_stat = (wal_wifi_radio_stat_stru *)p_out_data;
    pst_radio_stat->ul_num_channels = VENDOR_NUM_CHAN;
    pst_radio_stat->ul_on_time      = OAL_JIFFIES_TO_MSECS(OAL_TIME_JIFFY - g_st_wifi_radio_stat.ull_wifi_on_time_stamp);
    pst_radio_stat->ul_tx_time      = 0;
    pst_radio_stat->ul_rx_time      = 0;

    /* 获取interfac 统计 */
    pst_iface_stat = (wal_wifi_iface_stat_stru *)(p_out_data + OAL_SIZEOF(*pst_radio_stat));
    pst_iface_stat->ul_num_peers           = VENDOR_NUM_PEER;
    pst_iface_stat->peer_info->ul_num_rate = VENDOR_NUM_RATE;

    /* 上报link 统计 */
    pst_skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(pst_wiphy, ul_reply_len);
    if (OAL_UNLIKELY(!pst_skb))
    {
        oal_free(p_out_data);
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info::skb alloc failed.len %d}", ul_reply_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(pst_skb, ul_reply_len, p_out_data);

    l_err =  oal_cfg80211_vendor_cmd_reply(pst_skb);
    OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info::on_time %d, tx_time %d, rx_time %d, err %d",
                    pst_radio_stat->ul_on_time,
                    pst_radio_stat->ul_tx_time,
                    pst_radio_stat->ul_rx_time,
                    l_err);
    oal_free(p_out_data);
    return l_err;
}

OAL_STATIC OAL_CONST oal_wiphy_vendor_command_stru wal_vendor_cmds[] =
{
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = GSCAN_SUBCMD_GET_CHANNEL_LIST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_get_channel_list
    },

    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = ANDR_WIFI_SET_COUNTRY
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_set_country
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = ANDR_WIFI_SUBCMD_GET_FEATURE_SET
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_get_feature_set
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = ANDR_WIFI_RANDOM_MAC_OUI
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_set_random_mac_oui
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = DEBUG_GET_FEATURE
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_get_feature
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = DEBUG_GET_VER
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_get_version
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = DEBUG_GET_RING_STATUS
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_get_ring_status
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = DEBUG_TRIGGER_MEM_DUMP
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_trigger_mem_dump
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = DEBUG_START_LOGGING
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_start_logging
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = DEBUG_GET_RING_DATA
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_get_ring_data
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = LSTATS_SUBCMD_GET_INFO
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_lstats_get_info
    }
};

OAL_STATIC OAL_CONST oal_nl80211_vendor_cmd_info_stru wal_vendor_events[] =
{
    { OUI_HISI, HISI_VENDOR_EVENT_UNSPEC },
    { OUI_HISI, HISI_VENDOR_EVENT_PRIV_STR }
};



oal_void wal_cfgvendor_init(oal_wiphy_stru *wiphy)
{
    wiphy->vendor_commands  = wal_vendor_cmds;
    wiphy->n_vendor_commands = OAL_ARRAY_SIZE(wal_vendor_cmds);
    wiphy->vendor_events    = wal_vendor_events;
    wiphy->n_vendor_events  = OAL_ARRAY_SIZE(wal_vendor_events);
}

oal_void wal_cfgvendor_deinit(oal_wiphy_stru *wiphy)
{
    wiphy->vendor_commands  = NULL;
    wiphy->vendor_events    = NULL;
    wiphy->n_vendor_commands = 0;
    wiphy->n_vendor_events  = 0;
}

#endif  /* (defined(_PRE_PRODUCT_ID_HI110X_HOST) || (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

