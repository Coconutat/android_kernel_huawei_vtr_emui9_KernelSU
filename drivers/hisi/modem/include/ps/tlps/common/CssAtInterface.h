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

#ifndef  CSS_AT_INTERFACE_H
#define  CSS_AT_INTERFACE_H

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "vos.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define MCC_INFO_VERSION_LEN                 (9)
#define AT_CSS_MAX_MCC_ID_NUM                (17)


/*****************************************************************************
  3 枚举定义
*****************************************************************************/

enum CSS_AT_MSG_TYPE_ENUM
{
    /* AT->CSS */
    ID_AT_CSS_MCC_INFO_SET_REQ                = 0x0001,                           /* _H2ASN_MsgChoice AT_CSS_MCC_INFO_SET_REQ_STRU */
    ID_AT_CSS_MCC_VERSION_INFO_REQ            = 0x0002,                           /* _H2ASN_MsgChoice AT_CSS_MCC_VERSION_INFO_REQ_STRU */

    /* CSS->AT */
    ID_CSS_AT_MCC_INFO_SET_CNF                = 0x1001,                           /* _H2ASN_MsgChoice CSS_AT_MCC_INFO_SET_CNF_STRU */
    ID_CSS_AT_MCC_VERSION_INFO_CNF            = 0x1002,                           /* _H2ASN_MsgChoice CSS_AT_MCC_VERSION_INFO_CNF_STRU */
    ID_CSS_AT_QUERY_MCC_INFO_NOTIFY           = 0x1003,                           /* _H2ASN_MsgChoice CSS_AT_QUERY_MCC_INFO_NOTIFY_STRU */

    ID_CSS_AT_MSG_BUTT
};
typedef  VOS_UINT32  CSS_AT_MSG_TYPE_ENUM_UINT32;


enum AT_CSS_RAT_ENUM
{
    AT_CSS_RAT_TYPE_GSM = 0,                           /* GSM接入技术 */
    AT_CSS_RAT_TYPE_WCDMA,                             /* WCDMA接入技术 */
    AT_CSS_RAT_TYPE_LTE,                               /* LTE接入技术 */

    AT_CSS_RAT_TYPE_BUTT
};
typedef  VOS_UINT8  AT_CSS_RAT_ENUM_UINT8;

enum AT_CSS_SET_MCC_OPERATE_TYPE_ENUM
{
    AT_CSS_SET_MCC_TYPE_ADD_MCC = 0,                   /* 新增MCC */
    AT_CSS_SET_MCC_TYPE_DELETE_ALL_MCC,                /* 删除所有MCC信息 */
    AT_CSS_SET_MCC_TYPE_DELETE_ONE_MCC,                /* 删除一个MCC信息 */

    AT_CSS_SET_MCC_TYPE_BUTT
};
typedef  VOS_UINT8  AT_CSS_SET_MCC_OPERATE_TYPE_ENUM_UINT8;



/*****************************************************************************
  4 类型定义
*****************************************************************************/

typedef struct
{
    /*MCC在aucMcc[2]中的存放格式,mcc为460:
    ---------------------------------------------------------------------------
                 ||(BIT8)|(BIT7)|(BIT6)|(BIT5)|(BIT4)|(BIT3)|(BIT2)|(BIT1)
    ---------------------------------------------------------------------------
    aucMcc[0]    ||    MCC digit 2 = 6        |           MCC digit 1 = 4
    ---------------------------------------------------------------------------
    aucMcc[1]    ||    无效                   |           MCC digit 3 = 0
    ---------------------------------------------------------------------------*/
    VOS_UINT8                           aucMcc[2];                              /* MCC ID */
    VOS_UINT8                           aucRsv[2];
} AT_CSS_MCC_ID_STRU;

