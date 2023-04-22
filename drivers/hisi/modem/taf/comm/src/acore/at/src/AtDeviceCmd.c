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
  1 í·???t°üo?
*****************************************************************************/
#include "AtParse.h"

#include "ATCmdProc.h"
/* Added by f62575 for SMALL IMAGE, 2012-1-3, begin */
#include "AtDeviceCmd.h"
#include "AtCheckFunc.h"
#include "mdrv.h"
/* Added by f62575 for SMALL IMAGE, 2012-1-3, end   */
#include "AtCmdMsgProc.h"

/* Delete #include "phyoaminterface.h", Because it is unused for A CORE, 2016-07-21, begin */
/* Delete #include "phyoaminterface.h", Because it is unused for A CORE, 2016-07-21, end */

#include "AtInputProc.h"

#include "AtTestParaCmd.h"


#include "LNvCommon.h"
#include "RrcNvInterface.h"
#include "msp_nvim.h"

#include "NasNvInterface.h"
#include "TafNvInterface.h"

#include "AtCmdMiscProc.h"

/* ADD by c64416 for V9R1/V7R1 AT, 2013/09/18 begin */
#include "at_lte_common.h"
/* ADD by c64416 for V9R1/V7R1 AT, 2013/09/18 end */

#include <linux/random.h>


/*****************************************************************************
    D-òé??′òó?′òμ?·?ê???μ?.C???toê?¨ò?
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_AT_DEVICECMD_C


/*****************************************************************************
  2 è???±?á??¨ò?
*****************************************************************************/

/* ?üá?êü?T±ê??oê?¨ò??μ?÷
#define CMD_TBL_E5_IS_LOCKED        (0x00000001)     2?êüE5???¨????μ??üá?
#define CMD_TBL_PIN_IS_LOCKED       (0x00000002)     2?êüPIN?????¨????μ??üá?
#define CMD_TBL_IS_E5_DOCK          (0x00000004)     E5 DOCK?üá?
#define CMD_TBL_CLAC_IS_INVISIBLE   (0x00000008)     +CLAC?üá??D2?ê?3???ê?μ??üá?
*/

VOS_UINT32                 g_ulNVRD = 0;
VOS_UINT32                 g_ulNVWR = 0;

