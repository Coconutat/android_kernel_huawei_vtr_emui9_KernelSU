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

#ifndef __ATMSGPRINT_H__
#define __ATMSGPRINT_H__

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include  "vos.h"
#include  "AtCtx.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 常量定义
*****************************************************************************/
enum
{
    AT_MSG_SERV_STATE_NOT_SUPPORT,
    AT_MSG_SERV_STATE_SUPPORT
};
typedef VOS_UINT8 AT_MSG_SERV_STATE_ENUM_U8;

enum AT_MSG_PARSE_STATUS
{
    AT_MSG_PARSE_STATUS_START,
    AT_MSG_PARSE_STATUS_LOWERDATA,
    AT_MSG_PARSE_STATUS_WAITUPPER,
    AT_MSG_PARSE_STATUS_UPPERDATA,
    AT_MSG_PARSE_STATUS_END,
    AT_MSG_PARSE_STATUS_BUTT
};
typedef VOS_UINT32 AT_MSG_PARSE_STATUS_UINT32;

typedef VOS_UINT32 (* AT_MSG_STATUS_CHK_FUNC)(TAF_UINT8);


/*****************************************************************************
  3类型定义
*****************************************************************************/
typedef struct
{
    AT_MSG_SERV_STATE_ENUM_U8           enSmsMT;
    AT_MSG_SERV_STATE_ENUM_U8           enSmsMO;
    AT_MSG_SERV_STATE_ENUM_U8           enSmsBM;
}AT_MSG_SERV_STRU;

/*****************************************************************************
  4 宏定义
*****************************************************************************/
#define AT_MAX_TIMEZONE_VALUE                               48

#define At_GetNumTypeFromAddrType(enNumType, ucAddrType) ((enNumType) = ((ucAddrType >> 4) & 0x07))
#define At_GetNumPlanFromAddrType(enNumPlan, ucAddrType) ((enNumPlan) = (ucAddrType & 0x0f))

#define AT_MSG_TP_MTI_DELIVER                               0x00                /* SMS-DELIVER       */
#define AT_MSG_TP_MTI_DELIVER_REPORT                        0x00                /* SMS-DELIVER-REPORT*/
#define AT_MSG_TP_MTI_STATUS_REPORT                         0x02                /* SMS-STATUS-REPORT */
#define AT_MSG_TP_MTI_COMMAND                               0x02                /* SMS-COMMAND       */
#define AT_MSG_TP_MTI_SUBMIT                                0x01                /* SMS-SUBMIT        */
#define AT_MSG_TP_MTI_SUBMIT_REPORT                         0x01                /* SMS-SUBMIT-REPORT */
#define AT_MSG_TP_MTI_RESERVE                               0x03                /* RESERVE           */

/*23040 9.2.3.1 bits no 0 and 1 of the first octet of all PDUs*/
#define AT_GET_MSG_TP_MTI(ucMti, ucFo)                         ((ucMti) = ((ucFo) & AT_MSG_TP_MTI_MASK))

#define AT_GET_MSG_TP_RD(bFlag, ucFo)                          ((bFlag) = ((ucFo) & 0x04) ? TAF_TRUE : TAF_FALSE)
/*23040 9.2.3.2 bit no 2 of the first octet of SMS DELIVER and SMS STATUS REPORT*/
#define AT_GET_MSG_TP_MMS(bFlag, ucFo)                         ((bFlag) = ((ucFo) & 0x04) ? TAF_FALSE : TAF_TRUE)

/*23040 9.2.3.26 bit no. 5 of the first octet of SMS STATUS REPORT*/
#define AT_GET_MSG_TP_SRQ(bFlag, ucFo)                         ((bFlag) = ((ucFo) & 0x20) ? TAF_TRUE : TAF_FALSE)

/*23040 9.2.3.4 bit no. 5 of the first octet of SMS DELIVER*/
#define AT_GET_MSG_TP_SRI( bFlag, ucFo)                        ((bFlag) = ((ucFo) & 0x20) ? TAF_TRUE : TAF_FALSE)

/*23040 9.2.3.5 bit no. 5 of the first octet of SMS SUBMIT and SMS COMMAND*/
#define AT_GET_MSG_TP_SRR( bFlag, ucFo)                        ((bFlag) = ((ucFo) & 0x20) ? TAF_TRUE : TAF_FALSE)

#define AT_GET_MSG_TP_UDHI( bFlag, ucFo)                       ((bFlag) = ((ucFo) & 0x40) ? TAF_TRUE : TAF_FALSE)

#define AT_GET_MSG_TP_RP( bFlag, ucFo)                         ((bFlag) = ((ucFo) & 0x80) ? TAF_TRUE : TAF_FALSE)


