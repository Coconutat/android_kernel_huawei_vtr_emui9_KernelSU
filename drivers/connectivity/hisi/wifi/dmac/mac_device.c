


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "wlan_spec.h"
#include "hal_ext_if.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "mac_regdomain.h"
#include "dmac_reset.h"
#include "mac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_DEVICE_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
extern oal_uint32 band_5g_enabled;
#endif
#ifdef _PRE_WLAN_FEATURE_WMMAC
oal_bool_enum_uint8 g_en_wmmac_switch = OAL_FALSE;
#endif

/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_uint32  mac_device_exit(mac_device_stru *pst_device)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_device_exit::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_device->uc_vap_num = 0;
    pst_device->uc_sta_num = 0;
    pst_device->st_p2p_info.uc_p2p_device_num   = 0;
    pst_device->st_p2p_info.uc_p2p_goclient_num = 0;

    mac_res_free_dev(pst_device->uc_device_id);

#if 0
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_device->en_device_state = OAL_FALSE;
#endif
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_device_set_state(pst_device, OAL_FALSE);
#endif

    return OAL_SUCC;
}


oal_uint32  mac_chip_exit(mac_board_stru *pst_board, mac_chip_stru *pst_chip)
{
#if 0
    mac_device_stru   *pst_dev;
    oal_uint32         ul_ret;
    oal_uint8          uc_device;
#endif
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_chip || OAL_PTR_NULL == pst_board))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_chip_init::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /*放入Device自身结构释放*/
#if 0
    for (uc_device = 0; uc_device < pst_chip->uc_device_nums; uc_device++)
    {
         pst_dev = mac_res_get_dev(pst_chip->auc_device_id[uc_device]);
         ul_ret = pst_board->p_device_destroy_fun(pst_dev);
         if (OAL_SUCC != ul_ret)
         {
             OAM_WARNING_LOG1(0, OAM_SF_ANY, "{mac_chip_exit::p_device_destroy_fun failed[%d].}", ul_ret);

             return ul_ret;
         }
    }
#endif

    pst_chip->uc_device_nums = 0;

    /* destroy流程最后将状态置为FALSE */
    pst_chip->en_chip_state  = OAL_FALSE;

    return OAL_SUCC;
}



oal_uint32  mac_board_exit(mac_board_stru *pst_board)
{
#if 0
    oal_uint8  uc_chip_idx;
    oal_uint32 ul_ret;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_board))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_board_exit::pst_board null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
#if 0
    while (0 != pst_board->uc_chip_id_bitmap)
    {
        /* 获取最右边一位为1的位数，此值即为chip的数组下标 */
        uc_chip_idx = oal_bit_find_first_bit_one_byte(pst_board->uc_chip_id_bitmap);
        if (OAL_UNLIKELY(uc_chip_idx >= WLAN_CHIP_MAX_NUM_PER_BOARD))
        {
            MAC_ERR_LOG2(0, "uc_chip_idx is exceeded support spec.", uc_chip_idx, pst_board->uc_chip_id_bitmap);
            OAM_ERROR_LOG2(0, OAM_SF_ANY, "{mac_board_exit::invalid uc_chip_idx[%d] uc_chip_id_bitmap=%d.}",
                           uc_chip_idx, pst_board->uc_chip_id_bitmap);

            return OAL_ERR_CODE_ARRAY_OVERFLOW;
        }

        ul_ret = mac_chip_exit(pst_board, &pst_board->ast_chip[uc_chip_idx]);
        if (OAL_SUCC != ul_ret)
        {
            MAC_WARNING_LOG1(0, "mac_chip_destroy return fail.", ul_ret);
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{mac_board_exit::mac_chip_exit failed[%d].}", ul_ret);

            return ul_ret;
        }

        /* 清除对应的bitmap位 */
        oal_bit_clear_bit_one_byte(&pst_board->uc_chip_id_bitmap, uc_chip_idx);
    }
#endif

    return OAL_SUCC;
}


