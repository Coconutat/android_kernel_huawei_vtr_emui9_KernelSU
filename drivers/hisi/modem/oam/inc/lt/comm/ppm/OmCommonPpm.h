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


#ifndef __OM_COMMON_PPM_H__
#define __OM_COMMON_PPM_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "omnvinterface.h"
#include "mdrv.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)

extern VOS_UINT32                          g_ulOmAcpuDbgFlag ;

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define USB_MAX_DATA_LEN            (60*1024)   /*USB发送的最大数据长度*/

#define BIT_N(num)          (0x01 << (num))

#define OM_ACPU_RECV_USB        BIT_N(0)
#define OM_ACPU_DISPATCH_MSG    BIT_N(1)
#define OM_ACPU_SEND_SOCP       BIT_N(2)
#define OM_ACPU_SEND_USB        BIT_N(3)
#define OM_ACPU_USB_CB          BIT_N(4)
#define OM_ACPU_SEND_USB_IND    BIT_N(5)
#define OM_ACPU_ERRLOG_SEND     BIT_N(6)
#define OM_ACPU_ERRLOG_RCV      BIT_N(7)
#define OM_ACPU_ERRLOG_PRINT    BIT_N(8)
#define OM_ACPU_RECV_SOCKET     BIT_N(9)
#define OM_ACPU_SEND_SOCKET     BIT_N(10)
#define OM_ACPU_DATA            BIT_N(11)
#define OM_ACPU_READ_DONE       BIT_N(12)

#define SOCP_SEND_DATA_PARA_CHECK(pstDebugInfo, ulDataLen, pucData) \
    if ((0 == ulDataLen)\
    || (VOS_NULL_PTR == pucData))\
    {\
        pstDebugInfo->ulUSBSendCBAbnormalNum++;\
        return;\
    }


#define OM_ACPU_DEBUG_CHANNEL_TRACE(enChanID, pucData, ulDataLen, ulSwitch, ulDataSwitch) \
        if(VOS_FALSE != (g_ulOmAcpuDbgFlag&ulSwitch)) \
        { \
            (void)vos_printf("%s, channal ID: = %d, Data Len: = %d\n", __FUNCTION__, enChanID, ulDataLen); \
            if(VOS_FALSE != (g_ulOmAcpuDbgFlag&ulDataSwitch) )\
            {\
                if (VOS_NULL_PTR != pucData)\
                {\
                    VOS_UINT32 ulOmDbgIndex;\
                    for (ulOmDbgIndex = 0 ; ulOmDbgIndex < ulDataLen; ulOmDbgIndex++) \
                    { \
                        (void)vos_printf("%02x ", *((VOS_UINT8*)pucData + ulOmDbgIndex)); \
                    } \
                }\
            }\
        }

/*******************************************************************************
  3 枚举定义
*******************************************************************************/
/* UDI设备句柄 */
enum OM_PROT_HANDLE_ENUM
{
    OM_USB_IND_PORT_HANDLE      =   0,
    OM_USB_CFG_PORT_HANDLE      =   1,
    OM_USB_CBT_PORT_HANDLE      =   2,
    OM_HSIC_IND_PORT_HANDLE     =   3,
    OM_HSIC_CFG_PORT_HANDLE     =   4,
    OM_PORT_HANDLE_BUTT
};

typedef VOS_UINT32  OM_PROT_HANDLE_ENUM_UINT32;

enum OM_LOGIC_CHANNEL_ENUM
{
     OM_LOGIC_CHANNEL_CBT = 0,
     OM_LOGIC_CHANNEL_CNF,
     OM_LOGIC_CHANNEL_IND,
     OM_LOGIC_CHANNEL_BUTT
};

typedef VOS_UINT32     OM_LOGIC_CHANNEL_ENUM_UINT32;


/*****************************************************************************
  4 结构体定义
*****************************************************************************/

typedef struct
{
    VOS_SEM                             ulPseudoSyncSemId;   /* 伪造为同步接口需使用的信号量 */
    VOS_UINT8                          *pucAsyncCBData;      /* DRV_UDI_IOCTL接口异步返回后传入的数据指针 */
    VOS_UINT32                          ulLen;               /* DRV_UDI_IOCTL接口异步返回后返回的实际处理数据长度 */
    VOS_UINT32                          ulRsv;               /* Reserve */
}OM_PSEUDO_SYNC_STRU;


