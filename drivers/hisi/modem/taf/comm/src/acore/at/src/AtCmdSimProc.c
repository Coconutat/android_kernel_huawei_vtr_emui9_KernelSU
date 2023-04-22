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
#include "AtCmdSimProc.h"
#include "AtEventReport.h"


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID                 PS_FILE_ID_AT_CMD_SIM_PROC_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 外部接口声明
*****************************************************************************/
extern VOS_UINT32 AT_Hex2AsciiStrLowHalfFirst(
    VOS_UINT32                          ulMaxLength,
    VOS_INT8                            *pcHeadaddr,
    VOS_UINT8                           *pucDst,
    VOS_UINT8                           *pucSrc,
    VOS_UINT16                          usSrcLen
);

/*****************************************************************************
  4 函数实现
*****************************************************************************/


VOS_UINT32 At_IsSimSlotAllowed(
    VOS_UINT32                          ulModem0Slot,
    VOS_UINT32                          ulModem1Slot,
    VOS_UINT32                          ulModem2Slot
)
{
    /* modem0和modem1配置成相同的硬卡,返回错误 */
    if ( (ulModem0Slot == ulModem1Slot)
      && (SI_PIH_CARD_SLOT_2 != ulModem0Slot) )
    {
        return VOS_FALSE;
    }

    /* modem0和modem2配置成相同的硬卡,返回错误 */
    if ( (ulModem0Slot == ulModem2Slot)
      && (SI_PIH_CARD_SLOT_2 != ulModem0Slot) )
    {
        return VOS_FALSE;
    }

    /* modem1和modem2配置成相同的硬卡,返回错误 */
    if ( (ulModem1Slot == ulModem2Slot)
      && (SI_PIH_CARD_SLOT_2 != ulModem1Slot) )
    {
        return VOS_FALSE;
    }

    /* 所有modem均对应空卡槽,返回错误 */
    if ((SI_PIH_CARD_SLOT_2 == ulModem0Slot)
     && (SI_PIH_CARD_SLOT_2 == ulModem1Slot)
     && (SI_PIH_CARD_SLOT_2 == ulModem2Slot))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}


VOS_UINT32 At_SetSIMSlotPara(VOS_UINT8 ucIndex)
{
    TAF_NV_SCI_CFG_STRU                 stSCICfg;
    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数过多 */
    if (gucAtParaIndex > 3)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数检查 */
    if ( (0 == gastAtParaList[0].usParaLen)
       ||(0 == gastAtParaList[1].usParaLen) )
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 单modem不支持切换卡槽 */

    /* 三卡形态第3个参数不能为空，其余形态默认为卡槽2 */
    if (0 == gastAtParaList[2].usParaLen)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数检查 */
    if (VOS_FALSE == At_IsSimSlotAllowed(gastAtParaList[0].ulParaValue,
                                         gastAtParaList[1].ulParaValue,
                                         gastAtParaList[2].ulParaValue) )
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 从NV中读取当前SIM卡的SCI配置 */
    TAF_MEM_SET_S(&stSCICfg, sizeof(stSCICfg), 0x00, sizeof(stSCICfg));
    if (NV_OK != NV_ReadEx(MODEM_ID_0,
                            ev_NV_Item_SCI_DSDA_CFG,
                            &stSCICfg,
                            sizeof(stSCICfg)))
    {
        AT_ERR_LOG("At_SetSIMSlotPara: ev_NV_Item_SCI_DSDA_CFG read fail!");
        return AT_ERROR;
    }

    /*
         根据用户设置的值修改card0位和card1位的值，在NV项中，这两项对应的bit位和取值含义如下:
         card0: bit[8-10]：卡槽0使用的SCI接口
             0：使用SCI0（默认值）
             1：使用SCI1
             2：使用SCI2
             其余值：无效
         card1:bit[11-13]：卡1槽使用的SCI接口
             0：使用SCI0
             1：使用SCI1（默认值）
             2：使用SCI2
             其余值：无效
         card2:bit[14-16]：卡2槽使用的SCI接口
             0：使用SCI0
             1：使用SCI1
             2：使用SCI2（默认值）
             其余值：无效
     */
    stSCICfg.bitCard0   = gastAtParaList[0].ulParaValue;
    stSCICfg.bitCard1   = gastAtParaList[1].ulParaValue;

    /* 针对双卡形态增加保护，bitCard2使用NV默认值，与底软处理适配 */
    stSCICfg.bitCard2   = gastAtParaList[2].ulParaValue;
    stSCICfg.bitCardNum = 3;

    stSCICfg.bitReserved0 = 0;
    stSCICfg.bitReserved1 = 0;


    /* 将设置的SCI值保存到NV中 */
    if (NV_OK != NV_WriteEx(MODEM_ID_0,
                            ev_NV_Item_SCI_DSDA_CFG,
                            &stSCICfg,
                            sizeof(stSCICfg)))
    {
        AT_ERR_LOG("At_SetSIMSlotPara: ev_NV_Item_SCI_DSDA_CFG write failed");
        return AT_ERROR;
    }

    return AT_OK;
}


VOS_UINT32 At_QrySIMSlotPara(VOS_UINT8 ucIndex)
{
    TAF_NV_SCI_CFG_STRU                 stSCICfg;
    VOS_UINT16                          usLength;

    /*从NV中读取当前SIM卡的SCI配置*/
    TAF_MEM_SET_S(&stSCICfg, sizeof(stSCICfg), 0x00, sizeof(stSCICfg));
    if (NV_OK != NV_ReadEx(MODEM_ID_0,
                            ev_NV_Item_SCI_DSDA_CFG,
                            &stSCICfg,
                            sizeof(stSCICfg)))
    {
        AT_ERR_LOG("At_QrySIMSlotPara: ev_NV_Item_SCI_DSDA_CFG read fail!");
        gstAtSendData.usBufLen = 0;
        return AT_ERROR;
    }

    usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      "%s: %d,%d",
                                      g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                      stSCICfg.bitCard0,
                                      stSCICfg.bitCard1);

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       ",%d",
                                       stSCICfg.bitCard2);

    gstAtSendData.usBufLen = usLength;

    return AT_OK;

}


VOS_UINT32 At_Base16Decode(VOS_CHAR *pcData, VOS_UINT32 ulDataLen, VOS_UINT8* pucDst)
{
    VOS_UINT32 ulLen    = ulDataLen;
    VOS_UINT32 i        = 0;
    VOS_CHAR   n[2] = {0};
    VOS_UINT32 j;

    while(i < ulLen)
    {
        for(j = 0; j < 2; j++)
        {
            if(pcData[(VOS_ULONG)(i+j)] >= '0' && pcData[(VOS_ULONG)(i+j)] <= '9')
            {
                n[(VOS_ULONG)j] = pcData[(VOS_ULONG)(i+j)] - '0';
            }
            else if(pcData[(VOS_ULONG)(i+j)] >= 'a' && pcData[(VOS_ULONG)(i+j)] <= 'f')
            {
                n[(VOS_ULONG)j] = pcData[(VOS_ULONG)(i+j)] - 'a' + 10;
            }
            else if(pcData[(VOS_ULONG)(i+j)] >= 'A' && pcData[(VOS_ULONG)(i+j)] <= 'F')
            {
                n[(VOS_ULONG)j] = pcData[(VOS_ULONG)(i+j)] - 'A' + 10;
            }
            else
            {
                ;
            }
        }

        pucDst[i/2] = (VOS_UINT8)(n[0] * 16 + n[1]);

        i += 2;
    }

    return (ulDataLen/2);
}