wlan_bw_cap_enum_uint8 mac_device_max_band(oal_void)
{
#if ((_PRE_WLAN_CHIP_ASIC != _PRE_WLAN_CHIP_VERSION))
    return WLAN_BW_CAP_40M;
#else
    return WLAN_BW_CAP_80M;
#endif
}


oal_uint32  mac_device_init(mac_device_stru *pst_mac_device, oal_uint32 ul_chip_ver, oal_uint8 uc_chip_id, oal_uint8 uc_device_id)
{
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_device_init::pst_mac_device null.}");

       return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始化device的索引 */
    pst_mac_device->uc_chip_id   = uc_chip_id;
    pst_mac_device->uc_device_id = uc_device_id;

    /* 初始化device级别的一些参数 */
    pst_mac_device->en_max_bandwidth = WLAN_BAND_WIDTH_BUTT;
    pst_mac_device->en_max_band      = WLAN_BAND_BUTT;
    pst_mac_device->uc_max_channel   = 0;
    pst_mac_device->ul_beacon_interval = WLAN_BEACON_INTVAL_DEFAULT;

    pst_mac_device->uc_tx_chain  = 0xf;
    pst_mac_device->uc_rx_chain  = 0xf;

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    pst_mac_device->st_bss_id_list.us_num_networks = 0;
#endif


#ifdef _PRE_WLAN_FEATURE_SMPS
    pst_mac_device->en_smps = OAL_FALSE;
    pst_mac_device->uc_dev_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_MIMO;
    pst_mac_device->uc_no_smps_user_cnt = 0;
#endif

    pst_mac_device->en_device_state = OAL_TRUE;

#ifdef _PRE_WALN_FEATURE_LUT_RESET
    pst_mac_device->en_reset_switch = OAL_TRUE;
#else
    pst_mac_device->en_reset_switch = OAL_FALSE;
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

    /* 根据初始化通道数，设置支持的空间流数 */
    if ((WITP_RF_CHANNEL_NUMS == g_l_rf_channel_num) && (WITP_RF_CHANNEL_ZERO == g_l_rf_single_tran))
    {
        pst_mac_device->en_nss_num  = WLAN_DOUBLE_NSS;

        /* 发送通道为双通道，通道0 & 通道1 */
        pst_mac_device->uc_tx_chain = WITP_TX_CHAIN_DOUBLE;
    }
    else
    {
        pst_mac_device->en_nss_num = WLAN_SINGLE_NSS;

        if (WITP_RF_CHANNEL_ZERO == g_l_rf_single_tran)
        {
            /* 发送通道为双通道，通道0 */
            pst_mac_device->uc_tx_chain =  WITP_TX_CHAIN_ZERO;
        }
        else if(WITP_RF_CHANNEL_ONE == g_l_rf_single_tran)
        {
            /* 发送通道为双通道，通道1 */
            pst_mac_device->uc_tx_chain =  WITP_TX_CHAIN_ONE;
        }
    }
#else
    pst_mac_device->uc_tx_chain =  WITP_TX_CHAIN_ZERO;
#endif

    /* 默认关闭wmm,wmm超时计数器设为0 */
    pst_mac_device->en_wmm = OAL_TRUE;

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    OAL_MEMZERO(&pst_mac_device->st_psta, OAL_SIZEOF(pst_mac_device->st_psta));
#endif

    /* 根据芯片版本初始化device能力信息 */
   switch(ul_chip_ver)
   {
    case WLAN_CHIP_VERSION_HI1151V100H:
        pst_mac_device->en_protocol_cap  = WLAN_PROTOCOL_CAP_VHT;
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
        pst_mac_device->en_bandwidth_cap = (band_5g_enabled) ? mac_device_max_band() : WLAN_BW_CAP_40M;
        pst_mac_device->en_band_cap      = (band_5g_enabled) ? WLAN_BAND_CAP_2G_5G : WLAN_BAND_CAP_2G;
#else
        pst_mac_device->en_bandwidth_cap = mac_device_max_band();
        pst_mac_device->en_band_cap      = WLAN_BAND_CAP_2G_5G;
#endif
        break;

    case WLAN_CHIP_VERSION_HI1151V100L:
        pst_mac_device->en_protocol_cap  = WLAN_PROTOCOL_CAP_VHT;
        pst_mac_device->en_bandwidth_cap = WLAN_BW_CAP_40M;
        pst_mac_device->en_band_cap      = WLAN_BAND_CAP_2G;

        break;

    default:
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{mac_device_init::ul_chip_ver is not supportted[0x%x].}", ul_chip_ver);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;


    }
    pst_mac_device->bit_ldpc_coding  = OAL_TRUE;
