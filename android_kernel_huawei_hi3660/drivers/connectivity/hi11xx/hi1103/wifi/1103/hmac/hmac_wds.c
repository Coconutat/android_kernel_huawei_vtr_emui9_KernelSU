


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_main.h"
#include "mac_vap.h"
#include "hmac_vap.h"
#include "mac_resource.h"
#include "hmac_user.h"
#include "hmac_mgmt_ap.h"
#include "hmac_tx_data.h"
#include "hmac_wds.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_WDS_C

#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)



/*****************************************************************************
  2 全局变量定义
*****************************************************************************/



/*****************************************************************************
3 函数实现
*****************************************************************************/


OAL_STATIC oal_uint32  hmac_wds_find_node(
                hmac_vap_stru        *pst_hmac_vap,
                oal_uint8            *puc_addr,
                hmac_wds_node_stru   **ppst_wds_node)
{
    oal_uint32                  ul_hash_value = 0;
    hmac_wds_node_stru         *pst_node = OAL_PTR_NULL;
    oal_dlist_head_stru        *pst_entry = OAL_PTR_NULL;

    ul_hash_value = WDS_CALC_MAC_HASH_VAL(puc_addr);

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->st_wds_table.st_peer_node[ul_hash_value]))
    {
        pst_node = (hmac_wds_node_stru *)pst_entry;

        if (OAL_PTR_NULL == pst_node)
        {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_find_node::pst_node null.sta idx %d}",ul_hash_value);
            continue;
        }

        /* 相同的MAC地址 */
        if (!oal_compare_mac_addr(pst_node->auc_mac, puc_addr))
        {
            *ppst_wds_node = pst_node;
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


oal_uint32  hmac_wds_find_sta(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_addr,
                hmac_wds_stas_stru      **ppst_wds_sta)
{
    oal_uint32                  ul_hash_value = 0;
    hmac_wds_stas_stru         *pst_sta = OAL_PTR_NULL;
    oal_dlist_head_stru        *pst_entry = OAL_PTR_NULL;

    ul_hash_value = WDS_CALC_MAC_HASH_VAL(puc_addr);

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->st_wds_table.st_wds_stas[ul_hash_value]))
    {
        pst_sta = (hmac_wds_stas_stru *)pst_entry;

        if (OAL_PTR_NULL == pst_sta)
        {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_find_sta::pst_sta null.sta idx %d}",ul_hash_value);
            continue;
        }

        /* 相同的MAC地址 */
        if (!oal_compare_mac_addr(pst_sta->auc_mac, puc_addr))
        {
            *ppst_wds_sta = pst_sta;
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


oal_uint32  hmac_wds_del_sta_entry(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_addr,
                hmac_wds_stas_stru      *pst_sta)
{
    if ((OAL_PTR_NULL == pst_sta) && (OAL_SUCC != hmac_wds_find_sta(pst_hmac_vap, puc_addr, &pst_sta)))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_sta_entry::hmac_wds_find_sta failed for mac[%02x:%02x:%02x:%02x]}", puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);
        return OAL_SUCC;
    }

    if (OAL_PTR_NULL != pst_sta->pst_related_node)
    {
        pst_sta->pst_related_node->uc_stas_num--;
    }

    oal_dlist_delete_entry(&(pst_sta->st_entry));

    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_sta_entry::del sta success for mac[%02x:%02x:%02x:%02x]}", puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);

    OAL_MEM_FREE(pst_sta, OAL_TRUE);
    pst_hmac_vap->st_wds_table.uc_wds_stas_num--;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_wds_add_sta_entry(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_addr,
                hmac_wds_node_stru      *pst_wds_node,
                hmac_wds_stas_stru      **ppst_wds_sta)
{
    oal_uint32                  ul_hash_value = 0;
    hmac_wds_stas_stru          *pst_sta = OAL_PTR_NULL;

    pst_sta = (hmac_wds_stas_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_wds_stas_stru), OAL_TRUE);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_sta))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_add_sta_entry::OAL_MEM_ALLOC wds sta struct fail!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(pst_sta, OAL_SIZEOF(hmac_wds_stas_stru));
    oal_set_mac_addr(pst_sta->auc_mac, puc_addr);
    pst_sta->pst_related_node = pst_wds_node;
    pst_wds_node->uc_stas_num++;

    pst_sta->ul_last_pkt_age = OAL_TIME_JIFFY;

    ul_hash_value = WDS_CALC_MAC_HASH_VAL(puc_addr);

    oal_dlist_add_head(&(pst_sta->st_entry), &pst_hmac_vap->st_wds_table.st_wds_stas[ul_hash_value]);

    *ppst_wds_sta = pst_sta;
    pst_hmac_vap->st_wds_table.uc_wds_stas_num++;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_wds_update_sta_entry(
                hmac_vap_stru            *pst_hmac_vap,
                hmac_wds_node_stru      *pst_new_wds_node,
                hmac_wds_stas_stru      *pst_wds_sta)
{
    if (OAL_PTR_NULL != pst_wds_sta->pst_related_node)
    {
        pst_wds_sta->pst_related_node->uc_stas_num--;
    }

    pst_wds_sta->pst_related_node = pst_new_wds_node;
    pst_new_wds_node->uc_stas_num++;
    pst_wds_sta->ul_last_pkt_age = OAL_TIME_JIFFY;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_wds_del_all_node_sta_entry(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_node_mac)
{
    oal_uint8                       uc_hash_value = 0;
    oal_uint32                      ul_ret = OAL_SUCC;
    hmac_wds_stas_stru              *pst_sta = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_next_entry = OAL_PTR_NULL;

    for (uc_hash_value = 0; uc_hash_value < WDS_HASH_NUM; uc_hash_value++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_next_entry, &(pst_hmac_vap->st_wds_table.st_wds_stas[uc_hash_value]))
        {
            pst_sta = (hmac_wds_stas_stru *)pst_entry;

            if (OAL_PTR_NULL == pst_sta)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_all_node_sta_entry::pst_sta null.sta idx %d}", (oal_uint32)uc_hash_value);
                ul_ret = OAL_FAIL;
                break;
            }

            /* 相同的MAC地址 */
            if (!oal_compare_mac_addr(pst_sta->pst_related_node->auc_mac, puc_node_mac)
                && (OAL_SUCC != hmac_wds_del_sta_entry(pst_hmac_vap, pst_sta->auc_mac, pst_sta)))
            {
                OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_all_node_sta_entry::hmac_wds_del_sta_entry fail for wds node[%02x:%02x:%02x:%02x].}",
                    puc_node_mac[2], puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);

                ul_ret = OAL_FAIL;
                break;
            }
        }

        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_all_node_sta_entry::Del related wds sta fail for wds node[%02x:%02x:%02x:%02x].}",
                puc_node_mac[2], puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);
            break;
        }
    }

    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_wds_del_node_entry(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_addr,
                hmac_wds_node_stru      *pst_node)
{
    if (OAL_SUCC != hmac_wds_del_all_node_sta_entry(pst_hmac_vap, puc_addr))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_node_entry::hmac_wds_del_all_node_sta_entry failed for mac[%02x:%02x:%02x:%02x]}",
            puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);
        return OAL_FAIL;
    }

    if ((OAL_PTR_NULL == pst_node) && (OAL_SUCC != hmac_wds_find_node(pst_hmac_vap, puc_addr, &pst_node)))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_node_entry::hmac_wds_find_node failed for mac[%02x:%02x:%02x:%02x]}", puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);
        return OAL_SUCC;
    }

    oal_dlist_delete_entry(&(pst_node->st_entry));

    OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_node_entry::del node success for mac[%02x:%02x:%02x:%02x]}", puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);

    OAL_MEM_FREE(pst_node, OAL_TRUE);
    pst_hmac_vap->st_wds_table.uc_wds_node_num--;

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  hmac_wds_add_node_entry(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_addr,
                hmac_wds_node_stru      **ppst_wds_node)
{
    oal_uint32                  ul_hash_value = 0;
    hmac_wds_node_stru          *pst_node = OAL_PTR_NULL;

    pst_node = (hmac_wds_node_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_wds_node_stru), OAL_TRUE);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_node))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_add_node_entry::OAL_MEM_ALLOC wds sta struct fail!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(pst_node, OAL_SIZEOF(hmac_wds_node_stru));
    oal_set_mac_addr(pst_node->auc_mac, puc_addr);

    ul_hash_value = WDS_CALC_MAC_HASH_VAL(puc_addr);
    oal_dlist_add_head(&(pst_node->st_entry), &pst_hmac_vap->st_wds_table.st_peer_node[ul_hash_value]);

    *ppst_wds_node = pst_node;
    pst_hmac_vap->st_wds_table.uc_wds_node_num++;

    return OAL_SUCC;
}



