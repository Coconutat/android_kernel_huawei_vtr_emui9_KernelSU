

#ifndef __FRW_EVENT_MAIN_H__
#define __FRW_EVENT_MAIN_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "frw_main.h"
#include "frw_event_sched.h"
#include "wlan_types.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_EVENT_MAIN_H


/*****************************************************************************
  2 枚举定义
*****************************************************************************/
/*****************************************************************************
  枚举名  : frw_event_deploy_enum_uint8
  协议表格:
  枚举说明: 事件部署类型
*****************************************************************************/
typedef enum
{
    FRW_EVENT_DEPLOY_NON_IPC  = 0,    /* 非核间通信 */
    FRW_EVENT_DEPLOY_IPC,             /* 核间通讯 */

    FRW_EVENT_DEPLOY_BUTT
}frw_event_deploy_enum;
typedef oal_uint8 frw_event_deploy_enum_uint8;

/*****************************************************************************
  8 宏定义
*****************************************************************************/
/* 事件表的最大个数 */
/* 一种类型的事件对应两个表项，所以事件表的大小为事件类型的2倍 */
#define FRW_EVENT_TABLE_MAX_ITEMS    (FRW_EVENT_TYPE_BUTT * 2)

/*****************************************************************************
  3 全局变量声明
*****************************************************************************/
#ifdef _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
typedef struct _frw_event_segment_stru_
{
    frw_event_type_enum_uint8        en_type;         /* 事件类型 */
    oal_uint8                        uc_sub_type;     /* 事件子类型 */
    frw_event_pipeline_enum_uint8    en_pipeline;     /* 事件分段号 */
    oal_uint8                        uc_vap_id;       /* VAP ID */
}frw_event_segment_stru;

typedef struct _frw_event_trace_item_stru_
{
    frw_event_segment_stru st_event_seg;
    oal_uint64  timestamp;
}frw_event_trace_item_stru;

#define CONFIG_FRW_MAX_TRACE_EVENT_NUMS (50)
typedef struct _frw_event_trace_stru_
{
    frw_event_trace_item_stru st_trace_item[CONFIG_FRW_MAX_TRACE_EVENT_NUMS];
    oal_uint32  ul_current_pos;
    oal_uint32  ul_over_flag;
    const char*       pst_func_name;
    oal_int32   line_num;
    //oal_void*   pst_last_ip;/*last pc address*/
}frw_event_trace_stru;
#endif
/*****************************************************************************
  结构名  : frw_event_mgmt_stru
  结构说明: 事件管理结构体
*****************************************************************************/
typedef struct _frw_event_mgmt_stru_
{
    frw_event_queue_stru          st_event_queue[FRW_EVENT_MAX_NUM_QUEUES];    /* 事件队列 */
    frw_event_sched_queue_stru    st_sched_queue[FRW_SCHED_POLICY_BUTT];         /* 可调度队列 */
#ifdef  _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
    frw_event_trace_stru          *pst_frw_trace;
#endif
}frw_event_mgmt_stru;
extern frw_event_table_item_stru g_ast_event_table_etc[FRW_EVENT_TABLE_MAX_ITEMS];

extern frw_event_mgmt_stru g_ast_event_manager_etc[WLAN_FRW_MAX_NUM_CORES];

/*****************************************************************************
  4 消息头定义
*****************************************************************************/


/*****************************************************************************
  5 消息定义
*****************************************************************************/


/*****************************************************************************
  6 STRUCT定义
*****************************************************************************/