#ifdef _PRE_WLAN_FEATURE_TXBF
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    pst_mac_device->bit_tx_stbc      = (WITP_TX_CHAIN_DOUBLE == pst_mac_device->uc_tx_chain) ? OAL_TRUE : OAL_FALSE;
    pst_mac_device->bit_su_bfmer     = (WITP_TX_CHAIN_DOUBLE == pst_mac_device->uc_tx_chain) ? OAL_TRUE : OAL_FALSE;
#else
    pst_mac_device->bit_tx_stbc      =  OAL_FALSE;
    pst_mac_device->bit_su_bfmer     =  OAL_FALSE;
#endif
    pst_mac_device->bit_su_bfmee     = OAL_TRUE;
    pst_mac_device->bit_rx_stbc      = 1;                       /* 支持2个空间流 */

    /* 关闭MU-BFMEE */
    pst_mac_device->bit_mu_bfmee     = OAL_FALSE;

#else
    pst_mac_device->bit_tx_stbc      = OAL_FALSE;
    pst_mac_device->bit_su_bfmer     = OAL_FALSE;
    pst_mac_device->bit_su_bfmee     = OAL_FALSE;
    pst_mac_device->bit_mu_bfmee     = OAL_FALSE;
    pst_mac_device->bit_rx_stbc      = 1;
#endif

    /* 初始化vap num统计信息 */
    pst_mac_device->uc_vap_num = 0;
    pst_mac_device->uc_sta_num = 0;
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_mac_device->st_p2p_info.uc_p2p_device_num   = 0;
    pst_mac_device->st_p2p_info.uc_p2p_goclient_num = 0;
    pst_mac_device->st_p2p_info.pst_primary_net_device = OAL_PTR_NULL;/* 初始化主net_device 为空指针 */
#endif

    /* 初始化默认管制域 */
    mac_init_regdomain();

    /* 初始化信道列表 */
    mac_init_channel_list();

    /* 初始化复位状态*/
    MAC_DEV_RESET_IN_PROGRESS(pst_mac_device, OAL_FALSE);
    pst_mac_device->us_device_reset_num = 0;

    /* 默认关闭DBAC特性 */
#ifdef _PRE_WLAN_FEATURE_DBAC
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    pst_mac_device->en_dbac_enabled = OAL_FALSE;
#else
    pst_mac_device->en_dbac_enabled = OAL_TRUE;
#endif
#endif


#ifdef _PRE_SUPPORT_ACS
    oal_memset(&pst_mac_device->st_acs_switch, 0, OAL_SIZEOF(pst_mac_device->st_acs_switch));
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_set_2040bss_switch(pst_mac_device, OAL_TRUE);
#else
    mac_set_2040bss_switch(pst_mac_device, OAL_FALSE);
#endif
#endif
    pst_mac_device->uc_in_suspend       = OAL_FALSE;
    pst_mac_device->uc_arpoffload_switch   = OAL_FALSE;

    pst_mac_device->uc_wapi = OAL_FALSE;

    /* AGC绑定通道默认为自适应   */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    pst_mac_device->uc_lock_channel = 0x02;
#endif
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    pst_mac_device->uc_scan_count    = 0;
#endif

    /* 初始化随机mac oui为0(3个字节都是0),确保只有Android下发有效mac oui才进行随机mac地址扫描(在随机mac扫描开关打开的情况下) */
    pst_mac_device->en_is_random_mac_addr_scan = OAL_FALSE;
    pst_mac_device->auc_mac_oui[0] = 0x0;
    pst_mac_device->auc_mac_oui[1] = 0x0;
    pst_mac_device->auc_mac_oui[2] = 0x0;

    return OAL_SUCC;
}