VOS_UINT32 At_SetHvsstPara(
    VOS_UINT8                           ucIndex
)
{
    VOS_UINT32                              ulResult;
    SI_PIH_HVSST_SET_STRU                   stHvSStSet;

    /* 命令类型检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数过多 */
    if (gucAtParaIndex != 2)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数检查 */
    if ( (0 == gastAtParaList[0].usParaLen)
      || (0 == gastAtParaList[1].usParaLen) )
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    TAF_MEM_SET_S(&stHvSStSet, sizeof(stHvSStSet), 0x00, sizeof(stHvSStSet));

    stHvSStSet.ucIndex = (VOS_UINT8)gastAtParaList[0].ulParaValue;
    stHvSStSet.enSIMSet = (VOS_UINT8)gastAtParaList[1].ulParaValue;

    ulResult = SI_PIH_HvSstSet(gastAtClientTab[ucIndex].usClientId,
                               gastAtClientTab[ucIndex].opId,
                               &stHvSStSet);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("At_SetHvsstPara: SI_PIH_HvSstSet fail.");
        return AT_ERROR;
    }

    /* 设置AT模块实体的状态为等待异步返回 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_HVSST_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 At_QryHvsstPara(
    VOS_UINT8                           ucIndex
)
{
    VOS_UINT32                          ulResult;

    ulResult = SI_PIH_HvSstQuery(gastAtClientTab[ucIndex].usClientId,
                                 gastAtClientTab[ucIndex].opId);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_QryPortAttribSetPara: SI_PIH_HvSstQuery fail.");
        return AT_ERROR;
    }

    /* 设置AT模块实体的状态为等待异步返回 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_HVSST_QRY;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT16 At_HvsstQueryCnf(
    VOS_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent)
{
    VOS_UINT16                          usLength;
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT32                          ulRslt;
    TAF_NV_SCI_CFG_STRU                 stSCICfg;
    VOS_UINT32                          ulSlot;
    SI_PIH_SIM_INDEX_ENUM_UINT8         enSimIndex;

    usLength    = 0;
    ulRslt      = AT_GetModemIdFromClient(ucIndex, &enModemId);

    if (VOS_OK != ulRslt)
    {
        AT_ERR_LOG("At_HvsstQueryCnf: Get modem id fail.");
        return usLength;
    }

    /* 从NV中读取当前SIM卡的SCI配置 */
    TAF_MEM_SET_S(&stSCICfg, sizeof(stSCICfg), 0x00, sizeof(stSCICfg));
    if (NV_OK != NV_ReadEx(MODEM_ID_0,
                            ev_NV_Item_SCI_DSDA_CFG,
                            &stSCICfg,
                            sizeof(stSCICfg)))
    {
        AT_ERR_LOG("At_HvsstQueryCnf: ev_NV_Item_SCI_DSDA_CFG read fail!");
        return usLength;
    }

    if (MODEM_ID_0 == enModemId)
    {
        ulSlot = stSCICfg.bitCard0;
    }
    else if (MODEM_ID_1 == enModemId)
    {
        ulSlot = stSCICfg.bitCard1;
    }
    else
    {
        ulSlot = stSCICfg.bitCard2;
    }

    if (SI_PIH_SIM_ENABLE == pstEvent->PIHEvent.HVSSTQueryCnf.enVSimState)
    {
        enSimIndex = SI_PIH_SIM_VIRT_SIM1;
    }
    else
    {
        enSimIndex = SI_PIH_SIM_REAL_SIM1;
    }

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "%s: %d,%d,%d,%d",
                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                       enSimIndex,
                                       1,
                                       ulSlot,
                                       pstEvent->PIHEvent.HVSSTQueryCnf.enCardUse);

    return usLength;
}


VOS_UINT32 At_SetSciChgPara(
    VOS_UINT8                           ucIndex
)
{
    VOS_UINT32                          ulResult;
    /* 命令类型检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数过多 */
    if (gucAtParaIndex > 3)
    {
        return AT_TOO_MANY_PARA;
    }

    /* 参数检查 */
    if ( (0 == gastAtParaList[0].usParaLen)
       ||(0 == gastAtParaList[1].usParaLen) )
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 单modem不支持卡槽切换 */

    /* 三卡形态第3个参数不能为空，其余形态默认为卡槽2 */
    if (0 == gastAtParaList[2].usParaLen)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 任意两个Modem不能同时配置为同一卡槽 */
    if ( (gastAtParaList[0].ulParaValue == gastAtParaList[1].ulParaValue)
      || (gastAtParaList[0].ulParaValue == gastAtParaList[2].ulParaValue)
      || (gastAtParaList[1].ulParaValue == gastAtParaList[2].ulParaValue) )
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulResult = SI_PIH_SciCfgSet(gastAtClientTab[ucIndex].usClientId,
                                gastAtClientTab[ucIndex].opId,
                                gastAtParaList[0].ulParaValue,
                                gastAtParaList[1].ulParaValue,
                                gastAtParaList[2].ulParaValue);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("At_SetSciChgPara: SI_PIH_HvSstSet fail.");
        return AT_CME_PHONE_FAILURE;
    }

    /* 设置AT模块实体的状态为等待异步返回 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_SCICHG_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 At_QrySciChgPara(
    VOS_UINT8                           ucIndex
)
{
    VOS_UINT32                          ulResult;

    ulResult = SI_PIH_SciCfgQuery(gastAtClientTab[ucIndex].usClientId,
                                  gastAtClientTab[ucIndex].opId);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("At_QrySciChgPara: SI_PIH_SciCfgQuery fail.");
        return AT_ERROR;
    }

    /* 设置AT模块实体的状态为等待异步返回 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_SCICHG_QRY;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT16 At_SciCfgQueryCnf(
    VOS_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent)
{
    VOS_UINT16                          usLength;

    usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      "%s: %d,%d",
                                      g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                      pstEvent->PIHEvent.SciCfgCnf.enCard0Slot,
                                      pstEvent->PIHEvent.SciCfgCnf.enCard1Slot);

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       ",%d",
                                       pstEvent->PIHEvent.SciCfgCnf.enCard2Slot);

    return usLength;
}

/*****************************************************************************
 Prototype      : At_Hex2Base16
 Description    : 将16进制转换BASE64编码
 Input          : nptr --- 字符串
 Output         :
 Return Value   : 数据长度
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-04-19
    Author      : ---
    Modification: Created function
*****************************************************************************/
VOS_UINT16 At_Hex2Base16(VOS_UINT32 MaxLength,VOS_CHAR *headaddr,VOS_CHAR *pucDst,VOS_UINT8 *pucSrc,VOS_UINT16 usSrcLen)
{
    VOS_UINT32          i;
    VOS_CHAR            *pcData;

    pcData = pucDst;

    for(i=0; i<usSrcLen; i++)
    {
        At_sprintf((VOS_INT32)MaxLength,headaddr,pcData,"%X",((pucSrc[i]&0xF0)>>4));

        pcData++;

        At_sprintf((VOS_INT32)MaxLength,headaddr,pcData,"%X",(pucSrc[i]&0x0F));

        pcData++;
    }

    return (VOS_UINT16)(usSrcLen*2);
}