/*****************************************************************************
  7 UNION定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  frw_event_init_etc(oal_void);
extern oal_uint32  frw_event_exit_etc(oal_void);
extern oal_uint32  frw_event_queue_enqueue_etc(frw_event_queue_stru *pst_event_queue, frw_event_mem_stru *pst_event_mem);
extern frw_event_mem_stru *frw_event_queue_dequeue_etc(frw_event_queue_stru *pst_event_queue);
extern oal_uint32  frw_event_post_event_etc(frw_event_mem_stru *pst_event_mem, oal_uint32 ul_core_id);
extern oal_void  frw_event_deploy_register_etc(oal_uint32 (*p_func)(frw_event_mem_stru *pst_event_mem, oal_uint8 *puc_deploy_result));
extern oal_void  frw_event_ipc_event_queue_full_register_etc(oal_uint32 (*p_func)(oal_void));
extern oal_void  frw_event_ipc_event_queue_empty_register_etc(oal_uint32 (*p_func)(oal_void));
extern frw_event_sched_queue_stru* frw_event_get_sched_queue_etc(oal_uint32 ul_core_id, frw_sched_policy_enum_uint8 en_policy);
extern oal_void  frw_event_vap_pause_event_etc(oal_uint8 uc_vap_id);
extern oal_void  frw_event_vap_resume_event_etc(oal_uint8 uc_vap_id);
extern oal_uint32  frw_event_vap_flush_event_etc(oal_uint8 uc_vap_id, frw_event_type_enum_uint8 en_event_type, oal_bool_enum_uint8 en_drop);
//extern oal_uint32  frw_event_lookup_process_entry(frw_event_mem_stru *pst_event_mem, frw_event_hdr_stru *pst_event_hrd);
extern oal_uint32  frw_event_queue_info_etc(oal_void);


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


OAL_STATIC OAL_INLINE oal_uint32  frw_event_to_qid(frw_event_mem_stru *pst_event_mem, oal_uint16 *pus_qid)
{
    oal_uint16            us_qid;
    frw_event_hdr_stru   *pst_event_hrd;

    /* 获取事件头结构 */
    pst_event_hrd = (frw_event_hdr_stru *)pst_event_mem->puc_data;

    us_qid        = pst_event_hrd->uc_vap_id * FRW_EVENT_TYPE_BUTT + pst_event_hrd->en_type;

  /*
    us_qid = (pst_event_hrd->uc_chip_id * (WLAN_DEVICE_MAX_NUM_PER_CHIP * WLAN_VAP_MAX_NUM_PER_DEVICE) +
              pst_event_hrd->uc_device_id * (WLAN_VAP_MAX_NUM_PER_DEVICE) + pst_event_hrd->uc_vap_id) *
              FRW_EVENT_TYPE_BUTT + pst_event_hrd->en_type;
  */

    /* 异常: 队列ID超过最大值 */
    if ((us_qid >= FRW_EVENT_MAX_NUM_QUEUES))
    {
        OAM_ERROR_LOG4(0, OAM_SF_FRW, "{frw_event_to_qid, array overflow! us_qid[%d], vap_id[%d], en_type[%d], sub_type[%d]}",
                                        us_qid, pst_event_hrd->uc_vap_id, pst_event_hrd->en_type, pst_event_hrd->uc_sub_type);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    *pus_qid = us_qid;

    return OAL_SUCC;
}



