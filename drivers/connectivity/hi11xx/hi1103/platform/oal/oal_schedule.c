


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_schedule.h"

#ifdef _PRE_WIFI_DMT
#include "dmt_stub.h"
#endif

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )

OAL_DLIST_CREATE_HEAD(g_wakelock_head);
OAL_DEFINE_SPINLOCK(g_wakelock_lock);
/*lint -e19*/
oal_module_symbol(g_wakelock_head);
oal_module_symbol(g_wakelock_lock);
/*lint +e19*/
#endif


#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)

oal_void  oal_timer_init(oal_timer_list_stru *pst_timer, oal_uint32 ul_delay, oal_timer_func p_func, oal_uint ui_arg)
{
}


oal_int32  oal_timer_delete(oal_timer_list_stru *pst_timer)
{

    return 0;
}


oal_int32  oal_timer_delete_sync(oal_timer_list_stru *pst_timer)
{

    return 0;
}


oal_void  oal_timer_add(oal_timer_list_stru *pst_timer)
{
}


oal_int32  oal_timer_start(oal_timer_list_stru *pst_timer, oal_uint ui_expires)
{

#ifdef  _PRE_WIFI_DMT
	DmtStub_StartTimer(MGMT_TIMER, ui_expires);  /*15ms 的定时任务，触发frw_timer_timeout_proc_event_etc */
#endif

    return OAL_SUCC;
}

#endif

#ifdef _PRE_OAL_FEATURE_TASK_NEST_LOCK
oal_void _oal_smp_task_lock__etc(oal_task_lock_stru* pst_lock,oal_ulong  claim_addr)
{
	DECLARE_WAITQUEUE(wait, current);

	oal_ulong flags;

	//might_sleep();/*called task must can sleep*/

	if(OAL_WARN_ON(in_interrupt() || in_atomic()))
	{
		DECLARE_DFT_TRACE_KEY_INFO("smp_task_lock_sched_warn", OAL_DFT_TRACE_EXCEP);
	    return;
	}

	add_wait_queue(&pst_lock->wq, &wait);
	oal_spin_lock_irq_save(&pst_lock->lock, &flags);
	while (1) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (!pst_lock->claimed || pst_lock->claimer == current)
			break;
		oal_spin_unlock_irq_restore(&pst_lock->lock, &flags);
		schedule();
		oal_spin_lock_irq_save(&pst_lock->lock, &flags);
	}
	set_current_state(TASK_RUNNING);

	pst_lock->claimed = 1;
	pst_lock->claimer = current;
	pst_lock->claim_cnt++;

	oal_spin_unlock_irq_restore(&pst_lock->lock, &flags);
	remove_wait_queue(&pst_lock->wq, &wait);
}
oal_module_symbol(_oal_smp_task_lock__etc);
#endif

oal_int32  oal_print_all_wakelock_buff(char* buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
    oal_dlist_head_stru      *pst_entry;
    oal_dlist_head_stru      *pst_entry_temp;
    ret +=  snprintf(buf + ret , buf_len - ret,"lockname     lockcnt  debug  lockuser \n");
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_entry_temp, &g_wakelock_head)
    {
        oal_wakelock_stru* pst_wakelock = (oal_wakelock_stru*)OAL_DLIST_GET_ENTRY(pst_entry, oal_wakelock_stru, list);

        ret +=  snprintf(buf + ret , buf_len - ret,"%s     %lu   %s %pf\n",
                            pst_wakelock->st_wakelock.name,
                            pst_wakelock->lock_count,
                            pst_wakelock->debug ? "on ":"off",
                            (oal_void*)pst_wakelock->locked_addr);
    }
#else
    OAL_REFERENCE(ret);
    OAL_REFERENCE(buf);
    OAL_REFERENCE(buf_len);
#endif

    return ret;
}

oal_void oal_print_all_wakelock_info(oal_void)
{
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )
    oal_void* pst_mem = oal_memalloc(PAGE_SIZE);
    if(NULL == pst_mem)
    {
        OAL_IO_PRINT("mem alloc failed %u\n", (oal_uint32)PAGE_SIZE);
        return;
    }

    oal_print_all_wakelock_buff((char*)pst_mem, PAGE_SIZE);
    OAL_IO_PRINT("%s", (char*)pst_mem);
    oal_free(pst_mem);
#endif
}

oal_int32  oal_set_wakelock_debuglevel(char* name, oal_uint32 level)
{
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) )

    oal_dlist_head_stru      *pst_entry;
    oal_dlist_head_stru      *pst_entry_temp;
    OAL_IO_PRINT("lockname     lockcnt  debug  lockuser");
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_entry_temp, &g_wakelock_head)
    {
        oal_wakelock_stru* pst_wakelock = (oal_wakelock_stru*)OAL_DLIST_GET_ENTRY(pst_entry, oal_wakelock_stru, list);

        if(!oal_strcmp(name, pst_wakelock->st_wakelock.name))
        {
            OAL_IO_PRINT("set wakelock %s debuglevel from %u to %u\n", pst_wakelock->st_wakelock.name, pst_wakelock->debug, level);
            pst_wakelock->debug = level;
            return OAL_SUCC;
        }
    }
#else
    OAL_REFERENCE(name);
    OAL_REFERENCE(level);
#endif
    return -OAL_ENODEV;
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

