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
#include "AtInit.h"

/* 保护此头文件是因为包含了PS域相关的定义，但是本项目未修改PS域相关的，后续需要调整 */
#include "AtDataProc.h"
#include "AcpuReset.h"
#include "msp_nvim.h"
#include "msp_nv_id.h"
#include "msp_nv_def.h"
#include "LPsNvInterface.h"
#include "at_lte_common.h"
#include "GasNvInterface.h"
#include "NasNvInterface.h"
#include "TafNvInterface.h"
#include "product_config.h"
#include "mdrv.h"
#include "msp_diag_comm.h"
#include "GuNasLogFilter.h"
#include "PsLogFilterInterface.h"





/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID                 PS_FILE_ID_AT_INIT_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/


VOS_VOID AT_ReadPlatformNV(VOS_VOID)
{
    MODEM_ID_ENUM_UINT16                enModemID;
    PLATAFORM_RAT_CAPABILITY_STRU       stPlatFormRat;
    VOS_UINT8                           ucRatIndex;
    AT_MODEM_SPT_RAT_STRU              *pstAtSptRatList = VOS_NULL_PTR;

    TAF_MEM_SET_S(&stPlatFormRat, sizeof(stPlatFormRat), 0x00, sizeof(PLATAFORM_RAT_CAPABILITY_STRU));

    for (enModemID = 0; enModemID < MODEM_ID_BUTT; enModemID++)
    {
        pstAtSptRatList = AT_GetSptRatFromModemId(enModemID);

        /* 读取平台NV成功 */
        if(NV_OK == NV_ReadEx(enModemID, en_NV_Item_Platform_RAT_CAP, &stPlatFormRat,
                              sizeof(PLATAFORM_RAT_CAPABILITY_STRU)))
        {
            pstAtSptRatList->ucPlatformSptGsm        = VOS_FALSE;
            pstAtSptRatList->ucPlatformSptWcdma      = VOS_FALSE;
            pstAtSptRatList->ucPlatformSptLte        = VOS_FALSE;
            pstAtSptRatList->ucPlatformSptUtralTDD   = VOS_FALSE;

            for (ucRatIndex = 0; ucRatIndex < stPlatFormRat.usRatNum; ucRatIndex++)
            {
                /* 平台支持LTE */
                if (PLATFORM_RAT_LTE == stPlatFormRat.aenRatList[ucRatIndex])
                {
                    pstAtSptRatList->ucPlatformSptLte = VOS_TRUE;
                }
                /* 平台支持WCDMA */
                if (PLATFORM_RAT_WCDMA == stPlatFormRat.aenRatList[ucRatIndex])
                {
                    pstAtSptRatList->ucPlatformSptWcdma = VOS_TRUE;
                }

                /* 平台支持TDS*/
                if (PLATFORM_RAT_TDS == stPlatFormRat.aenRatList[ucRatIndex])
                {
                    pstAtSptRatList->ucPlatformSptUtralTDD = VOS_TRUE;
                }
                /* 平台支持GSM */
                if (PLATFORM_RAT_GSM == stPlatFormRat.aenRatList[ucRatIndex])
                {
                    pstAtSptRatList->ucPlatformSptGsm = VOS_TRUE;
                }
            }
        }
    }

    return;
}


VOS_VOID AT_ReadClientConfigNV(VOS_VOID)
{
    TAF_AT_NVIM_AT_CLIENT_CFG_STRU      stAtClientCfg;
    AT_CLINET_CONFIG_DESC_STRU         *pstCfgDesc;
    AT_CLIENT_CONFIGURATION_STRU       *pstClientCfg;
    AT_CLIENT_CFG_MAP_TAB_STRU         *pstCfgMapTbl;
    VOS_UINT8                           i;

    TAF_MEM_SET_S(&stAtClientCfg, sizeof(stAtClientCfg), 0x00, sizeof(TAF_AT_NVIM_AT_CLIENT_CFG_STRU));

    /* NV项en_NV_Item_AT_CLIENT_CONFIG的
       aucAtClientConfig[Index]
       Index = 0 -- AT_CLIENT_TAB_PCUI_INDEX
       Index = 1 -- AT_CLIENT_TAB_CTRL_INDEX
       ......
       按index顺序递增
       aucAtClientConfig[Index]
       BIT0-BIT1对应一个client归属于哪个ModemId:00:表示modem0 01:表示modem1
       BIT2对应一个client是否允许广播:0:表示不允许 1:表示允许
    */
    if (VOS_OK != NV_ReadEx(MODEM_ID_0,
                            en_NV_Item_AT_CLIENT_CFG,
                            &stAtClientCfg,
                            sizeof(TAF_AT_NVIM_AT_CLIENT_CFG_STRU)))
    {
        return;
    }

    pstCfgDesc = (AT_CLINET_CONFIG_DESC_STRU *)&(stAtClientCfg.aucAtClientConfig[0]);

    for (i = 0; i < AT_GET_CLIENT_CFG_TAB_LEN(); i++)
    {
        pstCfgMapTbl = AT_GetClientCfgMapTbl(i);
        pstClientCfg = AT_GetClientConfig(pstCfgMapTbl->enClientId);

        pstClientCfg->enModemId   = pstCfgDesc[pstCfgMapTbl->enNvIndex].ucModemId;
        pstClientCfg->ucReportFlg = pstCfgDesc[pstCfgMapTbl->enNvIndex].ucReportFlg;
    }

    return;
}


VOS_VOID AT_GetCpmsMtMem(
    MODEM_ID_ENUM_UINT16                enModemId,
    MN_MSG_MEM_STORE_ENUM_U8           *penSmMemStore
)
{
    VOS_UINT32                          ulRet;
    TAF_NVIM_SMS_SERVICE_PARA_STRU      stSmsServicePara;
    /* 短信接收存储介质保存在NV配置文件的使能标志
       VOS_TRUE        短信接收存储介质保存在NV配置文件
       VOS_FALSE       短信接收存储介质不保存在NV配置文件，每次上电后恢复为SM存储 */
    VOS_BOOL                            bSmMemEnable;
    AT_MODEM_SMS_CTX_STRU              *pstSmsCtx = VOS_NULL_PTR;

    TAF_MEM_SET_S(&stSmsServicePara, sizeof(stSmsServicePara), 0x00, sizeof(TAF_NVIM_SMS_SERVICE_PARA_STRU));

    *penSmMemStore = MN_MSG_MEM_STORE_SIM;

    ulRet = NV_ReadEx(enModemId, en_NV_Item_SMS_SERVICE_Para, &stSmsServicePara, MN_MSG_SRV_PARAM_LEN);
    if (NV_OK != ulRet)
    {
        AT_ERR_LOG("AT_GetCpmsMtMem: Read Service Parm From Nvim Failed");
        return;
    }

    bSmMemEnable                  = (stSmsServicePara.aucSmsServicePara)[MN_MSG_SRV_SM_MEM_ENABLE_OFFSET];

    pstSmsCtx                     = AT_GetModemSmsCtxAddrFromModemId(enModemId);

    if (MN_MSG_ME_STORAGE_ENABLE == pstSmsCtx->enMsgMeStorageStatus)
    {
        if (VOS_TRUE == bSmMemEnable)
        {
            *penSmMemStore = (stSmsServicePara.aucSmsServicePara)[MN_MSG_SRV_RCV_SM_MEM_STORE_OFFSET];
        }
    }

    return;
}


AT_CGSMS_SEND_DOMAIN_ENUM_U8 AT_SendDomainNvimToProto(
    VOS_UINT32                           ulNvimSendDomain
)
{
    AT_CGSMS_SEND_DOMAIN_ENUM_U8        enProtoSendDomain;

    if (AT_NV_SMS_DOMAIN_PS_PREFERRED == ulNvimSendDomain)
    {
        enProtoSendDomain = MN_MSG_SEND_DOMAIN_PS_PREFERRED;
    }
    else if ((AT_NV_SMS_DOMAIN_PS_ONLY == ulNvimSendDomain))
    {
        enProtoSendDomain = MN_MSG_SEND_DOMAIN_PS;
    }
    else if (AT_NV_SMS_DOMAIN_CS_ONLY == ulNvimSendDomain)
    {
        enProtoSendDomain = MN_MSG_SEND_DOMAIN_CS;
    }
    else
    {
        enProtoSendDomain = MN_MSG_SEND_DOMAIN_CS_PREFERRED;
    }

    return enProtoSendDomain;
}


VOS_VOID AT_ReadSmsSendDomainNV(VOS_VOID)
{
    VOS_UINT32                          ulRet;
    AT_NVIM_SEND_DOMAIN_STRU            stSendDomain;
    MODEM_ID_ENUM_UINT16                enModemId;
    AT_MODEM_SMS_CTX_STRU              *pstSmsCtx = VOS_NULL_PTR;
    VOS_UINT32                          ulLength;

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        pstSmsCtx = AT_GetModemSmsCtxAddrFromModemId(enModemId);

        /* 读取失败和未激活一样处理,首先判断NVIM项是否激活,未激活则取默认值 */
        TAF_MEM_SET_S(&stSendDomain, sizeof(stSendDomain), 0x00, sizeof(stSendDomain));
        ulLength = 0;
        (VOS_VOID)NV_GetLength(en_NV_Item_SMS_SEND_DOMAIN, &ulLength);
        ulRet = NV_ReadEx(enModemId, en_NV_Item_SMS_SEND_DOMAIN, &stSendDomain, ulLength);
        if ( (NV_OK == ulRet)
          && (VOS_TRUE == stSendDomain.ucActFlg))
        {
            /* 激活则将NVIM中的取值进行转化
               由于NVIM项中的值与协议中的值不一致,所以需要进行一次转换 */
            pstSmsCtx->stCgsmsSendDomain.ucActFlg     = VOS_TRUE;
            pstSmsCtx->stCgsmsSendDomain.enSendDomain = AT_SendDomainNvimToProto((VOS_UINT32)stSendDomain.ucSendDomain);
        }
    }
    return;
}


