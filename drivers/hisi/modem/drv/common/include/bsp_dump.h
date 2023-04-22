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
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
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

#ifndef __BSP_DUMP_H__
#define __BSP_DUMP_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "osl_common.h"
#if defined(__KERNEL__) || defined(__VXWORKS__) || defined(__OS_RTOSCK__) ||defined(__OS_RTOSCK_SMP__)
#include "mdrv_om.h"
#else
#include "mdrv_om_common.h"
#endif
#include "drv_comm.h"
#include "bsp_trace.h"
#include "bsp_om.h"
#if defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
#include "sre_task.h"
#include "sre_exc.h"
#endif

#if defined(__KERNEL__)
#include <linux/stddef.h>
#endif

#include "bsp_om_enum.h"
#include "mdrv_errno.h"

/*****************************************************************************
  1 宏定义
*****************************************************************************/


/* dump侵入内核修改 begin */
#ifdef __KERNEL__
#define EXC_VEC_RESET       0x00    /* reset */
#define EXC_VEC_UNDEF       0x04    /* undefined instruction */
#define EXC_VEC_SWI         0x08    /* software interrupt */
#define EXC_VEC_PREFETCH    0x0c    /* prefetch abort */
#define EXC_VEC_DATA        0x10    /* data abort */
#define EXC_VEC_IRQ         0x18    /* interrupt */
#define EXC_VEC_FIQ         0x1C    /* fast interrupt */
#endif
/* dump侵入内核修改 end */

#define DUMP_INT_IN_FLAG                    0xAAAA
#define DUMP_INT_EXIT_FLAG                  0xBBBB
#define DUMP_INT_UNLOCK_FLAG                0xCCCC
#define DUMP_SAVE_SUCCESS                   0xA4A4A4A4
#define DUMP_TASK_INFO_SIZE                 0x200
#define DUMP_TASK_INFO_STACK_SIZE           (DUMP_TASK_INFO_SIZE - 32*4)



#define DUMP_FIELD_CPUINFO_SIZE             1024
#define TASK_NAME_LEN                       16
#define ARM_REGS_NUM                        17
#define DUMP_REG_SET_MAX                    0x1000

/*****************************************************************************
  2 枚举定义
*****************************************************************************/
typedef DUMP_SAVE_MOD_ENUM          dump_save_modid_t;

typedef enum{
    DUMP_SYSVIEW_TASKSWITCH       = 0,
    DUMP_SYSVIEW_INTSWITCH,
    DUMP_SYSVIEW_INTLOCK,
    DUMP_SYSVIEW_BUTT
}dump_sysview_t;

typedef enum
{
    DUMP_MBB    = 0,
    DUMP_PHONE,
    DUMP_PRODUCT_BUTT
}dump_product_type_t;

typedef enum
{
    DUMP_CCPU_ARCH_VSMA    = 0,
    DUMP_CCPU_ARCH_PSMA,
    DUMP_CCPU_ARCH_BUTT
}dump_ccpu_arch_t;

#define NVID_DUMP   (NV_ID_DRV_DUMP)


/* 枚举值与dump_file_cfg_s必须保持匹配 */
typedef enum
{
    MODEM_DUMP      = 0,
    MODME_SRAM,
    MODEM_SHARE,
    MODEM_DDR,
    LPHY_TCM,
    LPM3_TCM,
    AP_ETB,
    MODME_ETB,
    DUMP_FILE_BUTT
}dump_file_index_e;

typedef enum
{
    DUMP_MODE_NORMAL,
    DUMP_MODE_LLT,
}dump_mode_e;

/* dump文件结点，ioctl命令字 */
#define DUMP_CMD_SET_COUNT      0x1f000000  /* 配置最大保存份数 */
#define DUMP_CMD_SET_FILE       0x1f000001  /* 配置保存文件列表 */
#define DUMP_CMD_FLUSH          0x1f000002  /* 保存配置，写入NV */

typedef enum
{
    DUMP_SAVE_FILE_NORMAL = 0x0,
    DUMP_SAVE_FILE_NEED   = 0xAABBCCDD,
    DUMP_SAVE_FILE_END    = 0x5A5A5A5A
}dump_save_flag_t;

typedef enum
{
    DUMP_START_POWER_ON = 0,
    DUMP_START_REBOOT   = 0x5A5A1111,
    DUMP_START_EXCH     = 0x5A5A2222,
    DUMP_START_CRASH    = 0x5A5A3333
}dump_start_flag_e;

