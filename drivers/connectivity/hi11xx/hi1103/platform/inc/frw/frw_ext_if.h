

#ifndef __FRW_EXT_IF_H__
#define __FRW_EXT_IF_H__

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

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_EXT_IF_H

#define FRW_TIMER_RESTART_TIMER(_pst_timeout, _ul_timeout, _en_is_periodic) \
        frw_timer_restart_timer_etc(_pst_timeout, _ul_timeout, _en_is_periodic)
#define FRW_TIMER_STOP_TIMER(_pst_timeout) \
        frw_timer_stop_timer_etc(_pst_timeout)
#define FRW_TIMER_CREATE_TIMER(_pst_timeout, _p_timeout_func, _ul_timeout, _p_timeout_arg, _en_is_periodic, _en_module_id, _ul_core_id) \
        frw_timer_create_timer_etc(THIS_FILE_ID, __LINE__, _pst_timeout, _p_timeout_func, _ul_timeout, _p_timeout_arg, _en_is_periodic, _en_module_id, _ul_core_id)
#define FRW_TIMER_DESTROY_TIMER(_pst_timeout)\
        frw_timer_immediate_destroy_timer_etc(THIS_FILE_ID, __LINE__, _pst_timeout)
#define FRW_TIMER_IMMEDIATE_DESTROY_TIMER(_pst_timeout)\
        frw_timer_immediate_destroy_timer_etc(THIS_FILE_ID, __LINE__, _pst_timeout)

#define FRW_TIMER_TRACK_NUM     256
#define FRW_TIMEOUT_TRACK_NUM   256

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#define FRW_PROCESS_MAX_EVENT   100
#else
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
#define FRW_PROCESS_MAX_EVENT   100
#else
#define FRW_PROCESS_MAX_EVENT   100
#endif
#endif

/*****************************************************************************
  2 枚举定义
*****************************************************************************/
/* 此枚举用于定义从FRW模块以上各模块的初始化状态 */
typedef enum
{
    FRW_INIT_STATE_START,                /* 表示初始化刚启动，即FRW初始化开始 */
    FRW_INIT_STATE_FRW_SUCC,             /* 表示FRW模块初始化成功 */
    FRW_INIT_STATE_HAL_SUCC,             /* 表示HAL模块初始化成功 */
    FRW_INIT_STATE_DMAC_CONFIG_VAP_SUCC, /* 表示DMAC模块启动初始化成功，配置VAP已正常；如果在此状态之后再初始化，即为业务VAP的初始化 */
    FRW_INIT_STATE_HMAC_CONFIG_VAP_SUCC, /* 表示HMAC模块启动初始化成功，配置VAP已正常；如果在此状态之后再初始化，即为业务VAP的初始化 */
    FRW_INIT_STATE_ALL_SUCC,             /* 此状态表示HMAC以上模块均已初始化成功 */

    FRW_INIT_STATE_BUTT
}frw_init_enum;
typedef oal_uint16 frw_init_enum_uint16;

/*****************************************************************************
  枚举名  : frw_event_type_enum_uint8
  协议表格:
  枚举说明: 事件类型
*****************************************************************************/
typedef enum
{
    FRW_EVENT_TYPE_HIGH_PRIO = 0,        /*高优先级事件队列 */
    FRW_EVENT_TYPE_HOST_CRX,             /* 接收Host侧发来的配置事件 */
    FRW_EVENT_TYPE_HOST_DRX,             /* 接收Host侧发来的数据事件 */
    FRW_EVENT_TYPE_HOST_CTX,             /* 发向HOST侧的配置事件 */
    FRW_EVENT_TYPE_HOST_SDT_REG  = 4,    /* SDT读取寄存器，wifi驱动侧上报SDT */
    FRW_EVENT_TYPE_WLAN_CRX,             /* 接收Wlan侧发来的管理/控制帧事件 */
    FRW_EVENT_TYPE_WLAN_DRX,             /* 接收Wlan侧发来的数据帧事件 */
    FRW_EVENT_TYPE_WLAN_CTX,             /* 管理/控制帧发送至Wlan侧事件 */
    FRW_EVENT_TYPE_WLAN_DTX,             /* 数据帧发送至Wlan侧事件*/
    FRW_EVENT_TYPE_WLAN_TX_COMP  = 9,    /* 发送完成事件 */
    FRW_EVENT_TYPE_TBTT,                 /* TBTT中断事件 */
    FRW_EVENT_TYPE_TIMEOUT,              /* FRW普通超时事件 */
    FRW_EVENT_TYPE_HMAC_MISC,            /* HMAC杂散事件，例如雷达检测事件 */
    FRW_EVENT_TYPE_DMAC_MISC     = 13,   /* DMAC杂散事件 */
    /* 添加新的事件类型 */

    FRW_EVENT_TYPE_BUTT
}frw_event_type_enum;