VOS_VOID AT_ReadSmsMeStorageInfoNV(VOS_VOID)
{
    VOS_UINT32                          ulRet;
    MN_MSG_ME_STORAGE_PARM_STRU         stMeStorageParm;
    MODEM_ID_ENUM_UINT16                enModemId;
    AT_MODEM_SMS_CTX_STRU              *pstSmsCtx = VOS_NULL_PTR;

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        pstSmsCtx = AT_GetModemSmsCtxAddrFromModemId(enModemId);

        stMeStorageParm.enMeStorageStatus   = MN_MSG_ME_STORAGE_DISABLE;
        stMeStorageParm.usMeStorageNum      = 0;

        /* 获取ME短信存储介质的支持状态 */
        ulRet = NV_ReadEx(enModemId, en_NV_Item_Sms_Me_Storage_Info, &stMeStorageParm, sizeof(stMeStorageParm));
        if (NV_OK == ulRet)
        {
            pstSmsCtx->enMsgMeStorageStatus = stMeStorageParm.enMeStorageStatus;
        }

        /* 获取短信及状态报告接收存储介质*/
        AT_GetCpmsMtMem(enModemId, &(pstSmsCtx->stCpmsInfo.stRcvPath.enSmMemStore));
        pstSmsCtx->stCpmsInfo.stRcvPath.enStaRptMemStore = pstSmsCtx->stCpmsInfo.stRcvPath.enSmMemStore;
    }

    return;
}


VOS_VOID AT_ReadSmsClass0TailorNV(VOS_VOID)
{
    VOS_UINT32                          ulRet;
    MN_MSG_NVIM_CLASS0_TAILOR_STRU      stClass0Tailor;
    VOS_UINT32                          ulLength;

    ulLength                            = 0;
    stClass0Tailor.ucActFlg             = MN_MSG_NVIM_ITEM_INACTIVE;
    stClass0Tailor.enClass0Tailor       = MN_MSG_CLASS0_DEF;

    (VOS_VOID)NV_GetLength(en_NV_Item_SMS_CLASS0_TAILOR, &ulLength);
    /* 获取CLASS0类短信接收上报方式 */
    ulRet = NV_ReadEx(MODEM_ID_0, en_NV_Item_SMS_CLASS0_TAILOR, &stClass0Tailor, ulLength);
    if ( (NV_OK == ulRet)
      && (MN_MSG_NVIM_ITEM_ACTIVE == stClass0Tailor.ucActFlg))
    {
        g_enClass0Tailor = stClass0Tailor.enClass0Tailor;
    }
    else
    {
        g_enClass0Tailor = MN_MSG_CLASS0_DEF;
    }

    return;
}


VOS_VOID AT_ReadSmsClosePathNV(VOS_VOID)
{
    TAF_NVIM_SMS_CLOSE_PATH_CFG_STRU    stClosePath;
    AT_MODEM_SMS_CTX_STRU              *pstSmsCtx = VOS_NULL_PTR;
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT32                          ulRet;

    TAF_MEM_SET_S(&stClosePath, sizeof(stClosePath), 0x00, sizeof(stClosePath));

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        pstSmsCtx = AT_GetModemSmsCtxAddrFromModemId(enModemId);

        /* 获取ME短信存储介质的支持状态 */
        ulRet = NV_ReadEx(enModemId, en_NV_Item_SMS_Close_Path, &stClosePath, sizeof(stClosePath));
        if ( (NV_OK == ulRet)
           && (VOS_TRUE == stClosePath.ucNvimValid))
        {
            pstSmsCtx->ucLocalStoreFlg = stClosePath.ucSmsClosePathFlg;
        }
    }
}


VOS_VOID AT_ReadMtCustomizeInfo(VOS_VOID)
{
    VOS_UINT32                          ulRet;
    MN_MSG_MT_CUSTOMIZE_INFO_STRU       stMtCustomize;
    MODEM_ID_ENUM_UINT16                enModemId;
    AT_MODEM_SMS_CTX_STRU              *pstSmsCtx = VOS_NULL_PTR;

    TAF_MEM_SET_S(&stMtCustomize, (VOS_SIZE_T)sizeof(stMtCustomize), 0x00, (VOS_SIZE_T)sizeof(MN_MSG_MT_CUSTOMIZE_INFO_STRU));

    ulRet = NV_Read(en_NV_Item_SMS_MT_CUSTOMIZE_INFO,
                &stMtCustomize,
                sizeof(stMtCustomize));
    if (NV_OK != ulRet)
    {
        AT_INFO_LOG("AT_ReadMtCustomizeInfo: Fail to read ");
        return;
    }

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        pstSmsCtx = AT_GetModemSmsCtxAddrFromModemId(enModemId);

        if (MN_MSG_NVIM_ITEM_ACTIVE == stMtCustomize.ucActFlag)
        {
            pstSmsCtx->stSmMeFullCustomize.ucActFlg      = VOS_TRUE;
            pstSmsCtx->stSmMeFullCustomize.enMtCustomize = stMtCustomize.enMtCustomize;
        }
    }
    return;
}


VOS_VOID AT_ReadPortBuffCfgNV(VOS_VOID)
{
    AT_COMM_CTX_STRU                   *pstCommCtx = VOS_NULL_PTR;
    VOS_UINT32                          ulRet;
    TAF_NV_PORT_BUFF_CFG_STRU           stSmsBuffCfg;

    pstCommCtx = AT_GetCommCtxAddr();

    TAF_MEM_SET_S(&stSmsBuffCfg, sizeof(stSmsBuffCfg), 0x00, sizeof(stSmsBuffCfg));

    ulRet = NV_ReadEx(MODEM_ID_0, en_NV_Item_PORT_BUFF_CFG, &stSmsBuffCfg, sizeof(stSmsBuffCfg));

    if (NV_OK == ulRet)
    {
        if (stSmsBuffCfg.ucEnableFlg < AT_PORT_BUFF_BUTT)
        {
            pstCommCtx->stPortBuffCfg.enSmsBuffCfg = stSmsBuffCfg.ucEnableFlg;
        }
        else
        {
            pstCommCtx->stPortBuffCfg.enSmsBuffCfg = AT_PORT_BUFF_DISABLE;
        }
    }
    else
    {
        pstCommCtx->stPortBuffCfg.enSmsBuffCfg = AT_PORT_BUFF_DISABLE;
    }

    return;
}



VOS_VOID AT_ReadSmsNV(VOS_VOID)
{
    AT_ReadSmsSendDomainNV();

    AT_ReadSmsMeStorageInfoNV();

    AT_ReadSmsClass0TailorNV();

    AT_ReadSmsClosePathNV();


    AT_ReadMtCustomizeInfo();

    return;
}


VOS_VOID  AT_ReadRoamCapaNV(VOS_VOID)
{
    NAS_NVIM_ROAM_CFG_INFO_STRU         stRoamCfgInfo;
    MODEM_ID_ENUM_UINT16                enModemId;
    AT_MODEM_NET_CTX_STRU              *pstNetCtx = VOS_NULL_PTR;
    VOS_UINT32                          ulLength;

    ulLength = 0;
    TAF_MEM_SET_S(&stRoamCfgInfo, sizeof(stRoamCfgInfo), 0x00, sizeof(stRoamCfgInfo));

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        pstNetCtx = AT_GetModemNetCtxAddrFromModemId(enModemId);

        (VOS_VOID)NV_GetLength(en_NV_Item_Roam_Capa, &ulLength);
        if (NV_OK == NV_ReadEx(enModemId, en_NV_Item_Roam_Capa, &stRoamCfgInfo, ulLength))
        {
            if (stRoamCfgInfo.ucRoamFeatureFlg >1)
            {
                 pstNetCtx->ucRoamFeature = AT_ROAM_FEATURE_OFF;
            }
            else
            {
                pstNetCtx->ucRoamFeature = stRoamCfgInfo.ucRoamFeatureFlg;
            }
        }
        else
        {
            pstNetCtx->ucRoamFeature = AT_ROAM_FEATURE_OFF;
        }
    }
    return;
}


VOS_VOID AT_ReadPrivacyFilterCfgNv(VOS_VOID)
{
    AT_MODEM_PRIVACY_FILTER_CTX_STRU   *pstFilterCtx = VOS_NULL_PTR;
    NAS_NV_PRIVACY_FILTER_CFG_STRU      stPrivacyFilterCfg;
    MODEM_ID_ENUM_UINT16                enModemId;

    /* 初始化 */
    TAF_MEM_SET_S(&stPrivacyFilterCfg, sizeof(stPrivacyFilterCfg), 0x00, sizeof(NAS_NV_PRIVACY_FILTER_CFG_STRU));

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        /* 读取NV项 */
        if (NV_OK != NV_ReadEx(enModemId,
                               en_NV_Item_Privacy_Log_Filter_Cfg,
                               &stPrivacyFilterCfg,
                               sizeof(NAS_NV_PRIVACY_FILTER_CFG_STRU)))
        {
            stPrivacyFilterCfg.ucFilterEnableFlg = VOS_FALSE;
        }

        /* NV读取值保护 */
        if (VOS_TRUE == stPrivacyFilterCfg.ucFilterEnableFlg)
        {
            stPrivacyFilterCfg.ucFilterEnableFlg = VOS_TRUE;
        }

        pstFilterCtx = AT_GetModemPrivacyFilterCtxAddrFromModemId(enModemId);

        pstFilterCtx->ucFilterEnableFlg = stPrivacyFilterCfg.ucFilterEnableFlg;
    }

    return;
}


VOS_VOID  AT_ReadSystemAppConfigNV(VOS_VOID)
{
    VOS_UINT8                          *pucSystemAppConfig;
    NAS_NVIM_SYSTEM_APP_CONFIG_STRU     stSysAppConfig;
    VOS_UINT32                          ulLength;

    stSysAppConfig.usSysAppConfigType = SYSTEM_APP_BUTT;

    /* 获取控制特性的NV上下文地址 */
    pucSystemAppConfig                  = AT_GetSystemAppConfigAddr();
    ulLength                            = 0;

    (VOS_VOID)NV_GetLength(en_NV_Item_System_APP_Config, &ulLength);
    /*读取失败按默认值处理 */
    if (VOS_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_System_APP_Config,&stSysAppConfig,ulLength))
    {
        *pucSystemAppConfig  = SYSTEM_APP_MP;
        return;
    }

    if ( SYSTEM_APP_ANDROID < stSysAppConfig.usSysAppConfigType)
    {
        *pucSystemAppConfig  = SYSTEM_APP_MP;
    }
    else
    {
        *pucSystemAppConfig  = (VOS_UINT8)stSysAppConfig.usSysAppConfigType;
    }

    return;

}


