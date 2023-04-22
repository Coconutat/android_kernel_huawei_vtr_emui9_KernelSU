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

/*****************************************************************************/
/*                                                                           */
/*                Copyright 1999 - 2003, Huawei Tech. Co., Ltd.              */
/*                           ALL RIGHTS RESERVED                             */
/*                                                                           */
/* FileName: v_timer.c                                                       */
/*                                                                           */
/* Author: Yang Xiangqian                                                    */
/*                                                                           */
/* Version: 1.0                                                              */
/*                                                                           */
/* Date: 2006-10                                                             */
/*                                                                           */
/* Description: implement timer                                              */
/*                                                                           */
/* Others:                                                                   */
/*                                                                           */
/* History:                                                                  */
/* 1. Date:                                                                  */
/*    Author:                                                                */
/*    Modification: Create this file                                         */
/*                                                                           */
/* 2. Date: 2006-10                                                          */
/*    Author: Xu Cheng                                                       */
/*    Modification: Standardize code                                         */
/*                                                                           */
/*****************************************************************************/

#include "vos.h"
#include "v_IO.h"
#include "v_private.h"
#include "mdrv.h"

/* LINUX不支持 */


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_V_RTC_TIMER_C


/* the state of Timer */
#define RTC_TIMER_CTRL_BLK_RUNNIG                           0
#define RTC_TIMER_CTRL_BLK_PAUSE                            1
#define RTC_TIMER_CTRL_BLK_STOP                             2

/* the tag of  RTC_TimerCtrlBlkFree */
#define THE_FIRST_RTC_TIMER_TAG                             1
#define THE_SECOND_RTC_TIMER_TAG                            2

/* can't make a mistake,add threshold */
#define RTC_TIMER_CHECK_PRECISION                           10

#define RTC_SOC_TIMER_CHECK_PRECISION                       100
#define RTC_TIMER_CHECK_LONG_PRECISION                      32768

/* the tag of  RTC_Add_Timer_To_List */
enum
{
    VOS_ADD_TIMER_TAG_1,
    VOS_ADD_TIMER_TAG_2,
    VOS_ADD_TIMER_TAG_3,
    VOS_ADD_TIMER_TAG_4,
    VOS_ADD_TIMER_TAG_5,
    VOS_ADD_TIMER_TAG_BUTT
};

/* timer task's stack size */
#define RTC_TIMER_TASK_STACK_SIZE                           3072

/* 1s->1000ms */
#define RTC_ONE_SECOND                                      1000

/* 32.768K */
#define RTC_PRECISION_CYCLE_LENGTH                          0.32768


#define RTC_TIMER_NORMAL_COUNT                              5

#ifndef BIT
#define BIT(x)                  ((unsigned)0x1 << (x))
#endif

typedef struct RTC_TIMER_CONTROL_STRU
{
    VOS_UINT32      TimerId;/* timer ID */
    VOS_UINT32      ulUsedFlag;/* whether be used or not */
    VOS_PID         Pid;/* who allocate the timer */
    VOS_UINT32      Name;/* timer's name */
    VOS_UINT32      Para;/* timer's paremate */
    VOS_UINT8       aucRsv[4];
    REL_TIMER_FUNC  CallBackFunc;/* timer's callback function */
    HTIMER          *phTm;/* user's pointer which point the real timer room */
    VOS_UINT32      TimeOutValueInMilliSeconds;
    VOS_UINT32      TimeOutValueInCycle;
    struct RTC_TIMER_CONTROL_STRU *next;
    struct RTC_TIMER_CONTROL_STRU *previous;
    VOS_UINT8       Mode;/* timer's mode */
    VOS_UINT8       State;/* timer's state */
    VOS_UINT8       Reserved[2];/* for 4 byte aligned */
    VOS_UINT32      ulPrecision;/* record only */
    VOS_UINT32      ulPrecisionInCycle;/* unit is 32K cycle */

    VOS_UINT32      ulAllocTick;/* CPU tick of block allocation */
    VOS_UINT32      ulFreeTick;/* CPU tick of block free */
    VOS_UINT32      ulFileID;/* alloc file ID */
    VOS_UINT32      ulLineNo;/* alloc line no. */
    VOS_UINT32      ulBackUpTimeOutValueInCycle;
    VOS_UINT32      ulBackUpTimerId;/* timer ID */
} RTC_TIMER_CONTROL_BLOCK;


/* Added by g47350 for DRX timer Project, 2012/11/5, begin */
typedef struct DRX_TIMER_CONTROL_STRU
{
    VOS_UINT32      ulUsedFlag;                     /* whether be used or not */
    VOS_PID         ulPid;                          /* who allocate the timer */
    VOS_UINT32      ulName;                         /* timer's name */
    VOS_UINT32      ulPara;                         /* timer's paremate */
    HTIMER         *phTm;                           /* user's pointer which point the real timer room */
    VOS_UINT32      ulTimeOutValueInMilliSeconds;   /* timer's length(ms) */
    VOS_UINT32      ulTimeOutValueSlice;            /* timer's length(32k) */
    VOS_UINT32      ulTimeEndSlice;                 /* the end slice time of timer */

    VOS_UINT32      ulAllocTick;                    /* CPU tick of block allocation */
    VOS_UINT32      ulFileID;                       /* alloc file ID */
    VOS_UINT32      ulLineNo;                       /* alloc line no. */
} DRX_TIMER_CONTROL_BLOCK;
/* Added by g47350 for DRX timer Project, 2012/11/5, end */

/* the number of task's control block */
VOS_UINT32                RTC_TimerCtrlBlkNumber;

/* the number of free task's control block */
VOS_UINT32                RTC_TimerIdleCtrlBlkNumber;

/* the start address of task's control block */
RTC_TIMER_CONTROL_BLOCK   *RTC_TimerCtrlBlk;

/* the start address of free task's control block list */
RTC_TIMER_CONTROL_BLOCK   *RTC_TimerIdleCtrlBlk;

/* the begin address of timer control block */
VOS_VOID                  *RTC_TimerCtrlBlkBegin;

/* the end address of timer control block */
VOS_VOID                  *RTC_TimerCtrlBlkEnd;

/* the head of the running timer list */
RTC_TIMER_CONTROL_BLOCK   *RTC_Timer_head_Ptr = VOS_NULL_PTR;

/* the task ID of timer's task */
VOS_UINT32                RTC_TimerTaskId;

/* the Min usage of timer */
VOS_UINT32                RTC_TimerMinTimerIdUsed;

/* the queue will be given when RTC's interrupt occures */
VOS_UINT32                g_ulRTCTaskQueueId;

/*record start value */
VOS_UINT32                RTC_Start_Value = ELAPESD_TIME_INVAILD;

#define RTC_TIMER_CTRL_BUF_SIZE (sizeof(RTC_TIMER_CONTROL_BLOCK) * RTC_MAX_TIMER_NUMBER )

VOS_CHAR g_acRtcTimerCtrlBuf[RTC_TIMER_CTRL_BUF_SIZE];

/* 循环记录SOC Timer的启停记录 */
enum
{
    RTC_SOC_TIMER_SEND_ERR = 0xfffffffd,
    RTC_SOC_TIMER_EXPIRED = 0xfffffffe,
    RTC_SOC_TIMER_STOPED = 0xffffffff
};

VOS_UINT32  g_ulRtcSocTimerDebugInfoSuffix = 0;

RTC_SOC_TIMER_DEBUG_INFO_STRU g_astRtcSocTimerDebugInfo[RTC_MAX_TIMER_NUMBER];

/* 记录 RTC timer 可维可测信息 */
VOS_TIMER_SOC_TIMER_INFO_STRU g_stRtcSocTimerInfo;

/* Added by g47350 for DRX timer Project, 2012/11/5, begin */

/* the array of DRX timer's control block */
DRX_TIMER_CONTROL_BLOCK   g_astDRXTimerCtrlBlk[DRX_TIMER_MAX_NUMBER];

/* the semaphore will be given when DRX's interrupt occures */
VOS_SEM                   g_ulDRXSem;

/* the task ID of DRX timer's task */
VOS_UINT32                g_ulDRXTimerTaskId;

/* Added by g47350 for DRX timer Project, 2012/11/5, end */


#define VOS_RTC_TIMER_ID  (TIMER_ACPU_OSA_ID)


VOS_VOID VOS_ShowUsed32KTimerInfo( VOS_VOID );


