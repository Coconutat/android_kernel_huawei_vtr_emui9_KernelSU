


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "mac_ie.h"
#include "mac_regdomain.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "dmac_ext_if.h"
#include "hmac_fsm.h"
#include "hmac_sme_sta.h"
#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_scan.h"
#include "hmac_mgmt_sta.h"
#include "frw_ext_if.h"
#ifdef _PRE_SUPPORT_ACS
#include "dmac_acs.h"
#include "hmac_acs.h"
#endif
#include "hmac_chan_mgmt.h"
#include "hmac_p2p.h"
#ifdef _PRE_WLAN_FEATURE_DFS
#include "hmac_dfs.h"
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
#include "hmac_11k.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SCAN_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if defined(_PRE_WLAN_CHIP_TEST_ALG) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_DEBUG_MODE)
struct kobject     *g_scan_ct_sys_kobject = OAL_PTR_NULL;
#endif
hmac_scan_state_enum_uint8 g_en_bgscan_enable_flag_etc = HMAC_BGSCAN_ENABLE;

/*****************************************************************************
  3 函数实现
*****************************************************************************/
#if defined(_PRE_WLAN_CHIP_TEST_ALG) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_DEBUG_MODE)
OAL_STATIC oal_ssize_t  hmac_scan_ct_proc_read(oal_device_stru *dev, oal_device_attribute_stru *attr, char *buf);
OAL_STATIC OAL_DEVICE_ATTR(alg_test_result, OAL_S_IRUGO|OAL_S_IWUSR, hmac_scan_ct_proc_read, OAL_PTR_NULL);


OAL_STATIC oal_ssize_t hmac_scan_ct_proc_read(oal_device_stru *dev, oal_device_attribute_stru *attr, char *buf)

{
    hmac_device_stru                *pst_hmac_device;
    hmac_scan_record_stru           *pst_record;
    oal_int32                        l_len;

    /* 获取hmac device和扫描运行记录 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(0);
    pst_record       = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);

    l_len = OAL_SIZEOF(wlan_scan_chan_stats_stru);

    oal_memcopy(buf, &(pst_record->ast_chan_results[0]), l_len);

    return l_len;
}


oal_int32  hmac_scan_ct_init(oal_void)
{
    /* hi1102-cb add sys for 51/02 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_int32           l_ret = OAL_SUCC;

    if(OAL_PTR_NULL == g_scan_ct_sys_kobject)
    {
        g_scan_ct_sys_kobject = kobject_create_and_add("scan_ct", OAL_PTR_NULL);
        l_ret = sysfs_create_file(g_scan_ct_sys_kobject, &dev_attr_alg_test_result.attr);
    }

    return l_ret;
#else
    return OAL_SUCC;
#endif
}


oal_void  hmac_scan_ct_exit(oal_void)
{
    if(OAL_PTR_NULL != g_scan_ct_sys_kobject)
    {
        sysfs_remove_file(g_scan_ct_sys_kobject,&dev_attr_alg_test_result.attr);
        kobject_del(g_scan_ct_sys_kobject);
        g_scan_ct_sys_kobject = OAL_PTR_NULL;
    }
}
#endif


oal_int32 hmac_snprintf_hex(oal_uint8 *puc_buf, oal_int32 l_buf_size, oal_uint8 *puc_data, oal_int32 l_len)
{
    oal_int32       l_loop;
    oal_uint8      *puc_pos;
    oal_uint8      *puc_end;
    oal_int32       l_ret;

    if (l_buf_size <= 0)
    {
        return 0;
    }

    puc_pos = puc_buf;
    puc_end = puc_buf + l_buf_size;
    for (l_loop = 0; l_loop < l_len; l_loop++)
    {
        l_ret = OAL_SPRINTF((oal_int8 *)puc_pos, (oal_uint16)(puc_end - puc_pos), "%02x ", puc_data[l_loop]);
        if ((l_ret < 0) || (l_ret >= puc_end - puc_pos))
        {
            puc_buf[l_buf_size - 1] = '\0';
            return puc_pos - puc_buf;
        }

        puc_pos += l_ret;
    }

    puc_buf[l_buf_size - 1] = '\0';
    return puc_pos - puc_buf;
}


OAL_STATIC oal_void hmac_scan_print_scan_params(mac_scan_req_stru *pst_scan_params, mac_vap_stru *pst_mac_vap)
{
    OAM_WARNING_LOG4(pst_scan_params->uc_vap_id, OAM_SF_SCAN, "hmac_scan_print_scan_params::Now Scan channel_num[%d] in [%d]ms with scan_func[0x%x], and ssid_num[%d]!",
                pst_scan_params->uc_channel_nums,
                pst_scan_params->us_scan_time,
                pst_scan_params->uc_scan_func,
                pst_scan_params->uc_ssid_num);

    OAM_WARNING_LOG3(pst_scan_params->uc_vap_id, OAM_SF_SCAN, "hmac_scan_print_scan_params::Scan param, p2p_scan[%d], max_scan_count_per_channel[%d], need back home_channel[%d]!",
            pst_scan_params->bit_is_p2p0_scan,
            pst_scan_params->uc_max_scan_count_per_channel,
            pst_scan_params->en_need_switch_back_home_channel);
    return;
}


oal_void hmac_scan_print_scanned_bss_info_etc(oal_uint8 uc_device_id)
{
    hmac_device_stru        *pst_hmac_device;
    hmac_bss_mgmt_stru      *pst_bss_mgmt;
    hmac_scanned_bss_info   *pst_scanned_bss;
    mac_bss_dscr_stru       *pst_bss_dscr;
    oal_dlist_head_stru     *pst_entry;
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    oal_uint8                   auc_sdt_parse_hdr[MAC_80211_FRAME_LEN];

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev_etc(uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_print_scanned_bss_info_etc::pst_hmac_device null.}");
        return;
    }

    /* 获取指向扫描结果的管理结构体地址 */
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        /* 仅显示新申请到的BSS帧 */
        if (OAL_TRUE == pst_scanned_bss->st_bss_dscr_info.en_new_scan_bss)
        {
            pst_scanned_bss->st_bss_dscr_info.en_new_scan_bss = OAL_FALSE;
            /*上报beacon和probe帧*/
            pst_frame_hdr  = (mac_ieee80211_frame_stru *)pst_bss_dscr->auc_mgmt_buff;

            /* 将beacon中duration字段(2字节)复用为rssi以及channel,方便SDT显示 */
            oal_memcopy((oal_uint8 *)auc_sdt_parse_hdr, (oal_uint8 *)pst_frame_hdr, MAC_80211_FRAME_LEN);
            auc_sdt_parse_hdr[2]  = (oal_uint8)pst_bss_dscr->c_rssi;
            auc_sdt_parse_hdr[3]  = pst_bss_dscr->st_channel.uc_chan_number;

            /* 上报beacon帧或者probe rsp帧 */
            /*lint -e416*/
            oam_report_80211_frame_etc(BROADCAST_MACADDR,
                                   (oal_uint8 *)auc_sdt_parse_hdr,
                                   MAC_80211_FRAME_LEN,
                                   pst_bss_dscr->auc_mgmt_buff + MAC_80211_FRAME_LEN,
                                   (oal_uint16) pst_bss_dscr->ul_mgmt_len,
                                   OAM_OTA_FRAME_DIRECTION_TYPE_RX);
            /*lint +e416*/
        }
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return;
}




OAL_STATIC hmac_scanned_bss_info *hmac_scan_alloc_scanned_bss(oal_uint32 ul_mgmt_len)
{
    hmac_scanned_bss_info    *pst_scanned_bss;

    /* 申请内存，存储扫描到的bss信息 */
    pst_scanned_bss = oal_memalloc(OAL_SIZEOF(hmac_scanned_bss_info) + ul_mgmt_len - OAL_SIZEOF(pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_scanned_bss))
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_alloc_scanned_bss::alloc memory failed for storing scanned result.}");
        return OAL_PTR_NULL;
    }

    /* 为申请的内存清零 */
    OAL_MEMZERO(pst_scanned_bss, OAL_SIZEOF(hmac_scanned_bss_info) + ul_mgmt_len - OAL_SIZEOF(pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff));

    /* 初始化链表头节点指针 */
    oal_dlist_init_head(&(pst_scanned_bss->st_dlist_head));

     //pst_scanned_bss->st_bss_dscr_info.puc_mgmt_buff = (oal_uint8 *)pst_scanned_bss + OAL_SIZEOF(hmac_scanned_bss_info) - OAL_SIZEOF(pst_scanned_bss->st_bss_dscr_info.puc_mgmt_buff);

    return pst_scanned_bss;
}


OAL_STATIC oal_uint32 hmac_scan_add_bss_to_list(hmac_scanned_bss_info *pst_scanned_bss, hmac_device_stru *pst_hmac_device)
{
    hmac_bss_mgmt_stru  *pst_bss_mgmt;      /* 管理扫描结果的结构体 */

    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);
    pst_scanned_bss->st_bss_dscr_info.en_new_scan_bss = OAL_TRUE;

    /* 对链表写操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 添加扫描结果到链表中，并更新扫描到的bss计数 */
    oal_dlist_add_tail(&(pst_scanned_bss->st_dlist_head), &(pst_bss_mgmt->st_bss_list_head));

    pst_bss_mgmt->ul_bss_num++;
    /* 解锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_scan_del_bss_from_list_nolock(hmac_scanned_bss_info *pst_scanned_bss, hmac_device_stru *pst_hmac_device)
{
    hmac_bss_mgmt_stru  *pst_bss_mgmt;      /* 管理扫描结果的结构体 */

    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* 从链表中删除节点，并更新扫描到的bss计数 */
    oal_dlist_delete_entry(&(pst_scanned_bss->st_dlist_head));

    pst_bss_mgmt->ul_bss_num--;

    return OAL_SUCC;
}


oal_void hmac_scan_clean_scan(hmac_scan_stru  *pst_scan)
{
    hmac_scan_record_stru           *pst_scan_record;
    oal_dlist_head_stru             *pst_entry;
    hmac_scanned_bss_info           *pst_scanned_bss;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;

    /* 参数合法性检查 */
    if (OAL_PTR_NULL == pst_scan)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_clean_scan::pst_scan is null.}");
        return;
    }

    pst_scan_record = &pst_scan->st_scan_record_mgmt;

    /* 1.一定要先清除扫描到的bss信息，再进行清零处理 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 对链表写操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历链表，删除扫描到的bss信息 */
    while(OAL_FALSE == oal_dlist_is_empty(&(pst_bss_mgmt->st_bss_list_head)))
    {
        pst_entry       = oal_dlist_delete_head(&(pst_bss_mgmt->st_bss_list_head));
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);

        pst_bss_mgmt->ul_bss_num--;

        /* 释放扫描队列里的内存 */
        oal_free(pst_scanned_bss);
    }

    /* 对链表写操作前加锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    /* 2.其它信息清零 */
    OAL_MEMZERO(pst_scan_record, OAL_SIZEOF(hmac_scan_record_stru));
    pst_scan_record->en_scan_rsp_status = MAC_SCAN_STATUS_BUTT;     /* 初始化扫描完成时状态码为无效值 */
    pst_scan_record->en_vap_last_state  = MAC_VAP_STATE_BUTT;       /* 必须置BUTT,否则aput停扫描会vap状态恢复错 */

    /* 3.重新初始化bss管理结果链表和锁 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);
    oal_dlist_init_head(&(pst_bss_mgmt->st_bss_list_head));
    oal_spin_lock_init(&(pst_bss_mgmt->st_lock));

    /* 4.删除扫描超时定时器 */
    if (OAL_TRUE == pst_scan->st_scan_timeout.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_scan->st_scan_timeout));
    }

    OAM_INFO_LOG0(0, OAM_SF_SCAN, "{hmac_scan_clean_scan::cleaned scan record success.}");

    return;
}



OAL_STATIC oal_int32 hmac_is_connected_ap_bssid(oal_uint8 uc_device_id, oal_uint8 auc_bssid[WLAN_MAC_ADDR_LEN])
{
    oal_uint8                    uc_vap_idx;
    mac_vap_stru                *pst_mac_vap;
    mac_device_stru             *pst_mac_device;

    pst_mac_device = mac_res_get_dev_etc(uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_is_connected_ap_bssid::mac_res_get_dev_etc return null.}");
        return OAL_FALSE;
    }

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_P2P, "{hmac_is_connected_ap_bssid::mac_res_get_mac_vap fail! vap id is %d}",
                            pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if(IS_LEGACY_VAP(pst_mac_vap)
           && (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state))
        {
            if(0 == oal_memcmp(auc_bssid, pst_mac_vap->auc_bssid, WLAN_MAC_ADDR_LEN))
            {
                /* 不老化当前关联的AP */
                OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_is_connected_ap_bssid::connected AP bssid:%02X:XX:XX:XX:%02X:%02X}",
                                 auc_bssid[0], auc_bssid[4], auc_bssid[5]);

                return OAL_TRUE;
            }
        }

    }

    return OAL_FALSE;
}


OAL_STATIC oal_void hmac_scan_clean_expire_scanned_bss(hmac_vap_stru *pst_hmac_vap, hmac_scan_record_stru  *pst_scan_record)
{
    oal_dlist_head_stru             *pst_entry;
    oal_dlist_head_stru             *pst_entry_tmp = OAL_PTR_NULL;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;
    hmac_scanned_bss_info           *pst_scanned_bss;
    mac_bss_dscr_stru               *pst_bss_dscr;
    oal_uint32                       ul_curr_time_stamp;

    /* 参数合法性检查 */
    if (OAL_PTR_NULL == pst_scan_record)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_clean_expire_scanned_bss::scan record is null.}");
        return;
    }

    /* 管理扫描的bss结果的结构体 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    ul_curr_time_stamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    /* 对链表写操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历链表，删除上一次扫描结果中到期的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_entry_tmp, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        if (ul_curr_time_stamp - pst_bss_dscr->ul_timestamp < HMAC_SCAN_MAX_SCANNED_BSS_EXPIRE)
        {
            OAM_INFO_LOG0(0, OAM_SF_SCAN,
                             "{hmac_scan_clean_expire_scanned_bss::do not remove the BSS, because it has not expired.}");
            continue;
        }

        /* 不老化当前正在关联的AP */
        if(hmac_is_connected_ap_bssid(pst_scan_record->uc_device_id, pst_bss_dscr->auc_bssid))
        {
            pst_bss_dscr->c_rssi = pst_hmac_vap->station_info.signal;
            continue;
        }

        /* 从链表中删除节点，并更新扫描到的bss计数 */
        oal_dlist_delete_entry(&(pst_scanned_bss->st_dlist_head));
        pst_bss_mgmt->ul_bss_num--;

        /* 释放对应内存 */
        oal_free(pst_scanned_bss);
    }

    /* 对链表写操作前加锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));
    return;
}




mac_bss_dscr_stru *hmac_scan_find_scanned_bss_dscr_by_index_etc(oal_uint8  uc_device_id,
                                                                         oal_uint32 ul_bss_index)
{
    oal_dlist_head_stru             *pst_entry;
    hmac_scanned_bss_info           *pst_scanned_bss;
    hmac_device_stru                *pst_hmac_device;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;
    oal_uint8                        ul_loop;

    /* 获取hmac device 结构 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_find_scanned_bss_by_index::pst_hmac_device is null.}");
        return OAL_PTR_NULL;
    }

    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* 对链表删操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 如果索引大于总共扫描的bss个数，返回异常 */
    if (ul_bss_index >= pst_bss_mgmt->ul_bss_num)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_find_scanned_bss_by_index::no such bss in bss list!}");

        /* 解锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));
        return OAL_PTR_NULL;
    }

    ul_loop = 0;
    /* 遍历链表，返回对应index的bss dscr信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);

        /* 相同的bss index返回 */
        if (ul_bss_index == ul_loop)
        {
            /* 解锁 */
            oal_spin_unlock(&(pst_bss_mgmt->st_lock));
            return &(pst_scanned_bss->st_bss_dscr_info);
        }

        ul_loop++;
    }
    /* 解锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_PTR_NULL;
}


hmac_scanned_bss_info *hmac_scan_find_scanned_bss_by_bssid_etc(hmac_bss_mgmt_stru *pst_bss_mgmt, oal_uint8 *puc_bssid)
{
    oal_dlist_head_stru             *pst_entry;
    hmac_scanned_bss_info           *pst_scanned_bss;

    /* 遍历链表，查找链表中是否已经存在相同bssid的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);

        /* 相同的bssid地址 */
        if (0 == oal_compare_mac_addr(pst_scanned_bss->st_bss_dscr_info.auc_bssid, puc_bssid))
        {
            return pst_scanned_bss;
        }
    }

    return OAL_PTR_NULL;
}


oal_void *hmac_scan_get_scanned_bss_by_bssid(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr)
{
    hmac_bss_mgmt_stru             *pst_bss_mgmt;          /* 管理扫描的bss结果的结构体 */
    hmac_scanned_bss_info          *pst_scanned_bss_info;
    hmac_device_stru               *pst_hmac_device;

    /* 获取hmac device 结构 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_get_scanned_bss_by_bssid::pst_hmac_device is null, dev id[%d].}", pst_mac_vap->uc_device_id);
        return OAL_PTR_NULL;
    }

    /* 获取管理扫描的bss结果的结构体 */
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    pst_scanned_bss_info = hmac_scan_find_scanned_bss_by_bssid_etc(pst_bss_mgmt, puc_mac_addr);
    if (OAL_PTR_NULL == pst_scanned_bss_info)
    {
       OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                        "{hmac_scan_get_scanned_bss_by_bssid::find the bss failed[%02X:XX:XX:%02X:%02X:%02X]}",
                        puc_mac_addr[0], puc_mac_addr[3], puc_mac_addr[4], puc_mac_addr[5]);

       /* 解锁 */
       oal_spin_unlock(&(pst_bss_mgmt->st_lock));
       return OAL_PTR_NULL;
    }

    /* 解锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return &(pst_scanned_bss_info->st_bss_dscr_info);
}


OAL_STATIC OAL_INLINE oal_void  hmac_scan_update_bss_list_wmm(mac_bss_dscr_stru   *pst_bss_dscr,
                                                                         oal_uint8           *puc_frame_body,
                                                                         oal_uint16           us_frame_len)
{
    oal_uint8  *puc_ie;

    pst_bss_dscr->uc_wmm_cap   = OAL_FALSE;
    pst_bss_dscr->uc_uapsd_cap = OAL_FALSE;

    puc_ie = mac_get_wmm_ie_etc(puc_frame_body, us_frame_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        pst_bss_dscr->uc_wmm_cap = OAL_TRUE;

        /* --------------------------------------------------------------------------------- */
        /* WMM Information/Parameter Element Format                                          */
        /* ----------------------------------------------------------------------------------*/
        /* EID | IE LEN | OUI | OUIType | OUISubtype | Version | QoSInfo | OUISubtype based |*/
        /* --------------------------------------------------------------------------------- */
        /* 1   |   1    |  3  | 1       | 1          | 1       | 1       | ---------------- |*/
        /* --------------------------------------------------------------------------------- */
        /* puc_ie[1] IE len 不包含EID和LEN字段,获取QoSInfo，uc_ie_len必须大于7字节长度 */
        /* Check if Bit 7 is set indicating U-APSD capability */
        if ((puc_ie[1] >= 7)&&(puc_ie[8] & BIT7))  /* wmm ie的第8个字节是QoS info字节 */
        {
            pst_bss_dscr->uc_uapsd_cap = OAL_TRUE;
        }
    }
    else
    {
        puc_ie = mac_find_ie_etc(MAC_EID_HT_CAP, puc_frame_body, us_frame_len);
        if (OAL_PTR_NULL != puc_ie)
        {
            
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            /* --------------------------------------------------------------*/
            /* HT capability Information/Parameter Element Format            */
            /* --------------------------------------------------------------*/
            /* EID | IE LEN |  HT capability Info |                 based   |*/
            /* --------------------------------------------------------------*/
            /* 1   |   1    |         2           | ------------------------|*/
            /* --------------------------------------------------------------*/
            /* puc_ie[1] IE len 不包含EID和LEN字段,获取HT cap Info，uc_ie_len必须大于2字节长度 */
            /* ht cap的第 2,3个字节是HT capability Info信息 */
            /* Check if Bit 5 is set indicating short GI for 20M capability */
            if ((puc_ie[1] >= 2)&&(puc_ie[2] & BIT5))
#endif
            {
                pst_bss_dscr->uc_wmm_cap = OAL_TRUE;
            }
        }
    }
}

#ifdef _PRE_WLAN_FEATURE_11D

OAL_STATIC oal_void  hmac_scan_update_bss_list_country(mac_bss_dscr_stru   *pst_bss_dscr,
                                                                oal_uint8           *puc_frame_body,
                                                                oal_uint16           us_frame_len)
{
    oal_uint8 *puc_ie;

    puc_ie = mac_find_ie_etc(MAC_EID_COUNTRY, puc_frame_body, us_frame_len);

    /* 国家码不存在, 全部标记为0 */
    if (OAL_PTR_NULL == puc_ie)
    {
        pst_bss_dscr->ac_country[0] = 0;
        pst_bss_dscr->ac_country[1] = 0;
        pst_bss_dscr->ac_country[2] = 0;

        return;
    }
    else
    {
        pst_bss_dscr->puc_country_ie = puc_ie;
    }
    /* 国家码采用2个字节,IE LEN必须大于等于2 */
    if(puc_ie[1] >= 2)
    {
        pst_bss_dscr->ac_country[0] = (oal_int8)puc_ie[MAC_IE_HDR_LEN];
        pst_bss_dscr->ac_country[1] = (oal_int8)puc_ie[MAC_IE_HDR_LEN + 1];
        pst_bss_dscr->ac_country[2] = 0;
    }
}
#endif

#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN) || defined(_PRE_WLAN_FEATURE_FTM) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)

OAL_STATIC oal_void hmac_scan_update_bss_list_rrm(mac_bss_dscr_stru *pst_bss_dscr,
                                                            oal_uint8                   *puc_frame_body,
                                                            oal_uint16                   us_frame_len)
{
    oal_uint8 *puc_ie;
    puc_ie = mac_find_ie_etc(MAC_EID_RRM, puc_frame_body, us_frame_len);
    if(OAL_PTR_NULL == puc_ie)
    {
        pst_bss_dscr->en_support_rrm = OAL_FALSE;
    }
    else
    {
        pst_bss_dscr->en_support_rrm = OAL_TRUE;
    }
}
#endif

