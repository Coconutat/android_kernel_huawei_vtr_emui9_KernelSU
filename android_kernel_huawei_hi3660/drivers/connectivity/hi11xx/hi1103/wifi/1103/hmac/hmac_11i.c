


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_types.h"
#include "oal_net.h"
#include "oal_aes.h"
#include "frw_ext_if.h"
#include "wlan_spec.h"
#include "wlan_types.h"
#include "mac_resource.h"
#include "mac_frame.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "mac_vap.h"
//#include "mac_11i.h"
#include "hmac_11i.h"
#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif
#include "hmac_main.h"
#include "hmac_crypto_tkip.h"
#include "hmac_config.h"
#include "hmac_mgmt_bss_comm.h"
#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif //_PRE_WLAN_FEATURE_ROAM

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_11i_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/


OAL_STATIC wlan_priv_key_param_stru *hmac_get_key_info(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr,
                                        oal_bool_enum_uint8 en_pairwise,oal_uint8 uc_key_index, oal_uint16 *pus_user_idx)
{
    oal_bool_enum_uint8         en_macaddr_is_zero;
    mac_user_stru              *pst_mac_user = OAL_PTR_NULL;
    oal_uint32                  ul_ret       = OAL_SUCC;

    /*1.1 根据mac addr 找到对应sta索引号*/
    en_macaddr_is_zero = mac_addr_is_zero_etc(puc_mac_addr);

    if(!MAC_11I_IS_PTK(en_macaddr_is_zero, en_pairwise))
    {
        /* 如果是组播用户，不能使用mac地址来查找 */

        /* 根据索引找到组播user内存区域 */
        *pus_user_idx = pst_mac_vap->us_multi_user_idx;
    }
    else  /* 单播用户 */
    {
        ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_mac_addr, pus_user_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                             "{hmac_get_key_info::mac_vap_find_user_by_macaddr_etc failed[%d]}", ul_ret);
            return OAL_PTR_NULL;
        }
    }

    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user_etc(*pus_user_idx);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_get_key_info::pst_mac_user[%d] null.}", *pus_user_idx);
        return OAL_PTR_NULL;
    }

    /*LOG*/
    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                  "{hmac_get_key_info::key_index=%d,pairwise=%d.}", uc_key_index, en_pairwise);

    if (OAL_PTR_NULL != puc_mac_addr)
    {
        OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                      "{hmac_get_key_info::user[%d] mac_addr = %02X:XX:XX:XX:%02X:%02X.}",
                      *pus_user_idx, puc_mac_addr[0], puc_mac_addr[4], puc_mac_addr[5]);
    }

    return mac_user_get_key_etc(pst_mac_user, uc_key_index);
}
#ifdef _PRE_WLAN_FEATURE_WAPI

oal_uint32 hmac_config_wapi_add_key_etc(mac_vap_stru *pst_mac_vap, mac_addkey_param_stru *pst_payload_addkey_params)
{
    oal_uint8                        uc_key_index;
    oal_bool_enum_uint8              en_pairwise;
    oal_uint8                       *puc_mac_addr;
    mac_key_params_stru             *pst_key_param;
    hmac_wapi_stru                  *pst_wapi;
    //hmac_wapi_key_stru              *pst_key;
    oal_uint32                       ul_ret;
    oal_uint16                       us_user_index = 0;
    mac_device_stru                 *pst_mac_device;

    uc_key_index = pst_payload_addkey_params->uc_key_index;
    if(HMAC_WAPI_MAX_KEYID <= uc_key_index)
    {
        OAM_ERROR_LOG1(0, OAM_SF_WPA, "{hmac_config_wapi_add_key_etc::keyid==%u Err!.}", uc_key_index);
        return OAL_FAIL;
    }

    en_pairwise  = pst_payload_addkey_params->en_pairwise;
    puc_mac_addr = (oal_uint8*)pst_payload_addkey_params->auc_mac_addr;
    pst_key_param   = &pst_payload_addkey_params->st_key;

    if ((WAPI_KEY_LEN * 2) != pst_key_param->key_len)
    {
        OAM_ERROR_LOG1(0, OAM_SF_WPA, "{hmac_config_wapi_add_key_etc:: key_len %u Err!.}", pst_key_param->key_len);
        return OAL_FAIL;
    }

    if(en_pairwise)
    {
        ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_mac_addr, &us_user_index);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_wapi_add_key_etc::mac_vap_find_user_by_macaddr_etc failed. %u}", ul_ret);
            return OAL_PTR_NULL;
        }
    }


    pst_wapi = hmac_user_get_wapi_ptr_etc(pst_mac_vap, en_pairwise, us_user_index);
    if (OAL_PTR_NULL == pst_wapi)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{hmac_config_wapi_add_key_etc:: get pst_wapi  Err!.}");
        return OAL_FAIL;
    }

    hmac_wapi_add_key_etc(pst_wapi, uc_key_index, pst_key_param->auc_key);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wapi_add_key_etc::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_device->uc_wapi = OAL_TRUE;

    return OAL_SUCC;
}


