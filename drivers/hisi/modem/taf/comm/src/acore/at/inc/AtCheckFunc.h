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

#ifndef _ATCHECKFUNC_H_
#define _ATCHECKFUNC_H_

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "AtParse.h"
#include "PsCommonDef.h"
#include "AtMntn.h"

/* Moved from AtGnainterface.h by 傅映君/f62575 for AT Project, 2011/11/13, begin */
#define AT_GNA_E5_MAC_ADDR_LEN_BETWEEN_COLONS   (2)                             /* MAC地址中冒号之间的字符个数*/
#define AT_GNA_E5_MAC_ADDR_COLON_NUM            (5)                             /* MAC地址中冒号的个数*/
#define AT_MAC_ADDR_LEN                         (32)                            /* MAC地址长度 AT_GNA_E5_MAC_ADDR_LEN ->AT_MAC_ADDR_LEN */
/* Moved from AtGnainterface.h by 傅映君/f62575 for AT Project, 2011/11/13, end */
#define AT_CDATA_DIAL_777_LEN                   (4)
#define AT_DATA_DIAL_LEN                        (3)

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)
/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*******************************************************************************
  3 枚举定义
*******************************************************************************/

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/

/*****************************************************************************
  5 消息头定义
*****************************************************************************/
/* 模块名＋意义＋HEADER */

/*****************************************************************************
  6 消息定义
*****************************************************************************/

/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/*****************************************************************************
  8 UNION定义
*****************************************************************************/

