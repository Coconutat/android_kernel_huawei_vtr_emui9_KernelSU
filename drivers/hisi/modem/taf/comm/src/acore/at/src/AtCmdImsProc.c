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
#include "AtCmdImsProc.h"
#include "ATCmdProc.h"

#include "AtDataProc.h"
#include "AtDeviceCmd.h"


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID                 PS_FILE_ID_AT_CMD_IMS_PROC_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/*AT与IMSA模块间消息处理函数指针*/
const AT_IMSA_MSG_PRO_FUNC_STRU g_astAtImsaMsgTab[]=
{
    /* 消息ID */                            /* 消息处理函数 */
    {ID_IMSA_AT_CIREG_SET_CNF,              AT_RcvImsaCiregSetCnf},
    {ID_IMSA_AT_CIREG_QRY_CNF,              AT_RcvImsaCiregQryCnf},
    {ID_IMSA_AT_CIREP_SET_CNF,              AT_RcvImsaCirepSetCnf},
    {ID_IMSA_AT_CIREP_QRY_CNF,              AT_RcvImsaCirepQryCnf},
    {ID_IMSA_AT_VOLTEIMPU_QRY_CNF,          AT_RcvImsaImpuSetCnf},
    {ID_IMSA_AT_CIREGU_IND,                 AT_RcvImsaCireguInd},
    {ID_IMSA_AT_CIREPH_IND,                 AT_RcvImsaCirephInd},
    {ID_IMSA_AT_CIREPI_IND,                 AT_RcvImsaCirepiInd},

    /* CONFIG_VOWIFI_NEW_FRW宏打开时不需要上报vt的状态 */
    {ID_IMSA_AT_MT_STATES_IND,              AT_RcvImsaMtStateInd},
    {ID_IMSA_AT_IMS_CTRL_MSG,               AT_RcvImsaImsCtrlMsg},
    {ID_IMSA_AT_IMS_REG_DOMAIN_QRY_CNF,     AT_RcvImsaImsRegDomainQryCnf},
    {ID_IMSA_AT_CALL_ENCRYPT_SET_CNF,       AT_RcvImsaCallEncryptSetCnf},
    {ID_IMSA_AT_ROAMING_IMS_QRY_CNF,        AT_RcvImsaRoamImsServiceQryCnf},

    {ID_IMSA_AT_IMS_RAT_HANDOVER_IND,       AT_RcvImsaRatHandoverInd},
    {ID_IMSA_AT_IMS_SRV_STATUS_UPDATE_IND,  AT_RcvSrvStatusUpdateInd},

    {ID_IMSA_AT_PCSCF_SET_CNF,              AT_RcvImsaPcscfSetCnf},
    {ID_IMSA_AT_PCSCF_QRY_CNF,              AT_RcvImsaPcscfQryCnf},

    {ID_IMSA_AT_DMDYN_SET_CNF,              AT_RcvImsaDmDynSetCnf},
    {ID_IMSA_AT_DMDYN_QRY_CNF,              AT_RcvImsaDmDynQryCnf},

    {ID_IMSA_AT_DMCN_IND,                   AT_RcvImsaDmcnInd},

    {ID_IMSA_AT_IMSTIMER_QRY_CNF,           AT_RcvImsaImsTimerQryCnf},
    {ID_IMSA_AT_IMSTIMER_SET_CNF,           AT_RcvImsaImsTimerSetCnf},
    {ID_IMSA_AT_SMSPSI_SET_CNF,             AT_RcvImsaImsPsiSetCnf},
    {ID_IMSA_AT_SMSPSI_QRY_CNF,             AT_RcvImsaImsPsiQryCnf},
    {ID_IMSA_AT_DMUSER_QRY_CNF,             AT_RcvImsaDmUserQryCnf},

    {ID_IMSA_AT_NICKNAME_SET_CNF,           AT_RcvImsaNickNameSetCnf},
    {ID_IMSA_AT_NICKNAME_QRY_CNF,           AT_RcvImsaNickNameQryCnf},
    {ID_IMSA_AT_REGFAIL_IND,                AT_RcvImsaImsRegFailInd},
    {ID_IMSA_AT_BATTERYINFO_SET_CNF,        AT_RcvImsaBatteryInfoSetCnf},

    {ID_IMSA_AT_CANCEL_ADD_VIDEO_CNF,       AT_RcvImsaImsVideoCallCancelSetCnf},

    {ID_IMSA_AT_VOLTEIMPI_QRY_CNF,          AT_RcvImsaVolteImpiQryCnf},
    {ID_IMSA_AT_VOLTEDOMAIN_QRY_CNF,        AT_RcvImsaVolteDomainQryCnf},
    {ID_IMSA_AT_REGERR_REPORT_SET_CNF,      AT_RcvImsaRegErrRptSetCnf},
    {ID_IMSA_AT_REGERR_REPORT_QRY_CNF,      AT_RcvImsaRegErrRptQryCnf},
    {ID_IMSA_AT_REGERR_REPORT_IND,          AT_RcvImsaRegErrRptInd},

    {ID_IMSA_AT_IMS_IP_CAP_SET_CNF,         AT_RcvImsaImsIpCapSetCnf},
    {ID_IMSA_AT_IMS_IP_CAP_QRY_CNF,         AT_RcvImsaImsIpCapQryCnf},

    {ID_IMSA_AT_EMC_PDN_ACTIVATE_IND,       AT_RcvImsaEmcPdnActivateInd},
    {ID_IMSA_AT_EMC_PDN_DEACTIVATE_IND,     AT_RcvImsaEmcPdnDeactivateInd},
    {ID_IMSA_AT_CALL_ALT_SRV_IND,           AT_RcvImsaCallAltSrvInd},

    {ID_IMSA_AT_USER_AGENT_CFG_SET_CNF,     AT_RcvImsaUserAgentSetCnf},

};


/*****************************************************************************
  3 函数实现
*****************************************************************************/


VOS_VOID AT_ProcImsaMsg(AT_IMSA_MSG_STRU *pstMsg)
{
    VOS_UINT32                          i;
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulMsgId;
    VOS_UINT32                          ulRst;

    /*从g_astAtProcMsgFromImsaTab中获取消息个数*/
    ulMsgCnt = sizeof(g_astAtImsaMsgTab)/sizeof(AT_IMSA_MSG_PRO_FUNC_STRU);

    /*从消息包中获取MSG ID*/
    ulMsgId  = pstMsg->ulMsgId;

    /*g_astAtProcMsgFromImsaTab查表，进行消息分发*/
    for (i = 0; i < ulMsgCnt; i++)
    {
        if (g_astAtImsaMsgTab[i].ulMsgId == ulMsgId)
        {
            ulRst = g_astAtImsaMsgTab[i].pProcMsgFunc(pstMsg);

            if (VOS_ERR == ulRst)
            {
                AT_ERR_LOG("AT_ProcImsaMsg: Msg Proc Err!");
            }

            return;
        }
    }

    /*没有找到匹配的消息*/
    if (ulMsgCnt == i)
    {
        AT_ERR_LOG("AT_ProcImsaMsg: Msg Id is invalid!");
    }

    return;
}


VOS_UINT32 AT_RcvImsaCiregSetCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CIREG_SET_CNF_STRU         *pstCiregCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstCiregCnf = (IMSA_AT_CIREG_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstCiregCnf->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaCiregSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaCiregSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_CIREG_SET */
    if ( AT_CMD_CIREG_SET != gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        AT_WARN_LOG("AT_RcvImsaCiregSetCnf: WARNING:Not AT_CMD_CIREG_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if ( VOS_OK == pstCiregCnf->ulResult )
    {
        ulResult    = AT_OK;
    }
    else
    {
        ulResult    = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaCiregQryCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CIREG_QRY_CNF_STRU         *pstCiregCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;

    /* 初始化消息变量 */
    ucIndex     = 0;
    usLength    = 0;
    pstCiregCnf = (IMSA_AT_CIREG_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstCiregCnf->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaCiregQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaCiregQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_CIREG_QRY */
    if ( AT_CMD_CIREG_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        AT_WARN_LOG("AT_RcvImsaCiregQryCnf: WARNING:Not AT_CMD_CIREG_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if ( VOS_OK == pstCiregCnf->ulResult )
    {

        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           "%s: %d,",
                                           g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                           pstCiregCnf->enCireg);

        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr+usLength,
                                           "%d",
                                           pstCiregCnf->ulRegInfo);

        /* 如果IMS未注册，<ext_info>参数无意义，且不输出，详见3GPP 27007 v11 8.71 */
        if ((VOS_FALSE != pstCiregCnf->ulRegInfo) && (VOS_FALSE != pstCiregCnf->bitOpExtInfo))
        {
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                               (VOS_CHAR *)pgucAtSndCodeAddr,
                                               (VOS_CHAR *)pgucAtSndCodeAddr+usLength,
                                               ",%d",
                                               pstCiregCnf->ulExtInfo);
        }

        ulResult                = AT_OK;
    }
    else
    {
        ulResult                = AT_ERROR;
    }

    gstAtSendData.usBufLen  = usLength;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaCirepSetCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CIREP_SET_CNF_STRU         *pstCirepCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstCirepCnf = (IMSA_AT_CIREP_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstCirepCnf->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaCirepSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaCirepSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_CIREP_SET */
    if ( AT_CMD_CIREP_SET != gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        AT_WARN_LOG("AT_RcvImsaCirepSetCnf: WARNING:Not AT_CMD_CIREP_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if ( VOS_OK == pstCirepCnf->ulResult )
    {
        ulResult    = AT_OK;
    }
    else
    {
        ulResult    = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaCirepQryCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CIREP_QRY_CNF_STRU         *pstCirepCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstCirepCnf = (IMSA_AT_CIREP_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstCirepCnf->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaCirepQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaCirepQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_CIREP_QRY */
    if ( AT_CMD_CIREP_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        AT_WARN_LOG("AT_RcvImsaCirepQryCnf: WARNING:Not AT_CMD_CIREP_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if ( VOS_OK == pstCirepCnf->ulResult )
    {

        gstAtSendData.usBufLen= (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                                       "%s: %d,%d",
                                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                       pstCirepCnf->enReport,
                                                       pstCirepCnf->enImsvops);

        ulResult                = AT_OK;
    }
    else
    {
        gstAtSendData.usBufLen  = 0;
        ulResult                = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}



VOS_UINT32 AT_RcvImsaImpuSetCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_VOLTEIMPU_QRY_CNF_STRU     *pstImpuCnf;
    VOS_UINT32                          ulResult;
    VOS_CHAR                            acString[AT_IMSA_IMPU_MAX_LENGTH+1];
    VOS_UINT8                           ucIndex;
    VOS_UINT16                          usLength;

    /* 初始化消息变量 */
    ucIndex     = 0;
    usLength    = 0;
    pstImpuCnf  = (IMSA_AT_VOLTEIMPU_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstImpuCnf->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaImpuSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImpuSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMPU_SET */
    if ( AT_CMD_IMPU_SET != gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        AT_WARN_LOG("AT_RcvImsaImpuSetCnf: WARNING:Not AT_CMD_IMPU_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    TAF_MEM_SET_S(acString, sizeof(acString), 0x00, sizeof(acString));
    if(pstImpuCnf->ulImpuLen > AT_IMSA_IMPU_MAX_LENGTH)
    {
        TAF_MEM_CPY_S(acString, sizeof(acString), pstImpuCnf->aucImpu, AT_IMSA_IMPU_MAX_LENGTH);
    }
    else
    {
        TAF_MEM_CPY_S(acString, sizeof(acString), pstImpuCnf->aucImpu, pstImpuCnf->ulImpuLen);
    }

    /* 判断查询操作是否成功 */
    if ( VOS_OK == pstImpuCnf->ulResult )
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           "%s: %s",
                                           g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                           acString);

        if ( (0 != pstImpuCnf->ulImpuLenVirtual)
          && (AT_IMSA_IMPU_MAX_LENGTH > pstImpuCnf->ulImpuLenVirtual))
        {
            pstImpuCnf->aucImpuVirtual[pstImpuCnf->ulImpuLenVirtual] = '\0';
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, ",%s", pstImpuCnf->aucImpuVirtual);
        }

        gstAtSendData.usBufLen  = usLength;
        ulResult                = AT_OK;
    }
    else
    {
        gstAtSendData.usBufLen  = 0;
        ulResult                = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_SetVolteImpiPara(TAF_UINT8 ucIndex)
{
    AT_IMSA_VOLTEIMPI_QRY_REQ_STRU      stImpi;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stImpi, sizeof(stImpi), 0x00, sizeof(AT_IMSA_VOLTEIMPI_QRY_REQ_STRU));

    /* 参数检查 */
    if(AT_CMD_OPT_SET_CMD_NO_PARA != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送^VOLTEIMPI设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                     0,
                                     ID_AT_IMSA_VOLTEIMPI_QRY_REQ,
                                     stImpi.aucContent,
                                     sizeof(stImpi.aucContent),
                                     I0_PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_VOLTEIMPI_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_SetVolteDomainPara(TAF_UINT8 ucIndex)
{
    AT_IMSA_VOLTEDOMAIN_QRY_REQ_STRU    stDomain;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stDomain, sizeof(stDomain), 0x00, sizeof(AT_IMSA_VOLTEDOMAIN_QRY_REQ_STRU));

    /* 参数检查 */
    if(AT_CMD_OPT_SET_CMD_NO_PARA != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送^VOLTEDOMAIN设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                     0,
                                     ID_AT_IMSA_VOLTEDOMAIN_QRY_REQ,
                                     stDomain.aucContent,
                                     sizeof(stDomain.aucContent),
                                     I0_PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_VOLTEDOMAIN_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvImsaVolteImpiQryCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_VOLTEIMPI_QRY_CNF_STRU     *pstImpiCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;
    VOS_CHAR                            acString[AT_IMSA_IMPI_MAX_LENGTH+1];
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex     = 0;
    usLength    = 0;
    pstImpiCnf  = (IMSA_AT_VOLTEIMPI_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstImpiCnf->usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaVolteImpiSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaVolteImpiSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMPI_SET */
    if (AT_CMD_VOLTEIMPI_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaVolteImpiSetCnf: WARNING:Not AT_CMD_IMPI_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    TAF_MEM_SET_S(acString, sizeof(acString), 0x00, sizeof(acString));
    if (pstImpiCnf->ulImpiLen > AT_IMSA_IMPI_MAX_LENGTH)
    {
        TAF_MEM_CPY_S(acString, sizeof(acString), pstImpiCnf->aucImpi, AT_IMSA_IMPI_MAX_LENGTH);
    }
    else
    {
        TAF_MEM_CPY_S(acString, sizeof(acString), pstImpiCnf->aucImpi, pstImpiCnf->ulImpiLen);
    }

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstImpiCnf->ulResult)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           "%s: %s",
                                           g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                           acString);

        gstAtSendData.usBufLen  = usLength;
        ulResult                = AT_OK;
    }
    else
    {
        gstAtSendData.usBufLen  = 0;
        ulResult                = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaVolteDomainQryCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_VOLTEDOMAIN_QRY_CNF_STRU   *pstDomainCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;
    VOS_CHAR                            acString[AT_IMSA_DOMAIN_MAX_LENGTH+1];
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex       = 0;
    usLength      = 0;
    pstDomainCnf  = (IMSA_AT_VOLTEDOMAIN_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstDomainCnf->usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaVolteDomainSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaVolteDomainSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_VOLTEDOMAIN_SET */
    if (AT_CMD_VOLTEDOMAIN_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaVolteDomainSetCnf: WARNING:Not AT_CMD_IMPI_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    TAF_MEM_SET_S(acString, sizeof(acString), 0x00, sizeof(acString));
    if (pstDomainCnf->ulDomainLen > AT_IMSA_DOMAIN_MAX_LENGTH)
    {
        TAF_MEM_CPY_S(acString, sizeof(acString), pstDomainCnf->aucDomain, AT_IMSA_DOMAIN_MAX_LENGTH);
    }
    else
    {
        TAF_MEM_CPY_S(acString, sizeof(acString), pstDomainCnf->aucDomain, pstDomainCnf->ulDomainLen);
    }

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstDomainCnf->ulResult)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           "%s: %s",
                                           g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                           acString);

        gstAtSendData.usBufLen  = usLength;
        ulResult                = AT_OK;
    }
    else
    {
        gstAtSendData.usBufLen  = 0;
        ulResult                = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaCirephInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CIREPH_IND_STRU            *pstCirephInd;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex      = 0;
    pstCirephInd = (IMSA_AT_CIREPH_IND_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstCirephInd->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaCirephInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /* 判断查询操作是否成功 */
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s%s: %d%s",
                                                    gaucAtCrLf,
                                                    gastAtStringTab[AT_STRING_CIREPH].pucText,
                                                    pstCirephInd->enHandover,
                                                    gaucAtCrLf);

    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaCirepiInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CIREPI_IND_STRU            *pstCirepiInd;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex      = 0;
    pstCirepiInd = (IMSA_AT_CIREPI_IND_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstCirepiInd->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaCirepiInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /* 判断查询操作是否成功 */
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s%s: %d%s",
                                                    gaucAtCrLf,
                                                    gastAtStringTab[AT_STRING_CIREPI].pucText,
                                                    pstCirepiInd->enImsvops,
                                                    gaucAtCrLf);

    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaCireguInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CIREGU_IND_STRU            *pstCireguInd;
    VOS_UINT8                           ucIndex;
    VOS_UINT16                          usLength;

    /* 初始化消息变量 */
    ucIndex      = 0;
    usLength     = 0;
    pstCireguInd = (IMSA_AT_CIREGU_IND_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstCireguInd->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaCireguInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /* 判断查询操作是否成功 */
    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       "%s%s: %d",
                                       gaucAtCrLf,
                                       gastAtStringTab[AT_STRING_CIREGU].pucText,
                                       pstCireguInd->ulRegInfo);

    /* 如果IMS未注册，<ext_info>参数无意义 */
    if ((VOS_FALSE != pstCireguInd->ulRegInfo) && (VOS_FALSE != pstCireguInd->bitOpExtInfo))
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                           ",%d",
                                           pstCireguInd->ulExtInfo);
    }

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "%s",
                                       gaucAtCrLf);

    gstAtSendData.usBufLen = usLength;
    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}




VOS_VOID AT_RcvImsaVtIpv4PdpActInd(
    VOS_UINT8                           ucIndex,
    IMSA_AT_VT_PDP_ACTIVATE_IND_STRU   *pstPdpActInd
)
{
    VOS_UINT32                          ulIPAddr;                               /* IP 地址，网侧分配*/
    VOS_UINT32                          ulSubNetMask;                           /* 子网掩码地址，根据IP地址计算*/
    VOS_UINT32                          ulGateWay;                              /* 网关地址，也是本DHCP Server的地址*/
    VOS_UINT32                          ulPrimDNS;                              /* 主 DNS地址，网侧分配*/
    VOS_UINT32                          ulSecDNS;                               /* 次 DNS地址，网侧分配*/

    ulIPAddr     = AT_GetLanAddr32(pstPdpActInd->stPdpAddr.aucIpv4Addr);
    ulSubNetMask = AT_DHCPGetIPMask(ulIPAddr);
    ulGateWay    = AT_DHCPGetGateWay(ulIPAddr, ulSubNetMask);
    ulPrimDNS    = 0;
    ulSecDNS     = 0;

    if (IMSA_AT_IMS_RAT_TYPE_LTE != pstPdpActInd->enRatType)
    {
        AT_WARN_LOG("AT_RcvImsaVtIpv4PdpActInd: WARNING: enRatType is not LTE");
        return;
    }

    if (pstPdpActInd->stIpv4Dns.bitOpPrimDnsAddr)
    {
        ulPrimDNS = AT_GetLanAddr32(pstPdpActInd->stIpv4Dns.aucPrimDnsAddr);
    }

    if (pstPdpActInd->stIpv4Dns.bitOpSecDnsAddr)
    {
        ulSecDNS = AT_GetLanAddr32(pstPdpActInd->stIpv4Dns.aucSecDnsAddr);
    }

    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s^VTDCONNV4:%08X,%08X,%08X,%08X,%08X,%08X,%d%s",
                                                    gaucAtCrLf,
                                                    VOS_HTONL(ulIPAddr),
                                                    VOS_HTONL(ulSubNetMask),
                                                    VOS_HTONL(ulGateWay),
                                                    VOS_HTONL(ulGateWay),
                                                    VOS_HTONL(ulPrimDNS),
                                                    VOS_HTONL(ulSecDNS),
                                                    pstPdpActInd->enRatType,
                                                    gaucAtCrLf);

    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);
}


VOS_VOID AT_RcvImsaVtIpv6PdpActInd(
    VOS_UINT8                           ucIndex,
    IMSA_AT_VT_PDP_ACTIVATE_IND_STRU   *pstPdpActInd
)
{
    VOS_UINT16                          usLength;
    VOS_UINT8                           aucIpv6AddrStr[TAF_MAX_IPV6_ADDR_COLON_STR_LEN];
    VOS_UINT8                           aucInvalidIpv6Addr[TAF_IPV6_ADDR_LEN];

    usLength     = 0;
    TAF_MEM_SET_S(aucIpv6AddrStr, sizeof(aucIpv6AddrStr), 0x00, (TAF_MAX_IPV6_ADDR_COLON_STR_LEN));
    TAF_MEM_SET_S(aucInvalidIpv6Addr, sizeof(aucInvalidIpv6Addr), 0x00, TAF_IPV6_ADDR_LEN);

    if (IMSA_AT_IMS_RAT_TYPE_LTE != pstPdpActInd->enRatType)
    {
        AT_WARN_LOG("AT_RcvImsaVtIpv6PdpActInd: WARNING: enRatType is not LTE");
        return;
    }

    usLength  += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                           (VOS_CHAR*)pgucAtSndCodeAddr + usLength, "%s^VTDCONNV6:", gaucAtCrLf);

    /* 填写IPV6地址 */
    AT_ConvertIpv6AddrToCompressedStr(aucIpv6AddrStr,
                                      pstPdpActInd->stPdpAddr.aucIpv6Addr,
                                      TAF_IPV6_STR_RFC2373_TOKENS);

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, "%s", aucIpv6AddrStr);

    /* 填写IPV6掩码, 该字段填全0 */
    AT_ConvertIpv6AddrToCompressedStr(aucIpv6AddrStr,
                                      aucInvalidIpv6Addr,
                                      TAF_IPV6_STR_RFC2373_TOKENS);
    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, ",%s", aucIpv6AddrStr);

    /* 填写IPV6网关, 该字段填全0 */
    AT_ConvertIpv6AddrToCompressedStr(aucIpv6AddrStr,
                                      aucInvalidIpv6Addr,
                                      TAF_IPV6_STR_RFC2373_TOKENS);
    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, ",%s", aucIpv6AddrStr);

    /* 填写DHCP IPV6, 该字段填全0 */
    AT_ConvertIpv6AddrToCompressedStr(aucIpv6AddrStr,
                                      aucInvalidIpv6Addr,
                                      TAF_IPV6_STR_RFC2373_TOKENS);
    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, ",%s", aucIpv6AddrStr);

    /* 填写IPV6 Primary DNS */
    AT_ConvertIpv6AddrToCompressedStr(aucIpv6AddrStr,
                                      pstPdpActInd->stIpv6Dns.aucPrimDnsAddr,
                                      TAF_IPV6_STR_RFC2373_TOKENS);
    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, ",%s", aucIpv6AddrStr);

    /* 填写IPV6 Secondary DNS */
    AT_ConvertIpv6AddrToCompressedStr(aucIpv6AddrStr,
                                      pstPdpActInd->stIpv6Dns.aucSecDnsAddr,
                                      TAF_IPV6_STR_RFC2373_TOKENS);
    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, ",%s", aucIpv6AddrStr);

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, ",%d", pstPdpActInd->enRatType);

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                          (VOS_CHAR*)pgucAtSndCodeAddr + usLength, "%s", gaucAtCrLf);


    gstAtSendData.usBufLen = usLength;

    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

}