oal_uint32 hmac_config_wapi_add_key_and_sync_etc(mac_vap_stru *pst_mac_vap, mac_addkey_param_stru *pst_payload_addkey_params)
{
    hmac_vap_stru                   *pst_hmac_vap;
    oal_uint32                       ul_ret;

    OAM_WARNING_LOG2(0, OAM_SF_WPA,"{hmac_config_wapi_add_key_and_sync_etc:: key idx==%u, pairwise==%u}", pst_payload_addkey_params->uc_key_index, pst_payload_addkey_params->en_pairwise);

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_11i_add_key_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
#if 0
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_wapi_add_key_and_sync_etc::wapi==%u!}", WAPI_IS_WORK(pst_hmac_vap));
    if(!WAPI_IS_WORK(pst_hmac_vap))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::wapi is not working!}");
        return OAL_SUCC;
    }
#endif
    ul_ret = hmac_config_wapi_add_key_etc(&pst_hmac_vap->st_vap_base_info, pst_payload_addkey_params);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::hmac_config_wapi_add_key_etc fail[%d].}", ul_ret);
        return ul_ret;
    }

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ADD_WAPI_KEY, 0, OAL_PTR_NULL);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::WLAN_CFGID_ADD_WAPI_KEY send fail[%d].}", ul_ret);
        return ul_ret;
    }

    return ul_ret;
}

#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */


oal_uint32 hmac_config_11i_add_key_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                       *puc_mac_addr;
    hmac_user_stru                  *pst_hmac_user;
    hmac_vap_stru                   *pst_hmac_vap;
    mac_key_params_stru             *pst_key;
    mac_addkey_param_stru           *pst_payload_addkey_params;
    oal_uint32                       ul_ret = OAL_SUCC;
    oal_uint16                       us_user_idx = MAC_INVALID_USER_ID;
    oal_bool_enum_uint8              en_pairwise;
    oal_uint8                        uc_key_index;

#ifdef _PRE_WLAN_FEATURE_WAPI
    mac_device_stru                 *pst_mac_device;
#endif

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::param null,pst_mac_vap=%d, puc_param=%d.}",
                       pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_11i_add_key_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 获取参数*/
    pst_payload_addkey_params = (mac_addkey_param_stru *)puc_param;
    uc_key_index = pst_payload_addkey_params->uc_key_index;
    en_pairwise  = pst_payload_addkey_params->en_pairwise;
    puc_mac_addr = (oal_uint8*)pst_payload_addkey_params->auc_mac_addr;
    pst_key      = &(pst_payload_addkey_params->st_key);

#ifdef _PRE_WLAN_FEATURE_WAPI
    if (OAL_UNLIKELY(WLAN_CIPHER_SUITE_SMS4 == pst_key->cipher))
    {
        return hmac_config_wapi_add_key_and_sync_etc(pst_mac_vap, pst_payload_addkey_params);
    }
