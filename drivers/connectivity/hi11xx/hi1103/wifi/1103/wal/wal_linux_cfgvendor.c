

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
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_CFGVENDOR_C

#define OUI_GOOGLE  0x001A11
#define OUI_HISI    0x001018

#if (defined(_PRE_PRODUCT_ID_HI110X_HOST) || (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
wal_cfgvendor_radio_stat_stru g_st_wifi_radio_stat_etc;


#if 0
OAL_STATIC oal_int32 wal_cfgvendor_get_gscan_capabilities(oal_wiphy_stru *wiphy,
                                                          oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int32  ul_ret = 0;
    wal_wifi_gscan_capabilities gscan_capa;
    oal_netbuf_stru *skb;

    if (wiphy == OAL_PTR_NULL || wdev == OAL_PTR_NULL || wdev->netdev == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::wdev or netdev is NULL! wiphy %p, wdev %p.}", wiphy, wdev);
        return -OAL_EFAIL;
    }

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb_etc(wiphy, sizeof(gscan_capa));
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_channel_list::skb alloc failed.len %d\r\n", sizeof(gscan_capa));
        return -OAL_ENOMEM;
    }

    OAL_MEMZERO(&gscan_capa, sizeof(gscan_capa));
    gscan_capa.max_number_blacklist_bssids = MAX_BLACKLIST_BSSID;
    gscan_capa.max_number_of_white_listed_ssid = MAX_WHITELIST_SSID;

    oal_nla_put_nohdr(skb, sizeof(gscan_capa), &gscan_capa); /* already have NL80211_ATTR_VENDOR_DATA header */

    ul_ret = oal_cfg80211_vendor_cmd_reply_etc(skb);
    if (OAL_UNLIKELY(ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_gscan_capabilities::oal_cfg80211_vendor_cmd_reply_etc failed ret:%d.\r\n", ul_ret);
        return ul_ret;
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_get_gscan_capabilities::ret_value %d.}", ul_ret);

    return ul_ret;
}
#endif


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
    if ((band & WIFI_BAND_24_GHZ) && (0 != pst_channel_list->uc_channel_num_2g))
    {
        puc_chanel_list = pst_channel_list->auc_channel_list_2g;
        for (ul_loop = 0; ul_loop < pst_channel_list->uc_channel_num_2g; ul_loop++)
        {
            pl_channel_list[ul_channel_num++] = oal_ieee80211_channel_to_frequency(puc_chanel_list[ul_loop],IEEE80211_BAND_2GHZ);
        }
    }

    /* 获取5G Band信道列表，包括非DFS Channels和DFS Channels */
    if ((band & (WIFI_BAND_5_GHZ | WIFI_BAND_5_GHZ_DFS_ONLY)) && (0 != pst_channel_list->uc_channel_num_5g))
    {
        oal_bool_enum_uint8 en_dfs = OAL_FALSE;
        puc_chanel_list = pst_channel_list->auc_channel_list_5g;

        for (ul_loop = 0; ul_loop < pst_channel_list->uc_channel_num_5g; ul_loop++)
        {
            en_dfs = mac_is_dfs_channel(MAC_RC_START_FREQ_5, puc_chanel_list[ul_loop]);

            if (((band & WIFI_BAND_5_GHZ_DFS_ONLY) && (en_dfs == OAL_TRUE))
                || ((band & WIFI_BAND_5_GHZ) && (en_dfs == OAL_FALSE)))
            {
                pl_channel_list[ul_channel_num++] = oal_ieee80211_channel_to_frequency(puc_chanel_list[ul_loop], IEEE80211_BAND_5GHZ);
            }
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
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_get_current_channel_list::NET_DEV_PRIV is NULL.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 消息参数准备 */
    st_query_msg.en_wid = WLAN_CFGID_VENDOR_CMD_GET_CHANNEL_LIST;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 发送消息 */
    l_ret = wal_send_cfg_event_etc(pst_netdev,
                               WAL_MSG_TYPE_QUERY,
                               WAL_MSG_WID_LENGTH,
                               (oal_uint8 *)&st_query_msg,
                               OAL_TRUE,
                               &pst_rsp_msg);

    if (OAL_SUCC != l_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_get_current_channel_list:: wal_send_cfg_event_etc return err code %d!}\r\n", l_ret);
        if (OAL_PTR_NULL != pst_rsp_msg)
        {
            oal_free(pst_rsp_msg);
        }
        return (oal_uint32)l_ret;/* [false alarm] */
    }

    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    pst_channel_list  = (mac_vendor_cmd_channel_list_stru *)(pst_query_rsp_msg->auc_value);

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
        /* 获取band值 bit0:2G信道列表 bit1:5G信道列表 bit2:DFS channel */
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
    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb_etc(wiphy, mem_needed);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_channel_list::skb alloc failed.len %d\r\n",  mem_needed);
        return -OAL_ENOMEM;
    }

    oal_nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_CHANNELS, num_channels);
    oal_nla_put(skb, GSCAN_ATTRIBUTE_CHANNEL_LIST, reply_len, al_channel_list);

    err = oal_cfg80211_vendor_cmd_reply_etc(skb);
    if (OAL_UNLIKELY(err))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_channel_list::oal_cfg80211_vendor_cmd_reply_etc failed ret:%d.\r\n", err);
        return err;
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
    oal_int32 l_ret = OAL_SUCC;
    oal_int8  auc_country_code[WLAN_COUNTRY_STR_LEN] = {0};
    OAL_CONST oal_nlattr_stru *iter;

    /* 测试阶段可采用定制化99配置 */
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if (OAL_TRUE == g_st_cust_country_code_ignore_flag.en_country_code_ingore_ini_flag)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_country::set_country ignored, flag ini[%d]",
                          g_st_cust_country_code_ignore_flag.en_country_code_ingore_ini_flag);
        return l_ret;
    }
