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
/* FileName: v_blkmen.c                                                      */
/*                                                                           */
/* Author: Yang Xiangqian                                                    */
/*                                                                           */
/* Version: 1.0                                                              */
/*                                                                           */
/* Date: 2006-10                                                             */
/*                                                                           */
/* Description: implement memory allotee                                     */
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

#include "vos_config.h"
#include "v_typdef.h"
#include "v_blkMem.h"
#include "v_int.h"
#include "v_IO.h"
#include "v_timer.h"
#include "vos_Id.h"
#include "v_private.h"
#include "v_lib.h"
#include "product_config.h"
#include "mdrv.h"
#include "v_MemCfg.inc"

/* LINUX 不支持 */

#include <asm/dma-mapping.h>
#include <linux/version.h>
#include <linux/of_device.h>



/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_V_BLKMEM_C


/* which be used to calc real size of mem */
typedef struct
{
    VOS_UINT_PTR ulAddress;
    VOS_UINT32 ulType;
    VOS_INT    lCycle;
    VOS_UINT32 ulFileID;
    VOS_INT32  usLineNo;
}VOS_LOCATION_MEM_INFO_STRU;

/* the start address of mem pool */
VOS_UINT_PTR g_ulMemPoolIntervalAddress[VOS_MEM_POOL_NUMBER+1];

/* message control blocks */
VOS_MEM_CTRL_BLOCK     VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER];

/* memory control blocks */
VOS_MEM_CTRL_BLOCK     VOS_SimpleMemCtrlBlk[VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER];

#define VOS_MEM_HEAD_BLOCK_SIZE             ( sizeof(VOS_MEM_HEAD_BLOCK) )

/* defination of app memory buffer */
extern VOS_MEM_BLOCK_INFO AppMemBlockInfo[];

/* be used the head and tail of memory to protect overflow */
#define VOS_MEMORY_PROTECT_SIZE                               1024

/* the number of words which on the head of the user's space */
#define VOS_MEMORY_RESERVED_WORD_HEAD                          2

/* the number of words which on the tail of the user's space */
#define VOS_MEMORY_RESERVED_WORD_TAIL                          1

/* 导出OSA内存控制信息的个数 */
#define VOS_DUMP_MEM_HEAD_NUM                                  (3)

/* 导出OSA内存控制信息的个数加一是为了带出其它信息 */
#define VOS_DUMP_MEM_HEAD_TOTAL_NUM                            (VOS_DUMP_MEM_HEAD_NUM+1)

/* the number of bytes which on the user's space */
#define VOS_MEMORY_RESERVED_BYTES \
    ((sizeof(VOS_UINT_PTR) * VOS_MEMORY_RESERVED_WORD_HEAD) \
    + (sizeof(VOS_UINT_PTR) * VOS_MEMORY_RESERVED_WORD_TAIL))

/* 由于LINUX不支持浮点类型，由之前的浮点类型改为整型 0.1 -> 1 */
/* the ratio of count */
#define VOS_CALC_MEM_RATIO                                     (10)
/* the begin address of user's space */
VOS_UINT_PTR gulVosSpaceStart;

/* the end address of user's space */
VOS_UINT_PTR gulVosSpaceEnd;

/* the begin address of user's space */
VOS_UINT_PTR gulVosSpaceAndProtectionStart;

/* the begin address of user's space */
VOS_UINT_PTR gulVosSpaceAndProtectionEnd;

/* the buf of VOS's mem */

/* the buf of VOS's mem */
VOS_CHAR g_acVosMemBuf[1577424]; /* 由于A核支持64位，内存需要增大 */



VOS_UINT32 g_ulAutoCheckMemoryThreshold;

VOS_VOID VOS_PrintUsedMsgInfo(VOS_VOID);

VOS_VOID VOS_PrintUsedMemoryInfo(VOS_MEM_CTRL_BLOCK *pstMemCtrl, VOS_INT lCycle);

VOS_VOID VOS_PrintUsedTimerMsgInfo(VOS_VOID);

VOS_VOID VOS_MemDump(VOS_UINT32 ulInfo, VOS_UINT32 ulSize,VOS_UINT32 ulFileID,
                VOS_INT32 ulLineNo,VOS_MEM_CTRL_BLOCK *pstMemCtrl, VOS_INT lCycle);

VOS_VOID VOS_MsgDump(VOS_UINT32 ulInfo, VOS_UINT32 ulSize,
                    VOS_UINT32 ulFileID, VOS_INT32 ulLineNo);


extern VOS_UINT32 VOS_GetMsgName(VOS_UINT_PTR ulAddrress);

/* 自旋锁，用来作Memory的临界资源保护 */
VOS_SPINLOCK             g_stVosMemSpinLock;

/* 自旋锁，用来作静态Memory的临界资源保护 */
VOS_SPINLOCK             g_stVosStaticMemSpinLock;



/*****************************************************************************
 Function   : VOS_MemCtrlBlkInit_1
 Description: Init one memory control block
 Input      : VOS_MemCtrlBlock -- address of control block
            : ulBlockLength -- length
            : ulTotalBlockNumber -- number
            : ulAddress -- the start of address
 Return     : void
 Other      :
 *****************************************************************************/
VOS_VOID VOS_MemCtrlBlkInit( VOS_MEM_CTRL_BLOCK *VOS_MemCtrlBlock,
                      VOS_INT ulBlockLength, VOS_INT ulTotalBlockNumber,
                      VOS_UINT_PTR *ulCtrlAddress, VOS_UINT_PTR *ulSpaceAddress )
{
    VOS_UINT32           ulLength;
    VOS_INT              i;
    VOS_MEM_HEAD_BLOCK   *Block;
    VOS_UINT_PTR         ulTemp;
    VOS_UINT_PTR         *pulTemp;

    VOS_MemCtrlBlock->BlockLength = ulBlockLength;
    VOS_MemCtrlBlock->TotalBlockNumber = ulTotalBlockNumber;
    VOS_MemCtrlBlock->IdleBlockNumber = ulTotalBlockNumber;

    ulLength = (VOS_UINT32)(VOS_MEMORY_RESERVED_BYTES + ulBlockLength);

    VOS_MemCtrlBlock->Buffer = *ulSpaceAddress;
    /*lint -e647*/
    VOS_MemCtrlBlock->BufferEnd = *ulSpaceAddress
        + ulLength * ulTotalBlockNumber;
    /*lint +e647*/
    VOS_MemCtrlBlock->Blocks = (VOS_MEM_HEAD_BLOCK *)(*ulCtrlAddress);

    Block = VOS_MemCtrlBlock->Blocks;

    ulTemp = VOS_MemCtrlBlock->Buffer;

    for( i=1; i<ulTotalBlockNumber; i++)
    {
        Block->ulMemCtrlAddress = (VOS_UINT_PTR)VOS_MemCtrlBlock;
        Block->ulMemAddress = ulTemp;
        Block->ulMemUsedFlag = VOS_NOT_USED;
        Block->pstNext = Block + 1;

        /* add protection on head */
        pulTemp = (VOS_UINT_PTR*)ulTemp;
        *pulTemp = (VOS_UINT_PTR)Block;
        pulTemp++;
        *pulTemp = VOS_MEMORY_CRC;

        /* offset next */
        Block++;
        ulTemp += ulLength;

        /* add protection on tail */
        pulTemp = (VOS_UINT_PTR *)ulTemp;
        pulTemp--;
        *pulTemp = VOS_MEMORY_CRC;
    }
    Block->ulMemCtrlAddress = (VOS_UINT_PTR)VOS_MemCtrlBlock;
    Block->ulMemAddress = ulTemp;
    Block->ulMemUsedFlag = VOS_NOT_USED;
    Block->pstNext = VOS_NULL_PTR;
     /* add protection on head */
    pulTemp = (VOS_UINT_PTR *)ulTemp;
    *pulTemp = (VOS_UINT_PTR)Block;
    pulTemp++;
    *pulTemp = VOS_MEMORY_CRC;

    /* offset next */
    Block++;
    ulTemp += ulLength;

    /* add protection on tail */
    pulTemp = (VOS_UINT_PTR *)ulTemp;
    pulTemp--;
    *pulTemp = VOS_MEMORY_CRC;

    *ulCtrlAddress = (VOS_UINT_PTR)Block;
    *ulSpaceAddress = ulTemp;

    VOS_MemCtrlBlock->BusyBlocks = VOS_NULL_PTR;

    VOS_MemCtrlBlock->MinIdleBlockNumber = ulTotalBlockNumber;
    VOS_MemCtrlBlock->MinSize = 0x0fffffff;
    VOS_MemCtrlBlock->MaxSize = 0;
    VOS_MemCtrlBlock->lRealNumber = 0;
    VOS_MemCtrlBlock->lMaxRealNumber = 0;
}