#endif

    /*2.2 索引值最大值检查*/
    if(uc_key_index >= WLAN_NUM_TK + WLAN_NUM_IGTK)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::invalid uc_key_index[%d].}", uc_key_index);
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }


    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                 "{hmac_config_11i_add_key_etc::key_index=%d, pairwise=%d.}",
                 uc_key_index, en_pairwise);
    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                 "{hmac_config_11i_add_key_etc::pst_params cipher=0x%08x, keylen=%d, seqlen=%d.}",
                 pst_key->cipher, pst_key->key_len, pst_key->seq_len);

    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::mac addr=%02X:XX:XX:%02X:%02X:%02X}",
                  puc_mac_addr[0], puc_mac_addr[3], puc_mac_addr[4], puc_mac_addr[5]);

    if (OAL_TRUE == en_pairwise)
    {
        /* 单播密钥存放在单播用户中 */
        ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_mac_addr, &us_user_idx);
        if (OAL_SUCC != ul_ret)
        {
            // 驱动删用户与hostapd删用户在时序上无法保证原子过程，可能出现二者同时删除的情形
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::us_user_idx[%d] find_user_by_macaddr fail[%d].}", us_user_idx, ul_ret);
            return ul_ret;
        }
    }
    else
    {
        /* 组播密钥存放在组播用户中 */
        us_user_idx = pst_mac_vap->us_multi_user_idx;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::get_mac_user null.idx:%u}",us_user_idx);
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

#ifdef _PRE_WLAN_FEATURE_WAPI
    /* 11i的情况下，关掉wapi端口 */
    hmac_wapi_reset_port_etc(&pst_hmac_user->st_wapi);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_11i_add_key_etc::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_device->uc_wapi = OAL_FALSE;
#endif


    /*3.1 将加密属性更新到用户中*/
    ul_ret = mac_vap_add_key_etc(pst_mac_vap,  &pst_hmac_user->st_user_base_info, uc_key_index, pst_key);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_key_etc::mac_11i_add_key fail[%d].}", ul_ret);
        return ul_ret;
    }

    if (OAL_TRUE == en_pairwise)
    {
        mac_user_set_key_etc(&pst_hmac_user->st_user_base_info, WLAN_KEY_TYPE_PTK, pst_key->cipher, uc_key_index);
    }
    else
    {
        mac_user_set_key_etc(&pst_hmac_user->st_user_base_info, WLAN_KEY_TYPE_RX_GTK, pst_key->cipher, uc_key_index);
    }

    /* 设置用户8021x端口合法性的状态为合法 */
    mac_user_set_port_etc(&pst_hmac_user->st_user_base_info, OAL_TRUE);

#ifdef _PRE_WLAN_FEATURE_ROAM
    if ((WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)  &&
        (MAC_VAP_STATE_ROAMING == pst_mac_vap->en_vap_state) &&
        (OAL_FALSE == en_pairwise))
    {
        hmac_roam_add_key_done_etc(pst_hmac_vap);
    }