#ifdef _PRE_WLAN_FEATURE_1024QAM

OAL_STATIC oal_void hmac_scan_update_bss_list_1024qam(mac_bss_dscr_stru     *pst_bss_dscr,
                                                                        oal_uint8              *puc_frame_body,
                                                                        oal_uint16              us_frame_len)
{
    oal_uint8 *puc_ie;
    puc_ie = mac_find_vendor_ie_etc(MAC_HUAWEI_VENDER_IE, MAC_HISI_1024QAM_IE, puc_frame_body, us_frame_len);
    if(OAL_PTR_NULL == puc_ie)
    {
        pst_bss_dscr->en_support_1024qam= OAL_FALSE;
    }
    else
    {
        pst_bss_dscr->en_support_1024qam = OAL_TRUE;
    }
}
#endif

#ifdef _PRE_WLAN_NARROW_BAND

OAL_STATIC oal_void  hmac_scan_update_bss_list_nb(mac_bss_dscr_stru   *pst_bss_dscr,
                                                                oal_uint8           *puc_frame_body,
                                                                oal_uint16           us_frame_len)
{
    oal_uint8 *puc_ie;

    puc_ie = mac_find_vendor_ie_etc(MAC_HUAWEI_VENDER_IE, MAC_HISI_NB_IE, puc_frame_body, us_frame_len);

    /* 判断是否携带该IE */
    if (OAL_PTR_NULL == puc_ie)
    {
        pst_bss_dscr->en_nb_capable = OAL_FALSE;
    }
    else
    {
        pst_bss_dscr->en_nb_capable = OAL_TRUE;
    }
}
#endif


OAL_STATIC oal_void  hmac_scan_update_11i(mac_bss_dscr_stru *pst_bss_dscr, oal_uint8 *puc_frame_body, oal_uint16 us_frame_len)
{
    pst_bss_dscr->puc_rsn_ie = mac_find_ie_etc(MAC_EID_RSN, puc_frame_body, (oal_int32)(us_frame_len));
    pst_bss_dscr->puc_wpa_ie = mac_find_vendor_ie_etc(MAC_WLAN_OUI_MICROSOFT, MAC_OUITYPE_WPA, puc_frame_body, (oal_int32)(us_frame_len));
}


OAL_STATIC oal_void  hmac_scan_update_bss_list_11n(mac_bss_dscr_stru *pst_bss_dscr, oal_uint8 *puc_frame_body, oal_uint16 us_frame_len)
{
    oal_uint8               *puc_ie;
    mac_ht_opern_stru       *pst_ht_op;
    oal_uint8                uc_sec_chan_offset;
    wlan_bw_cap_enum_uint8   en_ht_cap_bw = WLAN_BW_CAP_20M;
    wlan_bw_cap_enum_uint8   en_ht_op_bw  = WLAN_BW_CAP_20M;

    /* 11n */
    puc_ie = mac_find_ie_etc(MAC_EID_HT_CAP, puc_frame_body, us_frame_len);
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= 2))
    {
        /* puc_ie[2]是HT Capabilities Info的第1个字节 */
        pst_bss_dscr->en_ht_capable = OAL_TRUE;     /* 支持ht */
        pst_bss_dscr->en_ht_ldpc = (puc_ie[2] & BIT0);           /* 支持ldpc */
        en_ht_cap_bw = ((puc_ie[2] & BIT1) >> 1);                /* 取出支持的带宽 */
        pst_bss_dscr->en_ht_stbc = ((puc_ie[2] & BIT7) >> 7);
    }

    /* 默认20M,如果帧内容未携带HT_OPERATION则可以直接采用默认值 */
    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_20M;

    puc_ie = mac_find_ie_etc(MAC_EID_HT_OPERATION, puc_frame_body, us_frame_len);
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= 2))   //增加ie长度异常检查
    {
        pst_ht_op  = (mac_ht_opern_stru *)(puc_ie + MAC_IE_HDR_LEN);

        /* 提取次信道偏移 */
        uc_sec_chan_offset = pst_ht_op->bit_secondary_chan_offset;

        /* 防止ap的channel width=0, 但channel offset = 1或者3 此时以channel width为主 */
        /* ht cap 20/40 enabled && ht operation 40 enabled */
        if ((0 != pst_ht_op->bit_sta_chan_width) && (en_ht_cap_bw > WLAN_BW_CAP_20M)) //cap > 20M才取channel bw
        {
            if (MAC_SCB == uc_sec_chan_offset)
            {
                pst_bss_dscr->en_channel_bandwidth =  WLAN_BAND_WIDTH_40MINUS;
                en_ht_op_bw = WLAN_BW_CAP_40M;
            }
            else if (MAC_SCA == uc_sec_chan_offset)
            {
                pst_bss_dscr->en_channel_bandwidth =  WLAN_BAND_WIDTH_40PLUS;
                en_ht_op_bw = WLAN_BW_CAP_40M;
            }
        }
    }

    /* 将AP带宽能力取声明能力的最小值，防止AP异常发送超过带宽能力数据，造成数据不通 */
    pst_bss_dscr->en_bw_cap = OAL_MIN(en_ht_cap_bw, en_ht_op_bw);

    puc_ie = mac_find_ie_etc(MAC_EID_EXT_CAPS, puc_frame_body, us_frame_len);
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= 1))
    {
        /* Extract 20/40 BSS Coexistence Management Support */
        pst_bss_dscr->uc_coex_mgmt_supp = (puc_ie[2] & BIT0);
    }

#ifdef _PRE_WLAN_FEATURE_TXBF
    puc_ie = mac_find_vendor_ie_etc(MAC_HUAWEI_VENDER_IE, MAC_EID_11NTXBF, puc_frame_body, (oal_int32)us_frame_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        pst_bss_dscr->en_11ntxbf = (((mac_11ntxbf_vendor_ie_stru *)puc_ie)->st_11ntxbf.bit_11ntxbf == 1) ? OAL_TRUE : OAL_FALSE;

    }
#endif
}


OAL_STATIC oal_void  hmac_scan_update_bss_list_11ac(mac_bss_dscr_stru   *pst_bss_dscr,
                                                             oal_uint8           *puc_frame_body,
                                                             oal_uint16           us_frame_len,
                                                             oal_uint             en_is_vendor_ie)
{
    oal_uint8   *puc_ie;
    oal_uint8    uc_vht_chan_width;
    oal_uint8    uc_chan_center_freq;
    oal_uint8    uc_chan_center_freq_1;
    oal_uint8    uc_supp_ch_width;

    puc_ie = mac_find_ie_etc(MAC_EID_VHT_CAP, puc_frame_body, us_frame_len);
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= MAC_VHT_CAP_IE_LEN))
    {
        pst_bss_dscr->en_vht_capable = OAL_TRUE;     /* 支持vht */

        /* 说明vendor中携带VHT ie，则设置标志位，assoc req中也需携带vendor+vht ie */
        if(OAL_TRUE == en_is_vendor_ie)
        {
            pst_bss_dscr->en_vendor_vht_capable = OAL_TRUE;
        }

        /* 提取Supported Channel Width Set */
        uc_supp_ch_width = ((puc_ie[2] & (BIT3|BIT2)) >> 2);

        if (0 == uc_supp_ch_width)
        {
            pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_80M;   /* 80MHz */
        }
        else if (1 == uc_supp_ch_width)
        {
            pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_160M;  /* 160MHz */
        }
        else if(2 == uc_supp_ch_width)
        {
            pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_80M;   /* 80MHz */
        }
    }
    else
    {
        /* 私有vendor中不包含vht ie，适配BCM 5g 20M 私有协议 */
        if(OAL_TRUE == en_is_vendor_ie)
        {
            pst_bss_dscr->en_vendor_novht_capable = OAL_TRUE;
        }
    }

    puc_ie = mac_find_ie_etc(MAC_EID_VHT_OPERN, puc_frame_body, us_frame_len);
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= MAC_VHT_OPERN_LEN))
    {
        uc_vht_chan_width   = puc_ie[2];
        uc_chan_center_freq = puc_ie[3];
        uc_chan_center_freq_1 = puc_ie[4];

        /* 更新带宽信息 */
        if (0 == uc_vht_chan_width)          /* 40MHz */
        {
            /* do nothing，en_channel_bandwidth已经在HT Operation IE中获取 */
        }
        else if (1 == uc_vht_chan_width)     /* 80MHz */
        {
#ifdef _PRE_WLAN_FEATURE_160M
            if((uc_chan_center_freq_1 - uc_chan_center_freq == 8) || (uc_chan_center_freq - uc_chan_center_freq_1 == 8))
            {
                switch (uc_chan_center_freq_1 - pst_bss_dscr->st_channel.uc_chan_number)
                {
                    case 14:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSPLUSPLUS;
                        break;

                    case 10:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSPLUSPLUS;
                        break;

                    case 6:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSMINUSPLUS;
                        break;

                    case 2:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSMINUSPLUS;
                        break;

                    case -2:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSPLUSMINUS;
                        break;

                    case -6:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSPLUSMINUS;
                        break;

                    case -10:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSMINUSMINUS;
                        break;

                    case -14:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSMINUSMINUS;
                        break;

                    default:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_20M;
                        break;
                }
                return;
            }
#endif
            switch (uc_chan_center_freq - pst_bss_dscr->st_channel.uc_chan_number)
            {
                case 6:
                /***********************************************************************
                | 主20 | 从20 | 从40       |
                              |
                              |中心频率相对于主20偏6个信道
                ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80PLUSPLUS;
                    break;

                case -2:
                /***********************************************************************
                | 从40        | 主20 | 从20 |
                              |
                              |中心频率相对于主20偏-2个信道
                ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80PLUSMINUS;
                    break;

                case 2:
                /***********************************************************************
                | 从20 | 主20 | 从40       |
                              |
                              |中心频率相对于主20偏2个信道
                ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80MINUSPLUS;
                    break;

                case -6:
                /***********************************************************************
                | 从40        | 从20 | 主20 |
                              |
                              |中心频率相对于主20偏-6个信道
                ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80MINUSMINUS;
                    break;

                default:
                    break;

            }
        }
#ifdef _PRE_WLAN_FEATURE_160M
        else if (2 == uc_vht_chan_width)     /* 160MHz */
        {
            switch (uc_chan_center_freq - pst_bss_dscr->st_channel.uc_chan_number)
            {
                 case 14:
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSPLUSPLUS;
                    break;
                case 10:
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSPLUSPLUS;
                    break;

                case 6:
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSMINUSPLUS;
                    break;
                case 2:
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSMINUSPLUS;
                    break;
                case -2:
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSPLUSMINUS;
                    break;
                case -6:
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSPLUSMINUS;
                    break;
                case -10:
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSMINUSMINUS;
                    break;
                case -14:
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSMINUSMINUS;
                    break;
                default:
                    break;
            }
        }
#endif
        else if (3 == uc_vht_chan_width)     /* 80+80MHz */
        {
            switch (uc_chan_center_freq - pst_bss_dscr->st_channel.uc_chan_number)
            {
                case 6:
                /***********************************************************************
                | 主20 | 从20 | 从40       |
                              |
                              |中心频率相对于主20偏6个信道
                ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80PLUSPLUS;
                    break;

                case -2:
                /***********************************************************************
                | 从40        | 主20 | 从20 |
                              |
                              |中心频率相对于主20偏-2个信道
                ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80PLUSMINUS;
                    break;

                case 2:
                /***********************************************************************
                | 从20 | 主20 | 从40       |
                              |
                              |中心频率相对于主20偏2个信道
                ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80MINUSPLUS;
                    break;

                case -6:
                /***********************************************************************
                | 从40        | 从20 | 主20 |
                              |
                              |中心频率相对于主20偏-6个信道
                ************************************************************************/
                    pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80MINUSMINUS;
                    break;

                default:
                    break;

            }
        }
        else
        {
            /* Unsupported Channel BandWidth */
        }
    }
}

#ifdef _PRE_WLAN_FEATURE_11AX

OAL_STATIC oal_void  hmac_scan_update_bss_list_11ax(mac_bss_dscr_stru   *pst_bss_dscr,
                                                                 oal_uint8           *puc_frame_body,
                                                                 oal_uint16           us_frame_len)
{
    oal_uint8                     *puc_ie;
    mac_frame_he_cap_ie_stru       st_he_cap_value;
    mac_frame_he_oper_ie_stru      st_he_oper_ie_value;
    oal_uint32                     ul_ret;

    /*HE CAP*/
    puc_ie = mac_find_ie_ext_ie(MAC_EID_HE,MAC_EID_EXT_HE_CAP,puc_frame_body, us_frame_len);
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= MAC_HE_CAP_MIN_LEN))
    {
        OAL_MEMZERO(&st_he_cap_value, OAL_SIZEOF(st_he_cap_value));
        /*解析固定长度部分:MAC_Cap+PHY Cap + HE-MCS NSS(<=80MHz)  */
        ul_ret = mac_ie_parse_he_cap(puc_ie,&st_he_cap_value);
        if(OAL_SUCC != ul_ret)
        {
            return;
        }

        pst_bss_dscr->en_he_capable = OAL_TRUE;     /* 支持HE*/

        if(WLAN_BAND_2G == pst_bss_dscr->st_channel.en_band)
        {/*2G*/
            if(st_he_cap_value.st_he_phy_cap.bit_channel_width_set & 0x1)
            {/*Bit0 2G 是否支持40MHz */
                pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_40M;
            }
        }
        else
        {
            if(st_he_cap_value.st_he_phy_cap.bit_channel_width_set & 0x8)/*B3-5G 80+80MHz*/
            {
                pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_80PLUS80;
            }
            else if(st_he_cap_value.st_he_phy_cap.bit_channel_width_set & 0x4)/*B2-160MHz*/
            {
                pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_160M;
            }
            else if(st_he_cap_value.st_he_phy_cap.bit_channel_width_set & 0x2)/*B2-5G支持80MHz */
            {
                pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_80M;
            }
            else
            {
                pst_bss_dscr->en_bw_cap = WLAN_BW_CAP_20M;
            }

        }
    }

    /*HE Oper*/
    puc_ie = mac_find_ie_ext_ie(MAC_EID_HE,MAC_EID_EXT_HE_OPERATION,puc_frame_body, us_frame_len);
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= MAC_HE_OPERAION_MIN_LEN))
    {
        OAL_MEMZERO(&st_he_oper_ie_value, OAL_SIZEOF(st_he_oper_ie_value));
        ul_ret = mac_ie_parse_he_oper(puc_ie,&st_he_oper_ie_value);
        if(OAL_SUCC != ul_ret)
        {
            return;
        }

        if(1 == st_he_oper_ie_value.st_he_oper_param.bit_vht_operation_info_present)
        {
            if (0 == st_he_oper_ie_value.st_vht_operation_info.uc_channel_width)          /* 40MHz */
            {
                /* do nothing，en_channel_bandwidth已经在HT Operation IE中获取 */
            }
            else if (1 == st_he_oper_ie_value.st_vht_operation_info.uc_channel_width)     /* 80MHz */
            {
                switch (st_he_oper_ie_value.st_vht_operation_info.uc_center_freq_seg0 - pst_bss_dscr->st_channel.uc_chan_number)
                {
                    case 6:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80PLUSPLUS;
                        break;
                    case -2:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80PLUSMINUS;
                        break;
                    case 2:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80MINUSPLUS;
                        break;
                    case -6:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_80MINUSMINUS;
                        break;
                    default:
                        break;

                }
            }
        #ifdef _PRE_WLAN_FEATURE_160M
            else if (2 == st_he_oper_ie_value.st_vht_operation_info.uc_channel_width)     /* 160MHz */
            {
                switch (st_he_oper_ie_value.st_vht_operation_info.uc_center_freq_seg0 - pst_bss_dscr->st_channel.uc_chan_number)
                {
                    case 14:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSPLUSPLUS;
                        break;
                    case -2:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSPLUSMINUS;
                        break;
                    case 6:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSMINUSPLUS;
                        break;
                    case -10:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160PLUSMINUSMINUS;
                        break;
                    case 10:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSPLUSPLUS;
                        break;
                    case -6:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSPLUSMINUS;
                        break;
                    case 2:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSMINUSPLUS;
                        break;
                    case -14:
                        pst_bss_dscr->en_channel_bandwidth = WLAN_BAND_WIDTH_160MINUSMINUSMINUS;
                        break;
                    default:
                        break;

                }
            }
        #endif
        }
    }

}

#endif


OAL_STATIC OAL_INLINE oal_void  hmac_scan_update_bss_list_protocol(hmac_vap_stru *pst_hmac_vap,
                mac_bss_dscr_stru *pst_bss_dscr,oal_uint8 *puc_frame_body,oal_uint16 us_frame_len)
{

    oal_uint8 *puc_ie = OAL_PTR_NULL;
    oal_uint16 us_offset_vendor_vht = MAC_WLAN_OUI_VENDOR_VHT_HEADER + MAC_IE_HDR_LEN;

    puc_frame_body += MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    us_frame_len   -= MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    /*************************************************************************/
    /*                       Beacon Frame - Frame Body                       */
    /* ----------------------------------------------------------------------*/
    /* |Timestamp|BcnInt|CapInfo|SSID|SupRates|DSParamSet|TIM  |CountryElem |*/
    /* ----------------------------------------------------------------------*/
    /* |8        |2     |2      |2-34|3-10    |3         |6-256|8-256       |*/
    /* ----------------------------------------------------------------------*/
    /* |PowerConstraint |Quiet|TPC Report|ERP |RSN  |WMM |Extended Sup Rates|*/
    /* ----------------------------------------------------------------------*/
    /* |3               |8    |4         |3   |4-255|26  | 3-257            |*/
    /* ----------------------------------------------------------------------*/
    /* |BSS Load |HT Capabilities |HT Operation |Overlapping BSS Scan       |*/
    /* ----------------------------------------------------------------------*/
    /* |7        |28              |24           |16                         |*/
    /* ----------------------------------------------------------------------*/
    /* |Extended Capabilities |                                              */
    /* ----------------------------------------------------------------------*/
    /* |3-8                   |                                              */
    /*************************************************************************/

    /* wmm */
    hmac_scan_update_bss_list_wmm(pst_bss_dscr, puc_frame_body, us_frame_len);

    /* 11i */
    hmac_scan_update_11i(pst_bss_dscr, puc_frame_body, us_frame_len);

#ifdef _PRE_WLAN_FEATURE_11D
    /* 11d */
    hmac_scan_update_bss_list_country(pst_bss_dscr, puc_frame_body, us_frame_len);
#endif

    /* 11n */
    hmac_scan_update_bss_list_11n(pst_bss_dscr, puc_frame_body, us_frame_len);

    /*rrm*/
#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN) || defined(_PRE_WLAN_FEATURE_FTM) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)
        hmac_scan_update_bss_list_rrm(pst_bss_dscr, puc_frame_body, us_frame_len);
#endif

    /* 11ac */
    hmac_scan_update_bss_list_11ac(pst_bss_dscr,  puc_frame_body, us_frame_len, OAL_FALSE);

    /*11ax*/
#ifdef _PRE_WLAN_FEATURE_11AX
    if(IS_11AX_VAP(&pst_hmac_vap->st_vap_base_info))
    {
        hmac_scan_update_bss_list_11ax(pst_bss_dscr,puc_frame_body,us_frame_len);
    }
#endif
    /* 查找私有vendor ie */
    puc_ie = mac_find_vendor_ie_etc(MAC_WLAN_OUI_BROADCOM_EPIGRAM,
                                MAC_WLAN_OUI_VENDOR_VHT_TYPE,
                                puc_frame_body,
                                us_frame_len);
    if ((OAL_PTR_NULL != puc_ie) && (puc_ie[1] >= MAC_WLAN_OUI_VENDOR_VHT_HEADER))
    {
        hmac_scan_update_bss_list_11ac(pst_bss_dscr,  puc_ie + us_offset_vendor_vht, puc_ie[1] - MAC_WLAN_OUI_VENDOR_VHT_HEADER, OAL_TRUE);
    }

    /* nb */
#ifdef _PRE_WLAN_NARROW_BAND
    hmac_scan_update_bss_list_nb(pst_bss_dscr, puc_frame_body, us_frame_len);
#endif

#ifdef _PRE_WLAN_FEATURE_1024QAM
    hmac_scan_update_bss_list_1024qam(pst_bss_dscr, puc_frame_body, us_frame_len);
#endif

    /* nb */
#ifdef _PRE_WLAN_NARROW_BAND
    hmac_scan_update_bss_list_nb(pst_bss_dscr, puc_frame_body, us_frame_len);
#endif

#ifdef _PRE_WLAN_1103_DDC_BUGFIX
    if (OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_ATHEROSC, MAC_WLAN_CHIP_OUI_TYPE_ATHEROSC, puc_frame_body, us_frame_len))
    {
        pst_bss_dscr->en_ddc_whitelist_chip_oui = OAL_TRUE;
    }
    else
    {
        pst_bss_dscr->en_ddc_whitelist_chip_oui = OAL_FALSE;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    if (OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_RALINK, MAC_WLAN_CHIP_OUI_TYPE_RALINK, puc_frame_body, us_frame_len) ||
        OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_RALINK, MAC_WLAN_CHIP_OUI_TYPE_RALINK1, puc_frame_body, us_frame_len) ||
        OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_SHENZHEN, MAC_WLAN_CHIP_OUI_TYPE_SHENZHEN, puc_frame_body, us_frame_len))
    {
        pst_bss_dscr->en_btcoex_blacklist_chip_oui = OAL_TRUE;
    }
    else
    {
        pst_bss_dscr->en_btcoex_blacklist_chip_oui = OAL_FALSE;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
    if (OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_BROADCOM, MAC_WLAN_CHIP_OUI_TYPE_BROADCOM, puc_frame_body, us_frame_len))
    {
        pst_bss_dscr->en_roam_blacklist_chip_oui = OAL_TRUE;
    }
    else
    {
        pst_bss_dscr->en_roam_blacklist_chip_oui = OAL_FALSE;
    }
