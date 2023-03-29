/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and 
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *	notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *	notice, this list of conditions and the following disclaimer in the
 * *	documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may 
 * *	be used to endorse or promote products derived from this software 
 * *	without specific prior written permission.
 * 
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/wakelock.h>
#include <osl_module.h>
#include <osl_sem.h>
#include <osl_list.h>
#include <osl_spinlock.h>
#include <bsp_softtimer.h>
#include <bsp_trace.h>
#include <osl_thread.h>
#include "softtimer_balong.h"
#define MOD_NAME "softtimer"
#define softtimer_print(fmt, ...)  (bsp_trace(BSP_LOG_LEVEL_ERROR, BSP_MODU_SOFTTIMER, "<%s> "fmt" ", MOD_NAME, ##__VA_ARGS__))

struct debug_wakeup_softtimer_s
{
	u32 wakeup_flag;
	const char* wakeup_timer_name;
	u32 wakeup_timer_id;
}debug_wakeup_timer;
struct softtimer_ctrl
{
	unsigned char timer_id_alloc[SOFTTIMER_MAX_NUM];
	struct list_head timer_list_head;
	u32 softtimer_start_value;
	u32 hard_timer_id;
	spinlock_t  timer_list_lock;
	osl_sem_id soft_timer_sem;
	OSL_TASK_ID softtimer_task;
	u32 clk;	/*hardtimer work freq*/
	u32 support;/*whether support this type softtimer*/
	u32 wake_times;/*softtimer wake system times*/
	u64 start_slice;
	slice_curtime get_curtime;
	slice_value   get_slice_value;
	struct     wake_lock     wake_lock;
};
/*lint --e{64,456}*/
static struct softtimer_ctrl timer_control[2];	/*timer_control[0] wake, timer_control[1] normal*/
u32 check_softtimer_support_type(enum wakeup type){
	return timer_control[(u32)type].support;
}
static void start_hard_timer(struct softtimer_ctrl *ptimer_control, u32 ulvalue )
{
	ptimer_control->softtimer_start_value = ulvalue;
	(void)ptimer_control->get_curtime(&ptimer_control->start_slice);
	bsp_hardtimer_disable(ptimer_control->hard_timer_id);
	bsp_hardtimer_load_value(ptimer_control->hard_timer_id,ulvalue);
	bsp_hardtimer_enable(ptimer_control->hard_timer_id);
}

static void stop_hard_timer(struct softtimer_ctrl *ptimer_control)
{
	bsp_hardtimer_disable(ptimer_control->hard_timer_id);
	ptimer_control->softtimer_start_value = ELAPESD_TIME_INVAILD;
}
static inline u32 calculate_timer_start_value(u64 expect_cb_slice,u64 cur_slice)
{
	if(expect_cb_slice > cur_slice)
		return (u32)(expect_cb_slice - cur_slice);
	else
		return 0;
}

/*
 bsp_softtimer_add,add the timer to the list;
 */