/*****************************************************************************
 Function   : VOS_MemCtrlBlkInit
 Description: Init all message control blocks
 Input      : void
            :
 Return     : VOS_OK on success or errno on failure.
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_MemInit( VOS_VOID )
{
    VOS_INT    i;
    VOS_UINT32 lTotalSize = 0;
    VOS_UINT32 lTotalCtrlSize = 0;
    VOS_VOID   *pCtrlStart;
    VOS_UINT_PTR ulSpaceStart;
    VOS_UINT_PTR ulSpaceEnd;
    VOS_UINT_PTR ulCtrlStart;

    /* calculate msg+mem's size */
    for ( i=0; i<VOS_MEM_CTRL_BLOCK_NUMBER; i++ )
    {
        if ( 0x00000003 & MsgBlockInfo[i].size )/* 4 byte Aligned */
        {
            LogPrint2("# MsgBlockInfo[%d].size(%d) not aligned.\r\n",
                i, MsgBlockInfo[i].size );

            return VOS_ERR;
        }

        lTotalSize += ( MsgBlockInfo[i].size + VOS_MEMORY_RESERVED_BYTES
            + VOS_MEM_HEAD_BLOCK_SIZE )
            * MsgBlockInfo[i].number;
        lTotalCtrlSize += VOS_MEM_HEAD_BLOCK_SIZE * MsgBlockInfo[i].number;
    }

    for ( i=0; i<VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER; i++ )
    {
        if ( 0x00000003 & MemBlockInfo[i].size )/* 4 byte Aligned */
        {
            LogPrint2("# MemBlockInfo[%d].size(%d) not aligned.\r\n",
                    i, MemBlockInfo[i].size );

            return VOS_ERR;
        }

        lTotalSize += ( MemBlockInfo[i].size + VOS_MEMORY_RESERVED_BYTES
            + VOS_MEM_HEAD_BLOCK_SIZE )
            * MemBlockInfo[i].number;
        lTotalCtrlSize += VOS_MEM_HEAD_BLOCK_SIZE * MemBlockInfo[i].number;
    }

    /* add protected space */
    lTotalSize += 2 * VOS_MEMORY_PROTECT_SIZE;
    lTotalCtrlSize += VOS_MEMORY_PROTECT_SIZE;

    if ( lTotalSize > sizeof(g_acVosMemBuf) )
    {
        return VOS_ERR;
    }

    pCtrlStart = (VOS_VOID *)g_acVosMemBuf;

    ulCtrlStart = (VOS_UINT_PTR)pCtrlStart;

    ulSpaceStart = ulCtrlStart + lTotalCtrlSize;
    gulVosSpaceStart = ulSpaceStart;

    ulSpaceEnd = ulCtrlStart + lTotalSize - VOS_MEMORY_PROTECT_SIZE ;
    gulVosSpaceEnd = ulSpaceEnd;

    gulVosSpaceAndProtectionStart = gulVosSpaceStart - VOS_MEMORY_PROTECT_SIZE;

    gulVosSpaceAndProtectionEnd = gulVosSpaceEnd + VOS_MEMORY_PROTECT_SIZE;

    /* the start address of msg pool */
    g_ulMemPoolIntervalAddress[0] = gulVosSpaceStart;

    for ( i=0; i<VOS_MEM_CTRL_BLOCK_NUMBER; i++ )
    {
        VOS_MemCtrlBlkInit( &VOS_MemCtrlBlk[i],  MsgBlockInfo[i].size,
            MsgBlockInfo[i].number, &ulCtrlStart, &ulSpaceStart);
    }

    /* the start address of mem pool */
    g_ulMemPoolIntervalAddress[1] = ulSpaceStart;

    for ( i=0; i<VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER; i++ )
    {
        VOS_MemCtrlBlkInit( &VOS_SimpleMemCtrlBlk[i], MemBlockInfo[i].size,
            MemBlockInfo[i].number, &ulCtrlStart, &ulSpaceStart);
    }

    /* the start address of app mem pool */
    g_ulMemPoolIntervalAddress[2] = ulSpaceStart;

    VOS_SpinLockInit(&g_stVosMemSpinLock);

    VOS_SpinLockInit(&g_stVosStaticMemSpinLock);

    return VOS_OK;
}

/*****************************************************************************
 Function   : VOS_DumpVosMem
 Description: dump OSA Memory
 Input      :
            :
            :
 Return     :
 Other      :
 *****************************************************************************/
VOS_VOID VOS_DumpVosMem(VOS_MEM_HEAD_BLOCK *pstHeadBlock, VOS_UINT_PTR ulUsrAddr, VOS_UINT32 ulErrNo, VOS_UINT32 ulFileID, VOS_INT32 usLineNo)
{

    VOS_UINT8           *pucDumpBuffer;
    VOS_MEM_HEAD_BLOCK  *pstTmpMemHead = pstHeadBlock;
    VOS_CHAR            *pucUserAddr = (VOS_CHAR *)ulUsrAddr;
    VOS_MEM_HEAD_BLOCK  astMemHead[VOS_DUMP_MEM_HEAD_TOTAL_NUM];

    if ( VOS_NULL_PTR == VOS_MemSet_s(astMemHead, sizeof(astMemHead), 0, sizeof(astMemHead)) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    (VOS_VOID)VOS_TaskLock();

    /* 借用空间保存部分信息 */
    astMemHead[0].ulMemCtrlAddress = (VOS_UINT_PTR)pstHeadBlock;
    astMemHead[0].ulMemAddress = ulUsrAddr;
    astMemHead[0].ulMemUsedFlag = ulErrNo;

    /* 保存出错的控制块前后个一个，总共3个控制块 */
    if ( VOS_NULL_PTR != pstHeadBlock )
    {
        pstTmpMemHead--;

        if ( VOS_NULL_PTR == VOS_MemCpy_s((VOS_CHAR *)(&(astMemHead[1])), VOS_DUMP_MEM_HEAD_NUM*sizeof(VOS_MEM_HEAD_BLOCK), (VOS_CHAR *)pstTmpMemHead, VOS_DUMP_MEM_HEAD_NUM*sizeof(VOS_MEM_HEAD_BLOCK)) )
        {
            mdrv_om_system_error(VOS_REBOOT_MEMCPY_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
        }
    }

    pucDumpBuffer = (VOS_UINT8 *)VOS_EXCH_MEM_MALLOC;

    if (VOS_NULL_PTR == pucDumpBuffer)
    {
        VOS_ProtectionReboot(OSA_CHECK_MEM_ERROR, (VOS_INT)ulFileID,
                             (VOS_INT)usLineNo, (VOS_CHAR *)astMemHead, sizeof(astMemHead));

        (VOS_VOID)VOS_TaskUnlock();

        return;
    }

    if ( VOS_NULL_PTR == VOS_MemSet_s((VOS_VOID *)pucDumpBuffer, VOS_DUMP_MEM_TOTAL_SIZE, 0, VOS_DUMP_MEM_TOTAL_SIZE) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    /* 保存出错的地址前后各一半的空间 */
    pucUserAddr -= (VOS_DUMP_MEM_TOTAL_SIZE >>1);

    if ( VOS_NULL_PTR == VOS_MemCpy_s((VOS_CHAR *)pucDumpBuffer, VOS_DUMP_MEM_TOTAL_SIZE, pucUserAddr, VOS_DUMP_MEM_TOTAL_SIZE) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMCPY_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    VOS_ProtectionReboot(OSA_CHECK_MEM_ERROR, (VOS_INT)ulFileID,
                         (VOS_INT)usLineNo, (VOS_CHAR *)astMemHead, sizeof(astMemHead));

    (VOS_VOID)VOS_TaskUnlock();

	return;
}

/*****************************************************************************
 Function   : VOS_MemCtrlBlkMalloc
 Description: allocate a block
 Input      : VOS_MemCtrlBlock -- block address
            : ulLength -- length
            : alloc_info -- who allocate
 Return     : the address of the block
 Other      :
 *****************************************************************************/
VOS_VOID* VOS_MemCtrlBlkMalloc( VOS_MEM_CTRL_BLOCK *VOS_MemCtrlBlock,
                                VOS_INT ulLength, VOS_UINT32 ulFileID, VOS_INT32 usLineNo )
{
    VOS_ULONG             ulLockLevel;
    VOS_MEM_HEAD_BLOCK   *Block;
    VOS_UINT_PTR         *pulTemp;
    VOS_UINT_PTR          ulBlockAddr;
    VOS_UINT32           ulCrcTag;

    /*intLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosMemSpinLock, ulLockLevel);

    if( 0 >= VOS_MemCtrlBlock->IdleBlockNumber )
    {
        /*VOS_Splx(intLockLevel);*/
        VOS_SpinUnlockIntUnlock(&g_stVosMemSpinLock, ulLockLevel);

        return((VOS_VOID*)VOS_NULL_PTR);
    }
    else
    {
        VOS_MemCtrlBlock->IdleBlockNumber--;
        Block = VOS_MemCtrlBlock->Blocks;
        Block->ulMemUsedFlag = VOS_USED;
        VOS_MemCtrlBlock->Blocks = Block->pstNext;
    }

    Block->pstPre = VOS_NULL_PTR;
    Block->pstNext = VOS_MemCtrlBlock->BusyBlocks;
    if ( VOS_NULL_PTR != VOS_MemCtrlBlock->BusyBlocks )
    {
        VOS_MemCtrlBlock->BusyBlocks->pstPre = Block;
    }
    VOS_MemCtrlBlock->BusyBlocks = Block;

    /*VOS_Splx(intLockLevel);*/
    VOS_SpinUnlockIntUnlock(&g_stVosMemSpinLock, ulLockLevel);

    /* record the usage of control block */
    if(VOS_MemCtrlBlock->IdleBlockNumber < VOS_MemCtrlBlock->MinIdleBlockNumber)
    {
        VOS_MemCtrlBlock->MinIdleBlockNumber
            = VOS_MemCtrlBlock->IdleBlockNumber;
    }

    /* check memory */
    pulTemp = (VOS_UINT_PTR*)(Block->ulMemAddress);

    ulBlockAddr = *pulTemp++;
    ulCrcTag = *pulTemp;

    if ( ((VOS_UINT_PTR)Block != ulBlockAddr) || (VOS_MEMORY_CRC != ulCrcTag) )
    {
        VOS_DumpVosMem(Block, Block->ulMemAddress, VOS_ERRNO_MEMORY_ALLOC_CHECK, ulFileID, usLineNo);

        return((VOS_VOID*)VOS_NULL_PTR);
    }

    return (VOS_VOID *)Block->ulMemAddress;
}

/*****************************************************************************
 Function   : VOS_TIMER_MemCtrlBlkMalloc
 Description: allocate a timer message block
 Input      : ulLength -- length
 Return     : address
 Other      :
 *****************************************************************************/
VOS_VOID* VOS_TIMER_MemCtrlBlkMalloc( VOS_PID PID, VOS_INT ulLength )
{
    VOS_UINT_PTR         *pulSpace;
    VOS_MEM_HEAD_BLOCK   *pstTemp;

    pulSpace = (VOS_UINT_PTR *)VOS_MemCtrlBlkMalloc(
        &VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER-1], ulLength, 0, 0);
    if( VOS_NULL_PTR != pulSpace )
    {
        pstTemp = (VOS_MEM_HEAD_BLOCK *)(*pulSpace);
        pstTemp->ulcputickAlloc = VOS_GetSlice();
        pstTemp->ulAllocPid = PID;

        pstTemp->ulRealCtrlAddr
            = (VOS_UINT_PTR)(&VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER-1]);
        VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER-1].lRealNumber++;

        if ( VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER-1].lRealNumber
            > VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER-1].lMaxRealNumber )
        {
            VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER-1].lMaxRealNumber
                = VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER-1].lRealNumber;
        }

        /* offset space which be reserved of OSA */
        pulSpace += VOS_MEMORY_RESERVED_WORD_HEAD;
        return (VOS_VOID *)pulSpace;
    }

    (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MSG_MEMORY_FULL);

    LogPrint("# alloc timer msg fail.\r\n");

    VOS_ProtectionReboot(FAIL_TO_ALLOCATE_TIMER_MSG, 0,
                         (VOS_INT)ulLength, VOS_NULL_PTR, 0);

    return( VOS_NULL_PTR );
}

