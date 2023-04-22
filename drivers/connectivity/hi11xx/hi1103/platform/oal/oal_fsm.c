

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

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_FSM_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


oal_uint32  oal_fsm_create(oal_void                   *p_oshandle,         /*状态机owner的指针，对低功耗状态机，指向VAP结构*/
                                const oal_uint8          *p_name,             /*状态机的名字*/
                                oal_void                 *p_ctx,              /*状态机context*/
                                oal_fsm_stru             *pst_oal_fsm,        /* oal状态机内容 */
                                oal_uint8                 uc_init_state,      /*初始状态*/
                                const oal_fsm_state_info *p_state_info,       /*状态机实例指针*/
                                oal_uint8                 uc_num_states       /*本状态机的状态个数*/
)
{
    oal_uint32      ul_loop;

    if (OAL_PTR_NULL == pst_oal_fsm)
    {
        OAL_IO_PRINT("{oal_fsm_create:pst_oal_fsm is NULL }");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FSM_MAX_STATES < uc_num_states)
    {
       OAL_IO_PRINT("{oal_fsm_create:state number [%d] too big. }",uc_num_states);
       return OAL_FAIL;
    }

    /*检查状态信息顺序是否和状态定义匹配*/
    for (ul_loop = 0;ul_loop < uc_num_states;ul_loop++)
    {
        if ((p_state_info[ul_loop].state >= OAL_FSM_MAX_STATES) || (p_state_info[ul_loop].state!=ul_loop))
        {
            /* OAM日志中不能使用%s*/
            OAL_IO_PRINT("oal_fsm_create::entry %d has invalid state %d }",ul_loop,p_state_info[ul_loop].state);
            return OAL_FAIL;
        }
    }

    OAL_MEMZERO(pst_oal_fsm, OAL_SIZEOF(oal_fsm_stru));
    pst_oal_fsm->uc_cur_state  = uc_init_state;
    pst_oal_fsm->uc_prev_state = uc_init_state;
    pst_oal_fsm->p_state_info  = p_state_info;
    pst_oal_fsm->uc_num_states = uc_num_states;
    pst_oal_fsm->p_oshandler   = p_oshandle;
    pst_oal_fsm->p_ctx         = p_ctx;
    pst_oal_fsm->us_last_event = OAL_FSM_EVENT_NONE;

    /* strncpy fsm name */
    ul_loop = 0;
    while((ul_loop < OAL_FSM_MAX_NAME -1) && (p_name[ul_loop] != '\0')) {
        pst_oal_fsm->uc_name[ul_loop] = p_name[ul_loop];
        ul_loop++;
    }
    if (ul_loop < OAL_FSM_MAX_NAME) {
        pst_oal_fsm->uc_name[ul_loop] = '\0';
    }

    /*启动状态机*/
   if(pst_oal_fsm->p_state_info[pst_oal_fsm->uc_cur_state].oal_fsm_entry)
   {
       pst_oal_fsm->p_state_info[pst_oal_fsm->uc_cur_state].oal_fsm_entry(pst_oal_fsm->p_ctx);
   }

   return OAL_SUCC;
}
#if 0

oal_void oal_fsm_destroy(oal_fsm_stru* p_fsm)
{
    //OAL_MEM_FREE(p_fsm, OAL_TRUE);
    //p_fsm = OAL_PTR_NULL;
    return;
}
#endif

oal_uint32 oal_fsm_trans_to_state(oal_fsm_stru* p_fsm,oal_uint8 uc_state)
{
    oal_uint8                    uc_cur_state = p_fsm->uc_cur_state;

    if ((uc_state == OAL_FSM_STATE_NONE) || (uc_state >= OAL_FSM_MAX_STATES) || (uc_state>=p_fsm->uc_num_states))
    {
        OAL_IO_PRINT("oal_fsm_trans_to_state::trans to state %d needs to be a valid state cur_state=%d",uc_state,uc_cur_state);
        return OAL_FAIL;
    }

    if (uc_state == uc_cur_state)
    {
        OAL_IO_PRINT("oal_fsm_trans_to_state::trans to state %d needs to be a valid state cur_state=%d",uc_state,uc_cur_state);
        return OAL_SUCC;
    }

    /*调用前一状态的退出函数*/
    if (p_fsm->p_state_info[p_fsm->uc_cur_state].oal_fsm_exit)
    {
        p_fsm->p_state_info[p_fsm->uc_cur_state].oal_fsm_exit(p_fsm->p_ctx);
    }

    /*调用本状态的进入函数*/
    if (p_fsm->p_state_info[uc_state].oal_fsm_entry)
    {
       p_fsm->p_state_info[uc_state].oal_fsm_entry(p_fsm->p_ctx);
    }

    p_fsm->uc_prev_state = uc_cur_state;
    p_fsm->uc_cur_state  = uc_state;

    return OAL_SUCC;
}


oal_uint32 oal_fsm_event_dispatch(oal_fsm_stru* p_fsm ,oal_uint16 us_event,
                           oal_uint16 us_event_data_len, oal_void *p_event_data)
{
    oal_uint32 ul_event_handled = OAL_FAIL;

    if (p_fsm == OAL_PTR_NULL)
    {
        OAL_IO_PRINT("oal_fsm_event_dispatch:p_fsm = OAL_PTR_NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((p_fsm->uc_cur_state != OAL_FSM_STATE_NONE) && (p_fsm->uc_cur_state < p_fsm->uc_num_states))
    {
        p_fsm->us_last_event = us_event;
        ul_event_handled = (*p_fsm->p_state_info[p_fsm->uc_cur_state].oal_fsm_event)(p_fsm->p_ctx, us_event, us_event_data_len, p_event_data);
    }
    if (OAL_FAIL == ul_event_handled)
    {
        OAL_IO_PRINT("oal_fsm_event_dispatch:event[%d] did not handled in state %d",
                            us_event,p_fsm->p_state_info[p_fsm->uc_cur_state].state);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(oal_fsm_create);
//oal_module_symbol(oal_fsm_destroy);
oal_module_symbol(oal_fsm_trans_to_state);
oal_module_symbol(oal_fsm_event_dispatch);
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