#endif

    if (OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_RALINK, MAC_WLAN_CHIP_OUI_TYPE_RALINK, puc_frame_body, us_frame_len))
    {
        pst_bss_dscr->en_is_tplink_oui = WLAN_AP_CHIP_OUI_RALINK;
    }
    else if(OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_BROADCOM, MAC_WLAN_CHIP_OUI_TYPE_BROADCOM, puc_frame_body, us_frame_len))
    {
        pst_bss_dscr->en_is_tplink_oui = WLAN_AP_CHIP_OUI_BCM;
    }

#if 0
    if (OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_RALINK, MAC_WLAN_CHIP_OUI_TYPE_RALINK, puc_frame_body, us_frame_len))
    {
        pst_bss_dscr->en_is_tplink_oui = WLAN_AP_CHIP_OUI_RALINK;
    }
    else if(OAL_PTR_NULL !=
            mac_find_vendor_ie_etc(MAC_WLAN_CHIP_OUI_ATHEROSC, MAC_WLAN_CHIP_OUI_TYPE_ATHEROSC, puc_frame_body, us_frame_len))
    {
        pst_bss_dscr->en_is_tplink_oui = WLAN_AP_CHIP_OUI_ATHEROS;
    }
    else
    {
        pst_bss_dscr->en_is_tplink_oui = WLAN_AP_CHIP_OUI_NORMAL;
    }
#endif
}

#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1151)

oal_uint8  hmac_scan_check_bss_supp_rates_etc(mac_device_stru *pst_mac_dev,
                                                     oal_uint8 *puc_rate,
                                                     oal_uint8  uc_bss_rate_num,
                                                     oal_uint8 *puc_update_rate)
{
    mac_data_rate_stru            *pst_rates;
    oal_uint32                     i,j,k;
    oal_uint8                      uc_rate_num = 0;

    pst_rates   = mac_device_get_all_rates_etc(pst_mac_dev);

    for(i = 0; i < uc_bss_rate_num; i++)
    {
        for(j = 0; j < MAC_DATARATES_PHY_80211G_NUM; j++)
        {
            if ((IS_EQUAL_RATES(pst_rates[j].uc_mac_rate, puc_rate[i]))
                && (uc_rate_num < MAC_DATARATES_PHY_80211G_NUM))
            {
                /* 去除重复速率 */
                for(k = 0; k < uc_rate_num; k++)
                {
                    if(IS_EQUAL_RATES(puc_update_rate[k], puc_rate[i]))
                    {
                        break;
                    }
                }
                /* 当不存在重复速率时，k等于uc_rate_num */
                if(k == uc_rate_num)
                {
                    puc_update_rate[uc_rate_num++] = puc_rate[i];
                }
                break;
            }
        }
    }

    return uc_rate_num;
}
#endif


void hmac_scan_rm_repeat_sup_exsup_rates(mac_bss_dscr_stru *pst_bss_dscr,
                                                                        oal_uint8      *puc_rates,
                                                                        oal_uint8       uc_exrate_num)
{
    int i,j;
    for(i = 0; i < uc_exrate_num; i++)
    {
        /* 去除重复速率 */
        for(j = 0; j < pst_bss_dscr->uc_num_supp_rates; j++)
        {
            if(IS_EQUAL_RATES(puc_rates[i], pst_bss_dscr->auc_supp_rates[j]))
            {
                break;
            }
        }

        /* 只有不存在重复速率时，j等于pst_bss_dscr->uc_num_supp_rates */
        if(j == pst_bss_dscr->uc_num_supp_rates && WLAN_USER_MAX_SUPP_RATES > pst_bss_dscr->uc_num_supp_rates)
        {
            pst_bss_dscr->auc_supp_rates[pst_bss_dscr->uc_num_supp_rates++] = puc_rates[i];
        }
    }
}


OAL_STATIC OAL_INLINE oal_uint32  hmac_scan_update_bss_list_rates(mac_bss_dscr_stru *pst_bss_dscr,
                                                                          oal_uint8         *puc_frame_body,
                                                                          oal_uint16         us_frame_len,
                                                                          mac_device_stru   *pst_mac_dev)
{
    oal_uint8   *puc_ie;
    oal_uint8    uc_num_rates    = 0;
    oal_uint8    uc_num_ex_rates = 0;
    oal_uint8    us_offset;
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1151)
    oal_uint8    auc_rates[MAC_DATARATES_PHY_80211G_NUM] = {0};
#endif

    /* 设置Beacon帧的field偏移量 */
    us_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;

    //puc_ie = mac_get_supported_rates_ie(puc_frame_body, us_frame_len,us_offset);
    puc_ie = mac_find_ie_etc(MAC_EID_RATES, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (OAL_PTR_NULL != puc_ie)
    {
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1151)
        uc_num_rates = hmac_scan_check_bss_supp_rates_etc(pst_mac_dev, puc_ie + MAC_IE_HDR_LEN, puc_ie[1], auc_rates);
#else
        uc_num_rates = puc_ie[1];
#endif
        if (uc_num_rates > WLAN_USER_MAX_SUPP_RATES)
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_bss_list_rates::uc_num_rates=%d.}", uc_num_rates);
            uc_num_rates = WLAN_USER_MAX_SUPP_RATES;
        }

#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1151)
        oal_memcopy(pst_bss_dscr->auc_supp_rates, auc_rates, uc_num_rates);
#else
        oal_memcopy(pst_bss_dscr->auc_supp_rates, puc_ie + MAC_IE_HDR_LEN, uc_num_rates);
#endif
        pst_bss_dscr->uc_num_supp_rates = uc_num_rates;
    }

    //puc_ie = mac_get_exsup_rates_ie(puc_frame_body, us_frame_len, us_offset);
    puc_ie = mac_find_ie_etc(MAC_EID_XRATES, puc_frame_body + us_offset, us_frame_len - us_offset);
    if (OAL_PTR_NULL != puc_ie)
    {
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1151)
        uc_num_ex_rates = hmac_scan_check_bss_supp_rates_etc(pst_mac_dev, puc_ie + MAC_IE_HDR_LEN, puc_ie[1], auc_rates);
#else
        uc_num_ex_rates = puc_ie[1];
#endif

        if (uc_num_rates + uc_num_ex_rates > WLAN_USER_MAX_SUPP_RATES) /* 超出支持速率个数 */
        {
            OAM_WARNING_LOG2(0, OAM_SF_SCAN,
                            "{hmac_scan_update_bss_list_rates::number of rates too large, uc_num_rates=%d, uc_num_ex_rates=%d.}",
                            uc_num_rates, uc_num_ex_rates);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
            uc_num_ex_rates = WLAN_USER_MAX_SUPP_RATES - uc_num_rates;
#endif
        }

        if (uc_num_ex_rates > 0)
        {
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1151)
            /* support_rates和extended_rates去除重复速率，一并合入扫描结果的速率集中 */
            hmac_scan_rm_repeat_sup_exsup_rates(pst_bss_dscr, auc_rates, uc_num_ex_rates);
#else
            oal_memcopy(&(pst_bss_dscr->auc_supp_rates[uc_num_rates]), puc_ie + MAC_IE_HDR_LEN, (oal_uint32)uc_num_ex_rates);
            pst_bss_dscr->uc_num_supp_rates += uc_num_ex_rates;
#endif
        }
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  hmac_scan_update_bss_dscr(hmac_scanned_bss_info   *pst_scanned_bss,
                                                          dmac_tx_event_stru      *pst_dtx_event,
                                                          oal_uint8                uc_device_id,
                                                          oal_uint8                uc_vap_id)
{
    oal_netbuf_stru                      *pst_netbuf    = pst_dtx_event->pst_netbuf;
    mac_scanned_result_extend_info_stru  *pst_scan_result_extend_info;
    mac_device_stru                      *pst_mac_device;
    hmac_vap_stru                        *pst_hmac_vap;
    mac_ieee80211_frame_stru             *pst_frame_header;
    oal_uint8                            *puc_frame_body;
    mac_bss_dscr_stru                    *pst_bss_dscr;
    oal_uint8                            *puc_ssid;                 /* 指向beacon帧中的ssid */
    oal_uint8                            *puc_mgmt_frame;
    dmac_rx_ctl_stru                     *pst_rx_ctrl;
    oal_uint16                            us_netbuf_len = pst_dtx_event->us_frame_len;
    oal_uint16                            us_frame_len;
    oal_uint16                            us_frame_body_len;
    oal_uint16                            us_offset =  MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    oal_uint8                             uc_ssid_len;
    oal_uint8                             uc_frame_channel;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32                            ul_ret;
#endif


    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_update_bss_dscr::pst_hmac_vap is null.}");
        return OAL_FAIL;
    }

    /* 获取mac device */
    pst_mac_device  = mac_res_get_dev_etc(uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_update_bss_dscr::pst_mac_device is null.}");
        return OAL_FAIL;
    }

    /* 获取device上报的扫描结果信息，并将其更新到bss描述结构体中 */
    us_frame_len   = us_netbuf_len - OAL_SIZEOF(mac_scanned_result_extend_info_stru);
    puc_mgmt_frame = (oal_uint8 *)OAL_NETBUF_DATA(pst_netbuf);
    pst_rx_ctrl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    /* 指向netbuf中的上报的扫描结果的扩展信息的位置 */
    pst_scan_result_extend_info = (mac_scanned_result_extend_info_stru *)(puc_mgmt_frame + us_frame_len);

    /* 拷贝管理帧内容 */
    oal_memcopy(pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff, puc_mgmt_frame, (oal_uint32)us_frame_len);
    puc_mgmt_frame = pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff;

    /* 获取管理帧的帧头和帧体指针 */
    pst_frame_header  = (mac_ieee80211_frame_stru *)puc_mgmt_frame;
    puc_frame_body    = puc_mgmt_frame + MAC_80211_FRAME_LEN;
    us_frame_body_len = us_frame_len - MAC_80211_FRAME_LEN;

    /* 获取管理帧中的信道 */
    uc_frame_channel = mac_ie_get_chan_num_etc(puc_frame_body, us_frame_body_len, us_offset, pst_rx_ctrl->st_rx_info.uc_channel_number);

    /* 更新bss信息 */
    pst_bss_dscr = &(pst_scanned_bss->st_bss_dscr_info);

    /*****************************************************************************
        解析beacon/probe rsp帧，记录到pst_bss_dscr
    *****************************************************************************/
    /* 解析并保存ssid */
    puc_ssid = mac_get_ssid_etc(puc_frame_body, (oal_int32)us_frame_body_len, &uc_ssid_len);
    if ((OAL_PTR_NULL != puc_ssid) && (0 != uc_ssid_len))
    {
        /* 将查找到的ssid保存到bss描述结构体中 */
        oal_memcopy(pst_bss_dscr->ac_ssid, puc_ssid, uc_ssid_len);
        pst_bss_dscr->ac_ssid[uc_ssid_len] = '\0';
    }

    /* 解析bssid */
    oal_set_mac_addr(pst_bss_dscr->auc_mac_addr, pst_frame_header->auc_address2);
    oal_set_mac_addr(pst_bss_dscr->auc_bssid, pst_frame_header->auc_address3);

    /* bss基本信息 */
    pst_bss_dscr->en_bss_type = pst_scan_result_extend_info->en_bss_type;

    pst_bss_dscr->us_cap_info = *((oal_uint16 *)(puc_frame_body + MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN));

    pst_bss_dscr->c_rssi      =  (oal_int8)pst_scan_result_extend_info->l_rssi;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    pst_bss_dscr->c_ant0_rssi =  (oal_int8)pst_scan_result_extend_info->c_ant0_rssi;
    pst_bss_dscr->c_ant1_rssi =  (oal_int8)pst_scan_result_extend_info->c_ant1_rssi;
#endif
    /* 解析beacon周期与tim周期 */
    pst_bss_dscr->us_beacon_period = mac_get_beacon_period_etc(puc_frame_body);
    pst_bss_dscr->uc_dtim_period   = mac_get_dtim_period_etc(puc_frame_body, us_frame_body_len);
    pst_bss_dscr->uc_dtim_cnt      = mac_get_dtim_cnt_etc(puc_frame_body, us_frame_body_len);

    /* 信道 */
    pst_bss_dscr->st_channel.uc_chan_number = uc_frame_channel;
    pst_bss_dscr->st_channel.en_band        = mac_get_band_by_channel_num(uc_frame_channel);

#ifdef _PRE_WLAN_WEB_CMD_COMM
    switch (pst_scan_result_extend_info->en_bw)
    {
        case WLAN_BAND_ASSEMBLE_40M:
            pst_bss_dscr->st_channel.en_bandwidth = WLAN_BAND_WIDTH_40M;
            break;

        case WLAN_BAND_ASSEMBLE_80M:
            pst_bss_dscr->st_channel.en_bandwidth = WLAN_BAND_WIDTH_80M;
            break;

#ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_BAND_ASSEMBLE_160M:
            pst_bss_dscr->st_channel.en_bandwidth = WLAN_BAND_WIDTH_160MINUSMINUSMINUS;
            break;
#endif
        case WLAN_BAND_ASSEMBLE_20M:
        default:
            pst_bss_dscr->st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
            break;
    }
#endif

    /* 记录速率集 */
    hmac_scan_update_bss_list_rates(pst_bss_dscr, puc_frame_body, us_frame_body_len, pst_mac_device);

    /* 记录支持的最大空间流、最大速率信息 */
#ifdef _PRE_WLAN_WEB_CMD_COMM
    pst_bss_dscr->uc_max_nss        =  pst_scan_result_extend_info->uc_max_nss;
    pst_bss_dscr->ul_max_rate_kbps  =  pst_scan_result_extend_info->ul_max_rate_kbps;
#endif

    /* 03记录支持的最大空间流*/
#ifdef _PRE_WLAN_FEATURE_M2S
    pst_bss_dscr->en_support_max_nss = pst_scan_result_extend_info->en_support_max_nss;
    pst_bss_dscr->en_support_opmode = pst_scan_result_extend_info->en_support_opmode;
    pst_bss_dscr->uc_num_sounding_dim = pst_scan_result_extend_info->uc_num_sounding_dim;
#endif

    /* 协议类相关信息元素的获取 */
    hmac_scan_update_bss_list_protocol(pst_hmac_vap,pst_bss_dscr, puc_frame_body, us_frame_body_len);

    /* update st_channel.bandwidth in case hmac_sta_update_join_req_params_etc usage error */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_bss_dscr->st_channel.en_bandwidth = pst_bss_dscr->en_channel_bandwidth;
    ul_ret = mac_get_channel_idx_from_num_etc(pst_bss_dscr->st_channel.en_band,
        pst_bss_dscr->st_channel.uc_chan_number, &pst_bss_dscr->st_channel.uc_chan_idx);
    if(OAL_ERR_CODE_INVALID_CONFIG == ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{hmac_scan_update_bss_dscr::mac_get_channel_idx_from_num_etc fail.}");
    }
#endif

    /* 更新时间戳 */
    pst_bss_dscr->ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    pst_bss_dscr->ul_mgmt_len = us_frame_len;
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    //OAL_IO_PRINT("hmac_scan_update_bss_dscr\n");
    if (pst_hmac_vap->bit_11k_enable)
    {
        /*RSNI*/
        pst_bss_dscr->ac_rsni[0] = pst_scan_result_extend_info->c_snr_ant0;
        pst_bss_dscr->ac_rsni[1] = pst_scan_result_extend_info->c_snr_ant1;
        /*phy type*/
        if ( WLAN_BAND_2G == pst_bss_dscr->st_channel.en_band )
        {
            pst_bss_dscr->uc_phy_type = PHY_TYPE_DSSS;
        }
        else
        {
            pst_bss_dscr->uc_phy_type = PHY_TYPE_OFDM;
        }
        /*Parent TSF*/
        pst_bss_dscr->ul_parent_tsf = pst_scan_result_extend_info->ul_parent_tsf;
    }
#endif

    return OAL_SUCC;
}

oal_bool_enum_uint8 hmac_scan_is_hidden_ssid(oal_uint8 uc_vap_id, hmac_scanned_bss_info *pst_new_bss, hmac_scanned_bss_info *pst_old_bss)
{
    if (('\0' == pst_new_bss->st_bss_dscr_info.ac_ssid[0]) &&
           ('\0' != pst_old_bss->st_bss_dscr_info.ac_ssid[0]))
    {
        /*  隐藏SSID,若保存过此AP信息,且ssid不为空,此次通过BEACON帧扫描到此AP信息,且SSID为空,则不进行更新 */
        OAM_WARNING_LOG3(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_is_hidden_ssid::find hide ssid:%.2x:%.2x:%.2x,ignore this update.}",
                                pst_new_bss->st_bss_dscr_info.auc_bssid[3],
                                pst_new_bss->st_bss_dscr_info.auc_bssid[4],
                                pst_new_bss->st_bss_dscr_info.auc_bssid[5]);
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_bool_enum_uint8 hmac_scan_need_update_old_scan_result_etc(oal_uint8 uc_vap_id, hmac_scanned_bss_info *pst_new_bss, hmac_scanned_bss_info *pst_old_bss)
{
    
    if ((WLAN_PROBE_RSP == ((mac_ieee80211_frame_stru *)pst_old_bss->st_bss_dscr_info.auc_mgmt_buff)->st_frame_control.bit_sub_type)
        && (WLAN_BEACON == ((mac_ieee80211_frame_stru *)pst_new_bss->st_bss_dscr_info.auc_mgmt_buff)->st_frame_control.bit_sub_type)
        && (OAL_TRUE == pst_old_bss->st_bss_dscr_info.en_new_scan_bss))
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;

}


oal_uint8 hmac_scan_check_chan(oal_netbuf_stru *pst_netbuf, hmac_scanned_bss_info *pst_scanned_bss)
{
    dmac_rx_ctl_stru   *pst_rx_ctrl;
    oal_uint8           uc_curr_chan;
    oal_uint8          *puc_frame_body;
    oal_uint16          us_frame_body_len;
    oal_uint16          us_offset =  MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    oal_uint8          *puc_ie_start_addr;
    oal_uint8           uc_chan_num;

    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    uc_curr_chan = pst_rx_ctrl->st_rx_info.uc_channel_number;
    puc_frame_body = pst_scanned_bss->st_bss_dscr_info.auc_mgmt_buff + MAC_80211_FRAME_LEN;
    us_frame_body_len = pst_scanned_bss->st_bss_dscr_info.ul_mgmt_len - MAC_80211_FRAME_LEN;

    /* 在DSSS Param set ie中解析chan num */
    puc_ie_start_addr = mac_find_ie_etc(MAC_EID_DSPARMS, puc_frame_body + us_offset, us_frame_body_len - us_offset);
    if ((OAL_PTR_NULL != puc_ie_start_addr) && (puc_ie_start_addr[1] == MAC_DSPARMS_LEN))
    {
        uc_chan_num = puc_ie_start_addr[2];
        if (OAL_SUCC != mac_is_channel_num_valid_etc(mac_get_band_by_channel_num(uc_chan_num), uc_chan_num))
        {
            return  OAL_FALSE;
        }
    }

    /* 在HT operation ie中解析 chan num */
    puc_ie_start_addr = mac_find_ie_etc(MAC_EID_HT_OPERATION, puc_frame_body + us_offset, us_frame_body_len - us_offset);
    if ((OAL_PTR_NULL != puc_ie_start_addr) && (puc_ie_start_addr[1] >= 1))
    {
        uc_chan_num = puc_ie_start_addr[2];
        if (OAL_SUCC != mac_is_channel_num_valid_etc(mac_get_band_by_channel_num(uc_chan_num), uc_chan_num))
        {
            return  OAL_FALSE;
        }
    }

    uc_chan_num = pst_scanned_bss->st_bss_dscr_info.st_channel.uc_chan_number;
    if(((uc_curr_chan > uc_chan_num) && (uc_curr_chan - uc_chan_num >= 3))
        || ((uc_curr_chan < uc_chan_num) && (uc_chan_num - uc_curr_chan >= 3)))
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


oal_uint32 hmac_scan_proc_scanned_bss_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    frw_event_hdr_stru         *pst_event_hdr;
    hmac_vap_stru              *pst_hmac_vap;
    oal_netbuf_stru            *pst_bss_mgmt_netbuf;
    hmac_device_stru           *pst_hmac_device;
    dmac_tx_event_stru         *pst_dtx_event;
    hmac_scanned_bss_info      *pst_new_scanned_bss;
    hmac_scanned_bss_info      *pst_old_scanned_bss;
    oal_uint32                  ul_ret;
    oal_uint16                  us_mgmt_len;
    oal_uint8                   uc_vap_id;
    hmac_bss_mgmt_stru         *pst_bss_mgmt;
    oal_uint32                  ul_curr_time_stamp;
#ifdef _PRE_WLAN_FEATURE_M2S
    mac_ieee80211_frame_stru   *pst_frame_header;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event           = frw_get_event_stru(pst_event_mem);
    pst_event_hdr       = &(pst_event->st_event_hdr);
    pst_dtx_event       = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_bss_mgmt_netbuf = pst_dtx_event->pst_netbuf;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss_etc::pst_hmac_vap null.}");

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取vap id */
    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    /* 获取hmac device 结构 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_event_hdr->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_device))
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss_etc::pst_hmac_device null.}");

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 对dmac上报的netbuf内容进行解析，内容如下所示 */
    /***********************************************************************************************/
    /*            netbuf data域的上报的扫描结果的字段的分布                                        */
    /* ------------------------------------------------------------------------------------------  */
    /* beacon/probe rsp body  |     帧体后面附加字段(mac_scanned_result_extend_info_stru)          */
    /* -----------------------------------------------------------------------------------------   */
    /* 收到的beacon/rsp的body | rssi(4字节) | channel num(1字节)| band(1字节)|bss_tye(1字节)|填充  */
    /* ------------------------------------------------------------------------------------------  */
    /*                                                                                             */
    /***********************************************************************************************/

    /* 管理帧的长度等于上报的netbuf的长度减去上报的扫描结果的扩展字段的长度 */
    us_mgmt_len = pst_dtx_event->us_frame_len - OAL_SIZEOF(mac_scanned_result_extend_info_stru);

    /* 申请存储扫描结果的内存 */
    pst_new_scanned_bss = hmac_scan_alloc_scanned_bss(us_mgmt_len);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_new_scanned_bss))
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss_etc::alloc memory failed for storing scanned result.}");

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);
        return OAL_PTR_NULL;
    }

    /* 更新描述扫描结果的bss dscr结构体 */
    ul_ret = hmac_scan_update_bss_dscr(pst_new_scanned_bss, pst_dtx_event, pst_event_hdr->uc_device_id, pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss_etc::hmac_scan_update_bss_dscr failed[%d].}", ul_ret);

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);

        /* 释放申请的存储bss信息的内存 */
        oal_free(pst_new_scanned_bss);
        return ul_ret;
    }

    /* 获取管理扫描的bss结果的结构体 */
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);
    /* 对链表删操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));
    /* 判断相同bssid的bss是否已经扫描到 */
    pst_old_scanned_bss = hmac_scan_find_scanned_bss_by_bssid_etc(pst_bss_mgmt, pst_new_scanned_bss->st_bss_dscr_info.auc_bssid);
    if (OAL_PTR_NULL == pst_old_scanned_bss)
    {
        /* 解锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));

        /*lint -e801*/
        goto add_bss;
        /*lint +e801*/
    }

