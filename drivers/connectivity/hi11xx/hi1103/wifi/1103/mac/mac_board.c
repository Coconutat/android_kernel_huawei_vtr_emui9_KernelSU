



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_spec.h"
#include "mac_board.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_BOARD_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/* HOST CRX子表 */
frw_event_sub_table_item_stru g_ast_dmac_host_crx_table_etc[HMAC_TO_DMAC_SYN_BUTT];

/* DMAC模块，HOST_DRX事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_dmac_tx_host_drx_etc[DMAC_TX_HOST_DRX_BUTT];

/* DMAC模块，WLAN_DTX事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_dmac_tx_wlan_dtx_etc[DMAC_TX_WLAN_DTX_BUTT];

#ifndef _PRE_WLAN_PROFLING_MIPS
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rwdata = "BTCM", code ="ATCM", zidata = "BTCM", rodata = "ATCM"
#endif
#endif
/* DMAC模块，WLAN_CTX事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_dmac_wlan_ctx_event_sub_table_etc[DMAC_WLAN_CTX_EVENT_SUB_TYPE_BUTT];

/* DMAC模块,WLAN_DRX事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_dmac_wlan_drx_event_sub_table_etc[HAL_WLAN_DRX_EVENT_SUB_TYPE_BUTT];

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
/* DMAC模块，high prio事件处理函数注册结构体定义 */
frw_event_sub_table_item_stru g_ast_dmac_high_prio_event_sub_table_etc[HAL_EVENT_DMAC_HIGH_PRIO_SUB_TYPE_BUTT];
#else
/* DMAC模块，ERROR_IRQ事件处理函数注册结构体定义 */
frw_event_sub_table_item_stru g_ast_dmac_high_prio_event_sub_table_etc[HAL_EVENT_ERROR_IRQ_SUB_TYPE_BUTT];
#endif

/* DMAC模块,WLAN_CRX事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_dmac_wlan_crx_event_sub_table_etc[HAL_WLAN_CRX_EVENT_SUB_TYPE_BUTT];

/* DMAC模块，TX_COMP事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_dmac_tx_comp_event_sub_table_etc[HAL_TX_COMP_SUB_TYPE_BUTT];

/* DMAC模块, TBTT事件处理函数表定义 */
frw_event_sub_table_item_stru g_ast_dmac_tbtt_event_sub_table_etc[HAL_EVENT_TBTT_SUB_TYPE_BUTT];

/*DMAC模块, MISC事件处理函数表定义 */
frw_event_sub_table_item_stru g_ast_dmac_misc_event_sub_table_etc[HAL_EVENT_DMAC_MISC_SUB_TYPE_BUTT];

/* WLAN_DTX 事件子类型表 */
frw_event_sub_table_item_stru g_ast_hmac_wlan_dtx_event_sub_table_etc[DMAC_TX_WLAN_DTX_BUTT];

/* HMAC模块 WLAN_DRX事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_hmac_wlan_drx_event_sub_table_etc[DMAC_WLAN_DRX_EVENT_SUB_TYPE_BUTT];

/* HMAC模块 WLAN_CRX事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_hmac_wlan_crx_event_sub_table_etc[DMAC_WLAN_CRX_EVENT_SUB_TYPE_BUTT];

/* HMAC模块 TBTT事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_hmac_tbtt_event_sub_table_etc[DMAC_TBTT_EVENT_SUB_TYPE_BUTT];

/* HMAC模块 发向HOST侧的配置事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_hmac_wlan_ctx_event_sub_table_etc[DMAC_TO_HMAC_SYN_BUTT];

/* HMAC模块 MISC杂散事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_hmac_wlan_misc_event_sub_table_etc[DMAC_MISC_SUB_TYPE_BUTT];

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
/* HMAC模块 IPC事件处理函数注册结构定义 */
frw_event_sub_table_item_stru g_ast_hmac_host_drx_event_sub_table[DMAC_TX_HOST_DRX_BUTT];
#endif


#ifndef _PRE_WLAN_PROFLING_MIPS
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)  && defined (__CC_ARM)
#pragma arm section rodata, code, rwdata, zidata  // return to default placement
#endif
#endif