VOS_VOID AT_ReadAtDislogPwdNV(VOS_VOID)
{
    VOS_UINT8                          *pucSystemAppConfig;
    TAF_AT_NVIM_DISLOG_PWD_NEW_STRU     stDislogPwdNew;

    /* 按D25的做法  不保存权限标志, 则不用从NV中读取权限, 默认无权限 */
    g_enATE5RightFlag = AT_E5_RIGHT_FLAG_NO;
    TAF_MEM_SET_S(&stDislogPwdNew, sizeof(stDislogPwdNew), 0x00, sizeof(stDislogPwdNew));

    /* 读取NV项中当前产品形态 */
    pucSystemAppConfig = AT_GetSystemAppConfigAddr();

    /* 新加NV项保存DISLOG密码(OPWORD使用) */
    if (NV_OK == NV_ReadEx(MODEM_ID_0, en_NV_Item_AT_DISLOG_PWD_NEW,
                          &stDislogPwdNew,
                          AT_OPWORD_PWD_LEN))
    {
        TAF_MEM_CPY_S((VOS_INT8*)g_acATOpwordPwd,
                   AT_OPWORD_PWD_LEN + 1,
                   (VOS_INT8*)(&stDislogPwdNew),
                   AT_OPWORD_PWD_LEN);

        /* 跟上字符串结尾符 */
        g_acATOpwordPwd[AT_OPWORD_PWD_LEN] = 0;

        /* 如果密码为空，且非AP-Modem形态，则取消密码保护 */
        if ((0 == VOS_StrLen((VOS_CHAR *)g_acATOpwordPwd))
         && (SYSTEM_APP_ANDROID != *pucSystemAppConfig))
        {
            /* NV中密码为空则输入任何密码都能获取权限 */
            g_enATE5RightFlag = AT_E5_RIGHT_FLAG_YES;
        }
    }
    else
    {
        AT_WARN_LOG("AT_ReadAtDislogPwdNV:read en_NV_Item_AT_DISLOG_PWD_NEW failed");
    }

    return;
}


VOS_VOID AT_ReadAtRightPasswordNV(VOS_VOID)
{
    TAF_AT_NVIM_RIGHT_OPEN_FLAG_STRU        stNvimRightOpenFlg;

    TAF_MEM_SET_S(&stNvimRightOpenFlg, sizeof(stNvimRightOpenFlg), 0x00, sizeof(stNvimRightOpenFlg));

    /* 从NV中获取当前操作AT命令的权限 */
    if (NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_AT_RIGHT_PASSWORD,
                           &stNvimRightOpenFlg,
                           sizeof(stNvimRightOpenFlg)))
    {
        /* 读取NV失败,采用默认密码 */
        TAF_MEM_SET_S(&g_stAtRightOpenFlg, sizeof(g_stAtRightOpenFlg), 0x00, sizeof(g_stAtRightOpenFlg));
        AT_WARN_LOG("AT_ReadAtRightPasswordNV:read en_NV_Item_AT_RIGHT_PASSWORD failed");
    }

    TAF_MEM_CPY_S(&g_stAtRightOpenFlg, sizeof(AT_RIGHT_OPEN_FLAG_STRU), &stNvimRightOpenFlg, sizeof(AT_RIGHT_OPEN_FLAG_STRU));
    return;
}


VOS_VOID AT_ReadAtDissdPwdNV(VOS_VOID)
{
    TAF_AT_NVIM_DISSD_PWD_STRU          stDissdPwd;

    TAF_MEM_SET_S(&stDissdPwd, sizeof(stDissdPwd), 0x00, AT_DISSD_PWD_LEN);

    if (NV_OK == NV_ReadEx(MODEM_ID_0, en_NV_Item_AT_DISSD_PWD,
                           &stDissdPwd,
                           AT_DISSD_PWD_LEN))
    {
        TAF_MEM_CPY_S((VOS_INT8*)g_acATE5DissdPwd,
                   AT_DISSD_PWD_LEN + 1,
                   (VOS_INT8*)(&stDissdPwd),
                   AT_DISSD_PWD_LEN);
        /* 跟上字符串结尾符 */
        g_acATE5DissdPwd[AT_DISSD_PWD_LEN] = 0;
    }
    else
    {
        AT_WARN_LOG("AT_ReadAtDissdPwdNV:read en_NV_Item_AT_DISSD_PWD failed");
    }

    return;
}


VOS_VOID AT_ReadNotSupportRetValueNV(VOS_VOID)
{
    VOS_UINT32                                  ulRetLen;
    VOS_CHAR                                    acRetVal[AT_NOTSUPPORT_STR_LEN+1];
    TAF_AT_NOT_SUPPORT_CMD_ERROR_TEXT_STRU      stErrorText;

    TAF_MEM_SET_S(&stErrorText,  sizeof(stErrorText), 0x00, AT_NOTSUPPORT_STR_LEN);

     /* 从NV中获取不支持命令的返回值，当为E5、LCARD、DONGLE时，
        该NV设置为"ERROR",其他平台设置为"COMMAND NOT SUPPORT" */
    if (NV_OK == NV_ReadEx(MODEM_ID_0, en_NV_Item_NOT_SUPPORT_RET_VALUE, stErrorText.acErrorText,
                           AT_NOTSUPPORT_STR_LEN))
    {
         TAF_MEM_CPY_S(acRetVal,
                    sizeof(acRetVal),
                    stErrorText.acErrorText,
                    AT_NOTSUPPORT_STR_LEN);

         /* 跟上字符串结尾符 */
         acRetVal[AT_NOTSUPPORT_STR_LEN] = 0;
         ulRetLen                        = VOS_StrLen(acRetVal);
         if (ulRetLen > 0)
         {
             VOS_StrCpy_s((VOS_CHAR *)gaucAtCmdNotSupportStr, AT_NOTSUPPORT_STR_LEN+4, acRetVal);
         }
     }
     else
     {
         AT_WARN_LOG("AT_ReadNotSupportRetValueNV:read en_NV_Item_NOT_SUPPORT_RET_VALUE failed");
     }

    return;
}


VOS_VOID AT_ReadE5NV(VOS_VOID)
{
    AT_ReadAtDislogPwdNV();

    AT_ReadAtRightPasswordNV();

    AT_ReadAtDissdPwdNV();

    AT_ReadNotSupportRetValueNV();

    return;
}


VOS_UINT32 AT_IsAbortCmdCharValid(
    VOS_UINT8                          *pucAbortCmdChar,
    VOS_UINT32                          ulLen
)
{
    VOS_UINT32                          i;

    /* 由于在AT命令的处理的流程中, 会对输入的字符进行格式化处理, 大致会过滤掉不可见字符
       (<0x20的ASCII字符,空格, S3, S5)等.
       如果NV中设置的ABORT命令字符串中包含这些字符会导致打断命令匹配失败, 故做容错处理,
       如果NV中设置的ABORT命令字符串中包含这些字符则认为设置无效, 使用默认值 */

    /* 如果打断命令或打断回复为空指针，无效 */
    if ( VOS_NULL_PTR == pucAbortCmdChar )
    {
        return VOS_FALSE;
    }

    /* 如果打断命令或打断回复长度为0，无效 */
    if ( 0 == ulLen )
    {
        return VOS_FALSE;
    }

    /* 如果打断命令或打断回复有不可见字符(<0x20的ASCII字符,空格, S3, S5)等，无效 */
    for ( i = 0 ; i < ulLen; i++ )
    {
        if ( (pucAbortCmdChar[i] == ucAtS3)
          || (pucAbortCmdChar[i] == ucAtS5)
          || (pucAbortCmdChar[i] <= 0x20) )
        {
            return VOS_FALSE;
        }
    }

    return VOS_TRUE;
}