#ifdef _PRE_WLAN_FEATURE_M2S
    /* 只有probe rsp帧中ext cap 宣称支持OPMODE时，对端才确实支持OPMODE，beacon帧和assoc rsp帧中信息不可信 */
    pst_frame_header = (mac_ieee80211_frame_stru *)pst_new_scanned_bss->st_bss_dscr_info.auc_mgmt_buff;
    if(WLAN_PROBE_RSP == pst_frame_header->st_frame_control.bit_sub_type)
    {
        pst_old_scanned_bss->st_bss_dscr_info.en_support_opmode = pst_new_scanned_bss->st_bss_dscr_info.en_support_opmode;
    }
#endif

    /* 如果老的扫描的bss的信号强度大于当前扫描到的bss的信号强度，更新当前扫描到的信号强度为最强的信号强度 */
    if (pst_old_scanned_bss->st_bss_dscr_info.c_rssi > pst_new_scanned_bss->st_bss_dscr_info.c_rssi)
    {
        /* 1s中以内就采用之前的BSS保存的RSSI信息，否则就采用新的RSSI信息 */
        ul_curr_time_stamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        if ((ul_curr_time_stamp - pst_old_scanned_bss->st_bss_dscr_info.ul_timestamp) < HMAC_SCAN_MAX_SCANNED_RSSI_EXPIRE)
        {
            OAM_INFO_LOG0(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scanned_bss_etc::update signal strength value.}");
            pst_new_scanned_bss->st_bss_dscr_info.c_rssi = pst_old_scanned_bss->st_bss_dscr_info.c_rssi;
        }
    }

    if (OAL_TRUE == hmac_scan_is_hidden_ssid(uc_vap_id, pst_new_scanned_bss, pst_old_scanned_bss))
    {
        /*解锁*/
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));

        /* 释放申请的存储bss信息的内存 */
        oal_free(pst_new_scanned_bss);

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);

        return OAL_SUCC;
    }

    if (OAL_FALSE == hmac_scan_need_update_old_scan_result_etc(uc_vap_id, pst_new_scanned_bss, pst_old_scanned_bss)
            || OAL_FALSE == hmac_scan_check_chan(pst_bss_mgmt_netbuf, pst_new_scanned_bss))
    {
        pst_old_scanned_bss->st_bss_dscr_info.ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        pst_old_scanned_bss->st_bss_dscr_info.c_rssi = pst_new_scanned_bss->st_bss_dscr_info.c_rssi;

        /* 解锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));

        /* 释放申请的存储bss信息的内存 */
        oal_free(pst_new_scanned_bss);

        /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
        oal_netbuf_free(pst_bss_mgmt_netbuf);

        return OAL_SUCC;
    }

    /* 从链表中将原先扫描到的相同bssid的bss节点删除 */
    hmac_scan_del_bss_from_list_nolock(pst_old_scanned_bss, pst_hmac_device);
    /* 解锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));
    /* 释放内存 */
    oal_free(pst_old_scanned_bss);


add_bss:
    /* 将扫描结果添加到链表中 */
    hmac_scan_add_bss_to_list(pst_new_scanned_bss, pst_hmac_device);
    /* 释放上报的bss信息和beacon或者probe rsp帧的内存 */
    oal_netbuf_free(pst_bss_mgmt_netbuf);


    return OAL_SUCC;
}


OAL_STATIC oal_void  hmac_scan_print_channel_statistics_info(hmac_scan_record_stru   *pst_scan_record)
{
    wlan_scan_chan_stats_stru    *pst_chan_stats = pst_scan_record->ast_chan_results;
    oal_uint8                    uc_vap_id = pst_scan_record->uc_vap_id;
    oal_uint8                    uc_idx = 0;

    /* 检测本次扫描是否开启了信道测量，如果没有直接返回 */
    if (0 == pst_chan_stats[0].uc_stats_valid)
    {
        OAM_INFO_LOG0(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info:: curr scan don't enable channel measure.\n}");
        return;
    }

    /* 打印信道测量结果 */
    OAM_INFO_LOG0(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info:: The chan measure result: \n}");

    for (uc_idx = 0; uc_idx < pst_scan_record->uc_chan_numbers; uc_idx++)
    {
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Chan num      : %d\n}", pst_chan_stats[uc_idx].uc_channel_number);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Stats cnt     : %d\n}", pst_chan_stats[uc_idx].uc_stats_cnt);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Stats valid   : %d\n}", pst_chan_stats[uc_idx].uc_stats_valid);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Stats time us : %d\n}", pst_chan_stats[uc_idx].ul_total_stats_time_us);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Free time 20M : %d\n}", pst_chan_stats[uc_idx].ul_total_free_time_20M_us);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Free time 40M : %d\n}", pst_chan_stats[uc_idx].ul_total_free_time_40M_us);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Free time 80M : %d\n}", pst_chan_stats[uc_idx].ul_total_free_time_80M_us);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Send time     : %d\n}", pst_chan_stats[uc_idx].ul_total_send_time_us);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Recv time     : %d\n}", pst_chan_stats[uc_idx].ul_total_recv_time_us);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Free power cnt: %d\n}", pst_chan_stats[uc_idx].uc_free_power_cnt);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Free power 20M: %d\n}", (oal_int32)pst_chan_stats[uc_idx].s_free_power_stats_20M);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Free power 40M: %d\n}", (oal_int32)pst_chan_stats[uc_idx].s_free_power_stats_40M);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Free power 80M: %d\n}", (oal_int32)pst_chan_stats[uc_idx].s_free_power_stats_80M);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Radar detected: %d\n}", pst_chan_stats[uc_idx].uc_radar_detected);
        OAM_INFO_LOG1(uc_vap_id, OAM_SF_SCAN, "{hmac_scan_print_channel_statistics_info::Radar bw      : %d\n}", pst_chan_stats[uc_idx].uc_radar_bw);
    }

    return;
}


OAL_STATIC oal_void  hmac_scan_print_scan_record_info(hmac_vap_stru *pst_hmac_vap, hmac_scan_record_stru *pst_scan_record)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (defined(_PRE_PRODUCT_ID_HI110X_HOST))
    oal_time_t_stru   st_timestamp_diff;

    /* 获取扫描间隔时间戳 */
    st_timestamp_diff  = oal_ktime_sub(oal_ktime_get(), pst_scan_record->st_scan_start_time);

    /* 调用内核接口，打印此次扫描耗时 */
    OAM_WARNING_LOG4(pst_scan_record->uc_vap_id, OAM_SF_SCAN,
                     "{hmac_scan_print_scan_record_info::scan comp, scan_status[%d],vap ch_num:%d, cookie[%x], duration time is: [%lu]ms.}",
                     pst_scan_record->en_scan_rsp_status,
                     pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_number,
                     pst_scan_record->ull_cookie,
                     ktime_to_ms(st_timestamp_diff));
#endif

    /* 打印扫描到的bss信息 */
    hmac_scan_print_scanned_bss_info_etc(pst_scan_record->uc_device_id);

    /* 信道测量结果 */
    hmac_scan_print_channel_statistics_info(pst_scan_record);

    return;
}


oal_uint32  hmac_scan_proc_scan_comp_event_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    frw_event_hdr_stru                 *pst_event_hdr;
    hmac_vap_stru                      *pst_hmac_vap;
    hmac_device_stru                   *pst_hmac_device;
    mac_scan_rsp_stru                  *pst_d2h_scan_rsp_info;
    hmac_scan_stru                     *pst_scan_mgmt;
    oal_bool_enum_uint8                 uc_scan_abort_sync_state = OAL_FALSE;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_comp_event_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event     = frw_get_event_stru(pst_event_mem);
    pst_event_hdr = &(pst_event->st_event_hdr);

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_event_hdr->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_comp_event_etc::pst_hmac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_d2h_scan_rsp_info = (mac_scan_rsp_stru *)(pst_event->auc_event_data);
    pst_scan_mgmt = &(pst_hmac_device->st_scan_mgmt);

    if ((pst_event_hdr->uc_vap_id != pst_scan_mgmt->st_scan_record_mgmt.uc_vap_id) ||
        (pst_d2h_scan_rsp_info->ull_cookie != pst_scan_mgmt->st_scan_record_mgmt.ull_cookie))
    {
        OAM_WARNING_LOG4(pst_event_hdr->uc_vap_id, OAM_SF_SCAN,
                "{hmac_scan_proc_scan_comp_event_etc::Report vap(%d) Scan_complete(cookie %d), but there have anoter vap(%d) scaning(cookie %d) !}",
                pst_event_hdr->uc_vap_id,
                pst_d2h_scan_rsp_info->ull_cookie,
                pst_scan_mgmt->st_scan_record_mgmt.uc_vap_id,
                pst_scan_mgmt->st_scan_record_mgmt.ull_cookie);
        return OAL_SUCC;
    }

    OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_comp_event_etc::scan status:%d !}",
                     pst_d2h_scan_rsp_info->en_scan_rsp_status);

    /* 删除扫描超时保护定时器 */
    if ((OAL_TRUE == pst_scan_mgmt->st_scan_timeout.en_is_registerd)
        && (MAC_SCAN_PNO != pst_d2h_scan_rsp_info->en_scan_rsp_status))
    {
        /* PNO没有启动扫描定时器,考虑到取消PNO扫描,立即下发普通扫描,PNO扫描结束事件对随后的普通扫描的影响 */
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_scan_mgmt->st_scan_timeout));
    }

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_comp_event_etc::pst_hmac_vap null.}");

        /* 设置当前处于非扫描状态 */
        pst_scan_mgmt->en_is_scanning = OAL_FALSE;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 根据当前扫描的类型和当前vap的状态，决定切换vap的状态，如果是前景扫描，才需要切换vap的状态 */
    if ((WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode)
        && (MAC_SCAN_PNO != pst_d2h_scan_rsp_info->en_scan_rsp_status))
    {
        if (MAC_VAP_STATE_STA_WAIT_SCAN == pst_hmac_vap->st_vap_base_info.en_vap_state)
        {
            /* 改变vap状态到SCAN_COMP */
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_SCAN_COMP);
        }
    }

    if ((WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode)
        && (MAC_SCAN_ABORT_SYNC != pst_d2h_scan_rsp_info->en_scan_rsp_status))
    {
        uc_scan_abort_sync_state = OAL_TRUE;
    }

    if ((WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode)
        && (MAC_VAP_STATE_BUTT != pst_scan_mgmt->st_scan_record_mgmt.en_vap_last_state))
    {
        hmac_fsm_change_state_etc(pst_hmac_vap, pst_scan_mgmt->st_scan_record_mgmt.en_vap_last_state);
        pst_scan_mgmt->st_scan_record_mgmt.en_vap_last_state = MAC_VAP_STATE_BUTT;
    }

    /* 根据device上报的扫描结果，上报sme */
    /* 将扫描执行情况(扫描执行成功、还是失败等返回结果)记录到扫描运行记录结构体中 */
    pst_scan_mgmt->st_scan_record_mgmt.en_scan_rsp_status = pst_d2h_scan_rsp_info->en_scan_rsp_status;
    pst_scan_mgmt->st_scan_record_mgmt.ull_cookie         = pst_d2h_scan_rsp_info->ull_cookie;

    // 扫描开始时已经清理过
#if 0
    /* 上报扫描结果前，清除下到期的扫描bss，防止上报过多到期的bss */
    hmac_scan_clean_expire_scanned_bss(pst_hmac_vap, &(pst_scan_mgmt->st_scan_record_mgmt));
#endif

    hmac_scan_print_scan_record_info(pst_hmac_vap, &(pst_scan_mgmt->st_scan_record_mgmt));

    if (OAL_PTR_NULL != pst_scan_mgmt->st_scan_record_mgmt.p_fn_cb)
    {
        /* 终止扫描无需调用回调,防止终止扫描结束对随后发起PNO扫描的影响 */
        pst_scan_mgmt->st_scan_record_mgmt.p_fn_cb(&(pst_scan_mgmt->st_scan_record_mgmt));
    }
    else
    {
        if(OAL_TRUE == uc_scan_abort_sync_state)
        {
            hmac_cfg80211_scan_comp(&(pst_scan_mgmt->st_scan_record_mgmt));
        }
    }

    /* 设置当前处于非扫描状态 */
    if(MAC_SCAN_PNO != pst_d2h_scan_rsp_info->en_scan_rsp_status)
    {
        /* PNO扫描没有置此位为OAL_TRUE,PNO扫描结束,不能影响随后的常规扫描 */
        pst_scan_mgmt->en_is_scanning = OAL_FALSE;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    if (MAC_VAP_STATE_STA_LISTEN == pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
        hmac_p2p_listen_timeout_etc(pst_hmac_vap, &pst_hmac_vap->st_vap_base_info);
    }

    if (OAL_TRUE == pst_hmac_vap->en_wait_roc_end)
    {
        OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_comp_event_etc::scan rsp status[%d]}", pst_d2h_scan_rsp_info->en_scan_rsp_status);
        OAL_COMPLETE(&(pst_hmac_vap->st_roc_end_ready));
        pst_hmac_vap->en_wait_roc_end = OAL_FALSE;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
    /* STA背景扫描时，需要提前识别漫游场景 */
    if ((WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode) &&
        (MAC_VAP_STATE_UP == pst_hmac_vap->st_vap_base_info.en_vap_state))
    {
        hmac_roam_check_bkscan_result_etc(pst_hmac_vap, &(pst_scan_mgmt->st_scan_record_mgmt));
    }
#endif //_PRE_WLAN_FEATURE_ROAM

    return OAL_SUCC;
}

oal_uint32  hmac_scan_proc_scan_req_event_exception_etc(hmac_vap_stru *pst_hmac_vap, oal_void *p_params)
{
    frw_event_mem_stru         *pst_event_mem;
    frw_event_stru             *pst_event;
    hmac_scan_rsp_stru          st_scan_rsp;
    hmac_scan_rsp_stru         *pst_scan_rsp;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == p_params)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event_exception_etc::param null, %d %d.}", pst_hmac_vap, p_params);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 不支持发起扫描的状态发起了扫描 */
    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event_exception_etc::vap state is=%x.}",
                     pst_hmac_vap->st_vap_base_info.en_vap_state);

    OAL_MEMZERO(&st_scan_rsp, OAL_SIZEOF(hmac_scan_rsp_stru));

    /* 抛扫描完成事件到WAL, 执行SCAN_DONE , 释放扫描请求内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_scan_rsp_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event_exception_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_scan_rsp.uc_result_code = MAC_SCAN_REFUSED;
#ifdef _PRE_WLAN_FEATURE_ROAM
    /* When STA is roaming, scan req return success instead of failure,
       in case roaming failure which will cause UI scan list null  */
    if (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_ROAMING)
    {
        st_scan_rsp.uc_result_code = MAC_SCAN_SUCCESS;
    }
#endif
    st_scan_rsp.uc_num_dscr    = 0;

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_SCAN_COMP_STA,
                       OAL_SIZEOF(hmac_scan_rsp_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    pst_scan_rsp = (hmac_scan_rsp_stru *)pst_event->auc_event_data;

    oal_memcopy(pst_scan_rsp, (oal_void *)(&st_scan_rsp), OAL_SIZEOF(hmac_scan_rsp_stru));

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}



oal_void  hmac_scan_set_sour_mac_addr_in_probe_req_etc(hmac_vap_stru        *pst_hmac_vap,
                                                   oal_uint8            *puc_sour_mac_addr,
                                                   oal_bool_enum_uint8   en_is_rand_mac_addr_scan,
                                                   oal_bool_enum_uint8   en_is_p2p0_scan)
{
    mac_device_stru          *pst_mac_device = OAL_PTR_NULL;
    hmac_device_stru         *pst_hmac_device = OAL_PTR_NULL;

    if((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == puc_sour_mac_addr))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_scan_set_sour_mac_addr_in_probe_req_etc::param null,pst_hmac_vap:%p,puc_sour_mac_addr:%p.}",
                       pst_hmac_vap, puc_sour_mac_addr);
        return;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_scan_set_sour_mac_addr_in_probe_req_etc::pst_mac_device is null.}");
        return;
    }

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_scan_set_sour_mac_addr_in_probe_req_etc::pst_hmac_device is null. device_id %d}",
                        pst_hmac_vap->st_vap_base_info.uc_device_id);
        return;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    /* WLAN/P2P 特性情况下，p2p0 和p2p-p2p0 cl 扫描时候，需要使用不同设备 */
    if (OAL_TRUE == en_is_p2p0_scan)
    {
        oal_set_mac_addr(puc_sour_mac_addr, mac_mib_get_p2p0_dot11StationID(&pst_hmac_vap->st_vap_base_info));
    }
    else
#endif /* _PRE_WLAN_FEATURE_P2P */
    {
        /* 如果随机mac addr扫描特性开启且非P2P场景，设置随机mac addr到probe req帧中 */
        if ((OAL_TRUE == en_is_rand_mac_addr_scan) && (IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info)))
            && ((pst_mac_device->auc_mac_oui[0] != 0) || (pst_mac_device->auc_mac_oui[1] != 0) || (pst_mac_device->auc_mac_oui[2] != 0)))
        {
            /* 更新随机mac 地址,使用下发随机MAC OUI 生成的随机mac 地址更新到本次扫描 */
            oal_set_mac_addr(puc_sour_mac_addr, pst_hmac_device->st_scan_mgmt.auc_random_mac);
        }
        else
        {
            /* 设置地址为自己的MAC地址 */
            oal_set_mac_addr(puc_sour_mac_addr, mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info));
        }
    }

    return;
}

OAL_STATIC oal_bool_enum_uint8 hmac_scan_need_skip_channel(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_channel)
{
    wlan_channel_band_enum_uint8    en_band = mac_get_band_by_channel_num(uc_channel);

    if (OAL_TRUE != mac_chip_run_band(pst_hmac_vap->st_vap_base_info.uc_chip_id, en_band))
    {
        return OAL_TRUE;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_TEST_MODE_UT != _PRE_TEST_MODE) && defined(_PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND)
    return  ((pst_hmac_vap->en_restrict_band == WLAN_BAND_BUTT)
          || (pst_hmac_vap->en_restrict_band == en_band)) ? OAL_FALSE : OAL_TRUE;
#else
    return OAL_FALSE;
#endif
}

OAL_STATIC oal_uint32  hmac_scan_update_scan_params(hmac_vap_stru        *pst_hmac_vap,
                                                    mac_scan_req_stru    *pst_scan_params,
                                                    oal_bool_enum_uint8   en_is_random_mac_addr_scan)
{
    mac_device_stru             *pst_mac_device;
    mac_vap_stru                *pst_mac_vap_temp;
    oal_uint32                   ul_ret;
    wlan_vap_mode_enum_uint8     en_vap_mode;
    oal_uint8                    uc_loop;
    oal_uint8                    uc_chan_cnt = 0;

#ifdef _PRE_WLAN_FEATURE_HILINK
    if (WLAN_SCAN_MODE_BACKGROUND_HILINK == pst_scan_params->en_scan_mode)
    {
        return OAL_SUCC;
    }
#endif

    /* 获取mac device */
    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_device_id, OAM_SF_SCAN,
                         "{hmac_scan_update_scan_params::pst_mac_device null.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }
    /* 1.记录发起扫描的vap id到扫描参数 */
    pst_scan_params->uc_vap_id    = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    pst_scan_params->en_need_switch_back_home_channel = OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_ROAM
    if (WLAN_SCAN_MODE_ROAM_SCAN != pst_scan_params->en_scan_mode)
#endif
    {
        pst_scan_params->en_scan_mode = WLAN_SCAN_MODE_FOREGROUND;
    }

    /* 2.修改扫描模式和信道扫描次数: 根据是否存在up状态下的vap，如果是，则是背景扫描，如果不是，则是前景扫描 */
    ul_ret = mac_device_find_up_vap_etc(pst_mac_device, &pst_mac_vap_temp);
    if ((OAL_SUCC == ul_ret) && (OAL_PTR_NULL != pst_mac_vap_temp))
    {
        /* 判断vap的类型，如果是sta则为sta的背景扫描，如果是ap，则是ap的背景扫描，其它类型的vap暂不支持背景扫描 */
        en_vap_mode = pst_hmac_vap->st_vap_base_info.en_vap_mode;
        if (WLAN_VAP_MODE_BSS_STA == en_vap_mode)
        {
        #ifdef _PRE_WLAN_FEATURE_ROAM
            if (WLAN_SCAN_MODE_ROAM_SCAN != pst_scan_params->en_scan_mode)
        #endif
            {
                /* 修改扫描参数为sta的背景扫描 */
                pst_scan_params->en_scan_mode = WLAN_SCAN_MODE_BACKGROUND_STA;
            }
        }
        else if (WLAN_VAP_MODE_BSS_AP == en_vap_mode)
        {
            /* 修改扫描参数为sta的背景扫描 */
            pst_scan_params->en_scan_mode = WLAN_SCAN_MODE_BACKGROUND_AP;
        }
        else
        {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_scan_params::vap mode[%d], not support bg scan.}", en_vap_mode);
            return OAL_FAIL;
        }
        pst_scan_params->en_need_switch_back_home_channel = OAL_TRUE;

        if (1 == mac_device_calc_up_vap_num_etc(pst_mac_device) && !IS_LEGACY_VAP(pst_mac_vap_temp) && IS_LEGACY_VAP(&pst_hmac_vap->st_vap_base_info))
        {
            /* 修改扫描信道间隔(2)和回工作信道工作时间(60ms):仅仅针对P2P处于关联状态，wlan处于去关联状态,wlan发起的扫描 */
            pst_scan_params->uc_scan_channel_interval       = MAC_SCAN_CHANNEL_INTERVAL_PERFORMANCE;
            pst_scan_params->us_work_time_on_home_channel   = MAC_WORK_TIME_ON_HOME_CHANNEL_PERFORMANCE;

            if((pst_scan_params->us_scan_time > WLAN_DEFAULT_ACTIVE_SCAN_TIME)
               && (WLAN_SCAN_TYPE_ACTIVE == pst_scan_params->en_scan_type))
            {
                /* 指定SSID扫描超过3个,会修改每次扫描时间为40ms(默认是20ms) */
                /* P2P关联但wlan未关联场景,考虑到扫描时间增加对p2p wfd场景的影响,设置每信道扫描次数为1次(默认为2次) */
                pst_scan_params->uc_max_scan_count_per_channel = 1;
            }
        }
        else
        {
            /* 携带隐藏SSID的情况下扫3个信道回一次home信道，其他情况默认扫描6个信道回home信道工作100ms */
            pst_scan_params->uc_scan_channel_interval       = (pst_scan_params->uc_ssid_num > 1) ? MAC_SCAN_CHANNEL_INTERVAL_HIDDEN_SSID : MAC_SCAN_CHANNEL_INTERVAL_DEFAULT;
            pst_scan_params->us_work_time_on_home_channel   = MAC_WORK_TIME_ON_HOME_CHANNEL_DEFAULT;
        }
    }
    /* 3.设置发送的probe req帧中源mac addr */
    pst_scan_params->en_is_random_mac_addr_scan = en_is_random_mac_addr_scan;
    hmac_scan_set_sour_mac_addr_in_probe_req_etc(pst_hmac_vap, pst_scan_params->auc_sour_mac_addr,
                                             en_is_random_mac_addr_scan, pst_scan_params->bit_is_p2p0_scan);

    /* do foreground scan and switch back while requested by main proxysta */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_vap_is_msta(&pst_hmac_vap->st_vap_base_info)
        && (pst_hmac_vap->st_vap_base_info.en_vap_state != MAC_VAP_STATE_UP))
    {
        pst_scan_params->en_need_switch_back_home_channel = OAL_TRUE;
        pst_scan_params->en_scan_mode = WLAN_SCAN_MODE_FOREGROUND;
    }
