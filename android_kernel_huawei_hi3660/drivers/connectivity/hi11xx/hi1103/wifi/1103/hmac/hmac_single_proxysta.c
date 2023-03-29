


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_SINGLE_PROXYSTA

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_spec.h"
#include "mac_frame.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "hmac_device.h"
#include "hmac_resource.h"
#include "hmac_single_proxysta.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SINGLE_PROXYSTA_C




oal_uint32  hmac_proxysta_map_aging_timer(void *p_arg)
{
    oal_uint32          ul_present_time = 0;
    oal_uint32          ul_map_idle_time = 0;
    hmac_vap_stru        *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32          ul_loop = 0;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_dlist_temp = OAL_PTR_NULL;
    hmac_proxysta_ipv4_hash_stru    *pst_hash_ipv4 = OAL_PTR_NULL;
    hmac_proxysta_ipv6_hash_stru    *pst_hash_ipv6 = OAL_PTR_NULL;
    hmac_proxysta_unknow_hash_stru  *pst_hash_unknow = OAL_PTR_NULL;

    if (OAL_PTR_NULL == p_arg)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{hmac_proxysta_map_aging_timer::p_arg null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap = (hmac_vap_stru *)p_arg;
    if (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PROXYSTA, "{hmac_proxysta_map_aging_timer::pst_vap_proxysta null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取当前时间 */
    ul_present_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    /* 遍历所有链表，时间间隔超过老化时间的MAP表 进行删除 */
    oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_MAX_VALUE; ul_loop++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_temp, &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv4_head[ul_loop]))
        {
            pst_hash_ipv4 = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_ipv4_hash_stru, st_entry);
            ul_map_idle_time = (oal_uint32)OAL_TIME_GET_RUNTIME(pst_hash_ipv4->ul_last_active_timestamp, ul_present_time);
            if (HMAC_PROXYSTA_MAP_AGING_TIME < ul_map_idle_time)
            {
                oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
#ifdef _PRE_DEBUG_MODE
                OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_map_aging_timer:: remove ipv4 map[%d:%d:%d:%d].}",
                    pst_hash_ipv4->auc_ipv4[0], pst_hash_ipv4->auc_ipv4[1], pst_hash_ipv4->auc_ipv4[2], pst_hash_ipv4->auc_ipv4[3]);
                OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_map_aging_timer:: remove ipv4 map[%x:%x:ff:ff:%x:%x].}",
                    pst_hash_ipv4->auc_mac[0], pst_hash_ipv4->auc_mac[1], pst_hash_ipv4->auc_mac[4], pst_hash_ipv4->auc_mac[5]);
#endif
                oal_rw_lock_write_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
                oal_dlist_delete_entry(&pst_hash_ipv4->st_entry);
                OAL_MEM_FREE(pst_hash_ipv4, OAL_TRUE);
                pst_hmac_vap->pst_vap_proxysta->uc_map_ipv4_num --;
                oal_rw_lock_write_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
                oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            }
        }
    }

    for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_MAX_VALUE; ul_loop++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_temp, &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv6_head[ul_loop]))
        {
            pst_hash_ipv6 = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_ipv6_hash_stru, st_entry);
            ul_map_idle_time = (oal_uint32)OAL_TIME_GET_RUNTIME(pst_hash_ipv6->ul_last_active_timestamp, ul_present_time);
            if (HMAC_PROXYSTA_MAP_AGING_TIME < ul_map_idle_time)
            {
                oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
#ifdef _PRE_DEBUG_MODE
                OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_map_aging_timer:: remove one ipv6 map[%x:%x:xx:xx:xx:xx:%x:%x].}",
                    pst_hash_ipv6->auc_ipv6[0]*pst_hash_ipv6->auc_ipv6[1], pst_hash_ipv6->auc_ipv6[2]*pst_hash_ipv6->auc_ipv6[3],
                    pst_hash_ipv6->auc_ipv6[12]*pst_hash_ipv6->auc_ipv6[13], pst_hash_ipv6->auc_ipv6[14]*pst_hash_ipv6->auc_ipv6[15]);
                OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_map_aging_timer:: remove one ipv6 map[%x:%x:ff:ff:%x:%x].}",
                    pst_hash_ipv6->auc_mac[0], pst_hash_ipv6->auc_mac[1], pst_hash_ipv6->auc_mac[4], pst_hash_ipv6->auc_mac[5]);
#endif
                oal_rw_lock_write_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
                oal_dlist_delete_entry(&pst_hash_ipv6->st_entry);
                OAL_MEM_FREE(pst_hash_ipv6, OAL_TRUE);
                pst_hmac_vap->pst_vap_proxysta->uc_map_ipv6_num --;
                oal_rw_lock_write_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
                oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            }
        }
    }

    for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_UNKNOW_VALUE; ul_loop++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_temp, &(pst_hmac_vap->pst_vap_proxysta->ast_map_unknow_head[ul_loop]))
        {
            pst_hash_unknow = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_unknow_hash_stru, st_entry);
            ul_map_idle_time = (oal_uint32)OAL_TIME_GET_RUNTIME(pst_hash_unknow->ul_last_active_timestamp, ul_present_time);
            if (HMAC_PROXYSTA_MAP_AGING_TIME < ul_map_idle_time)
            {
                oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
#ifdef _PRE_DEBUG_MODE
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_map_aging_timer:: remove unknow map [%x].}", pst_hash_unknow->us_protocol);
                OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_map_aging_timer:: remove unknow map [%x:%x:ff:ff:%x:%x].}",
                    pst_hash_unknow->auc_mac[0], pst_hash_unknow->auc_mac[1], pst_hash_unknow->auc_mac[4], pst_hash_unknow->auc_mac[5]);
#endif
                oal_rw_lock_write_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
                oal_dlist_delete_entry(&pst_hash_unknow->st_entry);
                OAL_MEM_FREE(pst_hash_unknow, OAL_TRUE);
                pst_hmac_vap->pst_vap_proxysta->uc_map_unknow_num --;
                oal_rw_lock_write_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
                oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            }
        }
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);

    return OAL_SUCC;
}


oal_uint32  hmac_proxysta_init_vap(hmac_vap_stru *pst_hmac_vap, mac_cfg_add_vap_param_stru *pst_param)
{
    oal_uint32          ul_loop = 0;
    hmac_device_stru    *pst_hmac_device = OAL_PTR_NULL;

    /* 入参空指针判断 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_init_vap::null param: hmac_vap[%d] param[%d]!}",
            pst_hmac_vap, pst_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap->en_proxysta_mode = pst_param->en_proxysta_mode;
    /* repeater模式下 仅proxysta才进行指针内存申请 AP模式不需要map表 */
    if (PROXYSTA_MODE_SSTA == pst_param->en_proxysta_mode)
    {
        pst_hmac_vap->pst_vap_proxysta = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_vap_proxysta_stru), OAL_TRUE);
        if (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta)
        {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                "hmac_proxysta_init_vap malloc error: pointer null!");
            return OAL_ERR_CODE_PTR_NULL;
        }
        oal_rw_lock_init(&pst_hmac_vap->pst_vap_proxysta->st_lock);
        /* 初始化链表 */
        for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_MAX_VALUE; ul_loop++)
        {
            oal_dlist_init_head(&(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv4_head[ul_loop]));
        }
        pst_hmac_vap->pst_vap_proxysta->uc_map_ipv4_num = 0;
        for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_MAX_VALUE; ul_loop++)
        {
            oal_dlist_init_head(&(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv6_head[ul_loop]));
        }
        pst_hmac_vap->pst_vap_proxysta->uc_map_ipv6_num = 0;
        for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_UNKNOW_VALUE; ul_loop++)
        {
            oal_dlist_init_head(&(pst_hmac_vap->pst_vap_proxysta->ast_map_unknow_head[ul_loop]));
        }
        pst_hmac_vap->pst_vap_proxysta->uc_map_unknow_num = 0;
        /* 设定一个定时器，定时对proxysta的map表格进行老化 */
        /* 获取hmac device和扫描运行记录 */
        pst_hmac_device = hmac_res_get_mac_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
        if (OAL_PTR_NULL == pst_hmac_device)
        {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                "{hmac_proxysta_init_vap::null param: pst_hmac_device[%d]!}", pst_hmac_device);
            return OAL_ERR_CODE_PTR_NULL;
        }
        /* 启用map keepalive定时器, 若定时器已开启, 则不用再开启 */
        if (!pst_hmac_device->st_proxysta_map_timer.en_is_registerd)
        {
            FRW_TIMER_CREATE_TIMER(&(pst_hmac_device->st_proxysta_map_timer),
                                   hmac_proxysta_map_aging_timer,
                                   HMAC_PROXYSTA_MAP_AGING_TRIGGER_TIME,                /* 60s触发一次 */
                                   pst_hmac_vap,
                                   OAL_TRUE,
                                   OAM_MODULE_ID_HMAC,
                                   pst_hmac_device->pst_device_base_info->ul_core_id);
        }
    }
    return OAL_SUCC;
}