/*23040 9.2.3.1 bits no 0 and 1 of the first octet of all PDUs*/
#define AT_SET_MSG_TP_MTI(ucFo, ucMti)                         ((ucFo) |= ((ucMti) & 0x03))
/*23040 9.2.3.25  1 bit field located within bit 2 of the first octet of SMS SUBMIT*/
#define AT_SET_MSG_TP_RD(ucFo, bFlag)                          ((ucFo) |= (TAF_TRUE == (bFlag)) ? 0x04 : 0)
/*23040 9.2.3.2 bit no 2 of the first octet of SMS DELIVER and SMS STATUS REPORT*/
#define AT_SET_MSG_TP_MMS(ucFo, bFlag)                         ((ucFo) |= (TAF_TRUE == (bFlag)) ? 0 : 0x04)
/*23040 9.2.3.3 bit no 3 and 4 of the first octet of SMS SUBMIT*/
#define AT_SET_MSG_TP_VPF(ucFo, ucVpf)                         ((ucFo) |= (((ucVpf) << 3) & 0x18))
/*23040 9.2.3.26 bit no. 5 of the first octet of SMS STATUS REPORT*/
#define AT_SET_MSG_TP_SRQ(ucFo, bFlag)                         ((ucFo) |= (TAF_TRUE == (bFlag)) ? 0x20 : 0)
/*23040 9.2.3.4 bit no. 5 of the first octet of SMS DELIVER*/
#define AT_SET_MSG_TP_SRI(ucFo, bFlag)                         ((ucFo) |= (TAF_TRUE == (bFlag)) ? 0x20 : 0)
/*23040 9.2.3.5 bit no. 5 of the first octet of SMS SUBMIT and SMS COMMAND*/
#define AT_SET_MSG_TP_SRR(ucFo, bFlag)                         ((ucFo) |= (TAF_TRUE == (bFlag)) ? 0x20 : 0)
/*23040 9.2.3.23 1 bit field within bit 6 of the first octet of SMS SUBMIT SMS-SUBMIT-REPORT SMS DELIVER,SMS-DELIVER-REPORT SMS-STATUS-REPORT SMS-COMMAND.*/
#define AT_SET_MSG_TP_UDHI(ucFo, bFlag)                        ((ucFo) |= (TAF_TRUE == (bFlag))? 0x40 : 0)
/*23040 9.2.3.17 1 bit field, located within bit no 7 of the first octet of both SMS DELIVER and SMS SUBMIT,*/
#define AT_SET_MSG_TP_RP(ucFo, bFlag)                          ((ucFo) |= (TAF_TRUE == (bFlag))? 0x80 : 0)

/*****************************************************************************
  5 全局变量声明
*****************************************************************************/


/*****************************************************************************
  6 接口函数声明
*****************************************************************************/


VOS_UINT32 At_SendDomainProtoToNvim(
    AT_CGSMS_SEND_DOMAIN_ENUM_U8        enProtoSendDomain
);

/*****************************************************************************
 Prototype      : At_CheckEnd
 Description    : 比较、匹配结束符
 Input          : Char---需检查的字符
 Output         : ---
 Return Value   : AT_SUCCESS --- 成功
                  AT_FAILURE --- 失败
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-04-19
    Author      : ---
    Modification: Created function
*****************************************************************************/
extern VOS_UINT32 At_CheckEnd( VOS_UINT8 Char );

/*****************************************************************************
 Prototype      : At_CheckJuncture
 Description    : 比较、匹配连接符
 Input          : Char---需检查的字符
 Output         : ---
 Return Value   : AT_SUCCESS --- 成功
                  AT_FAILURE --- 失败
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-04-19
    Author      : ---
    Modification: Created function
*****************************************************************************/
extern VOS_UINT32 At_CheckJuncture( VOS_UINT8 Char );


extern VOS_UINT32 At_ParseCsmpFo(
    VOS_UINT8                           *pucFo
);

/* Modified by l60609 for DSDA Phase III, 2013-3-5, Begin */

VOS_UINT32 At_ParseCsmpVp(
    VOS_UINT8                           ucIndex,
    MN_MSG_VALID_PERIOD_STRU           *pstVp
);
/* Modified by l60609 for DSDA Phase III, 2013-3-5, End */

/* Added by f62575 for AT Project, 2011-10-04,  Begin */
VOS_UINT32  AT_AsciiNumberToBcd(
    const VOS_CHAR                      *pcAsciiNumber,
    VOS_UINT8                           *pucBcdNumber,
    VOS_UINT8                           *pucBcdLen
);
/* Added by f62575 for AT Project, 2011-10-04,  End */

VOS_UINT32  AT_BcdNumberToAscii(
    const VOS_UINT8                     *pucBcdNumber,
    VOS_UINT8                           ucBcdLen,
    VOS_CHAR                            *pcAsciiNumber
);

#if ((TAF_OS_VER == TAF_WIN32) || (TAF_OS_VER == TAF_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif  /* __ATMSGPRINT_H__ */

#endif /* end of AtSmsPrint.h */