#endif

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
                OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_cfgvendor_set_country::country code:%c %c %c, len = %d!}",
                                 auc_country_code[0], auc_country_code[1], auc_country_code[2], oal_nla_len(iter));
                /* 设置国家码到wifi 驱动 */
                l_ret = wal_regdomain_update_country_code_etc(wdev->netdev, auc_country_code);
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

OAL_STATIC oal_int32 wal_cfgvendor_do_get_feature_set(mac_wiphy_priv_stru  *pst_wiphy_priv)
{
    oal_int32         l_feature_set = 0;
    mac_device_stru  *pst_mac_device;

    if(NULL == pst_wiphy_priv)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_do_get_feature_set::pst_mac_device is null!}");
        return l_feature_set;
    }

    pst_mac_device = pst_wiphy_priv->pst_mac_device;
    if (NULL != pst_mac_device)
    {
        l_feature_set |= ((OAL_TRUE == mac_device_check_5g_enable(pst_mac_device->uc_device_id)) ?
                           WIFI_FEATURE_INFRA_5G : 0);
    }

#ifdef _PRE_WLAN_FEATURE_HS20
    l_feature_set |= WIFI_FEATURE_HOTSPOT;
#endif
    l_feature_set |= WIFI_FEATURE_LINK_LAYER_STATS; /** 0x10000 Link layer stats collection */
    
    //l_feature_set |= WIFI_FEATURE_CONTROL_ROAMING; /* 0x800000 Enable driver/firmware roaming */

    return l_feature_set;
}

OAL_STATIC oal_int32 wal_cfgvendor_get_feature_set(oal_wiphy_stru *wiphy,
    oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int32 l_err = 0;
    oal_int32 l_reply;
    oal_int32 l_reply_len = OAL_SIZEOF(oal_int32);
    oal_netbuf_stru       *skb;
    mac_wiphy_priv_stru   *pst_wiphy_priv;

    if(NULL == wdev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_get_feature_set::pst_wdev is null!!}\r\n");
        return -OAL_EFAUL;
    }

    pst_wiphy_priv = (mac_wiphy_priv_stru *)oal_wiphy_priv(OAL_WIRELESS_DEV_WIPHY(wdev));

    l_reply = wal_cfgvendor_do_get_feature_set(pst_wiphy_priv);

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb_etc(wiphy, l_reply_len);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_feature_set::skb alloc failed.len %d\r\n", l_reply_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, l_reply_len, &l_reply);

    l_err = oal_cfg80211_vendor_cmd_reply_etc(skb);
    if (OAL_UNLIKELY(l_err))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_get_feature_set::Vendor Command reply failed ret:%d.\r\n", l_err);
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_get_feature_set::set flag:0x%x.\r\n}", l_reply);

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
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
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
    oal_uint8 auc_random_mac_oui[WLAN_RANDOM_MAC_OUI_LEN] = {0};

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