#endif //_PRE_WLAN_FEATURE_ROAM
    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ADD_KEY, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{hmac_config_11i_add_key_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_11i_get_key_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    wlan_priv_key_param_stru     *pst_priv_key = OAL_PTR_NULL;
    oal_key_params_stru           st_key;
    oal_uint8                     uc_key_index;
    oal_uint16                    us_user_idx  = MAC_INVALID_USER_ID;
    oal_bool_enum_uint8           en_pairwise;
    oal_uint8                    *puc_mac_addr = OAL_PTR_NULL;
    void                         *cookie;
    void                        (*callback)(void*, oal_key_params_stru*);
    mac_getkey_param_stru        *pst_payload_getkey_params = OAL_PTR_NULL;


    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_WPA,
                      "{hmac_config_11i_get_key_etc::param null, pst_mac_vap=%d, puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 获取参数*/
    pst_payload_getkey_params = (mac_getkey_param_stru *)puc_param;
    uc_key_index = pst_payload_getkey_params->uc_key_index;
    en_pairwise  = pst_payload_getkey_params->en_pairwise;
    puc_mac_addr = pst_payload_getkey_params->puc_mac_addr;
    cookie       = pst_payload_getkey_params->cookie;
    callback     = pst_payload_getkey_params->callback;

    /*2.2 索引值最大值检查*/
    if(uc_key_index >= WLAN_NUM_TK + WLAN_NUM_IGTK)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_get_key_etc::uc_key_index invalid[%d].}", uc_key_index);
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    /*3.1 获取密钥*/
    pst_priv_key = hmac_get_key_info(pst_mac_vap, puc_mac_addr, en_pairwise, uc_key_index, &us_user_idx);
    if (OAL_PTR_NULL == pst_priv_key)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_get_key_etc::key is null.pairwise[%d], key_idx[%d]}",
                    en_pairwise, uc_key_index);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (0 == pst_priv_key->ul_key_len)
    {
        OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_get_key_etc::key len = 0.pairwise[%d], key_idx[%d]}",
                    en_pairwise, uc_key_index);
        return OAL_ERR_CODE_SECURITY_KEY_LEN;
    }


    /*4.1 密钥赋值转换*/
    oal_memset(&st_key, 0, sizeof(st_key));
    st_key.key     = pst_priv_key->auc_key;
    st_key.key_len = (oal_int32)pst_priv_key->ul_key_len;
    st_key.seq     = pst_priv_key->auc_seq;
    st_key.seq_len = (oal_int32)pst_priv_key->ul_seq_len;
    st_key.cipher  = pst_priv_key->ul_cipher;

    /*5.1 调用回调函数*/
    if (callback)
    {
        callback(cookie, &st_key);
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_11i_remove_key_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    wlan_priv_key_param_stru  *pst_priv_key = OAL_PTR_NULL;
    oal_uint32                 ul_ret       = OAL_SUCC;
    oal_uint8                  uc_key_index;
    oal_uint16                 us_user_idx  = MAC_INVALID_USER_ID;
    oal_bool_enum_uint8        en_pairwise;
    oal_uint8                 *puc_mac_addr;
    mac_removekey_param_stru  *pst_payload_removekey_params = OAL_PTR_NULL;
    wlan_cfgid_enum_uint16     en_cfgid;
	mac_user_stru             *pst_mac_user;
    oal_bool_enum_uint8        en_macaddr_is_zero;

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_WPA, "{hmac_config_11i_remove_key_etc::param null,pst_mac_vap=%d, puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 获取参数*/
    pst_payload_removekey_params = (mac_removekey_param_stru *)puc_param;
    uc_key_index = pst_payload_removekey_params->uc_key_index;
    en_pairwise  = pst_payload_removekey_params->en_pairwise;
    puc_mac_addr = pst_payload_removekey_params->auc_mac_addr;

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_remove_key_etc::uc_key_index=%d, en_pairwise=%d.}",
                  uc_key_index, en_pairwise);

    /*2.2 索引值最大值检查*/
    if(uc_key_index >= WLAN_NUM_TK + WLAN_NUM_IGTK)
    {
        /* 内核会下发删除6 个组播密钥，驱动现有6个组播密钥保存空间 */
        /* 对于检测到key idx > 最大密钥数，不做处理 */
        OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_remove_key_etc::invalid uc_key_index[%d].}", uc_key_index);
        return OAL_SUCC;
    }

    /*3.1 获取本地密钥信息*/
    pst_priv_key = hmac_get_key_info(pst_mac_vap, puc_mac_addr, en_pairwise, uc_key_index, &us_user_idx);
    if (OAL_PTR_NULL == pst_priv_key)
    {
        if(MAC_INVALID_USER_ID == us_user_idx)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_remove_key_etc::user already del.}");
            return OAL_SUCC;
        }
        else
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_remove_key_etc::pst_priv_key null.}");
            return OAL_ERR_CODE_SECURITY_USER_INVAILD;
        }
    }

    if (0 == pst_priv_key->ul_key_len)
    {
        /* 如果检测到密钥没有使用， 则直接返回正确 */
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_remove_key_etc::ul_key_len=0.}");
        return OAL_SUCC;
    }

    /*4.1 区分是wep还是wpa*/
    if ((WLAN_CIPHER_SUITE_WEP40 == pst_priv_key->ul_cipher) || (WLAN_CIPHER_SUITE_WEP104 == pst_priv_key->ul_cipher))
    {
        mac_mib_set_wep_etc(pst_mac_vap, uc_key_index, WLAN_WEP_40_KEY_SIZE);
        en_cfgid = WLAN_CFGID_REMOVE_WEP_KEY;
    }
    else
    {
        en_macaddr_is_zero = mac_addr_is_zero_etc(puc_mac_addr);
        if(MAC_11I_IS_PTK(en_macaddr_is_zero, en_pairwise))
        {
            pst_mac_user = mac_vap_get_user_by_addr_etc(pst_mac_vap, puc_mac_addr);
            if (OAL_PTR_NULL == pst_mac_user)
            {
                return OAL_ERR_CODE_SECURITY_USER_INVAILD;
            }
            pst_mac_user->st_user_tx_info.st_security.en_cipher_key_type = HAL_KEY_TYPE_BUTT;
        }
        else
        {
            pst_mac_user = (mac_user_stru *)mac_res_get_mac_user_etc(pst_mac_vap->us_multi_user_idx);
            if (OAL_PTR_NULL == pst_mac_user)
            {
                return OAL_ERR_CODE_SECURITY_USER_INVAILD;
            }
        }
        en_cfgid = WLAN_CFGID_REMOVE_KEY;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
        if (!mac_vap_is_vsta(pst_mac_vap))
#endif
        {
            mac_user_set_port_etc(pst_mac_user, OAL_FALSE);
            mac_user_init_key_etc(pst_mac_user);

            if(WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode && MAC_VAP_STATE_STA_FAKE_UP != pst_mac_vap->en_vap_state)
            {
                mac_user_set_pmf_active_etc(pst_mac_user, pst_mac_vap->en_user_pmf_cap);
            }

        }
    }

    /*4.2 抛事件到dmac层处理*/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, en_cfgid, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                      "{hmac_config_11i_remove_key_etc::hmac_config_send_event_etc failed[%d], en_cfgid=%d .}", ul_ret, en_cfgid);
        return ul_ret;
    }

    /* 5.1 删除密钥成功，设置密钥长度为0 */
    pst_priv_key->ul_key_len = 0;

    return ul_ret;
}