typedef struct
{
    VOS_UINT32                          ulPortTypeErr;
    VOS_UINT32                          ulSwitchFail;
    VOS_UINT32                          ulSwitchSucc;
    VOS_UINT32                          ulStartSlice;
    VOS_UINT32                          ulEndSlice;
}PPM_PORT_CFG_INFO_STRU;


/* 用于记录VCOM发送信息 */
typedef struct
{
    VOS_UINT32                          ulVCOMSendSn;
    VOS_UINT32                          ulVCOMSendNum;
    VOS_UINT32                          ulVCOMSendLen;
    VOS_UINT32                          ulVCOMSendErrNum;
    VOS_UINT32                          ulVCOMSendErrLen;
    VOS_UINT32                          ulMaxTimeLen;           /* 调用写接口花费的最大时长 */

    VOS_UINT32                          ulVCOMRcvSn;
    VOS_UINT32                          ulVCOMRcvNum;
    VOS_UINT32                          ulVCOMRcvLen;
    VOS_UINT32                          ulVCOMRcvErrNum;
    VOS_UINT32                          ulVCOMRcvErrLen;
}OM_VCOM_DEBUG_INFO;



typedef struct
{
    VOS_UINT32                      ulUSBWriteNum1;
    VOS_UINT32                      ulUSBWriteNum2;
    VOS_UINT32                      ulUSBWriteCBNum;
    VOS_UINT32                      ulUSBWriteMaxTime;
    VOS_UINT32                      ulUSBINNum;
    VOS_UINT32                      ulUSBINTime;
    VOS_UINT32                      ulUSBOutNum;
    VOS_UINT32                      ulUSBOutTime;
    VOS_UINT32                      ulUSBStateErrNum;
    VOS_UINT32                      ulUSBStateErrTime;
    VOS_UINT32                      ulUSBWriteErrTime;
    VOS_UINT32                      ulUSBWriteErrNum;
    VOS_UINT32                      ulUSBWriteErrValue;
    VOS_UINT32                      ulUSBWriteErrLen;

    VOS_UINT32                      ulUSBOpenNum;
    VOS_UINT32                      ulUSBOpenSlice;
    VOS_UINT32                      ulUSBOpenOkNum;
    VOS_UINT32                      ulUSBOpenOkSlice;
    VOS_UINT32                      ulUSBOpenOk2Num;
    VOS_UINT32                      ulUSBOpenOk2Slice;
    VOS_UINT32                      ulUSBCloseNum;
    VOS_UINT32                      ulUSBCloseSlice;
    VOS_UINT32                      ulUSBCloseOkNum;
    VOS_UINT32                      ulUSBCloseOkSlice;

    VOS_UINT32                      ulUSBIndPseudoSyncFailNum;
    VOS_UINT32                      ulUSBIndPseudoSyncFailSlice;
    VOS_UINT32                      ulUSBCnfPseudoSyncFailNum;
    VOS_UINT32                      ulUSBCnfPseudoSyncFailSlice;
    VOS_UINT32                      ulUSBUdiHandleErr;
    VOS_UINT32                      ulUSBUdiHandleReadGetBufferErr;
    VOS_UINT32                      ulUSBUdiCommRcvNullPtrErr;
    VOS_UINT32                      ulUSBUdiHandleReadBufferFreeErr;
    VOS_UINT32                      ulUSBRcvPktNum;
    VOS_UINT32                      ulUSBRcvPktByte;

}OM_ACPU_PORT_INFO;

typedef struct
{
    VOS_UINT32                          ulSocpCBParaErrNum;
    VOS_UINT32                          ulUSBSendNum;
    VOS_UINT32                          ulUSBSendLen;
    VOS_UINT32                          ulUSBSendRealLen;
    VOS_UINT32                          ulUSBSendErrNum;
    VOS_UINT32                          ulUSBSendErrLen;
    VOS_UINT32                          ulUSBSendCBAbnormalNum;
    VOS_UINT32                          ulUSBSendCBAbnormalLen;

    VOS_UINT32                          ulOmGetVirtErr;
    VOS_UINT32                          ulOmGetVirtSendLen;
    VOS_UINT32                          ulOmDiscardNum;
    VOS_UINT32                          ulOmDiscardLen;
    VOS_UINT32                          ulSocpReadDoneErrNum;
    VOS_UINT32                          ulSocpReadDoneErrLen;
}OM_SOCP_CHANNEL_DEBUG_INFO;

