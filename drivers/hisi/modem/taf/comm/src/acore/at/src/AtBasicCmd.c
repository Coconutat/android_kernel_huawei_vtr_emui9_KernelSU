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

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "AtParse.h"
#include "ATCmdProc.h"
#include "AtTestParaCmd.h"


/* ADD by c64416 for V9R1/V7R1 AT, 2013/09/18 begin */
#include "at_lte_common.h"
/* ADD by c64416 for V9R1/V7R1 AT, 2013/09/18 end */


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_AT_BASICCMD_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/* 命令受限标志宏定义

#define CMD_TBL_E5_IS_LOCKED        (0x00000001)     不受E5锁定控制的命令
#define CMD_TBL_PIN_IS_LOCKED       (0x00000002)     不受PIN码锁定控制的命令
#define CMD_TBL_IS_E5_DOCK          (0x00000004)     E5 DOCK命令
#define CMD_TBL_CLAC_IS_INVISIBLE   (0x00000008)     +CLAC命令中不输出显示的命令

*/

/* AT_PAR_CMD_ELEMENT_STRU g_astAtBasicCmdTbl[] = {}; */

/* 示例: ^CMDX 命令是不受E5密码保护命令，且在+CLAC列举所有命令时不显示，第一个参数是不带双引号的字符串,
        第二个参数是带双引号的字符串，第三个参数是整数型参数

   !!!!!!!!!!!注意: param1和param2是示例，实际定义命令时应尽量定义的简短(可提高解析效率)!!!!!!!!!!!!!

    {AT_CMD_CMDX,
    At_SetCmdxPara, AT_SET_PARA_TIME, At_QryCmdxPara, AT_QRY_PARA_TIME, At_TestCmdxPara, AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_E5_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE,
    (VOS_UINT8*)"^CMDX", (VOS_UINT8*)"(@param1),(param2),(0-255)"},
*/

AT_PAR_CMD_ELEMENT_STRU g_astAtBasicCmdTbl[] =
{
    /* CS */
    {AT_CMD_H,
    At_SetHPara,    AT_DETACT_PDP_TIME, VOS_NULL_PTR,   AT_NOT_SET_TIME,    At_CmdTestProcOK,   AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"H", (VOS_UINT8*)"(0)"},

    {AT_CMD_A,
    At_SetAPara,    AT_SET_PARA_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_LIMITED_NULL,
    (VOS_UINT8*)"A", VOS_NULL_PTR},

    {AT_CMD_D,
    At_SetDPara,    AT_SET_PARA_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"D", VOS_NULL_PTR},


    {AT_CMD_S0,
    At_SetS0Para,   AT_NOT_SET_TIME,    At_QryS0Para,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"S0", (VOS_UINT8*)"(0-255)"},

    /* ATC */
    {AT_CMD_S3,
    At_SetS3Para,   AT_NOT_SET_TIME,    At_QryS3Para,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"S3",   (VOS_UINT8*)"(0-127)"},

    {AT_CMD_S4,
    At_SetS4Para,   AT_NOT_SET_TIME,    At_QryS4Para,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"S4",   (VOS_UINT8*)"(0-127)"},

    {AT_CMD_S5,
    At_SetS5Para,   AT_NOT_SET_TIME,    At_QryS5Para,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"S5",   (VOS_UINT8*)"(0-127)"},

    {AT_CMD_S6,
    At_SetS6Para,   AT_NOT_SET_TIME,    At_QryS6Para,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"S6",   (VOS_UINT8*)"(2-10)"},

    {AT_CMD_S7,
    At_SetS7Para,   AT_NOT_SET_TIME,    At_QryS7Para,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"S7",   (VOS_UINT8*)"(1-255)"},

    {AT_CMD_E,
    At_SetEPara,    AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_E5_IS_LOCKED | CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"E",    (VOS_UINT8*)"(0,1)"},

    {AT_CMD_V,
    At_SetVPara,    AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"V",    (VOS_UINT8*)"(0,1)"},

    {AT_CMD_I,
    At_SetMsIdInfo, AT_SET_PARA_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_E5_IS_LOCKED | CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"I",    (VOS_UINT8*)"(0-255)"},

    {AT_CMD_T,
    At_SetTPara,    AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (TAF_UINT8*)"T",    TAF_NULL_PTR},

    {AT_CMD_P,
    At_SetPPara,    AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"P",    TAF_NULL_PTR},

    {AT_CMD_X,
    At_SetXPara,    AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"X",    (VOS_UINT8*)"(0-4)"},

    {AT_CMD_Z,
    At_SetZPara,    AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"Z",    (VOS_UINT8*)"(0)"},

    {AT_CMD_Q,
    At_SetQPara,    AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"Q",    (VOS_UINT8*)"(0,1)"},

    {AT_CMD_AMP_C,
    At_SetAmpCPara, AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"&C",    (VOS_UINT8*)"(0-2)"},

    {AT_CMD_AMP_D,
    At_SetAmpDPara, AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"&D",    (VOS_UINT8*)"(0-2)"},


    {AT_CMD_AMP_F,
    atSetNVFactoryRestore,    AT_NOT_SET_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"&F",    (VOS_UINT8*)"(0)"},


    {AT_CMD_L,
    AT_SetLPara, AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"L",    (VOS_UINT8*)"(0)"},

    {AT_CMD_M,
    AT_SetMPara, AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,    VOS_NULL_PTR,       AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE_E5,
    (VOS_UINT8*)"M",    (VOS_UINT8*)"(1)"},



};