oal_uint32  mac_chip_init(mac_chip_stru *pst_chip, oal_uint8 uc_chip_id)
{
    return OAL_SUCC;
}

oal_uint32  mac_board_init(mac_board_stru *pst_board)
{
    return OAL_SUCC;
}



mac_vap_stru * mac_device_find_another_up_vap(mac_device_stru *pst_mac_device, oal_uint8 uc_vap_id_self)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            continue;
        }

        if (uc_vap_id_self == pst_mac_vap->uc_vap_id)
        {
            continue;
        }

        if (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state
#ifdef _PRE_WLAN_FEATURE_P2P
            || (MAC_VAP_STATE_STA_LISTEN == pst_mac_vap->en_vap_state && pst_mac_vap->us_user_nums > 0)
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_ROAMING == pst_mac_vap->en_vap_state)
#endif //_PRE_WLAN_FEATURE_ROAM
            )
        {
            return pst_mac_vap;
        }
    }

    return OAL_PTR_NULL;
}


oal_uint32  mac_device_find_up_vap(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);

            *ppst_mac_vap = OAL_PTR_NULL;

            return OAL_ERR_CODE_PTR_NULL;
        }

        if (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state
#ifdef _PRE_WLAN_FEATURE_P2P
            || (MAC_VAP_STATE_STA_LISTEN == pst_mac_vap->en_vap_state && pst_mac_vap->us_user_nums > 0)
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_ROAMING == pst_mac_vap->en_vap_state)
#endif //_PRE_WLAN_FEATURE_ROAM
            )
        {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}


oal_uint32  mac_device_find_2up_vap(
                mac_device_stru *pst_mac_device,
                mac_vap_stru   **ppst_mac_vap1,
                mac_vap_stru   **ppst_mac_vap2)
{
    mac_vap_stru                  *pst_vap;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_vap_num = 0;
    mac_vap_stru                  *past_vap[2] = {0};

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((MAC_VAP_STATE_UP == pst_vap->en_vap_state)
            || (MAC_VAP_STATE_PAUSE == pst_vap->en_vap_state)
#ifdef _PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_ROAMING == pst_vap->en_vap_state)
#endif //_PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_STA_LISTEN == pst_vap->en_vap_state && pst_vap->us_user_nums > 0))
        {
            past_vap[ul_up_vap_num] = pst_vap;
            ul_up_vap_num++;

            if (ul_up_vap_num >=2)
            {
                break;
            }
        }
    }

    if (ul_up_vap_num < 2)
    {
        return OAL_FAIL;
    }

    *ppst_mac_vap1 = past_vap[0];
    *ppst_mac_vap2 = past_vap[1];

    return OAL_SUCC;
}


oal_uint32  mac_fcs_dbac_state_check(mac_device_stru *pst_mac_device)
{
    mac_vap_stru *pst_mac_vap1;
    mac_vap_stru *pst_mac_vap2;
    oal_uint32    ul_ret;

    ul_ret = mac_device_find_2up_vap(pst_mac_device, &pst_mac_vap1, &pst_mac_vap2);
    if(OAL_SUCC != ul_ret)
    {
        return MAC_FCS_DBAC_IGNORE;
    }

    if(pst_mac_vap1->st_channel.uc_chan_number == pst_mac_vap2->st_channel.uc_chan_number)
    {
        return MAC_FCS_DBAC_NEED_CLOSE;
    }

    return MAC_FCS_DBAC_NEED_OPEN;
}


oal_uint32  mac_device_find_up_ap(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }

        if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state)&&
            (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode))
        {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}


oal_uint32  mac_device_find_up_sta(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);

            *ppst_mac_vap = OAL_PTR_NULL;

            return OAL_ERR_CODE_PTR_NULL;
        }

        if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state)&&
            (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode))
        {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}