VOS_VOID AT_RcvImsaVtIpv4v6PdpActInd(
    VOS_UINT8                           ucIndex,
    IMSA_AT_VT_PDP_ACTIVATE_IND_STRU   *pstPdpActInd
)
{
    AT_RcvImsaVtIpv4PdpActInd(ucIndex, pstPdpActInd);
    AT_RcvImsaVtIpv6PdpActInd(ucIndex, pstPdpActInd);
}


VOS_UINT32 AT_RcvImsaVtPdpActInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_VT_PDP_ACTIVATE_IND_STRU   *pstPdpActInd;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstPdpActInd = (IMSA_AT_VT_PDP_ACTIVATE_IND_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstPdpActInd->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaVtPdpActInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    switch(pstPdpActInd->stPdpAddr.enPdpType)
    {
        case TAF_PDP_IPV4:
            AT_RcvImsaVtIpv4PdpActInd(ucIndex, pstPdpActInd);
            break;
        case TAF_PDP_IPV6:
            AT_RcvImsaVtIpv6PdpActInd(ucIndex, pstPdpActInd);
            break;
        case TAF_PDP_IPV4V6:
            AT_RcvImsaVtIpv4v6PdpActInd(ucIndex, pstPdpActInd);
            break;
        default:
            return VOS_ERR;
    }

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaVtPdpDeactInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_VT_PDP_DEACTIVATE_IND_STRU *pstPdpDeactInd;
    VOS_UINT8                           ucIndex;
    VOS_UINT16                          usLength;

    /* 初始化消息变量 */
    usLength       = 0;
    ucIndex        = 0;
    pstPdpDeactInd = (IMSA_AT_VT_PDP_DEACTIVATE_IND_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstPdpDeactInd->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaVtPdpDeactInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (IMSA_AT_IMS_RAT_TYPE_LTE != pstPdpDeactInd->enRatType)
    {
        AT_WARN_LOG("AT_RcvImsaVtPdpDeactInd: WARNING: enRatType is not LTE");
        return VOS_ERR;
    }

    switch(pstPdpDeactInd->enPdpType)
    {
        case TAF_PDP_IPV4:
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                               (VOS_CHAR *)pgucAtSndCodeAddr,
                                               (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                               "%s^VTDENDV4:%d%s",
                                               gaucAtCrLf,
                                               pstPdpDeactInd->enRatType,
                                               gaucAtCrLf);
            break;
        case TAF_PDP_IPV6:
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                            (VOS_CHAR *)pgucAtSndCodeAddr,
                                                            (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                                            "%s^VTDENDV6:%d%s",
                                                            gaucAtCrLf,
                                                            pstPdpDeactInd->enRatType,
                                                            gaucAtCrLf);
            break;
        case TAF_PDP_IPV4V6:
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                            (VOS_CHAR *)pgucAtSndCodeAddr,
                                                            (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                                            "%s^VTDENDV4:%d%s",
                                                            gaucAtCrLf,
                                                            pstPdpDeactInd->enRatType,
                                                            gaucAtCrLf);

            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                            (VOS_CHAR *)pgucAtSndCodeAddr,
                                                            (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                                            "%s^VTDENDV6:%d%s",
                                                            gaucAtCrLf,
                                                            pstPdpDeactInd->enRatType,
                                                            gaucAtCrLf);

            break;
        default:
            return VOS_ERR;
    }

    gstAtSendData.usBufLen = usLength;

    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;

}


VOS_VOID AT_ReportImsEmcStatResult(
    VOS_UINT8                           ucIndex,
    AT_PDP_STATUS_ENUM_UINT32           ulStatus
)
{
    VOS_UINT16                          usLength = 0;

    usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                      "%s^IMSEMCSTAT:%d%s",
                                      gaucAtCrLf,
                                      ulStatus,
                                      gaucAtCrLf);

    At_SendResultData(ucIndex, pgucAtSndCodeAddr, usLength);
    return;
}


VOS_UINT32 AT_RcvImsaEmcPdnActivateInd(VOS_VOID *pMsg)
{
    IMSA_AT_EMC_PDN_ACTIVATE_IND_STRU  *pstPdnActivateInd = VOS_NULL_PTR;
    AT_IMS_EMC_RDP_STRU                *pstImsEmcRdp = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex = 0;

    pstPdnActivateInd = (IMSA_AT_EMC_PDN_ACTIVATE_IND_STRU *)pMsg;

    /* 转换ClientId */
    if (AT_FAILURE == At_ClientIdToUserId(pstPdnActivateInd->usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaEmcPdnActivateInd: ClientId is invalid.");
        return VOS_ERR;
    }

    /* 获取IMS EMC RDP */
    pstImsEmcRdp = AT_GetImsEmcRdpByClientId(ucIndex);
    if (VOS_NULL_PTR == pstImsEmcRdp)
    {
        AT_WARN_LOG("AT_RcvImsaEmcPdnActivateInd: ImsEmcRdp not found.");
        return VOS_ERR;
    }

    /* 清除IMS EMC信息 */
    TAF_MEM_SET_S(pstImsEmcRdp, sizeof(AT_IMS_EMC_RDP_STRU), 0x00, sizeof(AT_IMS_EMC_RDP_STRU));

    /* 更新IPv4 PDN信息 */
    if ( (TAF_PDP_IPV4   == pstPdnActivateInd->stPdpAddr.enPdpType)
      || (TAF_PDP_IPV4V6 == pstPdnActivateInd->stPdpAddr.enPdpType) )
    {
        pstImsEmcRdp->bitOpIPv4PdnInfo = VOS_TRUE;

        TAF_MEM_CPY_S(pstImsEmcRdp->stIPv4PdnInfo.aucIpAddr,
                      sizeof(pstImsEmcRdp->stIPv4PdnInfo.aucIpAddr),
                      pstPdnActivateInd->stPdpAddr.aucIpv4Addr,
                      sizeof(pstPdnActivateInd->stPdpAddr.aucIpv4Addr));

        if (VOS_TRUE == pstPdnActivateInd->stIpv4Dns.bitOpPrimDnsAddr)
        {
            TAF_MEM_CPY_S(pstImsEmcRdp->stIPv4PdnInfo.aucDnsPrimAddr,
                          sizeof(pstImsEmcRdp->stIPv4PdnInfo.aucDnsPrimAddr),
                          pstPdnActivateInd->stIpv4Dns.aucPrimDnsAddr,
                          sizeof(pstPdnActivateInd->stIpv4Dns.aucPrimDnsAddr));
        }

        if (VOS_TRUE == pstPdnActivateInd->stIpv4Dns.bitOpSecDnsAddr)
        {
            TAF_MEM_CPY_S(pstImsEmcRdp->stIPv4PdnInfo.aucDnsSecAddr,
                          sizeof(pstImsEmcRdp->stIPv4PdnInfo.aucDnsSecAddr),
                          pstPdnActivateInd->stIpv4Dns.aucSecDnsAddr,
                          sizeof(pstPdnActivateInd->stIpv4Dns.aucSecDnsAddr));
        }

        pstImsEmcRdp->stIPv4PdnInfo.usMtu = pstPdnActivateInd->usMtu;
    }

    /* 更新IPv6 PDN信息 */
    if ( (TAF_PDP_IPV6   == pstPdnActivateInd->stPdpAddr.enPdpType)
      || (TAF_PDP_IPV4V6 == pstPdnActivateInd->stPdpAddr.enPdpType) )
    {
        pstImsEmcRdp->bitOpIPv6PdnInfo = VOS_TRUE;

        TAF_MEM_CPY_S(pstImsEmcRdp->stIPv6PdnInfo.aucIpAddr,
                      sizeof(pstImsEmcRdp->stIPv6PdnInfo.aucIpAddr),
                      pstPdnActivateInd->stPdpAddr.aucIpv6Addr,
                      sizeof(pstPdnActivateInd->stPdpAddr.aucIpv6Addr));

        if (VOS_TRUE == pstPdnActivateInd->stIpv6Dns.bitOpPrimDnsAddr)
        {
            TAF_MEM_CPY_S(pstImsEmcRdp->stIPv6PdnInfo.aucDnsPrimAddr,
                          sizeof(pstImsEmcRdp->stIPv6PdnInfo.aucDnsPrimAddr),
                          pstPdnActivateInd->stIpv6Dns.aucPrimDnsAddr,
                          sizeof(pstPdnActivateInd->stIpv6Dns.aucPrimDnsAddr));
        }

        if (VOS_TRUE == pstPdnActivateInd->stIpv6Dns.bitOpSecDnsAddr)
        {
            TAF_MEM_CPY_S(pstImsEmcRdp->stIPv6PdnInfo.aucDnsSecAddr,
                          sizeof(pstImsEmcRdp->stIPv6PdnInfo.aucDnsSecAddr),
                          pstPdnActivateInd->stIpv6Dns.aucSecDnsAddr,
                          sizeof(pstPdnActivateInd->stIpv6Dns.aucSecDnsAddr));
        }

        pstImsEmcRdp->stIPv6PdnInfo.usMtu = pstPdnActivateInd->usMtu;
    }

    /* 上报连接状态 */
    AT_ReportImsEmcStatResult(ucIndex, AT_PDP_STATUS_ACT);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaEmcPdnDeactivateInd(VOS_VOID *pMsg)
{
    IMSA_AT_EMC_PDN_DEACTIVATE_IND_STRU    *pstPdnDeactivateInd = VOS_NULL_PTR;
    AT_IMS_EMC_RDP_STRU                    *pstImsEmcRdp = VOS_NULL_PTR;
    VOS_UINT8                               ucIndex = 0;

    pstPdnDeactivateInd = (IMSA_AT_EMC_PDN_DEACTIVATE_IND_STRU *)pMsg;

    /* 检查ClientId */
    if (AT_FAILURE == At_ClientIdToUserId(pstPdnDeactivateInd->usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaEmcPdnDeactivateInd: ClientId is invalid.");
        return VOS_ERR;
    }

    /* 获取IMS EMC上下文 */
    pstImsEmcRdp = AT_GetImsEmcRdpByClientId(ucIndex);
    if (VOS_NULL_PTR == pstImsEmcRdp)
    {
        AT_WARN_LOG("AT_RcvImsaEmcPdnDeactivateInd: ImsEmcRdp not found.");
        return VOS_ERR;
    }

    /* 检查IMS EMC状态 */
    if ( (VOS_TRUE != pstImsEmcRdp->bitOpIPv4PdnInfo)
      && (VOS_TRUE != pstImsEmcRdp->bitOpIPv6PdnInfo) )
    {
        AT_WARN_LOG("AT_RcvImsaEmcPdnDeactivateInd: IMS EMC PDN not active.");
        return VOS_ERR;
    }

    /* 清除IMS EMC信息 */
    TAF_MEM_SET_S(pstImsEmcRdp, sizeof(AT_IMS_EMC_RDP_STRU), 0x00, sizeof(AT_IMS_EMC_RDP_STRU));

    /* 上报连接状态 */
    AT_ReportImsEmcStatResult(ucIndex, AT_PDP_STATUS_DEACT);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaMtStateInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_MT_STATES_IND_STRU          *pstMtStatusInd;
    VOS_UINT8                           ucIndex;
    VOS_CHAR                            acString[AT_IMSA_CALL_ASCII_NUM_MAX_LENGTH + 1];

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstMtStatusInd  = (IMSA_AT_MT_STATES_IND_STRU*)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstMtStatusInd->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaImpuSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    TAF_MEM_SET_S(acString, sizeof(acString), 0x00, sizeof(acString));
    TAF_MEM_CPY_S(acString, sizeof(acString), pstMtStatusInd->aucAsciiCallNum, AT_IMSA_CALL_ASCII_NUM_MAX_LENGTH);

    gstAtSendData.usBufLen= (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   "%s^IMSMTRPT: %s,%d,%d%s",
                                                   gaucAtCrLf,
                                                   acString,
                                                   pstMtStatusInd->ucMtStatus,
                                                   pstMtStatusInd->ulCauseCode,
                                                   gaucAtCrLf);
    /* 调用At_SendResultData发送命令结果 */
    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}

VOS_UINT32 AT_RcvImsaImsRegDomainQryCnf(VOS_VOID *pMsg)
{
    /* 局部变量 */
    IMSA_AT_IMS_REG_DOMAIN_QRY_CNF_STRU     *pstImsRegDomainCnf = VOS_NULL_PTR;
    VOS_UINT8                                ucIndex;
    VOS_UINT32                               ulResult;

    /* 初始化变量 */
    ucIndex             = 0;
    ulResult            = 0;
    pstImsRegDomainCnf  = (IMSA_AT_IMS_REG_DOMAIN_QRY_CNF_STRU *)pMsg;

    /* 获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsRegDomainCnf->usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsRegDomainQryCnf: WARNING: AT INDEX NOT FOUND1");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsRegDomainQryCnf: WARNING: AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作是否为AT_CMD_IMSREGDOMAIN_QRY */
    if (AT_CMD_IMSREGDOMAIN_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaImsRegDomainQryCnf: WARNING: Not AT_CMD_IMSREGDOMAIN_QRY!");
        return VOS_ERR;
    }

    /* 复位状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断返回值是否合法 */
    if (IMSA_AT_IMS_REG_DOMAIN_TYPE_BUTT <= (pstImsRegDomainCnf->enImsRegDomain))
    {
        ulResult = AT_ERROR;
    }
    else
    {
       gstAtSendData.usBufLen += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                                       "%s: %d",
                                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                       pstImsRegDomainCnf->enImsRegDomain);
        ulResult= AT_OK;
    }

    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaImsCtrlMsg(VOS_VOID *pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_IMS_CTRL_MSG_STRU                              *pstImsCtrlMsgInd;
    AT_IMS_CTRL_MSG_RECEIVE_MODULE_ENUM_UINT8               enModule;
    VOS_UINT16                                              usLength = 0;
    VOS_UINT8                                               ucIndex;

    /* 初始化消息变量 */
    enModule          = AT_IMS_CTRL_MSG_RECEIVE_MODULE_MAPCON;
    ucIndex           = 0;
    pstImsCtrlMsgInd  = (IMSA_AT_IMS_CTRL_MSG_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsCtrlMsgInd->usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsCtrlMsg: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "%s^IMSCTRLMSGU: %d,%d,\"",
                                       gaucAtCrLf,
                                       enModule,
                                       pstImsCtrlMsgInd->ulWifiMsgLen);

    usLength += (TAF_UINT16)At_HexAlpha2AsciiString(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_UINT8 *)pgucAtSndCodeAddr + usLength,
                                                    pstImsCtrlMsgInd->aucWifiMsg,
                                                    (VOS_UINT16)pstImsCtrlMsgInd->ulWifiMsgLen);

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "\"%s",
                                       gaucAtCrLf);

    /* 调用At_SendResultData发送命令结果 */
    At_SendResultData(ucIndex, pgucAtSndCodeAddr, usLength);

    return VOS_OK;
}



VOS_UINT32 AT_RcvImsaCallEncryptSetCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CALL_ENCRYPT_SET_CNF_STRU  *pstCallEncryptCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 初始化消息变量 */
    ucIndex           = 0;
    pstCallEncryptCnf = (IMSA_AT_CALL_ENCRYPT_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstCallEncryptCnf->usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaCallEncryptSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaCallEncryptSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_CALLENCRYPT_SET */
    if (AT_CMD_CALLENCRYPT_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaCallEncryptSetCnf: WARNING:Not AT_CMD_CALLENCRYPT_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstCallEncryptCnf->ulResult)
    {
        ulResult    = AT_OK;
    }
    else
    {
        ulResult    = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaRoamImsServiceQryCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_ROAMING_IMS_QRY_CNF_STRU   *pstRoamImsServiceCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;

    /* 初始化消息变量 */
    ucIndex              = 0;
    usLength             = 0;
    pstRoamImsServiceCnf = (IMSA_AT_ROAMING_IMS_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstRoamImsServiceCnf->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaRoamImsServiceQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaRoamImsServiceQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_ROAM_IMS_QRY */
    if ( AT_CMD_ROAM_IMS_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        AT_WARN_LOG("AT_RcvImsaRoamImsServiceQryCnf: WARNING:Not AT_CMD_ROAM_IMS_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if ( VOS_OK == pstRoamImsServiceCnf->ulResult )
    {

        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           "%s: %d",
                                           g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                           ((pstRoamImsServiceCnf->enRoamingImsSupportFlag == AT_IMSA_ROAMING_IMS_SUPPORT) ? 1 : 0));

        ulResult  = AT_OK;
    }
    else
    {
        ulResult  = AT_ERROR;
    }

    gstAtSendData.usBufLen  = usLength;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}



VOS_UINT32 AT_RcvImsaRatHandoverInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_IMS_RAT_HANDOVER_IND_STRU  *pstHandoverInd;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstHandoverInd  = (IMSA_AT_IMS_RAT_HANDOVER_IND_STRU*)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstHandoverInd->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvImsaRatHandoverInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    gstAtSendData.usBufLen= (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   "%s^IMSRATHO: %d,%d,%d,%d%s",
                                                   gaucAtCrLf,
                                                   pstHandoverInd->enHoStatus,
                                                   pstHandoverInd->enSrcRat,
                                                   pstHandoverInd->enDstRat,
                                                   pstHandoverInd->enCause,
                                                   gaucAtCrLf);
    /* 调用At_SendResultData发送命令结果 */
    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}


VOS_UINT32 AT_RcvSrvStatusUpdateInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_IMS_SRV_STATUS_UPDATE_IND_STRU *pstSrvUpdateInd;
    VOS_UINT8                               ucIndex;

    /* 初始化消息变量 */
    ucIndex             = 0;
    pstSrvUpdateInd     = (IMSA_AT_IMS_SRV_STATUS_UPDATE_IND_STRU*)pMsg;

    /* 通过ClientId获取ucIndex */
    if ( AT_FAILURE == At_ClientIdToUserId(pstSrvUpdateInd->usClientId, &ucIndex) )
    {
        AT_WARN_LOG("AT_RcvSrvStatusUpdateInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    gstAtSendData.usBufLen= (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   "%s^IMSSRVSTATUS: %d,%d,%d,%d,%d,%d,%d,%d%s",
                                                   gaucAtCrLf,
                                                   pstSrvUpdateInd->enSmsSrvStatus,
                                                   pstSrvUpdateInd->enSmsSrvRat,
                                                   pstSrvUpdateInd->enVoIpSrvStatus,
                                                   pstSrvUpdateInd->enVoIpSrvRat,
                                                   pstSrvUpdateInd->enVtSrvStatus,
                                                   pstSrvUpdateInd->enVtSrvRat,
                                                   pstSrvUpdateInd->enVsSrvStatus,
                                                   pstSrvUpdateInd->enVsSrvRat,
                                                   gaucAtCrLf);
    /* 调用At_SendResultData发送命令结果 */
    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}


VOS_VOID At_FillDmdynNumParaInCmd (
    VOS_UINT16                         *pusLength,
    VOS_UINT32                          ulValue,
    VOS_UINT32                          ulValueValidFlg
)
{
    /* 数据有效 */
    if (VOS_TRUE == ulValueValidFlg)
    {
        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR*)pgucAtSndCodeAddr,
                                           (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                           "%d,",
                                           ulValue
                                           );
    }
    else
    {
        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR*)pgucAtSndCodeAddr,
                                           (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                           ","
                                           );
    }

    return;
}


VOS_VOID At_FillDmdynStrParaInCmd (
    VOS_UINT16                         *pusLength,
    VOS_CHAR                           *pcValue,
    VOS_UINT32                          ulValueValidFlg,
    VOS_UINT32                          ulLastParaFlg
)
{
    /* 数据有效 */
    if(VOS_TRUE == ulValueValidFlg)
    {
        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR*)pgucAtSndCodeAddr,
                                           (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                           "\"%s\"",
                                           pcValue
                                           );
    }

    /* 不是最后一个参数需要在后面添加逗号 */
    if (VOS_FALSE == ulLastParaFlg)
    {
        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR*)pgucAtSndCodeAddr,
                                           (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                           ","
                                           );
    }

    return;
}