VOS_VOID AT_ReadAbortCmdParaNV(VOS_VOID)
{
    AT_ABORT_CMD_PARA_STRU             *pstAbortCmdPara   = VOS_NULL_PTR;
    VOS_UINT8                           aucDfltAbortCmd[] = "AT";
    VOS_UINT8                           aucDfltAbortRsp[] = "OK";            /* 命令比较时不区分大小写, 此处默认值定义为大小写均可 */
    AT_NVIM_ABORT_CMD_PARA_STRU         stNvAbortCmdPara;
    VOS_UINT32                          ulIsAbortCmdValid;
    VOS_UINT32                          ulIsAbortRspValid;
    VOS_UINT32                          ulRlst;
    VOS_UINT8                           aucAnyCharAbortCmd[] = "ANY";
    VOS_UINT32                          ulLen;

    /* 初始化 避免TQE告警 */
    TAF_MEM_SET_S(&stNvAbortCmdPara,  sizeof(stNvAbortCmdPara), 0x00, sizeof(stNvAbortCmdPara));

    /* 打断命令初始化 */
    pstAbortCmdPara = AT_GetAbortCmdPara();
    TAF_MEM_SET_S(pstAbortCmdPara, sizeof(AT_ABORT_CMD_PARA_STRU), 0x00, sizeof(AT_ABORT_CMD_PARA_STRU));

    /* 读取NV项中配置的打断命令和打断回复 */
    ulRlst = NV_ReadEx(MODEM_ID_0, en_NV_Item_AT_ABORT_CMD_PARA,
                       &stNvAbortCmdPara,
                       sizeof(AT_NVIM_ABORT_CMD_PARA_STRU));

    /* NV读取失败则使用默认值 */
    if ( NV_OK != ulRlst )
    {
        /* 打断使能标志默认值为TRUE */
        pstAbortCmdPara->ucAbortEnableFlg = VOS_TRUE;

        /* 打断命令默认值 */
        VOS_StrNCpy_s((VOS_CHAR *)(pstAbortCmdPara->aucAbortAtCmdStr),
                    sizeof(pstAbortCmdPara->aucAbortAtCmdStr),
                    (VOS_CHAR*)aucDfltAbortCmd,
                    AT_MAX_ABORT_CMD_STR_LEN);
        pstAbortCmdPara->aucAbortAtCmdStr[AT_MAX_ABORT_CMD_STR_LEN] = '\0';

        /* 打断回复默认值 */
        VOS_StrNCpy_s((VOS_CHAR *)(pstAbortCmdPara->aucAbortAtRspStr),
                    sizeof(pstAbortCmdPara->aucAbortAtRspStr),
                    (VOS_CHAR *)aucDfltAbortRsp,
                    AT_MAX_ABORT_RSP_STR_LEN);
        pstAbortCmdPara->aucAbortAtRspStr[AT_MAX_ABORT_RSP_STR_LEN] = '\0';

        AT_WARN_LOG("AT_ReadAbortCmdParaNV:read en_NV_Item_AT_ABORT_CMD_PARA failed");

        return;
    }

    /* 如果打断功能未使能, 则可以不用关注其它内容 */
    pstAbortCmdPara->ucAbortEnableFlg = stNvAbortCmdPara.ucAbortEnableFlg;
    if ( VOS_TRUE != pstAbortCmdPara->ucAbortEnableFlg )
    {
        return;
    }

    /* 将NV中打断命令的内容赋值到全局变量，赋值长度为AT_MAX_ABORT_CMD_STR_LEN，
       然后将数组第AT_MAX_ABORT_CMD_STR_LEN+1个字符赋值为结束符，
       对应数组下标为AT_MAX_ABORT_CMD_STR_LEN */
    VOS_StrNCpy_s((VOS_CHAR *)(pstAbortCmdPara->aucAbortAtCmdStr),
                sizeof(pstAbortCmdPara->aucAbortAtCmdStr),
                (VOS_CHAR *)(stNvAbortCmdPara.aucAbortAtCmdStr),
                AT_MAX_ABORT_CMD_STR_LEN);
    pstAbortCmdPara->aucAbortAtCmdStr[AT_MAX_ABORT_CMD_STR_LEN] = '\0';

    /* 将NV中打断回复的内容赋值到全局变量，赋值长度为AT_MAX_ABORT_CMD_STR_LEN，
       然后将数组第AT_MAX_ABORT_RSP_STR_LEN+1个字符赋值为结束符，
       对应数组下标为AT_MAX_ABORT_RSP_STR_LEN */
    VOS_StrNCpy_s((VOS_CHAR *)(pstAbortCmdPara->aucAbortAtRspStr),
                sizeof(pstAbortCmdPara->aucAbortAtRspStr),
                (VOS_CHAR *)(stNvAbortCmdPara.aucAbortAtRspStr),
                AT_MAX_ABORT_RSP_STR_LEN);
    pstAbortCmdPara->aucAbortAtRspStr[AT_MAX_ABORT_RSP_STR_LEN] = '\0';


    /* 判断打断命令的NV设置中是否有无效字符 */
    ulIsAbortCmdValid = AT_IsAbortCmdCharValid(pstAbortCmdPara->aucAbortAtCmdStr,
                                     VOS_StrLen((VOS_CHAR *)(pstAbortCmdPara->aucAbortAtCmdStr)));

    ulIsAbortRspValid = AT_IsAbortCmdCharValid(pstAbortCmdPara->aucAbortAtRspStr,
                                     VOS_StrLen((VOS_CHAR *)(pstAbortCmdPara->aucAbortAtRspStr)));

    /* 如果NV设置中含有不可见字符(空格, S3, S5, 小于x020的字符), 则使用默认值 */
    if ( VOS_TRUE != ulIsAbortCmdValid )
    {
        VOS_StrNCpy_s((VOS_CHAR *)(pstAbortCmdPara->aucAbortAtCmdStr),
                    sizeof(pstAbortCmdPara->aucAbortAtCmdStr),
                    (VOS_CHAR*)aucDfltAbortCmd,
                    AT_MAX_ABORT_CMD_STR_LEN);
        pstAbortCmdPara->aucAbortAtCmdStr[AT_MAX_ABORT_CMD_STR_LEN] = '\0';

    }

    if ( VOS_TRUE != ulIsAbortRspValid )
    {
        VOS_StrNCpy_s((VOS_CHAR *)(pstAbortCmdPara->aucAbortAtRspStr),
                    sizeof(pstAbortCmdPara->aucAbortAtRspStr),
                    (VOS_CHAR *)aucDfltAbortRsp,
                    AT_MAX_ABORT_RSP_STR_LEN);
        pstAbortCmdPara->aucAbortAtRspStr[AT_MAX_ABORT_RSP_STR_LEN] = '\0';
    }

    ulLen = VOS_StrLen((VOS_CHAR *)pstAbortCmdPara->aucAbortAtCmdStr);

    if (VOS_StrLen((VOS_CHAR *)aucAnyCharAbortCmd) != ulLen)
    {
        return;
    }

    if ( (0 == VOS_StrNiCmp((VOS_CHAR *)aucAnyCharAbortCmd, (VOS_CHAR *)pstAbortCmdPara->aucAbortAtCmdStr, ulLen)) )
    {
        /* PS_MEM_SET的时候已经将其置成0，即为默认不支持任意字符打断 */
        pstAbortCmdPara->ucAnyAbortFlg = VOS_TRUE;
    }

    return;
}


VOS_VOID AT_ReadSysNV(VOS_VOID)
{
    AT_ReadRoamCapaNV();

    AT_ReadSystemAppConfigNV();

    AT_ReadE5NV();

    AT_ReadAbortCmdParaNV();

    return;
}


VOS_VOID AT_ReadCellSignReportCfgNV( VOS_VOID )
{
    NAS_NVIM_CELL_SIGN_REPORT_CFG_STRU  stCellSignReportCfg;
    MODEM_ID_ENUM_UINT16                enModemId;
    AT_MODEM_NET_CTX_STRU              *pstNetCtx = VOS_NULL_PTR;
    VOS_UINT32                          ulLength;

    ulLength                            = 0;

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        pstNetCtx = AT_GetModemNetCtxAddrFromModemId(enModemId);

        TAF_MEM_SET_S(&stCellSignReportCfg, sizeof(stCellSignReportCfg), 0x00, sizeof(stCellSignReportCfg));

        (VOS_VOID)NV_GetLength(en_NV_Item_CELL_SIGN_REPORT_CFG, &ulLength);
        if(NV_OK != NV_ReadEx(enModemId,
                              en_NV_Item_CELL_SIGN_REPORT_CFG,
                              &stCellSignReportCfg,
                              ulLength))
        {
             AT_WARN_LOG("AT_ReadCellSignReportCfgNV:read en_NV_Item_CELL_SIGN_REPORT_CFG failed");
             return;
        }

        pstNetCtx->ucCerssiReportType = stCellSignReportCfg.ucSignThreshold;

        if (stCellSignReportCfg.ucSignThreshold >= AT_CERSSI_REPORT_TYPE_BUTT)
        {
            pstNetCtx->ucCerssiReportType  = AT_CERSSI_REPORT_TYPE_NOT_REPORT;
        }

        pstNetCtx->ucCerssiMinTimerInterval = stCellSignReportCfg.ucMinRptTimerInterval;

        if (stCellSignReportCfg.ucMinRptTimerInterval > AT_CERSSI_MAX_TIMER_INTERVAL)
        {
            pstNetCtx->ucCerssiMinTimerInterval = 0;
        }
        }

    return;
}


