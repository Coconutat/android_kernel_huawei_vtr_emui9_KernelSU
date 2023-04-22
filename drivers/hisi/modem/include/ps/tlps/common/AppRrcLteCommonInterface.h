/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
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

#ifndef __APPRRCLTECOMMONINTERFACE_H__
#define __APPRRCLTECOMMONINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#include  "vos.h"

#if (VOS_OS_VER != VOS_WIN32)
#pragma pack(4)
#else
#pragma pack(push, 4)
#endif

#define LMAX_NEIGHBOR_CELL_NUM    (16)
#ifndef VOS_MSG_HEADER
#define VOS_MSG_HEADER  VOS_UINT32 uwSenderCpuId;  \
                        VOS_UINT32 uwSenderPid;    \
                        VOS_UINT32 uwReceiverCpuId;\
                        VOS_UINT32 uwReceiverPid;  \
                        VOS_UINT32 uwLength;
#endif
#ifndef APP_MSG_HEADER
#define APP_MSG_HEADER                  VOS_UINT16   usOriginalId;\
                                        VOS_UINT16   usTerminalId;\
                                        VOS_UINT32   ulTimeStamp; \
                                        VOS_UINT32   ulSN;
#endif

enum RRC_OM_GET_CELL_INFO_FLAG_ENUM
{
    EN_GET_SERVICE_CELL_INFO = 0,
    EN_GET_NEIGHBOR_CELL_INFO = 1
};

typedef VOS_UINT32 RRC_OM_GET_CELL_INFO_FLAG_ENUM_UINT32;


/*****************************************************************************
 结构名    : APP_LRRC_GET_NCELL_INFO_REQ_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                       /*_H2ASN_Skip*/
    VOS_UINT32             ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32             ulOpId;
    RRC_OM_GET_CELL_INFO_FLAG_ENUM_UINT32   enCellFlag;   /*=0标示获取服务小区，=1标示获取邻区*/
}APP_LRRC_GET_NCELL_INFO_REQ_STRU;


/*****************************************************************************
 结构名    : LRRC_APP_SRVING_CELL_MEAS_RESULT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 服务小区或同频、异频小区测量结果结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usPhyCellId;/* 小区id */
    VOS_INT16                           sRsrp;/* RSRP值 */
    VOS_INT16                           sRsrq;/* RSRq值 */
    VOS_INT16                           sRssi;/* RSSI值 */
}LRRC_APP_SRVING_CELL_MEAS_RESULT_STRU;


/*****************************************************************************
 结构名    : LRRC_APP_CELL_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 服务小区或同频、异频小区信息结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usFreqInfo;/* 服务小区频点 */
    VOS_UINT16                          enBandInd;/* 频带指示 */
    LRRC_APP_SRVING_CELL_MEAS_RESULT_STRU     stMeasRslt;
}LRRC_APP_CELL_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_CELL_MEAS_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 服务小区或同频、异频小区信息结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32   ulNCellNumber;
    LRRC_APP_CELL_INFO_STRU stCellMeasInfo[LMAX_NEIGHBOR_CELL_NUM];
}LRRC_APP_CELL_MEAS_INFO_STRU;

typedef LRRC_APP_CELL_MEAS_INFO_STRU LRRC_APP_SRV_CELL_MEAS_INFO_STRU;
typedef LRRC_APP_CELL_MEAS_INFO_STRU LRRC_APP_INTRA_FREQ_NCELL_MEAS_INFO_STRU;
typedef LRRC_APP_CELL_MEAS_INFO_STRU LRRC_APP_INTER_FREQ_NCELL_MEAS_INFO_STRU;


/*****************************************************************************
 结构名    : LRRC_APP_BSIC_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BSC信息
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                              usNcc;
    VOS_UINT16                                              usBcc;
}LRRC_APP_BSIC_INFO_STRU;
/*****************************************************************************
 结构名    : LRRC_GERAN_NCELL_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : L主模下，GSM 邻区信息
*****************************************************************************/

typedef struct
{
    VOS_UINT16    usArfcn;
    VOS_INT16     sRSSI;
    LRRC_APP_BSIC_INFO_STRU     stBSIC;
}LRRC_GERAN_NCELL_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_UMTS_NCELL_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : L主模下，UMTS邻区信息结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16    usARFCN;
    VOS_UINT16    usPrimaryScramCode;
    VOS_INT16     sCpichRscp;
    VOS_INT16     sCpichEcN0;
}LRRC_UMTS_NCELL_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_INTER_RAT_UMTS_NCELL_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : L主模下，UMTS邻区信息列表
*****************************************************************************/

typedef struct
{
    VOS_UINT32 ulNCellNumber;
    LRRC_UMTS_NCELL_INFO_STRU stUMTSNcellList[LMAX_NEIGHBOR_CELL_NUM];
}LRRC_APP_INTER_RAT_UMTS_NCELL_LIST_STRU;


/*****************************************************************************
 结构名    : LRRC_APP_INTER_RAT_GERAN_NCELL_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : L主模下，GSM 邻区信息列表
*****************************************************************************/
typedef struct
{
    VOS_UINT32    ulNCellNumber;
    LRRC_GERAN_NCELL_INFO_STRU stGeranNcellList[LMAX_NEIGHBOR_CELL_NUM];
}LRRC_APP_INTER_RAT_GERAN_NCELL_LIST_STRU;


/*****************************************************************************
 结构名    : LRRC_APP_NCELL_LIST_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 服务小区的同频、异频以及异系统测量结果
*****************************************************************************/
typedef struct
{
    RRC_OM_GET_CELL_INFO_FLAG_ENUM_UINT32      enCellFlag;   /*=0标示获取服务小区，=1标示获取邻区*/
    LRRC_APP_SRV_CELL_MEAS_INFO_STRU          stSevCellInfo; /*服务小区信息，包括频点、band、Phycial ID,RSRP,RSRQ,RSSI*/
    LRRC_APP_INTRA_FREQ_NCELL_MEAS_INFO_STRU  stIntraFreqNcellList;/* 同频邻区信息*/
    LRRC_APP_INTER_FREQ_NCELL_MEAS_INFO_STRU  stInterFreqNcellList;/* 异频邻区信息*/
    LRRC_APP_INTER_RAT_UMTS_NCELL_LIST_STRU    stInterRATUMTSNcellList; /* WCDMA/TDSCDMA异系统邻区*/
    LRRC_APP_INTER_RAT_GERAN_NCELL_LIST_STRU  stInterRATGeranNcellList; /* GERAN异系统邻区 */
}LRRC_APP_NCELL_LIST_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_GET_NCELL_INFO_CNF_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 服务小区的同频、异频以及异系统邻区信息
*****************************************************************************/

typedef struct
{
    VOS_MSG_HEADER                       /*_H2ASN_Skip*/
    VOS_UINT32             ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32             ulOpId;
    VOS_UINT32             enResult;
    LRRC_APP_NCELL_LIST_INFO_STRU stNcellListInfo;
}LRRC_APP_GET_NCELL_INFO_CNF_STRU;



#if (VOS_OS_VER != VOS_WIN32)
#pragma pack()
#else
#pragma pack(pop)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of AppRrcLteCommonInterface.h */
