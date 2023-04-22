


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_m2u.h"
#include "hmac_tx_data.h"

#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)
#include "hmac_wds.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_M2U_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/



oal_uint32 hmac_m2u_add_member_list(hmac_m2u_grp_list_entry_stru *pst_grp_list, hmac_m2u_list_update_stru *pst_list_entry)
{
    hmac_m2u_grp_member_stru *pst_grp_member;
    hmac_m2u_stru *pst_m2u_struct = (hmac_m2u_stru*)pst_list_entry->pst_hmac_vap->pst_m2u;
    oal_uint32 ul_ret             = OAL_SUCC;

    if ((pst_grp_list->uc_sta_num >= MAX_STA_NUM_OF_ONE_GROUP) ||
        (pst_m2u_struct->st_m2u_snooplist.us_total_sta_num >= MAX_STA_NUM_OF_ALL_GROUP))
    {
        OAM_WARNING_LOG2(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_add_member_list::sta num out of limit!current total_sta_num:[%d] ,sta_num:[%d].}",
            pst_m2u_struct->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
                                                               OAL_SIZEOF(hmac_m2u_grp_member_stru),
                                                               OAL_TRUE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_grp_member))
    {
        OAM_ERROR_LOG0(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_add_member_list:: OAL_MEM_ALLOC failed!!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memset(pst_grp_member, 0x0,  OAL_SIZEOF(hmac_m2u_grp_member_stru));
    oal_dlist_add_tail(&(pst_grp_member->st_member_entry), &(pst_grp_list->st_src_list));
    oal_set_ip_addr(pst_grp_member->auc_src_ip_addr , pst_list_entry->auc_src_ip_addr);
    oal_set_mac_addr(pst_grp_member->auc_grp_member_mac, pst_list_entry->auc_new_member_mac);

    pst_grp_member->pst_hmac_user       = pst_list_entry->pst_hmac_user;
    pst_grp_member->en_mode             = pst_list_entry->en_cmd;
    pst_grp_member->ul_timestamp        = pst_list_entry->ul_timestamp;
    pst_grp_member->uc_src_ip_addr_len  = pst_list_entry->uc_src_ip_addr_len;
    pst_grp_list->uc_sta_num++;
    pst_m2u_struct->st_m2u_snooplist.us_total_sta_num++;

    OAM_WARNING_LOG2(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_add_member_list::add one sta !current total_sta_num:[%d] ,sta_num:[%d].}",
                     pst_m2u_struct->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);
    return ul_ret;
}


oal_uint32  hmac_m2u_remove_expired_member(hmac_m2u_grp_list_entry_stru *pst_grp_list,
                                              hmac_vap_stru  *pst_hmac_vap,
                                              oal_uint32 ul_nowtimestamp)
{
    hmac_m2u_grp_member_stru *pst_grp_member;
    oal_dlist_head_stru      *pst_grp_member_entry;
    oal_dlist_head_stru      *pst_grp_member_entry_temp;
    hmac_m2u_stru            *pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    oal_uint32                ul_ret = OAL_SUCC;

    if (OAL_PTR_NULL == pst_m2u)
    {
        return OAL_FAIL;
    }

    /* 遍历一个组，每次取出组中成员，超时则删除该成员 */
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_grp_member_entry, pst_grp_member_entry_temp, &(pst_grp_list->st_src_list))
    {
        pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_DLIST_GET_ENTRY(pst_grp_member_entry,
                                                                           hmac_m2u_grp_member_stru,
                                                                           st_member_entry);
        if (OAL_TIME_GET_RUNTIME((pst_grp_member->ul_timestamp), ul_nowtimestamp) > (pst_m2u->ul_timeout))
        {
            oal_dlist_delete_entry(&(pst_grp_member->st_member_entry));
            OAL_MEM_FREE(pst_grp_member, OAL_TRUE);

            OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_remove_expired_member:: current total sta num:[%d],sta num:[%d]}\n",
                            pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);

            pst_grp_list->uc_sta_num--;
            pst_m2u->st_m2u_snooplist.us_total_sta_num--;

            OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_remove_expired_member::remove one sta. current total sta num:[%d],sta num:[%d]}\n",
                            pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);
        }
    }
    return ul_ret;
}


OAL_STATIC oal_void  hmac_m2u_remove_one_member(hmac_m2u_grp_list_entry_stru *pst_grp_list,
                                                        hmac_vap_stru  *pst_hmac_vap,
                                                                oal_uint8 *puc_grp_member_addr)
{
    hmac_m2u_grp_member_stru *pst_grp_member;
    oal_dlist_head_stru      *pst_grp_member_entry;
    oal_dlist_head_stru      *pst_grp_member_entry_temp;
    hmac_m2u_stru            *pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_PTR_NULL == pst_m2u)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_remove_one_member::pst_m2u is null}");
        return;
    }

    /* 遍历一个组播组删除指定成员 */
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_grp_member_entry, pst_grp_member_entry_temp, &(pst_grp_list->st_src_list))
    {
        pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_DLIST_GET_ENTRY(pst_grp_member_entry,
                                                                         hmac_m2u_grp_member_stru,
                                                                         st_member_entry);

        if (!oal_compare_mac_addr(puc_grp_member_addr, pst_grp_member->auc_grp_member_mac))
        {
            oal_dlist_delete_entry(&(pst_grp_member->st_member_entry));
            OAL_MEM_FREE(pst_grp_member, OAL_TRUE);

            OAM_WARNING_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_remove_one_member::current sta_num:%d  total_sta_num:%d  group_num:%d}",
                              pst_grp_list->uc_sta_num, pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_m2u->st_m2u_snooplist.us_group_list_count);

            pst_grp_list->uc_sta_num--;
            pst_m2u->st_m2u_snooplist.us_total_sta_num--;

            OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_remove_one_member::remove one sta from group:0x%02x 0x%02x 0x%02x 0x%02x!}",
                              pst_grp_list->auc_group_mac[2],pst_grp_list->auc_group_mac[3],pst_grp_list->auc_group_mac[3],pst_grp_list->auc_group_mac[5]);
            OAM_WARNING_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_remove_one_member::current sta_num:%d  total_sta_num:%d  group_num:%d}",
                              pst_grp_list->uc_sta_num, pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_m2u->st_m2u_snooplist.us_group_list_count);
        }
    }
}


OAL_STATIC hmac_m2u_grp_member_stru *hmac_m2u_find_member_src(hmac_m2u_grp_list_entry_stru *pst_grp_list,
                                                                        oal_uint8 *puc_grp_member_addr,
                                                                        oal_uint8 * puc_src_ip_addr)
{
    hmac_m2u_grp_member_stru *pst_grp_member;
    oal_dlist_head_stru      *pst_grp_member_entry;

    /* 遍历一个组播组，找到该组中src ip匹配的成员 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_grp_member_entry, &(pst_grp_list->st_src_list))
    {
        pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_DLIST_GET_ENTRY(pst_grp_member_entry,
                                                                         hmac_m2u_grp_member_stru,
                                                                         st_member_entry);

        if (!oal_compare_mac_addr(puc_grp_member_addr, pst_grp_member->auc_grp_member_mac) &&
            !oal_memcmp(puc_src_ip_addr, pst_grp_member->auc_src_ip_addr,pst_grp_member->uc_src_ip_addr_len))
        {
            return pst_grp_member;
        }
    }
    return OAL_PTR_NULL;
}


OAL_STATIC hmac_m2u_grp_member_stru *hmac_m2u_find_member(hmac_m2u_grp_list_entry_stru *pst_grp_list,
                                                                   oal_uint8 *puc_grp_member_addr)
{
    hmac_m2u_grp_member_stru *pst_grp_member;
    oal_dlist_head_stru      *pst_grp_member_entry;

    /* 遍历一个组播组，找到该组中地址匹配的成员 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_grp_member_entry, &(pst_grp_list->st_src_list))
    {
        pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_DLIST_GET_ENTRY(pst_grp_member_entry,
                                                                         hmac_m2u_grp_member_stru,
                                                                         st_member_entry);

        if (!oal_compare_mac_addr(puc_grp_member_addr, pst_grp_member->auc_grp_member_mac))
        {
            return pst_grp_member;
        }
    }

    return OAL_PTR_NULL;
}



OAL_STATIC hmac_m2u_grp_list_entry_stru *hmac_m2u_find_group_list(hmac_vap_stru  *pst_hmac_vap ,hmac_m2u_list_update_stru *pst_list_entry)
{
    hmac_m2u_snoop_list_stru         *pst_snp_list;
    hmac_m2u_grp_list_entry_stru     *pst_grp_list_member;
    oal_dlist_head_stru              *pst_grp_list_entry;
    hmac_m2u_stru                    *pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_PTR_NULL == pst_m2u)
    {
        return OAL_PTR_NULL;
    }

    pst_snp_list = &(pst_m2u->st_m2u_snooplist);

    /* 遍历组播组链表，找到地址匹配的组播组 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_grp_list_entry, &(pst_snp_list->st_grp_list))
    {
        pst_grp_list_member = (hmac_m2u_grp_list_entry_stru *)OAL_DLIST_GET_ENTRY(pst_grp_list_entry,
                                                                               hmac_m2u_grp_list_entry_stru,
                                                                               st_grp_entry);

        if ((!oal_compare_mac_addr(pst_list_entry->auc_grp_mac, pst_grp_list_member->auc_group_mac)) &&
            (!oal_memcmp(&(pst_list_entry->st_outer_vlan_tag),&(pst_grp_list_member->st_outer_vlan_tag),OAL_SIZEOF(mac_vlan_tag_stru))))
        {
            return pst_grp_list_member;
        }
     }

    return OAL_PTR_NULL;
}


OAL_STATIC hmac_m2u_grp_list_entry_stru *hmac_m2u_create_grp_list(hmac_vap_stru *pst_hmac_vap, hmac_m2u_list_update_stru *pst_list_entry)
{
    hmac_m2u_snoop_list_stru         *pst_snp_list;
    hmac_m2u_grp_list_entry_stru     *pst_grp_list_member = OAL_PTR_NULL;
    hmac_m2u_stru                    *pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        return pst_grp_list_member;
    }

    pst_snp_list        = &(pst_m2u->st_m2u_snooplist);
    pst_grp_list_member = hmac_m2u_find_group_list(pst_hmac_vap, pst_list_entry);

    if (OAL_PTR_NULL == pst_grp_list_member)
    {
        if (pst_snp_list->us_group_list_count >= MAX_NUM_OF_GROUP)
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_create_grp_list::pst_snp_list->us_group_list_count is[%d].}",pst_snp_list->us_group_list_count);
            return OAL_PTR_NULL;
        }
        pst_grp_list_member= (hmac_m2u_grp_list_entry_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
                                                                   OAL_SIZEOF(hmac_m2u_grp_list_entry_stru), OAL_TRUE);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_grp_list_member))
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_create_grp_list::pst_grp_list_member null.}");
            return pst_grp_list_member;
        }
        oal_memset(pst_grp_list_member, 0x0,   OAL_SIZEOF(hmac_m2u_grp_list_entry_stru));
        oal_memcopy(&(pst_grp_list_member->st_outer_vlan_tag), &(pst_list_entry->st_outer_vlan_tag), OAL_SIZEOF(mac_vlan_tag_stru));
        oal_dlist_add_tail(&(pst_grp_list_member->st_grp_entry), &(pst_snp_list->st_grp_list));
        oal_set_mac_addr(pst_grp_list_member->auc_group_mac, pst_list_entry->auc_grp_mac);
        oal_dlist_init_head(&(pst_grp_list_member->st_src_list));
        pst_snp_list->us_group_list_count++;

        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_create_grp_list::create a new group.group num:[%d]}",pst_snp_list->us_group_list_count);
    }
    return pst_grp_list_member;
}


oal_uint32 hmac_m2u_update_snoop_list(hmac_m2u_list_update_stru *pst_list_entry)
{
    hmac_m2u_grp_list_entry_stru  *pst_grp_list;
    hmac_m2u_grp_member_stru      *pst_grp_member_list;
    hmac_m2u_stru                 *pst_m2u = (hmac_m2u_stru *)(pst_list_entry->pst_hmac_vap->pst_m2u);
    oal_uint32                     ul_ret = OAL_SUCC;

    pst_list_entry->ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    pst_grp_list = hmac_m2u_find_group_list(pst_list_entry->pst_hmac_vap, pst_list_entry);

    if (OAL_PTR_NULL == pst_grp_list)
    {
        if (HMAC_M2U_CMD_EXCLUDE_LIST == pst_list_entry->en_cmd)
        {
            OAM_WARNING_LOG0(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_update_snoop_list::find_grp_list null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        else
        {
             pst_grp_list = hmac_m2u_create_grp_list(pst_list_entry->pst_hmac_vap, pst_list_entry);
             if (OAL_UNLIKELY(OAL_PTR_NULL == pst_grp_list))
             {
                OAM_WARNING_LOG0(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_update_snoop_list::create_grp_list null.}");
                return OAL_ERR_CODE_PTR_NULL;
             }
        }
    }

    /* 如果待更新的节点有指定的组播源，如果链表中存在指定该组播源的该节点，则更新该节点的状态，否则重新创建加入链表 */
    if(!OAL_IPV6_IS_UNSPECIFIED_ADDR(pst_list_entry->auc_src_ip_addr))
    {
        OAM_INFO_LOG4(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_update_snoop_list::pst_list_entry is assigned src_ip_addr,the first 4 byte: [%x], [%x],[%x],[%x]}\r\n",
                                                    pst_list_entry->auc_src_ip_addr[0],
                                                    pst_list_entry->auc_src_ip_addr[1],
                                                    pst_list_entry->auc_src_ip_addr[2],
                                                    pst_list_entry->auc_src_ip_addr[3]);
        pst_grp_member_list = hmac_m2u_find_member_src(pst_grp_list, pst_list_entry->auc_new_member_mac, pst_list_entry->auc_src_ip_addr);
        if (OAL_PTR_NULL != pst_grp_member_list)
        {
            pst_grp_member_list->en_mode      = pst_list_entry->en_cmd;
            pst_grp_member_list->ul_timestamp = pst_list_entry->ul_timestamp;
        }
        else
        {
            ul_ret = hmac_m2u_add_member_list(pst_grp_list, pst_list_entry);
        }
    }
    else
    {
        pst_grp_member_list = hmac_m2u_find_member(pst_grp_list, pst_list_entry->auc_new_member_mac);
        if (OAL_PTR_NULL != pst_grp_member_list)
        {
            /* 存在该节点，并且该组中的该节点的有指定的组播源，清空组中的所有该节点，如果更新节点的cmd为INC 再add */
            if (!OAL_IPV6_IS_UNSPECIFIED_ADDR(pst_grp_member_list->auc_src_ip_addr))
            {
                hmac_m2u_remove_one_member(pst_grp_list, pst_list_entry->pst_hmac_vap, pst_list_entry->auc_new_member_mac);
                if (HMAC_M2U_CMD_INCLUDE_LIST == pst_grp_member_list->en_mode)
                {
                    OAM_INFO_LOG1(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_update_snoop_list::pst_grp_member_list->en_mode is [%d].}", pst_grp_member_list->en_mode);
                    ul_ret = hmac_m2u_add_member_list(pst_grp_list, pst_list_entry);
                }
            }
            /* 原组中的该节点未指定组播源，如果待更新节点的CMD为EXC，则删除原组播组中的该节点 */
            else if (HMAC_M2U_CMD_EXCLUDE_LIST == pst_list_entry->en_cmd)
            {
                OAM_INFO_LOG1(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_update_snoop_list::pst_list_entry->en_cmd is [%d].}", pst_list_entry->en_cmd);
                oal_dlist_delete_entry(&(pst_grp_member_list->st_member_entry));
                OAL_MEM_FREE(pst_grp_member_list, OAL_TRUE);

                OAM_WARNING_LOG2(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_update_snoop_list::current total_sta_num:[%d] ,sta_num:[%d]}",
                                 pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);

                pst_grp_list->uc_sta_num--;
                pst_m2u->st_m2u_snooplist.us_total_sta_num--;

                OAM_WARNING_LOG2(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_update_snoop_list::remove one sta. current total_sta_num:[%d] ,sta_num:[%d]}",
                                 pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);
            }
        }
        /* 特定组中不存在该节点，并且更新节点的CMD不为EXC则ADD */
        else
        {
            if (HMAC_M2U_CMD_EXCLUDE_LIST != pst_list_entry->en_cmd)
            {
                OAM_INFO_LOG1(pst_list_entry->pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_update_snoop_list::pst_grp_member_list == NULL && pst_list_entry->en_cmd is [%d].}", pst_list_entry->en_cmd);
                ul_ret = hmac_m2u_add_member_list(pst_grp_list, pst_list_entry);
            }
        }
    }
    return ul_ret;
}