#endif

    for (uc_loop = 0; uc_loop < pst_scan_params->uc_channel_nums; uc_loop++)
    {
        if (!hmac_scan_need_skip_channel(pst_hmac_vap, pst_scan_params->ast_channel_list[uc_loop].uc_chan_number))
        {
            if (uc_chan_cnt != uc_loop)
            {
                pst_scan_params->ast_channel_list[uc_chan_cnt] = pst_scan_params->ast_channel_list[uc_loop];
            }
            uc_chan_cnt++;
        }
    }

    if (!uc_chan_cnt)
    {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_scan_update_scan_params::channels trimed to none!, ori cnt=%d}", pst_scan_params->uc_channel_nums);

        return OAL_FAIL;
    }

    pst_scan_params->uc_channel_nums = uc_chan_cnt;
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_scan_check_can_enter_scan_state(mac_vap_stru    *pst_mac_vap)
{
    /* p2p有可能进行监听动作，但是和scan req的优先级一样，因此当上层发起的是扫描请求时，统一可使用下面的接口判断 */
    return hmac_p2p_check_can_enter_state_etc(pst_mac_vap, HMAC_FSM_INPUT_SCAN_REQ);
}


OAL_STATIC oal_uint32  hmac_scan_check_is_dispatch_scan_req(hmac_vap_stru       *pst_hmac_vap,
                                                            hmac_device_stru    *pst_hmac_device)
{
    oal_uint32  ul_ret;

    /* 1.先检测其它vap的状态从而判断是否可进入扫描状态，使得扫描尽量不打断其它的入网流程 */
    ul_ret = hmac_scan_check_can_enter_scan_state(&(pst_hmac_vap->st_vap_base_info));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_scan_check_is_dispatch_scan_req::Because of err_code[%d], can't enter into scan state.}", ul_ret);
        return ul_ret;
    }

    /* 2.判断当前扫描是否正在执行 */
    if (OAL_TRUE == pst_hmac_device->st_scan_mgmt.en_is_scanning)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_check_is_dispatch_scan_req::the scan request is rejected.}");
        return OAL_FAIL;
    }

#ifdef _PRE_WLAN_FEATURE_ROAM
    /* 3.判断当前是否正在执行漫游 */
    if (MAC_VAP_STATE_ROAMING == pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_check_is_dispatch_scan_req:: roam reject new scan.}");
        return OAL_FAIL;
    }
#endif //_PRE_WLAN_FEATURE_ROAM

    return OAL_SUCC;
}


OAL_STATIC oal_void  hmac_scan_proc_last_scan_record(hmac_vap_stru       *pst_hmac_vap,
                                                              hmac_device_stru    *pst_hmac_device)
{
    /* 如果是proxysta发起的扫描无需清楚扫描结果 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_is_proxysta_enabled(pst_hmac_device->pst_device_base_info))
    {
        if (mac_vap_is_vsta(&pst_hmac_vap->st_vap_base_info))
        {
            /* proxysta do nothing */
            OAM_INFO_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event_etc:: proxysta don't need clean scan record.}");
            return;
        }
    }
#endif

    OAM_INFO_LOG0(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event_etc:: start clean last scan record.}");

#if 0
    /* 清空上一次扫描记录信息 */
    hmac_scan_clean_scan(&(pst_hmac_device->st_scan_mgmt));
#else
    /* 本次扫描请求发起时，清除上一次扫描结果中过期的bss信息 */
    hmac_scan_clean_expire_scanned_bss(pst_hmac_vap, &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt));
#endif

    return;
}



OAL_STATIC oal_uint32  hmac_scan_proc_scan_timeout_fn(void *p_arg)
{
    hmac_device_stru                   *pst_hmac_device = (hmac_device_stru *)p_arg;
    hmac_vap_stru                      *pst_hmac_vap    = OAL_PTR_NULL;
    hmac_scan_record_stru              *pst_scan_record = OAL_PTR_NULL;
    oal_uint32                          ul_pedding_data = 0;

    /* 获取扫描记录信息 */
    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(pst_scan_record->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_timeout_fn::pst_hmac_vap null.}");

        /* 扫描状态恢复为未在执行的状态 */
        pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_FALSE;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 根据当前扫描的类型和当前vap的状态，决定切换vap的状态，如果是前景扫描，才需要切换vap的状态 */
    if (WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        if (MAC_VAP_STATE_STA_WAIT_SCAN == pst_hmac_vap->st_vap_base_info.en_vap_state)
        {
            /* 改变vap状态到SCAN_COMP */
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_SCAN_COMP);
        }
    }

    if ((WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode)
        && (MAC_VAP_STATE_BUTT != pst_scan_record->en_vap_last_state))
    {
        hmac_fsm_change_state_etc(pst_hmac_vap, pst_scan_record->en_vap_last_state);
        pst_scan_record->en_vap_last_state = MAC_VAP_STATE_BUTT;
    }

    /* 设置扫描响应状态为超时 */
    pst_scan_record->en_scan_rsp_status = MAC_SCAN_TIMEOUT;
    OAM_WARNING_LOG1(pst_scan_record->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_timeout_fn::scan time out cookie [%x].}", pst_scan_record->ull_cookie);

    /* 如果扫描回调函数不为空，则调用回调函数 */
    if (OAL_PTR_NULL != pst_scan_record->p_fn_cb)
    {
        OAM_WARNING_LOG0(pst_scan_record->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_timeout_fn::scan callback func proc.}");
        pst_scan_record->p_fn_cb(pst_scan_record);
    }

    /* DMAC 超时未上报扫描完成，HMAC 下发扫描结束命令，停止DMAC 扫描 */
    hmac_config_scan_abort_etc(&pst_hmac_vap->st_vap_base_info, OAL_SIZEOF(oal_uint32), (oal_uint8 *)&ul_pedding_data);

    /* 扫描状态恢复为未在执行的状态 */
    pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_FALSE;

    CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_SCAN,CHR_WIFI_DRV_ERROR_SCAN_TIMEOUT));

    return OAL_SUCC;

}


oal_uint32  hmac_scan_proc_scan_req_event_etc(hmac_vap_stru *pst_hmac_vap, oal_void *p_params)
{
    frw_event_mem_stru         *pst_event_mem;
    frw_event_stru             *pst_event;
    mac_scan_req_stru          *pst_h2d_scan_req_params;     /* hmac发送到dmac的扫描请求参数 */
    mac_scan_req_stru          *pst_scan_params;
    hmac_device_stru           *pst_hmac_device;
    hmac_scan_record_stru      *pst_scan_record;
    oal_uint32                  ul_scan_timeout;
    oal_uint32                  ul_ret;

    /* 参数合法性检查 */
    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == p_params)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event_etc::param null, %p %p.}", pst_hmac_vap, p_params);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 扫描停止模块测试 */
    if (((HMAC_BGSCAN_DISABLE == g_en_bgscan_enable_flag_etc) && (MAC_VAP_STATE_UP == pst_hmac_vap->st_vap_base_info.en_vap_state))
        || (HMAC_SCAN_DISABLE == g_en_bgscan_enable_flag_etc))
    {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "hmac_scan_proc_scan_req_event_etc: g_en_bgscan_enable_flag_etc= %d.", g_en_bgscan_enable_flag_etc);
        return OAL_FAIL;
    }

    pst_scan_params = (mac_scan_req_stru *)p_params;

    /* 异常判断: 扫描的信道个数为0 */
    if (0 == pst_scan_params->uc_channel_nums)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event_etc::channel_nums=0.}");
        return OAL_FAIL;
    }

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_scan_req_event_etc::pst_hmac_device[%d] null.}",
                        pst_hmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 更新此次扫描请求的扫描参数 */
    if (pst_scan_params->uc_scan_func == MAC_SCAN_FUNC_P2P_LISTEN)
    {
        ul_ret = hmac_scan_update_scan_params(pst_hmac_vap, pst_scan_params, OAL_FALSE);
    }
    else
    {
        /* 更新此次扫描请求的扫描参数 */
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
        ul_ret = hmac_scan_update_scan_params(pst_hmac_vap, pst_scan_params, g_st_wlan_customize_etc.uc_random_mac_addr_scan);
#else
        ul_ret = hmac_scan_update_scan_params(pst_hmac_vap, pst_scan_params, pst_hmac_device->st_scan_mgmt.en_is_random_mac_addr_scan);
#endif
    }


    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_scan_proc_scan_req_event_etc::update scan mode failed, error[%d].}", ul_ret);
        return ul_ret;
    }

    /* 检测是否符合发起扫描请求的条件，如果不符合，直接返回 */
    ul_ret = hmac_scan_check_is_dispatch_scan_req(pst_hmac_vap, pst_hmac_device);
    if (OAL_SUCC != ul_ret)
    {
        if (MAC_SCAN_FUNC_P2P_LISTEN == pst_scan_params->uc_scan_func)
        {
            mac_vap_state_change_etc(&pst_hmac_vap->st_vap_base_info, pst_hmac_device->pst_device_base_info->st_p2p_info.en_last_vap_state);
        }

        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_scan_proc_scan_req_event_etc::Because of error[%d], can't dispatch scan req.}", ul_ret);
        return ul_ret;
    }

    /* 设置扫描模块处于扫描状态，其它扫描请求将丢弃 */
    pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_TRUE;

    /* 处理上一次扫描记录，目前直接清楚上一次结果，后续可能需要老化时间处理 */
    hmac_scan_proc_last_scan_record(pst_hmac_vap, pst_hmac_device);

    /* 记录扫描发起者的信息，某些模块回调函数使用 */
    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);
    pst_scan_record->uc_chip_id      = pst_hmac_device->pst_device_base_info->uc_chip_id;
    pst_scan_record->uc_device_id    = pst_hmac_device->pst_device_base_info->uc_device_id;
    pst_scan_record->uc_vap_id       = pst_scan_params->uc_vap_id;
    pst_scan_record->uc_chan_numbers = pst_scan_params->uc_channel_nums;
    pst_scan_record->p_fn_cb         = pst_scan_params->p_fn_cb;

    if (WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_scan_proc_scan_req_event_etc::save last en_vap_state:%d}", pst_hmac_vap->st_vap_base_info.en_vap_state);

        pst_scan_record->en_vap_last_state = pst_hmac_vap->st_vap_base_info.en_vap_state;
    }

    pst_scan_record->ull_cookie      = pst_scan_params->ull_cookie;

    /* 记录扫描开始时间 */
    pst_scan_record->st_scan_start_time = oal_ktime_get();

    /* 抛扫描请求事件到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(mac_scan_req_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{hmac_scan_proc_scan_req_event_etc::alloc memory(%u) failed.}", OAL_SIZEOF(mac_scan_req_stru));

        /* 恢复扫描状态为非运行状态 */
        pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_FALSE;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果发起扫描的vap的模式为sta，并且，其关联状态为非up状态，且非p2p监听状态，则切换其扫描状态 */
    if ((WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode) &&
        (MAC_SCAN_FUNC_P2P_LISTEN != pst_scan_params->uc_scan_func))
    {
        if (MAC_VAP_STATE_UP != pst_hmac_vap->st_vap_base_info.en_vap_state)
        {
            /* 切换vap的状态为WAIT_SCAN状态 */
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_WAIT_SCAN);
        }
    }

    /* AP的启动扫描做特殊处理，当hostapd下发扫描请求时，VAP还处于INIT状态 */
    if ( (WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode) &&
         (MAC_VAP_STATE_INIT == pst_hmac_vap->st_vap_base_info.en_vap_state) )
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
            "{hmac_scan_proc_scan_req_event_etc::ap startup scan.}");
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);
    }

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCAN_REQ,
                       OAL_SIZEOF(mac_scan_req_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /* 直接传内容，屏蔽多产品的差异 */
    pst_h2d_scan_req_params = (mac_scan_req_stru *)(pst_event->auc_event_data);

    /* 拷贝扫描请求参数到事件data区域 */
    oal_memcopy(pst_h2d_scan_req_params, pst_scan_params, OAL_SIZEOF(mac_scan_req_stru));

    /* 打印扫描参数，测试使用 */
    /* 如果是P2P 发起监听，则设置HMAC 扫描超时时间为P2P 监听时间 */
    if (MAC_SCAN_FUNC_P2P_LISTEN == pst_scan_params->uc_scan_func)
    {
        ul_scan_timeout = pst_scan_params->us_scan_time * 2;
    }
    else
    {
        ul_scan_timeout = WLAN_DEFAULT_MAX_TIME_PER_SCAN;
    }
    hmac_scan_print_scan_params(pst_h2d_scan_req_params, &pst_hmac_vap->st_vap_base_info);

    /* 启动扫描保护定时器，防止因拋事件、核间通信失败等情况下的异常保护，定时器初步的超时时间为4.5秒 */
    FRW_TIMER_CREATE_TIMER(&(pst_hmac_device->st_scan_mgmt.st_scan_timeout),
                           hmac_scan_proc_scan_timeout_fn,
                           ul_scan_timeout,
                           pst_hmac_device,
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_hmac_device->pst_device_base_info->ul_core_id);

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32  hmac_scan_proc_sched_scan_req_event_etc(hmac_vap_stru *pst_hmac_vap, oal_void *p_params)
{
    frw_event_mem_stru         *pst_event_mem;
    frw_event_stru             *pst_event;
    hmac_device_stru           *pst_hmac_device;
    hmac_scan_record_stru      *pst_scan_record;
    mac_pno_scan_stru          *pst_pno_scan_params;
    oal_uint32                  ul_ret;

    /* 参数合法性检查 */
    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == p_params)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_scan_proc_sched_scan_req_event_etc::param null, %p %p.}", pst_hmac_vap, p_params);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_pno_scan_params = (mac_pno_scan_stru *)p_params;

    /* 判断PNO调度扫描下发的过滤的ssid个数小于等于0 */
    if (pst_pno_scan_params->l_ssid_count <= 0)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_sched_scan_req_event_etc::ssid_count <=0.}");
        return OAL_FAIL;
    }

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_sched_scan_req_event_etc::pst_hmac_device[%d] null.}",
                        pst_hmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 检测是否符合发起扫描请求的条件，如果不符合，直接返回 */
    ul_ret = hmac_scan_check_is_dispatch_scan_req(pst_hmac_vap, pst_hmac_device);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_scan_proc_sched_scan_req_event_etc::Because of error[%d], can't dispatch scan req.}", ul_ret);
        return ul_ret;
    }

    /* 清空上一次的扫描结果 */
    hmac_scan_proc_last_scan_record(pst_hmac_vap, pst_hmac_device);

    /* 记录扫描发起者的信息，某些模块回调函数使用 */
    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);
    pst_scan_record->uc_chip_id   = pst_hmac_device->pst_device_base_info->uc_chip_id;
    pst_scan_record->uc_device_id = pst_hmac_device->pst_device_base_info->uc_device_id;
    pst_scan_record->uc_vap_id    = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    pst_scan_record->p_fn_cb      = pst_pno_scan_params->p_fn_cb;

    /* 抛扫描请求事件到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(pst_pno_scan_params));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_scan_proc_sched_scan_req_event_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCHED_SCAN_REQ,
                       OAL_SIZEOF(pst_pno_scan_params),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /* 事件data域内携带PNO扫描请求参数 */
    oal_memcopy(frw_get_event_payload(pst_event_mem), (oal_uint8 *)&pst_pno_scan_params, OAL_SIZEOF(mac_pno_scan_stru *));

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32  hmac_scan_process_chan_result_event_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    frw_event_hdr_stru                 *pst_event_hdr;
    hmac_device_stru                   *pst_hmac_device;
    dmac_crx_chan_result_stru          *pst_chan_result_param;
    hmac_scan_record_stru              *pst_scan_record;
    oal_uint8                           uc_scan_idx;

    /* 获取事件信息 */
    pst_event               = frw_get_event_stru(pst_event_mem);
    pst_event_hdr           = &(pst_event->st_event_hdr);
    pst_chan_result_param   = (dmac_crx_chan_result_stru *)(pst_event->auc_event_data);
    uc_scan_idx             = pst_chan_result_param->uc_scan_idx;

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_event_hdr->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_scan_process_chan_result_event_etc::pst_hmac_device is null.}");
        return OAL_FAIL;
    }

    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);

    /* 检查上报的索引是否合法 */
    if (uc_scan_idx >= pst_scan_record->uc_chan_numbers)
    {
        /* dmac上报的扫描结果超出了要扫描的信道个数 */
        OAM_WARNING_LOG2(0, OAM_SF_SCAN,
                         "{hmac_scan_process_chan_result_event_etc::result from dmac error! scan_idx[%d], chan_numbers[%d].}",
                         uc_scan_idx, pst_scan_record->uc_chan_numbers);

        return OAL_FAIL;
    }

    pst_scan_record->ast_chan_results[uc_scan_idx] = pst_chan_result_param->st_chan_result;

    return OAL_SUCC;
}
#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN)

oal_uint32 hmac_scan_rrm_proc_save_bss_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_netbuf_stru                 *pst_action_table_bcn_rpt;
    oal_uint8                       *puc_data;
    mac_user_stru                   *pst_mac_user;
    oal_uint16                       us_index = 0;
    mac_meas_rpt_ie_stru            *pst_meas_rpt_ie;
    mac_bcn_rpt_stru                *pst_bcn_rpt;
    mac_tx_ctl_stru                 *pst_tx_ctl;
    mac_vap_rrm_trans_req_info_stru *pst_trans_req_info;
    oal_uint32                       ul_ret;
    hmac_device_stru                *pst_hmac_device;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;
    oal_dlist_head_stru             *pst_entry;
    hmac_scanned_bss_info           *pst_scanned_bss;
    oal_uint8                        uc_bss_idx = 0;
    hmac_vap_stru                   *pst_hmac_vap;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    oal_uint8                        uc_snr_ant0;
    oal_uint8                        uc_snr_ant1;
    oal_uint8                        uc_snr;