oal_uint32  hmac_wds_add_node(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_node_mac)
{
    oal_uint8               ul_ret = OAL_SUCC;
    hmac_wds_node_stru   *pst_wds_node = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap)
                  || (OAL_PTR_NULL == puc_node_mac)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_add_node::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_rw_lock_read_lock(&pst_hmac_vap->st_wds_table.st_lock);
    if (pst_hmac_vap->st_wds_table.uc_wds_node_num >= WDS_MAX_NODE_NUM)
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_add_node::Exceed max node %d, fail to add wds node[xx:xx:xx:%02x:%02x:%02x].}",
            (oal_uint32)pst_hmac_vap->st_wds_table.uc_wds_node_num, puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);

        oal_rw_lock_read_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_FAIL;
    }

    if (OAL_SUCC == hmac_wds_find_node(pst_hmac_vap, puc_node_mac, &pst_wds_node))
    {
        OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_add_node::Same wds sta to the same wds node[%02x:%02x:%02x:%02x].}",
            puc_node_mac[2], puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);

        oal_rw_lock_read_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_SUCC;
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    ul_ret = hmac_wds_add_node_entry(pst_hmac_vap, puc_node_mac, &pst_wds_node);
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_add_node::ul_ret: %d to add wds node[xx:xx:xx:%02x:%02x:%02x].}",
        ul_ret, puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);

    return ul_ret;
}


oal_uint32  hmac_wds_del_node(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_node_mac)
{
    oal_uint32                      ul_ret = OAL_SUCC;
    hmac_wds_node_stru              *pst_wds_node = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap)
                  || (OAL_PTR_NULL == puc_node_mac)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_del_node::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    if (OAL_SUCC != hmac_wds_find_node(pst_hmac_vap, puc_node_mac, &pst_wds_node))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_del_node::hmac_wds_find_node fail for wds node[%02x:%02x:%02x:%02x].}",
            puc_node_mac[2], puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_FAIL;
    }

    if (OAL_SUCC != hmac_wds_del_node_entry(pst_hmac_vap, puc_node_mac, pst_wds_node))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_node::hmac_wds_del_node_entry fail for mac[%02x:%02x:%02x:%02x]}", puc_node_mac[2], puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_FAIL;
    }
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_del_node::ul_ret: %d to del wds node[xx:xx:xx:%02x:%02x:%02x].}",
        ul_ret, puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);

    return ul_ret;
}


oal_uint32  hmac_wds_add_sta(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_node_mac,
                oal_uint8               *puc_sta_mac)
{
    oal_uint32              ul_ret = OAL_SUCC;
    hmac_wds_stas_stru      *pst_wds_sta = OAL_PTR_NULL;
    hmac_wds_node_stru      *pst_wds_node = OAL_PTR_NULL;
    hmac_wds_node_stru      *pst_old_wds_node = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap)
                  || (OAL_PTR_NULL == puc_node_mac)
                  || (OAL_PTR_NULL == puc_sta_mac)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_add_sta::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_rw_lock_read_lock(&pst_hmac_vap->st_wds_table.st_lock);
    if (pst_hmac_vap->st_wds_table.uc_wds_stas_num >= WDS_MAX_STAS_NUM)
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_add_sta::Exceed the max wds sta number %d.Fail to add for mac [xx:xx:xx:%02x:%02x:%02x].}",
            pst_hmac_vap->st_wds_table.uc_wds_stas_num, puc_sta_mac[3], puc_sta_mac[4], puc_sta_mac[5]);

        oal_rw_lock_read_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_FAIL;
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    if (OAL_SUCC == hmac_wds_find_sta(pst_hmac_vap, puc_sta_mac, &pst_wds_sta))
    {
        if (!oal_compare_mac_addr(pst_wds_sta->pst_related_node->auc_mac, puc_node_mac))
        {
            OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_add_sta::Same wds sta to the same wds node[%02x:%02x:%02x:%02x].}",
                puc_node_mac[2], puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);

            ul_ret = OAL_SUCC;
        }
        else if (OAL_SUCC == hmac_wds_find_node(pst_hmac_vap, puc_node_mac, &pst_wds_node))
        {
            pst_old_wds_node = pst_wds_sta->pst_related_node;
            ul_ret = hmac_wds_update_sta_entry(pst_hmac_vap, pst_wds_node, pst_wds_sta);
            if((OAL_SUCC == ul_ret) && (OAL_PTR_NULL != pst_old_wds_node) && (0 == pst_old_wds_node->uc_stas_num))
            {
                hmac_wds_del_node_entry(pst_hmac_vap, pst_old_wds_node->auc_mac, pst_old_wds_node);
            }
        }
    }
    else
    {
        if (OAL_SUCC == hmac_wds_find_node(pst_hmac_vap, puc_node_mac, &pst_wds_node))
        {
            ul_ret = hmac_wds_add_sta_entry(pst_hmac_vap, puc_sta_mac, pst_wds_node, &pst_wds_sta);
        }
        else
        {
            OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_add_sta::No wds node for mac [%02x:%02x:%02x:%02x].}",
                puc_node_mac[2], puc_node_mac[3], puc_node_mac[4], puc_node_mac[5]);
            ul_ret = OAL_FAIL;
        }
    }

    if (OAL_SUCC == ul_ret)
    {
        pst_wds_sta->ul_last_pkt_age = OAL_TIME_JIFFY;
    }
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_add_sta::ul_ret: %d to add wds sta[xx:xx:xx:%02x:%02x:%02x].}",
        ul_ret, puc_sta_mac[3], puc_sta_mac[4], puc_sta_mac[5]);

    return ul_ret;
}