typedef oal_uint8 frw_event_type_enum_uint8;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* HCC事件子类型定义*/
typedef enum
{
    DMAC_HCC_TX_EVENT_SUB_TYPE,

    DMAC_HCC_TX_EVENT_SUB_TYPE_BUTT
}dmac_hcc_tx_event_sub_type_enum;
typedef oal_uint8 dmac_hcc_tx_event_sub_type_enum_uint8;

typedef enum
{
    DMAC_HCC_RX_EVENT_SUB_TYPE,

    DMAC_HCC_RX_EVENT_SUB_TYPE_BUTT
}dmac_hcc_rx_event_sub_type_enum;
typedef oal_uint8 dmac_hcc_rx_event_sub_type_enum_uint8;


typedef struct
{
    frw_event_type_enum_uint8  en_nest_type;   /*嵌套的业务事件的主类型*/
    oal_uint8                  uc_nest_sub_type; /*嵌套的业务事件的子类型*/
    void                      *pst_netbuf;     /*如果是数据事件，则对应于链表头netbuf的地址;其他事件，则对应于buff首地址*/
    oal_uint32                 ul_buf_len;     /*如果是数据事件，则对应于netbuf个数;其他事件，则对应于buff len*/
}hcc_event_stru;

/*record the data type by the hcc*/
#pragma pack(push,1)
/*4B*/
struct frw_hcc_extend_hdr
{
    frw_event_type_enum_uint8  en_nest_type;
    oal_uint8                  uc_nest_sub_type;
    oal_uint8                  chip_id:2;             /*this is not good*/
    oal_uint8                  device_id:2;
    oal_uint8                  vap_id:4;
}__OAL_DECLARE_PACKED ;
#pragma pack(pop)

#endif


typedef oal_uint32          (*frw_timeout_func)(oal_void *);

typedef struct
{
    oal_void                   *p_timeout_arg;      /* 超时处理函数入参 */
    frw_timeout_func            p_func;             /* 超时处理函数 */
    oal_uint32                  ul_time_stamp;      /* 定时器启动时间 */
    oal_uint32                  ul_curr_time_stamp; /* 定时器进入当前时间 */
    oal_uint32                  ul_timeout;         /* 过多长时间定时器超时 */
    oal_bool_enum_uint8         en_is_registerd;    /* 定时器是否已经注册 */
    oal_bool_enum_uint8         en_is_periodic;     /* 定时器是否为周期的 */
    oal_bool_enum_uint8         en_is_enabled;      /* 定时器是否使能 */
    oal_uint8                   uc_pad;
    oam_module_id_enum_uint16   en_module_id;       /* 维测用模块id */
    oal_uint32                  ul_core_id;         /* 绑定的核id */
    oal_uint32                  ul_file_id;
    oal_uint32                  ul_line_num;
    oal_dlist_head_stru         st_entry;           /* 定期器链表索引 */
}frw_timeout_stru;
#if defined(_PRE_DEBUG_MODE) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

typedef struct
{
    oal_uint32                  ul_file_id;
    oal_uint32                  ul_line_num;
    oal_uint32                  ul_execute_time;
}frw_timer_track_stru;

