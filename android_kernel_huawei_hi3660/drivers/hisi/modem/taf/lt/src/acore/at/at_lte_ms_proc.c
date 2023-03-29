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

#include "osm.h"
#include "gen_msg.h"
#include "at_lte_common.h"
/*lint --e{7,64,537,545,322,718,746,958,734,830,587,713,516}*/
#include "gen_msg.h"
//#include "AtNdisInterface.h"
#include "LPsNvInterface.h"
#include "LNvCommon.h"

#include "LPsNvInterface.h"

/*lint -e767 原因:Log打印*/
#define    THIS_FILE_ID        MSP_FILE_ID_AT_LTE_MS_PROC_C
/*lint +e767 */

VOS_UINT32                 g_ulNVRDTL = 0;
VOS_UINT32                 g_ulNVWRTL = 0;

extern VOS_UINT32 AT_NVWRGetParaInfo( AT_PARSE_PARA_TYPE_STRU * pstPara, VOS_UINT8 * pu8Data, VOS_UINT32 * pulLen);
extern VOS_BOOL AT_IsNVWRAllowedNvId(VOS_UINT16 usNvId);



// *****************************************************************************
// oˉêy??3?: atSetNVRDLenPara
// 1|?ü?èê?: ?áNV3¤?èY
//
// 2?êy?μ?÷:
//   ulIndex [in] ó??§?÷òy
//
// ·μ ?? ?μ:
//    TODO: ...
//
// μ÷ó?òa?ó: TODO: ...
// μ÷ó??ùày: TODO: ...
// ×÷    ??: óú???1?/00193980 [2013-02-24]
// *****************************************************************************
VOS_UINT32 atSetNVRDLenPara(VOS_UINT8 ucClientId)
{
    VOS_UINT32 ulRet = ERR_MSP_FAILURE;
    VOS_UINT16 usNVID = 0;
    VOS_UINT32 ulNVLen = 0;


    /*PCLINT*/
    if(ucClientId == 0)
    {

    }

    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        g_ulNVRDTL = 1;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if(gucAtParaIndex > 1)
    {
        g_ulNVRDTL = 2;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if(0 == gastAtParaList[0].usParaLen)
    {
        g_ulNVRDTL = 3;
        return AT_CME_INCORRECT_PARAMETERS;
    }
    else
    {
        usNVID = (VOS_UINT16)gastAtParaList[0].ulParaValue;
    }
    ulRet = NV_GetLength(usNVID, (VOS_UINT32 *)&ulNVLen);
	//vos_printf("\n usNVID=0x%x ulNVLen =0x%x\n",usNVID,ulNVLen);
    //vos_printf("\n nvim_GetItemLenById ulRet=0x%x \n",ulRet);
   // HAL_DIAG_SDM_FUN(EN_SDM_NVRD_GETNVLEN, ulRet, usNVID, ulNVLen);

    if(ERR_MSP_SUCCESS != ulRet)
    {
        g_ulNVRDTL = 4;
        return AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,(VOS_CHAR*)pgucAtSndCodeAddr,
        "^NVRDLEN: %d", ulNVLen);
    g_ulNVRDTL = 7;
    return AT_OK;
}

VOS_UINT32 atSetNVRDExPara(VOS_UINT8 ucClientId)
{
    VOS_UINT32 ulRet = ERR_MSP_FAILURE;
    VOS_UINT16 usNVID = 0;
    VOS_UINT32 ulNVLen = 0;
    VOS_UINT16 usOffset = 0;
    VOS_UINT16 usRdLen = 0;
    VOS_UINT8 * pu8Data = NULL;
    VOS_UINT32 i = 0;

    /*PCLINT*/
    if(ucClientId == 0)
    {

    }

    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        g_ulNVRDTL = 1;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if(3 != gucAtParaIndex)
    {
        g_ulNVRDTL = 2;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if(0 == gastAtParaList[0].usParaLen || 0 == gastAtParaList[1].usParaLen || 0 == gastAtParaList[2].usParaLen)
    {
        g_ulNVRDTL = 3;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    usNVID = (VOS_UINT16)gastAtParaList[0].ulParaValue;
    usOffset = (VOS_UINT16)gastAtParaList[1].ulParaValue;
    usRdLen = (VOS_UINT16)gastAtParaList[2].ulParaValue;

    ulRet = NV_GetLength(usNVID, (VOS_UINT32 *)&ulNVLen);
	//vos_printf("\n usNVID=0x%x ulNVLen =0x%x\n",usNVID,ulNVLen);
    //vos_printf("\n nvim_GetItemLenById ulRet=0x%x \n",ulRet);
//    HAL_DIAG_SDM_FUN(EN_SDM_NVRD_GETNVLEN, ulRet, usNVID, ulNVLen);

    if(ERR_MSP_SUCCESS != ulRet)
    {
        g_ulNVRDTL = 4;
        return AT_ERROR;
    }

    if(usOffset > ulNVLen - 1 || usOffset + usRdLen > ulNVLen)
    {
    	g_ulNVRDTL = 5;
       return AT_ERROR;
    }

/*    ulNVLen = (ulNVLen > 128)?128:ulNVLen;*/

    pu8Data = VOS_MemAlloc(WUEPS_PID_AT, (DYNAMIC_MEM_PT), ulNVLen);
    if(NULL == pu8Data)
    {
        g_ulNVRDTL = 6;
        return AT_ERROR;
    }

    ulRet = NVM_Read((VOS_UINT32)usNVID, (VOS_VOID*)pu8Data, ulNVLen);
    //vos_printf("NVM_Read ulRet = 0x%x",ulRet);
    //HAL_DIAG_SDM_FUN(EN_SDM_NVRD_READNVITEM, ulRet, usNVID, ulNVLen);

    if(ERR_MSP_SUCCESS != ulRet)
    {
        VOS_MemFree(WUEPS_PID_AT, pu8Data);
        g_ulNVRDTL = 7;
        return AT_ERROR;
    }
    gstAtSendData.usBufLen = 0;
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,(VOS_CHAR*)pgucAtSndCodeAddr,
        "^NVRDEX: %d,%d,%d,", usNVID, usOffset, usRdLen);

    for(i = usOffset; i < usOffset + usRdLen; i++)
    {
        if(usOffset == i)
        {
            gstAtSendData.usBufLen += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                (VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR*)pgucAtSndCodeAddr + gstAtSendData.usBufLen,"%02X", pu8Data[i]);
        }
        else
        {
            gstAtSendData.usBufLen += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                (VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR*)pgucAtSndCodeAddr + gstAtSendData.usBufLen," %02X", pu8Data[i]);
        }
    }

    VOS_MemFree(WUEPS_PID_AT, pu8Data);
    g_ulNVRDTL = 8;
    return AT_OK;
}