extern VOS_VOID OM_RecordInfoEnd(VOS_EXC_DUMP_MEM_NUM_ENUM_UINT32 enNumber);
extern VOS_VOID OM_RecordInfoStart(VOS_EXC_DUMP_MEM_NUM_ENUM_UINT32 enNumber, VOS_UINT32 ulSendPid, VOS_UINT32 ulRcvPid, VOS_UINT32 ulMsgName);
extern VOS_VOID PAMOM_DrxTimer_Event(VOS_VOID *pData, VOS_UINT32 ulLength);

/* Just for Sparse checking. */
VOS_VOID StartHardTimer( VOS_UINT32 value );
VOS_VOID StopHardTimer(VOS_VOID);
VOS_VOID RTC_DualTimerIsrEntry(VOS_VOID);
VOS_INT32 RTC_DualTimerIsr(VOS_INT lPara);
RTC_TIMER_CONTROL_BLOCK *RTC_TimerCtrlBlkGet(VOS_UINT32 ulFileID, VOS_INT32 usLineNo);
VOS_UINT32 RTC_TimerCtrlBlkFree(RTC_TIMER_CONTROL_BLOCK *Ptr, VOS_UINT8 ucTag );
VOS_UINT32 GetHardTimerElapsedTime(VOS_VOID);
VOS_VOID RTC_TimerTaskFunc( VOS_UINT32 Para0, VOS_UINT32 Para1, VOS_UINT32 Para2, VOS_UINT32 Para3 );
VOS_VOID RTC_Add_Timer_To_List( RTC_TIMER_CONTROL_BLOCK  *Timer);
VOS_VOID RTC_Del_Timer_From_List( RTC_TIMER_CONTROL_BLOCK  *Timer);
VOS_VOID ShowRtcTimerLog( VOS_VOID );

typedef struct RTC_TIMER_PMLOG_STRU
{
    PM_LOG_COSA_PAM_ENUM_UINT32     stPamType;
    VOS_UINT32                      ulPid;
    VOS_UINT32                      ulTimerId;
} RTC_TIMER_PMLOG;

VOS_UINT32 g_ulTimerlpm = VOS_FALSE;

VOS_UINT32 VOS_TimerLpmCb(VOS_INT x)
{
    g_ulTimerlpm = VOS_TRUE;

    return VOS_OK;
}


/*****************************************************************************
 Function   : RTC_MUL_32_DOT_768
 Description: 乘以32.768
 Input      : ulValue -- timer's value.uint is 32K cycle.
              ulFileID -- 文件ID
              usLineNo -- 行号
 Return     : 与32.768做乘法的结果
 Other      :
 *****************************************************************************/