/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
TAF_UINT32 At_CheckSemicolon( TAF_UINT8 Char );
TAF_UINT32 At_CheckDialNum( TAF_UINT8 Char );
TAF_UINT32 At_CheckDmChar( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharStar( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharWell( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharRightArrow( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharG( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharI( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharS( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharD( TAF_UINT8 Char );
TAF_UINT32 At_CheckBasicCmdName( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharA( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharT( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharPlus( TAF_UINT8 Char );
TAF_UINT32 At_CheckLeftBracket( TAF_UINT8 Char );
TAF_UINT32 At_CheckRightBracket( TAF_UINT8 Char );
TAF_UINT32 At_CheckDigit( TAF_UINT8 Char );
TAF_UINT32 At_CheckLetter( TAF_UINT8 Char );
TAF_UINT32 At_CheckUpLetter( TAF_UINT8 Char );
TAF_UINT32 At_CheckChar( TAF_UINT8 Char );
TAF_UINT32 At_CheckAlpha( TAF_UINT8 Char );
TAF_UINT32 At_CheckColon( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharSub( TAF_UINT8 Char );
TAF_UINT32 At_CheckQuot( TAF_UINT8 Char );
TAF_UINT32 At_CheckHorizontalLine( TAF_UINT8 Char );
TAF_UINT32 At_CheckEqu( TAF_UINT8 Char );
TAF_UINT32 At_CheckReq( TAF_UINT8 Char );
TAF_UINT32 At_CheckNoQuot( TAF_UINT8 Char );
TAF_UINT32 At_CheckHorizontalLinePara( AT_PARSE_PARA_TYPE_STRU *pPara);
TAF_UINT32 At_CheckHexStrWithoutQuotationPara( AT_PARSE_PARA_TYPE_STRU *pPara);
TAF_UINT32 At_CheckNumPara( AT_PARSE_PARA_TYPE_STRU *pPara);
TAF_UINT32 At_CheckCharPara( AT_PARSE_PARA_TYPE_STRU *pPara);
TAF_UINT32 At_CheckNumString( TAF_UINT8 *pData,TAF_UINT16 usLen );
TAF_UINT32 At_CheckNumCharString( TAF_UINT8 *pData,TAF_UINT16 usLen );
TAF_UINT32 At_CheckMmiString( TAF_UINT8 *pData,TAF_UINT16 usLen );
TAF_UINT32 At_CheckChar0x26( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharE( TAF_UINT8 Char );
TAF_UINT32 At_CheckChar1( TAF_UINT8 Char );
TAF_UINT32 At_CheckChar0( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharV( TAF_UINT8 Char );
TAF_UINT32 At_CheckCharF( TAF_UINT8 Char );

TAF_UINT32 At_CheckRightArrowStr( TAF_UINT8 *pData,TAF_UINT16 usLen );

VOS_UINT32 AT_CheckDateFormat(
    VOS_UINT8                           *pucDateStr,
    VOS_UINT32                          ulDateStrLen
);

VOS_UINT32  AT_GetDate(
    VOS_UINT8                           *pucDateStr,
    VOS_UINT32                          ulDateStrLen,
    AT_DATE_STRU                        *pstDate
);

VOS_UINT32  AT_CheckDate(
    AT_DATE_STRU                        *pstDate
);
VOS_UINT32  At_GetParaCnt(
    VOS_UINT8                           *pucData,
    VOS_UINT16                          usLen
);

VOS_UINT32 At_CheckAndParsePara(
    AT_PARSE_PARA_TYPE_STRU             *pstAtPara,
    VOS_UINT8                           *pucBegain,
    VOS_UINT8                           *pucEnd
);

VOS_UINT32 AT_CheckStrStartWith(
    VOS_UINT8                          *pucLineStr,
    VOS_UINT16                          usLineStrLen,
    VOS_UINT8                          *pucPefixStr,
    VOS_UINT16                          usPefixStrLen
);

/* 比较、匹配基本命令名 */
VOS_UINT32 atCheckBasicCmdName( VOS_UINT8 Char );

/* 比较、匹配空格 */
VOS_UINT32 atCheckblank( VOS_UINT8 Char );

/* 比较、匹配逗号 */
VOS_UINT32 atCheckComma( VOS_UINT8 Char );

/* 比较、匹配冒号 */
VOS_UINT32 atCheckColon( VOS_UINT8 Char );

VOS_UINT32 atCheckChar0x2f( VOS_UINT8 Char );

/* Added by l60609 for At Prj, 2011-10-12, Begin */
/* 比较、匹配字符+ */
VOS_UINT32 atCheckCharPlus( VOS_UINT8 Char );

VOS_UINT32 atCheckChar( VOS_UINT8 Char );

VOS_UINT32 atCheckEqu( VOS_UINT8 Char );

VOS_UINT32 atCheckReq( VOS_UINT8 Char );

VOS_UINT32 atCheckQuot( VOS_UINT8 Char );

VOS_UINT32 atCheckNoQuot( VOS_UINT8 Char );


/* Added by l60609 for At Prj, 2011-10-12, End */

/* 比较、匹配字符串类型 */
VOS_UINT32 At_CheckStringPara( AT_PARSE_PARA_TYPE_STRU *pPara);

/* 比较、匹配数值类型的参数 */
VOS_UINT32 atCheckNumPara( AT_PARSE_PARA_TYPE_STRU *pPara);

/* 比较、匹配字符范围类型的参数 */
VOS_UINT32 atCheckCharPara( AT_PARSE_PARA_TYPE_STRU *pPara);

/* 比较、匹配数字 */
VOS_UINT32 atCheckNumString( VOS_UINT8 *pData,VOS_UINT16 usLen );

/* 判断字符串中是否有"*"和"#" */
VOS_UINT32 atCheckMmiString( VOS_UINT8 *pData,VOS_UINT16 usLen );

/* 判断字符串中是否有">" */
VOS_UINT32 atCheckRightArrowStr( VOS_UINT8 *pData,VOS_UINT16 usLen );


/******************************************************************************
 功能描述: 比较、匹配无双引号包括字符串类型

 参数说明:
   pPara [in/out] 需检查的字符串

 返 回 值: AT_SUCCESS --- 字符串的首尾都是 "
           AT_FAILURE --- 字符串的首尾不全是 "
******************************************************************************/
extern VOS_UINT32 atCheckNoQuotStringPara(AT_PARSE_PARA_TYPE_STRU *pPara);


/******************************************************************************
 功能描述: 比较、匹配无双引号包括字母起始

 参数说明:
   Char [in] 需检查的字符

 返 回 值: AT_SUCCESS --- 成功
           AT_FAILURE --- 失败
******************************************************************************/
extern VOS_UINT32 atNoQuotLetter(VOS_UINT8 Char);


/******************************************************************************
 功能描述: 比较、匹配十六进制(0x或0X)

 参数说明:
   Char [in] 需检查的字符

 返 回 值: AT_SUCCESS --- 成功
           AT_FAILURE --- 失败
******************************************************************************/
extern VOS_UINT32 atCheckHex(VOS_UINT8 Char);


/******************************************************************************
 功能描述: 比较、匹配十六进制数字

 参数说明:
   Char [in] 需检查的字符

 返 回 值: AT_SUCCESS --- 成功
           AT_FAILURE --- 失败
******************************************************************************/
extern VOS_UINT32 atCheckHexNum(VOS_UINT8 Char);

extern VOS_UINT32 atfwParseSaveParam(VOS_UINT8* pStringBuf, VOS_UINT16 usLen);

extern VOS_UINT32 AT_SaveCdataDialParam(
    VOS_UINT8                          *pStringBuf,
    VOS_UINT16                          usLen
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
#endif


#endif /* __AT_CHECK_FUNC_H__ */