/*****************************************************************************
 Function   : VOS_MemCtrlBlkFree
 Description: free a block
 Input      :
            :
 Return     : VOS_OK on success or errno on failure.
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_MemCtrlBlkFree( VOS_MEM_CTRL_BLOCK *VOS_MemCtrlBlock,
                               VOS_MEM_HEAD_BLOCK *Block,
                               VOS_UINT32 ulFileID, VOS_INT32 usLineNo )
{
    VOS_ULONG             ulLockLevel;
    VOS_MEM_CTRL_BLOCK   *pstTemp;

    /*intLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosMemSpinLock, ulLockLevel);

    if ( VOS_NOT_USED == Block->ulMemUsedFlag )
    {
        /*VOS_Splx(intLockLevel);*/
        VOS_SpinUnlockIntUnlock(&g_stVosMemSpinLock, ulLockLevel);

        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_FREE_REPECTION);
        LogPrint3("# Free Msg again.F %d L %d.Tick %d.\r\n",
            (VOS_INT)ulFileID, usLineNo, (VOS_INT)VOS_GetSlice() );

        return VOS_ERR;
    }

    if ( Block == VOS_MemCtrlBlock->BusyBlocks )
    {
        VOS_MemCtrlBlock->BusyBlocks = Block->pstNext;
        if ( VOS_NULL_PTR != VOS_MemCtrlBlock->BusyBlocks )
        {
            VOS_MemCtrlBlock->BusyBlocks->pstPre= VOS_NULL_PTR;
        }
    }
    else
    {
        Block->pstPre->pstNext = Block->pstNext;
        if ( VOS_NULL_PTR != Block->pstNext )
        {
            (Block->pstNext)->pstPre = Block->pstPre;
        }
    }



    Block->ulMemUsedFlag = VOS_NOT_USED;
    Block->pstNext = VOS_MemCtrlBlock->Blocks;
    VOS_MemCtrlBlock->Blocks = Block;

    VOS_MemCtrlBlock->IdleBlockNumber++;

    pstTemp = (VOS_MEM_CTRL_BLOCK *)(Block->ulRealCtrlAddr);
    pstTemp->lRealNumber--;

    /*VOS_Splx(intLockLevel);*/
    VOS_SpinUnlockIntUnlock(&g_stVosMemSpinLock, ulLockLevel);

    return VOS_OK;
}

/*****************************************************************************
 Function   : VOS_MemBlkMalloc
 Description: allocate messagge memory
 Input      :
            :
 Return     : address
 Other      :
 *****************************************************************************/
VOS_VOID * VOS_MemBlkMalloc( VOS_PID PID, VOS_INT ulLength,
                            VOS_UINT32 ulFileID, VOS_INT32 usLineNo )
{
    VOS_INT              i;
    VOS_UINT_PTR         *pulSpace;
    VOS_MEM_HEAD_BLOCK   *pstTemp;
    VOS_BOOL             ulFlag = VOS_TRUE;
    VOS_INT              lTempSuffix = 0;

    if( 0 == ulLength )
    {
        return( VOS_NULL_PTR );
    }

    for ( i=0; i<VOS_MEM_CTRL_BLOCK_NUMBER-1; i++ )
    {
        if( ulLength <= MsgBlockInfo[i].size )
        {
            if ( VOS_TRUE == ulFlag )
            {
                ulFlag = VOS_FALSE;
                lTempSuffix = i;
            }
            pulSpace = (VOS_UINT_PTR *)VOS_MemCtrlBlkMalloc( &VOS_MemCtrlBlk[i],
                ulLength, ulFileID, usLineNo);
            if( VOS_NULL_PTR != pulSpace )
            {
                pstTemp = (VOS_MEM_HEAD_BLOCK *)(*pulSpace);
                pstTemp->ulAllocSize = (VOS_UINT32)ulLength;
                pstTemp->ulcputickAlloc = VOS_GetSlice();
                pstTemp->ulAllocPid = PID;
                pstTemp->aulMemRecord[0] = ulFileID;
                pstTemp->aulMemRecord[1] = (VOS_UINT32)usLineNo;


                pstTemp->ulRealCtrlAddr
                    = (VOS_UINT_PTR)(&VOS_MemCtrlBlk[lTempSuffix]);
                VOS_MemCtrlBlk[lTempSuffix].lRealNumber++;

                if ( VOS_MemCtrlBlk[lTempSuffix].lRealNumber
                    > VOS_MemCtrlBlk[lTempSuffix].lMaxRealNumber )
                {
                    VOS_MemCtrlBlk[lTempSuffix].lMaxRealNumber
                        = VOS_MemCtrlBlk[lTempSuffix].lRealNumber;
                }

                if ( VOS_MemCtrlBlk[lTempSuffix].MaxSize < ulLength )
                {
                    VOS_MemCtrlBlk[lTempSuffix].MaxSize = ulLength;
                }

                if ( VOS_MemCtrlBlk[lTempSuffix].MinSize > ulLength)
                {
                    VOS_MemCtrlBlk[lTempSuffix].MinSize = ulLength;
                }

                /* offset space which be reserved of OSA */
                pulSpace += VOS_MEMORY_RESERVED_WORD_HEAD;
                return (VOS_VOID *)pulSpace;
            }
        }
    }

    (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MSG_MEMORY_FULL);

    LogPrint3("# alloc msg fail size %d.F %d L %d.\r\n",
        (VOS_INT)ulLength, (VOS_INT)ulFileID, usLineNo);

    VOS_MsgDump(PID, (VOS_UINT32)ulLength, ulFileID, usLineNo);

    return( VOS_NULL_PTR );
}

/*****************************************************************************
 Function   : V_MemAlloc
 Description: allocate memory
 Input      :
            :
 Return     : address or NULL
 Other      :
 *****************************************************************************/
