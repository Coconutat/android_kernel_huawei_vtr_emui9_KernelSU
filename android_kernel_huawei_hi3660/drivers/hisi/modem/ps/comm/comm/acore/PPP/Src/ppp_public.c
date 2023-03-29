/*
 *
 * All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses. You may choose this file to be licensed under the terms
 * of the GNU General Public License (GPL) Version 2 or the 2-clause
 * BSD license listed below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */



/******************************************************************************
   1 头文件包含
******************************************************************************/
#include "PPP/Inc/ppp_public.h"
#include "PPP/Inc/ppp_init.h"
#include "PPP/Inc/hdlc_interface.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
/*lint -e767  打点日志文件宏ID定义 */
#define    THIS_FILE_ID                 PS_FILE_ID_PPP_PUBLIC_C
/*lint +e767   */


/******************************************************************************
   2 外部函数变量声明
******************************************************************************/



/******************************************************************************
   3 私有定义
******************************************************************************/


/******************************************************************************
   4 全局变量定义
******************************************************************************/
extern PPP_DATA_Q_CTRL_ST               g_PppDataQCtrl;

/******************************************************************************
   5 函数实现
******************************************************************************/

PPP_ZC_STRU * PPP_MemAlloc(VOS_UINT16 usLen, VOS_UINT16 usReserveLen)
{
    /* 该接口用在上行时需要保留MAC头长度，
      上行与ADS收发数为IP包，为与NDIS、E5保持数据结构统一，需要保留MAC头，
      零拷贝指针从C核返回的时候统一偏移固定字节，找到零拷贝头部。
    */
    /*
       用在下行时保留长度填0，下行与USB收发数据为字节流形式的PPP帧，无MAC头
    */
    PPP_ZC_STRU *pstMem = PPP_ZC_MEM_ALLOC(usLen + usReserveLen);


    if (VOS_NULL_PTR != pstMem)
    {
        if ( usReserveLen > 0)
        {
            /* 空出保留长度，对PPP模块而言数据长度是usLen，这个函数必须在未赋值前调用 */
            PPP_ZC_RESERVE(pstMem, usReserveLen);

            /* 更新上行申请总次数 */
            g_PppDataQCtrl.stStat.ulMemAllocUplinkCnt++;

            /* 用于区分协商阶段释放的数据来源 */
            PPP_ZC_SET_DATA_APP(pstMem, (VOS_UINT16)(1 << 8) | (VOS_UINT16)PPP_PULL_PACKET_TYPE);
        }
        else
        {
            /* 更新下行申请总次数 */
            g_PppDataQCtrl.stStat.ulMemAllocDownlinkCnt++;

            /* 用于区分协商阶段释放的数据来源 */
            PPP_ZC_SET_DATA_APP(pstMem, (VOS_UINT16)(1 << 8) | (VOS_UINT16)PPP_PUSH_PACKET_TYPE);
        }
    }
    else
    {
        if ( usReserveLen > 0)
        {
            /* 更新上行申请失败次数 */
            g_PppDataQCtrl.stStat.ulMemAllocUplinkFailCnt++;
        }
        else
        {
            /* 更新下行申请失败次数 */
            g_PppDataQCtrl.stStat.ulMemAllocDownlinkFailCnt++;
        }
    }

    return pstMem;
}


VOS_VOID PPP_MemWriteData(PPP_ZC_STRU *pstMem, VOS_UINT8 *pucSrc, VOS_UINT16 usLen)
{
    /* 设置好将要写入零拷贝内存数据内容长度 */
    PPP_ZC_SET_DATA_LEN(pstMem, usLen);

    /* 拷贝至内存数据部分 */
    PPP_MemSingleCopy(PPP_ZC_GET_DATA_PTR(pstMem), pucSrc, usLen);

    return;
}


PPP_ZC_STRU * PPP_MemCopyAlloc(VOS_UINT8 *pSrc, VOS_UINT16 usLen, VOS_UINT16 usReserveLen)
{
    PPP_ZC_STRU                        *pstMem = VOS_NULL_PTR;


    pstMem = PPP_MemAlloc(usLen, usReserveLen);

    if ( VOS_NULL_PTR != pstMem )
    {
        /* 拷贝至内存数据部分 */
        PPP_MemWriteData(pstMem, pSrc, usLen);
    }

    return pstMem;
}