typedef struct
{
    oal_uint8               uc_timer_cnt;
    frw_timer_track_stru    st_timer_track[FRW_TIMER_TRACK_NUM];
    oal_uint32              ul_os_timer_interval;
}frw_timeout_track_stru;
#endif
/*****************************************************************************
  枚举名  : frw_event_type_enum_uint8
  协议表格:
  枚举说明: 事件分段号，取值[0, 1]
*****************************************************************************/
typedef enum
{
    FRW_EVENT_PIPELINE_STAGE_0 = 0, //抛事件
    FRW_EVENT_PIPELINE_STAGE_1,     //直接调用
#if (((_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)||(_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE))
    FRW_EVENT_PIPELINE_STAGE_2,
#endif
    FRW_EVENT_PIPELINE_STAGE_BUTT
}frw_event_pipeline_enum;
typedef oal_uint8 frw_event_pipeline_enum_uint8;


#define FRW_RX_EVENT_TRACK_NUM 256
#define FRW_EVENT_TRACK_NUM     128



/*****************************************************************************
  3 全局变量声明
*****************************************************************************/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    extern oal_uint8 g_tx_debug;
#endif


/*****************************************************************************
  4 消息头定义
*****************************************************************************/
typedef oal_mem_stru    frw_event_mem_stru;    /* 事件结构体内存块转定义 */


/*****************************************************************************
  5 消息定义
*****************************************************************************/


/*****************************************************************************
  6 STRUCT定义
*****************************************************************************/
/*****************************************************************************
  结构名  : frw_event_hdr_stru
  结构说明: 事件头结构体,
  备注    : uc_length的值为(payload长度 + 事件头长度 - 2)
*****************************************************************************/
typedef struct
{
    frw_event_type_enum_uint8        en_type;         /* 事件类型 */
    oal_uint8                        uc_sub_type;     /* 事件子类型 */
    oal_uint16                       us_length;        /* 事件总体长度 */
    frw_event_pipeline_enum_uint8    en_pipeline;     /* 事件分段号 */
    oal_uint8                        uc_chip_id;      /* 芯片ID */
    oal_uint8                        uc_device_id;    /* 设备ID */
    oal_uint8                        uc_vap_id;       /* VAP ID */
}frw_event_hdr_stru;

/*****************************************************************************
  结构名  : frw_event_stru
  结构说明: 事件结构体
*****************************************************************************/
typedef struct
{
    frw_event_hdr_stru    st_event_hdr;           /* 事件头 */
    oal_uint8             auc_event_data[4];      /* payload */
}frw_event_stru;