OAL_STATIC OAL_INLINE oal_uint32  _frw_event_lookup_process_entry_(frw_event_mem_stru *pst_event_mem, frw_event_hdr_stru *pst_event_hrd)
{
    oal_uint8                    uc_index;
    oal_uint8                    uc_sub_type;
    frw_event_table_item_stru   *pst_frw_event_table;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    frw_event_mem_stru          *pst_tmp_event_mem;
    oal_uint32                   ul_ret = 0;
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    oal_bool_enum                b_is_rx_adapt = OAL_FALSE;
#endif
#endif

    uc_sub_type = pst_event_hrd->uc_sub_type;

#if (((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE))

    /*UT环境下打桩实现测试*/
    if (FRW_EVENT_PIPELINE_STAGE_2 == pst_event_hrd->en_pipeline)
    {
        pst_event_hrd->en_pipeline = FRW_EVENT_PIPELINE_STAGE_1;
        b_is_rx_adapt = OAL_TRUE;
    }
#endif

    /* 根据事件类型及分段号计算事件表索引 */
    uc_index = (oal_uint8)((pst_event_hrd->en_type << 1) | (pst_event_hrd->en_pipeline & 0x01));
    if (OAL_UNLIKELY(uc_index >= FRW_EVENT_TABLE_MAX_ITEMS))
    {
        OAM_ERROR_LOG3(0, OAM_SF_FRW, "{frw_event_lookup_process_entry::array overflow! type[0x%x], sub_type[0x%x], pipeline[0x%x]}",
                       pst_event_hrd->en_type, uc_sub_type, pst_event_hrd->en_pipeline);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

#if (((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE))
    if (OAL_TRUE == b_is_rx_adapt)
    {
        pst_event_hrd->en_pipeline = FRW_EVENT_PIPELINE_STAGE_2;
    }
#endif

    /* 先把全局变量变成局部变量 */
    pst_frw_event_table = &g_ast_event_table_etc[uc_index];

    if (OAL_PTR_NULL == pst_frw_event_table->pst_sub_table)
    {
        OAM_ERROR_LOG2(0, OAM_SF_FRW, "{frw_event_lookup_process_entry::pst_sub_table is NULL! sub_type[0x%x], index[0x%x].}",
                       uc_sub_type, uc_index);

        /* 打印当前事件内存的部分内容 */
        oal_print_hex_dump(frw_get_event_data(pst_event_mem), (oal_int32)(FRW_EVENT_HDR_LEN + 20), 32, "current event mem info:");

        /* 打印当前事件内存的前64字节内容 */
        oal_print_hex_dump(frw_get_event_data(pst_event_mem) + 64, 64, 32, "64bytes mem info in front of this mem:");

        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    if (OAL_PTR_NULL == pst_frw_event_table->pst_sub_table[uc_sub_type].p_func)
    {
        OAM_ERROR_LOG2(0, OAM_SF_FRW, "{frw_event_lookup_process_entry::pst_sub_table.p_func is NULL! sub_type[0x%x], index[0x%x].}",
                       uc_sub_type, uc_index);
        return OAL_ERR_CODE_PTR_NULL;
    }
    #if (_PRE_TEST_MODE_UT == _PRE_TEST_MODE)
    pst_frw_event_table->pst_sub_table[uc_sub_type].p_func(pst_event_mem);
    return OAL_SUCC;
    #else
    /* pipeline=1的抛事件处理，需要增加返回值，同时frw_event_dispatch_event调用时也要增加返回值判断，做正确处理 */
    return pst_frw_event_table->pst_sub_table[uc_sub_type].p_func(pst_event_mem);
    #endif
#else /* 1102 */
    /*For pipeline 0 type*/
    if (0 == pst_event_hrd->en_pipeline)
    {
        if (OAL_PTR_NULL != pst_frw_event_table->pst_sub_table[uc_sub_type].p_func)
        {
            pst_frw_event_table->pst_sub_table[uc_sub_type].p_func(pst_event_mem);
            return OAL_SUCC;
        }
        else
        {
            OAM_ERROR_LOG2(0, OAM_SF_FRW, "{frw_event_lookup_process_entry::pst_sub_table.p_func is NULL! sub_type[0x%x], index[0x%x], pipeline=0.}",
                           uc_sub_type, uc_index);
            return OAL_ERR_CODE_PTR_NULL;
        }
    }

    /*For pipeline 1 type*/
#if (((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT))
    //在51单板上打桩实现tx adapt和rx adapt的测试，后续该代码删除
    if ((OAL_PTR_NULL == pst_frw_event_table->pst_sub_table[uc_sub_type].p_tx_adapt_func)
            && (OAL_PTR_NULL == pst_frw_event_table->pst_sub_table[uc_sub_type].p_rx_adapt_func))
    {
        if (OAL_PTR_NULL == pst_frw_event_table->pst_sub_table[uc_sub_type].p_func)
        {
            OAM_ERROR_LOG4(0, OAM_SF_FRW, "{frw_event_lookup_process_entry::sub type pointer is NULL! sub_type[%d], index[%d].en_type[%d],en_pipeline[%d]}",
                           uc_sub_type, uc_index, pst_event_hrd->en_type, pst_event_hrd->en_pipeline);
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_frw_event_table->pst_sub_table[uc_sub_type].p_func(pst_event_mem);

        return OAL_SUCC;

    }

    if (1 == pst_event_hrd->en_pipeline)
    {
        if (OAL_SUCC != pst_frw_event_table->pst_sub_table[uc_sub_type].p_tx_adapt_func(pst_event_mem))
        {
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }
    }
    else if (2 == pst_event_hrd->en_pipeline)
    {
        /*rx adapt first*/
        pst_tmp_event_mem = pst_frw_event_table->pst_sub_table[uc_sub_type].p_rx_adapt_func(pst_event_mem);
        if ((OAL_PTR_NULL != pst_tmp_event_mem) && (OAL_PTR_NULL != pst_frw_event_table->pst_sub_table[uc_sub_type].p_func))
        {
            /*then call action frame*/
            pst_frw_event_table->pst_sub_table[uc_sub_type].p_func(pst_tmp_event_mem);
            FRW_EVENT_FREE(pst_tmp_event_mem);
        }
        else
        {
            OAM_ERROR_LOG2(0, OAM_SF_FRW, "{frw_event_lookup_process_entry::rx adapt prcocess failed! sub_type[0x%x], index[0x%x].}",
                          uc_sub_type, uc_index);
            if(OAL_PTR_NULL != pst_tmp_event_mem)
            {
                FRW_EVENT_FREE(pst_tmp_event_mem);
            }
            return OAL_ERR_CODE_PTR_NULL;
        }
    }

#else // (((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT))
    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_frw_event_table->pst_sub_table[uc_sub_type].p_tx_adapt_func)
            && (OAL_PTR_NULL == pst_frw_event_table->pst_sub_table[uc_sub_type].p_rx_adapt_func)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_FRW, "{frw_event_lookup_process_entry::tx and rx adapt pointer all NULL! sub_type[0x%x], index[0x%x].}",
                       uc_sub_type, uc_index);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*For tx adapt*/
    if (OAL_PTR_NULL != pst_frw_event_table->pst_sub_table[uc_sub_type].p_tx_adapt_func)
    {
        ul_ret = pst_frw_event_table->pst_sub_table[uc_sub_type].p_tx_adapt_func(pst_event_mem);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG3(0, OAM_SF_FRW, "{_frw_event_lookup_process_entry_::tx adapt process failed!sub_type[0x%x],index[0x%x],ret[%d].}",
                           uc_sub_type, uc_index, ul_ret);
            return ul_ret;
        }

        return OAL_SUCC;
    }

    /*For rx adapt*/
    if (OAL_PTR_NULL != pst_frw_event_table->pst_sub_table[uc_sub_type].p_rx_adapt_func)
    {
        /*rx adapt first*/
        pst_tmp_event_mem = pst_frw_event_table->pst_sub_table[uc_sub_type].p_rx_adapt_func(pst_event_mem);
        if ((OAL_PTR_NULL != pst_tmp_event_mem) && (OAL_PTR_NULL != pst_frw_event_table->pst_sub_table[uc_sub_type].p_func))
        {
            /*then call action frame*/
            pst_frw_event_table->pst_sub_table[uc_sub_type].p_func(pst_tmp_event_mem);
            FRW_EVENT_FREE(pst_tmp_event_mem);
        }
        else
        {
            OAM_ERROR_LOG4(0, OAM_SF_FRW, "{frw_event_lookup_process_entry::rx adapt process failed! sub_type[0x%x], index[0x%x].[%d][%d]}",
                           uc_sub_type, uc_index,pst_tmp_event_mem,pst_frw_event_table->pst_sub_table[uc_sub_type].p_func);

            if(OAL_PTR_NULL != pst_tmp_event_mem)
            {
                FRW_EVENT_FREE(pst_tmp_event_mem);
            }
            return OAL_ERR_CODE_PTR_NULL;
        }
    }

#endif
    return OAL_SUCC;
#endif
}