typedef enum _dump_reboot_cpu_e
{
    DUMP_CPU_APP     = 0x01000000,
    DUMP_CPU_COMM    = 0x02000000,
    DUMP_CPU_BBE     = 0x03000000,
    DUMP_CPU_LPM3    = 0x04000000,
    DUMP_CPU_IOM3    = 0x05000000,
    DUMP_CPU_HIFI    = 0x06000000,
    DUMP_CPU_TEEOS   = 0x07000000,
    DUMP_CPU_BUTTON  = 0x08000000

}dump_reboot_cpu_t;

typedef enum _dump_reboot_reason_e
{
    DUMP_REASON_NORMAL           = 0x0,
    DUMP_REASON_ARM              = 0x1,
    DUMP_REASON_STACKFLOW        = 0x2,
    DUMP_REASON_WDT              = 0x3,
    DUMP_REASON_PMU              = 0x4,
    DUMP_REASON_REBOOT           = 0x5,
    DUMP_REASON_NOC              = 0x6,
    DUMP_REASON_DMSS             = 0x7,
    DUMP_REASON_RST_FAIL         = 0x8,
    DUMP_REASON_RST_NOT_SUPPORT  = 0x9,
    DUMP_REASON_DLOCK            = 0x10,
    DUMP_REASON_UNDEF            = 0xff
}dump_reboot_reason_t;

typedef struct _dump_exc_contex
{
   dump_reboot_cpu_t     reboot_core;
   dump_reboot_reason_t  reboot_reason;
} dump_reboot_contex_s;

#define DUMP_LOAD_MAGIC     0xDDDD1234
typedef struct _dump_load_info_s
{
    u32 magic_num;      /* dump加载信息区标识，用于兼容之前版本 */
    u32 ap_ddr;         /* AP DDR加载地址 */
    u32 ap_share;       /* AP共享内存加载地址 */
    u32 ap_dump;        /* AP可维可测内存加载地址 */
    u32 ap_sram;        /* AP SRAM加载地址 */
    u32 ap_dts;         /* AP DTS加载地址 */
    u32 mdm_ddr;        /* MODEM DDR加载地址 */
    u32 mdm_share;      /* MODEM共享内存加载地址 */
    u32 mdm_dump;       /* MODEM可维可测内存加载地址 */
    u32 mdm_sram;       /* MODEM SRAM加载地址 */
    u32 mdm_dts;        /* MODEM DTS加载地址 */
    u32 lpm3_tcm0;      /* LPM3 TCM0加载地址 */
    u32 lpm3_tcm1;      /* LPM3 TCM1加载地址 */
}dump_load_info_t;

/*RTOSck里任务结构体*/
typedef struct tagListObject
{
    struct tagListObject *pstPrev;
    struct tagListObject *pstNext;
} LIST_OBJECT_S;

typedef struct tagMsgQHead
{
    u32 uwMsgNum;
    LIST_OBJECT_S stMsgList;
} MSG_QHead_S;