/*****************************************************************************
  结构名  : frw_event_sub_table_item_stru
  结构说明: 事件子表结构体
*****************************************************************************/
typedef struct
{
    oal_uint32 (*p_func)(frw_event_mem_stru *);       /* (type, subtype, pipeline)类型的事件对应的处理函数 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32 (*p_tx_adapt_func)(frw_event_mem_stru *);
    frw_event_mem_stru *(*p_rx_adapt_func)(frw_event_mem_stru *);
#endif
}frw_event_sub_table_item_stru;

/*****************************************************************************
  结构名  : frw_event_table_item_stru
  结构说明: 事件表结构体
*****************************************************************************/
typedef struct
{
    frw_event_sub_table_item_stru   *pst_sub_table;    /* 指向子表的指针 */
}frw_event_table_item_stru;

/*****************************************************************************
  结构名  : frw_ipc_msg_header_stru
  结构说明: IPC(核间通信)头结构体
*****************************************************************************/
typedef struct
{
    oal_uint16      us_seq_number;          /* 核间消息序号 */
    oal_uint8       uc_target_cpuid;        /* 目标核cpuid frw_ipc_cpu_id_enum_uint8 */
    oal_uint8       uc_msg_type;            /* 消息类型 frw_ipc_msg_type_enum_uint8 */
}frw_ipc_msg_header_stru;

typedef struct
{
    oal_uint32      ul_event_cnt;
    oal_uint32      aul_event_time[FRW_EVENT_TRACK_NUM];
    oal_uint16      us_event_type[FRW_EVENT_TRACK_NUM];
    oal_uint16      us_event_sub_type[FRW_EVENT_TRACK_NUM];
}frw_event_track_time_stru;

#define FRW_IPC_MSG_HEADER_LENGTH       (OAL_SIZEOF(frw_ipc_msg_header_stru))

/*****************************************************************************
  7 UNION定义
*****************************************************************************/


/*****************************************************************************
  8 宏定义
*****************************************************************************/
/* 事件头长度 */
#define FRW_EVENT_HDR_LEN    OAL_SIZEOF(frw_event_hdr_stru)

/* 事件队列最大个数 */
#define FRW_EVENT_MAX_NUM_QUEUES    (FRW_EVENT_TYPE_BUTT * WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)

#define FRW_FIELD_SETUP(_p, _m, _v)    ((_p)->_m = (_v))

/* 事件头修改宏(修改事件头中的pipeline和subtype) */
#define FRW_EVENT_HDR_MODIFY_PIPELINE_AND_SUBTYPE(_pst_event_hdr, _uc_sub_type)      \
    do {                                                                            \
        FRW_FIELD_SETUP((_pst_event_hdr), en_pipeline, 1);             \
        FRW_FIELD_SETUP((_pst_event_hdr), uc_sub_type, (_uc_sub_type));                  \
    } while(0)

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#define FRW_EVENT_HDR_MODIFY_PIPELINE_AND_TYPE_AND_SUBTYPE(_pst_event_hdr, _en_type, _uc_sub_type)      \
        do {                                                                            \
            FRW_FIELD_SETUP((_pst_event_hdr), en_pipeline, 1);             \
            FRW_FIELD_SETUP((_pst_event_hdr), uc_sub_type, (_uc_sub_type));                  \
            FRW_FIELD_SETUP((_pst_event_hdr), en_type, (_en_type));                  \
        } while(0)
#endif

/* 获取事件内存payload */
//#define FRW_GET_EVENT_DATA(_pst_event_mem) ((_pst_event_mem)->puc_data)

#define FRW_EVENT_ALLOC(_us_len) \
    frw_event_alloc_etc(THIS_FILE_ID, __LINE__, _us_len);

#define FRW_EVENT_FREE(_pst_event_mem) \
    frw_event_free_etc(THIS_FILE_ID, __LINE__, _pst_event_mem)

/* Hi10X共仓代码适配51，Hi10X为了编译时进行检查，后续可能会有调整 */
#define FRW_EVENT_ALLOC_BIG(_us_len) \
    frw_event_alloc_etc(THIS_FILE_ID, __LINE__, _us_len);

#define FRW_EVENT_ALLOC_LARGE(_us_len) \
    frw_event_alloc_etc(THIS_FILE_ID, __LINE__, _us_len);

/* 事件头初始化宏 */
#define FRW_EVENT_HDR_INIT(_pst_event_hdr, _en_type, _uc_sub_type, _us_length, _en_pipeline, _uc_chip_id, _uc_device_id, _uc_vap_id) \
    do{\
         FRW_FIELD_SETUP((_pst_event_hdr), us_length, (_us_length + FRW_EVENT_HDR_LEN));\
         FRW_FIELD_SETUP((_pst_event_hdr), en_type, (_en_type));\
         FRW_FIELD_SETUP((_pst_event_hdr), uc_sub_type, (_uc_sub_type));\
         FRW_FIELD_SETUP((_pst_event_hdr), en_pipeline, (_en_pipeline));\
         FRW_FIELD_SETUP((_pst_event_hdr), uc_chip_id, (_uc_chip_id));\
         FRW_FIELD_SETUP((_pst_event_hdr), uc_device_id, (_uc_device_id));\
         FRW_FIELD_SETUP((_pst_event_hdr), uc_vap_id, (_uc_vap_id));\
    }while(0)

/* 为了hi110x和51共仓代码一致，这里保留该宏定义，使用时注意和frw_get_event_stru进行区分 */
#define frw_get_event_data(pst_event_mem)      ((pst_event_mem)->puc_data)

#define frw_get_event_stru(pst_event_mem)      ((frw_event_stru *)(pst_event_mem)->puc_data)
#define frw_get_event_hdr(pst_event_mem)       ((frw_event_hdr_stru*)(&((frw_event_stru *)(pst_event_mem)->puc_data)->st_event_hdr))
#define frw_get_event_payload(pst_event_mem)   ((oal_uint8*)((frw_event_stru *)(pst_event_mem)->puc_data)->auc_event_data)

/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


OAL_STATIC OAL_INLINE frw_event_mem_stru*  frw_event_alloc_etc(oal_uint32    ul_file_id,
                                                           oal_uint32    ul_line_num,
                                                           oal_uint16    us_payload_length)
{
    frw_event_mem_stru   *pst_event_mem;

    us_payload_length += OAL_MEM_INFO_SIZE;

    pst_event_mem = oal_mem_alloc_enhanced_etc(ul_file_id, ul_line_num, OAL_MEM_POOL_ID_EVENT, (us_payload_length + FRW_EVENT_HDR_LEN), OAL_TRUE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        oal_mem_print_normal_pool_info_etc(OAL_MEM_POOL_ID_EVENT);
#endif

        return OAL_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_event_mem->ul_return_addr  = OAL_RET_ADDR;
#endif

#if (_PRE_MULTI_CORE_MODE_SMP == _PRE_MULTI_CORE_MODE)
    pst_event_mem->puc_data += FRW_IPC_MSG_HEADER_LENGTH;
#endif

    return pst_event_mem;
}


OAL_STATIC OAL_INLINE oal_uint32  frw_event_free_etc(oal_uint32            ul_file_id,
                                                 oal_uint32            ul_line_num,
                                                 frw_event_mem_stru   *pst_event_mem)
{
    oal_uint32       ul_ret;
    //oal_uint     ul_irq_flag;
    frw_event_stru  *pst_frw_event;

    /* 中断上半部会申请事件，所以需要关中断 */
    //oal_irq_save(&ul_irq_flag, OAL_5115IRQ_FEF);

    ul_ret = oal_mem_free_enhanced_etc(ul_file_id, ul_line_num, pst_event_mem, OAL_TRUE);
    if(OAL_WARN_ON(OAL_SUCC != ul_ret))
    {
        pst_frw_event = frw_get_event_stru(pst_event_mem);
        OAL_IO_PRINT("[E]frw event free failed!, ret:%d, type:%d, subtype:%d\r\n",ul_ret, pst_frw_event->st_event_hdr.en_type, pst_frw_event->st_event_hdr.uc_sub_type);
        oal_dump_stack();
    }
    //oal_irq_restore(&ul_irq_flag, OAL_5115IRQ_FEF);

    return ul_ret;
}

/*****************************************************************************
  10 函数声明
*****************************************************************************/
#ifdef _PRE_DEBUG_MODE
extern oal_uint32                g_ul_rx_event_idx_etc;
extern oal_bool_enum_uint8       g_en_event_track_switch_etc;
extern oal_uint32                g_ul_schedule_idx_etc;
extern oal_uint32                g_aul_schedule_time_etc[FRW_RX_EVENT_TRACK_NUM];
extern oal_uint32                g_ul_mac_process_event;
#endif

#if defined(_PRE_DEBUG_MODE) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern frw_event_track_time_stru g_ast_event_time_track_etc[FRW_RX_EVENT_TRACK_NUM];
extern frw_timeout_track_stru    g_st_timeout_track_etc[FRW_TIMEOUT_TRACK_NUM];
extern oal_uint8                 g_uc_timeout_track_idx_etc;
#endif

extern oal_uint8 g_uc_timer_pause_etc;
extern oal_int32  frw_main_init_etc(oal_void);
extern oal_void  frw_main_exit_etc(oal_void);
extern oal_void  frw_set_init_state_etc(frw_init_enum_uint16 en_init_state);
extern frw_init_enum_uint16  frw_get_init_state_etc(oal_void);
extern oal_uint32  frw_event_exit_etc(oal_void);
extern oal_uint32 frw_event_init_etc(oal_void);
extern oal_uint32  frw_event_dispatch_event_etc(frw_event_mem_stru *pst_mem);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
extern oal_uint32  frw_event_post_event_etc(frw_event_mem_stru *pst_event_mem,oal_uint32 ul_core_id);
#endif
extern oal_void  frw_event_table_register_etc(
                frw_event_type_enum_uint8      en_event_type,
                frw_event_pipeline_enum        en_pipeline,
                frw_event_sub_table_item_stru *pst_sub_table);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
oal_void frw_event_sub_rx_adapt_table_init_etc(frw_event_sub_table_item_stru *pst_sub_table, oal_uint32 ul_table_nums,
                                                frw_event_mem_stru *(*p_rx_adapt_func)(frw_event_mem_stru *));
#endif
extern oal_void  frw_event_dump_event_etc(oal_uint8 *puc_event);
extern oal_uint32  frw_event_flush_event_queue_etc(frw_event_type_enum_uint8 uc_event_type);
extern oal_void frw_event_vap_pause_event_etc(oal_uint8 uc_vap_id);
extern oal_void frw_event_vap_resume_event_etc(oal_uint8 uc_vap_id);
extern oal_uint32 frw_event_vap_flush_event_etc(oal_uint8                 uc_vap_id,
                                            frw_event_type_enum_uint8 en_event_type,
                                            oal_bool_enum_uint8       en_drop);
extern oal_void  frw_timer_create_timer_etc(
                                            oal_uint32 ul_file_id,
                                            oal_uint32 ul_line_num,
                                            frw_timeout_stru *pst_timeout,
                                            frw_timeout_func  p_timeout_func,
                                            oal_uint32 ul_timeout,
                                            oal_void *p_timeout_arg,
                                            oal_bool_enum_uint8  en_is_periodic,
                                            oam_module_id_enum_uint16   en_module_id,
                                            oal_uint32 ul_core_id);
extern oal_void  frw_timer_immediate_destroy_timer_etc(oal_uint32 ul_file_id,
                                                           oal_uint32 ul_line_num,
                                                           frw_timeout_stru *pst_timeout);
extern oal_void  frw_timer_restart_timer_etc(frw_timeout_stru *pst_timeout, oal_uint32 ul_timeout, oal_bool_enum_uint8  en_is_periodic);
extern oal_void  frw_timer_add_timer_etc(frw_timeout_stru *pst_timeout);
extern oal_void  frw_timer_stop_timer_etc(frw_timeout_stru *pst_timeout);
extern oal_void  frw_timer_dump_timer_etc(oal_uint32 ul_core_id);
extern oal_void  frw_timer_delete_all_timer_etc(oal_void);
extern oal_uint32  frw_event_queue_info_etc(oal_void);
extern oal_void  frw_event_process_all_event_etc(oal_uint ui_data);
extern oal_bool_enum_uint8  frw_is_event_queue_empty_etc(frw_event_type_enum_uint8 uc_event_type);
extern oal_bool_enum_uint8  frw_is_vap_event_queue_empty_etc(oal_uint32 ul_core_id, oal_uint8 uc_vap_id, oal_uint8 event_type);
extern oal_uint8 frw_task_thread_condition_check_etc(oal_uint32 ul_core_id);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
extern oal_void   hcc_host_update_vi_flowctl_param_etc(oal_uint32 be_cwmin, oal_uint32 vi_cwmin);
#endif
extern oal_void   frw_timer_clean_timer(oam_module_id_enum_uint16 en_module_id);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of frw_ext_if.h */