oal_uint32  hmac_proxysta_exit_vap(hmac_vap_stru *pst_hmac_vap)
{
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;
    hmac_proxysta_ipv4_hash_stru    *pst_hash_ipv4 = OAL_PTR_NULL;
    hmac_proxysta_ipv6_hash_stru    *pst_hash_ipv6 = OAL_PTR_NULL;
    hmac_proxysta_unknow_hash_stru  *pst_hash_unknow = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_dlist_tmp = OAL_PTR_NULL;
    hmac_device_stru                *pst_hmac_device = OAL_PTR_NULL;
    oal_uint32                      ul_loop = 0;

    /* 入参空指针判断 */
    if (OAL_PTR_NULL == pst_hmac_vap )
    {
        OAM_ERROR_LOG1(0, OAM_SF_PROXYSTA, "{hmac_proxysta_exit_vap::null param: hmac_vap[%d]!}", pst_hmac_vap);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 仅PROXYSTA有map表需要释放内存  指针为空则不处理 */
    if ( (PROXYSTA_MODE_SSTA != pst_hmac_vap->en_proxysta_mode) ||
        (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta) )
    {
        return OAL_SUCC;
    }
    /* 关掉hmac_device下的定时器 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_exit_vap::null param: pst_hmac_device[%d]!}", pst_hmac_device);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (pst_hmac_device->st_proxysta_map_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_device->st_proxysta_map_timer);
    }
    /* 删除链表 */
    for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_MAX_VALUE; ul_loop++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv4_head[ul_loop]))
        {
            pst_hash_ipv4 = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_ipv4_hash_stru, st_entry);
            oal_dlist_delete_entry(pst_entry);
            OAL_MEM_FREE(pst_hash_ipv4, OAL_TRUE);
        }
    }
    pst_hmac_vap->pst_vap_proxysta->uc_map_ipv4_num = 0;
    for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_MAX_VALUE; ul_loop++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv6_head[ul_loop]))
        {
            pst_hash_ipv6 = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_ipv6_hash_stru, st_entry);
            oal_dlist_delete_entry(pst_entry);
            OAL_MEM_FREE(pst_hash_ipv6, OAL_TRUE);
        }
    }
    pst_hmac_vap->pst_vap_proxysta->uc_map_ipv6_num = 0;
    for (ul_loop = 0; ul_loop < MAC_VAP_PROXYSTA_MAP_UNKNOW_VALUE; ul_loop++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_hmac_vap->pst_vap_proxysta->ast_map_unknow_head[ul_loop]))
        {
            pst_hash_unknow = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_unknow_hash_stru, st_entry);
            oal_dlist_delete_entry(pst_entry);
            OAL_MEM_FREE(pst_hash_unknow, OAL_TRUE);
        }
    }
    pst_hmac_vap->pst_vap_proxysta->uc_map_unknow_num = 0;

    OAL_MEM_FREE(pst_hmac_vap->pst_vap_proxysta, OAL_TRUE);
    pst_hmac_vap->pst_vap_proxysta = OAL_PTR_NULL;

    return OAL_SUCC;
}


OAL_STATIC oal_uint16  hmac_proxysta_cal_checksum(oal_uint16   us_protocol,
                                                 oal_uint16   us_len,
                                                 oal_uint8    auc_src_addr[],
                                                 oal_uint8    auc_dest_addr[],
                                                 oal_uint16   us_addrleninbytes,
                                                 oal_uint8   *puc_buff)
{
    oal_uint16 us_pad = 0;
    oal_uint16 us_word16;
    oal_uint32 ul_sum = 0;
    oal_int    l_loop;

    if (us_len & 1)
    {
        us_len -= 1;
        us_pad  = 1;
    }

    for (l_loop = 0; l_loop < us_len; l_loop = l_loop + 2)
    {
        us_word16 = puc_buff[l_loop];
        us_word16 = (oal_uint16)((us_word16 << 8) + puc_buff[l_loop+1]);
        ul_sum = ul_sum + (oal_uint32)us_word16;
    }

    if (us_pad)
    {
        us_word16 = puc_buff[us_len];
        us_word16 <<= 8;
        ul_sum = ul_sum + (oal_uint32)us_word16;
    }

    for (l_loop = 0; l_loop < us_addrleninbytes; l_loop = l_loop + 2)
    {
        us_word16 = auc_src_addr[l_loop];
        us_word16 = (oal_uint16)((us_word16 << 8) + auc_src_addr[l_loop + 1]);
        ul_sum = ul_sum + (oal_uint32)us_word16;
    }


    for (l_loop = 0; l_loop < us_addrleninbytes; l_loop = l_loop + 2)
    {
        us_word16 = auc_dest_addr[l_loop];
        us_word16 = (oal_uint16)((us_word16 << 8) + auc_dest_addr[l_loop + 1]);
        ul_sum = ul_sum + (oal_uint32)us_word16;
    }

    ul_sum = ul_sum + (oal_uint32)us_protocol + (oal_uint32)(us_len+us_pad);

    while (ul_sum >> 16)
    {
        ul_sum = (ul_sum & 0xFFFF) + (ul_sum >> 16);
    }

    ul_sum = ~ul_sum;

    return ((oal_uint16) ul_sum);
}


OAL_STATIC oal_uint8 *hmac_proxysta_find_tlv(oal_uint8 *puc_buff,
                                             oal_uint16 us_buff_len,
                                             oal_uint8 uc_type,
                                             oal_uint8 uc_length)
{
    if (OAL_PTR_NULL == puc_buff)
    {
        return OAL_PTR_NULL;
    }
    while (us_buff_len > 0)
    {
        /* 如果data值异常，导致length值为0 就会出现死循环 此类情况直接返回空指针 */
        if(*(puc_buff+1) == 0)
        {
            return OAL_PTR_NULL;
        }
        /* type和length值 一致，则找到TLV字段 且TLV的长度信息需正确  length字段是以8字节为单位的 */
        if ((*puc_buff == uc_type) && (*(puc_buff+1) == uc_length) && (us_buff_len >= uc_length*8))
        {
            return puc_buff;
        }
        us_buff_len -= (*(puc_buff+1))*8;
        puc_buff += (*(puc_buff+1))*8;
    }
    /* 没找到则返回空指针 */
    return OAL_PTR_NULL;
}


OAL_STATIC oal_uint32  hmac_proxysta_find_ipv4_mac(hmac_vap_stru *pst_hmac_vap,
                                                   oal_uint8 *puc_ip_addr,
                                                   oal_uint8 *puc_mac_addr)
{
    oal_uint8   uc_hash = 0;
    hmac_proxysta_ipv4_hash_stru    *pst_hash_ipv4 = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;

    if ( (OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == puc_ip_addr) )
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_find_ipv4_mac:: null param, mac_vap:%d ip_addr:%d.}",
            pst_hmac_vap, puc_ip_addr);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_find_ipv4_mac:: vap_proxysta  null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取HASH桶值 以及HASH链表 */
    uc_hash = (oal_uint8)HMAC_PROXYSTA_CAL_IPV4_HASH(puc_ip_addr);
    /* 查找链表 获取实际的MAC地址 */
    oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv4_head[uc_hash]))
    {
        pst_hash_ipv4 = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_ipv4_hash_stru, st_entry);
        /* 比较IP地址 */
        if (HMAC_PROXYSTA_MEMCMP_EQUAL == OAL_MEMCMP(pst_hash_ipv4->auc_ipv4, puc_ip_addr, ETH_TARGET_IP_ADDR_LEN))
        {
            oal_memcopy(puc_mac_addr, pst_hash_ipv4->auc_mac, WLAN_MAC_ADDR_LEN);
            /* 找到后刷新下MAP表格的时间戳 */
            pst_hash_ipv4->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            return OAL_SUCC;
        }
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    /* 遍历完成后未找到 返回失败 */
    return OAL_FAIL;
}


OAL_STATIC oal_uint32  hmac_proxysta_find_ipv6_mac(hmac_vap_stru *pst_hmac_vap,
                                                   oal_uint8 *puc_ip_addr,
                                                   oal_uint8 *puc_mac_addr)
{
    oal_uint8   uc_hash = 0;
    hmac_proxysta_ipv6_hash_stru    *pst_hash_ipv6 = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;

    if ( (OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == puc_ip_addr) )
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_find_ipv6_mac:: null param, mac_vap:%d ip_addr:%d.}",
            pst_hmac_vap, puc_ip_addr);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_find_ipv6_mac:: vap_proxysta  null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取HASH桶值 以及HASH链表 */
    uc_hash = (oal_uint8)HMAC_PROXYSTA_CAL_IPV6_HASH(puc_ip_addr);
    /* 查找链表 获取实际的MAC地址 */
    oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv6_head[uc_hash]))
    {
        pst_hash_ipv6 = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_ipv6_hash_stru, st_entry);
        /* 比较IP地址 */
        if (HMAC_PROXYSTA_MEMCMP_EQUAL == OAL_MEMCMP(pst_hash_ipv6->auc_ipv6, puc_ip_addr, HMAC_PROXYSTA_IPV6_ADDR_LEN))
        {
            oal_memcopy(puc_mac_addr, pst_hash_ipv6->auc_mac, WLAN_MAC_ADDR_LEN);
            /* 找到后刷新下MAP表格的时间戳 */
            pst_hash_ipv6->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            return OAL_SUCC;
        }
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    /* 遍历完成后未找到 返回失败 */
    return OAL_FAIL;
}