oal_void hmac_m2u_show_snoop_deny_table(hmac_vap_stru *pst_hmac_vap)
{
    hmac_m2u_stru             *pst_m2u;
    oal_uint8                  uc_idx;
    hmac_m2u_snoop_list_stru  *pst_snp_list;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_add_snoop_ipv6_deny_entry::pst_hmac_vap is null}");
        return;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_add_snoop_ipv6_deny_entry::pst_m2u is null}");
        return;
    }

    pst_snp_list = &(pst_m2u->st_m2u_snooplist);

    if ((0 == pst_snp_list->uc_deny_count_ipv4) && (0 == pst_snp_list->uc_deny_count_ipv6))
    {
        return;
    }
    for (uc_idx = 0 ; uc_idx < pst_snp_list->uc_deny_count_ipv4; uc_idx++)
    {
         /* 打印黑名单的IPv4地址 */
        OAM_WARNING_LOG4(0, OAM_SF_M2U, "{hmac_m2u_show_snoop_deny_table:: deny addr_ipv4 = %x.%x.%x.%x}\r\n",
                                                (oal_uint32)((pst_snp_list->aul_deny_group[uc_idx] >> 24) & 0xff),
                                                (oal_uint32)((pst_snp_list->aul_deny_group[uc_idx] >> 16) & 0xff),
                                                (oal_uint32)((pst_snp_list->aul_deny_group[uc_idx] >> 8) & 0xff),
                                                (oal_uint32)((pst_snp_list->aul_deny_group[uc_idx] & 0xff)));
    }
    for (uc_idx = 0 ; uc_idx < pst_snp_list->uc_deny_count_ipv6; uc_idx++)
    {
        /* 打印黑名单的IPv6地址 */
        OAM_WARNING_LOG4(0, OAM_SF_M2U, "{hmac_m2u_show_snoop_deny_table::  deny addr_ipv6 = [%08x]:[%08x]:[%08x]:[%08x]}\r\n",
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_snp_list->aul_deny_group_ipv6[uc_idx][0])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_snp_list->aul_deny_group_ipv6[uc_idx][4])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_snp_list->aul_deny_group_ipv6[uc_idx][8])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_snp_list->aul_deny_group_ipv6[uc_idx][12])));
    }
}


OAL_STATIC oal_uint32 hmac_m2u_snoop_is_denied_ipv4(hmac_vap_stru *pst_hmac_vap, oal_uint32 ul_grpaddr)
{
    oal_uint8                 uc_idx;
    hmac_m2u_snoop_list_stru *pst_snp_list;
    hmac_m2u_stru            *pst_m2u;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_snoop_is_denied_ipv4::pst_hmac_vap is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_PTR_NULL == pst_m2u)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_snoop_is_denied_ipv4::pst_m2u is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_snp_list = &(pst_m2u->st_m2u_snooplist);

    if (0 == pst_snp_list->uc_deny_count_ipv4)
    {
        return OAL_FALSE;
    }
    for (uc_idx = 0; uc_idx < pst_snp_list->uc_deny_count_ipv4; uc_idx++)
    {
        if (ul_grpaddr != pst_snp_list->aul_deny_group[uc_idx])
        {
             continue;
        }
        return OAL_TRUE;
    }
    return OAL_FALSE;
}

/*****************************************************************************
 函 数 名  : hmac_m2u_snoop_is_denied_ipv6
 功能描述  : 所加入组播组的IPv6地址是否在黑名单内
 输入参数  : pst_hmac_vap vap结构体; ul_grpaddr 组播组的IPv6地址
 输出参数  : OAL_FALSE OR OAL_TRUE
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :


*****************************************************************************/
OAL_STATIC oal_uint32 hmac_m2u_snoop_is_denied_ipv6(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_grpaddr)
{
    oal_uint8                 uc_idx;
    hmac_m2u_snoop_list_stru *pst_snp_list;
    hmac_m2u_stru            *pst_m2u;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_snoop_is_denied_ipv6::pst_hmac_vap is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_PTR_NULL == pst_m2u)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_snoop_is_denied_ipv6::pst_m2u is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_snp_list = &(pst_m2u->st_m2u_snooplist);
    if (0 == pst_snp_list->uc_deny_count_ipv6)
    {
        return OAL_FALSE;
    }
    for (uc_idx = 0; uc_idx < pst_snp_list->uc_deny_count_ipv6; uc_idx++)
    {
        if (!oal_memcmp(puc_grpaddr, pst_snp_list->aul_deny_group_ipv6[uc_idx], OAL_IPV6_ADDR_SIZE))
        {
             return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}

oal_void hmac_m2u_clear_deny_table(hmac_vap_stru *pst_hmac_vap)
{
    hmac_m2u_snoop_list_stru *pst_snp_list;
    hmac_m2u_stru            *pst_m2u;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_clear_deny_table::pst_hmac_vap is null}");
        return;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_clear_deny_table::pst_m2u is null}");
        return;
    }

    pst_snp_list = &(pst_m2u->st_m2u_snooplist);
    pst_snp_list->uc_deny_count_ipv4 = DEFAULT_IPV4_DENY_GROUP_COUNT;
    pst_snp_list->uc_deny_count_ipv6 = DEFAULT_IPV6_DENY_GROUP_COUNT;
    return;
}


oal_void hmac_m2u_add_snoop_ipv4_deny_entry(hmac_vap_stru *pst_hmac_vap, oal_uint32 *pul_grpaddr)
{
    oal_uint8                 uc_idx;
    hmac_m2u_snoop_list_stru *pst_snp_list;
    hmac_m2u_stru            *pst_m2u;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_add_snoop_ipv4_deny_entry::pst_hmac_vap is null}");
        return;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_add_snoop_ipv4_deny_entry::pst_m2u is null}");
        return;
    }

    pst_snp_list = &(pst_m2u->st_m2u_snooplist);
    uc_idx = pst_snp_list->uc_deny_count_ipv4;

    if (uc_idx + pst_snp_list->uc_deny_count_ipv6 > HMAC_M2U_GRPADDR_FILTEROUT_NUM) /* ipv4和ipv6目前的黑名单个数相加超过最大黑名单个数 */
    {
        return;
    }
    pst_snp_list->uc_deny_count_ipv4++;
    pst_snp_list->aul_deny_group[uc_idx] = *pul_grpaddr;
    return;
}


oal_void hmac_m2u_add_snoop_ipv6_deny_entry(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_grpaddr)
{
    oal_uint8                 uc_idx;
    hmac_m2u_snoop_list_stru *pst_snp_list;
    hmac_m2u_stru            *pst_m2u;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_add_snoop_ipv6_deny_entry::pst_hmac_vap is null}");
        return;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_add_snoop_ipv6_deny_entry::pst_m2u is null}");
        return;
    }
    pst_snp_list = &(pst_m2u->st_m2u_snooplist);
    uc_idx = pst_snp_list->uc_deny_count_ipv6;
    if (uc_idx + pst_snp_list->uc_deny_count_ipv4 > HMAC_M2U_GRPADDR_FILTEROUT_NUM)/* ipv4和ipv6目前的黑名单个数相加超过最大黑名单个数 */
    {
        return;
    }
    pst_snp_list->uc_deny_count_ipv6++;
    oal_memcopy(&pst_snp_list->aul_deny_group_ipv6[uc_idx],puc_grpaddr,OAL_IPV6_ADDR_SIZE);

    return;
}


oal_void hmac_m2u_del_ipv4_deny_entry(hmac_vap_stru *pst_hmac_vap, oal_uint32 *pul_grpaddr)
{
    oal_uint8                 uc_idx;
    oal_uint8                 uc_num;
    hmac_m2u_snoop_list_stru *pst_snp_list;
    hmac_m2u_stru            *pst_m2u;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_del_ipv4_deny_entry::pst_hmac_vap is null}");
        return;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_del_ipv4_deny_entry::pst_m2u is null}");
        return;
    }

    pst_snp_list = &(pst_m2u->st_m2u_snooplist);
    uc_num       = pst_snp_list->uc_deny_count_ipv4;

    if (uc_num <= DEFAULT_IPV4_DENY_GROUP_COUNT)
    {
        return;
    }
    for (uc_idx = DEFAULT_IPV4_DENY_GROUP_COUNT; uc_idx < uc_num; uc_idx++)
    {
        if (*pul_grpaddr == pst_snp_list->aul_deny_group[uc_idx])
        {
            break;
        }
    }

    if (uc_idx < uc_num)
    {
        pst_snp_list->aul_deny_group[uc_idx] = pst_snp_list->aul_deny_group[uc_num - 1];
        pst_snp_list->uc_deny_count_ipv4--;
    }
    return;
}


oal_void hmac_m2u_del_ipv6_deny_entry(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_grpaddr)
{
    oal_uint8                 uc_idx;
    oal_uint8                 uc_num;
    hmac_m2u_snoop_list_stru *pst_snp_list;
    hmac_m2u_stru            *pst_m2u;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_del_ipv6_deny_entry::pst_hmac_vap is null}");
        return;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_del_ipv6_deny_entry::pst_m2u is null}");
        return;
    }

    pst_snp_list = &(pst_m2u->st_m2u_snooplist);
    uc_num       = pst_snp_list->uc_deny_count_ipv6;

    if (uc_num <= DEFAULT_IPV6_DENY_GROUP_COUNT)
    {
        return;
    }
    for (uc_idx = DEFAULT_IPV6_DENY_GROUP_COUNT; uc_idx < uc_num; uc_idx++)
    {
        if (!oal_memcmp(puc_grpaddr, pst_snp_list->aul_deny_group_ipv6[uc_idx], OAL_IPV6_ADDR_SIZE))
        {
            break;
        }
    }

    if (uc_idx < uc_num)
    {
        oal_memcopy(&pst_snp_list->aul_deny_group_ipv6[uc_idx],&pst_snp_list->aul_deny_group_ipv6[uc_num - 1],OAL_IPV6_ADDR_SIZE);
        pst_snp_list->uc_deny_count_ipv6--;
    }
    return;

}


/*lint -e550*/
oal_uint32 hmac_m2u_print_all_snoop_list(hmac_vap_stru *pst_hmac_vap, oal_snoop_all_group_stru *pst_snoop_all_grp)
{
    hmac_m2u_snoop_list_stru        *pst_snp_list;
    hmac_m2u_grp_list_entry_stru    *pst_grp_list_member;
    hmac_m2u_grp_member_stru        *pst_grp_member = OAL_PTR_NULL;
    oal_dlist_head_stru             *pst_grp_member_entry;
    oal_dlist_head_stru             *pst_grp_member_entry_temp;
    oal_dlist_head_stru             *pst_grp_list_entry;
    oal_int8                        ac_tmp_buff[100];
    hmac_m2u_stru                   *pst_m2u;
    oal_uint8                       uc_sta_cnt;
    oal_uint16                      us_group_cnt = 0;
    oal_snoop_group_stru            st_snoop_group;


    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_print_all_snoop_list::pst_hmac_vap is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_print_all_snoop_list::pst_m2u is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_snp_list = &(pst_m2u->st_m2u_snooplist);
    if(OAL_PTR_NULL == pst_snp_list)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "hmac_m2u_print_all_snoop_list::pst_snp_list is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memset(ac_tmp_buff, 0, OAL_SIZEOF(ac_tmp_buff));
    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "total_sta_num: [%d] group_num:[%d]\n",
                     pst_snp_list->us_total_sta_num,pst_snp_list->us_group_list_count);

    if ((WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode) &&
         (pst_m2u->en_snoop_enable))
    {
        if(OAL_PTR_NULL != pst_snoop_all_grp)
        {
            pst_snoop_all_grp->us_group_cnt = pst_snp_list->us_group_list_count;
            us_group_cnt = 0;
        }
        OAL_DLIST_SEARCH_FOR_EACH(pst_grp_list_entry, &(pst_snp_list->st_grp_list))
        {
            pst_grp_list_member = (hmac_m2u_grp_list_entry_stru *)OAL_DLIST_GET_ENTRY(pst_grp_list_entry,
                                                                               hmac_m2u_grp_list_entry_stru,
                                                                               st_grp_entry);
            /* 打印组播组的mac地址 */
            oal_memset(ac_tmp_buff, 0, OAL_SIZEOF(ac_tmp_buff));
            OAL_SPRINTF(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), " group addr = [%02x]:[%02x]::xx::[%02x]:[%02x] vlan tag = 0x%x \n",
                                                        pst_grp_list_member->auc_group_mac[0],
                                                        pst_grp_list_member->auc_group_mac[1],
                                                        pst_grp_list_member->auc_group_mac[4],
                                                        pst_grp_list_member->auc_group_mac[5],
                                                        *(oal_uint32*)(&(pst_grp_list_member->st_outer_vlan_tag)));
            OAL_MEMZERO(&st_snoop_group, sizeof(oal_snoop_group_stru));
            if(OAL_PTR_NULL != pst_snoop_all_grp)
            {
                oal_memcopy(st_snoop_group.auc_group_mac, pst_grp_list_member->auc_group_mac, WLAN_MAC_ADDR_LEN);
            }
            uc_sta_cnt = 0;
            oam_print_etc(ac_tmp_buff);
            /* 打印组播组成员的mac地址以及组播源地址 */
            OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_grp_member_entry, pst_grp_member_entry_temp, &(pst_grp_list_member->st_src_list))
            {
                pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_DLIST_GET_ENTRY(pst_grp_member_entry,
                                                                         hmac_m2u_grp_member_stru,
                                                                         st_member_entry);
                /* 打印组播组内成员的src ip地址和 mac地址 */
                oal_memset(ac_tmp_buff, 0, OAL_SIZEOF(ac_tmp_buff));
                if (OAL_IPV6_ADDR_SIZE == pst_grp_member->uc_src_ip_addr_len)
                {
                     OAL_SPRINTF(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), "group_src_ip_addr = [%x]:[%x]:[%x]:[%x]:[%x]:[%x]:[%x]:[%x]\n",
                                                       OAL_NET2HOST_SHORT(*(oal_uint16*)(&pst_grp_member->auc_src_ip_addr[0])),
                                                       OAL_NET2HOST_SHORT(*(oal_uint16*)(&pst_grp_member->auc_src_ip_addr[2])),
                                                       OAL_NET2HOST_SHORT(*(oal_uint16*)(&pst_grp_member->auc_src_ip_addr[4])),
                                                       OAL_NET2HOST_SHORT(*(oal_uint16*)(&pst_grp_member->auc_src_ip_addr[6])),
                                                       OAL_NET2HOST_SHORT(*(oal_uint16*)(&pst_grp_member->auc_src_ip_addr[8])),
                                                       OAL_NET2HOST_SHORT(*(oal_uint16*)(&pst_grp_member->auc_src_ip_addr[10])),
                                                       OAL_NET2HOST_SHORT(*(oal_uint16*)(&pst_grp_member->auc_src_ip_addr[12])),
                                                       OAL_NET2HOST_SHORT(*(oal_uint16*)(&pst_grp_member->auc_src_ip_addr[14])));
                }
                else
                {
                     OAL_SPRINTF(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), "group_src_ip_addr = [%d].[%d].[%d].[%d]\n",
                                                       pst_grp_member->auc_src_ip_addr[0],
                                                       pst_grp_member->auc_src_ip_addr[1],
                                                       pst_grp_member->auc_src_ip_addr[2],
                                                       pst_grp_member->auc_src_ip_addr[3]);
                }
                oam_print_etc(ac_tmp_buff);

                oal_memset(ac_tmp_buff, 0, OAL_SIZEOF(ac_tmp_buff));
                OAL_SPRINTF(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), "sta_mac and mode = [%02x]:[%02x]::xx::[%02x]:[%02x]:%d\n",
                                                        pst_grp_member->auc_grp_member_mac[0],
                                                        pst_grp_member->auc_grp_member_mac[1],
                                                        pst_grp_member->auc_grp_member_mac[4],
                                                        pst_grp_member->auc_grp_member_mac[5],
                                                        pst_grp_member->en_mode);
                if(OAL_PTR_NULL != pst_snoop_all_grp)
                {
                    if(uc_sta_cnt < MAX_STA_NUM_OF_ONE_GROUP)
                    {
                        st_snoop_group.uc_sta_num = uc_sta_cnt + 1;
                        oal_memcopy(st_snoop_group.auc_sta_mac[uc_sta_cnt], pst_grp_member->auc_grp_member_mac, WLAN_MAC_ADDR_LEN);
                        uc_sta_cnt++;
                    }
                    #if 1
                    OAL_IO_PRINT("sta_num=%d %02X:%02X::XX::%02X:%02X\n",
                        st_snoop_group.uc_sta_num,
                        st_snoop_group.auc_sta_mac[uc_sta_cnt-1][0],
                        st_snoop_group.auc_sta_mac[uc_sta_cnt-1][1],
                        st_snoop_group.auc_sta_mac[uc_sta_cnt-1][4],
                        st_snoop_group.auc_sta_mac[uc_sta_cnt-1][5]);
                    #endif
                }
                oam_print_etc(ac_tmp_buff);
            }
            if(OAL_PTR_NULL != pst_snoop_all_grp)
            {
                if(OAL_PTR_NULL != pst_snoop_all_grp->pst_buf)
                {
                    OAL_IO_PRINT("pst_buf is not null\n");
                    oal_copy_to_user((pst_snoop_all_grp->pst_buf + us_group_cnt), &st_snoop_group, OAL_SIZEOF(oal_snoop_group_stru));
                }
                #if 1
                OAL_IO_PRINT("grp_mac=%02X:%02X::XX::%02X:%02X,sta_num=%d,sta_mac=%02X:%02X::XX::%02X:%02X\n",
                st_snoop_group.auc_group_mac[0],
                st_snoop_group.auc_group_mac[1],
                st_snoop_group.auc_group_mac[4],
                st_snoop_group.auc_group_mac[5],
                st_snoop_group.uc_sta_num,
                st_snoop_group.auc_sta_mac[0][0],
                st_snoop_group.auc_sta_mac[0][1],
                st_snoop_group.auc_sta_mac[0][4],
                st_snoop_group.auc_sta_mac[0][5]);
                #endif
                us_group_cnt ++;
                OAL_IO_PRINT("us_group_cnt=%d\n",us_group_cnt);
            }
        }
    }
    else
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2U, "{hmac_m2u_print_all_snoop_list::en_snoop_enable en_vap_mode = [%d].[%d]}\r\n",
                                                    pst_m2u->en_snoop_enable,
                                                    pst_hmac_vap->st_vap_base_info.en_vap_mode);
    }
    return OAL_SUCC;
}