oal_uint32  hmac_wds_del_sta(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_addr)
{
    hmac_wds_stas_stru      *pst_sta = OAL_PTR_NULL;
    hmac_wds_node_stru      *pst_node = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap)
        || (OAL_PTR_NULL == puc_addr)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_del_sta::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);

    if ((OAL_PTR_NULL == pst_sta) && (OAL_SUCC != hmac_wds_find_sta(pst_hmac_vap, puc_addr, &pst_sta)))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_sta::hmac_wds_find_sta failed for mac[%02x:%02x:%02x:%02x]}", puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_SUCC;
    }

    pst_node = pst_sta->pst_related_node;

    if (OAL_SUCC != hmac_wds_del_sta_entry(pst_hmac_vap, puc_addr, pst_sta))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_del_sta::hmac_wds_del_sta_entry fail for mac[%02x:%02x:%02x:%02x]}", puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_FAIL;
    }

    if((OAL_PTR_NULL != pst_node) && (0 == pst_node->uc_stas_num))
    {
        hmac_wds_del_node_entry(pst_hmac_vap, pst_node->auc_mac, pst_node);
    }

    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    return OAL_SUCC;
}



oal_uint32  hmac_find_valid_user_by_wds_sta(
                hmac_vap_stru       *pst_hmac_vap,
                oal_uint8           *puc_sta_mac_addr,
                oal_uint16          *pus_user_idx)
{
    oal_uint32                  ul_ret = OAL_FAIL;
    oal_uint32                  ul_now = 0;
    hmac_wds_stas_stru          *pst_wds_sta = OAL_PTR_NULL;
    hmac_wds_node_stru          *pst_node = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap)
                  || (OAL_PTR_NULL == puc_sta_mac_addr)
                  || ((OAL_PTR_NULL == pus_user_idx))))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{hmac_find_valid_user_by_wds_sta::param null: pst_hmac_vap[%x]  puc_sta_mac_addr[%x]  pus_user_idx[%x]}", pst_hmac_vap, puc_sta_mac_addr, pus_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 放在指针空判断后 vap开启了Multi-STA本地能力位 */
    if (WDS_MODE_ROOTAP != pst_hmac_vap->st_wds_table.en_wds_vap_mode)
    {
        return ul_ret;
    }

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    if (OAL_SUCC != hmac_wds_find_sta(pst_hmac_vap, puc_sta_mac_addr, &pst_wds_sta))
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_find_valid_user_by_wds_sta::hmac_wds_find_sta fail.}");
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return ul_ret;
    }

    ul_now = OAL_TIME_JIFFY;
    if (OAL_TIME_GET_RUNTIME(pst_wds_sta->ul_last_pkt_age, ul_now) > pst_hmac_vap->st_wds_table.ul_wds_aging)
    {
        pst_node = pst_wds_sta->pst_related_node;
        if((OAL_SUCC == hmac_wds_del_sta_entry(pst_hmac_vap, puc_sta_mac_addr, pst_wds_sta))
            && (OAL_PTR_NULL != pst_node) && (0 == pst_node->uc_stas_num))
        {
            hmac_wds_del_node_entry(pst_hmac_vap, pst_node->auc_mac, pst_node);
        }
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_find_valid_user_by_wds_sta::Wds sta rule timeout, to remove the sta [%02x:%02x:%02x:%02x]}", puc_sta_mac_addr[2], puc_sta_mac_addr[3], puc_sta_mac_addr[4], puc_sta_mac_addr[5]);
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return ul_ret;
    }

    ul_ret = mac_vap_find_user_by_macaddr_etc(&(pst_hmac_vap->st_vap_base_info), pst_wds_sta->pst_related_node->auc_mac, pus_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_find_valid_user_by_wds_sta::mac_vap_find_user_by_macaddr_etc failed. mac[%02x:%02x:%02x:%02x]}", puc_sta_mac_addr[2], puc_sta_mac_addr[3], puc_sta_mac_addr[4], puc_sta_mac_addr[5]);
    }
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    return ul_ret;
}


oal_uint32  hmac_wds_update_table(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_node_mac,
                oal_uint8               *puc_sta_mac,
                oal_uint8               uc_update_mode)
{
    oal_uint32                  ul_ret = OAL_SUCC;

    if (WDS_TABLE_ADD_ENTRY == uc_update_mode)
    {
        hmac_wds_add_node(pst_hmac_vap, puc_node_mac);
        hmac_wds_add_sta(pst_hmac_vap, puc_node_mac, puc_sta_mac);
    }
    else if (WDS_TABLE_DEL_ENTRY == uc_update_mode)
    {
        hmac_wds_del_sta(pst_hmac_vap, puc_sta_mac);
    }
    else
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_update_table::Not supported update mode: %d.}", uc_update_mode);
        ul_ret = OAL_FAIL;
    }

    return ul_ret;
}