VOS_UINT32 atSetNVWRExPara(VOS_UINT8 ucClientId)
{
    VOS_UINT32 ulRet = ERR_MSP_FAILURE;
    VOS_UINT32 ulNVID = 0;
    VOS_UINT32 ulNVWrTotleLen = 0;
    VOS_UINT32 ulNVLen = 0;
    VOS_UINT16 usNVWrLen = 0;
    static VOS_UINT8 * pu8NvWrData = NULL;
    VOS_UINT32 ulNVNum = 0;
    VOS_UINT32 i = 0;
    VOS_UINT8 au8Data[128] = {0};//MAX_NV_NUM_PER_PARA

    VOS_UINT16 usOffset = 0;
    gstAtSendData.usBufLen = 0;

    /*PCLINT*/
    if(ucClientId == 0)
    {

    }

    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        g_ulNVWRTL =1;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if((0 == gastAtParaList[0].usParaLen)
        || (0 == gastAtParaList[1].usParaLen)
        || (0 == gastAtParaList[2].usParaLen)
        || (0 == gastAtParaList[3].usParaLen))
    {
        g_ulNVWRTL =2;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulNVID = gastAtParaList[0].ulParaValue;
    /*HAL_SDMLOG("\n atSetNVWRPara ulNVID = %d\n",ulNVID);*/

    if (VOS_TRUE != AT_IsNVWRAllowedNvId((VOS_UINT16)ulNVID))
    {
        g_ulNVRDTL = 3;
        return AT_CME_OPERATION_NOT_ALLOWED;
    }

    usOffset = (VOS_UINT16)gastAtParaList[1].ulParaValue;

    usNVWrLen = (VOS_UINT16)gastAtParaList[2].ulParaValue;
    HAL_SDMLOG("\n atSetNVWRPara usNVWrLen = %d\n",(VOS_INT)usNVWrLen);

    ulRet = NV_GetLength(ulNVID, (VOS_UINT32 *)&ulNVWrTotleLen);

    //HAL_DIAG_SDM_FUN(EN_SDM_NVRD_GETNVLEN, ulRet, ulNVID, ulNVWrTotleLen);
    if(ERR_MSP_SUCCESS != ulRet)
    {
        g_ulNVRDTL = 4;
        return AT_ERROR;
    }

    if((usOffset > (ulNVWrTotleLen - 1)) || ((usOffset + usNVWrLen) > ulNVWrTotleLen))
    {
        g_ulNVRDTL = 5;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if(0 == usOffset)
    {
        if(0 != pu8NvWrData)
        {
            VOS_MemFree(WUEPS_PID_AT, pu8NvWrData);
            pu8NvWrData = 0;
        }

        pu8NvWrData = VOS_MemAlloc(WUEPS_PID_AT, (DYNAMIC_MEM_PT), ulNVWrTotleLen);
        if(NULL == pu8NvWrData)
        {
            g_ulNVWRTL =6;
            return AT_ERROR;
        }
    }

    if(NULL == pu8NvWrData)
    {
         return AT_CME_INCORRECT_PARAMETERS;
    }

    i = 0;
    while(0 != gastAtParaList[(VOS_UINT32)(3 + i)].usParaLen)
    {
        ulRet = AT_NVWRGetParaInfo((AT_PARSE_PARA_TYPE_STRU*)(&(gastAtParaList[(VOS_UINT32)(3 + i)])), au8Data, (VOS_UINT32 *)&ulNVNum);
        if(ERR_MSP_SUCCESS != ulRet)
        {
            VOS_MemFree(WUEPS_PID_AT, pu8NvWrData);
            pu8NvWrData = 0;
            g_ulNVWRTL =7;
            return AT_CME_INCORRECT_PARAMETERS;
        }

        if(ulNVNum > 128)//MAX_NV_NUM_PER_PARA
        {
            VOS_MemFree(WUEPS_PID_AT, pu8NvWrData);
            pu8NvWrData = 0;
            g_ulNVWRTL =8;
            return AT_CME_INCORRECT_PARAMETERS;
        }

        if((ulNVLen+ulNVNum) > usNVWrLen)
        {
            VOS_MemFree(WUEPS_PID_AT, pu8NvWrData);
            pu8NvWrData = 0;
            g_ulNVWRTL =9;
            return AT_CME_INCORRECT_PARAMETERS;
        }

        MSP_MEMCPY((pu8NvWrData + usOffset + ulNVLen), ulNVWrTotleLen - usOffset - ulNVLen, au8Data, ulNVNum);

        ulNVLen += ulNVNum;
        i++;

        if(i >= (AT_MAX_PARA_NUMBER-3))
        {
            break;
        }
    }

    if(ulNVLen != usNVWrLen)
    {
        VOS_MemFree(WUEPS_PID_AT, pu8NvWrData);
        pu8NvWrData = 0;
        g_ulNVWRTL =10;
        return AT_CME_INCORRECT_PARAMETERS;
    }
    if (usOffset + usNVWrLen == ulNVWrTotleLen)
    {
        ulRet = NVM_Write(ulNVID, (VOS_VOID*)pu8NvWrData, ulNVWrTotleLen);

        //HAL_DIAG_SDM_FUN(EN_SDM_NVWR_WRITENVITEM, ulRet, ulNVID, ulNVWrTotleLen);

        if(ERR_MSP_SUCCESS != ulRet)
        {
            VOS_MemFree(WUEPS_PID_AT, pu8NvWrData);
            pu8NvWrData = 0;
            g_ulNVWRTL =11;

            return AT_ERROR;
        }
        ulRet = mdrv_nv_flush();
        if(MDRV_OK != ulRet)
        {
            VOS_MemFree(WUEPS_PID_AT, pu8NvWrData);
            pu8NvWrData = 0;
            g_ulNVWRTL =12;

            return AT_ERROR;
        }
        VOS_MemFree(WUEPS_PID_AT, pu8NvWrData);
        pu8NvWrData = 0;
    }
    g_ulNVWRTL =9;

    return AT_OK;
}


VOS_UINT32 AT_QryLwclashPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32 ulRet = ERR_MSP_SUCCESS;
    L4A_READ_LWCLASH_REQ_STRU stLwclash = {0};

    stLwclash.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;

    stLwclash.stCtrl.ulOpId = 0;
    stLwclash.stCtrl.ulPid = WUEPS_PID_AT;

    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_LWCLASHQRY_REQ,
                             (VOS_VOID*)(&stLwclash),
                             sizeof(L4A_READ_LWCLASH_REQ_STRU));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = (AT_CMD_CURRENT_OPT_ENUM)AT_CMD_LWCLASH_READ;
        return AT_WAIT_ASYNC_RETURN;
    }
    return AT_ERROR;
}