/*lint +e550*/

oal_void hmac_m2u_get_group_mac(oal_uint8 *puc_group_mac, oal_uint8 *puc_group_ip, oal_uint8 uc_ip_len)
{
    if (OAL_IPV6_ADDR_SIZE == uc_ip_len)
    {
        //ipv6下，组播mac最后4字节由IP地址映射而来
        puc_group_mac[0] = 0x33;
        puc_group_mac[1] = 0x33;
        puc_group_mac   += 2;
        puc_group_ip    += 12; //取最后4个字节
        oal_memcopy(puc_group_mac,puc_group_ip,4);
    }
    else
    {
        //ipv4下，组播mac最后23bit由IP地址映射而来
        puc_group_mac[0] = 0x01;
        puc_group_mac[1] = 0x0;
        puc_group_mac[2] = 0x5e;
        puc_group_ip    += 1;
        oal_memcopy(puc_group_mac + 3,puc_group_ip,3);
        puc_group_mac[3] &= 0x7f;
    }
}



oal_uint32 hmac_m2u_igmp_v1v2_update(hmac_vap_stru *pst_hmac_vap, hmac_m2u_list_update_stru *pst_list_entry, mac_igmp_header_stru *pst_igmp)
{
    oal_uint32           ul_group_addr ;                    /* to hold group address from group record */
    oal_uint32           ul_ret = OAL_SUCC;

    ul_group_addr = pst_igmp->ul_group;
    if (hmac_m2u_snoop_is_denied_ipv4(pst_hmac_vap, OAL_NET2HOST_LONG(ul_group_addr)))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v1v2_update::ul_group_addr [%x].[%x].[%x].[%x] is denied}\r\n}",
                                                                           (oal_uint32)((ul_group_addr) & 0xff),
                                                                           (oal_uint32)((ul_group_addr>> 8)  & 0xff),
                                                                           (oal_uint32)((ul_group_addr>> 16) & 0xff),
                                                                           (oal_uint32)((ul_group_addr>> 24) & 0xff));
        return ul_ret;
    }

    if (MAC_IGMPV2_LEAVE_TYPE  != pst_igmp->uc_type)
    {
        pst_list_entry->en_cmd = HMAC_M2U_CMD_INCLUDE_LIST;
    }
    else
    {
        pst_list_entry->en_cmd = HMAC_M2U_CMD_EXCLUDE_LIST;
    }

    pst_list_entry->uc_src_ip_addr_len = OAL_IPV4_ADDR_SIZE;
    hmac_m2u_get_group_mac(pst_list_entry->auc_grp_mac,(oal_uint8*)(&ul_group_addr),OAL_IPV4_ADDR_SIZE);
    oal_memset(pst_list_entry->auc_src_ip_addr , 0, OAL_IPV6_ADDR_SIZE);
    ul_ret = hmac_m2u_update_snoop_list(pst_list_entry);
    return ul_ret;
}


oal_uint32 hmac_m2u_igmp_v3_update(hmac_vap_stru *pst_hmac_vap, hmac_m2u_list_update_stru *pst_list_entry, mac_igmp_v3_report_stru *pst_igmpr3)
{
    oal_uint32                           ul_group_ip;                       /* to hold group ip address from group record */
    oal_uint16                           us_no_grec;                        /* no of group records  */
    oal_uint16                           us_no_srec;                        /* no of source records */
    oal_uint32                          *pul_src_addr;                      /* 组播源的IP地址 */
    mac_igmp_v3_grec_stru               *pst_grec;                          /* igmp group record */
    hmac_m2u_grp_list_entry_stru        *pst_grp_list;
    hmac_m2u_grp_member_stru            *pst_grp_member_list;
    oal_uint32                           ul_ret = OAL_SUCC;
    hmac_m2u_stru                       *pst_m2u;

    us_no_grec   = OAL_HOST2NET_SHORT(pst_igmpr3->us_ngrec);
    pst_grec     = (mac_igmp_v3_grec_stru *)(pst_igmpr3 + 1);
    pst_m2u      = (hmac_m2u_stru*)(pst_list_entry->pst_hmac_vap->pst_m2u);

    if (0 == us_no_grec)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update::us_no_grec is 0.}");
        return OAL_FAIL;
    }

    while (us_no_grec)
    {
        pst_list_entry->en_cmd = pst_grec->uc_grec_type;
        ul_group_ip            = pst_grec->ul_grec_group_ip;////获取组播组ip地址

        if (hmac_m2u_snoop_is_denied_ipv4(pst_hmac_vap, OAL_NET2HOST_LONG(ul_group_ip)))
        {
            OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update::ul_group_addr [%x].[%x].[%x].[%x] is denied}\r\n}",
                                                                                        (oal_uint32)((ul_group_ip>> 24) & 0xff),
                                                                                        (oal_uint32)((ul_group_ip>> 16) & 0xff),
                                                                                        (oal_uint32)((ul_group_ip>> 8)  & 0xff),
                                                                                        (oal_uint32)((ul_group_ip) & 0xff));
            /* move the grec to next group record */
            pst_grec = (mac_igmp_v3_grec_stru *)((oal_uint8 *)pst_grec + IGMPV3_GRP_REC_LEN(pst_grec));
            us_no_grec--;
            continue;
        }
        /* 非IGMPV3六种cmd的异常处理 */
        if (!IS_IGMPV3_MODE(pst_grec->uc_grec_type))
        {
            OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update::uc_grec_type is [%x] not inside the six cmd.}", pst_grec->uc_grec_type);
            /* move the grec to next group record */
            pst_grec = (mac_igmp_v3_grec_stru *)((oal_uint8 *)pst_grec + IGMPV3_GRP_REC_LEN(pst_grec));
            us_no_grec--;
            continue;
        }

        us_no_srec = OAL_HOST2NET_SHORT(pst_grec->us_grec_nsrcs);
        hmac_m2u_get_group_mac(pst_list_entry->auc_grp_mac , (oal_uint8*)(&ul_group_ip), OAL_IPV4_ADDR_SIZE);
        pst_list_entry->uc_src_ip_addr_len = OAL_IPV4_ADDR_SIZE;

        /* IGMP V3 exc的处理 */
        if ( IGMPV3_CHANGE_TO_EXCLUDE == (pst_grec->uc_grec_type)||
            IGMPV3_MODE_IS_EXCLUDE == (pst_grec->uc_grec_type))
        {
            OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update::pst_grec->uc_grec_type is %d.}", pst_grec->uc_grec_type);
            pst_list_entry->en_cmd = HMAC_M2U_CMD_EXCLUDE_LIST;
            pst_grp_list = hmac_m2u_find_group_list(pst_hmac_vap, pst_list_entry);

            /* 更新组播组内成员的状态，如果组播组内存在该目标成员，则清空目标成员 */
            if (OAL_PTR_NULL == pst_grp_list)
            {
                OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update::pst_grp_list is null.}");
            }
            else
            {
                hmac_m2u_remove_one_member(pst_grp_list, pst_hmac_vap, pst_list_entry->auc_new_member_mac);
            }

            /* 更新组播组内成员的状态，如果组播组内不存在该目标成员，该成员inc所有src ip */
            if (0 == us_no_srec)
            {
                OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update::not exist the user us_no_srec is %d.}", us_no_srec);
                oal_memset(pst_list_entry->auc_src_ip_addr , 0, OAL_IPV6_ADDR_SIZE);
                pst_list_entry->en_cmd = HMAC_M2U_CMD_INCLUDE_LIST;
                ul_ret = hmac_m2u_update_snoop_list(pst_list_entry);
            }
        }

        /* IGMP V3 INC的处理 */
        else if (IGMPV3_CHANGE_TO_INCLUDE == (pst_grec->uc_grec_type) ||
                  IGMPV3_MODE_IS_INCLUDE == (pst_grec->uc_grec_type))
        {
            pst_list_entry->en_cmd = HMAC_M2U_CMD_INCLUDE_LIST;
            pst_grp_list = hmac_m2u_find_group_list(pst_hmac_vap, pst_list_entry);

            if (OAL_PTR_NULL == pst_grp_list)
            {
                OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update::pst_grp_list is null.}");
            }
            else
            {
                hmac_m2u_remove_one_member(pst_grp_list, pst_hmac_vap, pst_list_entry->auc_new_member_mac);
            }
        }

        else if (IGMPV3_ALLOW_NEW_SOURCES == (pst_grec->uc_grec_type))
        {
            pst_list_entry->en_cmd = HMAC_M2U_CMD_INCLUDE_LIST;
        }

        pul_src_addr = (oal_uint32 *)((oal_uint8 *)pst_grec + OAL_SIZEOF(mac_igmp_v3_grec_stru));

        /* 同一组播组内不同src ip的链表更新 */
        while (us_no_srec)
        {
            *(oal_uint32*)(pst_list_entry->auc_src_ip_addr) = *pul_src_addr;
            if (IGMPV3_BLOCK_OLD_SOURCES != (pst_grec->uc_grec_type))
            {
                ul_ret = hmac_m2u_update_snoop_list(pst_list_entry);
            }
            /* block old source时清空该成员 */
            else
            {
                pst_grp_list = hmac_m2u_find_group_list(pst_hmac_vap, pst_list_entry);

                if (OAL_PTR_NULL != pst_grp_list)
                {
                    pst_grp_member_list = hmac_m2u_find_member_src(pst_grp_list,
                                                                   pst_list_entry->auc_new_member_mac, pst_list_entry->auc_src_ip_addr);
                    if (OAL_PTR_NULL != pst_grp_member_list)
                    {
                        oal_dlist_delete_entry(&pst_grp_member_list->st_member_entry);
                        OAL_MEM_FREE(pst_grp_member_list, OAL_TRUE);

                        OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update:: current total_sta_num:[%d] ,sta_num:[%d]}",
                                         pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);

                        pst_grp_list->uc_sta_num--;
                        pst_m2u->st_m2u_snooplist.us_total_sta_num--;

                        OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_igmp_v3_update::remove one sta. current total_sta_num:[%d] ,sta_num:[%d]}",
                                         pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);
                    }
                }
            }
            pul_src_addr++;
            us_no_srec--;
        }
        /* 取下一个组播组 */
        pst_grec = (mac_igmp_v3_grec_stru *)((oal_uint8 *)pst_grec + IGMPV3_GRP_REC_LEN(pst_grec));
        us_no_grec--;
    }
    return ul_ret;
}



oal_uint32 hmac_m2u_mld_v1_update(hmac_vap_stru *pst_hmac_vap, hmac_m2u_list_update_stru *pst_list_entry, mac_mld_v1_head_stru *pst_mld_head)
{
    oal_uint32           ul_ret = OAL_SUCC;
    if (hmac_m2u_snoop_is_denied_ipv6(pst_hmac_vap, pst_mld_head->auc_group_ip))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v1_update::auc_group_ip [%08x]:[%08x]:[%08x]:[%08x] is denied}\r\n}",
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_mld_head->auc_group_ip[0])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_mld_head->auc_group_ip[4])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_mld_head->auc_group_ip[8])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_mld_head->auc_group_ip[12])));
        return ul_ret;
    }

    /* 初始化组播组地址 */
    hmac_m2u_get_group_mac(pst_list_entry->auc_grp_mac,pst_mld_head->auc_group_ip,OAL_IPV6_ADDR_SIZE);

    if (MLDV1_DONE_TYPE != pst_mld_head->uc_type)
    {
        pst_list_entry->en_cmd = HMAC_M2U_CMD_INCLUDE_LIST;
    }
    else
    {
        pst_list_entry->en_cmd = HMAC_M2U_CMD_EXCLUDE_LIST;
    }
    pst_list_entry->uc_src_ip_addr_len = OAL_IPV6_ADDR_SIZE;
    oal_memset(pst_list_entry->auc_src_ip_addr, 0, OAL_IPV6_ADDR_SIZE);
    ul_ret = hmac_m2u_update_snoop_list(pst_list_entry);
    return ul_ret;
}