oal_uint32  hmac_wds_reset_sta_mapping_table(hmac_vap_stru *pst_hmac_vap)
{
    oal_uint8                       uc_hash_value = 0;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_next_entry = OAL_PTR_NULL;
    hmac_wds_node_stru              *pst_node = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (WDS_MODE_ROOTAP != pst_hmac_vap->st_wds_table.en_wds_vap_mode))
    {
        return OAL_SUCC;
    }

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    for (uc_hash_value = 0; uc_hash_value < WDS_HASH_NUM; uc_hash_value++)
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_reset_sta_mapping_table::hash idx %d}", (oal_uint32)uc_hash_value);
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_next_entry, &(pst_hmac_vap->st_wds_table.st_peer_node[uc_hash_value]))
        {
            pst_node = (hmac_wds_node_stru *)pst_entry;
            if (OAL_PTR_NULL == pst_node)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_reset_sta_mapping_table::pst_sta null.hash idx %d}", (oal_uint32)uc_hash_value);
                ul_ret = OAL_ERR_CODE_PTR_NULL;
                break;
            }

            if (OAL_SUCC != hmac_wds_del_node_entry(pst_hmac_vap, pst_node->auc_mac, pst_node))
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_reset_sta_mapping_table::hmac_wds_del_node fail for hash idx %d}", (oal_uint32)uc_hash_value);
                ul_ret = OAL_FAIL;
                break;
            }
        }
    }
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_reset_sta_mapping_table::Leave.}");

    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_wds_table_timerout_handler(oal_void *p_arg)
{
    oal_uint8                       uc_hash_value = 0;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_next_entry = OAL_PTR_NULL;
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;
    hmac_wds_node_stru              *pst_node = OAL_PTR_NULL;
    hmac_wds_stas_stru              *pst_wds_sta = OAL_PTR_NULL;

    pst_hmac_vap = (hmac_vap_stru *)p_arg;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (WDS_MODE_ROOTAP != pst_hmac_vap->st_wds_table.en_wds_vap_mode))
    {
        return OAL_SUCC;
    }

    if (MAC_VAP_STATE_UP != pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
        return OAL_SUCC;
    }

    OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_table_timerout_handler::in}");

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    for (uc_hash_value = 0; uc_hash_value < WDS_HASH_NUM; uc_hash_value++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_next_entry, &(pst_hmac_vap->st_wds_table.st_wds_stas[uc_hash_value]))
        {
            pst_wds_sta = (hmac_wds_stas_stru *)pst_entry;
            if (OAL_PTR_NULL == pst_wds_sta)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_table_timerout_handler::pst_sta null.hash idx %d}", (oal_uint32)uc_hash_value);
                ul_ret = OAL_ERR_CODE_PTR_NULL;
                break;
            }

            if (OAL_TIME_GET_RUNTIME(pst_wds_sta->ul_last_pkt_age, OAL_TIME_JIFFY) > pst_hmac_vap->st_wds_table.ul_wds_aging)
            {
                pst_node = pst_wds_sta->pst_related_node;
                if((OAL_SUCC == hmac_wds_del_sta_entry(pst_hmac_vap, pst_wds_sta->auc_mac, pst_wds_sta))
                    && (OAL_PTR_NULL != pst_node) && (0 == pst_node->uc_stas_num))
                {
                    hmac_wds_del_node_entry(pst_hmac_vap, pst_node->auc_mac, pst_node);
                }
            }
        }
    }
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    return ul_ret;
}


oal_uint32  hmac_wds_node_ergodic(
                hmac_vap_stru        *pst_hmac_vap,
                oal_uint8            *src_addr,
                p_hmac_wds_node_func pst_hmac_wds_node,
                oal_void             *pst_arg)
{
    oal_uint32                 find_sta_ret = OAL_FAIL;
    hmac_wds_stas_stru         *pst_wds_sta = OAL_PTR_NULL;
    oal_uint32                 ul_hash_value = 0;
    hmac_wds_node_stru         *pst_node = OAL_PTR_NULL;
    oal_dlist_head_stru        *pst_entry = OAL_PTR_NULL;

    if ((pst_hmac_vap == OAL_PTR_NULL) || (pst_hmac_wds_node == OAL_PTR_NULL))
    {
        return OAL_FAIL;
    }

    oal_rw_lock_read_lock(&pst_hmac_vap->st_wds_table.st_lock);

    if (pst_hmac_vap->st_wds_table.uc_wds_node_num == 0)
    {
        oal_rw_lock_read_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_SUCC;
    }

    /* 查找报文源MAC是否是Root AP WDS Table里的STA */
    find_sta_ret = hmac_wds_find_sta(pst_hmac_vap, src_addr, &pst_wds_sta);

    for (ul_hash_value = 0; ul_hash_value < WDS_HASH_NUM; ul_hash_value++)
    {
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->st_wds_table.st_peer_node[ul_hash_value]))
        {
            pst_node = (hmac_wds_node_stru *)pst_entry;

            if (OAL_PTR_NULL == pst_node)
            {
                OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_node_ergodic::pst_node null.node idx %d}",ul_hash_value);
                continue;
            }

            /* 如果检测到广播报文源地址是对应repeater ap下的sta 不应该往这个repeater发送广播报文 */
            if ((find_sta_ret == OAL_SUCC) && (pst_wds_sta != OAL_PTR_NULL) && (pst_wds_sta->pst_related_node != OAL_PTR_NULL)
                && !oal_compare_mac_addr(pst_node->auc_mac, pst_wds_sta->pst_related_node->auc_mac))
            {
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_node_ergodic::repeater ap broadcast pkt, cannot send back!}");
                continue;
            }

            /* 执行函数 */
            (*pst_hmac_wds_node)(pst_hmac_vap, pst_node->auc_mac, pst_arg);
        }
    }

    oal_rw_lock_read_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  hmac_wds_find_neigh(
                hmac_vap_stru                   *pst_hmac_vap,
                oal_uint8                       *puc_addr,
                hmac_wds_neigh_stru             **ppst_neigh)
{
    oal_uint32                  ul_hash_value = 0;
    hmac_wds_neigh_stru         *pst_neigh = OAL_PTR_NULL;
    oal_dlist_head_stru         *pst_entry = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap)
                  || (OAL_PTR_NULL == puc_addr)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_find_neigh::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_hash_value = WDS_CALC_MAC_HASH_VAL(puc_addr);

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->st_wds_table.st_neigh[ul_hash_value]))
    {
        pst_neigh = (hmac_wds_neigh_stru *)pst_entry;

        if (OAL_PTR_NULL == pst_neigh)
        {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_find_neigh::pst_sta null.sta idx %d}",ul_hash_value);
            continue;
        }

        /* 相同的MAC地址 */
        if (!oal_compare_mac_addr(pst_neigh->auc_mac, puc_addr))
        {
            *ppst_neigh = pst_neigh;
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


oal_uint32 hmac_wds_update_neigh(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_mac)
{
    oal_uint32              ul_hash_value = 0;
    hmac_wds_neigh_stru     *pst_neigh = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap)
                  || (OAL_PTR_NULL == puc_mac)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WDS, "{hmac_wds_update_neigh::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    /* Find related mac already exist */
    if (OAL_SUCC == hmac_wds_find_neigh(pst_hmac_vap, puc_mac, &pst_neigh))
    {
        pst_neigh->ul_last_pkt_age = OAL_TIME_JIFFY;
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_SUCC;
    }

    if(pst_hmac_vap->st_wds_table.uc_neigh_num >= WDS_MAX_NEIGH_NUM)
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WDS, "{hmac_wds_update_neigh::Exceed the max wds sa from br number %d.Fail to add for mac [xx:xx:xx:%02x:%02x:%02x].}",
            pst_hmac_vap->st_wds_table.uc_neigh_num, puc_mac[3], puc_mac[4], puc_mac[5]);
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_FAIL;
    }

    /* Add to hash table if not exist */
    pst_neigh = (hmac_wds_neigh_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_wds_neigh_stru), OAL_TRUE);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_neigh))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_update_neigh::OAL_MEM_ALLOC wds sa from br struct fail!}");
        oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(pst_neigh, OAL_SIZEOF(hmac_wds_neigh_stru));
    oal_set_mac_addr(pst_neigh->auc_mac, puc_mac);
    pst_neigh->ul_last_pkt_age = OAL_TIME_JIFFY;
    ul_hash_value = WDS_CALC_MAC_HASH_VAL(puc_mac);
    oal_dlist_add_head(&(pst_neigh->st_entry), &pst_hmac_vap->st_wds_table.st_neigh[ul_hash_value]);

    pst_hmac_vap->st_wds_table.uc_neigh_num++;
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    return OAL_SUCC;
}