oal_uint32 hmac_config_11i_set_default_key_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                    ul_ret       = OAL_SUCC;
    oal_uint8                     uc_key_index = 0;
    oal_bool_enum_uint8           en_unicast   = OAL_FALSE;
    oal_bool_enum_uint8           en_multicast = OAL_FALSE;
    mac_setdefaultkey_param_stru *pst_payload_setdefaultkey_params;

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{hmac_config_11i_set_default_key_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*2.1 获取参数*/
    pst_payload_setdefaultkey_params = (mac_setdefaultkey_param_stru *)puc_param;
    uc_key_index = pst_payload_setdefaultkey_params->uc_key_index;
    en_unicast   = pst_payload_setdefaultkey_params->en_unicast;
    en_multicast = pst_payload_setdefaultkey_params->en_multicast;

    /*2.2 索引值最大值检查*/
    if(uc_key_index >= (WLAN_NUM_TK + WLAN_NUM_IGTK))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_set_default_key_etc::invalid uc_key_index[%d].}",
                       uc_key_index);
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    /*2.3 参数有效性检查*/
    if ((OAL_FALSE == en_multicast) && (OAL_FALSE == en_unicast))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_set_default_key_etc::not ptk or gtk,invalid mode.}");
        return OAL_ERR_CODE_SECURITY_PARAMETERS;
    }

    if (uc_key_index >= WLAN_NUM_TK)
    {
        /*3.1 设置default mgmt key属性*/
        ul_ret = mac_vap_set_default_mgmt_key_etc(pst_mac_vap, uc_key_index);
    }
    else
    {
        ul_ret = mac_vap_set_default_key_etc(pst_mac_vap, uc_key_index);
    }

    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{hmac_config_11i_set_default_key_etc::set key[%d] failed[%d].}", uc_key_index, ul_ret);
        return ul_ret;
    }

    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DEFAULT_KEY, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{hmac_config_11i_set_default_key_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_set_default_key_etc::key_id[%d] un[%d] mu[%d] OK}",
                  uc_key_index, en_unicast, en_multicast);

    return ul_ret;
}


oal_uint32 hmac_config_11i_add_wep_entry_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_user_stru                    *pst_mac_user;
    oal_uint32                        ul_ret;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{hmac_config_11i_add_wep_entry_etc::PARMA NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_user =  (mac_user_stru *)mac_vap_get_user_by_addr_etc(pst_mac_vap, puc_param);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_11i_add_wep_entry_etc::mac_user NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = mac_user_update_wep_key_etc(pst_mac_user, pst_mac_vap->us_multi_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{hmac_config_11i_add_wep_entry_etc::mac_wep_add_usr_key failed[%d].}", ul_ret);
        return ul_ret;
    }
    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ADD_WEP_ENTRY, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{hmac_config_11i_add_wep_entry_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    /* 设置用户的发送加密套件*/
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                   "{hmac_config_11i_add_wep_entry_etc:: usridx[%d] OK.}", pst_mac_user->us_assoc_id);

    return ul_ret;
}