#endif
    oal_uint16                       us_len = 0;

    if (OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_scan_rrm_proc_save_bss_etc::puc_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_rrm_proc_save_bss_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_rrm_proc_save_bss_etc::pst_hmac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_user = mac_res_get_mac_user_etc(pst_mac_vap->us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "{hmac_scan_rrm_proc_save_bss_etc::pst_mac_user[%d] null.", pst_mac_vap->us_assoc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_trans_req_info = (mac_vap_rrm_trans_req_info_stru *)puc_param;

    pst_action_table_bcn_rpt = (oal_netbuf_stru *)OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if(OAL_PTR_NULL == pst_action_table_bcn_rpt)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_scan_rrm_proc_save_bss_etc::pst_action_table_bcn_rpt null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(oal_netbuf_cb(pst_action_table_bcn_rpt), OAL_NETBUF_CB_SIZE());

    puc_data = (oal_uint8 *)OAL_NETBUF_HEADER(pst_action_table_bcn_rpt);

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    if (OAL_PTR_NULL == pst_hmac_vap->pst_rrm_info)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_rrm_proc_save_bss_etc::pst_rrm_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif
    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                Set the fields in the frame header                     */
    /*************************************************************************/

    /* All the fields of the Frame Control Field are set to zero. Only the   */
    /* Type/Subtype field is set.                                            */
    mac_hdr_set_frame_control(puc_data, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* duration */
    puc_data[2] = 0;
    puc_data[3] = 0;

    /* DA is address of STA requesting association */
    oal_set_mac_addr(puc_data + 4, pst_mac_user->auc_user_mac_addr);

    /* SA is the dot11MACAddress */
    oal_set_mac_addr(puc_data + 10, mac_mib_get_StationID(pst_mac_vap));

    oal_set_mac_addr(puc_data + 16, pst_mac_vap->auc_bssid);

    /* seq control */
    puc_data[22] = 0;
    puc_data[23] = 0;

    /*************************************************************************/
    /*                    Radio Measurement Report Frame - Frame Body        */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Measurement Report Elements         |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          |  var                                 */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* Initialize index and the frame data pointer */
    us_index = MAC_80211_FRAME_LEN;

    /* Category */
    puc_data[us_index++] = MAC_ACTION_CATEGORY_RADIO_MEASURMENT;

    /* Action */
    puc_data[us_index++] = MAC_RM_ACTION_RADIO_MEASUREMENT_REPORT;

    /* Dialog Token  */
    puc_data[us_index++]  = pst_trans_req_info->uc_action_dialog_token;

    us_len = us_index;
    /* 获取管理扫描的bss结果的结构体 */
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    if ((WLAN_MEM_NETBUF_SIZE2 - us_len) < (MAC_MEASUREMENT_RPT_FIX_LEN + MAC_BEACON_RPT_FIX_LEN))
    {
        /*释放*/
        oal_netbuf_free(pst_action_table_bcn_rpt);
        return OAL_SUCC;
    }
    pst_meas_rpt_ie = (mac_meas_rpt_ie_stru *)(puc_data + us_index);

    /* 对链表删操作前加锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);

        /* BSSID过滤 */
        if (!ETHER_IS_BROADCAST(pst_trans_req_info->auc_bssid)
            && OAL_MEMCMP(pst_scanned_bss->st_bss_dscr_info.auc_bssid, pst_trans_req_info->auc_bssid, WLAN_MAC_ADDR_LEN))
        {
            continue;
        }

        /* SSID过滤，若请求中ssid长度为0，则不过滤 */
        if (0 != pst_trans_req_info->us_ssid_len
            && OAL_MEMCMP(pst_scanned_bss->st_bss_dscr_info.ac_ssid, pst_trans_req_info->auc_ssid, pst_trans_req_info->us_ssid_len))
        {
            continue;
        }

        /*************************************************************************/
        /*                   Measurement Report IE - Frame Body                  */
        /* --------------------------------------------------------------------- */
        /* |Element ID |Length |Meas Token| Meas Rpt Mode | Meas Type | Meas Rpt|*/
        /* --------------------------------------------------------------------- */
        /* |1          |1      | 1        |  1            | 1         | var      */
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/
        pst_meas_rpt_ie->uc_eid = MAC_EID_MEASREP;
        pst_meas_rpt_ie->uc_len = MAC_MEASUREMENT_RPT_FIX_LEN - MAC_IE_HDR_LEN + MAC_BEACON_RPT_FIX_LEN;
        pst_meas_rpt_ie->uc_token = pst_trans_req_info->uc_meas_token;
        OAL_MEMZERO(&(pst_meas_rpt_ie->st_rptmode), OAL_SIZEOF(mac_meas_rpt_mode_stru));
        pst_meas_rpt_ie->uc_rpttype = RM_RADIO_MEAS_BCN;

        pst_bcn_rpt = (mac_bcn_rpt_stru *)pst_meas_rpt_ie->auc_meas_rpt;
        OAL_MEMZERO(pst_bcn_rpt, OAL_SIZEOF(mac_bcn_rpt_stru));
        oal_memcopy(pst_bcn_rpt->auc_bssid, pst_scanned_bss->st_bss_dscr_info.auc_bssid, WLAN_MAC_ADDR_LEN);
        pst_bcn_rpt->uc_channum = pst_scanned_bss->st_bss_dscr_info.st_channel.uc_chan_number;
        if (0 != pst_hmac_vap->bit_11k_auth_oper_class)
        {
            pst_bcn_rpt->uc_optclass = pst_hmac_vap->bit_11k_auth_oper_class;
        }
        else
        {
            pst_bcn_rpt->uc_optclass = pst_trans_req_info->uc_oper_class;
        }

        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_scan_rrm_proc_save_bss_etc::In Channel [%d] Find BSS %02X:XX:XX:XX:%02X:%02X.}",
        pst_scanned_bss->st_bss_dscr_info.st_channel.uc_chan_number,
        pst_scanned_bss->st_bss_dscr_info.auc_bssid[0],
        pst_scanned_bss->st_bss_dscr_info.auc_bssid[4],
        pst_scanned_bss->st_bss_dscr_info.auc_bssid[5]);

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
        oal_memcopy(pst_bcn_rpt->aul_act_meas_start_time, pst_hmac_vap->pst_rrm_info->aul_act_meas_start_time,
                            OAL_SIZEOF(pst_hmac_vap->pst_rrm_info->aul_act_meas_start_time));
        //pst_bcn_rpt->us_duration = (oal_uint16)HMAC_RRM_CAL_DURATION(pst_bcn_rpt->aul_act_meas_start_time[0], pst_scanned_bss->st_bss_dscr_info.ul_parent_tsf);
        /* Meas Duration,参考商用设备, bcn report与req duration填写一致 */
        pst_bcn_rpt->us_duration = pst_trans_req_info->us_duration;

        pst_bcn_rpt->bit_condensed_phy_type    = pst_scanned_bss->st_bss_dscr_info.uc_phy_type;
        pst_bcn_rpt->bit_rpt_frm_type          = 0; /* Beacon/Probe rsp */
        pst_bcn_rpt->uc_rcpi                   = HMAC_RRM_CAL_RCPI(pst_scanned_bss->st_bss_dscr_info.c_rssi);
        pst_bcn_rpt->uc_rsni                   = (oal_uint8)((pst_scanned_bss->st_bss_dscr_info.ac_rsni[0] + pst_scanned_bss->st_bss_dscr_info.ac_rsni[1])/2);
        pst_bcn_rpt->uc_antenna_id             = 0; //unknown
        pst_bcn_rpt->ul_parent_tsf             = pst_scanned_bss->st_bss_dscr_info.ul_parent_tsf;
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
        pst_bcn_rpt->uc_rcpi  = (oal_uint8)((oal_uint32)(pst_scanned_bss->st_bss_dscr_info.c_rssi + 110) << 1);

        /* 获取信噪比  RSNI=(信噪比 + 10) * 2*/
        uc_snr_ant0 = (oal_uint8)pst_scanned_bss->st_bss_dscr_info.c_ant0_rssi;
        uc_snr_ant1 = (oal_uint8)pst_scanned_bss->st_bss_dscr_info.c_ant0_rssi;
        if(OAL_SNR_INIT_VALUE == uc_snr_ant0 && OAL_SNR_INIT_VALUE != uc_snr_ant1)
        {
            uc_snr = ((uc_snr_ant1>>1) + 10) << 1;
        }
        else if(OAL_SNR_INIT_VALUE != uc_snr_ant0 &&  OAL_SNR_INIT_VALUE == uc_snr_ant1)
        {
            uc_snr = ((uc_snr_ant0>>1) + 10) << 1;
        }
        else if(OAL_SNR_INIT_VALUE != uc_snr_ant0 &&  OAL_SNR_INIT_VALUE != uc_snr_ant1)
        {
            uc_snr = OAL_MAX(uc_snr_ant0, uc_snr_ant1);
            uc_snr = ((uc_snr>>1) + 10) << 1;
        }
        else
        {
            uc_snr = 0xFF;/*无效值为0xFF*/
        }

        pst_bcn_rpt->uc_rsni                   = uc_snr;
#endif

        us_len += (MAC_MEASUREMENT_RPT_FIX_LEN + MAC_BEACON_RPT_FIX_LEN);
        uc_bss_idx++;
        if ((WLAN_MEM_NETBUF_SIZE2 - us_len) < (MAC_MEASUREMENT_RPT_FIX_LEN + MAC_BEACON_RPT_FIX_LEN))
        {
            break;
        }

        pst_meas_rpt_ie = (mac_meas_rpt_ie_stru *)pst_bcn_rpt->auc_subelm;

    }
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_scan_rrm_proc_save_bss_etc::Find BssNum=[%d],us_len=[%d].buf_size=[%d].}",
    uc_bss_idx,us_len,WLAN_MEM_NETBUF_SIZE2);

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_action_table_bcn_rpt);
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)      = us_len;
    ul_ret = mac_vap_set_cb_tx_user_idx(pst_mac_vap, pst_tx_ctl, pst_mac_user->auc_user_mac_addr);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "(hmac_scan_rrm_proc_save_bss_etc::fail to find user by xx:xx:xx:0x:0x:0x.}",
        pst_mac_user->auc_user_mac_addr[3],
        pst_mac_user->auc_user_mac_addr[4],
        pst_mac_user->auc_user_mac_addr[5]);
    }

    if (MAC_GET_CB_MPDU_LEN(pst_tx_ctl) > WLAN_MEM_NETBUF_SIZE2)
    {
        oal_netbuf_free(pst_action_table_bcn_rpt);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_scan_rrm_proc_save_bss_etc::invalid us_len=[%d].}",
        MAC_GET_CB_MPDU_LEN(pst_tx_ctl));
        return OAL_SUCC;
    }
    oal_netbuf_put(pst_action_table_bcn_rpt, MAC_GET_CB_MPDU_LEN(pst_tx_ctl));

    ul_ret = hmac_tx_mgmt_send_event_etc(pst_mac_vap, pst_action_table_bcn_rpt, MAC_GET_CB_MPDU_LEN(pst_tx_ctl));
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_action_table_bcn_rpt);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_scan_rrm_proc_save_bss_etc::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}
#endif

oal_void  hmac_scan_init_etc(hmac_device_stru *pst_hmac_device)
{
    hmac_scan_stru      *pst_scan_mgmt;
    hmac_bss_mgmt_stru  *pst_bss_mgmt;

    /* 初始化扫描管理结构体信息 */
    pst_scan_mgmt = &(pst_hmac_device->st_scan_mgmt);
    OAL_MEMZERO(pst_scan_mgmt, OAL_SIZEOF(hmac_scan_stru));
    pst_scan_mgmt->en_is_scanning = OAL_FALSE;
    pst_scan_mgmt->st_scan_record_mgmt.en_vap_last_state = MAC_VAP_STATE_BUTT;

    /* 初始化bss管理结果链表和锁 */
    pst_bss_mgmt = &(pst_scan_mgmt->st_scan_record_mgmt.st_bss_mgmt);
    oal_dlist_init_head(&(pst_bss_mgmt->st_bss_list_head));
    oal_spin_lock_init(&(pst_bss_mgmt->st_lock));

    /* 初始化内核下发扫描request资源锁 */
    oal_spin_lock_init(&(pst_scan_mgmt->st_scan_request_spinlock));

    /* 初始化 st_wiphy_mgmt 结构 */
    OAL_WAIT_QUEUE_INIT_HEAD(&(pst_scan_mgmt->st_wait_queue));

    /* 初始化扫描生成随机MAC 地址 */
    oal_random_ether_addr(pst_hmac_device->st_scan_mgmt.auc_random_mac);

#if  defined(_PRE_WLAN_CHIP_TEST_ALG) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_DEBUG_MODE)
    hmac_scan_ct_init();
#endif

    return;
}


oal_void  hmac_scan_exit_etc(hmac_device_stru *pst_hmac_device)
{
    hmac_scan_stru      *pst_scan_mgmt;

    /* 清空扫描记录信息 */
    hmac_scan_clean_scan(&(pst_hmac_device->st_scan_mgmt));

#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
    if (pst_hmac_device->st_scan_mgmt.st_init_scan_timeout.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_device->st_scan_mgmt.st_init_scan_timeout);
    }
#endif

    /* 清除扫描管理结构体信息 */
    pst_scan_mgmt = &(pst_hmac_device->st_scan_mgmt);
    OAL_MEMZERO(pst_scan_mgmt, OAL_SIZEOF(hmac_scan_stru));
    pst_scan_mgmt->en_is_scanning = OAL_FALSE;

#if  defined(_PRE_WLAN_CHIP_TEST_ALG) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_DEBUG_MODE)
    hmac_scan_ct_exit();
#endif

    return;
}


#ifdef _PRE_DEBUG_MODE

oal_void  hmac_scan_test_cb(void *p_scan_record)
{
    hmac_scan_record_stru           *pst_scan_record;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;
    mac_bss_dscr_stru               *pst_bss_dscr;
    hmac_scanned_bss_info           *pst_scanned_bss;
    oal_dlist_head_stru             *pst_entry;
    oal_time_us_stru                 st_curr_time;
    oal_uint8                        uc_idx;

    if (OAL_PTR_NULL == p_scan_record)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "pst_scan_record is null ptr.");
        return;
    }

    pst_scan_record = (hmac_scan_record_stru *)p_scan_record;

    /* 打印信道测量结果 */
    OAL_IO_PRINT("The chan measure result: \n");
    for (uc_idx = 0; uc_idx < pst_scan_record->uc_chan_numbers; uc_idx++)
    {
        OAL_IO_PRINT("[channel_result]Chan num      : %d\n", pst_scan_record->ast_chan_results[uc_idx].uc_channel_number);
        OAL_IO_PRINT("[channel_result]Stats cnt     : %d\n", pst_scan_record->ast_chan_results[uc_idx].uc_stats_cnt);
        OAL_IO_PRINT("[channel_result]Stats valid   : %d\n", pst_scan_record->ast_chan_results[uc_idx].uc_stats_valid);
        OAL_IO_PRINT("[channel_result]Stats time us : %d\n", pst_scan_record->ast_chan_results[uc_idx].ul_total_stats_time_us);
        OAL_IO_PRINT("[channel_result]Free time 20M : %d\n", pst_scan_record->ast_chan_results[uc_idx].ul_total_free_time_20M_us);
        OAL_IO_PRINT("[channel_result]Free time 40M : %d\n", pst_scan_record->ast_chan_results[uc_idx].ul_total_free_time_40M_us);
        OAL_IO_PRINT("[channel_result]Free time 80M : %d\n", pst_scan_record->ast_chan_results[uc_idx].ul_total_free_time_80M_us);
        OAL_IO_PRINT("[channel_result]Send time     : %d\n", pst_scan_record->ast_chan_results[uc_idx].ul_total_send_time_us);
        OAL_IO_PRINT("[channel_result]Recv time     : %d\n", pst_scan_record->ast_chan_results[uc_idx].ul_total_recv_time_us);
        OAL_IO_PRINT("[channel_result]Free power cnt: %d\n", pst_scan_record->ast_chan_results[uc_idx].uc_free_power_cnt);
        OAL_IO_PRINT("[channel_result]Free power 20M: %d\n", pst_scan_record->ast_chan_results[uc_idx].s_free_power_stats_20M);
        OAL_IO_PRINT("[channel_result]Free power 40M: %d\n", pst_scan_record->ast_chan_results[uc_idx].s_free_power_stats_40M);
        OAL_IO_PRINT("[channel_result]Free power 80M: %d\n", pst_scan_record->ast_chan_results[uc_idx].s_free_power_stats_80M);
        OAL_IO_PRINT("[channel_result]Radar detected: %d\n", pst_scan_record->ast_chan_results[uc_idx].uc_radar_detected);
        OAL_IO_PRINT("[channel_result]Radar bw      : %d\n\n", pst_scan_record->ast_chan_results[uc_idx].uc_radar_bw);
    }

    /* 打印BSS结果 */
    OAL_IO_PRINT("The bss result: \n");

    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
        if (OAL_PTR_NULL == pst_bss_dscr)
        {
            continue;
        }

        OAL_IO_PRINT("Chan num      : %d\n", pst_bss_dscr->st_channel.uc_chan_number);
        OAL_IO_PRINT("BSSID         : 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n\n", pst_bss_dscr->auc_bssid[0],
                                                                          pst_bss_dscr->auc_bssid[1],
                                                                          pst_bss_dscr->auc_bssid[2],
                                                                          pst_bss_dscr->auc_bssid[3],
                                                                          pst_bss_dscr->auc_bssid[4],
                                                                          pst_bss_dscr->auc_bssid[5]);
    }

	/* 释放锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    /* 打印此次扫描耗时 */
    oal_time_get_stamp_us(&st_curr_time);
    OAL_IO_PRINT("Scan start time: %d %d\n", (oal_uint32)pst_scan_record->st_scan_start_timestamp.i_sec, (oal_uint32)pst_scan_record->st_scan_start_timestamp.i_usec);
    OAL_IO_PRINT("Scan end   time: %d %d\n\n", (oal_uint32)st_curr_time.i_sec, (oal_uint32)st_curr_time.i_usec);

    return;
}


oal_uint32  hmac_scan_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_scan_req_stru   st_scan_req;
    oal_uint8           uc_chan_idx;
    oal_uint32          ul_ret;
    oal_uint8           uc_2g_chan_num = 0;
    oal_uint8           uc_5g_chan_num = 0;
    oal_uint8           uc_chan_number = 0;
    oal_int8            ac_param[15] = {0};
    oal_bool_enum_uint8 en_2g_all = OAL_FALSE;
    oal_bool_enum_uint8 en_5g_all = OAL_FALSE;
    mac_ioctl_scan_test_config_stru *pst_scan_test;
    hmac_vap_stru                   *pst_hmac_vap;

    pst_scan_test = (mac_ioctl_scan_test_config_stru *)puc_param;

    oal_memcopy(ac_param, pst_scan_test->ac_scan_type, sizeof(pst_scan_test->ac_scan_type));
    OAL_MEMZERO(&st_scan_req, OAL_SIZEOF(st_scan_req));

    if (0 == oal_strcmp("2g", ac_param))
    {
        en_2g_all = OAL_TRUE;
    }
    else if (0 == oal_strcmp("5g", ac_param))
    {
        en_5g_all = OAL_TRUE;
    }
    else if (0 == oal_strcmp("all", ac_param))
    {
        en_2g_all = OAL_TRUE;
        en_5g_all = OAL_TRUE;
    }
    else
    {
        /* 指定单个信道 */
        uc_chan_number = (oal_uint8)oal_atoi(ac_param);
    }

    if (en_2g_all)
    {
        /* 2G全信道 */
        for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++)
        {
            ul_ret = mac_is_channel_idx_valid_etc(WLAN_BAND_2G, uc_chan_idx);
            if (OAL_SUCC == ul_ret)
            {
                mac_get_channel_num_from_idx_etc(WLAN_BAND_2G, uc_chan_idx, &uc_chan_number);

                st_scan_req.ast_channel_list[uc_2g_chan_num].uc_chan_number = uc_chan_number;
                st_scan_req.ast_channel_list[uc_2g_chan_num].en_band        = WLAN_BAND_2G;
                st_scan_req.ast_channel_list[uc_2g_chan_num].uc_chan_idx         = uc_chan_idx;
                st_scan_req.ast_channel_list[uc_2g_chan_num].en_bandwidth   = pst_scan_test->en_bandwidth;
                st_scan_req.uc_channel_nums++;
                uc_2g_chan_num++;
            }
        }
    }

    if (en_5g_all)
    {
        /* 5G全信道 */
        for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++)
        {
            ul_ret = mac_is_channel_idx_valid_etc(WLAN_BAND_5G, uc_chan_idx);
            if (OAL_SUCC == ul_ret)
            {
                mac_get_channel_num_from_idx_etc(WLAN_BAND_5G, uc_chan_idx, &uc_chan_number);

                st_scan_req.ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_chan_number = uc_chan_number;
                st_scan_req.ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].en_band        = WLAN_BAND_5G;
                st_scan_req.ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_chan_idx         = uc_chan_idx;
                st_scan_req.ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].en_bandwidth   = pst_scan_test->en_bandwidth;
                st_scan_req.uc_channel_nums++;
                uc_5g_chan_num++;
            }
        }
    }

    if (OAL_FALSE == en_2g_all && OAL_FALSE == en_5g_all)
    {
        if (uc_chan_number < 15)
        {
            ul_ret = mac_is_channel_num_valid_etc(WLAN_BAND_2G, uc_chan_number);
            if (OAL_SUCC != ul_ret)
            {
                OAL_IO_PRINT("Invalid chan num: %d. return!\n", uc_chan_number);
                return OAL_FAIL;
            }
            st_scan_req.ast_channel_list[0].en_band        = WLAN_BAND_2G;
        }
        else
        {
            ul_ret = mac_is_channel_num_valid_etc(WLAN_BAND_5G, uc_chan_number);
            if (OAL_SUCC != ul_ret)
            {
                OAL_IO_PRINT("Invalid chan num: %d. return!\n", uc_chan_number);
                return OAL_FAIL;
            }
            st_scan_req.ast_channel_list[0].en_band        = WLAN_BAND_5G;
        }

        st_scan_req.ast_channel_list[0].uc_chan_number = uc_chan_number;
        st_scan_req.ast_channel_list[0].en_bandwidth   = pst_scan_test->en_bandwidth;

        ul_ret = mac_get_channel_idx_from_num_etc(st_scan_req.ast_channel_list[0].en_band,
                                     st_scan_req.ast_channel_list[0].uc_chan_number,
                                     &(st_scan_req.ast_channel_list[0].uc_chan_idx));
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "hmac_scan_test::mac_get_channel_idx_from_num_etc fail=%d", ul_ret);
            return OAL_FAIL;
        }
        st_scan_req.uc_channel_nums                    = 1;
    }

    st_scan_req.en_bss_type  = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_req.en_scan_type = WLAN_SCAN_TYPE_ACTIVE;
    st_scan_req.uc_bssid_num = 0;
    st_scan_req.uc_ssid_num  = 0;

    oal_set_mac_addr(st_scan_req.auc_bssid[0], BROADCAST_MACADDR);
    st_scan_req.uc_bssid_num = 1;

    st_scan_req.us_scan_time = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    st_scan_req.en_scan_mode = WLAN_SCAN_MODE_FOREGROUND;
    st_scan_req.uc_scan_func = MAC_SCAN_FUNC_ALL;

    st_scan_req.p_fn_cb = hmac_scan_test_cb;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    return hmac_scan_proc_scan_req_event_etc(pst_hmac_vap, &st_scan_req);
}
#endif


oal_uint32  hmac_bgscan_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_scan_state_enum_uint8    pen_bgscan_enable_flag;

    pen_bgscan_enable_flag =  *puc_param;                 /*背景扫描停止使能位*/

    /* 背景扫描停止命令 */
    switch (pen_bgscan_enable_flag)
    {
        case 0:
            g_en_bgscan_enable_flag_etc = HMAC_BGSCAN_DISABLE;
            break;
        case 1:
            g_en_bgscan_enable_flag_etc = HMAC_BGSCAN_ENABLE;
            break;
        case 2:
            g_en_bgscan_enable_flag_etc = HMAC_SCAN_DISABLE;
            break;
        default:
            g_en_bgscan_enable_flag_etc = HMAC_BGSCAN_ENABLE;
            break;
    }

    OAM_WARNING_LOG1(0, OAM_SF_SCAN, "hmac_bgscan_enable_etc: g_en_bgscan_enable_flag_etc=%d.", g_en_bgscan_enable_flag_etc);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_DBAC