OAL_STATIC oal_uint32  wal_cfgvendor_blacklist_mode(oal_net_device_stru *pst_net_dev, oal_uint8 uc_mode)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));

    *(oal_uint8 *)(st_write_msg.auc_value) = uc_mode;
    us_len = OAL_SIZEOF(uc_mode);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BLACKLIST_MODE, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_cfgvendor_blacklist_mode:wal_send_cfg_event_etc return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_cfgvendor_blacklist_add(oal_net_device_stru *pst_net_dev, oal_uint8 *puc_mac_addr)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;
    mac_blacklist_stru             *pst_blklst;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));
    pst_blklst = (mac_blacklist_stru*)(st_write_msg.auc_value);

    oal_memcopy(pst_blklst->auc_mac_addr, puc_mac_addr, OAL_MAC_ADDR_LEN);

    us_len = OAL_SIZEOF(mac_blacklist_stru);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_cfgvendor_blacklist_add:wal_send_cfg_event_etc return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_cfgvendor_blacklist_del(oal_net_device_stru *pst_net_dev, oal_uint8 *puc_mac_addr)
{
    wal_msg_write_stru              st_write_msg;
    oal_int32                       l_ret;
    oal_uint16                      us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    OAL_MEMZERO((oal_uint8*)&st_write_msg, OAL_SIZEOF(st_write_msg));

    oal_memcopy(st_write_msg.auc_value, puc_mac_addr, OAL_MAC_ADDR_LEN);

    us_len = OAL_MAC_ADDR_LEN; /* OAL_SIZEOF(oal_uint8); */

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                              WAL_MSG_TYPE_WRITE,
                              WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                              (oal_uint8 *)&st_write_msg,
                              OAL_FALSE,
                              OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{wal_cfgvendor_blacklist_del:wal_send_cfg_event_etc return[%d].}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_cfgvendor_set_bssid_blacklist(oal_wiphy_stru *pst_wiphy,
                                                       oal_wireless_dev_stru *pst_wdev,
                                                       OAL_CONST oal_void  *p_data,
                                                       oal_int32 l_len)
{
    oal_int32 l_type = 0;
    oal_nlattr_stru *pst_nla;
    wal_wifi_bssid_params st_bssid_info;
    oal_uint8 i = 0;

    if (OAL_PTR_NULL == pst_wdev || OAL_PTR_NULL == pst_wdev->netdev)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::wdev or netdev is NULL! wdev=%p.}", pst_wdev);
        return -OAL_EFAIL;
    }

    if ((OAL_PTR_NULL == p_data) || (l_len <= NLA_HDRLEN))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::invalid para for p_data %p, l_len=%d.}", p_data, l_len);
        return -OAL_EINVAL;
    }

    OAL_MEMZERO(&st_bssid_info, sizeof(st_bssid_info));
    pst_nla = (oal_nlattr_stru *)p_data;
    l_type = oal_nla_type(pst_nla);
    if (GSCAN_ATTRIBUTE_NUM_BSSID == l_type)
    {
        st_bssid_info.num_bssid = oal_nla_get_u32(pst_nla);
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::num_bssid=%d}\r\n", st_bssid_info.num_bssid);
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist:: unknown bssid type:%d}\r\n", l_type);
        return -OAL_EFAIL;
    }

    pst_nla = (oal_nlattr_stru *)((oal_uint8 *)pst_nla + oal_nla_total_size(pst_nla));
    if (!st_bssid_info.num_bssid) /* Flush BSSID Blacklist */
    {
        oal_bool_enum_uint8 en_flush = OAL_FALSE;

        l_type = oal_nla_type(pst_nla);
        if (GSCAN_ATTRIBUTE_BSSID_BLACKLIST_FLUSH == l_type)
        {
           en_flush = oal_nla_get_u32(pst_nla);
        }
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::l_type=%d en_flush=%d}\r\n", l_type, en_flush);

        if (OAL_TRUE == en_flush)
        {
            oal_uint8 mac_addr[OAL_MAC_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            wal_cfgvendor_blacklist_del(pst_wdev->netdev, mac_addr); /* broadcast address as delete all bssids */
        }
    }
    else /* set BSSID Blacklist */
    {
        for (i = 0; i < st_bssid_info.num_bssid; i++) {
            l_type = oal_nla_type(pst_nla);
            if (GSCAN_ATTRIBUTE_BLACKLIST_BSSID == l_type)
                oal_memcopy(st_bssid_info.bssids[i], oal_nla_data(pst_nla), OAL_MAC_ADDR_LEN);
            pst_nla = (oal_nlattr_stru *)((oal_uint8 *)pst_nla + oal_nla_total_size(pst_nla));
        }

        wal_cfgvendor_blacklist_mode(pst_wdev->netdev, CS_BLACKLIST_MODE_BLACK);

        for (i = 0; i < st_bssid_info.num_bssid; i++) {
            wal_cfgvendor_blacklist_add(pst_wdev->netdev, st_bssid_info.bssids[i]);
        }

        OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::bssid[0]=xx:xx:xx:%02x:%02x:%02x}\r\n",
                         st_bssid_info.bssids[0][3], st_bssid_info.bssids[0][4], st_bssid_info.bssids[0][5]);
        OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::bssid[1]=xx:xx:xx:%02x:%02x:%02x}\r\n",
                         st_bssid_info.bssids[1][3], st_bssid_info.bssids[1][4], st_bssid_info.bssids[1][5]);
        OAM_WARNING_LOG3(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::bssid[2]=xx:xx:xx:%02x:%02x:%02x}\r\n",
                         st_bssid_info.bssids[2][3], st_bssid_info.bssids[2][4], st_bssid_info.bssids[2][5]);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 wal_cfgvendor_roam_enable(oal_net_device_stru *pst_net_dev, wal_wifi_roaming_state roam_state)
{
    oal_int32                  l_ret;
    wal_msg_write_stru         st_write_msg;

    if (OAL_PTR_NULL == pst_net_dev)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_roam_enable:: netdev is NULL! netdev=%p.}", pst_net_dev);
        return -OAL_EFAIL;
    }

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ROAM_ENABLE, OAL_SIZEOF(oal_uint32));
    *((oal_bool_enum_uint8 *)(st_write_msg.auc_value)) = roam_state;

    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(oal_bool_enum_uint8),
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_roam_enable::return err code[%d]!}\r\n", l_ret);
        return (oal_uint32)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_int32 wal_cfgvendor_set_roam_policy(oal_wiphy_stru *pst_wiphy,
                                                   oal_wireless_dev_stru *pst_wdev,
                                                   OAL_CONST oal_void  *p_data,
                                                   oal_int32 l_len)
{
    oal_int32 l_type;
    wal_wifi_roaming_state roam_state = 0;

    if (OAL_PTR_NULL == pst_wdev || OAL_PTR_NULL == pst_wdev->netdev)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy::wdev or netdev is NULL! wdev=%p.}", pst_wdev);
        return -OAL_EFAIL;
    }

    if ((OAL_PTR_NULL == p_data) || (l_len <= NLA_HDRLEN))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy::invalid para for p_data %p, l_len=%d.}", p_data, l_len);
        return -OAL_EINVAL;
    }

    l_type = oal_nla_type(p_data);
    if (GSCAN_ATTRIBUTE_ROAM_STATE_SET == l_type)
    {
        roam_state = oal_nla_get_u32(p_data);
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy::roam_state=%d}\r\n", roam_state);
        /* enable or disable roaming */
        if ((VENDOR_WIFI_ROAMING_ENABLE == roam_state) || (VENDOR_WIFI_ROAMING_DISABLE == roam_state))
        {
            wal_cfgvendor_roam_enable(pst_wdev->netdev, roam_state);
        }
        else
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy::invalid roam_state=%d}\r\n", roam_state);
        }
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy:: unknown roaming type:%d}\r\n", l_type);
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

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb_etc(wiphy, OAL_SIZEOF(oal_uint32));
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_feature::skb alloc failed. len %d\r\n", OAL_SIZEOF(oal_uint32));
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, OAL_SIZEOF(oal_uint32), &supported_features);

    l_ret = oal_cfg80211_vendor_cmd_reply_etc(skb);
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

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb_etc(wiphy, l_buf_len);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::skb alloc failed.len %d\r\n", l_buf_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, l_buf_len, auc_driver_version);

    l_ret = oal_cfg80211_vendor_cmd_reply_etc(skb);
    if (OAL_UNLIKELY(l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::Vendor Command reply failed. ret:%d.\r\n", l_ret);
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::SUCCESS. driver version len %d\r\n", l_buf_len);
    return l_ret;
}


oal_int32 wal_cfgvendor_dbg_get_ring_status_etc(oal_wiphy_stru *wiphy,
        oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    oal_int32 l_ret,l_buf_len;
    debug_ring_status_st debug_ring_status = {{VENDOR_FW_EVENT_RING_NAME},0,FW_EVENT_RING_ID,
                                                VENDOR_FW_EVENT_RING_SIZE,0,0,0,0};
    oal_netbuf_stru *skb;
    l_buf_len = OAL_SIZEOF(debug_ring_status_st);

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb_etc(wiphy, l_buf_len+100);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_status_etc::skb alloc failed.len %d\r\n", l_buf_len+100);
        return -OAL_ENOMEM;
    }

    oal_nla_put_u32(skb, DEBUG_ATTRIBUTE_RING_NUM, 1);
    oal_nla_put(skb, DEBUG_ATTRIBUTE_RING_STATUS, l_buf_len,&debug_ring_status);

    l_ret = oal_cfg80211_vendor_cmd_reply_etc(skb);
    if (OAL_UNLIKELY(l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_status_etc::Vendor Command reply failed. ret:%d.\r\n", l_ret);
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_status_etc::SUCCESS. driver version len %d\r\n", l_buf_len);
    return l_ret;
}


oal_int32 wal_cfgvendor_dbg_trigger_mem_dump_etc(oal_wiphy_stru *wiphy,
        oal_wireless_dev_stru *wdev, OAL_CONST oal_void  *data, oal_int32 len)
{
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_trigger_mem_dump_etc::ENTER");
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

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

OAL_STATIC oal_int32 wal_send_vowifi_nat_keep_alive_params(oal_net_device_stru *pst_net_dev,
                                             oal_uint8 *pc_keep_alive_info, oal_uint8 uc_msg_len)
{
    wal_msg_write_stru st_write_msg;
    oal_int32          l_ret;

    if(OAL_PTR_NULL == pst_net_dev || OAL_PTR_NULL == pc_keep_alive_info)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_send_vowifi_nat_keep_alive_params:: null point argument,pst_net_dev:%p, auc_random_mac_oui:%p.}",
                       pst_net_dev, pc_keep_alive_info);
        return -OAL_EFAIL;
    }

    OAL_MEMZERO(&st_write_msg, OAL_SIZEOF(wal_msg_write_stru));

    /***************************************************************************
        抛事件到wal层处理  WLAN_CFGID_SET_VOWIFI_KEEP_ALIVE
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_VOWIFI_KEEP_ALIVE, uc_msg_len);
    oal_memcopy(st_write_msg.auc_value, pc_keep_alive_info, uc_msg_len);

    /* 发送消息 */
    l_ret = wal_send_cfg_event_etc(pst_net_dev,
                               WAL_MSG_TYPE_WRITE,
                               WAL_MSG_WRITE_MSG_HDR_LENGTH + uc_msg_len,
                               (oal_uint8 *)&st_write_msg,
                               OAL_FALSE,
                               OAL_PTR_NULL);

    if (OAL_UNLIKELY(OAL_SUCC != l_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_send_vowifi_nat_keep_alive_params::send WLAN_CFGID_SET_VOWIFI_KEEP_ALIVE failed, err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return l_ret;
}



OAL_STATIC oal_int32 wl_cfgvendor_start_vowifi_nat_keep_alive(oal_wiphy_stru *pst_wiphy,
                                                                 oal_wireless_dev_stru *pst_wdev,
                                                                 OAL_CONST oal_void  *p_data,
                                                                 oal_int32 l_len)
{
    oal_int32                                   l_type;
    oal_int32                                   l_rem;
    oal_uint32                                  ul_period_msec        = 0;
    oal_uint8                                   uc_keep_alive_id      = 0;
    oal_uint8                                   uc_ip_pkt_len         = 0;
    oal_bool_enum_uint8                         en_find_keepid_flag   = OAL_FALSE ;
    oal_uint8                                   uc_msg_len;
    oal_uint8                                   auc_src_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                                   auc_dst_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                                  *puc_ip_pkt = OAL_PTR_NULL;
    OAL_CONST oal_nlattr_stru                  *pst_iter;
    mac_vowifi_nat_keep_alive_start_info_stru  *pst_start_info;

    OAL_NLA_FOR_EACH_ATTR(pst_iter, p_data, l_len, l_rem)
    {
        l_type = oal_nla_type(pst_iter);
        switch(l_type)
        {
            case MKEEP_ALIVE_ATTRIBUTE_ID:
                uc_keep_alive_id    = oal_nla_get_u8(pst_iter);
                en_find_keepid_flag = OAL_TRUE;
                break;
            case MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN:
                uc_ip_pkt_len = oal_nla_get_u16(pst_iter);
                if(uc_ip_pkt_len > MKEEP_ALIVE_IP_PKT_MAXLEN || 0 == uc_ip_pkt_len)
                {
                    if(OAL_PTR_NULL != puc_ip_pkt)
                    {
                        OAL_MEM_FREE(puc_ip_pkt, OAL_TRUE);
                    }

                    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wl_cfgvendor_start_vowifi_nat_keep_alive::uc_ip_pkt_len=[%d]is invalid.}",uc_ip_pkt_len);
                    return OAL_EFAIL;
                }
                break;
            case MKEEP_ALIVE_ATTRIBUTE_IP_PKT:
                if(OAL_PTR_NULL != puc_ip_pkt)
                {
                    OAL_MEM_FREE(puc_ip_pkt, OAL_TRUE);
                }
                if(uc_ip_pkt_len > 0)
                {
                    puc_ip_pkt = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, uc_ip_pkt_len, OAL_TRUE);
                    if(OAL_PTR_NULL == puc_ip_pkt)
                    {
                        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wl_cfgvendor_start_vowifi_nat_keep_alive::alloc %d fail.}",uc_ip_pkt_len);
                        return OAL_EFAIL;
                    }
                    oal_memcopy(puc_ip_pkt, oal_nla_data(pst_iter), uc_ip_pkt_len);
                }
                break;
            case MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR:
                oal_memcopy(auc_src_mac, oal_nla_data(pst_iter), WLAN_MAC_ADDR_LEN);
                break;
            case MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR:
                oal_memcopy(auc_dst_mac, oal_nla_data(pst_iter), WLAN_MAC_ADDR_LEN);
                break;
            case MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC:
                ul_period_msec = oal_nla_get_u32(pst_iter);
                break;
            default:
                if(OAL_PTR_NULL != puc_ip_pkt)
                {
                    OAL_MEM_FREE(puc_ip_pkt, OAL_TRUE);
                }
                return OAL_EFAIL;
        }
    }

    if(0 == ul_period_msec || OAL_FALSE == en_find_keepid_flag)
    {
        if(OAL_PTR_NULL != puc_ip_pkt)
        {
            OAL_MEM_FREE(puc_ip_pkt, OAL_TRUE);
        }
        return OAL_FAIL;
    }

    if(uc_ip_pkt_len > 0 && OAL_PTR_NULL != puc_ip_pkt)
    {
        uc_msg_len      = uc_ip_pkt_len + OAL_SIZEOF(mac_vowifi_nat_keep_alive_start_info_stru);
        pst_start_info  = (mac_vowifi_nat_keep_alive_start_info_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, uc_msg_len, OAL_TRUE);
        if(OAL_PTR_NULL == pst_start_info)
        {
            if(OAL_PTR_NULL != puc_ip_pkt)
            {
                OAL_MEM_FREE(puc_ip_pkt, OAL_TRUE);
            }
            return OAL_EFAIL;
        }

        pst_start_info->st_basic_info.uc_keep_alive_id = uc_keep_alive_id;
        pst_start_info->st_basic_info.en_type          = VOWIFI_MKEEP_ALIVE_TYPE_START;
        pst_start_info->us_ip_pkt_len                  = uc_ip_pkt_len;
        pst_start_info->ul_period_msec                 = ul_period_msec;

        oal_memcopy(pst_start_info->auc_ip_pkt_data, puc_ip_pkt, uc_ip_pkt_len);
        oal_memcopy(pst_start_info->auc_src_mac, auc_src_mac, WLAN_MAC_ADDR_LEN);
        oal_memcopy(pst_start_info->auc_dst_mac, auc_dst_mac, WLAN_MAC_ADDR_LEN);

       OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wl_cfgvendor_start_vowifi_nat_keep_alive::en_type=[%d],id=[%d],ip_len=[%d],period=[%d].}",
                  pst_start_info->st_basic_info.en_type,pst_start_info->st_basic_info.uc_keep_alive_id,
                  pst_start_info->us_ip_pkt_len, pst_start_info->ul_period_msec);
       wal_send_vowifi_nat_keep_alive_params(pst_wdev->netdev, (oal_uint8 *)pst_start_info, uc_msg_len);
       OAL_MEM_FREE(pst_start_info, OAL_TRUE);
    }

    if(OAL_PTR_NULL != puc_ip_pkt)
    {
        OAL_MEM_FREE(puc_ip_pkt, OAL_TRUE);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_int32 wl_cfgvendor_stop_vowifi_nat_keep_alive(oal_wiphy_stru *pst_wiphy,
                                                                 oal_wireless_dev_stru *pst_wdev,
                                                                 OAL_CONST oal_void  *p_data,
                                                                 oal_int32 l_len)
{
    oal_int32                                  l_type;
    oal_int32                                  l_ret;
    oal_int32                                  l_rem;
    oal_bool_enum_uint8                        en_find_keepid_flag  = OAL_FALSE;
    OAL_CONST oal_nlattr_stru                 *pst_iter;
    mac_vowifi_nat_keep_alive_basic_info_stru  st_stop_info;

    OAL_MEMZERO(&st_stop_info, OAL_SIZEOF(mac_vowifi_nat_keep_alive_basic_info_stru));

    OAL_NLA_FOR_EACH_ATTR(pst_iter, p_data, l_len, l_rem)
    {
        l_type = oal_nla_type(pst_iter);
        switch(l_type)
        {
            case MKEEP_ALIVE_ATTRIBUTE_ID:
                st_stop_info.uc_keep_alive_id = oal_nla_get_u8(pst_iter);
                en_find_keepid_flag           = OAL_TRUE;
                break;
            default:
                return OAL_EFAIL;

        }
    }

    if(OAL_TRUE == en_find_keepid_flag)
    {
        st_stop_info.en_type = VOWIFI_MKEEP_ALIVE_TYPE_STOP;
    #if 1 /*待正式测试完成后，删除*/
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wl_cfgvendor_stop_vowifi_nat_keep_alive::en_type=[%d],id=[%d].}",
                   st_stop_info.en_type,st_stop_info.uc_keep_alive_id);
    #endif
        l_ret = wal_send_vowifi_nat_keep_alive_params(pst_wdev->netdev, (oal_uint8 *)&st_stop_info, OAL_SIZEOF(mac_vowifi_nat_keep_alive_basic_info_stru));
        if (OAL_UNLIKELY(OAL_SUCC != l_ret))
        {
            return -OAL_EFAIL;
        }
    }

    return OAL_SUCC;
}