VOS_UINT32 AT_QryLcacellPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32 ulRet = ERR_MSP_SUCCESS;
    L4A_READ_LCACELL_REQ_STRU stLcacell = {0};

    stLcacell.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;

    stLcacell.stCtrl.ulOpId = 0;
    stLcacell.stCtrl.ulPid = WUEPS_PID_AT;

    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_LCACELLQRY_REQ,
                             (VOS_VOID*)(&stLcacell),
                             sizeof(L4A_READ_LCACELL_REQ_STRU));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = (AT_CMD_CURRENT_OPT_ENUM)AT_CMD_LCACELL_QUERY;
        return AT_WAIT_ASYNC_RETURN;
    }
    return AT_ERROR;
}

VOS_UINT32 AT_SetRadverPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32 ulRet;
    L4A_SET_RADVER_REQ_STRU stReq = {0};

    if((2 != gucAtParaIndex) || (0 == gastAtParaList[0].usParaLen) || (0 == gastAtParaList[1].usParaLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stReq.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;
    stReq.stCtrl.ulOpId     = 0;
    stReq.stCtrl.ulPid      = WUEPS_PID_AT;

    stReq.ulMod             = gastAtParaList[0].ulParaValue;
    stReq.ulVer             = gastAtParaList[1].ulParaValue;

    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_RADVER_SET_REQ,
                             (VOS_VOID*)(&stReq),
                             sizeof(stReq));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = (AT_CMD_CURRENT_OPT_ENUM)AT_CMD_RADVER_SET;
        return AT_WAIT_ASYNC_RETURN;
    }

    return AT_ERROR;
}