VOS_VOID * V_MemAlloc( VOS_UINT32 ulInfo, VOS_UINT8  ucPtNo, VOS_UINT32 ulSize,
                        VOS_UINT32 ulRebootFlag, VOS_UINT32 ulFileID, VOS_INT32 usLineNo  )
{
    int                  i;
    VOS_UINT_PTR         *pulSpace;
    VOS_MEM_HEAD_BLOCK   *pstTemp;
    VOS_MEM_BLOCK_INFO   *pstMemInfo;
    VOS_MEM_CTRL_BLOCK   *pstMemCtrl;
    VOS_INT              lCycle;
    VOS_BOOL             ulFlag = VOS_TRUE;
    VOS_INT              lTempSuffix = 0;

    if( MEM_PT_BUTT <= ucPtNo )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_ALLOC_INPUTMSGISNULL);

        Print3("# V_MemAlloc partion invalid, F %d L %d P %d.\r\n",
                ulFileID, usLineNo, ulInfo);

        return VOS_NULL_PTR;
    }

    if( 0 == ulSize )
    {
        Print2("# V_MemAlloc size is 0,F %d L %d.\r\n", ulFileID, usLineNo);

        return VOS_NULL_PTR;
    }

    /* 如果申请的空间大小超过0x7FFFFFFF个Byte，直接返回空指针 */
    if (0x7FFFFFFF < ulSize)
    {
        Print2("# V_MemAlloc size over 0x7FFFFFFF,FileID: %d LineNo: %d.\r\n", ulFileID, usLineNo);

        return VOS_NULL_PTR;
    }

    pstMemInfo = MemBlockInfo;

    lCycle = VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER;

    pstMemCtrl = VOS_SimpleMemCtrlBlk;

    for ( i=0; i<lCycle; i++ )
    {
        if( (VOS_INT)ulSize <= pstMemInfo[i].size )
        {
            if ( VOS_TRUE == ulFlag )
            {
                ulFlag = VOS_FALSE;
                lTempSuffix = i;
            }
            pulSpace = (VOS_UINT_PTR *)VOS_MemCtrlBlkMalloc(
                &(pstMemCtrl[i]), (VOS_INT)ulSize, ulFileID, usLineNo);
            if( VOS_NULL_PTR != pulSpace )
            {
                pstTemp = (VOS_MEM_HEAD_BLOCK *)(*pulSpace);
                pstTemp->ulAllocSize = ulSize;
                pstTemp->ulcputickAlloc = VOS_GetSlice();
                pstTemp->ulAllocPid = ulInfo;
                pstTemp->aulMemRecord[0] = ulFileID;
                pstTemp->aulMemRecord[1] = (VOS_UINT32)usLineNo;
                /* record for check the leak of memory automatic */
                pstTemp->aulMemRecord[2] = (VOS_UINT32)ucPtNo;


                pstTemp->ulRealCtrlAddr
                    = (VOS_UINT_PTR)(&(pstMemCtrl[lTempSuffix]));

                pstMemCtrl[lTempSuffix].lRealNumber++;

                if ( pstMemCtrl[lTempSuffix].lRealNumber
                    > pstMemCtrl[lTempSuffix].lMaxRealNumber )
                {
                    pstMemCtrl[lTempSuffix].lMaxRealNumber
                        = pstMemCtrl[lTempSuffix].lRealNumber;
                }

                if ( pstMemCtrl[lTempSuffix].MaxSize < (VOS_INT)ulSize)
                {
                    pstMemCtrl[lTempSuffix].MaxSize = (VOS_INT)ulSize;
                }

                if ( pstMemCtrl[lTempSuffix].MinSize > (VOS_INT)ulSize)
                {
                    pstMemCtrl[lTempSuffix].MinSize = (VOS_INT)ulSize;
                }

                /* offset space which be reserved of OSA */
                pulSpace += VOS_MEMORY_RESERVED_WORD_HEAD;


                return (VOS_VOID *)pulSpace;
            }
        }
    }

    (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_FULL);

    LogPrint3("# alloce memory fail size %d.F %d L %d.\r\n",
        (VOS_INT)ulSize, (VOS_INT)ulFileID, usLineNo);

    if ( VOS_FALSE == ulRebootFlag )
    {
        return VOS_NULL_PTR;
    }

    (VOS_VOID)VOS_MemDump(ulInfo, ulSize, ulFileID, usLineNo, pstMemCtrl, lCycle);

    return VOS_NULL_PTR;
}

/*****************************************************************************
 Function   : VOS_SearchMem
 Description: calc mem's real size
 Input      :
            :
 Return     : VOS_OK on success or errno on failure.
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_CalcMem(VOS_MEM_CTRL_BLOCK *pstMemCtrl, VOS_UINT32 *pulRealSize,
                        VOS_LOCATION_MEM_INFO_STRU stInfo)
{
    VOS_UINT_PTR ulMemAddress;
    VOS_INT    i;
    VOS_INT    suffix = 0x0fffffff;
    VOS_UINT32 ulQuotient;
    VOS_UINT32 ulActualSize;
    VOS_UINT_PTR ulStartAddress;
    VOS_UINT_PTR ulEndAddress;
    VOS_UINT_PTR *pulstart;
    VOS_UINT_PTR *pulEnd;

    ulMemAddress = stInfo.ulAddress;

    for ( i=0; i<stInfo.lCycle; i++ )
    {
        if ( (ulMemAddress >= pstMemCtrl[i].Buffer)
            && (ulMemAddress < pstMemCtrl[i].BufferEnd) )
        {
            suffix = i;
            break;
        }
    }

    if ( 0x0fffffff == suffix )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_GLOBAL_COVER);

        LogPrint2("# MEM Global Cover: F %d L %d.\r\n",
            (VOS_INT)stInfo.ulFileID, stInfo.usLineNo);

        return(VOS_ERRNO_MEMORY_GLOBAL_COVER);
    }

    ulActualSize = (VOS_UINT32)(VOS_MEMORY_RESERVED_BYTES
        + pstMemCtrl[suffix].BlockLength);

    ulQuotient = (ulMemAddress - pstMemCtrl[suffix].Buffer)
        / ulActualSize;
    /*lint -e647*/
    ulStartAddress = pstMemCtrl[suffix].Buffer +
        (ulQuotient * ulActualSize);
    /*lint +e647*/
    ulEndAddress = ulStartAddress + ulActualSize;

    pulstart = (VOS_UINT_PTR *)ulStartAddress;
    pulEnd = (VOS_UINT_PTR *)ulEndAddress;

    pulstart++;/* offset Block */

    if ( VOS_MEMORY_CRC != *pulstart )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_HEAD_COVER);

        LogPrint2("# MEM HEAD FAIL: F %d L %d.\r\n",
            (VOS_INT)stInfo.ulFileID, stInfo.usLineNo);

        return(VOS_ERRNO_MEMORY_HEAD_COVER);
    }

    pulstart++;/* offset protection */

    pulEnd--;/* offset protection */

    if ( VOS_MEMORY_CRC != *pulEnd )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_TAIL_COVER);

        LogPrint2("# MEM TAIL FAIL: F %d L %d.\r\n",
            (VOS_INT)stInfo.ulFileID, stInfo.usLineNo);

        return(VOS_ERRNO_MEMORY_TAIL_COVER);
    }

    ulStartAddress = (VOS_UINT_PTR)pulstart;

    if ( VOS_LOCATION_MSG == stInfo.ulType)
    {
        ulStartAddress += VOS_MSG_BLK_HEAD_LEN;
    }

    ulEndAddress = (VOS_UINT_PTR)pulEnd;

    if ( (ulMemAddress < ulStartAddress) || (ulMemAddress >= ulEndAddress) )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_LOCATION_CANNOTDO);

        LogPrint2("# address out space: F %d L %d.\r\n",
            (VOS_INT)stInfo.ulFileID, stInfo.usLineNo);

        return(VOS_ERRNO_MEMORY_LOCATION_CANNOTDO);
    }

    *pulRealSize = (VOS_UINT32)(ulEndAddress - ulMemAddress);

    return VOS_OK;
}

/*****************************************************************************
 Function   : VOS_LocationMem
 Description: location mem's head
 Input      :
            :
 Return     : VOS_OK on success or errno on failure.
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_LocationMem( VOS_VOID *pAddr, VOS_UINT32 *pulRealSize,
                            VOS_UINT32 ulFileID, VOS_INT32 usLineNo )
{
    VOS_UINT_PTR                ulMemAddress;
    VOS_LOCATION_MEM_INFO_STRU  stInfo;

    ulMemAddress = (VOS_UINT_PTR)pAddr;

    if ( (ulMemAddress < gulVosSpaceAndProtectionStart)
        || (ulMemAddress > gulVosSpaceAndProtectionEnd) )
    {
        *pulRealSize = 0xffffffff;
        return VOS_OK;
    }

    stInfo.ulAddress = ulMemAddress;
    stInfo.ulFileID = ulFileID;
    stInfo.usLineNo = usLineNo;

    /* MSG */
    if ((ulMemAddress >= g_ulMemPoolIntervalAddress[0]) && (ulMemAddress < g_ulMemPoolIntervalAddress[1]))
    {
        stInfo.lCycle = VOS_MEM_CTRL_BLOCK_NUMBER-1;
        stInfo.ulType = VOS_LOCATION_MSG;

        return VOS_CalcMem(VOS_MemCtrlBlk, pulRealSize, stInfo);
    }

    /* memory */
    if ( (ulMemAddress >= g_ulMemPoolIntervalAddress[1]) && (ulMemAddress < g_ulMemPoolIntervalAddress[2]))
    {
        stInfo.lCycle = VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER;
        stInfo.ulType = VOS_LOCATION_MEM;

        return VOS_CalcMem(VOS_SimpleMemCtrlBlk, pulRealSize, stInfo);
    }

    (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_LOCATION_INPUTINVALID);

    LogPrint2("# OSA MEM INPUTINVALID: F %d L %d.\r\n",
        (VOS_INT)ulFileID, usLineNo);

    return VOS_ERRNO_MEMORY_LOCATION_INPUTINVALID;
}