void bsp_softtimer_add(struct softtimer_list * timer)
{
	struct softtimer_list *p = NULL;
	unsigned long flags = 0;
	u64 now_slice = 0;
	if (NULL == timer)
	{
		softtimer_print("timer to be added is NULL\n");
		return;
	}
	spin_lock_irqsave(&(timer_control[timer->wake_type].timer_list_lock),flags);
	if(!list_empty(&timer->entry))
	{
		spin_unlock_irqrestore(&(timer_control[timer->wake_type].timer_list_lock),flags);
		return;
	}
	(void)timer_control[timer->wake_type].get_curtime(&now_slice);
	timer->expect_cb_slice = timer->count_num + now_slice;
	list_for_each_entry(p,&(timer_control[timer->wake_type].timer_list_head),entry)
	{
		if(p->expect_cb_slice > timer->expect_cb_slice)
		{
			break;
		}
	}
	list_add_tail(&(timer->entry),&(p->entry));
	if (timer_control[timer->wake_type].timer_list_head.next == &(timer->entry))
	{
		if ((timer->entry.next)!=(&(timer_control[timer->wake_type].timer_list_head)))
		{
			p = list_entry(timer->entry.next,struct softtimer_list,entry);
			if(TIMER_TRUE==p->is_running)
			{
				p->is_running = TIMER_FALSE;
			}
		}
		timer->is_running = TIMER_TRUE;
		start_hard_timer(&timer_control[timer->wake_type],calculate_timer_start_value(timer->expect_cb_slice,now_slice));
	}
	spin_unlock_irqrestore(&(timer_control[timer->wake_type].timer_list_lock),flags);
}
s32 bsp_softtimer_delete(struct softtimer_list * timer)
{
	struct softtimer_list * p=NULL;
	unsigned long flags;
	u64 now_slice = 0;
	if (NULL == timer)
	{
		softtimer_print("NULL pointer \n");
		return BSP_ERROR;
	}
	spin_lock_irqsave(&(timer_control[timer->wake_type].timer_list_lock),flags);
	(void)timer_control[timer->wake_type].get_curtime(&now_slice);
	if (list_empty(&timer->entry))
	{
		spin_unlock_irqrestore(&(timer_control[timer->wake_type].timer_list_lock),flags);
		return NOT_ACTIVE;
	}
	else
	{
	 	/*if the timer bo be deleted is the first node */
		if((timer->entry.prev == &(timer_control[timer->wake_type].timer_list_head))
			&&(timer->entry.next != &(timer_control[timer->wake_type].timer_list_head)))
		{
			timer->is_running = TIMER_FALSE;
			list_del_init(&(timer->entry));
			p=list_first_entry(&(timer_control[timer->wake_type].timer_list_head),struct softtimer_list,entry);
			start_hard_timer(&timer_control[p->wake_type], calculate_timer_start_value(p->expect_cb_slice,now_slice));
			p->is_running = TIMER_TRUE;
		}
		else
		{
			timer->is_running = TIMER_FALSE;
			list_del_init(&(timer->entry));
		}
	}
	/*if the list is empty after delete node, then stop hardtimer*/
	if (list_empty(&(timer_control[timer->wake_type].timer_list_head)))
	{
		stop_hard_timer(&timer_control[timer->wake_type]);
	}
	spin_unlock_irqrestore(&(timer_control[timer->wake_type].timer_list_lock),flags);
	return BSP_OK;
}
static inline s32 set_timer_expire_time_s(struct softtimer_list *timer,u32 new_expire_time)
{
	u32 timer_freq = 0;
	timer_freq = timer_control[timer->wake_type].clk;
	if(timer->timeout>0xFFFFFFFF/timer_freq)
	{
		softtimer_print("time too long\n");
		return BSP_ERROR;
	}
	timer->count_num= timer_freq * new_expire_time;
	return BSP_OK;
}

static inline s32 set_timer_expire_time_ms(struct softtimer_list *timer,u32 new_expire_time)
{
	u32 timer_freq = 0;
	timer_freq = timer_control[timer->wake_type].clk;
	if(timer->timeout>0xFFFFFFFF/timer_freq*1000)
	{
		softtimer_print("time too long\n");
		return BSP_ERROR;
	}
	if(timer_freq%1000)
	{
		if((new_expire_time) < (0xFFFFFFFF/timer_freq)) 
		{
			timer->count_num= (timer_freq* new_expire_time)/1000;
		}
		else 
		{
			timer->count_num= timer_freq * (new_expire_time/1000);
		}
	}
	 else 
	{
		timer->count_num= (timer_freq/1000)* new_expire_time;
	}
	return BSP_OK;
}

static inline s32 set_timer_expire_time(struct softtimer_list *timer,u32 expire_time)
{
	if(TYPE_S == timer->unit_type)
	{
		return set_timer_expire_time_s(timer,expire_time);
	}
	else if(TYPE_MS == timer->unit_type)
	{
		return set_timer_expire_time_ms(timer,expire_time);
	}
	else
	{
		return BSP_ERROR;
	}
}
s32 bsp_softtimer_modify(struct softtimer_list *timer,u32 new_expire_time)
{
	
	if((NULL == timer)||(!list_empty(&timer->entry)) )
	{
		return BSP_ERROR;
	}
	timer->timeout = new_expire_time;
	return set_timer_expire_time(timer,new_expire_time);
}