oal_uint32 hmac_wds_neigh_not_expired(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *puc_mac)
{
    oal_uint32                ul_ret = OAL_FAIL;
    hmac_wds_neigh_stru       *pst_neigh = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap)
                  || (OAL_PTR_NULL == puc_mac)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_wds_neigh_not_expired::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    /* Find related mac already exist */
    ul_ret = hmac_wds_find_neigh(pst_hmac_vap, puc_mac, &pst_neigh);

    /* Delete the entry if expired */
    if ((OAL_SUCC == ul_ret)
        && (OAL_TIME_GET_RUNTIME(pst_neigh->ul_last_pkt_age, OAL_TIME_JIFFY) > pst_hmac_vap->st_wds_table.ul_wds_aging))
    {
        ul_ret = OAL_FAIL;
        oal_dlist_delete_entry(&(pst_neigh->st_entry));
        OAL_MEM_FREE(pst_neigh, OAL_TRUE);
        pst_hmac_vap->st_wds_table.uc_neigh_num--;
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_neigh_not_expired::Expired for br neigh[xx:xx:%02x:%02x:%02x:%02x].}",
            pst_neigh->auc_mac[2], pst_neigh->auc_mac[3], pst_neigh->auc_mac[4], pst_neigh->auc_mac[5]);
    }
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    return ul_ret;
}


oal_uint32  hmac_wds_reset_neigh_table(hmac_vap_stru *pst_hmac_vap)
{
    oal_uint8                       uc_hash_value = 0;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_next_entry = OAL_PTR_NULL;
    hmac_wds_neigh_stru             *pst_neigh = OAL_PTR_NULL;
    oal_uint8                       *puc_addr;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (WDS_MODE_REPEATER_STA != pst_hmac_vap->st_wds_table.en_wds_vap_mode))
    {
        return OAL_SUCC;
    }

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    for (uc_hash_value = 0; uc_hash_value < WDS_HASH_NUM; uc_hash_value++)
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_reset_neigh_table::hash idx %d}", (oal_uint32)uc_hash_value);
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_next_entry, &(pst_hmac_vap->st_wds_table.st_neigh[uc_hash_value]))
        {
            pst_neigh = (hmac_wds_neigh_stru *)pst_entry;
            if (OAL_PTR_NULL == pst_neigh)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_reset_neigh_table::pst_sta null.hash idx %d}", (oal_uint32)uc_hash_value);
                ul_ret = OAL_FAIL;
                break;
            }

            oal_dlist_delete_entry(&(pst_neigh->st_entry));
            puc_addr = pst_neigh->auc_mac;

            OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_reset_neigh_table::del sa from bridge success for mac[%02x:%02x:%02x:%02x]}", puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);

            OAL_MEM_FREE(pst_neigh, OAL_TRUE);
            pst_hmac_vap->st_wds_table.uc_neigh_num--;

        }
    }

    if (0 != pst_hmac_vap->st_wds_table.uc_neigh_num)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_wds_reset_neigh_table::pst_vap->st_wds_table.uc_neigh_num[%d] != 0.}",
            pst_hmac_vap->st_wds_table.uc_neigh_num);

        pst_hmac_vap->st_wds_table.uc_neigh_num = 0;
    }
    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_wds_neigh_timeout_handler(oal_void *p_arg)
{
    oal_uint8                       uc_hash_value = 0;
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_next_entry = OAL_PTR_NULL;
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;
    hmac_wds_neigh_stru             *pst_neigh = OAL_PTR_NULL;
    oal_uint8                       *puc_addr;

    pst_hmac_vap = (hmac_vap_stru *)p_arg;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (WDS_MODE_REPEATER_STA != pst_hmac_vap->st_wds_table.en_wds_vap_mode))
    {
        return OAL_SUCC;
    }

    if (MAC_VAP_STATE_UP != pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
        return OAL_SUCC;
    }

    OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_neigh_timeout_handler::in}");

    oal_rw_lock_write_lock(&pst_hmac_vap->st_wds_table.st_lock);
    for (uc_hash_value = 0; uc_hash_value < WDS_HASH_NUM; uc_hash_value++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_next_entry, &(pst_hmac_vap->st_wds_table.st_neigh[uc_hash_value]))
        {
            pst_neigh = (hmac_wds_neigh_stru *)pst_entry;
            if (OAL_PTR_NULL == pst_neigh)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_neigh_timeout_handler::pst_sta null.hash idx %d}", (oal_uint32)uc_hash_value);
                ul_ret = OAL_FAIL;
                break;
            }

            if (OAL_TIME_GET_RUNTIME(pst_neigh->ul_last_pkt_age, OAL_TIME_JIFFY) > pst_hmac_vap->st_wds_table.ul_wds_aging)
            {
                oal_dlist_delete_entry(&(pst_neigh->st_entry));
                puc_addr = pst_neigh->auc_mac;

                OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_neigh_timeout_handler::del sa from bridge success for mac[%02x:%02x:%02x:%02x]}", puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);

                OAL_MEM_FREE(pst_neigh, OAL_TRUE);
                pst_hmac_vap->st_wds_table.uc_neigh_num--;
            }

        }
    }

    oal_rw_lock_write_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    return ul_ret;
}


oal_uint32  hmac_wds_table_create_timer(hmac_vap_stru *pst_hmac_vap)
{
    if ((OAL_PTR_NULL == pst_hmac_vap) || (WDS_MODE_NONE == pst_hmac_vap->st_wds_table.en_wds_vap_mode))
    {
        return OAL_SUCC;
    }

    /* 删除WDS定时器 */
    if (OAL_TRUE == pst_hmac_vap->st_wds_table.st_wds_timer.en_is_registerd)
    {
        FRW_TIMER_DESTROY_TIMER(&(pst_hmac_vap->st_wds_table.st_wds_timer));
    }

    if (WDS_MODE_ROOTAP == pst_hmac_vap->st_wds_table.en_wds_vap_mode)
    {
        FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap->st_wds_table.st_wds_timer),
                               hmac_wds_table_timerout_handler,
                               WDS_TABLE_DEF_TIMER,
                               (oal_void*)pst_hmac_vap,
                               OAL_TRUE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
    }
    else if (WDS_MODE_REPEATER_STA == pst_hmac_vap->st_wds_table.en_wds_vap_mode)
    {
        FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap->st_wds_table.st_wds_timer),
                               hmac_wds_neigh_timeout_handler,
                               WDS_TABLE_DEF_TIMER,
                               (oal_void*)pst_hmac_vap,
                               OAL_TRUE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_WDS

