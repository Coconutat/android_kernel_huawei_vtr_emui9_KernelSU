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


#include "NVIM_ResumeId.h"
#include "NvIdList.h"

#include "mdrv_nvim_comm.h"
#include <drv_nv_id.h>

/*lint -e553 */
#include "LNvCommon.h"
#include "lt_phy_nv_define.h"
#include "product_nv_id.h"
/*lint +e553 */


#define BAND_NV_RESUME_WCDMA(BAND_ID)    \
    en_NV_Item_W_RX_IP2_CAL_AT1_##BAND_ID,\
    en_NV_Item_W_RX_IP2_CAL_AT2_##BAND_ID,\
    en_NV_Item_W_RX_DCOFFSET_##BAND_ID,\
    en_NV_Item_W_RX_AGC_GAIN_##BAND_ID,\
    en_NV_Item_W_RX_AGC_FREQ_COMP_AT1_##BAND_ID,\
    en_NV_Item_W_RX_AGC_FREQ_COMP_AT2_##BAND_ID,\
    en_NV_Item_W_TX_IQ_MISMATCH_##BAND_ID,\
    en_NV_Item_W_TX_APC_TEMP_##BAND_ID,\
    en_NV_Item_W_TX_PA_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_ATTEN_MID_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_ATTEN_LOW_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_STATE_INDEX_MID_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_STATE_INDEX_LOW_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_APC_RFIC_CTRL_MID_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_APC_RFIC_CTRL_LOW_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_ATTEN_MID_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_ATTEN_LOW_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_STATE_INDEX_MID_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_STATE_INDEX_LOW_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_APC_RFIC_CTRL_MID_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_APC_RFIC_CTRL_LOW_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_ATTEN_HIGH_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_STATE_INDEX_HIGH_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_APC_RFIC_CTRL_HIGH_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_APC_DBB_ATTEN_HIGH_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_APC_DBB_ATTEN_MID_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_APC_DBB_ATTEN_LOW_GAIN_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_ATTEN_HIGH_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_RF_GAIN_STATE_INDEX_HIGH_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_APC_RFIC_CTRL_HIGH_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_APC_DBB_ATTEN_HIGH_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_APC_DBB_ATTEN_MID_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_APC_DBB_ATTEN_LOW_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_HDET_VGA_##BAND_ID,\
    en_NV_Item_W_TX_HDET_DCOFFSET_##BAND_ID,\
    en_NV_Item_W_TX_HDET_POWER_##BAND_ID,\
    en_NV_Item_W_TX_HDET_HKADC_##BAND_ID,\
    en_NV_Item_W_TX_HDET_FREQ_COMP_##BAND_ID,\
    en_NV_Item_W_TX_MID_GAIN_FREQ_COMP_##BAND_ID,\
    en_NV_Item_W_TX_LOW_GAIN_FREQ_COMP_##BAND_ID,\
    en_NV_Item_W_TX_HIGH_GAIN_FREQ_COMP_##BAND_ID,\
    en_NV_Item_W_TX_APT_COMP_##BAND_ID,\
    en_NV_Item_W_TX_PA_GAIN_VOICE_MODE_##BAND_ID,\
    en_NV_Item_W_TX_PA_APT_TABLE_##BAND_ID,\
    en_NV_Item_W_ET_TIME_DELAY_##BAND_ID,\
    en_NV_Item_W_ET_COEFF_##BAND_ID,\
    en_NV_Item_W_ET_PWR_VCC_LUT_##BAND_ID,\
    en_NV_Item_W_ET_DELAY_FREQ_COMP_##BAND_ID,\
    en_NV_Item_W_DPD_LUT_##BAND_ID,\
    en_NV_Item_W_RX_AGC_FREQ_COMP_MAIN_##BAND_ID,\
    en_NV_Item_W_RX_AGC_FREQ_COMP_DIV_##BAND_ID