typedef struct
{
    VOS_UINT32                          ulFreqHigh;
    VOS_UINT32                          ulFreqLow;
} AT_CSS_FREQ_RANGE_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           aucRsv[2];
} AT_CSS_MCC_VERSION_INFO_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           aucRsv[1];
    VOS_UINT8                           aucVersionId[MCC_INFO_VERSION_LEN];     /* 版本号，固定为xx.xx.xxx */
} CSS_AT_MCC_VERSION_INFO_CNF_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                                        /* _H2ASN_Skip */
    VOS_UINT32                                    ulMsgId;                                /* _H2ASN_Skip */
    
    MODEM_ID_ENUM_UINT16                          usModemId;
    VOS_UINT16                                    usClientId;
    VOS_UINT8                                     ucSeq;                                  /* 流水号 */
    AT_CSS_SET_MCC_OPERATE_TYPE_ENUM_UINT8        ucOperateType;                          /* 操作类型 */
    VOS_UINT8                                     aucVersionId[MCC_INFO_VERSION_LEN];     /* 版本号，固定为xx.xx.xxx */

    VOS_UINT8                                     aucRsv[1];

    /*
        1)aucMccINfoBuff里存储的是MCC的信息，存储区的真实大小是ulMccInfoBuffLen里记录的字节数；
        2)aucMccINfoBuff里的格式为AT_CSS_MCC_INFO_STRU结构的格式，其中MNC个数,
          BAND个数，预置频段个数，预置频点的个数都是可变的。

        typedef struct
        {
            VOS_UINT8                           ucSupportFlag;    // 1:表示支持GSM 2:表示支持WCDMA 4:表示支持LTE，三者可以自由组合
            AT_CSS_FREQ_RANGE_STRU              stFreqRange;
        } AT_CSS_FREQ_RANGE_WITH_RAT_STRU;

        typedef struct
        {
            VOS_UINT8                                   ucBandInd;
            VOS_UINT8                                   ucFreqRangNum;
            //后面必须紧跟ucFreqRangNum个AT_CSS_FREQ_RANGE_WITH_RAT_STRU结构
              的FreqRange信息，如果没有FreqRange，则需要将ucFreqRangNum填为0
            AT_CSS_FREQ_RANGE_WITH_RAT_STRU             astFreqRangeArray[ucFreqRangNum];

            VOS_UINT8                                   ucPreConfigFreqNum;
            //后面必须紧跟ucPreConfigFreqNum个AT_CSS_FREQ_RANGE_WITH_RAT_STRU结构
              的PreConfigFreq信息，如果没有PreConfigFreq，则需要将ucPreConfigFreqNum填为0
            AT_CSS_FREQ_RANGE_WITH_RAT_STRU             astPreConfigFreqArray[ucPreConfigFreqNum];      //预置频点列表
        }AT_CSS_BAND_INFO_STRU;


        typedef struct
        {
            MNC在aucMnc[2]中的存放格式，mnc为01:
            ---------------------------------------------------------------------------
                         ||(BIT8)|(BIT7)|(BIT6)|(BIT5)|(BIT4)|(BIT3)|(BIT2)|(BIT1)
            ---------------------------------------------------------------------------
            aucMnc[0]    ||    MNC digit 3 = f        |           无效
            ---------------------------------------------------------------------------
            aucMnc[1]    ||    MNC digit 2 = 1        |           MNC digit 1 = 0
            ---------------------------------------------------------------------------
            VOS_UINT8                           aucMnc[2];
            VOS_UINT8                           ucBandCount;// BAND的个数
            //后面必须紧跟ucBandCount个band的信息，如果没有BAND，则需要将ucBandCount填为0
            AT_CSS_BAND_INFO_STRU               astBandArray[ucBandCount];
        }AT_CSS_MNC_INFO_STRU;


        typedef struct
        {
            MCC在aucMcc[2]中的存放格式,mcc为460:
            ---------------------------------------------------------------------------
                         ||(BIT8)|(BIT7)|(BIT6)|(BIT5)|(BIT4)|(BIT3)|(BIT2)|(BIT1)
            ---------------------------------------------------------------------------
            aucMcc[0]    ||    MCC digit 2 = 6        |           MCC digit 1 = 4
            ---------------------------------------------------------------------------
            aucMcc[1]    ||    无效                   |           MCC digit 3 = 0
            ---------------------------------------------------------------------------
            VOS_UINT8                           aucMcc[2];
            VOS_UINT8                           ucMncCount;// MNC的个数
            //后面必须紧跟ucMncCount个AT_CSS_MNC_INFO_STRU结构的mnc信息，如果没有mnc，则需要将ucMncCount填为0
            AT_CSS_MNC_INFO_STRU                astMncAarry[ucMncCount];
        }AT_CSS_MCC_INFO_STRU;


        3)aucMccINfoBuff中存储信息的格式AP与CSS直接对接，AT不会进行修改，AP先形成上述的格式，
          然后转成字符串格式发给AT，AT将字符串格式还原二进制码流，然后发给CSS；
          例如AP形成某一个字节为0x22，然后转化为字符串'22'，AT收到后再转成0x22;
        4)aucMccINfoBuff中的格式为小端；
        5)ulMccInfoBuffLen不能超过1.6K；
    */
    VOS_UINT32                          ulMccInfoBuffLen;
    VOS_UINT8                           aucMccInfoBuff[4];
} AT_CSS_MCC_INFO_SET_REQ_STRU;

typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucSeq;                                  /* 流水号 */
    VOS_UINT8                           aucRsv[1];

    VOS_UINT32                          ulResult;                               /*0表示成功，1表示失败*/
} CSS_AT_MCC_INFO_SET_CNF_STRU;

typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           aucRsv[1];
    VOS_UINT8                           aucVersionId[MCC_INFO_VERSION_LEN];     /* 版本号，固定为xx.xx.xxx */
    VOS_UINT32                          ulMccNum;                               /* MCC 个数 */
    AT_CSS_MCC_ID_STRU                  astMccId[AT_CSS_MAX_MCC_ID_NUM];        /* MCC ID列表 */
} CSS_AT_QUERY_MCC_INFO_NOTIFY_STRU;





/*****************************************************************************
  5 全局变量声明
*****************************************************************************/


/*****************************************************************************
  6 接口函数声明
*****************************************************************************/

/*****************************************************************************
  7 OTHERS定义
*****************************************************************************/

/* ASN解析结构 */
typedef struct
{
    VOS_UINT32                          ulMsgId;                                /*_H2ASN_MsgChoice_Export CSS_AT_MSG_TYPE_ENUM_UINT32 */
    VOS_UINT8                           aucMsg[4];
    /***************************************************************************
        _H2ASN_MsgChoice_When_Comment          CSS_AT_MSG_TYPE_ENUM_UINT32
    ****************************************************************************/
}CSS_AT_INTERFACE_MSG_DATA;
/*_H2ASN_Length UINT32*/

typedef struct
{
    VOS_MSG_HEADER
    CSS_AT_INTERFACE_MSG_DATA           stMsgData;
} CssAtInterface_MSG;


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

#endif