/*****************************************************************************
  3 函数实现
*****************************************************************************/


VOS_UINT32 At_RegisterBasicCmdTable(VOS_VOID)
{
    return AT_RegisterCmdTable(g_astAtBasicCmdTbl, sizeof(g_astAtBasicCmdTbl)/sizeof(g_astAtBasicCmdTbl[0]));
}


/* Added by c64416 for AT Project 2011-10-23  Begin */

VOS_UINT32 At_CheckIfDCmdValidChar(VOS_UINT8 ucChar)
{
    /* LINUX系统下的拨号命令格式为:ATDT"*99#"，比规范的拨号命令多了引号，而引号不在
       27007和ITUT-V.250规范中的拨号命令合法字符集中，但为了保证LINUX拨号成功，增加
       引号为拨号命令的合法字符 */
    VOS_UINT8 aucDCmdValidChar[] = {'1', '2', '3', '4', '5', '6', '7', '8',
        '9', '0', '*', '#', '+', 'A', 'B', 'C', ',', 'T', 'P', '!', 'W', '@',
        '>', 'I', 'i', 'G', 'g', '"'};

    VOS_UINT8 ucIndex = 0;

    while (ucIndex < sizeof(aucDCmdValidChar))
    {
        if (ucChar == aucDCmdValidChar[ucIndex])
        {
            return AT_SUCCESS;
        }

        ucIndex++;
    }

    return AT_FAILURE;
}



VOS_UINT8* At_GetFirstBasicCmdAddr(VOS_UINT8 *pData, VOS_UINT32* pulLen)
{
    VOS_UINT32 i = 0, j = 0;
    VOS_UINT8* pucAddr = NULL;
    VOS_UINT8* pucCurAddr = NULL;
    VOS_UINT32 ulDCmdLen = 0;
    VOS_UINT32 ulBasicCmdNum = sizeof(g_astAtBasicCmdTbl)/sizeof(AT_PAR_CMD_ELEMENT_STRU);

    for(i = 0; i < ulBasicCmdNum; i++)
    {
        if(NULL == g_astAtBasicCmdTbl[i].pszCmdName)
        {
            break;
        }

        pucCurAddr = (VOS_UINT8*)strstr((VOS_CHAR*)pData, (VOS_CHAR*)(g_astAtBasicCmdTbl[i].pszCmdName));
        if(NULL != pucCurAddr)
        {
            if((NULL == pucAddr) || (pucCurAddr < pucAddr))
            {
                if(AT_CMD_D == g_astAtBasicCmdTbl[i].ulCmdIndex)  /* D命令中可能有特殊字符，需要特殊处理 */
                {
                    ulDCmdLen = strlen((VOS_CHAR*)pucCurAddr);

                    /* 依次检查D命令后面的有效字符，1表示'D'字符不检测 */
                    for(j = 1; j < ulDCmdLen; j++)
                    {
                        if(AT_SUCCESS != At_CheckIfDCmdValidChar(*(pucCurAddr+j)))
                        {
                            /* 遇到非D命令的参数字符后退出 */
                            break;
                        }
                    }

                    *pulLen = j;
                }
                else
                {
                    *pulLen = strlen((VOS_CHAR*)(g_astAtBasicCmdTbl[i].pszCmdName));
                }

                pucAddr = pucCurAddr;
            }
        }
    }

    return pucAddr;
}
/* Added by c64416 for AT Project 2011-10-23  End*/