oal_uint32   hmac_scan_start_dbac_etc(mac_device_stru *pst_dev)
{
    oal_uint8   auc_cmd[32];
    oal_uint16  us_len;
    oal_uint32  ul_ret = OAL_FAIL;
    oal_uint8   uc_idx;
#define DBAC_START_STR     " dbac start"
#define DBAC_START_STR_LEN 11
    mac_vap_stru  *pst_mac_vap = OAL_PTR_NULL;

    mac_ioctl_alg_config_stru  *pst_alg_config = (mac_ioctl_alg_config_stru *)auc_cmd;

    oal_memcopy(auc_cmd + OAL_SIZEOF(mac_ioctl_alg_config_stru), (const oal_int8 *)DBAC_START_STR, 11);
    auc_cmd[OAL_SIZEOF(mac_ioctl_alg_config_stru) + DBAC_START_STR_LEN ] = 0;

    pst_alg_config->uc_argc = 2;
    pst_alg_config->auc_argv_offset[0] = 1;
    pst_alg_config->auc_argv_offset[1] = 6;

    for(uc_idx = 0; uc_idx < pst_dev->uc_vap_num; uc_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_dev->auc_vap_id[uc_idx]);
        if(OAL_PTR_NULL != pst_mac_vap &&
        WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        {
            break;
        }
    }
    if(pst_mac_vap)
    {
        us_len = OAL_SIZEOF(mac_ioctl_alg_config_stru) + DBAC_START_STR_LEN + 1;
        ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ALG, us_len, auc_cmd);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        }
        OAL_IO_PRINT("start dbac\n");
    }
    else
    {
        OAL_IO_PRINT("no vap found to start dbac\n");
    }

    return ul_ret;
}
#endif

oal_uint32 hmac_start_all_bss_of_device_etc(hmac_device_stru *pst_hmac_dev)
{
    oal_uint8      uc_idx;
    hmac_vap_stru *pst_hmac_vap;
    mac_device_stru *pst_dev = pst_hmac_dev->pst_device_base_info;

    OAM_WARNING_LOG1(0, OAM_SF_ACS, "{hmac_start_all_bss_of_device_etc:device id=%d}", pst_hmac_dev->pst_device_base_info->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_start_all_bss_of_device_etc::pst_device_base_info null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for(uc_idx = 0; uc_idx < pst_dev->uc_vap_num; uc_idx++)
    {
        pst_hmac_vap = mac_res_get_hmac_vap(pst_dev->auc_vap_id[uc_idx]);
        if (!pst_hmac_vap)
        {
            OAM_WARNING_LOG2(0, OAM_SF_ACS, "hmac_start_all_bss_of_device_etc:null ap, idx=%d id=%d", uc_idx, pst_dev->auc_vap_id[uc_idx]);
            continue;
        }

        if((WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode) &&
            (MAC_VAP_STATE_AP_WAIT_START == pst_hmac_vap->st_vap_base_info.en_vap_state
#ifdef _PRE_WLAN_FEATURE_DBAC
            || (mac_is_dbac_enabled(pst_dev) && MAC_VAP_STATE_PAUSE== pst_hmac_vap->st_vap_base_info.en_vap_state)
#endif
            || MAC_VAP_STATE_UP == pst_hmac_vap->st_vap_base_info.en_vap_state))
        {
            hmac_chan_start_bss_etc(pst_hmac_vap, OAL_PTR_NULL, WLAN_PROTOCOL_BUTT);
        }
        else
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ACS, "vap %d not started\n", pst_dev->auc_vap_id[uc_idx]);
            continue;
        }
    }

#ifdef _PRE_WLAN_FEATURE_DBAC
    if (mac_is_dbac_enabled(pst_dev))
    {
        return hmac_scan_start_dbac_etc(pst_dev);
    }
#endif

    return OAL_SUCC;
}

/* TBD 此函数随着scan模块上移打桩于此，需要ACS重写此接口 放到hmac_acs文件中 */
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_uint32 hmac_get_pri_sec_chan(mac_bss_dscr_stru *pst_bss_dscr, oal_uint32 *pul_pri_chan, oal_uint32 *pul_sec_chan)
{
    *pul_pri_chan = *pul_sec_chan = 0;

    *pul_pri_chan = pst_bss_dscr->st_channel.uc_chan_number;

    OAM_INFO_LOG1(0, OAM_SF_2040, "hmac_get_pri_sec_chan:pst_bss_dscr->st_channel.en_bandwidth = %d\n", pst_bss_dscr->en_channel_bandwidth);

    if (WLAN_BAND_WIDTH_40PLUS == pst_bss_dscr->en_channel_bandwidth)
    {
        *pul_sec_chan = *pul_pri_chan + 4;
    }
    else if (WLAN_BAND_WIDTH_40MINUS == pst_bss_dscr->en_channel_bandwidth)
    {
        *pul_sec_chan = *pul_pri_chan - 4;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_2040, "hmac_get_pri_sec_chan: pst_bss_dscr is not support 40Mhz, *pul_sec_chan = %d\n", *pul_sec_chan);
    }

    OAM_INFO_LOG2(0, OAM_SF_2040, "*pul_pri_chan = %d, *pul_sec_chan = %d\n\n", *pul_pri_chan, *pul_sec_chan);

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_switch_pri_sec(mac_channel_stru *pst_channel)
{
    if (WLAN_BAND_WIDTH_40PLUS == pst_channel->en_bandwidth)
    {
        pst_channel->uc_chan_number = pst_channel->uc_chan_number + 4;
        pst_channel->en_bandwidth = WLAN_BAND_WIDTH_40MINUS;
    }
    else if (WLAN_BAND_WIDTH_40MINUS == pst_channel->en_bandwidth)
    {
        pst_channel->uc_chan_number = pst_channel->uc_chan_number - 4;
        pst_channel->en_bandwidth = WLAN_BAND_WIDTH_40PLUS;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_2040, "hmac_switch_pri_sec:en_bandwidth = %d\n not need obss scan\n",
                        pst_channel->en_bandwidth);
    }
}


OAL_STATIC oal_bool_enum_uint8 hmac_obss_check_40mhz_2g(mac_vap_stru          *pst_mac_vap,
                                                        hmac_scan_record_stru *pst_scan_record,
                                                        mac_channel_stru      *pst_dst_channel)
{
    oal_uint32                  ul_pri_freq;
    oal_uint32                  ul_sec_freq;
    oal_uint32                  ul_affected_start;
    oal_uint32                  ul_affected_end;

    oal_uint32                  ul_pri;
    oal_uint32                  ul_sec;
    oal_uint32                  ul_sec_chan, ul_pri_chan;
    oal_int8                    c_obss_rssi_th;

    hmac_bss_mgmt_stru          *pst_bss_mgmt;
    mac_bss_dscr_stru           *pst_bss_dscr;
    hmac_scanned_bss_info       *pst_scanned_bss;
    oal_dlist_head_stru         *pst_entry;

    *pst_dst_channel = pst_mac_vap->st_channel;
    c_obss_rssi_th = HMAC_OBSS_RSSI_TH;
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    c_obss_rssi_th = (oal_int8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_OBSS_RSSI_TH);
#endif
#endif
    /* 获取主信道、次信道中心频点 */
    ul_pri_freq = (oal_int32)g_ast_freq_map_2g_etc[pst_mac_vap->st_channel.uc_chan_number - 1].us_freq;//2412 + (pst_mac_vap->st_channel.uc_chan_number - 1) * 5;

    if (WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth)
    {
        ul_sec_freq = ul_pri_freq + 20;
    }
    else if (WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth)
    {
        ul_sec_freq = ul_pri_freq - 20;
    }
    else
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "hmac_obss_check_40mhz_2g:en_bandwidth = %d not need obss\n",
                        pst_mac_vap->st_channel.en_bandwidth);
        return OAL_TRUE;
    }

    /* 2.4G共存检测,检测范围是40MHz带宽中心频点为中心,左右各25MHZ */
    ul_affected_start   = ((ul_pri_freq + ul_sec_freq) >> 1) - 25;
    ul_affected_end     = ((ul_pri_freq + ul_sec_freq) >> 1) + 25;

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040, "hmac_obss_check_40mhz_2g:40 MHz affected channel range: [%d, %d] MHz",
                    ul_affected_start, ul_affected_end);

    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
        if (OAL_PTR_NULL == pst_bss_dscr)
        {
            OAM_WARNING_LOG0(0,OAM_SF_2040,"{hmac_obss_check_40mhz_2g::pst_bss_dscr is NULL}");
            continue;
        }

        if(pst_bss_dscr->c_rssi < c_obss_rssi_th)
        {
            continue;
        }

        ul_pri = (oal_int32)g_ast_freq_map_2g_etc[pst_bss_dscr->st_channel.uc_chan_number - 1].us_freq;//2412 + (st_chan_result.uc_channel_number - 1) * 5;
        ul_sec = ul_pri;

        OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040, "pst_bss_dscr->st_channel.uc_chan_number = %d, ul_pri = %d\n",
                        pst_bss_dscr->st_channel.uc_chan_number, ul_pri);

        /* 获取扫描到的BSS的信道、频点信息 */
        hmac_get_pri_sec_chan(pst_bss_dscr, &ul_pri_chan, &ul_sec_chan);

        /* 该BSS为40MHz带宽,计算次信道频点 */
        if (ul_sec_chan)
        {
            if (ul_sec_chan < ul_pri_chan)
                ul_sec = ul_pri - 20;
            else
                ul_sec = ul_pri + 20;
        }

        if ((ul_pri < ul_affected_start || ul_pri > ul_affected_end) &&
            (ul_sec < ul_affected_start || ul_sec > ul_affected_end))
            continue; /* not within affected channel range */

        if (ul_pri_freq != ul_pri)//有叠频，且不是同一主信道，则当前AP不能使用40M
        {
            OAM_WARNING_LOG4(0, OAM_SF_2040, "hmac_obss_check_40mhz_2g:40 MHz pri/sec <%d, %d >mismatch with BSS <%d, %d>\n",
                            ul_pri_freq, ul_sec_freq, ul_pri, ul_sec);
            /* 解除锁 */
            oal_spin_unlock(&(pst_bss_mgmt->st_lock));

            // just trim bandwidth to 2G
            pst_dst_channel->en_bandwidth = WLAN_BAND_WIDTH_20M;

            return OAL_FALSE;
        }
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_TRUE;
}


OAL_STATIC oal_bool_enum_uint8 hmac_obss_check_40mhz_5g(mac_vap_stru          *pst_mac_vap,
                                                        hmac_scan_record_stru *pst_scan_record,
                                                        mac_channel_stru      *pst_dst_channel)
{
    oal_uint32                  ul_pri_chan;
    oal_uint32                  ul_sec_chan;
    oal_uint32                  ul_pri_bss;
    oal_uint32                  ul_sec_bss;
    oal_uint32                  ul_bss_pri_chan;
    oal_uint32                  ul_bss_sec_chan;
    oal_uint8                   uc_match;
    oal_uint8                   uc_inverse;
    oal_uint8                   uc_pri_20_bss;
    oal_uint8                   uc_sec_20_bss;
    hmac_bss_mgmt_stru          *pst_bss_mgmt;
    mac_bss_dscr_stru           *pst_bss_dscr;
    hmac_scanned_bss_info       *pst_scanned_bss;
    oal_dlist_head_stru         *pst_entry;
    oal_int8                    c_obss_rssi_th;

    *pst_dst_channel = pst_mac_vap->st_channel;

    /* 获取主信道和次信道 */
    ul_pri_chan = pst_mac_vap->st_channel.uc_chan_number;
    if (WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth)
    {
        ul_sec_chan = ul_pri_chan + 4;
    }
    else if (WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth)
    {
        ul_sec_chan = ul_pri_chan - 4;
    }
    else
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "hmac_obss_check_40mhz_5g: en_bandwidth = %d not need obss scan",
                        pst_mac_vap->st_channel.en_bandwidth);
        return OAL_TRUE;
    }

    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 若在次信道检测到Beacon, 但是主信道上没有, 则需要交换主次信道 */
    ul_pri_bss = ul_sec_bss = 0;
    c_obss_rssi_th = HMAC_OBSS_RSSI_TH;
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    c_obss_rssi_th = (oal_int8)hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_OBSS_RSSI_TH);
#endif
#endif
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        if (OAL_PTR_NULL == pst_bss_dscr)
        {
            OAM_WARNING_LOG0(0,OAM_SF_2040,"{hmac_obss_check_40mhz_5g::pst_bss_dscr is NULL}");
            continue;
        }
        if(pst_bss_dscr->c_rssi < c_obss_rssi_th)
        {
            continue;
        }

        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "hmac_obss_check_40mhz_5g:bss uc_channel_number = %d\n", pst_bss_dscr->st_channel.uc_chan_number);
        if (pst_bss_dscr->st_channel.uc_chan_number == ul_pri_chan)
            ul_pri_bss++;
        else if (pst_bss_dscr->st_channel.uc_chan_number == ul_sec_chan)
            ul_sec_bss++;
    }

    if (ul_sec_bss && !ul_pri_bss)
    {
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "hmac_obss_check_40mhz_5g:Switch own primary and secondary channel \
                        to get secondary channel with no Beacons from other BSSes\n");

        hmac_switch_pri_sec(pst_dst_channel);

        /* 此处主次交换完后直接返回即可, 按hostapd-2.4.0版本修改 */
		/* 释放锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));
        //这里不必判断是否需要切换到20M
	    return OAL_TRUE;
    }

    /*
     * Match PRI/SEC channel with any existing HT40 BSS on the same
     * channels that we are about to use (if already mixed order in
     * existing BSSes, use own preference).
     */

    //是否有与当前AP主次信道完全一致或者相反的40M BSS，另外也许确认是否有在主信道或者次信道的20M BSS
    uc_match        = OAL_FALSE;
    uc_inverse      = OAL_FALSE;
    uc_pri_20_bss   = OAL_FALSE;
    uc_sec_20_bss   = OAL_FALSE;
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
        if (OAL_PTR_NULL == pst_bss_dscr)
        {
            OAM_WARNING_LOG0(0,OAM_SF_2040,"{hmac_obss_check_40mhz_5g::pst_bss_dscr is NULL}");
            continue;
        }
        if(pst_bss_dscr->c_rssi < c_obss_rssi_th)
        {
            continue;
        }

        hmac_get_pri_sec_chan(pst_bss_dscr, &ul_bss_pri_chan, &ul_bss_sec_chan);
        if (ul_pri_chan == ul_bss_pri_chan && 0 == ul_bss_sec_chan)
        {
            uc_pri_20_bss = OAL_TRUE;//出现在当前AP主信道的20M BSS
        }
        if (ul_sec_chan == ul_bss_pri_chan && 0 == ul_bss_sec_chan)
        {
            uc_sec_20_bss = OAL_TRUE;//出现在当前AP次信道的20M BSS
        }
        if (ul_pri_chan == ul_bss_sec_chan &&
            ul_sec_chan == ul_bss_pri_chan) {
            uc_inverse = OAL_TRUE;//出现与当前AP主次信道相反的
        }
        if (ul_pri_chan == ul_bss_pri_chan &&
            ul_sec_chan == ul_bss_sec_chan) {
            uc_match = OAL_TRUE;//出现与当前AP主次信道一致的
        }
    }

    if(OAL_FALSE == uc_match && OAL_TRUE == uc_inverse && OAL_FALSE == uc_pri_20_bss)
    {//有相反的40M,没有一致的，且主20没有20M BSS,交换主次信道
        OAM_WARNING_LOG0(0, OAM_SF_2040, "hmac_obss_check_40mhz_5g:switch own primary and secondary channel due to BSS overlap with\n");

        hmac_switch_pri_sec(pst_dst_channel);
    }
    else if(OAL_FALSE == uc_match && OAL_TRUE == uc_inverse &&
        OAL_TRUE == uc_pri_20_bss && OAL_FALSE == uc_sec_20_bss)
    {//有相反的40M,没有一致的，且主20有20M BSS,次20没有20M BSS,需要交换主次信道并且切换到20M
        OAM_WARNING_LOG0(0, OAM_SF_2040, "hmac_obss_check_40mhz_5g:switch own primary and secondary channel due to BSS overlap and to 20M\n");

        hmac_switch_pri_sec(pst_dst_channel);
        pst_dst_channel->en_bandwidth = WLAN_BAND_WIDTH_20M;
    }
    else if(OAL_TRUE == uc_sec_20_bss)
    {//次信道有20M BSS，需要切换到20M
        OAM_WARNING_LOG2(0, OAM_SF_2040, "hmac_obss_check_40mhz_5g:40 MHz pri/sec <%d, %d > to 20M\n",
                        ul_pri_chan, ul_sec_chan);
        pst_dst_channel->en_bandwidth = WLAN_BAND_WIDTH_20M;
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_TRUE;
}

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_obss_check_40mhz(mac_vap_stru          *pst_mac_vap,
                                                                hmac_scan_record_stru *pst_scan_record,
                                                                mac_channel_stru      *pst_dst_channel)
{
    return (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band) ? hmac_obss_check_40mhz_2g(pst_mac_vap, pst_scan_record, pst_dst_channel)
                                                             : hmac_obss_check_40mhz_5g(pst_mac_vap, pst_scan_record, pst_dst_channel);
}


oal_uint32  hmac_obss_init_scan_hook_etc(hmac_scan_record_stru   *pst_scan_record,
                                     hmac_device_stru        *pst_dev)
{
    /*
    1、当前VAP的信道即为目标信道
    2、判断之后按照需要更新VAP的信道和带宽
    3、当前实现中，不考虑更改信道号，只更改带宽
    4、信道仅保存到device下，vap的信道并不更新，在init scan hook中集中处理
    */
    oal_uint8            uc_idx;
    mac_vap_stru        *pst_mac_vap;
    mac_vap_stru        *pst_vap[WLAN_BAND_BUTT] = {OAL_PTR_NULL, OAL_PTR_NULL};

    OAM_WARNING_LOG0(0, OAM_SF_ACS, "hmac_obss_init_scan_hook_etc run\n");

    /* 2G/5G vap 各找一个 */
    for(uc_idx = 0; uc_idx < pst_dev->pst_device_base_info->uc_vap_num; uc_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_dev->pst_device_base_info->auc_vap_id[uc_idx]);

        if (pst_mac_vap && WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        {
            if ((pst_mac_vap->st_channel.en_band == WLAN_BAND_2G) && !pst_vap[WLAN_BAND_2G])
            {
                pst_vap[WLAN_BAND_2G] = pst_mac_vap;
            }
            else if ((pst_mac_vap->st_channel.en_band == WLAN_BAND_5G) && !pst_vap[WLAN_BAND_5G])
            {
                pst_vap[WLAN_BAND_5G] = pst_mac_vap;
            }
        }
    }

    for(uc_idx = 0; uc_idx < WLAN_BAND_BUTT; uc_idx++)
    {
        if (pst_vap[uc_idx])
        {
            hmac_obss_check_40mhz(pst_vap[uc_idx], pst_scan_record, pst_dev->ast_best_channel + uc_idx);
            pst_vap[uc_idx]->st_ch_switch_info.en_user_pref_bandwidth = pst_dev->ast_best_channel[uc_idx].en_bandwidth;

            OAM_WARNING_LOG4(pst_vap[uc_idx]->uc_vap_id, OAM_SF_2040, "hmac_obss_init_scan_hook_etc::before:ch=%d bw=%d, after :ch=%d bw=%d",
                             pst_vap[uc_idx]->st_channel.uc_chan_number,
                             pst_vap[uc_idx]->st_channel.en_bandwidth,
                             pst_dev->ast_best_channel[uc_idx].uc_chan_number,
                             pst_dev->ast_best_channel[uc_idx].en_bandwidth);
        }
    }

    return OAL_SUCC;
}
#endif

#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)

oal_bool_enum_uint8 hmac_device_in_init_scan_etc(mac_device_stru *pst_mac_device)
{
    hmac_device_stru    *pst_hmac_device = OAL_PTR_NULL;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_device->uc_device_id);

    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_res_get_mac_dev_etc:pst_hmac_device is null\n");
        return OAL_FALSE;
    }
    else
    {
        return pst_hmac_device->en_in_init_scan;
    }

}