oal_uint32 hmac_m2u_mld_v2_update(hmac_vap_stru *pst_hmac_vap, hmac_m2u_list_update_stru *pst_list_entry, mac_mld_v2_report_stru *pst_mldv2)
{
    //oal_uint8                           *puc_group_ip;                      /* to hold group ip address from group record */
    oal_uint16                           us_no_grec;                        /* no of group records  */
    oal_uint16                           us_no_srec;                        /* no of source records */
    oal_uint8                           *puc_src_addr;                      /* 组播源的IP地址 */
    mac_mld_v2_group_record_stru        *pst_grec;                          /* igmp group record */
    hmac_m2u_grp_list_entry_stru        *pst_grp_list;
    hmac_m2u_grp_member_stru            *pst_grp_member_list;
    hmac_m2u_stru                       *pst_m2u;
    oal_uint32                           ul_ret = OAL_SUCC;

    us_no_grec = OAL_HOST2NET_SHORT(pst_mldv2->us_group_address_num);
    pst_grec = (mac_mld_v2_group_record_stru *)(pst_mldv2 + 1);
    pst_m2u = (hmac_m2u_stru*)(pst_list_entry->pst_hmac_vap->pst_m2u);

    if (0 == us_no_grec)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v2_update::us_no_grec is 0.}");
        return OAL_FAIL;
    }

    while (us_no_grec)
    {
        pst_list_entry->en_cmd = pst_grec->uc_grec_type;
        //puc_group_ip = pst_grec->auc_group_ip;////获取组播组ip地址
        if (hmac_m2u_snoop_is_denied_ipv6(pst_hmac_vap, pst_grec->auc_group_ip))
        {
            OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v2_update::auc_group_ip [%08x]:[%08x]:[%08x]:[%08x] is denied}\r\n}",
                                        OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_grec->auc_group_ip[0])),
                                        OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_grec->auc_group_ip[4])),
                                        OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_grec->auc_group_ip[8])),
                                        OAL_HOST2NET_LONG(*(oal_uint32*)(&pst_grec->auc_group_ip[12])));
            pst_grec = (mac_mld_v2_group_record_stru *)((oal_uint8 *)pst_grec + MLDV2_GRP_REC_LEN(pst_grec));
            us_no_grec--;
            continue;
        }

        /* 非MLDV2六种cmd的异常处理 */
        if (!IS_MLDV2_MODE(pst_grec->uc_grec_type))
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v2_update::uc_grec_type is [%x] not inside the six cmd.}", pst_grec->uc_grec_type);
            /* move the grec to next group record */
            pst_grec = (mac_mld_v2_group_record_stru *)((oal_uint8 *)pst_grec + MLDV2_GRP_REC_LEN(pst_grec));
            us_no_grec--;
            continue;
        }

        us_no_srec = OAL_HOST2NET_SHORT(pst_grec->us_grec_srcaddr_num);

        //oal_set_mac_addr(pst_list_entry->auc_grp_mac, uc_group_addr);
        hmac_m2u_get_group_mac(pst_list_entry->auc_grp_mac , pst_grec->auc_group_ip ,OAL_IPV6_ADDR_SIZE);
        pst_list_entry->uc_src_ip_addr_len = OAL_IPV6_ADDR_SIZE;

        /* MLD V2 exc的处理 */
        if ( MLDV2_CHANGE_TO_EXCLUDE == (pst_grec->uc_grec_type)||
            MLDV2_MODE_IS_EXCLUDE == (pst_grec->uc_grec_type))
        {
            OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v2_update::pst_grec->uc_grec_type is %d.}", pst_grec->uc_grec_type);
            pst_list_entry->en_cmd = HMAC_M2U_CMD_EXCLUDE_LIST;
            pst_grp_list = hmac_m2u_find_group_list(pst_hmac_vap, pst_list_entry);

            /* 更新组播组内成员的状态，如果组播组内存在该目标成员，则清空目标成员 */
            if (OAL_PTR_NULL == pst_grp_list)
            {
                OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v2_update::pst_grp_list is null.}");
            }
            else
            {
                hmac_m2u_remove_one_member(pst_grp_list, pst_hmac_vap, pst_list_entry->auc_new_member_mac);
            }

            /* 更新组播组内成员的状态，如果组播组内不存在该目标成员，该成员inc所有src ip */
            if (0 == us_no_srec)
            {
                OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v2_update::not exist the user us_no_srec is %d.}", us_no_srec);
                oal_memset(pst_list_entry->auc_src_ip_addr , 0, OAL_IPV6_ADDR_SIZE);
                pst_list_entry->en_cmd = HMAC_M2U_CMD_INCLUDE_LIST;
                ul_ret = hmac_m2u_update_snoop_list(pst_list_entry);
            }
        }

        /* MLD V2 INC的处理 */
        else if (MLDV2_CHANGE_TO_INCLUDE == (pst_grec->uc_grec_type) ||
                  MLDV2_MODE_IS_INCLUDE == (pst_grec->uc_grec_type))
        {
            pst_list_entry->en_cmd = HMAC_M2U_CMD_INCLUDE_LIST;
            pst_grp_list = hmac_m2u_find_group_list(pst_hmac_vap, pst_list_entry);

            if (OAL_PTR_NULL != pst_grp_list)
            {
                hmac_m2u_remove_one_member(pst_grp_list, pst_hmac_vap, pst_list_entry->auc_new_member_mac);
            }
        }

        else if (MLDV2_ALLOW_NEW_SOURCES == (pst_grec->uc_grec_type))
        {
            pst_list_entry->en_cmd = HMAC_M2U_CMD_INCLUDE_LIST;
        }

        puc_src_addr = (oal_uint8 *)pst_grec + OAL_SIZEOF(mac_mld_v2_group_record_stru);

        /* 同一组播组内不同src ip的链表更新 */
        while (us_no_srec)
        {
            oal_memcopy(pst_list_entry->auc_src_ip_addr , puc_src_addr, OAL_IPV6_ADDR_SIZE);
            if (MLDV2_BLOCK_OLD_SOURCES != (pst_grec->uc_grec_type))
            {
                ul_ret = hmac_m2u_update_snoop_list(pst_list_entry);
            }
            /* block old source时清空该成员 */
            else
            {
                pst_grp_list = hmac_m2u_find_group_list(pst_hmac_vap, pst_list_entry);

                if (OAL_PTR_NULL != pst_grp_list)
                {
                    pst_grp_member_list = hmac_m2u_find_member_src(pst_grp_list,pst_list_entry->auc_new_member_mac,
                                                                    pst_list_entry->auc_src_ip_addr);
                    if (OAL_PTR_NULL != pst_grp_member_list)
                    {
                        oal_dlist_delete_entry(&pst_grp_member_list->st_member_entry);
                        OAL_MEM_FREE(pst_grp_member_list, OAL_TRUE);

                        OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v2_update::current total_sta_num:[%d] ,sta_num:[%d]}",
                                         pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);
                        pst_grp_list->uc_sta_num--;
                        pst_m2u->st_m2u_snooplist.us_total_sta_num--;

                        OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_mld_v2_update::remove one sta.current total_sta_num:[%d] ,sta_num:[%d]}",
                                         pst_m2u->st_m2u_snooplist.us_total_sta_num,pst_grp_list->uc_sta_num);
                    }
                }
            }
            puc_src_addr += OAL_IPV6_ADDR_SIZE;
            us_no_srec--;
        }
        /* 取下一个组播组 */
        pst_grec = (mac_mld_v2_group_record_stru *)((oal_uint8 *)pst_grec + MLDV2_GRP_REC_LEN(pst_grec));
        us_no_grec--;
    }
    return ul_ret;
}



oal_void hmac_m2u_snoop_inspecting_ipv4(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, hmac_m2u_list_update_stru *pst_list_entry,oal_uint8 *puc_buf)
{

    mac_ip_header_stru              *pst_ip_hdr;
    mac_igmp_header_stru            *pst_igmp;                             /* igmp header for v1 and v2 */
    mac_igmp_v3_report_stru         *pst_igmpr3;                           /* igmp header for v3 */
    oal_uint8                        uc_ip_hdr_len;

    /* 取ip头 */
    pst_ip_hdr = (mac_ip_header_stru *)puc_buf;

    if (IPPROTO_IGMP != pst_ip_hdr->uc_protocol)
    {
        return;
    }

    pst_list_entry->pst_hmac_vap  = pst_hmac_vap;
    pst_list_entry->pst_hmac_user = pst_hmac_user;

    /* bit序不同，取不同的ip头长度*/
    if (OAL_BITFIELD_BIG_ENDIAN ==  oal_netbuf_get_bitfield())
    {
        uc_ip_hdr_len = pst_ip_hdr->uc_version_ihl & 0x0F;
    }
    else
    {
        uc_ip_hdr_len = (pst_ip_hdr->uc_version_ihl & 0xF0) >> 4;
    }
    if (MIN_IP_HDR_LEN > uc_ip_hdr_len)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting_ipv4::ip_hdr_len is [%x].}", uc_ip_hdr_len);
        return;
    }
    /* v1 & v2 igmp */
    pst_igmp = (mac_igmp_header_stru *)(puc_buf + (uc_ip_hdr_len << 2));

    /* v3 igmp */
    pst_igmpr3 = (mac_igmp_v3_report_stru *) pst_igmp;

    /* 如果报文不是IGMP report报文或leave报文,不进行链表更新 */
    if (!IS_IGMP_REPORT_LEAVE_PACKET(pst_igmp->uc_type))
    {
       OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting_ipv4::not igmp report[%x].}", pst_igmp->uc_type);
       return;
    }

    /* IGMP v1 v2 报文的链表更新  */
    if (pst_igmp->uc_type != MAC_IGMPV3_REPORT_TYPE)
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting_ipv4::v1v2 update[%x].}", pst_igmp->uc_type);
        hmac_m2u_igmp_v1v2_update(pst_hmac_vap, pst_list_entry, pst_igmp);
    }

    /* IGMP v3 report 报文的链表更新*/
    else
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting_ipv4::v3 update[%x].}", pst_igmp->uc_type);
        hmac_m2u_igmp_v3_update(pst_hmac_vap, pst_list_entry, pst_igmpr3);
    }

}



oal_void hmac_m2u_snoop_inspecting_ipv6(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, hmac_m2u_list_update_stru *pst_list_entry,oal_uint8 *puc_buf)
{

    oal_ipv6hdr_stru            *pst_ip_hdr;
    mac_mld_v2_report_stru      *pst_mldv2;                           /* mld report header for v2 */
    mac_mld_v1_head_stru        *pst_mld_head;
    oal_icmp6hdr_stru           *pst_icmp_head;

     /* 取ip头 */
    pst_ip_hdr = (oal_ipv6hdr_stru *)(puc_buf);
    pst_icmp_head = (oal_icmp6hdr_stru*)(pst_ip_hdr + 1);//跳过IPV6 头获取ICMPV6头

    if ((OAL_IPPROTO_ICMPV6 != pst_icmp_head->icmp6_type) || (0x0 != pst_ip_hdr->nexthdr))
    {
        return;
    }

    pst_mld_head = (mac_mld_v1_head_stru*)(pst_icmp_head + 1);//跳过IPV6 头获取ICMPV6头
    if (!IS_MLD_REPORT_LEAVE_PACKET(pst_mld_head->uc_type))
    {
       return;
    }

    pst_list_entry->pst_hmac_vap  = pst_hmac_vap;
    pst_list_entry->pst_hmac_user = pst_hmac_user;
    pst_mldv2 = (mac_mld_v2_report_stru *) pst_mld_head;

    /* MLD v1  报文的链表更新  */
    if (pst_mld_head->uc_type != MLDV2_REPORT_TYPE)
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting_ipv6::v1 update[%x].}", pst_mld_head->uc_type);
        hmac_m2u_mld_v1_update(pst_hmac_vap, pst_list_entry, pst_mld_head);
    }

    /* MLD v2 report 报文的链表更新*/
    else
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting_ipv6::v2 update[%x].}", pst_mld_head->uc_type);
        hmac_m2u_mld_v2_update(pst_hmac_vap, pst_list_entry, pst_mldv2);
    }

}


oal_void hmac_m2u_snoop_inspecting(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_buf)
{
    mac_ether_header_stru                *pst_ether_header;
    oal_uint8                            *puc_src_addr;                          /* source address which send the report and it is the member */
    hmac_m2u_list_update_stru             st_list_entry;                        /* list entry where all member details will be updated and passed on updating the snoop list */
    hmac_m2u_stru                        *pst_m2u;
    mac_vlan_tag_stru                    *pst_vlan_tag;
    oal_uint16                            us_ether_data_type ;
    oal_uint8                            *puc_ip_head;


    oal_memset(&st_list_entry, 0, OAL_SIZEOF(hmac_m2u_list_update_stru));
    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    if (OAL_FALSE == pst_m2u->en_snoop_enable)
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting::snoop is [%d] not enable}", pst_m2u->en_snoop_enable);
        return;
    }

    /* 获取以太网头 */
    pst_ether_header  = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    puc_src_addr  = pst_ether_header->auc_ether_shost;
    oal_set_mac_addr(st_list_entry.auc_new_member_mac, puc_src_addr);

/*lint -e778*/
    /* 带vlan tag 的情况*/
    if (ETHER_IS_WITH_VLAN_TAG(pst_ether_header->us_ether_type))
    {
        pst_vlan_tag = (mac_vlan_tag_stru*)(oal_netbuf_data(pst_buf) + (ETHER_ADDR_LEN << 1));  //偏移2个mac地址的长度,获取外层vlan tag
        oal_memcopy(&(st_list_entry.st_outer_vlan_tag), pst_vlan_tag, OAL_SIZEOF(mac_vlan_tag_stru));

        pst_vlan_tag += 1;//判断内层tag
        if (ETHER_IS_WITH_VLAN_TAG(pst_vlan_tag->us_tpid))
        {
            pst_vlan_tag += 1;//不考虑内层tag，跳过
        }

        us_ether_data_type = *((oal_uint16*)(pst_vlan_tag));//跳过tag后获取eth  type
        puc_ip_head        = (oal_uint8*)(pst_vlan_tag ) + 2;//跳过 type 指向ip头起始地址
    }
     /* 非vlan  的情况*/
    else
    {
        puc_ip_head = (oal_uint8*)(pst_ether_header + 1);
        us_ether_data_type = pst_ether_header->us_ether_type;
    }

    /* ap模式的接收处理 */
    if ((WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode) &&
            (!ETHER_IS_BROADCAST(pst_ether_header->auc_ether_dhost)))
    {
        if ((OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == us_ether_data_type) &&
              (ETHER_IS_IPV4_MULTICAST(pst_ether_header->auc_ether_dhost)))
        {
            hmac_m2u_snoop_inspecting_ipv4(pst_hmac_vap, pst_hmac_user,&st_list_entry, puc_ip_head);
        }
        else if ((OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6) == us_ether_data_type) &&
                    ETHER_IS_IPV6_MULTICAST(pst_ether_header->auc_ether_dhost))
        {
            hmac_m2u_snoop_inspecting_ipv6(pst_hmac_vap, pst_hmac_user,&st_list_entry, puc_ip_head);
        }
        else
        {
            OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting::ether type is not IP Protocol.[0x%x]}", OAL_HOST2NET_SHORT(pst_ether_header->us_ether_type));
            return;
        }
    }
/*lint +e778*/
    /* STA模式的接收处理 去掉自定义的tunnel协议头 */
    if (OAL_HOST2NET_SHORT(ETHER_TUNNEL_TYPE) == pst_ether_header->us_ether_type &&
        WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_inspecting::STA Mode && Ether Type is ETHER_TUNNEL_TYPE.}");
        oal_netbuf_pull(pst_buf, OAL_SIZEOF(mac_ether_header_stru));
        oal_netbuf_pull(pst_buf, OAL_SIZEOF(mcast_tunnel_hdr_stru));
    }
    return;
}



OAL_STATIC oal_uint8 hmac_m2u_count_member_anysrclist(hmac_m2u_grp_list_entry_stru *pst_grp_list, oal_uint8 *puc_table,
                                                        oal_uint32 ul_timestamp)
{
    hmac_m2u_grp_member_stru *pst_grp_member;
    oal_dlist_head_stru      *pst_grp_member_entry;
    oal_uint8                 uc_count = 0;
    oal_bool_enum_uint8       en_ip_is_zero = OAL_FALSE;


    OAL_DLIST_SEARCH_FOR_EACH(pst_grp_member_entry, &(pst_grp_list->st_src_list))
    {
        pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_DLIST_GET_ENTRY(pst_grp_member_entry,
                                                                     hmac_m2u_grp_member_stru,
                                                                     st_member_entry);

        if (OAL_IPV6_ADDR_SIZE == pst_grp_member->uc_src_ip_addr_len)
        {
            en_ip_is_zero = (oal_uint8)OAL_IPV6_IS_UNSPECIFIED_ADDR(pst_grp_member->auc_src_ip_addr);
        }
        else
        {
            en_ip_is_zero = (oal_uint8)(0 == *((oal_int32*)(pst_grp_member->auc_src_ip_addr)));
        }
        if (en_ip_is_zero)
        {
            if (uc_count > MAX_STA_NUM_OF_ONE_GROUP)
            {
                break;
            }
            oal_set_mac_addr(&puc_table[uc_count * WLAN_MAC_ADDR_LEN], pst_grp_member->auc_grp_member_mac);
            pst_grp_member->ul_timestamp = ul_timestamp;
            uc_count++;
        }
    }
    return (uc_count);
}



OAL_STATIC oal_uint8 hmac_m2u_count_member_src_list(hmac_m2u_grp_list_entry_stru *pst_grp_list,
                                                    oal_uint8 *puc_src_ip_addr, oal_uint8 *puc_table,
                                                    oal_uint32 ul_timestamp, oal_uint8 uc_count)
{
    hmac_m2u_grp_member_stru *pst_grp_member;
    oal_dlist_head_stru      *pst_grp_member_entry;

    if (uc_count > MAX_STA_NUM_OF_ONE_GROUP)
    {
        return uc_count;
    }

    OAL_DLIST_SEARCH_FOR_EACH(pst_grp_member_entry, &(pst_grp_list->st_src_list))
    {
        pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_DLIST_GET_ENTRY(pst_grp_member_entry,
                                                                         hmac_m2u_grp_member_stru,
                                                                         st_member_entry);

        /* 组播源地址符合，模式是inc，加入到输出的table中 */
        if (!oal_memcmp(puc_src_ip_addr, pst_grp_member->auc_src_ip_addr, pst_grp_member->uc_src_ip_addr_len))
        {
            if (HMAC_M2U_CMD_INCLUDE_LIST == pst_grp_member->en_mode)
            {
                if (uc_count > MAX_STA_NUM_OF_ONE_GROUP)
                {
                    break;
                }
                oal_set_mac_addr(&puc_table[uc_count * WLAN_MAC_ADDR_LEN], pst_grp_member->auc_grp_member_mac);
                pst_grp_member->ul_timestamp = ul_timestamp;
                uc_count++;
            }
        }
        else
        {
            /* 组播源未匹配，但模式为exc的情况也加入到输出table中 */
            if (HMAC_M2U_CMD_EXCLUDE_LIST == pst_grp_member->en_mode)
            {
                if (uc_count > MAX_STA_NUM_OF_ONE_GROUP)
                {
                    break;
                }
                oal_set_mac_addr(&puc_table[uc_count * WLAN_MAC_ADDR_LEN], pst_grp_member->auc_grp_member_mac);
                pst_grp_member->ul_timestamp = ul_timestamp;
                uc_count++;
            }
        }
     }
    return (uc_count);
}



