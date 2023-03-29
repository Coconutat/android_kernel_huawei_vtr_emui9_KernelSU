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



#include "vos.h"
#include "si_pih.h"
#include "si_pb.h"
#include "sitypedef.h"
#include "product_config.h"
#include "AtOamInterface.h"



/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID PS_FILE_ID_PIH_API_C



VOS_UINT32 SI_PIH_IsSvlte(VOS_VOID)
{
    SVLTE_SUPPORT_FLAG_STRU             stSvlteFlag = {0};

    if (VOS_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_SVLTE_FLAG, &stSvlteFlag, sizeof(SVLTE_SUPPORT_FLAG_STRU)))
    {
        return VOS_FALSE;
    }

    return (VOS_UINT32)stSvlteFlag.ucSvlteSupportFlag;

}


VOS_UINT32 SI_PIH_GetReceiverPid(
    MN_CLIENT_ID_T                      ClientId,
    VOS_UINT32                          *pulReceiverPid)
{
    MODEM_ID_ENUM_UINT16    enModemID;

    /* 调用接口获取Modem ID */
    if(VOS_OK != AT_GetModemIdFromClient(ClientId,&enModemID))
    {
        return VOS_ERR;
    }

    if(MODEM_ID_1 == enModemID)
    {
        *pulReceiverPid = I1_MAPS_PIH_PID;
    }
    else if (MODEM_ID_2 == enModemID)
    {
        *pulReceiverPid = I2_MAPS_PIH_PID;
    }
    else
    {
        *pulReceiverPid = I0_MAPS_PIH_PID;
    }

    if (VOS_TRUE == SI_PIH_IsSvlte())
    {
        *pulReceiverPid = I0_MAPS_PIH_PID;
    }

    return VOS_OK;
}



VOS_UINT32 SI_PIH_FdnEnable (
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    VOS_UINT8                           *pPIN2)
{
    SI_PIH_FDN_ENABLE_REQ_STRU     *pMsg;
    VOS_UINT32                      ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_FdnEnable:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }


    pMsg = (SI_PIH_FDN_ENABLE_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_FDN_ENABLE_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_FdnEnable:WARNING AllocMsg FAILED.");
        return TAF_FAILURE;
    }

    pMsg->stMsgHeader.ulReceiverPid = ulReceiverPid;
    pMsg->stMsgHeader.ulMsgName     = SI_PIH_FDN_ENABLE_REQ;
    pMsg->stMsgHeader.usClient      = ClientId;
    pMsg->stMsgHeader.ucOpID        = OpId;
    pMsg->stMsgHeader.ulEventType   = SI_PIH_EVENT_FDN_CNF;

    if(VOS_NULL_PTR != pPIN2)
    {
        PAM_MEM_CPY_S(pMsg->aucPIN2, SI_PIH_PIN_CODE_LEN, pPIN2, SI_PIH_PIN_CODE_LEN);
    }

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_FdnEnable:WARNING SendMsg FAILED.");
        return TAF_FAILURE;
    }

    return TAF_SUCCESS;

}