VOS_UINT32 RTC_MUL_32_DOT_768(VOS_UINT32 ulValue,VOS_UINT32 ulFileID,
                                 VOS_INT32 usLineNo)
{
    VOS_UINT32 ulProductHigh;
    VOS_UINT32 ulProductLow;
    VOS_UINT32 ulQuotientHigh;
    VOS_UINT32 ulQuotientLow;
    VOS_UINT32 ulRemainder;
    VOS_UINT32 ulReturn;

    ulReturn = VOS_64Multi32(0, ulValue, 32768, &ulProductHigh, &ulProductLow);
    if ( VOS_OK != ulReturn )
    {
        VOS_ProtectionReboot(RTC_FLOAT_MUL_32_DOT_768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    ulReturn = VOS_64Div32(ulProductHigh, ulProductLow, 1000, &ulQuotientHigh, &ulQuotientLow, &ulRemainder);
    if ( VOS_OK != ulReturn )
    {
        VOS_ProtectionReboot(RTC_FLOAT_MUL_32_DOT_768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    if ( VOS_NULL != ulQuotientHigh )
    {
        VOS_ProtectionReboot(RTC_FLOAT_MUL_32_DOT_768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    return ulQuotientLow;
}

/*****************************************************************************
 Function   : RTC_DIV_32_DOT_768
 Description: 除以32.768
 Input      : ulValue -- timer's value.uint is 32K cycle.
              ulFileID -- 文件ID
              usLineNo -- 行号
 Return     : 与32.768做除法的结果
 Other      :
 *****************************************************************************/
VOS_UINT32 RTC_DIV_32_DOT_768(VOS_UINT32 ulValue,VOS_UINT32 ulFileID,
                                 VOS_INT32 usLineNo)
{

    VOS_UINT32 ulProductHigh;
    VOS_UINT32 ulProductLow;
    VOS_UINT32 ulQuotientHigh;
    VOS_UINT32 ulQuotientLow;
    VOS_UINT32 ulRemainder;
    VOS_UINT32 ulReturn;

    ulReturn = VOS_64Multi32(0, ulValue, 1000, &ulProductHigh, &ulProductLow);
    if ( VOS_OK != ulReturn )
    {
        VOS_ProtectionReboot(RTC_FLOAT_DIV_32_DOT_768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    ulReturn = VOS_64Div32(ulProductHigh, ulProductLow, 32768, &ulQuotientHigh, &ulQuotientLow, &ulRemainder);
    if ( VOS_OK != ulReturn )
    {
        VOS_ProtectionReboot(RTC_FLOAT_DIV_32_DOT_768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    if ( VOS_NULL != ulQuotientHigh )
    {
        VOS_ProtectionReboot(RTC_FLOAT_DIV_32_DOT_768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    return ulQuotientLow;

}

/*****************************************************************************
 Function   : RTC_MUL_DOT_32768
 Description: 乘以0.32768
 Input      : ulValue -- timer's value.uint is 32K cycle.
              ulFileID -- 文件ID
              usLineNo -- 行号
 Return     : 与0.32768做乘法的结果
 Other      :
 *****************************************************************************/
VOS_UINT32 RTC_MUL_DOT_32768(VOS_UINT32 ulValue,VOS_UINT32 ulFileID,
                                 VOS_INT32 usLineNo)
{
    VOS_UINT32 ulProductHigh;
    VOS_UINT32 ulProductLow;
    VOS_UINT32 ulQuotientHigh;
    VOS_UINT32 ulQuotientLow;
    VOS_UINT32 ulRemainder;
    VOS_UINT32 ulReturn;

    ulReturn = VOS_64Multi32(0, ulValue, 32768, &ulProductHigh, &ulProductLow);
    if ( VOS_OK != ulReturn )
    {
        VOS_ProtectionReboot(RTC_FLOAT_MUL_DOT_32768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    ulReturn = VOS_64Div32(ulProductHigh, ulProductLow, 100000, &ulQuotientHigh, &ulQuotientLow, &ulRemainder);
    if ( VOS_OK != ulReturn )
    {
        VOS_ProtectionReboot(RTC_FLOAT_MUL_DOT_32768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    if ( VOS_NULL != ulQuotientHigh )
    {
        VOS_ProtectionReboot(RTC_FLOAT_MUL_DOT_32768, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)&ulValue, sizeof(ulValue));
        return VOS_ERR;
    }

    return ulQuotientLow;
}


/*****************************************************************************
 Function   : RTC_DebugSocInfo
 Description: record the track of Soc timer
 Input      :
 Return     :
 Other      :
 *****************************************************************************/
VOS_VOID RTC_DebugSocInfo(VOS_UINT32 ulAction, VOS_UINT32 ulSlice)
{
    g_astRtcSocTimerDebugInfo[g_ulRtcSocTimerDebugInfoSuffix].ulAction = ulAction;
    g_astRtcSocTimerDebugInfo[g_ulRtcSocTimerDebugInfoSuffix].ulSlice = ulSlice;

    g_ulRtcSocTimerDebugInfoSuffix++;

    if ( RTC_MAX_TIMER_NUMBER <= g_ulRtcSocTimerDebugInfoSuffix )
    {
        g_ulRtcSocTimerDebugInfoSuffix = 0;
    }
}


/*****************************************************************************
 Function   : RTC_DualTimerIsrEntry
 Description: handle timer's list
 Input      :
 Return     :
 Other      :
 *****************************************************************************/
VOS_VOID RTC_DualTimerIsrEntry(VOS_VOID)
{
    RTC_TIMER_CONTROL_BLOCK     *head_Ptr;
    /* the timer control which expire */
    RTC_TIMER_CONTROL_BLOCK     *RTC_TimerCtrlBlkCurrent;
    /* the timer head control which expire */
    RTC_TIMER_CONTROL_BLOCK     *RTC_TimerCtrlBlkexpired = VOS_NULL_PTR;
    RTC_TIMER_CONTROL_BLOCK     *RTC_TimerCtrlBlkexpiredTail = VOS_NULL_PTR;
    VOS_ULONG                   ulLockLevel;
    VOS_UINT32                  ulTempCount;
    VOS_UINT32                  ulElapsedCycles;


    /*intLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosTimerSpinLock, ulLockLevel);

    ulElapsedCycles = RTC_Start_Value;
    RTC_Start_Value = ELAPESD_TIME_INVAILD;

    if (VOS_NULL_PTR == RTC_Timer_head_Ptr)
    {
        /*VOS_Splx(intLockLevel);*/
        VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

        return;
    }

    head_Ptr = RTC_Timer_head_Ptr;

    /* sub timer value */
    head_Ptr->TimeOutValueInCycle -= ulElapsedCycles;

    ulTempCount = 0;

    /* check the left timer */
    while ( ( VOS_NULL_PTR != head_Ptr )
        && ( 0 == head_Ptr->TimeOutValueInCycle ) )
    {
        ulTempCount++;
        if ( RTC_TimerCtrlBlkNumber < ulTempCount )
        {
            /*VOS_Splx(intLockLevel);*/
            VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

            return;
        }

        RTC_TimerCtrlBlkCurrent = head_Ptr;

        *(head_Ptr->phTm) = VOS_NULL_PTR;
        /*head_Ptr->State = RTC_TIMER_CTRL_BLK_STOP;*/

        head_Ptr = RTC_TimerCtrlBlkCurrent->next;

        RTC_TimerCtrlBlkCurrent->next = VOS_NULL_PTR;
        if ( VOS_NULL_PTR == RTC_TimerCtrlBlkexpired )
        {
            RTC_TimerCtrlBlkexpired = RTC_TimerCtrlBlkCurrent;
            RTC_TimerCtrlBlkexpiredTail = RTC_TimerCtrlBlkCurrent;
        }
        else
        {
            /*lint -e613 */
            RTC_TimerCtrlBlkexpiredTail->next = RTC_TimerCtrlBlkCurrent;/* [false alarm]: 屏蔽Fortify 错误 */
            /*lint +e613 */
            RTC_TimerCtrlBlkexpiredTail = RTC_TimerCtrlBlkCurrent;
        }

        RTC_Timer_head_Ptr = head_Ptr;
    }

    if ( VOS_NULL_PTR != RTC_Timer_head_Ptr )
    {
        RTC_Timer_head_Ptr->previous = VOS_NULL_PTR;

        /* 上面已经把为0的都过滤了，这里不会再有为0的 */
        if (0 == RTC_Timer_head_Ptr->TimeOutValueInCycle)
        {
            RTC_Timer_head_Ptr->TimeOutValueInCycle += 1;
        }

        StartHardTimer(RTC_Timer_head_Ptr->TimeOutValueInCycle);
    }

    /*VOS_Splx(intLockLevel);*/
    VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

    if ( VOS_NULL_PTR != RTC_TimerCtrlBlkexpired )
    {
        if (VOS_OK != VOS_FixedQueueWriteDirect(g_ulRTCTaskQueueId, (VOS_VOID *)RTC_TimerCtrlBlkexpired, VOS_NORMAL_PRIORITY_MSG))
        {
            g_stRtcSocTimerInfo.ulExpiredSendErrCount++;
            g_stRtcSocTimerInfo.ulExpiredSendErrSlice   = VOS_GetSlice();

            RTC_DebugSocInfo((VOS_UINT32)RTC_SOC_TIMER_SEND_ERR, VOS_GetSlice());
        }
    }

    return;
}

/*****************************************************************************
 Function   : RTC_DualTimerIsr
 Description: ISR of DualTimer
 Input      :
 Return     :
 Other      :
 *****************************************************************************/
VOS_INT32 RTC_DualTimerIsr(VOS_INT lPara)
{
    VOS_UINT32      ulCurrentSlice;

    g_stRtcSocTimerInfo.ulExpireCount++;

    ulCurrentSlice = VOS_GetSlice();

    if ( (ulCurrentSlice - g_stRtcSocTimerInfo.ulStartSlice) < RTC_Start_Value )
    {
        g_stRtcSocTimerInfo.ulExpiredShortErrCount++;
        g_stRtcSocTimerInfo.ulExpiredShortErrSlice  = VOS_GetSlice();
    }

    if ( (ulCurrentSlice - g_stRtcSocTimerInfo.ulStartSlice) > (RTC_Start_Value + RTC_SOC_TIMER_CHECK_PRECISION) )
    {
        g_stRtcSocTimerInfo.ulExpiredLongErrCount++;
        g_stRtcSocTimerInfo.ulExpiredLongErrSlice  = VOS_GetSlice();
    }

    if ( VOS_OK != mdrv_timer_stop((unsigned int)lPara) )
    {
        g_stRtcSocTimerInfo.ulStopErrCount++;
    }

    RTC_DebugSocInfo((VOS_UINT32)RTC_SOC_TIMER_EXPIRED, ulCurrentSlice);

    RTC_DualTimerIsrEntry();

    return 0;
}

/*****************************************************************************
 Function   : StopHardTimer
 Description: stop hard timer
 Input      :
 Return     :
 Other      :
 *****************************************************************************/
VOS_VOID StopHardTimer(VOS_VOID)
{
    g_stRtcSocTimerInfo.ulStopCount++;

    if ( VOS_OK != mdrv_timer_stop(VOS_RTC_TIMER_ID) )
    {
        g_stRtcSocTimerInfo.ulStopErrCount++;
    }

    RTC_Start_Value = ELAPESD_TIME_INVAILD;

    RTC_DebugSocInfo((VOS_UINT32)RTC_SOC_TIMER_STOPED, VOS_GetSlice());

    return;
}

/*****************************************************************************
 Function   : StartHardTimer
 Description: start hard timer
 Input      : value -- timer's value.uint is 32K cycle.
 Return     :
 Other      :
 *****************************************************************************/
VOS_VOID StartHardTimer( VOS_UINT32 value )
{
    g_stRtcSocTimerInfo.ulStartCount++;

    StopHardTimer();

    g_stRtcSocTimerInfo.ulStartSlice = VOS_GetSlice();

    RTC_Start_Value = value;

    if ( VOS_OK != mdrv_timer_start(VOS_RTC_TIMER_ID, (FUNCPTR_1)RTC_DualTimerIsr, VOS_RTC_TIMER_ID, value, TIMER_ONCE_COUNT,TIMER_UNIT_NONE) )
    {
        g_stRtcSocTimerInfo.ulStartErrCount++;
    }

    RTC_DebugSocInfo(value, g_stRtcSocTimerInfo.ulStartSlice);

    return;
}

/*****************************************************************************
 Function   : GetHardTimerElapsedTime
 Description: get the elapsed time from hard timer
 Input      :
 Return     :
 Other      :
 *****************************************************************************/
VOS_UINT32 GetHardTimerElapsedTime(VOS_VOID)
{
    VOS_UINT32 ulTempValue = 0;

    if ( ELAPESD_TIME_INVAILD == RTC_Start_Value )
    {
        return 0;
    }

    if ( VOS_OK != mdrv_timer_get_rest_time(VOS_RTC_TIMER_ID, TIMER_UNIT_NONE, (VOS_UINT*)&ulTempValue) )
    {
        g_stRtcSocTimerInfo.ulElapsedErrCount++;
    }

    if ( RTC_Start_Value < ulTempValue )
    {
        g_stRtcSocTimerInfo.ulElapsedContentErrCount++;
        g_stRtcSocTimerInfo.ulElapsedContentErrSlice    = VOS_GetSlice();

        return RTC_Start_Value;
    }

    return RTC_Start_Value - ulTempValue;
}

/*****************************************************************************
 Function   : RTC_TimerCtrlBlkInit
 Description: Init timer's control block
 Input      : ulTimerCtrlBlkNumber -- number
 Return     : VOS_OK on success or errno on failure.
 Other      :
 *****************************************************************************/
VOS_UINT32 RTC_TimerCtrlBlkInit(VOS_VOID)
{
    VOS_UINT32                i;

    RTC_TimerCtrlBlkNumber  = RTC_MAX_TIMER_NUMBER;
    RTC_TimerIdleCtrlBlkNumber = RTC_MAX_TIMER_NUMBER;

    RTC_TimerCtrlBlk = (RTC_TIMER_CONTROL_BLOCK*)g_acRtcTimerCtrlBuf;

    RTC_TimerIdleCtrlBlk = RTC_TimerCtrlBlk;
    RTC_TimerCtrlBlkBegin = (VOS_VOID *)RTC_TimerCtrlBlk;
    RTC_TimerCtrlBlkEnd  = (VOS_VOID*)( (VOS_UINT_PTR)(RTC_TimerCtrlBlk) +
        RTC_TimerCtrlBlkNumber * sizeof(RTC_TIMER_CONTROL_BLOCK) );

    for(i=0; i<RTC_TimerCtrlBlkNumber-1; i++)
    {
        RTC_TimerCtrlBlk[i].State        = RTC_TIMER_CTRL_BLK_STOP;
        RTC_TimerCtrlBlk[i].ulUsedFlag   = VOS_NOT_USED;
        RTC_TimerCtrlBlk[i].TimerId      = i;
        RTC_TimerCtrlBlk[i].ulBackUpTimerId = i;
        RTC_TimerCtrlBlk[i].phTm         = VOS_NULL_PTR;
        RTC_TimerCtrlBlk[i].CallBackFunc = VOS_NULL_PTR;
        RTC_TimerCtrlBlk[i].previous     = VOS_NULL_PTR;
        /*lint -e679*/
        RTC_TimerCtrlBlk[i].next         = &(RTC_TimerCtrlBlk[i+1]);
        /*lint +e679*/
    }

    RTC_TimerCtrlBlk[RTC_TimerCtrlBlkNumber-1].State        = RTC_TIMER_CTRL_BLK_STOP;
    RTC_TimerCtrlBlk[RTC_TimerCtrlBlkNumber-1].ulUsedFlag   = VOS_NOT_USED;
    RTC_TimerCtrlBlk[RTC_TimerCtrlBlkNumber-1].TimerId      = RTC_TimerCtrlBlkNumber-1;
    RTC_TimerCtrlBlk[RTC_TimerCtrlBlkNumber-1].ulBackUpTimerId = RTC_TimerCtrlBlkNumber-1;
    RTC_TimerCtrlBlk[RTC_TimerCtrlBlkNumber-1].phTm         = VOS_NULL_PTR;
    RTC_TimerCtrlBlk[RTC_TimerCtrlBlkNumber-1].CallBackFunc = VOS_NULL_PTR;
    RTC_TimerCtrlBlk[RTC_TimerCtrlBlkNumber-1].previous     = VOS_NULL_PTR;
    RTC_TimerCtrlBlk[RTC_TimerCtrlBlkNumber-1].next         = VOS_NULL_PTR;

    RTC_TimerMinTimerIdUsed             = RTC_TimerCtrlBlkNumber;

    if ( VOS_NULL_PTR == VOS_MemSet_s(&g_stRtcSocTimerInfo, sizeof(VOS_TIMER_SOC_TIMER_INFO_STRU), 0x0, sizeof(VOS_TIMER_SOC_TIMER_INFO_STRU)) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    if ( VOS_NULL_PTR == VOS_MemSet_s((VOS_VOID *)g_astRtcSocTimerDebugInfo, sizeof(g_astRtcSocTimerDebugInfo), 0x0, sizeof(g_astRtcSocTimerDebugInfo)) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    /* 1 -> only one queue */
    if( VOS_OK != VOS_FixedQueueCreate( VOS_FID_QUEUE_LENGTH, &g_ulRTCTaskQueueId, VOS_MSG_Q_FIFO, VOS_FID_MAX_MSG_LENGTH, 1 ) )
    {
        return VOS_ERR;
    }

    mdrv_timer_debug_register(VOS_RTC_TIMER_ID, (FUNCPTR_1)VOS_TimerLpmCb, 0);

    return(VOS_OK);
}

/*****************************************************************************
 Function   : RTC_TimerCtrlBlkGet
 Description: allocte a block
 Input      : void
 Return     : address
 Other      :
 *****************************************************************************/
RTC_TIMER_CONTROL_BLOCK *RTC_TimerCtrlBlkGet(VOS_UINT32 ulFileID, VOS_INT32 usLineNo)
{
    /*int                      intLockLevel;*/
    RTC_TIMER_CONTROL_BLOCK  *temp_Timer_Ctrl_Ptr;

    VOS_VOID                 *pDumpBuffer;

    /*intLockLevel = VOS_SplIMP();*/

    if( 0 == RTC_TimerIdleCtrlBlkNumber )
    {
        /*VOS_Splx(intLockLevel);*/

        (VOS_VOID)VOS_SetErrorNo(VOS_RTC_ERRNO_SYSTIMER_FULL);

        pDumpBuffer = (VOS_VOID*)VOS_EXCH_MEM_MALLOC;

        if (VOS_NULL_PTR == pDumpBuffer)
        {
            return((RTC_TIMER_CONTROL_BLOCK*)VOS_NULL_PTR);
        }

        if ( VOS_NULL_PTR == VOS_MemSet_s(pDumpBuffer, VOS_DUMP_MEM_TOTAL_SIZE, 0, VOS_DUMP_MEM_TOTAL_SIZE) )
        {
            mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
        }

        /* 防止拷贝内存越界，取最小值 */
        /*lint -e506 */
        if ( VOS_NULL_PTR == VOS_MemCpy_s(pDumpBuffer, VOS_DUMP_MEM_TOTAL_SIZE, (VOS_VOID *)g_acRtcTimerCtrlBuf,
                   ((VOS_DUMP_MEM_TOTAL_SIZE < RTC_TIMER_CTRL_BUF_SIZE) ? VOS_DUMP_MEM_TOTAL_SIZE : RTC_TIMER_CTRL_BUF_SIZE )) )
        {
            mdrv_om_system_error(VOS_REBOOT_MEMCPY_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
        }
        /*lint +e506 */

        return((RTC_TIMER_CONTROL_BLOCK*)VOS_NULL_PTR);
    }
    else
    {
        RTC_TimerIdleCtrlBlkNumber--;

        temp_Timer_Ctrl_Ptr = RTC_TimerIdleCtrlBlk;
        temp_Timer_Ctrl_Ptr->ulUsedFlag = VOS_USED;
        RTC_TimerIdleCtrlBlk = RTC_TimerIdleCtrlBlk->next;
    }

    /*VOS_Splx(intLockLevel);*/

    /* record the usage of timer control block */
    if ( RTC_TimerIdleCtrlBlkNumber < RTC_TimerMinTimerIdUsed )
    {
        RTC_TimerMinTimerIdUsed = RTC_TimerIdleCtrlBlkNumber;
    }

    temp_Timer_Ctrl_Ptr->next = VOS_NULL_PTR;
    temp_Timer_Ctrl_Ptr->previous = VOS_NULL_PTR;

        temp_Timer_Ctrl_Ptr->ulFileID = ulFileID;
        temp_Timer_Ctrl_Ptr->ulLineNo = (VOS_UINT32)usLineNo;
        temp_Timer_Ctrl_Ptr->ulAllocTick = VOS_GetSlice();

    return temp_Timer_Ctrl_Ptr;
}

/*****************************************************************************
 Function   : RTC_TimerCtrlBlkFree
 Description: free a block
 Input      : Ptr -- address
              ucTag -- where call this function.this should be deleted when release
 Return     : void
 Other      :
 *****************************************************************************/
VOS_UINT32 RTC_TimerCtrlBlkFree(RTC_TIMER_CONTROL_BLOCK *Ptr, VOS_UINT8 ucTag )
{
    /*int             intLockLevel;*/

    if ( (VOS_UINT_PTR)Ptr < (VOS_UINT_PTR)RTC_TimerCtrlBlkBegin
        || (VOS_UINT_PTR)Ptr > (VOS_UINT_PTR)RTC_TimerCtrlBlkEnd )
    {
        return VOS_ERR;
    }

    /*intLockLevel = VOS_SplIMP();*/

    if ( VOS_NOT_USED == Ptr->ulUsedFlag )
    {
        /*VOS_Splx(intLockLevel);*/

        (VOS_VOID)VOS_SetErrorNo(VOS_RTC_ERRNO_RELTM_FREE_RECEPTION);

        return VOS_RTC_ERRNO_RELTM_FREE_RECEPTION;
    }

    Ptr->ulUsedFlag = VOS_NOT_USED;
    Ptr->Reserved[0] = ucTag;
    Ptr->next = RTC_TimerIdleCtrlBlk;
    RTC_TimerIdleCtrlBlk = Ptr;

    RTC_TimerIdleCtrlBlkNumber++;

    Ptr->ulFreeTick = VOS_GetSlice();

    /*VOS_Splx(intLockLevel);*/

    return VOS_OK;
}

/*****************************************************************************
 Function   : RTC_TimerTaskFunc
 Description: RTC timer task entry
 Input      : void
 Return     : void
 Other      :
 *****************************************************************************/
VOS_VOID RTC_TimerTaskFunc( VOS_UINT32 Para0, VOS_UINT32 Para1,
                            VOS_UINT32 Para2, VOS_UINT32 Para3 )
{
    RTC_TIMER_CONTROL_BLOCK     *head_Ptr;
    RTC_TIMER_CONTROL_BLOCK     *RTC_TimerCtrlBlkexpired = VOS_NULL_PTR;
    VOS_ULONG                   ulLockLevel;
    REL_TIMER_MSG               *pstExpireMsg;
    VOS_UINT32                  ulCurrentSlice;
    VOS_UINT32                  ulIntervalSlice;
    VOS_UINT_PTR                ulCtrlBlkAddress;
    VOS_UINT_PTR                TempValue;

    for(;;)
    {
        if (VOS_ERR == VOS_FixedQueueRead(g_ulRTCTaskQueueId, 0, &ulCtrlBlkAddress, VOS_FID_MAX_MSG_LENGTH))
        {
            continue;
        }

        ulCurrentSlice = VOS_GetSlice();

        RTC_TimerCtrlBlkexpired = (RTC_TIMER_CONTROL_BLOCK *)ulCtrlBlkAddress;

        while ( VOS_NULL_PTR != RTC_TimerCtrlBlkexpired )
        {
            head_Ptr = RTC_TimerCtrlBlkexpired;

            if( VOS_RELTIMER_LOOP == head_Ptr->Mode )
            {
                if ( VOS_NULL_PTR == head_Ptr->CallBackFunc )
                {
                   (VOS_VOID)V_Start32KRelTimer( head_Ptr->phTm,
                               head_Ptr->Pid,
                               head_Ptr->TimeOutValueInMilliSeconds,
                               head_Ptr->Name,
                               head_Ptr->Para,
                               VOS_RELTIMER_LOOP,
                               head_Ptr->ulPrecision,
                               head_Ptr->ulFileID,
                               (VOS_INT32)head_Ptr->ulLineNo);
                }
                else
                {
                    (VOS_VOID)V_Start32KCallBackRelTimer( head_Ptr->phTm,
                               head_Ptr->Pid,
                               head_Ptr->TimeOutValueInMilliSeconds,
                               head_Ptr->Name,
                               head_Ptr->Para,
                               VOS_RELTIMER_LOOP,
                               head_Ptr->CallBackFunc,
                               head_Ptr->ulPrecision,
                               head_Ptr->ulFileID,
                               (VOS_INT32)head_Ptr->ulLineNo);
                }
            }

            TempValue = (VOS_UINT_PTR)(RTC_TimerCtrlBlkexpired->CallBackFunc);

            /* CallBackFunc需要用32位传入，所以和name互换位置保证数据不丢失 */
            OM_RecordInfoStart(VOS_EXC_DUMP_MEM_NUM_3, (VOS_UINT32)(RTC_TimerCtrlBlkexpired->Pid), RTC_TimerCtrlBlkexpired->Name, (VOS_UINT32)TempValue);

            if ( VOS_NULL_PTR == RTC_TimerCtrlBlkexpired->CallBackFunc )
            {
                /* Alloc expires's Msg */
                pstExpireMsg
                    = VOS_TimerPreAllocMsg(RTC_TimerCtrlBlkexpired->Pid);

                if ( VOS_NULL_PTR != pstExpireMsg )
                {
                    pstExpireMsg->ulName = RTC_TimerCtrlBlkexpired->Name;
                    pstExpireMsg->ulPara = RTC_TimerCtrlBlkexpired->Para;
                    TempValue            = (VOS_UINT_PTR)(RTC_TimerCtrlBlkexpired->TimeOutValueInMilliSeconds);
                    pstExpireMsg->hTm    = (HTIMER)TempValue;

                    TempValue            = (VOS_UINT_PTR)RTC_TimerCtrlBlkexpired->ulAllocTick;

                    pstExpireMsg->pNext
                        = (struct REL_TIMER_MSG_STRU *)TempValue;

                    TempValue            = (VOS_UINT_PTR)ulCurrentSlice;

                    pstExpireMsg->pPrev
                        = (struct REL_TIMER_MSG_STRU *)TempValue;

                    ulIntervalSlice = (RTC_TimerCtrlBlkexpired->ulBackUpTimeOutValueInCycle - RTC_TimerCtrlBlkexpired->ulPrecisionInCycle);

                    ulIntervalSlice = ( (ulIntervalSlice > RTC_TIMER_CHECK_PRECISION) ? (ulIntervalSlice - RTC_TIMER_CHECK_PRECISION) : ulIntervalSlice );

                    if ( (ulCurrentSlice - RTC_TimerCtrlBlkexpired->ulAllocTick) < ulIntervalSlice )
                    {
                    }

                    ulIntervalSlice = (RTC_TimerCtrlBlkexpired->ulBackUpTimeOutValueInCycle + RTC_TimerCtrlBlkexpired->ulPrecisionInCycle);

                    if ( (ulCurrentSlice - RTC_TimerCtrlBlkexpired->ulAllocTick) > (ulIntervalSlice + RTC_TIMER_CHECK_LONG_PRECISION) )
                    {
                    }
                    (VOS_VOID)VOS_SendMsg(DOPRA_PID_TIMER, pstExpireMsg);

                }
            }
            else
            {
                RTC_TimerCtrlBlkexpired->CallBackFunc(
                    RTC_TimerCtrlBlkexpired->Para,
                    RTC_TimerCtrlBlkexpired->Name);
            }

            OM_RecordInfoEnd(VOS_EXC_DUMP_MEM_NUM_3);

            if(VOS_TRUE == g_ulTimerlpm)
            {
                g_ulTimerlpm = VOS_FALSE;

                (VOS_VOID)vos_printf("rtc_timer: allocpid %d, timername 0x%x.\n",
                    RTC_TimerCtrlBlkexpired->Pid, RTC_TimerCtrlBlkexpired->Name);
            }


            RTC_TimerCtrlBlkexpired = RTC_TimerCtrlBlkexpired->next;

            VOS_SpinLockIntLock(&g_stVosTimerSpinLock, ulLockLevel);

            (VOS_VOID)RTC_TimerCtrlBlkFree(head_Ptr, THE_FIRST_RTC_TIMER_TAG);

            VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);
        }
    }
}

/*****************************************************************************
 Function   : RTC_TimerTaskCreat
 Description: create RTC timer task
 Input      : void
 Return     : VOS_OK on success or errno on failure.
 Other      :
 *****************************************************************************/
VOS_UINT32 RTC_TimerTaskCreat(VOS_VOID)
{
    VOS_UINT32 TimerArguments[4] = {0,0,0,0};

    return( VOS_CreateTask( "RTC_TIMER",
                            &RTC_TimerTaskId,
                            RTC_TimerTaskFunc,
                            COMM_RTC_TIMER_TASK_PRIO,
                            RTC_TIMER_TASK_STACK_SIZE,
                            TimerArguments) );
}

/*****************************************************************************
 Function   : RTC_Add_Timer_To_List
 Description: add a timer to list
 Input      : Timer -- the tiemr's adddress
 Return     : void
 Other      :
 *****************************************************************************/
VOS_VOID RTC_Add_Timer_To_List( RTC_TIMER_CONTROL_BLOCK  *Timer)
{
    RTC_TIMER_CONTROL_BLOCK  *temp_Ptr;
    RTC_TIMER_CONTROL_BLOCK  *pre_temp_Ptr;
    /*int                      intLockLevel;*/
    VOS_UINT32               ElapsedCycles = 0;

    /* intLockLevel = VOS_SplIMP(); */

    if ( VOS_NULL_PTR == RTC_Timer_head_Ptr )
    {
        RTC_Timer_head_Ptr = Timer;

        Timer->Reserved[1] = VOS_ADD_TIMER_TAG_1;
    }
    else
    {
        ElapsedCycles = GetHardTimerElapsedTime();

        Timer->TimeOutValueInCycle += ElapsedCycles;

        /*  find the location to insert */
        temp_Ptr = pre_temp_Ptr = RTC_Timer_head_Ptr;

        while ( temp_Ptr != VOS_NULL_PTR )
        {
            if (Timer->TimeOutValueInCycle >= temp_Ptr->TimeOutValueInCycle)
            {
                Timer->TimeOutValueInCycle -= temp_Ptr->TimeOutValueInCycle;

                if ( Timer->TimeOutValueInCycle <= Timer->ulPrecisionInCycle )
                {
                    /* forward adjust; do nothindg when TimeOutValueInCycle == 0 */
                    Timer->TimeOutValueInCycle = 0;
                    Timer->Reserved[1] = VOS_ADD_TIMER_TAG_2;

                    pre_temp_Ptr = temp_Ptr;
                    temp_Ptr = temp_Ptr->next;

                    while ( (temp_Ptr != VOS_NULL_PTR)
                        && (Timer->TimeOutValueInCycle == temp_Ptr->TimeOutValueInCycle) )
                    {
                        pre_temp_Ptr = temp_Ptr;
                        temp_Ptr = temp_Ptr->next;
                    }/* make sure the order of expiry */

                    break;
                }

                pre_temp_Ptr = temp_Ptr;
                temp_Ptr = temp_Ptr->next;

                Timer->Reserved[1] = VOS_ADD_TIMER_TAG_3;
            }
            else
            {
                if ( temp_Ptr->TimeOutValueInCycle - Timer->TimeOutValueInCycle <= Timer->ulPrecisionInCycle )
                {
                    /* backward adjust */
                    Timer->TimeOutValueInCycle = 0;
                    Timer->Reserved[1] = VOS_ADD_TIMER_TAG_4;

                    pre_temp_Ptr = temp_Ptr;
                    temp_Ptr = temp_Ptr->next;

                    while ( (temp_Ptr != VOS_NULL_PTR)
                        && (Timer->TimeOutValueInCycle == temp_Ptr->TimeOutValueInCycle) )
                    {
                        pre_temp_Ptr = temp_Ptr;
                        temp_Ptr = temp_Ptr->next;
                    }/* make sure the order of expiry */

                    break;
                }

                Timer->Reserved[1] = VOS_ADD_TIMER_TAG_5;

                /* can't adjust */
                break;
            }
        }

        /* insert timer < head timer*/
        if ( temp_Ptr == RTC_Timer_head_Ptr )
        {
            Timer->next = RTC_Timer_head_Ptr;
            RTC_Timer_head_Ptr = Timer;
        }
        else
        {
            Timer->next = temp_Ptr;
            pre_temp_Ptr->next = Timer;
            Timer->previous = pre_temp_Ptr;
        }

        if ( temp_Ptr != VOS_NULL_PTR )
        {
            temp_Ptr->TimeOutValueInCycle
                = temp_Ptr->TimeOutValueInCycle - Timer->TimeOutValueInCycle;
            temp_Ptr->previous = Timer;
        }
    }

    /* restart RTC timer */
    if ( RTC_Timer_head_Ptr == Timer)
    {
        /* judge timer value when the new timer at head */
        Timer->TimeOutValueInCycle -= ElapsedCycles;

        if (0 == Timer->TimeOutValueInCycle)
        {
            Timer->TimeOutValueInCycle += 1;
        }

        StartHardTimer(Timer->TimeOutValueInCycle);
    }

    /*VOS_Splx(intLockLevel);*/
}

/*****************************************************************************
 Function   : RTC_Del_Timer_From_List
 Description: del a timer from list
 Input      : Timer -- the timer's address
 Return     : void
 Other      :
 *****************************************************************************/
VOS_VOID RTC_Del_Timer_From_List( RTC_TIMER_CONTROL_BLOCK  *Timer)
{
    /*int                      intLockLevel;*/
    VOS_BOOL                 bIsHead = VOS_FALSE;

    /*intLockLevel = VOS_SplIMP();*/

    /* deletet this timer from list */
    if ( Timer == RTC_Timer_head_Ptr )
    {
        bIsHead = VOS_TRUE;

        RTC_Timer_head_Ptr = Timer->next;
        if ( VOS_NULL_PTR != RTC_Timer_head_Ptr )
        {
            RTC_Timer_head_Ptr->previous = VOS_NULL_PTR;
        }
    }
    else
    {
        (Timer->previous)->next = Timer->next;
        if ( VOS_NULL_PTR != Timer->next )
        {
            (Timer->next)->previous = Timer->previous;
        }
    }

    /* adjust the time_val after this timer */
    if ( Timer->next != NULL )
    {
        Timer->next->TimeOutValueInCycle += Timer->TimeOutValueInCycle;

        if (VOS_TRUE == bIsHead)
        {
            Timer->next->TimeOutValueInCycle -= GetHardTimerElapsedTime();

            if (0 == Timer->next->TimeOutValueInCycle)
            {
                Timer->next->TimeOutValueInCycle += 1;
            }

            StartHardTimer(Timer->next->TimeOutValueInCycle);
        }
    }

    /* Stop timer3 if no timer */
    if ( VOS_NULL_PTR == RTC_Timer_head_Ptr )
    {
        StopHardTimer();
    }

    /*VOS_Splx(intLockLevel);*/
}

/*****************************************************************************
 Function   : R_Stop32KTimer
 Description: stop a 32K relative timer which was previously started.
 Input      : phTm -- where store the timer to be stopped
 Return     :  VOS_OK on success or errno on failure
 *****************************************************************************/
VOS_UINT32 R_Stop32KTimer( HTIMER *phTm, VOS_UINT32 ulFileID, VOS_INT32 usLineNo, VOS_TIMER_OM_EVENT_STRU *pstEvent )
{
    VOS_UINT32               TimerId;
    RTC_TIMER_CONTROL_BLOCK  *Timer;
    /*int                      intLockLevel;*/

    /*intLockLevel = VOS_SplIMP();*/

    if( VOS_NULL_PTR == *phTm )
    {
        /*VOS_Splx(intLockLevel);*/

        return VOS_OK;
    }

    if ( VOS_OK != RTC_CheckTimer(phTm, &TimerId, ulFileID, usLineNo ) )
    {
        /*VOS_Splx(intLockLevel);*/

        (VOS_VOID)VOS_SetErrorNo(VOS_RTC_ERRNO_RELTM_STOP_TIMERINVALID);
        return(VOS_RTC_ERRNO_RELTM_STOP_TIMERINVALID);
    }

    Timer = &RTC_TimerCtrlBlk[TimerId];

    /* del the timer from the running list */
    RTC_Del_Timer_From_List( Timer );

    *(Timer->phTm) = VOS_NULL_PTR;

    /* OM */
    if ( VOS_NULL_PTR != pstEvent )
    {
        pstEvent->ucMode      = Timer->Mode;
        pstEvent->Pid         = Timer->Pid;
        pstEvent->ulLength    = Timer->TimeOutValueInMilliSeconds;
        pstEvent->ulName      = Timer->Name;
        pstEvent->ulParam     = Timer->Para;
        pstEvent->enPrecision = (VOS_TIMER_PRECISION_ENUM_UINT32)Timer->ulPrecision;
    }

    /*VOS_Splx(intLockLevel);*/

    /*Timer->State = RTC_TIMER_CTRL_BLK_STOP;*/

    return RTC_TimerCtrlBlkFree(Timer, THE_SECOND_RTC_TIMER_TAG);
}

/*****************************************************************************
 Function     : R_Get32KRelTmRemainTime
 Description  : get left time
 Input Param  : phTm
 Output       : pulTime
 Return Value : VOS_OK on success or errno on failure
*****************************************************************************/
VOS_UINT32 R_Get32KRelTmRemainTime( HTIMER * phTm, VOS_UINT32 * pulTime,
                                 VOS_UINT32 ulFileID, VOS_INT32 usLineNo )
{
    VOS_UINT32                  TimerId;
    VOS_UINT32                  remain_value = 0;
    RTC_TIMER_CONTROL_BLOCK     *head_Ptr;
    RTC_TIMER_CONTROL_BLOCK     *temp_Ptr;
    /*int                         intLockLevel;*/
    VOS_UINT32                  ulTempValue;

    /*intLockLevel = VOS_SplIMP();*/

    if( VOS_NULL_PTR == *phTm )
    {
        /*VOS_Splx(intLockLevel);*/

        (VOS_VOID)VOS_SetErrorNo(VOS_RTC_ERRNO_RELTM_STOP_INPUTISNULL);
        return(VOS_RTC_ERRNO_RELTM_STOP_INPUTISNULL);
    }

    if ( VOS_OK != RTC_CheckTimer(phTm, &TimerId, ulFileID, usLineNo ) )
    {
        /*VOS_Splx(intLockLevel);*/

        (VOS_VOID)VOS_SetErrorNo(VOS_RTC_ERRNO_RELTM_STOP_TIMERINVALID);
        return(VOS_RTC_ERRNO_RELTM_STOP_TIMERINVALID);
    }

    head_Ptr = RTC_Timer_head_Ptr;

    while ( (VOS_NULL_PTR != head_Ptr) && (head_Ptr->TimerId != TimerId) )
    {
        remain_value += (head_Ptr->TimeOutValueInCycle);

        temp_Ptr = head_Ptr;
        head_Ptr = temp_Ptr->next;
    }

    if ( (VOS_NULL_PTR == head_Ptr) || ( head_Ptr->TimerId != TimerId) )
    {
        /*VOS_Splx(intLockLevel);*/

        return VOS_ERR;
    }
    else
    {
        remain_value += (head_Ptr->TimeOutValueInCycle);

        ulTempValue = GetHardTimerElapsedTime();

        /*VOS_Splx(intLockLevel);*/

        *pulTime
            = (VOS_UINT32)RTC_DIV_32_DOT_768((remain_value - ulTempValue), ulFileID, usLineNo);

        return(VOS_OK);
    }
}


VOS_UINT32 RTC_CheckTimer( HTIMER  *phTm, VOS_UINT32 *ulTimerID,
                           VOS_UINT32 ulFileID, VOS_INT32 usLineNo )
{
    RTC_TIMER_CONTROL_BLOCK  *Timer;

    if ( ((VOS_UINT_PTR)*phTm >= (VOS_UINT_PTR)RTC_TimerCtrlBlkBegin)
        && ((VOS_UINT_PTR)*phTm < (VOS_UINT_PTR)RTC_TimerCtrlBlkEnd) )
    {
        Timer = (RTC_TIMER_CONTROL_BLOCK *)(*phTm);

        if ( phTm == Timer->phTm )
        {
            *ulTimerID = Timer->ulBackUpTimerId;

            if ( Timer->ulBackUpTimerId != Timer->TimerId)
            {
                Timer->TimerId = Timer->ulBackUpTimerId;
            }

            return VOS_OK;
        }

        VOS_SIMPLE_FATAL_ERROR(RTC_CHECK_TIMER_ID);
    }
    else
    {
        VOS_ProtectionReboot(RTC_CHECK_TIMER_RANG, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)phTm, 128);
    }

    return VOS_ERR;
}


/*****************************************************************************
 Function   : V_Start32KCallBackRelTimer
 Description: allocate and start a relative timer using callback function.
 Input      : Pid           -- process ID of application
              ulLength       -- expire time. unit is millsecond
              ulName         -- timer name to be pass to app as a parameter
              ulParam        -- additional parameter to be pass to app
              ucMode         -- timer work mode
                                VOS_RELTIMER_LOOP  -- start periodically
                                VOS_RELTIMER_NOLOO -- start once time
              TimeOutRoutine -- Callback function when time out
              ulPrecision    -- precision,unit is 0 - 100->0%- 100%
 Output     : phTm           -- timer pointer which system retuns to app
 Return     : VOS_OK on success and errno on failure
 *****************************************************************************/
VOS_UINT32 V_Start32KCallBackRelTimer( HTIMER *phTm, VOS_PID Pid, VOS_UINT32 ulLength,
    VOS_UINT32 ulName, VOS_UINT32 ulParam, VOS_UINT8 ucMode, REL_TIMER_FUNC TimeOutRoutine,
    VOS_UINT32 ulPrecision, VOS_UINT32 ulFileID, VOS_INT32 usLineNo )
{
    RTC_TIMER_CONTROL_BLOCK  *Timer;
    VOS_UINT32               TimerId;
    VOS_ULONG                ulLockLevel;

    /* stop the timer if exists */
    /*intLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosTimerSpinLock, ulLockLevel);

    if( VOS_NULL_PTR != *phTm )
    {
        if ( VOS_OK == RTC_CheckTimer(phTm, &TimerId, ulFileID, usLineNo) )
        {
            if ( VOS_OK != R_Stop32KTimer( phTm, ulFileID, usLineNo, VOS_NULL_PTR ) )
            {
                /*VOS_Splx(intLockLevel);*/
                VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

                VOS_SIMPLE_FATAL_ERROR(START_32K_CALLBACK_RELTIMER_FAIL_TO_STOP);

                return VOS_ERR;
            }
        }
    }

    Timer = RTC_TimerCtrlBlkGet(ulFileID, usLineNo);
    if(Timer == VOS_NULL_PTR)
    {
        /*VOS_Splx(intLockLevel);*/
        VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

        (VOS_VOID)VOS_SetErrorNo(VOS_RTC_ERRNO_RELTM_START_MSGNOTINSTALL);

        VOS_ProtectionReboot(START_32K_CALLBACK_RELTIMER_FAIL_TO_ALLOCATE, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)(&g_stRtcSocTimerInfo), sizeof(VOS_TIMER_SOC_TIMER_INFO_STRU));

        return(VOS_RTC_ERRNO_RELTM_START_MSGNOTINSTALL);
    }

    /* if(phTm != VOS_NULL_PTR)
    {
        *phTm = (HTIMER)(&(Timer->TimerId));
    }*/

    Timer->Pid                          = Pid;
    Timer->Name                         = ulName;
    Timer->Para                         = ulParam;
    Timer->Mode                         = ucMode;
    Timer->phTm                         = phTm;
    Timer->TimeOutValueInMilliSeconds   = ulLength;
    Timer->ulPrecision                  = ulPrecision;
    Timer->TimeOutValueInCycle = (VOS_UINT32)RTC_MUL_32_DOT_768(ulLength, ulFileID, usLineNo);
    Timer->ulPrecisionInCycle = (VOS_UINT32)RTC_MUL_DOT_32768((ulLength * ulPrecision), ulFileID, usLineNo);
    Timer->ulBackUpTimeOutValueInCycle = Timer->TimeOutValueInCycle;
    /*Timer->State                        = RTC_TIMER_CTRL_BLK_RUNNIG;*/
    Timer->CallBackFunc                 = TimeOutRoutine;
    /*Timer->next                         = VOS_NULL_PTR;*/

    *phTm = (HTIMER)(&(Timer->TimerId));

    RTC_Add_Timer_To_List( Timer );

    /*VOS_Splx(intLockLevel);*/
    VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

    return(VOS_OK);
}

/*****************************************************************************
 Function   : V_Start32KRelTimer
 Description: allocate and start a relative timer using callback function.
 Input      : Pid           -- process ID of application
              ulLength       -- expire time. unit is millsecond
              ulName         -- timer name to be pass to app as a parameter
              ulParam        -- additional parameter to be pass to app
              ucMode         -- timer work mode
                                VOS_RELTIMER_LOOP  -- start periodically
                                VOS_RELTIMER_NOLOO -- start once time
              ulPrecision    -- precision,unit is 0 - 100->0%- 100%
 Output     : phTm           -- timer pointer which system retuns to app
 Return     : VOS_OK on success and errno on failure
 *****************************************************************************/
VOS_UINT32 V_Start32KRelTimer( HTIMER *phTm, VOS_PID Pid, VOS_UINT32 ulLength,
    VOS_UINT32 ulName, VOS_UINT32 ulParam, VOS_UINT8 ucMode, VOS_UINT32 ulPrecision,
    VOS_UINT32 ulFileID, VOS_INT32 usLineNo)
{
    RTC_TIMER_CONTROL_BLOCK  *Timer;
    VOS_UINT32               TimerId;
    VOS_ULONG                ulLockLevel;

    /* stop the timer if exists */
    /*intLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosTimerSpinLock, ulLockLevel);

    if( VOS_NULL_PTR != *phTm )
    {
        if ( VOS_OK == RTC_CheckTimer( phTm, &TimerId, ulFileID, usLineNo ) )
        {
            if ( VOS_OK != R_Stop32KTimer( phTm, ulFileID, usLineNo, VOS_NULL_PTR ) )
            {
                /*VOS_Splx(intLockLevel);*/
                VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

                VOS_SIMPLE_FATAL_ERROR(START_32K_RELTIMER_FAIL_TO_STOP);

                return VOS_ERR;
            }
        }
    }

    Timer = RTC_TimerCtrlBlkGet(ulFileID, usLineNo);

    if( VOS_NULL_PTR == Timer )
    {
        /*VOS_Splx(intLockLevel);*/
        VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

        (VOS_VOID)VOS_SetErrorNo(VOS_RTC_ERRNO_RELTM_START_MSGNOTINSTALL);

        VOS_ProtectionReboot(START_32K_CALLBACK_RELTIMER_FAIL_TO_ALLOCATE, (VOS_INT)ulFileID, (VOS_INT)usLineNo, (VOS_CHAR *)(&g_stRtcSocTimerInfo), sizeof(VOS_TIMER_SOC_TIMER_INFO_STRU));

        return(VOS_RTC_ERRNO_RELTM_START_MSGNOTINSTALL);
    }

    /*if( VOS_NULL_PTR != phTm )
    {
        *phTm = (HTIMER)(&(Timer->TimerId));
    }*/

    Timer->Pid                          = Pid;
    Timer->Name                         = ulName;
    Timer->Para                         = ulParam;
    Timer->Mode                         = ucMode;
    Timer->phTm                         = phTm;
    Timer->TimeOutValueInMilliSeconds   = ulLength;
    Timer->ulPrecision                  = ulPrecision;
    Timer->TimeOutValueInCycle = (VOS_UINT32)RTC_MUL_32_DOT_768(ulLength, ulFileID, usLineNo);
    Timer->ulPrecisionInCycle = (VOS_UINT32)RTC_MUL_DOT_32768((ulLength * ulPrecision), ulFileID, usLineNo);
    Timer->ulBackUpTimeOutValueInCycle = Timer->TimeOutValueInCycle;
    /*Timer->State                        = VOS_TIMER_CTRL_BLK_RUNNIG;*/
    Timer->CallBackFunc                 = VOS_NULL_PTR;
    /*Timer->next                         = VOS_NULL_PTR;*/

    *phTm = (HTIMER)(&(Timer->TimerId));

    /* add the timer to the running list */
    RTC_Add_Timer_To_List( Timer );

    /*VOS_Splx(intLockLevel);*/
    VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

    return(VOS_OK);
}

/*****************************************************************************
 Function   : R_Restart32KRelTimer
 Description: Restart a relative timer which was previously started
 Input      : phTm -- where store timer ID to be restarted
 Return     : VOS_OK on success or errno on failure.
 Other      : the properties of timer should not be changed,
              but timer ID could have been changed
 *****************************************************************************/
VOS_UINT32 R_Restart32KRelTimer( HTIMER *phTm, VOS_UINT32 ulFileID,
                                 VOS_INT32 usLineNo )
{
    VOS_UINT32               TimerId;
    RTC_TIMER_CONTROL_BLOCK  *Timer;
    /*int                      intLockLevel;*/

    /*intLockLevel = VOS_SplIMP();*/

    if( VOS_NULL_PTR == *phTm )
    {
        /*VOS_Splx(intLockLevel);*/

        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_RELTM_RESTART_TIMERINVALID);

        VOS_SIMPLE_FATAL_ERROR(RESTART_32K_RELTIMER_NULL);

        return(VOS_ERRNO_RELTM_RESTART_TIMERINVALID);
    }

    if ( VOS_OK != RTC_CheckTimer( phTm, &TimerId, ulFileID, usLineNo ) )
    {
        /*VOS_Splx(intLockLevel);*/

        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_RELTM_RESTART_TIMERINVALID);

        VOS_SIMPLE_FATAL_ERROR(RESTART_32K_RELTIMER_FAIL_TO_CHECK);

        return(VOS_ERRNO_RELTM_RESTART_TIMERINVALID);
    }

    Timer   = &RTC_TimerCtrlBlk[TimerId];

    /* Del the old timer but not free timer control block */
    RTC_Del_Timer_From_List( Timer );

    /* reset timer value */
    Timer->TimeOutValueInCycle = (VOS_UINT32)RTC_MUL_32_DOT_768(Timer->TimeOutValueInMilliSeconds, ulFileID, usLineNo);
    Timer->ulBackUpTimeOutValueInCycle = Timer->TimeOutValueInCycle;

    Timer->next = VOS_NULL_PTR;
    Timer->previous = VOS_NULL_PTR;

    /* add the new timer to list */
    RTC_Add_Timer_To_List( Timer );

    /*VOS_Splx(intLockLevel);*/

    return VOS_OK;
}

/*****************************************************************************
 Function   : RTC_timer_running
 Description: a APP RTC timer is running or not
 Input      : void
 Return     : true or false
 Other      :
 *****************************************************************************/
VOS_UINT32 RTC_timer_running( VOS_VOID )
{
    RTC_TIMER_CONTROL_BLOCK     *head_Ptr;
    VOS_ULONG                    ulLockLevel;

    /*intLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosTimerSpinLock, ulLockLevel);

    head_Ptr = RTC_Timer_head_Ptr;

    if ( head_Ptr != VOS_NULL_PTR)
    {
        while ( VOS_NULL_PTR != head_Ptr )
        {
            if ( APP_PID != head_Ptr->Pid )
            {
                head_Ptr = head_Ptr->next;
            }
            else
            {
                /*VOS_Splx(intLockLevel);*/
                VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

                return VOS_TRUE;
            }
        }

        /*VOS_Splx(intLockLevel);*/
        VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

        return VOS_FALSE;
    }
    else
    {
        /*VOS_Splx(intLockLevel);*/
        VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

        return VOS_FALSE;
    }
}

/*****************************************************************************
 Function   : VOS_GetSlice
 Description: get left time of the first timer.Unit is 30us
 Input      : void
 Return     :
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_GetSlice(VOS_VOID)
{
    return mdrv_timer_get_normal_timestamp();
}

/*****************************************************************************
 Function   : VOS_GetSliceUnit
 Description:
 Input      : void
 Return     :
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_GetSliceUnit(VOS_VOID)
{

    return 32768;
}

/*****************************************************************************
 Function   : RTC_CalcTimerInfo
 Description: print the usage info of timer
 Input      : void
 Return     : void
 Other      :
 *****************************************************************************/
VOS_BOOL RTC_CalcTimerInfo(VOS_VOID)
{
    if ( RTC_UPPER_TIMER_NUMBER > RTC_TimerMinTimerIdUsed )
    {
        RTC_TimerMinTimerIdUsed = RTC_TimerCtrlBlkNumber;
        return VOS_TRUE;
    }

    return VOS_FALSE;
}

/*****************************************************************************
 Function   : VOS_ShowUsed32KTimerInfo
 Description: print the usage info of 32K timer's control block
 Input      : void
 Return     : void
 Other      :
 *****************************************************************************/
VOS_VOID VOS_ShowUsed32KTimerInfo( VOS_VOID )
{
    VOS_ULONG                    ulLockLevel;
    RTC_TIMER_CONTROL_BLOCK     *pstTimer;

    LogPrint("# VOS_ShowUsed32KTimerInfo:");

    /*nIntLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosTimerSpinLock, ulLockLevel);

    pstTimer = RTC_Timer_head_Ptr;
    while( VOS_NULL_PTR != pstTimer )
    {
        LogPrint6("# F %d L %d P %d N %d R %d T %d.\r\n",
               (VOS_INT)pstTimer->ulFileID,
               (VOS_INT)pstTimer->ulLineNo,
               (VOS_INT)pstTimer->Pid,
               (VOS_INT)pstTimer->Name,
               (VOS_INT)pstTimer->Para,
               (VOS_INT)pstTimer->ulAllocTick);

        pstTimer = pstTimer->next;
    }

    /*VOS_Splx(nIntLockLevel);*/
    VOS_SpinUnlockIntUnlock(&g_stVosTimerSpinLock, ulLockLevel);

    return;
}


/* Added by g47350 for DRX timer Project, 2012/11/5, begin */

/* Added by g47350 for DRX timer Project, 2012/11/5, end */

/*****************************************************************************
 Function   : ShowRtcTimerLog
 Description:
 Input      : VOID
 Return     : VOID
 Other      :
 *****************************************************************************/
VOS_VOID ShowRtcTimerLog( VOS_VOID )
{
    (VOS_VOID)vos_printf("g_stRtcSocTimerInfo.ulStartCount = %d. (call DRV Start timer num)\r\n",
                g_stRtcSocTimerInfo.ulStartCount);
    (VOS_VOID)vos_printf("g_stRtcSocTimerInfo.ulStopCount =  %d. (call DRV Stop timer num)\r\n",
                g_stRtcSocTimerInfo.ulStopCount);
    (VOS_VOID)vos_printf("g_stRtcSocTimerInfo.ulExpireCount = %d. (receive DRV ISR of DualTimer num)\r\n",
                g_stRtcSocTimerInfo.ulExpireCount);
    (VOS_VOID)vos_printf("g_stRtcSocTimerInfo.ulStartErrCount = %d. (call DRV Stop timer err num)\r\n",
                g_stRtcSocTimerInfo.ulStartErrCount);
    (VOS_VOID)vos_printf("g_stRtcSocTimerInfo.ulStopErrCount = %d. (call DRV Start timer err num)\r\n",
                g_stRtcSocTimerInfo.ulStopErrCount);
    (VOS_VOID)vos_printf("g_stRtcSocTimerInfo.ulElapsedErrCount = %d. (call DRV get rest timer num)\r\n",
                g_stRtcSocTimerInfo.ulElapsedErrCount);
    (VOS_VOID)vos_printf("g_stRtcSocTimerInfo.ulElapsedContentErrCount = %d. (call DRV get rest timer err num)\r\n",
                g_stRtcSocTimerInfo.ulElapsedContentErrCount);
}