/* Manufacture ID need to restore */
unsigned short  g_ausNvResumeManufactureIdList[] =
{
    en_NV_Item_USB_Enum_Status,

    en_NV_Item_IMEI,
    en_NV_Auth_Code_ID,
    en_NV_Auth_Num_ID,
    en_NV_Item_Serial_Num,
    en_NV_Item_LiveTime,
    en_NV_Item_ManufactureInfo,
    en_NV_Item_BATT_TEMP_SENSOR_TABLE,
    en_NV_Item_Factory_Info,
    en_NV_Item_BODY_SAR_PARA,

    en_NV_Item_BATTERY_ADC,
    en_NV_Item_WIFI_KEY,
    en_NV_Item_WIFI_MAC_ADDR,
    en_NV_Item_PRI_VERSION,
    en_NV_Item_WIFI_STATUS_SSID,
    en_NV_Item_WEB_ADMIN_PASSWORD,
    en_NV_Item_DSFLOW_REPORT,
    en_NV_Item_MULTI_WIFI_KEY,
    en_NV_Item_MULTI_WIFI_STATUS_SSID,

    en_NV_Item_XO_INIT_FREQUENCY,
    en_NV_Item_XO_DYNAMIC_FREQUENCY,
    en_NV_Item_DCXO_C_TRIM,
    en_NV_Item_DCXO_FREQ_VS_TEMP_ARRAY,
    en_NV_Item_XO_INIT_LOW_TEMP,
    en_NV_Item_TCXO_AFC_SLOPE,
    en_NV_Item_DCXO_C2_FIX,
    en_NV_Item_DCXO_SAMPLE_ARRAY_ALL_MODE,
    en_NV_Item_XO_SELF_CAL_DONE,
    en_NV_Item_XO_TEMP_COMP_CAL_RESULT,
    en_NV_Item_DCXO_TEMP_COMP,

    en_NV_Item_VSIM_HVSDH_INFO,


    /* Gsm 850 */
    en_NV_Item_GSM850_RX_DCOFFSET,
    en_NV_Item_GSM850_RX_AGC_GAIN,
    en_NV_Item_GSM850_TX_HD3_HD5,
    en_NV_Item_GSM850_RX_AGC_FREQ_COMP,
    en_NV_Item_EDGE850_TX_PA_GAIN,
    en_NV_Item_GSM850_RX_IQ_MISMATCH,

    en_NV_Item_GSM850_TX_IQ_MISMATCH,
    en_NV_Item_GSM850_TX_POWER,
    en_NV_Item_GSM850_TX_DAC,
    en_NV_Item_EDGE850_TX_RF_GAIN_ATTEN,
    en_NV_Item_EDGE850_TX_APC_CAL_TEMP,
    en_NV_Item_EDGE850_TX_APC_DBB_ATTEN,
    en_NV_Item_EDGE850_TX_RF_GAIN_STATE_INDEX,
    en_NV_Item_EDGE850_PA_VBIAS_COMP,
    en_NV_Item_GSM850_PA_PRECHG_VOLT,
    en_NV_Item_GSM850_LINEAR_PA_GAIN,
    en_NV_Item_EDGE850_LINEAR_PA_GAIN,
    en_NV_Item_GSM850_TX_LINEAR_APC_CAL_TEMP,
    en_NV_Item_GSM850_TX_RF_GAIN_ATTEN,
    en_NV_Item_GSM850_LINEAR_DBB_ATTEN,
    en_NV_Item_GSM850_TX_LINEAR_RFIC_GAIN_CTRL,
    en_NV_Item_EDGE850_TX_APC_RFIC_CTRL,
    en_NV_Item_GSM850_TX_GAIN0_FREQ_COMP,
    en_NV_Item_GSM850_TX_GAIN1_FREQ_COMP,
    en_NV_Item_GSM850_TX_GAIN2_FREQ_COMP,
    en_NV_Item_GSM850_TX_GAIN3_FREQ_COMP,
    en_NV_Item_EDGE850_TX_GAIN0_FREQ_COMP,
    en_NV_Item_EDGE850_TX_GAIN1_FREQ_COMP,
    en_NV_Item_EDGE850_TX_GAIN2_FREQ_COMP,
    en_NV_Item_EDGE850_TX_GAIN3_FREQ_COMP,

    /* Gsm 900 */
    en_NV_Item_GSM900_RX_DCOFFSET,
    en_NV_Item_GSM900_RX_AGC_GAIN,
    en_NV_Item_GSM900_TX_HD3_HD5,
    en_NV_Item_GSM900_RX_AGC_FREQ_COMP,
    en_NV_Item_EDGE900_TX_PA_GAIN,
    en_NV_Item_GSM900_RX_IQ_MISMATCH,

    en_NV_Item_GSM900_TX_IQ_MISMATCH,
    en_NV_Item_GSM900_TX_POWER,
    en_NV_Item_GSM900_TX_DAC,
    en_NV_Item_EDGE900_TX_RF_GAIN_ATTEN,
    en_NV_Item_EDGE900_TX_APC_CAL_TEMP,
    en_NV_Item_EDGE900_TX_APC_DBB_ATTEN,
    en_NV_Item_EDGE900_TX_RF_GAIN_STATE_INDEX,
    en_NV_Item_EDGE900_PA_VBIAS_COMP,
    en_NV_Item_GSM900_PA_PRECHG_VOLT,
    en_NV_Item_GSM900_LINEAR_PA_GAIN,
    en_NV_Item_EDGE900_LINEAR_PA_GAIN,
    en_NV_Item_GSM900_TX_LINEAR_APC_CAL_TEMP,
    en_NV_Item_GSM900_TX_RF_GAIN_ATTEN,
    en_NV_Item_GSM900_LINEAR_DBB_ATTEN,
    en_NV_Item_GSM900_TX_LINEAR_RFIC_GAIN_CTRL,
    en_NV_Item_EDGE900_TX_APC_RFIC_CTRL,
    en_NV_Item_GSM900_TX_GAIN0_FREQ_COMP,
    en_NV_Item_GSM900_TX_GAIN1_FREQ_COMP,
    en_NV_Item_GSM900_TX_GAIN2_FREQ_COMP,
    en_NV_Item_GSM900_TX_GAIN3_FREQ_COMP,
    en_NV_Item_EDGE900_TX_GAIN0_FREQ_COMP,
    en_NV_Item_EDGE900_TX_GAIN1_FREQ_COMP,
    en_NV_Item_EDGE900_TX_GAIN2_FREQ_COMP,
    en_NV_Item_EDGE900_TX_GAIN3_FREQ_COMP,

    /* Gsm 1800 */
    en_NV_Item_DCS1800_RX_DCOFFSET,
    en_NV_Item_DCS1800_RX_AGC_GAIN,
    en_NV_Item_DCS1800_TX_HD3_HD5,
    en_NV_Item_DCS1800_RX_AGC_FREQ_COMP,
    en_NV_Item_EDGE1800_TX_PA_GAIN,
    en_NV_Item_DCS1800_RX_IQ_MISMATCH,

    en_NV_Item_DCS1800_TX_IQ_MISMATCH,
    en_NV_Item_DCS1800_TX_POWER,
    en_NV_Item_DCS1800_TX_DAC,
    en_NV_Item_EDGE1800_TX_RF_GAIN_ATTEN,
    en_NV_Item_EDGE1800_TX_APC_CAL_TEMP,
    en_NV_Item_EDGE1800_TX_APC_DBB_ATTEN,
    en_NV_Item_EDGE1800_TX_RF_GAIN_STATE_INDEX,
    en_NV_Item_EDGE1800_PA_VBIAS_COMP,
    en_NV_Item_DCS1800_PA_PRECHG_VOLT,
    en_NV_Item_DCS1800_LINEAR_PA_GAIN,
    en_NV_Item_EDGE1800_LINEAR_PA_GAIN,
    en_NV_Item_DCS1800_TX_LINEAR_APC_CAL_TEMP,
    en_NV_Item_DCS1800_TX_RF_GAIN_ATTEN,
    en_NV_Item_DCS1800_LINEAR_DBB_ATTEN,
    en_NV_Item_DCS1800_TX_LINEAR_RFIC_GAIN_CTRL,
    en_NV_Item_EDGE1800_TX_APC_RFIC_CTRL,
    en_NV_Item_DCS1800_TX_GAIN0_FREQ_COMP,
    en_NV_Item_DCS1800_TX_GAIN1_FREQ_COMP,
    en_NV_Item_DCS1800_TX_GAIN2_FREQ_COMP,
    en_NV_Item_DCS1800_TX_GAIN3_FREQ_COMP,
    en_NV_Item_EDGE1800_TX_GAIN0_FREQ_COMP,
    en_NV_Item_EDGE1800_TX_GAIN1_FREQ_COMP,
    en_NV_Item_EDGE1800_TX_GAIN2_FREQ_COMP,
    en_NV_Item_EDGE1800_TX_GAIN3_FREQ_COMP,

    /* Gsm 1900 */
    en_NV_Item_PCS1900_RX_DCOFFSET,
    en_NV_Item_PCS1900_RX_AGC_GAIN,
    en_NV_Item_PCS1900_TX_HD3_HD5,
    en_NV_Item_PCS1900_RX_AGC_FREQ_COMP,
    en_NV_Item_EDGE1900_TX_PA_GAIN,
    en_NV_Item_PCS1900_RX_IQ_MISMATCH,

    en_NV_Item_PCS1900_TX_IQ_MISMATCH,
    en_NV_Item_PCS1900_TX_POWER,
    en_NV_Item_PCS1900_TX_DAC,
    en_NV_Item_EDGE1900_TX_RF_GAIN_ATTEN,
    en_NV_Item_EDGE1900_TX_APC_CAL_TEMP,
    en_NV_Item_EDGE1900_TX_APC_DBB_ATTEN,
    en_NV_Item_EDGE1900_TX_RF_GAIN_STATE_INDEX,
    en_NV_Item_EDGE1900_PA_VBIAS_COMP,
    en_NV_Item_PCS1900_PA_PRECHG_VOLT,
    en_NV_Item_PCS1900_LINEAR_PA_GAIN,
    en_NV_Item_EDGE1900_LINEAR_PA_GAIN,
    en_NV_Item_PCS1900_TX_LINEAR_APC_CAL_TEMP,
    en_NV_Item_PCS1900_TX_RF_GAIN_ATTEN,
    en_NV_Item_PCS1900_LINEAR_DBB_ATTEN,
    en_NV_Item_PCS1900_TX_LINEAR_RFIC_GAIN_CTRL,
    en_NV_Item_EDGE1900_TX_APC_RFIC_CTRL,
    en_NV_Item_PCS1900_TX_GAIN0_FREQ_COMP,
    en_NV_Item_PCS1900_TX_GAIN1_FREQ_COMP,
    en_NV_Item_PCS1900_TX_GAIN2_FREQ_COMP,
    en_NV_Item_PCS1900_TX_GAIN3_FREQ_COMP,
    en_NV_Item_EDGE1900_TX_GAIN0_FREQ_COMP,
    en_NV_Item_EDGE1900_TX_GAIN1_FREQ_COMP,
    en_NV_Item_EDGE1900_TX_GAIN2_FREQ_COMP,
    en_NV_Item_EDGE1900_TX_GAIN3_FREQ_COMP,

    /* CMCC 1800 */
    en_NV_Item_CMCC1800_RX_DCOFFSET,
    en_NV_Item_CMCC1800_RX_AGC_GAIN,
    en_NV_Item_CMCC1800_RX_AGC_FREQ_COMP,
    en_NV_Item_CMCC1800_RX_IQ_MISMATCH,

    BAND_NV_RESUME_WCDMA(B1),
    BAND_NV_RESUME_WCDMA(B2),
    BAND_NV_RESUME_WCDMA(B3),
    BAND_NV_RESUME_WCDMA(B4),
    BAND_NV_RESUME_WCDMA(B5),
    BAND_NV_RESUME_WCDMA(B6),
    BAND_NV_RESUME_WCDMA(B8),
    BAND_NV_RESUME_WCDMA(B9),
    BAND_NV_RESUME_WCDMA(B11),
    BAND_NV_RESUME_WCDMA(B19),

    /* CDMA nv项， */
    en_NV_Item_CDMA_RX_IP2_CAL_AT1_BC0,
    en_NV_Item_CDMA_RX_IP2_CAL_AT2_BC0,
    en_NV_Item_CDMA_RX_DCOFFSET_BC0,
    en_NV_Item_CDMA_RX_AGC_GAIN_BC0,
    en_NV_Item_CDMA_RX_AGC_FREQ_COMP_AT1_BC0,
    en_NV_Item_CDMA_RX_AGC_FREQ_COMP_AT2_BC0,
    en_NV_Item_CDMA_TX_IQ_MISMATCH_BC0,
    en_NV_Item_CDMA_TX_APC_TEMP_BC0,
    en_NV_Item_CDMA_TX_PA_GAIN_BC0,
    en_NV_Item_CDMA_TX_RF_GAIN_STATE_INDEX_MID_GAIN_BC0,
    en_NV_Item_CDMA_TX_RF_GAIN_STATE_INDEX_LOW_GAIN_BC0,
    en_NV_Item_CDMA_TX_APC_RFIC_CTRL_MID_GAIN_BC0,
    en_NV_Item_CDMA_TX_APC_RFIC_CTRL_LOW_GAIN_BC0,
    en_NV_Item_CDMA_TX_RF_GAIN_STATE_INDEX_HIGH_GAIN_BC0,
    en_NV_Item_CDMA_TX_APC_RFIC_CTRL_HIGH_GAIN_BC0,
    en_NV_Item_CDMA_TX_APC_DBB_ATTEN_HIGH_GAIN_BC0,
    en_NV_Item_CDMA_TX_APC_DBB_ATTEN_MID_GAIN_BC0,
    en_NV_Item_CDMA_TX_APC_DBB_ATTEN_LOW_GAIN_BC0,
    en_NV_Item_CDMA_TX_HDET_VGA_BC0,
    en_NV_Item_CDMA_TX_HDET_DCOFFSET_BC0,
    en_NV_Item_CDMA_TX_HDET_POWER_BC0,
    en_NV_Item_CDMA_TX_HDET_HKADC_BC0,
    en_NV_Item_CDMA_TX_HDET_FREQ_COMP_BC0,
    en_NV_Item_CDMA_TX_MID_GAIN_FREQ_COMP_BC0,
    en_NV_Item_CDMA_TX_LOW_GAIN_FREQ_COMP_BC0,

    en_NV_Item_CDMA_TX_HIGH_GAIN_FREQ_COMP_BC0,
    en_NV_Item_CDMA_TX_APT_COMP_BC0,
    en_NV_Item_CDMA_TX_PA_APT_TABLE_BC0,
    en_NV_Item_CDMA_RX_AGC_FREQ_COMP_MAIN_BC0,
    en_NV_Item_CDMA_RX_AGC_FREQ_COMP_DIV_BC0,


    /* for boston et */
    en_NV_Item_CDMA_ET_LUT_POLY_COFF_BC0,
    en_NV_Item_CDMA_ET_TIME_DELAY_BC0,
    en_NV_Item_CDMA_ET_TIME_DELAY_TEMP_COMP_BC0,
    en_NV_Item_CDMA_ET_TIME_DELAY_FREQ_COMP_BC0,

    en_NV_Item_CDMA_ET_COEFF_BC0,


    en_NV_Item_PESN                                 /*0x4002*/,
    en_NV_Item_MEID                                 /*0x4003*/,

    en_NV_Item_CDMA_1X_CUSTOMIZE_PREF_CHANNELS_INFO                /*2079*/,
    en_NV_Item_CDMA_HRPD_CUSTOMIZE_PREF_CHANNELS_INFO              /*2080*/,




    EN_NV_ID_LTE_TCXO_INIT_FREQ                    /*0xe900*/,
//#if (defined(FEATURE_TLPHY_TCXO_OVER_TEMP_PROTECT))
    EN_NV_ID_TCXO_DYNAMIC_CONFIG_PARA              /*0xD3E2*/,
//#endif
    //EN_NV_ID_ANT_MODEM_LOSS_B20                    /*0xeb00*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B20        /*0xeb0d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B20        /*0xeb0e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B20      /*0xeb0f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B20      /*0xeb10*/,
    EN_NV_ID_LTE_IP2_CAL_B20                       /*0xeb11*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B20             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B20       /*0xeb16*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B20        /*0xeb17*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B20        /*0xeb18*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B20            ,
        /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B20,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B20,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B20 ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B20               ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B20,
   EN_NV_ID_LTE_TX_PD_AUTO_CAL_B20,
   EN_NV_ID_LTE_TX_PD_PWR_TABLE_B20,
   EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B20,
   EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B20,
   /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B20,
   EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B20,
   EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B20,
   EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B20,*/
   /* Add For DPD */
   EN_NV_ID_DPD_FAC_RESULT_B20,
   EN_NV_ID_DPD_FAC_LUT_00_B20,
   EN_NV_ID_DPD_FAC_LUT_01_B20,
   EN_NV_ID_DPD_FAC_LUT_02_B20,
   EN_NV_ID_DPD_FAC_LUT_03_B20,
   /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B20,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B20,


    /*band40*/
    //EN_NV_ID_ANT_MODEM_LOSS_B40                     /*0xeb40*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B40         /*0xeb4D*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B40         /*0xeb4E*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B40       /*0xeb4F*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B40       /*0xeb50*/,
    EN_NV_ID_LTE_IP2_CAL_B40                        /*0xeb51*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B40             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B40        /*0xeb56*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B40         /*0xeb57*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B40,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B40,

        /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B40                     /*0xeb59*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B40                   /*0xeb5a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B40                   /*0xeb5b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B40                        /*0xeb5d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B40               /*0xeb5f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B40         /* 0xeb60*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B40            /* 0xeb61*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B40           /* 0xeb62*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B40        /* 0xeb63*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B40       /* 0xeb64*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B40       /* 0xeb66*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B40   /* 0xeb67*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B40   /* 0xeb68*/,
        /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B40,
    EN_NV_ID_DPD_FAC_LUT_00_B40,
    EN_NV_ID_DPD_FAC_LUT_01_B40,
    EN_NV_ID_DPD_FAC_LUT_02_B40,
    EN_NV_ID_DPD_FAC_LUT_03_B40,
    /* End Add For DPD */
EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B40,
EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B40,

//#if defined(LPHY_K3V6)

    /*bdan38*/
    //EN_NV_ID_ANT_MODEM_LOSS_B38                     ,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B38         ,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B38         ,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B38       ,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B38       ,
    EN_NV_ID_LTE_IP2_CAL_B38                        ,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B38             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B38        ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B38         ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B38         ,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B38   ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B38                    ,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B38                  ,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B38                  ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B38                       ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B38              ,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B38                ,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B38               ,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B38              ,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B38           ,
    /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B38         ,
    EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B38         ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B38       ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B38       ,*/
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B38,
    EN_NV_ID_DPD_FAC_LUT_00_B38,
    EN_NV_ID_DPD_FAC_LUT_01_B38,
    EN_NV_ID_DPD_FAC_LUT_02_B38,
    EN_NV_ID_DPD_FAC_LUT_03_B38,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B38,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B38,


    /*band41*/
    //EN_NV_ID_ANT_MODEM_LOSS_B41                     ,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B41         ,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B41         ,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B41       ,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B41       ,
    EN_NV_ID_LTE_IP2_CAL_B41                        ,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B41             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B41        ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B41         ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B41         ,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B41             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B41                    ,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B41                  ,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B41                  ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B41                       ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B41              ,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B41                ,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B41               ,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B41              ,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B41           ,
    /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B41    ,
    EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B41    ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B41  ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B41  ,*/

    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B41,
    EN_NV_ID_DPD_FAC_LUT_00_B41,
    EN_NV_ID_DPD_FAC_LUT_01_B41,
    EN_NV_ID_DPD_FAC_LUT_02_B41,
    EN_NV_ID_DPD_FAC_LUT_03_B41,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B41,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B41,

    /*band7*/
    //EN_NV_ID_ANT_MODEM_LOSS_B7                      ,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B7          ,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B7          ,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B7        ,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B7        ,
    EN_NV_ID_LTE_IP2_CAL_B7                         ,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B7             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B7         ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B7          ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B7          ,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B7             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B7                    ,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B7                  ,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B7                  ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B7                       ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B7              ,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B7                ,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B7               ,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B7              ,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B7           ,
    /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B7    ,
    EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B7    ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B7  ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B7  ,*/

    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B7,
    EN_NV_ID_DPD_FAC_LUT_00_B7,
    EN_NV_ID_DPD_FAC_LUT_01_B7,
    EN_NV_ID_DPD_FAC_LUT_02_B7,
    EN_NV_ID_DPD_FAC_LUT_03_B7,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B7,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B7,

    /*band3*/
    //EN_NV_ID_ANT_MODEM_LOSS_B3                      ,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B3          ,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B3          ,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B3        ,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B3        ,
    EN_NV_ID_LTE_IP2_CAL_B3                         ,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B3             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B3         ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B3          ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B3          ,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B3             ,
     /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B3                    ,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B3                  ,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B3                  ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B3                       ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B3              ,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B3                ,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B3               ,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B3              ,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B3           ,
    /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B3    ,
    EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B3    ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B3  ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B3  ,*/

    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B3,
    EN_NV_ID_DPD_FAC_LUT_00_B3,
    EN_NV_ID_DPD_FAC_LUT_01_B3,
    EN_NV_ID_DPD_FAC_LUT_02_B3,
    EN_NV_ID_DPD_FAC_LUT_03_B3,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B3,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B3,

    /*band1*/
    //EN_NV_ID_ANT_MODEM_LOSS_B1                      ,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B1          ,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B1          ,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B1        ,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B1        ,
    EN_NV_ID_LTE_IP2_CAL_B1                         ,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B1             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B1         ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B1          ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B1          ,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B1            ,
     /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B1                    ,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B1                  ,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B1                  ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B1                      ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B1              ,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B1                ,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B1               ,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B1              ,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B1           ,
    /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B1    ,
    EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B1    ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B1  ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B1  ,*/
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B1,
    EN_NV_ID_DPD_FAC_LUT_00_B1,
    EN_NV_ID_DPD_FAC_LUT_01_B1,
    EN_NV_ID_DPD_FAC_LUT_02_B1,
    EN_NV_ID_DPD_FAC_LUT_03_B1,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B1,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B1,


    /*band5*/
    //EN_NV_ID_ANT_MODEM_LOSS_B5                    /* 0xed80*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B5          /*0xed8D*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B5          /*0xed8E*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B5        /*0xed8F*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B5        /*0xed90*/,
    EN_NV_ID_LTE_IP2_CAL_B5                         /*0xed91*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B5             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B5         /*0xed96*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B5          /*0xed97*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B5          /*0xed98*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B5             ,

         /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B5                      /* 0xed99*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B5                    /* 0xed9a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B5                    /* 0xed9b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B5                        /* 0xed9d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B5                /* 0xed9f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B5                  /* 0xedA5*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B5                 /* 0xedA6*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B5                /* 0xedA7*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B5             /* 0xedA8*/,
    /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B5        ,
    EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B5      ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B5    ,
    EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B5    ,*/
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B5,
    EN_NV_ID_DPD_FAC_LUT_00_B5,
    EN_NV_ID_DPD_FAC_LUT_01_B5,
    EN_NV_ID_DPD_FAC_LUT_02_B5,
    EN_NV_ID_DPD_FAC_LUT_03_B5,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B5,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B5,


    /*band8*/
    //EN_NV_ID_ANT_MODEM_LOSS_B8                      /*0xedc0*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B8          /*0xedcD*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B8          /*0xedcE*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B8        /*0xedcF*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B8        /*0xedd0*/,
    EN_NV_ID_LTE_IP2_CAL_B8                         /*0xedd1*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B8             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B8         /*0xedd6*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B8          /*0xedd7*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B8          /*0xedd8*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B8            ,

         /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B8                    /*0xedd9*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B8                  /*0xedda*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B8                  /*0xeddb*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B8                       /*0xeddd*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B8              /*0xeddf*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B8                /*0xede5*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B8               /*0xede6*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B8              /*0xede7*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B8           /*0xede8*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B8    /*0xede9*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B8    /*0xedea*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B8  /*0xedeb*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B8  /*0xedec*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B8,
    EN_NV_ID_DPD_FAC_LUT_00_B8,
    EN_NV_ID_DPD_FAC_LUT_01_B8,
    EN_NV_ID_DPD_FAC_LUT_02_B8,
    EN_NV_ID_DPD_FAC_LUT_03_B8,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B8,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B8,


    /*band19*/
    //EN_NV_ID_ANT_MODEM_LOSS_B19                     ,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B19         ,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B19         ,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B19       ,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B19       ,
    EN_NV_ID_LTE_IP2_CAL_B19                        ,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B19             ,

    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B19        ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B19         ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B19         ,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B19              ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B19                     ,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B19                   ,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B19                   ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B19                       ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B19               ,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B19                  ,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B19                 ,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B19                ,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B19             ,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B19      ,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B19      ,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B19    ,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B19    ,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B19,
    EN_NV_ID_DPD_FAC_LUT_00_B19,
    EN_NV_ID_DPD_FAC_LUT_01_B19,
    EN_NV_ID_DPD_FAC_LUT_02_B19,
    EN_NV_ID_DPD_FAC_LUT_03_B19,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B19,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B19,

    /*band21*/
    //EN_NV_ID_ANT_MODEM_LOSS_B21                     ,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B21         ,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B21         ,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B21       ,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B21       ,
    EN_NV_ID_LTE_IP2_CAL_B21                        ,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B21             ,

    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B21        ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B21         ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B21         ,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B21             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B21                     ,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B21                   ,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B21                   ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B21                        ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B21               ,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B21                  ,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B21                 ,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B21                ,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B21             ,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B21      ,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B21      ,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B21    ,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B21    ,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B21,
    EN_NV_ID_DPD_FAC_LUT_00_B21,
    EN_NV_ID_DPD_FAC_LUT_01_B21,
    EN_NV_ID_DPD_FAC_LUT_02_B21,
    EN_NV_ID_DPD_FAC_LUT_03_B21,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B21,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B21,


    /*band2*/
    //EN_NV_ID_ANT_MODEM_LOSS_B2                      /*0xEE80*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B2          /*0xEE8d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B2          /*0xEE8e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B2        /*0xEE8f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B2        /*0xEE90*/,
    EN_NV_ID_LTE_IP2_CAL_B2                         /*0xEE91*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B2             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B2         /*0xEE96*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B2          /*0xEE97*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B2          /*0xEE98*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B2             ,

        /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B2                     /*0xEE99*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B2                   /*0xEE9a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B2                   /*0xEE9b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B2                        /*0xEE9d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B2               /*0xEE9f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B2                 /*0xEEa0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B2                /*0xEEa1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B2               /*0xEEa2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B2            /*0xEEa3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B2       /*0xEEa4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B2       /*0xEEa5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B2   /*0xEEa6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B2   /*0xEEa7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B2,
    EN_NV_ID_DPD_FAC_LUT_00_B2,
    EN_NV_ID_DPD_FAC_LUT_01_B2,
    EN_NV_ID_DPD_FAC_LUT_02_B2,
    EN_NV_ID_DPD_FAC_LUT_03_B2,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B2,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B2,



    /*band4*/
    //EN_NV_ID_ANT_MODEM_LOSS_B4                     /* 0xEEc0*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B4          /*0xEEcd*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B4          /*0xEEce*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B4        /*0xEEcf*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B4        /*0xEEd0*/,
    EN_NV_ID_LTE_IP2_CAL_B4                         /*0xEEd1*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B4             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B4         /*0xEEd6*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B4          /*0xEEd7*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B4          /*0xEEd8*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B4             ,

     /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B4                      /*0xEEd9*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B4                    /*0xEEda*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B4                    /*0xEEdb*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B4                         /*0xEEdd*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B4                /*0xEEdf*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B4                  /*0xEEe0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B4                 /*0xEEe1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B4                /*0xEEe2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B4             /*0xEEe3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B4        /*0xEEe4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B4        /*0xEEe5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B4        /*0xEEe6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B4        /*0xEEe7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B4,
    EN_NV_ID_DPD_FAC_LUT_00_B4,
    EN_NV_ID_DPD_FAC_LUT_01_B4,
    EN_NV_ID_DPD_FAC_LUT_02_B4,
    EN_NV_ID_DPD_FAC_LUT_03_B4,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B4,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B4,


    /*band6*/

    //EN_NV_ID_ANT_MODEM_LOSS_B6                      /*0xEf00*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B6         /*0xEf0d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B6          /*0xEf0e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B6        /*0xEf0f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B6       /*0xEf10*/,
    EN_NV_ID_LTE_IP2_CAL_B6                         /*0xEf11*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B6             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B6         /*0xEf16*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B6         /*0xEf17*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B6          /*0xEf18*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B6             ,

     /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B6                     /*0xEf19*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B6                   /*0xEf1a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B6                   /*0xEf1b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B6                        /*0xEf1d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B6               /*0xEf1f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B6                 /*0xEf20*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B6                /*0xEf21*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B6               /*0xEf22*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B6            /*0xEf23*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B6       /*0xEf24*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B6       /*0xEf25*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B6   /*0xEf26*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B6   /*0xEf27*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B6,
    EN_NV_ID_DPD_FAC_LUT_00_B6,
    EN_NV_ID_DPD_FAC_LUT_01_B6,
    EN_NV_ID_DPD_FAC_LUT_02_B6,
    EN_NV_ID_DPD_FAC_LUT_03_B6,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B6,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B6,


    /*band9*/

    //EN_NV_ID_ANT_MODEM_LOSS_B9                      /*0xEf40*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B9          /*0xEf4d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B9         /*0xEf4e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B9        /*0xEf4f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B9        /*0xEf50*/,
    EN_NV_ID_LTE_IP2_CAL_B9                        /*0xEf51*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B9             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B9         /*0xEf56*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B9          /*0xEf57*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B9          /*0xEf58*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B9             ,

     /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B9                     /* 0xEf59*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B9                   /* 0xEf5a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B9                   /* 0xEf5b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B9                        /* 0xEf5d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B9               /* 0xEf5f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B9                 /* 0xEf60*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B9                /* 0xEf61*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B9               /* 0xEf62*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B9            /* 0xEf63*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B9       /* 0xEf64*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B9       /* 0xEf65*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B9   /* 0xEf66*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B9   /* 0xEf67*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B9,
    EN_NV_ID_DPD_FAC_LUT_00_B9,
    EN_NV_ID_DPD_FAC_LUT_01_B9,
    EN_NV_ID_DPD_FAC_LUT_02_B9,
    EN_NV_ID_DPD_FAC_LUT_03_B9,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B9,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B9,

    /*band10*/
    //EN_NV_ID_ANT_MODEM_LOSS_B10                     /*0xEf80*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B10         /*0xEf8d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B10         /*0xEf8e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B10       /*0xEf8f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B10       /*0xEf90*/,
    EN_NV_ID_LTE_IP2_CAL_B10                        /*0xEf91*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B10             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B10        /*0xEf96*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B10         /*0xEf97*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B10         /*0xEf98*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B10              ,

     /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B10                    /* 0xEf99*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B10                  /* 0xEf9a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B10                  /* 0xEf9b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B10                       /* 0xEf9d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B10              /* 0xEf9f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B10                /* 0xEfa0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B10               /* 0xEfa1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B10              /* 0xEfa2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B10           /* 0xEfa3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B10      /* 0xEfa4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B10      /* 0xEfa5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B10  /* 0xEfa6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B10  /* 0xEfa7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B10,
    EN_NV_ID_DPD_FAC_LUT_00_B10,
    EN_NV_ID_DPD_FAC_LUT_01_B10,
    EN_NV_ID_DPD_FAC_LUT_02_B10,
    EN_NV_ID_DPD_FAC_LUT_03_B10,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B10,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B10,


    /*band11*/
    //EN_NV_ID_ANT_MODEM_LOSS_B11                     /*0xEfc0*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B11         /*0xEfcd*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B11         /*0xEfce*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B11       /*0xEfcf*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B11       /*0xEfd0*/,
    EN_NV_ID_LTE_IP2_CAL_B11                        /*0xEfd1*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B11             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B11        /*0xEfd6*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B11         /*0xEfd7*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B11        /*0xEfd8*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B11              ,

      /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B11                    /*0xEfd9*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B11                  /*0xEfda*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B11                  /*0xEfdb*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B11                       /*0xEfdd*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B11              /*0xEfdf*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B11                /*0xEfe0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B11               /*0xEfe1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B11              /*0xEfe2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B11           /*0xEfe3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B11      /*0xEfe4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B11      /*0xEfe5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B11  /*0xEfe6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B11  /*0xEfe7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B11,
    EN_NV_ID_DPD_FAC_LUT_00_B11,
    EN_NV_ID_DPD_FAC_LUT_01_B11,
    EN_NV_ID_DPD_FAC_LUT_02_B11,
    EN_NV_ID_DPD_FAC_LUT_03_B11,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B11,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B11,

    /*band12*/
    //EN_NV_ID_ANT_MODEM_LOSS_B12                     /*0xf000*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B12        /*0xf00d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B12         /*0xf00e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B12       /*0xf00f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B12      /*0xf010*/,
    EN_NV_ID_LTE_IP2_CAL_B12                       /*0xf011*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B12             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B12        /*0xf016*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B12        /*0xf017*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B12        /*0xf018*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B12                      ,

    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B12                    /* 0xf019*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B12                  /* 0xf01a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B12                  /* 0xf01b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B12                       /* 0xf01d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B12              /* 0xf01f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B12                /* 0xf020*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B12               /* 0xf021*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B12              /* 0xf022*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B12           /* 0xf023*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B12      /* 0xf024*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B12      /* 0xf025*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B12  /* 0xf026*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B12  /* 0xf027*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B12,
    EN_NV_ID_DPD_FAC_LUT_00_B12,
    EN_NV_ID_DPD_FAC_LUT_01_B12,
    EN_NV_ID_DPD_FAC_LUT_02_B12,
    EN_NV_ID_DPD_FAC_LUT_03_B12,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B12,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B12,

    /*band13*/
    //EN_NV_ID_ANT_MODEM_LOSS_B13                    /*0xf040*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B13         /*0xf04d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B13        /*0xf04e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B13      /*0xf04f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B13       /*0xf050*/,
    EN_NV_ID_LTE_IP2_CAL_B13                        /*0xf051*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B13             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B13       /*0xf056*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B13         /*0xf057*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B13        /*0xf058*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B13                     ,

     /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B13                    /* 0xf059*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B13                  /* 0xf05a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B13                  /* 0xf05b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B13                       /* 0xf05d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B13              /* 0xf05f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B13            /* 0xf060*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B13           /* 0xf061*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B13          /* 0xf062*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B13       /* 0xf063*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B13    /* 0xf064*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B13    /* 0xf065*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B13  /* 0xf066*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B13  /* 0xf067*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B13,
    EN_NV_ID_DPD_FAC_LUT_00_B13,
    EN_NV_ID_DPD_FAC_LUT_01_B13,
    EN_NV_ID_DPD_FAC_LUT_02_B13,
    EN_NV_ID_DPD_FAC_LUT_03_B13,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B13,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B13,

    /*band14*/
    //EN_NV_ID_ANT_MODEM_LOSS_B14                    /*0xf080*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B14        /*0xf08d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B14        /*0xf08e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B14      /*0xf08f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B14      /*0xf090*/,
    EN_NV_ID_LTE_IP2_CAL_B14                       /*0xf091*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B14             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B14        /*0xf096*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B14         /*0xf097*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B14        /*0xf098*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B14                     ,

    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B14                    /* 0xf099*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B14                  /* 0xf09a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B14                  /* 0xf09b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B14                       /* 0xf09d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B14              /* 0xf09f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B14            /* 0xf0a0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B14           /* 0xf0a1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B14          /* 0xf0a2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B14       /* 0xf0a3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B14    /* 0xf0a4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B14    /* 0xf0a5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B14  /* 0xf0a6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B14  /* 0xf0a7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B14,
    EN_NV_ID_DPD_FAC_LUT_00_B14,
    EN_NV_ID_DPD_FAC_LUT_01_B14,
    EN_NV_ID_DPD_FAC_LUT_02_B14,
    EN_NV_ID_DPD_FAC_LUT_03_B14,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B14,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B14,


    /*band17*/
    //EN_NV_ID_ANT_MODEM_LOSS_B17                    /*0xf0c0*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B17         /*0xf0cd*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B17         /*0xf0ce*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B17       /*0xf0cf*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B17       /*0xf0d0*/,
    EN_NV_ID_LTE_IP2_CAL_B17                        /*0xf0d1*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B17             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B17        /*0xf0d6*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B17         /*0xf0d7*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B17         /*0xf0d8*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B17                   ,

    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B17                    /* 0xf0d9*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B17                  /* 0xf0da*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B17                  /* 0xf0db*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B17                      /* 0xf0dd*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B17              /* 0xf0df*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B17            /* 0xf0e0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B17           /* 0xf0e1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B17          /* 0xf0e2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B17       /* 0xf0e3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B17    /* 0xf0e4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B17    /* 0xf0e5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B17  /* 0xf0e6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B17  /* 0xf0e7*/,
   /* Add For DPD */
   EN_NV_ID_DPD_FAC_RESULT_B17,
   EN_NV_ID_DPD_FAC_LUT_00_B17,
   EN_NV_ID_DPD_FAC_LUT_01_B17,
   EN_NV_ID_DPD_FAC_LUT_02_B17,
   EN_NV_ID_DPD_FAC_LUT_03_B17,
   /* End Add For DPD */
   EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B17,
   EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B17,

   /*band18*/
    //EN_NV_ID_ANT_MODEM_LOSS_B18                     /* 0xf100*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B18         /* 0xf10d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B18         /* 0xf10e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B18       /* 0xf10f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B18       /* 0xf110*/,
    EN_NV_ID_LTE_IP2_CAL_B18                        /* 0xf111*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B18             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B18        /* 0xf116*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B18         /* 0xf117*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B18         /* 0xf118*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B18                     ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B18                     /* 0xf119*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B18                   /* 0xf11a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B18                   /* 0xf11b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B18                        /* 0xf11d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B18               /* 0xf11f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B18             /* 0xf125*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B18            /* 0xf126*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B18           /* 0xf127*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B18        /* 0xf128*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B18     /* 0xf129*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B18     /* 0xf12a*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B18   /* 0xf12b*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B18   /* 0xf12c*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B18,
    EN_NV_ID_DPD_FAC_LUT_00_B18,
    EN_NV_ID_DPD_FAC_LUT_01_B18,
    EN_NV_ID_DPD_FAC_LUT_02_B18,
    EN_NV_ID_DPD_FAC_LUT_03_B18,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B18,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B18,

    /*band22*/

    //EN_NV_ID_ANT_MODEM_LOSS_B22                     /* 0xf140*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B22         /* 0xf14d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B22         /* 0xf14e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B22       /* 0xf14f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B22       /* 0xf150*/,
    EN_NV_ID_LTE_IP2_CAL_B22                        /* 0xf151*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B22             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B22        /* 0xf156*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B22         /* 0xf157*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B22         /* 0xf158*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B22             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B22                    /* 0xf159*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B22                  /* 0xf15a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B22                  /* 0xf15b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B22                       /* 0xf15d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B22              /* 0xf15f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B22            /* 0xf160*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B22           /* 0xf161*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B22          /* 0xf162*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B22       /* 0xf163*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B22    /* 0xf164*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B22    /* 0xf165*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B22  /* 0xf166*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B22  /* 0xf167*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B22,
    EN_NV_ID_DPD_FAC_LUT_00_B22,
    EN_NV_ID_DPD_FAC_LUT_01_B22,
    EN_NV_ID_DPD_FAC_LUT_02_B22,
    EN_NV_ID_DPD_FAC_LUT_03_B22,
    /* End Add For DPD */
   EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B22,
   EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B22,

   /*band23*/
    //EN_NV_ID_ANT_MODEM_LOSS_B23                       /* 0xf180*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B23         /* 0xf18d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B23         /* 0xf18e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B23       /* 0xf18f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B23       /* 0xf190*/,
    EN_NV_ID_LTE_IP2_CAL_B23                        /* 0xf191*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B23             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B23        /* 0xf196*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B23         /* 0xf197*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B23         /* 0xf198*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B23             ,
    EN_NV_ID_IIP2_CAL_TABLE_B23                     /* 0xf199*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B23                   /* 0xf19a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B23                   /* 0xf19b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B23                        /* 0xf19d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B23               /* 0xf19f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B23             /* 0xf1a0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B23            /* 0xf1a1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B23           /* 0xf1a2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B23        /* 0xf1a3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B23     /* 0xf1a4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B23     /* 0xf1a5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B23   /* 0xf1a6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B23   /* 0xf1a7*/,

    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B23,
    EN_NV_ID_DPD_FAC_LUT_00_B23,
    EN_NV_ID_DPD_FAC_LUT_01_B23,
    EN_NV_ID_DPD_FAC_LUT_02_B23,
    EN_NV_ID_DPD_FAC_LUT_03_B23,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B23,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B23,




    //EN_NV_ID_ANT_MODEM_LOSS_B24                     /* 0xf1c0*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B24         /* 0xf1cd*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B24         /* 0xf1ce*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B24       /* 0xf1cf*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B24       /* 0xf1d0*/,
    EN_NV_ID_LTE_IP2_CAL_B24                        /* 0xf1d1*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B24             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B24        /* 0xf1d6*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B24         /* 0xf1d7*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B24         /* 0xf1d8*/,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B24             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B24                    /* 0xf1d9*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B24                  /* 0xf1da*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B24                  /* 0xf1db*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B24                       /* 0xf1dd*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B24              /* 0xf1df*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B24            /* 0xf1e0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B24           /* 0xf1e1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B24          /* 0xf1e2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B24       /* 0xf1e3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B24    /* 0xf1e4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B24    /* 0xf1e5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B24  /* 0xf1e6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B24  /* 0xf1e7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B24,
    EN_NV_ID_DPD_FAC_LUT_00_B24,
    EN_NV_ID_DPD_FAC_LUT_01_B24,
    EN_NV_ID_DPD_FAC_LUT_02_B24,
    EN_NV_ID_DPD_FAC_LUT_03_B24,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B24,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B24,





    //EN_NV_ID_ANT_MODEM_LOSS_B25                     /* 0xf200*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B25         /* 0xf20d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B25         /* 0xf20e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B25       /* 0xf20f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B25       /* 0xf210*/,
    EN_NV_ID_LTE_IP2_CAL_B25                        /* 0xf211*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B25             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B25        /* 0xf216*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B25         /* 0xf217*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B25         /* 0xf218*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B25             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B25                    /* 0xf219*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B25                  /* 0xf21a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B25                  /* 0xf21b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B25                       /* 0xf21d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B25              /* 0xf21f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B25        /* 0xf220*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B25       /* 0xf221*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B25      /* 0xf222*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B25          /* 0xf223*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B25   /* 0xf224*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B25   /* 0xf225*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B25 /* 0xf226*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B25 /* 0xf227*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B25,
    EN_NV_ID_DPD_FAC_LUT_00_B25,
    EN_NV_ID_DPD_FAC_LUT_01_B25,
    EN_NV_ID_DPD_FAC_LUT_02_B25,
    EN_NV_ID_DPD_FAC_LUT_03_B25,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B25,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B25,





    //EN_NV_ID_ANT_MODEM_LOSS_B33                     /* 0xf240*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B33         /* 0xf24d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B33         /* 0xf24e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B33       /* 0xf24f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B33       /* 0xf250*/,
    EN_NV_ID_LTE_IP2_CAL_B33                        /* 0xf251*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B33             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B33        /* 0xf256*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B33         /* 0xf257*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B33         /* 0xf258*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B33             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B33                    /* 0xf259*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B33                  /* 0xf25a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B33                  /* 0xf25b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B33                      /* 0xf25d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B33              /* 0xf25f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B33        /* 0xf260*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B33               /* 0xf261*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B33              /* 0xf262*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B33           /* 0xf263*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B33    /* 0xf264*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B33    /* 0xf265*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B33  /* 0xf266*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B33  /* 0xf267*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B33,
    EN_NV_ID_DPD_FAC_LUT_00_B33,
    EN_NV_ID_DPD_FAC_LUT_01_B33,
    EN_NV_ID_DPD_FAC_LUT_02_B33,
    EN_NV_ID_DPD_FAC_LUT_03_B33,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B33,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B33,





    //EN_NV_ID_ANT_MODEM_LOSS_B34                     /* 0xf280*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B34         /* 0xf28d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B34         /* 0xf28e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B34       /* 0xf28f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B34       /* 0xf290*/,
    EN_NV_ID_LTE_IP2_CAL_B34                        /* 0xf291*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B34             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B34        /* 0xf296*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B34         /* 0xf297*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B34         /* 0xf298*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B34             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B34                    /* 0xf299*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B34                  /* 0xf29a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B34                  /* 0xf29b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B34                       /* 0xf29d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B34              /* 0xf29f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B34                /* 0xf2a0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B34               /* 0xf2a1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B34              /* 0xf2a2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B34           /*0xf2a3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B34   /*0xf2a4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B34   /*0xf2a5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B34 /* 0xf2a6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B34 /* 0xf2a7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B34,
    EN_NV_ID_DPD_FAC_LUT_00_B34,
    EN_NV_ID_DPD_FAC_LUT_01_B34,
    EN_NV_ID_DPD_FAC_LUT_02_B34,
    EN_NV_ID_DPD_FAC_LUT_03_B34,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B34,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B34,





    //EN_NV_ID_ANT_MODEM_LOSS_B35                     /* 0xf2c0*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B35         /* 0xf2cd*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B35         /* 0xf2ce*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B35       /* 0xf2cf*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B35       /* 0xf2d0*/,
    EN_NV_ID_LTE_IP2_CAL_B35                        /* 0xf2d1*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B35             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B35        /* 0xf2d6*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B35         /* 0xf2d7*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B35         /* 0xf2d8*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B35             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B35                    /* 0xf2d9*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B35                  /* 0xf2da*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B35                  /* 0xf2db*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B35                       /* 0xf2dd*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B35              /* 0xf2df*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B35                /* 0xf2e0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B35               /* 0xf2e1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B35              /* 0xf2e2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B35           /* 0xf2e3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B35    /* 0xf2e4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B35    /* 0xf2e5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B35  /* 0xf2e6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B35  /* 0xf2e7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B35,
    EN_NV_ID_DPD_FAC_LUT_00_B35,
    EN_NV_ID_DPD_FAC_LUT_01_B35,
    EN_NV_ID_DPD_FAC_LUT_02_B35,
    /* End Add For DPD */


    //EN_NV_ID_ANT_MODEM_LOSS_B36                     /* 0xf300*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B36         /* 0xf30d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B36         /* 0xf30e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B36       /* 0xf30f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B36       /* 0xf310*/,
    EN_NV_ID_LTE_IP2_CAL_B36                        /* 0xf311*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B36             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B36        /* 0xf316*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B36         /* 0xf317*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B36         /* 0xf318*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B36             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B36                    /* 0xf319*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B36                  /* 0xf31a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B36                  /* 0xf31b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B36                       /* 0xf31d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B36              /* 0xf31f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B36                /* 0xf320*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B36               /* 0xf321*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B36              /* 0xf322*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B36           /* 0xf323*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B36   /*0xf324*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B36   /*0xf325*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B36 /* 0xf326*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B36 /* 0xf327*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B36,
    EN_NV_ID_DPD_FAC_LUT_00_B36,
    EN_NV_ID_DPD_FAC_LUT_01_B36,
    EN_NV_ID_DPD_FAC_LUT_02_B36,
    EN_NV_ID_DPD_FAC_LUT_03_B36,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B36,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B36,





    //EN_NV_ID_ANT_MODEM_LOSS_B37                     /* 0xf340*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B37         /* 0xf34d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B37         /* 0xf34e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B37       /* 0xf34f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B37       /* 0xf350*/,
    EN_NV_ID_LTE_IP2_CAL_B37                        /* 0xf351*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B37             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B37        /* 0xf356*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B37         /* 0xf357*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B37         /* 0xf358*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B37             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B37                    /* 0xf359*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B37                  /* 0xf35a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B37                  /* 0xf35b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B37                       /* 0xf35d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B37              /* 0xf35f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B37                /* 0xf360*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B37               /* 0xf361*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B37              /* 0xf362*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B37           /*0xf363*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B37   /*0xf364*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B37   /*0xf365*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B37 /* 0xf366*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B37 /* 0xf367*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B37,
    EN_NV_ID_DPD_FAC_LUT_00_B37,
    EN_NV_ID_DPD_FAC_LUT_01_B37,
    EN_NV_ID_DPD_FAC_LUT_02_B37,
    EN_NV_ID_DPD_FAC_LUT_03_B37,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B37,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B37,






    //EN_NV_ID_ANT_MODEM_LOSS_B42                     /* 0xf380*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B42         /* 0xf38d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B42         /* 0xf38e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B42       /* 0xf38f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B42       /* 0xf390*/,
    EN_NV_ID_LTE_IP2_CAL_B42                        /* 0xf391*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B42             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B42        /* 0xf396*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B42         /* 0xf397*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B42         /* 0xf398*/,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B42             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B42                    /* 0xf399*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B42                  /* 0xf39a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B42                  /* 0xf39b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B42                       /* 0xf39d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B42              /* 0xf39f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B42                /* 0xf3a0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B42               /* 0xf3a1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B42              /* 0xf3a2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B42           /* 0xf3a3*/,
    ///EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B42   /*0xf3a4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B42   /*0xf3a5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B42 /* 0xf3a6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B42 /* 0xf3a7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B42,
    EN_NV_ID_DPD_FAC_LUT_00_B42,
    EN_NV_ID_DPD_FAC_LUT_01_B42,
    EN_NV_ID_DPD_FAC_LUT_02_B42,
    EN_NV_ID_DPD_FAC_LUT_03_B42,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B42,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B42,





    //EN_NV_ID_ANT_MODEM_LOSS_B43                     /* 0xf3c0*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B43         /* 0xf3cd*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B43         /* 0xf3ce*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B43       /* 0xf3cf*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B43       /* 0xf3d0*/,
    EN_NV_ID_LTE_IP2_CAL_B43                        /* 0xf3d1*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B43             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B43        /* 0xf3d6*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B43         /* 0xf3d7*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B43         /* 0xf3d8*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B43             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B43                    /* 0xf3d9*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B43                  /* 0xf3da*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B43                  /* 0xf3db*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B43                       /* 0xf3dd*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B43              /* 0xf3df*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B43                /* 0xf3e0*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B43               /* 0xf3e1*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B43              /* 0xf3e2*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B43           /* 0xf3e3*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B43   /*0xf3e4*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B43   /*0xf3e5*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B43 /* 0xf3e6*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B43 /* 0xf3e7*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B43,
    EN_NV_ID_DPD_FAC_LUT_00_B43,
    EN_NV_ID_DPD_FAC_LUT_01_B43,
    EN_NV_ID_DPD_FAC_LUT_02_B43,
    EN_NV_ID_DPD_FAC_LUT_03_B43,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B43,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B43,









    /*band39*/
    //EN_NV_ID_ANT_MODEM_LOSS_B39                    /*0xf400*/,
   EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B39         /*0xf40d*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B39        /*0xf40e*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B39       /*0xf40f*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B39      /*0xf410*/,
    EN_NV_ID_LTE_IP2_CAL_B39                        /*0xf411*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B39             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B39        /*0xf416*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B39         /*0xf417*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B39         /*0xf418*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B39            ,

     /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B39                    /* 0xf419*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B39                  /* 0xf41a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B39                  /* 0xf41b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B39                       /* 0xf41d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B39              /* 0xf41f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B39                /* 0xef420*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B39               /* 0xef421*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B39              /* 0xef422*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B39           /* 0xef423*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B39     /* 0xef424*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B39     /* 0xef425*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B39  /* 0xef426*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B39  /* 0xef427*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B39,
    EN_NV_ID_DPD_FAC_LUT_00_B39,
    EN_NV_ID_DPD_FAC_LUT_01_B39,
    EN_NV_ID_DPD_FAC_LUT_02_B39,
    EN_NV_ID_DPD_FAC_LUT_03_B39,
    /* End Add For DPD */
EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B39,
EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B39,




/*BAND28 生产NV*/
    //EN_NV_ID_ANT_MODEM_LOSS_B28               /* 0xf440*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B28         /* 0xf44D*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B28         /* 0xf44E*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B28     /* 0xf44F*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B28     /* 0xf450*/,
    EN_NV_ID_LTE_IP2_CAL_B28                /* 0xf451*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B28             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B28        /* 0xf456*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B28         /* 0xf457*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B28         /* 0xf458*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B28            ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B28               /* 0xf459*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B28                 /* 0xf45a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B28                 /* 0xf45b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B28                 /* 0xf45d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B28             /* 0xf45f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B28               /* 0xf465*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B28              /* 0xf466*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B28             /* 0xf467*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B28          /* 0xf468*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B28     /* 0xf469*/,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B28     /* 0xf46a*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B28  /* 0xf46b*/,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B28  /* 0xf46c*/,

     /* Add For DPD */
     EN_NV_ID_DPD_FAC_RESULT_B28,
     EN_NV_ID_DPD_FAC_LUT_00_B28,
     EN_NV_ID_DPD_FAC_LUT_01_B28,
     EN_NV_ID_DPD_FAC_LUT_02_B28,
     EN_NV_ID_DPD_FAC_LUT_03_B28,
     /* End Add For DPD */
     EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B28,
     EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B28,

     /*band26*/
    //EN_NV_ID_ANT_MODEM_LOSS_B26                       ,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B26               ,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B26         ,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B26      ,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B26       ,
    EN_NV_ID_LTE_IP2_CAL_B26                        ,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B26             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B26        ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B26         ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B26         ,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B26             ,
    EN_NV_ID_IIP2_CAL_TABLE_B26                     ,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B26                   ,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B26                   ,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B26                        ,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B26               ,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B26             ,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B26            ,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B26           ,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B26        ,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B26     ,
    //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B26     ,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B26   ,
    //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B26   ,

    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B26,
    EN_NV_ID_DPD_FAC_LUT_00_B26,
    EN_NV_ID_DPD_FAC_LUT_01_B26,
    EN_NV_ID_DPD_FAC_LUT_02_B26,
    EN_NV_ID_DPD_FAC_LUT_03_B26,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B26,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B26,

/*BAND128 生产NV*/
    //EN_NV_ID_ANT_MODEM_LOSS_B128              /* 0xf440*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B128            /* 0xf44D*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B128            /* 0xf44E*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B128        /* 0xf44F*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B128        /* 0xf450*/,
    EN_NV_ID_LTE_IP2_CAL_B128               /* 0xf451*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B128             ,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B128       /* 0xf456*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B128            /* 0xf457*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B128            /* 0xf458*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B128            ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B128                  /* 0xf459*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B128                /* 0xf45a*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B128                /* 0xf45b*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B128                /* 0xf45d*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B128            /* 0xf45f*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B128                  /* 0xf465*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B128             /* 0xf466*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B128            /* 0xf467*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B128         /* 0xf468*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B128,
    EN_NV_ID_DPD_FAC_LUT_00_B128,
    EN_NV_ID_DPD_FAC_LUT_01_B128,
    EN_NV_ID_DPD_FAC_LUT_02_B128,
    EN_NV_ID_DPD_FAC_LUT_03_B128,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B128,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B128,


    /*band29 生产NV*/


     //EN_NV_ID_ANT_MODEM_LOSS_B29                     /* 0xfc40*/,
    EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B29         /* 0xf4cd*/,
    EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B29         /* 0xf4ce*/,
    EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B29       /* 0xf4cf*/,
    EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B29       /* 0xf4d0*/,
    EN_NV_ID_LTE_IP2_CAL_B29                        /* 0xf4d1*/,
    EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B29            /*0x4d4 */,
    EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B29        /* 0xf4d6*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B29         /* 0xf4d7*/,
    EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B29         /* 0xf4d8*/,
    EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B29             ,
    /*add for V9R1_6361 Begin*/
    EN_NV_ID_IIP2_CAL_TABLE_B29                   /* 0xf4d9*/,
    EN_NV_ID_RF_DCOC_CAL_ANT1_B29                  /* 0xf4da*/,
    EN_NV_ID_RF_DCOC_CAL_ANT2_B29                  /* 0xf4db*/,
    EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B29                       /* 0xf4dd*/,
    EN_NV_ID_TX_RF_FREQ_COMP_STRU_B29              /* 0xf4df*/,
    EN_NV_ID_LTE_TX_PD_AUTO_CAL_B29        /* 0xf4e5*/,
    EN_NV_ID_LTE_TX_PD_PWR_TABLE_B29       /* 0xf4e6*/,
    EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B29      /* 0xf4e7*/,
    EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B29          /* 0xf4e8*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B29,
    EN_NV_ID_DPD_FAC_LUT_00_B29,
    EN_NV_ID_DPD_FAC_LUT_01_B29,
    EN_NV_ID_DPD_FAC_LUT_02_B29,
    EN_NV_ID_DPD_FAC_LUT_03_B29,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B29,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B29,

    /*band32 生产NV*/

     //EN_NV_ID_ANT_MODEM_LOSS_B32                        = 0xf500*/,
     EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B32         /* 0xf50d*/,
     EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B32         /* 0xf50e*/,
     EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B32         /* 0xf50f*/,
     EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B32        /* 0xf510*/,
     EN_NV_ID_LTE_IP2_CAL_B32                        /* 0xf511*/,
     EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B32        /* 0xf514*/,
     EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B32       /* 0xf516*/,
     EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B32         /* 0xf517*/,
     EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B32         /* 0xf518*/,
     EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B32              ,
     /*add for V9R1_6361 Begin*/
     EN_NV_ID_IIP2_CAL_TABLE_B32                         /* 0xf519*/,
     EN_NV_ID_RF_DCOC_CAL_ANT1_B32                   /* 0xf51a*/,
     EN_NV_ID_RF_DCOC_CAL_ANT2_B32                  /* 0xf51b*/,
     EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B32               /* = 0xf51d*/,
     EN_NV_ID_TX_RF_FREQ_COMP_STRU_B32              /* 0xf51f*/,
     EN_NV_ID_LTE_TX_PD_AUTO_CAL_B32                 /* = 0xf525*/,
     EN_NV_ID_LTE_TX_PD_PWR_TABLE_B32                /* 0xf526*/,
     EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B32               /* 0xf527*/,
     EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B32            /* 0xf528*/,
    /* Add For DPD */
    EN_NV_ID_DPD_FAC_RESULT_B32,
    EN_NV_ID_DPD_FAC_LUT_00_B32,
    EN_NV_ID_DPD_FAC_LUT_01_B32,
    EN_NV_ID_DPD_FAC_LUT_02_B32,
    EN_NV_ID_DPD_FAC_LUT_03_B32,
    /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B32,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B32,


      /*band30 生产NV*/

     //EN_NV_ID_ANT_MODEM_LOSS_B30                        = 0xf540*/,
     EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B30         /* 0xf54d*/,
     EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B30         /* 0xf54e*/,
     EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B30         /* 0xf54f*/,
     EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B30        /* 0xf550*/,
     EN_NV_ID_LTE_IP2_CAL_B30                        /* 0xf551*/,
     EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B30        /* 0xf554*/,
     EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B30       /* 0xf556*/,
     EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B30         /* 0xf557*/,
     EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B30         /* 0xf558*/,
     EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B30              ,
     /*add for V9R1_6361 Begin*/
     EN_NV_ID_IIP2_CAL_TABLE_B30                         /* 0xf559*/,
     EN_NV_ID_RF_DCOC_CAL_ANT1_B30                   /* 0xf55a*/,
     EN_NV_ID_RF_DCOC_CAL_ANT2_B30                  /* 0xf55b*/,
     EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B30               /* = 0xf55d*/,
     EN_NV_ID_TX_RF_FREQ_COMP_STRU_B30              /* 0xf55f*/,
     EN_NV_ID_LTE_TX_PD_AUTO_CAL_B30                 /* = 0xf565*/,
     EN_NV_ID_LTE_TX_PD_PWR_TABLE_B30                /* 0xf566*/,
     EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B30               /* 0xf567*/,
     EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B30            /* 0xf568*/,
        /* Add For DPD */
        EN_NV_ID_DPD_FAC_RESULT_B30,
        EN_NV_ID_DPD_FAC_LUT_00_B30,
        EN_NV_ID_DPD_FAC_LUT_01_B30,
        EN_NV_ID_DPD_FAC_LUT_02_B30,
        EN_NV_ID_DPD_FAC_LUT_03_B30,
        /* End Add For DPD */
    EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B30,
    EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B30,


    /*TDS*/
    /*Band34 工厂nv*/
    EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B34                    /*0xf8a0*/,
    EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B34                     /*0xf8a2*/,
    EN_NV_ID_TDS_AGC_FREQ_COMP_B34                     /*0xf8a3*/,
    EN_NV_ID_TDS_DCOC_CAL_B34                         /*0xf8a4*/,
    EN_NV_ID_TDS_RF_TXIQ_CAL_B34                     /*0xf8a5*/,
    //EN_NV_ID_TDS_APC_TABLE_STRU_B34                     /*0xf8a6*/,
    /*Band39 工厂nv*/
    EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B39                     /*0xf8b0*/,
    EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B39             /*0xf8b1*/,
    EN_NV_ID_TDS_AGC_FREQ_COMP_B39                     /*0xf8b3*/,
    EN_NV_ID_TDS_DCOC_CAL_B39                         /*0xf8b4*/,
    EN_NV_ID_TDS_RF_TXIQ_CAL_B39                     /*0xf8b5*/,
    //EN_NV_ID_TDS_APC_TABLE_STRU_B39                    /*0xf8b6*/,

    /*Band40 工厂nv*/
    EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B40                     /*0xf8c0*/,
    EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B40             /*0xf8b1*/,
    EN_NV_ID_TDS_AGC_FREQ_COMP_B40                     /*0xf8c3*/,
    EN_NV_ID_TDS_DCOC_CAL_B40                         /*0xf8c4*/,
    EN_NV_ID_TDS_RF_TXIQ_CAL_B40                     /*0xf8c5*/,
    //EN_NV_ID_TDS_APC_TABLE_STRU_B40                     /*0xf8c6*/,
//#if defined(LPHY_K3V6)
    /*Band_Reserved 工厂nv*/
     EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_BRESERVED             /*0xf8d0*/,
     EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_BRESERVED       ,
    EN_NV_ID_TDS_AGC_FREQ_COMP_BRESERVED             /*0xf8d3*/,
    EN_NV_ID_TDS_DCOC_CAL_BRESERVED                 /*0xf8d4*/,
    EN_NV_ID_TDS_RF_TXIQ_CAL_BRESERVED                 /*0xf8d5*/,
    //EN_NV_ID_TDS_APC_TABLE_STRU_BRESERVED                 /*0xf8d6*/,

};