OAL_STATIC oal_uint32  hmac_proxysta_find_unknow_mac(hmac_vap_stru *pst_hmac_vap,
                                                     oal_uint16 us_protocol,
                                                     oal_uint8 *puc_mac_addr)
{
    oal_uint8   uc_hash = 0;
    hmac_proxysta_unknow_hash_stru      *pst_hash_unknow = OAL_PTR_NULL;
    oal_dlist_head_stru                 *pst_entry = OAL_PTR_NULL;

    if ( OAL_PTR_NULL == pst_hmac_vap )
    {
        OAM_ERROR_LOG1(0, OAM_SF_PROXYSTA, "{hmac_proxysta_find_ipv4_mac:: null param, mac_vap:%d.}",
            pst_hmac_vap);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_find_unknow_mac:: vap_proxysta  null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取HASH桶值 以及HASH链表 */
    uc_hash = (oal_uint8)HMAC_PROXYSTA_CAL_UNKNOW_HASH(us_protocol);
    /* 查找链表 获取实际的MAC地址 */
    oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->pst_vap_proxysta->ast_map_unknow_head[uc_hash]))
    {
        pst_hash_unknow = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_unknow_hash_stru, st_entry);
        /* 比较IP地址 */
        if (pst_hash_unknow->us_protocol == us_protocol)
        {
            oal_memcopy(puc_mac_addr, pst_hash_unknow->auc_mac, WLAN_MAC_ADDR_LEN);
            /* 找到后刷新下MAP表格的时间戳 */
            pst_hash_unknow->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            return OAL_SUCC;
        }
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    /* 遍历完成后未找到 返回失败 */
    return OAL_FAIL;
}


OAL_STATIC oal_uint32 hmac_proxysta_rx_arp_addr_replace(hmac_vap_stru *pst_hmac_vap,
                                                        mac_ether_header_stru *pst_ether_header,
                                                        oal_uint32 ul_pkt_len)
{
    oal_eth_arphdr_stru *pst_arp          = OAL_PTR_NULL;
    oal_uint8           *puc_eth_body     = OAL_PTR_NULL;
    oal_uint8           *puc_des_mac      = OAL_PTR_NULL;
    oal_bool_enum_uint8 en_is_mcast;
    oal_uint32          ul_contig_len;
    oal_uint8           *puc_oma          = OAL_PTR_NULL;
    oal_uint32          ul_ret = OAL_SUCC;

    /***************************************************************************/
    /*                      ARP Frame Format                                   */
    /* ----------------------------------------------------------------------- */
    /* |以太网目的地址|以太网源地址|帧类型|硬件类型|协议类型|硬件地址长度|     */
    /* ----------------------------------------------------------------------- */
    /* | 6 (待替换)   |6           |2     |2       |2       |1           |     */
    /* ----------------------------------------------------------------------- */
    /* |协议地址长度|op|发送端以太网地址|发送端IP地址|目的以太网地址|目的IP地址*/
    /* ----------------------------------------------------------------------- */
    /* | 1          |2 |6               |4           |6 (待替换)    |4         */
    /* ----------------------------------------------------------------------- */
    /*                                                                         */
    /***************************************************************************/

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ether_header))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA,
                       "{hmac_proxysta_rx_arp_addr_replace:: null param, mac_vap:%d ether_header:%d.}",
                       pst_hmac_vap, pst_ether_header);
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    /* 获取以太网目的mac和数据段 */
    puc_des_mac = pst_ether_header->auc_ether_dhost;
    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);

    /* 获取以太网帧目的地址是否为多播地址 */
    en_is_mcast = ETHER_IS_MULTICAST(puc_des_mac);
    /* ARP 包地址转换 */
    pst_arp = (oal_eth_arphdr_stru *)puc_eth_body;
    ul_contig_len += OAL_SIZEOF(oal_eth_arphdr_stru);

    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                       "{hmac_proxysta_rx_arp_addr_replace::The length of buf is invalid.}");
        return OAL_FAIL;
    }
    /*  获取报文中目的IP地址 解析MAP表 获取真实目的MAC地址 */
    puc_oma = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WLAN_MAC_ADDR_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == puc_oma)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_arp_addr_replace::mem alloc oma ptr null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_ret = hmac_proxysta_find_ipv4_mac(pst_hmac_vap, pst_arp->auc_ar_tip, puc_oma);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_psta_rx_arp_match_addr:: can't find mac addr of ip:%d:%d:%d:%d.}",
            pst_arp->auc_ar_tip[0], pst_arp->auc_ar_tip[1], pst_arp->auc_ar_tip[2], pst_arp->auc_ar_tip[3]);
        /* 释放内存 */
        OAL_MEM_FREE(puc_oma, OAL_TRUE);
        puc_oma = OAL_PTR_NULL;
        return ul_ret;
    }

    /* 替换arp报文中的mac地址为真实的MAC地址 */
    oal_set_mac_addr(pst_arp->auc_ar_tha, puc_oma);
    /* 组播或者广播报文地址替换 只需要替换APR报文的目的地址即可 */
    /* 单播报文需要替换ARP的报文的目的地址和以太网目的地址 */
    if (!en_is_mcast)
    {
        oal_set_mac_addr(puc_des_mac, puc_oma);
    }

    /* 释放内存 */
    OAL_MEM_FREE(puc_oma, OAL_TRUE);
    puc_oma = OAL_PTR_NULL;
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_proxysta_rx_ip_addr_replace(hmac_vap_stru *pst_hmac_vap,
                                                       mac_ether_header_stru *pst_ether_header,
                                                       oal_uint32 ul_pkt_len)
{
    oal_ip_header_stru   *pst_ip_header    = OAL_PTR_NULL;
    oal_udp_header_stru  *pst_udp_header   = OAL_PTR_NULL;
    oal_dhcp_packet_stru *pst_dhcp_packet  = OAL_PTR_NULL;
    oal_uint8            *puc_eth_body     = OAL_PTR_NULL;
    oal_uint8            *puc_des_mac      = OAL_PTR_NULL;
    oal_uint16           us_ip_header_len = 0;
    oal_uint32           ul_contig_len = 0;
    oal_uint32           ul_ret = OAL_SUCC;
    oal_uint8            *puc_oma          = OAL_PTR_NULL;
    oal_bool_enum_uint8  en_is_mcast = OAL_FALSE;
    oal_uint8            *puc_ipv4  = OAL_PTR_NULL;
    oal_uint16           us_old_flag = 0;
    oal_uint32           ul_new_sum = 0;

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ether_header))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_rx_ip_addr_replace::null param vap:%d ether_header:%d.}",
            pst_hmac_vap, pst_ether_header);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    /* 获取以太网目的mac和数据段 */
    puc_des_mac = pst_ether_header->auc_ether_dhost;
    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);
    /* 获取以太网帧目的地址是否为多播地址 */
    en_is_mcast = ETHER_IS_MULTICAST(puc_des_mac);
    /*************************************************************************/
    /*                      DHCP Frame Format                                */
    /* --------------------------------------------------------------------- */
    /* |以太网头        |   IP头         | UDP头           |DHCP报文       | */
    /* --------------------------------------------------------------------- */
    /* | 14             |20              |8                | 不定          | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    pst_ip_header = (oal_ip_header_stru *)puc_eth_body;

    /*************************************************************************/
    /*                    IP头格式 (oal_ip_header_stru)                      */
    /* --------------------------------------------------------------------- */
    /* | 版本 | 报头长度 | 服务类型 | 总长度  |标识  |标志  |段偏移量 |      */
    /* --------------------------------------------------------------------- */
    /* | 4bits|  4bits   | 1        | 2       | 2    |3bits | 13bits  |      */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | 生存期 | 协议        | 头部校验和| 源地址（SrcIp）|目的地址（DstIp）*/
    /* --------------------------------------------------------------------- */
    /* | 1      |  1 (17为UDP)| 2         | 4              | 4               */
    /* --------------------------------------------------------------------- */
    /*************************************************************************/

    ul_contig_len += OAL_SIZEOF(oal_ip_header_stru);
    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                       "{hmac_proxysta_rx_ip_addr_replace::The length of buf is invalid.}");
        return OAL_FAIL;
    }
    /* IP报文头长度 */
    us_ip_header_len = pst_ip_header->us_ihl * 4;

    /* 如果是UDP包，并且是DHCP协议的报文地址转换 */
    if (OAL_IPPROTO_UDP == pst_ip_header->uc_protocol)
    {
        pst_udp_header  = (oal_udp_header_stru *)((oal_uint8 *)pst_ip_header + us_ip_header_len);
        ul_contig_len += OAL_SIZEOF(oal_udp_header_stru);
        if (ul_pkt_len < ul_contig_len)
        {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                "{hmac_proxysta_rx_ip_addr_replace::The length of buf is invalid.}");
            return OAL_FAIL;
        }
        /*************************************************************************/
        /*                      UDP 头 (oal_udp_header_stru)                     */
        /* --------------------------------------------------------------------- */
        /* |源端口号（SrcPort）|目的端口号（DstPort）| UDP长度    | UDP检验和  | */
        /* --------------------------------------------------------------------- */
        /* | 2                 | 2                   |2           | 2          | */
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/
        /* DHCP request UDP Client SP = 68 (bootpc), DP = 67 (bootps) */
        /* Repeater STA发送的DHCP REQUEST报文中要求DHCP SERVER以广播形式发送ACK报文
           经由REPEATER发送的DHCP应答报文不会是单播报文 故不区分单播报文 */
/*lint -e778*/
        if ( DHCP_PORT_BOOTPS == OAL_HOST2NET_SHORT(pst_udp_header->dest) )