OAL_STATIC oal_uint8 hmac_m2u_get_snooplist_member(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_grp_addr,
                                oal_uint8 *puc_src_ip_addr, oal_uint8 *puc_table,mac_vlan_tag_stru *pst_vlan_tag)
{
    hmac_m2u_grp_list_entry_stru     *pst_grp_list_member;
    oal_uint8                         uc_count = 0;
    oal_uint32                        ul_nowtime;
    hmac_m2u_list_update_stru         st_list_entry;

    oal_memset(&st_list_entry, 0,  OAL_SIZEOF(hmac_m2u_list_update_stru));
    oal_memcopy(&(st_list_entry.auc_grp_mac), puc_grp_addr, WLAN_MAC_ADDR_LEN);
    if (OAL_PTR_NULL != pst_vlan_tag)
    {
        oal_memcopy(&(st_list_entry.st_outer_vlan_tag), pst_vlan_tag, OAL_SIZEOF(mac_vlan_tag_stru));
    }

    ul_nowtime = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    pst_grp_list_member = hmac_m2u_find_group_list(pst_hmac_vap, &st_list_entry);
    if (OAL_PTR_NULL != pst_grp_list_member)
    {
         uc_count  = hmac_m2u_count_member_anysrclist(pst_grp_list_member, &puc_table[0], ul_nowtime);
         uc_count  = hmac_m2u_count_member_src_list(pst_grp_list_member, puc_src_ip_addr, &puc_table[0], ul_nowtime, uc_count);
         OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_get_snooplist_member::uc_count is [%d]}", uc_count);
    }
    return (uc_count);
}


OAL_STATIC oal_void hmac_m2u_remove_node_grp(hmac_m2u_grp_list_entry_stru *pst_grp_list, hmac_user_stru *pst_hmac_user,oal_uint8  *puc_sta_num_removed)
{
    hmac_m2u_grp_member_stru *pst_grp_member;
    oal_dlist_head_stru      *pst_grp_member_entry;
    oal_dlist_head_stru      *pst_grp_member_entry_temp;
    oal_uint8                 uc_sta_count = 0;

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_grp_member_entry, pst_grp_member_entry_temp, &pst_grp_list->st_src_list)
    {
        pst_grp_member = (hmac_m2u_grp_member_stru *)OAL_DLIST_GET_ENTRY(pst_grp_member_entry,
                                                                         hmac_m2u_grp_member_stru,
                                                                         st_member_entry);

        if ((pst_hmac_user == pst_grp_member->pst_hmac_user) || (OAL_PTR_NULL == pst_hmac_user))
        {
            oal_dlist_delete_entry(&(pst_grp_member->st_member_entry));
            OAL_MEM_FREE(pst_grp_member, OAL_TRUE);
            uc_sta_count++;
        }
    }
    if (pst_grp_list->uc_sta_num >= uc_sta_count)
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2U, "{hmac_m2u_clean_snp_list::remove [%d] sta from a group with [%d] sta.}",uc_sta_count,pst_grp_list->uc_sta_num);
        pst_grp_list->uc_sta_num -= uc_sta_count;
    }
    else
    {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{hmac_m2u_clean_snp_list::could not remove [%d] sta from a group with [%d] sta.}",uc_sta_count,pst_grp_list->uc_sta_num);
        pst_grp_list->uc_sta_num = 0;
        uc_sta_count = pst_grp_list->uc_sta_num;
    }
    *puc_sta_num_removed = uc_sta_count;
}


OAL_STATIC oal_void hmac_m2u_clean_snp_list(hmac_vap_stru *pst_hmac_vap)
{
    oal_uint8                      uc_sta_num_removed = 0;
    hmac_m2u_stru                 *pst_m2u;
    hmac_m2u_snoop_list_stru      *pst_snp_list;
    hmac_m2u_grp_list_entry_stru  *pst_grp_list;
    oal_dlist_head_stru           *pst_grp_list_entry;
    oal_dlist_head_stru           *pst_grp_list_entry_temp;

    /* 获取snoop链表头 */
    pst_m2u      = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    pst_snp_list = &(pst_m2u->st_m2u_snooplist);

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_grp_list_entry, pst_grp_list_entry_temp, &(pst_snp_list->st_grp_list))
    {
        pst_grp_list = (hmac_m2u_grp_list_entry_stru *)OAL_DLIST_GET_ENTRY(pst_grp_list_entry,
                                                                            hmac_m2u_grp_list_entry_stru,
                                                                                     st_grp_entry);

        hmac_m2u_remove_node_grp(pst_grp_list, OAL_PTR_NULL,&uc_sta_num_removed);
        oal_dlist_delete_entry(&(pst_grp_list->st_grp_entry));
        OAL_MEM_FREE(pst_grp_list, OAL_TRUE);
        pst_grp_list = OAL_PTR_NULL;
        pst_snp_list->us_group_list_count--;
        if (pst_snp_list->us_total_sta_num >= uc_sta_num_removed)
        {
            pst_snp_list->us_total_sta_num -= uc_sta_num_removed;
            OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_clean_snp_list::remove a group with [%d] sta.current total sta num:[%d]}",
                             uc_sta_num_removed,pst_snp_list->us_total_sta_num);
        }
        else
        {
            OAM_ERROR_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_clean_snp_list::remove a group with [%d] sta. but current total sta num is[%d]}",
                uc_sta_num_removed,pst_snp_list->us_total_sta_num);
            pst_snp_list->us_total_sta_num = 0;
        }
    }
}


oal_void hmac_m2u_cleanup_snoopwds_node(hmac_user_stru *pst_hmac_user)
{
    oal_uint8                      uc_sta_num_removed = 0;
    hmac_vap_stru                 *pst_hmac_vap ;
    hmac_m2u_stru                 *pst_m2u;
    hmac_m2u_snoop_list_stru      *pst_snp_list;
    hmac_m2u_grp_list_entry_stru  *pst_grp_list;
    oal_dlist_head_stru           *pst_grp_list_entry;
    oal_dlist_head_stru           *pst_grp_list_entry_temp;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_hmac_user->st_user_base_info.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_INFO_LOG1(0, OAM_SF_ANY, "{hmac_m2u_cleanup_snoopwds_node::pst_hmac_vap[id=%d] null!!}",
                        pst_hmac_user->st_user_base_info.uc_vap_id);
        return;
    }

    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    pst_snp_list = &(pst_m2u->st_m2u_snooplist);

    if (WLAN_VAP_MODE_BSS_AP == (pst_hmac_vap->st_vap_base_info.en_vap_mode) &&
        OAL_PTR_NULL != pst_snp_list && (pst_m2u->en_snoop_enable))
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_grp_list_entry, pst_grp_list_entry_temp, &(pst_snp_list->st_grp_list))
        {
            pst_grp_list = (hmac_m2u_grp_list_entry_stru *)OAL_DLIST_GET_ENTRY(pst_grp_list_entry,
                                                                            hmac_m2u_grp_list_entry_stru,
                                                                                     st_grp_entry);
            hmac_m2u_remove_node_grp(pst_grp_list, pst_hmac_user,&uc_sta_num_removed);
            pst_snp_list->us_total_sta_num -= uc_sta_num_removed;
            OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_cleanup_snoopwds_node::removed [%d] sta  }",uc_sta_num_removed);
            if (OAL_TRUE == (oal_dlist_is_empty(&(pst_grp_list->st_src_list)))) //删除没有用户的组播组
            {
                oal_dlist_delete_entry(&(pst_grp_list->st_grp_entry));
                OAL_MEM_FREE(pst_grp_list, OAL_TRUE);
                pst_grp_list = OAL_PTR_NULL;
                pst_snp_list->us_group_list_count--;
                OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_cleanup_snoopwds_node::remove a null group  }");
            }
        }
    }
}


oal_uint32 hmac_m2u_tx_event( hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf)
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
                          "{hmac_tx_lan_mpdu_process_ap::hmac_tx_encap_etc failed[%d].}", ul_ret);
         OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
         return ul_ret;
     }

     /* 抛事件，传给DMAC */
     pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
     if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
     {
         OAM_WARNING_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan_etc::pst_event_mem null.}");
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
         OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_tx_lan_to_wlan_etc::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
         OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
     }

     /* 释放事件 */
     FRW_EVENT_FREE(pst_event_mem);

     return ul_ret;
}


oal_void hmac_m2u_convert_loop_end(oal_netbuf_stru *pst_copy_buf, oal_netbuf_stru **pst_buf,
                                    oal_uint8 *puc_ucast_sta_cnt, oal_uint8 *puc_ucast_sta_idx)
{
    if (OAL_PTR_NULL != pst_copy_buf)
    {
        *pst_buf = pst_copy_buf;
        pst_copy_buf = OAL_PTR_NULL;
    }
    (*puc_ucast_sta_idx)++;
    if(*puc_ucast_sta_cnt > 0)
    {
        (*puc_ucast_sta_cnt)--;
    }
}


oal_void hmac_m2u_snoop_convert_count(hmac_vap_stru *pst_vap, oal_uint8 uc_ucast_sta_cnt, oal_uint32 ul_ret, oal_netbuf_stru *pst_buf)
{
    /* ucast event fail 的发送计数 */
    if (OAL_SUCC != ul_ret)
    {
        if (uc_ucast_sta_cnt > 0)
        {
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_droped, 1);
        }
        else
        {
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_droped, 1);
        }
        hmac_free_netbuf_list_etc(pst_buf);
    }
    /* ucast发送成功的已发送组播和单播的计数 */
    else
    {
        if (uc_ucast_sta_cnt > 0)
        {
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_cnt, 1);
        }
        else
        {
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
        }
    }
}

 
oal_void hmac_m2u_snoop_change_mac_hdr(hmac_m2u_stru *pst_m2u, mac_ether_header_stru  **pst_ucast_ether_hdr, oal_netbuf_stru *pst_buf)
{
    mcast_tunnel_hdr_stru  *pst_mcast_tunHdr;
    mac_ether_header_stru  *pst_ether_hdr;
    oal_uint8               auc_srcmac[WLAN_MAC_ADDR_LEN];

    pst_ether_hdr  = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    oal_set_mac_addr(auc_srcmac, pst_ether_hdr->auc_ether_shost);
    /* 加自定义tunnel协议 */
    if (pst_m2u->en_mcast_mode & BIT0)   /* en_mcast_mode = 1 */
    {
      pst_mcast_tunHdr = (mcast_tunnel_hdr_stru *) oal_netbuf_push(pst_buf, OAL_SIZEOF(mcast_tunnel_hdr_stru));
      *pst_ucast_ether_hdr = (mac_ether_header_stru *)oal_netbuf_push(pst_buf, OAL_SIZEOF(mac_ether_header_stru));
      pst_mcast_tunHdr->proto = MAC_ETH_PROTOCOL_SUBTYPE;

      /* 拷贝原始组播源地址 */
      oal_set_mac_addr(&((*pst_ucast_ether_hdr)->auc_ether_shost[0]), auc_srcmac);

      /*拷贝新的协议类型 */
      (*pst_ucast_ether_hdr)->us_ether_type = OAL_HOST2NET_SHORT(ETHER_TUNNEL_TYPE);
    }
    /* 不加自定义tunnel协议 */
    else                                /* en_mcast_mode = 2 */
    {
        *pst_ucast_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    }
}


oal_uint32 hmac_m2u_group_is_special(hmac_vap_stru *pst_hmac_vap, oal_uint16 us_ether_data_type, oal_uint8 *puc_ip_head)
{
    mac_ip_header_stru       *pst_ip_header;
    oal_ipv6hdr_stru         *pst_ipv6_hdr;
    hmac_m2u_snoop_list_stru *pst_snp_list;
    oal_uint8                 uc_idx;
    oal_uint32                ul_grp_addr = 0;
    oal_uint8                 auc_grp_ip_addr[OAL_IPV6_ADDR_SIZE] = {0};
    hmac_m2u_stru            *pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_PTR_NULL == pst_m2u)
    {
        return OAL_FALSE;
    }
    pst_snp_list = &(pst_m2u->st_m2u_snooplist);

/*lint -e778*/
    if (us_ether_data_type == OAL_HOST2NET_SHORT(ETHER_TYPE_IP))
/*lint +e778*/
    {
        pst_ip_header = (mac_ip_header_stru *)puc_ip_head;
        ul_grp_addr = OAL_HOST2NET_LONG(pst_ip_header->ul_daddr);

        for (uc_idx = 0; uc_idx < SPECIAL_M2U_GROUP_COUNT_IPV4; uc_idx++)
        {
            if (ul_grp_addr == pst_snp_list->aul_special_group_ipv4[uc_idx])
            {
                return OAL_TRUE;
            }
        }
    }

    /*lint -e778*/
    if (us_ether_data_type == OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6))
    /*lint +e778*/
    {
        pst_ipv6_hdr = (oal_ipv6hdr_stru*)(puc_ip_head);
        oal_memcopy(auc_grp_ip_addr, (oal_int8*)(&(pst_ipv6_hdr->daddr)), OAL_IPV6_ADDR_SIZE);

        for (uc_idx = 0; uc_idx < SPECIAL_M2U_GROUP_COUNT_IPV6; uc_idx++)
        {
            if (!oal_memcmp(auc_grp_ip_addr, pst_snp_list->aul_special_group_ipv6[uc_idx], OAL_IPV6_ADDR_SIZE))
            {
                return OAL_TRUE;
            }
        }
    }

    return OAL_FALSE;
}


oal_uint32 hmac_m2u_sta_convert(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf, oal_uint8 *puc_srcmac)
{
    hmac_user_stru         *pst_user = OAL_PTR_NULL;
    oal_uint8              *puc_dstmac;
    oal_uint8               uc_ucast_sta_cnt = 0;
    oal_uint8               uc_ucast_sta_idx = 0;
    mac_ether_header_stru  *pst_ucast_ether_hdr =  OAL_PTR_NULL;
    oal_netbuf_stru        *pst_copy_buf = OAL_PTR_NULL;
    oal_uint16              us_user_idx;
    oal_uint32              ul_ret = OAL_SUCC;
    mac_tx_ctl_stru        *pst_tx_ctl;
    hmac_m2u_stru          *pst_m2u = (hmac_m2u_stru *)(pst_vap->pst_m2u);
    oal_dlist_head_stru    *pst_entry;
    oal_dlist_head_stru    *pst_dlist_tmp;
    mac_user_stru          *pst_user_tmp;
    mac_vap_stru           *pst_mac_vap;

    pst_mac_vap      = &(pst_vap->st_vap_base_info);
    uc_ucast_sta_cnt = (oal_uint8)pst_mac_vap->us_user_nums;

    if (0 == uc_ucast_sta_cnt)
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_sta_convert:: no user associate this vap}");
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
        return HMAC_TX_PASS ;
    }

    /* 遍历vap下所有用户 */
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head))
    {
        if (uc_ucast_sta_cnt > 1)
        {
            pst_copy_buf = oal_netbuf_copy(pst_buf, GFP_ATOMIC);
        }

        pst_user_tmp = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        if (OAL_PTR_NULL == pst_user_tmp)
        {
            hmac_free_netbuf_list_etc(pst_buf);
            OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_sta_convert::pst_user_tmp null.}");
            /* 组播转单播发送循环的末尾处理 */
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }

        /* 如果用户尚未关联成功 */
        if (MAC_USER_STATE_ASSOC != pst_user_tmp->en_user_asoc_state)
        {
            hmac_free_netbuf_list_etc(pst_buf);
            OAM_WARNING_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_sta_convert::sta not assoc.}");
            /* 组播转单播发送循环的末尾处理 */
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }

        pst_user = mac_res_get_hmac_user_etc(pst_user_tmp->us_assoc_id);
        if (OAL_PTR_NULL == pst_user)
        {
            hmac_free_netbuf_list_etc(pst_buf);
            OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_sta_convert::pst_hmac_user_tmp null.idx:%u}",pst_user_tmp->us_assoc_id);
            /* 组播转单播发送循环的末尾处理 */
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }

        puc_dstmac  = pst_user_tmp->auc_user_mac_addr;

        /* 发送的目的地址和发来的源地址相同的异常处理 */
        if (!oal_compare_mac_addr(puc_dstmac, puc_srcmac))
        {
            hmac_free_netbuf_list_etc(pst_buf);
            OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_sta_convert::dstmac == srcmac.}");
            /* 组播转单播发送循环的末尾处理 */
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }

        us_user_idx = pst_user_tmp->us_assoc_id;
        pst_tx_ctl  = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_buf);

        /* 组播转单播 CB字段处理 */
        MAC_GET_CB_IS_MCAST(pst_tx_ctl)     = OAL_FALSE;
        MAC_GET_CB_ACK_POLACY(pst_tx_ctl)   = WLAN_TX_NORMAL_ACK;
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)  = us_user_idx;
        MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl) = pst_m2u->en_tid_num;                     //WLAN_TIDNO_BEST_EFFORT;
        MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl)  = WLAN_WME_TID_TO_AC(pst_m2u->en_tid_num); //WLAN_WME_AC_BE;

        /* 组播转单播，mac头的封装 */
        hmac_m2u_snoop_change_mac_hdr(pst_m2u, &pst_ucast_ether_hdr, pst_buf);

        /* 将DA替换成关联设备的MAC */
        oal_set_mac_addr(&pst_ucast_ether_hdr->auc_ether_dhost[0], puc_dstmac);

        ul_ret = hmac_tx_ucast_process_etc(pst_vap, pst_buf, pst_user, pst_tx_ctl);
        if (OAL_UNLIKELY(HMAC_TX_PASS != ul_ret))
        {
            if (HMAC_TX_BUFF != ul_ret)
            {
                 /* 不等于HMAC_TX_BUFF，不缓存的直接释放 */
                 OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_sta_convert::hmac_tx_ucast_process_etc not pass or buff, ul_ret = [%d]}", ul_ret);
                 OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_droped, 1);
                 hmac_free_netbuf_list_etc(pst_buf);
            }
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }

        ul_ret = hmac_m2u_tx_event(pst_vap, pst_user, pst_buf);

        /* 组播转单播发送计数统计 */
        hmac_m2u_snoop_convert_count(pst_vap, uc_ucast_sta_cnt, ul_ret, pst_buf);

        /* 组播转单播发送循环的末尾处理 */
        hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
    }

    return HMAC_TX_DONE;

}


