


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "frw_task.h"

#include "wlan_spec.h"

#include "hal_ext_if.h"

#include "mac_vap.h"
#include "mac_resource.h"
#include "dmac_resource.h"
#include "dmac_vap.h"
#include "dmac_dfx.h"
#include "dmac_reset.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_DFX_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
dmac_dft_stru       g_st_dmac_dft_info;
oal_uint16   g_us_wlan_assoc_user_max_num     = WLAN_ASSOC_USER_MAX_NUM_LIMIT;     /* 关联用户的最大个数 Root AP模式下为32个,Repeater模式下是15个 */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* 由于WL_L2_DRAM大小限制，目前暂时开放2个业务vap，整体规格开放待后续优化 TBD */
oal_uint32   g_ul_wlan_vap_max_num_per_device = 4 + 1;  /* 4个AP + 1个配置vap */
#else
oal_uint32   g_ul_wlan_vap_max_num_per_device = 4 + 1;  /* 4个AP + 1个配置vap */
#endif


/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_uint32  dmac_dft_init(oal_void)
{
    oal_uint32  ul_core_id;
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
    /* 找一个绑定了wifi中断的核*/
    for(ul_core_id = 0; ul_core_id < WLAN_FRW_MAX_NUM_CORES; ul_core_id++)
    {
        if(frw_task_get_state(ul_core_id))
        {
            break;
        }
    }
#else
    ul_core_id = OAL_GET_CORE_ID();
#endif
    if(ul_core_id == WLAN_FRW_MAX_NUM_CORES)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "dmac_timestamp_init:dft timer init failed");
        return OAL_FAIL;
    }
    /* 创建异常统计上报定时器 */
    FRW_TIMER_CREATE_TIMER(&g_st_dmac_dft_info.st_excp_stat_info.st_timer,
                           oam_exception_stat_timeout,
                           OAM_EXCP_STATS_TIMEOUT,
                           OAL_PTR_NULL,
                           OAL_TRUE,
                           OAM_MODULE_ID_DMAC,
                           ul_core_id);
    return OAL_SUCC;
}


oal_uint32  dmac_dft_exit(oal_void)
{
    /* 删除异常统计上报定时器 */
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&g_st_dmac_dft_info.st_excp_stat_info.st_timer);

    return OAL_SUCC;
}

/*
TBD，注册到事件中，被事件调用完成同步操作
*/
#if ((defined(_PRE_PRODUCT_ID_HI110X_DEV)) || (defined(_PRE_PRODUCT_ID_HI110X_HOST)))

oal_uint32  dmac_custom_init(oal_uint32 ul_psta_enable)
{
    /* 硬件限制:3个STA; 2个AP */
    /* 软件规格:
        1)AP 模式:  2个ap + 1个配置vap
        2)STA 模式: 3个sta + 1个配置vap
        3)STA+P2P共存模式:  1个sta + 1个P2P_dev + 1个P2P_GO/Client + 1个配置vap
        4)STA+Proxy STA共存模式:  1个sta + ?个proxy STA + 1个配置vap
    */
    g_us_wlan_assoc_user_max_num     = 8;
    // TBD 设定为WLAN_ASSOC_USER_MAX_NUM_LIMIT;
    g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;

    return OAL_SUCC;
}

#else