s32 bsp_softtimer_create(struct softtimer_list *timer)
{
	s32 ret = 0,i=0;
	unsigned long flags;
	if (!timer || !timer_control[(u32)timer->wake_type].support)
	{
		softtimer_print("para or wake type is error\n");
		return BSP_ERROR;
	}
	if(timer->init_flags==TIMER_INIT_FLAG)
		return BSP_ERROR;
	spin_lock_irqsave(&(timer_control[timer->wake_type].timer_list_lock),flags);/*lint !e550*/	
	INIT_LIST_HEAD(&(timer->entry));
	timer->is_running = TIMER_FALSE;
	ret = set_timer_expire_time(timer,timer->timeout);
	if(ret)
	{
		spin_unlock_irqrestore(&(timer_control[timer->wake_type].timer_list_lock),flags);	
		return BSP_ERROR;
	}
	for (i=0 ;i < SOFTTIMER_MAX_NUM; i++)
	{
		if (timer_control[timer->wake_type].timer_id_alloc[i] == 0)
		{
			timer->timer_id = i;
			timer_control[timer->wake_type].timer_id_alloc[i] = 1;
			break;
		}
	}
	if (SOFTTIMER_MAX_NUM == i)
	{
		bsp_trace(BSP_LOG_LEVEL_ERROR,BSP_MODU_SOFTTIMER,"error,not enough timerid for alloc, already 40 exists\n");
		spin_unlock_irqrestore(&(timer_control[timer->wake_type].timer_list_lock),flags);	
		return BSP_ERROR;
	}
	timer->init_flags=TIMER_INIT_FLAG;
	spin_unlock_irqrestore(&(timer_control[timer->wake_type].timer_list_lock),flags);	
	return BSP_OK;
}

s32 bsp_softtimer_free(struct softtimer_list *timer)
{
	unsigned long flags;
	if ((NULL == timer)||(!list_empty(&timer->entry)))
	{
		return BSP_ERROR;
	}
	spin_lock_irqsave(&(timer_control[timer->wake_type].timer_list_lock),flags);/*lint !e550*/	
	timer->init_flags = 0;
	timer_control[timer->wake_type].timer_id_alloc[timer->timer_id] = 0;
	spin_unlock_irqrestore(&(timer_control[timer->wake_type].timer_list_lock),flags);
	return BSP_OK;   
}


int  softtimer_task_func(void* data)
{
	struct softtimer_ctrl *ptimer_control;
	struct softtimer_list	 *p = NULL,*q = NULL;
	unsigned long flags;
	u64 now_slice = 0;
	u32 temp1 = 0,temp2 = 0;

	ptimer_control = (struct softtimer_ctrl *)data;
	/* coverity[no_escape] */
	for( ; ; )
	{
		/* coverity[sleep] */
		osl_sem_down(&ptimer_control->soft_timer_sem);
		 /* coverity[lock_acquire] */
		spin_lock_irqsave(&ptimer_control->timer_list_lock,flags);
		(void)ptimer_control->get_curtime(&now_slice);
		ptimer_control->softtimer_start_value = ELAPESD_TIME_INVAILD;
		list_for_each_entry_safe(p,q,&(ptimer_control->timer_list_head),entry)
		{
			if(!p->emergency)
			{				
				if(now_slice >= p->expect_cb_slice)
				{
					list_del_init(&p->entry);
					p->is_running = TIMER_FALSE;
					spin_unlock_irqrestore(&ptimer_control->timer_list_lock,flags); 
					temp1 = bsp_get_slice_value();
					if(p->func)
						p->func(p->para);
					temp2 = bsp_get_slice_value();
					p->run_cb_delta = get_timer_slice_delta(temp1,temp2);
					spin_lock_irqsave(&ptimer_control->timer_list_lock,flags);
					(void)ptimer_control->get_curtime(&now_slice);
				}
				else
				{
					break;
				}
			}
			else
				break;
		}
		if (!list_empty(&(ptimer_control->timer_list_head)))/*如果还有未超时定时器*/
		{
			p=list_first_entry(&(ptimer_control->timer_list_head),struct softtimer_list,entry);
			if(p->is_running == TIMER_FALSE)
			{
				p->is_running = TIMER_TRUE;
				start_hard_timer(ptimer_control,calculate_timer_start_value(p->expect_cb_slice , now_slice));
			}
		}
		else 
		{
			stop_hard_timer(ptimer_control);
		}
		if(ACORE_SOFTTIMER_ID==ptimer_control->hard_timer_id)
			wake_unlock(&ptimer_control->wake_lock); /*lint !e455*/
		if(debug_wakeup_timer.wakeup_flag)
		{
			softtimer_print("wakeup timer name:%s,wakeup_timer_id:%d",debug_wakeup_timer.wakeup_timer_name,debug_wakeup_timer.wakeup_timer_id);
			debug_wakeup_timer.wakeup_flag = 0;
		}
		spin_unlock_irqrestore(&ptimer_control->timer_list_lock,flags); 
	} 
	/*lint -save -e527*/ 
	return 0;
	/*lint -restore +e527*/ 
}


OSL_IRQ_FUNC(static irqreturn_t,softtimer_interrupt_call_back,irq,dev)
{	
	struct softtimer_ctrl *ptimer_control;
	u32 readValue = 0,temp1 = 0,temp2 = 0;
	u64 now_slice = 0;
	struct softtimer_list	 *p = NULL,*q = NULL;
	unsigned long flags=0;

	ptimer_control = dev;
	(void)ptimer_control->get_curtime(&now_slice);
	readValue = bsp_hardtimer_int_status(ptimer_control->hard_timer_id);
	if (0 != readValue)
	{
		bsp_hardtimer_int_clear(ptimer_control->hard_timer_id);
		spin_lock_irqsave(&ptimer_control->timer_list_lock,flags);/*lint !e550*/
		list_for_each_entry_safe(p,q,&ptimer_control->timer_list_head,entry)
		{
			if(p->emergency)
			{
				if(now_slice >= p->expect_cb_slice)
				{
					list_del_init(&p->entry);
					p->is_running = TIMER_FALSE;
					spin_unlock_irqrestore(&ptimer_control->timer_list_lock,flags); /*lint !e550*/
					temp1 = bsp_get_slice_value();
					if(p->func)
						p->func(p->para);
					temp2 = bsp_get_slice_value();
					p->run_cb_delta = get_timer_slice_delta(temp1,temp2);
					spin_lock_irqsave(&ptimer_control->timer_list_lock,flags);/*lint !e550*/
				}
				else
				{
					break;
				}
			}
			else
				break;
		}
		spin_unlock_irqrestore(&ptimer_control->timer_list_lock,flags); /*lint !e550*/
		if(ACORE_SOFTTIMER_ID==ptimer_control->hard_timer_id)
			wake_lock(&ptimer_control->wake_lock);  /*lint !e454*/
		osl_sem_up(&ptimer_control->soft_timer_sem);  /*lint !e456*/
	}
	return IRQ_HANDLED; /*lint !e454*/ /*lint !e456*/
}
static int get_softtimer_int_stat(int arg)
{
	struct softtimer_list	 *p = NULL;
	timer_control[SOFTTIMER_WAKE].wake_times++;
	if(!list_empty(&(timer_control[SOFTTIMER_WAKE].timer_list_head)))
	{
		p=list_first_entry(&(timer_control[SOFTTIMER_WAKE].timer_list_head),struct softtimer_list,entry);/*lint !e826*/
		debug_wakeup_timer.wakeup_timer_name = p->name;
		debug_wakeup_timer.wakeup_timer_id = p->timer_id;
		debug_wakeup_timer.wakeup_flag = 1;
	}
	return 0;
}

int  bsp_softtimer_init(void)
{
	s32 ret = 0;
	struct device_node *node = NULL;
	/* coverity[var_decl] */
	struct bsp_hardtimer_control timer_ctrl ;
	INIT_LIST_HEAD(&(timer_control[SOFTTIMER_WAKE].timer_list_head));
	INIT_LIST_HEAD(&(timer_control[SOFTTIMER_NOWAKE].timer_list_head));
	timer_control[SOFTTIMER_NOWAKE].hard_timer_id	  = ACORE_SOFTTIMER_NOWAKE_ID;
	timer_control[SOFTTIMER_WAKE].hard_timer_id		  = ACORE_SOFTTIMER_ID;
	timer_control[SOFTTIMER_WAKE].softtimer_start_value  = ELAPESD_TIME_INVAILD;
	timer_control[SOFTTIMER_NOWAKE].softtimer_start_value = ELAPESD_TIME_INVAILD;
	osl_sem_init(SEM_EMPTY,&(timer_control[SOFTTIMER_NOWAKE].soft_timer_sem));
	osl_sem_init(SEM_EMPTY,&(timer_control[SOFTTIMER_WAKE].soft_timer_sem));
	spin_lock_init(&(timer_control[SOFTTIMER_WAKE].timer_list_lock));
	spin_lock_init(&(timer_control[SOFTTIMER_NOWAKE].timer_list_lock));
	timer_ctrl.func = (irq_handler_t)softtimer_interrupt_call_back;
	timer_ctrl.mode=TIMER_ONCE_COUNT;
	timer_ctrl.timeout=0xffffffff;/*default value set 0xFFFFFFFF*/
	timer_ctrl.unit=TIMER_UNIT_NONE;
	node = of_find_compatible_node(NULL, NULL, "hisilicon,softtimer_support_type");
	if(!node)
	{
		softtimer_print("softtimer_support_type get failed.\n");
		return BSP_ERROR;
	}
	if (!of_device_is_available(node)){
		softtimer_print("softtimer_support_type status not ok.\n");
		return BSP_ERROR;
	}
	debug_wakeup_timer.wakeup_flag = 0;
	debug_wakeup_timer.wakeup_timer_id = 0xffffffff;
	ret = of_property_read_u32(node, "support_wake", &timer_control[SOFTTIMER_WAKE].support);
	ret |= of_property_read_u32(node, "wake-frequency", &timer_control[SOFTTIMER_WAKE].clk);
	ret |= of_property_read_u32(node, "support_unwake", &timer_control[SOFTTIMER_NOWAKE].support);
	ret |= of_property_read_u32(node, "unwake-frequency", &timer_control[SOFTTIMER_NOWAKE].clk);
	if(ret)
	{
		softtimer_print(" softtimer property  get failed.\n");
		return BSP_ERROR;
	}
	if (timer_control[SOFTTIMER_WAKE].support)
	{
		if(ERROR == osl_task_init("softtimer_wake", TIMER_TASK_WAKE_PRI, TIMER_TASK_STK_SIZE ,(void *)softtimer_task_func, (void*)&timer_control[SOFTTIMER_WAKE], /*lint !e611*/
			&timer_control[SOFTTIMER_WAKE].softtimer_task))
		{
			softtimer_print("softtimer_wake task create failed\n");
			return BSP_ERROR;
		}
		timer_ctrl.para = (void*)&timer_control[SOFTTIMER_WAKE];
		timer_ctrl.timerId =ACORE_SOFTTIMER_ID;
		timer_ctrl.irq_flags = IRQF_NO_SUSPEND;
		ret =bsp_hardtimer_config_init(&timer_ctrl);
		if (ret)
		{
			softtimer_print("bsp_hardtimer_alloc error,softtimer init failed 2\n");
			return BSP_ERROR;
		}
		wake_lock_init(&timer_control[SOFTTIMER_WAKE].wake_lock, WAKE_LOCK_SUSPEND, "softtimer_wake");
		timer_control[SOFTTIMER_WAKE].get_curtime = bsp_slice_getcurtime;
		timer_control[SOFTTIMER_WAKE].get_slice_value = bsp_get_slice_value;
		mdrv_timer_debug_register(timer_control[SOFTTIMER_WAKE].hard_timer_id,(FUNCPTR_1)get_softtimer_int_stat,0);
	}
	 if (timer_control[SOFTTIMER_NOWAKE].support)
	 {
		 if(ERROR == osl_task_init("softtimer_nowake", TIMER_TASK_NOWAKE_PRI, TIMER_TASK_STK_SIZE ,(void *)softtimer_task_func, (void*)&timer_control[SOFTTIMER_NOWAKE], /*lint !e611*/
			&timer_control[SOFTTIMER_NOWAKE].softtimer_task))
			{
				softtimer_print("softtimer_normal task create failed\n");
				return BSP_ERROR;
			}
		timer_ctrl.para = (void*)&timer_control[SOFTTIMER_NOWAKE];
		timer_ctrl.timerId =ACORE_SOFTTIMER_NOWAKE_ID;
		timer_ctrl.irq_flags = 0;
		/* coverity[uninit_use_in_call] */
		 ret =bsp_hardtimer_config_init(&timer_ctrl);
		if (ret)
		{
			softtimer_print("bsp_hardtimer_alloc error,softtimer init failed 2\n");
			return BSP_ERROR;
		}
		if(timer_control[SOFTTIMER_NOWAKE].clk%1000)
		{
			timer_control[SOFTTIMER_NOWAKE].get_curtime = bsp_slice_getcurtime;
			timer_control[SOFTTIMER_NOWAKE].get_slice_value = bsp_get_slice_value;
		}
		else
		{
			timer_control[SOFTTIMER_NOWAKE].get_curtime = bsp_slice_getcurtime_hrt;
			timer_control[SOFTTIMER_NOWAKE].get_slice_value = bsp_get_slice_value_hrt;
		}
		wake_lock_init(&timer_control[SOFTTIMER_NOWAKE].wake_lock, WAKE_LOCK_SUSPEND, "softtimer_nowake");
	 }
	 bsp_trace(BSP_LOG_LEVEL_ERROR,BSP_MODU_SOFTTIMER,"softtimer init success\n");
	return BSP_OK;
}