VOS_VOID At_FillIpv6AddrInCmd (
    VOS_UINT16                         *pusLength,
    VOS_UINT8                          *pucAddr,
    VOS_UINT32                          ulAddrValidFlg,
    VOS_UINT32                          ulSipPort,
    VOS_UINT32                          ulPortValidFlg
)
{
    VOS_UINT8                           aucIPv6Str[TAF_MAX_IPV6_ADDR_DOT_STR_LEN];

    TAF_MEM_SET_S(aucIPv6Str, TAF_MAX_IPV6_ADDR_DOT_STR_LEN, 0, TAF_MAX_IPV6_ADDR_DOT_STR_LEN);

    /* IPV6地址有效 */
    if (VOS_TRUE == ulAddrValidFlg)
    {
        /* 将IPV6地址从num转换为str */
        AT_Ipv6AddrToStr(aucIPv6Str, pucAddr, AT_IPV6_STR_TYPE_HEX);

        /* 是否存在端口号 */
        if (VOS_TRUE == ulPortValidFlg)
        {
            *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                 (VOS_CHAR*)pgucAtSndCodeAddr,
                                                 (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                                 ",\"[%s]:%u\"",
                                                 aucIPv6Str,
                                                 ulSipPort
                                                 );
        }
        else
        {
            *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                 (VOS_CHAR*)pgucAtSndCodeAddr,
                                                 (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                                 ",\"%s\"",
                                                 aucIPv6Str
                                                 );
        }
    }
    /* IPV6地址无效 */
    else
    {
        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                             (VOS_CHAR*)pgucAtSndCodeAddr,
                                             (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                             ","
                                             );
    }

    return;
}


VOS_VOID At_FillIpv4AddrInCmd (
    VOS_UINT16                         *pusLength,
    VOS_UINT8                          *pucAddr,
    VOS_UINT32                          ulAddrValidFlg,
    VOS_UINT32                          ulSipPort,
    VOS_UINT32                          ulPortValidFlg
)
{
    VOS_CHAR                           aucIPv4Str[TAF_MAX_IPV4_ADDR_STR_LEN + 1];

    TAF_MEM_SET_S(aucIPv4Str, TAF_MAX_IPV4_ADDR_STR_LEN + 1, 0, TAF_MAX_IPV4_ADDR_STR_LEN + 1);

    /* IPV4地址有效 */
    if (VOS_TRUE == ulAddrValidFlg)
    {
        /* 将IPV4地址从num转换为str */
        AT_PcscfIpv4Addr2Str(aucIPv4Str, pucAddr);

        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                             (VOS_CHAR*)pgucAtSndCodeAddr,
                                             (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                             ",\"%s",
                                             aucIPv4Str
                                             );

        /* 是否存在端口号 */
        if (VOS_TRUE == ulPortValidFlg)
        {
            *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                 (VOS_CHAR*)pgucAtSndCodeAddr,
                                                 (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                                 ":%u\"",
                                                 ulSipPort
                                                 );
        }
        else
        {
            *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                 (VOS_CHAR*)pgucAtSndCodeAddr,
                                                 (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                                 "\""
                                                 );
        }
    }
    /* IPV4地址无效 */
    else
    {
        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                             (VOS_CHAR*)pgucAtSndCodeAddr,
                                             (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                             ","
                                             );
    }

    return;
}