oal_uint32 hmac_m2u_snoop_convert(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf)
{
    hmac_user_stru         *pst_user = OAL_PTR_NULL;
    mac_ether_header_stru  *pst_ether_hdr;
    oal_uint8              *puc_dstmac;
    oal_uint8               auc_src_ip_addr[OAL_IPV6_ADDR_SIZE] = {0};
    oal_uint8               auc_dta_ip_addr[OAL_IPV6_ADDR_SIZE] = {0};
    oal_uint32              ul_grp_addr = 0;
    oal_uint8               auc_srcmac[WLAN_MAC_ADDR_LEN];
    oal_uint8               auc_grpmac[WLAN_MAC_ADDR_LEN];
    oal_uint8               auc_empty_entry_mac[WLAN_MAC_ADDR_LEN] = {0};
    oal_uint8               auc_ucast_sta_mac[MAX_STA_NUM_OF_ONE_GROUP][WLAN_MAC_ADDR_LEN];
    oal_uint8               uc_ucast_sta_cnt = 0;
    oal_uint8               uc_ucast_sta_idx = 0;
    mac_ether_header_stru  *pst_ucast_ether_hdr =  OAL_PTR_NULL;
    oal_netbuf_stru        *pst_copy_buf = OAL_PTR_NULL;
    oal_uint16              us_user_idx;
    oal_uint16              us_ether_data_type;
    oal_uint32              ul_ret = OAL_SUCC;
    oal_uint8              *puc_ip_head;
    mac_tx_ctl_stru        *pst_tx_ctl;
    mac_ip_header_stru     *pst_ip_header;
    oal_ipv6hdr_stru       *pst_ipv6_hdr;
    mac_vlan_tag_stru      *pst_vlan_tag;
    hmac_m2u_stru          *pst_m2u = (hmac_m2u_stru *)(pst_vap->pst_m2u);
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
    hmac_m2u_grp_list_entry_stru     *pst_grp_list_member;
    hmac_m2u_list_update_stru         st_list_entry;
#endif

    /* 未打开组转单播开关，发送原组播 */
    if (OAL_FALSE == pst_m2u->en_snoop_enable)
    {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::snoop is [%d] not enable}", pst_m2u->en_snoop_enable);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
        return HMAC_TX_PASS;
    }
    pst_ether_hdr  = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

/*lint -e778*/
      /* 带vlan tag 的情况*/
    if (ETHER_IS_WITH_VLAN_TAG(pst_ether_hdr->us_ether_type))
    {
        pst_vlan_tag = (mac_vlan_tag_stru*)(oal_netbuf_data(pst_buf) + (ETHER_ADDR_LEN << 1));  //偏移2个mac地址的长度,获取外层vlan tag
        pst_vlan_tag += 1;
        if (ETHER_IS_WITH_VLAN_TAG(pst_vlan_tag->us_tpid))                  //双层tag的情况，内层tag不考虑
        {
            us_ether_data_type = *((oal_uint16*)(pst_vlan_tag + 1));        //跳过内层tag后获取eth  type
            puc_ip_head = (oal_uint8*)(pst_vlan_tag + 1) + 2;               //跳过 type 和内层指向ip头起始地址
        }
        else
        {
             us_ether_data_type = *((oal_uint16*)pst_vlan_tag);             //跳过tag后获取eth  type
             puc_ip_head = (oal_uint8*)pst_vlan_tag + 2;                    //跳过 type 和tag指向ip头起始地址
        }
        pst_vlan_tag -= 1;                                                  //指向外层tag，作为参数传递到下层
    }
     /* 非vlan  的情况*/
    else
    {
        puc_ip_head = (oal_uint8*)(pst_ether_hdr + 1);
        us_ether_data_type = pst_ether_hdr->us_ether_type;
        pst_vlan_tag = OAL_PTR_NULL;
    }
/*lint +e778*/
    oal_set_mac_addr(auc_srcmac, pst_ether_hdr->auc_ether_shost);
    oal_set_mac_addr(auc_grpmac, pst_ether_hdr->auc_ether_dhost);

/*lint -e778*/
    /* 部分组播控制管理帧，进行组播转单播，发送给每一个关联STA */
    if (OAL_IS_MDNSV4_MAC((oal_uint8*)pst_ether_hdr, us_ether_data_type) ||
        OAL_IS_MDNSV6_MAC((oal_uint8*)pst_ether_hdr, us_ether_data_type) ||
        OAL_IS_ICMPV6_PROTO(us_ether_data_type, puc_ip_head) ||
        OAL_IS_IGMP_PROTO(us_ether_data_type, puc_ip_head) ||
        hmac_m2u_group_is_special(pst_vap, us_ether_data_type, puc_ip_head))
/*lint +e778*/
    {
        return hmac_m2u_sta_convert(pst_vap, pst_buf, auc_srcmac);
    }

/*lint -e778*/
    if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == us_ether_data_type)
    {
        pst_ip_header = (mac_ip_header_stru *)puc_ip_head;
        *((oal_uint32*)auc_src_ip_addr) = pst_ip_header->ul_saddr;
        ul_grp_addr = OAL_HOST2NET_LONG(pst_ip_header->ul_daddr);

        /* 该组在黑名单内,不进行组播转单播，组播处理 */
        if (hmac_m2u_snoop_is_denied_ipv4(pst_vap, ul_grp_addr))
        {
            OAM_INFO_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::group_addr [%x].[%x].[%x].[%x] is denied}\r\n}",
                                                                        (oal_uint32)((ul_grp_addr>> 24) & 0xff),
                                                                        (oal_uint32)((ul_grp_addr>> 16) & 0xff),
                                                                        (oal_uint32)((ul_grp_addr>> 8)  & 0xff),
                                                                        (oal_uint32)((ul_grp_addr) & 0xff));
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
            return HMAC_TX_PASS;
        }

    }
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6) == us_ether_data_type)
    {
        pst_ipv6_hdr = (oal_ipv6hdr_stru*)(puc_ip_head);
        oal_memcopy(auc_src_ip_addr, (oal_int8*)(&(pst_ipv6_hdr->saddr)), OAL_IPV6_ADDR_SIZE);
        oal_memcopy(auc_dta_ip_addr, (oal_int8*)(&(pst_ipv6_hdr->daddr)), OAL_IPV6_ADDR_SIZE);
        if (hmac_m2u_snoop_is_denied_ipv6(pst_vap, auc_dta_ip_addr))
        {
            OAM_INFO_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::group_addr_IPV6  [%08x]:[%08x]:[%08x]:[%08x] is denied}\r\n}",
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&auc_dta_ip_addr[0])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&auc_dta_ip_addr[4])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&auc_dta_ip_addr[8])),
                                                OAL_HOST2NET_LONG(*(oal_uint32*)(&auc_dta_ip_addr[12])));
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
            return HMAC_TX_PASS;
        }
    }
    else
    {
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
        return HMAC_TX_PASS;
    }
/*lint +e778*/

#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
    oal_memset(&st_list_entry, 0,  OAL_SIZEOF(hmac_m2u_list_update_stru));
    oal_memcopy(&(st_list_entry.auc_grp_mac), auc_grpmac, WLAN_MAC_ADDR_LEN);
    if (OAL_PTR_NULL != pst_vlan_tag)
    {
        oal_memcopy(&(st_list_entry.st_outer_vlan_tag), pst_vlan_tag, OAL_SIZEOF(mac_vlan_tag_stru));
    }
    pst_grp_list_member = hmac_m2u_find_group_list(pst_vap, &st_list_entry);
    /* 组播snoop表中不存在当前组播帧所在的组播组,组播转发处理*/
    if (OAL_PTR_NULL == pst_grp_list_member)
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert:: pst_grp_list_member is null}");
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
        return HMAC_TX_PASS ;
    }
#endif
    uc_ucast_sta_cnt = hmac_m2u_get_snooplist_member(pst_vap, auc_grpmac, auc_src_ip_addr, auc_ucast_sta_mac[0],pst_vlan_tag);//找出接收该组播的 sta 个数及他们的mac

    /* 如果没有STA需要此netbuf,不进行组播转单播处理 */
    if (0 == uc_ucast_sta_cnt)
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert:: no user need this packet}");
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
        if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6) != us_ether_data_type)
        {
            return HMAC_TX_DROP_MTOU_FAIL;
        }
#endif
        return HMAC_TX_PASS ;
    }

    /* 如果有用户需要该netbuf，但不需要组播转单播，直接发送原组播 */
    if (HMAC_M2U_MCAST_MAITAIN == pst_m2u->en_mcast_mode)  /* en_mcast_mode = 0 */
    {
        OAM_INFO_LOG2(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert:: %d user found , mcast mode is %d}", uc_ucast_sta_cnt, pst_m2u->en_mcast_mode);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_mcast_cnt, 1);
        return HMAC_TX_PASS;
    }

    pst_tx_ctl = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_buf);

    /* 组播转单播 CB字段处理 */
    MAC_GET_CB_IS_MCAST(pst_tx_ctl)     = OAL_FALSE;
    MAC_GET_CB_ACK_POLACY(pst_tx_ctl)   = WLAN_TX_NORMAL_ACK;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)  = MAC_INVALID_USER_ID;
    MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl) = pst_m2u->en_tid_num;                     //WLAN_TIDNO_BEST_EFFORT;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl)  = WLAN_WME_TID_TO_AC(pst_m2u->en_tid_num); //WLAN_WME_AC_BE;

    //OAM_INFO_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert:: current tid:%d}", pst_tx_ctl->uc_tid);

    do{
        puc_dstmac = &auc_ucast_sta_mac[uc_ucast_sta_idx][0];
        pst_tx_ctl = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_buf);

        if (uc_ucast_sta_cnt > 1)
        {
            pst_copy_buf = oal_netbuf_copy(pst_buf, GFP_ATOMIC);
        }

        /* 发送的目的地址和发来的源地址相同的异常处理 */
        if (!oal_compare_mac_addr(puc_dstmac, auc_srcmac))
        {
            hmac_free_netbuf_list_etc(pst_buf);
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_droped, 1);
            OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::dstmac == srcmac.}");

            /* 组播转单播发送循环的末尾处理 */
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }

        /* 如果接收端的STA的地址为空，则说明至少有一个STA加入到这个组后又离开了 */
        /* 这样的情况需要丢弃该帧 */
        if (!oal_compare_mac_addr(puc_dstmac, auc_empty_entry_mac))
        {
            OAM_INFO_LOG2(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::dstmac == NULL && uc_ucast_sta_cnt is %d && discard mcast is %d}", uc_ucast_sta_cnt, pst_m2u->en_discard_mcast);
            if ((uc_ucast_sta_cnt > 1) || (pst_m2u->en_discard_mcast))
            {
                hmac_free_netbuf_list_etc(pst_buf);
                OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_droped, 1);
                /* 组播转单播发送循环的末尾处理 */
                hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
                continue;
            }
            /* 如果只有一个地址为空的STA用户,将该MAC地址还原成组播组的MAC地址 */
            else
            {
                uc_ucast_sta_cnt = 0;
                puc_dstmac = pst_ether_hdr->auc_ether_dhost;

                /* 组播CB字段处理 */
                MAC_GET_CB_IS_MCAST(pst_tx_ctl)      = OAL_TRUE;
                MAC_GET_CB_ACK_POLACY(pst_tx_ctl)    = WLAN_TX_NO_ACK;
                MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl)  = WLAN_TIDNO_BCAST;
                MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl)   = WLAN_WME_TID_TO_AC(WLAN_TIDNO_BCAST);
            }
        }

        /* 组播用户的查找 */
        if ((ETHER_IS_MULTICAST(puc_dstmac)) && (!ETHER_IS_BROADCAST(puc_dstmac)))
        {
            us_user_idx = pst_vap->st_vap_base_info.us_multi_user_idx;
            ul_ret = HMAC_TX_PASS;
        }

        /* 组播转单播用户的查找 */
        else
        {
            ul_ret = mac_vap_find_user_by_macaddr_etc(&(pst_vap->st_vap_base_info), puc_dstmac, &us_user_idx);
        }

         /* 用户未找到，丢弃该帧 */
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::find user fail[%d].}", ul_ret);
            hmac_free_netbuf_list_etc(pst_buf);
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_droped, 1);
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }

        /* 转成HMAC的USER结构体 */
        pst_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_user_idx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_user))
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_m2u_snoop_convert::null param,pst_user[%d].}",us_user_idx);
            hmac_free_netbuf_list_etc(pst_buf);
            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_droped, 1);
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }
        /* 用户状态判断 */
        if (OAL_UNLIKELY(MAC_USER_STATE_ASSOC != pst_user->st_user_base_info.en_user_asoc_state))
        {
            if (uc_ucast_sta_cnt > 0)
            {
                hmac_m2u_cleanup_snoopwds_node(pst_user);
            }
            /* 发送失败计数 */
            hmac_m2u_snoop_convert_count(pst_vap, uc_ucast_sta_cnt, OAL_FAIL, pst_buf);
            OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::asoc state is [%d].}", pst_user->st_user_base_info.en_user_asoc_state);
            /* 组播转单播发送循环的末尾处理 */
            hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
            continue;
        }

        /* 目标user指针 */
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = us_user_idx;

        /* 组播转单播,单播以太网头的封装 */
        if (uc_ucast_sta_cnt > 0)
        {
            /* 组播转单播，mac头的封装 */
            hmac_m2u_snoop_change_mac_hdr(pst_m2u, &pst_ucast_ether_hdr, pst_buf);
#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)
            if (OAL_TRUE == pst_user->uc_is_wds)
            {
                hmac_vap_stru           *pst_hmac_vap;
                hmac_wds_stas_stru      *pst_sta = OAL_PTR_NULL;
                pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_user->st_user_base_info.uc_vap_id);
                if (OAL_SUCC == hmac_wds_find_sta(pst_hmac_vap,&auc_srcmac[0], &pst_sta)) /* 发送目的地址对应的sta挂在当前repeater下 */
                {
                    hmac_free_netbuf_list_etc(pst_buf);
                    OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_droped, 1);
                    OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::sta in wds tables.}");
                    hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
                    continue;
                }
            }
            else
            {
                oal_set_mac_addr(&pst_ucast_ether_hdr->auc_ether_dhost[0], &auc_ucast_sta_mac[uc_ucast_sta_idx][0]);
            }
#else
            {
                oal_set_mac_addr(&pst_ucast_ether_hdr->auc_ether_dhost[0], &auc_ucast_sta_mac[uc_ucast_sta_idx][0]);
            }
#endif
            pst_tx_ctl->bit_is_m2u_data = OAL_TRUE;
            ul_ret = hmac_tx_ucast_process_etc(pst_vap, pst_buf, pst_user, pst_tx_ctl);

            if (OAL_UNLIKELY(HMAC_TX_PASS != ul_ret))
            {
                if (HMAC_TX_BUFF != ul_ret)
                {
                    /* 不等于HMAC_TX_BUFF，不缓存的直接释放 */
                    OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_snoop_convert::hmac_tx_ucast_process_etc not pass or buff, ul_ret = [%d]}", ul_ret);
                    OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_m2u_ucast_droped, 1);
                    hmac_free_netbuf_list_etc(pst_buf);
                }

                /* 组播转单播发送循环的末尾处理 */
                hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);
                continue;
            }
        }
        ul_ret = hmac_m2u_tx_event(pst_vap, pst_user, pst_buf);

        /* 组播转单播发送计数统计 */
        hmac_m2u_snoop_convert_count(pst_vap, uc_ucast_sta_cnt, ul_ret, pst_buf);

        /* 组播转单播发送循环的末尾处理 */
        hmac_m2u_convert_loop_end(pst_copy_buf, &pst_buf, &uc_ucast_sta_cnt, &uc_ucast_sta_idx);

    }while (uc_ucast_sta_cnt > 0 && pst_m2u->en_snoop_enable);

    return HMAC_TX_DONE;
}