VOS_UINT32 AT_ValidateWasCategory(
    AT_NV_UE_CAPABILITY_STRU           *pstUeCapability
)
{
    if (VOS_TRUE != pstUeCapability->ucHSDSCHSupport)
    {
        return VOS_ERR;
    }

    if ( (pstUeCapability->ucHSDSCHPhyCategory < 1)
      || (pstUeCapability->ucHSDSCHPhyCategory > 12) )
    {
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_UINT32 AT_ValidateWasCategoryExt(
    AT_NV_UE_CAPABILITY_STRU           *pstUeCapability
)
{
    if ( (VOS_TRUE != pstUeCapability->ucHSDSCHSupport)
      || (VOS_TRUE != pstUeCapability->ucMacEhsSupport) )
    {
        return VOS_ERR;
    }

    if ( (pstUeCapability->ucHSDSCHPhyCategoryExt < 13)
      || (pstUeCapability->ucHSDSCHPhyCategoryExt > 20) )
    {
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_UINT32 AT_ValidateWasCategoryExt2(
    AT_NV_UE_CAPABILITY_STRU           *pstUeCapability
)
{
    if ( (VOS_TRUE != pstUeCapability->ucHSDSCHSupport)
      || (VOS_TRUE != pstUeCapability->ucMacEhsSupport)
      || (VOS_TRUE != pstUeCapability->ucMultiCellSupport) )
    {
        return VOS_ERR;
    }

    if ( (pstUeCapability->ucHSDSCHPhyCategoryExt2 < 21)
      || (pstUeCapability->ucHSDSCHPhyCategoryExt2 > 24) )
    {
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_UINT32 AT_ValidateWasCategoryExt3(
    AT_NV_UE_CAPABILITY_STRU           *pstUeCapability
)
{
    if ( (VOS_TRUE != pstUeCapability->ucHSDSCHSupport)
      || (VOS_TRUE != pstUeCapability->ucMacEhsSupport)
      || (VOS_TRUE != pstUeCapability->ucMultiCellSupport)
      || (VOS_TRUE != pstUeCapability->ucDcMimoSupport) )
    {
        return VOS_ERR;
    }

    if ( (pstUeCapability->ucHSDSCHPhyCategoryExt3 < 25)
      || (pstUeCapability->ucHSDSCHPhyCategoryExt3 > 28) )
    {
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_UINT8 AT_GetWasDefaultCategory(
    AT_NV_UE_CAPABILITY_STRU           *pstUeCapability
)
{
    AT_RATE_WCDMA_CATEGORY_ENUM_UINT8   enRateCategory;

    switch (pstUeCapability->ucAsRelIndicator)
    {
        case PS_PTL_VER_R5:
        case PS_PTL_VER_R6:
            enRateCategory = AT_RATE_WCDMA_CATEGORY_10;
            break;

        case PS_PTL_VER_R7:
            enRateCategory = AT_RATE_WCDMA_CATEGORY_18;
            break;

        case PS_PTL_VER_R8:
            enRateCategory = AT_RATE_WCDMA_CATEGORY_28;
            break;

        default:
            enRateCategory = AT_RATE_WCDMA_CATEGORY_10;
            break;
    }

    return enRateCategory;
}


VOS_UINT8 AT_CalcWasCategory(
    AT_NV_UE_CAPABILITY_STRU           *pstUeCapability
)
{
    AT_RATE_WCDMA_CATEGORY_ENUM_UINT8   enRateCategory;

    if (VOS_OK == AT_ValidateWasCategoryExt3(pstUeCapability))
    {
        return pstUeCapability->ucHSDSCHPhyCategoryExt3;
    }

    if (VOS_OK == AT_ValidateWasCategoryExt2(pstUeCapability))
    {
        return pstUeCapability->ucHSDSCHPhyCategoryExt2;
    }

    if (VOS_OK == AT_ValidateWasCategoryExt(pstUeCapability))
    {
        return pstUeCapability->ucHSDSCHPhyCategoryExt;
    }

    if (VOS_OK == AT_ValidateWasCategory(pstUeCapability))
    {
        return pstUeCapability->ucHSDSCHPhyCategory;
    }

    enRateCategory = AT_GetWasDefaultCategory(pstUeCapability);

    return enRateCategory;
}


VOS_VOID AT_ReadWasCapabilityNV(VOS_VOID)
{
    AT_NV_UE_CAPABILITY_STRU            stNvUeCapability;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stNvUeCapability, sizeof(stNvUeCapability), 0x00, sizeof(stNvUeCapability));

    ulResult = NV_ReadEx(MODEM_ID_0, en_NV_Item_WAS_RadioAccess_Capa_New,
                         &stNvUeCapability,
                         sizeof(AT_NV_UE_CAPABILITY_STRU));
    if (VOS_OK != ulResult)
    {
        /* NV读取失败, 协议版本默认为R99 */
        g_stAtDlRateCategory.cWasRelIndicator = PS_PTL_VER_R99;
        g_stAtDlRateCategory.ucWasCategory    = AT_RATE_WCDMA_R99;
    }
    else
    {
        g_stAtDlRateCategory.cWasRelIndicator = (VOS_INT8)stNvUeCapability.ucAsRelIndicator;
        g_stAtDlRateCategory.ucWasCategory    = AT_CalcWasCategory(&stNvUeCapability);
    }
}


VOS_VOID AT_CalcGasCategory(
    NVIM_GAS_HIGH_MULTISLOT_CLASS_STRU         *pstHighMultislotclass,
    VOS_UINT16                                  usGprsMultiSlotClass,
    VOS_UINT16                                  usEgprsMultiSlotClass,
    VOS_UINT16                                  usEgprsFlag
)
{
    /* 默认为VOS_FALSE */
    g_stAtDlRateCategory.ucGasMultislotClass33Flg = VOS_FALSE;

    if((VOS_TRUE == pstHighMultislotclass->usHighMultislotClassFlg)
     &&(AT_GAS_HIGH_MULTISLOT_CLASS_0 == pstHighMultislotclass->usHighMultislotClass))
    {
        if ((VOS_FALSE  == usEgprsFlag)
          &&(AT_GAS_GRR_MULTISLOT_CLASS_MAX == usGprsMultiSlotClass))
        {
            g_stAtDlRateCategory.ucGasMultislotClass33Flg = VOS_TRUE;
        }

        if((VOS_TRUE == usEgprsFlag)
         &&(AT_GAS_GRR_MULTISLOT_CLASS_MAX == usEgprsMultiSlotClass))
        {
            g_stAtDlRateCategory.ucGasMultislotClass33Flg = VOS_TRUE;
        }
    }

    return;
}


VOS_VOID AT_ReadGasCapabilityNV(VOS_VOID)
{
    VOS_UINT32                                  ulResult;
    NVIM_GAS_HIGH_MULTISLOT_CLASS_STRU          stHighMultislotclass;
    VOS_UINT16                                  usGprsMultiSlotClass;
    VOS_UINT16                                  usEgprsMultiSlotClass;
    VOS_UINT16                                  usEgprsFlag;

    stHighMultislotclass.usHighMultislotClassFlg = VOS_FALSE;
    stHighMultislotclass.usHighMultislotClass    = AT_GAS_HIGH_MULTISLOT_CLASS_0;

    usGprsMultiSlotClass  = 0;
    usEgprsMultiSlotClass = 0;
    usEgprsFlag           = 0;

    ulResult = NV_ReadEx(MODEM_ID_0, en_NV_Item_Egprs_Flag, &usEgprsFlag, sizeof(VOS_UINT16));
    if ( NV_OK != ulResult )
    {
        usEgprsFlag = 0x01;
    }

    ulResult = NV_ReadEx(MODEM_ID_0, en_Nv_Item_Gprs_Multi_Slot_Class, &usGprsMultiSlotClass, sizeof(VOS_UINT16));

    /* 读取失败，默认多时隙能力等级为12 */
    if ( NV_OK != ulResult )
    {
        usGprsMultiSlotClass = AT_GAS_GRR_MULTISLOT_CLASS_MAX;
    }

    /* 多时隙能力等级取值为1-12 */
    if (( 0 == usGprsMultiSlotClass )
      ||( AT_GAS_GRR_MULTISLOT_CLASS_MAX < usGprsMultiSlotClass ))
    {
        usGprsMultiSlotClass = AT_GAS_GRR_MULTISLOT_CLASS_MAX;
    }

    ulResult = NV_ReadEx(MODEM_ID_0, en_NV_Item_Egprs_Multi_Slot_Class, &usEgprsMultiSlotClass, sizeof(VOS_UINT16));
    if ( NV_OK != ulResult )
    {
        usEgprsMultiSlotClass = AT_GAS_GRR_MULTISLOT_CLASS_MAX;
    }

    /* 多时隙能力等级取值为1-12 */
    if (( 0 == usEgprsMultiSlotClass )
      ||( AT_GAS_GRR_MULTISLOT_CLASS_MAX < usEgprsMultiSlotClass ))
    {
        usEgprsMultiSlotClass = AT_GAS_GRR_MULTISLOT_CLASS_MAX;
    }

    ulResult = NV_ReadEx(MODEM_ID_0, en_NV_Item_GAS_High_Multislot_Class,&stHighMultislotclass, sizeof(NVIM_GAS_HIGH_MULTISLOT_CLASS_STRU));

    /* 读取失败，默认High Multislot Class 无效  */
    if ( NV_OK != ulResult )
    {
        return  ;
    }

    if((VOS_TRUE == stHighMultislotclass.usHighMultislotClassFlg)
     &&(AT_GAS_HIGH_MULTISLOT_CLASS_0 != stHighMultislotclass.usHighMultislotClass))
    {
        stHighMultislotclass.usHighMultislotClass = AT_GAS_HIGH_MULTISLOT_CLASS_0;
    }

    AT_CalcGasCategory(&stHighMultislotclass,
                       usGprsMultiSlotClass,
                       usEgprsMultiSlotClass,
                       usEgprsFlag);

    return;
}


VOS_VOID AT_ReadPppDialErrCodeNV( VOS_VOID )
{
    NAS_NV_PPP_DIAL_ERR_CODE_STRU       stPppErrRpt;
    VOS_UINT32                          ulResult;
    VOS_UINT32                          ulLength;

    stPppErrRpt.ucStatus         = VOS_FALSE;
    stPppErrRpt.ucErrCodeRpt     = PPP_DIAL_ERR_CODE_BUTT;
    ulLength                     = 0;

    (VOS_VOID)NV_GetLength(en_NV_Item_PPP_DIAL_ERR_CODE, &ulLength);
    ulResult = NV_ReadEx(MODEM_ID_0, en_NV_Item_PPP_DIAL_ERR_CODE,
                         &stPppErrRpt,
                         ulLength);

    if ((ulResult == NV_OK)
     && (VOS_TRUE == stPppErrRpt.ucStatus)
     && (PPP_DIAL_ERR_CODE_ENABLE == stPppErrRpt.ucErrCodeRpt))

    {
        gucPppDialErrCodeRpt = PPP_DIAL_ERR_CODE_ENABLE;
    }
    else
    {
        gucPppDialErrCodeRpt = PPP_DIAL_ERR_CODE_DISABLE;
    }

    return;
}


VOS_VOID AT_ReadReportRegActFlgNV( VOS_VOID )
{
    VOS_UINT32                          ulResult;
    TAF_AT_NVIM_REPORT_REG_ACT_FLG_STRU stReportRegFlg;

    TAF_MEM_SET_S(&stReportRegFlg, sizeof(stReportRegFlg), 0x00, sizeof(TAF_AT_NVIM_REPORT_REG_ACT_FLG_STRU));

    ulResult = NV_ReadEx(MODEM_ID_0, en_NV_Item_REPORT_REG_ACT_FLG,
                         &(stReportRegFlg.usReportRegActFlg),
                         sizeof(stReportRegFlg.usReportRegActFlg));
    if (NV_OK != ulResult)
    {
        g_usReportCregActParaFlg = VOS_FALSE;
    }
    else
    {
        g_usReportCregActParaFlg = stReportRegFlg.usReportRegActFlg;
    }

    return;
}


VOS_VOID AT_ReadCregAndCgregCiFourByteRptNV( VOS_VOID )
{
    NAS_NV_CREG_CGREG_CI_FOUR_BYTE_RPT_STRU     stCiFourByteRpt;
    VOS_UINT32                                  ulResult;
    VOS_UINT32                                  ulLength;

    stCiFourByteRpt.ucStatus     = VOS_FALSE;
    stCiFourByteRpt.ucCiBytesRpt = 0;
    ulLength                     = 0;

    (VOS_VOID)NV_GetLength(en_NV_Item_CREG_CGREG_CI_Four_Byte_Rpt, &ulLength);
    ulResult = NV_ReadEx(MODEM_ID_0, en_NV_Item_CREG_CGREG_CI_Four_Byte_Rpt,
                         &stCiFourByteRpt,
                         ulLength);
    if ((NV_OK == ulResult)
     && (VOS_TRUE == stCiFourByteRpt.ucStatus)
     && (CREG_CGREG_CI_RPT_FOUR_BYTE == stCiFourByteRpt.ucCiBytesRpt))
    {
        gucCiRptByte = CREG_CGREG_CI_RPT_FOUR_BYTE;
    }
    else
    {
        gucCiRptByte = CREG_CGREG_CI_RPT_TWO_BYTE;
    }

    return;
}


VOS_VOID AT_ReadIpv6CapabilityNV( VOS_VOID )
{
    AT_NV_IPV6_CAPABILITY_STRU          stNvIpv6Capability;
    VOS_UINT32                          ulRslt;
    AT_COMM_PS_CTX_STRU                *pstCommPsCtx = VOS_NULL_PTR;
    VOS_UINT32                          ulIpv6CapabilityValid;

    TAF_MEM_SET_S(&stNvIpv6Capability, sizeof(stNvIpv6Capability), 0x00, sizeof(AT_NV_IPV6_CAPABILITY_STRU));

    pstCommPsCtx = AT_GetCommPsCtxAddr();

    ulRslt = NV_ReadEx(MODEM_ID_0,
                       en_NV_Item_IPV6_CAPABILITY,
                       &stNvIpv6Capability,
                       sizeof(AT_NV_IPV6_CAPABILITY_STRU));

    ulIpv6CapabilityValid = AT_PS_IsIpv6CapabilityValid(stNvIpv6Capability.ucIpv6Capablity);

    if ((NV_OK == ulRslt)
     && (VOS_TRUE == stNvIpv6Capability.ucStatus)
     && (VOS_TRUE == ulIpv6CapabilityValid))
    {
        pstCommPsCtx->ucIpv6Capability = stNvIpv6Capability.ucIpv6Capablity;
    }
    else
    {
        pstCommPsCtx->ucIpv6Capability = AT_IPV6_CAPABILITY_IPV4_ONLY;
    }

    return;
}

/* Added by l60609 for V9R1 IPv6&TAF/SM Project, 2013-4-24, begin */

VOS_VOID AT_ReadIpv6BackProcExtCauseNV(VOS_VOID)
{
    AT_PS_IPV6_BACKPROC_EXT_CAUSE_STRU *pstPsBackProcExtCause = VOS_NULL_PTR;
    TAF_NV_IPV6_FALLBACK_EXT_CAUSE_STRU stNvBackProcExtCause;
    VOS_UINT32                          ulCauseNum;
    VOS_UINT32                          ulRslt;
    VOS_UINT32                          ulCnt;

    pstPsBackProcExtCause = &(AT_GetCommPsCtxAddr()->stIpv6BackProcExtCauseTbl);
    ulCauseNum            = 0;

    TAF_MEM_SET_S(&stNvBackProcExtCause, sizeof(stNvBackProcExtCause), 0x00, sizeof(TAF_NV_IPV6_FALLBACK_EXT_CAUSE_STRU));

    ulRslt = NV_ReadEx(MODEM_ID_0,
                       en_NV_Item_IPV6_BACKPROC_EXT_CAUSE,
                       &stNvBackProcExtCause,
                       sizeof(TAF_NV_IPV6_FALLBACK_EXT_CAUSE_STRU));

    if ( (NV_OK == ulRslt)
      && (VOS_TRUE == stNvBackProcExtCause.ulActiveFlag) )
    {
        for (ulCnt = 0 ; ulCnt < TAF_NV_IPV6_FALLBACK_EXT_CAUSE_MAX_NUM; ulCnt++)
        {
            /* 检查(E)SM Cause是否有效 */
            if (!TAF_PS_IS_SM_CAUSE_VALID(stNvBackProcExtCause.aucSmCause[ulCnt]))
            {
                continue;
            }
            else
            {
                pstPsBackProcExtCause->aenPsCause[ulCauseNum] = TAF_PS_CONVERT_SM_CAUSE_TO_PS_CAUSE(stNvBackProcExtCause.aucSmCause[ulCnt]);
                ulCauseNum++;
            }
        }
    }

    pstPsBackProcExtCause->ulCauseNum = ulCauseNum;

    return;
}


VOS_VOID AT_ReadIpv6AddrTestModeCfgNV(VOS_VOID)
{
    TAF_NVIM_IPV6_ADDR_TEST_MODE_CFG_STRU                   stIpv6AddrTestModeCfg;
    AT_COMM_PS_CTX_STRU                                    *pstCommPsCtx = VOS_NULL_PTR;
    VOS_UINT32                                              ulRslt;

    TAF_MEM_SET_S(&stIpv6AddrTestModeCfg, sizeof(stIpv6AddrTestModeCfg), 0x00, sizeof(TAF_NVIM_IPV6_ADDR_TEST_MODE_CFG_STRU));

    pstCommPsCtx = AT_GetCommPsCtxAddr();

    ulRslt = NV_ReadEx(MODEM_ID_0,
                       en_NV_Item_Ipv6_Address_Test_Mode_Cfg,
                       &stIpv6AddrTestModeCfg,
                       sizeof(stIpv6AddrTestModeCfg));

    if ((NV_OK == ulRslt))
    {
        pstCommPsCtx->ulIpv6AddrTestModeCfg = stIpv6AddrTestModeCfg.ulIpv6AddrTestModeCfg;
    }
    else
    {
        pstCommPsCtx->ulIpv6AddrTestModeCfg = 0;
    }

    return;
}

/* Added by l60609 for V9R1 IPv6&TAF/SM Project, 2013-4-24, end */


VOS_VOID AT_ReadSharePdpInfoNV(VOS_VOID)
{
    TAF_NVIM_SHARE_PDP_INFO_STRU        stSharePdpInfo;
    AT_COMM_PS_CTX_STRU                *pstCommPsCtx = VOS_NULL_PTR;
    VOS_UINT32                          ulRslt;

    TAF_MEM_SET_S(&stSharePdpInfo, sizeof(stSharePdpInfo), 0x00, sizeof(TAF_NVIM_SHARE_PDP_INFO_STRU));

    pstCommPsCtx = AT_GetCommPsCtxAddr();

    ulRslt = NV_ReadEx(MODEM_ID_0,
                       en_NV_Item_SHARE_PDP_INFO,
                       &stSharePdpInfo,
                       sizeof(stSharePdpInfo));

    if (NV_OK == ulRslt)
    {
        pstCommPsCtx->ucSharePdpFlag = stSharePdpInfo.ucEnableFlag;
    }
    else
    {
        pstCommPsCtx->ucSharePdpFlag = VOS_FALSE;
    }

    return;
}


VOS_VOID AT_ReadDialConnectDisplayRateNV(VOS_VOID)
{
    VOS_UINT32                                              ulResult;
    AT_NVIM_DIAL_CONNECT_DISPLAY_RATE_STRU                  stDialConnectDisplayRate;

    TAF_MEM_SET_S(&stDialConnectDisplayRate, sizeof(stDialConnectDisplayRate), 0x00, sizeof(stDialConnectDisplayRate));


    ulResult = NV_ReadEx(MODEM_ID_0, en_NV_Item_DIAL_CONNECT_DISPLAY_RATE,
                         &stDialConnectDisplayRate,
                         sizeof(AT_NVIM_DIAL_CONNECT_DISPLAY_RATE_STRU));
    if (NV_OK != ulResult)
    {
        TAF_MEM_SET_S(&g_stDialConnectDisplayRate, sizeof(g_stDialConnectDisplayRate), 0x00, sizeof(g_stDialConnectDisplayRate));
		return;
    }

    g_stDialConnectDisplayRate.ucDpaConnectRate             = stDialConnectDisplayRate.ucDpaConnectRate;
    g_stDialConnectDisplayRate.ucEdgeConnectRate            = stDialConnectDisplayRate.ucEdgeConnectRate;
    g_stDialConnectDisplayRate.ucGprsConnectRate            = stDialConnectDisplayRate.ucGprsConnectRate;
    g_stDialConnectDisplayRate.ucGsmConnectRate             = stDialConnectDisplayRate.ucGsmConnectRate;
    g_stDialConnectDisplayRate.ucReserve1                   = stDialConnectDisplayRate.ucReserve1;
    g_stDialConnectDisplayRate.ucReserve2                   = stDialConnectDisplayRate.ucReserve2;
    g_stDialConnectDisplayRate.ucReserve3                   = stDialConnectDisplayRate.ucReserve3;
    g_stDialConnectDisplayRate.ucWcdmaConnectRate           = stDialConnectDisplayRate.ucWcdmaConnectRate;

    return;
}


VOS_VOID  AT_ReadPsNV(VOS_VOID)
{
    /* 读取拨号错误码上报NV */
    AT_ReadPppDialErrCodeNV();

    /* 读取是否上报CREG/CGREG的ACT参数控制NV */
    AT_ReadReportRegActFlgNV();

    /* 读取CREG/CGREG的CI参数是否以4字节上报的控制NV(Vodafone需求) */
    AT_ReadCregAndCgregCiFourByteRptNV();

    /* 读取IPV6能力的NV, 目前IPV6能力只能支持到IPV4V6_OVER_ONE_PDP */
    AT_ReadIpv6CapabilityNV();

    /* Added by l60609 for V9R1 IPv6&TAF/SM Project, 2013-4-24, begin */
    /* 读取IPv6回退处理扩展原因值 */
    AT_ReadIpv6BackProcExtCauseNV();
    /* Added by l60609 for V9R1 IPv6&TAF/SM Project, 2013-4-24, end */

    AT_ReadIpv6AddrTestModeCfgNV();

    /* 读取拨号系统托盘显示速率定制NV */
    AT_ReadDialConnectDisplayRateNV();

    /* 读取WCDMA接入等级NV, 用于PS域拨号速率上报 */
    AT_ReadWasCapabilityNV();

    /* 读取GAS是否支持CLASS33 NV项目 */
    AT_ReadGasCapabilityNV();

    AT_ReadCellSignReportCfgNV();

    AT_ReadSharePdpInfoNV();

    /* 读取网侧无原因值重拨功能开关 */
    AT_ReadRedialNoCauseFlagNV();

    return;
}


VOS_VOID  AT_ReadCsNV(VOS_VOID)
{
    TAF_USSD_NVIM_TRANS_MODE_STRU       stUssdTranMode;
    TAF_CCA_TELE_PARA_STRU              stTimeInfo;
    MODEM_ID_ENUM_UINT16                enModemId;
    AT_MODEM_CC_CTX_STRU               *pstCcCtx = VOS_NULL_PTR;
    AT_MODEM_SS_CTX_STRU               *pstSsCtx = VOS_NULL_PTR;
    VOS_UINT32                          ulLength;

    ulLength = 0;

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        pstCcCtx = AT_GetModemCcCtxAddrFromModemId(enModemId);
        pstSsCtx = AT_GetModemSsCtxAddrFromModemId(enModemId);

        TAF_MEM_SET_S(&stTimeInfo, sizeof(stTimeInfo), 0x00, sizeof(stTimeInfo));

        /*读取语音自动应答 */
        if(NV_OK == NV_ReadEx(enModemId,
                              en_NV_Item_CCA_TelePara,
                              &stTimeInfo,
                              sizeof(TAF_CCA_TELE_PARA_STRU)))
        {
            pstCcCtx->stS0TimeInfo.ucS0TimerLen = stTimeInfo.ucS0TimerLen;
        }

        TAF_MEM_SET_S(&stUssdTranMode, sizeof(stUssdTranMode), 0x00, sizeof(stUssdTranMode));

        (VOS_VOID)NV_GetLength(en_NV_Item_CUST_USSD_MODE, &ulLength);
        /*读取USSD是否透传设置 */
        if (NV_OK == NV_ReadEx(MODEM_ID_0,
                               en_NV_Item_CUST_USSD_MODE,
                               &stUssdTranMode,
                               ulLength))
        {
            if (VOS_TRUE == stUssdTranMode.ucStatus)
            {
                pstSsCtx->usUssdTransMode = stUssdTranMode.ucUssdTransMode;
            }
        }

    }

    AT_ReadSsNV();

    return;
}


VOS_VOID AT_ReadLTENV(VOS_VOID)
{
    VOS_UINT32 ulResult = NV_OK;

    TAF_MEM_SET_S(&g_stRsrpCfg, sizeof(g_stRsrpCfg), 0x00, sizeof(g_stRsrpCfg));
    TAF_MEM_SET_S(&g_stRscpCfg, sizeof(g_stRscpCfg), 0x00, sizeof(g_stRscpCfg));
    TAF_MEM_SET_S(&g_stEcioCfg, sizeof(g_stEcioCfg), 0x00, sizeof(g_stEcioCfg));

    /* 从NV中获取门限值 */
    ulResult = NVM_Read(EN_NV_ID_RSRP_CFG, &g_stRsrpCfg, sizeof(NVIM_RSRP_CFG_STRU));

    if(NV_OK != ulResult)
    {
        (VOS_VOID)vos_printf("read RSRP NV fail!\n");
        return ;
    }

    /* 从NV中获取门限值 */
    ulResult = NVM_Read(EN_NV_ID_RSCP_CFG, &g_stRscpCfg, sizeof(NVIM_RSCP_CFG_STRU));

    if(NV_OK != ulResult)
    {
        (VOS_VOID)vos_printf("read RSCP NV fail!\n");
        return ;
    }

    /* 从NV中获取门限值 */
    ulResult = NVM_Read(EN_NV_ID_ECIO_CFG, &g_stEcioCfg, sizeof(NVIM_ECIO_CFG_STRU));

    if(NV_OK != ulResult)
    {
        (VOS_VOID)vos_printf("read ECIO NV fail!\n");
        return ;
    }

    return;
}



VOS_VOID AT_ReadAgpsNv(VOS_VOID)
{
    AT_MODEM_AGPS_CTX_STRU             *pstAgpsCtx      = VOS_NULL_PTR;
    TAF_NVIM_XCPOSRRPT_CFG_STRU         stXcposrRptCfg;
    MODEM_ID_ENUM_UINT16                enModemId;

    TAF_MEM_SET_S(&stXcposrRptCfg, (VOS_SIZE_T)sizeof(stXcposrRptCfg), 0x00, (VOS_SIZE_T)sizeof(stXcposrRptCfg));

    /* 读取NV项 */
    if (NV_OK == NV_Read(en_NV_Item_XCPOSRRPT_CFG, &stXcposrRptCfg, sizeof(stXcposrRptCfg)))
    {
        for (enModemId = MODEM_ID_0; enModemId < MODEM_ID_BUTT; enModemId++)
        {
            pstAgpsCtx = AT_GetModemAgpsCtxAddrFromModemId(enModemId);
            pstAgpsCtx->enCposrReport   = stXcposrRptCfg.ucCposrDefault;
            pstAgpsCtx->enXcposrReport  = stXcposrRptCfg.ucXcposrRptDefault;
        }
    }
    else
    {
        AT_ERR_LOG("AT_ReadAgpsNv: NV_Read Failed!");
    }

    return;
}


VOS_VOID  AT_ReadNV(VOS_VOID)
{
    /* 平台接入技术NV读取 */
    AT_ReadPlatformNV();

    /* client NV读取 */
    AT_ReadClientConfigNV();


    /* 读取系统相关的NV项 */
    AT_ReadSysNV();

    /* 读取PS域相关的NV项 */
    AT_ReadPsNV();

    /* 读取cs域（CC和SS相关的NV) */
    AT_ReadCsNV();

    /* 读取SMS相关的NV */
    AT_ReadSmsNV();

    /* 读取LTE NV项 */
    AT_ReadLTENV();



    AT_ReadPortBuffCfgNV();

    AT_ReadPrivacyFilterCfgNv();

    AT_ReadAgpsNv();

    AT_ReadCdmaModemSwitchNotResetCfgNv();

    return;
}


VOS_VOID AT_WriteCcpuResetRecordNvim(VOS_UINT8 ucCcpuResetFlag)
{
    VOS_UINT32                          ulResult;
    VOS_UINT32                          ulLength;
    TAF_NVIM_CCPU_RESET_RECORD_STRU     stCcpuResetRecord;

    ulLength = 0;

    TAF_MEM_SET_S(&stCcpuResetRecord, sizeof(stCcpuResetRecord), 0x00, sizeof(TAF_NVIM_CCPU_RESET_RECORD_STRU));

    (VOS_VOID)NV_GetLength(en_NV_Item_Ccpu_Reset_Record, &ulLength);
    if (ulLength > sizeof(TAF_NVIM_CCPU_RESET_RECORD_STRU))
    {
        return;
    }

    stCcpuResetRecord.ucCcpuResetFlag = ucCcpuResetFlag;

    ulResult = NV_WriteEx(MODEM_ID_0, en_NV_Item_Ccpu_Reset_Record, &stCcpuResetRecord, ulLength);

    if (NV_OK != ulResult)
    {
        AT_WARN_LOG("AT_WriteCcpuResetRecordNvim: en_NV_Item_Ccpu_Reset_Record NV Write Fail!");
    }

    return;
}


VOS_UINT32  AT_ReadPhyNV(VOS_VOID)
{
    VOS_BOOL                            bImeiIsNull;
    MODEM_ID_ENUM_UINT16                enModemId;

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        if (AT_OK != AT_PhyNumIsNull(enModemId, AT_PHYNUM_TYPE_IMEI, &bImeiIsNull))
        {
            return VOS_ERR;
        }

        if (VOS_TRUE != bImeiIsNull)
        {
            return VOS_ERR;
        }
    }

    return VOS_OK;
}


VOS_VOID AT_InitDeviceCmd(VOS_VOID)
{
    if (VOS_OK != AT_ReadPhyNV())
    {
        return;
    }

    /* IMEI为全零时，认为DATALOCK解锁状态 */
    g_bAtDataLocked = VOS_FALSE;

    return;
}


VOS_VOID AT_InitStk(VOS_VOID)
{
    USIMM_STK_CFG_STRU                  stProfile;

    TAF_MEM_SET_S(&stProfile, sizeof(stProfile), 0x00, sizeof(stProfile));

    if(NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_TerminalProfile_Set, &stProfile, sizeof(USIMM_STK_CFG_STRU)))
    {
        AT_ERR_LOG("AT_StkInit: read en_NV_Item_TerminalProfile_Set fail.");

        g_ulSTKFunctionFlag = VOS_FALSE;
    }
    else
    {
        g_ulSTKFunctionFlag = stProfile.ucFuncEnable;
    }

    return;
}


VOS_VOID AT_InitPara(VOS_VOID)
{
    VOS_UINT8                          *pucSystemAppConfig;
    VOS_UINT8                           ucAtCmdNotSupportStr1[]   = "ERROR";
    VOS_UINT8                           ucAtCmdNotSupportStr2[]   = "COMMAND NOT SUPPORT";
    MODEM_ID_ENUM_UINT16                enModemId;
    AT_MODEM_NET_CTX_STRU              *pstNetCtx = VOS_NULL_PTR;

    pucSystemAppConfig                  = AT_GetSystemAppConfigAddr();

    /* 根据产品类型分别进行处理 */
    switch(*pucSystemAppConfig)
    {
        /* E5 */
        case SYSTEM_APP_WEBUI:

            pstNetCtx = AT_GetModemNetCtxAddrFromModemId(MODEM_ID_0);

            /*E5版本对不支持命令返回ERROR*/
            TAF_MEM_CPY_S((VOS_CHAR*)gaucAtCmdNotSupportStr,
                       AT_NOTSUPPORT_STR_LEN + 4,
                       (VOS_CHAR*)ucAtCmdNotSupportStr1,
                       sizeof(ucAtCmdNotSupportStr1));

            /* E5的默认格式改为数字格式 */
            pstNetCtx->ucCopsFormatType = AT_COPS_NUMERIC_TYPE;

            /* E5错误码默认错误编号 */
            gucAtCmeeType   = AT_CMEE_ERROR_CODE;

            break;

         /* STICK*/
        case SYSTEM_APP_MP:

            pstNetCtx = AT_GetModemNetCtxAddrFromModemId(MODEM_ID_0);

            /*命令不支持提示字串*/
            TAF_MEM_CPY_S((VOS_CHAR*)gaucAtCmdNotSupportStr,
                       AT_NOTSUPPORT_STR_LEN + 4,
                       (VOS_CHAR*)ucAtCmdNotSupportStr2,
                       sizeof(ucAtCmdNotSupportStr2));

            pstNetCtx->ucCopsFormatType = AT_COPS_LONG_ALPH_TYPE;

            /* 指示错误命令返回码类型 */
            gucAtCmeeType   = AT_CMEE_ERROR_CONTENT;


            break;

        /* PAD,目前没有，为保持与原代码一致性，暂按stick处理*/
        case SYSTEM_APP_ANDROID:

            /*命令不支持提示字串*/
            TAF_MEM_CPY_S((VOS_CHAR*)gaucAtCmdNotSupportStr,
                       AT_NOTSUPPORT_STR_LEN + 4,
                       (VOS_CHAR*)ucAtCmdNotSupportStr2,
                       sizeof(ucAtCmdNotSupportStr2));

            for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
            {
                pstNetCtx = AT_GetModemNetCtxAddrFromModemId(enModemId);
                pstNetCtx->ucCopsFormatType = AT_COPS_LONG_ALPH_TYPE;
            }

            /* 指示错误命令返回码类型 */
            gucAtCmeeType   = AT_CMEE_ERROR_CODE;

            break;

        default:
            break;
    }

    return;
}


VOS_VOID AT_UsbSwitchGwMode(VOS_VOID)
{
    AT_PDP_ENTITY_STRU                 *pstAppPdpEntity;
    VOS_UINT32                          ulResult;
    VOS_UINT8                           aucCid[TAF_MAX_CID+1];

    pstAppPdpEntity                     = AT_APP_GetPdpEntInfoAddr();

    g_enHiLinkMode                      = AT_HILINK_GATEWAY_MODE;

    /* 初始化CID列表 */
    TAF_MEM_SET_S(aucCid, sizeof(aucCid), 0x00, sizeof(aucCid));

    /* 如果所有的PDP都处于IDLE, 返回; 如果有一个处于DEACTING, 返回 */
    ulResult = AT_AppCheckPdpIdleState();
    if (VOS_TRUE == ulResult)
    {
        AT_NORM_LOG("AT_UsbSwitchGwMode: The state is already IDLE.");
        return;
    }

    if ( (AT_PDP_STATE_ACTING == pstAppPdpEntity->enIpv4v6State)
      || (AT_PDP_STATE_ACTED  == pstAppPdpEntity->enIpv4v6State) )
    {
        if (VOS_OK != TAF_PS_CallEnd(WUEPS_PID_AT,
                                     AT_PS_BuildExClientId(gastAtClientTab[AT_CLIENT_TAB_APP_INDEX].usClientId),
                                     0,
                                     g_stAtAppPdpEntity.ucIpv4v6Cid))
        {
            AT_WARN_LOG("AT_UsbSwitchGwMode(): Disconnect IPv4v6 fail");
        }

        AT_AppSetPdpState(TAF_PDP_IPV4V6, AT_PDP_STATE_DEACTING);
    }
    else
    {
        if ( (AT_PDP_STATE_ACTING == pstAppPdpEntity->enIpv4State)
          || (AT_PDP_STATE_ACTED  == pstAppPdpEntity->enIpv4State) )
        {
            if (VOS_OK != TAF_PS_CallEnd(WUEPS_PID_AT,
                                         AT_PS_BuildExClientId(gastAtClientTab[AT_CLIENT_TAB_APP_INDEX].usClientId),
                                         0,
                                         pstAppPdpEntity->ucIpv4Cid))
            {
                AT_WARN_LOG("AT_UsbSwitchGwMode():TAF_PS_CallEnd fail");
            }

            AT_AppSetPdpState(TAF_PDP_IPV4, AT_PDP_STATE_DEACTING);
        }

        if ( (AT_PDP_STATE_ACTING == pstAppPdpEntity->enIpv6State)
          || (AT_PDP_STATE_ACTED  == pstAppPdpEntity->enIpv6State) )
        {
            if (VOS_OK != TAF_PS_CallEnd(WUEPS_PID_AT,
                                         AT_PS_BuildExClientId(gastAtClientTab[AT_CLIENT_TAB_APP_INDEX].usClientId),
                                         0,
                                         pstAppPdpEntity->ucIpv6Cid))
            {
                AT_WARN_LOG("AT_UsbSwitchGwMode():TAF_PS_CallEnd fail");
            }

            AT_AppSetPdpState(TAF_PDP_IPV6, AT_PDP_STATE_DEACTING);
        }
    }

    return;
}


VOS_VOID AT_UsbEnableCB(VOS_VOID)
{
    /* USB MODEM口链路的建立 */
    AT_UsbModemInit();

    /* 注册打开NCM的接口,NCM端口打开和关闭由DRV调用协议栈接口负责打开或关闭 */
    AT_OpenUsbNdis();
}


VOS_VOID AT_UsbDisableCB(VOS_VOID)
{
    (VOS_VOID)vos_printf("AT_UsbDisableCB\r\n");
    /* USB MODEM口链路的建立 */
    AT_UsbModemClose();

    /* 注册关闭NCM的接口,NCM端口打开和关闭由DRV调用协议栈接口负责打开或关闭 */
    AT_CloseUsbNdis();

    /* 从记录表中删除USB相关的client ID */
    AT_RmUsedClientIdFromTab(AT_CLIENT_ID_PCUI);
    AT_RmUsedClientIdFromTab(AT_CLIENT_ID_CTRL);
    AT_RmUsedClientIdFromTab(AT_CLIENT_ID_NDIS);
    AT_RmUsedClientIdFromTab(AT_CLIENT_ID_MODEM);
    AT_RmUsedClientIdFromTab(AT_CLIENT_ID_PCUI2);
}



VOS_VOID AT_InitPort(VOS_VOID)
{
    VOS_UINT8                           i;

    for (i = 0; i < AT_CLIENT_BUTT; i++)
    {
        g_alAtUdiHandle[i] = UDI_INVALID_HANDLE;
    }

    /* 为了保证PC回放工程和 NAS GTR PC工程中SDT USBCOM AT端口的注册client ID
       相同，必须让USB COM口第一个注册 */
    /* USB PCUI口链路的建立 */
    At_UsbPcuiEst(AT_USB_COM_PORT_NO);

    /* USB Control口链路的建立 */
    At_UsbCtrEst(AT_CTR_PORT_NO);

    /* USB PCUI2口链路的建立 */
    At_UsbPcui2Est(AT_PCUI2_PORT_NO);

    /* UART口链路的建立 */
    AT_UART_InitPort();


    /* NDIS MODEM口链路的建立 */
    AT_UsbNdisEst();

    mdrv_usb_reg_enablecb((USB_UDI_ENABLE_CB_T)AT_UsbEnableCB);
    mdrv_usb_reg_disablecb((USB_UDI_ENABLE_CB_T)AT_UsbDisableCB);

    /* APP 建立通道 */
    AT_AppComEst();

    /* 注册DRV回调函数指针 */
    if (VOS_ERROR == DRV_USB_NAS_SWITCH_GATEWAY_REGFUNC((USB_NET_DEV_SWITCH_GATEWAY)AT_UsbSwitchGwMode))
    {
        AT_ERR_LOG( "AT_InitPort: DRV_USB_NAS_SWITCH_GATEWAY_REGFUNC Failed!" );
    }

    AT_SockComEst(AT_SOCK_PORT_NO);

    AT_AppSockComEst(AT_APP_SOCK_PORT_NO);



    return;
}



VOS_UINT32  At_PidInit(enum VOS_INIT_PHASE_DEFINE enPhase)
{
    switch ( enPhase )
    {
        case VOS_IP_INITIAL:

            /* 初始化AT的上下文 */
            AT_InitCtx();


            AT_InitPortBuffCfg();

            AT_InitTraceMsgTab();

            /* 初始化复位相关的上下文 */
            AT_InitResetCtx();

            /*读取NV项*/
            AT_ReadNV();

            /*AT 解析器初始化*/
            At_ParseInit();

            /*注册AT命令表*/
            At_RegisterBasicCmdTable();
            At_RegisterExCmdTable();
            At_RegisterExPrivateCmdTable();
            At_RegisterDeviceCmdTable();
            At_RegisterDeviceCmdTLTable();
            At_RegisterTLCmdTable();                  /* ADDED by c64416 for V9R1/V7R1 AT, 2013/09/18 */
            /* 装备初始化 */
            AT_InitDeviceCmd();

            /* STK在AT模块的初始化 */
            AT_InitStk();

            /* AT模块参数的初始化 */
            AT_InitPara();

            /* AT消息数量控制上下文初始化 */
            AT_InitMsgNumCtrlCtx();

            /* 端口初始化 */
            AT_InitPort();

            /* 初始化g_stFcIdMaptoFcPri */
            AT_InitFcMap();

            /* 向流控模块注册动态调整NCM/ECM组包参数的回调接口 */
            FC_ACORE_RegDrvAssemFunc((FC_ACORE_DRV_ASSEMBLE_PARA_FUNC)AT_UsbEthDeviceAccumTuneCB,
                                     (FC_ACORE_DRV_ASSEMBLE_PARA_FUNC)AT_UsbEthHostAccumTuneCB);


            /* 给低软注册回调函数，用于C核单独复位的处理 */
            mdrv_sysboot_register_reset_notify(NAS_AT_FUNC_PROC_NAME,
                                               AT_CCpuResetCallback,
                                               0,
                                               ACPU_RESET_PRIORITY_AT);

            /* 给低软注册回调函数，用于HIFI单独复位的处理 */
            hifireset_regcbfunc(NAS_AT_FUNC_PROC_NAME,
                                    AT_HifiResetCallback,
                                    0,
                                    ACPU_RESET_PRIORITY_AT);

            AT_InitMntnCtx();

            /* 注册层间消息过滤函数 */
            PS_OM_LayerMsgFilterInit();
            PS_OM_LayerMsgFilterFuncReg(GUNAS_OM_LayerMsgFilter);
            break;

        default:
            break;
    }

    return VOS_OK;
}


VOS_VOID AT_ReadSsNV( VOS_VOID )
{
    TAF_MEM_SET_S(&g_stAtSsCustomizePara, sizeof(g_stAtSsCustomizePara), 0x00, sizeof(g_stAtSsCustomizePara));

    /* 读取SS定制的NV项到全局变量 */
    if(NV_OK != NV_ReadEx(MODEM_ID_0,
                          en_NV_Item_SS_CUSTOMIZE_PARA,
                          &g_stAtSsCustomizePara,
                          sizeof(g_stAtSsCustomizePara)))
    {
        g_stAtSsCustomizePara.ucStatus = VOS_FALSE;
    }

    return;
}



VOS_VOID AT_ReadRedialNoCauseFlagNV(VOS_VOID)
{
    AT_COMM_PS_CTX_STRU                        *pstCommPsCtx = VOS_NULL_PTR;
    TAF_NV_PDP_REDIAL_FOR_NO_CAUSE_CFG_STRU     stNvRedialForNoCauseCfg;
    VOS_UINT32                                  ulRslt;

    TAF_MEM_SET_S(&stNvRedialForNoCauseCfg, (VOS_SIZE_T)sizeof(stNvRedialForNoCauseCfg),
                  0x00, (VOS_SIZE_T)sizeof(TAF_NV_PDP_REDIAL_FOR_NO_CAUSE_CFG_STRU));

    pstCommPsCtx = AT_GetCommPsCtxAddr();

    ulRslt = NV_ReadEx(MODEM_ID_0,
                       en_NV_Item_PDP_REDIAL_FOR_NO_CAUSE_CFG,
                       &stNvRedialForNoCauseCfg,
                       (VOS_SIZE_T)sizeof(TAF_NV_PDP_REDIAL_FOR_NO_CAUSE_CFG_STRU));

    if ((NV_OK == ulRslt)
     && (VOS_FALSE == stNvRedialForNoCauseCfg.ucEnable))
    {
        pstCommPsCtx->ucRedialForNoCauseFlag = stNvRedialForNoCauseCfg.ucEnable;
    }
    else
    {
        pstCommPsCtx->ucRedialForNoCauseFlag = VOS_TRUE;
    }

    return;
}


VOS_VOID AT_ReadCdmaModemSwitchNotResetCfgNv(VOS_VOID)
{
    AT_MODEM_CDMAMODEMSWITCH_CTX_STRU                      *pstCdmaModemSwitchCtx = VOS_NULL_PTR;
    TAF_NVIM_CDMAMODEMSWITCH_NOT_RESET_CFG_STRU             stCdmaModemSwitchNvCfg;
    MODEM_ID_ENUM_UINT16                                    enModemId;

    /* 初始化 */
    TAF_MEM_SET_S(&stCdmaModemSwitchNvCfg, (VOS_UINT32)sizeof(TAF_NVIM_CDMAMODEMSWITCH_NOT_RESET_CFG_STRU), 0x00, (VOS_UINT32)sizeof(TAF_NVIM_CDMAMODEMSWITCH_NOT_RESET_CFG_STRU));

    /* 读取NV项 */
    if (NV_OK != NV_ReadEx(MODEM_ID_0,
                           en_NV_Item_CDMAMODEMSWITCH_NOT_RESET_CFG,
                           &stCdmaModemSwitchNvCfg,
                           (VOS_UINT32)sizeof(TAF_NVIM_CDMAMODEMSWITCH_NOT_RESET_CFG_STRU)))
    {
        stCdmaModemSwitchNvCfg.ucEnableFlg = VOS_FALSE;
    }

    if (VOS_TRUE != stCdmaModemSwitchNvCfg.ucEnableFlg)
    {
        stCdmaModemSwitchNvCfg.ucEnableFlg = VOS_FALSE;
    }

    for (enModemId = 0; enModemId < MODEM_ID_BUTT; enModemId++)
    {
        pstCdmaModemSwitchCtx = AT_GetModemCdmaModemSwitchCtxAddrFromModemId(enModemId);

        pstCdmaModemSwitchCtx->ucEnableFlg = stCdmaModemSwitchNvCfg.ucEnableFlg;
    }

    return;
}


