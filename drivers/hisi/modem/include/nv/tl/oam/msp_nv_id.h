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



#ifndef __MSP_NV_ID_H__
#define __MSP_NV_ID_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


typedef enum
{
    NV_ID_MSP_IMEI                       = 0,
    NV_ID_MSP_CALIBRATED_TIME            = 5,/*保留*/
    NV_ID_MSP_SERIAL_NUMBER              = 6,
    NV_ID_MSP_LOGFILE_MAX_SIZE           = 8,/*保留*/
    NV_ID_MSP_WATCHDOG_FLAG              = 9,
    NV_ID_MSP_KICKDOG_TIME               = 11,
    NV_ID_MSP_SCHEDULE_WATCHDOG_TIME     = 12,
    NV_ID_MSP_AGING_TEST_TABLE           = 16,
    NV_ID_MSP_MAX_REG_CNT_IN_GSM_MODE    = 28,
    NV_ID_MSP_DFS_CONFIG                 = 29,
    NV_ID_MSP_SW_VER_FLAG                = 36,
    NV_ID_MSP_ERRLOG_ENABLE              = 37,
    NV_ID_MSP_ERRLOG_FLUSH               = 38,
    NV_ID_MSP_ERRLOG_RECOVER             = 39,
    NV_ID_MSP_USIM_TEMP_PROTECT_NEW      = 40,
    NV_ID_MSP_OM_PORT_TYPE               = 45,
    NV_ID_MSP_PRINTF_PORT_TYPE           = 46,
    NV_ID_MSP_PSLOG_PORT_TYPE            = 47,
    NV_ID_MSP_M2_ENABLE                  = 59,
    NV_ID_MSP_LOG_FILE_SAVE_CON          = 117,
    NV_ID_MSP_OAM_CONFIG                 = 120,
    NV_ID_MSP_ZSP_LDF_CONFIG             = 122,
    NV_ID_MSP_HIFI_LDF_CONFIG            = 123,
    NV_ID_MSP_PRIORITY_THREAD            = 127,
    NV_ID_MSP_MANUFACTUREINFO            = 32768,
    NV_ID_MSP_PPP_CONFIG_MRU_TYPE        = 32769,
    NV_ID_MSP_WIFI_KEY                   = 50012,
    NV_ID_MSP_USIM_SUPPORT_FEATURE       = 9090,
    NV_ID_MSP_USIM_STATUS_NEED_FCP       = 9108,
    NV_ID_MSP_USIM_OPS_FILE_CONFIG       = 9126,
    NV_ID_MSP_SIM_OPS_FILE_CONFIG        = 9127,
    NV_ID_MSP_TEST_CARD_CONFIG           = 9129,
    NV_ID_MSP_CSIM                       = 0xD003,
    NV_ID_MSP_RSRP_CFG                   = 0xD004,
    NV_ID_MSP_ECIO_CFG                   = 0xD005,
    NV_ID_MSP_RSCP_CFG                   = 0xD006,
    NV_ID_MSP_FLASH_LESS_MID_THRED       = 0xD007,
    NV_ID_MSP_AGENT_FLAG                 = 0xD008,
    NV_ID_MSP_AUTHORITY_VER              = 0xD009,
    NV_ID_MSP_AUTHORITY_TYPE             = 0xD00a,
    NV_ID_MSP_AUTHDATA_1                 = 0xD00b,
    NV_ID_MSP_AUTHDATA_2                 = 0xD00c,
    NV_ID_MSP_AUTHDATA_3                 = 0xD00d,
    NV_ID_MSP_AUTHDATA_4                 = 0xD00e,
    NV_ID_MSP_AUTHDATA_5                 = 0xD00f,
    NV_ID_MSP_AUTHDATA_6                 = 0xD010,
    NV_ID_MSP_AUTHDATA_7                 = 0xD011,
    NV_ID_MSP_AUTHDATA_8                 = 0xD012,
    NV_ID_MSP_AUTHDATA_9                 = 0xD013,
    NV_ID_MSP_AUTHDATA_10                = 0xD014,
    NV_ID_MSP_AUTHDATA_11                = 0xD015,
    NV_ID_MSP_AUTHDATA_12                = 0xD016,
    NV_ID_MSP_AUTHDATA_13                = 0xD017,
    NV_ID_MSP_AUTHDATA_14                = 0xD018,
    NV_ID_MSP_AUTHDATA_15                = 0xD019,
    NV_ID_MSP_AUTHDATA_16                = 0xD01a,
    NV_ID_MSP_TL_DRX_RESUME_TIME         = 0xD01c,
    EN_NV_ID_AGENT_FLAG                  = 0xD01D,
    NV_ID_MSP_TMODE_RAT                  = 0xD0EF,
    NV_ID_MSP_BOARD_PLATFORM             = 0xE514,
    NV_ID_MSP_SYS_TIME                   = 0xD101,
    NV_ID_MSP_UDP_TYPE                   = 0xE517,
    NV_ID_END    = 0XFFFF
}NV_ID_MSP_ENUM;

/*为保证编译通过移入老版本，后续增加请增加到上面的定义中*/
/*******************************NV ID START**************************************/
typedef enum
{
    EN_NV_ID_IMEI                       = 0,
    EN_NV_ID_RSRP_CFG                   = 0xD004,
	EN_NV_ID_ECIO_CFG                   = 0xD005,
    EN_NV_ID_RSCP_CFG                   = 0xD006,
    EN_NV_ID_AUTHORITY_VER              = 0xD00A,
    EN_NV_ID_AUTHORITY_TYPE             = 0xD00B,
    EN_NV_ID_TMODE_RAT                  = 0xD0EF,

    EN_NV_ID_FLASH_LESS_MID_THRED       = 0xD100,
    EN_NV_ID_SOCP_LOG_CFG 				= 0xD101,
	EN_NV_ID_PWC_SWITCH 				= 0xD10B,
    EN_NV_ID_DRV_E5_HW_TEST				= 0xD10D,

	EN_NV_ID_UDP_TYPE					= 0xE517,
    NV_ID_TDS_TX_PA_POWER_BAND_A        = 0xF8f0,
    NV_ID_TDS_TX_APC_COMP_BAND_A        = 0xF8f1,
    NV_ID_TDS_TX_APC_FREQ_COMP_BAND_A   = 0xF8f2,
    NV_ID_TDS_RX_AGC_COMP_BAND_A        = 0xF8f3,
    NV_ID_TDS_RX_AGC_FREQ_COMP_BAND_A   = 0xF8f4,
    NV_ID_TDS_TX_PA_POWER_BAND_E        = 0xF8f5,
    NV_ID_TDS_TX_APC_COMP_BAND_E        = 0xF8f6,
    NV_ID_TDS_TX_APC_FREQ_COMP_BAND_E   = 0xF8f7,
    NV_ID_TDS_RX_AGC_COMP_BAND_E        = 0xF8f8,
    NV_ID_TDS_RX_AGC_FREQ_COMP_BAND_E   = 0xF8f9,
    NV_ID_TDS_TX_PA_POWER_BAND_F        = 0xF8fa,
    NV_ID_TDS_TX_APC_COMP_BAND_F        = 0xF8fb,
    NV_ID_TDS_TX_APC_FREQ_COMP_BAND_F   = 0xF8fc,
    NV_ID_TDS_RX_AGC_COMP_BAND_F        = 0xF8fd,
    NV_ID_TDS_RX_AGC_FREQ_COMP_BAND_F   = 0xF8fe,
    EN_NV_ID_END                        = 0XFFFF
} LNV_ITEM_ID_ENUM;
/*******************************NV ID END**************************************/



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