#if defined(__OS_RTOSCK__)
typedef struct tagPublicTskCB
{
    VOID               *pStackPointer;              // 当前任务的SP
    TSK_STATUS_T        usTaskStatus;               // 任务状态
    TSK_PRIOR_T         usPriority;                 // 任务的运行优先级
    UINT32              uwStackSizes;                // 任务栈大小
    UINT32              uwTopOfStack;               // 任务栈顶
    TSK_HANDLE_T        uwTaskPID;                  // 任务PID
    TSK_ENTRY_FUNC      pfnTaskEntry;               // 任务入口函数
    VOID               *pTaskSem;                   // 任务Pend的信号量指针
    INT32               swFsemCount;                // 快速信号量计数
    UINT32              auwArgs[4];                 // 任务的参数
#if (OS_HAVE_COPROCESSOR1 == YES)                   // 只有Tensilica平台才有该功能
    VOID               *pCpSaveAreaA;               // 矢量寄存器缓存地址A
    VOID               *pCpSaveAreaB;               // 矢量寄存器缓存地址B
#endif
    TSK_PRIOR_T         usOrigPriority;             // 任务的原始优先级
    UINT16              usStackCfgFlg;              // 任务栈配置标记
    UINT16              usQNum;                     // 消息队列数
    UINT16              usRecvQID;                  // 期望接收消息的QID
    UINT32              uwPrivateData;              // 私有数据
    MSG_QHead_S        *pstMsgQ;                    // 指向消息队列数组
    LIST_OBJECT_S       stPendList;                 // 信号量链表指针
    LIST_OBJECT_S       stTimerList;                // 任务延时链表指针
    LIST_OBJECT_S       stSemBList;                 // 持有互斥信号量链表
    UINT64              ullExpirationTick;          // 任务恢复的时间点(单位Tick)
    UINT32              uwEvent;                    // 任务事件
    UINT32              uwEventMask;                // 任务事件掩码
    UINT32              uwLastErr;                  // 任务记录的最后一个错误码
    UINT32              uwReserved;                 // 增加一个PAD，保证TCB 8字节对齐
} TSK_CB_S;
#elif  defined(__OS_RTOSCK_SMP__)
typedef struct tagPushablTskList
{
    UINT32 uwPrio;
    LIST_OBJECT_S stPrioList;
    LIST_OBJECT_S stNodeList;
}PUSHABL_TLIST;
typedef struct tagPublicTskCB
{
    VOID               *pStackPointer;              // 当前任务的SP
    TSK_STATUS_T        usTaskStatus;               // 任务状态
#if (OS_HARDWARE_PLATFORM == OS_CORTEX_RX)
    UINT16              uwReserved1;                // 不使用，保证可以对usTaskStatus进行32位的原子操作
#endif
    TSK_PRIOR_T         usPriority;                 // 任务的运行优先级
#if (OS_HARDWARE_PLATFORM == OS_CORTEX_RX)
    UINT16              usSpinLockCount;            // 任务持有的spinlock锁个数
    UINT32              uwLastSpinLock;             // 任务持有的最后一个spinlock
    volatile UINT32     uwTaskOperating;            // 任务正在进行的操作类型
    UINT32              uwReserved2;                // 保留
#endif
    UINT32              uwStackSize;                // 任务栈大小
    UINT32              uwTopOfStack;               // 任务栈顶
    TSK_HANDLE_T        uwTaskPID;                  // 任务PID：SMP下为任务OS内部索引,AMP下为核号 | 任务索引
    TSK_ENTRY_FUNC      pfnTaskEntry;               // 任务入口函数
    VOID               *pTaskSem;                   // 任务Pend的信号量指针
    INT32               swFsemCount;                // 快速信号量计数
    UINT32              auwArgs[4];                 // 任务的参数
#if (OS_OPTION_COPROCESSOR == YES)                  // 只有Tensilica平台才有该功能
    VOID               *pCpSaveAreaA;               // 矢量寄存器缓存地址A
    VOID               *pCpSaveAreaB;               // 矢量寄存器缓存地址B
#endif
    TSK_PRIOR_T         usOrigPriority;             // 任务的原始优先级
    UINT16              usStackCfgFlg;              // 任务栈配置标记
    UINT16              usQNum;                     // 消息队列数
    UINT16              usRecvQID;                  // 期望接收消息的QID
    UINT32              uwPrivateData;              // 私有数据
    MSG_QHead_S        *pstMsgQ;                    // 指向消息队列数组
    LIST_OBJECT_S       stPendList;                 // 信号量链表指针
    LIST_OBJECT_S       stTimerList;                // 任务延时链表指针
    LIST_OBJECT_S       stSemBList;                 // 持有互斥信号量链表
    UINT64              ullExpirationTick;          // 任务恢复的时间点(单位Tick)
    UINT32              uwEvent;                    // 任务事件
    UINT32              uwEventMask;                // 任务事件掩码
    UINT32              uwLastErr;                  // 任务记录的最后一个错误码
#if (OS_OPTION_SMP == YES)
    UINT32              uwTcbLock;                  // 任务tcb锁，核间操作需要上锁
    BOOL                bIsInterruptable;           // 任务特殊临界区保护标志
    UINT32              uwSMPReserve;               // reserved
    PUSHABL_TLIST       stPushAbleList;             // 可push队列链表
    UINT32              uwCoreAllowedMask;          // 该任务可以执行的核bitmap
    UINT32              uwNrCoresAllowed;           // 该任务可以执行的核个数
    UINT32              uwCoreID;                   // 该任务所处的rq挂接的核号
    BOOL                bIsOnRq;                    // 该任务是否在运行队列上(ready)
    UINT32              uwTskLoad;                  // 该任务的负载
    void               *pstScheClass;               // 该任务使用的调度类
#endif

    UINT32              uwReserved;                 // 增加一个PAD，保证TCB 8字节对齐
} TSK_CB_S;

