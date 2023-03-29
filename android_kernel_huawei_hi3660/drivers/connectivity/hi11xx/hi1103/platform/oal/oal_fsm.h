
#ifndef __OAL_FSM_H__
#define __OAL_FSM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
//#undef  THIS_FILE_ID
//#define THIS_FILE_ID OAM_FILE_ID_OAL_FSM_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define OAL_FSM_MAX_NAME    8  /*状态机名最大长度*/
#define OAL_FSM_MAX_STATES 100
#define OAL_FSM_MAX_EVENTS 100
#define OAL_FSM_STATE_NONE 255 /* invalid state */
#define OAL_FSM_EVENT_NONE 255 /* invalid event */
/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/*状态信息结构定义*/
typedef struct __oal_fsm_state_info{
    oal_uint32          state;                   /*状态ID*/
    const oal_int8      *name;                   /*状态名*/
    oal_void (*oal_fsm_entry)(oal_void *p_ctx);  /*进入本状态的处理回调函数指针*/
    oal_void (*oal_fsm_exit)(oal_void *p_ctx);   /*退出本状态的处理回调函数指针*/
                                                 /*本状态下的事件处理回调函数指针*/
    oal_uint32 (*oal_fsm_event)(oal_void *p_ctx,oal_uint16 event,oal_uint16 event_data_len,oal_void *event_data);
} oal_fsm_state_info;

/*状态机结构定义*/
typedef struct  __oal_fsm {
    oal_uint8  uc_name[OAL_FSM_MAX_NAME];              /*状态机名字 */
    oal_uint8  uc_cur_state;                           /*当前状态*/
    oal_uint8  uc_prev_state;                          /*前一状态，发出状态切换事件的状态 */
    oal_uint8  uc_num_states;                          /*状态机的状态个数*/
    oal_uint8  uc_rsv[1];
    const oal_fsm_state_info *p_state_info;
    oal_void   *p_ctx;                                 /*上下文，指向状态机实例拥有者 */
    oal_void   *p_oshandler;                           /*owner指针，指向VAP或者device,由具体的状态机决定*/
    oal_uint16  us_last_event;                          /*最后处理的事件*/
    oal_uint8  uc_rsv1[2];
} oal_fsm_stru;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  oal_fsm_create(oal_void            *p_oshandle,         /*状态机owner的指针，对低功耗状态机，指向VAP结构*/
                                const oal_uint8          *p_name,             /*状态机的名字*/
                                oal_void                 *p_ctx,              /*状态机context*/
                                oal_fsm_stru             *pst_oal_fsm,          /* oal状态机内容 */
                                oal_uint8                 uc_init_state,      /*初始状态*/
                                const oal_fsm_state_info *p_state_info,       /*状态机实例指针*/
                                oal_uint8                 uc_num_states     /*本状态机的状态个数*/
);

//extern oal_void oal_fsm_destroy(oal_fsm_stru* p_fsm);
extern oal_uint32 oal_fsm_trans_to_state(oal_fsm_stru* p_fsm,oal_uint8 uc_state);

extern oal_uint32 oal_fsm_event_dispatch(oal_fsm_stru* p_fsm ,oal_uint16 us_event,
                           oal_uint16 us_event_data_len, oal_void *p_event_data);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif
