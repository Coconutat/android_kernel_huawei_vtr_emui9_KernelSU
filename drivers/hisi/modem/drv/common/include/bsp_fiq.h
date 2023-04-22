#ifndef _BSP_FIQ_H
#define _BSP_FIQ_H

#include <product_config.h>
#include <drv_comm.h>
#include <bsp_shared_ddr.h>
#include <bsp_slice.h>
#include <osl_bio.h>
#if defined(__OS_RTOSCK__) ||defined(__OS_RTOSCK_SMP__)
#include <sre_exc.h>
#endif
#ifdef CCPU_CORE_NUM
#define SHM_FIQ_BASE_ADDR                              (SHM_OFFSET_CCORE_FIQ + (unsigned long)SHM_BASE_ADDR)
#define SHM_FIQ_BARRIER                	               (SHM_FIQ_BASE_ADDR)
#define SHM_FIQ_STATUS_ADDR                            (SHM_FIQ_BARRIER + (unsigned long)0x4)
#define SHM_FIQ_CLEAR_ADDR                             (SHM_FIQ_STATUS_ADDR + (unsigned long)(0x4 *CCPU_CORE_NUM))
#define SHM_FIQ_CP_TIMER_ADDR                          (SHM_FIQ_CLEAR_ADDR + (unsigned long)(0x4 *CCPU_CORE_NUM))
#define SHM_FIQ_TOTAL_SEND_CNT                         (SHM_FIQ_CP_TIMER_ADDR + (unsigned long)(0x4 *CCPU_CORE_NUM))
#define SHM_FIQ_TOTAL_RECIVE_CNT                       (SHM_FIQ_TOTAL_SEND_CNT + (unsigned long)(0x4 *CCPU_CORE_NUM))
#define SHM_FIQ_INFO_ADDR                              (SHM_FIQ_TOTAL_RECIVE_CNT + (unsigned long)(0x4 *CCPU_CORE_NUM))
#else
#define SHM_FIQ_BASE_ADDR                              (SHM_OFFSET_CCORE_FIQ + (unsigned long)SHM_BASE_ADDR)
#define SHM_FIQ_BARRIER                	               (SHM_FIQ_BASE_ADDR)
#define SHM_FIQ_STATUS_ADDR                            (SHM_FIQ_BARRIER + (unsigned long)0x4)
#define SHM_FIQ_CLEAR_ADDR                             (SHM_FIQ_STATUS_ADDR + (unsigned long)0x4)
#define SHM_FIQ_CP_TIMER_ADDR                          (SHM_FIQ_CLEAR_ADDR + (unsigned long)0x4)
#define SHM_FIQ_TOTAL_SEND_CNT                         (SHM_FIQ_CP_TIMER_ADDR + (unsigned long)0x4)
#define SHM_FIQ_TOTAL_RECIVE_CNT                       (SHM_FIQ_TOTAL_SEND_CNT + (unsigned long)0x4)
#define SHM_FIQ_INFO_ADDR                              (SHM_FIQ_TOTAL_RECIVE_CNT + (unsigned long)0x4)
#endif
#define FIQ_TRIGGER_TAG                     		   (0xFFFFFFF0)

/*优先级高的先处理，优先级数字越小越高*/
typedef enum __fiq_num
{
	FIQ_RESET = 0,
	FIQ_DUMP,
	FIQ_MAX
}fiq_num;

enum fiq_handler_return_value
{
	FIQ_RETURN_RUNNING = 0,                       /*FIQ钩子退出后，系统继续运行*/
	FIQ_ENTER_LOOP_1_NO_RETRUN = 1      , /*FIQ钩子退出后，系统进行死循环，不响应FIQ*/
	FIQ_ENTER_WFE_NO_RETRUN =  2            /*FIQ钩子退出后，系统进入WFE循环，响应FIQ*/
};

typedef int (* FIQ_PROC_FUNC)(void *);

#if defined(__OS_RTOSCK__) ||defined(__OS_RTOSCK_SMP__)
/**
 * @brief 创建FIQ
 *
 * @par 描述:
 * 注册FIQ处理函数
 *
 * @attention
 * </ul>允许多次调用，以最后一次注册为准
  * </ul>FIQ只连接给了A9，不具备唤醒能力，A9下电场景下发送FIQ会发生异常
 *用户注册的钩子返回值定义见enum fiq_handler_return_value
 * 
 **/ 

int request_fiq(fiq_num fiq_num_t, FIQ_PROC_FUNC pfnHandler,void * uwArg);

/**
 * @brief 删除创建FIQ
 *
 * @par 描述:
 *去 注册FIQ处理函数
 *
 * @attention
 * </ul>调用一次即删除
 *
 * 
 **/ 
int free_fiq(fiq_num fiq_num_t);

/**
 * @brief 处理FIQ
 *
 * @par 描述:
 * FIQ处理函数
 *
 * @attention
 * </ul>由异常处理钩子在FIQ场景下调用
 *
 * 
 **/ 

 UINT32 fiq_handler(EXC_INFO_S *pstExcInfo);
/**
 * @brief 处理mem debug info save
 *
 * @par 描述:
 * dump save os debug info
 *
 * @attention
 *
 * 
 **/
void os_mem_info_save(void);

#elif defined __KERNEL__
/**
 * @brief 触发FIQ
 *
 * @par 描述:
 * 触发FIQ中断
 *
 * @attention
 * </ul>
  * </ul>FIQ只连接给了A9，不具备唤醒能力，A9下电场景下发送FIQ会发生异常
 * 
 **/

int bsp_send_cp_fiq(fiq_num fiq_num_t);
#else
static inline void os_mem_info_save(void){}
#endif
#endif