#if 0/*测试命令，待测试完成后删除*/
oal_int32 wal_send_vowifi_nat_keep_alive_test(oal_net_device_stru *pst_net_dev,
                                             oal_uint8 uc_start_flag, oal_uint8 uc_keep_id, oal_uint32 ul_perrod_msc)
{
    mac_vowifi_nat_keep_alive_start_info_stru  *pst_start_info = OAL_PTR_NULL;
    mac_vowifi_nat_keep_alive_basic_info_stru   st_stop_info;
    oal_uint8                                   uc_msg_len;
    oal_uint8                                  *puc_msg = OAL_PTR_NULL;
    oal_uint8                                   auc_ip_data[]={\
    0x45,0x00,0x00,0x1d,0x00,0x40,0x00,0x00,\
    0xff,0x11,0x0b,0x5d,0x0a,0x01,0x01,0x71,\
    0x3a,0x3c,0x6a,0x85,0x83,0xdf,0x11,0x94,\
    0x00,0x09,0xbb,0x34,0xff};

    OAL_MEMZERO(&st_stop_info, OAL_SIZEOF(mac_vowifi_nat_keep_alive_basic_info_stru));

    if(OAL_PTR_NULL == pst_net_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_send_vowifi_nat_keep_alive_test:: null point argument.}");
        return -OAL_EFAIL;
    }

    if(0 == uc_start_flag)
    {/*stop*/
        st_stop_info.en_type          = VOWIFI_MKEEP_ALIVE_TYPE_STOP;
        st_stop_info.uc_keep_alive_id = uc_keep_id;
        puc_msg                       = (oal_uint8 *)&st_stop_info;
        uc_msg_len                    = OAL_SIZEOF(st_stop_info);
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{wal_send_vowifi_nat_keep_alive_test::en_type=[%d],ID=[%d],uc_msg_len=[%d].}",
                   st_stop_info.en_type, st_stop_info.uc_keep_alive_id, uc_msg_len);
    }
    else
    {/*start*/
        uc_msg_len     = ARRAY_SIZE(auc_ip_data) + OAL_SIZEOF(mac_vowifi_nat_keep_alive_start_info_stru);
        pst_start_info = (mac_vowifi_nat_keep_alive_start_info_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, uc_msg_len, OAL_TRUE);
        if(OAL_PTR_NULL == pst_start_info)
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_send_vowifi_nat_keep_alive_test:: malloc pst_mkeep_start_info fail.}");
            return OAL_FAIL;
        }

        OAL_MEMZERO(pst_start_info, uc_msg_len);
        pst_start_info->st_basic_info.en_type          = VOWIFI_MKEEP_ALIVE_TYPE_START;
        pst_start_info->st_basic_info.uc_keep_alive_id = uc_keep_id;
        pst_start_info->us_ip_pkt_len                  = ARRAY_SIZE(auc_ip_data);
        pst_start_info->ul_period_msec                 = ul_perrod_msc;
        oal_memcopy(pst_start_info->auc_ip_pkt_data, auc_ip_data, pst_start_info->us_ip_pkt_len);
        puc_msg = (oal_uint8 *)pst_start_info;

        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_send_vowifi_nat_keep_alive_test::en_type=[%d],id=[%d].}",
                   pst_start_info->st_basic_info.en_type,pst_start_info->st_basic_info.uc_keep_alive_id);
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{wal_send_vowifi_nat_keep_alive_test::,ip_pkt_len=[%d],uc_msg_len=[%d],perriod=[%d].}",
                   pst_start_info->us_ip_pkt_len, uc_msg_len, pst_start_info->ul_period_msec);

    }

    wal_send_vowifi_nat_keep_alive_params(pst_net_dev, puc_msg, uc_msg_len);

    if(OAL_PTR_NULL == pst_start_info)
    {
       OAL_MEM_FREE(pst_start_info, OAL_TRUE);
    }


    return OAL_SUCC;
}
#endif