mac_need_init_scan_res_enum_uint8 hmac_need_init_scan_etc(hmac_device_stru *pst_hmac_device, mac_vap_stru *pst_in_mac_vap, mac_try_init_scan_type en_type)
{
    mac_need_init_scan_res_enum_uint8 en_result = MAC_INIT_SCAN_NOT_NEED;
    mac_device_stru    *pst_mac_device = pst_hmac_device->pst_device_base_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_need_init_scan_etc::pst_device_base_info null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    {
        mac_vap_stru *pst_main_sta = mac_find_main_proxysta(pst_mac_device);

        // proxysta模式下仅在以下情况下进行初始信道选择
        // 1、main sta不存在
        // 2、main sta存在，但是其状态为INIT
        // tscancode-suppress *
        if (!(!pst_main_sta || (pst_main_sta && pst_main_sta->en_vap_state == MAC_VAP_STATE_INIT)))
        {
            return MAC_INIT_SCAN_NOT_NEED;
        }
    }
#endif
    if (pst_in_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP && pst_in_mac_vap->en_vap_mode != WLAN_VAP_MODE_CONFIG)
    {
        return MAC_INIT_SCAN_NOT_NEED;
    }

#ifdef _PRE_WLAN_FEATURE_DBAC
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    if (mac_is_dbac_enabled(pst_mac_device) && en_type != MAC_TRY_INIT_SCAN_START_DBAC)
    {
        return MAC_INIT_SCAN_NOT_NEED;
    }
#endif
#endif

    if(pst_hmac_device->en_in_init_scan)
    {
        return MAC_TRY_INIT_SCAN_RESCAN == en_type ? MAC_INIT_SCAN_NOT_NEED : MAC_INIT_SCAN_IN_SCAN;
    }

#ifdef _PRE_SUPPORT_ACS
    en_result |=  ((mac_get_acs_switch(pst_mac_device) == MAC_ACS_SW_INIT
                  || mac_get_acs_switch(pst_mac_device) == MAC_ACS_SW_BOTH ? OAL_TRUE : OAL_FALSE));
    if (MAC_TRY_INIT_SCAN_RESCAN == en_type)
    {
        return en_result;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
    /*lint -e514*/
    en_result |= (mac_vap_get_dfs_enable(pst_in_mac_vap) && mac_dfs_get_cac_enable(pst_hmac_device->pst_device_base_info)
               && hmac_dfs_need_for_cac(pst_mac_device, pst_in_mac_vap)) ? OAL_TRUE : OAL_FALSE;
    /*lint +e514*/

#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    if(!pst_in_mac_vap->bit_bw_fixed)
    {
        en_result |= (mac_get_2040bss_switch(pst_mac_device)
            &&(WLAN_BAND_WIDTH_20M != pst_in_mac_vap->st_channel.en_bandwidth))? OAL_TRUE : OAL_FALSE;
    }
#endif


    return en_result;

}

oal_uint32 hmac_init_scan_sync_channel(hmac_scan_record_stru   *pst_scan_record,
                                       hmac_device_stru        *pst_hmac_dev)
{
    mac_device_stru  *pst_mac_device;
    mac_channel_stru *pst_channel;
    oal_uint8         uc_idx;
    mac_vap_stru     *pst_mac_vap;
    oal_bool_enum_uint8 en_acs_en = OAL_FALSE, en_obss_en = OAL_FALSE;
#if (defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE))
    hmac_vap_stru      *pst_hmac_vap;
#endif
    if (!pst_scan_record || !pst_hmac_dev || !pst_hmac_dev->pst_device_base_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_init_scan_sync_channel:null ptr\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_hmac_dev->pst_device_base_info;
    pst_channel    = pst_hmac_dev->ast_best_channel;

#ifdef _PRE_SUPPORT_ACS
    if (mac_get_acs_switch(pst_mac_device))
    {
        en_acs_en = OAL_TRUE;
    }
#endif

    // just in case someone missing
    for (uc_idx = 0; uc_idx < WLAN_BAND_BUTT; uc_idx++)
    {
        mac_get_channel_idx_from_num_etc(pst_channel[uc_idx].en_band,
                                     pst_channel[uc_idx].uc_chan_number,
                                    &pst_channel[uc_idx].uc_chan_idx);
    }

    /*
     * ACS未使能、OBSS使能时，使用dev下设置的信道
     * ACS使能，  未omit acs时，使用dev下设置的信道
     *              omit acs时，使用原vap信道，补做OBSS检查
     */
    for (uc_idx = 0; uc_idx < pst_mac_device->uc_vap_num; uc_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_idx]);
        if(OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            continue;
        }

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
        if (mac_get_2040bss_switch(pst_mac_device)&&(!pst_mac_vap->bit_bw_fixed))
        {
            en_obss_en = OAL_TRUE;
        }
#endif

        if(!en_acs_en && !en_obss_en)
        {
            continue;
        }

        if ((WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
            && (MAC_VAP_STATE_AP_WAIT_START == pst_mac_vap->en_vap_state))
        {

#if (defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE))
            pst_hmac_vap = OAL_DLIST_GET_ENTRY(pst_mac_vap, hmac_vap_stru, st_vap_base_info);

            if (pst_hmac_vap->en_omit_acs_chan && en_acs_en)
            {
                // 忽略ACS信道，将VAP原信道覆盖ACS信道
                pst_channel[pst_mac_vap->st_channel.en_band] = pst_mac_vap->st_channel;
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "hmac_init_scan_sync_channel:override acs channel\n");

                if ( en_obss_en &&
                    ((WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth) ||
                    (WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth)))
                {
                    if (!hmac_obss_check_40mhz(pst_mac_vap, pst_scan_record, &pst_channel[pst_mac_vap->st_channel.en_band]))
                    {
                        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "hmac_init_scan_sync_channel:channel trimed to 20MHz for obss\n");
                    }
                    pst_mac_vap->st_ch_switch_info.en_user_pref_bandwidth = pst_channel[pst_mac_vap->st_channel.en_band].en_bandwidth;
                }
            }
#endif

            // 若dev下选择的信道合法，则将其覆盖到vap结构体
            if (pst_channel[pst_mac_vap->st_channel.en_band].uc_chan_number)
            {
                pst_mac_vap->st_channel = pst_channel[pst_mac_vap->st_channel.en_band];
                OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "hmac_init_scan_sync_channel:vap ch info(band=%d,bw=%d,ch=%d) sync from acs\n",
                                 pst_mac_vap->st_channel.en_band,
                                 pst_mac_vap->st_channel.en_bandwidth,
                                 pst_mac_vap->st_channel.uc_chan_number);
            }
            else
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "hmac_init_scan_sync_channel:no acs channel found for band %d\n", pst_mac_vap->st_channel.en_band);
            }
        }
    }

    return OAL_SUCC;
}

oal_uint32  hmac_init_scan_timeout_etc(void *p_arg)
{
    hmac_device_stru *pst_dev = (hmac_device_stru *)p_arg;

    if (!pst_dev->en_init_scan)
    {
        return OAL_SUCC;
    }

    pst_dev->en_init_scan      = OAL_FALSE;
    pst_dev->en_in_init_scan   = OAL_FALSE;
    hmac_start_all_bss_of_device_etc(pst_dev);

    return OAL_SUCC;
}


oal_uint32  hmac_init_scan_cancel_timer_etc(hmac_device_stru *pst_hmac_dev)
{
    if (pst_hmac_dev && pst_hmac_dev->st_scan_mgmt.st_init_scan_timeout.en_is_registerd)
    {
        /* 关闭超时定时器 */
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_dev->st_scan_mgmt.st_init_scan_timeout);
    }

    return OAL_SUCC;
}


oal_void hmac_init_scan_cb_etc(void *p_scan_record)
{
    hmac_scan_record_stru           *pst_scan_record = (hmac_scan_record_stru *)p_scan_record;
    oal_uint8                        uc_device_id    = pst_scan_record->uc_device_id;
    hmac_device_stru                 *pst_hmac_dev    = hmac_res_get_mac_dev_etc(uc_device_id);
    mac_device_stru                  *pst_mac_dev;
    mac_vap_stru                     *pst_mac_vap;

    OAM_WARNING_LOG0(0, OAM_SF_ACS, "{hmac_init_scan_cb_etc called}");

    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "{hmac_init_scan_cb_etc:pst_hmac_dev=NULL, device_id=%d}", uc_device_id);
        return;
    }

    pst_mac_dev = pst_hmac_dev->pst_device_base_info;
    if (OAL_PTR_NULL == pst_hmac_dev || OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "{hmac_init_scan_cb_etc:pst_mac_dev=NULL, device_id=%d}", uc_device_id);
        return;
    }

    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_scan_record->uc_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "{hmac_init_scan_cb_etc:pst_mac_vap=NULL, vap_id=%d}", pst_scan_record->uc_vap_id);
        return;
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_init_scan_cb_etc::scan report ch=%d bss=%d init=%d}\n", pst_scan_record->uc_chan_numbers,
                      pst_scan_record->st_bss_mgmt.ul_bss_num, pst_hmac_dev->en_init_scan);

#ifdef _PRE_SUPPORT_ACS
    if (MAC_ACS_SW_INIT == mac_get_acs_switch(pst_mac_dev) || MAC_ACS_SW_BOTH == mac_get_acs_switch(pst_mac_dev))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ACS, "acs  enable, post and return\n");
        /* 若ACS在运行中，则等待ACS APP返回响应结果 */
        if(OAL_SUCC == hmac_acs_init_scan_hook(pst_scan_record, pst_hmac_dev))
        {
            return;
        }
    }
#endif

    OAM_WARNING_LOG0(0, OAM_SF_ACS, "acs not enable, cancel timer\n");
    /* ACS未执行，超时保护至此结束 */
    hmac_init_scan_cancel_timer_etc(pst_hmac_dev);

    /* 若ACS未执行或者执行失败，继续执行后续操作 */
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    if (mac_get_2040bss_switch(pst_mac_dev)&&(!pst_mac_vap->bit_bw_fixed))
    {
        hmac_obss_init_scan_hook_etc(pst_scan_record, pst_hmac_dev);
    }
#endif

    /*
     * 至此，信道最终确认，均保存在device下且未同步至vap的信道结构中
     * 此处同步并记录原始信道，同时保证CAC在正常的信道上执行
     * 但不会设置信道至硬件，由后续流程完成
     */
    hmac_init_scan_sync_channel(pst_scan_record, pst_hmac_dev);

#ifdef _PRE_WLAN_FEATURE_DFS
    if (mac_vap_get_dfs_enable(pst_mac_vap))
    {
        /* 若成功开始了CAC或者已经开始CAC，返回，由CAC超时函数处理VAP START */
        if(OAL_SUCC == hmac_dfs_init_scan_hook_etc(pst_scan_record, pst_hmac_dev))
        {
            return;
        }
    }
#endif

    /* ACS未运行、DFS未运行，直接启动BSS */
    hmac_init_scan_timeout_etc(pst_hmac_dev);
}


oal_uint32 hmac_init_scan_do_etc(hmac_device_stru *pst_hmac_dev, mac_vap_stru *pst_mac_vap, mac_init_scan_req_stru *pst_cmd, hmac_acs_cfg_stru *pst_acs_cfg)
{
    oal_uint8                           uc_idx, uc_cnt;
    mac_scan_req_stru                   st_scan_req;
    wlan_channel_band_enum_uint8        en_band;
    wlan_channel_bandwidth_enum_uint8   en_bandwidth;
    oal_uint32                          ul_ret = OAL_FAIL;
    hmac_vap_stru                      *pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if(pst_hmac_vap == OAL_PTR_NULL || pst_acs_cfg == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ACS, "{hmac_init_scan_do_etc:get hmac vap=NULL or acs_cfg is NULL, vapid=%d}", pst_mac_vap->uc_vap_id);
        return OAL_FAIL;
    }

    OAL_MEMZERO(&st_scan_req, OAL_SIZEOF(st_scan_req));
    st_scan_req.en_scan_mode  = pst_cmd->auc_arg[0];
    st_scan_req.en_scan_type  = pst_cmd->auc_arg[1];
    st_scan_req.uc_scan_func  = pst_cmd->auc_arg[2];
    st_scan_req.en_bss_type   = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_req.uc_bssid_num  = 0;
    st_scan_req.uc_ssid_num   = 0;

    st_scan_req.uc_max_scan_count_per_channel           = 1;
    st_scan_req.uc_max_send_probe_req_count_per_channel = st_scan_req.en_scan_mode == WLAN_SCAN_MODE_FOREGROUND ? 2 : 1;

    st_scan_req.us_scan_time        = st_scan_req.en_scan_mode == WLAN_SCAN_MODE_FOREGROUND ? 120 : 30;
    st_scan_req.uc_probe_delay      = 0;
    st_scan_req.uc_vap_id           = pst_mac_vap->uc_vap_id; /* 其实是该device下的vap_id[0] */
    st_scan_req.p_fn_cb             = hmac_init_scan_cb_etc;

    st_scan_req.uc_channel_nums     = 0;        /* 信道列表中信道的个数 */
#ifdef _PRE_SUPPORT_ACS
    st_scan_req.uc_acs_type         = pst_acs_cfg->uc_acs_type;
    st_scan_req.en_switch_chan      = pst_acs_cfg->en_switch_chan;
#endif
    uc_cnt = 0;
    for (uc_idx = 0; uc_idx < pst_cmd->ul_cmd_len; uc_idx += 2)
    {
        en_band      = pst_cmd->auc_data[uc_idx] & 0X0F;
        en_bandwidth = (pst_cmd->auc_data[uc_idx] >> 4) & 0x0F;
        if (OAL_SUCC != mac_is_channel_num_valid_etc(en_band, pst_cmd->auc_data[uc_idx+1]))
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ACS, "{hmac_init_scan_do_etc:invalid channel number, ch=%d, band=%d bw=%d}",
                                        pst_cmd->auc_data[uc_idx+1], en_band, en_bandwidth);
            OAL_IO_PRINT("{hmac_init_scan_do_etc:invalid channel number, ch=%d, band=%d bw=%d}\n",
                                        pst_cmd->auc_data[uc_idx+1], en_band, en_bandwidth);

            continue;
        }

        st_scan_req.ast_channel_list[uc_cnt].uc_chan_number = pst_cmd->auc_data[uc_idx + 1];
        st_scan_req.ast_channel_list[uc_cnt].en_band        = en_band;
        st_scan_req.ast_channel_list[uc_cnt].en_bandwidth   = en_bandwidth;
        ul_ret = mac_get_channel_idx_from_num_etc(st_scan_req.ast_channel_list[uc_cnt].en_band,
                                     st_scan_req.ast_channel_list[uc_cnt].uc_chan_number,
                                    &st_scan_req.ast_channel_list[uc_cnt].uc_chan_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ACS, "{hmac_init_scan_do_etc:mac_get_channel_idx_from_num_etc failed en_band:[%d],chan_number:[%d]}",
                st_scan_req.ast_channel_list[uc_cnt].en_band,st_scan_req.ast_channel_list[uc_cnt].uc_chan_number);
        }
        uc_cnt++;
    }

    OAL_IO_PRINT("hmac_init_scan_do_etc::got=5:do scan mode=%d func=0x%x type=%d ch_cnt=%d\n",
        st_scan_req.en_scan_mode,
        st_scan_req.uc_scan_func,st_scan_req.en_scan_type, uc_cnt);

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "hmac_init_scan_do_etc::got=5:do scan mode=%d func=0x%x type=%d ch_cnt=%d\n",
                        st_scan_req.en_scan_mode,
                        st_scan_req.uc_scan_func,st_scan_req.en_scan_type, uc_cnt);

    if (uc_cnt != 0)
    {
        st_scan_req.uc_channel_nums = uc_cnt;

        /* 直接调用扫描模块扫描请求处理函数 */
        ul_ret = hmac_scan_proc_scan_req_event_etc(pst_hmac_vap, &st_scan_req);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "hmac_init_scan_do_etc:hmac_scan_add_req failed, ret=%d", ul_ret);
        }
    }
    else
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "hmac_init_scan_do_etc:no valid channel found, not scan");
    }

    return ul_ret;
}

oal_bool_enum_uint8 hmac_init_scan_skip_channel_etc(hmac_device_stru              *pst_hmac_dev,
                                                wlan_channel_band_enum_uint8   en_band,
                                                oal_uint8                      uc_idx)
{
    /* skip any illegal channel */
    if (OAL_SUCC != mac_is_channel_idx_valid_etc(en_band, uc_idx))
    {
        return OAL_TRUE;
    }
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_dev->pst_device_base_info))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_init_scan_skip_channel_etc::pst_device_base_info null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* on 1151, scan all channels when dbac enabled */
#ifdef _PRE_WLAN_FEATURE_DBAC
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    if (mac_is_dbac_enabled(pst_hmac_dev->pst_device_base_info))
    {
        return OAL_FALSE;
    }
#endif
#endif

    if (pst_hmac_dev->pst_device_base_info
     && pst_hmac_dev->pst_device_base_info->en_max_band != en_band)
    {
        return  OAL_TRUE;
    }

    return OAL_FALSE;
}

oal_uint32 hmac_init_scan_process_etc(hmac_device_stru *pst_hmac_dev, mac_vap_stru *pst_mac_vap, hmac_acs_cfg_stru *pst_acs_cfg)
{
    oal_uint8         ast_buf[OAL_SIZEOF(mac_acs_cmd_stru) - 4 + WLAN_MAX_CHANNEL_NUM * 2];
    oal_uint8         uc_idx;
    oal_uint8        *puc_dat, uc_tot, uc_chan_num;
    oal_uint32        ul_ret;

    mac_init_scan_req_stru *pst_cmd = (mac_init_scan_req_stru *)ast_buf;

    puc_dat = pst_cmd->auc_data;

    pst_hmac_dev->en_init_scan    = (pst_acs_cfg->en_scan_op ==  MAC_SCAN_OP_INIT_SCAN) ? OAL_TRUE : OAL_FALSE;
    pst_hmac_dev->en_in_init_scan = OAL_TRUE;

    pst_cmd->uc_cmd     = 5; //DMAC_ACS_CMD_DO_SCAN;
    pst_cmd->auc_arg[0] =   (pst_acs_cfg->en_scan_op ==  MAC_SCAN_OP_FG_SCAN_ONLY
                          || pst_acs_cfg->en_scan_op ==  MAC_SCAN_OP_INIT_SCAN) ? WLAN_SCAN_MODE_FOREGROUND : WLAN_SCAN_MODE_BACKGROUND_AP;

    pst_cmd->auc_arg[1] = WLAN_SCAN_TYPE_ACTIVE;
    pst_cmd->auc_arg[2] = MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS | MAC_SCAN_FUNC_BSS;
    pst_cmd->ul_cmd_cnt = 0;

    uc_tot = 0;
    for (uc_idx = 0; uc_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_idx++)
    {
        if(!hmac_init_scan_skip_channel_etc(pst_hmac_dev, WLAN_BAND_2G, uc_idx))
        {
            mac_get_channel_num_from_idx_etc(WLAN_BAND_2G, uc_idx, &uc_chan_num);
            *puc_dat++ = ((WLAN_BAND_WIDTH_20M) << 4) | WLAN_BAND_2G;
            *puc_dat++ = uc_chan_num;
             uc_tot++;
        }
    }
    for (uc_idx = 0; uc_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_idx++)
    {
        if(!hmac_init_scan_skip_channel_etc(pst_hmac_dev, WLAN_BAND_5G, uc_idx))
        {
            mac_get_channel_num_from_idx_etc(WLAN_BAND_5G, uc_idx, &uc_chan_num);
            *puc_dat++ = ((WLAN_BAND_WIDTH_20M) << 4) | WLAN_BAND_5G;
            *puc_dat++ = uc_chan_num;
             uc_tot++;
        }
    }

    pst_cmd->ul_cmd_len = uc_tot * 2;

    /* best信道清空为0，在start bss作为合法性判断依据 */
    if(pst_hmac_dev->en_init_scan)
    {
        oal_memset(pst_hmac_dev->ast_best_channel, 0, OAL_SIZEOF(pst_hmac_dev->ast_best_channel));
    }

    /* 启动定时器, 超时后强制启动BSS: 保护时间包括扫描时长与APP交互时长 */
    {
        FRW_TIMER_CREATE_TIMER(&pst_hmac_dev->st_scan_mgmt.st_init_scan_timeout,
                               hmac_init_scan_timeout_etc,
                               HMAC_INIT_SCAN_TIMEOUT_MS,
                               pst_hmac_dev,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_dev->pst_device_base_info->ul_core_id);
    }

    ul_ret = hmac_init_scan_do_etc(pst_hmac_dev, pst_mac_vap, pst_cmd, pst_acs_cfg);
    if(OAL_SUCC != ul_ret)
    {
        pst_hmac_dev->en_init_scan    = OAL_FALSE;
        pst_hmac_dev->en_in_init_scan = OAL_FALSE;
    }

    return ul_ret;
}

oal_uint32  hmac_init_scan_try_etc(mac_device_stru *pst_mac_device, mac_vap_stru *pst_in_mac_vap, mac_try_init_scan_type en_type, hmac_acs_cfg_stru *pst_acs_cfg)
{
    oal_uint8               uc_idx;
    mac_vap_stru           *pst_mac_vap, *pst_mac_vap_scan = NULL;
    hmac_vap_stru          *pst_hmac_vap;
    oal_uint8               en_scan_type;
    hmac_device_stru       *pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_device->uc_device_id);
    oal_bool_enum_uint8     en_need_do_init_scan = OAL_FALSE;

    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_init_scan_try_etc:mac_res_get_hmac_vap failed. device_id:%d.}",pst_mac_device->uc_device_id);
        return OAL_FAIL;
    }
    en_scan_type = hmac_need_init_scan_etc(pst_hmac_device, pst_in_mac_vap, en_type);
    if(en_scan_type == MAC_INIT_SCAN_NOT_NEED)
    {
        return OAL_FAIL;
    }
    else if(en_scan_type == MAC_INIT_SCAN_IN_SCAN)
    {
        OAL_IO_PRINT("just in init scan\n");

        mac_vap_init_rates_etc(pst_in_mac_vap);
        pst_hmac_vap = mac_res_get_hmac_vap(pst_in_mac_vap->uc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_init_scan_try_etc:mac_res_get_hmac_vap failed vap_id:%d.}",pst_in_mac_vap->uc_vap_id);
            return OAL_FAIL;
        }
        if (pst_hmac_vap->st_vap_base_info.en_vap_state == MAC_VAP_STATE_UP)
        {
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);
        }

        return OAL_SUCC;
    }
    else
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "hmac_init_scan_try_etc: need init scan");
    }

    for (uc_idx = 0; uc_idx < pst_hmac_device->pst_device_base_info->uc_vap_num; uc_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_hmac_device->pst_device_base_info->auc_vap_id[uc_idx]);
        if (!pst_mac_vap)
        {
            continue;
        }

        if(WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        {
            mac_vap_init_rates_etc(pst_mac_vap);

            /* 强制设置AP侧状态机为 WAIT_START，因为需要执行初始信道检查 */
            pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
            if (OAL_PTR_NULL == pst_hmac_vap)
            {
                OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_init_scan_try_etc::need init scan fail.vap_id = %u}",pst_mac_vap->uc_vap_id);
                continue;
            }
            switch (pst_hmac_vap->st_vap_base_info.en_vap_state)
            {
                case MAC_VAP_STATE_UP :
                case MAC_VAP_STATE_PAUSE : // dbac
                        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);
                        // no break here!
                case MAC_VAP_STATE_AP_WAIT_START :
                        en_need_do_init_scan = OAL_TRUE;
                        pst_mac_vap_scan = pst_mac_vap;
                        break;
                default :
                     break;
            }
        }
    }

    if (en_need_do_init_scan)
    {
        return hmac_init_scan_process_etc(pst_hmac_device, pst_mac_vap_scan, pst_acs_cfg);
    }

    return OAL_SUCC;
}

#endif
/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_scan_find_scanned_bss_dscr_by_index_etc);
oal_module_symbol(hmac_scan_find_scanned_bss_by_bssid_etc);
/*lint -e578*//*lint -e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