oal_uint32  hmac_wds_vap_show_all(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_param)
{
    oal_uint8                       uc_hash_value = 0;
    oal_int8                        *pc_print_buff = OAL_PTR_NULL;
    oal_uint32                      ul_str_len = 0;
    oal_uint32                      ul_str_idx = 0;
    oal_uint32                      ul_str_left = OAM_REPORT_MAX_STRING_LEN;
    oal_uint32                      ul_cur_time = 0;
    hmac_wds_stas_stru              *pst_sta = OAL_PTR_NULL;
    hmac_wds_node_stru              *pst_node = OAL_PTR_NULL;
    hmac_wds_neigh_stru             *pst_br_sa = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_entry = OAL_PTR_NULL;
    oal_wds_info_stru               *pst_wds_info = (puc_param ? (oal_wds_info_stru*)puc_param : OAL_PTR_NULL);
    oal_wds_node_stru                st_wds_node;
    oal_wds_stas_stru                st_wds_sta;
    oal_wds_neigh_stru               st_wds_neigh;
    oal_uint8                        uc_idx = 0;

    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_print_buff)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_wds_vap_show_all::pc_print_buff null,string_len=%d.}", OAM_REPORT_MAX_STRING_LEN);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);

    oal_rw_lock_read_lock(&pst_hmac_vap->st_wds_table.st_lock);
    ul_str_len = OAL_SPRINTF(pc_print_buff + ul_str_idx, ul_str_left,
            "vap%d\n"
            "\t wds vap mode:           %d.\n"
            "\t wds node number:        %d.\n"
            "\t wds sta number:         %d.\n"
            "\t wds br neigh number:    %d.\n"
            "\t wds entry aging time:   %u.\n\n",
            pst_hmac_vap->st_vap_base_info.uc_vap_id,
            pst_hmac_vap->st_wds_table.en_wds_vap_mode,
            (oal_uint32)pst_hmac_vap->st_wds_table.uc_wds_node_num,
            (oal_uint32)pst_hmac_vap->st_wds_table.uc_wds_stas_num,
            (oal_uint32)pst_hmac_vap->st_wds_table.uc_neigh_num,
            pst_hmac_vap->st_wds_table.ul_wds_aging/OAL_TIME_HZ);

    ul_str_idx += ul_str_len;
    ul_str_left -= ul_str_len;
    ul_cur_time = OAL_TIME_JIFFY;

    if (pst_hmac_vap->st_wds_table.uc_wds_stas_num) {
        ul_str_len = OAL_SPRINTF(pc_print_buff + ul_str_idx, ul_str_left,
                "\nWDS Node Table:\n"
                "\tWDS Sta              Last Active    Peer Node            Last Active\n");
        ul_str_idx += ul_str_len;
        ul_str_left -= ul_str_len;
    }

    for (uc_hash_value = 0; uc_hash_value < WDS_HASH_NUM; uc_hash_value++)
    {
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->st_wds_table.st_wds_stas[uc_hash_value]))
        {
            pst_sta = (hmac_wds_stas_stru *)pst_entry;

            if ((OAL_PTR_NULL == pst_sta) || (OAL_PTR_NULL == (pst_node = pst_sta->pst_related_node)))
            {
                OAM_WARNING_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_vap_show_all::pst_sta|pst_node is null.sta idx %d, pst_sta: %x, pst_node: %x}",uc_hash_value, (oal_uint32 *)pst_sta, (oal_uint32 *)pst_node);
                break;
            }

            ul_str_len = OAL_SPRINTF(pc_print_buff + ul_str_idx, ul_str_left,
                    "\t%02x:%02x:%02x:%02x:%02x:%02x    %-15u"
                    "%02x:%02x:%02x:%02x:%02x:%02x    %d\n",
                    pst_sta->auc_mac[0], pst_sta->auc_mac[1], pst_sta->auc_mac[2],
                    pst_sta->auc_mac[3], pst_sta->auc_mac[4], pst_sta->auc_mac[5],
                    (ul_cur_time - pst_sta->ul_last_pkt_age)/OAL_TIME_HZ,
                    pst_node->auc_mac[0], pst_node->auc_mac[1], pst_node->auc_mac[2],
                    pst_node->auc_mac[3], pst_node->auc_mac[4], pst_node->auc_mac[5],
                    pst_node->uc_stas_num);
            ul_str_idx += ul_str_len;
            ul_str_left -= ul_str_len;

            if(OAL_PTR_NULL != pst_wds_info && (OAL_PTR_NULL != pst_wds_info->pst_wds_stas
                && uc_idx < OAL_MIN(pst_wds_info->uc_wds_stas_num, pst_hmac_vap->st_wds_table.uc_wds_stas_num)))
            {
                OAL_MEMZERO(&st_wds_sta, OAL_SIZEOF(oal_wds_stas_stru));
                oal_memcopy(&st_wds_sta.auc_mac, &pst_sta->auc_mac, WLAN_MAC_ADDR_LEN);
                oal_memcopy(&st_wds_sta.st_related_node.auc_mac, &pst_node->auc_mac, WLAN_MAC_ADDR_LEN);
                st_wds_sta.st_related_node.uc_stas_num = pst_node->uc_stas_num;
                st_wds_sta.ul_last_pkt_age = (ul_cur_time - pst_sta->ul_last_pkt_age)/OAL_TIME_HZ;
                oal_copy_to_user(pst_wds_info->pst_wds_stas + uc_idx, &st_wds_sta, OAL_SIZEOF(oal_wds_stas_stru));
                uc_idx++;
            }
        }

        if (ul_str_left < 500)
        {
            oam_print_etc(pc_print_buff);
            OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);

            ul_str_len = 0;
            ul_str_idx = 0;
            ul_str_left = OAM_REPORT_MAX_STRING_LEN;
        }
    }

    if (pst_hmac_vap->st_wds_table.uc_wds_node_num) {
        ul_str_len = OAL_SPRINTF(pc_print_buff + ul_str_idx, ul_str_left,
               "Peer Node:\n"
               "\tPeer Node            Last Active\n");
        ul_str_idx += ul_str_len;
        ul_str_left -= ul_str_len;
    }

    uc_idx = 0;
    for (uc_hash_value = 0; uc_hash_value < WDS_HASH_NUM; uc_hash_value++)
    {
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->st_wds_table.st_peer_node[uc_hash_value]))
        {
            pst_node = (hmac_wds_node_stru *)pst_entry;

            if (OAL_PTR_NULL == pst_node)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_vap_show_all::pst_node is null.sta idx %d}",uc_hash_value);
                break;
            }

            ul_str_len = OAL_SPRINTF(pc_print_buff + ul_str_idx, ul_str_left,
                    "\t%02x:%02x:%02x:%02x:%02x:%02x    %u\n",
                    pst_node->auc_mac[0], pst_node->auc_mac[1], pst_node->auc_mac[2],
                    pst_node->auc_mac[3], pst_node->auc_mac[4], pst_node->auc_mac[5],
                    pst_node->uc_stas_num);
            ul_str_idx += ul_str_len;
            ul_str_left -= ul_str_len;

            if(OAL_PTR_NULL != pst_wds_info && (OAL_PTR_NULL != pst_wds_info->pst_peer_node
                && uc_idx < OAL_MIN(pst_wds_info->uc_wds_node_num, pst_hmac_vap->st_wds_table.uc_wds_node_num)))
            {
                OAL_MEMZERO(&st_wds_node, OAL_SIZEOF(oal_wds_node_stru));
                oal_memcopy(&st_wds_node.auc_mac, &pst_node->auc_mac, WLAN_MAC_ADDR_LEN);
                st_wds_node.uc_stas_num = pst_node->uc_stas_num;
                oal_copy_to_user(pst_wds_info->pst_peer_node + uc_idx, &st_wds_node, OAL_SIZEOF(oal_wds_node_stru));
                uc_idx++;
            }

        }

        if (ul_str_left < 500)
        {
            oam_print_etc(pc_print_buff);
            OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);

            ul_str_len = 0;
            ul_str_idx = 0;
            ul_str_left = OAM_REPORT_MAX_STRING_LEN;
        }
    }

    if (pst_hmac_vap->st_wds_table.uc_neigh_num) {
        ul_str_len = OAL_SPRINTF(pc_print_buff + ul_str_idx, ul_str_left,
                "Bridge neighbours:\n"
                "\tNeighbours           Last Active\n");
        ul_str_idx += ul_str_len;
        ul_str_left -= ul_str_len;
    }

    uc_idx = 0;
    for (uc_hash_value = 0; uc_hash_value < WDS_HASH_NUM; uc_hash_value++)
    {
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_hmac_vap->st_wds_table.st_neigh[uc_hash_value]))
        {
            pst_br_sa = (hmac_wds_neigh_stru *)pst_entry;

            if (OAL_PTR_NULL == pst_br_sa)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_wds_vap_show_all::pst_br_sa is null.sta idx %d}",uc_hash_value);
                break;
            }

            ul_str_len = OAL_SPRINTF(pc_print_buff + ul_str_idx, ul_str_left,
                    "\t%02x:%02x:%02x:%02x:%02x:%02x    %u\n",
                    pst_br_sa->auc_mac[0], pst_br_sa->auc_mac[1], pst_br_sa->auc_mac[2],
                    pst_br_sa->auc_mac[3], pst_br_sa->auc_mac[4], pst_br_sa->auc_mac[5],
                    (ul_cur_time - pst_br_sa->ul_last_pkt_age)/OAL_TIME_HZ);
            ul_str_idx += ul_str_len;
            ul_str_left -= ul_str_len;

            if(OAL_PTR_NULL != pst_wds_info && (OAL_PTR_NULL != pst_wds_info->pst_neigh
                && uc_idx < OAL_MIN(pst_wds_info->uc_neigh_num, pst_hmac_vap->st_wds_table.uc_neigh_num)))
            {
                OAL_MEMZERO(&st_wds_neigh, OAL_SIZEOF(oal_wds_neigh_stru));
                oal_memcopy(&st_wds_neigh.auc_mac, &pst_br_sa->auc_mac, WLAN_MAC_ADDR_LEN);
                st_wds_neigh.ul_last_pkt_age = (ul_cur_time - pst_sta->ul_last_pkt_age)/OAL_TIME_HZ;
                oal_copy_to_user(pst_wds_info->pst_neigh + uc_idx, &st_wds_neigh, OAL_SIZEOF(oal_wds_neigh_stru));
                uc_idx++;
            }

        }

        if (ul_str_left < 500)
        {
            oam_print_etc(pc_print_buff);
            OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);

            ul_str_len = 0;
            ul_str_idx = 0;
            ul_str_left = OAM_REPORT_MAX_STRING_LEN;
        }
    }
    oal_rw_lock_read_unlock(&pst_hmac_vap->st_wds_table.st_lock);

    oam_print_etc(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);

    return OAL_SUCC;
}