#endif

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

    /* ??radio ?? */
    pst_radio_stat = (wal_wifi_radio_stat_stru *)p_out_data;
    pst_radio_stat->ul_num_channels = VENDOR_NUM_CHAN;
    pst_radio_stat->ul_on_time      = OAL_JIFFIES_TO_MSECS(OAL_TIME_JIFFY - g_st_wifi_radio_stat_etc.ull_wifi_on_time_stamp);
    pst_radio_stat->ul_tx_time      = 0;
    pst_radio_stat->ul_rx_time      = 0;

    /* ??interfac ?? */
    pst_iface_stat = (wal_wifi_iface_stat_stru *)(p_out_data + OAL_SIZEOF(*pst_radio_stat));
    pst_iface_stat->ul_num_peers           = VENDOR_NUM_PEER;
    pst_iface_stat->peer_info->ul_num_rate = VENDOR_NUM_RATE;

    /* ??link ?? */
    pst_skb = oal_cfg80211_vendor_cmd_alloc_reply_skb_etc(pst_wiphy, ul_reply_len);
    if (OAL_UNLIKELY(!pst_skb))
    {
        oal_free(p_out_data);
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info::skb alloc failed.len %d}", ul_reply_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(pst_skb, ul_reply_len, p_out_data);

    l_err =  oal_cfg80211_vendor_cmd_reply_etc(pst_skb);
    OAM_WARNING_LOG4(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info::on_time %d, tx_time %d, rx_time %d, err %d",
                    pst_radio_stat->ul_on_time,
                    pst_radio_stat->ul_tx_time,
                    pst_radio_stat->ul_rx_time,
                    l_err);
    oal_free(p_out_data);
    return l_err;
}