/*lint +e778*/
        {
            pst_dhcp_packet = (oal_dhcp_packet_stru *)(((oal_uint8 *)pst_udp_header) + OAL_SIZEOF(oal_udp_header_stru));
            ul_contig_len += OAL_SIZEOF(oal_dhcp_packet_stru);
            if (ul_pkt_len < ul_contig_len)
            {
                return OAL_FAIL;
            }
            /* 客户端发过来的DHCP请求报文 更改标志字段要求服务器以广播形式发送ACK 如果是自己的DHCP则不更改 要求服务器以单播形式回复 */
            /* STA的应用场景应该不会接收到DHCP REQUEST(除非DHCP服务器部署在REPEATER上，家用场景稀奇)，
                更不可能收到自己的DHCP REQUEST 不过写上也没关系，说不定后面能用到呢 */
            if (OAL_MEMCMP(mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info), pst_dhcp_packet->chaddr, WLAN_MAC_ADDR_LEN))
            {
                us_old_flag = pst_dhcp_packet->flags;
/*lint -e778*/
                pst_dhcp_packet->flags = OAL_NET2HOST_SHORT(DHCP_FLAG_BCAST);
/*lint +e778*/
                /* 修改后重新计算UDP的校验和 */
                ul_new_sum = (oal_uint32)pst_udp_header->check;
                ul_new_sum += us_old_flag + (~(pst_dhcp_packet->flags) & 0XFFFF);
                ul_new_sum  = (ul_new_sum >> 16) + (ul_new_sum & 0XFFFF);
                pst_udp_header->check = (oal_uint16)((ul_new_sum >> 16) + ul_new_sum);
            }
            return OAL_SUCC;
        }
    }

    /* 其它类型IP报文以及不是DHCP的其它UDP包 的地址转换处理 */
    /* 需要更新以太网目的地址为proxysta的oma */
    if (en_is_mcast)      /* 多播报文不需要替换目的地址 */
    {
        return OAL_SUCC;
    }
    puc_oma = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WLAN_MAC_ADDR_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == puc_oma)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_ip_addr_replace::mem alloc oma ptr null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_ret = hmac_proxysta_find_ipv4_mac(pst_hmac_vap, (oal_uint8 *)(&pst_ip_header->ul_daddr), puc_oma);
    if (OAL_SUCC != ul_ret)
    {
        puc_ipv4 = (oal_uint8 *)(&pst_ip_header->ul_daddr);
        OAM_ERROR_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_ip_addr_replace:: can't find mac addr of ip:%d:%d:%d:%d.}",
            puc_ipv4[0], puc_ipv4[1], puc_ipv4[2], puc_ipv4[3]);
        /* 释放内存 */
        OAL_MEM_FREE(puc_oma, OAL_TRUE);
        puc_oma = OAL_PTR_NULL;
        return ul_ret;
    }

    /* 更新以太网目的地址为proxysta的vma */
    oal_set_mac_addr(puc_des_mac, puc_oma);
    /* 释放内存 */
    OAL_MEM_FREE(puc_oma, OAL_TRUE);
    puc_oma = OAL_PTR_NULL;
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_proxysta_rx_ipv6_addr_replace(hmac_vap_stru *pst_hmac_vap,
                                                         mac_ether_header_stru *pst_ether_header,
                                                         oal_uint32 ul_pkt_len)
{
    oal_ipv6_header_stru    *pst_ipv6_hdr_data = OAL_PTR_NULL;
    oal_uint8           *puc_eth_body     = OAL_PTR_NULL;
    oal_uint8           *puc_des_mac      = OAL_PTR_NULL;
    oal_bool_enum_uint8 en_is_mcast = OAL_FALSE;
    oal_uint32          ul_contig_len = 0;
    oal_uint8           *puc_oma          = OAL_PTR_NULL;
    oal_uint32          ul_ret = OAL_SUCC;
    oal_uint16          *pus_ipv6 = OAL_PTR_NULL;

    /*************************************************************************/
    /*                      IPV6 Frame Format                                */
    /* --------------------------------------------------------------------- */
    /* |以太网头        |   IPV6头         | Next Frame Boady              | */
    /* --------------------------------------------------------------------- */
    /* | 14             |   40             |  不定                         | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ether_header))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA,
                       "{hmac_proxysta_rx_ipv6_addr_replace:: null param, mac_vap:%d ether_header:%d.}",
                       pst_hmac_vap, pst_ether_header);
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    /* 获取以太网目的mac和数据段 */
    puc_des_mac = pst_ether_header->auc_ether_dhost;
    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);

    /* 获取以太网帧目的地址是否为多播地址 */
    en_is_mcast = ETHER_IS_MULTICAST(puc_des_mac);
    /* 广播报文不需要替换MAC地址，直接返回 */
    if (en_is_mcast)
    {
        return OAL_SUCC;
    }
    /*************************************************************************/
    /*                   IPV6头格式 (oal_ipv6_header_stru)                   */
    /* --------------------------------------------------------------------- */
    /* | 版本 | Traffic Class |                 Flow label            |      */
    /* --------------------------------------------------------------------- */
    /* | 4bits|  8bits        |                 20bits                |      */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | Payload Length             | Next Header    |   Hop Limit    |      */
    /* --------------------------------------------------------------------- */
    /* | 2                          | 1              |   1            |      */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |                           Source Address                            */
    /* --------------------------------------------------------------------- */
    /* |                                 16                                  */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |                           Destination Address                       */
    /* --------------------------------------------------------------------- */
    /* |                                 16                                  */
    /* --------------------------------------------------------------------- */
    /*************************************************************************/
    pst_ipv6_hdr_data = (oal_ipv6_header_stru *)puc_eth_body;
    ul_contig_len += OAL_SIZEOF(oal_ipv6_header_stru);

    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                       "{hmac_proxysta_rx_ipv6_addr_replace::The length of buf is invalid.}");
        return OAL_FAIL;
    }

    /*  获取报文中目的IP地址 解析MAP表 获取真实目的MAC地址 */
    puc_oma = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WLAN_MAC_ADDR_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == puc_oma)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_ipv6_addr_replace::mem alloc oma ptr null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_ret = hmac_proxysta_find_ipv6_mac(pst_hmac_vap, pst_ipv6_hdr_data->st_daddr.auc_ipv6_union_addr, puc_oma);
    if (OAL_SUCC != ul_ret)
    {
        pus_ipv6 = pst_ipv6_hdr_data->st_daddr.aus_ipv6_union_addr;
        OAM_ERROR_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_ipv6_addr_replace:: can't find mac addr of ip:%d:%d:%d:%d.}",
            pus_ipv6[0], pus_ipv6[1], pus_ipv6[2], pus_ipv6[3]);
        OAM_ERROR_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_ipv6_addr_replace:: can't find mac addr of ip:%d:%d:%d:%d.}",
            pus_ipv6[4], pus_ipv6[5], pus_ipv6[6], pus_ipv6[7]);
        /* 释放内存 */
        OAL_MEM_FREE(puc_oma, OAL_TRUE);
        puc_oma = OAL_PTR_NULL;
        return ul_ret;
    }

    /* 替换以太网的目的MAC */
    oal_set_mac_addr(puc_des_mac, puc_oma);
    /* 释放内存 */
    OAL_MEM_FREE(puc_oma, OAL_TRUE);
    puc_oma = OAL_PTR_NULL;
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_proxysta_rx_unknow_addr_replace(hmac_vap_stru *pst_hmac_vap,
                                                           mac_ether_header_stru *pst_ether_header,
                                                           oal_uint32 ul_pkt_len)
{
    oal_uint8            *puc_des_mac      = OAL_PTR_NULL;
    oal_uint32           ul_contig_len = 0;
    oal_uint32           ul_ret = OAL_SUCC;
    oal_uint8            *puc_oma           = OAL_PTR_NULL;
    oal_bool_enum_uint8  en_is_mcast = OAL_FALSE;

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ether_header))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_rx_unknow_addr_replace::null param vap:%d ether_header:%d device:%d.}",
            pst_hmac_vap, pst_ether_header);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_unknow_addr_replace::::The length of buf:%d is less than mac_ether_header_stru.}", ul_pkt_len);
        return OAL_FAIL;
    }

    /* 获取以太网目的mac和数据段 */
    puc_des_mac = pst_ether_header->auc_ether_dhost;
    /* 获取以太网帧目的地址是否为多播地址 */
    en_is_mcast = ETHER_IS_MULTICAST(puc_des_mac);
    if (en_is_mcast)      /* 多播报文不需要替换目的地址 */
    {
        return OAL_SUCC;
    }
    puc_oma = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, WLAN_MAC_ADDR_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == puc_oma)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_unknow_addr_replace::mem alloc oma ptr null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_ret = hmac_proxysta_find_unknow_mac(pst_hmac_vap, pst_ether_header->us_ether_type , puc_oma);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_rx_unknow_addr_replace:: can't find mac addr of unknow protocol:0x%x.}",
            pst_ether_header->us_ether_type);
        /* 释放内存 */
        OAL_MEM_FREE(puc_oma, OAL_TRUE);
        puc_oma = OAL_PTR_NULL;
        return ul_ret;
    }
    /* 更新以太网目的地址为实际的STA MAC地址 */
    oal_set_mac_addr(puc_des_mac, puc_oma);

    /* 释放内存 */
    OAL_MEM_FREE(puc_oma, OAL_TRUE);
    puc_oma = OAL_PTR_NULL;
    return OAL_SUCC;
}