VOS_UINT32 At_QryIccVsimVer(
    VOS_UINT8                           ucIndex
)
{
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       "^ICCVSIMVER: %u",
                                       SI_PIH_GetSecIccVsimVer());
    return AT_OK;
}


VOS_UINT32 At_QryHvCheckCardPara(
    VOS_UINT8                           ucIndex
)
{
    if(AT_SUCCESS == SI_PIH_HvCheckCardQuery(gastAtClientTab[ucIndex].usClientId,
                                            gastAtClientTab[ucIndex].opId))
    {
        /* 设置当前操作类型 */
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_HVSCONT_READ;

        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }

    return AT_ERROR;
}


/*****************************************************************************
 Prototype      : AT_UiccAuthCnf
 Description    : 命令返回
 Input          : pstEvent --- 消息内容
 Output         :
 Return Value   : 数据长度
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-04-19
    Author      : ---
    Modification: Created function
*****************************************************************************/
VOS_UINT16 AT_UiccAuthCnf(TAF_UINT8 ucIndex, SI_PIH_EVENT_INFO_STRU *pstEvent)
{
    VOS_UINT16 usLength = 0;

    if (AT_CMD_UICCAUTH_SET == gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"^UICCAUTH:");

        /* <result> */
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"%d",pstEvent->PIHEvent.UiccAuthCnf.enStatus);

        if (SI_PIH_AUTH_SUCCESS == pstEvent->PIHEvent.UiccAuthCnf.enStatus)
        {
            /* ,<Res> */
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,",\"");
            usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr+usLength, &pstEvent->PIHEvent.UiccAuthCnf.stAkaData.aucAuthRes[1], pstEvent->PIHEvent.UiccAuthCnf.stAkaData.aucAuthRes[0]);
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"\"");

            if (SI_PIH_UICCAUTH_AKA == pstEvent->PIHEvent.UiccAuthCnf.enAuthType)
            {
                /* ,<ck> */
                usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,",\"");
                usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr+usLength, &pstEvent->PIHEvent.UiccAuthCnf.stAkaData.aucCK[1], pstEvent->PIHEvent.UiccAuthCnf.stAkaData.aucCK[0]);
                usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"\"");

                /* ,<ik> */
                usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,",\"");
                usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr+usLength, &pstEvent->PIHEvent.UiccAuthCnf.stAkaData.aucIK[1], pstEvent->PIHEvent.UiccAuthCnf.stAkaData.aucIK[0]);
                usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"\"");
            }
        }

        if (SI_PIH_AUTH_SYNC == pstEvent->PIHEvent.UiccAuthCnf.enStatus)
        {
            /* ,"","","",<autn> */
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,",\"\",\"\",\"\",\"");
            usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr+usLength, &pstEvent->PIHEvent.UiccAuthCnf.stAkaData.aucAuts[1], pstEvent->PIHEvent.UiccAuthCnf.stAkaData.aucAuts[0]);
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"\"");
        }
    }

    if (AT_CMD_KSNAFAUTH_SET == gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"^KSNAFAUTH:");

        /* <status> */
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"%d",pstEvent->PIHEvent.UiccAuthCnf.enStatus);

        if (VOS_NULL != pstEvent->PIHEvent.UiccAuthCnf.stNAFData.aucKs_ext_NAF[0])
        {
            /* ,<ks_Naf> */
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,",\"");
            usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr+usLength, &pstEvent->PIHEvent.UiccAuthCnf.stNAFData.aucKs_ext_NAF[1], pstEvent->PIHEvent.UiccAuthCnf.stNAFData.aucKs_ext_NAF[0]);
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"\"");
        }
    }

    return usLength;
}

/*****************************************************************************
 Prototype      : AT_UiccAccessFileCnf
 Description    : 命令返回
 Input          : pstEvent --- 消息内容
 Output         :
 Return Value   : 数据长度
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-04-19
    Author      : ---
    Modification: Created function
*****************************************************************************/
VOS_UINT16 AT_UiccAccessFileCnf(TAF_UINT8 ucIndex, SI_PIH_EVENT_INFO_STRU *pstEvent)
{
    VOS_UINT16      usLength = 0;

    if ((0 != pstEvent->PIHEvent.UiccAcsFileCnf.ulDataLen)
        && (SI_PIH_ACCESS_READ == pstEvent->PIHEvent.UiccAcsFileCnf.enCmdType))
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"^CURSM:");

        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"\"");
        usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr+usLength, pstEvent->PIHEvent.UiccAcsFileCnf.aucCommand, (VOS_UINT16)pstEvent->PIHEvent.UiccAcsFileCnf.ulDataLen);
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,(VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR *)pgucAtSndCodeAddr + usLength,"\"");
    }

    return usLength;
}


TAF_UINT32 At_CrlaFilePathCheck(
    TAF_UINT32                          ulEfId,
    TAF_UINT8                          *pucFilePath,
    TAF_UINT16                         *pusPathLen)
{
    TAF_UINT16                          usLen;
    TAF_UINT16                          ausPath[USIMM_MAX_PATH_LEN]  = {0};
    TAF_UINT16                          ausTmpPath[USIMM_MAX_PATH_LEN]  = {0};
    TAF_UINT16                          usPathLen;
    TAF_UINT16                          i;

    usPathLen   = *pusPathLen;
    usLen       = 0;

    if (usPathLen > USIMM_MAX_PATH_LEN)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    for (i = 0; i < (usPathLen/sizeof(TAF_UINT16)); i++)
    {
        ausTmpPath[i] = ((pucFilePath[i*2]<<0x08)&0xFF00) + pucFilePath[(i*2)+1];
    }

    /* 如果路径不是以3F00开始，需要添加3F00作开头 */
    if (MFID != ausTmpPath[0])
    {
        if (USIMM_MAX_PATH_LEN == usPathLen)
        {
            return AT_CME_INCORRECT_PARAMETERS;
        }

        ausPath[0] = MFID;

        usLen++;
    }

    TAF_MEM_CPY_S(&ausPath[usLen], sizeof(TAF_UINT16) * (USIMM_MAX_PATH_LEN - usLen), ausTmpPath, usPathLen);

    usLen += (usPathLen/sizeof(TAF_UINT16));

    if ((ulEfId & 0xFF00) == EFIDUNDERMF)
    {
        usLen = 1;
    }
    /* 4F文件要在5F下，路径长度为3 */
    else if ((ulEfId & 0xFF00) == EFIDUNDERMFDFDF)
    {
        if ((usLen != 3)
            ||((ausPath[1]&0xFF00) != DFIDUNDERMF)
            ||((ausPath[2]&0xFF00) != DFIDUNDERMFDF))
        {
            return AT_CME_INCORRECT_PARAMETERS;
        }
    }
    /* 6F文件要在7F下，路径长度为2 */
    else if ((ulEfId & 0xFF00) == EFIDUNDERMFDF)
    {
        if ((usLen != 2)
            ||((ausPath[1]&0xFF00) != DFIDUNDERMF))
        {
            return AT_CME_INCORRECT_PARAMETERS;
        }
    }
    else
    {
        /* return AT_CME_INCORRECT_PARAMETERS; */
    }

    *pusPathLen  = usLen;

    TAF_MEM_CPY_S(pucFilePath, AT_PARA_MAX_LEN + 1, ausPath, (VOS_SIZE_T)(usLen*2));

    return AT_SUCCESS;
}