oal_uint32  hmac_vap_set_wds_user(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *pst_addr)
{
    oal_uint16              us_user_idx   = 0xffff;
    oal_uint32              ul_ret = OAL_SUCC;
    hmac_user_stru          *pst_hmac_user = OAL_PTR_NULL;

    if (WDS_MODE_NONE == pst_hmac_vap->st_wds_table.en_wds_vap_mode)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_vap_set_wds_user::not wds vap.}");
        return OAL_FAIL;
    }

    ul_ret = mac_vap_find_user_by_macaddr_etc(&(pst_hmac_vap->st_vap_base_info), pst_addr, &us_user_idx);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_vap_set_wds_user::mac_vap_get_user_by_addr_etc failed. mac[XX%02x:%02x:%02x%02x]}", pst_addr[2], pst_addr[3], pst_addr[4], pst_addr[5]);
        return OAL_FAIL;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_user_idx);
    pst_hmac_user->uc_is_wds = OAL_TRUE;

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA

/* 4地址ie check接口 用于驱动上报帧给产品,产品判断后输出用户是否支持4地址 */
typedef oal_int32 (*p_vmsta_a4_check_func)(oal_uint8 *puc_data, oal_uint32 ul_len);

OAL_STATIC p_vmsta_a4_check_func g_p_vmsta_user_a4_support = OAL_PTR_NULL;
OAL_STATIC p_vmsta_a4_check_func g_p_vmsta_vap_a4_support = OAL_PTR_NULL;

oal_void hmac_vmsta_user_check_reg(p_vmsta_a4_check_func p_func)
{
    g_p_vmsta_user_a4_support = p_func;
}

oal_void hmac_vmsta_vap_check_reg(p_vmsta_a4_check_func p_func)
{
    g_p_vmsta_vap_a4_support = p_func;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_vmsta_user_check_reg);
oal_module_symbol(hmac_vmsta_vap_check_reg);
/*lint +e578*//*lint +e19*/


oal_bool_enum_uint8 hmac_vmsta_get_user_a4_support(
                hmac_vap_stru           *pst_hmac_vap,
                oal_uint8               *pst_addr)
{
    oal_uint16              us_user_idx   = 0xffff;
    oal_uint32              ul_ret = OAL_SUCC;
    hmac_user_stru          *pst_hmac_user = OAL_PTR_NULL;

    ul_ret = mac_vap_find_user_by_macaddr_etc(&(pst_hmac_vap->st_vap_base_info), pst_addr, &us_user_idx);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_vmsta_get_user_a4_support::mac_vap_get_user_by_addr_etc failed. mac[XX%02x:%02x:%02x%02x]}", pst_addr[2], pst_addr[3], pst_addr[4], pst_addr[5]);
        return OAL_FALSE;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_user_idx);
    /* 复用wds用户标识，如果能力交互是用户支持4地址通信，则会设置该标识为TRUE */
    return pst_hmac_user->uc_is_wds;
}


oal_uint32 hmac_vmsta_set_vap_a4_enable(mac_vap_stru *pst_mac_vap)
{
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_vmsta_set_vap_a4_enable::mac_res_get_hmac_vap null pointer}");
        return OAL_FAIL;
    }
    /* 复用wds VAP标识，如果上层传入4地址IE，则会设置VAP为对应的wds模式 */
    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        pst_hmac_vap->st_wds_table.en_wds_vap_mode = WDS_MODE_ROOTAP;
    }
    else if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
        pst_hmac_vap->st_wds_table.en_wds_vap_mode = WDS_MODE_REPEATER_STA;
    }
    else
    {
        pst_hmac_vap->st_wds_table.en_wds_vap_mode = WDS_MODE_NONE;
    }
    return OAL_SUCC;
}