oal_uint32  hmac_proxysta_rx_process(oal_netbuf_stru *pst_buf, hmac_vap_stru *pst_hmac_vap)
{
    mac_ether_header_stru   *pst_ether_header;
    oal_uint16               us_ether_type;
    oal_uint32              ul_pkt_len = 0;
    oal_uint32              ul_ret = OAL_SUCC;

    if ((OAL_PTR_NULL == pst_buf) || (OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_rx_process:null ptr pst_buf=%x pst_hmac_vap=%x.}",
            pst_buf, pst_hmac_vap);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 将skb的data指针指向以太网的帧头 */
    oal_netbuf_push(pst_buf, ETHER_HDR_LEN);
    pst_ether_header = (mac_ether_header_stru *)OAL_NETBUF_HEADER(pst_buf);
    ul_pkt_len = OAL_NETBUF_LEN(pst_buf);
    /* 还原skb的data指针 */
    oal_netbuf_pull(pst_buf, ETHER_HDR_LEN);
    /* 获取以太网报文的数据 PROXYSTA将根据报文类型将数据的目的地址进行替换 */
    /******************************************/
    /*        Ethernet Frame Format           */
    /* -------------------------------------  */
    /* |Dst      MAC | Source MAC   | TYPE |  */
    /* -------------------------------------  */
    /* | 6           | 6            | 2    |  */
    /* -------------------------------------  */
    /*                                        */
    /******************************************/
    us_ether_type = pst_ether_header->us_ether_type;

    /* ether_type 小于0x0600非协议报文 不处理 */
/*lint -e778*/
    if (HMAC_ETHERTYPE_PROTOCOL_START > OAL_HOST2NET_SHORT(us_ether_type) )
/*lint +e778*/
    {
        return OAL_SUCC;
    }
    /* IP包地址转换 */
/*lint -e778*/
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == us_ether_type)
/*lint +e778*/
    {
        ul_ret = hmac_proxysta_rx_ip_addr_replace(pst_hmac_vap, pst_ether_header, ul_pkt_len);
        return ul_ret;
    }
    /* ARP 包地址转换 */
/*lint -e778*/
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_ARP) == us_ether_type)
/*lint +e778*/
    {
        ul_ret = hmac_proxysta_rx_arp_addr_replace(pst_hmac_vap, pst_ether_header, ul_pkt_len);
        return ul_ret;
    }
    /*icmpv6 包地址转换 */
/*lint -e778*/
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6) == us_ether_type)
/*lint +e778*/
    {
        ul_ret = hmac_proxysta_rx_ipv6_addr_replace(pst_hmac_vap, pst_ether_header, ul_pkt_len);
        return ul_ret;
    }
    /* IPX 报文地址替换 */
/*lint -e778*/
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPX) == us_ether_type)
/*lint +e778*/
    {
        /* TO BE DONE */
        return ul_ret;
    }
    /* APPLE TALK  & AARP报文地址替换 */
/*lint -e778*/
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_AARP) == us_ether_type)
/*lint +e778*/
    {
        /* TO BE DONE */
        return ul_ret;
    }
    /* PPOE 报文地址替换 */
/*lint -e778*/
    else if ( (OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_DISC) == us_ether_type) ||
              (OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_SES) == us_ether_type) )
    {
/*lint +e778*/
        /* TO BE DONE */
        return ul_ret;
    }
    /* 此类报文协议不替换 0xe2ae 0xe2af为realtek处理的报文协议类型，类型未找到暂存 */
/*lint -e778*/
    else if ( (OAL_HOST2NET_SHORT(ETHER_TYPE_PAE) == us_ether_type) ||
              (OAL_HOST2NET_SHORT(0xe2ae) == us_ether_type) ||
              (OAL_HOST2NET_SHORT(0xe2af) == us_ether_type) )
/*lint +e778*/
    {
        return OAL_SUCC;
    }
    /* 其他未知类型的地址替换 */
    else
    {
        ul_ret = hmac_proxysta_rx_unknow_addr_replace(pst_hmac_vap, pst_ether_header, ul_pkt_len);
        return ul_ret;
    }
}