s32 show_list(u32 wake_type)
{
	struct softtimer_list * timer = NULL;
	unsigned long flags = 0;
	u64 now_slice = 0;
	softtimer_print("softttimer wakeup %d times\n",timer_control[wake_type].wake_times);
	(void)timer_control[wake_type].get_curtime(&now_slice);
	softtimer_print("id name  expect_cb  now_slice  cb_cost  emerg\n");
	softtimer_print("----------------------------------------------------------------------------------\n");
	spin_lock_irqsave(&(timer_control[wake_type].timer_list_lock),flags); 
	list_for_each_entry(timer,&(timer_control[wake_type].timer_list_head),entry)
	{
		softtimer_print("%d %s  0x%x  0x%x  %d  %d\n",timer->timer_id,timer->name,(u32)timer->expect_cb_slice,(u32)now_slice,timer->run_cb_delta,timer->emergency);
	}
	 spin_unlock_irqrestore(&(timer_control[wake_type].timer_list_lock),flags); 
	return BSP_OK;
}

EXPORT_SYMBOL(bsp_softtimer_create);
EXPORT_SYMBOL(bsp_softtimer_delete);
EXPORT_SYMBOL(bsp_softtimer_modify);
EXPORT_SYMBOL(bsp_softtimer_add);
EXPORT_SYMBOL(bsp_softtimer_free);
EXPORT_SYMBOL(check_softtimer_support_type);
EXPORT_SYMBOL(show_list);
arch_initcall(bsp_softtimer_init);