VOS_UINT32 AT_RcvImsaPcscfSetCnf(
    VOS_VOID                           *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_PCSCF_SET_CNF_STRU         *pstPcscfCnf = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstPcscfCnf = (IMSA_AT_PCSCF_SET_CNF_STRU*)pMsg;

    AT_INFO_LOG("AT_RcvImsaPcscfSetCnf entered!");

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstPcscfCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmPcscfSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmPcscfSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_PCSCF_SET */
    if (AT_CMD_PCSCF_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_ERR_LOG("AT_RcvImsaDmPcscfSetCnf: WARNING:Not AT_CMD_PCSCF_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断设置操作是否成功 */
    if (VOS_OK == pstPcscfCnf->ulResult)
    {
        ulResult = AT_OK;
    }
    else
    {
        ulResult = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaPcscfQryCnf(
    VOS_VOID                           *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_PCSCF_QRY_CNF_STRU         *pstPcscfCnf = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;

    /* 初始化变量 */
    pstPcscfCnf = (IMSA_AT_PCSCF_QRY_CNF_STRU *)pMsg;
    ucIndex     = 0;
    usLength    = 0;
    ulResult    = AT_OK;

    AT_INFO_LOG("AT_RcvImsaPcscfQryCnf entered!");

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstPcscfCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaPcscfQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaPcscfQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_PCSCF_QRY */
    if (AT_CMD_PCSCF_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_ERR_LOG("AT_RcvImsaPcscfQryCnf: WARNING:Not AT_CMD_PCSCF_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstPcscfCnf->ulResult)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           "%s: %d",
                                           g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                           pstPcscfCnf->stAtPcscf.enSrc
                                           );

        /* <PrimIpv6Pcscf> */
        At_FillIpv6AddrInCmd(&usLength,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.aucPrimPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.bitOpPrimPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.ulPrimPcscfSipPort,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.bitOpPrimPcscfSipPort
                             );

        /* <SecIpv6Pcscf> */
        At_FillIpv6AddrInCmd(&usLength,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.aucSecPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.bitOpSecPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.ulSecPcscfSipPort,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.bitOpSecPcscfSipPort
                             );

        /* <ThiIpv6Pcscf> */
        At_FillIpv6AddrInCmd(&usLength,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.aucThiPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.bitOpThiPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.ulThiPcscfSipPort,
                             pstPcscfCnf->stAtPcscf.stIpv6Pcscf.bitOpThiPcscfSipPort
                             );

        /* <PrimIpv4Pcscf> */
        At_FillIpv4AddrInCmd(&usLength,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.aucPrimPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.bitOpPrimPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.ulPrimPcscfSipPort,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.bitOpPrimPcscfSipPort
                             );

        /* <SecIpv4Pcscf> */
        At_FillIpv4AddrInCmd(&usLength,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.aucSecPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.bitOpSecPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.ulSecPcscfSipPort,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.bitOpSecPcscfSipPort
                             );

        /* <ThiIpv4Pcscf> */
        At_FillIpv4AddrInCmd(&usLength,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.aucThiPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.bitOpThiPcscfAddr,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.ulThiPcscfSipPort,
                             pstPcscfCnf->stAtPcscf.stIpv4Pcscf.bitOpThiPcscfSipPort
                             );

        gstAtSendData.usBufLen = usLength;

    }
    /* 消息携带结果不是OK时，直接返回ERROR */
    else
    {
        gstAtSendData.usBufLen  = 0;
        ulResult                = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaDmDynSetCnf(
    VOS_VOID                           *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_DMDYN_SET_CNF_STRU         *pstDmdynCnf = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstDmdynCnf = (IMSA_AT_DMDYN_SET_CNF_STRU*)pMsg;

    AT_INFO_LOG("AT_RcvImsaDmDynSetCnf entered!");

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstDmdynCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmDynSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmDynSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_DMDYN_SET */
    if (AT_CMD_DMDYN_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_ERR_LOG("AT_RcvImsaDmDynSetCnf: WARNING:Not AT_CMD_DMDYN_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断设置操作是否成功 */
    if (VOS_OK == pstDmdynCnf->ulResult)
    {
        ulResult = AT_OK;
    }
    else
    {
        ulResult = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}



VOS_UINT32 AT_RcvImsaDmDynQryCnf(
    VOS_VOID                           *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_DMDYN_QRY_CNF_STRU         *pstDmDynCnf = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;

    /* 初始化消息变量 */
    ucIndex     = 0;
    usLength    = 0;
    pstDmDynCnf = (IMSA_AT_DMDYN_QRY_CNF_STRU*)pMsg;

    AT_INFO_LOG("AT_RcvImsaDmDynQryCnf entered!");

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstDmDynCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmDynQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmDynQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_DMDYN_QRY */
    if (AT_CMD_DMDYN_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_ERR_LOG("AT_RcvImsaDmDynQryCnf: WARNING:Not AT_CMD_DMDYN_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstDmDynCnf->ulResult)
    {

        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           "%s: ",
                                           g_stParseContext[ucIndex].pstCmdElement->pszCmdName
                                           );

        /* <ulAmrWbOctetAcigned> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulAmrWbOctetAcigned, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpAmrWbOctetAcigned));

        /* <ulAmrWbBandWidthEfficient> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulAmrWbBandWidthEfficient, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpAmrWbBandWidthEfficient));

        /* <ulAmrOctetAcigned> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulAmrOctetAcigned, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpAmrOctetAcigned));

        /* <ulAmrBandWidthEfficient> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulAmrBandWidthEfficient, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpAmrBandWidthEfficient));

        /* <ulAmrWbMode> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulAmrWbMode, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpAmrWbMode));

        /* <ulDtmfWb> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulDtmfWb, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpDtmfWb));

        /* <ulDtmfNb> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulDtmfNb, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpDtmfNb));

        /* <ulSpeechStart> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulSpeechStart, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpSpeechStart));

        /* <ulSpeechEnd> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulSpeechEnd, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpSpeechEnd));

        /* <ulSpeechStart> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulVideoStart, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpVideoStart));

        /* <ulSpeechEnd> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulVideoEnd, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpVideoEnd));

        /* <ulRetryBaseTime> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulRetryBaseTime, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpRetryBaseTime));

        /* <ulRetryMaxTime> */
        At_FillDmdynNumParaInCmd(&usLength, pstDmDynCnf->stDmdyn.ulRetryMaxTime, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpRetryMaxTime));

        /* <acPhoneContext> */
        At_FillDmdynStrParaInCmd(&usLength, pstDmDynCnf->stDmdyn.acPhoneContext, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpPhoneContext), VOS_FALSE);

        /* <acPhoneContextImpu> */
        At_FillDmdynStrParaInCmd(&usLength, pstDmDynCnf->stDmdyn.acPhoneContextImpu, (VOS_UINT32)(pstDmDynCnf->stDmdyn.bitOpPhoneContextImpu), VOS_TRUE);

        gstAtSendData.usBufLen = usLength;

        ulResult = AT_OK;
    }
    /* 消息携带结果不是OK时，直接返回ERROR */
    else
    {
        gstAtSendData.usBufLen = 0;
        ulResult               = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaDmcnInd(
    VOS_VOID                           *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_DMCN_IND_STRU              *pstDmcnInd = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstDmcnInd  = (IMSA_AT_DMCN_IND_STRU*)pMsg;

    AT_INFO_LOG("AT_RcvImsaDmcnInd entered!");

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstDmcnInd->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmcnInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /* 构造AT主动上报^DMCN */
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s%s%s",
                                                    gaucAtCrLf,
                                                    gastAtStringTab[AT_STRING_DMCN].pucText,
                                                    gaucAtCrLf);

    /* 调用At_SendResultData发送命令结果 */
    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}


VOS_UINT32 AT_ParseIpv6PcscfData(
    VOS_UINT32                         *pulAddrExistFlg,
    VOS_UINT8                          *pucIpAddr,
    VOS_UINT32                         *pulPortExistFlg,
    VOS_UINT32                         *pulPortNum,
    VOS_UINT32                          ulIndex
)
{
    /* IPV6地址长度超过限制 */
    if ((AT_PARA_MAX_LEN + 1) < gastAtParaList[ulIndex].usParaLen)
    {
        *pulPortExistFlg = VOS_FALSE;
        *pulAddrExistFlg = VOS_FALSE;
        AT_ERR_LOG("AT_FillIpv6PcscfData: PCSCF IPV6 address length OUT OF RANGE");
        return VOS_ERR;
    }

    /* AT参数为空，代表地址不存在 */
    if (0 == gastAtParaList[ulIndex].usParaLen)
    {
        AT_NORM_LOG("AT_ParseIpv6PcscfData: PCSCF IPV6 address is NULL");
        *pulPortExistFlg = VOS_FALSE;
        *pulAddrExistFlg = VOS_FALSE;
        return VOS_OK;
    }

    /* 解析IPV6地址和端口号 */
    if (VOS_OK != AT_Ipv6PcscfDataToAddr(gastAtParaList[ulIndex].aucPara, pucIpAddr, pulPortExistFlg, pulPortNum))
    {
        *pulAddrExistFlg = VOS_FALSE;
        AT_ERR_LOG("AT_ParseIpv6PcscfData: PCSCF IPV6 address decode ERROR");
        return VOS_ERR;
    }
    else
    {
        AT_NORM_LOG("AT_ParseIpv6PcscfData: PCSCF IPV6 address decode SUCC");
        *pulAddrExistFlg = VOS_TRUE;
        return VOS_OK;
    }
}


VOS_UINT32 AT_FillIpv6PcscfData(
    AT_IMSA_PCSCF_SET_REQ_STRU         *pstPcscf
)
{
    VOS_UINT32                          ulResult;
    VOS_UINT32                          ulIndex;
    VOS_UINT32                          ulAddrExistsFlg;
    VOS_UINT32                          ulPortExistsFlg;

    /* 解析AT，组装发给IMSA的消息
       ^ IMSPCSCF =<Source>,
                   <IPV6Address1>,
                   <IPV6Address2>，
                   <IPV6Address3>,
                   <IPV4Address1>,
                   <IPV4Address2>,
                   <IPV4Address3>
    */

    /* 解析<IPV6Address1>，将索引设置为AT命令的第2个参数 */
    ulIndex = 1;
    ulAddrExistsFlg = VOS_FALSE;
    ulPortExistsFlg = VOS_FALSE;

    ulResult = AT_ParseIpv6PcscfData(&ulAddrExistsFlg,
                                     pstPcscf->stAtPcscf.stIpv6Pcscf.aucPrimPcscfAddr,
                                     &ulPortExistsFlg,
                                     &(pstPcscf->stAtPcscf.stIpv6Pcscf.ulPrimPcscfSipPort),
                                     ulIndex);

    pstPcscf->stAtPcscf.stIpv6Pcscf.bitOpPrimPcscfAddr    = ulAddrExistsFlg;
    pstPcscf->stAtPcscf.stIpv6Pcscf.bitOpPrimPcscfSipPort = ulPortExistsFlg;

    /* 解码错误直接返回 */
    if (VOS_OK != ulResult)
    {
        AT_ERR_LOG("AT_FillIpv6PcscfData: Primary PCSCF IPV6 address decode ERROR");
        return ulResult;
    }

    /* 解析<IPV6Address2>,将索引设置为AT命令的第3个参数 */
    ulIndex = 2;
    ulAddrExistsFlg = VOS_FALSE;
    ulPortExistsFlg = VOS_FALSE;

    ulResult = AT_ParseIpv6PcscfData(&ulAddrExistsFlg,
                                     pstPcscf->stAtPcscf.stIpv6Pcscf.aucSecPcscfAddr,
                                     &ulPortExistsFlg,
                                     &(pstPcscf->stAtPcscf.stIpv6Pcscf.ulSecPcscfSipPort),
                                     ulIndex);

    pstPcscf->stAtPcscf.stIpv6Pcscf.bitOpSecPcscfAddr    = ulAddrExistsFlg;
    pstPcscf->stAtPcscf.stIpv6Pcscf.bitOpSecPcscfSipPort = ulPortExistsFlg;

    /* 解码错误直接返回 */
    if (VOS_OK != ulResult)
    {
        AT_ERR_LOG("AT_FillIpv6PcscfData: Secondary PCSCF IPV6 address decode ERROR");
        return ulResult;
    }

    /* 解析<IPV6Address3>,将索引设置为AT命令的第4个参数 */
    ulIndex = 3;
    ulAddrExistsFlg = VOS_FALSE;
    ulPortExistsFlg = VOS_FALSE;

    ulResult = AT_ParseIpv6PcscfData(&ulAddrExistsFlg,
                                      pstPcscf->stAtPcscf.stIpv6Pcscf.aucThiPcscfAddr,
                                      &ulPortExistsFlg,
                                      &(pstPcscf->stAtPcscf.stIpv6Pcscf.ulThiPcscfSipPort),
                                      ulIndex);

    pstPcscf->stAtPcscf.stIpv6Pcscf.bitOpThiPcscfAddr    = ulAddrExistsFlg;
    pstPcscf->stAtPcscf.stIpv6Pcscf.bitOpThiPcscfSipPort = ulPortExistsFlg;

    /* 解码错误直接返回 */
    if (VOS_OK != ulResult)
    {
        AT_ERR_LOG("AT_FillIpv6PcscfData: Third PCSCF IPV6 address decode ERROR");
        return ulResult;
    }

    return ulResult;
}


VOS_UINT32 AT_ParseIpv4PcscfData(
    VOS_UINT32                         *pulAddrExistFlg,
    VOS_UINT8                          *pucIpAddr,
    VOS_UINT32                         *pulPortExistFlg,
    VOS_UINT32                         *pulPortNum,
    VOS_UINT32                          ulIndex
)
{
    VOS_CHAR                           *pcPortStr = VOS_NULL_PTR;
    VOS_UINT8                           aucTmpIpAddr[AT_PARA_MAX_LEN + 1];
    VOS_UINT32                          ulStrLen = 0;

    TAF_MEM_SET_S(aucTmpIpAddr, AT_PARA_MAX_LEN + 1, 0, AT_PARA_MAX_LEN + 1);

    /* IPV4地址长度超过限制 */
    if ((AT_PARA_MAX_LEN + 1) < gastAtParaList[ulIndex].usParaLen)
    {
        AT_ERR_LOG("AT_ParseIpv4PcscfData: PCSCF IPV4 address length OUT OF RANGE");
        return VOS_ERR;
    }

    /* 参数为空，代表此地址不存在 */
    if (0 == gastAtParaList[ulIndex].usParaLen)
    {
        AT_NORM_LOG("AT_ParseIpv4PcscfData: PCSCF IPV4 address is NULL");
        *pulPortExistFlg = VOS_FALSE;
        *pulAddrExistFlg = VOS_FALSE;
        return VOS_OK;
    }

    pcPortStr = VOS_StrStr((VOS_CHAR *)(gastAtParaList[ulIndex].aucPara), ":");

    /* 检查是否有端口号 */
    if (VOS_NULL_PTR != pcPortStr)
    {
        if (VOS_OK == AT_PortAtoI(pcPortStr + 1, pulPortNum))
        {
            AT_NORM_LOG("AT_ParseIpv4PcscfData: PCSCF IPV4 port num decode SUCC");
            *pulPortExistFlg = VOS_TRUE;
            ulStrLen = (VOS_UINT32)(pcPortStr - (VOS_CHAR*)(gastAtParaList[ulIndex].aucPara));
        }
        /* 解析端口号失败 */
        else
        {
            *pulPortExistFlg = VOS_FALSE;
            AT_ERR_LOG("AT_ParseIpv4PcscfData: PCSCF IPV4 port num decode ERROR");
            return VOS_ERR;
        }
    }
    else
    {
        AT_NORM_LOG("AT_ParseIpv4PcscfData: No port in PCSCF IPV4 addr");
        *pulPortExistFlg = VOS_FALSE;
        ulStrLen = VOS_StrLen((VOS_CHAR*)(gastAtParaList[ulIndex].aucPara));
    }

    TAF_MEM_CPY_S(aucTmpIpAddr, AT_PARA_MAX_LEN + 1, gastAtParaList[ulIndex].aucPara, AT_MIN(ulStrLen, AT_PARA_MAX_LEN + 1));

    /* 解析IPV4地址 */
    if (VOS_OK != AT_Ipv4AddrAtoi((VOS_CHAR*)aucTmpIpAddr, pucIpAddr))
    {

        *pulAddrExistFlg = VOS_FALSE;
        AT_ERR_LOG("AT_ParseIpv4PcscfData: PCSCF IPV4 address decode ERROR");
        return VOS_ERR;
    }
    else
    {
        AT_NORM_LOG("AT_ParseIpv4PcscfData: PCSCF IPV4 address decode SUCC");
        *pulAddrExistFlg = VOS_TRUE;
    }


    return VOS_OK;
}


VOS_UINT32 AT_FillIpv4PcscfData(
    AT_IMSA_PCSCF_SET_REQ_STRU         *pstPcscf
)
{
    VOS_UINT32                          ulResult;
    VOS_UINT32                          ulIndex;
    VOS_UINT32                          ulAddrExistsFlg;
    VOS_UINT32                          ulPortExistsFlg;


    /* 解析AT，组装发给IMSA的消息
       ^ IMSPCSCF =<Source>,
                   <IPV6Address1>,
                   <IPV6Address2>，
                   <IPV6Address3>,
                   <IPV4Address1>,
                   <IPV4Address2>,
                   <IPV4Address3>
    */

    /* 解析Primary IPV4，将索引设置为AT命令的第5个参数 */
    ulIndex         = 4;
    ulAddrExistsFlg = VOS_FALSE;
    ulPortExistsFlg = VOS_FALSE;

    ulResult = AT_ParseIpv4PcscfData(&ulAddrExistsFlg,
                                     pstPcscf->stAtPcscf.stIpv4Pcscf.aucPrimPcscfAddr,
                                     &ulPortExistsFlg,
                                     &(pstPcscf->stAtPcscf.stIpv4Pcscf.ulPrimPcscfSipPort),
                                     ulIndex);

    pstPcscf->stAtPcscf.stIpv4Pcscf.bitOpPrimPcscfAddr    = ulAddrExistsFlg;
    pstPcscf->stAtPcscf.stIpv4Pcscf.bitOpPrimPcscfSipPort = ulPortExistsFlg;

    /* 解码错误直接返回 */
    if (VOS_OK != ulResult)
    {
        AT_ERR_LOG("AT_FillIpv4PcscfData: Primary PCSCF IPV4 address Decode ERROR");
        return ulResult;
    }

    /* 解析Secondary IPV4，将索引设置为AT命令的第6个参数 */
    ulIndex = 5;
    ulAddrExistsFlg = VOS_FALSE;
    ulPortExistsFlg = VOS_FALSE;

    ulResult = AT_ParseIpv4PcscfData(&ulAddrExistsFlg,
                                     pstPcscf->stAtPcscf.stIpv4Pcscf.aucSecPcscfAddr,
                                     &ulPortExistsFlg,
                                     &(pstPcscf->stAtPcscf.stIpv4Pcscf.ulSecPcscfSipPort),
                                     ulIndex);

    pstPcscf->stAtPcscf.stIpv4Pcscf.bitOpSecPcscfAddr    = ulAddrExistsFlg;
    pstPcscf->stAtPcscf.stIpv4Pcscf.bitOpSecPcscfSipPort = ulPortExistsFlg;

    /* 解码错误直接返回 */
    if (VOS_OK != ulResult)
    {
        AT_ERR_LOG("AT_FillIpv4PcscfData: Secondary PCSCF IPV4 address Decode ERROR");
        return ulResult;
    }

    /* 解析Third IPV4，将索引设置为AT命令的第7个参数 */
    ulIndex = 6;
    ulAddrExistsFlg = VOS_FALSE;
    ulPortExistsFlg = VOS_FALSE;

    ulResult = AT_ParseIpv4PcscfData(&ulAddrExistsFlg,
                                     pstPcscf->stAtPcscf.stIpv4Pcscf.aucThiPcscfAddr,
                                     &ulPortExistsFlg,
                                     &(pstPcscf->stAtPcscf.stIpv4Pcscf.ulThiPcscfSipPort),
                                     ulIndex);

    pstPcscf->stAtPcscf.stIpv4Pcscf.bitOpThiPcscfAddr    = ulAddrExistsFlg;
    pstPcscf->stAtPcscf.stIpv4Pcscf.bitOpThiPcscfSipPort = ulPortExistsFlg;

    /* 解码错误直接返回 */
    if (VOS_OK != ulResult)
    {
        AT_ERR_LOG("AT_FillIpv4PcscfData: Third PCSCF IPV4 address Decode ERROR");
        return ulResult;
    }

    return ulResult;
}


VOS_UINT32 AT_FillDataToPcscf(
    AT_IMSA_PCSCF_SET_REQ_STRU         *pstPcscf
)
{
    VOS_UINT32                          ulResult;

    if (VOS_NULL_PTR == pstPcscf)
    {
        AT_ERR_LOG("AT_FillDataToPcscf: pstPcscf is NULL, return ERROR");
        return VOS_ERR;
    }

    /* 没有填写<Source>或者超出范围 */
    if ((0 == gastAtParaList[0].usParaLen)
     || (IMSA_AT_PCSCF_SRC_BUTT <= gastAtParaList[0].ulParaValue))
    {
        AT_ERR_LOG("AT_FillDataToPcscf: No <source> parameter or out of range, return ERROR");
        return VOS_ERR;
    }

    /* <Source> */
    pstPcscf->stAtPcscf.enSrc = gastAtParaList[0].ulParaValue;

    ulResult = AT_FillIpv6PcscfData(pstPcscf);

    /* IPV6和IPV4都解析成功才返回OK */
    if ((VOS_OK == AT_FillIpv4PcscfData(pstPcscf))
     && (VOS_OK == ulResult))
    {
        return VOS_OK;
    }

    return VOS_ERR;
}



VOS_UINT32 AT_SetImsPcscfPara(VOS_UINT8 ucIndex)
{
    AT_IMSA_PCSCF_SET_REQ_STRU          stPcscf;
    VOS_UINT32                          ulResult;

    AT_INFO_LOG("AT_SetImsPcscfPara Entered");

    TAF_MEM_SET_S(&stPcscf, sizeof(AT_IMSA_PCSCF_SET_REQ_STRU), 0x00, sizeof(AT_IMSA_PCSCF_SET_REQ_STRU));

    /* 不是设置命令则出错 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_ERR_LOG("AT_SetImsPcscfPara: NOT SET CMD, return ERROR");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 解析AT，组装发给IMSA的消息
        ^IMSPCSCF =<Source>,
                   <IPV6Address1>,
                   <IPV6Address2>，
                   <IPV6Address3>,
                   <IPV4Address1>,
                   <IPV4Address2>,
                   <IPV4Address3>
    */

    /* ^IMSPCSCF携带7个参数，否则出错*/
    if (7 != gucAtParaIndex)
    {
        AT_ERR_LOG("AT_SetImsPcscfPara: Para number incorrect, return ERROR");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 组装发给IMSA的消息 */
    if (VOS_OK != AT_FillDataToPcscf(&stPcscf))
    {
        AT_ERR_LOG("AT_SetImsPcscfPara: AT_FillDataToPcscf FAIL, return ERROR");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      gastAtClientTab[ucIndex].opId,
                                      ID_AT_IMSA_PCSCF_SET_REQ,
                                      &stPcscf.stAtPcscf,
                                      (VOS_UINT32)sizeof(IMSA_AT_PCSCF_STRU),
                                      PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_ERR_LOG("AT_SetImsPcscfPara: AT_FillAndSndAppReqMsg FAIL");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_PCSCF_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 At_FillImsPayloadTypePara (
    VOS_UINT32                         *pulValue,
    VOS_UINT32                         *pulValueValidFlg,
    VOS_UINT32                          ulIndex
)
{
    /* 初始化 */
    *pulValueValidFlg = VOS_FALSE;

    /* IMS要求PAYLOAD TYPE的参数范围只能使用动态解码范围0X60-0X7F */
    if (0 != gastAtParaList[ulIndex].usParaLen)
    {
        if ((AT_IMS_PAYLOAD_TYPE_RANGE_MIN > gastAtParaList[ulIndex].ulParaValue)
         || (AT_IMS_PAYLOAD_TYPE_RANGE_MAX < gastAtParaList[ulIndex].ulParaValue))
        {
            AT_ERR_LOG("At_FillImsPayloadTypePara: IMS Payload type para OUT OF RANGE");
            return VOS_ERR;
        }

        *pulValue    = gastAtParaList[ulIndex].ulParaValue;
        *pulValueValidFlg = VOS_TRUE;
    }
    else
    {
        AT_NORM_LOG("At_FillImsPayloadTypePara: IMS Payload type para length is 0");
        *pulValueValidFlg = VOS_FALSE;
    }

    return VOS_OK;
}


VOS_UINT32 At_FillImsAmrWbModePara (
    VOS_UINT32                         *pulValue,
    VOS_UINT32                         *pulValueValidFlg,
    VOS_UINT32                          ulIndex
)
{
    /* 初始化 */
    *pulValueValidFlg = VOS_FALSE;

    /* IMS要求AMR WB MODE的参数取值范围为0-8 */
    if (0 != gastAtParaList[ulIndex].usParaLen)
    {
        if (AT_IMS_AMR_WB_MODE_MAX < gastAtParaList[ulIndex].ulParaValue)
        {
            AT_ERR_LOG("At_FillImsPayloadTypePara: IMS AMR WB MODE OUT OF RANGE");
            return VOS_ERR;
        }

        *pulValue    = gastAtParaList[ulIndex].ulParaValue;
        *pulValueValidFlg = VOS_TRUE;
    }
    else
    {
        AT_NORM_LOG("At_FillImsPayloadTypePara: IMS AMR WB MODE para length is 0");
        *pulValueValidFlg = VOS_FALSE;
    }

    return VOS_OK;
}


VOS_UINT32 At_FillImsRtpPortPara (
    VOS_UINT32                         *pulValue,
    VOS_UINT32                         *pulValueValidFlg,
    VOS_UINT32                          ulIndex
)
{
    /* 初始化 */
    *pulValueValidFlg = VOS_FALSE;

    /* IMS要求RTP PORT的参数只能使用偶数且不能为0 */
    if (0 != gastAtParaList[ulIndex].usParaLen)
    {
        if ((0 != (gastAtParaList[ulIndex].ulParaValue)%2)
         || (0 == gastAtParaList[ulIndex].ulParaValue))
        {
            AT_ERR_LOG("At_FillImsPayloadTypePara: IMS RTP Port para incorrect");
            return VOS_ERR;
        }

        *pulValue    = gastAtParaList[ulIndex].ulParaValue;
        *pulValueValidFlg = VOS_TRUE;
    }
    else
    {
        AT_NORM_LOG("At_FillImsPayloadTypePara: IMS RTP Port para length is 0");
        *pulValueValidFlg = VOS_FALSE;
    }

    return VOS_OK;
}


VOS_UINT32 At_FillImsaNumericPara (
    VOS_UINT32                         *pulValue,
    VOS_UINT32                         *pulValueValidFlg,
    VOS_UINT32                          ulIndex
)
{
    /* 初始化 */
    *pulValueValidFlg = VOS_FALSE;

    if (0 != gastAtParaList[ulIndex].usParaLen)
    {
        *pulValue    = gastAtParaList[ulIndex].ulParaValue;
        *pulValueValidFlg = VOS_TRUE;
    }
    else
    {
        AT_NORM_LOG("At_FillImsaNumericPara: IMSA numeric para length is 0");
        *pulValueValidFlg = VOS_FALSE;
    }

    return VOS_OK;
}


VOS_UINT32 At_FillImsaStrPara (
    VOS_CHAR                           *pucStr,
    VOS_UINT32                         *pulStrValidFlg,
    VOS_UINT32                          ulMaxLen,
    VOS_UINT32                          ulIndex
)
{
    /* 初始化 */
    *pulStrValidFlg = VOS_FALSE;

    if (0 != gastAtParaList[ulIndex].usParaLen)
    {
        if (ulMaxLen < gastAtParaList[13].usParaLen)
        {
            AT_ERR_LOG("At_FillImsPayloadTypePara: IMS string para out of range");
            return VOS_ERR;
        }

        TAF_MEM_CPY_S(pucStr , ulMaxLen, (VOS_CHAR*)gastAtParaList[ulIndex].aucPara, AT_MIN(ulMaxLen, gastAtParaList[ulIndex].usParaLen));
        *pulStrValidFlg = VOS_TRUE;
    }
    else
    {
        AT_NORM_LOG("At_FillImsaNumericPara: IMSA string para length is 0");
        *pulStrValidFlg = VOS_FALSE;
    }

    return VOS_OK;
}


VOS_UINT32 AT_SetDmDynPara(TAF_UINT8 ucIndex)
{
    AT_IMSA_DMDYN_SET_REQ_STRU          stDmdynSetReq;
    VOS_UINT32                          ulResult;
    VOS_UINT32                          ulTmpResult;
    VOS_UINT32                          ulValueValidFlg;

    AT_INFO_LOG("AT_SetDmDynPara Entered");

    TAF_MEM_SET_S(&stDmdynSetReq, sizeof(AT_IMSA_DMDYN_SET_REQ_STRU), 0x00, sizeof(AT_IMSA_DMDYN_SET_REQ_STRU));
    ulResult    = AT_SUCCESS;

    /* 不是设置命令则出错 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_ERR_LOG("AT_SetDmDynPara: NOT SET CMD, return ERROR");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* ^DMDYN携带15个参数，否则出错*/
    if (15 != gucAtParaIndex)
    {
        AT_ERR_LOG("AT_SetDmDynPara: Para number incorrect, return ERROR");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 解析AT，组装发给IMSA的消息
    ^DMDYN=<AMR_WB_octet_aligned>,
           <AMR_WB_bandwidth_efficient>,
           <AMR_octet_aligned>,
           <AMR_bandwidth_efficient>,
           <AMR_WB_mode>,
           <DTMF_WB>,
           <DTMF_NB>,
           <Speech_start>,
           <Speech_end>,
           <Video_start>,
           <Video_end>,
           <RegRetryBaseTime>,
           <RegRetryMaxTime>,
           <PhoneContext>,
           <Public_user_identity>
    */

    /* <AMR_WB_octet_aligned> */
    ulTmpResult     = At_FillImsPayloadTypePara(&(stDmdynSetReq.stDmdyn.ulAmrWbOctetAcigned), &ulValueValidFlg, 0);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpAmrWbOctetAcigned = ulValueValidFlg;

    /* <AMR_WB_bandwidth_efficient> */
    ulTmpResult     = At_FillImsPayloadTypePara(&(stDmdynSetReq.stDmdyn.ulAmrWbBandWidthEfficient), &ulValueValidFlg, 1);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpAmrWbBandWidthEfficient = ulValueValidFlg;

    /* <AMR_octet_aligned> */
    ulTmpResult     = At_FillImsPayloadTypePara(&(stDmdynSetReq.stDmdyn.ulAmrOctetAcigned), &ulValueValidFlg, 2);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpAmrOctetAcigned = ulValueValidFlg;

    /* <AMR_bandwidth_efficient> */
    ulTmpResult     = At_FillImsPayloadTypePara(&(stDmdynSetReq.stDmdyn.ulAmrBandWidthEfficient), &ulValueValidFlg, 3);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpAmrBandWidthEfficient = ulValueValidFlg;

    /* <AMR_WB_mode> */
    ulTmpResult     = At_FillImsAmrWbModePara(&(stDmdynSetReq.stDmdyn.ulAmrWbMode), &ulValueValidFlg, 4);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpAmrWbMode = ulValueValidFlg;

    /* <DTMF_WB> */
    ulTmpResult     = At_FillImsPayloadTypePara(&(stDmdynSetReq.stDmdyn.ulDtmfWb), &ulValueValidFlg, 5);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpDtmfWb = ulValueValidFlg;

    /* <DTMF_NB> */
    ulTmpResult     = At_FillImsPayloadTypePara(&(stDmdynSetReq.stDmdyn.ulDtmfNb), &ulValueValidFlg, 6);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpDtmfNb = ulValueValidFlg;

    /* <Speech_start> */
    ulTmpResult     = At_FillImsRtpPortPara(&(stDmdynSetReq.stDmdyn.ulSpeechStart), &ulValueValidFlg, 7);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpSpeechStart = ulValueValidFlg;

    /* <Speech_end> */
    ulTmpResult     = At_FillImsRtpPortPara(&(stDmdynSetReq.stDmdyn.ulSpeechEnd), &ulValueValidFlg, 8);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpSpeechEnd = ulValueValidFlg;

    /* <Video_start> */
    ulTmpResult     = At_FillImsRtpPortPara(&(stDmdynSetReq.stDmdyn.ulVideoStart), &ulValueValidFlg, 9);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpVideoStart = ulValueValidFlg;

    /* <Video_end> */
    ulTmpResult     = At_FillImsRtpPortPara(&(stDmdynSetReq.stDmdyn.ulVideoEnd), &ulValueValidFlg, 10);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpVideoEnd = ulValueValidFlg;

    /* <RegRetryBaseTime> */
    ulTmpResult     = At_FillImsaNumericPara(&(stDmdynSetReq.stDmdyn.ulRetryBaseTime), &ulValueValidFlg, 11);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpRetryBaseTime = ulValueValidFlg;

    /* <RegRetryMaxTime> */
    ulTmpResult     = At_FillImsaNumericPara(&(stDmdynSetReq.stDmdyn.ulRetryMaxTime), &ulValueValidFlg, 12);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpRetryMaxTime = ulValueValidFlg;

    /* 因IMSA要求Base Time的时间不能超过Max Time */
    if (stDmdynSetReq.stDmdyn.ulRetryBaseTime > stDmdynSetReq.stDmdyn.ulRetryMaxTime)
    {
        AT_ERR_LOG("AT_SetDmDynPara: ulRetryBaseTime is larger than ulRetryMaxTime, return ERROR");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* <PhoneContext> */
    ulTmpResult     = At_FillImsaStrPara(stDmdynSetReq.stDmdyn.acPhoneContext, &ulValueValidFlg, IMSA_PHONECONTEXT_MAX_LENGTH, 13);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpPhoneContext = ulValueValidFlg;

    /* <Public_user_identity> */
    ulTmpResult     = At_FillImsaStrPara(stDmdynSetReq.stDmdyn.acPhoneContextImpu, &ulValueValidFlg, IMSA_PUBLICEUSERID_MAX_LENGTH, 14);
    ulResult        = ulResult | ulTmpResult;

    stDmdynSetReq.stDmdyn.bitOpPhoneContextImpu = ulValueValidFlg;

    if (VOS_OK != ulResult)
    {
        AT_ERR_LOG("AT_SetDmDynPara: There have out of range para in setting command");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      gastAtClientTab[ucIndex].opId,
                                      ID_AT_IMSA_DMDYN_SET_REQ,
                                      &stDmdynSetReq.stDmdyn,
                                      (VOS_UINT32)sizeof(AT_IMSA_DMDYN_STRU),
                                      PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_ERR_LOG("AT_SetDmDynPara: AT_FillAndSndAppReqMsg FAIL");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_DMDYN_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_QryImsPcscfPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulRet;

    AT_INFO_LOG("AT_QryImsPcscfPara Entered");

    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    ulRet = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_IMSA_PCSCF_QRY_REQ,
                                   VOS_NULL_PTR,
                                   0,
                                   PS_PID_IMSA);
    if (TAF_SUCCESS != ulRet)
    {
        AT_WARN_LOG("AT_QryImsPcscfPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_PCSCF_QRY;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_QryDmDynPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulRet;

    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    ulRet = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_IMSA_DMDYN_QRY_REQ,
                                   VOS_NULL_PTR,
                                   0,
                                   PS_PID_IMSA);
    if (TAF_SUCCESS != ulRet)
    {
        AT_WARN_LOG("AT_QryDmdynPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_DMDYN_QRY;

    return AT_WAIT_ASYNC_RETURN;
}



VOS_UINT32 AT_FillImsTimerReqBitAndPara(
    VOS_UINT32                          *pulValue,
    VOS_UINT32                          *pulBitOpValue,
    VOS_UINT32                          ulIndex
)
{
    /* 初始化 */
    *pulBitOpValue = VOS_FALSE;

    /* 判断AT命令参数长度是否为0,定时器时长范围是0-128000ms */
    if (0 != gastAtParaList[ulIndex].usParaLen)
    {
        if (AT_IMS_TIMER_DATA_RANGE_MAX < gastAtParaList[ulIndex].ulParaValue)
        {
            AT_ERR_LOG("AT_FillImsTimerReqBitAndPara: IMS Payload type para OUT OF RANGE");
            return VOS_ERR;
        }

        *pulValue       = gastAtParaList[ulIndex].ulParaValue;
        *pulBitOpValue  = VOS_TRUE;
    }
    else
    {
        AT_ERR_LOG("AT_FillImsTimerReqBitAndPara: IMS Timer para length is 0");
        *pulValue       = 0;
        *pulBitOpValue  = VOS_FALSE;

    }

    return VOS_OK;

}



VOS_UINT32 AT_FillImsTimerReqData(
    AT_IMSA_IMSTIMER_SET_REQ_STRU       *pstImsTimer
)
{
    VOS_UINT32                          ulBitOpValueFlg;


    /* 初始化 */
    TAF_MEM_SET_S(pstImsTimer, sizeof(AT_IMSA_IMSTIMER_SET_REQ_STRU), 0x00, sizeof(AT_IMSA_IMSTIMER_SET_REQ_STRU));

    /* 填入AT命令参数<Timer_T1> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerT1Value),
                                         &(ulBitOpValueFlg),
                                         0))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerT1Value = ulBitOpValueFlg;

    /*<Timer_T2>*/
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerT2Value),
                                         &(ulBitOpValueFlg),
                                         1))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerT2Value = ulBitOpValueFlg;

    /* <Timer_T4> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerT4Value),
                                         &(ulBitOpValueFlg),
                                         2))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerT4Value = ulBitOpValueFlg;

    /* <Timer_TA> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTAValue),
                                         &(ulBitOpValueFlg),
                                         3))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTAValue = ulBitOpValueFlg;

    /* <Timer_TB> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTBValue),
                                         &(ulBitOpValueFlg),
                                         4))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTBValue = ulBitOpValueFlg;

    /* <Timer_TC> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTCValue),
                                         &(ulBitOpValueFlg),
                                         5))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTCValue = ulBitOpValueFlg;

    /* <Timer_TD> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTDValue),
                                         &(ulBitOpValueFlg),
                                         6))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTDValue = ulBitOpValueFlg;

    /* <Timer_TE> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTEValue),
                                         &(ulBitOpValueFlg),
                                         7))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTEValue = ulBitOpValueFlg;

    /* <Timer_TF> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTFValue),
                                         &(ulBitOpValueFlg),
                                         8))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTFValue = ulBitOpValueFlg;

    /* <Timer_TG> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTGValue),
                                         &(ulBitOpValueFlg),
                                         9))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTGValue = ulBitOpValueFlg;

    /* <Timer_TH> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTHValue),
                                         &(ulBitOpValueFlg),
                                         10))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTHValue = ulBitOpValueFlg;

    /* <Timer_TI> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTIValue),
                                         &(ulBitOpValueFlg),
                                         11))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTIValue = ulBitOpValueFlg;

    /* <Timer_TJ> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTJValue),
                                         &(ulBitOpValueFlg),
                                         12))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTJValue = ulBitOpValueFlg;

    /* <Timer_TK> */
    if (VOS_OK != AT_FillImsTimerReqBitAndPara(&(pstImsTimer->stImsTimer.ulTimerTKValue),
                                         &(ulBitOpValueFlg),
                                         13))
    {
        AT_ERR_LOG("AT_FillImsTimerReqData: IMS Timer para is invalid");
        return VOS_ERR;
    }

    pstImsTimer->stImsTimer.bitOpTimerTKValue = ulBitOpValueFlg;

    return VOS_OK;

}


VOS_VOID At_FillImsaTimerParaInCmd(
    VOS_UINT16                         *pusLength,
    VOS_UINT32                          ulValue,
    VOS_UINT32                          ulValueValidFlg,
    VOS_UINT32                          ulLastParaFlg
)
{
    /* 数据有效 */
    if (VOS_TRUE == ulValueValidFlg)
    {
        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR*)pgucAtSndCodeAddr,
                                           (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                           "%d",
                                           ulValue
                                           );
    }
    if (VOS_FALSE == ulLastParaFlg)
    {
        *pusLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR*)pgucAtSndCodeAddr,
                                           (VOS_CHAR*)pgucAtSndCodeAddr + *pusLength,
                                           ","
                                           );
    }

    return;
}


VOS_UINT32 AT_SetImsTimerPara(
    VOS_UINT8                           ucIndex
)
{
    AT_IMSA_IMSTIMER_SET_REQ_STRU       stTimer;

    VOS_UINT32                          ulResult;

    /* 不是设置命令则出错 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_ERR_LOG("AT_SetImsTimerPara():ucCmdOptType is not AT_CMD_OPT_SET_PARA_CMD");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 14个参数 */
    if (14 != gucAtParaIndex)
    {
        AT_ERR_LOG("AT_SetImsTimerPara():incorrect parameters");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 组装发给IMSA的消息 */
    if (VOS_OK != AT_FillImsTimerReqData(&stTimer))
    {
        AT_ERR_LOG("AT_SetImsTimerPara():incorrect parameters");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                     gastAtClientTab[ucIndex].opId,
                                     ID_AT_IMSA_IMSTIMER_SET_REQ,
                                     &stTimer.stImsTimer,
                                     (VOS_UINT32)sizeof(IMSA_AT_IMS_TIMER_STRU),
                                     PS_PID_IMSA);
    if (TAF_SUCCESS != ulResult)
    {
        AT_ERR_LOG("AT_SetImsTimerPara():AT_FillAndSndAppReqMsg fail");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_DMTIMER_SET;
    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_SetImsSmsPsiPara(
    VOS_UINT8                           ucIndex
)
{

    AT_IMSA_SMSPSI_SET_REQ_STRU         stSmsPsi;
    VOS_UINT32                          ulResult;

    /* 清空 */
    TAF_MEM_SET_S(&stSmsPsi, sizeof(AT_IMSA_SMSPSI_SET_REQ_STRU), 0x00, sizeof(AT_IMSA_SMSPSI_SET_REQ_STRU));

    /* 不是设置命令则出错 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_ERR_LOG("AT_SetImsSmsPsiPara():WARNING:ucCmdOptType is not AT_CMD_OPT_SET_PARA_CMD");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 判断参数个数*/
    if (1 != gucAtParaIndex)
    {
        AT_ERR_LOG("AT_SetImsSmsPsiPara: incorrect parameter, return ERROR");
        return AT_CME_INCORRECT_PARAMETERS;
    }


    if (AT_IMSA_MAX_SMSPSI_LEN < gastAtParaList[0].usParaLen)
    {
        AT_ERR_LOG("AT_SetImsSmsPsiPara: <SMS_PSI> parameter over boundary , return ERROR");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 组装发给IMSA的消息 */
    if (0 != gastAtParaList[0].usParaLen)
    {
        TAF_MEM_CPY_S(stSmsPsi.stSmsPsi.acSmsPsi, sizeof(stSmsPsi.stSmsPsi.acSmsPsi),
                      gastAtParaList[0].aucPara, gastAtParaList[0].usParaLen);
    }

    /* 给IMSA发送设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                     gastAtClientTab[ucIndex].opId,
                                     ID_AT_IMSA_SMSPSI_SET_REQ,
                                     &stSmsPsi.stSmsPsi,
                                     (VOS_UINT32)sizeof(IMSA_SMS_PSI_STRU),
                                     PS_PID_IMSA);
    if (TAF_SUCCESS != ulResult)
    {
        AT_ERR_LOG("AT_SetImsSmsPsiPara():WARNING:AT_FillAndSndAppReqMsg fail");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSPSI_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_QryImsTimerPara(
    VOS_UINT8                           ucIndex
)
{
    VOS_UINT32                          ulResult;

    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_ERR_LOG1("AT_QryImsTimerPara: cmt type is not AT_CMD_OPT_READ_CMD",g_stATParseCmd.ucCmdOptType);
        return AT_ERROR;
    }

    /* 发送消息 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_IMSA_IMSTIMER_QRY_REQ,
                                   VOS_NULL_PTR,
                                   0,
                                   PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_ERR_LOG("AT_QryImsTimerPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /* 查询结束挂起通道 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_DMTIMER_QRY;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_QryImsSmsPsiPara(
    VOS_UINT8                           ucIndex
)
{
    VOS_UINT32                          ulResult;

    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    /* 发送消息 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_IMSA_SMSPSI_QRY_REQ,
                                   VOS_NULL_PTR,
                                   0,
                                   PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_ERR_LOG("AT_QryImsSmsPsiPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /* 查询结束挂起通道 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSPSI_QRY;

    return AT_WAIT_ASYNC_RETURN;

}


VOS_UINT32 AT_QryDmUserPara(
    VOS_UINT8                           ucIndex
)
{
    VOS_UINT32                          ulResult;

    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    /* 发送消息 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_IMSA_DMUSER_QRY_REQ,
                                   VOS_NULL_PTR,
                                   0,
                                   PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_ERR_LOG("AT_QryDmUserPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /* 查询结束挂起通道 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_DMUSER_QRY;

    return AT_WAIT_ASYNC_RETURN;

}


VOS_UINT32 AT_RcvImsaImsTimerSetCnf(
    VOS_VOID                            *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_IMSTIMER_SET_CNF_STRU       *pstCmTimerCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstCmTimerCnf = (IMSA_AT_IMSTIMER_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstCmTimerCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaImsTimerSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaImsTimerSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_DMTIMER_SET */
    if (AT_CMD_DMTIMER_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_ERR_LOG("AT_RcvImsaImsTimerSetCnf: WARNING:Not AT_CMD_DMTIMER_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstCmTimerCnf->ulResult)
    {
        ulResult    = AT_OK;
    }
    else
    {
        ulResult    = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaImsTimerQryCnf(
    VOS_VOID                            *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_IMSTIMER_QRY_CNF_STRU       *pstImsTimerCnf = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;

    /* 初始化消息变量 */
    ucIndex         = 0;
    usLength        = 0;
    pstImsTimerCnf    = (IMSA_AT_IMSTIMER_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsTimerCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaImsTimerQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaImsTimerQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_DMTIMER_QRY */
    if (AT_CMD_DMTIMER_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        AT_ERR_LOG("AT_RcvImsaImsTimerQryCnf: WARNING:Not AT_CMD_DMTIMER_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstImsTimerCnf->ulResult )
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          "%s: ",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName
                                          );
        /* <Timer_T1> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerT1Value,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerT1Value,
                                  VOS_FALSE);

        /* <Timer_T2> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerT2Value,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerT2Value,
                                  VOS_FALSE);

        /* <Timer_T4> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerT4Value,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerT4Value,
                                  VOS_FALSE);

        /* <Timer_TA> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTAValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTAValue,
                                  VOS_FALSE);

        /* <Timer_TB> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTBValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTBValue,
                                  VOS_FALSE);

        /* <Timer_TC> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTCValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTCValue,
                                  VOS_FALSE);

        /* <Timer_TD> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTDValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTDValue,
                                  VOS_FALSE);

        /* <Timer_TE> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTEValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTEValue,
                                  VOS_FALSE);

        /* <Timer_TF> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTFValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTFValue,
                                  VOS_FALSE);

        /* <Timer_TG> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTGValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTGValue,
                                  VOS_FALSE);

        /* <Timer_TH> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTHValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTHValue,
                                  VOS_FALSE);

        /* <Timer_TI> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTIValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTIValue,
                                  VOS_FALSE);

        /* <Timer_TJ> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTJValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTJValue,
                                  VOS_FALSE);

        /* <Timer_TK> */
        At_FillImsaTimerParaInCmd(&usLength,
                                  pstImsTimerCnf->stImsTimer.ulTimerTKValue,
                                  pstImsTimerCnf->stImsTimer.bitOpTimerTKValue,
                                  VOS_TRUE);

        gstAtSendData.usBufLen = usLength;

        ulResult                = AT_OK;
    }
    else
    {
        gstAtSendData.usBufLen  = 0;
        ulResult                = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}

VOS_UINT32 AT_RcvImsaImsPsiSetCnf(
    VOS_VOID                            *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_SMSPSI_SET_CNF_STRU         *pstSmsPsiCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstSmsPsiCnf = (IMSA_AT_SMSPSI_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstSmsPsiCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaImsPsiSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaImsPsiSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型AT_CMD_IMSPSI_SET */
    if (AT_CMD_IMSPSI_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_ERR_LOG("AT_RcvImsaImsPsiSetCnf: WARNING:Not AT_CMD_IMSPSI_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstSmsPsiCnf->ulResult)
    {
        ulResult    = AT_OK;
    }
    else
    {
        ulResult    = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;

}


VOS_UINT32 AT_RcvImsaImsPsiQryCnf(
    VOS_VOID                            *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_SMSPSI_QRY_CNF_STRU         *pstSmsPsiCnf = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 变量初始化 */
    ucIndex     = 0;
    pstSmsPsiCnf  = (IMSA_AT_SMSPSI_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstSmsPsiCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaImsPsiQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaImsPsiQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作是否是AT_CMD_IMSPSI_QRY */
    if (AT_CMD_IMSPSI_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_ERR_LOG("AT_RcvImsaImsPsiQryCnf: WARNING:Not AT_CMD_IMSPSI_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstSmsPsiCnf->ulResult)
    {
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                                       "%s: \"%s\"",
                                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                       pstSmsPsiCnf->stSmsPsi.acSmsPsi
                                                       );

        ulResult                = AT_OK;
    }
    else
    {
        gstAtSendData.usBufLen  = 0;
        ulResult                = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;

}

VOS_UINT32 AT_RcvImsaDmUserQryCnf(
    VOS_VOID                            *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_DMUSER_QRY_CNF_STRU         *pstDmUserCnf = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;
    TAF_NVIM_SMS_DOMAIN_STRU            stNvSmsDomain;

    /* 初始化消息变量 */
    ucIndex     = 0;
    pstDmUserCnf  = (IMSA_AT_DMUSER_QRY_CNF_STRU *)pMsg;

    TAF_MEM_SET_S(&stNvSmsDomain, sizeof(TAF_NVIM_SMS_DOMAIN_STRU), 0x00, sizeof(TAF_NVIM_SMS_DOMAIN_STRU));

    /* 读NV项en_NV_Item_SMS_DOMAIN，失败，直接返回 */
    if (NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_SMS_DOMAIN,
                           &stNvSmsDomain, (VOS_UINT32)sizeof(TAF_NVIM_SMS_DOMAIN_STRU)))
    {

        AT_ERR_LOG("AT_RcvImsaDmUserQryCnf():WARNING: read en_NV_Item_SMS_DOMAIN Error");

        return VOS_ERR;
    }

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstDmUserCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmUserQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_ERR_LOG("AT_RcvImsaDmUserQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_DMUSER_QRY */
    if (AT_CMD_DMUSER_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_ERR_LOG("AT_RcvImsaDmUserQryCnf: WARNING:Not AT_CMD_DMUSER_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstDmUserCnf->ulResult)
    {
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                                       "%s: \"%s\",\"%s\",\"%s\",%d,%d,%d",
                                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                       pstDmUserCnf->stDmUser.acImpi,
                                                       pstDmUserCnf->stDmUser.acImpu,
                                                       pstDmUserCnf->stDmUser.acHomeNetWorkDomain,
                                                       pstDmUserCnf->stDmUser.enVoiceDomain,
                                                       stNvSmsDomain.ucSmsDomain,
                                                       pstDmUserCnf->stDmUser.ulIpsecEnable
                                                       );

        ulResult                = AT_OK;

    }
    else
    {
        gstAtSendData.usBufLen  = 0;
        ulResult                = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}



VOS_UINT32 AT_SetNickNamePara(TAF_UINT8 ucIndex)
{
    AT_IMSA_NICKNAME_SET_REQ_STRU       stNickname;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usMaxNickNameLen;

    TAF_MEM_SET_S(&stNickname, sizeof(stNickname), 0x00, sizeof(AT_IMSA_NICKNAME_SET_REQ_STRU));
    usMaxNickNameLen = 2 * (MN_CALL_DISPLAY_NAME_STRING_SZ - 1);

    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_WARN_LOG("AT_SetNickNamePara: NOT AT_CMD_OPT_SET_PARA_CMD!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数个数不正确 */
    if (1 != gucAtParaIndex)
    {
        AT_WARN_LOG("AT_SetNickNamePara: para num is not equal 1!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if ( (0 == gastAtParaList[0].usParaLen)
      || (usMaxNickNameLen < gastAtParaList[0].usParaLen)
      || (0 != gastAtParaList[0].usParaLen % 2))
    {
        AT_WARN_LOG("AT_SetNickNamePara: para len is error!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (AT_SUCCESS != At_AsciiNum2HexString(gastAtParaList[0].aucPara, &gastAtParaList[0].usParaLen))
    {
        AT_WARN_LOG("AT_SetNickNamePara: At_AsciiNum2HexString is error!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stNickname.stNickName.ucNickNameLen = (VOS_UINT8)gastAtParaList[0].usParaLen;
    TAF_MEM_CPY_S( stNickname.stNickName.acNickName,
                   stNickname.stNickName.ucNickNameLen,
                   gastAtParaList[0].aucPara,
                   gastAtParaList[0].usParaLen);
    stNickname.stNickName.acNickName[stNickname.stNickName.ucNickNameLen] = '\0';

    /* 给IMSA发送^NICKNAME设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                     0,
                                     ID_AT_IMSA_NICKNAME_SET_REQ,
                                     &(stNickname.stNickName),
                                     (VOS_UINT32)sizeof(IMSA_AT_NICKNAME_INFO_STRU),
                                     PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetNickNamePara: AT_FillAndSndAppReqMsg is error!");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_NICKNAME_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_QryNickNamePara(TAF_UINT8 ucIndex)
{
    AT_IMSA_NICKNAME_QRY_REQ_STRU       stNickNameQryReq;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stNickNameQryReq, sizeof(stNickNameQryReq), 0x00, sizeof(AT_IMSA_NICKNAME_QRY_REQ_STRU));

    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送^NICKNAME查询请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                     0,
                                     ID_AT_IMSA_NICKNAME_QRY_REQ,
                                     VOS_NULL_PTR,
                                     0,
                                     PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_QryNickNamePara: AT_FillAndSndAppReqMsg is error!");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_NICKNAME_QRY;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvImsaNickNameSetCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_NICKNAME_SET_CNF_STRU      *pstNickNameSetCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex              = 0;
    pstNickNameSetCnf    = (IMSA_AT_NICKNAME_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstNickNameSetCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaNickNameSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaNickNameSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_NICKNAME_SET */
    if (AT_CMD_NICKNAME_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaNickNameSetCnf: WARNING:Not AT_CMD_NICKNAME_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstNickNameSetCnf->ulResult)
    {
        ulResult  = AT_OK;
    }
    else
    {
        ulResult  = AT_ERROR;
    }

    gstAtSendData.usBufLen  = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaNickNameQryCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_NICKNAME_QRY_CNF_STRU      *pstNickNameQryCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;
    VOS_UINT8                           ucIndex;
    VOS_UINT8                           i;

    /* 初始化消息变量 */
    ucIndex              = 0;
    usLength             = 0;
    i                    = 0;
    pstNickNameQryCnf    = (IMSA_AT_NICKNAME_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstNickNameQryCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaNickNameQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaNickNameQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_NICKNAME_QRY */
    if (AT_CMD_NICKNAME_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaNickNameQryCnf: WARNING:Not AT_CMD_NICKNAME_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if ( (VOS_OK == pstNickNameQryCnf->ulResult)
      && (MN_CALL_DISPLAY_NAME_STRING_SZ > pstNickNameQryCnf->stNickName.ucNickNameLen))
    {
        if (0 != pstNickNameQryCnf->stNickName.ucNickNameLen)
        {
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                               (VOS_CHAR *)pgucAtSndCodeAddr,
                                               (VOS_CHAR *)pgucAtSndCodeAddr,
                                               "%s: ",
                                               g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

            for (i = 0; i < pstNickNameQryCnf->stNickName.ucNickNameLen; i++)
            {
                usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                                   "%X",
                                                   (VOS_UINT8)pstNickNameQryCnf->stNickName.acNickName[i]);
            }
        }
        else
        {
            AT_WARN_LOG("AT_RcvImsaNickNameQryCnf: WARNING: ucNickNameLen is 0!");
        }

        ulResult  = AT_OK;
    }
    else
    {
        ulResult  = AT_ERROR;
    }

    gstAtSendData.usBufLen  = usLength;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaImsRegFailInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_REG_FAIL_IND_STRU              *pstRegFailInd = VOS_NULL_PTR;
    VOS_UINT8                               ucIndex;

    /* 初始化消息变量 */
    ucIndex             = 0;
    pstRegFailInd       = (IMSA_AT_REG_FAIL_IND_STRU*)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstRegFailInd->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsRegFailInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   "%s%s%d%s",
                                                   gaucAtCrLf,
                                                   gastAtStringTab[AT_STRING_IMS_REG_FAIL].pucText,
                                                   pstRegFailInd->stRegFailInfo.enFailCode,
                                                   gaucAtCrLf);
    /* 调用At_SendResultData发送命令结果 */
    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}


VOS_UINT32 AT_SetBatteryInfoPara(TAF_UINT8 ucIndex)
{
    AT_MODEM_IMS_CONTEXT_STRU          *pstLocalBatteryInfo = VOS_NULL_PTR;
    AT_IMSA_BATTERY_INFO_SET_REQ_STRU   stBatteryInfoSetReq;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stBatteryInfoSetReq, sizeof(stBatteryInfoSetReq), 0x00, sizeof(AT_IMSA_BATTERY_INFO_SET_REQ_STRU));
    pstLocalBatteryInfo = AT_GetModemImsCtxAddrFromClientId(gastAtClientTab[ucIndex].usClientId);

    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_WARN_LOG("AT_SetBatteryInfoPara: NOT AT_CMD_OPT_SET_PARA_CMD!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数个数不正确 */
    if (1 != gucAtParaIndex)
    {
        AT_WARN_LOG("AT_SetBatteryInfoPara: para num is not equal 1!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (0 == gastAtParaList[0].usParaLen)
    {
        AT_WARN_LOG("AT_SetBatteryInfoPara: para len is error!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stBatteryInfoSetReq.stBatteryStatusInfo.enBatteryStatus = (AT_IMSA_BATTERY_STATUS_ENUM_UINT8)gastAtParaList[0].ulParaValue;

    /* 给IMSA发送^BATTERYINFO设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      0,
                                      ID_AT_IMSA_BATTERYINFO_SET_REQ,
                                      &(stBatteryInfoSetReq.stBatteryStatusInfo),
                                      (VOS_UINT32)sizeof(AT_IMSA_BATTERY_STATUS_INFO_STRU),
                                      PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetBatteryInfoPara: AT_FillAndSndAppReqMsg is error!");
        return AT_ERROR;
    }

    pstLocalBatteryInfo->stBatteryInfo.enTempBatteryInfo = stBatteryInfoSetReq.stBatteryStatusInfo.enBatteryStatus;

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_BATTERYINFO_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvImsaBatteryInfoSetCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_BATTERY_INFO_SET_CNF_STRU  *pstBatteryInfoSetCnf = VOS_NULL_PTR;
    AT_MODEM_IMS_CONTEXT_STRU          *pstLocalBatteryInfo  = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex              = 0;
    pstLocalBatteryInfo  = AT_GetModemImsCtxAddrFromClientId(gastAtClientTab[ucIndex].usClientId);
    pstBatteryInfoSetCnf = (IMSA_AT_BATTERY_INFO_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstBatteryInfoSetCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaBatteryInfoSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaBatteryInfoSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_BATTERYINFO_SET */
    if (AT_CMD_BATTERYINFO_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaBatteryInfoSetCnf: WARNING:Not AT_CMD_BATTERYINFO_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断设置操作是否成功 */
    if (VOS_OK == pstBatteryInfoSetCnf->ulResult)
    {
        pstLocalBatteryInfo->stBatteryInfo.enCurrBatteryInfo = pstLocalBatteryInfo->stBatteryInfo.enTempBatteryInfo;
        pstLocalBatteryInfo->stBatteryInfo.enTempBatteryInfo = AT_IMSA_BATTERY_STATUS_BUTT;
        AT_NORM_LOG1("AT_RcvImsaBatteryInfoSetCnf: Local enBatteryInfo is ", pstLocalBatteryInfo->stBatteryInfo.enCurrBatteryInfo);

        ulResult  = AT_OK;
    }
    else
    {
        ulResult  = AT_ERROR;
    }

    gstAtSendData.usBufLen  = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_QryBatteryInfoPara(TAF_UINT8 ucIndex)
{
    AT_MODEM_IMS_CONTEXT_STRU          *pstLocalBatteryInfo = VOS_NULL_PTR;

    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }


    pstLocalBatteryInfo = AT_GetModemImsCtxAddrFromClientId(gastAtClientTab[ucIndex].usClientId);

    if (AT_IMSA_BATTERY_STATUS_BUTT > pstLocalBatteryInfo->stBatteryInfo.enCurrBatteryInfo)
    {
        gstAtSendData.usBufLen  = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                         (VOS_CHAR*)pgucAtSndCodeAddr,
                                                         (VOS_CHAR*)pgucAtSndCodeAddr,
                                                         "%s: %d",
                                                         g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                         pstLocalBatteryInfo->stBatteryInfo.enCurrBatteryInfo);

        return AT_OK;
    }
    else
    {
        return AT_ERROR;
    }
}


VOS_UINT32 AT_SetVolteRegPara(TAF_UINT8 ucIndex)
{
    VOS_UINT32                          ulResult;

    if (AT_CMD_OPT_SET_CMD_NO_PARA != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送^VOLTEREG查询请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      0,
                                      ID_AT_IMSA_VOLTEREG_NTF,
                                      VOS_NULL_PTR,
                                      0,
                                      PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetVolteRegPara: AT_FillAndSndAppReqMsg is error!");
        return AT_ERROR;
    }

    return AT_OK;
}



VOS_UINT32 AT_SetImsVideoCallCancelPara(TAF_UINT8 ucIndex)
{
    VOS_UINT32                          ulResult;

    if (AT_CMD_OPT_SET_CMD_NO_PARA != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送^IMSVIDEOCALLCANCEL查询请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      0,
                                      ID_AT_IMSA_CANCEL_ADD_VIDEO_REQ,
                                      VOS_NULL_PTR,
                                      0,
                                      PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetImsVideoCallCancelPara: AT_FillAndSndAppReqMsg is error!");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSVIDEOCALLCANCEL_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvImsaImsVideoCallCancelSetCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CANCEL_ADD_VIDEO_CNF_STRU          *pstImsVideoCallCancelSetCnf = VOS_NULL_PTR;
    VOS_UINT32                                  ulResult;
    VOS_UINT8                                   ucIndex;

    /* 初始化消息变量 */
    ucIndex                     = 0;
    pstImsVideoCallCancelSetCnf = (IMSA_AT_CANCEL_ADD_VIDEO_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsVideoCallCancelSetCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsVideoCallCancelSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsVideoCallCancelSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMSVIDEOCALLCANCEL_SET */
    if (AT_CMD_IMSVIDEOCALLCANCEL_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaImsVideoCallCancelSetCnf: WARNING:Not AT_CMD_IMSVIDEOCALLCANCEL_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断设置操作是否成功 */
    if (VOS_OK == pstImsVideoCallCancelSetCnf->ulResult)
    {
        ulResult  = AT_OK;
    }
    else
    {
        ulResult  = AT_ERROR;
    }

    gstAtSendData.usBufLen  = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 At_ParseImsvtcapcfgPara(
    TAF_MMA_IMS_VIDEO_CALL_CAP_STRU    *pstImsVtCap
)
{
    switch (gastAtParaList[0].ulParaValue)
    {
        /* chicagoc20只支持视频电话呼叫等待能力的配置，所以第一个参数必须为1 */
        case 1:
            pstImsVtCap->enVideoCallCapType  = TAF_MMA_IMS_VIDEO_CALL_CAP_CCWA;
            pstImsVtCap->ulVideoCallCapValue = gastAtParaList[1].ulParaValue;
            return VOS_TRUE;
        default:
            return VOS_FALSE;
    }
}


VOS_UINT32 AT_SetImsVtCapCfgPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulRst;
    TAF_MMA_IMS_VIDEO_CALL_CAP_STRU     stImsVtCap;

    /* 初始化结构体 */
    TAF_MEM_SET_S(&stImsVtCap, sizeof(stImsVtCap), 0x00, sizeof(stImsVtCap));

    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数过多 */
    if (2 != gucAtParaIndex)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数值不对 */
    if ((0 == gastAtParaList[0].usParaLen)
     || (0 == gastAtParaList[1].usParaLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (VOS_TRUE != At_ParseImsvtcapcfgPara(&stImsVtCap))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 执行命令操作 */
    ulRst = TAF_MMA_SetImsVtCapCfgReq(WUEPS_PID_AT,
                                      gastAtClientTab[ucIndex].usClientId,
                                      0,
                                      &stImsVtCap);

    if (VOS_TRUE == ulRst)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSVTCAPCFG_SET;
        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }
    else
    {
        return AT_ERROR;
    }
}


VOS_UINT32 AT_RcvMmaImsVideoCallCapSetCnf(
    VOS_VOID                           *pMsg
)
{
    TAF_MMA_IMS_VIDEO_CALL_CAP_SET_CNF_STRU                *pstImsVtCapSetCnf = VOS_NULL_PTR;
    VOS_UINT32                                              ulResult;
    VOS_UINT8                                               ucIndex;

    ucIndex           = 0;
    pstImsVtCapSetCnf = (TAF_MMA_IMS_VIDEO_CALL_CAP_SET_CNF_STRU *)pMsg;

    /* 通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsVtCapSetCnf->stCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMmaImsVideoCallCapSetCnf :WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /* 判断是否为广播 */
    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMmaImsVideoCallCapSetCnf : AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMSSMSCFG_SET */
    if (AT_CMD_IMSVTCAPCFG_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMmaImsVideoCallCapSetCnf : WARNING:Not AT_CMD_ImsSmsCfg_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    if (TAF_ERR_NO_ERROR == pstImsVtCapSetCnf->enResult)
    {
        ulResult = AT_OK;
    }
    else
    {
        ulResult = At_ChgTafErrorCode(ucIndex, pstImsVtCapSetCnf->enResult);
    }

    gstAtSendData.usBufLen = 0;
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_SetImsSmsCfgPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulRst;
    VOS_UINT8                           ucEnableFlg;

    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数过多 */
    if (1 != gucAtParaIndex)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

     /* 参数为空 */
    if (0 == gastAtParaList[0].usParaLen)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 当前只能使能或不使能IMS短信，故使用VOS_TRUE和VOS_FALSE标示 */
    ucEnableFlg = (VOS_UINT8)gastAtParaList[0].ulParaValue;

    /* 执行命令操作 */
    ulRst = TAF_MMA_SetImsSmsCfgReq(WUEPS_PID_AT,
                                   gastAtClientTab[ucIndex].usClientId,
                                   0,
                                   ucEnableFlg);

    if (VOS_TRUE == ulRst)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSSMSCFG_SET;
        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }
    else
    {
        return AT_ERROR;
    }
}


VOS_UINT32 AT_QryImsSmsCfgPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32 ulResult;

    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    /* AT 给MMA 发送查询请求消息 */
    ulResult = TAF_MMA_QryImsSmsCfgReq(WUEPS_PID_AT,
                                      gastAtClientTab[ucIndex].usClientId,
                                      0);

    if (VOS_TRUE == ulResult)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSSMSCFG_QRY;
        return AT_WAIT_ASYNC_RETURN;
    }
    else
    {
        return AT_ERROR;
    }

}


VOS_UINT32 AT_RcvMmaImsSmsCfgSetCnf(
    VOS_VOID                           *pMsg
)
{
    TAF_MMA_IMS_SMS_CFG_SET_CNF_STRU   *pstImsSmsCfgSetCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT8                           ucIndex;

    pstImsSmsCfgSetCnf = (TAF_MMA_IMS_SMS_CFG_SET_CNF_STRU *)pMsg;

    /* 通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsSmsCfgSetCnf->stCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMmaImsSmsCfgSetCnf :WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /* 判断是否为广播 */
    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMmaImsSmsCfgSetCnf : AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMSSMSCFG_SET */
    if (AT_CMD_IMSSMSCFG_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMmaImsSmsCfgSetCnf : WARNING:Not AT_CMD_ImsSmsCfg_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    if (TAF_ERR_NO_ERROR == pstImsSmsCfgSetCnf->enResult)
    {
        ulResult = AT_OK;
    }
    else
    {
        ulResult = At_ChgTafErrorCode(ucIndex, pstImsSmsCfgSetCnf->enResult);
    }

    gstAtSendData.usBufLen = 0;
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvMmaImsSmsCfgQryCnf(
    VOS_VOID                           *pMsg
)
{
    TAF_MMA_IMS_SMS_CFG_QRY_CNF_STRU   *pstImsSmsCfgQryCnf = VOS_NULL_PTR;
    VOS_UINT8                          ucIndex;
    VOS_UINT32                         ulResult;

    pstImsSmsCfgQryCnf = (TAF_MMA_IMS_SMS_CFG_QRY_CNF_STRU *)pMsg;

    /* 通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsSmsCfgQryCnf->stCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMmaImsSmsCfgQryCnf :WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMmaImsSmsCfgQryCnf : AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMSSMSCFG_QRY */
    if (AT_CMD_IMSSMSCFG_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMmaImsSmsCfgQryCnf : WARNING:Not AT_CMD_IMSSMSCFG_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    gstAtSendData.usBufLen = 0;

    if (TAF_ERR_NO_ERROR == pstImsSmsCfgQryCnf->enResult)
    {
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s: %d",
                                                    g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    pstImsSmsCfgQryCnf->ucEnableFlg);

        ulResult = AT_OK;
    }
    else
    {
        ulResult = At_ChgTafErrorCode(ucIndex, pstImsSmsCfgQryCnf->enResult);
    }

    At_FormatResultData(ucIndex, ulResult);
    return VOS_OK;
}


VOS_UINT32 AT_SetImsRegErrRpt(
    TAF_UINT8                           ucIndex
)
{
    VOS_UINT32                          ulRst;
    VOS_UINT8                           ucReportFlag;

    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数过多 */
    if (1 != gucAtParaIndex)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数为空 */
    if (0 == gastAtParaList[0].usParaLen)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 当前只能使能或不使能ims注册错误原因值上报，故使用VOS_TRUE和VOS_FALSE标示 */
    ucReportFlag = (VOS_UINT8)gastAtParaList[0].ulParaValue;

    /* 给IMSA发送^IMSREGERRRPT设置请求 */
    ulRst = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_IMSA_REGERR_REPORT_SET_REQ,
                                   &(ucReportFlag),
                                   (VOS_UINT32)sizeof(ucReportFlag),
                                   I0_PS_PID_IMSA);

    if (TAF_SUCCESS == ulRst)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSREGERRRPT_SET;
        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }
    else
    {
        return AT_ERROR;
    }
}


VOS_UINT32 AT_RcvImsaRegErrRptSetCnf(
    VOS_VOID                           *pMsg
)
{
    IMSA_AT_REGERR_REPORT_SET_CNF_STRU *pstErrRptSetCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT8                           ucIndex;

    ucIndex         = 0;
    pstErrRptSetCnf = (IMSA_AT_REGERR_REPORT_SET_CNF_STRU *)pMsg;


    /* 通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstErrRptSetCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaRegErrRptSetCnf :WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /* 判断是否为广播 */
    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaRegErrRptSetCnf : AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMSREGERRRPT_SET */
    if (AT_CMD_IMSREGERRRPT_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaRegErrRptSetCnf : WARNING:Not AT_CMD_IMSREGERRRPT_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstErrRptSetCnf->ulResult)
    {
        ulResult  = AT_OK;
    }
    else
    {
        ulResult  = AT_ERROR;
    }

    gstAtSendData.usBufLen  = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_QryImsRegErrRpt(
    TAF_UINT8                           ucIndex
)
{
    VOS_UINT32                          ulRst;

    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_ERR_LOG1("AT_QryImsRegErrRpt: cmt type is not AT_CMD_OPT_READ_CMD", g_stATParseCmd.ucCmdOptType);
        return AT_ERROR;
    }

    /* 给IMSA发送^IMSREGERRRPT查询请求 */
    ulRst = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_IMSA_REGERR_REPORT_QRY_REQ,
                                   VOS_NULL_PTR,
                                   0,
                                   I0_PS_PID_IMSA);

    if (TAF_SUCCESS == ulRst)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSREGERRRPT_QRY;
        return AT_WAIT_ASYNC_RETURN;    /* 返回命令处理挂起状态 */
    }
    else
    {
        return AT_ERROR;
    }
}


VOS_UINT32 AT_RcvImsaRegErrRptQryCnf(
    VOS_VOID                           *pMsg
)
{
    IMSA_AT_REGERR_REPORT_QRY_CNF_STRU *pstErrRptQryCnf = VOS_NULL_PTR;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    ucIndex         = 0;
    pstErrRptQryCnf = (IMSA_AT_REGERR_REPORT_QRY_CNF_STRU *)pMsg;

    /* 通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstErrRptQryCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaRegErrRptQryCnf :WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaRegErrRptQryCnf : AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMSREGERRRPT_QRY */
    if (AT_CMD_IMSREGERRRPT_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaRegErrRptQryCnf : WARNING:Not AT_CMD_IMSREGERRRPT_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    gstAtSendData.usBufLen = 0;

    if (VOS_OK == pstErrRptQryCnf->ulResult)
    {
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s: %d",
                                                    g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    pstErrRptQryCnf->ucReportFlag);

        ulResult = AT_OK;
    }
    else
    {
        ulResult = AT_ERROR;
    }

    At_FormatResultData(ucIndex, ulResult);
    return VOS_OK;
}


VOS_UINT32 AT_RcvImsaRegErrRptInd(
    VOS_VOID                           *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_REGERR_REPORT_IND_STRU     *pstRegErrInd = VOS_NULL_PTR;
    VOS_UINT32                          ulFailStage;
    VOS_UINT32                          ulFailCause;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    pstRegErrInd    = (IMSA_AT_REGERR_REPORT_IND_STRU *)pMsg;
    ulFailStage     = 0;
    ulFailCause     = 0;
    ucIndex         = 0;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstRegErrInd->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaRegErrRptInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /* 添加异常原因值检查 */
    if (VOS_TRUE != AT_IsImsRegErrRptParaValid(pMsg))
    {
        return VOS_ERR;
    }

    if (IMSA_AT_REG_ERR_TYPE_PDN_FAIL == pstRegErrInd->enImsaRegErrType)
    {
        ulFailStage = pstRegErrInd->enImsaRegErrType;
        ulFailCause = pstRegErrInd->enImsaPdnFailCause;
    }

    if (IMSA_AT_REG_ERR_TYPE_IMS_REG_FAIL == pstRegErrInd->enImsaRegErrType)
    {
        ulFailStage = pstRegErrInd->enImsaRegErrType;
        ulFailCause = pstRegErrInd->enImsaRegFailCause;
    }

    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       "%s%s %d,%u,%u,\"%s\"%s",
                                       gaucAtCrLf,
                                       gastAtStringTab[AT_STRING_IMS_REG_ERR].pucText,
                                       pstRegErrInd->enImsaRegDomain,
                                       ulFailStage,
                                       ulFailCause,
                                       pstRegErrInd->acImsRegFailReasonCtx,
                                       gaucAtCrLf);


    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}


VOS_UINT32 AT_IsImsRegErrRptParaValid(
    VOS_VOID                           *pMsg
)
{
    IMSA_AT_REGERR_REPORT_IND_STRU     *pstRegErrInd = VOS_NULL_PTR;
    VOS_UINT32                          ulStrLen;

    /* 初始化消息变量 */
    pstRegErrInd    = (IMSA_AT_REGERR_REPORT_IND_STRU *)pMsg;
    ulStrLen        = 0;

    /* 如果注册域为非wifi和非lte，认为上报异常 */
    if (pstRegErrInd->enImsaRegDomain >= IMSA_AT_IMS_REG_DOMAIN_TYPE_UNKNOWN)
    {
        return VOS_FALSE;
    }

    /* 如果注册失败类型为非pdn和非reg，认为上报异常 */
    if (pstRegErrInd->enImsaRegErrType >= IMSA_AT_REG_ERR_TYPE_BUTT)
    {
        return VOS_FALSE;
    }

    /* PDN类型失败，但失败原因值大于pdn失败原因值的最大值，则认为上报异常 */
    if ((IMSA_AT_REG_ERR_TYPE_PDN_FAIL == pstRegErrInd->enImsaRegErrType)
     && (pstRegErrInd->enImsaPdnFailCause >= IMSA_AT_PDN_FAIL_CAUSE_BUTT))
    {
        return VOS_FALSE;
    }

    /* reg类型失败，但失败原因值大于reg失败原因值的最大值，则认为上报异常 */
    if ((IMSA_AT_REG_ERR_TYPE_IMS_REG_FAIL == pstRegErrInd->enImsaRegErrType)
     && (pstRegErrInd->enImsaRegFailCause >= IMSA_AT_REG_FAIL_CAUSE_BUTT))
    {
        return VOS_FALSE;
    }

    ulStrLen = VOS_StrLen(pstRegErrInd->acImsRegFailReasonCtx);

    /* 失败字符串约定最大长度为255，大于255，认为上报异常 */
    if (ulStrLen > (IMSA_AT_REG_FAIL_CAUSE_STR_MAX_LEN - 1))
    {
        AT_ERR_LOG1("AT_IsImsRegErrRptParaValid: str len beyond IMSA_AT_REG_FAIL_CAUSE_STR_MAX_LEN!", ulStrLen);
        return VOS_FALSE;
    }

    return VOS_TRUE;
}


VOS_UINT32 AT_RcvImsaCallAltSrvInd(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_CALL_ALT_SRV_IND_STRU              *pstCallAltSrvInd    = VOS_NULL_PTR;
    VOS_UINT8                                   ucIndex;

    /* 初始化消息变量 */
    ucIndex             = 0;
    pstCallAltSrvInd    = (IMSA_AT_CALL_ALT_SRV_IND_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstCallAltSrvInd->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaCallAltSrvInd: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       "%s%s%s",
                                       gaucAtCrLf,
                                       gastAtStringTab[AT_STRING_CALL_ALT_SRV].pucText,
                                       gaucAtCrLf);

    At_SendResultData(ucIndex, pgucAtSndCodeAddr, gstAtSendData.usBufLen);

    return VOS_OK;
}



VOS_VOID AT_ProcUserAgentPara(
    AT_IMSA_USER_AGENT_CFG_STRU             *pstUserAgentCfg
)
{

    /* 给参数赋值*/
    if (0 != gastAtParaList[0].usParaLen)
    {
        TAF_MEM_CPY_S(pstUserAgentCfg->aucPara1, sizeof(pstUserAgentCfg->aucPara1), gastAtParaList[0].aucPara, gastAtParaList[0].usParaLen);
    }

    if (0 != gastAtParaList[1].usParaLen)
    {
        TAF_MEM_CPY_S(pstUserAgentCfg->aucPara2, sizeof(pstUserAgentCfg->aucPara2), gastAtParaList[1].aucPara, gastAtParaList[1].usParaLen);
    }

    if (0 != gastAtParaList[2].usParaLen)
    {
        TAF_MEM_CPY_S(pstUserAgentCfg->aucPara3, sizeof(pstUserAgentCfg->aucPara3), gastAtParaList[2].aucPara, gastAtParaList[2].usParaLen);
    }

    if (0 != gastAtParaList[3].usParaLen)
    {
        TAF_MEM_CPY_S(pstUserAgentCfg->aucPara4, sizeof(pstUserAgentCfg->aucPara4), gastAtParaList[3].aucPara, gastAtParaList[3].usParaLen);
    }

    if (0 != gastAtParaList[4].usParaLen)
    {
        TAF_MEM_CPY_S(pstUserAgentCfg->aucPara5, sizeof(pstUserAgentCfg->aucPara5), gastAtParaList[4].aucPara, gastAtParaList[4].usParaLen);
    }

    if (0 != gastAtParaList[5].usParaLen)
    {
        TAF_MEM_CPY_S(pstUserAgentCfg->aucPara6, sizeof(pstUserAgentCfg->aucPara6), gastAtParaList[5].aucPara, gastAtParaList[5].usParaLen);
    }

    return;
}


VOS_UINT32 AT_SetUserAgentCfgPara(VOS_UINT8 ucIndex)
{
    AT_IMSA_USER_AGENT_CFG_STRU              stUserAgentCfg;
    VOS_UINT32                               ulResult;

    TAF_MEM_SET_S(&stUserAgentCfg, sizeof(stUserAgentCfg), 0x00, sizeof(AT_IMSA_USER_AGENT_CFG_STRU));

    /* 参数类型检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数个数错误 */
    if (6 != gucAtParaIndex)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数长度错误 */
    if ((AT_IMSA_USER_AGENT_STR_LEN < gastAtParaList[0].usParaLen)
      ||(AT_IMSA_USER_AGENT_STR_LEN < gastAtParaList[1].usParaLen)
      ||(AT_IMSA_USER_AGENT_STR_LEN < gastAtParaList[2].usParaLen)
      ||(AT_IMSA_USER_AGENT_STR_LEN < gastAtParaList[3].usParaLen)
      ||(AT_IMSA_USER_AGENT_STR_LEN < gastAtParaList[4].usParaLen)
      ||(AT_IMSA_USER_AGENT_STR_LEN < gastAtParaList[5].usParaLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给参数赋值*/
    AT_ProcUserAgentPara(&stUserAgentCfg);

    /* 给IMSA发送^USERAGENTCFG设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      gastAtClientTab[ucIndex].opId,
                                      ID_AT_IMSA_USER_AGENT_CFG_SET_REQ,
                                      &stUserAgentCfg,
                                      (VOS_UINT32)sizeof(AT_IMSA_USER_AGENT_CFG_STRU),
                                      PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetUserAgentCfgPara: AT_FillAndSndAppReqMsg is error!");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_USERAGENTCFG_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvImsaUserAgentSetCnf(VOS_VOID * pMsg)
{
    /* 定义局部变量 */
    IMSA_AT_USER_AGENT_CFG_SET_CNF_STRU       *pstImsUserAgentSetCnf = VOS_NULL_PTR;
    VOS_UINT32                                 ulResult;
    VOS_UINT8                                  ucIndex;

    /* 初始化消息变量 */
    ucIndex                    = 0;
    pstImsUserAgentSetCnf      = (IMSA_AT_USER_AGENT_CFG_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsUserAgentSetCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaUserAgentSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    /*广播*/
    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaUserAgentSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_USERAGENTCFG_SET */
    if (AT_CMD_USERAGENTCFG_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaUserAgentSetCnf: WARNING:Not AT_CMD_USERAGENTCFG_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断设置操作是否成功 */
    if (VOS_OK == pstImsUserAgentSetCnf->ulResult)
    {
        ulResult  = AT_OK;
    }
    else
    {
        ulResult  = AT_ERROR;
    }

    gstAtSendData.usBufLen  = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 At_SetEflociInfoPara(VOS_UINT8 ucIndex)
{
    TAF_MMA_EFLOCIINFO_STRU             stEfLociInfo;
    VOS_UINT32                          ulPlmnHex;

    /* 初始化结构体 */
    TAF_MEM_SET_S(&stEfLociInfo, sizeof(stEfLociInfo), 0x00, sizeof(stEfLociInfo));

    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_WARN_LOG("At_SetEflociInfoPara: NOT AT_CMD_OPT_SET_PARA_CMD!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数个数不正确 */
    if (5 != gucAtParaIndex)
    {
        AT_WARN_LOG("At_SetEflociInfoPara: PARA NUM IS NOT EQUAL 5!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 转换Tmsi */
    if (0 == gastAtParaList[0].usParaLen)
    {
        AT_WARN_LOG("At_SetEflociInfoPara: Tmsi LEN IS 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stEfLociInfo.ulTmsi = gastAtParaList[0].ulParaValue;

    /* 转换PLMN */
    if ((AT_PLMN_STR_MAX_LEN != gastAtParaList[1].usParaLen)
     && (AT_PLMN_STR_MIN_LEN != gastAtParaList[1].usParaLen))
    {
        AT_WARN_LOG("At_SetEflociInfoPara: PLMN LEN IS ERROR!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulPlmnHex = 0;

    if (AT_SUCCESS != AT_String2Hex(gastAtParaList[1].aucPara,
                                 gastAtParaList[1].usParaLen,
                                 &ulPlmnHex) )
    {
        AT_WARN_LOG("At_SetEflociInfoPara: String2Hex error!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (0xffffff == ulPlmnHex)
    {
        stEfLociInfo.stPlmnId.Mcc = 0xFFFFFFFF;
        stEfLociInfo.stPlmnId.Mnc = 0xFFFFFFFF;
    }
    else
    {
        if (VOS_TRUE != AT_DigitString2Hex(gastAtParaList[1].aucPara, 3, &stEfLociInfo.stPlmnId.Mcc))
        {
            AT_WARN_LOG("At_SetEflociInfoPara: Mcc IS ERROR!");
            return AT_CME_INCORRECT_PARAMETERS;
        }

        if (VOS_TRUE != AT_DigitString2Hex(&(gastAtParaList[1].aucPara[3]), gastAtParaList[1].usParaLen - 3, &(stEfLociInfo.stPlmnId.Mnc)))
        {
            AT_WARN_LOG("At_SetEflociInfoPara: Mnc IS ERROR!");
            return AT_CME_INCORRECT_PARAMETERS;
        }

        stEfLociInfo.stPlmnId.Mcc |= 0xFFFFF000;
        stEfLociInfo.stPlmnId.Mnc |= (0xFFFFFFFF << ((gastAtParaList[1].usParaLen - 3) * 4));
    }

    /* 转换LAC */
    if (0 == gastAtParaList[2].usParaLen)
    {
        AT_WARN_LOG("At_SetEflociInfoPara: LAC LEN IS 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stEfLociInfo.usLac = (VOS_UINT16)gastAtParaList[2].ulParaValue;

    /* 转换location_update_Status */
    if (0 == gastAtParaList[3].usParaLen)
    {
        AT_WARN_LOG("At_SetEflociInfoPara: location_update_Status LEN IS 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stEfLociInfo.ucLocationUpdateStatus = (VOS_UINT8)gastAtParaList[3].ulParaValue;

    /* 转换rfu */
    if (0 == gastAtParaList[4].usParaLen)
    {
        AT_WARN_LOG("At_SetEflociInfoPara: rfu LEN IS 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stEfLociInfo.ucRfu = (VOS_UINT8)gastAtParaList[4].ulParaValue;

    /* 执行操作命令 */
    if (VOS_TRUE != TAF_MMA_SetEflociInfo(WUEPS_PID_AT,
                                          gastAtClientTab[ucIndex].usClientId,
                                          0,
                                          &stEfLociInfo))
    {
        AT_WARN_LOG("At_SetEflociInfoPara: TAF_MMA_SetEflociInfo return is not VOS_TRUE !");
        return AT_ERROR;
    }

    /* 设置当前操作类型 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_EFLOCIINFO_SET;

    /* 返回命令处理挂起状态 */
    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 At_QryEflociInfoPara(VOS_UINT8 ucIndex)
{
    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (VOS_TRUE != TAF_MMA_QryEflociInfo(WUEPS_PID_AT, gastAtClientTab[ucIndex].usClientId, 0))
    {
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_EFLOCIINFO_QRY;

    /* 返回命令处理挂起状态 */
    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 At_SetPsEflociInfoPara(VOS_UINT8 ucIndex)
{
    TAF_MMA_EFPSLOCIINFO_STRU           stPsefLociInfo;
    VOS_UINT32                          ulPlmnHex;

    /* 初始化结构体 */
    TAF_MEM_SET_S(&stPsefLociInfo, sizeof(stPsefLociInfo), 0x00, sizeof(TAF_MMA_EFPSLOCIINFO_STRU));

    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: NOT AT_CMD_OPT_SET_PARA_CMD!");
       return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数个数不正确 */
    if (6 != gucAtParaIndex)
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: Para num is error!");
       return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 转换PTMSI */
    if (0 == gastAtParaList[0].usParaLen)
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: PTMSI len is 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stPsefLociInfo.ulPTmsi = gastAtParaList[0].ulParaValue;

    /* 转换PtmsiSignature */
    if (0 == gastAtParaList[1].usParaLen)
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: PtmsiSignature len is 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stPsefLociInfo.ulPTmsiSignature = gastAtParaList[1].ulParaValue;

    /* 转换PLMN */
    if ((AT_PLMN_STR_MAX_LEN != gastAtParaList[2].usParaLen)
     && (AT_PLMN_STR_MIN_LEN != gastAtParaList[2].usParaLen))
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: PLMN len is Error!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulPlmnHex = 0;

    if (AT_SUCCESS != AT_String2Hex(gastAtParaList[2].aucPara,
                                 gastAtParaList[2].usParaLen,
                                 &ulPlmnHex) )
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: String2Hex error!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (0xffffff == ulPlmnHex)
    {
        stPsefLociInfo.stPlmnId.Mcc = 0xFFFFFFFF;
        stPsefLociInfo.stPlmnId.Mnc = 0xFFFFFFFF;
    }
    else
    {
        if (VOS_TRUE != AT_DigitString2Hex(gastAtParaList[2].aucPara, 3, &stPsefLociInfo.stPlmnId.Mcc))
        {
            AT_WARN_LOG("At_SetPsEflociInfoPara: Mcc num is Error!");
            return AT_CME_INCORRECT_PARAMETERS;
        }

        if (VOS_TRUE != AT_DigitString2Hex(&(gastAtParaList[2].aucPara[3]), gastAtParaList[2].usParaLen - 3, &(stPsefLociInfo.stPlmnId.Mnc)))
        {
            AT_WARN_LOG("At_SetPsEflociInfoPara: Mnc num is Error!");
            return AT_CME_INCORRECT_PARAMETERS;
        }

        stPsefLociInfo.stPlmnId.Mcc |= 0xFFFFF000;
        stPsefLociInfo.stPlmnId.Mnc |= (0xFFFFFFFF << ((gastAtParaList[2].usParaLen -3 ) * 4));
    }

    /* 转换LAC */
    if (0 == gastAtParaList[3].usParaLen)
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: LAC len is 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stPsefLociInfo.usLac = (VOS_UINT16)gastAtParaList[3].ulParaValue;

    /* 转换RAC */
    if (0 == gastAtParaList[4].usParaLen)
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: RAC len is 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stPsefLociInfo.ucRac = (VOS_UINT8)gastAtParaList[4].ulParaValue;

    /* 转换location_update_Status */
    if (0 == gastAtParaList[5].usParaLen)
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: location_update_Status len is 0!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stPsefLociInfo.ucPsLocationUpdateStatus = (VOS_UINT8)gastAtParaList[5].ulParaValue;

    /* 执行操作命令 */
    if (VOS_TRUE != TAF_MMA_SetPsEflociInfo(WUEPS_PID_AT,
                                            gastAtClientTab[ucIndex].usClientId,
                                            0,
                                            &stPsefLociInfo))
    {
        AT_WARN_LOG("At_SetPsEflociInfoPara: TAF_MMA_SetPsEflociInfo return is not VOS_TRUE !");
        return AT_ERROR;
    }

    /* 设置当前操作类型 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_EFPSLOCIINFO_SET;

    /* 返回命令处理挂起状态 */
    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 At_QryPsEflociInfoPara(VOS_UINT8 ucIndex)
{
    /* 参数检查 */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (VOS_TRUE != TAF_MMA_QryPsEflociInfo(WUEPS_PID_AT, gastAtClientTab[ucIndex].usClientId, 0))
    {
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_EFPSLOCIINFO_QRY;

    /* 返回命令处理挂起状态 */
    return AT_WAIT_ASYNC_RETURN;
}




VOS_UINT32 AT_SetImsIpCapPara(VOS_UINT8 ucIndex)
{
    AT_IMSA_IMS_IP_CAP_SET_INFO_STRU    stImsIpCapSetInfo;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stImsIpCapSetInfo, sizeof(stImsIpCapSetInfo), 0x00, sizeof(AT_IMSA_IMS_IP_CAP_SET_INFO_STRU));

    /* 参数检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_WARN_LOG("AT_SetImsIpCapPara: NOT AT_CMD_OPT_SET_PARA_CMD!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数个数不正确 */
    if (2 != gucAtParaIndex)
    {
        AT_WARN_LOG("AT_SetImsIpCapPara: para num is not equal 2!");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (0 != gastAtParaList[0].usParaLen)
    {
        stImsIpCapSetInfo.bitOpIpsecFlag = VOS_TRUE;
        stImsIpCapSetInfo.ulIpsecFlag    = gastAtParaList[0].ulParaValue;
    }

    if (0 != gastAtParaList[1].usParaLen)
    {
        stImsIpCapSetInfo.bitOpKeepAliveFlag    = VOS_TRUE;
        stImsIpCapSetInfo.ulKeepAliveFlag       = gastAtParaList[1].ulParaValue;
    }

    /* 给IMSA发送^IMSIPCAPCFG设置请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                     gastAtClientTab[ucIndex].opId,
                                     ID_AT_IMSA_IMS_IP_CAP_SET_REQ,
                                     &(stImsIpCapSetInfo),
                                     (VOS_SIZE_T)sizeof(AT_IMSA_IMS_IP_CAP_SET_INFO_STRU),
                                     PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetImsIpCapPara: AT_FillAndSndAppReqMsg is error!");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSIPCAPCFG_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvImsaImsIpCapSetCnf(
    VOS_VOID                           *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_IMS_IP_CAP_SET_CNF_STRU    *pstImsIpCapSetCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT8                           ucIndex;

    /* 初始化消息变量 */
    ucIndex              = 0;
    pstImsIpCapSetCnf    = (IMSA_AT_IMS_IP_CAP_SET_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsIpCapSetCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsIpCapSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsIpCapSetCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMSUECAP_SET */
    if (AT_CMD_IMSIPCAPCFG_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaImsIpCapSetCnf: WARNING:Not AT_CMD_IMSIPCAP_SET!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstImsIpCapSetCnf->ulResult)
    {
        ulResult  = AT_OK;
    }
    else
    {
        ulResult  = AT_ERROR;
    }

    gstAtSendData.usBufLen  = 0;

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_QryImsIpCapPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulResult;

    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 给IMSA发送^IMSIPCAPCFG查询请求 */
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                     gastAtClientTab[ucIndex].opId,
                                     ID_AT_IMSA_IMS_IP_CAP_QRY_REQ,
                                     VOS_NULL_PTR,
                                     0,
                                     PS_PID_IMSA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_QryImsIpCapPara: AT_FillAndSndAppReqMsg is error!");
        return AT_ERROR;
    }

    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_IMSIPCAPCFG_QRY;

    return AT_WAIT_ASYNC_RETURN;
}



VOS_UINT32 AT_RcvImsaImsIpCapQryCnf(
    VOS_VOID                           *pMsg
)
{
    /* 定义局部变量 */
    IMSA_AT_IMS_IP_CAP_QRY_CNF_STRU    *pstImsIpCapQryCnf = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;
    VOS_UINT8                           ucIndex;


    /* 初始化消息变量 */
    ucIndex              = 0;

    pstImsIpCapQryCnf    = (IMSA_AT_IMS_IP_CAP_QRY_CNF_STRU *)pMsg;

    /* 通过ClientId获取ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstImsIpCapQryCnf->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsIpCapQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvImsaImsIpCapQryCnf: WARNING:AT_BROADCAST_INDEX!");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_IMSIPCAP_QRY */
    if (AT_CMD_IMSIPCAPCFG_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvImsaImsIpCapQryCnf: WARNING:Not AT_CMD_IMSIPCAPCFG_QRY!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 判断查询操作是否成功 */
    if (VOS_OK == pstImsIpCapQryCnf->ulResult)
    {
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                        (VOS_CHAR *)pgucAtSndCodeAddr,
                                                        (VOS_CHAR *)pgucAtSndCodeAddr,
                                                        "%s: %d,%d",
                                                        g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                        pstImsIpCapQryCnf->ulIpsecFlag,
                                                        pstImsIpCapQryCnf->ulKeepAliveFlag);

        ulResult  = AT_OK;
    }
    else
    {
        gstAtSendData.usBufLen = 0;
        ulResult               = AT_ERROR;
    }

    /* 调用At_FormatResultData发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}



LOCAL VOS_UINT32 AT_CheckCacdcPara(VOS_VOID)
{

    /* 参数有效性检查 */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        AT_WARN_LOG("AT_CheckCacdcPara: At Cmd Opt Set Para Error.");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 参数个数不为TAF_AT_EOPLMN_PARA_NUM，返回AT_CME_INCORRECT_PARAMETERS */
    if ( TAF_MMA_ACDC_PARA_NUM != gucAtParaIndex )
    {
        AT_WARN_LOG("AT_CheckCacdcPara: At Para Num Error.");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 第1个参数检查，长度不等于TAF_MMA_OSID_ORIGINAL_LEN，否则返回AT_CME_INCORRECT_PARAMETERS */
    if (gastAtParaList[0].usParaLen != TAF_MMA_OSID_ORIGINAL_LEN)
    {
        AT_WARN_LOG1("AT_CheckCacdcPara: OsId Error.", gastAtParaList[0].usParaLen);
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 第2个参数检查，长度大于0小于等于TAF_MMA_MAX_APPID_LEN，返回AT_CME_INCORRECT_PARAMETERS */
    if ( (gastAtParaList[1].usParaLen == 0)
      || (gastAtParaList[1].usParaLen > TAF_MMA_MAX_APPID_LEN) )
    {
        AT_WARN_LOG1("AT_CheckCacdcPara: AppId Error.", gastAtParaList[1].usParaLen);
        return AT_CME_INCORRECT_PARAMETERS;
    }

    return AT_SUCCESS;
}


VOS_UINT32 AT_SetCacdcPara(VOS_UINT8 ucIndex)
{
    TAF_MMA_ACDC_APP_INFO_STRU          stAcdcAppInfo;
    VOS_UINT8                           aucOsId[TAF_MMA_OSID_ORIGINAL_LEN];
    VOS_UINT16                          usLen;
    VOS_UINT32                          ulResult;

    usLen = TAF_MMA_OSID_ORIGINAL_LEN;

    TAF_MEM_SET_S(&stAcdcAppInfo, sizeof(stAcdcAppInfo), 0x00, sizeof(TAF_MMA_ACDC_APP_INFO_STRU));
    TAF_MEM_SET_S(aucOsId, sizeof(aucOsId), 0x00, sizeof(aucOsId));

    /* 参数个数和合法性检查,不合法直接返回失败 */
    ulResult = AT_CheckCacdcPara();
    if (AT_SUCCESS != ulResult)
    {
        return ulResult;
    }

    TAF_MEM_CPY_S(aucOsId, TAF_MMA_OSID_ORIGINAL_LEN, gastAtParaList[0].aucPara, TAF_MMA_OSID_ORIGINAL_LEN);

    /* 填写消息结构 */
    /* 填写OSID */
    ulResult = At_AsciiNum2HexString(aucOsId, &usLen);
    if ( (AT_SUCCESS       != ulResult)
      || (TAF_MMA_OSID_LEN != usLen   ) )
    {
        AT_ERR_LOG1("AT_SetCacdcPara: Ascii to Hex Error", usLen);
        return AT_ERROR;
    }

    TAF_MEM_CPY_S(stAcdcAppInfo.aucOsId, TAF_MMA_OSID_LEN, aucOsId, TAF_MMA_OSID_LEN);

    /* 填写APPID */
    VOS_StrNCpy_s((VOS_CHAR*)stAcdcAppInfo.aucAppId,
                  gastAtParaList[1].usParaLen,
                  (VOS_CHAR*)gastAtParaList[1].aucPara,
                  gastAtParaList[1].usParaLen);

    /* 填写enAppIndication */
    stAcdcAppInfo.enAppIndication = gastAtParaList[2].ulParaValue;

    /* 通过ID_TAF_MMA_ACDC_APP_NOTIFY消息来设置ACDC */
    ulResult = TAF_MMA_AcdcAppNotify(WUEPS_PID_AT,
                                     gastAtClientTab[ucIndex].usClientId,
                                     At_GetOpId(),
                                     &stAcdcAppInfo);

    if (VOS_TRUE == ulResult)
    {
        return AT_OK;
    }

    return AT_ERROR;
}