/*****************************************************************************
 Function   : VOS_MemCheck
 Description: check memory is OSA's or not
 Input      :
            :
 Return     : VOS_OK on success or errno on failure.
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_MemCheck( VOS_VOID *pAddr, VOS_UINT_PTR *pulBlock,
                         VOS_UINT_PTR *pulCtrl, VOS_UINT32 ulFileID,
                         VOS_INT32 usLineNo )
{
    VOS_UINT_PTR        *pulTemp;
    VOS_UINT_PTR        ulHeadValue;
    VOS_UINT_PTR        ulTailValue;
    VOS_UINT_PTR        ulBlockAddress;
    VOS_MEM_HEAD_BLOCK  *pstHeadBlock;
    VOS_MEM_CTRL_BLOCK  *pstMemCtrl;
    VOS_UINT_PTR        ulTempAddress;
    VOS_UINT_PTR        ulUserAddress;

    ulTempAddress = (VOS_UINT_PTR)pAddr;

    if ( ulTempAddress < gulVosSpaceAndProtectionStart
        || ulTempAddress > gulVosSpaceAndProtectionEnd )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_NON_DOPRAMEM);

        LogPrint2("# NOT OSA MEM: F %d L %d.\r\n",
            (VOS_INT)ulFileID, usLineNo);

        return(VOS_ERRNO_MEMORY_NON_DOPRAMEM);
    }

    if ( ulTempAddress <= gulVosSpaceStart
        || ulTempAddress >= gulVosSpaceEnd )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_DOPRAMEM_OVERFLOW);

        LogPrint2("# OSA MEM OVERFLOW: F %d L %d.\r\n",
            (VOS_INT)ulFileID, usLineNo);

        return(VOS_ERRNO_MEMORY_DOPRAMEM_OVERFLOW);
    }

    pulTemp = (VOS_UINT_PTR *)pAddr;

    pulTemp -= VOS_MEMORY_RESERVED_WORD_HEAD;

    ulUserAddress = (VOS_UINT_PTR)pulTemp;

    ulBlockAddress = *pulTemp;
    pulTemp++;
    ulHeadValue = *pulTemp;
    pulTemp++;

    if ( VOS_MEMORY_CRC != ulHeadValue )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_HEAD_COVER);

        VOS_DumpVosMem(VOS_NULL_PTR, ulUserAddress, VOS_ERRNO_MEMORY_HEAD_COVER, ulFileID, usLineNo);

        LogPrint2("# MEM HEAD FAIL: F %d L %d.\r\n",
            (VOS_INT)ulFileID, usLineNo);

        return(VOS_ERRNO_MEMORY_HEAD_COVER);
    }

    pstHeadBlock = (VOS_MEM_HEAD_BLOCK *)ulBlockAddress;
    if ( pstHeadBlock->ulMemAddress != ulUserAddress)
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_LINK_COVER);

        VOS_DumpVosMem(VOS_NULL_PTR, ulUserAddress, VOS_ERRNO_MEMORY_LINK_COVER, ulFileID, usLineNo);

        LogPrint2("# MEM LINK FAIL: F %d L %d.\r\n",
            (VOS_INT)ulFileID, usLineNo);

        return(VOS_ERRNO_MEMORY_LINK_COVER);
    }

    *pulBlock = ulBlockAddress;

    ulTempAddress = pstHeadBlock->ulMemCtrlAddress;
    pstMemCtrl = (VOS_MEM_CTRL_BLOCK *)ulTempAddress;

    *pulCtrl = ulTempAddress;

    pulTemp = (VOS_UINT_PTR *)( (VOS_UINT_PTR)pulTemp + pstMemCtrl->BlockLength );

    ulTailValue = *pulTemp;

    if ( VOS_MEMORY_CRC != ulTailValue )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_TAIL_COVER);

        VOS_DumpVosMem(pstHeadBlock, ulUserAddress, VOS_ERRNO_MEMORY_TAIL_COVER, ulFileID, usLineNo);

        LogPrint2("# MEM TAIL FAIL: F %d L %d.\r\n",
            (VOS_INT)ulFileID, usLineNo);

        return(VOS_ERRNO_MEMORY_TAIL_COVER);
    }

    return VOS_OK;
}


/*****************************************************************************
 Function   : V_MemFree
 Description: free memory
 Input      :
            :
 Return     : VOS_OK on success or errno on failure.
 Other      :
 *****************************************************************************/
VOS_UINT32 V_MemFree( VOS_UINT32 ulInfo, VOS_VOID **ppAddr,
                          VOS_UINT32 ulFileID, VOS_INT32 usLineNo )
{
    VOS_UINT_PTR                        ulBlockAdd;
    VOS_UINT_PTR                        ulCtrlkAdd;

    if( VOS_NULL_PTR == ppAddr )
    {
        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_FREE_INPUTPIDINVALID);
        return(VOS_ERRNO_MEMORY_FREE_INPUTPIDINVALID);
    }

    if( VOS_NULL_PTR == *ppAddr )
    {
        LogPrint3("# V_MemFree,free mem again.F %d L %d T %d.\r\n",
            (VOS_INT)ulFileID, usLineNo, (VOS_INT)VOS_GetSlice() );

        (VOS_VOID)VOS_SetErrorNo(VOS_ERRNO_MEMORY_FREE_INPUTPIDINVALID);
        return(VOS_ERRNO_MEMORY_FREE_INPUTPIDINVALID);
    }

    if ( VOS_OK != VOS_MemCheck( *ppAddr, &ulBlockAdd, &ulCtrlkAdd,
        ulFileID, usLineNo ) )
    {
        return VOS_ERR;
    }

    *ppAddr = VOS_NULL_PTR;


    return VOS_MemCtrlBlkFree( (VOS_MEM_CTRL_BLOCK *)ulCtrlkAdd,
            (VOS_MEM_HEAD_BLOCK *)ulBlockAdd, ulFileID, usLineNo );
}

/*****************************************************************************
 Function   : VOS_MemReAlloc
 Description: realloc memory
 Input      : ucPtNo -- partition number.
            : ulInfo -- PIDSID information.
            : pOldMemPtr -- address of old memory.
            : ulNewSize -- size to realloc.
 Return     : Address
 Other      :
 *****************************************************************************/