VOS_UINT32 SI_PIH_FdnDisable (
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    VOS_UINT8                           *pPIN2)
{
    SI_PIH_FDN_DISABLE_REQ_STRU     *pMsg;
    VOS_UINT32                      ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_FdnDisable:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }


    pMsg = (SI_PIH_FDN_DISABLE_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_FDN_DISABLE_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_FdnDisable:WARNING AllocMsg FAILED");
        return TAF_FAILURE;
    }

    pMsg->stMsgHeader.ulReceiverPid = ulReceiverPid;
    pMsg->stMsgHeader.ulMsgName     = SI_PIH_FDN_DISALBE_REQ;
    pMsg->stMsgHeader.usClient      = ClientId;
    pMsg->stMsgHeader.ucOpID        = OpId;
    pMsg->stMsgHeader.ulEventType   = SI_PIH_EVENT_FDN_CNF;

    if(VOS_NULL_PTR != pPIN2)
    {
        PAM_MEM_CPY_S(pMsg->aucPIN2, SI_PIH_PIN_CODE_LEN, pPIN2, SI_PIH_PIN_CODE_LEN);
    }

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_FdnDisable:WARNING SendMsg FAILED");
        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_FdnBdnQuery(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_QUERY_TYPE_ENUM_UINT32       enQueryType)
{
    SI_PIH_MSG_HEADER_STRU          *pMsg;
    VOS_UINT32                      ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_FdnBdnQuery:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pMsg = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                        sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_FdnDisable:WARNING AllocMsg FAILED");
        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;

    if( SI_PIH_FDN_QUERY == enQueryType )
    {
        pMsg->ulEventType   = SI_PIH_EVENT_FDN_CNF;
        pMsg->ulMsgName     = SI_PIH_FDN_QUERY_REQ;
    }
    else
    {
        pMsg->ulEventType   = SI_PIH_EVENT_BDN_CNF;
        pMsg->ulMsgName     = SI_PIH_BDN_QUERY_REQ;
    }

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_FdnDisable:WARNING SendMsg FAILED");
        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_GenericAccessReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_CSIM_COMMAND_STRU            *pstData)
{
    SI_PIH_GACCESS_REQ_STRU         *pMsg;
    VOS_UINT32                      ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_GenericAccessReq:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    if(pstData->ulLen == 0)
    {
        PIH_ERROR_LOG("SI_PIH_GenericAccessReq: Data Len is Error");
        return TAF_FAILURE;
    }

    if(  (pstData->ulLen > SI_PIH_APDU_HDR_LEN)
        &&(pstData->aucCommand[4] != pstData->ulLen-SI_PIH_APDU_HDR_LEN))
    {
        PIH_ERROR_LOG("SI_PIH_GenericAccessReq: Data Len is Not Eq P3");
        return TAF_FAILURE;
    }

    pMsg = (SI_PIH_GACCESS_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                sizeof(SI_PIH_GACCESS_REQ_STRU)-VOS_MSG_HEAD_LENGTH+pstData->ulLen);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_GenericAccessReq: AllocMsg FAILED");
        return TAF_FAILURE;
    }

    pMsg->stMsgHeader.ulReceiverPid = ulReceiverPid;
    pMsg->stMsgHeader.ulMsgName     = SI_PIH_GACCESS_REQ;
    pMsg->stMsgHeader.usClient      = ClientId;
    pMsg->stMsgHeader.ucOpID        = OpId;
    pMsg->stMsgHeader.ulEventType   = SI_PIH_EVENT_GENERIC_ACCESS_CNF;
    pMsg->ulDataLen                 = pstData->ulLen;

    if(0 != pstData->ulLen)
    {
        PAM_MEM_CPY_S(pMsg->aucData, pstData->ulLen, pstData->aucCommand, pstData->ulLen);
    }

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_FdnDisable:WARNING SendMsg FAILED");
        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_IsdbAccessReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_ISDB_ACCESS_COMMAND_STRU    *pstData)
{
    SI_PIH_ISDB_ACCESS_REQ_STRU     *pstMsg;
    VOS_UINT32                      ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_IsdbAccessReq:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    if (0 == pstData->ulLen)
    {
        PIH_ERROR_LOG("SI_PIH_IsdbAccessReq: Data Len is Error");

        return TAF_FAILURE;
    }

    pstMsg  = (SI_PIH_ISDB_ACCESS_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                                                   sizeof(SI_PIH_ISDB_ACCESS_REQ_STRU) - VOS_MSG_HEAD_LENGTH + pstData->ulLen);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_IsdbAccessReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName       =   SI_PIH_ISDB_ACCESS_REQ;
    pstMsg->stMsgHeader.usClient        =   ClientId;
    pstMsg->stMsgHeader.ucOpID          =   OpId;
    pstMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_ISDB_ACCESS_CNF;
    pstMsg->ulDataLen                   =   pstData->ulLen;

    if (0 != pstData->ulLen)
    {
        PAM_MEM_CPY_S(pstMsg->aucData, pstData->ulLen, pstData->aucCommand, pstData->ulLen);
    }

    if (VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_IsdbAccessReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_CchoSetReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_CCHO_COMMAND_STRU           *pstCchoCmd)
{
    SI_PIH_CCHO_SET_REQ_STRU           *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    /* 参数检测 */
    if ((0 == pstCchoCmd->ulAIDLen)
        || ((USIMM_AID_LEN_MAX*2) < pstCchoCmd->ulAIDLen))
    {
        PIH_ERROR_LOG("SI_PIH_CchoSetReq: AID length is incorrect.");

        return TAF_FAILURE;
    }

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CchoSetReq:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstMsg  = (SI_PIH_CCHO_SET_REQ_STRU*)VOS_AllocMsg(WUEPS_PID_AT,
                            (VOS_UINT32)(sizeof(SI_PIH_CCHO_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CchoSetReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName       =   SI_PIH_CCHO_SET_REQ;
    pstMsg->stMsgHeader.usClient        =   ClientId;
    pstMsg->stMsgHeader.ucOpID          =   OpId;
    pstMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_CCHO_SET_CNF;
    pstMsg->ulAIDLen                    =   pstCchoCmd->ulAIDLen;

    PAM_MEM_CPY_S(pstMsg->aucADFName, pstCchoCmd->ulAIDLen, pstCchoCmd->pucADFName, pstCchoCmd->ulAIDLen);

    if (VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CchoSetReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;

}


VOS_UINT32 SI_PIH_CchpSetReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_CCHP_COMMAND_STRU           *pstCchpCmd)
{
    SI_PIH_CCHP_SET_REQ_STRU           *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    /* 参数检测 */
    if ((0 == pstCchpCmd->ulAIDLen)
      || ((USIMM_AID_LEN_MAX*2) < pstCchpCmd->ulAIDLen))
    {
        PIH_ERROR_LOG("SI_PIH_CchpSetReq: AID length is incorrect.");

        return TAF_FAILURE;
    }

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CchpSetReq:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstMsg  = (SI_PIH_CCHP_SET_REQ_STRU*)VOS_AllocMsg(WUEPS_PID_AT,
                            (VOS_UINT32)(sizeof(SI_PIH_CCHP_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CchpSetReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName       =   SI_PIH_CCHP_SET_REQ;
    pstMsg->stMsgHeader.usClient        =   ClientId;
    pstMsg->stMsgHeader.ucOpID          =   OpId;
    pstMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_CCHP_SET_CNF;
    pstMsg->ucAPDUP2                    =   pstCchpCmd->ucAPDUP2;
    pstMsg->ulAIDLen                    =   pstCchpCmd->ulAIDLen;

    PAM_MEM_CPY_S(pstMsg->aucADFName, pstCchpCmd->ulAIDLen, pstCchpCmd->pucADFName, pstCchpCmd->ulAIDLen);

    if (VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CchpSetReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;

}



VOS_UINT32 SI_PIH_CchcSetReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    VOS_UINT32                          ulSessionID)
{
    SI_PIH_CCHC_SET_REQ_STRU           *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CchcSetReq:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstMsg  = (SI_PIH_CCHC_SET_REQ_STRU*)VOS_AllocMsg(WUEPS_PID_AT,
                        sizeof(SI_PIH_CCHC_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CchcSetReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName       =   SI_PIH_CCHC_SET_REQ;
    pstMsg->stMsgHeader.usClient        =   ClientId;
    pstMsg->stMsgHeader.ucOpID          =   OpId;
    pstMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_CCHC_SET_CNF;
    pstMsg->ulSessionID                 =   ulSessionID;

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CchcSetReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;

}


VOS_UINT32 SI_PIH_CglaSetReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_CGLA_COMMAND_STRU           *pstData)
{
    SI_PIH_CGLA_REQ_STRU               *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    /* 参数检测 */
    if ((SI_APDU_MAX_LEN + 1) < pstData->ulLen)
    {
        PIH_ERROR_LOG("SI_PIH_CglaSetReq:Command length is incorrect.");

        return TAF_FAILURE;
    }

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CglaSetReq:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstMsg  = (SI_PIH_CGLA_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                                                   sizeof(SI_PIH_CGLA_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CglaSetReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName       =   SI_PIH_CGLA_SET_REQ;
    pstMsg->stMsgHeader.usClient        =   ClientId;
    pstMsg->stMsgHeader.ucOpID          =   OpId;
    pstMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_CGLA_SET_CNF;
    pstMsg->ulSessionID                 =   pstData->ulSessionID;
    pstMsg->ulDataLen                   =   pstData->ulLen;

    PAM_MEM_CPY_S(pstMsg->aucData, pstData->ulLen, pstData->pucCommand, pstData->ulLen);

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CglaSetReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_GetCardATRReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    SI_PIH_MSG_HEADER_STRU             *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CchoSetReq:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstMsg  = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                                                   sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CglaSetReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->ulReceiverPid   =   ulReceiverPid;
    pstMsg->ulMsgName       =   SI_PIH_CARD_ATR_QRY_REQ;
    pstMsg->usClient        =   ClientId;
    pstMsg->ucOpID          =   OpId;
    pstMsg->ulEventType     =   SI_PIH_EVENT_CARD_ATR_QRY_CNF;

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CglaSetReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}



VOS_UINT32 SI_PIH_HvSstSet (
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_HVSST_SET_STRU              *pstHvSStSet)
{
    SI_PIH_HVSST_REQ_STRU   *pMsg;
    VOS_UINT32               ulReceiverPid;

    if(VOS_NULL_PTR == pstHvSStSet)
    {
        PIH_WARNING_LOG("SI_PIH_HvSstSet:Parameter is Wrong");

        return TAF_FAILURE;
    }

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_HvSstSet:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pMsg = (SI_PIH_HVSST_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_HVSST_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_HvSstSet:WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pMsg->stMsgHeader.ulReceiverPid = ulReceiverPid;
    pMsg->stMsgHeader.usClient      = ClientId;
    pMsg->stMsgHeader.ucOpID        = OpId;
    pMsg->stMsgHeader.ulMsgName     = SI_PIH_HVSST_SET_REQ;
    pMsg->stMsgHeader.ulEventType   = SI_PIH_EVENT_HVSST_SET_CNF;

    PAM_MEM_CPY_S(&pMsg->stHvSSTData, sizeof(SI_PIH_HVSST_SET_STRU), pstHvSStSet, sizeof(SI_PIH_HVSST_SET_STRU));

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_HvSstSet:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_HvSstQuery(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    SI_PIH_MSG_HEADER_STRU *pMsg;
    VOS_UINT32              ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_HvSstQuery:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pMsg = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_HvSstQuery:WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;
    pMsg->ulMsgName     = SI_PIH_HVSST_QUERY_REQ;
    pMsg->ulEventType   = SI_PIH_EVENT_HVSST_QUERY_CNF;

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_HvSstQuery:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}



VOS_UINT32 SI_PIH_SciCfgSet (
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_CARD_SLOT_ENUM_UINT32        enCard0Slot,
    SI_PIH_CARD_SLOT_ENUM_UINT32        enCard1Slot,
    SI_PIH_CARD_SLOT_ENUM_UINT32        enCard2Slot
)
{
    SI_PIH_SCICFG_SET_REQ_STRU  *pstMsg;
    VOS_UINT32                  ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_SciCfgSet:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    pstMsg = (SI_PIH_SCICFG_SET_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_SCICFG_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_SciCfgSet:WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   = ulReceiverPid;
    pstMsg->stMsgHeader.usClient        = ClientId;
    pstMsg->stMsgHeader.ucOpID          = OpId;
    pstMsg->stMsgHeader.ulMsgName       = SI_PIH_SCICFG_SET_REQ;
    pstMsg->stMsgHeader.ulEventType     = SI_PIH_EVENT_SCICFG_SET_CNF;

    pstMsg->enCard0Slot                 = enCard0Slot;
    pstMsg->enCard1Slot                 = enCard1Slot;
    pstMsg->enCard2Slot                 = enCard2Slot;

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_SciCfgSet:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}



VOS_UINT32 SI_PIH_SciCfgQuery (
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    SI_PIH_MSG_HEADER_STRU *pstMsg;
    VOS_UINT32              ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_SciCfgQuery:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    pstMsg = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_SciCfgQuery:WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->ulReceiverPid = ulReceiverPid;
    pstMsg->usClient      = ClientId;
    pstMsg->ucOpID        = OpId;
    pstMsg->ulMsgName     = SI_PIH_SCICFG_QUERY_REQ;
    pstMsg->ulEventType   = SI_PIH_EVENT_SCICFG_QUERY_CNF;

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_SciCfgQuery:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_VOID SI_PIH_AcpuInit(VOS_VOID)
{
    VOS_UINT8    aucUUID[] = {0x47,0x91,0xe8,0xab,
                                0x61,0xcd,
                                0x3f,0xf4,
                                0x71,0xc4,0x1a,0x31,0x7e,0x40,0x53,0x12};

    if (VOS_OK != TC_NS_RegisterServiceCallbackFunc((VOS_CHAR*)aucUUID,
                                                    SI_PIH_TEETimeOutCB,
                                                    VOS_NULL_PTR))
    {
        vos_printf("SI_PIH_AcpuInit: Reg TEE Timeout CB FUN Fail\r\n");
    }

    vos_printf("SI_PIH_AcpuInit: Reg TEE Timeout CB FUN\r\n");

    return;
}


VOS_UINT32 SI_PIH_GetSecIccVsimVer(VOS_VOID)
{
    return SI_PIH_SEC_ICC_VSIM_VER;
}



VOS_VOID SI_PIH_TEETimeOutCB (
    TEEC_TIMER_PROPERTY_STRU            *pstTimerData
)
{
    MN_APP_PIH_AT_CNF_STRU *pstMsg;

    pstMsg = (MN_APP_PIH_AT_CNF_STRU*)VOS_AllocMsg(MAPS_PIH_PID,
                                                sizeof(MN_APP_PIH_AT_CNF_STRU)-VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_ERROR_LOG("SI_PIH_TEETimeOutCB: Alloc Msg Failed!");

        return ;
    }

    pstMsg->stPIHAtEvent.EventType                  = SI_PIH_EVENT_TEETIMEOUT_IND;

    pstMsg->stPIHAtEvent.PIHError                   = TAF_ERR_NO_ERROR;

    pstMsg->stPIHAtEvent.PIHEvent.TEETimeOut.ulData = pstTimerData->time_type;

    pstMsg->ulReceiverPid                           = WUEPS_PID_AT;

    pstMsg->ulMsgId                                 = PIH_AT_EVENT_CNF;

    pstMsg->stPIHAtEvent.ClientId                   = (MN_CLIENT_ALL&AT_BROADCAST_CLIENT_ID_MODEM_0);

    (VOS_VOID)VOS_SendMsg(MAPS_PIH_PID, pstMsg);

    return ;
}


VOS_UINT32 SI_PIH_HvCheckCardQuery(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    SI_PIH_HVCHECKCARD_REQ_STRU         *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_HvCheckCardQuery:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pstMsg = (SI_PIH_HVCHECKCARD_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_HVCHECKCARD_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_HvCheckCardQuery: AllocMsg FAILED");
        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid = ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName     = SI_PIH_HVCHECKCARD_REQ;
    pstMsg->stMsgHeader.usClient      = ClientId;
    pstMsg->stMsgHeader.ucOpID        = OpId;
    pstMsg->stMsgHeader.ulEventType   = SI_PIH_EVENT_HVCHECKCARD_CNF;

    if(VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_HvCheckCardQuery:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}



VOS_UINT32 SI_PIH_UiccAuthReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_UICCAUTH_STRU                *pstData)
{
    SI_PIH_UICCAUTH_REQ_STRU        *pstMsg;
    VOS_UINT32                      ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_UiccAuthReq:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    if (VOS_NULL_PTR == pstData)
    {
        PIH_WARNING_LOG("SI_PIH_UiccAuthReq:Para Check Error");

        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstMsg  = (SI_PIH_UICCAUTH_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_UICCAUTH_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_UiccAuthReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName       =   SI_PIH_UICCAUTH_REQ;
    pstMsg->stMsgHeader.usClient        =   ClientId;
    pstMsg->stMsgHeader.ucOpID          =   OpId;
    pstMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_UICCAUTH_CNF;

    PAM_MEM_CPY_S(&pstMsg->stAuthData, sizeof(SI_PIH_UICCAUTH_STRU), pstData, sizeof(SI_PIH_UICCAUTH_STRU));

    if (VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_UiccAuthReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_AccessUICCFileReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_ACCESSFILE_STRU              *pstData)
{
    SI_PIH_ACCESSFILE_REQ_STRU          *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_AccessUICCFileReq:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    if (VOS_NULL_PTR == pstData)
    {
        PIH_WARNING_LOG("SI_PIH_AccessUICCFileReq:Para Check Error");

        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstMsg  = (SI_PIH_ACCESSFILE_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                    (VOS_UINT32)(sizeof(SI_PIH_ACCESSFILE_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_AccessUICCFileReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName       =   SI_PIH_URSM_REQ;
    pstMsg->stMsgHeader.usClient        =   ClientId;
    pstMsg->stMsgHeader.ucOpID          =   OpId;
    pstMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_URSM_CNF;

    PAM_MEM_CPY_S(&pstMsg->stCmdData, sizeof(SI_PIH_ACCESSFILE_STRU), pstData, sizeof(SI_PIH_ACCESSFILE_STRU));

    if (VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_AccessUICCFileReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_CardTypeQuery(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    SI_PIH_MSG_HEADER_STRU *pMsg;
    VOS_UINT32              ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CardTypeQuery: Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pMsg = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                        (VOS_UINT32)sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CardTypeQuery: WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;
    pMsg->ulMsgName     = SI_PIH_CARDTYPE_QUERY_REQ;
    pMsg->ulEventType   = SI_PIH_EVENT_CARDTYPE_QUERY_CNF;

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CardTypeQuery: WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_CardVoltageQuery(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    SI_PIH_MSG_HEADER_STRU *pMsg;
    VOS_UINT32              ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CardVoltageQuery: Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pMsg = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                        (VOS_UINT32)(sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH));

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CardVoltageQuery: WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;
    pMsg->ulMsgName     = SI_PIH_CARDVOLTAGE_QUERY_REQ;
    pMsg->ulEventType   = SI_PIH_EVENT_CARDVOLTAGE_QUERY_CNF;

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CardVoltageQuery: WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_PrivateCglaSetReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_PIH_CGLA_COMMAND_STRU           *pstData)
{
    SI_PIH_CGLA_REQ_STRU               *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CglaHandleReq:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstMsg  = (SI_PIH_CGLA_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
                                            (VOS_UINT32)(sizeof(SI_PIH_CGLA_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CglaHandleReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstMsg->stMsgHeader.ulMsgName       =   SI_PIH_PRIVATECGLA_SET_REQ;
    pstMsg->stMsgHeader.usClient        =   ClientId;
    pstMsg->stMsgHeader.ucOpID          =   OpId;
    pstMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_PRIVATECGLA_SET_CNF;
    pstMsg->ulSessionID                 =   pstData->ulSessionID;
    pstMsg->ulDataLen                   =   pstData->ulLen;

    PAM_MEM_CPY_S(pstMsg->aucData, pstData->ulLen, pstData->pucCommand, pstData->ulLen);

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CglaHandleReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_CrsmSetReq(
    MN_CLIENT_ID_T                          ClientId,
    MN_OPERATION_ID_T                       OpId,
    SI_PIH_CRSM_STRU                       *pstCrsmPara
)
{
    SI_PIH_CRSM_SET_REQ_STRU           *pstCrsmMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CrsmSetReq:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstCrsmMsg  = (SI_PIH_CRSM_SET_REQ_STRU*)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_CRSM_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstCrsmMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CrsmSetReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstCrsmMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstCrsmMsg->stMsgHeader.ulMsgName       =   SI_PIH_CRSM_SET_REQ;
    pstCrsmMsg->stMsgHeader.usClient        =   ClientId;
    pstCrsmMsg->stMsgHeader.ucOpID          =   OpId;
    pstCrsmMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_CRSM_SET_CNF;

    PAM_MEM_CPY_S(&(pstCrsmMsg->stMsgContent), sizeof(SI_PIH_CRSM_STRU), pstCrsmPara, sizeof(SI_PIH_CRSM_STRU));

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pstCrsmMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CrsmSetReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_CrlaSetReq(
    MN_CLIENT_ID_T                          ClientId,
    MN_OPERATION_ID_T                       OpId,
    SI_PIH_CRLA_STRU                       *pstCrlaPara
)
{
    SI_PIH_CRLA_SET_REQ_STRU           *pstCrlaMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CrlaSetReq:Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    /* 分配消息内存 */
    pstCrlaMsg  = (SI_PIH_CRLA_SET_REQ_STRU*)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_CRLA_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstCrlaMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CrlaSetReq: AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstCrlaMsg->stMsgHeader.ulReceiverPid   =   ulReceiverPid;
    pstCrlaMsg->stMsgHeader.ulMsgName       =   SI_PIH_CRLA_SET_REQ;
    pstCrlaMsg->stMsgHeader.usClient        =   ClientId;
    pstCrlaMsg->stMsgHeader.ucOpID          =   OpId;
    pstCrlaMsg->stMsgHeader.ulEventType     =   SI_PIH_EVENT_CRLA_SET_CNF;

    PAM_MEM_CPY_S(&(pstCrlaMsg->stMsgContent), sizeof(SI_PIH_CRLA_STRU), pstCrlaPara, sizeof(SI_PIH_CRLA_STRU));

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pstCrlaMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CchcSetReq:WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_CardSessionQuery(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId
)
{
    SI_PIH_MSG_HEADER_STRU             *pMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CardSessionQuery: Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pMsg = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CardSessionQuery: WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;
    pMsg->ulMsgName     = SI_PIH_SESSION_QRY_REQ;
    pMsg->ulEventType   = SI_PIH_EVENT_SESSION_QRY_CNF;

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CardSessionQuery: WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_CimiSetReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId
)
{
    SI_PIH_MSG_HEADER_STRU             *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CardSessionQuery: Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pstMsg = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CimiSetReq: WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pstMsg->ulReceiverPid = ulReceiverPid;
    pstMsg->usClient      = ClientId;
    pstMsg->ucOpID        = OpId;
    pstMsg->ulMsgName     = SI_PIH_CIMI_QRY_REQ;
    pstMsg->ulEventType   = SI_PIH_EVENT_CIMI_QRY_CNF;

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CimiSetReq: WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


VOS_UINT32 SI_PIH_CCimiSetReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId
)
{
    SI_PIH_MSG_HEADER_STRU             *pMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != SI_PIH_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PIH_ERROR_LOG("SI_PIH_CardSessionQuery: Get ulReceiverPid Error.");
        return TAF_FAILURE;
    }

    pMsg = (SI_PIH_MSG_HEADER_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PIH_MSG_HEADER_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PIH_WARNING_LOG("SI_PIH_CardSessionQuery: WARNING AllocMsg FAILED");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;
    pMsg->ulMsgName     = SI_PIH_CCIMI_QRY_REQ;
    pMsg->ulEventType   = SI_PIH_EVENT_CCIMI_QRY_CNF;

    if (VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PIH_WARNING_LOG("SI_PIH_CardSessionQuery: WARNING SendMsg FAILED");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}