TAF_UINT32 At_CrlaApduParaCheck(VOS_VOID)
{
    TAF_UINT16                          usFileTag;

    /* 命令类型参数检查，第二个参数不能为空 */
    if (0 == gastAtParaList[1].usParaLen)
    {
        AT_ERR_LOG("At_CrlaApduParaCheck: command type null");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 除STATUS命令外，文件ID输入不能为空 */
    if ((0 == gastAtParaList[2].ulParaValue)
        && (USIMM_STATUS != gastAtParaList[1].ulParaValue))
    {
        AT_ERR_LOG("At_CrlaApduParaCheck: File Id null.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 获取文件ID前两位 */
    usFileTag   = (gastAtParaList[2].ulParaValue >> 8) & (0x00FF);

    /* 输入的文件ID必须是EF文件，前两位不可以是3F/5F/7F */
    if ((MFLAB == usFileTag)
       || (DFUNDERMFLAB == usFileTag)
       || (DFUNDERDFLAB == usFileTag))
    {
        AT_ERR_LOG("At_CrlaApduParaCheck: File Id error.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* <P1><P2><P3>这三个参数全部为空 */
    if ((0 == gastAtParaList[3].usParaLen)
        && (0 == gastAtParaList[4].usParaLen)
        && (0 == gastAtParaList[5].usParaLen))
    {
        return AT_SUCCESS;
    }

    /* <P1><P2><P3>这三个参数全部不为空 */
    if ((0 != gastAtParaList[3].usParaLen)
        && (0 != gastAtParaList[4].usParaLen)
        && (0 != gastAtParaList[5].usParaLen))
    {
        return AT_SUCCESS;
    }

    /* 其它情况下属于输入AT命令参数不完整 */
    return AT_CME_INCORRECT_PARAMETERS;

}


TAF_UINT32 At_CrlaFilePathParse(
    SI_PIH_CRLA_STRU                   *pstCommand
)
{
    TAF_UINT32                          ulResult;

    /* 如果词法解析器解析第八个参数为空，说明没有文件路径输入，直接返回成功 */
    if ((0 == gastAtParaList[7].usParaLen)
     && (VOS_NULL_WORD != pstCommand->usEfId))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 在转换前输入的文件路径长度必须是4的整数倍 */
    if (0 != (gastAtParaList[7].usParaLen % 4))
    {
        AT_ERR_LOG("At_CrlaFilePathParse: Path error");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /*将输入的字符串转换成十六进制数组*/
    if(AT_FAILURE == At_AsciiNum2HexString(gastAtParaList[7].aucPara, &gastAtParaList[7].usParaLen))
    {
        AT_ERR_LOG("At_CrlaFilePathParse: At_AsciiNum2HexString error.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 如果有填写文件ID和路径，要做文件路径检查，输入的路径长度以U16为单位 */
    ulResult = At_CrlaFilePathCheck((TAF_UINT16)gastAtParaList[2].ulParaValue,
                                    gastAtParaList[7].aucPara,
                                   &gastAtParaList[7].usParaLen);

    if (AT_SUCCESS != ulResult)
    {
        AT_ERR_LOG("At_CrlaFilePathParse: At_CrsmFilePathCheck error.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 填充文件路径和长度 */
    pstCommand->usPathLen   = gastAtParaList[7].usParaLen;

    /* 文件路径长度是U16为单位的，路径拷贝的长度要乘2 */
    TAF_MEM_CPY_S(pstCommand->ausPath, sizeof(pstCommand->ausPath), gastAtParaList[7].aucPara, (VOS_SIZE_T)(gastAtParaList[7].usParaLen*sizeof(VOS_UINT16)));

    return AT_SUCCESS;
}


TAF_UINT32 At_CrlaParaStatusCheck(
    SI_PIH_CRLA_STRU                   *pstCommand
)
{
    /* STATUS命令如果没有输入文件ID，就不需要做选文件操作，直接发STATUS命令 */
    if (0 == gastAtParaList[2].ulParaValue)
    {
        pstCommand->usEfId = VOS_NULL_WORD;
    }
    else
    {
        pstCommand->usEfId = (TAF_UINT16)gastAtParaList[2].ulParaValue;
    }

    /* 填写数据结构中的<P1><P2><P3>对应的IE项 */
    pstCommand->ucP1        =   (TAF_UINT8)gastAtParaList[3].ulParaValue;
    pstCommand->ucP2        =   (TAF_UINT8)gastAtParaList[4].ulParaValue;
    pstCommand->ucP3        =   (TAF_UINT8)gastAtParaList[5].ulParaValue;
    pstCommand->enCmdType   =   USIMM_STATUS;

    return At_CrlaFilePathParse(pstCommand);
}


TAF_UINT32 At_CrlaParaReadBinaryCheck(
    SI_PIH_CRLA_STRU                   *pstCommand
)
{
    /* 填写数据结构中的<P1><P2><P3>对应的IE项 */
    pstCommand->ucP1        =   (TAF_UINT8)gastAtParaList[3].ulParaValue;
    pstCommand->ucP2        =   (TAF_UINT8)gastAtParaList[4].ulParaValue;
    pstCommand->ucP3        =   (TAF_UINT8)gastAtParaList[5].ulParaValue;
    pstCommand->usEfId      =   (TAF_UINT16)gastAtParaList[2].ulParaValue;
    pstCommand->enCmdType   =   USIMM_READ_BINARY;

    /* 如果有输入文件路径需要检查输入参数 */
    return At_CrlaFilePathParse(pstCommand);
}


TAF_UINT32 At_CrlaParaReadRecordCheck(
    SI_PIH_CRLA_STRU                   *pstCommand
)
{
    /* 填写数据结构中的<P1><P2><P3>对应的IE项 */
    pstCommand->ucP1        =   (TAF_UINT8)gastAtParaList[3].ulParaValue;
    pstCommand->ucP2        =   (TAF_UINT8)gastAtParaList[4].ulParaValue;
    pstCommand->ucP3        =   (TAF_UINT8)gastAtParaList[5].ulParaValue;
    pstCommand->usEfId      =   (TAF_UINT16)gastAtParaList[2].ulParaValue;
    pstCommand->enCmdType   =   USIMM_READ_RECORD;

    /* 如果有输入文件路径需要检查输入参数 */
    return At_CrlaFilePathParse(pstCommand);
}


VOS_UINT32 At_CrlaParaGetRspCheck(
    SI_PIH_CRLA_STRU                   *pstCommand
)
{
    /* 参数个数不能少于2个，至少要有命令类型和文件ID */
    if (gucAtParaIndex < 2)
    {
        AT_ERR_LOG("At_CrlaParaGetRspCheck: Para less than 2.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 填写数据结构中的<P1><P2><P3>对应的IE项 */
    pstCommand->ucP1        =   (TAF_UINT8)gastAtParaList[3].ulParaValue;
    pstCommand->ucP2        =   (TAF_UINT8)gastAtParaList[4].ulParaValue;
    pstCommand->ucP3        =   (TAF_UINT8)gastAtParaList[5].ulParaValue;
    pstCommand->usEfId      =   (TAF_UINT16)gastAtParaList[2].ulParaValue;
    pstCommand->enCmdType   =   USIMM_GET_RESPONSE;

    /* 如果有输入文件路径需要检查输入参数 */
    return At_CrlaFilePathParse(pstCommand);
}


VOS_UINT32 At_CrlaParaUpdateBinaryCheck(
    SI_PIH_CRLA_STRU                       *pstCommand
)
{
    /* Update Binary命令至少要有6个参数，可以没有文件路径 */
    if (gucAtParaIndex < 6)
    {
        AT_ERR_LOG("At_CrlaParaUpdateBinaryCheck: Para less than 6.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 填写数据结构中的<P1><P2><P3>对应的IE项 */
    pstCommand->ucP1        =   (TAF_UINT8)gastAtParaList[3].ulParaValue;
    pstCommand->ucP2        =   (TAF_UINT8)gastAtParaList[4].ulParaValue;
    pstCommand->ucP3        =   (TAF_UINT8)gastAtParaList[5].ulParaValue;
    pstCommand->usEfId      =   (TAF_UINT16)gastAtParaList[2].ulParaValue;
    pstCommand->enCmdType   =   USIMM_UPDATE_BINARY;

    /* 第六个参数输入的<data>字符串在转换前数据长度必须是2的倍数且不能为0 */
    if ((0 != (gastAtParaList[6].usParaLen % 2))
        || (0 == gastAtParaList[6].usParaLen))
    {
        AT_ERR_LOG("At_CrlaParaUpdateBinaryCheck: <data> error.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    if(AT_FAILURE == At_AsciiNum2HexString(gastAtParaList[6].aucPara, &gastAtParaList[6].usParaLen))
    {
        AT_ERR_LOG("At_CrlaParaUpdateBinaryCheck: At_AsciiNum2HexString fail.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (gastAtParaList[6].usParaLen > sizeof(pstCommand->aucContent))
    {
        AT_ERR_LOG("At_CrlaParaUpdateBinaryCheck: gastAtParaList[6] too long");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 设置<data>，其长度由<data>参数输入确定，P3参数照常下发，不关心<data>的长度是否和P3的值匹配 */
    TAF_MEM_CPY_S((TAF_VOID*)pstCommand->aucContent,
               sizeof(pstCommand->aucContent),
               (TAF_VOID*)gastAtParaList[6].aucPara,
               gastAtParaList[6].usParaLen);

    return At_CrlaFilePathParse(pstCommand);
}


VOS_UINT32 At_CrlaParaUpdateRecordCheck (
    SI_PIH_CRLA_STRU                   *pstCommand
)
{

    /* Update Binary命令至少要有6个参数，可以没有文件路径 */
    if (gucAtParaIndex < 6)
    {
        AT_ERR_LOG("At_CrlaParaUpdateRecordCheck: Para less than 6.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 填写数据结构中的<P1><P2><P3>对应的IE项 */
    pstCommand->ucP1        =   (TAF_UINT8)gastAtParaList[3].ulParaValue;
    pstCommand->ucP2        =   (TAF_UINT8)gastAtParaList[4].ulParaValue;
    pstCommand->ucP3        =   (TAF_UINT8)gastAtParaList[5].ulParaValue;
    pstCommand->usEfId      =   (TAF_UINT16)gastAtParaList[2].ulParaValue;
    pstCommand->enCmdType   =   USIMM_UPDATE_RECORD;

     /* 第六个参数输入的<data>字符串数据长度必须是2的倍数且不能为0 */
    if ((0 != (gastAtParaList[6].usParaLen % 2))
        || (0 == gastAtParaList[6].usParaLen))
    {
        AT_ERR_LOG("At_CrlaParaUpdateRecordCheck: <data> error.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    if(AT_FAILURE == At_AsciiNum2HexString(gastAtParaList[6].aucPara, &gastAtParaList[6].usParaLen))
    {
        AT_ERR_LOG("At_CrlaParaUpdateRecordCheck: At_AsciiNum2HexString fail.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 防止因为数据长度过长导致单板复位 */
    if (gastAtParaList[6].usParaLen > sizeof(pstCommand->aucContent))
    {
        AT_ERR_LOG("At_CrlaParaUpdateRecordCheck: gastAtParaList[6] too long");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 设置<data>，其长度由<data>参数输入确定，P3参数照常下发，不关心<data>的长度是否和P3的值匹配 */
    TAF_MEM_CPY_S((TAF_VOID*)pstCommand->aucContent,
               sizeof(pstCommand->aucContent),
               (TAF_VOID*)gastAtParaList[6].aucPara,
               gastAtParaList[6].usParaLen);

    return At_CrlaFilePathParse(pstCommand);
}


TAF_UINT32 At_SetCrlaPara(TAF_UINT8 ucIndex)
{
    SI_PIH_CRLA_STRU                    stCommand;
    TAF_UINT32                          ulResult;

    /* 参数过多 */
    if (gucAtParaIndex > 8)
    {
        AT_ERR_LOG("At_SetCrlaPara: too many para");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 如果有输入<P1><P2><P3>，这三个参数不能只填写部分参数 */
    if (AT_SUCCESS != At_CrlaApduParaCheck())
    {
       AT_ERR_LOG("At_SetCrlaPara: At_CrlaApduParaCheck fail.");

       return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 初始化 */
    TAF_MEM_SET_S(&stCommand, sizeof(stCommand), 0x00, sizeof(SI_PIH_CRLA_STRU));

    stCommand.ulSessionID = gastAtParaList[0].ulParaValue;

    switch(gastAtParaList[1].ulParaValue)
    {
        case USIMM_STATUS:
            ulResult = At_CrlaParaStatusCheck(&stCommand);
            break;
        case USIMM_READ_BINARY:
            ulResult = At_CrlaParaReadBinaryCheck(&stCommand);
            break;
        case USIMM_READ_RECORD:
            ulResult = At_CrlaParaReadRecordCheck(&stCommand);
            break;
        case USIMM_GET_RESPONSE:
            ulResult = At_CrlaParaGetRspCheck(&stCommand);
            break;
        case USIMM_UPDATE_BINARY:
            ulResult = At_CrlaParaUpdateBinaryCheck(&stCommand);
            break;
        case USIMM_UPDATE_RECORD:
            ulResult = At_CrlaParaUpdateRecordCheck(&stCommand);
            break;
        default:
            return AT_CME_INCORRECT_PARAMETERS;
    }

    if (AT_SUCCESS != ulResult )
    {
        AT_ERR_LOG("At_SetCrlaPara: para parse fail");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 执行命令操作 */
    if (TAF_SUCCESS == SI_PIH_CrlaSetReq(gastAtClientTab[ucIndex].usClientId, 0,&stCommand))
    {
        /* 设置当前操作类型 */
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_CRLA_SET;
        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }
    else
    {
        return AT_ERROR;
    }
}


VOS_UINT32 At_QryCardSession(VOS_UINT8 ucIndex)
{
    if (TAF_SUCCESS == SI_PIH_CardSessionQuery(gastAtClientTab[ucIndex].usClientId, gastAtClientTab[ucIndex].opId))
    {
        /* 设置当前操作类型 */
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_CARDSESSION_QRY;
        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }

    AT_WARN_LOG("At_QryCardSession: SI_PIH_CardSessionQuery fail.");

    /* 返回命令处理挂起状态 */
    return AT_ERROR;
}


TAF_UINT32 At_QryCardVoltagePara(TAF_UINT8 ucIndex)
{
    VOS_UINT32 ulResult;

    ulResult = SI_PIH_CardVoltageQuery(gastAtClientTab[ucIndex].usClientId,
                                       gastAtClientTab[ucIndex].opId);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("At_QryCardVoltagePara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_CARDVOLTAGE_QUERY;

    return AT_WAIT_ASYNC_RETURN;
}


TAF_UINT32 At_SetPrivateCglaPara(TAF_UINT8 ucIndex)
{
    SI_PIH_CGLA_COMMAND_STRU    stCglaCmd;

    /* 参数检查 */
    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数过多 */
    if(3 != gucAtParaIndex)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* <length>需要为2的整数倍 */
    if(0 != (gastAtParaList[1].ulParaValue % 2))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 字符串长度不为2的整数倍 */
    if(0 != (gastAtParaList[2].usParaLen % 2))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 将字符串转换为16进制数组 */
    if(AT_FAILURE == At_AsciiNum2HexString(gastAtParaList[2].aucPara, &gastAtParaList[2].usParaLen))
    {
        AT_ERR_LOG("At_SetCglaCmdPara: At_AsciiNum2HexString fail.");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* length字段是实际命令长度的2倍 */
    if(gastAtParaList[1].ulParaValue != (TAF_UINT32)(gastAtParaList[2].usParaLen * 2))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stCglaCmd.ulSessionID   = gastAtParaList[0].ulParaValue;
    stCglaCmd.ulLen         = gastAtParaList[2].usParaLen;
    stCglaCmd.pucCommand    = gastAtParaList[2].aucPara;

    /* 执行命令操作 */
    if(AT_SUCCESS == SI_PIH_PrivateCglaSetReq(gastAtClientTab[ucIndex].usClientId, 0, &stCglaCmd))
    {
        /* 设置当前操作类型 */
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_PRIVATECGLA_REQ;
        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }

    return AT_ERROR;
}


TAF_UINT32 AT_SetPrfApp(
    AT_CARDAPP_ENUM_UINT32              enCardApp,
    USIMM_NV_CARDAPP_ENUM_UINT32        enCardAPP,
    MODEM_ID_ENUM_UINT16                enModemId)
{
    USIMM_APP_PRIORITY_CONFIG_STRU      stAppInfo;
    TAF_UINT32                          ulAppHit      = 0;
    TAF_UINT32                          ulAppOrderPos = 0;
    TAF_UINT32                          ulRslt        = NV_OK;
    TAF_UINT32                          i;

    TAF_MEM_SET_S(&stAppInfo, (VOS_SIZE_T)sizeof(stAppInfo), 0x00, (VOS_SIZE_T)sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));

    ulRslt = NV_ReadEx(enModemId,
                       en_NV_Item_Usim_App_Priority_Cfg,
                       &stAppInfo,
                       sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));

    if (NV_OK != ulRslt)
    {
        AT_ERR_LOG("AT_SetPrfApp: Get en_NV_Item_Usim_App_Priority_Cfg fail.");

        return AT_ERROR;
    }

    /* 设置CDMA应用优先 */
    for (i = 0; i < stAppInfo.ucAppNum; i++)
    {
        if (enCardAPP == stAppInfo.aenAppList[i])
        {
            ulAppHit      = VOS_TRUE;
            ulAppOrderPos = i;
            break;
        }
    }

    if (VOS_FALSE == ulAppHit)
    {
        /* 没有找到，插入到最前边 */
        if (USIMM_NV_CARDAPP_BUTT <= stAppInfo.ucAppNum)
        {
            ulRslt = AT_ERROR;
        }
        else
        {
            VOS_MemMove_s((VOS_VOID *)&stAppInfo.aenAppList[1],
                        sizeof(stAppInfo.aenAppList) - 1 * sizeof(stAppInfo.aenAppList[1]),
                        (VOS_VOID *)&stAppInfo.aenAppList[0],
                        (sizeof(VOS_UINT32) * stAppInfo.ucAppNum));

            stAppInfo.aenAppList[0] = enCardAPP;
            stAppInfo.ucAppNum ++;

            ulRslt = NV_WriteEx(enModemId,
                                en_NV_Item_Usim_App_Priority_Cfg,
                                &stAppInfo,
                                sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));
        }
    }
    else
    {
        if (0 != ulAppOrderPos)
        {
            /* 从第一个起，往后移动i个*/
            VOS_MemMove_s((VOS_VOID *)&stAppInfo.aenAppList[1],
                        sizeof(stAppInfo.aenAppList) - 1 * sizeof(stAppInfo.aenAppList[1]),
                        (VOS_VOID *)&stAppInfo.aenAppList[0],
                        (sizeof(VOS_UINT32) * ulAppOrderPos));

            stAppInfo.aenAppList[0] = enCardAPP;

            ulRslt = NV_WriteEx(enModemId,
                                en_NV_Item_Usim_App_Priority_Cfg,
                                &stAppInfo,
                                sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));
        }
    }

    if (NV_OK != ulRslt)
    {
        AT_ERR_LOG("NV_WriteEx: Write NV Fail.");
        return AT_ERROR;
    }

    return AT_OK;
}


TAF_UINT32 At_SetPrfAppPara(TAF_UINT8 ucIndex)
{
    MODEM_ID_ENUM_UINT16                enModemId;
    TAF_UINT32                          ulRslt = AT_OK;

    /* 参数过多 */
    if (gucAtParaIndex > 1)
    {
        AT_ERR_LOG("At_SetPrfAppPara: too many para");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 根据AT命令通道号得到MODEM ID */
    if (VOS_OK != AT_GetModemIdFromClient(ucIndex, &enModemId))
    {
        AT_ERR_LOG("AT_SetRATCombinePara: Get modem id fail.");

        return AT_ERROR;
    }

    if (AT_PREFER_APP_CDMA == gastAtParaList[0].ulParaValue)
    {
        ulRslt = AT_SetPrfApp(AT_PREFER_APP_CDMA, USIMM_NV_CDMA_APP, enModemId);
    }
    else
    {
        ulRslt = AT_SetPrfApp(AT_PREFER_APP_GUTL, USIMM_NV_GUTL_APP, enModemId);
    }

    return ulRslt;
}


TAF_UINT32 At_QryPrfAppPara(TAF_UINT8 ucIndex)
{
    TAF_UINT32                          i;
    TAF_UINT32                          ulCdmaHit;
    TAF_UINT32                          ulGutlHit;
    USIMM_APP_PRIORITY_CONFIG_STRU      stAppInfo;
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT16                          usLength;

    /* 根据AT命令通道号得到MODEM ID */
    if (VOS_OK != AT_GetModemIdFromClient(ucIndex, &enModemId))
    {
        AT_ERR_LOG("At_QryPrfAppPara: Get modem id fail.");

        return AT_ERROR;
    }

    TAF_MEM_SET_S(&stAppInfo, (VOS_SIZE_T)sizeof(stAppInfo), 0x00, (VOS_SIZE_T)sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));

    if (NV_OK != NV_ReadEx(enModemId, en_NV_Item_Usim_App_Priority_Cfg, &stAppInfo, sizeof(USIMM_APP_PRIORITY_CONFIG_STRU)))
    {
        AT_ERR_LOG("At_QryPrfAppPara: Get en_NV_Item_Usim_App_Priority_Cfg fail.");

        return AT_ERROR;
    }

    ulCdmaHit = VOS_FALSE;
    ulGutlHit = VOS_FALSE;

    /* 查找CDMA和GUTL应用在NV项中的位置 */
    for (i = 0; i < stAppInfo.ucAppNum; i++)
    {
        if (USIMM_NV_GUTL_APP == stAppInfo.aenAppList[i])
        {
            ulGutlHit = VOS_TRUE;

            break;
        }

        if (USIMM_NV_CDMA_APP == stAppInfo.aenAppList[i])
        {
            ulCdmaHit = VOS_TRUE;

            break;
        }
    }

    if (VOS_TRUE == ulGutlHit)
    {
        usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          "%s: 1",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

        gstAtSendData.usBufLen = usLength;

        return AT_OK;
    }
    else if (VOS_TRUE == ulCdmaHit)
    {
        usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          "%s: 0",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

        gstAtSendData.usBufLen = usLength;

        return AT_OK;
    }
    else
    {
        return AT_ERROR;
    }
}


TAF_UINT32 At_TestPrfAppPara(TAF_UINT8 ucIndex)
{
    VOS_UINT16      usLength;

    usLength = 0;

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "%s: (0,1)",
                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

    gstAtSendData.usBufLen = usLength;

    return AT_OK;
}


TAF_UINT32 AT_SetUiccPrfApp(
    AT_CARDAPP_ENUM_UINT32              enCardApp,
    USIMM_NV_CARDAPP_ENUM_UINT32        enCardAPP,
    MODEM_ID_ENUM_UINT16                enModemId)
{
    USIMM_APP_PRIORITY_CONFIG_STRU      stAppInfo;
    TAF_UINT32                          ulAppHit      = 0;
    TAF_UINT32                          ulAppOrderPos = 0;
    TAF_UINT32                          ulRslt        = NV_OK;
    TAF_UINT32                          i;

    TAF_MEM_SET_S(&stAppInfo, (VOS_SIZE_T)sizeof(stAppInfo), 0x00, (VOS_SIZE_T)sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));

    ulRslt = NV_ReadEx(enModemId,
                       en_NV_Item_Usim_Uicc_App_Priority_Cfg,
                       &stAppInfo,
                       sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));

    if (NV_OK != ulRslt)
    {
        AT_ERR_LOG("AT_SetUiccPrfApp: Get en_NV_Item_Usim_Uicc_App_Priority_Cfg fail.");

        return AT_ERROR;
    }

    /* 设置CDMA应用优先 */
    for (i = 0; i < stAppInfo.ucAppNum; i++)
    {
        if (enCardAPP == stAppInfo.aenAppList[i])
        {
            ulAppHit      = VOS_TRUE;
            ulAppOrderPos = i;
            break;
        }
    }

    if (VOS_FALSE == ulAppHit)
    {
        /* 没有找到，插入到最前边 */
        if (USIMM_NV_CARDAPP_BUTT <= stAppInfo.ucAppNum)
        {
            ulRslt = AT_ERROR;
        }
        else
        {
            (VOS_VOID)VOS_MemMove_s((VOS_VOID *)&stAppInfo.aenAppList[1],
                                     sizeof(stAppInfo.aenAppList) - 1 * sizeof(stAppInfo.aenAppList[1]),
                                     (VOS_VOID *)&stAppInfo.aenAppList[0],
                                     (sizeof(VOS_UINT32) * stAppInfo.ucAppNum));

            stAppInfo.aenAppList[0] = enCardAPP;
            stAppInfo.ucAppNum ++;

            ulRslt = NV_WriteEx(enModemId,
                                en_NV_Item_Usim_Uicc_App_Priority_Cfg,
                                &stAppInfo,
                                sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));
        }
    }
    else
    {
        if (0 != ulAppOrderPos)
        {
            /* 从第一个起，往后移动i个*/
            (VOS_VOID)VOS_MemMove_s((VOS_VOID *)&stAppInfo.aenAppList[1],
                                     sizeof(stAppInfo.aenAppList) - 1 * sizeof(stAppInfo.aenAppList[1]),
                                     (VOS_VOID *)&stAppInfo.aenAppList[0],
                                     (sizeof(VOS_UINT32) * ulAppOrderPos));

            stAppInfo.aenAppList[0] = enCardAPP;

            ulRslt = NV_WriteEx(enModemId,
                                en_NV_Item_Usim_Uicc_App_Priority_Cfg,
                                &stAppInfo,
                                sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));
        }
    }

    if (NV_OK != ulRslt)
    {
        AT_ERR_LOG("NV_WriteEx: Write NV Fail.");
        return AT_ERROR;
    }

    return AT_OK;
}


TAF_UINT32 At_SetUiccPrfAppPara(TAF_UINT8 ucIndex)
{
    MODEM_ID_ENUM_UINT16                enModemId;
    TAF_UINT32                          ulRslt = AT_OK;

    /* 参数过多 */
    if (gucAtParaIndex > 1)
    {
        AT_ERR_LOG("At_SetUiccPrfAppPara: too many para");

        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 根据AT命令通道号得到MODEM ID */
    if (VOS_OK != AT_GetModemIdFromClient(ucIndex, &enModemId))
    {
        AT_ERR_LOG("At_SetUiccPrfAppPara: Get modem id fail.");

        return AT_ERROR;
    }

    if (AT_PREFER_APP_CDMA == gastAtParaList[0].ulParaValue)
    {
        ulRslt = AT_SetUiccPrfApp(AT_PREFER_APP_CDMA, USIMM_NV_CDMA_APP, enModemId);
    }
    else
    {
        ulRslt = AT_SetUiccPrfApp(AT_PREFER_APP_GUTL, USIMM_NV_GUTL_APP, enModemId);
    }

    return ulRslt;
}


TAF_UINT32 At_QryUiccPrfAppPara(TAF_UINT8 ucIndex)
{
    TAF_UINT32                          i;
    TAF_UINT32                          ulCdmaHit;
    TAF_UINT32                          ulGutlHit;
    USIMM_APP_PRIORITY_CONFIG_STRU      stAppInfo;
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT16                          usLength;

    /* 根据AT命令通道号得到MODEM ID */
    if (VOS_OK != AT_GetModemIdFromClient(ucIndex, &enModemId))
    {
        AT_ERR_LOG("At_QryUiccPrfAppPara: Get modem id fail.");

        return AT_ERROR;
    }

    TAF_MEM_SET_S(&stAppInfo, (VOS_SIZE_T)sizeof(stAppInfo), 0x00, (VOS_SIZE_T)sizeof(USIMM_APP_PRIORITY_CONFIG_STRU));

    if (NV_OK != NV_ReadEx(enModemId, en_NV_Item_Usim_Uicc_App_Priority_Cfg, &stAppInfo, sizeof(USIMM_APP_PRIORITY_CONFIG_STRU)))
    {
        AT_ERR_LOG("At_QryUiccPrfAppPara: Get en_NV_Item_Usim_Uicc_App_Priority_Cfg fail.");

        return AT_ERROR;
    }

    ulCdmaHit = VOS_FALSE;
    ulGutlHit = VOS_FALSE;

    /* 查找CDMA和GUTL应用在NV项中谁优先 */
    for (i = 0; i < stAppInfo.ucAppNum; i++)
    {
        if (USIMM_NV_GUTL_APP == stAppInfo.aenAppList[i])
        {
            ulGutlHit = VOS_TRUE;

            break;
        }

        if (USIMM_NV_CDMA_APP == stAppInfo.aenAppList[i])
        {
            ulCdmaHit = VOS_TRUE;

            break;
        }
    }

    if (VOS_TRUE == ulGutlHit)
    {
        usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          "%s: 1",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

        gstAtSendData.usBufLen = usLength;

        return AT_OK;
    }
    else if (VOS_TRUE == ulCdmaHit)
    {
        usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          "%s: 0",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

        gstAtSendData.usBufLen = usLength;

        return AT_OK;
    }
    else
    {
        return AT_ERROR;
    }
}


TAF_UINT32 At_TestUiccPrfAppPara(TAF_UINT8 ucIndex)
{
    VOS_UINT16                          usLength;

    usLength = 0;

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "%s: (0,1)",
                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

    gstAtSendData.usBufLen = usLength;

    return AT_OK;
}


TAF_UINT32 At_SetCCimiPara(TAF_UINT8 ucIndex)
{
    /* 参数检查 */
    if (AT_CMD_OPT_SET_CMD_NO_PARA != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 执行命令操作 */
    if (AT_SUCCESS == SI_PIH_CCimiSetReq(gastAtClientTab[ucIndex].usClientId,0))
    {
        /* 设置当前操作类型 */
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_CCIMI_SET;
        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }
    else
    {
        return AT_ERROR;
    }
}


TAF_UINT16 At_CardErrorInfoInd(
    TAF_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent)
{
    VOS_UINT16                          usLength = 0;
    VOS_UINT32                          i;

    if (VOS_NULL == pstEvent->PIHEvent.UsimmErrorInd.ulErrNum)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          "%s^USIMMEX: NULL\r\n",
                                          gaucAtCrLf);

        return usLength;
    }

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      "%s^USIMMEX: ",
                                      gaucAtCrLf);

    for(i=0; i<pstEvent->PIHEvent.UsimmErrorInd.ulErrNum; i++)
    {
        if (i == 0)
        {
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                          "0x%X",
                                          pstEvent->PIHEvent.UsimmErrorInd.aulErrInfo[i]);
        }
        else
        {
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                          ",0x%X",
                                          pstEvent->PIHEvent.UsimmErrorInd.aulErrInfo[i]);
        }
    }

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      (VOS_CHAR *)pgucAtSndCodeAddr+ usLength,
                                      "%s",
                                      gaucAtCrLf);

    return usLength;
}


TAF_UINT16 At_CardIccidInfoInd(
    TAF_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent
)
{
    VOS_UINT16                          usLength = 0;

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      "%s^USIMICCID: ",
                                      gaucAtCrLf);

    usLength += (TAF_UINT16)AT_Hex2AsciiStrLowHalfFirst(AT_CMD_MAX_LEN,
                                                        (TAF_INT8 *)pgucAtSndCodeAddr,
                                                        (TAF_UINT8 *)pgucAtSndCodeAddr + usLength,
                                                        (TAF_UINT8 *)pstEvent->PIHEvent.aucIccidContent,
                                                        USIMM_ICCID_FILE_LEN);

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      (VOS_CHAR *)pgucAtSndCodeAddr+ usLength,
                                      "%s",
                                      gaucAtCrLf);

    return usLength;
}


TAF_UINT16 At_SimHotPlugStatusInd(
    TAF_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent
)
{
    VOS_UINT16                          usLength = 0;

    usLength += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (TAF_CHAR *)pgucAtSndCodeAddr,
                                       (TAF_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "%s^SIMHOTPLUG: %d%s",
                                       gaucAtCrLf,
                                       pstEvent->PIHEvent.ulSimHotPlugStatus,
                                       gaucAtCrLf);
    return usLength;
}


TAF_UINT16 At_PrintPrivateCglaResult(
    TAF_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent)
{
    VOS_UINT16                          usLength = 0;

    /* 非最后一条打印上报IND，需要在行首添加回车换行 */
    if (VOS_TRUE != pstEvent->PIHEvent.stCglaHandleCnf.ucLastDataFlag)
    {
        usLength += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN, (TAF_CHAR *)pgucAtSndCodeAddr, (TAF_CHAR *)pgucAtSndCodeAddr, "\r\n");
    }

    /* ^CGLA: <flag>,<length>,"[<command>]<SW1><SW2>" */
    usLength += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (TAF_CHAR *)pgucAtSndCodeAddr,
                                       (TAF_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "%s: %d,%d,\"",
                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                       pstEvent->PIHEvent.stCglaHandleCnf.ucLastDataFlag,
                                       (pstEvent->PIHEvent.stCglaHandleCnf.usLen + 2) * 2);

    if(pstEvent->PIHEvent.stCglaHandleCnf.usLen != 0)
    {
        /* <command>, */
        usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr + usLength, pstEvent->PIHEvent.stCglaHandleCnf.aucCommand, pstEvent->PIHEvent.stCglaHandleCnf.usLen);
    }

    /* <SW1> */
    usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr + usLength, &pstEvent->PIHEvent.stCglaHandleCnf.ucSW1, (VOS_UINT16)sizeof(TAF_UINT8));

    /* <SW2> */
    usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN, (TAF_INT8 *)pgucAtSndCodeAddr, (TAF_UINT8 *)pgucAtSndCodeAddr + usLength, &pstEvent->PIHEvent.stCglaHandleCnf.ucSW2, (VOS_UINT16)sizeof(TAF_UINT8));
    usLength += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN, (TAF_CHAR *)pgucAtSndCodeAddr, (TAF_CHAR *)pgucAtSndCodeAddr + usLength, "\"");

    return usLength;
}


TAF_UINT16 At_SWCheckStatusInd(
    SI_PIH_EVENT_INFO_STRU             *pstEvent
)
{
    VOS_UINT16                          usLength = 0;

    usLength += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (TAF_CHAR *)pgucAtSndCodeAddr,
                                       (TAF_CHAR *)pgucAtSndCodeAddr,
                                       "%s^SWCHECK: %d%s",
                                       gaucAtCrLf,
                                       pstEvent->PIHEvent.ulApduSWCheckResult,
                                       gaucAtCrLf);
    return usLength;
}