VOS_UINT32 PPP_MemCutTailData
(
    PPP_ZC_STRU **ppMemSrc,
    VOS_UINT8 *pucDest,
    VOS_UINT16 usLen,
    VOS_UINT16 usReserveLen
)
{
    PPP_ZC_STRU                        *pCurrMem;
    VOS_UINT16                          usCurrLen;
    VOS_UINT16                          usCurrOffset;


    /* 参数检查 */
    if ( (VOS_NULL_PTR == ppMemSrc) ||
         (VOS_NULL_PTR == *ppMemSrc) ||
         (VOS_NULL_PTR == pucDest))
    {
        PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,
                      "PPP_MemCutTailData input parameters error, \
                      src addr'addr: 0X%p, dest addr: 0X%p\r\n",
                      (VOS_UINT_PTR)ppMemSrc, (VOS_UINT_PTR)pucDest);

        return PS_FAIL;
    }

    pCurrMem    = (PPP_ZC_STRU *)(*ppMemSrc);
    usCurrLen   = PPP_ZC_GET_DATA_LEN(pCurrMem);

    if ( ( 0 == usLen) || (usCurrLen < usLen) )
    {
        PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,
                      "PPP_MemCutTailData, Warning, usCurrLen %d Less Than usLen %d!\r\n",
                      usCurrLen, usLen);

        return PS_FAIL;
    }

    /* 从尾部拷贝定长数据，只会有一个节点 */
    usCurrOffset = usCurrLen - usLen;

    mdrv_memcpy(pucDest, &(PPP_ZC_GET_DATA_PTR(pCurrMem)[usCurrOffset]), (VOS_ULONG)usLen);

    if ( usCurrOffset > 0 )
    {
        /* 还有剩余数据，目前没有重算长度并将Tail指针前移的接口，重新申请 */
        (*ppMemSrc) = PPP_MemCopyAlloc(PPP_ZC_GET_DATA_PTR(pCurrMem), usCurrOffset, usReserveLen);
    }
    else
    {
        (*ppMemSrc) = VOS_NULL_PTR;
    }

    /* 释放内存 */
    PPP_MemFree(pCurrMem);

    return PS_SUCC;
}


VOS_UINT32 PPP_MemCutHeadData
(
    PPP_ZC_STRU **ppMemSrc,
    VOS_UINT8 *pucDest,
    VOS_UINT16 usDataLen
)
{
    PPP_ZC_STRU                        *pCurrMem;
    VOS_UINT16                          usMemSrcLen;


    if ( (VOS_NULL_PTR == ppMemSrc) ||
         (VOS_NULL_PTR == *ppMemSrc) ||
         (VOS_NULL_PTR == pucDest) )
    {
        PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                     "PPP_MemCutHeadData input parameters error, \
                     src addr'addr: 0x%p, dest addr: 0x%p\r\n",
                    (VOS_UINT_PTR)ppMemSrc, (VOS_UINT_PTR)pucDest);

        return PS_FAIL;
    }

    /* 判断TTF内存块的长度是否符合要求 */
    pCurrMem        = (PPP_ZC_STRU *)(*ppMemSrc);
    usMemSrcLen     = PPP_ZC_GET_DATA_LEN(pCurrMem);

    if ( ( 0 == usDataLen) || (usMemSrcLen < usDataLen) )
    {
        PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                      "PPP_MemCutHeadData, Warning: usMemSrcLen: %d Less Than usDataLen: %d!\r\n",
                      usMemSrcLen, usDataLen);

        return PS_FAIL;
    }

    /* 从头部拷贝定长数据，只会有一个节点 */
    mdrv_memcpy(pucDest, PPP_ZC_GET_DATA_PTR(pCurrMem), (VOS_ULONG)usDataLen);

    if ( usMemSrcLen >  usDataLen)
    {
        /* 还有剩余数据，更新数据指针和长度 */
        PPP_ZC_REMOVE_HDR(pCurrMem, usDataLen);
    }
    else
    {
        /* 释放原始内存 */
        PPP_MemFree(pCurrMem);
        (*ppMemSrc) = VOS_NULL_PTR;
    }

    return PS_SUCC;
}