VOS_UINT32 atSetRadverCnfProc(VOS_VOID *pMsgBlock)
{
    L4A_SET_RADVER_CNF_STRU *pstCnf = NULL;

    pstCnf = (L4A_SET_RADVER_CNF_STRU *)pMsgBlock;

    CmdErrProc((VOS_UINT8)(pstCnf->usClientId), pstCnf->ulErrorCode, 0, NULL);

    return AT_FW_CLIENT_STATUS_READY;
}

VOS_UINT32 At_QryCellIdPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32 ulRet = ERR_MSP_SUCCESS;
    L4A_READ_CELL_ID_REQ_STRU stReq = {0};

    stReq.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;
    stReq.stCtrl.ulOpId = 0;
    stReq.stCtrl.ulPid = WUEPS_PID_AT;
    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_CELL_ID_REQ,
                             (VOS_VOID*)(&stReq),
                             sizeof(L4A_READ_CELL_ID_REQ_STRU));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_CELL_ID_READ;
        return AT_WAIT_ASYNC_RETURN;
    }
    return AT_ERROR;
}

VOS_UINT32 At_SetLFromConnToIdlePara(VOS_UINT8 ucIndex)
{
    VOS_UINT32 ulRet = ERR_MSP_SUCCESS;
    L4A_SET_LTE_TO_IDLE_REQ_STRU stReq = {0};
    LPS_SWITCH_PARA_STRU stDrxControlFlag = {0};

    stReq.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;
    stReq.stCtrl.ulOpId = 0;
    stReq.stCtrl.ulPid = WUEPS_PID_AT;

    /* 参数检查 */
    if(AT_CMD_OPT_SET_CMD_NO_PARA != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if(NV_OK != NVM_Read(EN_NV_ID_SWITCH_PARA, &stDrxControlFlag,sizeof(LPS_SWITCH_PARA_STRU)))
    {
        return AT_ERROR;
    }

    if( 0 == ((*((VOS_UINT32 * )(&(stDrxControlFlag.stPsFunFlag01)))) & LPS_NV_JP_DCOM_CON_TO_IDLE_BIT))
    {
        return AT_OK;
    }

    /*L模调用如下接口转发*/
    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_LTE_TO_IDLE_REQ,
                             (VOS_VOID*)(&stReq),
                             sizeof(L4A_SET_LTE_TO_IDLE_REQ_STRU));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        return AT_OK;
    }
    return AT_ERROR;
}