oal_bool_enum_uint8 hmac_vmsta_check_vap_a4_support(oal_uint8 *puc_ie, oal_uint32 ul_ie_len)
{
    /* 产品已注册接口则调用接口进行a4 check */
    if (OAL_PTR_NULL != g_p_vmsta_vap_a4_support)
    {
        if (VMSTA_4ADDR_SUPPORT == g_p_vmsta_vap_a4_support(puc_ie, ul_ie_len))
        {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}


oal_bool_enum_uint8 hmac_vmsta_check_user_a4_support(oal_uint8 *puc_frame, oal_uint32 ul_frame_len)
{
    /* 产品已注册接口则调用接口进行a4 check */
    if (OAL_PTR_NULL != g_p_vmsta_user_a4_support)
    {
        if (VMSTA_4ADDR_SUPPORT == g_p_vmsta_user_a4_support(puc_frame, ul_frame_len))
        {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}

#endif  //_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA


oal_void hmac_wds_init_table(hmac_vap_stru *pst_hmac_vap)
{
    oal_uint32              ul_loop = 0;

    oal_rw_lock_init(&pst_hmac_vap->st_wds_table.st_lock);

    pst_hmac_vap->st_wds_table.en_wds_vap_mode = WDS_MODE_NONE;
    pst_hmac_vap->st_wds_table.uc_wds_node_num = 0;
    pst_hmac_vap->st_wds_table.uc_wds_stas_num = 0;
    pst_hmac_vap->st_wds_table.uc_neigh_num = 0;
    pst_hmac_vap->st_wds_table.ul_wds_aging = WDS_AGING_TIME;

    for (ul_loop = 0; ul_loop < WDS_HASH_NUM; ul_loop++)
    {
        oal_dlist_init_head(&(pst_hmac_vap->st_wds_table.st_peer_node[ul_loop]));
        oal_dlist_init_head(&(pst_hmac_vap->st_wds_table.st_wds_stas[ul_loop]));
        oal_dlist_init_head(&(pst_hmac_vap->st_wds_table.st_neigh[ul_loop]));
    }
}


OAL_STATIC oal_uint32 hmac_wds_tx_event( hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf)
{
    frw_event_stru          *pst_event;        /* 事件结构体 */
    frw_event_mem_stru      *pst_event_mem;
    dmac_tx_event_stru      *pst_dtx_stru;
    oal_uint32               ul_ret = OAL_SUCC;

    /* 封装802.11头 */
     ul_ret = hmac_tx_encap_etc(pst_vap, pst_user, pst_buf);
     if (OAL_UNLIKELY((OAL_SUCC != ul_ret)))
     {
         OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
                          "{hmac_wds_tx_event::hmac_tx_encap_etc failed[%d].}", ul_ret);
         OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
         return ul_ret;
     }

     /* 抛事件，传给DMAC */
     pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
     if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
     {
         OAM_WARNING_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_wds_tx_event::pst_event_mem null.}");
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
         pst_vap->st_vap_base_info.st_vap_stats.ul_tx_dropped_packets++;
#endif
         return OAL_ERR_CODE_ALLOC_MEM_FAIL;
     }

     pst_event = frw_get_event_stru(pst_event_mem);

     /* 填写事件头 */
     FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                      FRW_EVENT_TYPE_HOST_DRX,
                      DMAC_TX_HOST_DRX,
                      OAL_SIZEOF(dmac_tx_event_stru),
                      FRW_EVENT_PIPELINE_STAGE_1,
                      pst_vap->st_vap_base_info.uc_chip_id,
                      pst_vap->st_vap_base_info.uc_device_id,
                      pst_vap->st_vap_base_info.uc_vap_id);

     pst_dtx_stru             = (dmac_tx_event_stru *)pst_event->auc_event_data;
     pst_dtx_stru->pst_netbuf = pst_buf;

     /* 调度事件 */
     ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
     if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
     {
         OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_wds_tx_event::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
         OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
     }

     /* 释放事件 */
     FRW_EVENT_FREE(pst_event_mem);

     return ul_ret;
}



oal_uint32 hmac_wds_tx_broadcast_pkt(hmac_vap_stru *pst_vap, oal_uint8 *puc_addr, oal_void *pst_arg)
{
    oal_netbuf_stru         *pst_buf = (oal_netbuf_stru *)pst_arg;
    oal_netbuf_stru         *pst_copy_buf;
    hmac_user_stru          *pst_user;      /* 目标STA结构体 */
    oal_uint32               ul_ret;
    oal_uint16               us_user_idx = MAC_INVALID_USER_ID;

    ul_ret = mac_vap_find_user_by_macaddr_etc(&(pst_vap->st_vap_base_info), puc_addr, &us_user_idx);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        return OAL_FALSE;
    }

    pst_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_user_idx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_user))
    {
        return OAL_FALSE;
    }

    pst_copy_buf = oal_netbuf_copy(pst_buf, GFP_ATOMIC);
    ul_ret = hmac_wds_tx_event(pst_vap, pst_user, pst_copy_buf);
    if (OAL_UNLIKELY((OAL_SUCC != ul_ret) ))
    {
        if (OAL_PTR_NULL != pst_copy_buf)
        {
            oal_netbuf_free(pst_copy_buf);
        }
        return OAL_FALSE;
    }

    OAM_WARNING_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,
        "{hmac_wds_tx_broadcast_pkt::send broadcast pkt to mac[%02x:%02x:%02x:%02x]}",
        puc_addr[2], puc_addr[3], puc_addr[4], puc_addr[5]);

    return OAL_TRUE;
}


oal_module_symbol(hmac_wds_update_table);
oal_module_symbol(hmac_wds_add_node);
oal_module_symbol(hmac_wds_del_node);
oal_module_symbol(hmac_wds_reset_sta_mapping_table);
oal_module_symbol(hmac_wds_add_sta);
oal_module_symbol(hmac_wds_del_sta);
oal_module_symbol(hmac_find_valid_user_by_wds_sta);
oal_module_symbol(hmac_wds_node_ergodic);
oal_module_symbol(hmac_wds_update_neigh);
oal_module_symbol(hmac_wds_neigh_not_expired);
oal_module_symbol(hmac_wds_reset_neigh_table);
oal_module_symbol(hmac_wds_table_create_timer);
#ifdef _PRE_WLAN_FEATURE_WDS
oal_module_symbol(hmac_wds_vap_show_all);
oal_module_symbol(hmac_vap_set_wds_user);
#endif
oal_module_symbol(hmac_wds_init_table);
oal_module_symbol(hmac_wds_find_sta);
oal_module_symbol(hmac_wds_tx_broadcast_pkt);

#endif /* #if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA) */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