oal_uint32  mac_device_find_up_p2p_go(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state)&&
            (WLAN_P2P_GO_MODE == pst_mac_vap->en_p2p_mode))
        {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}


oal_uint32  mac_device_calc_up_vap_num(mac_device_stru *pst_mac_device)
{
    mac_vap_stru                  *pst_vap;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_ap_num = 0;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((MAC_VAP_STATE_UP == pst_vap->en_vap_state)
            || (MAC_VAP_STATE_PAUSE == pst_vap->en_vap_state)
#ifdef _PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_ROAMING == pst_vap->en_vap_state)
#endif //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_P2P
            || (MAC_VAP_STATE_STA_LISTEN == pst_vap->en_vap_state && pst_vap->us_user_nums > 0)
#endif
            )
        {
            ul_up_ap_num++;
        }
    }

    return ul_up_ap_num;
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

mac_vap_stru *mac_find_main_proxysta(mac_device_stru *pst_mac_device)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    if(!pst_mac_device || !mac_is_proxysta_enabled(pst_mac_device))
    {
        return OAL_PTR_NULL;
    }

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            continue;
        }

        if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA)
        {
            continue;
        }

        if (mac_vap_is_msta(pst_mac_vap))
        {
            return pst_mac_vap;
        }
    }

    return OAL_PTR_NULL;
}
#endif


oal_uint32 mac_device_calc_work_vap_num(mac_device_stru *pst_mac_device)
{
    mac_vap_stru *pst_vap;
    oal_uint8 uc_vap_idx;
    oal_uint8 ul_work_vap_num = 0;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_device_calc_work_vap_numv::vap[%d] is null",
            pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((MAC_VAP_STATE_INIT != pst_vap->en_vap_state) && (MAC_VAP_STATE_BUTT != pst_vap->en_vap_state))
        {
            ul_work_vap_num++;
        }
    }

    return ul_work_vap_num;
}


oal_uint32  mac_device_is_p2p_connected(mac_device_stru *pst_mac_device)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_P2P, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }
        if((IS_P2P_GO(pst_mac_vap)||IS_P2P_CL(pst_mac_vap))&&
           (pst_mac_vap->us_user_nums > 0))
        {
            return OAL_SUCC;
        }
    }
    return OAL_FAIL;
}


oal_void mac_device_set_vap_id(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap,  oal_uint8 uc_vap_idx, wlan_vap_mode_enum_uint8 en_vap_mode, wlan_p2p_mode_enum_uint8 en_p2p_mode, oal_uint8 is_add_vap)
{
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_uint8                       uc_vap_tmp_idx = 0;
    mac_vap_stru                   *pst_tmp_vap;
#endif

    if (is_add_vap)
    {
        /* ?offload???,????HMAC????? */
        pst_mac_device->auc_vap_id[pst_mac_device->uc_vap_num++] = uc_vap_idx;

        /* device?sta???1 */
        if (WLAN_VAP_MODE_BSS_STA == en_vap_mode)
        {
            pst_mac_device->uc_sta_num++;

            /* ???uc_assoc_vap_id??????ap??? */
            pst_mac_vap->uc_assoc_vap_id = 0xff;
        }

    #ifdef _PRE_WLAN_FEATURE_P2P
        pst_mac_vap->en_p2p_mode = en_p2p_mode;
        mac_inc_p2p_num(pst_mac_vap);
        if (IS_P2P_GO(pst_mac_vap))
        {
            for (uc_vap_tmp_idx = 0; uc_vap_tmp_idx < pst_mac_device->uc_vap_num; uc_vap_tmp_idx++)
            {
                pst_tmp_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_tmp_idx]);
                if (OAL_PTR_NULL == pst_tmp_vap)
                {
                    OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_config_add_vap::pst_mac_vap null,vap_idx=%d.}",
                                   pst_mac_device->auc_vap_id[uc_vap_tmp_idx]);
                    continue;
                }

                if ((MAC_VAP_STATE_UP == pst_tmp_vap->en_vap_state) && (pst_tmp_vap != pst_mac_vap))
                {
                    pst_mac_vap->st_channel.en_band        = pst_tmp_vap->st_channel.en_band;
                    pst_mac_vap->st_channel.en_bandwidth   = pst_tmp_vap->st_channel.en_bandwidth;
                    pst_mac_vap->st_channel.uc_chan_number = pst_tmp_vap->st_channel.uc_chan_number;
                    pst_mac_vap->st_channel.uc_idx         = pst_tmp_vap->st_channel.uc_idx;
                    break;
                }
            }
        }
    #endif
    }
    else
    {
        /* ?offload???,????HMAC????? */
        pst_mac_device->auc_vap_id[pst_mac_device->uc_vap_num--] = 0;

        /* device?sta???1 */
        if (WLAN_VAP_MODE_BSS_STA == en_vap_mode)
        {
            pst_mac_device->uc_sta_num--;

            /* ???uc_assoc_vap_id??????ap??? */
            pst_mac_vap->uc_assoc_vap_id = 0xff;
        }

    #ifdef _PRE_WLAN_FEATURE_P2P
        pst_mac_vap->en_p2p_mode = en_p2p_mode;
        mac_dec_p2p_num(pst_mac_vap);
    #endif
    }
}