/*用于记录当前OM通道的信息*/
typedef struct
{
    OM_SOCP_CHANNEL_DEBUG_INFO          stIndDebugInfo;
    OM_SOCP_CHANNEL_DEBUG_INFO          stCnfDebugInfo;
    OM_ACPU_PORT_INFO                   astPortInfo[OM_PORT_HANDLE_BUTT];
    VOS_UINT32                          ulInvaldChannel;
}OM_ACPU_DEBUG_INFO;

/*****************************************************************************
  4 函数声明
*****************************************************************************/
#if((VOS_OS_VER == VOS_LINUX) || (VOS_OSA_CPU == OSA_CPU_ACPU))
extern VOS_UINT32 PPM_DisconnectGUPort(OM_LOGIC_CHANNEL_ENUM_UINT32 enChannel);

extern VOS_UINT32 PPM_DisconnectTLPort(VOS_VOID);

extern VOS_VOID   PPM_DisconnectAllPort(OM_LOGIC_CHANNEL_ENUM_UINT32 enChannel);

extern VOS_INT32  PPM_ReadPortData(CPM_PHY_PORT_ENUM_UINT32 enPhyPort, UDI_HANDLE UdiHandle, OM_PROT_HANDLE_ENUM_UINT32 enHandle);

extern VOS_VOID   PPM_PortWriteAsyCB(OM_PROT_HANDLE_ENUM_UINT32 enHandle, VOS_UINT8* pucData, VOS_INT lLen);

extern VOS_VOID   PPM_ReadPortDataInit(CPM_PHY_PORT_ENUM_UINT32        enPhyPort,
                                    OM_PROT_HANDLE_ENUM_UINT32          enHandle,
                                    VOS_VOID                            *pReadCB,
                                    VOS_VOID                            *pWriteCB,
                                    VOS_VOID                            *pStateCB);

extern VOS_UINT32 PPM_UdiRegCallBackFun(UDI_HANDLE enHandle, VOS_UINT32 ulCmdType, VOS_VOID* pFunc);

extern VOS_VOID   PPM_PortCloseProc(OM_PROT_HANDLE_ENUM_UINT32  enHandle, CPM_PHY_PORT_ENUM_UINT32 enPhyPort);

extern VOS_UINT32 PPM_PortSend(OM_PROT_HANDLE_ENUM_UINT32 enHandle, VOS_UINT8 *pucVirAddr, VOS_UINT8 *pucPhyAddr, VOS_UINT32 ulDataLen);

extern VOS_VOID   PPM_PortStatus(OM_PROT_HANDLE_ENUM_UINT32 enHandle, CPM_PHY_PORT_ENUM_UINT32 enPhyPort,ACM_EVT_E enPortState);

extern VOS_VOID   PPM_GetSendDataLen(SOCP_CODER_DST_ENUM_U32 enChanID, VOS_UINT32 ulDataLen, VOS_UINT32 *pulSendDataLen, CPM_PHY_PORT_ENUM_UINT32 *penPhyport);

extern VOS_UINT32 PPM_PortInit(VOS_VOID);

extern VOS_UINT32 PPM_InitPhyPort(VOS_VOID);
#endif

extern VOS_UINT32 PPM_UsbPortInit(VOS_VOID);

extern VOS_VOID   PPM_HsicPortInit(VOS_VOID);

extern VOS_VOID   PPM_VComPortInit(VOS_VOID);

extern VOS_UINT32 SCM_RlsDestBuf(VOS_UINT32 ulChanlID, VOS_UINT32 ulReadSize);

extern VOS_UINT8* SCM_CoderDestMemVirtToPhy(VOS_UINT32 ulDstChID, VOS_UINT8 *pucVirtAddr);

extern OM_PSEUDO_SYNC_STRU * PPM_ComPpmGetSyncInfo(VOS_VOID);
extern OM_ACPU_DEBUG_INFO * PPM_ComPpmGetDebugInfo(VOS_VOID);

/*****************************************************************************
  5 全局变量声明
*****************************************************************************/
extern UDI_HANDLE                              g_astOMPortUDIHandle[OM_PORT_HANDLE_BUTT];

/* USB承载的OM IND端口中，伪造为同步接口使用的数据结构体 */
extern OM_PSEUDO_SYNC_STRU                     g_stUsbIndPseudoSync;

/* USB承载的OM CNF端口中，伪造为同步接口使用的数据结构体 */
extern OM_PSEUDO_SYNC_STRU                     g_stUsbCfgPseudoSync;

/*****************************************************************************
  6 OTHERS定义
*****************************************************************************/




#if ((VOS_OS_VER == VOS_WIN32) || (VOS_OS_VER == VOS_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of OmCommonPpm.h*/