AT_PAR_CMD_ELEMENT_STRU g_astAtDeviceCmdTbl[] = {
    {AT_CMD_GTIMER,
    AT_SetGTimerPara,    AT_NOT_SET_TIME,    AT_QryGTimerPara,      AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^GTIMER",   (VOS_UINT8*)"(0-429496728)"},

    {AT_CMD_RSIM,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    AT_QryRsimPara,        AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^RSIM",     VOS_NULL_PTR},

    {AT_CMD_PHYNUM,
    AT_SetPhyNumPara,    AT_NOT_SET_TIME,    AT_QryPhyNumPara,      AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PHYNUM",   VOS_NULL_PTR},

    {AT_CMD_CSVER,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    At_QryCsVer,           AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^CSVER",    VOS_NULL_PTR},

    {AT_CMD_QOS,
    At_SetQosPara,       AT_NOT_SET_TIME,    At_QryQosPara,         AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
   AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^QOS",      VOS_NULL_PTR},

    {AT_CMD_SDOMAIN,
    At_SetSDomainPara,   AT_NOT_SET_TIME,    At_QrySDomainPara,     AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SDOMAIN",  VOS_NULL_PTR},

    {AT_CMD_DPACAT,
    At_SetDpaCat,        AT_NOT_SET_TIME,    At_QryDpaCat,          AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^DPACAT",   VOS_NULL_PTR},

    {AT_CMD_HSSPT,
    AT_SetHsspt  ,       AT_NOT_SET_TIME,    AT_QryHsspt,         AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^HSSPT",    (VOS_UINT8*)"(0,1,2,6)"},

    {AT_CMD_PLATFORM,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    At_QryPlatForm,        AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PLATFORM", VOS_NULL_PTR},

    {AT_CMD_BSN,
    At_SetBsn,           AT_NOT_SET_TIME,    At_QryBsn,             AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^BSN",      VOS_NULL_PTR},

    {AT_CMD_SFM,
    At_SetSfm,          AT_SET_PARA_TIME,   At_QrySfm,            AT_NOT_SET_TIME,    VOS_NULL_PTR , AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SFM",     (VOS_UINT8*)"(0,1)"},

    /* Added by f62575 for SMALL IMAGE, 2012-1-3, begin */
    {AT_CMD_TMODE,
    At_SetTModePara,     AT_SET_PARA_TIME,   At_QryTModePara,       AT_QRY_PARA_TIME ,  At_TestTmodePara , AT_TEST_PARA_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^TMODE",    (VOS_UINT8*)"(0,1,2,3,4,11,12,13,14,15,16,17,18,19)"},
    /* Added by f62575 for SMALL IMAGE, 2012-1-3, end   */

    {AT_CMD_FCHAN,
    At_SetFChanPara,     AT_SET_PARA_TIME,   At_QryFChanPara,       AT_QRY_PARA_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_FCHAN_OTHER_ERR  ,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FCHAN",    (VOS_UINT8*)"(0-7),(0-255),(0-65535),(0-1)"},

    {AT_CMD_FTXON,
    At_SetFTxonPara,     AT_SET_PARA_TIME,   At_QryFTxonPara,       AT_QRY_PARA_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_FTXON_OTHER_ERR  ,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FTXON",    (VOS_UINT8*)"(0,1,2)"},

    {AT_CMD_FDAC,
    AT_SetFDac,          AT_SET_PARA_TIME,   AT_QryFDac,            AT_NOT_SET_TIME,    At_TestFdacPara , AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FDAC",     (VOS_UINT8*)"(0-65536)"},


    {AT_CMD_FRXON,
    At_SetFRxonPara,     AT_SET_PARA_TIME,   At_QryFRxonPara,       AT_QRY_PARA_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_FRXON_OTHER_ERR  ,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FRXON",    (VOS_UINT8*)"(0-1)"},

    {AT_CMD_FPA,
    At_SetFpaPara,       AT_SET_PARA_TIME,   At_QryFpaPara,         AT_NOT_SET_TIME,    VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_FPA_OTHER_ERR  ,      CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FPA",      (VOS_UINT8*)"(0-255)"},


    {AT_CMD_FLNA,
    At_SetFlnaPara,      AT_SET_PARA_TIME,   At_QryFlnaPara,        AT_QRY_PARA_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_FLNA_OTHER_ERR  ,     CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FLNA",     (VOS_UINT8*)"(0-255)"},

    {AT_CMD_FRSSI,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    At_QryFrssiPara,       AT_QRY_PARA_TIME,  VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FRSSI",    VOS_NULL_PTR},

    {AT_CMD_MDATE,
    AT_SetMDatePara,     AT_NOT_SET_TIME,    AT_QryMDatePara,       AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^MDATE",    (VOS_UINT8*)"(@time)"},

    {AT_CMD_FACINFO,
    AT_SetFacInfoPara,   AT_NOT_SET_TIME,    AT_QryFacInfoPara,     AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FACINFO",  (VOS_UINT8*)"(0,1),(@valueInfo)"},


    {AT_CMD_SD,
    At_SetSD,            AT_NOT_SET_TIME,    At_QrySD,              AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_SD_CARD_OTHER_ERR ,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SD",       (VOS_UINT8*)"(0-4),(0-429496728),(0-3)"},

    {AT_CMD_GPIOPL,
    At_SetGPIOPL,        AT_SET_PARA_TIME,   At_QryGPIOPL,          AT_QRY_PARA_TIME,  VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^GPIOPL",   (VOS_UINT8*)"(@GPIOPL)"},

    {AT_CMD_GETEXBANDINFO,
    AT_SetExbandInfoPara,      AT_NOT_SET_TIME,  VOS_NULL_PTR,            AT_NOT_SET_TIME,   VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,       CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^GETEXBANDINFO",     (VOS_UINT8*)"(101-116)"},

    {AT_CMD_GETEXBANDTESTINFO,
    AT_SetExbandTestInfoPara,      AT_NOT_SET_TIME, VOS_NULL_PTR ,     AT_NOT_SET_TIME,   VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,       CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^GETEXBANDTESTINFO",      (VOS_UINT8*)"(101-116),(14,50,100,150,200)"},
    /* éú2úNV???′ */
    {AT_CMD_INFORRS,
    At_SetInfoRRS,       AT_SET_PARA_TIME,    VOS_NULL_PTR,          AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^INFORRS",  VOS_NULL_PTR},

    {AT_CMD_INFORBU,
    atSetNVFactoryBack,  AT_SET_PARA_TIME,    VOS_NULL_PTR,          AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^INFORBU",  VOS_NULL_PTR},

    {AT_CMD_DATALOCK,
    At_SetDataLock,      AT_SET_PARA_TIME,   At_QryDataLock,        AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^DATALOCK", (VOS_UINT8*)"(@nlockCode)"},

    {AT_CMD_VERSION,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    At_QryVersion,         AT_QRY_PARA_TIME,  VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^VERSION",  VOS_NULL_PTR},



    {AT_CMD_SIMLOCK,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    At_QrySimLockPlmnInfo, AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SIMLOCK",  VOS_NULL_PTR},

    {AT_CMD_MAXLCK_TIMES,
    At_SetMaxLockTimes,  AT_SET_PARA_TIME,   At_QryMaxLockTimes,    AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^MAXLCKTMS",(VOS_UINT8*)"(0-429496728)"},

    {AT_CMD_CALLSRV,
    At_SetCallSrvPara,   AT_NOT_SET_TIME,    At_QryCallSrvPara,     AT_NOT_SET_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^CALLSRV",  (VOS_UINT8*)"(0,1)"},

    {AT_CMD_CSDFLT,
    At_SetCsdfltPara,    AT_NOT_SET_TIME,    At_QryCsdfltPara,      AT_QRY_PARA_TIME,   VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR,                       CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^CSDFLT",   (VOS_UINT8*)"(0,1)"},

    {AT_CMD_SECUBOOTFEATURE,
    VOS_NULL_PTR,    AT_NOT_SET_TIME,    At_QrySecuBootFeaturePara, AT_NOT_SET_TIME,  VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SECUBOOTFEATURE",  (VOS_UINT8*)"(0-3)"},

    /* Modified by f62575 for B050 Project, 2012-2-3, begin   */
    {AT_CMD_SECUBOOT,
    At_SetSecuBootPara,  AT_SET_PARA_TIME,    At_QrySecuBootPara,    AT_QRY_PARA_TIME,  VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SECUBOOT", (VOS_UINT8*)"(0-3)"},

    {AT_CMD_SETKEY,
    At_SetKeyPara,  AT_SET_PARA_TIME,    VOS_NULL_PTR,    AT_NOT_SET_TIME,  VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SETKEY", (VOS_UINT8*)"(1-4)"},

    {AT_CMD_GETKEYINFO,
    At_GetKeyInfoPara,  AT_SET_PARA_TIME,    VOS_NULL_PTR,    AT_NOT_SET_TIME,  VOS_NULL_PTR ,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^GETKEYINFO", (VOS_UINT8*)"(1-4)"},

    /* Modified by f62575 for B050 Project, 2012-2-3, end */

    {AT_CMD_TMMI,
    AT_SetTmmiPara,      AT_NOT_SET_TIME,    AT_QryTmmiPara,        AT_NOT_SET_TIME,   At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^TMMI",  (VOS_UINT8*)"(0,1)"},

    /* V7R1òa?óê1ó?"=?"2é?ˉ3?μ?ê1?ü×′ì? */
    {AT_CMD_TCHRENABLE,
    AT_SetChrgEnablePara,AT_NOT_SET_TIME,    AT_QryChrgEnablePara,  AT_NOT_SET_TIME,   AT_TestChrgEnablePara, AT_SET_PARA_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^TCHRENABLE",(VOS_UINT8*)"(0,1,4)"},

    {AT_CMD_TCHRINFO,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    AT_QryChrgInfoPara,    AT_NOT_SET_TIME,   At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^TCHRINFO",VOS_NULL_PTR},

    {AT_CMD_TSCREEN,
    AT_SetTestScreenPara,AT_NOT_SET_TIME,    VOS_NULL_PTR,          AT_NOT_SET_TIME,   At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^TSCREEN",  (VOS_UINT8*)"(0-255),(0-255)"},

    {AT_CMD_BATVOL,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    AT_QryBatVolPara,      AT_QRY_PARA_TIME,  At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^TBATVOLT",  VOS_NULL_PTR},

    {AT_CMD_WUPWD,
    AT_SetWebPwdPara,    AT_NOT_SET_TIME,    VOS_NULL_PTR,          AT_NOT_SET_TIME,   At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WUPWD",  (VOS_UINT8*)"(0,1),(@WUPWD)"},

    {AT_CMD_PRODTYPE,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    AT_QryProdTypePara,    AT_QRY_PARA_TIME,  At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PRODTYPE",  VOS_NULL_PTR},

    {AT_CMD_FEATURE,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    AT_QryFeaturePara,     AT_QRY_PARA_TIME,  At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SFEATURE",  VOS_NULL_PTR},

    {AT_CMD_PRODNAME,
    AT_SetProdNamePara,  AT_NOT_SET_TIME,    AT_QryProdNamePara,    AT_NOT_SET_TIME,   At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PRODNAME",  (VOS_UINT8*)"(@ProductName)"},

    {AT_CMD_FWAVE,
    AT_SetFwavePara,     AT_SET_PARA_TIME,   VOS_NULL_PTR,          AT_NOT_SET_TIME,   At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FWAVE",  (VOS_UINT8*)"(0-7),(0-65535)"},

    {AT_CMD_EQVER,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,    AT_QryEqverPara,       AT_NOT_SET_TIME,   VOS_NULL_PTR,     AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^EQVER",  VOS_NULL_PTR},

    {AT_CMD_WIENABLE,
    AT_SetWiFiEnablePara, AT_NOT_SET_TIME, AT_QryWiFiEnablePara, AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIENABLE",(VOS_UINT8*)"(0,1,2)"},

    {AT_CMD_WIMODE,
    AT_SetWiFiModePara,   AT_NOT_SET_TIME, AT_QryWiFiModePara,   AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIMODE",  (VOS_UINT8*)"(0,1,2,3,4)"},

    {AT_CMD_WIBAND,
    AT_SetWiFiBandPara,   AT_NOT_SET_TIME, AT_QryWiFiBandPara,   AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIBAND",  (VOS_UINT8*)"(0,1)"},

    {AT_CMD_WIFREQ,
    AT_SetWiFiFreqPara,   AT_NOT_SET_TIME, AT_QryWiFiFreqPara,   AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIFREQ",  (VOS_UINT8*)"(0-65535),(@offset)"},

    {AT_CMD_WIRATE,
    AT_SetWiFiRatePara,   AT_NOT_SET_TIME, AT_QryWiFiRatePara,   AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIDATARATE",  (VOS_UINT8*)"(0-65535)"},

    {AT_CMD_WIPOW,
    AT_SetWiFiPowerPara,  AT_NOT_SET_TIME, AT_QryWiFiPowerPara,  AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIPOW",    (VOS_UINT8*)"(0-65535)"},

    {AT_CMD_WITX,
    AT_SetWiFiTxPara,     AT_NOT_SET_TIME, AT_QryWiFiTxPara,     AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WITX",     (VOS_UINT8*)"(0,1)"},

    {AT_CMD_WIRX,
    AT_SetWiFiRxPara,     AT_NOT_SET_TIME, AT_QryWiFiRxPara,     AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIRX",     (VOS_UINT8*)"(0,1),(@smac),(@dmac)"},

    {AT_CMD_WIRPCKG,
    AT_SetWiFiPacketPara, AT_NOT_SET_TIME, AT_QryWiFiPacketPara, AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIRPCKG",  (VOS_UINT8*)"(0)"},

/* Add by z60575 for multi_ssid, 2012-9-5 begin */
    {AT_CMD_SSID,
    AT_SetWiFiSsidPara,   AT_NOT_SET_TIME, AT_QryWiFiSsidPara,   AT_NOT_SET_TIME, AT_TestSsidPara, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SSID",  (VOS_UINT8*)"(0-3),(@SSID)"},
/* Add by z60575 for multi_ssid, 2012-9-5 end */

    {AT_CMD_WIKEY,
    AT_SetWiFiKeyPara,    AT_NOT_SET_TIME, AT_QryWiFiKeyPara,    AT_NOT_SET_TIME, AT_TestWikeyPara, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIKEY",  (VOS_UINT8*)"(0-3),(@WIKEY)"},

    {AT_CMD_WILOG,
    AT_SetWiFiLogPara,    AT_NOT_SET_TIME, AT_QryWiFiLogPara,    AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WILOG",  (VOS_UINT8*)"(0,1),(@content)"},

    {AT_CMD_WIINFO,
    AT_SetWifiInfoPara,   AT_NOT_SET_TIME, VOS_NULL_PTR,         AT_NOT_SET_TIME, At_CmdTestProcERROR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIINFO",  (VOS_UINT8*)"(0,1),(0,1)"},

    {AT_CMD_WIPARANGE,
    AT_SetWifiPaRangePara, AT_NOT_SET_TIME, AT_QryWifiPaRangePara, AT_NOT_SET_TIME, AT_TestWifiPaRangePara, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^WIPARANGE",  (VOS_UINT8*)"(@gainmode)"},

    {AT_CMD_NVRD,
    AT_SetNVReadPara,     AT_SET_PARA_TIME,  VOS_NULL_PTR,        AT_NOT_SET_TIME,  VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^NVRD",(VOS_UINT8*)"(0-65535)"},


    {AT_CMD_NVWR,
    AT_SetNVWritePara,    AT_SET_PARA_TIME,  VOS_NULL_PTR,        AT_NOT_SET_TIME,  VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^NVWR",(VOS_UINT8*)"(0-65535),(0-2048),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data)"},
    /* Added by ?μó3?y/f62575 for AT Project, SIM?¨±￡?¤±ê??è·è?, 2011/11/15, begin */

    {AT_CMD_NVWRPART,
    AT_SetNVWRPartPara,       AT_SET_PARA_TIME,  VOS_NULL_PTR,        AT_NOT_SET_TIME,  VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^NVWRPART",(VOS_UINT8*)"(0-65535),(0-2048),(0-2048),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data),(@data)"},


    {AT_CMD_CURC,
    At_SetCurcPara,      AT_NOT_SET_TIME,     At_QryCurcPara,     AT_QRY_PARA_TIME, VOS_NULL_PTR,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^CURC", (VOS_UINT8*)"(0-2)"},
    /* Added by ?μó3?y/f62575 for AT Project, SIM?¨±￡?¤±ê??è·è?, 2011/11/15, end */


    {AT_CMD_SN,
    At_SetSnPara,        AT_NOT_SET_TIME,    VOS_NULL_PTR,           AT_NOT_SET_TIME,    At_CmdTestProcOK, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_DEVICE_OTHER_ERROR, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^SN",       VOS_NULL_PTR},


    /* Added by f62575 for SMALL IMAGE, 2012-1-3, begin */
    {AT_CMD_TBAT,
    AT_SetTbatPara,     AT_SET_PARA_TIME,   AT_QryTbatPara,  AT_QRY_PARA_TIME,  VOS_NULL_PTR, AT_SET_PARA_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^TBAT",    (VOS_UINT8 *)"(0,1),(0,1),(0-65535),(0-65535)"},

    {AT_CMD_PSTANDBY,
    AT_SetPstandbyPara,     AT_SET_PARA_TIME,   VOS_NULL_PTR,  AT_QRY_PARA_TIME,  VOS_NULL_PTR, AT_SET_PARA_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^PSTANDBY",    (VOS_UINT8 *)"(0-65535),(0-65535)"},

/* Add by z60575 for multi_ssid, 2012-9-5 begin */
    {AT_CMD_WIWEP,
    AT_SetWiwepPara,        AT_SET_PARA_TIME,   AT_QryWiwepPara,  AT_QRY_PARA_TIME,  AT_TestWiwepPara, AT_SET_PARA_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^WIWEP",    (VOS_UINT8 *)"(0-3),(@wifikey),(0-3)"},
/* Add by z60575 for multi_ssid, 2012-9-5 end */

    {AT_CMD_CMDLEN,
    AT_SetCmdlenPara,        AT_SET_PARA_TIME,   AT_QryCmdlenPara,  AT_QRY_PARA_TIME,  At_CmdTestProcOK, AT_SET_PARA_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^CMDLEN",    (VOS_UINT8 *)"(0-65535),(0-65535)"},

    {AT_CMD_TSELRF,
    AT_SetTseLrfPara,        AT_SET_PARA_TIME,   AT_QryTseLrfPara,  AT_QRY_PARA_TIME,  VOS_NULL_PTR, AT_SET_PARA_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^TSELRF",    (VOS_UINT8 *)"(0-255),(0-255)"},
    /* Added by f62575 for SMALL IMAGE, 2012-1-3, end   */

    {AT_CMD_HUK,
    AT_SetHukPara,              AT_SET_PARA_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^HUK",         (VOS_UINT8 *)"(@huk)"},

    {AT_CMD_FACAUTHPUBKEYEX,
    VOS_NULL_PTR,               AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FACAUTHPUBKEYEX",       (VOS_UINT8 *)"(1-20),(1-20),(@Pubkey)"},

    {AT_CMD_IDENTIFYSTART,
    AT_SetIdentifyStartPara,    AT_SET_PARA_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^IDENTIFYSTART",       (VOS_UINT8 *)"(@Rsa)"},

    {AT_CMD_IDENTIFYEND,
    AT_SetIdentifyEndPara,      AT_SET_PARA_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^IDENTIFYEND",       VOS_NULL_PTR},



    {AT_CMD_SIMLOCKDATAWRITE,
    VOS_NULL_PTR,               AT_NOT_SET_TIME,    VOS_NULL_PTR,   AT_NOT_SET_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SIMLOCKDATAWRITE",    (VOS_UINT8 *)"(@SimlockData)"},

    {AT_CMD_PHONESIMLOCKINFO,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,    AT_QryPhoneSimlockInfoPara, AT_QRY_PARA_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PHONESIMLOCKINFO",    VOS_NULL_PTR},

    {AT_CMD_SIMLOCKDATAREAD,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,    AT_QrySimlockDataReadPara,  AT_QRY_PARA_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SIMLOCKDATAREAD",     VOS_NULL_PTR},

    {AT_CMD_PHONEPHYNUM,
    AT_SetPhonePhynumPara,  AT_SET_PARA_TIME,    AT_QryPhonePhynumPara,  AT_QRY_PARA_TIME,   AT_TestHsicCmdPara,   AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PHONEPHYNUM",         (VOS_UINT8 *)"(@type),(@Phynum)"},

    {AT_CMD_PORTCTRLTMP,
    AT_SetPortCtrlTmpPara,          AT_SET_PARA_TIME,   AT_QryPortCtrlTmpPara,  AT_NOT_SET_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PORTCTRLTMP",    (VOS_UINT8 *)"(@password)"},

    {AT_CMD_PORTATTRIBSET,
    AT_SetPortAttribSetPara,          AT_SET_PARA_TIME,   AT_QryPortAttribSetPara,  AT_QRY_PARA_TIME,  AT_TestHsicCmdPara,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PORTATTRIBSET",    (VOS_UINT8 *)"(0,1),(@password)"},

    {AT_CMD_SIMLOCKUNLOCK,
    AT_SetSimlockUnlockPara, AT_SET_PARA_TIME,  VOS_NULL_PTR,   AT_NOT_SET_TIME,    AT_TestSimlockUnlockPara, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS,    CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SIMLOCKUNLOCK",    (VOS_UINT8 *)"(\"NET\",\"NETSUB\",\"SP\"),(pwd)"},

    {AT_CMD_FPLLSTATUS,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,    AT_QryFPllStatusPara,  AT_QRY_PARA_TIME,  VOS_NULL_PTR,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FPLLSTATUS",     (VOS_UINT8 *)"(0,1),(0,1)"},

    {AT_CMD_FPOWDET,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,    AT_QryFpowdetTPara,  AT_QRY_PARA_TIME,  VOS_NULL_PTR,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^FPOWDET",     VOS_NULL_PTR},

    {AT_CMD_NVWRSECCTRL,
    AT_SetNvwrSecCtrlPara,  AT_SET_PARA_TIME,   AT_QryNvwrSecCtrlPara,  AT_NOT_SET_TIME,   At_CmdTestProcOK,   AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^NVWRSECCTRL",   (VOS_UINT8*)"(0-2),(@SecString)"},

    {AT_CMD_MEID,
    AT_SetMeidPara,    AT_SET_PARA_TIME,    AT_QryMeidPara,    AT_SET_PARA_TIME,    VOS_NULL_PTR,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,    AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8 *)"^MEID",   (VOS_UINT8*)"(@number)"},

    {AT_CMD_DOSYSEVENT,
    AT_SetEvdoSysEvent,    AT_SET_PARA_TIME,    VOS_NULL_PTR,    AT_NOT_SET_TIME,    VOS_NULL_PTR,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,    AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8 *)"^DOSYSEVENT",   (VOS_UINT8*)"(0-4294967295)"},

    {AT_CMD_DOSIGMASK,
    AT_SetDoSigMask,    AT_SET_PARA_TIME,    VOS_NULL_PTR,    AT_NOT_SET_TIME,    VOS_NULL_PTR,    AT_NOT_SET_TIME,
    VOS_NULL_PTR,    AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8 *)"^DOSIGMASK",   (VOS_UINT8*)"(0-4294967295)"},



    {AT_CMD_MIPIWR,
    AT_SetMipiWrPara,  AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^MIPIWR",  (TAF_UINT8*)"(0-9),(0-15),(0-255),(0-65535),(0-3)"},/*mode, slave_id,  address, data,channel*/

   {AT_CMD_MIPIRD,
    AT_SetMipiRdPara,  AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^MIPIRD",  (TAF_UINT8*)"(0-9),(0-1),(0-15),(0-255)"},

   {AT_CMD_SSIWR,
    AT_SetSSIWrPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^SSIWR",  (TAF_UINT8*)"(0-9),(0-1),(0-255),(0-65535)"}, /*mode, channel, address, data*/

    {AT_CMD_SSIRD,
    AT_SetSSIRdPara, AT_SET_PARA_TIME, VOS_NULL_PTR, AT_QRY_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^SSIRD",  (TAF_UINT8*)"(0-9),(0-1),(0-255)"},/*mode, channel, address*/



    {AT_CMD_MIPIWREX,
    AT_SetMipiWrParaEx,  AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^MIPIWREX",  (TAF_UINT8*)"(0,1),(0-15),(0-15),(0-255),(1,2,4),(0-4294967295)"},/*extend_flag, mipi_id, slave_id, reg_addr, byte_cnt, value*/

   {AT_CMD_MIPIRDEX,
    AT_SetMipiRdParaEx,  AT_SET_PARA_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME, VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (TAF_UINT8*)"^MIPIRDEX",  (TAF_UINT8*)"(0,1),(0-15),(0-15),(0-255),(1,2,4)"},/*extend_flag, mipi_id, slave_id, reg_addr, byte_cnt*/

    {AT_CMD_SECURESTATE,
    AT_SetSecureStatePara,   AT_NOT_SET_TIME,   AT_QrySecureStatePara,   AT_NOT_SET_TIME,   VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SECURESTATE", (VOS_UINT8*)"(0-2)"},

    {AT_CMD_KCE,
    AT_SetKcePara,   AT_NOT_SET_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,   VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^KCE", (VOS_UINT8*)"(@KceString)"},

    {AT_CMD_SOCID,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,   AT_QrySocidPara,   AT_NOT_SET_TIME,   VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SOCID", (VOS_UINT8*)"(@SocidString)"},

    {AT_CMD_DIVERSITYSWITCH,
    AT_SetCdmaAttDiversitySwitch,   AT_SET_PARA_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,   VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^DIVERSITYSWITCH", (VOS_UINT8*)"(0,1)"},

    {AT_CMD_SETSLAVE,
    AT_SetSlavePara,  AT_SET_PARA_TIME,   VOS_NULL_PTR,  AT_NOT_SET_TIME,   VOS_NULL_PTR,   AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^SETSLAVE",   (VOS_UINT8*)"(0,1)"},

    {AT_CMD_RFICID,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,   AT_QryRficDieIDPara,   AT_QRY_PARA_TIME,   VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^RFICID", VOS_NULL_PTR},

    {AT_CMD_PMUDIESN,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,   AT_QryPmuDieSNPara,   AT_QRY_PARA_TIME,   VOS_NULL_PTR, AT_NOT_SET_TIME,
    VOS_NULL_PTR,   AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^PMUDIESN", VOS_NULL_PTR},

    {AT_CMD_TAS_TEST,
    AT_SetTasTestCfg,   AT_SET_PARA_TIME, VOS_NULL_PTR,  AT_NOT_SET_TIME,  VOS_NULL_PTR,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^TXTASTEST",    (VOS_UINT8*)"(0-4294967295),(0-4294967295),(0-4294967295),(0-4294967295),(0-4294967295)"},

    {AT_CMD_TAS_TEST_QUERY,
    AT_QryTasTestCfgPara,   AT_SET_PARA_TIME, VOS_NULL_PTR,  AT_NOT_SET_TIME,  VOS_NULL_PTR,  AT_NOT_SET_TIME,
    VOS_NULL_PTR,        AT_NOT_SET_TIME,
    AT_CME_INCORRECT_PARAMETERS, CMD_TBL_PIN_IS_LOCKED,
    (VOS_UINT8*)"^TXTASTESTQRY",    (VOS_UINT8*)"(0-4294967295)"}

};


/* ê?ày: ^CMDX ?üá?ê?2?êüE5?ü??±￡?¤?üá?￡??ò?ú+CLACáD?ù?ùóD?üá?ê±2???ê?￡?μúò???2?êyê?2?′???òyo?μ?×?·?′?,
        μú?t??2?êyê?′???òyo?μ?×?·?′?￡?μúèy??2?êyê???êyDí2?êy

   !!!!!!!!!!!×￠òa: param1oíparam2ê?ê?ày￡?êμ?ê?¨ò??üá?ê±ó|??á??¨ò?μ??ò?ì(?éìá???a??D§?ê)!!!!!!!!!!!!!

    {AT_CMD_CMDX,
    At_SetCmdxPara, AT_SET_PARA_TIME, At_QryCmdxPara, AT_QRY_PARA_TIME, At_TestCmdxPara, AT_NOT_SET_TIME,
    AT_ERROR, CMD_TBL_E5_IS_LOCKED | CMD_TBL_CLAC_IS_INVISIBLE,
    (VOS_UINT8*)"^CMDX", (VOS_UINT8*)"(@param1),(param2),(0-255)"},
*/


/*****************************************************************************
  3 oˉêyêμ??
*****************************************************************************/


VOS_UINT32 At_QrySecuBootFeaturePara( VOS_UINT8 ucIndex )
{
    VOS_UINT8                          usSecBootSupportedFlag;
    usSecBootSupportedFlag = 0;

    /* 调用底软接口 */
    if(MDRV_OK != mdrv_crypto_secboot_supported(&usSecBootSupportedFlag))
    {
        AT_WARN_LOG("At_QrySecuBootFeaturePara: get mdrv_crypto_secboot_supported() failed");
        return AT_ERROR;
    }


    /* 打印输出 */
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                            (VOS_CHAR *)pgucAtSndCodeAddr,
                                            (VOS_CHAR *)pgucAtSndCodeAddr,
                                            "%s: %d",
                                            g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                            usSecBootSupportedFlag);

    return AT_OK;
}


VOS_INT32 AT_SetSecDbgState(VOS_VOID)
{
    VOS_INT32               usResult;

    /*1.执行SECUREDEBUG=3,设置安全DEBUG授权由安全证书控制*/
    usResult = mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_SET_SECUREDEBUG,
                                AT_SECUREDEBUG_VALUE,
                                NULL,
                                0);

    /*设置接口只有三种返回值，<0,执行错误；=0，执行正确；=1，重复设置；
      所以这里只判断返回错误情况*/
    if(MDRV_OK > usResult)
    {
        AT_WARN_LOG("AT_SetSecuState: set SECUREDEBUG state error.\n");
        return AT_ERROR;
    }

    /*2.执行SECDBGRESET=1和CSRESET=1,在对安全世界和Coresight调试时，
        临时复位SecEngine*/
    usResult = mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_SET_CSRESET,
                                AT_CSRESET_VALUE,
                                NULL,
                                0);
    if(MDRV_OK > usResult)
    {
        AT_WARN_LOG("AT_SetSecuState: set CSRESET state error.\n");
        return AT_ERROR;
    }

    /*3.执行DFTSEL=1 可以通过密码验证方式开启安全DFT功能*/
    usResult = mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_SET_DFTSEL,
                                AT_DFTSEL_VALUE,
                                NULL,
                                0);
    if(MDRV_OK > usResult)
    {
        AT_WARN_LOG("AT_SetSecuState: set CSRESET state error.\n");
        return AT_ERROR;
    }

    return AT_OK;
}


VOS_UINT32 At_SetSecuBootPara(VOS_UINT8 ucIndex)
{
    if ((1 != gucAtParaIndex)
        || (0 == gastAtParaList[0].usParaLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (AT_DX_SECURE_STATE < gastAtParaList[0].ulParaValue)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    switch(gastAtParaList[0].ulParaValue)
    {
        case AT_NO_DX_SECU_ENABLE_STATE:
        {
            if (MDRV_OK != mdrv_crypto_start_secure())
            {
                AT_WARN_LOG("At_SetSecuBootPara: mdrv_crypto_start_secure() failed");
                return AT_ERROR;
            }
            break;
        }
        case AT_DX_RMA_STATE:
        {
            /*先获取下芯片状态，如果已经设置为RMA状态，直接返回ok*/
            if(AT_DRV_STATE_RMA == mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_GET_SECURESTATE,
                                                    0, NULL, 0))
            {
                AT_WARN_LOG("At_SetSecuBootPara: chip is already set to RMA state");
                return AT_OK;
            }

            if (MDRV_OK != mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_SET_SECURESTATE,
                                            AT_SET_RMA_STATE, NULL, 0))
            {
                AT_WARN_LOG("At_SetSecuBootPara: SET_SECURE_DISABLED_STATE  failed");
                return AT_ERROR;
            }
            break;
        }
        case AT_DX_SECURE_STATE:
        {
            /*安全状态已经在Fastboot里设置，这里check下，以保证产线流程OK*/
            if(AT_DRV_STATE_SECURE != mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_GET_SECURESTATE,
                                                       0, NULL, 0))
            {
                AT_WARN_LOG("At_SetSecuBootPara: chip don't set to SECURE state");
                return AT_ERROR;
            }
            if (AT_OK != AT_SetSecDbgState())
            {
                return AT_ERROR;
            }
            break;
        }
        default:
        {
            return AT_CME_INCORRECT_PARAMETERS;
        }
    }

    return AT_OK;
}


VOS_UINT32 At_QrySecuBootPara(VOS_UINT8 ucIndex)
{
    VOS_UINT8                           usSecBootStartedFlag = 0;

    if(MDRV_OK != mdrv_crypto_sec_started(&usSecBootStartedFlag))
    {
        AT_WARN_LOG("At_QrySecuBootPara: get mdrv_crypto_sec_started() failed");
        return AT_ERROR;
    }

/*对于支持DX安全引擎的，需要查询芯片状态*/

    /* 打印输出 */
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                            (VOS_CHAR *)pgucAtSndCodeAddr,
                                            (VOS_CHAR *)pgucAtSndCodeAddr,
                                            "%s: %d",
                                            g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                            usSecBootStartedFlag);

     return AT_OK;

}


VOS_UINT32 At_SetKeyPara(VOS_UINT8 ucIndex)
{
    VOS_INT32                           usRet;
    VOS_UINT8                           aulKeyBuf[AT_AUTHKEY_LEN];

    if ((1 != gucAtParaIndex)
     || (0 == gastAtParaList[0].usParaLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (AT_KEY_TYPE_BUTT <= gastAtParaList[0].ulParaValue)
    {
        AT_WARN_LOG("At_SetKeyPara: not support the para,pls check\n");
        return AT_CME_INCORRECT_PARAMETERS;
    }

    TAF_MEM_SET_S(aulKeyBuf, (VOS_SIZE_T)sizeof(aulKeyBuf), 0x00, (VOS_SIZE_T)sizeof(aulKeyBuf));

    switch(gastAtParaList[0].ulParaValue)
    {
        case AT_KEY_TYPE_DIEID:
        case AT_KEY_TYPE_TBOX_SMS:
        case AT_KEY_TYPE_SOCID:
            break;
        case AT_KEY_TYPE_AUTHKEY:
        {
            get_random_bytes(aulKeyBuf, AT_AUTHKEY_LEN);

            usRet = mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_SET_AUTHKEY,
                                     0,
                                     aulKeyBuf,
                                     AT_AUTHKEY_LEN);
            if(MDRV_OK != usRet)
            {
                AT_WARN_LOG("At_SetKeyPara: mdrv_set_authkey error.\n");
                return AT_ERROR;
            }
            break;
        }
        default:
        {
            AT_WARN_LOG("At_SetKeyPara: para is error,pls check.\n");
            return AT_CME_INCORRECT_PARAMETERS;
        }
    }
    return AT_OK;
}


VOS_UINT32 At_GetKeyInfoPara(VOS_UINT8 ucIndex)
{
    VOS_INT                 iResult;
    VOS_UINT16              usLength;
    VOS_UINT16              usKeyLen;
    VOS_UINT8               auckeybuf[AT_KEYBUFF_LEN];
    VOS_UINT8               usHashbuf[AT_KEY_HASH_LEN];
    VOS_UINT32              i;


    if (1 != gucAtParaIndex || (0 == gastAtParaList[0].usParaLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (AT_KEY_TYPE_BUTT <= gastAtParaList[0].ulParaValue)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 局部变量初始化 */
    TAF_MEM_SET_S(usHashbuf, (VOS_SIZE_T)sizeof(usHashbuf), 0x00, AT_KEY_HASH_LEN);
    TAF_MEM_SET_S(auckeybuf, (VOS_SIZE_T)sizeof(auckeybuf), 0x00, AT_KEYBUFF_LEN);

    switch(gastAtParaList[0].ulParaValue)
    {
        case AT_KEY_TYPE_DIEID:
        case AT_KEY_TYPE_TBOX_SMS:
        {
            /*打桩，暂不处理*/
            return AT_OK;
        }
        case AT_KEY_TYPE_SOCID:
        {
            usKeyLen = AT_SOCID_LEN;
            iResult = mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_GET_SOCID,
                                       0,
                                       auckeybuf,
                                       AT_SOCID_LEN);

            /* 处理异常查询结果 */
            if (MDRV_OK != iResult)
            {
                AT_WARN_LOG("At_QryGetKeyInfoPara:get soc id error.\n");
                return AT_ERROR;
            }
            break;
        }
        case AT_KEY_TYPE_AUTHKEY:
        {
            usKeyLen = AT_AUTHKEY_LEN;
            iResult = mdrv_efuse_ioctl(MDRV_EFUSE_IOCTL_CMD_GET_AUTHKEY,
                                       0,
                                       auckeybuf,
                                       AT_AUTHKEY_LEN);
            if(MDRV_OK != iResult)
            {
                AT_WARN_LOG("At_QryGetKeyInfoPara:get authkey error.\n");
                return AT_ERROR;
            }
            break;
        }
        default:
        {
            return AT_CME_INCORRECT_PARAMETERS;
        }

    }

    /*计算hash值,基线打桩，产品线适配*/
    /*
    iResult = mdrv_crypto_hash256(auckeybuf, usKeyLen, usHashbuf);
    if(MDRV_OK != iResult)
    {
        AT_WARN_LOG("At_QryGetKeyInfoPara:get hash error.\n");
        return AT_ERROR;
    } */

    /* 打印输出AT名称 */
    usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                     (VOS_CHAR *)pgucAtSndCodeAddr,
                                     (VOS_CHAR *)pgucAtSndCodeAddr,
                                      "%s:%s",
                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                     gaucAtCrLf);

    /*key info*/
    for(i = 0; i < usKeyLen; i++)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                         (VOS_CHAR *)pgucAtSndCodeAddr,
                                         (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                          "%02X",
                                          auckeybuf[i]);

    }
    /*key len*/
    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                         (VOS_CHAR *)pgucAtSndCodeAddr,
                                         (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                          ",%d,",
                                          usKeyLen * 2);
    /*key hash*/
    for(i = 0; i < AT_KEY_HASH_LEN; i++)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                         (VOS_CHAR *)pgucAtSndCodeAddr,
                                         (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                          "%02x",
                                          usHashbuf[i]);

    }
    gstAtSendData.usBufLen = usLength;
    return AT_OK;
}


VOS_UINT32 At_TestTmodePara(VOS_UINT8 ucIndex)
{
    VOS_UINT16                          usLength;

    usLength = 0;

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                  (VOS_CHAR *)pgucAtSndCodeAddr,
                                  (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                  "%s: %d",
                                  g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                  g_stAtDevCmdCtrl.ucCurrentTMode);

    gstAtSendData.usBufLen = usLength;

    return AT_OK;

}


VOS_UINT32 At_TestFdacPara(VOS_UINT8 ucIndex)
{
    VOS_UINT16                          usLength;

    usLength  = 0;

    if ((AT_RAT_MODE_WCDMA == g_stAtDevCmdCtrl.ucDeviceRatMode)
     || (AT_RAT_MODE_AWS == g_stAtDevCmdCtrl.ucDeviceRatMode))
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                          "%s: (0-2047)",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName);
    }
    else
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                          "%s: (0-1023)",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName);
    }
    gstAtSendData.usBufLen = usLength;

    return AT_OK;

}

/*****************************************************************************
 oˉ êy ??  : At_RegisterDeviceCmdTable
 1|?ü?èê?  : ×￠2á×°±??üá?±í
 ê?è?2?êy  : VOS_VOID
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2011?ê10??21è?
    ×÷    ??   : ?3á?/l60609
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 At_RegisterDeviceCmdTable(VOS_VOID)
{
    return AT_RegisterCmdTable(g_astAtDeviceCmdTbl, sizeof(g_astAtDeviceCmdTbl)/sizeof(g_astAtDeviceCmdTbl[0]));
}

/* Added by f62575 for AT Project, 2011-10-28, begin */

/*****************************************************************************
 oˉ êy ??  : AT_TestSsidPara
 1|?ü?èê?  : SSID2aê??üá?
 ê?è?2?êy  : VOS_UINT8 ucIndex
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2011?ê10??28è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy
  2.è?    ?ú   : 2012?ê9??17è?
    ×÷    ??   : z60575
    DT???úèY   : MULTI_SSIDDT??
*****************************************************************************/
VOS_UINT32 AT_TestSsidPara(VOS_UINT8 ucIndex)
{
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */
    if (BSP_MODULE_UNSUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_WIFI))
    {
        return AT_ERROR;
    }
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */

    /* Modified by z60575 for multi_ssid, 2012-9-5 begin */
    gstAtSendData.usBufLen = (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr,
                                                    "%s:%d",
                                                    g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    AT_WIFI_MAX_SSID_NUM);
    /* Modified by z60575 for multi_ssid, 2012-9-5 end */
    return AT_OK;
}

/*****************************************************************************
 oˉ êy ??  : AT_TestWikeyPara
 1|?ü?èê?  : WIKEY2aê??üá?
 ê?è?2?êy  : VOS_UINT8 ucIndex
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2011?ê10??28è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_TestWikeyPara(VOS_UINT8 ucIndex)
{
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */
    if (BSP_MODULE_UNSUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_WIFI) )
    {
        return AT_ERROR;
    }
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */

    gstAtSendData.usBufLen = (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr,
                                                    "%s:%d",
                                                    g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    AT_WIFI_KEY_NUM);
    return AT_OK;
}

/* Added by f62575 for AT Project, 2011-10-28, end */

/* Added by f62575 for SMALL IMAGE, 2012-1-3, begin   */
/*****************************************************************************
 oˉ êy ??  : AT_SetTbatPara
 1|?ü?èê?  : ^TBATéè???üá?
 ê?è?2?êy  : VOS_UINT8 ucIndex      ó??§?÷òy
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32             ATC·μ????
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??2è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy
 2.è?    ?ú   : 2012?ê03??03è?
   ×÷    ??   : s62952
   DT???úèY   : BalongV300R002 Buildó??ˉ????:é?3yFEATURE_CHARGEoê

*****************************************************************************/
VOS_UINT32 AT_SetTbatPara(VOS_UINT8 ucIndex)
{
    return atSetTBATPara(ucIndex);

    /*
    ?ù?Y2?êy2?í???DD??ê?2ù×÷:
    1.  ?§3?ó??§ê?è?AT^TBAT=1,0??è?μ?3?μ??1êy×??μ￡?
    μ÷ó?μ×èí/OM?ó?ú??è?μ?3?êy×??μ
    2.  ?§3?ó??§ê?è?AT^TBAT=1,1,<value1>,<value2>éè??μ?3?μ??1êy×??μ￡?ó?óúμ?3?D￡×?￡?
    D′D￡×?μ??1μ?NVID 90(en_NV_Item_BATTERY_ADC)￡?′?′|óDòé?ê′yè·è?￡?
    en_NV_Item_BATTERY_ADC?D????μ?ê???ê?á????μ￡???AT?üá???ò???2?êy￡?è?o?ó3é?
    3.4V μ??1??ó|μ?ADC?μ
    4.2V μ??1??ó|μ?ADC?μ
    */
}

/*****************************************************************************
 oˉ êy ??  : AT_QryTbatPara
 1|?ü?èê?  : ^TBAT2é?ˉ?üá?￡?ó?óú??è?μ?3?μ?°2×°·?ê?
 ê?è?2?êy  : VOS_UINT8 ucIndex      ó??§?÷òy
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32             ATC·μ????
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??2è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_QryTbatPara(VOS_UINT8 ucIndex)
{
    /*
    μ÷ó?μ×èí?ó?ú??è?μ?3?°2×°·?ê?:
    <mount type> μ?3?°2×°·?ê?
    0 ?Tμ?3?
    1 ?é?ü??μ?3?
    2 ?ú??ò?ì??ˉμ?3?
    */
    /*?üá?×′ì?ààDí?ì2é*/
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_DEVICE_OTHER_ERROR;
    }

    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */
    if (BSP_MODULE_UNSUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_CHARGE) )
    {
        return AT_ERROR;
    }
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */

    if (TAF_SUCCESS == AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                               gastAtClientTab[ucIndex].opId,
                                               DRV_AGENT_TBAT_QRY_REQ,
                                               VOS_NULL_PTR,
                                               0,
                                               I0_WUEPS_PID_DRV_AGENT))
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_TBAT_QRY;             /*éè??μ±?°2ù×÷?￡ê? */
        return AT_WAIT_ASYNC_RETURN;                                                              /* μè′yòì2?ê??t·μ?? */
    }
    else
    {
        return AT_ERROR;
    }
}


VOS_UINT32 AT_SetPstandbyPara(VOS_UINT8 ucIndex)
{
    DRV_AGENT_PSTANDBY_REQ_STRU         stPstandbyInfo;

    /* Added by c64416 for ^PSTANDBY low power proc, 2013-9-13, Begin */

    TAF_MMA_PHONE_MODE_PARA_STRU        stPhoneModePara;

    /* Added by c64416 for ^PSTANDBY low power proc, 2013-9-13, End */

    /* ^PSTANDBYéè???üá?óD?ò??óD2??2?êy: ??è?′y?ú×′ì?μ?ê±??3¤?èoíμ￥°???è?′y?ú×′ì?μ??D??ê±?? */
    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if ((0 == gastAtParaList[0].usParaLen)
     || (0 == gastAtParaList[1].usParaLen)
     || (2 != gucAtParaIndex))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    At_FormatResultData(ucIndex, AT_OK); /*Dèòa?è???′OK*/

    /*±￡?¤·μ??êy?Y·￠?ííê3é*/
    VOS_TaskDelay(10);

    /*
    μ÷ó?μ×èíoíOM?ó?úê1μ￥°???è?′y?ú×′ì?:
    ACPUé?íê3éμ?2ù×÷￡o
    1?￠??μ?WIFI
    2?￠LED??μ?
    3?￠USB PowerOff

    ·￠???￠μ?Co?￡???ê?CCPUé?íê3éμ?2ù×÷￡o
    1?￠í¨D??￡?é??μ?
    2?￠1??¨ê±?÷
    3?￠1??D??
    4?￠μ÷ó?μ×èí?ó?ú??è?é??ˉ
    */
    /* Modify by f62575 for V7′ú??í?2?, 2012-04-07, Begin   */
    stPstandbyInfo.ulStandbyTime = gastAtParaList[0].ulParaValue;
    stPstandbyInfo.ulSwitchTime   = gastAtParaList[1].ulParaValue;

    DRV_PWRCTRL_STANDBYSTATEACPU(stPstandbyInfo.ulStandbyTime, stPstandbyInfo.ulSwitchTime);

    /*·￠?í???￠μ?co?*/
    if (TAF_SUCCESS != AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                           gastAtClientTab[ucIndex].opId,
                           DRV_AGENT_PSTANDBY_SET_REQ,
                           &stPstandbyInfo,
                           sizeof(stPstandbyInfo),
                           I0_WUEPS_PID_DRV_AGENT))
    {
        AT_ERR_LOG("AT_SetPstandbyPara: AT_FillAndSndAppReqMsg fail.");
    }

    /* Added by c64416 for ^PSTANDBY low power proc, 2013-9-13, Begin */
    /* V7R22éó?1??ú??è?μí1|o?á÷3ìá÷3ì */

    stPhoneModePara.PhMode = TAF_PH_MODE_MINI;

    if (VOS_TRUE == TAF_MMA_PhoneModeSetReq(WUEPS_PID_AT, gastAtClientTab[ucIndex].usClientId, 0, &stPhoneModePara))
    {
        /* éè??μ±?°2ù×÷ààDí */
        gastAtClientTab[ucIndex].CmdCurrentOpt = (AT_CMD_CURRENT_OPT_ENUM)AT_CMD_PSTANDBY_SET;

        return AT_WAIT_ASYNC_RETURN;    /* ·μ???üá?′|àí1ò?e×′ì? */
    }
    /* Added by c64416 for ^PSTANDBY low power proc, 2013-9-13, End */

    return AT_SUCCESS;
}

VOS_UINT32 AT_SetExbandInfoPara(VOS_UINT8 ucIndex)
{
    LTE_COMM_NON_STANDARD_BAND_COMM_STRU stLpsNonstandBand = {0};
    VOS_UINT32 ulRet  = AT_OK;
    VOS_UINT32 ulNvId = 0;

    /* 1?￠AT?üá?ààDíê?·??yè· */
    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 2?￠2?êy??êyê?·?·?o?òa?ó */
    if(1 != gucAtParaIndex)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulNvId = gastAtParaList[0].ulParaValue -LTE_COMM_NONSTANDARD_BAND_BEGIN  + EN_NV_ID_BANDNon1_BAND_INFO  ;
    if((ulNvId < EN_NV_ID_BANDNon1_BAND_INFO )||(ulNvId > EN_NV_ID_BANDNon16_BAND_INFO))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulRet = NVM_Read(ulNvId, (VOS_VOID*) &stLpsNonstandBand, sizeof(LTE_COMM_NON_STANDARD_BAND_COMM_STRU));
     if(  NV_OK != ulRet)
     {
        (VOS_VOID)vos_printf("read non stand band nv fail,ulNvId = %d,ulRet = %d!\n",ulNvId,ulRet);
        return AT_ERROR;
     }

    gstAtSendData.usBufLen = (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr,
                                                    "%s:BANDNO:%d\r\n",
                                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                     stLpsNonstandBand.ucBandID);


    gstAtSendData.usBufLen += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr + gstAtSendData.usBufLen,
                                                     "%s:DUPLEX:%d\r\n",
                                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                     stLpsNonstandBand.enBandMode);

    gstAtSendData.usBufLen += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                (TAF_CHAR *)pgucAtSndCodeAddr,
                                                (TAF_CHAR *)pgucAtSndCodeAddr + gstAtSendData.usBufLen,
                                                   "%s:FREQ:%d,%d,%d,%d\r\n",
                                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    stLpsNonstandBand.stUlFreqInfo.usFLow,
                                                    stLpsNonstandBand.stUlFreqInfo.usFHigh,
                                                    stLpsNonstandBand.stDlFreqInfo.usFLow,
                                                    stLpsNonstandBand.stDlFreqInfo.usFHigh
                                                 );

    gstAtSendData.usBufLen += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                (TAF_CHAR *)pgucAtSndCodeAddr,
                                                (TAF_CHAR *)pgucAtSndCodeAddr + gstAtSendData.usBufLen,
                                                   "%s:CHANNUM:%d,%d\r\n",
                                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    stLpsNonstandBand.stUlFreqInfo.usRangOfNLow,
                                                    stLpsNonstandBand.stDlFreqInfo.usRangOfNLow);

     return AT_OK;
}

VOS_UINT32 AT_SetExbandTestInfoPara(VOS_UINT8 ucIndex)
{
    LTE_COMM_NON_STANDARD_BAND_COMM_STRU stLpsNonstandBand = {0};
    VOS_UINT32 ulRet  = AT_OK;
    VOS_UINT32 ulNvId = 0,ulArrayID=0 ,i=0;

    VOS_UINT32 BandWidthArray[BAND_WIDTH_NUMS]= {14,30,50,100,150,200};

    /* 1?￠AT?üá?ààDíê?·??yè· */
    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 2?￠2?êy??êyê?·?·?o?òa?ó */
    if(2 != gucAtParaIndex)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulNvId = gastAtParaList[0].ulParaValue -LTE_COMM_NONSTANDARD_BAND_BEGIN  + EN_NV_ID_BANDNon1_BAND_INFO  ;

    if((ulNvId  < EN_NV_ID_BANDNon1_BAND_INFO ) ||(ulNvId > EN_NV_ID_BANDNon16_BAND_INFO))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulRet = NVM_Read(ulNvId, (VOS_VOID*) &stLpsNonstandBand, sizeof(LTE_COMM_NON_STANDARD_BAND_COMM_STRU));

    if(  NV_OK != ulRet)
    {
        (VOS_VOID)vos_printf("read non stand band nv fail,ulRet = %d!\n",ulRet);
        return AT_ERROR;
    }

    ulArrayID = BandWidthArray[BAND_WIDTH_NUMS -1];

    for(i = 0 ; i < BAND_WIDTH_NUMS; i++)
    {
        if( BandWidthArray[i] == gastAtParaList[1].ulParaValue)
        {
            ulArrayID = i;
            break;
        }
    }

    gstAtSendData.usBufLen = 0 ;

    gstAtSendData.usBufLen += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr,
                                                    (TAF_CHAR *)pgucAtSndCodeAddr + gstAtSendData.usBufLen,
                                                     "%s:FREQS:%d,%d,%d\r\n",
                                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                     stLpsNonstandBand.stTestInfo[ulArrayID].usTestFreqs[0],
                                                     stLpsNonstandBand.stTestInfo[ulArrayID].usTestFreqs[1],
                                                     stLpsNonstandBand.stTestInfo[ulArrayID].usTestFreqs[2]);

    gstAtSendData.usBufLen += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                (TAF_CHAR *)pgucAtSndCodeAddr,
                                                (TAF_CHAR *)pgucAtSndCodeAddr + gstAtSendData.usBufLen,
                                                   "%s:MAXPOWTOLERANCE:%d,%d\r\n",
                                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    stLpsNonstandBand.stUePowerClassInfo.sPowerToleranceHigh,
                                                    stLpsNonstandBand.stUePowerClassInfo.sPowerToleranceLow
                                                 );

    gstAtSendData.usBufLen += (TAF_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                (TAF_CHAR *)pgucAtSndCodeAddr,
                                                (TAF_CHAR *)pgucAtSndCodeAddr + gstAtSendData.usBufLen,
                                                   "%s:REFSENSPOW:%d\r\n",
                                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                   stLpsNonstandBand.stTestInfo[ulArrayID].sRxRefSensPower);
      return AT_OK;
}

/* Modify by z60575 for multi_ssid, 2012-9-5 begin */

VOS_UINT32 AT_WriteWiWep(
    VOS_UINT32                          ulIndex,
    VOS_UINT8                           aucWiWep[],
    VOS_UINT16                          usWiWepLen,
    TAF_AT_MULTI_WIFI_SEC_STRU         *pstWifiSecInfo,
    VOS_UINT8                           ucGroup
)
{
    VOS_UINT32                          ulRet;
    VOS_UINT32                          ulLoop;
    VOS_UINT8                          *paucWifiWepKey;

    /* òò?a??DT??en_NV_Item_WIFI_KEY?Dμ?WIFI KEY×ó??￡??ùò?Dèòa?è??è?è?2?NV?μ￡?è?oó?üD?DT??2?·? */
    ulRet = NV_ReadEx(MODEM_ID_0, en_NV_Item_MULTI_WIFI_KEY, pstWifiSecInfo, sizeof(TAF_AT_MULTI_WIFI_SEC_STRU));
    if (NV_OK != ulRet)
    {
        AT_WARN_LOG("AT_WriteWiWep: Fail to read en_NV_Item_WIFI_KEY.");
        return AT_ERROR;
    }

    /* ?ù?Yindex??è?NV?D±￡′?μ??ü?? */
    paucWifiWepKey = ((0 == ulIndex) ? pstWifiSecInfo->aucWifiWepKey1[ucGroup] :
                      ((1 == ulIndex) ? pstWifiSecInfo->aucWifiWepKey2[ucGroup] :
                      ((2 == ulIndex) ? pstWifiSecInfo->aucWifiWepKey3[ucGroup] : pstWifiSecInfo->aucWifiWepKey4[ucGroup])));

    /* ?D??D?μ?WIFI KEYó?NV?D????μ?ê?·?ò??? */
    for (ulLoop = 0; ulLoop < AT_NV_WLKEY_LEN; ulLoop++)
    {
        if (paucWifiWepKey[ulLoop] != aucWiWep[ulLoop])
        {
            break;
        }
    }

    /* ?D??D?μ?WIFI KEYó?NV?D????μ?ò????ò?±?ó·μ??2ù×÷íê3é*/
    if (AT_NV_WLKEY_LEN == ulLoop)
    {
        return AT_OK;
    }

    /* ?üD?êy?Yμ?NV??en_NV_Item_WIFI_KEY */
    TAF_MEM_SET_S(paucWifiWepKey, AT_WIFI_KEY_LEN_MAX, 0x00, AT_NV_WLKEY_LEN);

    TAF_MEM_CPY_S(paucWifiWepKey, AT_WIFI_KEY_LEN_MAX, aucWiWep, usWiWepLen);

    ulRet = NV_WriteEx(MODEM_ID_0, en_NV_Item_MULTI_WIFI_KEY, pstWifiSecInfo, sizeof(TAF_AT_MULTI_WIFI_SEC_STRU));
    if (NV_OK != ulRet)
    {
         AT_WARN_LOG("AT_WriteWiWep: Fail to write NV en_NV_Item_WIFI_KEY.");
         return AT_ERROR;
    }

    return AT_OK;
}


VOS_UINT32 AT_SetWiwepPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulIndex;
    VOS_UINT8                           ucGroup;
    VOS_UINT32                          ulRet;
    TAF_AT_MULTI_WIFI_SEC_STRU         *pstWifiSecInfo;

    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */
    if (BSP_MODULE_UNSUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_WIFI))
    {
        return AT_ERROR;
    }
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */

    /* ê?è?2?êy?ì2é: óD?ò??óD< index >oí< content >á???2?êy */
    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    if ((0 == gastAtParaList[0].usParaLen)
     || (0 == gastAtParaList[1].usParaLen)
     || (0 == gastAtParaList[2].usParaLen))
    {
        return AT_ERROR;
    }

    /* WIFI key 2ù×÷êüDATALOCK±￡?¤ */
    if (VOS_TRUE == g_bAtDataLocked)
    {
        return AT_ERROR;
    }

    /* < index >±?D??ú0-3·??§?ú￡?< content >3¤?èD?óúNV_WLKEY_LEN */
    ulIndex = gastAtParaList[0].ulParaValue;
    if (ulIndex > AT_WIWEP_CARD_WIFI_KEY_TOTAL)
    {
        return AT_ERROR;
    }

    if (gastAtParaList[1].usParaLen > AT_NV_WLKEY_LEN)
    {
        return AT_ERROR;
    }

    ucGroup = (VOS_UINT8)gastAtParaList[2].ulParaValue;

    if (ucGroup >= AT_WIFI_MAX_SSID_NUM)
    {
        return AT_ERROR;
    }

    /* ×é×°WIFI KEYμ?NV?á112￠?üD?êy?Yμ?NV??en_NV_Item_WIFI_KEY */
    pstWifiSecInfo = (TAF_AT_MULTI_WIFI_SEC_STRU *)PS_MEM_ALLOC(WUEPS_PID_AT,
                                                  sizeof(TAF_AT_MULTI_WIFI_SEC_STRU));
    if (VOS_NULL_PTR == pstWifiSecInfo)
    {
        return AT_ERROR;
    }

    ulRet = AT_WriteWiWep(ulIndex,
                          gastAtParaList[1].aucPara,
                          gastAtParaList[1].usParaLen,
                          pstWifiSecInfo,
                          ucGroup);

    PS_MEM_FREE(WUEPS_PID_AT, pstWifiSecInfo);

    return ulRet;
}


VOS_UINT32 AT_QryWiwepPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulRet;
    VOS_UINT32                          ulLoop;
    VOS_UINT16                          usLength;
    TAF_AT_MULTI_WIFI_SEC_STRU         *pstWifiSecInfo;
    VOS_UINT8                           aucWifiWepKey[AT_NV_WLKEY_LEN + 1];
    VOS_UINT8                           aucWepKeyLen1[AT_WIFI_MAX_SSID_NUM];
    VOS_UINT8                           aucWepKeyLen2[AT_WIFI_MAX_SSID_NUM];
    VOS_UINT8                           aucWepKeyLen3[AT_WIFI_MAX_SSID_NUM];
    VOS_UINT8                           aucWepKeyLen4[AT_WIFI_MAX_SSID_NUM];
    VOS_UINT8                           ucWepKeyNum;

    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */
    if (BSP_MODULE_UNSUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_WIFI))
    {
        return AT_ERROR;
    }
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */

    /* ?a?áè?WIFI KEYéê???ú′?￡??áNV??en_NV_Item_WIFI_KEY??è?WIFI KEYD??￠ */
    pstWifiSecInfo = (TAF_AT_MULTI_WIFI_SEC_STRU *)PS_MEM_ALLOC(WUEPS_PID_AT,
                                                      sizeof(TAF_AT_MULTI_WIFI_SEC_STRU));
    if (VOS_NULL_PTR == pstWifiSecInfo)
    {
        return AT_ERROR;
    }

    ulRet = NV_ReadEx(MODEM_ID_0, en_NV_Item_MULTI_WIFI_KEY, pstWifiSecInfo, sizeof(TAF_AT_MULTI_WIFI_SEC_STRU));
    if (NV_OK != ulRet)
    {
        AT_WARN_LOG("AT_QryWiwepPara: Fail to read en_NV_Item_WIFI_KEY.");
        PS_MEM_FREE(WUEPS_PID_AT, pstWifiSecInfo);
        return AT_ERROR;
    }

    ucWepKeyNum = 0;
    TAF_MEM_SET_S(aucWepKeyLen1, sizeof(aucWepKeyLen1), 0x00, sizeof(aucWepKeyLen1));
    TAF_MEM_SET_S(aucWepKeyLen2, sizeof(aucWepKeyLen2), 0x00, sizeof(aucWepKeyLen2));
    TAF_MEM_SET_S(aucWepKeyLen3, sizeof(aucWepKeyLen3), 0x00, sizeof(aucWepKeyLen3));
    TAF_MEM_SET_S(aucWepKeyLen4, sizeof(aucWepKeyLen4), 0x00, sizeof(aucWepKeyLen4));

    for (ulLoop = 0; ulLoop < AT_WIFI_MAX_SSID_NUM; ulLoop++)
    {
        /* KEY1??ó|μ?NV2???±íê?KEY1óDD§ */
        aucWepKeyLen1[ulLoop] = (VOS_UINT8)VOS_StrLen((VOS_CHAR*)pstWifiSecInfo->aucWifiWepKey1[ulLoop]);
        if (0 != aucWepKeyLen1[ulLoop])
        {
            ucWepKeyNum++;
        }

        aucWepKeyLen2[ulLoop] = (VOS_UINT8)VOS_StrLen((VOS_CHAR*)pstWifiSecInfo->aucWifiWepKey2[ulLoop]);
        if (0 != aucWepKeyLen2[ulLoop])
        {
            ucWepKeyNum++;
        }

        aucWepKeyLen3[ulLoop] = (VOS_UINT8)VOS_StrLen((VOS_CHAR*)pstWifiSecInfo->aucWifiWepKey3[ulLoop]);
        if (0 != aucWepKeyLen3[ulLoop])
        {
            ucWepKeyNum++;
        }

        aucWepKeyLen4[ulLoop] = (VOS_UINT8)VOS_StrLen((VOS_CHAR*)pstWifiSecInfo->aucWifiWepKey4[ulLoop]);
        if (0 != aucWepKeyLen4[ulLoop])
        {
            ucWepKeyNum++;
        }
    }

    /* ?′?a??ê±￡?Dèòa·μ??ò??¨??0×é */
    if (VOS_TRUE == g_bAtDataLocked)
    {
        ucWepKeyNum = 0;
        TAF_MEM_SET_S(aucWepKeyLen1, sizeof(aucWepKeyLen1), 0x00, sizeof(aucWepKeyLen1));
        TAF_MEM_SET_S(aucWepKeyLen2, sizeof(aucWepKeyLen2), 0x00, sizeof(aucWepKeyLen2));
        TAF_MEM_SET_S(aucWepKeyLen3, sizeof(aucWepKeyLen3), 0x00, sizeof(aucWepKeyLen3));
        TAF_MEM_SET_S(aucWepKeyLen4, sizeof(aucWepKeyLen4), 0x00, sizeof(aucWepKeyLen4));
    }

    /*
    ?ú×?1¤?????óòa?ó￡?Dèòaê?3?1¤???§3?μ?è?2?êy20??WiFi WEP￡¨WIFIμ?KEY￡?
    ′òó???×éμ￥°??§3?μ?WIFI KEYD??￠
    ì?3?16DD^WIWEP: <index>,ó?1¤??òa?óμ?20??μ?WiFi WEP￡¨WIFIμ?KEY￡?ò???￡?
    */
    usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                     (VOS_CHAR *)pgucAtSndCodeAddr,
                                     (VOS_CHAR *)pgucAtSndCodeAddr,
                                     "%s:%d%s",
                                     g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                     ucWepKeyNum,
                                     gaucAtCrLf);

    for (ulLoop = 0; ulLoop < AT_WIFI_MAX_SSID_NUM; ulLoop++)
    {
        if (0 != aucWepKeyLen1[ulLoop])
        {
            /* wifikey1 */
            TAF_MEM_SET_S(aucWifiWepKey, sizeof(aucWifiWepKey), 0x00, (VOS_SIZE_T)(AT_NV_WLKEY_LEN + 1));

            TAF_MEM_CPY_S(aucWifiWepKey,
                       sizeof(aucWifiWepKey),
                       pstWifiSecInfo->aucWifiWepKey1[ulLoop],
                       AT_NV_WLKEY_LEN);

            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                              (VOS_CHAR *)pgucAtSndCodeAddr,
                                              (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                              "%s:%d,%s,%d%s",
                                              g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                              0,
                                              aucWifiWepKey,
                                              ulLoop,
                                              gaucAtCrLf);
        }

        if (0 != aucWepKeyLen2[ulLoop])
        {
            /* wifikey1 */
            TAF_MEM_SET_S(aucWifiWepKey, sizeof(aucWifiWepKey), 0x00, (VOS_SIZE_T)(AT_NV_WLKEY_LEN + 1));

            TAF_MEM_CPY_S(aucWifiWepKey,
                       sizeof(aucWifiWepKey),
                       pstWifiSecInfo->aucWifiWepKey2[ulLoop],
                       AT_NV_WLKEY_LEN);

            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                              (VOS_CHAR *)pgucAtSndCodeAddr,
                                              (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                              "%s:%d,%s,%d%s",
                                              g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                              1,
                                              aucWifiWepKey,
                                              ulLoop,
                                              gaucAtCrLf);
        }

        if (0 != aucWepKeyLen3[ulLoop])
        {
            /* wifikey1 */
            TAF_MEM_SET_S(aucWifiWepKey, sizeof(aucWifiWepKey), 0x00, (VOS_SIZE_T)(AT_NV_WLKEY_LEN + 1));

            TAF_MEM_CPY_S(aucWifiWepKey,
                       sizeof(aucWifiWepKey),
                       pstWifiSecInfo->aucWifiWepKey3[ulLoop],
                       AT_NV_WLKEY_LEN);

            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                              (VOS_CHAR *)pgucAtSndCodeAddr,
                                              (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                              "%s:%d,%s,%d%s",
                                              g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                              2,
                                              aucWifiWepKey,
                                              ulLoop,
                                              gaucAtCrLf);
        }

        if (0 != aucWepKeyLen4[ulLoop])
        {
            /* wifikey1 */
            TAF_MEM_SET_S(aucWifiWepKey, sizeof(aucWifiWepKey), 0x00, (VOS_SIZE_T)(AT_NV_WLKEY_LEN + 1));

            TAF_MEM_CPY_S(aucWifiWepKey,
                       sizeof(aucWifiWepKey),
                       pstWifiSecInfo->aucWifiWepKey4[ulLoop],
                       AT_NV_WLKEY_LEN);

            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                              (VOS_CHAR *)pgucAtSndCodeAddr,
                                              (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                              "%s:%d,%s,%d%s",
                                              g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                              3,
                                              aucWifiWepKey,
                                              ulLoop,
                                              gaucAtCrLf);
        }
    }

    gstAtSendData.usBufLen = usLength - (VOS_UINT16)VOS_StrLen((VOS_CHAR *)gaucAtCrLf);

    PS_MEM_FREE(WUEPS_PID_AT, pstWifiSecInfo);

    return AT_OK;
}
/* Modify by z60575 for multi_ssid, 2012-9-5 end */

/*****************************************************************************
 oˉ êy ??  : AT_TestWiwepPara
 1|?ü?èê?  : WIWEPμ?2aê??üá?￡?ê?3?1¤???§3?μ?WIFI KEY×üêy
 ê?è?2?êy  : VOS_UINT8 ucIndex      ó??§?÷òy
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32             ATC·μ????
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??3è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_TestWiwepPara(VOS_UINT8 ucIndex)
{
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */
    if (BSP_MODULE_UNSUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_WIFI) )
    {
        return AT_ERROR;
    }
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */

    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   "%s:%d",
                                                   g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                   AT_WIWEP_TOOLS_WIFI_KEY_TOTAL);

    return AT_OK;
}


VOS_UINT32 AT_TestWifiPaRangePara (VOS_UINT8 ucIndex)
{
    AT_WIFI_MODE_ENUM_UINT8             ucWifiMode;

    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */
    if (BSP_MODULE_UNSUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_WIFI) )
    {
        return AT_ERROR;
    }
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */

    /* 3?ê??ˉ */
    ucWifiMode                          = (VOS_UINT8)WIFI_GET_PA_MODE();

    /* 2é?ˉμ￥°??§3?μ??￡ê?￡oè?1????§3?PA?￡ê?￡???óDNO PA?￡ê?￡??ò??·μ??h?′?é￡?è?1?á????￡ê????§3?￡??ò·μ??h,l?￡*/
    if (AT_WIFI_MODE_ONLY_PA == ucWifiMode)
    {
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                (VOS_CHAR *)pgucAtSndCodeAddr,
                                                (VOS_CHAR *)pgucAtSndCodeAddr,
                                                "%s:%s",
                                                g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                "h");
    }
    else if (AT_WIFI_MODE_ONLY_NOPA == ucWifiMode)
    {
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                (VOS_CHAR *)pgucAtSndCodeAddr,
                                                (VOS_CHAR *)pgucAtSndCodeAddr,
                                                "%s:%s",
                                                g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                "l");
    }
    else if (AT_WIFI_MODE_PA_NOPA == ucWifiMode)
    {
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                (VOS_CHAR *)pgucAtSndCodeAddr,
                                                (VOS_CHAR *)pgucAtSndCodeAddr,
                                                "%s:%s",
                                                g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                "h,l");
    }
    else
    {
        return AT_ERROR;
    }

    return AT_OK;
}


VOS_VOID AT_GetTseLrfLoadDspInfo(
    AT_TSELRF_PATH_ENUM_UINT32          enPath,
    VOS_BOOL                           *pbLoadDsp,
    DRV_AGENT_TSELRF_SET_REQ_STRU      *pstTseLrf
)
{
    /* ^TSELRF?üá?éè??μ?é??μí¨?·±ào??aGSM?òμ±?°ò??-LOADá???í¨?·￡??TDèLOAD */
    if (AT_TSELRF_PATH_GSM == enPath)
    {
        if ((AT_RAT_MODE_GSM == g_stAtDevCmdCtrl.ucDeviceRatMode)
         && (VOS_TRUE == g_stAtDevCmdCtrl.bDspLoadFlag))
        {
            *pbLoadDsp = VOS_FALSE;
        }
        else
        {
            pstTseLrf->ucLoadDspMode     = VOS_RATMODE_GSM;
            pstTseLrf->ucDeviceRatMode   = AT_RAT_MODE_GSM;
            *pbLoadDsp                   = VOS_TRUE;
        }
        return;
    }

    /* ^TSELRF?üá?éè??μ?é??μí¨?·±ào??aWCDMA?÷?ˉ?òμ±?°ò??-LOADá???í¨?·￡??TDèLOAD */
    if (AT_TSELRF_PATH_WCDMA_PRI == enPath)
    {
        if (((AT_RAT_MODE_WCDMA == g_stAtDevCmdCtrl.ucDeviceRatMode)
          || (AT_RAT_MODE_AWS == g_stAtDevCmdCtrl.ucDeviceRatMode))
         && (VOS_TRUE == g_stAtDevCmdCtrl.bDspLoadFlag))
        {
            *pbLoadDsp = VOS_FALSE;
        }
        else
        {
            pstTseLrf->ucLoadDspMode     = VOS_RATMODE_WCDMA;
            pstTseLrf->ucDeviceRatMode   = AT_RAT_MODE_WCDMA;
            *pbLoadDsp                   = VOS_TRUE;
        }
        return;
    }

    *pbLoadDsp = VOS_FALSE;

    AT_WARN_LOG("AT_GetTseLrfLoadDspInfo: enPath only support GSM or WCDMA primary.");

    return;
}


VOS_UINT32 AT_SetTseLrfPara(VOS_UINT8 ucIndex)
{
    DRV_AGENT_TSELRF_SET_REQ_STRU       stTseLrf;
    VOS_BOOL                            bLoadDsp;

    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    if ((AT_TSELRF_PATH_WCDMA_PRI!=gastAtParaList[0].ulParaValue)
     && (AT_TSELRF_PATH_WCDMA_DIV!=gastAtParaList[0].ulParaValue)
     && (AT_TSELRF_PATH_GSM !=gastAtParaList[0].ulParaValue))
    {
        return atSetTselrfPara(ucIndex);
    }

    if(AT_TSELRF_PATH_TD == gastAtParaList[0].ulParaValue)
    {
        return atSetTselrfPara(ucIndex);
    }

    /* 2?êy2?·?o?òa?ó */
    if ((1 != gucAtParaIndex)
     || (0 == gastAtParaList[0].usParaLen))
    {
        return AT_ERROR;
    }

    if (AT_TSELRF_PATH_WIFI == gastAtParaList[0].ulParaValue)
    {
        /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */
        if ( BSP_MODULE_SUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_WIFI) )
        {
            /*WIFI?′Enable?±?ó·μ??ê§°ü*/
            if(VOS_FALSE == (VOS_UINT32)WIFI_GET_STATUS())
            {
                return AT_ERROR;
            }

            g_stAtDevCmdCtrl.ucDeviceRatMode = AT_RAT_MODE_WIFI;

            return AT_OK;
        }
        else
        {
            return AT_ERROR;
        }
        /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, end */
    }

    if (AT_TMODE_FTM != g_stAtDevCmdCtrl.ucCurrentTMode)
    {
        return AT_ERROR;
    }

    /* ′ò?a·??ˉ±?D??úFRXON??oó￡?2???RXDIVêμ?? */
    if (AT_TSELRF_PATH_WCDMA_DIV == gastAtParaList[0].ulParaValue)
    {
        if (DRV_AGENT_DSP_RF_SWITCH_ON != g_stAtDevCmdCtrl.ucRxOnOff)
        {
            g_stAtDevCmdCtrl.ucPriOrDiv = AT_RX_DIV_ON;
            return AT_OK;
        }
        if (AT_FAILURE == At_SendRfCfgAntSelToHPA(AT_RX_DIV_ON, ucIndex))
        {
            return AT_ERROR;
        }

        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_TSELRF_SET;
        g_stAtDevCmdCtrl.ucIndex               = ucIndex;

        /* ·μ???üá?′|àí1ò?e×′ì? */
        return AT_WAIT_ASYNC_RETURN;
    }

    /* Modify by f62575 for V7′ú??í?2?, 2012-04-07, Begin   */
    if ((AT_TSELRF_PATH_GSM != gastAtParaList[0].ulParaValue)
     && (AT_TSELRF_PATH_WCDMA_PRI != gastAtParaList[0].ulParaValue))
    {
        return AT_ERROR;
    }
    /* Modify by f62575 for V7′ú??í?2?, 2012-04-07, End   */

    if (AT_TSELRF_PATH_WCDMA_PRI == gastAtParaList[0].ulParaValue)
    {
        if (DRV_AGENT_DSP_RF_SWITCH_ON != g_stAtDevCmdCtrl.ucRxOnOff)
        {
            g_stAtDevCmdCtrl.ucPriOrDiv = AT_RX_PRI_ON;
            return AT_OK;
        }

        if (AT_FAILURE == At_SendRfCfgAntSelToHPA(AT_RX_PRI_ON, ucIndex))
        {
            return AT_ERROR;
        }

        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_TSELRF_SET;
        g_stAtDevCmdCtrl.ucIndex               = ucIndex;

        return AT_WAIT_ASYNC_RETURN;
    }

    /* ′?′|?D??ê?·?Dèòa??D??ó??DSP: Dèòa?ò·￠???óμ?Co??ó??DSP￡?·??ò￡??±?ó·μ??OK */
    AT_GetTseLrfLoadDspInfo(gastAtParaList[0].ulParaValue, &bLoadDsp, &stTseLrf);
    if (VOS_TRUE == bLoadDsp)
    {
        if (TAF_SUCCESS == AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                                   gastAtClientTab[ucIndex].opId,
                                                   DRV_AGENT_TSELRF_SET_REQ,
                                                   &stTseLrf,
                                                   sizeof(stTseLrf),
                                                   I0_WUEPS_PID_DRV_AGENT))
        {
            gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_TSELRF_SET;             /*éè??μ±?°2ù×÷?￡ê? */
            return AT_WAIT_ASYNC_RETURN;                                           /* μè′yòì2?ê??t·μ?? */
        }
        else
        {
            return AT_ERROR;
        }
    }

    return AT_OK;
}

/*****************************************************************************
 oˉ êy ??  : AT_QryTseLrfPara
 1|?ü?èê?  : 2é?ˉ?üá?ó?à′·μ??μ￥°??§3?μ??ùóD?é??￡??????é??·?DD??ê?￡?????<path>
             ààDí????ê?ò?DD￡?
             ?ùàyà′?μ￡?è??§3?2×éWIFIìì??￡??é??ê??a?±^TSELRF: 7,0,1?±
 ê?è?2?êy  : VOS_UINT8 ucIndex ó??§?÷òy
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32 ·μ???′DD?á1?3é1|?ò?àó|′í?ó??
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??10è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_QryTseLrfPara(VOS_UINT8 ucIndex)
{

    return atQryTselrfPara(ucIndex);

}

/*****************************************************************************
 oˉ êy ??  : AT_SetCmdlenPara
 1|?ü?èê?  : ???aμ￥°?PCò?′??úí?·￠?íμ?×?·???êyoíPC?úí?μ￥°?ò?′?·￠?íμ?×?·???êy
             ???üá?ó?óú?§3?SIMLOCKêy?Yμ?·???·￠?í￡?BALONG?T′?Dè?ó￡???·μ??OK±￡?¤1¤
             ??1éò??ˉá÷3ì?y3￡?′DD
 ê?è?2?êy  : VOS_UINT8 ucIndex      ó??§?÷òy
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32             ATC·μ????
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??2è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_SetCmdlenPara(VOS_UINT8 ucIndex)
{
    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if ((0 == gastAtParaList[0].usParaLen)
     || (0 == gastAtParaList[1].usParaLen)
     || (2 != gucAtParaIndex))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if (gastAtParaList[0].ulParaValue > (AT_COM_BUFF_LEN - sizeof("AT")))
    {
        return AT_ERROR;
    }

    return AT_OK;
}

/*****************************************************************************
 oˉ êy ??  : AT_QryCmdlenPara
 1|?ü?èê?  : 2é?ˉμ￥°?×?′ó?éò??±?ó?óê?AT?üá?×?·???êy￡?ò??°μ￥°?í¨1yATò?′?×?′ó
             ?éò??ìó|μ?×?·???êy￡???×?·???êy?ù2?°üo?AT?aá???×?·??￡
 ê?è?2?êy  : VOS_UINT8 ucIndex      ó??§?÷òy
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32             ATC·μ????
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??2è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_QryCmdlenPara(VOS_UINT8 ucIndex)
{
    /*
      ê?3?μ￥°?×?′ó?éò??±?ó?óê?AT?üá?×?·???êy(AT_CMD_MAX_LEN - sizeof("AT"))￡?
      ??×?·???êy?ù2?°üo?AT?aá???×?·?￡?ò??°μ￥°?í¨1yATò?′?×?′ó?éò??ìó|μ?×?·???êy￡?
      BALONG2ú?·?ìó|×?·?′?3¤?è?éò?·?
      ?à′?é?±¨￡???óD×?′ó?μ????￡?1¤???????μ?T′|àí￡?′?′|ê?3?ò?′?é?±¨μ?×?′ó?μAT_CMD_MAX_LEN?￡
    */
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s:%d,%d",
                                                    g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    (AT_COM_BUFF_LEN - VOS_StrLen("AT")),
                                                    AT_CMD_MAX_LEN);

    return AT_OK;
}

/*****************************************************************************
 oˉ êy ??  : AT_UpdateMacPara
 1|?ü?èê?  : ?üD?MACμ??·μ?NV
             ê?·??§3?2??MAC′yè·è?￡?
 ê?è?2?êy  : VOS_UINT8                           aucMac[]   MACμ??·×?·?′?ê×μ??·
             VOS_UINT16                          usMacLength    MACμ??·×?·?′?3¤?è
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32
             2ù×÷3é1|￡?·μ??AT_OK
             ??àío?3¤?è2?o?·¨·μ??AT_PHYNUM_LENGTH_ERR
             MT?ú2?′í?ó￡?·μ??AT_ERROR
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??3è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_UpdateMacPara(
    VOS_UINT8                           aucMac[],
    VOS_UINT16                          usMacLength
)
{
    VOS_UINT32                          ulRet;
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulWifiGlobalMacOffset;
    VOS_UINT32                          ulPhyNumMacOffset;
    VOS_UINT8                           aucWifiGlobalMac[AT_MAC_ADDR_LEN];

    /* MACμ??·3¤?è?ì2é: ±?D?12?? */
    if (AT_PHYNUM_MAC_LEN != usMacLength)
    {
        return AT_PHYNUM_LENGTH_ERR;
    }

    /* MACμ??·??ê??￥??: 7AFEE22111E4=>7A:FE:E2:21:11:E4*/
    ulWifiGlobalMacOffset = 0;
    ulPhyNumMacOffset     = 0;
    for (ulLoop = 0; ulLoop < (AT_PHYNUM_MAC_COLON_NUM + 1); ulLoop++)
    {
        TAF_MEM_CPY_S(&aucWifiGlobalMac[ulWifiGlobalMacOffset],
                   AT_MAC_ADDR_LEN - ulWifiGlobalMacOffset,
                   &aucMac[ulPhyNumMacOffset],
                   AT_WIFIGLOBAL_MAC_LEN_BETWEEN_COLONS);
        ulWifiGlobalMacOffset += AT_WIFIGLOBAL_MAC_LEN_BETWEEN_COLONS;
        ulPhyNumMacOffset     += AT_WIFIGLOBAL_MAC_LEN_BETWEEN_COLONS;
        aucWifiGlobalMac[ulWifiGlobalMacOffset] = ':';
        ulWifiGlobalMacOffset++;
    }

    aucWifiGlobalMac[AT_PHYNUM_MAC_LEN + AT_PHYNUM_MAC_COLON_NUM] = '\0';

    /* ?üD?MACμ??·μ?NV */
    ulRet = NV_WriteEx(MODEM_ID_0, en_NV_Item_WIFI_MAC_ADDR, aucWifiGlobalMac, AT_MAC_ADDR_LEN);
    if (NV_OK != ulRet)
    {
         AT_WARN_LOG("AT_UpdateMacPara: Fail to write NV.");
         return AT_ERROR;
    }

    return AT_OK;
}


VOS_UINT32 AT_SetTmodeAutoPowerOff(VOS_UINT8 ucIndex)
{
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, begin */

    TAF_MMA_PHONE_MODE_PARA_STRU        stPhModeSet;

    VOS_UINT8                             *pucSystemAppConfig;


    TAF_MEM_SET_S(&stPhModeSet, sizeof(stPhModeSet), 0x00, sizeof(TAF_MMA_PHONE_MODE_PARA_STRU));

    pucSystemAppConfig                    = AT_GetSystemAppConfigAddr();

    if ( SYSTEM_APP_WEBUI == *pucSystemAppConfig)
    {
        /* ·￠???￠??Co?í¨?a1??ú2￠??μ? */
        /* ?èí¨?aó??§AT?üá?ò??′DD￡?ó??§í¨1y?ì2a???úê?·???ê§à′è·è?è???ê?·??yè·?′DD
           ′??üá??′DDíê3éoóò???μ?￡??TDèμè′yòì2?·μ??
        */

        stPhModeSet.PhMode  = TAF_PH_MODE_POWEROFF;

        if (VOS_TRUE == TAF_MMA_PhoneModeSetReq(WUEPS_PID_AT, gastAtClientTab[ucIndex].usClientId, 0, &stPhModeSet))
        {
            return AT_OK;
        }
        else
        {
            return AT_ERROR;
        }

    }
    /* Modified by s62952 for BalongV300R002 Buildó??ˉ???? 2012-02-28, end */

    return AT_ERROR;

}
/* Added by f62575 for SMALL IMAGE, 2012-1-3, end   */


/*****************************************************************************
 oˉ êy ??  : AT_SDParamErrCode
 1|?ü?èê?  : SD?üá?2?êy′í?ó·μ??′í?ó??μ?′|àí
 ê?è?2?êy  : ?T
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??19è?
    ×÷    ??   : c64416
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_SDParamErrCode(VOS_VOID)
{
    if(0 == g_stATParseCmd.ucParaCheckIndex)
    {
        return AT_SD_CARD_OPRT_NOT_ALLOWED;
    }
    else if(1 == g_stATParseCmd.ucParaCheckIndex)
    {
        return AT_SD_CARD_ADDR_ERR;
    }
    else
    {
        return AT_SD_CARD_OTHER_ERR;
    }
}


/* Added by f62575 for B050 Project, 2012-2-3, Begin   */
/*****************************************************************************
 oˉ êy ??  : AT_GetSpecificPort
 1|?ü?èê?  : ??è????¨ààDíμ????ú
 ê?è?2?êy  : VOS_UINT8                           ucPortType ???úààDí
 ê?3?2?êy  : VOS_UINT32                         *pulPortPos ???ú?úNV???Dμ???ò?
             VOS_UINT32                         *pulPortNum NV???Dμ????ú×üêy
 ·μ ?? ?μ  : ?T
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??3è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_VOID AT_GetSpecificPort(
    VOS_UINT8                           ucPortType,
    VOS_UINT8                           aucRewindPortStyle[],
    VOS_UINT32                         *pulPortPos,
    VOS_UINT32                         *pulPortNum
)
{
    VOS_UINT32                          ulLoop;

    *pulPortPos = AT_DEV_NONE;
    for (ulLoop = 0; ulLoop < AT_SETPORT_PARA_MAX_LEN; ulLoop++)
    {
        if (0 == aucRewindPortStyle[ulLoop])
        {
            break;
        }

        /* ???ú?μ?aucPortType￡?±íê??òμ????¨???ú￡?????ulLoop?a???ú?úNV???Dμ???ò? */
        if (ucPortType == aucRewindPortStyle[ulLoop])
        {
            *pulPortPos = ulLoop;
        }
    }

    /* ???ú?μ?a0?òμ?′??-?·é??T￡?ulLoop?′?aμ￥°?μ????ú×üêy */
    *pulPortNum = ulLoop;

    return;
}


VOS_UINT32 AT_ExistSpecificPort(VOS_UINT8 ucPortType)
{
    AT_DYNAMIC_PID_TYPE_STRU            stDynamicPidType;
    VOS_UINT32                          ulPortPos;
    VOS_UINT32                          ulPortNum;


    TAF_MEM_SET_S(&stDynamicPidType, sizeof(stDynamicPidType), 0x00, sizeof(stDynamicPidType));

    ulPortPos = 0;
    ulPortNum = 0;


    /* ?áNV??en_NV_Item_Huawei_Dynamic_PID_Type??è?μ±?°μ????ú×′ì? */
    if (NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_Huawei_Dynamic_PID_Type,
                        &stDynamicPidType,
                        sizeof(AT_DYNAMIC_PID_TYPE_STRU)))
    {
        AT_WARN_LOG("AT_ExistSpecificPort: Read NV fail!");
        return VOS_FALSE;
    }

    /* ?D??DIAG???úê?·?ò??-′ò?a: ò??-′ò?a?ò?±?ó·μ??AT_OK */
    if (VOS_TRUE == stDynamicPidType.ulNvStatus)
    {
        /* 2é?ˉNV??en_NV_Item_Huawei_Dynamic_PID_Type?Dê?·?ò??-′??úDIAG?ú */
        AT_GetSpecificPort(ucPortType,
                           stDynamicPidType.aucRewindPortStyle,
                           &ulPortPos,
                           &ulPortNum);

        if (AT_DEV_NONE != ulPortPos)
        {
            return VOS_TRUE;
        }

    }

    return VOS_FALSE;
}



VOS_UINT32 AT_OpenDiagPort(VOS_VOID)
{
    AT_DYNAMIC_PID_TYPE_STRU            stDynamicPidType;
    VOS_UINT32                          ulPortPos;
    VOS_UINT32                          ulPortNum;


    TAF_MEM_SET_S(&stDynamicPidType, sizeof(stDynamicPidType), 0x00, sizeof(stDynamicPidType));

    ulPortPos = 0;
    ulPortNum = 0;


    /* ?áNV??en_NV_Item_Huawei_Dynamic_PID_Type??è?μ±?°μ????ú×′ì? */
    if (NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_Huawei_Dynamic_PID_Type,
                        &stDynamicPidType,
                        sizeof(AT_DYNAMIC_PID_TYPE_STRU)))
    {
        AT_WARN_LOG("AT_OpenDiagPort: Read NV fail!");
        return AT_ERROR;
    }

    /* ?D??DIAG???úê?·?ò??-′ò?a: ò??-′ò?a?ò?±?ó·μ??AT_OK */
    if (VOS_TRUE == stDynamicPidType.ulNvStatus)
    {
        /* 2é?ˉNV??en_NV_Item_Huawei_Dynamic_PID_Type?Dê?·?ò??-′??úDIAG?ú */
        AT_GetSpecificPort(AT_DEV_DIAG,
                           stDynamicPidType.aucRewindPortStyle,
                           &ulPortPos,
                           &ulPortNum);

        if (AT_DEV_NONE != ulPortPos)
        {
            return AT_OK;
        }

    }
    else
    {
        AT_WARN_LOG("AT_OpenDiagPort: en_NV_Item_Huawei_Dynamic_PID_Type is inactive!");
        return AT_OK;
    }

    /* DIAG???ú2ù×÷è¨?T?′??è?: ?±?ó·μ??AT_OK */
    if (AT_E5_RIGHT_FLAG_NO == g_enATE5RightFlag)
    {
        return AT_OK;
    }

    /* ×·?óDIAG???úμ??D??oó???ú?ˉ */
    if (AT_SETPORT_PARA_MAX_LEN == ulPortNum)
    {
        return AT_OK;
    }

    stDynamicPidType.aucRewindPortStyle[ulPortNum] = AT_DEV_DIAG;

    /* ?üD????ú?ˉo?êy?Yμ?NV??en_NV_Item_Huawei_Dynamic_PID_Type */
    if (NV_OK != NV_WriteEx(MODEM_ID_0, en_NV_Item_Huawei_Dynamic_PID_Type,
                        &stDynamicPidType,
                        sizeof(AT_DYNAMIC_PID_TYPE_STRU)))
    {
        AT_ERR_LOG("AT_OpenDiagPort: Write NV fail");
        return AT_ERROR;
    }
    else
    {
        return AT_OK;
    }
}


VOS_UINT32 AT_CloseDiagPort(VOS_VOID)
{
    AT_DYNAMIC_PID_TYPE_STRU            stDynamicPidType;
    VOS_UINT32                          ulPortPos;
    VOS_UINT32                          ulPortNum;
    VOS_UINT32                          ulLoop;


    TAF_MEM_SET_S(&stDynamicPidType, sizeof(stDynamicPidType), 0x00, sizeof(stDynamicPidType));

    ulPortPos = 0;
    ulPortNum = 0;


    /* ?áNV??en_NV_Item_Huawei_Dynamic_PID_Type??è?μ±?°μ????ú×′ì? */
    if (NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_Huawei_Dynamic_PID_Type,
                          &stDynamicPidType,
                          sizeof(AT_DYNAMIC_PID_TYPE_STRU)))
    {
        AT_ERR_LOG("AT_CloseDiagPort: Read NV fail!");
        return AT_ERROR;
    }

    /* ?D??DIAG???úê?·?ò??-′ò?a: ò??-′ò?a?ò?±?ó·μ??AT_OK */
    if (VOS_TRUE == stDynamicPidType.ulNvStatus)
    {
        /* 2é?ˉNV??en_NV_Item_Huawei_Dynamic_PID_Type?Dê?·?ò??-′??úDIAG?ú */
        AT_GetSpecificPort(AT_DEV_DIAG,
                           stDynamicPidType.aucRewindPortStyle,
                           &ulPortPos,
                           &ulPortNum);

        if (AT_DEV_NONE == ulPortPos)
        {
            return AT_OK;
        }
    }
    else
    {
        return AT_OK;
    }

    /* DIAG???ú2ù×÷è¨?T?′??è?: ?±?ó·μ??AT_OK */
    if (AT_E5_RIGHT_FLAG_NO == g_enATE5RightFlag)
    {
        return AT_OK;
    }

    /* é?3yNV???Dμ?DIAG???ú */
    stDynamicPidType.aucRewindPortStyle[ulPortPos] = 0;
    ulPortNum--;
    for (ulLoop = ulPortPos; ulLoop < ulPortNum; ulLoop++)
    {
        stDynamicPidType.aucRewindPortStyle[ulLoop] = stDynamicPidType.aucRewindPortStyle[ulLoop + 1UL];
    }
    stDynamicPidType.aucRewindPortStyle[ulPortNum] = 0;

    /* ???úòì3￡êy?Y±￡?¤: ?D??oóμ?éè±?D?ì??D￡?μúò???éè±?2??ü?aMASSéè±?(0xa1,0xa2) */
    if (0 != ulPortNum)
    {
        if ((AT_DEV_CDROM == stDynamicPidType.aucRewindPortStyle[0])
         || (AT_DEV_SD == stDynamicPidType.aucRewindPortStyle[0]))
        {
            return AT_OK;
        }
    }

    /* ?üD????ú?ˉo?êy?Yμ?NV??en_NV_Item_Huawei_Dynamic_PID_Type */
    if (NV_OK != NV_WriteEx(MODEM_ID_0, en_NV_Item_Huawei_Dynamic_PID_Type,
                          &stDynamicPidType,
                          sizeof(AT_DYNAMIC_PID_TYPE_STRU)))
    {
        AT_ERR_LOG("AT_CloseDiagPort: Write NV fail");
        return AT_ERROR;
    }
    else
    {
        return AT_OK;
    }
}


VOS_UINT32 AT_ExistSpecificPortChange(
    VOS_UINT8                           ucPortType,
    VOS_UINT8                           aucOldRewindPortStyle[],
    VOS_UINT8                           aucNewRewindPortStyle[]
)
{
    VOS_UINT32                          ulOldPortPos;
    VOS_UINT32                          ulNewPortPos;
    VOS_UINT32                          ulPortNum;


    ulOldPortPos = 0;
    ulNewPortPos = 0;
    ulPortNum    = 0;

    AT_GetSpecificPort(ucPortType, aucOldRewindPortStyle, &ulOldPortPos, &ulPortNum);
    AT_GetSpecificPort(ucPortType, aucNewRewindPortStyle, &ulNewPortPos, &ulPortNum);

    /* D???ò??????¨???ú */
    if ((AT_DEV_NONE == ulOldPortPos)
     && (AT_DEV_NONE != ulNewPortPos))
    {
        return VOS_TRUE;
    }

    /* é?3yò??????¨???ú */
    if ((AT_DEV_NONE != ulOldPortPos)
     && (AT_DEV_NONE == ulNewPortPos))
    {
        return VOS_TRUE;
    }

    return VOS_FALSE;

}

/*****************************************************************************
 oˉ êy ??  : AT_CheckSetPortRight
 1|?ü?èê?  : ê?·?′??ú???¨ààDíμ????ú±??ü
 ê?è?2?êy  : VOS_UINT8                           aucOldRewindPortStyle[]±??ü?°μ????ú?ˉo?
             VOS_UINT8                           aucNewRewindPortStyle[]±??üoóμ????ú?ˉo?
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32             AT_OK       è¨?T?ì2éí¨1y
                                    AT_ERROR    è¨?T?ì2é2?í¨1y
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê1??3è?
    ×÷    ??   : f62575
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_CheckSetPortRight(
    VOS_UINT8                           aucOldRewindPortStyle[],
    VOS_UINT8                           aucNewRewindPortStyle[]
)
{
    VOS_UINT32                          ulRet;

    ulRet = AT_ExistSpecificPortChange(AT_DEV_DIAG,
                                       aucOldRewindPortStyle,
                                       aucNewRewindPortStyle);

    if ((VOS_TRUE == ulRet)
     && (AT_E5_RIGHT_FLAG_NO == g_enATE5RightFlag))
    {
        return AT_ERROR;
    }

    return AT_OK;
}

/* Added by f62575 for B050 Project, 2012-2-3, end   */

/*****************************************************************************
 oˉ êy ??  : AT_SetHsspt
 1|?ü?èê?  : éè??RRC°?±?D??￠,oˉêyμ?2?êy?y3￡D??ì2a·??ú???￡?é×?.
 ê?è?2?êy  : ucIndex    - ó??§?÷òy
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : ·μ??3?′íD??￠?òOK
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê4??21è?
    ×÷    ??   : l60609
    DT???úèY   : D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_SetHsspt(VOS_UINT8 ucIndex)
{
    VOS_UINT8                           ucRRCVer;

    /* 2?êy?ì2é */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_DPAUPA_ERROR;
    }

    /* 2?êy1y?à */
    if (gucAtParaIndex != 1)
    {
        return AT_DPAUPA_ERROR;
    }

    /* ?ì2éê?·??a3yêy?Y±￡?¤,?′?a3yê±·μ??3?′íD??￠:ErrCode:0 */
    if (VOS_TRUE == g_bAtDataLocked)
    {
        return  AT_DATA_UNLOCK_ERROR;
    }

    ucRRCVer = (VOS_UINT8)gastAtParaList[0].ulParaValue;

    /* μ÷ó?D′NV?ó?úoˉêy: AT_WriteRrcVerToNV,·μ??2ù×÷?á1? */
    if (VOS_OK == AT_WriteRrcVerToNV(ucRRCVer))
    {
        return  AT_OK;
    }
    else
    {
        AT_WARN_LOG("At_SetHsspt:WARNING:Write NV failed!");
        return AT_DPAUPA_ERROR;
    }
}

/*****************************************************************************
 oˉ êy ??  : AT_QryHsspt
 1|?ü?èê?  : 2é?ˉRRC°?±?D??￠
 ê?è?2?êy  : ucIndex - ó??§?÷òy
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : ·μ??3?′íD??￠?òOK
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê4??21è?
    ×÷    ??   : l60609
    DT???úèY   : D?éú3éoˉêy
*****************************************************************************/
VOS_UINT32 AT_QryHsspt(VOS_UINT8 ucIndex)
{
    VOS_UINT8                           ucRRCVer;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usLength;

    /* 2?êy?ì2é */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_DPAUPA_ERROR;
    }

    ulResult = AT_ReadRrcVerFromNV(&ucRRCVer);

    if (VOS_OK == ulResult)
    {
        usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (TAF_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr, "%s:",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName);
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (TAF_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr + usLength, "%d",
                                           (VOS_UINT32)ucRRCVer);
        gstAtSendData.usBufLen = usLength;
        return  AT_OK;
    }
    else
    {
        AT_WARN_LOG("AT_QryHspaSpt:WARNING:WAS_MNTN_QueryHspaSpt failed!");
        return AT_DPAUPA_ERROR;
    }
}


VOS_UINT32 AT_TestHsicCmdPara(VOS_UINT8 ucIndex)
{
    /* í¨μà?ì2é */
    /* Modified by L60609 for MUX￡?2012-08-13,  Begin */
    if (VOS_FALSE == AT_IsApPort(ucIndex))
    /* Modified by L60609 for MUX￡?2012-08-13,  End */
    {
        return AT_ERROR;
    }

    return AT_OK;
}


VOS_UINT32 At_TestTdsScalibPara(VOS_UINT8 ucIndex)
{
    VOS_UINT16                           usLength;

    usLength = 0;
    usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR*)pgucAtSndCodeAddr,
                                        (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                        "%s:%s", g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                        AT_TDS_SCALIB_TEST_STR);
    gstAtSendData.usBufLen = usLength;
    return AT_OK;
}

/*****************************************************************************
 oˉ êy ??  : AT_TestSimlockUnlockPara
 1|?ü?èê?  : ^SIMLOCKUNLOCK2aê??üá?′|àíoˉêy
 ê?è?2?êy  : VOS_UINT8 ucIndex
 ê?3?2?êy  : ?T
 ·μ ?? ?μ  : VOS_UINT32
 μ÷ó?oˉêy  :
 ±?μ÷oˉêy  :

 DT??àúê·      :
  1.è?    ?ú   : 2012?ê9??19è?
    ×÷    ??   : à?×??￡/00198894
    DT???úèY   : STK213?ì?D??°DCMDè?ó?a·￠????D?éú3éoˉêy

*****************************************************************************/
VOS_UINT32 AT_TestSimlockUnlockPara( VOS_UINT8 ucIndex )
{
    /* í¨μà?ì2é */
    if (VOS_FALSE == AT_IsApPort(ucIndex))
    {
        return AT_ERROR;
    }

    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s: (\"NET\",\"NETSUB\",\"SP\")",
                                                    g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

    return AT_OK;
}


VOS_UINT32 AT_String2Hex( VOS_UINT8 *nptr,VOS_UINT16 usLen, VOS_UINT32 *pRtn)
{
    VOS_UINT32                          c     = 0;         /* current Char */
    VOS_UINT32                          total = 0;         /* current total */
    VOS_UINT8                           Length = 0;        /* current Length */

    c = (VOS_UINT32)*nptr++;

    while(Length++ < usLen)
    {
        if( (c  >= '0') && (c  <= '9') )
        {
            c  = c  - '0';
        }
        else if( (c  >= 'a') && (c  <= 'f') )
        {
            c  = c  - 'a' + 10;
        }
        else if( (c  >= 'A') && (c  <= 'F') )
        {
            c  = c  - 'A' + 10;
        }
        else
        {
            return VOS_ERR;
        }

        if(total > 0x0FFFFFFF)              /* ·￠éú·′×a */
        {
            return VOS_ERR;
        }
        else
        {
            total = (total << 4) + c;       /* accumulate digit */
            c = (VOS_UINT32)(*(nptr++));    /* get next Char */
        }
    }
     /* return result, negated if necessary */
    *pRtn = total;
    return AT_SUCCESS;
}


VOS_UINT32 AT_NVWRGetItemValue( VOS_UINT8 *pucPara,  VOS_UINT8 *pucValue,  VOS_UINT8 **pucParaDst)
{
    VOS_UINT32                          ulTmp = 0;
    VOS_UINT8                          *pucStart = pucPara;
    VOS_UINT16                          usLen = 0;
    VOS_UINT8                          *pucEnd = VOS_NULL_PTR;
    VOS_UINT32                          ulRet;

    /* μ÷ó?μ?μ?·?±￡?¤pucPara,pucValue,pucParaDst2??aNULL */

    /* ?±μ?μúò???2?ê?' 'μ?×?·? */
    while(' ' == *pucStart)
    {
        pucStart++;
    }

    if(' ' == *(pucStart+1))
    {
        usLen = 1;
        pucEnd = pucStart+2;
    }
    else if(' ' == *(pucStart+2))
    {
        usLen = 2;
        pucEnd = pucStart+3;
    }
    else
    {
        return VOS_ERR;
    }

    ulRet = AT_String2Hex(pucStart, usLen, &ulTmp);
    if((VOS_OK != ulRet) || (ulTmp > 0xff))
    {
        return VOS_ERR;
    }

    *pucValue = (VOS_UINT8)ulTmp;
    *pucParaDst = pucEnd;

    return VOS_OK;
}


VOS_UINT32 AT_NVWRGetParaInfo( AT_PARSE_PARA_TYPE_STRU * pstPara, VOS_UINT8 * pu8Data, VOS_UINT32 * pulLen)
{
    VOS_UINT32                          ulNum = 0;
    VOS_UINT8                          *pu8Start   = VOS_NULL_PTR;
    VOS_UINT8                          *pu8ParaTmp = VOS_NULL_PTR;
    VOS_UINT16                          usLen = 0;
    VOS_UINT32                          ulRet;
    VOS_UINT16                          i = 0;

    /* μ÷ó?μ?μ?·?±￡?¤pstPara,pu8Data2??aNULL */

    pu8Start = pstPara->aucPara;
    usLen= pstPara->usParaLen;

    if(usLen < AT_PARA_MAX_LEN)
    {
        pstPara->aucPara[usLen] = ' ';
    }
    else
    {
        return VOS_ERR;
    }

    ulNum = 0;
    for(i = 0; i < usLen; )
    {
        ulRet = AT_NVWRGetItemValue(pu8Start, (pu8Data+ulNum), &pu8ParaTmp);
        if(ERR_MSP_SUCCESS != ulRet)
        {
            return ulRet;
        }

        ulNum++;

        /* ×??à128?? */
        /*MAX_NV_NUM_PER_PARA */
        if(ulNum == 128)
        {
            break;
        }

        if(pu8ParaTmp >= (pu8Start+usLen))
        {
            break;
        }

        i += (VOS_UINT16)(pu8ParaTmp - pu8Start);
        pu8Start = pu8ParaTmp;
    }

    *pulLen = ulNum;
    return VOS_OK;
}


VOS_UINT32 AT_SetNVReadPara(VOS_UINT8 ucIndex)
{
    VOS_UINT16                          usNvId  = 0;
    VOS_UINT32                          ulNvLen = 0;
    VOS_UINT8                          *pucData = VOS_NULL_PTR;
    VOS_UINT32                          i       = 0;
    MODEM_ID_ENUM_UINT16                enModemId = MODEM_ID_0;
    VOS_UINT32                          ulRet;

    /* 2?êy?ì2é */
    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        g_ulNVRD = 1;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 2?êy1y?à */
    if(gucAtParaIndex > 1)
    {
        g_ulNVRD = 2;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 2?êy?a?? */
    if(0 == gastAtParaList[0].usParaLen)
    {
        g_ulNVRD = 3;
        return AT_CME_INCORRECT_PARAMETERS;
    }
    else
    {
        usNvId = (VOS_UINT16)gastAtParaList[0].ulParaValue;
    }

    ulRet = NV_GetLength(usNvId, &ulNvLen);

    if(VOS_OK != ulRet)
    {
        g_ulNVRD = 4;
        return AT_ERROR;
    }

    /* 3¤?è′óóú128￡???è??°128??×??ú*/
    pucData = (VOS_UINT8*)PS_MEM_ALLOC(WUEPS_PID_AT, ulNvLen);
    if(VOS_NULL_PTR == pucData)
    {
        g_ulNVRD = 5;
        return AT_ERROR;
    }

    ulRet = AT_GetModemIdFromClient(ucIndex, &enModemId);

    if (VOS_OK != ulRet)
    {
        AT_ERR_LOG("AT_SetNVReadPara:Get modem id fail");

        PS_MEM_FREE(WUEPS_PID_AT, pucData);

        g_ulNVWR =8;
        return AT_ERROR;
    }

    ulRet = NV_ReadEx(enModemId, usNvId, (VOS_VOID*)pucData, ulNvLen);

    if(VOS_OK != ulRet)
    {
        PS_MEM_FREE(WUEPS_PID_AT, pucData);

        g_ulNVRD = 6;
        return AT_ERROR;
    }
    gstAtSendData.usBufLen = 0;
    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN, (VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR*)pgucAtSndCodeAddr,
        "^NVRD: %d,", ulNvLen);

    for(i = 0; i < ulNvLen; i++)
    {
        if(0 == i)
        {
            gstAtSendData.usBufLen += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                (VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR*)pgucAtSndCodeAddr + gstAtSendData.usBufLen,"%02X", pucData[i]);
        }
        else
        {
            gstAtSendData.usBufLen += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                (VOS_CHAR *)pgucAtSndCodeAddr,(VOS_CHAR*)pgucAtSndCodeAddr + gstAtSendData.usBufLen," %02X", pucData[i]);
        }
    }

    PS_MEM_FREE(WUEPS_PID_AT, pucData);
    g_ulNVRD = 7;
    return AT_OK;
}


VOS_UINT32 AT_SetNVWritePara(VOS_UINT8 ucIndex)
{
    VOS_UINT16                          usNvId = 0;
    VOS_UINT16                          usNvTotleLen = 0;
    VOS_UINT32                          ulNvLen = 0; /* VOS_UINT16 -> VOS_UINT32 */
    VOS_UINT8                          *pucData = VOS_NULL_PTR;
    VOS_UINT32                          ulNvNum = 0; /* VOS_UINT16 -> VOS_UINT32 */
    VOS_UINT8                           au8Data[128] = {0};/* MAX_NV_NUM_PER_PARA */
    MODEM_ID_ENUM_UINT16                enModemId = MODEM_ID_0;
    VOS_UINT32                          i = 0;
    VOS_UINT32                          ulRet;

    gstAtSendData.usBufLen = 0;

    /* 2?êy?ì2é */
    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        g_ulNVWR =1;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 2?êy?a?? */
    if((0 == gastAtParaList[0].usParaLen)
        || (0 == gastAtParaList[1].usParaLen)
        || (0 == gastAtParaList[2].usParaLen))
    {
        g_ulNVWR =2;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    usNvId = (VOS_UINT16)gastAtParaList[0].ulParaValue;

    if (VOS_TRUE != AT_IsNVWRAllowedNvId(usNvId))
    {
        g_ulNVWR = 10;
        return AT_CME_OPERATION_NOT_ALLOWED;
    }

    (VOS_VOID)vos_printf("\n atSetNVWRPara usNvId = %d\n",usNvId);

    usNvTotleLen = (VOS_UINT16)gastAtParaList[1].ulParaValue;
    (VOS_VOID)vos_printf("\n atSetNVWRPara usNvTotleLen = %d\n",usNvTotleLen);

    pucData = (VOS_UINT8*)PS_MEM_ALLOC(WUEPS_PID_AT, usNvTotleLen);
    if(VOS_NULL_PTR == pucData)
    {
        g_ulNVWR =3;
        return AT_ERROR;
    }

    i = 0;
    while(0 != gastAtParaList[2UL + i].usParaLen)
    {
        ulRet = AT_NVWRGetParaInfo((AT_PARSE_PARA_TYPE_STRU*)(&(gastAtParaList[2UL + i])), au8Data, &ulNvNum);

        if(VOS_OK != ulRet)
        {
            PS_MEM_FREE(WUEPS_PID_AT, pucData);
            g_ulNVWR =4;
            return AT_CME_INCORRECT_PARAMETERS;
        }

        /* è?1?2?êyμ?3¤?è′óóú128￡??ò·μ??ê§°ü */
        /*MAX_NV_NUM_PER_PARA */
        if(ulNvNum > 128)
        {
            PS_MEM_FREE(WUEPS_PID_AT, pucData);
            g_ulNVWR =5;
            return AT_CME_INCORRECT_PARAMETERS;
        }

        /* è?1?à??óμ?2?êy??êy′óóú×ü3¤?è */
        if((ulNvLen+ulNvNum) > usNvTotleLen)
        {
            PS_MEM_FREE(WUEPS_PID_AT, pucData);
            g_ulNVWR =6;
            return AT_CME_INCORRECT_PARAMETERS;
        }

        TAF_MEM_CPY_S((pucData + ulNvLen), usNvTotleLen - ulNvLen, au8Data, ulNvNum);

        ulNvLen += ulNvNum;
        i++;

        if(i >= (AT_MAX_PARA_NUMBER-2))
        {
            break;
        }
    }

    /* è?1?à??óμ?2?êy??êyó?×ü3¤?è2??àμè */
    if(ulNvLen != usNvTotleLen)
    {
        PS_MEM_FREE(WUEPS_PID_AT, pucData);
        g_ulNVWR =7;
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulRet = AT_GetModemIdFromClient(ucIndex, &enModemId);

    if (VOS_OK != ulRet)
    {
        AT_ERR_LOG("AT_SetNVWritePara:Get modem id fail");
        PS_MEM_FREE(WUEPS_PID_AT, pucData);
        g_ulNVWR =8;
        return AT_ERROR;
    }

    ulRet = NV_WriteEx(enModemId, usNvId, (VOS_VOID*)pucData, usNvTotleLen);

    if(VOS_OK != ulRet)
    {
        PS_MEM_FREE(WUEPS_PID_AT, pucData);
        g_ulNVWR =9;

        return AT_ERROR;
    }
    ulRet = mdrv_nv_flush();
    if(MDRV_OK != ulRet)
    {
        PS_MEM_FREE(WUEPS_PID_AT, pucData);
        g_ulNVWR =11;

        return AT_ERROR;
    }
    PS_MEM_FREE(WUEPS_PID_AT, pucData);
    g_ulNVWR =10;

    return AT_OK;
}



VOS_UINT32 AT_SetNVWRPartPara(VOS_UINT8 ucClientId)
{
    VOS_UINT32                          ulRet;
    VOS_UINT16                          usNVID;
    VOS_UINT32                          ulNVWrTotleLen;
    VOS_UINT32                          ulNVLen;
    VOS_UINT16                          usNVWrLen;
    VOS_UINT8                          *pucNvWrData = VOS_NULL_PTR;
    VOS_UINT32                          ulNVNum;
    VOS_UINT32                          i;
    VOS_UINT8                           aucData[128];
    VOS_UINT16                          usOffset;
    MODEM_ID_ENUM_UINT16                enModemId;

    i                       = 0;
    ulNVLen                 = 0;
    gstAtSendData.usBufLen  = 0;
    ulNVWrTotleLen          = 0;
    TAF_MEM_SET_S(aucData, sizeof(aucData), 0x00, sizeof(aucData));

    if(AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    if( (0 == gastAtParaList[0].usParaLen)
     || (0 == gastAtParaList[1].usParaLen)
     || (0 == gastAtParaList[2].usParaLen)
     || (0 == gastAtParaList[3].usParaLen) )
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    usNVID       = (VOS_UINT16)gastAtParaList[0].ulParaValue;
    usOffset     = (VOS_UINT16)gastAtParaList[1].ulParaValue;
    usNVWrLen    = (VOS_UINT16)gastAtParaList[2].ulParaValue;

    if (VOS_TRUE != AT_IsNVWRAllowedNvId(usNVID))
    {
        return AT_CME_OPERATION_NOT_ALLOWED;
    }

    /* check param#3 usNVWrLen */
    if (usNVWrLen == 0)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* get NV length */
    ulRet        = NV_GetLength(usNVID, &ulNVWrTotleLen);
    if(ERR_MSP_SUCCESS != ulRet)
    {
        return AT_ERROR;
    }

    /* check param#2 usOffset */
    if(((VOS_UINT32)usOffset > (ulNVWrTotleLen - 1)) || ((VOS_UINT32)(usOffset + usNVWrLen) > ulNVWrTotleLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    pucNvWrData = (VOS_UINT8*)PS_MEM_ALLOC(WUEPS_PID_AT, usNVWrLen);
    if(NULL == pucNvWrData)
    {
        return AT_ERROR;
    }

    /* get NV value form param#4\5\...\#16 */
    while(0 != gastAtParaList[3UL + i].usParaLen)
    {
        ulRet = AT_NVWRGetParaInfo((AT_PARSE_PARA_TYPE_STRU*)(&(gastAtParaList[3UL + i])), aucData, (VOS_UINT32 *)&ulNVNum);
        if(ERR_MSP_SUCCESS != ulRet)
        {
            PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);
            return AT_CME_INCORRECT_PARAMETERS;
        }

        if(ulNVNum > 128)
        {
            PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);
            return AT_CME_INCORRECT_PARAMETERS;
        }

        if((ulNVLen+ulNVNum) > usNVWrLen)
        {
            PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);
            return AT_CME_INCORRECT_PARAMETERS;
        }

        TAF_MEM_CPY_S(((VOS_UINT8*)pucNvWrData + ulNVLen), usNVWrLen - ulNVLen, (VOS_UINT8*)aucData, ulNVNum);

        ulNVLen += ulNVNum;
        i++;

        if(i >= (AT_MAX_PARA_NUMBER - 3))
        {
            break;
        }

    }

    /* check all the nv value have the exact length by param#3 */
    if(ulNVLen != usNVWrLen)
    {
        PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulRet = AT_GetModemIdFromClient(ucClientId, &enModemId);
    if (VOS_OK != ulRet)
    {
        AT_ERR_LOG("AT_SetNVWritePara:Get modem id fail");
        PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);
        return AT_ERROR;
    }

    if (enModemId >= MODEM_ID_BUTT)
    {
        PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);
        return AT_ERROR;
    }

    ulRet = NV_WritePartEx(enModemId, usNVID, usOffset, (VOS_VOID*)pucNvWrData, usNVWrLen);
    if(ERR_MSP_SUCCESS != ulRet)
    {
        PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);
        return AT_ERROR;
    }
    ulRet = mdrv_nv_flush();
    if(MDRV_OK != ulRet)
    {
        PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);
        return AT_ERROR;
    }

    PS_MEM_FREE(WUEPS_PID_AT, pucNvWrData);

    return AT_OK;
}



VOS_VOID AT_GetNvRdDebug(VOS_VOID)
{
    (VOS_VOID)vos_printf("\n g_ulNVRD=0x%x \n",g_ulNVRD);
    (VOS_VOID)vos_printf("\n g_ulNVWR=0x%x \n",g_ulNVWR);
}


VOS_UINT32 AT_QryFPllStatusPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulReceiverPid;
    AT_PHY_RF_PLL_STATUS_REQ_STRU      *pstMsg;
    VOS_UINT32                          ulLength;
    VOS_UINT16                          usMsgId;
    if ((AT_RAT_MODE_FDD_LTE == g_stAtDevCmdCtrl.ucDeviceRatMode)
            ||(AT_RAT_MODE_TDD_LTE == g_stAtDevCmdCtrl.ucDeviceRatMode))
    {
        return atQryFPllStatusPara(ucIndex);
    }
    /*?D??μ±?°?óè??￡ê?￡????§3?G/W*/
    if (AT_RAT_MODE_WCDMA == g_stAtDevCmdCtrl.ucDeviceRatMode)
    {
        ulReceiverPid = AT_GetDestPid(ucIndex, I0_DSP_PID_WPHY);
        usMsgId       = ID_AT_WPHY_RF_PLL_STATUS_REQ;
    }
    else if ( (AT_RAT_MODE_GSM == g_stAtDevCmdCtrl.ucDeviceRatMode)
            ||(AT_RAT_MODE_EDGE == g_stAtDevCmdCtrl.ucDeviceRatMode) )
    {
        ulReceiverPid = AT_GetDestPid(ucIndex, I0_DSP_PID_GPHY);
        usMsgId       = ID_AT_GPHY_RF_PLL_STATUS_REQ;
    }

    else
    {
        return AT_DEVICE_MODE_ERROR;
    }

    /* éê??AT_PHY_RF_PLL_STATUS_REQ_STRU???￠ */
    ulLength = sizeof(AT_PHY_RF_PLL_STATUS_REQ_STRU) - VOS_MSG_HEAD_LENGTH;
    pstMsg   = (AT_PHY_RF_PLL_STATUS_REQ_STRU *)PS_ALLOC_MSG(WUEPS_PID_AT, ulLength);

    if (VOS_NULL_PTR == pstMsg)
    {
        AT_WARN_LOG("AT_QryFPllStatusPara: Alloc msg fail!");
        return AT_ERROR;
    }

    /* ì?3????￠ */
    pstMsg->ulReceiverPid = ulReceiverPid;
    pstMsg->usMsgID       = usMsgId;
    pstMsg->usRsv1        = 0;
    pstMsg->usDspBand     = g_stAtDevCmdCtrl.stDspBandArfcn.usDspBand;
    pstMsg->usRsv2        = 0;

    /* ?ò??ó|PHY·￠?í???￠ */
    if (VOS_OK != PS_SEND_MSG(WUEPS_PID_AT, pstMsg))
    {
        AT_WARN_LOG("AT_QryFPllStatusPara: Send msg fail!");
        return AT_ERROR;
    }

    /* éè??μ±?°2ù×÷ààDí */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_FPLLSTATUS_QRY;
    g_stAtDevCmdCtrl.ucIndex               = ucIndex;

    /* ·μ???üá?′|àí1ò?e×′ì? */
    return AT_WAIT_ASYNC_RETURN;
}


VOS_VOID At_RfPllStatusCnfProc(PHY_AT_RF_PLL_STATUS_CNF_STRU *pstMsg)
{
    VOS_UINT8                           ucIndex;
    VOS_UINT16                          usLength;

    /* ??è?±?μ?±￡′?μ?ó??§?÷òy */
    ucIndex = g_stAtDevCmdCtrl.ucIndex;

    if (AT_CMD_FPLLSTATUS_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("At_RfPllStatusCnfProc: CmdCurrentOpt is not AT_CMD_FPLLSTATUS_QRY!");
        return;
    }

    /* ?′??AT×′ì? */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       "%s: %d,%d",
                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                       pstMsg->usTxStatus,
                                       pstMsg->usRxStatus);

    gstAtSendData.usBufLen = usLength;

    At_FormatResultData(ucIndex, AT_OK);

    return;
}


VOS_UINT32 AT_QryFpowdetTPara(VOS_UINT8 ucIndex)
{
    AT_PHY_POWER_DET_REQ_STRU          *pstMsg;
    VOS_UINT32                          ulLength;

    /*?D??μ±?°?óè??￡ê?￡????§3?W*/
    if ( (AT_RAT_MODE_WCDMA != g_stAtDevCmdCtrl.ucDeviceRatMode)
      && (AT_RAT_MODE_CDMA != g_stAtDevCmdCtrl.ucDeviceRatMode)
      && (AT_RAT_MODE_GSM != g_stAtDevCmdCtrl.ucDeviceRatMode) )
    {
        return AT_DEVICE_MODE_ERROR;
    }

    /* éê??AT_PHY_POWER_DET_REQ_STRU???￠ */
    ulLength = sizeof(AT_PHY_POWER_DET_REQ_STRU) - VOS_MSG_HEAD_LENGTH;
    pstMsg   = (AT_PHY_POWER_DET_REQ_STRU *)PS_ALLOC_MSG(WUEPS_PID_AT, ulLength);

    if (VOS_NULL_PTR == pstMsg)
    {
        AT_WARN_LOG("AT_QryFpowdetTPara: Alloc msg fail!");
        return AT_ERROR;
    }

    /* CDMA的话，发送给UPHY_PID_CSDR_1X_CM */
    if (AT_RAT_MODE_CDMA == g_stAtDevCmdCtrl.ucDeviceRatMode)
    {
        pstMsg->ulReceiverPid = UPHY_PID_CSDR_1X_CM;
    }
    else if (AT_RAT_MODE_GSM == g_stAtDevCmdCtrl.ucDeviceRatMode)
    {
        pstMsg->ulReceiverPid = AT_GetDestPid(ucIndex, I0_DSP_PID_GPHY);
    }
    else
    {
        pstMsg->ulReceiverPid = AT_GetDestPid(ucIndex, I0_DSP_PID_WPHY);
    }

    pstMsg->usMsgID       = ID_AT_PHY_POWER_DET_REQ;
    pstMsg->usRsv         = 0;

    /* ?ò??ó|PHY·￠?í???￠ */
    if (VOS_OK != PS_SEND_MSG(WUEPS_PID_AT, pstMsg))
    {
        AT_WARN_LOG("AT_QryFpowdetTPara: Send msg fail!");
        return AT_ERROR;
    }

    /* éè??μ±?°2ù×÷ààDí */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_FPOWDET_QRY;
    g_stAtDevCmdCtrl.ucIndex               = ucIndex;

    /* ·μ???üá?′|àí1ò?e×′ì? */
    return AT_WAIT_ASYNC_RETURN;
}


VOS_VOID At_RfFpowdetTCnfProc(PHY_AT_POWER_DET_CNF_STRU *pstMsg)
{
    VOS_UINT8                           ucIndex;
    VOS_UINT16                          usLength;

    /* ??è?±?μ?±￡′?μ?ó??§?÷òy */
    ucIndex = g_stAtDevCmdCtrl.ucIndex;

    if (AT_CMD_FPOWDET_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("At_RfFPOWDETCnfProc: CmdCurrentOpt is not AT_CMD_FPOWDET_QRY!");
        return;
    }

    /* ?′??AT×′ì? */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* ó|??àí2?òa?ó￡?è?1?·μ???μ?a0x7FFF?ò?a?TD§?μ￡???2é?ˉ??·μ??ERROR */
    if(0x7FFF == pstMsg->sPowerDet)
    {
        gstAtSendData.usBufLen = 0;
        At_FormatResultData(ucIndex, AT_ERROR);
    }
    else
    {
        usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           "%s: %d",
                                           g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                           pstMsg->sPowerDet);

        gstAtSendData.usBufLen = usLength;

        At_FormatResultData(ucIndex, AT_OK);
    }

    return;
}


VOS_UINT32 AT_SetNvwrSecCtrlPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulResult;
    AT_MTA_NVWRSECCTRL_SET_REQ_STRU     stNvwrSecCtrl;
    VOS_UINT16                          usLength;

    /* ??2?±?á?3?ê??ˉ */
    TAF_MEM_SET_S(&stNvwrSecCtrl, sizeof(stNvwrSecCtrl), 0x00, sizeof(stNvwrSecCtrl));

    /* 2?êy?ì2é */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

     /* 2?êy1y?à */
    if (gucAtParaIndex > 2)
    {
        return AT_TOO_MANY_PARA;
    }

    /* ?ì2é??á÷2?êy3¤?è */
    if (AT_NVWRSECCTRL_PARA_SECTYPE_LEN != gastAtParaList[0].usParaLen)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* éè??°2è?????ààDí */
    stNvwrSecCtrl.ucSecType = (VOS_UINT8)gastAtParaList[0].ulParaValue;

    /* ??×?·?′?2?êy×a???a??á÷ */
    usLength = gastAtParaList[1].usParaLen;
    if ( (2 == gucAtParaIndex)
      && (AT_RSA_CIPHERTEXT_PARA_LEN == usLength) )
    {
        if ( AT_SUCCESS == At_AsciiNum2HexString(gastAtParaList[1].aucPara, &usLength))
        {
            stNvwrSecCtrl.ucSecStrFlg = VOS_TRUE;
            TAF_MEM_CPY_S(stNvwrSecCtrl.aucSecString, sizeof(stNvwrSecCtrl.aucSecString), gastAtParaList[1].aucPara, AT_RSA_CIPHERTEXT_LEN);
        }
    }

    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      gastAtClientTab[ucIndex].opId,
                                      ID_AT_MTA_NVWRSECCTRL_SET_REQ,
                                      &stNvwrSecCtrl,
                                      sizeof(stNvwrSecCtrl),
                                      I0_UEPS_PID_MTA);

    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetNvwrSecCtrlPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /* éè??AT?￡?éêμì?μ?×′ì??aμè′yòì2?·μ?? */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_NVWRSECCTRL_SET;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvMtaNvwrSecCtrlSetCnf( VOS_VOID *pMsg )
{
    AT_MTA_MSG_STRU                    *pstRcvMsg;
    MTA_AT_RESULT_CNF_STRU             *pstResult;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /* 3?ê??ˉ */
    pstRcvMsg       = (AT_MTA_MSG_STRU *)pMsg;
    pstResult       = (MTA_AT_RESULT_CNF_STRU *)pstRcvMsg->aucContent;
    ucIndex         = AT_BROADCAST_CLIENT_INDEX_MODEM_0;

    /* í¨1yClientId??è?ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstRcvMsg->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaNvwrSecCtrlSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaNvwrSecCtrlSetCnf: AT_BROADCAST_INDEX.");
        return VOS_ERR;
    }

    /* ?D??μ±?°2ù×÷ààDíê?·??aAT_CMD_NVWRSECCTRL_SET */
    if (AT_CMD_NVWRSECCTRL_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMtaNvwrSecCtrlSetCnf: NOT CURRENT CMD OPTION!");
        return VOS_ERR;
    }

    /* ?′??AT×′ì? */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* ?D?????′???￠?Dμ?′í?ó?? */
    if (MTA_AT_RESULT_NO_ERROR == pstResult->enResult)
    {
        /* 3é1|￡?ê?3?OK */
        ulResult    = AT_OK;
    }
    else
    {
        /* ê§°ü￡?ê?3?ERROR */
        ulResult    = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* μ÷ó?At_FormatResultData·￠?í?üá??á1? */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_QryNvwrSecCtrlPara(VOS_UINT8 ucIndex)
{
    TAF_NV_NVWR_SEC_CTRL_STRU           stNvwrSecCtrlNV;
    VOS_UINT32                          ulResult;

    /* 2?êy3?ê??ˉ */
    ulResult = AT_ERROR;
    TAF_MEM_SET_S(&stNvwrSecCtrlNV, sizeof(stNvwrSecCtrlNV), 0x00, sizeof(stNvwrSecCtrlNV));

    /* DT??°2è?????NV */
    if (NV_OK == NV_ReadEx(MODEM_ID_0, en_NV_Item_NVWR_SEC_CTRL, &stNvwrSecCtrlNV, sizeof(stNvwrSecCtrlNV)))
    {
        ulResult = AT_OK;
        gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                        (VOS_CHAR*)pgucAtSndCodeAddr,
                                                        (VOS_CHAR*)pgucAtSndCodeAddr,
                                                        "%s: %d",
                                                        g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                        stNvwrSecCtrlNV.ucSecType);
    }

    return ulResult;
}


VOS_BOOL AT_IsNVWRAllowedNvId(VOS_UINT16 usNvId)
{
    TAF_NV_NVWR_SEC_CTRL_STRU           stNvwrSecCtrlNV;
    VOS_UINT8                           ucLoop;
    VOS_UINT8                           ucBlackListNum;

    /* 2?êy3?ê??ˉ */
    TAF_MEM_SET_S(&stNvwrSecCtrlNV, sizeof(stNvwrSecCtrlNV), 0x00, sizeof(stNvwrSecCtrlNV));

    /* ?áè?°2è?????NV */
    if (NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_NVWR_SEC_CTRL, &stNvwrSecCtrlNV, sizeof(stNvwrSecCtrlNV)))
    {
        AT_ERR_LOG("AT_IsNVWRAllowedNvId: NV_ReadEx fail!");
        return VOS_FALSE;
    }

    switch (stNvwrSecCtrlNV.ucSecType)
    {
        case AT_NVWR_SEC_TYPE_OFF:
            return VOS_TRUE;

        case AT_NVWR_SEC_TYPE_ON:
            return VOS_FALSE;

        case AT_NVWR_SEC_TYPE_BLACKLIST:
            ucBlackListNum = (stNvwrSecCtrlNV.ucBlackListNum <= TAF_NV_BLACK_LIST_MAX_NUM) ?
                             stNvwrSecCtrlNV.ucBlackListNum : TAF_NV_BLACK_LIST_MAX_NUM;
            for (ucLoop = 0; ucLoop < ucBlackListNum; ucLoop++)
            {
                if (usNvId == stNvwrSecCtrlNV.ausBlackList[ucLoop])
                {
                    return VOS_FALSE;
                }
            }
            return VOS_TRUE;

        default:
            AT_ERR_LOG1("AT_IsNVWRAllowedNvId: Error SecType:", stNvwrSecCtrlNV.ucSecType);
            break;
    }

    return VOS_FALSE;
}



VOS_UINT32 AT_AsciiToHex(
    VOS_UINT8                          *pucSrc,
    VOS_UINT8                          *pucDst
)
{

    if (( *pucSrc >= '0') && (*pucSrc <= '9')) /* the number is 0-9 */
    {
        *pucDst = (VOS_UINT8)(*pucSrc - '0');
    }
    else if ( (*pucSrc >= 'a') && (*pucSrc <= 'f') ) /* the number is a-f */
    {
        *pucDst = (VOS_UINT8)(*pucSrc - 'a') + 0x0a;
    }
    else if ( (*pucSrc >= 'A') && (*pucSrc <= 'F') ) /* the number is A-F */
    {
        *pucDst = (VOS_UINT8)(*pucSrc - 'A') + 0x0a;
    }
    else
    {
        return VOS_ERR;
    }

    return VOS_OK;
}



VOS_UINT32 AT_AsciiToHexCode_Revers(
    VOS_UINT8                          *pucSrc,
    VOS_UINT16                          usDataLen,
    VOS_UINT8                          *pucDst
)
{
    VOS_INT16                           sLoop1;
    VOS_UINT16                          usLoop2;
    VOS_UINT8                           ucTemp1;
    VOS_UINT8                           ucTemp2;
    VOS_UINT32                          ulRslt;

    sLoop1 = (VOS_INT16)(usDataLen - 1);
    for (usLoop2 = 0; sLoop1 >= 0; sLoop1--, usLoop2++)
    {
        ulRslt = AT_AsciiToHex(&(pucSrc[sLoop1]), &ucTemp1);
        if (VOS_ERR == ulRslt)
        {
            return VOS_ERR;
        }

        sLoop1--;

        ulRslt = AT_AsciiToHex(&(pucSrc[sLoop1]), &ucTemp2);
        if (VOS_ERR == ulRslt)
        {
            return VOS_ERR;
        }

        pucDst[usLoop2] = (VOS_UINT8)((ucTemp2 << 4) | ucTemp1);
    }

    return VOS_OK;
}


VOS_UINT32 AT_Hex2Ascii_Revers(
    VOS_UINT8                           aucHex[],
    VOS_UINT32                          ulLength,
    VOS_UINT8                           aucAscii[]
)
{
    VOS_INT32                           lLoopSrc;
    VOS_UINT32                          ulLoopDest;
    VOS_UINT8                           ucTemp;

    lLoopSrc = (VOS_INT32)(ulLength - 1);
    for (ulLoopDest = 0; lLoopSrc >= 0; lLoopSrc--, ulLoopDest++)
    {
        ucTemp = (aucHex[lLoopSrc]>>4) & 0x0F;
        if (ucTemp < 10)
        {
            /* 0~9 */
            aucAscii[ulLoopDest] = ucTemp + 0x30;
        }
        else
        {
            /* a~f */
            aucAscii[ulLoopDest] = ucTemp + 0x37;
        }

        ulLoopDest++;
        ucTemp = aucHex[lLoopSrc] & 0x0F;
        if (ucTemp < 10)
        {
            /* 0~9 */
            aucAscii[ulLoopDest] = ucTemp + 0x30;
        }
        else
        {
            /* a~f */
            aucAscii[ulLoopDest] = ucTemp + 0x37;
        }
    }

    return VOS_OK;
}


VOS_UINT32 AT_SetMeidPara(VOS_UINT8 ucIndex)
{
    AT_MTA_MEID_SET_REQ_STRU            stMeIdReq;
    VOS_UINT32                          ulRslt;

    TAF_MEM_SET_S(&stMeIdReq, sizeof(stMeIdReq), 0x00, sizeof(AT_MTA_MEID_SET_REQ_STRU));

    /* ?üá?×′ì??ì2é */
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /* 2?êy??êy2??a1?ò??×?·?′?3¤?è2??a14 */
    if ((1 != gucAtParaIndex)
     || (14 != gastAtParaList[0].usParaLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulRslt = AT_AsciiToHexCode_Revers(gastAtParaList[0].aucPara,
                                      gastAtParaList[0].usParaLen,
                                      stMeIdReq.aucMeid);

    if (VOS_OK != ulRslt)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ulRslt = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                    gastAtClientTab[ucIndex].opId,
                                    ID_AT_MTA_MEID_SET_REQ,
                                    &stMeIdReq,
                                    sizeof(stMeIdReq),
                                    I0_UEPS_PID_MTA);

    if (TAF_SUCCESS == ulRslt)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_MEID_SET;

        /* ·μ???üá?′|àí1ò?e×′ì? */
        return AT_WAIT_ASYNC_RETURN;
    }
    else
    {
        AT_WARN_LOG("AT_SetMeidPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

}


VOS_UINT32 AT_QryMeidPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulResult;

    /* 2?êy?ì2é */
    if (AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    /* ·￠?í???￠*/
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      gastAtClientTab[ucIndex].opId,
                                      ID_AT_MTA_MEID_QRY_REQ,
                                      VOS_NULL_PTR,
                                      0,
                                      I0_UEPS_PID_MTA);

    if (TAF_SUCCESS == ulResult)
    {
        gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_MEID_QRY;

        /* ·μ???üá?′|àí1ò?e×′ì? */
        return AT_WAIT_ASYNC_RETURN;
    }
    else
    {
        AT_WARN_LOG("AT_QryMeidPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }
}


VOS_UINT32 AT_RcvMtaMeidSetCnf(
    VOS_VOID                           *pMsg
)
{
    AT_MTA_MSG_STRU                    *pstRcvMsg;
    MTA_AT_RESULT_CNF_STRU             *pstSetCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulRslt;

    /* 3?ê??ˉ */
    pstRcvMsg       = (AT_MTA_MSG_STRU *)pMsg;
    pstSetCnf       = (MTA_AT_RESULT_CNF_STRU *)pstRcvMsg->aucContent;
    ucIndex         = AT_BROADCAST_CLIENT_INDEX_MODEM_0;

    /* í¨1yClientId??è?ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstRcvMsg->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaMeidSetCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaMeidSetCnf: AT_BROADCAST_INDEX.");
        return VOS_ERR;
    }

    /* ?D??μ±?°2ù×÷ààDíê?·??aAT_CMD_MEID_SET */
    if (AT_CMD_MEID_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMtaMeidSetCnf: NOT CURRENT CMD OPTION!");
        return VOS_ERR;
    }

    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* ??ê??ˉAT^MEID?üá?·μ?? */
    gstAtSendData.usBufLen = 0;

    switch (pstSetCnf->enResult)
    {
        case MTA_AT_RESULT_INCORRECT_PARAMETERS:
            ulRslt = AT_DEVICE_INVALID_PARAMETERS;
            break;

        case MTA_AT_RESULT_DEVICE_SEC_NV_ERROR:
            ulRslt = AT_DEVICE_NV_WRITE_FAIL_UNKNOWN;
            break;

        case MTA_AT_RESULT_NO_ERROR:
            ulRslt = AT_OK;
            break;

        default:
            ulRslt = AT_ERROR;
            break;
    }

    At_FormatResultData(ucIndex, ulRslt);

    return VOS_OK;
}


VOS_UINT32 AT_RcvMtaMeidQryCnf(
    VOS_VOID                           *pMsg
)
{
    AT_MTA_MSG_STRU                    *pstRcvMsg;
    MTA_AT_MEID_QRY_CNF_STRU           *pstQryCnf;
    VOS_UINT8                           ucIndex;
    NV_MEID_STRU                        stMeId;
    NV_PESN_STRU                        stPEsn;
    VOS_UINT8                           aucMeId[2*MEID_NV_DATA_LEN_NEW + 1];
    VOS_UINT8                           aucpEsn[2*PESN_NV_DATA_LEN + 1];
    VOS_UINT8                           aucpUimID[2*UIMID_DATA_LEN + 1];

    TAF_MEM_SET_S(&stMeId, sizeof(stMeId), 0x00, sizeof(NV_MEID_STRU));
    TAF_MEM_SET_S(&stPEsn, sizeof(stPEsn), 0x00, sizeof(NV_PESN_STRU));
    TAF_MEM_SET_S(aucMeId, sizeof(aucMeId), 0x00, sizeof(aucMeId));

    /* 3?ê??ˉ */
    pstRcvMsg       = (AT_MTA_MSG_STRU *)pMsg;
    pstQryCnf       = (MTA_AT_MEID_QRY_CNF_STRU *)pstRcvMsg->aucContent;
    ucIndex         = AT_BROADCAST_CLIENT_INDEX_MODEM_0;

    /* í¨1yClientId??è?ucIndex */
    if (AT_FAILURE == At_ClientIdToUserId(pstRcvMsg->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaMeidQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaMeidQryCnf: AT_BROADCAST_INDEX.");
        return VOS_ERR;
    }

    /* ?D??μ±?°2ù×÷ààDíê?·??aAT_CMD_MEID_QRY */
    if (AT_CMD_MEID_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMtaMeidQryCnf: NOT CURRENT CMD OPTION!");
        return VOS_ERR;
    }

    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* ??ê??ˉAT^MEID?üá?·μ?? */
    gstAtSendData.usBufLen = 0;

    /* ?áè?en_NV_Item_MEID */
    if (NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_MEID, &stMeId, sizeof(NV_MEID_STRU)))
    {
        AT_WARN_LOG("AT_RcvMtaMeidQryCnf:Read en_NV_Item_MEID Nvim Failed");
        return AT_DEVICE_NV_READ_FAILURE;
    }

    /* ?áè?en_NV_Item_PESN */
    if (NV_OK != NV_ReadEx(MODEM_ID_0, en_NV_Item_PESN, &stPEsn, sizeof(NV_PESN_STRU)))
    {
        AT_WARN_LOG("AT_RcvMtaMeidQryCnf:Read en_NV_Item_PESN Nvim Failed");
        return AT_DEVICE_NV_READ_FAILURE;
    }

    /* ?áè?UIMIDê§°ü */
    if (pstQryCnf->enResult != MTA_AT_RESULT_NO_ERROR)
    {
        AT_WARN_LOG("AT_RcvMtaMeidQryCnf:Read UIMID Failed");
    }

    AT_Hex2Ascii_Revers(&(pstQryCnf->aucEFRUIMID[1]), UIMID_DATA_LEN, aucpUimID);
    aucpUimID[2*UIMID_DATA_LEN] = '\0';

    AT_Hex2Ascii_Revers(&(stPEsn.aucPEsn[0]), PESN_NV_DATA_LEN, aucpEsn);
    aucpEsn[2*PESN_NV_DATA_LEN] = '\0';

    AT_Hex2Ascii_Revers(&(stMeId.aucMeID[0]), MEID_NV_DATA_LEN_NEW, aucMeId);
    aucMeId[2*MEID_NV_DATA_LEN_NEW] = '\0';

    gstAtSendData.usBufLen = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                   (VOS_CHAR *)pgucAtSndCodeAddr,
                                                    "%s: %s,%s,%s",
                                                    g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                                    aucMeId,
                                                    aucpEsn,
                                                    aucpUimID);

    At_FormatResultData(ucIndex, AT_OK);

    return VOS_OK;
}



VOS_UINT32 AT_SetSlavePara(VOS_UINT8 ucIndex)
{
    AT_MTA_SLAVE_SET_REQ_STRU           stSlave;
    VOS_UINT32                          ulResult;

    /*局部变量初始化*/
    TAF_MEM_SET_S(&stSlave, (VOS_SIZE_T)sizeof(stSlave), 0x00, (VOS_SIZE_T)sizeof(AT_MTA_SLAVE_SET_REQ_STRU));

    /*参数检查*/
    if (AT_CMD_OPT_SET_PARA_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /*参数数目过多*/
    if (1 != gucAtParaIndex )
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /*参数长度检查*/
    if (1 != gastAtParaList[0].usParaLen)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /*填写消息参数*/
    stSlave.ucRatType = gastAtParaList[0].ulParaValue;

    /*发送消息给MTA*/
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      gastAtClientTab[ucIndex].opId,
                                      ID_AT_MTA_SLAVE_SET_REQ,
                                      &stSlave,
                                      (VOS_SIZE_T)sizeof(stSlave),
                                      I0_UEPS_PID_MTA);
    /*发送失败*/
    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetSlavePara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /*发送成功，设置当前操作模式*/
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_SETSLAVE_SET;

    /*等待异步处理时间返回*/
    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvMtaSetSlaveCnf( VOS_VOID *pMsg )
{
    AT_MTA_MSG_STRU                    *pstRcvMsg;
    MTA_AT_RESULT_CNF_STRU             *pstResult;
    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /*初始化局部变量*/
    pstRcvMsg       = (AT_MTA_MSG_STRU *)pMsg;
    pstResult       = (MTA_AT_RESULT_CNF_STRU *)pstRcvMsg->aucContent;
    ucIndex         = AT_BROADCAST_CLIENT_INDEX_MODEM_0;

    /*通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstRcvMsg->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaSetSlaveCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaSetSlaveCnf: AT_BROADCAST_INDEX.");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_SETSLAVE_SET */
    if (AT_CMD_SETSLAVE_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMtaSetSlaveCnf: NOT CURRENT CMD OPTION!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* MTA传来消息命令结果处理 */
    if (MTA_AT_RESULT_NO_ERROR == pstResult->enResult)
    {
        /*命令结果 *AT_OK*/
        ulResult    = AT_OK;
    }
    else
    {
        /*命令结果 *AT_ERROR*/
        ulResult    = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;

    /* 调用AT_FormATResultDATa发送命令结果 */
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_QryRficDieIDPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulRst;

    if(AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    /* 发送跨核消息到C核, 查询RFIC IDE ID*/
    ulRst = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_MTA_RFIC_DIE_ID_QRY_REQ,
                                   VOS_NULL_PTR,
                                   0,
                                   I0_UEPS_PID_MTA);

    if (TAF_SUCCESS != ulRst)
    {
        AT_WARN_LOG("AT_QryRficDieIDPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /* 设置AT模块实体的状态为等待异步返回 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_RFIC_DIE_ID_QRY;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvMtaRficDieIDQryCnf( VOS_VOID *pMsg )
{
    AT_MTA_MSG_STRU                    *pstRcvMsg;
    MTA_AT_RFIC_DIE_ID_REQ_CNF_STRU    *pstRficDieIDReqCnf;
    VOS_UINT8                           ucIndex;
    VOS_UINT16                          usLength;

    /*初始化局部变量*/
    pstRcvMsg                 = (AT_MTA_MSG_STRU *)pMsg;
    pstRficDieIDReqCnf        = (MTA_AT_RFIC_DIE_ID_REQ_CNF_STRU *)pstRcvMsg->aucContent;
    ucIndex                   = AT_BROADCAST_CLIENT_INDEX_MODEM_0;
    usLength                  = 0;

    /*通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstRcvMsg->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaRficDieIDQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaRficDieIDQryCnf: AT_BROADCAST_INDEX.");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_RFIC_DIE_ID_QRY */
    if (AT_CMD_RFIC_DIE_ID_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMtaRficDieIDQryCnf: NOT CURRENT CMD OPTION!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /*格式化上报命令*/
    if (MTA_AT_RESULT_NO_ERROR != pstRficDieIDReqCnf->enResult)
    {
        /*命令结果 *AT_ERROR*/
        gstAtSendData.usBufLen = 0;
        At_FormatResultData(ucIndex, AT_ERROR);
    }
    else
    {
        /*命令结果 *AT_OK*/
        if (1 == pstRficDieIDReqCnf->usRfic0DieIdValid)
        {
            usLength = AT_RficDieIDOut((VOS_UINT8*)pstRficDieIDReqCnf->ausRfic0DieId, 0, usLength, ucIndex);
        }

        if (1 == pstRficDieIDReqCnf->usRfic1DieIdValid)
        {
            usLength = AT_RficDieIDOut((VOS_UINT8*)pstRficDieIDReqCnf->ausRfic1DieId, 1, usLength, ucIndex);
        }
        gstAtSendData.usBufLen = usLength;
        At_FormatResultData(ucIndex, AT_OK);
    }

    return VOS_OK;
}


VOS_UINT16 AT_RficDieIDOut(
    VOS_UINT8                          *pMsg,
    VOS_UINT32                          RficNum,
    VOS_UINT16                          usLength,
    VOS_UINT8                           ucIndex
)
{
    VOS_UINT32                          i;           /*循环用*/
    VOS_UINT16                          usLengthtemp;

    usLengthtemp = usLength;

    usLengthtemp += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLengthtemp,
                                       "%s: %d,\"",
                                       g_stParseContext[ucIndex].pstCmdElement->pszCmdName,
                                       RficNum);

    /*RFIC ID 使用低八位数据*/
    for (i =  0; i < (MTA_RCM_MAX_DIE_ID_LEN * 2); i += 2)
    {
        usLengthtemp += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                           (VOS_CHAR *)pgucAtSndCodeAddr,
                                           (VOS_CHAR *)pgucAtSndCodeAddr + usLengthtemp,
                                           "%02x",
                                           pMsg[i]);
    }

    usLengthtemp += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLengthtemp,
                                       "\"%s",
                                       gaucAtCrLf);
    return usLengthtemp;
}


VOS_UINT32 AT_QryPmuDieSNPara(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulRst;

    if(AT_CMD_OPT_READ_CMD != g_stATParseCmd.ucCmdOptType)
    {
        return AT_ERROR;
    }

    /* 发送跨核消息到C核, 查询RFIC IDE ID*/
    ulRst = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                   gastAtClientTab[ucIndex].opId,
                                   ID_AT_MTA_PMU_DIE_SN_QRY_REQ,
                                   VOS_NULL_PTR,
                                   0,
                                   I0_UEPS_PID_MTA);

    if (TAF_SUCCESS != ulRst)
    {
        AT_WARN_LOG("AT_QryPmuDieSNPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /* 设置AT模块实体的状态为等待异步返回 */
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_PMU_DIE_SN_QRY;

    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvMtaPmuDieSNQryCnf( VOS_VOID *pMsg )
{
    AT_MTA_MSG_STRU                    *pstRcvMsg;
    MTA_AT_PMU_DIE_SN_REQ_CNF_STRU     *pstPmuDieSNReqCnf;
    VOS_INT32                           i;        /*循环数*/
    VOS_UINT16                          usLength;
    VOS_UINT8                           ucIndex;

    /*初始化局部变量*/
    pstRcvMsg                 = (AT_MTA_MSG_STRU *)pMsg;
    pstPmuDieSNReqCnf         = (MTA_AT_PMU_DIE_SN_REQ_CNF_STRU *)pstRcvMsg->aucContent;
    ucIndex                   = AT_BROADCAST_CLIENT_INDEX_MODEM_0;
    usLength                  = 0;

    /*通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstRcvMsg->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaPmuDieSNQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaPmuDieSNQryCnf: AT_BROADCAST_INDEX.");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_PMU_DIE_SN_QRY */
    if (AT_CMD_PMU_DIE_SN_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMtaPmuDieSNQryCnf: NOT CURRENT CMD OPTION!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /*格式化上报命令*/
    if (MTA_AT_RESULT_NO_ERROR != pstPmuDieSNReqCnf->enResult)
    {
        /*命令结果 *AT_ERROR*/
        gstAtSendData.usBufLen = 0;
        At_FormatResultData(ucIndex, AT_ERROR);
    }
    else
    {
        /*命令结果 *AT_OK*/

        /* 最高位,高4 BIT置0 */
        pstPmuDieSNReqCnf->aucDieSN[MTA_PMU_MAX_DIE_SN_LEN - 1] = (pstPmuDieSNReqCnf->aucDieSN[MTA_PMU_MAX_DIE_SN_LEN - 1] & 0x0F);

        /* 格式化输出查询结果 */
        usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          "%s: 0x",
                                          g_stParseContext[ucIndex].pstCmdElement->pszCmdName);

        for (i = (MTA_PMU_MAX_DIE_SN_LEN-1); i >= 0; i--)
        {
            usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                               (VOS_CHAR *)pgucAtSndCodeAddr,
                                               (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                               "%02x",
                                               pstPmuDieSNReqCnf->aucDieSN[i]);
        }

        gstAtSendData.usBufLen = usLength;
        At_FormatResultData(ucIndex, AT_OK);
    }

    return VOS_OK;
}



VOS_UINT32 AT_SetTasTestCfg(VOS_UINT8 ucIndex)
{
    AT_MTA_TAS_TEST_CFG_STRU            stTasParamReq;
    VOS_UINT32                          ulResult;

    /*局部变量初始化*/
    TAF_MEM_SET_S(&stTasParamReq, (VOS_SIZE_T)sizeof(stTasParamReq), 0x00, (VOS_SIZE_T)sizeof(AT_MTA_TAS_TEST_CFG_STRU));

    /*参数检查*/
    if ((5 != gucAtParaIndex)
     || (0 == gastAtParaList[0].usParaLen)
     || (0 == gastAtParaList[1].usParaLen)
     || (0 == gastAtParaList[2].usParaLen)
     || (0 == gastAtParaList[3].usParaLen)
     || (0 == gastAtParaList[4].usParaLen))
        {
            return AT_CME_INCORRECT_PARAMETERS;
        }

    /*填写消息参数*/
    stTasParamReq.enRatMode = gastAtParaList[0].ulParaValue;
    stTasParamReq.ulPara0   = gastAtParaList[1].ulParaValue;
    stTasParamReq.ulPara1   = gastAtParaList[2].ulParaValue;
    stTasParamReq.ulPara2   = gastAtParaList[3].ulParaValue;
    stTasParamReq.ulPara3   = gastAtParaList[4].ulParaValue;

    /*发送消息给MTA*/
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      gastAtClientTab[ucIndex].opId,
                                      ID_AT_MTA_TAS_CFG_REQ,
                                      &stTasParamReq,
                                      (VOS_SIZE_T)sizeof(stTasParamReq),
                                      I0_UEPS_PID_MTA);
    /*发送失败*/
    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_SetTasTestCfg: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /*发送成功，设置当前操作模式*/
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_TAS_TEST_SET;

    /*等待异步处理时间返回*/
    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_QryTasTestCfgPara(VOS_UINT8 ucIndex)
{

    VOS_UINT32                          ulResult;
    AT_MTA_TAS_TEST_QRY_STRU            stAtMtaTasTestQry;

    /*参数检查*/
    if ((1 != gucAtParaIndex)
     || (0 == gastAtParaList[0].usParaLen))
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    /*填写消息参数*/
    stAtMtaTasTestQry.enRatMode = gastAtParaList[0].ulParaValue;

    /*发送消息给MTA*/
    ulResult = AT_FillAndSndAppReqMsg(gastAtClientTab[ucIndex].usClientId,
                                      gastAtClientTab[ucIndex].opId,
                                      ID_AT_MTA_TAS_QRY_REQ,
                                      &stAtMtaTasTestQry,
                                      (VOS_SIZE_T)sizeof(stAtMtaTasTestQry),
                                      I0_UEPS_PID_MTA);
    /*发送失败*/
    if (TAF_SUCCESS != ulResult)
    {
        AT_WARN_LOG("AT_QryTasTestCfgPara: AT_FillAndSndAppReqMsg fail.");
        return AT_ERROR;
    }

    /*发送成功，设置当前操作模式*/
    gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_TAS_TEST_QRY;

    /*等待异步处理时间返回*/
    return AT_WAIT_ASYNC_RETURN;
}


VOS_UINT32 AT_RcvMtaTasTestCfgCnf( VOS_VOID *pMsg )
{
    AT_MTA_MSG_STRU                    *pstRcvMsg;
    MTA_AT_TAS_TEST_CFG_CNF_STRU       *pstMtaAtTasTestCfgCnf = VOS_NULL_PTR;

    VOS_UINT8                           ucIndex;
    VOS_UINT32                          ulResult;

    /*初始化局部变量*/
    pstRcvMsg                 = (AT_MTA_MSG_STRU *)pMsg;
    pstMtaAtTasTestCfgCnf     = (MTA_AT_TAS_TEST_CFG_CNF_STRU *)pstRcvMsg->aucContent;
    ucIndex                   = AT_BROADCAST_CLIENT_INDEX_MODEM_0;

    /*通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstRcvMsg->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaTasTestCfgCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaTasTestCfgCnf: AT_BROADCAST_INDEX.");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_TAS_TEST_CFG_SET */
    if (AT_CMD_TAS_TEST_SET != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMtaTasTestCfgCnf: NOT CURRENT CMD OPTION!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /*格式化上报命令*/
    if (MTA_AT_RESULT_NO_ERROR == pstMtaAtTasTestCfgCnf->enResult)
    {
        /*命令结果 *AT_OK*/
        ulResult    = AT_OK;
    }
    else
    {
        /*命令结果 *AT_ERROR*/
        ulResult    = AT_ERROR;
    }

    gstAtSendData.usBufLen = 0;
    At_FormatResultData(ucIndex, ulResult);

    return VOS_OK;
}


VOS_UINT32 AT_RcvMtaTasTestQryCnf( VOS_VOID *pMsg )
{
    AT_MTA_MSG_STRU                    *pstRcvMsg;
    MTA_AT_TAS_TEST_QRY_CNF_STRU       *pstMtaAtTasTestReqCnf;
    VOS_UINT16                          usLength;
    VOS_UINT8                           ucIndex;

    /* 初始化局部变量 */
    pstRcvMsg                 = (AT_MTA_MSG_STRU *)pMsg;
    pstMtaAtTasTestReqCnf     = (MTA_AT_TAS_TEST_QRY_CNF_STRU *)pstRcvMsg->aucContent;
    usLength                  = 0;
    ucIndex                   = 0;

    /* 通过clientid获取index */
    if (AT_FAILURE == At_ClientIdToUserId(pstRcvMsg->stAppCtrl.usClientId, &ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaTasTestQryCnf: WARNING:AT INDEX NOT FOUND!");
        return VOS_ERR;
    }

    if (AT_IS_BROADCAST_CLIENT_INDEX(ucIndex))
    {
        AT_WARN_LOG("AT_RcvMtaTasTestQryCnf: AT_BROADCAST_INDEX.");
        return VOS_ERR;
    }

    /* 判断当前操作类型是否为AT_CMD_RFIC_DIE_ID_QRY */
    if (AT_CMD_TAS_TEST_QRY != gastAtClientTab[ucIndex].CmdCurrentOpt)
    {
        AT_WARN_LOG("AT_RcvMtaTasTestQryCnf: NOT CURRENT CMD OPTION!");
        return VOS_ERR;
    }

    /* 复位AT状态 */
    AT_STOP_TIMER_CMD_READY(ucIndex);

    /* 格式化上报命令 */
    if (MTA_AT_RESULT_NO_ERROR != pstMtaAtTasTestReqCnf->enResult)
    {
        /* 命令结果 *AT_ERROR */
        gstAtSendData.usBufLen = 0;
        At_FormatResultData(ucIndex, AT_ERROR);
    }
    else
    {
        /* 命令结果 *AT_OK */
        usLength = AT_TasTestOut(pstMtaAtTasTestReqCnf);
        gstAtSendData.usBufLen = usLength;
        At_FormatResultData(ucIndex, AT_OK);
    }

    return VOS_OK;
}


VOS_UINT16 AT_TasTestOut(
    MTA_AT_TAS_TEST_QRY_CNF_STRU       *pstMtaAtTasTestReqCnf
)
{
    VOS_UINT32                          i;           /*循环用*/
    VOS_UINT16                          usLength;

    usLength = 0;

    usLength = (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      (VOS_CHAR *)pgucAtSndCodeAddr,
                                      "level index: %d (%d, %d, %d, %d)%s",
                                       pstMtaAtTasTestReqCnf->ulCurrLevel,
                                       pstMtaAtTasTestReqCnf->stLevelInfo.uhwSrcAntTimeLength,
                                       pstMtaAtTasTestReqCnf->stLevelInfo.uhwDestAntTimeLength,
                                       pstMtaAtTasTestReqCnf->stLevelInfo.shwSrcAntTxPowerGain,
                                       pstMtaAtTasTestReqCnf->stLevelInfo.shwDestAntTxPowerGain,
                                       gaucAtCrLf);

    usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                       (VOS_CHAR *)pgucAtSndCodeAddr,
                                       (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                       "level table:%s",
                                       gaucAtCrLf);

    if (pstMtaAtTasTestReqCnf->stUsedTable.uhwLevelNum > MAX_STATEII_LEVEL_ITEM)
    {
        pstMtaAtTasTestReqCnf->stUsedTable.uhwLevelNum = MAX_STATEII_LEVEL_ITEM;
    }

    /* 打印表格 */
    for (i =  0; i < pstMtaAtTasTestReqCnf->stUsedTable.uhwLevelNum; i++)
    {
        usLength += (VOS_UINT16)At_sprintf(AT_CMD_MAX_LEN,
                                          (VOS_CHAR *)pgucAtSndCodeAddr,
                                          (VOS_CHAR *)pgucAtSndCodeAddr + usLength,
                                          "            (%d, %d, %d, %d)%s",
                                           pstMtaAtTasTestReqCnf->stUsedTable.astLevelItem[i].uhwSrcAntTimeLength,
                                           pstMtaAtTasTestReqCnf->stUsedTable.astLevelItem[i].uhwDestAntTimeLength,
                                           pstMtaAtTasTestReqCnf->stUsedTable.astLevelItem[i].shwSrcAntTxPowerGain,
                                           pstMtaAtTasTestReqCnf->stUsedTable.astLevelItem[i].shwDestAntTxPowerGain,
                                           gaucAtCrLf);
    }
    return usLength;
}


VOS_UINT32 AT_SetCdmaAttDiversitySwitch(VOS_UINT8 ucIndex)
{
    VOS_UINT32                          ulRet;
    VOS_UINT32                          ulNVWrTotleLen;
    VOS_UINT8                           ulNVWrLen;
    VOS_UINT8                           ucDiversitySwitch;

    if (VOS_TRUE != AT_IsNVWRAllowedNvId(en_NV_Item_CPROC_1X_NVIM_DM_THRESHOLD))
    {
        return AT_CME_OPERATION_NOT_ALLOWED;
    }

    if (0 == gastAtParaList[0].usParaLen)
    {
        return AT_CME_INCORRECT_PARAMETERS;
    }

    ucDiversitySwitch = (VOS_UINT8)gastAtParaList[0].ulParaValue;
    ulNVWrLen         = sizeof(ucDiversitySwitch);

    ulRet = NV_GetLength(en_NV_Item_CPROC_1X_NVIM_DM_THRESHOLD, &ulNVWrTotleLen);

    if ((ERR_MSP_SUCCESS != ulRet)
     || (ulNVWrLen > ulNVWrTotleLen) )
    {
        return AT_ERROR;
    }

    ulRet = NV_WritePartEx(MODEM_ID_0, en_NV_Item_CPROC_1X_NVIM_DM_THRESHOLD, 0, (VOS_VOID*)&ucDiversitySwitch, ulNVWrLen);
    if(ERR_MSP_SUCCESS != ulRet)
    {
        return AT_ERROR;
    }

    return AT_OK;
}