oal_void hmac_m2u_snoop_list_init(hmac_vap_stru *pst_hmac_vap)
{
    hmac_m2u_stru            *pst_m2u  = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    hmac_m2u_snoop_list_stru *pst_snp_list = &(pst_m2u->st_m2u_snooplist);

    pst_snp_list->us_group_list_count = 0;
    pst_snp_list->us_total_sta_num    = 0;
    pst_snp_list->us_misc             = 0;
    oal_dlist_init_head(&(pst_snp_list->st_grp_list));
}

#ifdef _PRE_WLAN_FEATURE_HERA_MCAST

oal_void hmac_m2u_unicast_convert_multicast(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_netbuf, dmac_msdu_stru *pst_msdu)
{
    mac_llc_snap_stru                  *pst_snap;
    mac_vlan_tag_stru                  *pst_vlan_tag;
    oal_uint16                          us_ether_type;
    oal_uint16                          us_ether_data_type;
    oal_uint8                          *puc_ip_head;
    oal_ipv6hdr_stru                   *pst_ipv6_hdr;
    mac_ip_header_stru                 *pst_ip_header;


/* pst_netbuf未转化为以太网格式的帧,此时只有8个字节的snap头空间*/
    pst_snap = (mac_llc_snap_stru *)oal_netbuf_data(pst_netbuf);
    us_ether_type = pst_snap->us_ether_type;

/*lint -e778*/
      /* 带vlan tag 的情况*/
    if (ETHER_IS_WITH_VLAN_TAG(us_ether_type))
    {
        pst_vlan_tag = (mac_vlan_tag_stru*)(oal_netbuf_data(pst_netbuf) + ETHER_ADDR_LEN);  //偏移1个mac地址的长度,获取外层vlan tag
        pst_vlan_tag += 1;
        if (ETHER_IS_WITH_VLAN_TAG(pst_vlan_tag->us_tpid))                  //双层tag的情况，内层tag不考虑
        {
            us_ether_data_type = *((oal_uint16*)(pst_vlan_tag + 1));        //跳过内层tag后获取eth  type
            puc_ip_head = (oal_uint8*)(pst_vlan_tag + 1) + 2;               //跳过 type 和内层指向ip头起始地址
        }
        else
        {
             us_ether_data_type = *((oal_uint16*)pst_vlan_tag);             //跳过tag后获取eth  type
             puc_ip_head = (oal_uint8*)pst_vlan_tag + 2;                    //跳过 type 和tag指向ip头起始地址
        }
        pst_vlan_tag -= 1;                                                  //指向外层tag，作为参数传递到下层
    }
     /* 非vlan  的情况*/
    else
    {
        puc_ip_head = (oal_uint8*)(oal_netbuf_data(pst_netbuf) + ETHER_ADDR_LEN + 2);  //偏移8个字节长度的snap头空间
        us_ether_data_type = *((oal_uint16*)(oal_netbuf_data(pst_netbuf) + ETHER_ADDR_LEN));
        pst_vlan_tag = OAL_PTR_NULL;
    }

    if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == us_ether_data_type)
    {
        pst_ip_header = (mac_ip_header_stru *)puc_ip_head;

        if (OAL_IPV4_IS_MULTICAST((oal_uint8 *)(&(pst_ip_header->ul_daddr))))
        {
            hmac_m2u_get_group_mac(pst_msdu->auc_da,(oal_uint8 *)(&(pst_ip_header->ul_daddr)),OAL_IPV4_ADDR_SIZE);
        }

    }
    else if (OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6) == us_ether_data_type)
    {
        pst_ipv6_hdr = (oal_ipv6hdr_stru*)(puc_ip_head);

        if (OAL_IPV6_IS_MULTICAST(pst_ipv6_hdr->daddr.s6_addr))
        {
            hmac_m2u_get_group_mac(pst_msdu->auc_da,pst_ipv6_hdr->daddr.s6_addr,OAL_IPV6_ADDR_SIZE);
        }
    }
    else
    {
        OAM_INFO_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_unicast_convert_multicast::ether type is not IP Protocol.[0x%x]}", OAL_HOST2NET_SHORT(us_ether_data_type));
        return;
    }
/*lint +e778*/

}


oal_void hmac_m2u_adaptive_list_init(hmac_vap_stru *pst_hmac_vap)
{
    oal_uint32          ul_loop  = 0;
    hmac_m2u_stru      *pst_m2u  = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    /* 初始化链表 */
    for (ul_loop = 0; ul_loop < HMAC_M2U_ADAPTIVE_STA_HASHSIZE; ul_loop++)
    {
        oal_dlist_init_head(&(pst_m2u->ast_m2u_adaptive_hash[ul_loop]));
    }
    pst_m2u->uc_adaptive_sta_count = 0;
}


OAL_STATIC oal_void hmac_m2u_clean_adaptive_list(hmac_vap_stru *pst_hmac_vap)
{
    hmac_m2u_stru                       *pst_m2u;
    hmac_m2u_adaptive_hash_stru         *pst_hash_adaptive = OAL_PTR_NULL;
    oal_dlist_head_stru                 *pst_sta_list_entry;
    oal_dlist_head_stru                 *pst_sta_list_entry_temp;
    oal_uint32                           ul_loop = 0;

    /* 获取配网STA链表头 */
    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    /* 删除链表 */
    for (ul_loop = 0; ul_loop < HMAC_M2U_ADAPTIVE_STA_HASHSIZE; ul_loop++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_sta_list_entry, pst_sta_list_entry_temp, &(pst_m2u->ast_m2u_adaptive_hash[ul_loop]))
        {
            pst_hash_adaptive = OAL_DLIST_GET_ENTRY(pst_sta_list_entry, hmac_m2u_adaptive_hash_stru, st_adaptive_entry);
            oal_dlist_delete_entry(pst_sta_list_entry);
            OAL_MEM_FREE(pst_hash_adaptive, OAL_TRUE);
        }
    }
    pst_m2u->uc_adaptive_sta_count= 0;
}


OAL_STATIC hmac_m2u_adaptive_hash_stru *hmac_m2u_find_adaptive_list(hmac_vap_stru  *pst_hmac_vap ,hmac_m2u_adaptive_list_update_stru *pst_adaptive_list_entry)
{
    hmac_m2u_adaptive_hash_stru         *pst_hash_adaptive = OAL_PTR_NULL;
    oal_dlist_head_stru                 *pst_sta_list_entry;
    hmac_m2u_stru                       *pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    oal_uint8                            uc_hash = 0;

    if (OAL_PTR_NULL == pst_m2u)
    {
        return OAL_PTR_NULL;
    }

    /* 获取HASH桶值 以及HASH链表 */
    uc_hash = (oal_uint8)HMAC_ADAPTIVE_CAL_HASH_VALUE(pst_adaptive_list_entry->auc_new_member_mac);

    /* 遍历配网STA组链表，找到地址匹配的配网STA */
    OAL_DLIST_SEARCH_FOR_EACH(pst_sta_list_entry, &(pst_m2u->ast_m2u_adaptive_hash[uc_hash]))
    {
        pst_hash_adaptive = OAL_DLIST_GET_ENTRY(pst_sta_list_entry, hmac_m2u_adaptive_hash_stru, st_adaptive_entry);

        if ((!oal_compare_mac_addr(pst_adaptive_list_entry->auc_new_member_mac, pst_hash_adaptive->auc_adaptive_mac)) &&
            (!oal_memcmp(&(pst_adaptive_list_entry->st_outer_vlan_tag),&(pst_hash_adaptive->st_outer_vlan_tag),OAL_SIZEOF(mac_vlan_tag_stru))))
        {
            return pst_hash_adaptive;
        }
     }

    return OAL_PTR_NULL;
}



OAL_STATIC hmac_m2u_adaptive_hash_stru *hmac_m2u_create_adaptive_list(hmac_vap_stru *pst_hmac_vap, hmac_m2u_adaptive_list_update_stru *pst_adaptive_list_entry)
{
    hmac_m2u_adaptive_hash_stru         *pst_hash_adaptive     = OAL_PTR_NULL;
    hmac_m2u_stru                       *pst_m2u               = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    oal_uint8                            uc_hash               = 0;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        return pst_hash_adaptive;
    }

    /* 获取HASH桶值 以及HASH链表 */
    uc_hash = (oal_uint8)HMAC_ADAPTIVE_CAL_HASH_VALUE(pst_adaptive_list_entry->auc_new_member_mac);

    pst_hash_adaptive = hmac_m2u_find_adaptive_list(pst_hmac_vap, pst_adaptive_list_entry);

    if (OAL_PTR_NULL == pst_hash_adaptive)
    {
        if (pst_m2u->uc_adaptive_sta_count >= MAX_STA_NUM_OF_ADAPTIVE)
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_create_adaptive_list::pst_m2u->uc_adaptive_sta_count is [%d].}",pst_m2u->uc_adaptive_sta_count);
            return OAL_PTR_NULL;
        }
        pst_hash_adaptive = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_m2u_adaptive_hash_stru), OAL_TRUE);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hash_adaptive))
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_create_adaptive_list::pst_hash_adaptive_new null.}");
            return pst_hash_adaptive;
        }

        oal_memset(pst_hash_adaptive, 0x0, OAL_SIZEOF(hmac_m2u_adaptive_hash_stru));
        oal_memcopy(&(pst_hash_adaptive->st_outer_vlan_tag), &(pst_adaptive_list_entry->st_outer_vlan_tag), OAL_SIZEOF(mac_vlan_tag_stru));
        oal_set_mac_addr(pst_hash_adaptive->auc_adaptive_mac, pst_adaptive_list_entry->auc_new_member_mac);
        pst_hash_adaptive->en_m2u_adaptive = OAL_FALSE;
        pst_hash_adaptive->ul_timestamp    = pst_adaptive_list_entry->ul_timestamp;
        pst_hash_adaptive->uc_adaptive_num = 1;
        oal_dlist_add_tail(&(pst_hash_adaptive->st_adaptive_entry), &(pst_m2u->ast_m2u_adaptive_hash[uc_hash]));
        pst_m2u->uc_adaptive_sta_count++;

        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_create_adaptive_list::create a new group.group num:[%d]}",pst_m2u->uc_adaptive_sta_count);
    }

    return pst_hash_adaptive;
}


oal_void hmac_m2u_adaptive_inspecting(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_buf)
{
    mac_ether_header_stru                *pst_ether_header;
    oal_uint8                            *puc_src_addr;                          /* source address which send the report and it is the member */
    hmac_m2u_adaptive_list_update_stru   st_adaptive_list_entry;                /* list entry where all member details will be updated and passed on updating the adaptive list */
    hmac_m2u_adaptive_hash_stru          *pst_hash_adaptive = OAL_PTR_NULL;
    hmac_m2u_stru                        *pst_m2u;
    mac_vlan_tag_stru                    *pst_vlan_tag;

    oal_memset(&st_adaptive_list_entry, 0, OAL_SIZEOF(hmac_m2u_adaptive_list_update_stru));
    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    if (OAL_FALSE == pst_m2u->en_adaptive_enable || OAL_FALSE == pst_m2u->en_snoop_enable)
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_adaptive_inspecting::adaptive is [%d] not enable}", pst_m2u->en_frequency_enable);
        return;
    }

    /* 获取以太网头 */
    pst_ether_header  = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    puc_src_addr  = pst_ether_header->auc_ether_shost;
    oal_set_mac_addr(st_adaptive_list_entry.auc_new_member_mac, puc_src_addr);
    st_adaptive_list_entry.pst_hmac_vap = pst_hmac_vap;
    st_adaptive_list_entry.ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

/*lint -e778*/
    /* 带vlan tag 的情况*/
    if (ETHER_IS_WITH_VLAN_TAG(pst_ether_header->us_ether_type))
    {
        pst_vlan_tag = (mac_vlan_tag_stru*)(oal_netbuf_data(pst_buf) + (ETHER_ADDR_LEN << 1));  //偏移2个mac地址的长度,获取外层vlan tag
        oal_memcopy(&(st_adaptive_list_entry.st_outer_vlan_tag), pst_vlan_tag, OAL_SIZEOF(mac_vlan_tag_stru));

        pst_vlan_tag += 1;//判断内层tag
        if (ETHER_IS_WITH_VLAN_TAG(pst_vlan_tag->us_tpid))
        {
            pst_vlan_tag += 1;//不考虑内层tag，跳过
        }
    }
     /* 非vlan  的情况*/
    else
    {
        pst_vlan_tag = OAL_PTR_NULL;
    }


    pst_hash_adaptive = hmac_m2u_find_adaptive_list(pst_hmac_vap, &st_adaptive_list_entry);

    if (OAL_PTR_NULL == pst_hash_adaptive)
    {
        pst_hash_adaptive = hmac_m2u_create_adaptive_list(pst_hmac_vap, &st_adaptive_list_entry);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hash_adaptive))
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_adaptive_inspecting::create_adaptive_list null.}");
            return;
        }
    }
    else
    {
        if (OAL_TIME_GET_RUNTIME((pst_hash_adaptive->ul_timestamp), (st_adaptive_list_entry.ul_timestamp)) > (pst_m2u->ul_threshold_time))
        {
           pst_hash_adaptive->uc_adaptive_num = 1;
           pst_hash_adaptive->ul_timestamp    = st_adaptive_list_entry.ul_timestamp;
           pst_hash_adaptive->en_m2u_adaptive = OAL_FALSE;
        }
        else
        {
           pst_hash_adaptive->ul_timestamp    = st_adaptive_list_entry.ul_timestamp;
           pst_hash_adaptive->uc_adaptive_num++;
           if (pst_hash_adaptive->uc_adaptive_num >= (pst_m2u->uc_adaptive_num))
           {
               pst_hash_adaptive->en_m2u_adaptive = OAL_TRUE;
               pst_hash_adaptive->uc_adaptive_num = pst_m2u->uc_adaptive_num;
           }
        }

    }
}


oal_uint32 hmac_m2u_sa_is_hwsmart(oal_uint16 us_ether_data_type, oal_uint8 *puc_ip_head)
{
    oal_uint32                ul_grp_addr = 0;
    mac_ip_header_stru       *pst_ip_header;

    if (us_ether_data_type != OAL_HOST2NET_SHORT(ETHER_TYPE_IP))
    {
        return OAL_FALSE;
    }

    pst_ip_header = (mac_ip_header_stru *)puc_ip_head;
    ul_grp_addr   = OAL_HOST2NET_LONG(pst_ip_header->ul_daddr);

    if ((ul_grp_addr == 0xeeeeeeee)
        || ((ul_grp_addr & 0xffff0000)>>16 == 0xef7e)
        || ((ul_grp_addr & 0xffff0000)>>16 == 0xef76)
        || ((ul_grp_addr & 0xffff0000)>>16 == 0xef0a)
        || ((ul_grp_addr & 0xffff0000)>>16 == 0xef14)
        || ((ul_grp_addr & 0xffff0000)>>16 == 0xef1e)
        || ((ul_grp_addr & 0xffff0000)>>16 == 0xef28))
    {
        return OAL_TRUE;
    }
    else
    {
        return OAL_FALSE;
    }
}


oal_uint32 hmac_m2u_sta_is_adaptive(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_mac_addr, mac_vlan_tag_stru *pst_vlan_tag)
{
    hmac_m2u_adaptive_hash_stru           *pst_hash_adaptive = OAL_PTR_NULL;
    hmac_m2u_adaptive_list_update_stru     st_adaptive_list_entry;
    oal_uint32 ul_ret = OAL_FALSE;

    oal_memset(&st_adaptive_list_entry, 0,  OAL_SIZEOF(hmac_m2u_adaptive_list_update_stru));
    oal_memcopy(&(st_adaptive_list_entry.auc_new_member_mac), puc_mac_addr, WLAN_MAC_ADDR_LEN);
    if (OAL_PTR_NULL != pst_vlan_tag)
    {
        oal_memcopy(&(st_adaptive_list_entry.st_outer_vlan_tag), pst_vlan_tag, OAL_SIZEOF(mac_vlan_tag_stru));
    }

    pst_hash_adaptive = hmac_m2u_find_adaptive_list(pst_hmac_vap, &st_adaptive_list_entry);
    if (OAL_PTR_NULL != pst_hash_adaptive)
    {
        ul_ret = pst_hash_adaptive->en_m2u_adaptive;
    }

    return ul_ret;
}