VOS_UINT32 PPP_MemGet(PPP_ZC_STRU *pMemSrc, VOS_UINT16 usOffset, VOS_UINT8 *pDest, VOS_UINT16 usLen)
{
    VOS_UINT16                          usMemSrcLen;


    /* 参数检查 */
    if ( (VOS_NULL_PTR == pMemSrc)||(VOS_NULL_PTR == pDest) )
    {
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,
                     "PPP_MemGet, Warning, Input Par pMemSrc Or pDest is Null!\r\n");

        return PS_FAIL;
    }

    if ( 0 == usLen )
    {
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,
                     "PPP_MemGet, Warning, Input Par usLen is 0!\r\n");

        return PS_FAIL;
    }

    /* 判断TTF内存块的长度是否符合要求 */
    usMemSrcLen = PPP_ZC_GET_DATA_LEN(pMemSrc);

    if ( usMemSrcLen < (usOffset + usLen) )
    {
        PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,
                      "PPP_MemGet, Warning, MemSrcLen %d Less Than (Offset + Len) %d!\r\n",
                      usMemSrcLen, (usOffset + usLen));

        return PS_FAIL;
    }

    mdrv_memcpy(pDest, PPP_ZC_GET_DATA_PTR(pMemSrc) + usOffset, (VOS_ULONG)usLen);

    return PS_SUCC;
}


VOS_VOID PPP_MemFree(PPP_ZC_STRU *pstMem)
{
    /* 释放零拷贝内存 */
    PPP_ZC_MEM_FREE(pstMem);

    g_PppDataQCtrl.stStat.ulMemFreeCnt++;

    return;
}


VOS_VOID PPP_MemSingleCopy(VOS_UINT8 *pucDest, VOS_UINT8 *pucSrc, VOS_UINT32 ulLen)
{
    /* 待修改为EDMA拷贝 */
    mdrv_memcpy(pucDest, pucSrc, (VOS_ULONG)ulLen);

    return;
}


VOS_UINT32 PPP_GenerateSeed
(
    VOS_UINT8                           aucSerial[],
    VOS_UINT8                           ucSerialLen,
    VOS_UINT32                          ulStartIndex,
    VOS_UINT32                          ulDisturbaceFactor
)
{

    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulSeed;
    VOS_UINT8                          *pucSeed;
    VOS_UINT8                           ucTempValue = 0xFF;


    /* 从随机序列里选四个字节作为种子 */
    pucSeed = (VOS_UINT8 *)&ulSeed;
    for ( ulLoop = 0; ulLoop < sizeof(ulSeed); ulLoop++ )
    {
        *pucSeed++ = aucSerial[(VOS_UINT8)((ulStartIndex + ulLoop) % ucSerialLen)];
    }

    /* 加扰 */
    ulSeed += ulDisturbaceFactor;

    /* 替换弱字节 */
    pucSeed = (VOS_UINT8 *)&ulSeed;
    for ( ulLoop = 0; ulLoop < sizeof(ulSeed); ulLoop++ )
    {
        if (0 == *pucSeed)
        {
            *pucSeed = ucTempValue--;
        }
        pucSeed++;
    }

    return ulSeed;
}


VOS_VOID PPP_Get16ByteSerial
(
    VOS_UINT8                           aucSerial[]
)
{
    VOS_UINT8                           aucSrcSerial[4][4];
    VOS_UINT32                          ulTick      = VOS_GetTick();
    VOS_UINT32                          ulSlice     = VOS_GetSlice();
    VOS_UINT32                          ulTaskId    = VOS_GetCurrentTaskID();
    VOS_UINT32                          ulStatSum;

    ulStatSum   = 0xECA8642F;

    /* 如果系统数据为0，用特殊序列替代 */
    ulTick = (0x00UL != ulTick) ? ulTick : 0x12345678;
    ulSlice = (0x00UL != ulSlice) ? ulSlice : 0x9ABCDEF1;
    ulTaskId = (0x00UL != ulTaskId) ? ulTaskId : 0xFDB97531;

    /* 将系统数据组合成一个序列 */
    PSACORE_MEM_CPY(&(aucSrcSerial[0][0]), sizeof(ulTick), &ulTick, sizeof(ulTick));
    PSACORE_MEM_CPY(&(aucSrcSerial[1][0]), sizeof(ulSlice), &ulSlice, sizeof(ulSlice));
    PSACORE_MEM_CPY(&(aucSrcSerial[2][0]), sizeof(ulTaskId), &ulTaskId, sizeof(ulTaskId));
    PSACORE_MEM_CPY(&(aucSrcSerial[3][0]), sizeof(ulStatSum), &ulStatSum, sizeof(ulStatSum));

    /* 进行矩阵变换生成新的随机序列*/
    PPP_MATRIX_TRANSFORM(aucSerial, aucSrcSerial);

    return;
}