#ifdef _PRE_WLAN_FEATURE_APF

OAL_STATIC oal_int32 wal_cfgvendor_apf_get_capabilities(oal_wiphy_stru *wiphy,
                                                        oal_wireless_dev_stru *wdev,
                                                        OAL_CONST oal_void *data,
                                                        oal_int32 len)
{
    oal_netbuf_stru *skb;
    oal_int32 l_ret, l_mem_needed;

    l_mem_needed = VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb_etc(wiphy, l_mem_needed);
    if (OAL_UNLIKELY(!skb))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_apf_get_capabilities::skb alloc failed, len %d", l_mem_needed);
        return -OAL_ENOMEM;
    }

    oal_nla_put_u32(skb, APF_ATTRIBUTE_VERSION, APF_VERSION);
    oal_nla_put_u32(skb, APF_ATTRIBUTE_MAX_LEN, APF_PROGRAM_MAX_LEN);

    l_ret = oal_cfg80211_vendor_cmd_reply_etc(skb);
    if (OAL_UNLIKELY(l_ret))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_apf_get_capabilities::Vendor Command reply failed, ret:%d.", l_ret);
    }
    return l_ret;
}


OAL_STATIC oal_int32 wal_cfgvendor_apf_set_filter(oal_wiphy_stru *wiphy,
                                                  oal_wireless_dev_stru *wdev,
                                                  OAL_CONST oal_void  *data,
                                                  oal_int32 len)
{
    OAL_CONST oal_nlattr_stru *iter;
    oal_int32 l_tmp, l_type;
    mac_apf_filter_cmd_stru st_apf_filter_cmd;
    wal_msg_write_stru st_write_msg;
    wal_msg_stru       *pst_rsp_msg = NULL;
    oal_uint32 ul_program_len;

    st_apf_filter_cmd.us_program_len = 0;

    /* assumption: length attribute must come first */
    OAL_NLA_FOR_EACH_ATTR(iter, data, len, l_tmp) {
        l_type = oal_nla_type(iter);
        switch (l_type) {
            case APF_ATTRIBUTE_PROGRAM_LEN:
                ul_program_len = oal_nla_get_u32(iter);
                if (OAL_UNLIKELY(!ul_program_len || ul_program_len > APF_PROGRAM_MAX_LEN))
                {
                    OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::program len[%d] is invalid", ul_program_len);
                    return -OAL_EINVAL;
                }
                st_apf_filter_cmd.us_program_len = (oal_uint16)ul_program_len;
            break;
            case APF_ATTRIBUTE_PROGRAM:
                if (OAL_UNLIKELY(!st_apf_filter_cmd.us_program_len))
                {
                    OAM_ERROR_LOG0(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::program len not set");
                    return -OAL_EINVAL;
                }

                st_apf_filter_cmd.puc_program = (oal_uint8*)oal_nla_data(iter);
                st_apf_filter_cmd.en_cmd_type = APF_SET_FILTER_CMD;

                /***************************************************************************
                    抛事件到wal层处理
                ***************************************************************************/
                WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_APF_FILTER, OAL_SIZEOF(st_apf_filter_cmd));
                oal_memcopy(st_write_msg.auc_value, &st_apf_filter_cmd, OAL_SIZEOF(st_apf_filter_cmd));

                /* 发送消息 */
                /* 需要将发送该函数设置为同步，否则hmac处理时会使用已释放的内存 */
                if (OAL_SUCC != wal_send_cfg_event_etc(wdev->netdev,
                                           WAL_MSG_TYPE_WRITE,
                                           WAL_MSG_WRITE_MSG_HDR_LENGTH + OAL_SIZEOF(st_apf_filter_cmd),
                                           (oal_uint8 *)&st_write_msg,
                                           OAL_TRUE,
                                           &pst_rsp_msg))
                {
                    OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_apf_set_filter::wal_send_cfg_event_etc fail!}");
                    return -OAL_EFAIL;
                }
                if (OAL_SUCC != wal_check_and_release_msg_resp_etc(pst_rsp_msg))
                {
                    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_cfgvendor_apf_set_filter::wal_check_and_release_msg_resp_etc fail!}");
                    return -OAL_EFAIL;
                }
            break;
            default:
                OAM_ERROR_LOG1(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::no such attribute %d", l_type);
                return -OAL_EINVAL;
        }
    }

    OAM_WARNING_LOG1(0, 0, "wal_cfgvendor_apf_set_filter: program len %d", st_apf_filter_cmd.us_program_len);
    return OAL_SUCC;
}
#endif