oal_uint32 hmac_init_security_etc(mac_vap_stru *pst_mac_vap,oal_uint8 *puc_addr)
{
    oal_uint32 ul_ret = OAL_SUCC;
    oal_uint16       us_len;
    oal_uint8       *puc_param;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_TRUE == mac_is_wep_enabled(pst_mac_vap))
    {
        puc_param = puc_addr;
        us_len = WLAN_MAC_ADDR_LEN;
        ul_ret = hmac_config_11i_add_wep_entry_etc(pst_mac_vap, us_len, puc_param);
    }
    return ul_ret;
}


oal_uint32 hmac_check_capability_mac_phy_supplicant_etc(mac_vap_stru      *pst_mac_vap,
                                                               mac_bss_dscr_stru  *pst_bss_dscr)
{
    oal_uint32 ul_ret = OAL_FAIL;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_bss_dscr))
    {
        OAM_WARNING_LOG2(0, OAM_SF_WPA, "{hmac_check_capability_mac_phy_supplicant_etc::input null %x %x.}", pst_mac_vap, pst_bss_dscr);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 根据协议模式重新初始化STA HT/VHT mib值 */
    mac_vap_config_vht_ht_mib_by_protocol_etc(pst_mac_vap);

    ul_ret = hmac_check_bss_cap_info_etc(pst_bss_dscr->us_cap_info, pst_mac_vap);
    if (ul_ret != OAL_TRUE)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                         "{hmac_check_capability_mac_phy_supplicant_etc::hmac_check_bss_cap_info_etc failed[%d].}", ul_ret);
    }

    /* check bss capability info PHY,忽略PHY能力不匹配的AP */
    mac_vap_check_bss_cap_info_phy_ap_etc(pst_bss_dscr->us_cap_info, pst_mac_vap);

    return OAL_SUCC;
}