oal_void mac_device_set_dfr_reset(mac_device_stru *pst_mac_device, oal_uint8 uc_device_reset_in_progress)
{
    pst_mac_device->uc_device_reset_in_progress = uc_device_reset_in_progress;
}


oal_void  mac_device_set_state(mac_device_stru *pst_mac_device, oal_uint8 en_device_state)
{
    pst_mac_device->en_device_state = en_device_state;
}

oal_void  mac_device_set_channel(mac_device_stru *pst_mac_device, mac_cfg_channel_param_stru * pst_channel_param)
{
    pst_mac_device->uc_max_channel = pst_channel_param->uc_channel;
    pst_mac_device->en_max_band = pst_channel_param->en_band;
    pst_mac_device->en_max_bandwidth = pst_channel_param->en_bandwidth;
}

oal_void  mac_device_get_channel(mac_device_stru *pst_mac_device, mac_cfg_channel_param_stru * pst_channel_param)
{
    pst_channel_param->uc_channel = pst_mac_device->uc_max_channel;
    pst_channel_param->en_band = pst_mac_device->en_max_band;
    pst_channel_param->en_bandwidth = pst_mac_device->en_max_bandwidth;
}


oal_void  mac_device_set_txchain(mac_device_stru *pst_mac_device, oal_uint8 uc_tx_chain)
{
    pst_mac_device->uc_tx_chain = uc_tx_chain;
}

oal_void  mac_device_set_rxchain(mac_device_stru *pst_mac_device, oal_uint8 uc_rx_chain)
{
    pst_mac_device->uc_rx_chain = uc_rx_chain;
}

oal_void  mac_device_set_beacon_interval(mac_device_stru *pst_mac_device, oal_uint32 ul_beacon_interval)
{
    pst_mac_device->ul_beacon_interval = ul_beacon_interval;
}

oal_void  mac_device_inc_active_user(mac_device_stru *pst_mac_device)
{
    /* ?????+1 */
    pst_mac_device->uc_active_user_cnt++;
}

oal_void  mac_device_dec_active_user(mac_device_stru *pst_mac_device)
{
    if (pst_mac_device->uc_active_user_cnt)
    {
        pst_mac_device->uc_active_user_cnt--;
    }
}

#if 0
oal_void  mac_device_set_dfs(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_dfs_switch, oal_uint8 uc_debug_level)
{
    /*待整改dfs变量后 生效*/
#if 0
    pst_mac_device->en_dfs_switch = en_dfs_switch;
    pst_mac_device->uc_debug_level = uc_debug_level;
#endif
}
#endif

oal_void* mac_device_get_all_rates(mac_device_stru *pst_dev)
{
    return (oal_void *)pst_dev->st_mac_rates_11g;
}
#ifdef _PRE_WLAN_FEATURE_HILINK