OAL_STATIC OAL_CONST oal_wiphy_vendor_command_stru wal_vendor_cmds[] =
{
#if 0
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = GSCAN_SUBCMD_GET_CAPABILITIES
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_get_gscan_capabilities
    },
#endif
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
            .subcmd = WIFI_SUBCMD_SET_BSSID_BLACKLIST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_set_bssid_blacklist
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = WIFI_SUBCMD_FW_ROAM_POLICY
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_set_roam_policy
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
        .doit = wal_cfgvendor_dbg_get_ring_status_etc
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = DEBUG_TRIGGER_MEM_DUMP
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_trigger_mem_dump_etc
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
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = WIFI_OFFLOAD_SUBCMD_START_MKEEP_ALIVE
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wl_cfgvendor_start_vowifi_nat_keep_alive
    },
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = WIFI_OFFLOAD_SUBCMD_STOP_MKEEP_ALIVE
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wl_cfgvendor_stop_vowifi_nat_keep_alive
    },
#endif
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = LSTATS_SUBCMD_GET_INFO
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_lstats_get_info
    },
#ifdef _PRE_WLAN_FEATURE_APF
    {
        {
            .vendor_id = OUI_GOOGLE,
            .subcmd = APF_SUBCMD_GET_CAPABILITIES
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_apf_get_capabilities
    },

    {
        {
        .vendor_id = OUI_GOOGLE,
        .subcmd = APF_SUBCMD_SET_FILTER
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_apf_set_filter
    },
#endif
};

OAL_STATIC OAL_CONST oal_nl80211_vendor_cmd_info_stru wal_vendor_events[] =
{
    { OUI_HISI, HISI_VENDOR_EVENT_UNSPEC },
    { OUI_HISI, HISI_VENDOR_EVENT_PRIV_STR }
};



oal_void wal_cfgvendor_init_etc(oal_wiphy_stru *wiphy)
{
    wiphy->vendor_commands  = wal_vendor_cmds;
    wiphy->n_vendor_commands = OAL_ARRAY_SIZE(wal_vendor_cmds);
    wiphy->vendor_events    = wal_vendor_events;
    wiphy->n_vendor_events  = OAL_ARRAY_SIZE(wal_vendor_events);
}

oal_void wal_cfgvendor_deinit_etc(oal_wiphy_stru *wiphy)
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