VOS_UINT32 At_SetLWThresholdCfgPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32 ulRet = ERR_MSP_SUCCESS;
    L4A_LW_THREASHOLD_CFG_REQ_STRU stReq = {0};
    LPS_SWITCH_PARA_STRU stDrxControlFlag = {0};

    stReq.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;
    stReq.stCtrl.ulOpId = 0;
    stReq.stCtrl.ulPid = WUEPS_PID_AT;
    stReq.ulFlag = gastAtParaList[0].ulParaValue;

    if(NV_OK != NVM_Read(EN_NV_ID_SWITCH_PARA, &stDrxControlFlag,sizeof(LPS_SWITCH_PARA_STRU)))
    {
        return AT_ERROR;
    }

    if( 0 == ((*((VOS_UINT32 * )(&(stDrxControlFlag.stPsFunFlag01)))) & LPS_NV_JP_DCOM_REL_OFFSET_BIT))
    {
        return AT_OK;
    }

    /*L模调用如下接口转发*/
    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_LW_THRESHOLD_REQ,
                             (VOS_VOID*)(&stReq),
                             sizeof(L4A_LW_THREASHOLD_CFG_REQ_STRU));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        return AT_OK;
    }
    return AT_ERROR;
}

VOS_UINT32 AT_SetLFastDormPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32 ulRet = ERR_MSP_SUCCESS;
    L4A_SET_FAST_DORM_REQ_STRU stReq = {0};
    LPS_SWITCH_PARA_STRU stDrxControlFlag = {0};

    /* 参数检查在AT入口已做
 */

    stReq.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;
    stReq.stCtrl.ulOpId = 0;
    stReq.stCtrl.ulPid = WUEPS_PID_AT;
    stReq.ulFlag = gastAtParaList[0].ulParaValue;
    stReq.ulTimerLen = gastAtParaList[1].ulParaValue;

    if(NV_OK != NVM_Read(EN_NV_ID_SWITCH_PARA, &stDrxControlFlag,sizeof(LPS_SWITCH_PARA_STRU)))
    {
        return AT_ERROR;
    }

    if( 0 == ((*((VOS_UINT32 * )(&(stDrxControlFlag.stPsFunFlag01)))) & LPS_NV_JP_DCOM_FAST_DORM_BIT))
    {
        return AT_OK;
    }

    /*L模调用如下接口转发*/
    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_FAST_DORM_REQ,
                             (VOS_VOID*)(&stReq),
                             sizeof(L4A_SET_FAST_DORM_REQ_STRU));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        return AT_OK;
    }
    return AT_ERROR;
}