#endif

typedef struct
{
    u32 task_id;            /*任务id*/
    u8  task_name[12];      /*任务名*/
}dump_task_info_s;


/*  CPSR R16
  31 30  29  28   27  26    7   6   5   4    3    2    1    0
----------------------------------------------------------------
| N | Z | C | V | Q | RAZ | I | F | T | M4 | M3 | M2 | M1 | M0 |
---------------------------------------------------------------- */

/*  REG
R0 R1 R2 R3 R4 R5 R6 R7 R8 R9 R10 R11 R12 R13/SP R14/LR R15/PC R16/CPSR
*/
typedef struct
{   
    u32 reboot_cpu;          /*0x00  */
    u32 reboot_context;     /*0x00  */
    u32 reboot_task;        /*0x04  */
    u32 reboot_task_tcb;    /*0x04  */
    u8  taskName[16];       /*0x08  */
    u32 reboot_int;         /*0x18  */
    u32 reboot_time;        /*0x84 */

    u32 modId;              /*0x1c  */
    u32 arg1;               /*0x20  */
    u32 arg2;               /*0x24  */
    u32 arg3;               /*0x28  */
    u32 arg3_length;        /*0x2c  */
    u32 vec;                /*0x30  */

    u32 cpu_max_num;        /*0x88 */
    u32 cpu_online_num;     /*0x88 */
    u8  version[32];        /*0xA0 */
    u8  compile_time[32];   /*0xB0 */
    u32 reboot_reason;      /*m3专用*/

}dump_base_info_t;





typedef struct
{
    u32 pid;
    u32 core;
    u32 entry;
    u32 status;
    u32 policy;
    u32 priority;
    u32 stack_base;
    u32 stack_end;
    u32 stack_high;
    u32 stack_current;
    u8  name[16];
    u32 regs[17];
    u32 offset;
    u32 rsv[1];
    char dump_stack[DUMP_TASK_INFO_STACK_SIZE];
} dump_task_info_t;



typedef enum _dump_reboot_ctx_e
{
    DUMP_CTX_TASK        = 0x0,
    DUMP_CTX_INT         = 0x1
}dump_reboot_ctx_t;


typedef struct
{   
    u32 cpu_state;                      /*cpu 状态*/
    dump_reboot_ctx_t current_ctx;      /*cpu上下文*/
    u32 current_int;                    /*当前的中断*/	
    u32 current_task;                   /*当前正在运行的任务*/
    u8  taskName[TASK_NAME_LEN];        /*任务名*/
    u32 regSet[ARM_REGS_NUM];           /*寄存器信息*/
    u8  callstack[DUMP_FIELD_CPUINFO_SIZE - sizeof(dump_reboot_ctx_t) - sizeof(u32)- sizeof(u32)- TASK_NAME_LEN - sizeof(u32) * ARM_REGS_NUM];
}dump_cpu_info_t;



typedef struct
{
    u32 maxNum;
    u32 front;
    u32 rear;
    u32 num;
    u32 data[1];
} dump_queue_t;



typedef enum{
    TASK_SCHED_NORMAL = 0,
    TASK_SCHED_FIFO,
    TASK_SCHED_RR,
    TASK_SCHED_BATCH,
    TASK_SCHED_IDLE
}dump_sched_policy_t;

typedef bool (*exc_hook)(void * param);

#define CCORE               "CP"
#define ACORE               "AP"
#define UNKNOW_CORE         "UNKNOW"
#define UNKNOW              "UNKNOW"
#define DATA_ABORT          "abort"
#define WDT                 "wdg"
#define NOARMAL             "syserr"
#define UNDEFFINE           "undefine"
#define NOC                 "noc"
#define DMSS                "dmss"
#define RST_FAIL            "rst_fail"
#define RST_NOT_SUPPORT     "rst_not_support"
#define DLOCK               "mdm_bus_err"

#define DUMP_MODULE_NAME_LEN (12)

#define DUMP_REBOOT_INT "Interrupt_No"
#define DUMP_REBOOT_TASK "task_name"