/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_void  event_fsm_table_register_etc(oal_void)
{
    /* Part1: 以下是Dmac收的事件*/

    /* 注册DMAC模块HOST_CRX事件 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_HOST_CRX, FRW_EVENT_PIPELINE_STAGE_1, g_ast_dmac_host_crx_table_etc);

    /* 注册DMAC模块HOST_DRX事件处理函数表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_HOST_DRX, FRW_EVENT_PIPELINE_STAGE_1, g_ast_dmac_tx_host_drx_etc);

    /* 注册DMAC模块WLAN_DTX事件处理函数表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_WLAN_DTX, FRW_EVENT_PIPELINE_STAGE_1, g_ast_dmac_tx_wlan_dtx_etc);

    /* 注册DMAC模块WLAN_CTX事件处理函数表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_WLAN_CTX, FRW_EVENT_PIPELINE_STAGE_1, g_ast_dmac_wlan_ctx_event_sub_table_etc);

    /* 注册DMAC模块WLAN_DRX事件子表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_WLAN_DRX, FRW_EVENT_PIPELINE_STAGE_0, g_ast_dmac_wlan_drx_event_sub_table_etc);

    /* 注册DMAC模块WLAN_CRX事件pipeline 0子表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_WLAN_CRX, FRW_EVENT_PIPELINE_STAGE_0, g_ast_dmac_wlan_crx_event_sub_table_etc);

    /* 注册DMAC模块TX_COMP事件子表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_WLAN_TX_COMP, FRW_EVENT_PIPELINE_STAGE_0, g_ast_dmac_tx_comp_event_sub_table_etc);

    /* 注册DMAC模块TBTT事件字表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_TBTT, FRW_EVENT_PIPELINE_STAGE_0, g_ast_dmac_tbtt_event_sub_table_etc);

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
    /* 注册DMAC模块实时事件子表 */
#else
    /* 注册DMAC模块ERR事件子表 */
#endif
    frw_event_table_register_etc(FRW_EVENT_TYPE_HIGH_PRIO, FRW_EVENT_PIPELINE_STAGE_0, g_ast_dmac_high_prio_event_sub_table_etc);

    /* 注册DMAC模块MISC事件字表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_DMAC_MISC, FRW_EVENT_PIPELINE_STAGE_0, g_ast_dmac_misc_event_sub_table_etc);
    /* Part2: 以下是Hmac收的事件*/

    /* 注册WLAN_DTX事件子表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_WLAN_DTX, FRW_EVENT_PIPELINE_STAGE_0, g_ast_hmac_wlan_dtx_event_sub_table_etc);

    /* 注册WLAN_DRX事件子表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_WLAN_DRX, FRW_EVENT_PIPELINE_STAGE_1, g_ast_hmac_wlan_drx_event_sub_table_etc);

    /* 注册HMAC模块WLAN_CRX事件子表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_WLAN_CRX, FRW_EVENT_PIPELINE_STAGE_1, g_ast_hmac_wlan_crx_event_sub_table_etc);

     /* 注册DMAC模块MISC事件字表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_DMAC_MISC, FRW_EVENT_PIPELINE_STAGE_1, g_ast_hmac_wlan_misc_event_sub_table_etc);

    /* 注册TBTT事件子表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_TBTT, FRW_EVENT_PIPELINE_STAGE_1, g_ast_hmac_tbtt_event_sub_table_etc);

    /* 注册统计结果查询事件子表 */
    frw_event_table_register_etc(FRW_EVENT_TYPE_HOST_SDT_REG, FRW_EVENT_PIPELINE_STAGE_1, g_ast_hmac_wlan_ctx_event_sub_table_etc);
}