VOS_UINT32 At_SetLCellInfoPara(VOS_UINT8 ucIndex)
{
    L4A_READ_CELL_INFO_REQ_STRU         stCellInfo;
    VOS_UINT32                          ulRet = ERR_MSP_SUCCESS;

    /* 参数过多 */
    if(gucAtParaIndex != 1)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    stCellInfo.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;
    stCellInfo.stCtrl.ulOpId = 0;
    stCellInfo.stCtrl.ulPid = WUEPS_PID_AT;
    stCellInfo.ulCellFlag = gastAtParaList[0].ulParaValue;

    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_CELL_INFO_REQ,
                             (VOS_VOID*)(&stCellInfo),
                             sizeof(L4A_READ_CELL_INFO_REQ_STRU));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = (AT_CMD_CURRENT_OPT_ENUM)AT_CMD_LCELLINFO_QUERY;
        return AT_WAIT_ASYNC_RETURN;
    }
    else
    {
        return AT_ERROR;
    }
}


VOS_UINT32 AT_SetLIndCfgReq(VOS_UINT8 ucIndex,L4A_IND_CFG_STRU* pstIndCfgReq)
{
    L4A_IND_CFG_REQ_STRU         stIndCfgReq = {0};
    VOS_UINT32                    ulRet = ERR_MSP_SUCCESS;

    stIndCfgReq.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;
    stIndCfgReq.stCtrl.ulOpId = 0;
    stIndCfgReq.stCtrl.ulPid = WUEPS_PID_AT;
    MSP_MEMCPY(&(stIndCfgReq.stIndCfg),(VOS_SIZE_T)sizeof(L4A_IND_CFG_STRU),pstIndCfgReq,(VOS_SIZE_T)sizeof(L4A_IND_CFG_STRU));
    
    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_IND_CFG_REQ,
                             (VOS_VOID*)(&stIndCfgReq),
                             sizeof(L4A_IND_CFG_REQ_STRU));
    if(ERR_MSP_SUCCESS == ulRet)
    {
        return AT_OK;
    }
    return AT_ERROR;
}


VOS_UINT32 AtQryLCerssiPara(VOS_UINT8 ucIndex)
{
    L4A_CSQ_INFO_REQ_STRU         stCerssiQry = {0};
    VOS_UINT32                    ulRet = ERR_MSP_SUCCESS;

    stCerssiQry.stCtrl.ulClientId = gastAtClientTab[ucIndex].usClientId;
    stCerssiQry.stCtrl.ulOpId = 0;
    stCerssiQry.stCtrl.ulPid = WUEPS_PID_AT;

    ulRet = atSendL4aDataMsg(gastAtClientTab[ucIndex].usClientId,
                             I0_MSP_L4_L4A_PID,
                             ID_MSG_L4A_CERSSI_INQ_REQ,
                             (VOS_VOID*)(&stCerssiQry),
                             sizeof(L4A_CSQ_INFO_REQ_STRU));

    if(ERR_MSP_SUCCESS == ulRet)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_CERSSI_READ;
        return AT_WAIT_ASYNC_RETURN;
    }
    else
    {
        return AT_ERROR;
    }
}