#define DUMP_MODID_OFFSET        (offsetof(dump_base_info_t,modId))
#define DUMP_TASK_NAME_OFFSET    (offsetof(dump_base_info_t,taskName))


/*****************************************************************************
  3 函数声明
*****************************************************************************/
#ifdef ENABLE_BUILD_OM

void system_error (u32 mod_id, u32 arg1, u32 arg2, char *data, u32 length);
s32  bsp_dump_init(void);
int bsp_dump_init_phase2(void);
dump_handle bsp_dump_register_hook(char * name, dump_hook func);
s32  bsp_dump_unregister_hook(dump_handle handle);
u8*  bsp_dump_register_field(u32 mod_id, char * name, void * virt_addr, void * phy_addr, u32 length, u16 version);
u8*  bsp_dump_get_field_addr(u32 field_id);
u8 * bsp_dump_get_field_phy_addr(u32 field_id);
void bsp_dump_set_hso_conn_flag(u32 flag);
s32  bsp_dump_register_sysview_hook(dump_sysview_t mod_id, dump_hook func);
void bsp_dump_trace_stop(void);
int bsp_dump_save_all_task_name(void);
void bsp_om_save_reboot_log(const char * func_name, const void* caller);
/* dump侵入内核修改 begin */
void adump_set_exc_vec(u32 vec);
void dump_int_enter(u32 dir, u32 irq_num);
void dump_int_exit(u32 dir, u32 irq_num);
int  dump_stack_trace_print(unsigned long where);
/* dump侵入内核修改 end */
s32  dump_exc_register(exc_hook func);
void bsp_ccore_wdt_register_hook(void);
void bsp_dump_get_reset_modid(u32 reason, u32 reboot_modid, u32 * modId);
s32 bsp_dump_mark_voice(u32 flag);
void bsp_dump_get_reboot_contex(u32* core ,u32* reason);

dump_product_type_t dump_get_product_type(void);

#else

static void inline system_error (u32 mod_id, u32 arg1, u32 arg2, char *data, u32 length)
{
    return ;
}
static s32 inline bsp_dump_init(void)
{
    return 0;
}

static dump_handle inline bsp_dump_register_hook(char * name, dump_hook func)
{
    return 0;
}

static s32 inline bsp_dump_unregister_hook(dump_handle handle)
{
    return 0;
}

static inline u8 * bsp_dump_register_field(u32 mod_id, char * name, void * virt_addr, void * phy_addr, u32 length, u16 version)
{
    return BSP_NULL;
}

static inline u8 * bsp_dump_get_field_addr(u32 field_id)
{
    return 0;
}

static inline u8 * bsp_dump_get_field_phy_addr(u32 field_id)
{
    return 0;
}

static s32 inline bsp_dump_register_sysview_hook(dump_sysview_t mod_id, dump_hook func)
{
    return 0;
}


static void inline bsp_dump_trace_stop(void)
{
    return;
}

static int inline bsp_dump_save_all_task_name(void)
{
    return 0;
}

static void inline bsp_om_save_reboot_log(const char * func_name,  const void* caller)
{
    return;
}



static s32 inline dump_exc_register(exc_hook func)
{
    return 0;
}
static void inline bsp_ccore_wdt_register_hook(void)
{
    return;
}



#endif

#if defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
BOOL bsp_rtosck_exc_hook(EXC_INFO_S *pstExcInfo);
#endif

/*****************************************************************************
  4 错误码声明
*****************************************************************************/
#define BSP_ERR_DUMP_BASE               (int)(0x80000000 | (BSP_DEF_ERR(BSP_MODU_DUMP, 0)))
#define BSP_ERR_DUMP_INIT_FAILED        (BSP_ERR_DUMP_BASE + 0x1)
#define BSP_ERR_DUMP_INVALID_MODULE     (BSP_ERR_DUMP_BASE + 0x1)
#define BSP_ERR_DUMP_INVALID_FILE       (BSP_ERR_DUMP_BASE + 0x2)
#define BSP_ERR_DUMP_INVALID_PARAM      (BSP_ERR_DUMP_BASE + 0x3)
#define BSP_ERR_DUMP_NO_BUF             (BSP_ERR_DUMP_BASE + 0x4)
#define BSP_ERR_DUMP_SOCP_ERR           (BSP_ERR_DUMP_BASE + 0x5)


#ifdef __cplusplus
}
#endif


#endif