VOS_VOID *VOS_MemReAlloc ( VOS_UINT32 ulPid, VOS_UINT8 ucPtNo,
                           VOS_VOID *pOldMemPtr, VOS_UINT32 ulNewSize )
{
    VOS_VOID *pTemp;

    if ( VOS_NULL_PTR == pOldMemPtr )
    {
        return VOS_MemAlloc( ulPid, ucPtNo, ulNewSize );
    }

    if ( 0 == ulNewSize )
    {
        (VOS_VOID)VOS_MemFree( ulPid, pOldMemPtr );

        return VOS_NULL_PTR;
    }

    pTemp = VOS_MemAlloc( ulPid, ucPtNo, ulNewSize );
    if ( VOS_NULL_PTR == pTemp )
    {
        (VOS_VOID)VOS_MemFree( ulPid, pOldMemPtr );

        return VOS_NULL_PTR;
    }

    if ( VOS_NULL_PTR == VOS_MemCpy_s(pTemp, ulNewSize, pOldMemPtr, ulNewSize) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMCPY_MEM, (VOS_INT)ulNewSize, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    (VOS_VOID)VOS_MemFree( ulPid, pOldMemPtr );

    return pTemp;
}

/*****************************************************************************
 Function   : VOS_RefreshMemInfo
 Description: write memory info to flash
 Input      : the value on flash.
            :
 Return     : no
 Other      :
 *****************************************************************************/
VOS_VOID VOS_RefreshMemInfo( VOS_MEM_RECORD_ST *pstPara,
                                VOS_MEM_CTRL_BLOCK *pstMemCtrl, VOS_INT lCycle)
{
    VOS_INT           i;

    for ( i=0; i<lCycle; i++ )
    {
        if ( pstMemCtrl[i].MaxSize > pstPara[i].MaxSize )
        {
            pstPara[i].MaxSize = pstMemCtrl[i].MaxSize;
        }

        if ( pstMemCtrl[i].MinSize < pstPara[i].MinSize )
        {
            pstPara[i].MinSize = pstMemCtrl[i].MinSize;
        }

        if ( pstMemCtrl[i].lMaxRealNumber > pstPara[i].lMaxRealNumber )
        {
            pstPara[i].lMaxRealNumber = pstMemCtrl[i].lMaxRealNumber;
        }
    }

    return;
}

/*****************************************************************************
 Function   : VOS_RecoedmemInfo
 Description: write memory info to flash
 Input      : the value on flash.
            :
 Return     : no
 Other      :
 *****************************************************************************/
VOS_VOID VOS_RecordmemInfo( VOS_MEM_RECORD_ST *pstPara )
{
    VOS_MEM_RECORD_ST *pstTemp;

    pstTemp = pstPara;

    VOS_RefreshMemInfo(pstTemp, VOS_MemCtrlBlk, VOS_MEM_CTRL_BLOCK_NUMBER);

    pstTemp = pstPara + VOS_MEM_CTRL_BLOCK_NUMBER;

    VOS_RefreshMemInfo(pstTemp, VOS_SimpleMemCtrlBlk, VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER);


    return;
}


/*****************************************************************************
 Function   : VOS_show_memory_info
 Description: print memory use info
 Input      : void
 Return     : void
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_show_memory_info(VOS_VOID)
{
    int i;

    for ( i=0; i<VOS_MEM_CTRL_BLOCK_NUMBER; i++)
    {
        Print2("MSG %d Max is %d.\r\n",i,VOS_MemCtrlBlk[i].MaxSize);
        Print2("MSG %d Min is %d.\r\n",i,VOS_MemCtrlBlk[i].MinSize);
        Print2("MSG %d Real number is %d.\r\n",i,VOS_MemCtrlBlk[i].lMaxRealNumber);
        Print2("MSG %d Max use is %d.\r\n",i,
            VOS_MemCtrlBlk[i].TotalBlockNumber
            - VOS_MemCtrlBlk[i].MinIdleBlockNumber);
        Print1("%s", "\r\n");
    }

    for ( i=0; i<VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER; i++)
    {
        Print2("MEM %d Max is %d.\r\n",i,VOS_SimpleMemCtrlBlk[i].MaxSize);
        Print2("MEM %d Min is %d.\r\n",i,VOS_SimpleMemCtrlBlk[i].MinSize);
        Print2("MEM %d Real number is %d.\r\n",i,VOS_SimpleMemCtrlBlk[i].lMaxRealNumber);
        Print2("MEM %d Max use is %d.\r\n",i,
            VOS_SimpleMemCtrlBlk[i].TotalBlockNumber
            - VOS_SimpleMemCtrlBlk[i].MinIdleBlockNumber);
        Print1("%s", "\r\n");
    }
}



/*****************************************************************************
 Function   : VOS_show_used_msg_info
 Description: print used msg's useage info
 Input      : void
 Return     : void
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_show_used_msg_info(VOS_UINT32 Para0, VOS_UINT32 Para1,
                                VOS_UINT32 Para2, VOS_UINT32 Para3 )
{
    int                 i;
    VOS_MEM_HEAD_BLOCK  *pstTemp;

    for ( i=VOS_MEM_CTRL_BLOCK_NUMBER-1; i>=0; i--)
    {
        Print1("messge Ctrl %d.\r\n",i);

        pstTemp = VOS_MemCtrlBlk[i].BusyBlocks;

        while ( VOS_NULL_PTR != pstTemp)
        {
            Print5("F %d L %d P %d S %d T %u.\r\n",
                pstTemp->aulMemRecord[0],
                pstTemp->aulMemRecord[1],
                pstTemp->ulAllocPid,
                pstTemp->ulAllocSize,
                pstTemp->ulcputickAlloc);

            pstTemp = pstTemp->pstNext;
        }
    }
}

/*****************************************************************************
 Function   : VOS_show_used_timer_msg_info
 Description: print used timer msg's useage info
 Input      : void
 Return     : void
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_show_used_timer_msg_info(VOS_UINT32 Para0, VOS_UINT32 Para1,
                                VOS_UINT32 Para2, VOS_UINT32 Para3 )
{
    VOS_MEM_HEAD_BLOCK  *pstTemp;

    Print1("%s", "Timer messge used info.\r\n");

    pstTemp = VOS_MemCtrlBlk[VOS_MEM_CTRL_BLOCK_NUMBER-1].BusyBlocks;

    while ( VOS_NULL_PTR != pstTemp)
    {
        Print4("F %d L %d P %d T %u.\r\n",
            pstTemp->aulMemRecord[0],
            pstTemp->aulMemRecord[1],
            pstTemp->ulAllocPid,
            pstTemp->ulcputickAlloc);

        pstTemp = pstTemp->pstNext;
    }
}

/*****************************************************************************
 Function   : VOS_show_used_memory_info
 Description: print used memory's useage info
 Input      : void
 Return     : void
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_show_used_memory_info(VOS_UINT32 Para0, VOS_UINT32 Para1,
                                   VOS_UINT32 Para2, VOS_UINT32 Para3)
{
    int                 i;
    VOS_MEM_HEAD_BLOCK  *pstTemp;
    VOS_MEM_CTRL_BLOCK  *pstMemCtrl;
    VOS_INT             lCycle = (VOS_INT)Para1;
    VOS_UINT_PTR        TempValue = (VOS_UINT_PTR)Para0;

    pstMemCtrl = (VOS_MEM_CTRL_BLOCK*)TempValue;

    for ( i=lCycle-1; i>=0; i--)
    {
        Print1("memory Ctrl %d.\r\n",i);

        pstTemp = pstMemCtrl[i].BusyBlocks;

        while ( VOS_NULL_PTR != pstTemp)
        {
            Print5("F %d L %d P %d S %d T %u.\r\n",
                pstTemp->aulMemRecord[0],
                pstTemp->aulMemRecord[1],
                pstTemp->ulAllocPid,
                pstTemp->ulAllocSize,
                pstTemp->ulcputickAlloc);

            pstTemp = pstTemp->pstNext;
        }
    }
}

/*****************************************************************************
 Function   : VOS_PrintUsedMsgInfo
 Description: print used MSG's useage info
 Input      : void
 Return     : void
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_PrintUsedMsgInfo(VOS_VOID)
{




}

/*****************************************************************************
 Function   : VOS_PrintUsedTimerMsgInfo
 Description: print used MSG's useage info
 Input      : void
 Return     : void
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_PrintUsedTimerMsgInfo(VOS_VOID)
{




}


/*****************************************************************************
 Function   : VOS_print_used_memory_info
 Description: show used memory's useage info
 Input      : void
 Return     : void
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_PrintUsedMemoryInfo(VOS_MEM_CTRL_BLOCK *pstMemCtrl, VOS_INT lCycle)
{



}

/*****************************************************************************
 Function   : VOS_FatalError
 Description: User suspend system
 Input      : void
 Return     : void
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_FatalError(VOS_VOID)
{
#if ((VOS_VXWORKS == VOS_OS_VER)||(VOS_NUCLEUS == VOS_OS_VER) \
        ||(VOS_WIN32 == VOS_OS_VER)||(VOS_RTOSCK == VOS_OS_VER))
    VOS_UINT32      TempTaskID;
    VOS_UINT32      Argument[4] = {0};
#endif

    LogPrint("# VOS_FatalError,system halt.\r\n");





}

/*****************************************************************************
 Function   : VOS_FindLeakMem
 Description: check the leak of memory
 Input      : void
 Return     : VOS_OK or VOS_ERR
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_FindLeakMem(VOS_MEM_CTRL_BLOCK *pstMemCtrl, VOS_INT lCycle, VOS_UINT32 ulType)
{
    int                 i;
    VOS_MEM_HEAD_BLOCK  *pstTemp;
    VOS_UINT32          ulTempTick;

    ulTempTick = VOS_GetSlice();

    for ( i=lCycle-1; i>=0; i--)
    {
        pstTemp = pstMemCtrl[i].BusyBlocks;

        while ( VOS_NULL_PTR != pstTemp)
        {
            if ( VOS_LOCATION_MEM == ulType )
            {
                if ( (STATIC_DOPRA_MEM_PT == pstTemp->aulMemRecord[2])
                      || (STATIC_MEM_PT == pstTemp->aulMemRecord[2]))
                {
                    pstTemp = pstTemp->pstNext;

                    continue;
                }
            }


            if ( (ulTempTick - pstTemp->ulcputickAlloc)
                >= g_ulAutoCheckMemoryThreshold )
            {
                LogPrint5("# V_AutoCheckMemory: F %d L %d P %d S %d T %u.\r\n",
                        (VOS_INT)pstTemp->aulMemRecord[0],
                        (VOS_INT)pstTemp->aulMemRecord[1],
                        (VOS_INT)pstTemp->ulAllocPid,
                        (VOS_INT)pstTemp->ulAllocSize,
                        (VOS_INT)pstTemp->ulcputickAlloc);
            }

            pstTemp = pstTemp->pstNext;
        }
    }
}

/*****************************************************************************
 Function   : VOS_AutoCheckMemory
 Description: check the leak of memory automatic
 Input      : void
 Return     : VOS_OK or VOS_ERR
 Other      : only for designer
 *****************************************************************************/
VOS_VOID VOS_AutoCheckMemory( VOS_VOID )
{
    VOS_ULONG           ulLockLevel;
    VOS_UINT32          ulTempTick;


    ulTempTick = VOS_GetSlice();

    Print1("# Auto Check Memory begin at T %d.\r\n", ulTempTick );

    /*intLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosMemSpinLock, ulLockLevel);

    VOS_FindLeakMem(VOS_MemCtrlBlk, VOS_MEM_CTRL_BLOCK_NUMBER, VOS_LOCATION_MSG);

    VOS_FindLeakMem(VOS_SimpleMemCtrlBlk, VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER, VOS_LOCATION_MEM);

    /*VOS_Splx(intLockLevel);*/
    VOS_SpinUnlockIntUnlock(&g_stVosMemSpinLock, ulLockLevel);

    Print1("# V_AutoCheckMemory end at T %d.\r\n", VOS_GetSlice() );

    return;
}


/********************************************************************
 Function   : VOS_MemDump
 Description: Record the info. of every memory block, before UE reboots.
 Input      :
 Return     :
 Other      :
 *****************************************************************************/
VOS_VOID VOS_MemDump(VOS_UINT32 ulInfo, VOS_UINT32 ulSize,VOS_UINT32 ulFileID,
                VOS_INT32 ulLineNo,VOS_MEM_CTRL_BLOCK *pstMemCtrl, VOS_INT lCycle)
{
    VOS_INT             i;
    VOS_UINT32          ulTotalSize = 0;
    VOS_UINT32          *pulDumpBuffer;
    VOS_MEM_HEAD_BLOCK  *pstTmpMemHead;

    (VOS_VOID)VOS_TaskLock();

    pulDumpBuffer = (VOS_UINT32*)VOS_EXCH_MEM_MALLOC;

    if (VOS_NULL_PTR == pulDumpBuffer)
    {
        VOS_ProtectionReboot(FAIL_TO_ALLOCATE_MEM, (VOS_INT)ulFileID,
                             (VOS_INT)ulLineNo, (VOS_CHAR *)&ulSize, sizeof(ulSize));
        (VOS_VOID)VOS_TaskUnlock();
        return;
    }

    if ( VOS_NULL_PTR == VOS_MemSet_s(pulDumpBuffer, VOS_DUMP_MEM_TOTAL_SIZE, 0, VOS_DUMP_MEM_TOTAL_SIZE) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    *pulDumpBuffer++ = ulFileID;
    *pulDumpBuffer++ = (VOS_UINT32)ulLineNo;
    *pulDumpBuffer++ = ulInfo;
    *pulDumpBuffer++ = ulSize;
    *pulDumpBuffer++ = VOS_GetSlice();

    ulTotalSize += VOS_MEM_RECORD_BLOCK_SIZE;

    /*Get every memory block info.*/
    for ( i=lCycle-1; i>=0; i--)
    {
        pstTmpMemHead = pstMemCtrl[i].BusyBlocks;

        while ( VOS_NULL_PTR != pstTmpMemHead)
        {
            *pulDumpBuffer++ = pstTmpMemHead->aulMemRecord[0];
            *pulDumpBuffer++ = pstTmpMemHead->aulMemRecord[1];
            *pulDumpBuffer++ = pstTmpMemHead->ulAllocPid;
            *pulDumpBuffer++ = pstTmpMemHead->ulAllocSize;
            *pulDumpBuffer++ = pstTmpMemHead->ulcputickAlloc;

            ulTotalSize += VOS_MEM_RECORD_BLOCK_SIZE;

            if ((ulTotalSize + VOS_MEM_RECORD_BLOCK_SIZE) > VOS_DUMP_MEM_TOTAL_SIZE)
            {
                /*Jump from for loop*/
                i = -1;
                break;
            }
            pstTmpMemHead = pstTmpMemHead->pstNext;
        }
    }

    VOS_ProtectionReboot(FAIL_TO_ALLOCATE_MEM, (VOS_INT)ulFileID,
                         (VOS_INT)ulLineNo, VOS_NULL_PTR, 0);

    (VOS_VOID)VOS_TaskUnlock();
    return;
}

/*****************************************************************************
 Function   : VOS_MsgDump
 Description: Record the info. of every message block, before UE reboots.
 Input      :
 Return     :
 Other      :
 *****************************************************************************/
VOS_VOID VOS_MsgDump(VOS_UINT32 ulInfo, VOS_UINT32 ulSize,
                    VOS_UINT32 ulFileID, VOS_INT32 ulLineNo)
{
    VOS_INT             i;
    VOS_UINT32          ulTotalSize = 0;
    VOS_UINT32          *pulDumpBuffer;
    VOS_MEM_HEAD_BLOCK  *pstTmpMemHead;
    VOS_UINT_PTR         ulAddress;

    (VOS_VOID)VOS_TaskLock();

    pulDumpBuffer = (VOS_UINT32*)VOS_EXCH_MEM_MALLOC;

    if (VOS_NULL_PTR == pulDumpBuffer)
    {
        VOS_ProtectionReboot(FAIL_TO_ALLOCATE_MSG, (VOS_INT)ulFileID,
                             (VOS_INT)ulLineNo, (VOS_CHAR *)&ulSize, sizeof(ulSize));
        (VOS_VOID)VOS_TaskUnlock();
        return;
    }

    if ( VOS_NULL_PTR == VOS_MemSet_s(pulDumpBuffer, VOS_DUMP_MEM_TOTAL_SIZE, 0, VOS_DUMP_MEM_TOTAL_SIZE) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    *(pulDumpBuffer++) = ulFileID;
    *(pulDumpBuffer++) = (VOS_UINT32)ulLineNo;
    *(pulDumpBuffer++) = ulInfo;
    *(pulDumpBuffer++) = ulSize;
    *(pulDumpBuffer++) = VOS_GetSlice();

    ulTotalSize += VOS_MEM_RECORD_BLOCK_SIZE;

    /*Get every message block info.*/
    for ( i=VOS_MEM_CTRL_BLOCK_NUMBER-1; i>=0; i--)
    {
        pstTmpMemHead = VOS_MemCtrlBlk[i].BusyBlocks;

        while ( VOS_NULL_PTR != pstTmpMemHead)
        {
            /* 省出一个阈来存放 cpu tick */
            *(pulDumpBuffer++) = (pstTmpMemHead->aulMemRecord[0] << 16)|(pstTmpMemHead->aulMemRecord[1] & 0x0000ffff);
            *(pulDumpBuffer++) = pstTmpMemHead->ulcputickAlloc;

            ulAddress = pstTmpMemHead->ulMemAddress;
            ulAddress += VOS_MEMORY_RESERVED_WORD_HEAD*sizeof(VOS_UINT32);

            *(pulDumpBuffer++) = VOS_GetMsgName(ulAddress);
            *(pulDumpBuffer++) = pstTmpMemHead->ulAllocSize;
            *(pulDumpBuffer++) = pstTmpMemHead->ulAllocPid;

            ulTotalSize += VOS_MEM_RECORD_BLOCK_SIZE;

            if ((ulTotalSize + VOS_MEM_RECORD_BLOCK_SIZE) > VOS_DUMP_MEM_TOTAL_SIZE)
            {
                /*Jump from for loop*/
                i = -1;
                break;
            }

            pstTmpMemHead = pstTmpMemHead->pstNext;
        }
    }

    VOS_ProtectionReboot(FAIL_TO_ALLOCATE_MSG, (VOS_INT)ulFileID,
                         (VOS_INT)ulLineNo, VOS_NULL_PTR, 0);

    (VOS_VOID)VOS_TaskUnlock();

    return;
}

/*****************************************************************************
 Function   : VOS_GetFreeMemoryInfo
 Description: get memory info
 Input      :
 Return     :
 Other      : only for L2
 *****************************************************************************/
VOS_UINT32 VOS_GetFreeMemoryInfo(VOS_UINT32 ulSize, VOS_UINT32 *pulFreeBlockNum,
                                 VOS_UINT32 *pulTotalBlockNum)
{
    VOS_UINT32 i;
    VOS_UINT32 j;
    VOS_UINT32 ulFreeBlockNum = 0;
    VOS_UINT32 ulTotalBlockNum = 0;

    if ( VOS_NULL_PTR == pulFreeBlockNum )
    {
        return VOS_ERR;
    }

    if (VOS_NULL_PTR == pulTotalBlockNum )
    {
        return VOS_ERR;
    }

    if( 0 == ulSize )
    {
        return VOS_ERR;
    }

    for ( i=0; i<VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER; i++ )
    {
        if( (VOS_INT)ulSize <= MemBlockInfo[i].size )
        {
            for ( j=i; j<VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER; j++ )
            {
                ulFreeBlockNum += VOS_SimpleMemCtrlBlk[j].IdleBlockNumber;
                ulTotalBlockNum += MemBlockInfo[j].number;
            }
            *pulFreeBlockNum = ulFreeBlockNum;
            *pulTotalBlockNum = ulTotalBlockNum;

            return VOS_OK;
        }
    }

    return VOS_ERR;
}

/*****************************************************************************
 Function   : VOS_CalcMsgInfo
 Description: calc msg info
 Input      :
 Return     :
 Other      : only for count
 *****************************************************************************/
VOS_BOOL VOS_CalcMsgInfo(VOS_VOID)
{
    VOS_UINT32 i;
    VOS_UINT32 ulFreeBlockNum = 0;
    VOS_UINT32 ulTotalBlockNum = 0;

    for ( i=0; i<VOS_MEM_CTRL_BLOCK_NUMBER; i++ )
    {
        ulFreeBlockNum += VOS_MemCtrlBlk[i].IdleBlockNumber;
        ulTotalBlockNum += MsgBlockInfo[i].number;
    }

    if ( ulFreeBlockNum < (ulTotalBlockNum / VOS_CALC_MEM_RATIO) )
    {
        return VOS_TRUE;
    }

    return VOS_FALSE;
}

/*****************************************************************************
 Function   : VOS_CalcMemInfo
 Description: calc mem info
 Input      :
 Return     :
 Other      : only for count
 *****************************************************************************/
VOS_BOOL VOS_CalcMemInfo(VOS_VOID)
{
    VOS_UINT32 i;
    VOS_UINT32 ulFreeBlockNum = 0;
    VOS_UINT32 ulTotalBlockNum = 0;

    for ( i=0; i<VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER; i++ )
    {
        ulFreeBlockNum += VOS_SimpleMemCtrlBlk[i].IdleBlockNumber;
        ulTotalBlockNum += MemBlockInfo[i].number;
    }

    if ( ulFreeBlockNum < (ulTotalBlockNum / VOS_CALC_MEM_RATIO) )
    {
        return VOS_TRUE;
    }

    return VOS_FALSE;
}

/*****************************************************************************
 Function   : VOS_CalcAppMemInfo
 Description: calc app memory info
 Input      :
 Return     :
 Other      : only for count
 *****************************************************************************/
VOS_BOOL VOS_CalcAppMemInfo(VOS_VOID)
{
    return VOS_FALSE;
}

/*****************************************************************************
 Function   : VOS_GetMemOccupyingInfo
 Description: realloc memory
 Input      : ucPtNo -- partition number.
            : ulInfo -- PIDSID information.
            : pOldMemPtr -- address of old memory.
            : ulNewSize -- size to realloc.
 Return     : Address
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_GetMemOccupyingInfo(VOS_MEM_OCCUPYING_INFO_ST *pstInfo,
                                   VOS_UINT32 ulLen)
{
    VOS_INT                    i;
    VOS_UINT32                 ulSuffix;
    VOS_MEM_HEAD_BLOCK         *pstTemp;

    if ( VOS_NULL_PTR == VOS_MemSet_s(pstInfo, ulLen, 0x00, ulLen) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, (VOS_INT)ulLen, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    (VOS_VOID)VOS_TaskLock();

    for ( i=VOS_SIMPLE_MEM_CTRL_BLOCK_NUMBER-1; i>=0; i--)
    {
        pstTemp = VOS_SimpleMemCtrlBlk[i].BusyBlocks;

        while ( VOS_NULL_PTR != pstTemp)
        {
            if ( VOS_PID_DOPRAEND > pstTemp->ulAllocPid )
            {
                (VOS_VOID)VOS_TaskUnlock();

                return VOS_ERR;
            }

            ulSuffix = pstTemp->ulAllocPid - VOS_PID_DOPRAEND;
            pstInfo[ulSuffix].ulTotalNumber++;
            pstInfo[ulSuffix].ulSize += pstTemp->ulAllocSize;

            pstTemp = pstTemp->pstNext;
        }
    }

    (VOS_VOID)VOS_TaskUnlock();

    return VOS_OK;
}

/*****************************************************************************
 Function   : VOS_GetMsgOccupyingInfo
 Description: realloc memory
 Input      : ucPtNo -- partition number.
            : ulInfo -- PIDSID information.
            : pOldMemPtr -- address of old memory.
            : ulNewSize -- size to realloc.
 Return     : Address
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_GetMsgOccupyingInfo(VOS_MEM_OCCUPYING_INFO_ST *pstInfo,
                                   VOS_UINT32 ulLen)
{
    VOS_INT                    i;
    VOS_UINT32                 ulSuffix;
    VOS_MEM_HEAD_BLOCK         *pstTemp;

    if ( VOS_NULL_PTR == VOS_MemSet_s(pstInfo, ulLen, 0x00, ulLen) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, (VOS_INT)ulLen, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }

    (VOS_VOID)VOS_TaskLock();

    for ( i=VOS_MEM_CTRL_BLOCK_NUMBER-1; i>=0; i--)
    {
        pstTemp = VOS_MemCtrlBlk[i].BusyBlocks;

        while ( VOS_NULL_PTR != pstTemp)
        {
            if ( VOS_PID_DOPRAEND > pstTemp->ulAllocPid )
            {
                (VOS_VOID)VOS_TaskUnlock();

                return VOS_ERR;
            }

            ulSuffix = pstTemp->ulAllocPid - VOS_PID_DOPRAEND;
            pstInfo[ulSuffix].ulTotalNumber++;
            pstInfo[ulSuffix].ulSize += pstTemp->ulAllocSize;

            pstTemp = pstTemp->pstNext;
        }
    }

    (VOS_VOID)VOS_TaskUnlock();

    return VOS_OK;
}

/*****************************************************************************
 Function   : VOS_StaticMemAlloc
 Description: allocate static memory
 Input      :
            :
 Return     : address or NULL
 Other      :
 *****************************************************************************/
VOS_VOID *VOS_StaticMemAlloc( VOS_CHAR *pcBuf, VOS_UINT32 ulBufSize,
                                    VOS_UINT32 ulAllocSize, VOS_UINT32 *pulSuffix)
{
    VOS_ULONG  ulLockLevel;
    VOS_UINT32 ulTempSize;
    VOS_UINT32 ulTempSuffix;

    /* for ARM 4 byte aligned. Do it for peformence only */
    ulTempSize = (ulAllocSize + VOS_ARM_ALIGNMENT)&(~VOS_ARM_ALIGNMENT);

    /*intLockLevel = VOS_SplIMP();*/
    VOS_SpinLockIntLock(&g_stVosStaticMemSpinLock, ulLockLevel);

    if ( (*pulSuffix + ulTempSize) <= ulBufSize )
    {
        ulTempSuffix = *pulSuffix;
        *pulSuffix += ulTempSize;

        /*VOS_Splx(intLockLevel);*/
        VOS_SpinUnlockIntUnlock(&g_stVosStaticMemSpinLock, ulLockLevel);

        return (VOS_VOID *)(&(pcBuf[ulTempSuffix]));
    }

    /*VOS_Splx(intLockLevel);*/
    VOS_SpinUnlockIntUnlock(&g_stVosStaticMemSpinLock, ulLockLevel);

    VOS_ProtectionReboot(VOS_FAIL_TO_ALLOC_STATIC_MEM,(VOS_INT)ulBufSize,0,VOS_NULL_PTR,0);

    return VOS_NULL_PTR;
}

/*****************************************************************************
 Function   : VOS_ModifyMemBlkInfo
 Description: modify the information which be used for debugging
 Input      :
            :
 Return     :
 Other      :
 *****************************************************************************/
VOS_VOID VOS_ModifyMemBlkInfo(VOS_UINT_PTR ulAddr, VOS_PID PID)
{
    VOS_MEM_HEAD_BLOCK   *pstTemp;

    pstTemp = (VOS_MEM_HEAD_BLOCK *)(ulAddr);

    pstTemp->ulAllocPid = PID;

   return;
}

/*****************************************************************************
 Function   : VOS_ExcDumpMemAlloc
 Description: allocate exc memory
 Input      :
            :
 Return     : address or NULL
 Other      :
 *****************************************************************************/
VOS_VOID *VOS_ExcDumpMemAlloc(VOS_UINT32 ulNumber)
{
    VOS_UINT8          *pucDumpBuffer;

    if ( VOS_EXC_DUMP_MEM_NUM_BUTT <= ulNumber )
    {
        return VOS_NULL_PTR;
    }

    pucDumpBuffer = (VOS_UINT8 *)VOS_EXCH_MEM_MALLOC;

    if (VOS_NULL_PTR == pucDumpBuffer)
    {
        return VOS_NULL_PTR;
    }

    /* skip mem&msg dump */
    pucDumpBuffer += VOS_DUMP_MEM_TOTAL_SIZE;

    /*lint -e679*/
    pucDumpBuffer += (ulNumber*VOS_TASK_DUMP_INFO_SIZE);
    /*lint +e679*/

    return (VOS_VOID *)(pucDumpBuffer);
}
/*****************************************************************************
 Function   : VOS_CacheMemFree
 Description: allocate cached memory.
 Input      : size & real address
            :
 Return     : null or vir address
 Other      :
 *****************************************************************************/
VOS_VOID *VOS_CacheMemAlloc(VOS_UINT32 ulSize)
{
    return kmalloc(ulSize, GFP_KERNEL);
}

/*****************************************************************************
 Function   : VOS_CacheMemFree
 Description: allocate unached memory.
 Input      : address
            :
 Return     : null or vir address
 Other      :
 *****************************************************************************/
VOS_UINT32 VOS_CacheMemFree(VOS_VOID *pAddr)
{
        kfree(pAddr);
        return VOS_OK;
}

/*****************************************************************************
 Function   : VOS_UnCacheMemAlloc
 Description: allocate uncached memory.
 Input      : size & real address
            :
 Return     : null or vir address
 Other      :
 *****************************************************************************/
VOS_VOID *VOS_UnCacheMemAlloc(VOS_UINT32 ulSize, VOS_UINT_PTR *pulRealAddr)
{
    VOS_VOID                           *pVirtAdd;

    dma_addr_t                          ulAddress = 0;

    struct device                       dev;


    if ( 0 == ulSize )
    {
        return VOS_NULL_PTR;
    }

    if ( VOS_NULL_PTR == pulRealAddr )
    {
        return VOS_NULL_PTR;
    }

    /* keep lint happy */
    *pulRealAddr = VOS_NULL;
    pVirtAdd     = VOS_NULL_PTR;




    if ( VOS_NULL_PTR == VOS_MemSet_s(&dev, sizeof(dev), 0, sizeof(dev)) )
    {
        mdrv_om_system_error(VOS_REBOOT_MEMSET_MEM, 0, (VOS_INT)((THIS_FILE_ID << 16) | __LINE__), 0, 0);
    }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
    dma_set_mask_and_coherent(&dev, 0xffffffffffffffff);
    of_dma_configure(&dev, dev.of_node);
#endif
    pVirtAdd = dma_alloc_coherent(&dev, ulSize, &ulAddress, GFP_KERNEL);

    *pulRealAddr = (VOS_UINT_PTR)ulAddress;


    return pVirtAdd;
}

/*****************************************************************************
 Function   : VOS_UnCacheMemFree
 Description: free uncached memory.
 Input      : size real address & virt address
            :
 Return     : void
 Other      :
 *****************************************************************************/
VOS_VOID VOS_UnCacheMemFree(VOS_VOID *pVirtAddr, VOS_VOID *pPhyAddr, VOS_UINT32 ulSize)
{
    dma_addr_t  ulAddress;

    if ( 0 == ulSize )
    {
        return;
    }

    if ( (VOS_NULL_PTR == pVirtAddr) || (VOS_NULL_PTR == pPhyAddr) )
    {
        return;
    }



    *(dma_addr_t *)(&ulAddress) = (dma_addr_t)pPhyAddr;

    dma_free_coherent(VOS_NULL_PTR, ulSize, pVirtAddr, ulAddress);


    return;
}


VOS_UINT_PTR VOS_UncacheMemPhyToVirt(VOS_UINT8 *pucCurPhyAddr, VOS_UINT8 *pucPhyStart, VOS_UINT8 *pucVirtStart, VOS_UINT32 ulBufLen)
{
    if((pucCurPhyAddr < pucPhyStart) || (pucCurPhyAddr >= (pucPhyStart+ulBufLen)))
    {
        (VOS_VOID)vos_printf("\r\n VOS_UncacheMemPhyToVirt: The PHY Addr 0x%p, pucPhyStart: 0x%p, pucVirtStart: 0x%p, ulBufLen: 0x%x.\r\n",
                    pucCurPhyAddr,
                    pucPhyStart,
                    pucVirtStart,
                    ulBufLen);

        return VOS_NULL;
    }

    return (VOS_UINT_PTR)((pucCurPhyAddr - pucPhyStart) + pucVirtStart);
}


VOS_UINT_PTR VOS_UncacheMemVirtToPhy(VOS_UINT8 *pucCurVirtAddr, VOS_UINT8 *pucPhyStart, VOS_UINT8 *pucVirtStart, VOS_UINT32 ulBufLen)
{
    if((pucCurVirtAddr < pucVirtStart) || (pucCurVirtAddr >= (pucVirtStart+ulBufLen)))
    {
        (VOS_VOID)vos_printf("\r\n VOS_UncacheMemVirtToPhy: The VIRT Addr 0x%p, pucPhyStart: 0x%p, pucVirtStart: 0x%p, ulBufLen: 0x%x.\r\n",
                    pucCurVirtAddr,
                    pucPhyStart,
                    pucVirtStart,
                    ulBufLen);

        return VOS_NULL;
    }

    return (VOS_UINT_PTR)((pucCurVirtAddr - pucVirtStart) + pucPhyStart);
}


/*lint -e522*/
VOS_VOID VOS_FlushCpuWriteBuf(VOS_VOID)
{

    __asm(" DSB sy ");
    __asm(" ISB sy ");

    return ;
}
/*lint +e522*/


VOS_VOID VOS_FlushCpuCache( VOS_VOID *pAddress, VOS_UINT ulSize )
{
}


VOS_VOID VOS_InvalidCpuCache( VOS_VOID *pAddress, VOS_INT lSize )
{
}