OAL_STATIC oal_uint32  hmac_proxysta_insert_ipv4_mac(hmac_vap_stru *pst_hmac_vap,
                                                     oal_uint8 *puc_ip_addr,
                                                     oal_uint8 *puc_src_mac)
{
    oal_uint8   uc_hash = 0;
    hmac_proxysta_ipv4_hash_stru    *pst_hash_ipv4 = OAL_PTR_NULL;
    hmac_proxysta_ipv4_hash_stru    *pst_hash_ipv4_new = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;

    if ( (OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == puc_ip_addr) || (OAL_PTR_NULL == puc_src_mac) )
    {
        OAM_ERROR_LOG3(0, OAM_SF_PROXYSTA, "{hmac_proxysta_insert_ipv4_mac:: null param, mac_vap:%d ip_addr:%d src_mac.}",
            pst_hmac_vap, puc_ip_addr, puc_src_mac);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_ipv4_mac:: vap_proxysta  null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取HASH桶值 以及HASH链表 */
    uc_hash = HMAC_PROXYSTA_CAL_IPV4_HASH(puc_ip_addr);
    /* 查找链表 先查询当前的MAP表中是否存在当前IP */
    oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv4_head[uc_hash]))
    {
        pst_hash_ipv4 = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_ipv4_hash_stru, st_entry);
        /* 比较IP地址 找到IP后在对比下MAC地址是否一致 */
        if (HMAC_PROXYSTA_MEMCMP_EQUAL == OAL_MEMCMP(pst_hash_ipv4->auc_ipv4, puc_ip_addr, ETH_TARGET_IP_ADDR_LEN))
        {
            /* IP对应的MAC不一致 需要刷新MAC */
            if (HMAC_PROXYSTA_MEMCMP_EQUAL != OAL_MEMCMP(pst_hash_ipv4->auc_mac, puc_src_mac, WLAN_MAC_ADDR_LEN))
            {
                oal_set_mac_addr(pst_hash_ipv4->auc_mac, puc_src_mac);
            }
            /* 刷新下MAP表格的时间戳 */
            pst_hash_ipv4->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            return OAL_SUCC;
        }
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);

    /* 遍历完成后未找到 重新申请内存并将节点插入到MAP表格中 */
    /* 先查看表格数量，如果超过128个则不再新建 */
    if (HMAC_PROXYSTA_MAP_MAX_NUM < (pst_hmac_vap->pst_vap_proxysta->uc_map_ipv4_num +
                                     pst_hmac_vap->pst_vap_proxysta->uc_map_ipv6_num +
                                     pst_hmac_vap->pst_vap_proxysta->uc_map_unknow_num) )
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_ipv4_mac:: map num exceed max size: %d.}", HMAC_PROXYSTA_MAP_MAX_NUM);
        return OAL_SUCC;
    }
    pst_hash_ipv4_new = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_proxysta_ipv4_hash_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_hash_ipv4_new)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_ipv4_mac:: mem alloc null pointer.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 设置MAP的结构变量 */
    oal_memcopy(pst_hash_ipv4_new->auc_ipv4, puc_ip_addr, ETH_TARGET_IP_ADDR_LEN);
    oal_memcopy(pst_hash_ipv4_new->auc_mac, puc_src_mac, WLAN_MAC_ADDR_LEN);
    pst_hash_ipv4_new->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    /* 加入链表 */
    oal_rw_lock_write_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    oal_dlist_add_head(&(pst_hash_ipv4_new->st_entry), &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv4_head[uc_hash]));
    pst_hmac_vap->pst_vap_proxysta->uc_map_ipv4_num ++;
    oal_rw_lock_write_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
        "{hmac_proxysta_insert_ipv4_mac:: insert ipv4 map ip[%d:%d:%d:%d.}",
        puc_ip_addr[0], puc_ip_addr[1], puc_ip_addr[2], puc_ip_addr[3]);
    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
        "{hmac_proxysta_insert_ipv4_mac:: insert ipv4 map mac[%x:%x:ff:ff:%x:%x.}",
        puc_src_mac[0], puc_src_mac[1], puc_src_mac[4], puc_src_mac[5]);
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_proxysta_insert_ipv6_mac(hmac_vap_stru *pst_hmac_vap,
                                                     oal_uint8 *puc_ip_addr,
                                                     oal_uint8 *puc_src_mac)
{
    oal_uint8   uc_hash = 0;
    hmac_proxysta_ipv6_hash_stru    *pst_hash_ipv6 = OAL_PTR_NULL;
    hmac_proxysta_ipv6_hash_stru    *pst_hash_ipv6_new = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;

    if ( (OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == puc_ip_addr) || (OAL_PTR_NULL == puc_src_mac) )
    {
        OAM_ERROR_LOG3(0, OAM_SF_PROXYSTA, "{hmac_proxysta_insert_ipv6_mac:: null param, mac_vap:%d ip_addr:%d src_mac.}",
            pst_hmac_vap, puc_ip_addr, puc_src_mac);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_ipv6_mac:: vap_proxysta  null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取HASH桶值 以及HASH链表 */
    uc_hash = (oal_uint8)HMAC_PROXYSTA_CAL_IPV6_HASH(puc_ip_addr);
    /* 查找链表 先查询当前的MAP表中是否存在当前IP */
    oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv6_head[uc_hash]))
    {
        pst_hash_ipv6 = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_ipv6_hash_stru, st_entry);
        /* 比较IP地址 找到IP后在对比下MAC地址是否一致 */
        if (HMAC_PROXYSTA_MEMCMP_EQUAL == OAL_MEMCMP(pst_hash_ipv6->auc_ipv6, puc_ip_addr, HMAC_PROXYSTA_IPV6_ADDR_LEN))
        {
            /* IP对应的MAC不一致 需要刷新MAC */
            if (HMAC_PROXYSTA_MEMCMP_EQUAL != OAL_MEMCMP(pst_hash_ipv6->auc_mac, puc_src_mac, WLAN_MAC_ADDR_LEN))
            {
                oal_set_mac_addr(pst_hash_ipv6->auc_mac, puc_src_mac);
            }
            /* 刷新下MAP表格的时间戳 */
            pst_hash_ipv6->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            return OAL_SUCC;
        }
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);

    /* 遍历完成后未找到 重新申请内存并将节点插入到MAP表格中 */
    /* 先查看表格数量，如果超过128个则不再新建 */
    if (HMAC_PROXYSTA_MAP_MAX_NUM < (pst_hmac_vap->pst_vap_proxysta->uc_map_ipv4_num +
                                     pst_hmac_vap->pst_vap_proxysta->uc_map_ipv6_num +
                                     pst_hmac_vap->pst_vap_proxysta->uc_map_unknow_num) )
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_ipv6_mac:: map num exceed max size: %d.}", HMAC_PROXYSTA_MAP_MAX_NUM);
        return OAL_SUCC;
    }
    pst_hash_ipv6_new = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_proxysta_ipv6_hash_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_hash_ipv6_new)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_ipv6_mac:: mem alloc null pointer.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 设置MAP的结构变量 */
    oal_memcopy(pst_hash_ipv6_new->auc_ipv6, puc_ip_addr, HMAC_PROXYSTA_IPV6_ADDR_LEN);
    oal_memcopy(pst_hash_ipv6_new->auc_mac, puc_src_mac, WLAN_MAC_ADDR_LEN);
    pst_hash_ipv6_new->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    /* 加入链表 */
    oal_rw_lock_write_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    oal_dlist_add_head(&(pst_hash_ipv6_new->st_entry), &(pst_hmac_vap->pst_vap_proxysta->ast_map_ipv6_head[uc_hash]));
    pst_hmac_vap->pst_vap_proxysta->uc_map_ipv6_num ++;
    oal_rw_lock_write_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
    "{hmac_proxysta_insert_ipv6_mac:: ipv6 map number:%d.}", pst_hmac_vap->pst_vap_proxysta->uc_map_ipv6_num);
    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
        "{hmac_proxysta_insert_ipv6_mac:: insert ipv6 map ip:0x%x-mac:0x%x.}",puc_ip_addr[15], puc_src_mac[5]);
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_proxysta_insert_unknow_mac(hmac_vap_stru *pst_hmac_vap,
                                                       oal_uint16 us_protocol,
                                                       oal_uint8 *puc_src_mac)
{
    oal_uint8   uc_hash = 0;
    hmac_proxysta_unknow_hash_stru      *pst_hash_unknow = OAL_PTR_NULL;
    hmac_proxysta_unknow_hash_stru      *pst_hash_unknow_new = OAL_PTR_NULL;
    oal_dlist_head_stru                 *pst_entry = OAL_PTR_NULL;

    if ( (OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == puc_src_mac) )
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_insert_unknow_mac:: null param, mac_vap:%d src_mac.}",
            pst_hmac_vap, puc_src_mac);
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == pst_hmac_vap->pst_vap_proxysta)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_unknow_mac:: vap_proxysta  null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取HASH桶值 以及HASH链表 */
    uc_hash = (oal_uint8)HMAC_PROXYSTA_CAL_UNKNOW_HASH(us_protocol);
    /* 查找链表 先查询当前的MAP表中是否存在当前IP */
    oal_rw_lock_read_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->pst_vap_proxysta->ast_map_unknow_head[uc_hash]))
    {
        pst_hash_unknow = OAL_DLIST_GET_ENTRY(pst_entry, hmac_proxysta_unknow_hash_stru, st_entry);
        /* 比较协议类型 找到协议后在对比下MAC地址是否一致 */
        if (pst_hash_unknow->us_protocol == us_protocol)
        {
            /* 协议对应的MAC不一致 需要刷新MAC */
            if (HMAC_PROXYSTA_MEMCMP_EQUAL != OAL_MEMCMP(pst_hash_unknow->auc_mac, puc_src_mac, WLAN_MAC_ADDR_LEN))
            {
                oal_set_mac_addr(pst_hash_unknow->auc_mac, puc_src_mac);
            }
            /* 刷新下MAP表格的时间戳 */
            pst_hash_unknow->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
            return OAL_SUCC;
        }
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);

    /* 遍历完成后未找到 重新申请内存并将节点插入到MAP表格中 */
    /* 先查看表格数量，如果超过128个则不再新建 */
    if (HMAC_PROXYSTA_MAP_MAX_NUM < (pst_hmac_vap->pst_vap_proxysta->uc_map_ipv4_num +
                                     pst_hmac_vap->pst_vap_proxysta->uc_map_ipv6_num +
                                     pst_hmac_vap->pst_vap_proxysta->uc_map_unknow_num) )
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_unknow_mac:: map num exceed max size: %d.}", HMAC_PROXYSTA_MAP_MAX_NUM);
        return OAL_SUCC;
    }
    pst_hash_unknow_new = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_proxysta_unknow_hash_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_hash_unknow_new)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_insert_unknow_mac:: mem alloc null pointer.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置MAP的结构变量 */
    pst_hash_unknow_new->us_protocol = us_protocol;
    oal_memcopy(pst_hash_unknow_new->auc_mac, puc_src_mac, WLAN_MAC_ADDR_LEN);
    pst_hash_unknow_new->ul_last_active_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    /* 加入链表 */
    oal_rw_lock_write_lock(&pst_hmac_vap->pst_vap_proxysta->st_lock);
    oal_dlist_add_head(&(pst_hash_unknow_new->st_entry), &(pst_hmac_vap->pst_vap_proxysta->ast_map_unknow_head[uc_hash]));
    pst_hmac_vap->pst_vap_proxysta->uc_map_unknow_num ++;
    oal_rw_lock_write_unlock(&pst_hmac_vap->pst_vap_proxysta->st_lock);

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
        "{hmac_proxysta_insert_unknow_mac:: insert unknow map protocol:0x%x-mac:%x:FF:FF:FF:%x:%x.}",
        us_protocol, puc_src_mac[0], puc_src_mac[4], puc_src_mac[5]);
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_proxysta_tx_ip_addr_insert(hmac_vap_stru *pst_hmac_vap,
                                                      mac_ether_header_stru *pst_ether_header,
                                                      oal_uint32 ul_pkt_len)
{
    oal_ip_header_stru   *pst_ip_header    = OAL_PTR_NULL;
    oal_udp_header_stru  *pst_udp_header   = OAL_PTR_NULL;
    oal_dhcp_packet_stru *pst_dhcp_packet  = OAL_PTR_NULL;
    oal_uint8            *puc_eth_body     = OAL_PTR_NULL;
    oal_uint8            *puc_src_mac      = OAL_PTR_NULL;
    oal_uint8            *puc_proxysta_mac = OAL_PTR_NULL;
    oal_uint16           us_ip_header_len  = 0;
    oal_uint16           us_old_flag       = 0;
    oal_uint32           ul_contig_len     = 0;
    oal_uint32           ul_new_sum        = 0;
    oal_uint8            *puc_ip_addr      = OAL_PTR_NULL;

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ether_header))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_rx_ip_addr_insert::null param vap:%d ether_header:%d.}",
            pst_hmac_vap, pst_ether_header);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* Proxysta 自身的MAC地址 */
    puc_proxysta_mac = mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info);
    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    /* 获取以太网源mac和数据段 */
    puc_src_mac = pst_ether_header->auc_ether_shost;
    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);
    /*************************************************************************/
    /*                      DHCP Frame Format                                */
    /* --------------------------------------------------------------------- */
    /* |以太网头        |   IP头         | UDP头           |DHCP报文       | */
    /* --------------------------------------------------------------------- */
    /* | 14             |20              |8                | 不定          | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    pst_ip_header = (oal_ip_header_stru *)puc_eth_body;

    /*************************************************************************/
    /*                    IP头格式 (oal_ip_header_stru)                      */
    /* --------------------------------------------------------------------- */
    /* | 版本 | 报头长度 | 服务类型 | 总长度  |标识  |标志  |段偏移量 |      */
    /* --------------------------------------------------------------------- */
    /* | 4bits|  4bits   | 1        | 2       | 2    |3bits | 13bits  |      */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | 生存期 | 协议        | 头部校验和| 源地址（SrcIp）|目的地址（DstIp）*/
    /* --------------------------------------------------------------------- */
    /* | 1      |  1 (17为UDP)| 2         | 4              | 4               */
    /* --------------------------------------------------------------------- */
    /*************************************************************************/

    ul_contig_len += OAL_SIZEOF(oal_ip_header_stru);
    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_tx_ip_addr_insert::The length of buf is less than the sum of mac_ether_header_stru and oal_ip_header_stru.}");
        return OAL_FAIL;
    }
    /* IP报文头长度 */
    us_ip_header_len = pst_ip_header->us_ihl * 4;

    /* 如果是UDP包，并且是DHCP协议的报文地址处理 */
    if (OAL_IPPROTO_UDP == pst_ip_header->uc_protocol)
    {
        pst_udp_header  = (oal_udp_header_stru *)((oal_uint8 *)pst_ip_header + us_ip_header_len);
        ul_contig_len += OAL_SIZEOF(oal_udp_header_stru);
        if (ul_pkt_len < ul_contig_len)
        {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                "{hmac_proxysta_tx_ip_addr_insert::The length of udp buf is invalid.}");
            return OAL_FAIL;
        }
        /*************************************************************************/
        /*                      UDP 头 (oal_udp_header_stru)                     */
        /* --------------------------------------------------------------------- */
        /* |源端口号（SrcPort）|目的端口号（DstPort）| UDP长度    | UDP检验和  | */
        /* --------------------------------------------------------------------- */
        /* | 2                 | 2                   |2           | 2          | */
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/

        /* DHCP request UDP Client SP = 68 (bootpc), DP = 67 (bootps) */
        /* Repeater STA发送的DHCP REQUEST报文中要求DHCP SERVER以广播形式发送ACK报文 经由REPEATER发送的DHCP应答报文不会是单播报文 故不区分单播报文 */