OAL_STATIC OAL_INLINE oal_uint32  frw_event_lookup_process_entry(frw_event_mem_stru *pst_event_mem, frw_event_hdr_stru *pst_event_hrd)
{
    oal_uint32 ul_ret;
    ul_ret = _frw_event_lookup_process_entry_(pst_event_mem,pst_event_hrd);
    return ul_ret;
}



OAL_STATIC OAL_INLINE oal_void  frw_event_report(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru      *pst_event;
    oal_uint8            auc_event[OAM_EVENT_INFO_MAX_LEN] = {0};
    pst_event = frw_get_event_stru(pst_event_mem);


    /* 复制事件头 */
    oal_memcopy((oal_void *)auc_event, (const oal_void *)&pst_event->st_event_hdr, OAL_SIZEOF(frw_event_hdr_stru));

    FRW_EVENT_INTERNAL(BROADCAST_MACADDR, 0, OAM_EVENT_INTERNAL, auc_event);
}


OAL_STATIC OAL_INLINE oal_uint32  frw_event_process(frw_event_mem_stru *pst_event_mem)
{
    frw_event_hdr_stru   *pst_event_hrd;
    oal_uint32            ul_core_id;

    /* 获取事件头结构 */
    pst_event_hrd  = (frw_event_hdr_stru *)pst_event_mem->puc_data;

    if (OAL_UNLIKELY(pst_event_hrd->en_pipeline >= FRW_EVENT_PIPELINE_STAGE_BUTT))
    {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    /* 如果pipleline为0，则将事件入队。否则，
       根据事件类型，子类型以及分段号，执行相应的事件处理函数 */
    if (FRW_EVENT_PIPELINE_STAGE_0 == pst_event_hrd->en_pipeline)
    {
        ul_core_id = OAL_GET_CORE_ID();
        return frw_event_post_event_etc(pst_event_mem, ul_core_id);
    }

    return frw_event_lookup_process_entry(pst_event_mem, pst_event_hrd);
}

#ifdef  _PRE_FRW_EVENT_PROCESS_TRACE_DEBUG
OAL_STATIC OAL_INLINE oal_void frw_event_last_pc_trace(const char* pst_func_name,oal_int32 line_num, oal_uint32 ul_core_id)
{
    if(OAL_WARN_ON(ul_core_id >= WLAN_FRW_MAX_NUM_CORES))
    {
           OAM_ERROR_LOG2(0, OAM_SF_FRW, "{frw_event_last_pc_trace: ul_core_id:%d beyond limit:%d}",ul_core_id,WLAN_FRW_MAX_NUM_CORES);
           return;
    };
    g_ast_event_manager_etc[ul_core_id].pst_frw_trace->pst_func_name = pst_func_name;
    g_ast_event_manager_etc[ul_core_id].pst_frw_trace->line_num = line_num;
}

OAL_STATIC OAL_INLINE oal_void frw_event_trace(frw_event_mem_stru *pst_event_mem, oal_uint32 ul_core_id)
{
    oal_uint32 ul_pos;
    frw_event_trace_item_stru *pst_trace_item;
    frw_event_hdr_stru        *pst_event_hdr;

    pst_event_hdr           = frw_get_event_hdr(pst_event_mem);
    ul_pos = g_ast_event_manager_etc[ul_core_id].pst_frw_trace->ul_current_pos;

    if(OAL_WARN_ON(ul_pos >= CONFIG_FRW_MAX_TRACE_EVENT_NUMS))
    {
           OAM_ERROR_LOG2(0, OAM_SF_FRW, "{frw_event_last_pc_trace: ul_core_id:%d beyond limit:%d}",ul_pos,CONFIG_FRW_MAX_TRACE_EVENT_NUMS);
           return;
    };

    if(OAL_WARN_ON(ul_core_id >= WLAN_FRW_MAX_NUM_CORES))
    {
           OAM_ERROR_LOG2(0, OAM_SF_FRW, "{frw_event_last_pc_trace: ul_core_id:%d beyond limit:%d}",ul_core_id,WLAN_FRW_MAX_NUM_CORES);
           return;
    };

    pst_trace_item = &(g_ast_event_manager_etc[ul_core_id].pst_frw_trace->st_trace_item[ul_pos]);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
    pst_trace_item->timestamp = local_clock();
#else
    pst_trace_item->timestamp = 0;
#endif
    pst_trace_item->st_event_seg.en_type = pst_event_hdr->en_type;
    pst_trace_item->st_event_seg.uc_sub_type = pst_event_hdr->uc_sub_type;
    pst_trace_item->st_event_seg.uc_vap_id = pst_event_hdr->uc_vap_id;
    pst_trace_item->st_event_seg.en_pipeline = pst_event_hdr->en_pipeline;

    if((++ul_pos) >= CONFIG_FRW_MAX_TRACE_EVENT_NUMS)
    {
        g_ast_event_manager_etc[ul_core_id].pst_frw_trace->ul_current_pos = 0;
        g_ast_event_manager_etc[ul_core_id].pst_frw_trace->ul_over_flag = 1;
    }
    else
    {
        g_ast_event_manager_etc[ul_core_id].pst_frw_trace->ul_current_pos++;
    }
}
#endif

#ifdef _PRE_OAL_FEATURE_TASK_NEST_LOCK
extern oal_task_lock_stru  g_frw_event_task_lock_etc;
#define frw_event_task_lock()   do{oal_smp_task_lock(&g_frw_event_task_lock_etc);}while(0)
#define frw_event_task_unlock()   do{oal_smp_task_unlock(&g_frw_event_task_lock_etc);}while(0)
#else
#define frw_event_task_lock()   do{}while(0)
#define frw_event_task_unlock()   do{}while(0)
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of frw_event_main.h */