oal_uint32  mac_device_clear_fbt_scan_list(mac_device_stru *pst_mac_dev, oal_uint8 *puc_param)
{

    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;
    oal_uint8                   uc_idx;
    mac_fbt_scan_result_stru   *pst_user;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_clear_fbt_scan_list::null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_clear_fbt_scan_list::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);
    /* 遍历所有用户，清零 */
    for(uc_idx = 0; uc_idx < HMAC_FBT_MAX_USER_NUM; uc_idx++)
    {
        pst_user = &(pst_fbt_scan_info->ast_fbt_scan_user_list[uc_idx]);
        OAL_MEMZERO(pst_user, OAL_SIZEOF(mac_fbt_scan_result_stru));
    }

    return OAL_SUCC;
}



oal_uint32  mac_device_set_fbt_scan_sta(mac_device_stru *pst_mac_dev, mac_fbt_scan_sta_addr_stru *pst_fbt_scan_sta)
{
    mac_fbt_scan_mgmt_stru                     *pst_fbt_scan_info;
    oal_uint32                                  ul_idx;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_set_fbt_scan_sta::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_fbt_scan_sta)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_set_fbt_scan_sta::null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);

    /* 遍历所有用户，在空用户处写入mac地址，更改使用状态 */
    for (ul_idx = 0; ul_idx < HMAC_FBT_MAX_USER_NUM; ul_idx++)
    {
        /* 如果需要侦听的STA已经在列表中，则不再重新添加 */
        if (HMAC_FBT_SCAN_USER_IS_USED == pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].uc_is_used)
        {
            if (0 == oal_memcmp(pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr, pst_fbt_scan_sta->auc_mac_addr, WLAN_MAC_ADDR_LEN))
            {
                break;
            }
        }
        else
        {
            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].uc_is_used = HMAC_FBT_SCAN_USER_IS_USED;
            oal_memcopy(pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr,
                        pst_fbt_scan_sta->auc_mac_addr, WLAN_MAC_ADDR_LEN);

            OAM_INFO_LOG4(pst_mac_dev->uc_device_id, OAM_SF_CFG, "{mac_device_set_fbt_scan_sta::MAC[%x:%x:%x:**:**:%x]}",
                            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr[0],
                            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr[1],
                            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr[2],
                            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr[5]);
            break;
        }
    }

    /* 列表已满 */
    if (OAL_UNLIKELY(HMAC_FBT_MAX_USER_NUM == ul_idx))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_set_fbt_scan_sta::scan user list is full}");
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    return OAL_SUCC;

}


oal_uint32  mac_device_set_fbt_scan_interval(mac_device_stru *pst_mac_dev, oal_uint32 ul_scan_interval)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_set_fbt_scan_interval::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);

    pst_fbt_scan_info->ul_scan_interval = ul_scan_interval;

    OAM_INFO_LOG1(pst_mac_dev->uc_device_id, OAM_SF_CFG, "{mac_device_set_fbt_scan_interval::=%d.}", pst_fbt_scan_info->ul_scan_interval);

    return OAL_SUCC;
}



oal_uint32  mac_device_set_fbt_scan_channel(mac_device_stru *pst_mac_dev, oal_uint8 uc_fbt_scan_channel)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_set_fbt_scan_channel::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);

    pst_fbt_scan_info->uc_scan_channel = uc_fbt_scan_channel;

    OAM_INFO_LOG1(pst_mac_dev->uc_device_id, OAM_SF_CFG, "{mac_device_set_fbt_scan_channel::=%d.}", pst_fbt_scan_info->uc_scan_channel);

    return OAL_SUCC;
}


oal_uint32  mac_device_set_fbt_scan_report_period(mac_device_stru *pst_mac_dev, oal_uint32 ul_fbt_scan_report_period)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_set_fbt_scan_report_period::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);
    pst_fbt_scan_info->ul_scan_report_period = ul_fbt_scan_report_period;

    OAM_INFO_LOG1(pst_mac_dev->uc_device_id, OAM_SF_CFG, "{mac_device_set_fbt_scan_report_period::ul_scan_report_period=%d.}", pst_fbt_scan_info->ul_scan_report_period);

    return OAL_SUCC;
}