/* User ID need to restore */
unsigned short  g_ausNvResumeUserIdList[] =
{
        NV_ID_DRV_SELF_CTRL,
    NV_ID_CRC_CHECK_RESULT,
    NV_ID_GUC_CHECK_ITEM,
    NV_ID_TL_CHECK_ITEM,
    NV_ID_GUC_M2_CHECK_ITEM,
    en_NV_Item_ScheduleWatchDog_Time

};


/* 非加密版本需要进行恢复的机要数据NV项 */
unsigned short  g_ausNvResumeSecureIdList[] =
{
    en_NV_Item_CustomizeSimLockPlmnInfo,
    en_NV_Item_CardlockStatus,
    en_NV_Item_CustomizeSimLockMaxTimes
};

/*****************************************************************************
Function   : NV_GetResumeNvIdNum
Description: Return the number of resumed NV.
Input      : NV_RESUME_ITEM_ENUM_UINT32 - resumed NV's type.
Return     : Zero or others.
Other      :
*****************************************************************************/
unsigned long bsp_nvm_getRevertNum(unsigned long enNvItem)
{
    if (NV_MANUFACTURE_ITEM == enNvItem)
    {
        return sizeof(g_ausNvResumeManufactureIdList)/sizeof(g_ausNvResumeManufactureIdList[0]);
    }

    if (NV_USER_ITEM == enNvItem)
    {
        return sizeof(g_ausNvResumeUserIdList)/sizeof(g_ausNvResumeUserIdList[0]);
    }

    if (NV_SECURE_ITEM == enNvItem)
    {
        return sizeof(g_ausNvResumeSecureIdList)/sizeof(g_ausNvResumeSecureIdList[0]);
    }

    return 0;
}