oal_uint32 hmac_m2u_multicast_is_frequency(hmac_vap_stru *pst_vap, oal_uint8 *puc_mac_addr)
{
    mac_vap_stru                 *pst_mac_vap;
    mac_device_stru              *pst_mac_device;
    oal_uint16                    uc_vap_idx;
    oal_uint16                    us_user_idx = 0;
    wlan_channel_band_enum_uint8  en_freq_band_now;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_M2U, "hmac_m2u_multicast_is_frequency::pst_hmac_vap is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap      = &(pst_vap->st_vap_base_info);
    pst_mac_device   = mac_res_get_dev_etc((oal_uint32)pst_mac_vap->uc_device_id);
    en_freq_band_now = pst_mac_vap->st_channel.en_band;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_multicast_is_frequency::pst_mac_device[%d] is NULL!}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            continue;
        }

        if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP)
        {
            continue;
        }

        if ((pst_mac_vap->st_channel.en_band == en_freq_band_now) && (OAL_SUCC == mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_mac_addr, &us_user_idx)))
        {
            return OAL_FALSE;
        }
    }

    return OAL_TRUE;
}



oal_uint32 hmac_m2u_multicast_drop(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf)
{
    mac_ether_header_stru  *pst_ether_hdr;
    oal_uint16              us_ether_data_type;
    oal_uint8              *puc_ip_head;
    mac_vlan_tag_stru      *pst_vlan_tag;
    hmac_m2u_stru          *pst_m2u = (hmac_m2u_stru *)(pst_vap->pst_m2u);

    /* 未打开组转单播开关，直接返回不处理 */
    if (OAL_FALSE == pst_m2u->en_snoop_enable)
    {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_multicast_drop::snoop is [%d] not enable}", pst_m2u->en_snoop_enable);
        return HMAC_TX_PASS;
    }
    pst_ether_hdr  = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

/*lint -e778*/
      /* 带vlan tag 的情况*/
    if (ETHER_IS_WITH_VLAN_TAG(pst_ether_hdr->us_ether_type))
    {
        pst_vlan_tag = (mac_vlan_tag_stru*)(oal_netbuf_data(pst_buf) + (ETHER_ADDR_LEN << 1));  //偏移2个mac地址的长度,获取外层vlan tag
        pst_vlan_tag += 1;
        if (ETHER_IS_WITH_VLAN_TAG(pst_vlan_tag->us_tpid))                  //双层tag的情况，内层tag不考虑
        {
            us_ether_data_type = *((oal_uint16*)(pst_vlan_tag + 1));        //跳过内层tag后获取eth  type
            puc_ip_head = (oal_uint8*)(pst_vlan_tag + 1) + 2;               //跳过 type 和内层指向ip头起始地址
        }
        else
        {
             us_ether_data_type = *((oal_uint16*)pst_vlan_tag);             //跳过tag后获取eth  type
             puc_ip_head = (oal_uint8*)pst_vlan_tag + 2;                    //跳过 type 和tag指向ip头起始地址
        }
        pst_vlan_tag -= 1;                                                  //指向外层tag，作为参数传递到下层
    }
     /* 非vlan  的情况*/
    else
    {
        puc_ip_head = (oal_uint8*)(pst_ether_hdr + 1);
        us_ether_data_type = pst_ether_hdr->us_ether_type;
        pst_vlan_tag = OAL_PTR_NULL;
    }
/*lint +e778*/

    /* 异频组播转发开关未打开 */
    if (OAL_FALSE == pst_m2u->en_frequency_enable)
    {
        /* 异频组播转发关闭，不满足智能家居单品特征 */
        if ((OAL_FALSE == hmac_m2u_sa_is_hwsmart(us_ether_data_type, puc_ip_head)) && (pst_m2u->en_discard_mcast))
        {
           return HMAC_TX_DROP_NOSMART;
        }
    }
    /* 异频组播转发开关打开 */
    else
    {
        /* 组播报文属于异频转发 */
        if (hmac_m2u_multicast_is_frequency(pst_vap, pst_ether_hdr->auc_ether_shost))
        {
            /* 组播报文来源STA不在配网模式 */
            if (((OAL_FALSE == pst_m2u->en_adaptive_enable) || (OAL_FALSE == hmac_m2u_sta_is_adaptive(pst_vap, pst_ether_hdr->auc_ether_shost, pst_vlan_tag))) && (pst_m2u->en_discard_mcast))
            {
                return HMAC_TX_DROP_NOADAP;
            }
        }
        /* 组播报文不属于异频转发 */
        else
        {
            /* 组播报文不属于异频转发,不满足智能家居单品特征 */
            if ((OAL_FALSE == hmac_m2u_sa_is_hwsmart(us_ether_data_type, puc_ip_head)) && (pst_m2u->en_discard_mcast))
            {
                return HMAC_TX_DROP_NOSMART;
            }
        }
    }
  return HMAC_TX_PASS;
}


oal_uint32  hmac_m2u_remove_old_sta(oal_void *p_arg)
{
    hmac_vap_stru *pst_hmac_vap                    = (hmac_vap_stru *)p_arg;
    hmac_m2u_stru *pst_m2u                         = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    hmac_m2u_adaptive_hash_stru *pst_hash_adaptive = OAL_PTR_NULL;
    oal_dlist_head_stru                 *pst_sta_list_entry;
    oal_dlist_head_stru                 *pst_sta_list_entry_temp;
    oal_uint32  ul_nowtime = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    oal_uint32  ul_loop    = 0;

    if (OAL_PTR_NULL == pst_m2u)
    {
        return OAL_FAIL;
    }

    /* 遍历配网STA组链表，找到地址匹配的配网STA */
    for (ul_loop = 0; ul_loop < HMAC_M2U_ADAPTIVE_STA_HASHSIZE; ul_loop++)
    {
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_sta_list_entry, pst_sta_list_entry_temp, &(pst_m2u->ast_m2u_adaptive_hash[ul_loop]))
            {
                pst_hash_adaptive = OAL_DLIST_GET_ENTRY(pst_sta_list_entry, hmac_m2u_adaptive_hash_stru, st_adaptive_entry);

                if ((OAL_FALSE == pst_hash_adaptive->en_m2u_adaptive) || ((OAL_TRUE == pst_hash_adaptive->en_m2u_adaptive) && (OAL_TIME_GET_RUNTIME((pst_hash_adaptive->ul_timestamp), ul_nowtime) >= (pst_m2u->ul_adaptive_timeout))))
                {
                    oal_dlist_delete_entry(&(pst_hash_adaptive->st_adaptive_entry));
                    OAL_MEM_FREE(pst_hash_adaptive, OAL_TRUE);
                    pst_m2u->uc_adaptive_sta_count--;
                }
             }
    }
    return OAL_SUCC;
}
#endif


oal_uint32 hmac_m2u_time_fn(oal_void *p_arg)
{
    hmac_vap_stru *pst_hmac_vap            = (hmac_vap_stru *)p_arg;
    hmac_m2u_stru *pst_m2u                 = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);
    hmac_m2u_snoop_list_stru *pst_snp_list = &(pst_m2u->st_m2u_snooplist);
    hmac_m2u_grp_list_entry_stru  *pst_grp_list;
    oal_dlist_head_stru           *pst_grp_list_entry;
    oal_dlist_head_stru           *pst_grp_list_entry_temp;
    oal_uint32                     ul_ret = OAL_SUCC;

    oal_uint32  ul_nowtime = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_grp_list_entry, pst_grp_list_entry_temp, &(pst_snp_list->st_grp_list))
    {
        pst_grp_list = (hmac_m2u_grp_list_entry_stru *)OAL_DLIST_GET_ENTRY(pst_grp_list_entry,
                                                                        hmac_m2u_grp_list_entry_stru,
                                                                            st_grp_entry);
        ul_ret = hmac_m2u_remove_expired_member(pst_grp_list, pst_hmac_vap, ul_nowtime);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }

        if (OAL_TRUE == (oal_dlist_is_empty(&(pst_grp_list->st_src_list))))
        {
            oal_dlist_delete_entry(&(pst_grp_list->st_grp_entry));
            OAL_MEM_FREE(pst_grp_list, OAL_TRUE);
            pst_grp_list = OAL_PTR_NULL;
            pst_snp_list->us_group_list_count--;
        }
    }
    return OAL_SUCC;
}


oal_void hmac_m2u_attach(hmac_vap_stru *pst_hmac_vap)
{
    hmac_m2u_stru            *pst_m2u;
    hmac_m2u_snoop_list_stru *pst_snp_list;

    pst_m2u = (hmac_m2u_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
                                            OAL_SIZEOF(hmac_m2u_stru),
                                            OAL_TRUE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_m2u))
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U, "{hmac_m2u_attach::pst_m2u is null!}");
        return;
    }

    pst_hmac_vap->pst_m2u = pst_m2u;
    pst_snp_list = &(pst_m2u->st_m2u_snooplist);

    oal_memset(pst_m2u, 0, OAL_SIZEOF(hmac_m2u_stru));
    /* 启动定时器 */
    FRW_TIMER_CREATE_TIMER(&(pst_m2u->st_snooplist_timer),
                           hmac_m2u_time_fn,
                           HMAC_DEF_M2U_TIMER,
                           pst_hmac_vap,
                           OAL_TRUE,
                           OAM_MODULE_ID_HMAC,
                           pst_hmac_vap->st_vap_base_info.ul_core_id);

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if ((mac_vap_is_msta(&pst_hmac_vap->st_vap_base_info))||(mac_vap_is_vsta(&pst_hmac_vap->st_vap_base_info)))
    {
        pst_m2u->en_snoop_enable = OAL_FALSE;
    }
    else
#endif
    {
        pst_m2u->en_snoop_enable = OAL_TRUE;  /* 默认使能，组播转发时需要根据snoop表确认是否有用户再转发 */
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
        pst_m2u->en_adaptive_enable  = OAL_TRUE;  /* 配网模式识别默认使能 */
        pst_m2u->en_frequency_enable = OAL_TRUE;  /* 异频组播转发默认使能 */
#endif
    }
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
        /* 启动配网老化定时器 */
    FRW_TIMER_CREATE_TIMER(&(pst_m2u->st_adaptivelist_timer),
                               hmac_m2u_remove_old_sta,
                               1000,
                               pst_hmac_vap,
                               OAL_TRUE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);

    hmac_m2u_adaptive_list_init(pst_hmac_vap);

    pst_m2u->ul_threshold_time        = HMAC_DEF_THRESHOLD_TIME;
    pst_m2u->ul_adaptive_timeout      = HMAC_DEF_ADAPTIVE_TIMEOUT;
    pst_m2u->uc_adaptive_num          = HMAC_DEF_NUM_OF_ADAPTIVE;
#endif
    /* 特殊业务组播 IPV4 */
    pst_snp_list->aul_special_group_ipv4[0]= HMAC_M2U_SPECIAL_GROUP1;     /* 224.0.0.1 */
    pst_snp_list->aul_special_group_ipv4[1]= HMAC_M2U_SPECIAL_GROUP2;     /* 224.0.0.2 */
    pst_snp_list->aul_special_group_ipv4[2]= HMAC_M2U_RIPV2_GROUP;        /* 224.0.0.9 */
    pst_snp_list->aul_special_group_ipv4[3]= HMAC_M2U_SPECIAL_GROUP3;     /* 224.0.0.22 */
    pst_snp_list->aul_special_group_ipv4[4]= HMAC_M2U_UPNP_GROUP;         /* 239.255.255.250 */
    /* 特殊业务组播 IPV6 */
    /*IPV6 ff02::1*/
    oal_memset(pst_snp_list->aul_special_group_ipv6[0],  0, OAL_IPV6_ADDR_SIZE);
    pst_snp_list->aul_special_group_ipv6[0][0]  = 0xff;
    pst_snp_list->aul_special_group_ipv6[0][1]  = 0x2;
    pst_snp_list->aul_special_group_ipv6[0][15] = 0x1;
    /*IPV6 ff02::2*/
    oal_memset(pst_snp_list->aul_special_group_ipv6[1],  0, OAL_IPV6_ADDR_SIZE);
    pst_snp_list->aul_special_group_ipv6[1][0]  = 0xff;
    pst_snp_list->aul_special_group_ipv6[1][1]  = 0x2;
    pst_snp_list->aul_special_group_ipv6[1][15] = 0x2;
    /*IPV6 ff02::16*/
    oal_memset(pst_snp_list->aul_special_group_ipv6[2],  0, OAL_IPV6_ADDR_SIZE);
    pst_snp_list->aul_special_group_ipv6[2][0]  = 0xff;
    pst_snp_list->aul_special_group_ipv6[2][1]  = 0x2;
    pst_snp_list->aul_special_group_ipv6[2][15] = 0x16;
    /*IPV6 ff02::1:2*/
    oal_memset(pst_snp_list->aul_special_group_ipv6[3],  0, OAL_IPV6_ADDR_SIZE);
    pst_snp_list->aul_special_group_ipv6[3][0]  = 0xff;
    pst_snp_list->aul_special_group_ipv6[3][1]  = 0x2;
    pst_snp_list->aul_special_group_ipv6[3][13] = 0x1;
    pst_snp_list->aul_special_group_ipv6[3][15] = 0x2;
    /*IPV6 ff02::1:ff02:b*/
    oal_memset(pst_snp_list->aul_special_group_ipv6[4],  0, OAL_IPV6_ADDR_SIZE);
    pst_snp_list->aul_special_group_ipv6[4][0]  = 0xff;
    pst_snp_list->aul_special_group_ipv6[4][1]  = 0x2;
    pst_snp_list->aul_special_group_ipv6[4][11] = 0x1;
    pst_snp_list->aul_special_group_ipv6[4][12] = 0xff;
    pst_snp_list->aul_special_group_ipv6[4][13] = 0x2;
    pst_snp_list->aul_special_group_ipv6[4][15] = 0xb;

    pst_m2u->en_discard_mcast         = OAL_TRUE;
    pst_m2u->ul_timeout               = HMAC_DEF_M2U_TIMEOUT;

    pst_m2u->en_mcast_mode            = HMAC_M2U_MCAST_TRANSLATE;  /*默认打开组播转单播开关*/

    /*永久组播组(224.0.0.0~224.0.0.255)中的组播已加入到组播黑名单；不在此范围中的组播组，添加到aul_deny_group*/
    pst_snp_list->uc_deny_count_ipv4  = DEFAULT_IPV4_DENY_GROUP_COUNT; /*默认额外添加1个组播为黑名单*/
    pst_snp_list->aul_deny_group[0]   = HMAC_M2U_DENY_GROUP;       /* 239.255.255.1 */

    pst_snp_list->uc_deny_count_ipv6  = DEFAULT_IPV6_DENY_GROUP_COUNT;
    oal_memset(pst_snp_list->aul_deny_group_ipv6[0],  0, OAL_IPV6_ADDR_SIZE);
    pst_snp_list->aul_deny_group_ipv6[0][0]  = 0xff;/*IPV6 LLMNR协议ff02::1:3*/
    pst_snp_list->aul_deny_group_ipv6[0][1]  = 0x2;
    pst_snp_list->aul_deny_group_ipv6[0][13] = 0x1;
    pst_snp_list->aul_deny_group_ipv6[0][15] = 0x3;

    pst_snp_list->us_total_sta_num    = 0;
    pst_snp_list->us_group_list_count = 0;
    hmac_m2u_snoop_list_init(pst_hmac_vap);
}


oal_void hmac_m2u_detach(hmac_vap_stru *pst_hmac_vap)
{
    hmac_m2u_stru    *pst_m2u  = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if(pst_m2u != OAL_PTR_NULL)
    {
        hmac_m2u_clean_snp_list(pst_hmac_vap);
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_m2u->st_snooplist_timer));
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_m2u->st_adaptivelist_timer));
        hmac_m2u_clean_adaptive_list(pst_hmac_vap);
#endif
        OAL_MEM_FREE(pst_m2u, OAL_TRUE);
        pst_hmac_vap->pst_m2u = OAL_PTR_NULL;
    }
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_m2u_snoop_inspecting);
oal_module_symbol(hmac_m2u_attach);
oal_module_symbol(hmac_m2u_snoop_convert);
oal_module_symbol(hmac_m2u_detach);
oal_module_symbol(hmac_m2u_clear_deny_table);
oal_module_symbol(hmac_m2u_print_all_snoop_list);
oal_module_symbol(hmac_m2u_show_snoop_deny_table);
oal_module_symbol(hmac_m2u_add_snoop_ipv4_deny_entry);
oal_module_symbol(hmac_m2u_add_snoop_ipv6_deny_entry);
oal_module_symbol(hmac_m2u_del_ipv4_deny_entry);
oal_module_symbol(hmac_m2u_del_ipv6_deny_entry);
oal_module_symbol(hmac_m2u_cleanup_snoopwds_node);
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
oal_module_symbol(hmac_m2u_unicast_convert_multicast);
#endif
/*lint +e578*//*lint +e19*/
#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