oal_void  event_fsm_unregister_etc(oal_void)
{
    /* Part1: 以下是Dmac侧的事件*/

    /* 去注册DMAC模块HOST_CRX事件 */
    OAL_MEMZERO(g_ast_dmac_host_crx_table_etc, OAL_SIZEOF(g_ast_dmac_host_crx_table_etc));

    /* 去注册DMAC模块HOST_DRX事件处理函数表 */
    OAL_MEMZERO(g_ast_dmac_tx_host_drx_etc, OAL_SIZEOF(g_ast_dmac_tx_host_drx_etc));

    /* 去注册DMAC模块WLAN_DTX事件处理函数表 */
    OAL_MEMZERO(g_ast_dmac_tx_wlan_dtx_etc, OAL_SIZEOF(g_ast_dmac_tx_wlan_dtx_etc));

    /* 去注册DMAC模块WLAN_DRX事件子表 */
    OAL_MEMZERO(g_ast_dmac_wlan_drx_event_sub_table_etc, OAL_SIZEOF(g_ast_dmac_wlan_drx_event_sub_table_etc));

    /* 去注册DMAC模块WLAN_CRX事件子表 */
    OAL_MEMZERO(g_ast_dmac_wlan_crx_event_sub_table_etc, OAL_SIZEOF(g_ast_dmac_wlan_crx_event_sub_table_etc));

    /* 去注册DMAC模块TX_COMP事件子表 */
    OAL_MEMZERO(g_ast_dmac_tx_comp_event_sub_table_etc, OAL_SIZEOF(g_ast_dmac_tx_comp_event_sub_table_etc));

    /* 去注册DMAC模块TBTT事件字表 */
    OAL_MEMZERO(g_ast_dmac_tbtt_event_sub_table_etc, OAL_SIZEOF(g_ast_dmac_tbtt_event_sub_table_etc));

#if ((_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
    /* 去注册DMAC模块实时事件子表 */
#else
    /* 去注册DMAC模块ERR事件子表 */
#endif
    OAL_MEMZERO(g_ast_dmac_high_prio_event_sub_table_etc, OAL_SIZEOF(g_ast_dmac_high_prio_event_sub_table_etc));

    /* 去注册DMAC模块杂散事件子表 */
    OAL_MEMZERO(g_ast_dmac_misc_event_sub_table_etc, OAL_SIZEOF(g_ast_dmac_misc_event_sub_table_etc));

    /* Part2: 以下是Hmac侧的事件*/
    OAL_MEMZERO(g_ast_hmac_wlan_dtx_event_sub_table_etc, OAL_SIZEOF(g_ast_hmac_wlan_dtx_event_sub_table_etc));

    OAL_MEMZERO(g_ast_hmac_wlan_drx_event_sub_table_etc, OAL_SIZEOF(g_ast_hmac_wlan_drx_event_sub_table_etc));

    OAL_MEMZERO(g_ast_hmac_wlan_crx_event_sub_table_etc, OAL_SIZEOF(g_ast_hmac_wlan_crx_event_sub_table_etc));

    OAL_MEMZERO(g_ast_hmac_tbtt_event_sub_table_etc, OAL_SIZEOF(g_ast_hmac_tbtt_event_sub_table_etc));

    OAL_MEMZERO(g_ast_hmac_wlan_ctx_event_sub_table_etc, OAL_SIZEOF(g_ast_hmac_wlan_ctx_event_sub_table_etc));

    OAL_MEMZERO(g_ast_hmac_wlan_misc_event_sub_table_etc, OAL_SIZEOF(g_ast_hmac_wlan_misc_event_sub_table_etc));

}



/*lint -e19*/
oal_module_symbol(g_ast_dmac_host_crx_table_etc);
oal_module_symbol(g_ast_dmac_tx_host_drx_etc);
oal_module_symbol(g_ast_dmac_tx_wlan_dtx_etc);
oal_module_symbol(g_ast_dmac_wlan_ctx_event_sub_table_etc);
oal_module_symbol(g_ast_dmac_wlan_drx_event_sub_table_etc);
oal_module_symbol(g_ast_dmac_high_prio_event_sub_table_etc);
oal_module_symbol(g_ast_dmac_wlan_crx_event_sub_table_etc);
oal_module_symbol(g_ast_dmac_tx_comp_event_sub_table_etc);
oal_module_symbol(g_ast_dmac_tbtt_event_sub_table_etc);
oal_module_symbol(g_ast_dmac_misc_event_sub_table_etc);

oal_module_symbol(g_ast_hmac_wlan_dtx_event_sub_table_etc);
oal_module_symbol(g_ast_hmac_wlan_drx_event_sub_table_etc);
oal_module_symbol(g_ast_hmac_wlan_crx_event_sub_table_etc);
oal_module_symbol(g_ast_hmac_tbtt_event_sub_table_etc);
oal_module_symbol(g_ast_hmac_wlan_ctx_event_sub_table_etc);
oal_module_symbol(g_ast_hmac_wlan_misc_event_sub_table_etc);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
oal_module_symbol(g_ast_hmac_host_drx_event_sub_table);
#endif


oal_module_symbol(event_fsm_table_register_etc);
oal_module_symbol(event_fsm_unregister_etc);

/*lint +e19*/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