oal_uint32  dmac_custom_init(oal_uint32 ul_psta_enable)
{

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (ul_psta_enable)
    {
        g_us_wlan_assoc_user_max_num  = 15;
        g_ul_wlan_vap_max_num_per_device = WLAN_REPEATER_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;/* 1个AP, 1个sta，15个Proxy STA，1个配置vap */
    }
    else
#endif
    {
        g_us_wlan_assoc_user_max_num  = WLAN_ASSOC_USER_MAX_NUM_LIMIT;
        g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;
    }

    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_DFT_LOG
oal_uint32 oam_exception_stat_timeout(oal_void *p_arg)
{

    oal_uint8                       uc_vap_idx;

    for (uc_vap_idx = 0; uc_vap_idx < WLAN_VAP_SUPPOTR_MAX_NUM_SPEC; uc_vap_idx++)
    {
        oam_exception_stat_handler(OM_WIFI, uc_vap_idx);
    }
    return OAL_SUCC;
}
#else
oal_uint32 oam_exception_stat_timeout(oal_void *p_arg)
{
    return OAL_SUCC;
}

#endif /* _PRE_WLAN_DFT_LOG */

oal_uint32 dmac_dfx_init(oal_void)
{
    dmac_custom_init(OAL_FALSE);
    return dmac_dft_init();
}

oal_uint32 dmac_dfx_exit(oal_void)
{
    return dmac_dft_exit();
}

#ifdef _PRE_WLAN_FEATURE_DFR

oal_uint32 dmac_dfr_init(oal_void *p_arg)
{
    dmac_device_stru    *pst_dmac_device;
    mac_device_stru     *pst_device;

    if (OAL_PTR_NULL == p_arg)
    {
        OAM_WARNING_LOG0(0, OAM_SF_TX, "odmac_dfr_init: invalid input param,p_arg is NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_device = (dmac_device_stru *)p_arg;
    pst_device = pst_dmac_device->pst_device_base_info;

    if (OAL_PTR_NULL == pst_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_TX, "odmac_dfr_init: invalid input param, mac device is NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    OAL_MEMZERO(&(pst_dmac_device->st_pcie_err_timer), OAL_SIZEOF(frw_timeout_stru));

    /* 初始化检测pcie err_nonfatal 定时器 */
    FRW_TIMER_CREATE_TIMER(&(pst_dmac_device->st_pcie_err_timer),
                               dmac_pcie_err_timeout,
                               OAL_PCIE_NONFATAL_ERR_TIMEOUT_MS,
                               pst_dmac_device,
                               OAL_TRUE,
                               OAM_MODULE_ID_DMAC,
                               pst_device->ul_core_id);

    pst_dmac_device->ul_pcie_err_cnt = 0;

#endif
    OAL_MEMZERO(&(pst_device->pst_device_stru->st_dfr_tx_prot.st_tx_prot_timer), OAL_SIZEOF(frw_timeout_stru));

    /* 初始化检测无发送完成中断定时器 */
    FRW_TIMER_CREATE_TIMER(&(pst_device->pst_device_stru->st_dfr_tx_prot.st_tx_prot_timer),
                               dmac_dfr_tx_comp_timeout_handle,
                               WLAN_TX_PROT_TIMEOUT,
                               pst_device,
                               OAL_FALSE,
                               OAM_MODULE_ID_DMAC,
                               pst_device->ul_core_id);
    FRW_TIMER_STOP_TIMER(&(pst_device->pst_device_stru->st_dfr_tx_prot.st_tx_prot_timer));

#ifdef _PRE_DEBUG_MODE
    pst_device->pst_device_stru->ul_cfg_loss_tx_comp_cnt = 0;           /* 初始化为不丢失发送完成中断 */
#endif

    pst_device->pst_device_stru->en_dfr_enable = OAL_TRUE;              /* 默认使能dfr开关 */

    return OAL_SUCC;
}


#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)


oal_uint32  dmac_pcie_err_timeout(oal_void *p_arg)
{
    dmac_device_stru    *pst_device;

    pst_device = (dmac_device_stru *)p_arg;
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_reg_timeout::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_device->ul_pcie_err_cnt >= OAL_PCIE_NONFATAL_ERR_CNT)
    {
        oal_pci_hand_reset(pst_device->pst_device_base_info->uc_chip_id);

        OAM_WARNING_LOG1(0, OAM_SF_TX, "oal_pci_hand_reset complete,chip_id = %d\r\n", pst_device->pst_device_base_info->uc_chip_id);
        OAL_IO_PRINT("oal_pci_hand_reset complete,chip_id = %d\r\n", pst_device->pst_device_base_info->uc_chip_id);
    }

    pst_device->ul_pcie_err_cnt = 0;
    return OAL_SUCC;
}

#endif


oal_uint32  dmac_dfr_tx_comp_timeout_handle(oal_void *p_arg)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_device_stru           *pst_mac_device;
    hal_to_dmac_device_stru   *pst_hal_device;
    hal_dfr_tx_prot_stru      *pst_dfr_tx_prot;
    dmac_reset_para_stru       st_reset_param;
    oal_bool_enum_uint8        en_rlst;

    pst_mac_device = (mac_device_stru *)p_arg;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*复位状态暂停调度*/
    if(OAL_TRUE == MAC_DEV_IS_RESET_IN_PROGRESS(pst_mac_device))
    {
        return OAL_FAIL;
    }

    pst_hal_device = pst_mac_device->pst_device_stru;
    pst_dfr_tx_prot = &(pst_hal_device->st_dfr_tx_prot);

    /* 如果没有打开dfr开关，则直接stop定时器后返回 */
    if (OAL_FALSE == pst_hal_device->en_dfr_enable)
    {
        OAM_WARNING_LOG0(0, OAM_SF_IRQ, "dmac_dfr_tx_comp_timeout_handle: dfr is disabled\r\n");
        FRW_TIMER_STOP_TIMER(&(pst_mac_device->pst_device_stru->st_dfr_tx_prot.st_tx_prot_timer));
        return OAL_SUCC;
    }

    OAM_WARNING_LOG1(0, OAM_SF_IRQ, "dmac_dfr_tx_comp_timeout_handle enter, cnt = %d\r\n", pst_hal_device->ul_cfg_loss_tx_comp_cnt);


    /* 如果发送完成队列还有待处理事件，说明还有发送完成中断产生，直接返回 */
    en_rlst = frw_is_event_queue_empty(FRW_EVENT_TYPE_WLAN_TX_COMP);
    if (OAL_FALSE == en_rlst)
    {
        OAM_WARNING_LOG0(0, OAM_SF_IRQ, "dmac_dfr_tx_comp_timeout_handle: tx_comp_event is not empty\r\n");
        return OAL_SUCC;
    }

    st_reset_param.uc_reset_type    = HAL_RESET_HW_TYPE_ALL;
    st_reset_param.uc_reset_mac_mod = HAL_RESET_MAC_ALL;
    st_reset_param.uc_reset_mac_reg = OAL_FALSE;
    st_reset_param.uc_reset_phy_reg = OAL_FALSE;
    st_reset_param.en_reason        = DMAC_RESET_REASON_TX_COMP_TIMEOUT;

    /* 复位 */
    dmac_reset_hw(pst_mac_device, (oal_uint8 *)&st_reset_param);
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{dmac_dfr_tx_comp_timeout_handle:: reset mac and phy done}");

    /* 重启定时器 */
    FRW_TIMER_RESTART_TIMER(&(pst_dfr_tx_prot->st_tx_prot_timer), WLAN_TX_PROT_TIMEOUT, OAL_TRUE);
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_void  dmac_get_reset_plan(hal_mac_error_type_enum_uint8 en_error_id,
                                     dmac_reset_para_stru  *pst_reset_param,
                                     oal_bool_enum_uint8   *pen_reset_immediately)
{
    /* HAL_RESET_HW_TYPE_MAC_PHY : 方案A : (MAC不自复位)复位MAC+PHY逻辑
       HAL_RESET_HW_TYPE_MAC_PHY : 方案B : (MAC可自复位)达到预定阈值，复位MAC+PHY逻辑
       HAL_RESET_HW_TYPE_MAC_PHY : 方案C : 复位MAC+PHY、逻辑+寄存器
       HAL_RESET_HW_TYPE_PHY     : 方案D : 复位PHY逻辑
       HAL_RESET_HW_TYPE_ALL     : 方案E : 复位MAC+PHY+RF、逻辑+寄存器
    */

    OAL_MEMZERO((oal_uint8 *)pst_reset_param, OAL_SIZEOF(dmac_reset_para_stru));
    switch (en_error_id)
    {
        /* 方案A */
        case HAL_MAC_ERROR_TX_ACBK_Q_OVERRUN:
        case HAL_MAC_ERROR_TX_ACBE_Q_OVERRUN:
        case HAL_MAC_ERROR_TX_ACVI_Q_OVERRUN:
        case HAL_MAC_ERROR_TX_ACVO_Q_OVERRUN:
        case HAL_MAC_ERROR_TX_HIPRI_Q_OVERRUN:
        case HAL_MAC_ERROR_BEACON_MISS:
            pst_reset_param->uc_reset_type    = HAL_RESET_HW_TYPE_MAC_PHY;
            pst_reset_param->uc_reset_mac_reg = OAL_FALSE;
            pst_reset_param->uc_reset_phy_reg = OAL_FALSE;
            pst_reset_param->uc_reset_mac_mod = HAL_RESET_MAC_ALL;
            pst_reset_param->en_reason = DMAC_RESET_REASON_HW_ERR;
            *pen_reset_immediately     = OAL_TRUE;
            break;

        /* 方案B */
        case HAL_MAC_ERROR_PHY_RX_FIFO_OVERRUN:
        case HAL_MAC_ERROR_TX_DATAFLOW_BREAK:
        case HAL_MAC_ERROR_RX_FSM_ST_TIMEOUT:
        case HAL_MAC_ERROR_TX_FSM_ST_TIMEOUT:
        case HAL_MAC_ERROR_RX_HANDLER_ST_TIMEOUT:
        case HAL_MAC_ERROR_TX_HANDLER_ST_TIMEOUT:
        case HAL_MAC_ERROR_BUS_RLEN_ERR:
        case HAL_MAC_ERROR_BUS_RADDR_ERR:
        case HAL_MAC_ERROR_BUS_WLEN_ERR:
        case HAL_MAC_ERROR_BUS_WADDR_ERR:
        case HAL_MAC_ERROR_MATRIX_CALC_TIMEOUT:
        case HAL_MAC_ERROR_RX_OVERLAP_ERR:
            pst_reset_param->uc_reset_type    = HAL_RESET_HW_TYPE_MAC_PHY;
            pst_reset_param->uc_reset_mac_reg = OAL_FALSE;
            pst_reset_param->uc_reset_phy_reg = OAL_FALSE;
            pst_reset_param->uc_reset_mac_mod = HAL_RESET_MAC_ALL;
            pst_reset_param->en_reason = DMAC_RESET_REASON_HW_ERR;
            *pen_reset_immediately     = OAL_FALSE;
            break;

        /* 方案D */
        case HAL_MAC_ERROR_PHY_TRLR_TIME_OUT:
            pst_reset_param->uc_reset_type    = HAL_RESET_HW_TYPE_PHY;
            pst_reset_param->uc_reset_mac_reg = OAL_FALSE;
            pst_reset_param->uc_reset_phy_reg = OAL_FALSE;
            pst_reset_param->uc_reset_mac_mod = HAL_RESET_MAC_ALL;
            pst_reset_param->en_reason = DMAC_RESET_REASON_HW_ERR;
            *pen_reset_immediately     = OAL_FALSE;
            break;

        default:
            pst_reset_param->uc_reset_type    = HAL_RESET_HW_TYPE_BUTT;
            pst_reset_param->uc_reset_mac_reg = OAL_FALSE;
            pst_reset_param->uc_reset_phy_reg = OAL_FALSE;
            pst_reset_param->uc_reset_mac_mod = HAL_RESET_MAC_BUTT;
            pst_reset_param->en_reason = DMAC_RESET_REASON_HW_ERR;
            *pen_reset_immediately     = OAL_FALSE;
            break;
    }

}


oal_uint32  dmac_dfr_process_mac_error(
                mac_device_stru                 *pst_mac_device,
                hal_mac_error_type_enum_uint8    en_error_id,
                oal_uint32     ul_error1_irq_state,
                oal_uint32     ul_error2_irq_state)
{
    hal_dfr_err_opern_stru    *pst_dft_err;
    hal_to_dmac_device_stru   *pst_hal_device;
    dmac_reset_para_stru       st_reset_param;
    oal_bool_enum_uint8        en_reset_immediately;
    hal_error_state_stru       st_error_state;
    dmac_device_stru          *pst_dmac_device;
#ifdef _PRE_WLAN_DFT_STAT
    dmac_vap_stru             *pst_dmac_sta;
#endif
    /* 入参检查 */
    if (OAL_UNLIKELY(en_error_id >= HAL_MAC_ERROR_TYPE_BUTT))
    {
        OAM_ERROR_LOG1(0, OAM_SF_DFR, "{dmac_dfr_process_mac_error::error_id[%d] OVERFLOW!.}", en_error_id);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_dmac_device = (dmac_device_stru  *)dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (OAL_PTR_NULL == pst_dmac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_DFR, "{dmac_dfr_process_mac_error::Can not find dmac_dev by mac_dev_id[%d]!.}", pst_mac_device->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == pst_dmac_device->en_dfr_hw_reset_enable)
    {
        return OAL_SUCC;
    }

    pst_hal_device = pst_mac_device->pst_device_stru;
    pst_dft_err = &(pst_hal_device->st_dfr_err_opern[en_error_id]);

    /* 错误中断计数加1 */
    pst_dft_err->us_err_cnt++;

    /* 获取复位方案 */
    dmac_get_reset_plan(en_error_id, &st_reset_param, &en_reset_immediately);
    if (HAL_RESET_HW_TYPE_BUTT <= st_reset_param.uc_reset_type)
    {
        return OAL_SUCC;
    }

    /* 在连续的10次TBTT中断中(每10次TBTT中断对该计数清零)，
       错误中断产生的次数没有达到需要处理的阈值，直接返回 */
    if ((pst_dft_err->us_err_cnt < HAL_MAC_ERROR_THRESHOLD) && (OAL_TRUE != en_reset_immediately))
    {
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_DFT_STAT
    pst_dmac_sta = mac_res_get_dmac_vap(pst_mac_device->uc_cfg_vap_id);
    if (OAL_PTR_NULL == pst_dmac_sta)
    {
        OAM_ERROR_LOG1(0, OAM_SF_DFR, "{dmac_dfr_process_mac_error::Cannot find vap[%d] by dev!}", pst_mac_device->uc_cfg_vap_id);
    }
    else
    {
        /* MAC&PHY&RF寄存器上报 */
        dmac_dft_report_all_ota_state(&pst_dmac_sta->st_vap_base_info);
    }
#endif

    st_error_state.ul_error1_val = 0xFFFFFFFF;
    st_error_state.ul_error2_val = 0xFFFFFFFF;

    /* 清中断 */
    hal_clear_mac_error_int_status(pst_hal_device, &st_error_state);

    /* 清中断状态 */
    hal_clear_mac_int_status(pst_hal_device, (1 << DMAC_PA_ERROR_OFFSET));

    /* unmask error stauts */
    st_error_state.ul_error1_val = ul_error1_irq_state;
    st_error_state.ul_error2_val = ul_error2_irq_state & 0xffffffdf;
    hal_unmask_mac_error_init_status(pst_hal_device, &st_error_state);

    /* unmask mac stauts */
    hal_unmask_mac_init_status(pst_hal_device, (1 << DMAC_PA_ERROR_OFFSET));

    OAM_WARNING_LOG2(0, OAM_SF_DFR, "{dmac_dfr_process_mac_error::There are too many errors[id:%d, cnt:%d], so we have to reset the HW!.}",
                    en_error_id, pst_dft_err->us_err_cnt);

    /* 复位，清除所有异常统计，重新开始计数 */
    pst_dft_err->us_err_cnt = 0;

    /* 开始复位 */
    dmac_reset_hw(pst_mac_device, (oal_uint8 *)&st_reset_param);
    return OAL_SUCC;
}
#else

oal_uint32  dmac_dfr_process_mac_error(
                mac_device_stru                 *pst_mac_device,
                hal_mac_error_type_enum_uint8    en_error_id,
                oal_uint32     ul_error1_irq_state,
                oal_uint32     ul_error2_irq_state)
{
    return OAL_SUCC;
}

#endif //_PRE_WLAN_FEATURE_DFR


/*TBD，待修订为同步后上库*/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

#else
/*lint -e19*/
oal_module_symbol(g_us_wlan_assoc_user_max_num);
oal_module_symbol(g_ul_wlan_vap_max_num_per_device);
#endif
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