oal_uint32  mac_device_set_fbt_scan_enable(mac_device_stru *pst_mac_device, oal_uint8 uc_cfg_fbt_scan_enable)
{

    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    oal_uint8                   uc_user_index = 0;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{mac_device_set_fbt_scan_enable::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (uc_cfg_fbt_scan_enable > HMAC_FBT_SCAN_OPEN)
    {
        OAM_WARNING_LOG1(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{mac_device_set_fbt_scan_enable::invalid uc_cfg_fbt_scan_enable=%d.}", uc_cfg_fbt_scan_enable);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    /* 如果要配置的模式与fbg scan管理实体中的模式一致，
       则不做处理，直接返回，并给出提示信息；*/
    if (uc_cfg_fbt_scan_enable == pst_fbt_scan_mgmt->uc_fbt_scan_enable)
    {
        OAM_WARNING_LOG0(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{mac_device_set_fbt_scan_enable::uc_cfg_fbt_scan_enable eq pst_fbt_scan_mgmt->uc_fbt_scan_enable,return.}");
        return OAL_SUCC;
    }


    /* 记录配置的模式到fbt scan管理实体中，当前只支持侦听一个用户 */
    pst_fbt_scan_mgmt->uc_fbt_scan_enable = uc_cfg_fbt_scan_enable;

    OAM_INFO_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "mac_device_set_fbt_scan_enable::user uc_cfg_fbt_scan_enable=%d, ul_scan_report_period=%d, uc_scan_channel=%d ul_scan_interval=%d",
                                    uc_cfg_fbt_scan_enable, pst_fbt_scan_mgmt->ul_scan_report_period, pst_fbt_scan_mgmt->uc_scan_channel, pst_fbt_scan_mgmt->ul_scan_interval);

    return OAL_SUCC;
}
#endif

/*lint -e19*/
oal_module_symbol(mac_device_set_vap_id);
oal_module_symbol(mac_device_set_dfr_reset);
oal_module_symbol(mac_device_set_state);
oal_module_symbol(mac_device_get_channel);
oal_module_symbol(mac_device_set_channel);
oal_module_symbol(mac_device_set_txchain);
oal_module_symbol(mac_device_set_rxchain);
oal_module_symbol(mac_device_set_beacon_interval);
oal_module_symbol(mac_device_inc_active_user);
oal_module_symbol(mac_device_dec_active_user);
#if 0
oal_module_symbol(mac_device_inc_assoc_user);
oal_module_symbol(mac_device_dec_assoc_user);
oal_module_symbol(mac_device_set_dfs);
#endif

oal_module_symbol(mac_device_init);
oal_module_symbol(mac_chip_init);
oal_module_symbol(mac_board_init);

oal_module_symbol(mac_device_exit);
oal_module_symbol(mac_chip_exit);
oal_module_symbol(mac_board_exit);

oal_module_symbol(mac_device_find_up_vap);
oal_module_symbol(mac_device_find_another_up_vap);
oal_module_symbol(mac_device_find_up_ap);
oal_module_symbol(mac_device_calc_up_vap_num);
oal_module_symbol(mac_device_calc_work_vap_num);
oal_module_symbol(mac_device_is_p2p_connected);
oal_module_symbol(mac_device_find_2up_vap);
oal_module_symbol(mac_device_find_up_p2p_go);
#ifdef _PRE_WLAN_FEATURE_HILINK
oal_module_symbol(mac_device_clear_fbt_scan_list);
oal_module_symbol(mac_device_set_fbt_scan_sta);
oal_module_symbol(mac_device_set_fbt_scan_interval);
oal_module_symbol(mac_device_set_fbt_scan_channel);
oal_module_symbol(mac_device_set_fbt_scan_report_period);
oal_module_symbol(mac_device_set_fbt_scan_enable);
#endif
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
oal_module_symbol(mac_find_main_proxysta);
#endif
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