/*lint -e778*/
        if (DHCP_PORT_BOOTPS == OAL_NET2HOST_SHORT(pst_udp_header->dest))
/*lint +e778*/
        {
            pst_dhcp_packet = (oal_dhcp_packet_stru *)(((oal_uint8 *)pst_udp_header) + OAL_SIZEOF(oal_udp_header_stru));
            ul_contig_len += OAL_SIZEOF(oal_dhcp_packet_stru);
            if (ul_pkt_len < ul_contig_len)
            {
                OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_tx_ip_addr_insert::The length of dhcp buf is invalid.}");
                return OAL_FAIL;
            }
            /* DHCP报文仅需替换源MAC地址即可 */
            oal_set_mac_addr(puc_src_mac, puc_proxysta_mac);
            /* 客户端发过来的DHCP请求报文 更改标志字段要求服务器以广播形式发送ACK 如果是自己的DHCP则不更改 要求服务器以单播形式回复 */
            if (OAL_MEMCMP(puc_proxysta_mac, pst_dhcp_packet->chaddr, WLAN_MAC_ADDR_LEN))
            {
                us_old_flag = pst_dhcp_packet->flags;
/*lint -e778*/
                pst_dhcp_packet->flags = OAL_NET2HOST_SHORT(DHCP_FLAG_BCAST);
/*lint +e778*/
                /* 修改后重新计算UDP的校验和 */
                ul_new_sum = (oal_uint32)pst_udp_header->check;
                ul_new_sum += us_old_flag + (~(pst_dhcp_packet->flags) & 0XFFFF);
                ul_new_sum  = (ul_new_sum >> 16) + (ul_new_sum & 0XFFFF);
                pst_udp_header->check = (oal_uint16)((ul_new_sum >> 16) + ul_new_sum);
            }
            return OAL_SUCC;
        }
    }

    puc_ip_addr = (oal_uint8 *)(&pst_ip_header->ul_saddr);
    /* 其它类型IP报文以及不是DHCP的其它UDP包 的地址转换处理 将IP地址和MAC地址更新到MA表格中 插入MAP表格失败不影响处理结果 */
    hmac_proxysta_insert_ipv4_mac(pst_hmac_vap, puc_ip_addr, puc_src_mac);
    /* 替换源MAC地址 */
    oal_set_mac_addr(puc_src_mac, puc_proxysta_mac);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_proxysta_tx_arp_addr_insert(hmac_vap_stru *pst_hmac_vap,
                                                       mac_ether_header_stru *pst_ether_header,
                                                       oal_uint32 ul_pkt_len)
{
    oal_eth_arphdr_stru *pst_arp          = OAL_PTR_NULL;
    oal_uint8           *puc_eth_body     = OAL_PTR_NULL;
    oal_uint8           *puc_src_mac      = OAL_PTR_NULL;
    oal_uint32          ul_contig_len;
    oal_uint8           *puc_proxysta_mac = OAL_PTR_NULL;

    /***************************************************************************/
    /*                      ARP Frame Format                                   */
    /* ----------------------------------------------------------------------- */
    /* |以太网目的地址|以太网源地址|帧类型|硬件类型|协议类型|硬件地址长度|     */
    /* ----------------------------------------------------------------------- */
    /* | 6 (待替换)   |6           |2     |2       |2       |1           |     */
    /* ----------------------------------------------------------------------- */
    /* |协议地址长度|op|发送端以太网地址|发送端IP地址|目的以太网地址|目的IP地址*/
    /* ----------------------------------------------------------------------- */
    /* | 1          |2 |6               |4           |6 (待替换)    |4         */
    /* ----------------------------------------------------------------------- */
    /*                                                                         */
    /***************************************************************************/

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ether_header))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_tx_arp_addr_insert:: null param, mac_vap:%d ether_header:%d.}",
            pst_hmac_vap, pst_ether_header);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取proxysta自己的MAC地址 */
    puc_proxysta_mac = mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info);
    /* 获取以太网目的mac和数据段 */
    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    puc_src_mac = pst_ether_header->auc_ether_shost;
    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);
    /* ARP 包地址转换 */
    pst_arp = (oal_eth_arphdr_stru *)puc_eth_body;
    ul_contig_len += OAL_SIZEOF(oal_eth_arphdr_stru);

    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                       "{hmac_proxysta_tx_arp_addr_insert::The length of buf is invalid.}");
        return OAL_FAIL;
    }
    /* 非IPV4的ARP报文不处理 */
/*lint -e778*/
    if ((ETHER_ADDR_LEN != pst_arp->uc_ar_hln) || (ETHER_TYPE_IP != OAL_HOST2NET_SHORT(pst_arp->us_ar_pro)))