oal_uint32  hmac_sta_protocol_down_by_chipher(mac_vap_stru *pst_mac_vap, mac_bss_dscr_stru *pst_bss_dscr)
{
    mac_cfg_mode_param_stru         st_cfg_mode;
    oal_uint32                      aul_pair_suite[WLAN_PAIRWISE_CIPHER_SUITES] = {0};
    oal_bool_enum_uint8             en_legcy_only     = OAL_FALSE;

    if (pst_mac_vap->en_protocol >= WLAN_HT_MODE)
    {
        /* 在WEP / TKIP 加密模式下，不能工作在HT MODE */
        if (OAL_TRUE == mac_mib_get_privacyinvoked(pst_mac_vap) &&
            OAL_FALSE == mac_mib_get_rsnaactivated(pst_mac_vap))
        {
            en_legcy_only = OAL_TRUE;
        }

        if (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_wpa)
        {
            aul_pair_suite[0] = MAC_WPA_CHIPER_TKIP;

            if (0 != mac_mib_wpa_pair_match_suites(pst_mac_vap, aul_pair_suite))
            {
                en_legcy_only = OAL_TRUE;
            }
        }

        if (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_wpa2)
        {
            aul_pair_suite[0] = MAC_RSN_CHIPER_TKIP;

            if (0 != mac_mib_rsn_pair_match_suites(pst_mac_vap, aul_pair_suite))
            {
                en_legcy_only = OAL_TRUE;
            }
        }

#ifdef _PRE_WLAN_FEATURE_WAPI
        if (pst_bss_dscr->uc_wapi)
        {
            en_legcy_only = OAL_TRUE;
        }
#endif
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "hmac_sta_protocol_down_by_chipher::legacy_only[%d],wpa[%d]wpa2[%d]",
                           en_legcy_only, pst_mac_vap->st_cap_flag.bit_wpa, pst_mac_vap->st_cap_flag.bit_wpa2);

    st_cfg_mode.en_protocol = pst_mac_vap->en_protocol;

    if (OAL_TRUE == en_legcy_only)
    {
        if (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
        {
            st_cfg_mode.en_protocol = WLAN_MIXED_ONE_11G_MODE;
            pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
        }
        if (WLAN_BAND_5G == pst_mac_vap->st_channel.en_band)
        {
            st_cfg_mode.en_protocol = WLAN_LEGACY_11A_MODE;
            pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
        }
    }

    if (st_cfg_mode.en_protocol >= WLAN_HT_MODE)
    {
        mac_mib_set_TxAggregateActived(pst_mac_vap, OAL_TRUE);
        mac_mib_set_AmsduAggregateAtive(pst_mac_vap, OAL_TRUE);
    }
    else
    {
        mac_mib_set_TxAggregateActived(pst_mac_vap, OAL_FALSE);
        mac_mib_set_AmsduAggregateAtive(pst_mac_vap, OAL_FALSE);
    }

#ifdef _PRE_WIFI_DMT
    hmac_config_sta_update_rates_etc(pst_mac_vap, &st_cfg_mode, OAL_PTR_NULL);
#endif

    mac_vap_init_by_protocol_etc(pst_mac_vap, st_cfg_mode.en_protocol);

    return OAL_SUCC;
}


oal_uint32 hmac_en_mic_etc(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf, oal_uint8 *puc_iv_len)
{
    wlan_priv_key_param_stru             *pst_key        = OAL_PTR_NULL;
    oal_uint32                            ul_ret         = OAL_SUCC;
    wlan_ciper_protocol_type_enum_uint8   en_cipher_type = WLAN_80211_CIPHER_SUITE_NO_ENCRYP;
    wlan_cipher_key_type_enum_uint8       en_key_type    = 0;

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_hmac_vap) ||
        (OAL_PTR_NULL == pst_hmac_user) ||
        (OAL_PTR_NULL == pst_netbuf) ||
        (OAL_PTR_NULL == puc_iv_len))
    {
        OAM_ERROR_LOG4(0, OAM_SF_WPA, "{hmac_en_mic_etc::pst_hmac_vap=%d pst_hmac_user=%d pst_netbuf=%d puc_iv_len=%d.}",
                       pst_hmac_vap, pst_hmac_user, pst_netbuf, puc_iv_len);
        return OAL_ERR_CODE_PTR_NULL;
    }

    *puc_iv_len    = 0;
    en_key_type    = pst_hmac_user->st_user_base_info.st_user_tx_info.st_security.en_cipher_key_type;
    en_cipher_type = pst_hmac_user->st_user_base_info.st_key_info.en_cipher_type;
    pst_key = mac_user_get_key_etc(&pst_hmac_user->st_user_base_info, pst_hmac_user->st_user_base_info.st_key_info.uc_default_index);
    if (OAL_PTR_NULL == pst_key)
    {
        OAM_ERROR_LOG1(0, OAM_SF_WPA, "{hmac_en_mic_etc::mac_user_get_key_etc FAIL. en_key_type[%d]}", en_key_type);
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    switch (en_cipher_type)
    {
        case WLAN_80211_CIPHER_SUITE_TKIP:
            if (en_key_type  == 0 || en_key_type > 5)
            {
                return OAL_ERR_CODE_SECURITY_KEY_TYPE;
            }
            ul_ret = hmac_crypto_tkip_enmic_etc(pst_key, pst_netbuf);
            if (OAL_SUCC != ul_ret)
            {
                OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WPA,
                              "{hmac_en_mic_etc::hmac_crypto_tkip_enmic_etc failed[%d].}", ul_ret);
                return ul_ret;
            }
            *puc_iv_len = WEP_IV_FIELD_SIZE + EXT_IV_FIELD_SIZE;
            break;
        case WLAN_80211_CIPHER_SUITE_CCMP:
            *puc_iv_len = WEP_IV_FIELD_SIZE + EXT_IV_FIELD_SIZE;
            break;
        default:
            break;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_de_mic_etc(hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf)
{
    wlan_priv_key_param_stru             *pst_key        = OAL_PTR_NULL;
    oal_uint32                            ul_ret         = OAL_SUCC;
    wlan_ciper_protocol_type_enum_uint8   en_cipher_type = WLAN_80211_CIPHER_SUITE_NO_ENCRYP;
    wlan_cipher_key_type_enum_uint8       en_key_type    = 0;

    /*1.1 入参检查*/
    if ((OAL_PTR_NULL == pst_hmac_user) ||
        (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{hmac_de_mic_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_key_type    = pst_hmac_user->st_user_base_info.st_user_tx_info.st_security.en_cipher_key_type;
    en_cipher_type = pst_hmac_user->st_user_base_info.st_key_info.en_cipher_type;
    pst_key = mac_user_get_key_etc(&pst_hmac_user->st_user_base_info, pst_hmac_user->st_user_base_info.st_key_info.uc_default_index);
    if (OAL_PTR_NULL == pst_key)
    {
        OAM_ERROR_LOG1(0, OAM_SF_WPA, "{hmac_de_mic_etc::mac_user_get_key_etc FAIL. en_key_type[%d]}", en_key_type);
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    switch (en_cipher_type)
    {
        case WLAN_80211_CIPHER_SUITE_TKIP:
            if (en_key_type  == 0 || en_key_type > 5)
            {
                return OAL_ERR_CODE_SECURITY_KEY_TYPE;
            }
            ul_ret = hmac_crypto_tkip_demic_etc(pst_key, pst_netbuf);
            if (OAL_SUCC != ul_ret)
            {
                OAM_ERROR_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_WPA,
                               "{hmac_de_mic_etc::hmac_crypto_tkip_demic_etc failed[%d].}", ul_ret);
                return ul_ret;
            }
            break;
        default:
            break;
    }

    return OAL_SUCC;
}



oal_uint32 hmac_rx_tkip_mic_failure_process_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    frw_event_mem_stru                 *pst_hmac_event_mem;
    frw_event_hdr_stru                 *pst_event_hdr;
    dmac_to_hmac_mic_event_stru        *pst_mic_event;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{hmac_rx_tkip_mic_failure_process_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event           = frw_get_event_stru(pst_event_mem);
    pst_event_hdr       = &(pst_event->st_event_hdr);
    pst_mic_event       = (dmac_to_hmac_mic_event_stru *)&(pst_event->auc_event_data);

    /* 将mic事件抛到WAL */
    pst_hmac_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_to_hmac_mic_event_stru));
    if (OAL_PTR_NULL == pst_hmac_event_mem)
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_WPA, "{hmac_rx_tkip_mic_failure_process_etc::pst_hmac_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_hmac_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_MIC_FAILURE,
                       OAL_SIZEOF(dmac_to_hmac_mic_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_event_hdr->uc_chip_id,
                       pst_event_hdr->uc_device_id,
                       pst_event_hdr->uc_vap_id);

    /* 去关联的STA mac地址 */
    oal_memcopy((oal_uint8 *)frw_get_event_payload(pst_hmac_event_mem),(oal_uint8 *)pst_mic_event, sizeof(dmac_to_hmac_mic_event_stru));

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_hmac_event_mem);
    FRW_EVENT_FREE(pst_hmac_event_mem);
    return OAL_SUCC;
}


oal_uint32 hmac_11i_ether_type_filter_etc(hmac_vap_stru *pst_vap, hmac_user_stru *pst_hmac_user, oal_uint16 us_ether_type)
{
    if (OAL_FALSE == pst_hmac_user->st_user_base_info.en_port_valid)/* 判断端口是否打开 */
    {
        /* 接收数据时，针对非EAPOL 的数据帧做过滤 */
        if (oal_byteorder_host_to_net_uint16(ETHER_TYPE_PAE) != us_ether_type)
        {
            OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_WPA,
                           "{hmac_11i_ether_type_filter_etc::TYPE 0x%04x not permission.}", us_ether_type);
            return OAL_ERR_CODE_SECURITY_PORT_INVALID;
        }
    }

    return OAL_SUCC;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_config_11i_set_default_key_etc);
oal_module_symbol(hmac_config_11i_remove_key_etc);
oal_module_symbol(hmac_config_11i_get_key_etc);
oal_module_symbol(hmac_config_11i_add_key_etc);
oal_module_symbol(hmac_config_11i_add_wep_entry_etc);
/*oal_module_symbol(hmac_config_rssi_switch);*/
/*lint +e578*//*lint +e19*/



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