VOS_VOID PPP_GetSecurityRand
(
    VOS_UINT8                           ucRandByteLen,
    VOS_UINT8                          *pucRand
)
{
    VOS_UINT8                           aucDictionary[256] = {0};
    VOS_UINT8                           aucSerial[16] = {0};
    VOS_UINT8                           ucValue = 0;
    VOS_UINT8                           ucTempValue;
    VOS_UINT32                          ulSeed;
    VOS_UINT32                          ulRandIndex;
    VOS_UINT32                          ulSwapIndex;
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulStart;
    VOS_UINT32                          ulRandSum;
    const VOS_UINT32                    ulMax = 0x100;/* 用于获取随机数, 随机数取值范围是[0x00..0xFF] */


    /***************************************************************************
     依据是热力学第二定律的熵增加原理。主要的思想是将有序的序列通过一定的交换将
     有序变成无序。思想类似于扑克牌的洗牌。假设扑克牌是从小到大的顺序放好的，通
     过多次的洗牌之后扑克牌的顺序性就逐渐消失，变的随机。换句话说，系统的熵值变
     大。而且这种过程是不可逆的，只能从有序变成无序，不能从无序变成有序。使用一
     些系统数据（时间值、TICK值、内存使用量、系统调用、中断、进/线程ID）带来的
     随机性，经过一定方式的提取和熵浓缩处理，产生随机数，这类随机数产生器叫做非
     物理真随机数产生器。
    ****************************************************************************/

    /***************************************************************************
     生成安全随机数分为两步:
     1、生成字典
     2、交换字典元素
    ****************************************************************************/

    /* 获取16byte随机序列 */
    PPP_Get16ByteSerial(aucSerial);

    /* 更新种子 */
    ulSeed = PPP_GenerateSeed(aucSerial, (VOS_UINT8)sizeof(aucSerial), 0, 0);
    VOS_SetSeed(ulSeed);

    /***************************************************************************
     1、生成字典
    ****************************************************************************/
    /* a、生成随机起始位置, 生成的范围:[0..0xFF] */
    ulStart = VOS_Rand(ulMax);
    ulRandSum = ulStart;

    /* b、生成字典的后半部分: [ulStart，0xFF] */
    for (ulLoop = ulStart; ulLoop < ulMax; ulLoop++)
    {
        aucDictionary[ulLoop] = ucValue;
        ucValue++;
    }

    /* c、生成字典的前半部分: [0, ulStart) */
    for (ulLoop = 0; ulLoop < ulStart; ulLoop++)
    {
        aucDictionary[ulLoop] = ucValue;
        ucValue++;
    }

    /***************************************************************************
     2、交换字典元素
       生成ucRandByteLen字节随机数序列，从数组下标ulLoop = 0开始，
       随机交换字典元素(ulLoop和[ulLoop, 0xFF]交换)，打乱字典序列。
    ****************************************************************************/
    for (ulLoop = 0; ulLoop < ucRandByteLen; ulLoop++)
    {
        /* 更新种子 */
        ulSeed = PPP_GenerateSeed(aucSerial, (VOS_UINT8)sizeof(aucSerial), ulLoop, ulRandSum);
        VOS_SetSeed(ulSeed);

        /* 生成随机数, 生成的范围:[0..0xFF] */
        ulRandIndex                 = VOS_Rand(ulMax);
        ulRandSum                  += ulRandIndex;

        /* 计算交换的位置，范围:[ulLoop..0xFF] */
        ulSwapIndex                 = (ulRandIndex % (ulMax - ulLoop)) + ulLoop;

        /* 交换aucDictionary[ulLoop]和aucDictionary[ulSwapIndex] */
        ucTempValue                 = aucDictionary[ulLoop];
        aucDictionary[ulLoop]       = aucDictionary[ulSwapIndex];
        aucDictionary[ulSwapIndex]  = ucTempValue;
    }

    /* 获取ucRandByteLen字节随机序列 */
    PSACORE_MEM_CPY(pucRand, ucRandByteLen, &(aucDictionary[0]), ucRandByteLen);

    return;
}