/*lint +e778*/
    {
        OAM_ERROR_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                       "{hmac_proxysta_tx_arp_addr_insert::arp hln:%d, arp pro: %d ,not to process.}",
                       pst_arp->uc_ar_hln, pst_arp->us_ar_pro);
        return OAL_SUCC;
    }

    /*  获取报文中目的IP地址 解析MAP表 获取真实目的MAC地址 */
    hmac_proxysta_insert_ipv4_mac(pst_hmac_vap, pst_arp->auc_ar_sip, puc_src_mac);
    /* 替换arp报文中的mac地址为proxysta的MAC地址 */
    oal_set_mac_addr(pst_arp->auc_ar_sha, puc_proxysta_mac);
    /* 替换报文以太网源地址 */
    oal_set_mac_addr(puc_src_mac, puc_proxysta_mac);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_proxysta_tx_icmpv6_mac_replace(hmac_vap_stru *pst_hmac_vap,
                                                          oal_ipv6_header_stru *pst_ipv6_hdr,
                                                          oal_uint32 ul_pkt_len)
{
    oal_icmp6hdr_stru         *pst_icmp6hdr         = OAL_PTR_NULL;
    oal_eth_icmp6_lladdr_stru *pst_eth_icmp6_lladdr = OAL_PTR_NULL;
    oal_uint8                 *puc_option_buff      = OAL_PTR_NULL;
    oal_uint16                 us_check_sum = 0;
    oal_uint32                 ul_contig_len = 0;
    oal_bool_enum_uint8        en_packet_need_change = OAL_FALSE;

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ipv6_hdr))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_tx_icmpv6_mac_replace:: null param hmac_vap[0x%x] ipv6_hdr[0x%x].}",
            pst_hmac_vap, pst_ipv6_hdr);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_icmp6hdr = (oal_icmp6hdr_stru *)(pst_ipv6_hdr+1);
    ul_contig_len = OAL_SIZEOF(oal_icmp6hdr_stru);
    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_tx_icmpv6_mac_replace::The length of buf is invalid ul_pkt_len[%d], ul_contig_len[%d] icmp6_type[%d].}",
            ul_pkt_len, ul_contig_len, pst_icmp6hdr->icmp6_type);
        return OAL_FAIL;
    }
    /* 需要替换路由请求、通告以及邻居请求、通告报文中的源MAC地址 */
    switch (pst_icmp6hdr->icmp6_type)
    {
        case OAL_NDISC_NEIGHBOUR_SOLICITATION:
        case OAL_NDISC_NEIGHBOUR_ADVERTISEMENT:
        {
            /* 数据报文长度异常 返回失败 */
            ul_contig_len += OAL_IPV6_MAC_ADDR_LEN;
            if (ul_pkt_len < ul_contig_len)
            {
                OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_tx_icmpv6_mac_replace::neighbor-ad buf len is invalid.}");
                return OAL_FAIL;
            }
            en_packet_need_change = OAL_TRUE;
            /* 将指针定位到option起始地址 */
            puc_option_buff = (oal_uint8 *)(pst_icmp6hdr) + OAL_SIZEOF(oal_icmp6hdr_stru) + OAL_IPV6_MAC_ADDR_LEN;
            break;
        }
        case OAL_NDISC_ROUTER_SOLICITATION:
        {
            en_packet_need_change = OAL_TRUE;
            /* 将指针定位到option起始地址 */
            puc_option_buff = (oal_uint8 *)(pst_icmp6hdr) + OAL_SIZEOF(oal_icmp6hdr_stru);
            break;
        }
        case OAL_NDISC_ROUTER_ADVERTISEMENT:
        {
            /* 数据报文长度异常 返回失败 */
            ul_contig_len += PROTOCOL_ICMPV6_ROUTER_AD_OFFLOAD;
            if (ul_pkt_len < ul_contig_len)
            {
                OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_tx_icmpv6_mac_replace::router-ad buf len is invalid.}");
                return OAL_FAIL;
            }
            en_packet_need_change = OAL_TRUE;
            /* 将指针定位到option起始地址 */
            puc_option_buff = (oal_uint8 *)(pst_icmp6hdr) + OAL_SIZEOF(oal_icmp6hdr_stru) + PROTOCOL_ICMPV6_ROUTER_AD_OFFLOAD;
            break;
        }
        case OAL_NDISC_REDIRECT:
        {
            /* 数据报文长度异常 返回失败 */
            ul_contig_len += (OAL_IPV6_MAC_ADDR_LEN + OAL_IPV6_MAC_ADDR_LEN);
            if (ul_pkt_len < ul_contig_len)
            {
                OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                    "{hmac_proxysta_tx_icmpv6_mac_replace::redirect buf len is invalid.}");
                return OAL_FAIL;
            }
            en_packet_need_change = OAL_TRUE;
            /* 将指针定位到option起始地址 重定向消息含2个IP address字段 */
            puc_option_buff = (oal_uint8 *)(pst_icmp6hdr) + OAL_SIZEOF(oal_icmp6hdr_stru) + OAL_IPV6_MAC_ADDR_LEN + OAL_IPV6_MAC_ADDR_LEN;
            break;
        }
        /* 其他类型消息不需要替换源MAC地址 */
        default:
            break;
    }

    if (en_packet_need_change)
    {
        /* Source MAC地址的type为1 Length也为1 */
        pst_eth_icmp6_lladdr = (oal_eth_icmp6_lladdr_stru *)hmac_proxysta_find_tlv(puc_option_buff, (oal_uint16)(ul_pkt_len-ul_contig_len), 1, 1);
        if (OAL_PTR_NULL == pst_eth_icmp6_lladdr)
        {
#ifdef _PRE_DEBUG_MOE
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                "{hmac_proxysta_tx_icmpv6_mac_replace:: frame[%d] no source addr.}", pst_icmp6hdr->icmp6_type);
#endif
            return OAL_SUCC;
        }
        /* 替换地址并重新计算校验和 */
        oal_set_mac_addr(pst_eth_icmp6_lladdr->uc_addr, mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info));
        us_check_sum = hmac_proxysta_cal_checksum((oal_uint16)OAL_IPPROTO_ICMPV6,
                                                  pst_ipv6_hdr->us_payload_len,
                                                  pst_ipv6_hdr->st_saddr.auc_ipv6_union_addr,
                                                  pst_ipv6_hdr->st_daddr.auc_ipv6_union_addr,
                                                  OAL_IPV6_MAC_ADDR_LEN,
                                                  (oal_uint8 *)pst_icmp6hdr);

        pst_icmp6hdr->icmp6_cksum = OAL_HOST2NET_SHORT(us_check_sum);
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_proxysta_tx_ipv6_addr_insert(hmac_vap_stru *pst_hmac_vap,
                                                        mac_ether_header_stru *pst_ether_header,
                                                        oal_uint32 ul_pkt_len)
{
    oal_ipv6_header_stru    *pst_ipv6_hdr_data = OAL_PTR_NULL;
    oal_uint8           *puc_eth_body     = OAL_PTR_NULL;
    oal_uint8           *puc_src_mac      = OAL_PTR_NULL;
    oal_uint8           *puc_proxysta_mac = OAL_PTR_NULL;
    oal_uint32          ul_contig_len = 0;

    /*************************************************************************/
    /*                      IPV6 Frame Format                                */
    /* --------------------------------------------------------------------- */
    /* |以太网头        |   IPV6头         | Next Frame Boady              | */
    /* --------------------------------------------------------------------- */
    /* | 14             |   40             |  不定                         | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ether_header))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_tx_ipv6_addr_insert:: null param, mac_vap:%d ether_header:%d.}",
                       pst_hmac_vap, pst_ether_header);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取PROXYSTA 自己的MAC地址 */
    puc_proxysta_mac = mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info);
    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    /* 获取以太网目的mac和数据段 */
    puc_src_mac= pst_ether_header->auc_ether_shost;
    puc_eth_body = (oal_uint8 *)(pst_ether_header + 1);
    /*************************************************************************/
    /*                   IPV6头格式 (oal_ipv6_header_stru)                   */
    /* --------------------------------------------------------------------- */
    /* | 版本 | Traffic Class |                 Flow label            |      */
    /* --------------------------------------------------------------------- */
    /* | 4bits|  8bits        |                 20bits                |      */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | Payload Length             | Next Header    |   Hop Limit    |      */
    /* --------------------------------------------------------------------- */
    /* | 2                          | 1              |   1            |      */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |                           Source Address                            */
    /* --------------------------------------------------------------------- */
    /* |                                 16                                  */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |                           Destination Address                       */
    /* --------------------------------------------------------------------- */
    /* |                                 16                                  */
    /* --------------------------------------------------------------------- */
    /*************************************************************************/
    pst_ipv6_hdr_data = (oal_ipv6_header_stru *)puc_eth_body;
    ul_contig_len += OAL_SIZEOF(oal_ipv6_header_stru);

    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
                       "{hmac_proxysta_tx_ipv6_addr_insert::The length of buf is invalid.}");
        return OAL_FAIL;
    }
    /* 如果是ICMPV6报文则需要继续替换报文中的MAC地址 */
    if (PROTOCOL_ICMPV6 == pst_ipv6_hdr_data->uc_nexthdr)
    {
        hmac_proxysta_tx_icmpv6_mac_replace(pst_hmac_vap, pst_ipv6_hdr_data, ul_pkt_len-ul_contig_len);
    }
    /*  获取报文中目的IP地址 解析MAP表 获取真实目的MAC地址 */
    hmac_proxysta_insert_ipv6_mac(pst_hmac_vap, pst_ipv6_hdr_data->st_saddr.auc_ipv6_union_addr, puc_src_mac);
    oal_set_mac_addr(puc_src_mac, puc_proxysta_mac);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_proxysta_tx_unknow_addr_insert(hmac_vap_stru *pst_hmac_vap,
                                                          mac_ether_header_stru *pst_ether_header,
                                                          oal_uint32 ul_pkt_len)
{
    oal_uint8            *puc_src_mac      = OAL_PTR_NULL;
    oal_uint8            *puc_proxysta_mac = OAL_PTR_NULL;
    oal_uint32           ul_contig_len = 0;

    /* 参数合法性检查 */
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_ether_header))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_tx_unknow_addr_insert::null param vap:%d ether_header:%d device:%d.}",
            pst_hmac_vap, pst_ether_header);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_contig_len = OAL_SIZEOF(mac_ether_header_stru);
    if (ul_pkt_len < ul_contig_len)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA,
            "{hmac_proxysta_tx_unknow_addr_insert::::The length of buf:%d is less than mac_ether_header_stru.}", ul_pkt_len);
        return OAL_FAIL;
    }
    /* 获取PROXYSTA自己的MAC地址 */
    puc_proxysta_mac = mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info);
    /* 获取以太网源mac和数据段 */
    puc_src_mac= pst_ether_header->auc_ether_shost;

    hmac_proxysta_insert_unknow_mac(pst_hmac_vap, pst_ether_header->us_ether_type , puc_src_mac);
    /* 更新以太网目的地址为实际的STA MAC地址 */
    oal_set_mac_addr(puc_src_mac, puc_proxysta_mac);
    return OAL_SUCC;
}


oal_uint32 hmac_proxysta_tx_process(oal_netbuf_stru *pst_buf, hmac_vap_stru *pst_hmac_vap)
{
    mac_ether_header_stru  *pst_ether_header;
    oal_uint16             us_ether_type = 0;
    oal_uint32             ul_pkt_len = 0;

    if ((OAL_PTR_NULL == pst_buf) || (OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG2(0, OAM_SF_PROXYSTA, "{hmac_proxysta_tx_process::null param: netbuf:%d vap:%d.}",
                       pst_buf, pst_hmac_vap);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ether_header = (mac_ether_header_stru *)OAL_NETBUF_HEADER(pst_buf);
    /* 获取帧长度 包含以太网报文头 */
    ul_pkt_len = OAL_NETBUF_LEN(pst_buf);
    us_ether_type = pst_ether_header->us_ether_type;

    /* 根据报文类型进行相应的处理 */
/*lint -e778*/
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == us_ether_type)
/*lint +e778*/
    {
        return hmac_proxysta_tx_ip_addr_insert(pst_hmac_vap, pst_ether_header, ul_pkt_len);
    }
    /* ARP 包地址转换 */
/*lint -e778*/
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_ARP) == us_ether_type)
/*lint +e778*/
    {
        return hmac_proxysta_tx_arp_addr_insert(pst_hmac_vap, pst_ether_header, ul_pkt_len);
    }
    /*icmpv6 包地址转换 */
/*lint -e778*/
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6) == us_ether_type)
/*lint +e778*/
    {
        return hmac_proxysta_tx_ipv6_addr_insert(pst_hmac_vap, pst_ether_header, ul_pkt_len);
    }
    /* IPX 报文地址替换 */
/*lint -e778*/
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPX) == us_ether_type)
/*lint +e778*/
    {
        /* TO BE DONE */
        return OAL_SUCC;
    }
    /* APPLE TALK  & AARP报文地址替换 */
/*lint -e778*/
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_AARP) == us_ether_type)
/*lint +e778*/
    {
        /* TO BE DONE */
        return OAL_SUCC;
    }
    /* PPOE 报文地址替换 */
/*lint -e778*/
    else if ( (OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_DISC) == us_ether_type) ||
              (OAL_HOST2NET_SHORT(ETHER_TYPE_PPP_SES) == us_ether_type) )
/*lint +e778*/
    {
        /* TO BE DONE */
        return OAL_SUCC;
    }
    /* 此类报文协议不替换 0xe2ae 0xe2af为realtek处理的报文协议类型，未找到类型报文说明  暂保留 */
/*lint -e778*/
    else if ( (OAL_HOST2NET_SHORT(ETHER_TYPE_PAE) == us_ether_type) ||
              (OAL_HOST2NET_SHORT(0xe2ae) == us_ether_type) ||
              (OAL_HOST2NET_SHORT(0xe2af) == us_ether_type) )
/*lint +e778*/
    {
        return OAL_SUCC;
    }
    /* 其他未知类型的地址替换 */
    else
    {
        return hmac_proxysta_tx_unknow_addr_insert(pst_hmac_vap, pst_ether_header, ul_pkt_len);
    }
}

#endif  //_PRE_WLAN_FEATURE_SINGLE_PROXYSTA

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
