/*
* Copyright (C) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
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

#ifndef __LTPHYNVDEFINEN_H__
#define __LTPHYNVDEFINEN_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif



/*****************************************************************************
  1 Include Headfile
*****************************************************************************/

/* this file is included by drv */
/* #pragma pack(4) */

/*****************************************************************************
  2 macro
*****************************************************************************/

/*****************************************************************************
  3 Massage Declare
*****************************************************************************/


/*****************************************************************************
  4 Enum
*****************************************************************************/
#if defined(TL_PHY_V760)

/*TL PHY: 34000~48999 for Boston new added.
**the original TL PHY: 0xD3C0~0xF8FF    **/

enum NV_BAND_INDEX_ENUM
{
BAND_1      =  0, /*9,  */
BAND_2      =  1, /*14, */
BAND_3      =  2, /*8,  */
BAND_4      =  3, /*15, */
BAND_5      =  4, /*10, */
BAND_6      =  8, /*16, */
BAND_7      =  9, /* 4,  */
BAND_8      =  10,/* 11, */
BAND_9      =  11,/* 17, */
BAND_10     =  12,/* 18, */
BAND_11     =  13,/* 19, */
BAND_12     =  14,/* 20, */
BAND_13     =  15,/* 21, */
BAND_14     =  16,/* 22, */
BAND_17     =  17,/* 23, */
BAND_18     =  18,/* 24, */
BAND_19     =  19,/* 12, */
BAND_20     =  20,/* 0,  */
BAND_21     =  21,/* 13,*/
BAND_22     =  22,/* 25,*/
BAND_23     =  23,/* 26,*/
BAND_24     =  24,/* 27,*/
BAND_25     =  25,/* 28,*/
BAND_26     =  26,/* 54,*/
BAND_28     =  27,/* 37,*/
BAND_29     =  28,/* 39,*/
BAND_30     =  29,/*    */
BAND_32     =  30,/* 40,*/
BAND_33     =  31,/* 29,*/
BAND_34     =  32,/* 30,*/
BAND_35     =  33,/* 31,*/
BAND_36     =  34,/* 32,*/
BAND_37     =  35,/* 33,*/
BAND_38     =  36,/* 2, */
BAND_39     =  37,/* 36,*/
BAND_40     =  38,/* 1, */
BAND_41     =  39,/* 3,  */
BAND_42     =  40,/* 34, */
BAND_43     =  41,/* 35, */
BAND_128    =  42,/* 38, */
BAND_140    =  43,
BAND_44     =  44,
BAND_46     =  45,
BAND_66     =  46,
BAND_110    =  47,
BAND_111    =  48,
BAND_112    =  49,
BAND_113    =  50,
BAND_114    =  51,
BAND_115    =  52,
BAND_116    =  54,
BAND_BUTT        = 0xff
};




#define EN_NV_ID_B(bandnum)\
        EN_NV_ID_FTM_CAND_CELL_LIST_B##bandnum                 = (0xD600 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B##bandnum            = (0xD601 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_CAL_LIST_B##bandnum                    = (0xD602 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_RX_AGC_TEMP_COMP_B##bandnum               = (0xD603 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_PATH0_B##bandnum         = (0xD604 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_PATH1_B##bandnum         = (0xD605 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_PATH2_B##bandnum         = (0xD606 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_PATH3_B##bandnum         = (0xD607 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_TAS_B##bandnum                         = (0xD608 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_USED_AGC_TBL_B##bandnum                       = (0xD618 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_IP2_CAL_CHAN_B##bandnum                   = (0xD61e + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B##bandnum         = (0xD620 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B##bandnum             = (0xD622 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_TX_FILTER_CMP_STRU_B##bandnum                 = (0xD623 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_CAL_COMP_STRU_B##bandnum               = (0xD624 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B##bandnum       = (0xD625 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APT_PARA_B##bandnum                    = (0xD626 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B##bandnum            = (0xD627 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B##bandnum             = (0xD628 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B##bandnum             = (0xD629 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_MPR_B##bandnum                         = (0xD62a + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_GAIN_BALANCE_B##bandnum                   = (0xD62d + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_ANT_SEL_B##bandnum                     = (0Xd630 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_LTE_TX_BODY_SAR_B##bandnum                    = (0Xd631 + (BAND_##bandnum)*0x40),   \
        EN_NV_ID_EXT_LNA_AGC_PARA_B##bandnum                   = (0xD633 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B##bandnum              = (0xD639 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_TX_AMPR_NS03_B##bandnum                   = (0xD63a + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_TX_AMPR_NS22_B##bandnum                   = (0xD63b + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_HP_SAR_B##bandnum                         = (0xD63c + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B##bandnum             = (0xD63d + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_CIM_CAL_PARA_STRU_B##bandnum              = (0xD63e + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_CIM_CAL_RESULT_STRU_B##bandnum            = (0xD63f + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ETM_ET_APT_BAND_LAB_STRU_B##bandnum               = (0xD60f + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ET_LUT_BAND_STRU_B##bandnum                       = (0xD609 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ET_DELAY_BAND_FAC_STRU_B##bandnum                 = (0xD60a + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_BAND_LAB_STRU_B##bandnum     = (0xD60b + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_BAND_LAB_STRU_B##bandnum = (0xD60c + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ET_POUT_FREQ_COMP_STRU_B##bandnum                 = (0xD60e + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ET_CAL_DSP_PARA_STRU_B##bandnum                   = (0xD610 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ET_PWR_AVG_TEMP_COMP_PATH_STRU_B##bandnum = (0xD611 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_ET_DELAY_TEMP_COMP_PATH_STRU_B##bandnum = (0xD612 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_APC_TEMP_COMP_ET_STRU_B##bandnum = (0xD613 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_TX_CAL_TEMP_ET_POW_STRU_B##bandnum = (0xD614 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_TX_CAL_TEMP_ET_DELAY_STRU_B##bandnum = (0xD615 + (BAND_##bandnum)*0x40)

#define EN_NV_ID_LAB_FAC_B(bandnum)\
        EN_NV_ID_ANT_MODEM_LOSS_B##bandnum                    = (0xeb00 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_RX_AGC_CAL_RESULT_PATH0_B##bandnum       = (0xeb01 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_RX_AGC_CAL_RESULT_PATH1_B##bandnum       = (0xeb02 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_RX_AGC_CAL_RESULT_PATH2_B##bandnum       = (0xeb03 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_RX_AGC_CAL_RESULT_PATH3_B##bandnum       = (0xeb04 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_RF_DCOC_CAL_PATH0_B##bandnum                 = (0xeb05 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_RF_DCOC_CAL_PATH1_B##bandnum                 = (0xeb06 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_RF_DCOC_CAL_PATH2_B##bandnum                 = (0xeb07 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_RF_DCOC_CAL_PATH3_B##bandnum                 = (0xeb08 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_RX_CAL_HKADC_B##bandnum                  = (0xeb09 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_TX_CAL_HKADC_B##bandnum                  = (0xeb0a + (BAND_##bandnum)*0x40),\
        EN_NV_ID_IIP2_CAL_TABLE_B##bandnum                    = (0xeb19 + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_APC_TABLE_STRU_B##bandnum                = (0xeb1c + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B##bandnum              = (0xeb1d + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B##bandnum        = (0xeb1e + (BAND_##bandnum)*0x40),\
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B##bandnum              = (0xeb1f + (BAND_##bandnum)*0x40),\
        EN_NV_ID_LTE_APC_TABLE_DEFAULT_STRU_B##bandnum        = (0xeb20 + (BAND_##bandnum)*0x40),\
        NV_NV_ID_RF_RX_RFFE_ILOSS_B##bandnum                  = (0xeb21 + (BAND_##bandnum)*0x40),


#define EN_NV_ID_BUFFER(name, hundred,decimal, index)\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##0          =  (index+0x0),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##1          =  (index+0x1),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##2          =  (index+0x2),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##3          =  (index+0x3),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##4          =  (index+0x4),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##5          =  (index+0x5),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##6          =  (index+0x6),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##7          =  (index+0x7),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##8          =  (index+0x8),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##9          =  (index+0x9),

#define EN_NV_ID_BUFFER_8(name, hundred,decimal, index)\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##0          =  (index+0x0),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##1          =  (index+0x1),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##2          =  (index+0x2),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##3          =  (index+0x3),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##4          =  (index+0x4),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##5          =  (index+0x5),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##6          =  (index+0x6),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##7          =  (index+0x7),

#define EN_NV_ID_BUFFER_6(name, hundred,decimal, index)\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##0          =  (index+0x0),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##1          =  (index+0x1),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##2          =  (index+0x2),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##3          =  (index+0x3),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##4          =  (index+0x4),\
        EN_NV_ID_##name##_INDEX_##hundred##decimal##5          =  (index+0x5),


#define EN_NV_ID_BUFFER_100(name,hundred, index)\
        EN_NV_ID_BUFFER(name,hundred,0,index)\
        EN_NV_ID_BUFFER(name,hundred,1,(index+10))\
        EN_NV_ID_BUFFER(name,hundred,2,(index+20))\
        EN_NV_ID_BUFFER(name,hundred,3,(index+30))\
        EN_NV_ID_BUFFER(name,hundred,4,(index+40))\
        EN_NV_ID_BUFFER(name,hundred,5,(index+50))\
        EN_NV_ID_BUFFER(name,hundred,6,(index+60))\
        EN_NV_ID_BUFFER(name,hundred,7,(index+70))\
        EN_NV_ID_BUFFER(name,hundred,8,(index+80))\
        EN_NV_ID_BUFFER(name,hundred,9,(index+90))

#define EN_NV_ID_BUFFER_128(name, index)\
        EN_NV_ID_BUFFER_100(name,0, index)\
        EN_NV_ID_BUFFER(name,1,0,(index+100))\
        EN_NV_ID_BUFFER(name,1,1,(index+110))\
        EN_NV_ID_BUFFER_8(name,1,2,(index+120))

#define EN_NV_ID_BUFFER_256(name, index)\
        EN_NV_ID_BUFFER_100(name,0, index)\
        EN_NV_ID_BUFFER_100(name,1, (index+100))\
        EN_NV_ID_BUFFER(name,2,0,(index+200))\
        EN_NV_ID_BUFFER(name,2,1,(index+210))\
        EN_NV_ID_BUFFER(name,2,2,(index+220))\
        EN_NV_ID_BUFFER(name,2,3,(index+230))\
        EN_NV_ID_BUFFER(name,2,4,(index+240))\
        EN_NV_ID_BUFFER_6(name,2,5,(index+250))


#define DYN_BAND_APC_TEMP_NV(bandnum, index)\
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_PATH4_B##bandnum       = (0x9080 + (index)*0x04 + 0), \
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_PATH5_B##bandnum       = (0x9080 + (index)*0x04 + 1), \
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_PATH6_B##bandnum       = (0x9080 + (index)*0x04 + 2), \
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_PATH7_B##bandnum       = (0x9080 + (index)*0x04 + 3),

#define DYN_BAND_AGC_CAL_NV(bandnum, index)\
        EN_NV_ID_LTE_RX_AGC_CAL_RESULT_PATH4_B##bandnum      = (0x9000 + (index)*0x04 + 0), \
        EN_NV_ID_LTE_RX_AGC_CAL_RESULT_PATH5_B##bandnum      = (0x9000 + (index)*0x04 + 1), \
        EN_NV_ID_LTE_RX_AGC_CAL_RESULT_PATH6_B##bandnum      = (0x9000 + (index)*0x04 + 2), \
        EN_NV_ID_LTE_RX_AGC_CAL_RESULT_PATH7_B##bandnum      = (0x9000 + (index)*0x04 + 3),

#define DYN_BAND_DCOC_CAL_NV(bandnum, index)\
        EN_NV_ID_RF_DCOC_CAL_PATH4_B##bandnum                = (0x9040 + (index)*0x04 + 0), \
        EN_NV_ID_RF_DCOC_CAL_PATH5_B##bandnum                = (0x9040 + (index)*0x04 + 1), \
        EN_NV_ID_RF_DCOC_CAL_PATH6_B##bandnum                = (0x9040 + (index)*0x04 + 2), \
        EN_NV_ID_RF_DCOC_CAL_PATH7_B##bandnum                = (0x9040 + (index)*0x04 + 3),

#define DYN_BAND_NV(bandnum, index) \
        DYN_BAND_AGC_CAL_NV(bandnum, index) \
        DYN_BAND_DCOC_CAL_NV(bandnum, index) \
        DYN_BAND_APC_TEMP_NV(bandnum, index)


enum NV_TLPHY_ITEM_ID_ENUM
{
        EN_NV_ID_AGC_PARA                               = 0xD3CD,

        /* modify by   for 所有band begin*/
        EN_NV_ID_B(19),

       /* EN_NV_ID_FTM_CAND_CELL_LIST_B19                 = 0xD900,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B19            = 0xD901,
        EN_NV_ID_LTE_TX_CAL_LIST_B19                    = 0xD902,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B19          = 0xD903,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B19          = 0xD904,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B19                = 0xD905,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B19               = 0xD906,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B19               = 0xD907,
        EN_NV_ID_TEMP_SENSOR_TABLE_B19                  = 0xD90f,
        EN_NV_ID_HI6360_AGC_PARA_B19                    = 0xD918,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B19        = 0xD91b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B19        = 0xD91c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B19                  = 0xD91d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B19                   = 0xD91e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B19              = 0xD91f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B19         = 0xD920,
        EN_NV_ID_LTE_TX_ATTEN_B19                       = 0xD921,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B19             = 0xD922,
        EN_NV_ID_TX_FILTER_CMP_STRU_B19                 = 0xD923,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B19               = 0xD924,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B19       = 0xD925,
        EN_NV_ID_LTE_TX_APT_PARA_B19                    = 0xD926,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B19            = 0xD927,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B19             = 0xD928,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B19             = 0xD929,
        EN_NV_ID_LTE_TX_MPR_B19                         = 0xD92a,
        EN_NV_ID_LTE_ANT_SAR_B19                     = 0xD92b,
        EN_NV_ID_LTE_TX_AMPR_B19                        = 0xD92c,
        EN_NV_ID_LTE_OTHER_COMP_B19         = 0xD92d,
        EN_NV_ID_LTE_TX_AMPR_NS05_B19                    = 0xD92e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B19                    = 0xD92f,
        EN_NV_ID_LTE_TX_ANT_SEL_B19                      = 0Xd930,
        EN_NV_ID_TX_PA_TEMP_COMP_B19                    = 0xD932,
        EN_NV_ID_EXT_LNA_AGC_PARA_B19                      = 0xD933,
        EN_NV_ID_CALB_AGC_B19                              = 0xD934,
         EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B19                = 0xD935,
        EN_NV_ID_LTE_TX_PD_PARA_B19                      = 0xD936,
        EN_NV_ID_TX_ET_BAND_PARA_B19                      = 0xD937,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B19           = 0xD938,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B19               = 0xD939,
        EN_NV_ID_LTE_TX_AMPR_NS03_B19                   = 0xD93a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B19                   = 0xD93b,
        EN_NV_ID_LTE_HP_SAR_B19                         = 0xD93c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B19              = 0xD93d,*/

        EN_NV_ID_B(21),
        /*EN_NV_ID_FTM_CAND_CELL_LIST_B21                 = 0xD940,*/
        EN_NV_ID_B(2),
        /*EN_NV_ID_FTM_CAND_CELL_LIST_B2                  = 0xD980,*/
        EN_NV_ID_B(4),
        EN_NV_ID_B(6),
        EN_NV_ID_B(9),
        EN_NV_ID_B(10),
        EN_NV_ID_B(11),
        EN_NV_ID_B(12),
        EN_NV_ID_B(13),
        EN_NV_ID_B(14),
        EN_NV_ID_B(17),
        EN_NV_ID_B(18),
        EN_NV_ID_B(22),
        EN_NV_ID_B(23),
        EN_NV_ID_B(24),
        /*EN_NV_ID_FTM_CAND_CELL_LIST_B4                  = 0xD9c0,
        EN_NV_ID_FTM_CAND_CELL_LIST_B6                  = 0xDa00,
        EN_NV_ID_FTM_CAND_CELL_LIST_B9                  = 0xDa40,
        EN_NV_ID_FTM_CAND_CELL_LIST_B10                 = 0xDa80,
        EN_NV_ID_FTM_CAND_CELL_LIST_B11                 = 0xDac0,
        EN_NV_ID_FTM_CAND_CELL_LIST_B12                 = 0xDb00,
        EN_NV_ID_FTM_CAND_CELL_LIST_B13                 = 0xDb40,
        EN_NV_ID_FTM_CAND_CELL_LIST_B14                 = 0xDb80,
        EN_NV_ID_FTM_CAND_CELL_LIST_B17                 = 0xDbc0,
        EN_NV_ID_FTM_CAND_CELL_LIST_B18                 = 0xDc00,
        EN_NV_ID_FTM_CAND_CELL_LIST_B22                 = 0xDc40,
        EN_NV_ID_FTM_CAND_CELL_LIST_B24                 = 0xDcc0,*/

        EN_NV_ID_B(25),
        EN_NV_ID_B(33),
        EN_NV_ID_B(34),
        EN_NV_ID_B(35),
        EN_NV_ID_B(36),
        EN_NV_ID_B(37),
        EN_NV_ID_B(42),
        EN_NV_ID_B(43),
        EN_NV_ID_B(39),
        EN_NV_ID_B(20),


        /*EN_NV_ID_FTM_CAND_CELL_LIST_B25                 = 0xDd00,
        EN_NV_ID_FTM_CAND_CELL_LIST_B33                 = 0xDd40,
        EN_NV_ID_FTM_CAND_CELL_LIST_B34                 = 0xDd80,
        EN_NV_ID_FTM_CAND_CELL_LIST_B35                 = 0xDdc0,
        EN_NV_ID_FTM_CAND_CELL_LIST_B36                 = 0xDe00,
        EN_NV_ID_FTM_CAND_CELL_LIST_B37                 = 0xDe40,
        EN_NV_ID_FTM_CAND_CELL_LIST_B42                 = 0xDe80,
        EN_NV_ID_FTM_CAND_CELL_LIST_B43                 = 0xDec0,
        EN_NV_ID_FTM_CAND_CELL_LIST_B39                 = 0xDf00,
        EN_NV_ID_FTM_CAND_CELL_LIST_B20                 = 0xD600,*/
        EN_NV_ID_B(40),
        EN_NV_ID_B(38),
        EN_NV_ID_B(41),
        EN_NV_ID_B(7),
        EN_NV_ID_B(3),
        EN_NV_ID_B(1),
        EN_NV_ID_B(5),
        EN_NV_ID_B(8),
        EN_NV_ID_B(28),

        /*EN_NV_ID_FTM_CAND_CELL_LIST_B40                 = 0xD640,
        EN_NV_ID_FTM_CAND_CELL_LIST_B38                 = 0xD680,
        EN_NV_ID_FTM_CAND_CELL_LIST_B41                 = 0xD6c0,
        EN_NV_ID_FTM_CAND_CELL_LIST_B7                  = 0xD700,
        EN_NV_ID_FTM_CAND_CELL_LIST_B3                  = 0xD800,
        EN_NV_ID_FTM_CAND_CELL_LIST_B1                  = 0xD840,
        EN_NV_ID_FTM_CAND_CELL_LIST_B5                  = 0xD880,
        EN_NV_ID_FTM_CAND_CELL_LIST_B8                  = 0xD8c0,
        EN_NV_ID_FTM_CAND_CELL_LIST_B28                  = 0xDf40,*/

        EN_NV_ID_B(26),
        EN_NV_ID_B(128),
        EN_NV_ID_B(29),
        EN_NV_ID_B(32),
        EN_NV_ID_B(30),

        /*EN_NV_ID_FTM_CAND_CELL_LIST_B26                 = 0xE380,
        EN_NV_ID_FTM_CAND_CELL_LIST_B128                  = 0xDf80,
        EN_NV_ID_FTM_CAND_CELL_LIST_B29                  = 0xDfc0,
        EN_NV_ID_FTM_CAND_CELL_LIST_B32                  = 0xe000,
        EN_NV_ID_FTM_CAND_CELL_LIST_B30                  = 0xe040,*/

        EN_NV_ID_B(140),
        EN_NV_ID_B(44),
        EN_NV_ID_B(46),
        EN_NV_ID_B(66),
        EN_NV_ID_B(110),
        EN_NV_ID_B(111),
        EN_NV_ID_B(112),
        EN_NV_ID_B(113),
        EN_NV_ID_B(114),
        EN_NV_ID_B(115),
        EN_NV_ID_B(116),

       /* EN_NV_ID_FTM_CAND_CELL_LIST_BNon6                 = 0xe080,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon7                 = 0xe0c0,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon8                 = 0xe100,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon9                 = 0xe140,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon10                 = 0xe180,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon11                 = 0xe1c0,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon12                 = 0xe200,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon13                 = 0xe240,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon14                 = 0xe280,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon15                 = 0xe2c0,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon16                 = 0xe300,*/
        /* 非标频段end */
        EN_NV_ID_MODEM_END                              = 0xE4ff,

        EN_NV_ID_TCXO_DYN_CONFIG                        = 0x1401,

        EN_NV_ID_LTE_TCXO_INIT_FREQ                     = 0xe900,

        EN_NV_ID_LAB_FAC_B(20)
        EN_NV_ID_LAB_FAC_B(40)
        EN_NV_ID_LAB_FAC_B(38)
        EN_NV_ID_LAB_FAC_B(41)
        EN_NV_ID_LAB_FAC_B(7)
        EN_NV_ID_LAB_FAC_B(3)
        EN_NV_ID_LAB_FAC_B(1)
        EN_NV_ID_LAB_FAC_B(5)
        EN_NV_ID_LAB_FAC_B(8)
        EN_NV_ID_LAB_FAC_B(19)
        EN_NV_ID_LAB_FAC_B(21)
        EN_NV_ID_LAB_FAC_B(2)

        /*EN_NV_ID_ANT_MODEM_LOSS_B20                     = 0xeb00,
        EN_NV_ID_ANT_MODEM_LOSS_B40                     = 0xeb40,
        EN_NV_ID_ANT_MODEM_LOSS_B38                     = 0xeb80,
        EN_NV_ID_ANT_MODEM_LOSS_B41                     = 0xebc0,
        EN_NV_ID_ANT_MODEM_LOSS_B7                      = 0xec00,
        EN_NV_ID_ANT_MODEM_LOSS_B3                      = 0xed00,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B3    =    0xed01,

        EN_NV_ID_ANT_MODEM_LOSS_B1                      = 0xed40,
        EN_NV_ID_ANT_MODEM_LOSS_B5                      = 0xed80,
        EN_NV_ID_ANT_MODEM_LOSS_B8                      = 0xedc0,
        EN_NV_ID_ANT_MODEM_LOSS_B19                     = 0xEE00,
        EN_NV_ID_ANT_MODEM_LOSS_B21                     = 0xEE40,
        EN_NV_ID_ANT_MODEM_LOSS_B2                      = 0xEE80,*/

        EN_NV_ID_LAB_FAC_B(4)
        EN_NV_ID_LAB_FAC_B(6)
        EN_NV_ID_LAB_FAC_B(9)
        EN_NV_ID_LAB_FAC_B(10)
        EN_NV_ID_LAB_FAC_B(11)
        EN_NV_ID_LAB_FAC_B(12)
        EN_NV_ID_LAB_FAC_B(13)
        EN_NV_ID_LAB_FAC_B(14)
        EN_NV_ID_LAB_FAC_B(17)
        EN_NV_ID_LAB_FAC_B(18)
        EN_NV_ID_LAB_FAC_B(22)
        EN_NV_ID_LAB_FAC_B(23)

       /* EN_NV_ID_ANT_MODEM_LOSS_B4                      = 0xEEc0,
        EN_NV_ID_ANT_MODEM_LOSS_B6                      = 0xEf00,
        EN_NV_ID_ANT_MODEM_LOSS_B9                      = 0xEf40,
        EN_NV_ID_ANT_MODEM_LOSS_B10                     = 0xEf80,
        EN_NV_ID_ANT_MODEM_LOSS_B11                     = 0xEfc0,
        EN_NV_ID_ANT_MODEM_LOSS_B12                     = 0xf000,
        EN_NV_ID_ANT_MODEM_LOSS_B13                     = 0xf040,
        EN_NV_ID_ANT_MODEM_LOSS_B14                     = 0xf080,
        EN_NV_ID_ANT_MODEM_LOSS_B17                     = 0xf0c0,
        EN_NV_ID_ANT_MODEM_LOSS_B18                     = 0xf100,
        EN_NV_ID_ANT_MODEM_LOSS_B22                     = 0xf140,
        EN_NV_ID_ANT_MODEM_LOSS_B23                     = 0xf180,*/

        EN_NV_ID_LAB_FAC_B(24)
        EN_NV_ID_LAB_FAC_B(25)
        EN_NV_ID_LAB_FAC_B(33)
        EN_NV_ID_LAB_FAC_B(34)
        EN_NV_ID_LAB_FAC_B(35)
        EN_NV_ID_LAB_FAC_B(36)
        EN_NV_ID_LAB_FAC_B(37)
        EN_NV_ID_LAB_FAC_B(42)
        EN_NV_ID_LAB_FAC_B(43)
        EN_NV_ID_LAB_FAC_B(39)
        EN_NV_ID_LAB_FAC_B(28)
        EN_NV_ID_LAB_FAC_B(128)

       /* EN_NV_ID_ANT_MODEM_LOSS_B24                     = 0xf1c0,
        EN_NV_ID_ANT_MODEM_LOSS_B25                     = 0xf200,
        EN_NV_ID_ANT_MODEM_LOSS_B33                     = 0xf240,
        EN_NV_ID_ANT_MODEM_LOSS_B34                     = 0xf280,
        EN_NV_ID_ANT_MODEM_LOSS_B35                     = 0xf2c0,
        EN_NV_ID_ANT_MODEM_LOSS_B36                     = 0xf300,
        EN_NV_ID_ANT_MODEM_LOSS_B37                     = 0xf340,
        EN_NV_ID_ANT_MODEM_LOSS_B42                     = 0xf380,
        EN_NV_ID_ANT_MODEM_LOSS_B43                     = 0xf3c0,
        EN_NV_ID_ANT_MODEM_LOSS_B39                     = 0xf400,
        EN_NV_ID_ANT_MODEM_LOSS_B28                     = 0xf440,
        EN_NV_ID_ANT_MODEM_LOSS_B128                        = 0xf480,*/

        EN_NV_ID_LAB_FAC_B(29)
        EN_NV_ID_LAB_FAC_B(32)
        EN_NV_ID_LAB_FAC_B(30)

        /*EN_NV_ID_ANT_MODEM_LOSS_B29                        = 0xf4c0,
        EN_NV_ID_ANT_MODEM_LOSS_B32                        = 0xf500,
        EN_NV_ID_ANT_MODEM_LOSS_B30                     = 0xf540,*/

        EN_NV_ID_LAB_FAC_B(140)
        EN_NV_ID_LAB_FAC_B(44)
        EN_NV_ID_LAB_FAC_B(46)
        EN_NV_ID_LAB_FAC_B(66)
        EN_NV_ID_LAB_FAC_B(110)
        EN_NV_ID_LAB_FAC_B(111)
        EN_NV_ID_LAB_FAC_B(112)
        EN_NV_ID_LAB_FAC_B(113)
        EN_NV_ID_LAB_FAC_B(114)
        EN_NV_ID_LAB_FAC_B(115)
        EN_NV_ID_LAB_FAC_B(116)

        /*EN_NV_ID_ANT_MODEM_LOSS_BNon6                     = 0xf580,
        EN_NV_ID_ANT_MODEM_LOSS_BNon7                     = 0xf5c0,
        EN_NV_ID_ANT_MODEM_LOSS_BNon8                     = 0xf600,
        EN_NV_ID_ANT_MODEM_LOSS_BNon10                     = 0xf680,
        EN_NV_ID_ANT_MODEM_LOSS_BNon11                     = 0xf6c0,
        EN_NV_ID_ANT_MODEM_LOSS_BNon12                     = 0xf700,
        EN_NV_ID_ANT_MODEM_LOSS_BNon13                     = 0xf740,
        EN_NV_ID_ANT_MODEM_LOSS_BNon14                     = 0xf780,
        EN_NV_ID_ANT_MODEM_LOSS_BNon15                     = 0xf7c0,
        EN_NV_ID_ANT_MODEM_LOSS_BNon16                     = 0xf800,*/

        EN_NV_ID_LAB_FAC_B(26)/*0xF880*/
        /*EN_NV_ID_ANT_MODEM_LOSS_B26                     = 0xf820,*/

        /* modify by   for 所有band end*/

        EN_NV_ID_TIMING_PARA                            = 0xD3C0,
        EN_NV_ID_EMU_FAKECELL_PARA                      = 0xD3C1,
        EN_NV_ID_CQI_PARA                               = 0xD3C2,
        EN_NV_ID_ANTCORR_PARA                           = 0xD3C3,
        EN_NV_ID_RLM_PARA                               = 0xD3C4,
        EN_NV_ID_AFC_PARA                               = 0xD3C5,
        EN_NV_ID_IRC_PUB_PARA                           = 0xD3C6,
        EN_NV_ID_CHE_PARA                               = 0xD3C7,
        EN_NV_ID_VITERBI_PARA                           = 0xD3C8,
        EN_NV_ID_TURBO_PARA                             = 0xD3C9,
        EN_NV_ID_DEM_LIST_PARA                          = 0xD3CA,
        EN_NV_ID_HI6360_UL_PARA                         = 0xD3CC,
        EN_NV_ID_EMU_PARA                               = 0xD3CE,
        /* ETM BAND无关NV*/
        EN_NV_ID_ETM_CMD_PARA                           = 0xD3CF,

        /* modify by   begin */
        EN_NV_ID_PHY_SINGLE_ANTENNA_TEST_PARA           = 0xD3E0,
        /* modify by   end */

        /*tcx0*/
        EN_NV_ID_TCXO_DYNAMIC_CONFIG_PARA               = 0xD3E2,

        /* add by  2012-6-8 for TX_FILTER begin */
        EN_NV_ID_TX_FILTER_CMP                          = 0xD3E3,
        /* add by  2012-6-8 for TX_FILTER end */
        EN_NV_ID_FE_STATIC_TIMER_PARA                   = 0xD3E4,
        EN_NV_ID_FE_DYNAMIC_TIMER_PARA                  = 0xD3E5,

        /*orignal single Band Index 32? :
        EN_NV_ID_SINGLE_BAND_RF_DL_FEM_PATH_INDEX_000           = 0xD400~0x0xD41F,*/
        /*0x84d0 ~0x84d0+127 = 0x854f*/
        EN_NV_ID_BUFFER_128(SINGLE_BAND_FE_PARA_BAND ,     0x84d0)
        /*0x8550 ~0x864f*/
        EN_NV_ID_BUFFER_256(RF_DL_2R_PATH,       0x8550)
        /*0x8750 ~0x884f*/
        EN_NV_ID_BUFFER_256(RX_CAL_LAB_PARA,          0x8750)
        /*0x8950 ~0x89cf*/
        EN_NV_ID_BUFFER_256(RX_CAL_RESULT,          0x8950)
        /*0x8650 ~0x86cf*/
        EN_NV_ID_BUFFER_128(RF_UL_1T_PATH,     0x8b50)
        /* 0x8a50 ~0x8acf*/
        EN_NV_ID_BUFFER_128(TX_APC_LAB_PARA,          0x8c50)
         /* 0x8ad0 ~0x8b4f*/
        EN_NV_ID_BUFFER_128(TX_APC_LAB_COMP,          0x8d50)
         /* 0x8b50 ~0x8bcf*/
        EN_NV_ID_BUFFER_128(TX_FREQ_TEMP_COMP,          0x8e50)
         /* 0x8bd0 ~0x8c4f*/
        EN_NV_ID_BUFFER_128(TX_DEFAULT_POW_LAB,          0x8f50)
         /* 0x8c50 ~0x8ccf*/
        EN_NV_ID_BUFFER_128(TX_SELF_CAL_PARA,          0x9050)
         /* 0x8cd0 ~0x8d4f*/
        EN_NV_ID_BUFFER_128(TX_CAL_RESULT_FAC,          0x9150)
         /* 0x8d50 ~0x8dcf*/
        EN_NV_ID_BUFFER_128(TX_SELF_CAL_FAC,           0x9250)
        EN_NV_ID_BUFFER_128(TX_POW_CFG_PARA,       0x9350)

        EN_NV_ID_BUFFER_128(LTE_ETM_ET_APT_BAND_LAB,        0x9450)
        EN_NV_ID_BUFFER_128(LTE_ET_LUT_BAND,                0x9550)
        EN_NV_ID_BUFFER_128(LTE_ET_DELAY_BAND_FAC,          0x9650)
        EN_NV_ID_BUFFER_128(LTE_ETM_DYNAMIC_MIPI_CMD_BAND_LAB,           0x9750)
        EN_NV_ID_BUFFER_128(LTE_ETM_SEMI_STATIC_MIPI_CMD_BAND_LAB,       0x9850)
        EN_NV_ID_BUFFER_128(LTE_ET_PIN_COMP_BAND,           0x9950)
        EN_NV_ID_BUFFER_128(LTE_ET_POUT_FREQ_COMP,          0x9a50)
        EN_NV_ID_BUFFER_128(LTE_ET_CAL_DSP_PARA,            0x9b50)
        EN_NV_ID_BUFFER_128(LTE_ET_PWR_AVG_TEMP_COMP,       0x9c50)
        EN_NV_ID_BUFFER_128(LTE_ET_FRAC_DELAY_TEMP_COMP,       0x9d50)
        EN_NV_ID_BUFFER_128(LTE_ET_APC_TEMP_COMP,              0x9e50)

        EN_NV_ID_BUFFER_128(LTE_DPD_LAB_PARA,               0x9f50)
        EN_NV_ID_BUFFER_128(LTE_DPD_LAB_THRE_PARA,                0xa050)
        EN_NV_ID_BUFFER_128(LTE_DPD_FAC_RESULT,          0xa150)
        EN_NV_ID_BUFFER_128(LTE_DPD_FAC_LUT0,           0xa250)
        EN_NV_ID_BUFFER_128(LTE_DPD_FAC_LUT1,       0xa350)
        EN_NV_ID_BUFFER_128(LTE_DPD_FAC_LUT2,           0xa450)
        EN_NV_ID_BUFFER_128(LTE_DPD_FAC_LUT3,          0xa550)
        EN_NV_ID_BUFFER_128(LTE_DPD_LAB_PIN_OFFSET_COMP0,            0xa650)
        EN_NV_ID_BUFFER_128(LTE_DPD_LAB_PIN_OFFSET_COMP1,       0xa750)
        EN_NV_ID_BUFFER_128(LTE_DPD_LAB_PIN_OFFSET_COMP2,       0xa850)
        EN_NV_ID_BUFFER_128(LTE_DPD_LAB_PIN_OFFSET_COMP3,       0xa950)
        EN_NV_ID_BUFFER_128(LTE_DPD_LAB_POUT_TEMP_COMP,           0xaa50)
        EN_NV_ID_BUFFER_128(LTE_SCENE_SAR_PARA,                   0xab50)

        /*EN_NV_ID_BUFFER_128(LTE_PD_MRX_AGC_USEDINFO,          0xac50)
        EN_NV_ID_BUFFER_128(LTE_PD_MRX_IL,           0xad50)
        EN_NV_ID_BUFFER_128(LTE_PD_MRX_DCOC_CAL_RESULT,       0xae50)
        EN_NV_ID_BUFFER_128(LTE_PD_MRX_AGC_CAL_RESULT,           0xaf50)
        EN_NV_ID_BUFFER_128(LTE_MRX_2_TX_CHAIN_DLY_RESULT,          0xb050)*/

        /* orignal combination INdex 128:
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_000                  = 0xD420~0xD49f,*/
        /* 0xd400~0xd4ff*/
        EN_NV_ID_BUFFER_256(BAND_COMB_FE_PARA, 0xD400)

        /*EN_NV_ID_FE_BASIC_INFO                          = 0xD500,*/
        EN_NV_ID_FE_RFIC_INIT                           = 0xD501,
        EN_NV_ID_LTE_COM_ANT_SAR_PARA                   = 0xD502,
        EN_NV_FE_TIMER_STRU_PARA                        = 0xD503,

        EN_NV_ID_LVRAMP_PARA                            = 0xD504,
        EN_NV_ID_FE_NOTCH_INFO                          = 0xD505, /*notch*/

        EN_NV_ID_APC_GAIN_DEFALUT                       = 0xD507,
        EN_NV_ID_PA_POWER_DIFFERENCE_DEFALUT            = 0xD508,
        EN_NV_ID_DSP_NV_PARA_SIZE                       = 0xD509,
        EN_NV_ID_MIPIDEV_INIT                           = 0xD50A,

        /*begin added by   2014/06/23*/
        EN_NV_ID_LPHY_LWCOEX_INIT_PARA                  = 0xD510,
        /*end added by   2014/06/23*/

        /*begin added by   2015/12/26*/
        EN_NV_ID_LPHY_LWCOEX_FEATURE_SELECT_STRU    = 0xD527,
        /*end added by   2015/12/26*/

        /* use one ID for TAS/MAS */
        EN_NV_ID_MAS_ASU_PARA                       = 0xD511,
        EN_NV_SINGLE_XO_DEFINE                      = 0xD51d,
        EN_NV_ID_TEMP_DEFINE                        = 0xD51e,
        NV_ID_PA_TYPE_FLAG_PARA                     = 0xD51f,

#if 1 //defined (FEATURE_TLPHY_SINGLE_XO) || defined (FEATURE_TLPHY_TCXO_OVER_TEMP_PROTECT))
        EN_NV_ID_TL_COMM_NV_PARA_SIZE                = 0xD512,
        EN_NV_ID_DCXO_C_TRIM_DEFAULT                 = 0xD513,
        EN_NV_ID_DCXO_C2_FIX_DEFAULT                 = 0xD514,
        EN_NV_ID_XO_INIT_FREQUENCY                   = 0xD515,
        EN_NV_ID_DCXO_C_TRIM                         = 0xD516,
        EN_NV_ID_DCXO_C2_FIX                         = 0xD517,
        EN_NV_ID_DCXO_TEMP_COMP_POLY_COEF            = 0xD518,
        EN_NV_ID_DCXO_TEMP_COMP_THRESHOLD            = 0xD519,
        EN_NV_ID_DCXO_FREQ_VS_TEMP_ARRAY             = 0xD51a,
        EN_NV_ID_DCXO_TEMP_READ_PERIOD               = 0xD51b,
        EN_NV_ID_DCXO_TEMP_COMP_POLY_ALL             = 0xD51c,
        EN_NV_ID_XO_AGING_INFO                       = 0xF93F,
#endif
        EN_NV_ID_PA_MIPI_ADDED                       = 0xD520,
        EN_NV_ID_CONVERT_FACTOR                      = 0xD521,
        EN_NV_ID_AGC_TABLE                           = 0xD522,
        EN_NV_ID_RF_RX_AGC_RFIC_TBL                  = 0xD528,
        EN_NV_ID_RF_RX_AGC_GAIN_TBL                  = 0xD529,
        EN_NV_ID_TL_PA_GPIO_CTRL                     = 0xd530,
        EN_NV_APT_PDM_HOLD_NTX_STRU                  = 0xD531,
        EN_NV_ID_FEICIC_CONFIG                       = 0xD532,
        
        EN_NV_ID_NARROW_BAND                         = 0xD533,
        EN_NV_ID_NLIC                                = 0xD534,
        EN_NV_ID_NLIC_SUPPORT_BAND                   = 0xD543,
        EN_NV_ID_NLIC_DELAY                          = 0xD544,
        EN_NV_ID_NLIC_CMP_DELAY                      = 0xD545,
        EN_NV_ID_NLIC_PWR_THR                        = 0xD546,
        EN_NV_ID_LTE_BODY_SAR_WIRED_FLAG             = 0xD535,
        EN_NV_ID_MRX_AGC_TABLE                      = 0xD536,
        EN_NV_ID_CT_CAL_COMM_PARA                   = 0xD537,
        EN_NV_ID_APC_INIT_CFG_LAB                   = 0xD538,
        EN_NV_ID_APC_INIT_CFG_FAC                   = 0xD539,
        EN_NV_ID_APC_TABLE_BANK_LAB                 = 0xD53a,
        EN_NV_ID_APC_TABLE_BANK_FAC                 = 0xD53b,
        EN_NV_ID_RF_TUNER_BAND                      = 0xD53c,

        EN_NV_ID_LTE_PD_CFG_PARA                    = 0xD53d,
        EN_NV_ID_LOLEAKAGE_MRX_AGC_TABLE            = 0xD53e,
        EN_NV_ID_IQIMBALANCE_MRX_AGC_TABLE          = 0xD53f,
        /*mipi apt begin*/
        EN_NV_LPHY_MIPI_APT_PARA                     = 0xD525,

        /***============================LPC begin========================***/
        EN_NV_ID_UNIPHY_LPC_CTRL_PARA                = 0xD52A,
        EN_NV_ID_TLCOMM_LPC_CTRL_PARA                = 0xD540,
        /***============================LPC End========================***/

        EN_NV_ID_NV_PM_MODE_CHAN_PARA               = 0xd541,
        EN_NV_ID_NV_TL_PA_DCDC_CTRL                 = 0xd542,

        /*mipi apt end*/

        /*begin: add for feature k3v3+tas 2015/02/25*/
      /*  EN_NV_ID_TAS_GPIO_PARA                           = 0xD5d0,

        EN_NV_ID_TAS_DPDT_PROTECT_PARA                   = 0xD5d1,
        EN_NV_ID_LTE_TAS_BS_RSSI_THD_PARA                = 0xD5d2,
        EN_NV_ID_TAS_BLIND_SW_THD_PARA                   = 0xD5d3,
        EN_NV_ID_TAS_CLG_MODE_GPIO_MAP                   = 0xD5d4,
        EN_NV_ID_TAS_HAPPY_THD_PARA                      = 0xD5d5,
        EN_NV_ID_MAS_THR_PARA                            = 0xD5d6,*/
        /*end: add for feature tas 2015/02/25*/

        /*begin: add for band29 and band32 RX  2015/12/25*/
        EN_NV_LPHY_SINGLE_RX_BAND_CAL_PARA               = 0xD5d7,
        /*end: add for band29 and band32 RX  2015/12/25*/
        EN_NV_BT_DBB_REDUCE_STRU                         = 0xD5d8,
        EN_NV_PHY_CT_CAL_THRES_PARA                      = 0xD5d9,
 /*         EN_NV_ET_APT_MIPI_CMD_LAB_PARA                  = 0xD5D9,
        EN_NV_ET_SELECT_WORK_MODE_LAB_PARA              = 0xD5DA,

        EN_NV_ID_TAS_ACCESS_THR_PARA                     = 0xD5db,
        EN_NV_ID_LTE_TAS_DPDT_MIPI_CTRL_WORD             = 0xD5dd, */

/*        EN_NV_ID_LPHY_AGC_BASE_TABLE_B20                = 0xD61A,

        EN_NV_ID_LPHY_AGC_BASE_TABLE_B41                = 0xD6da,

        EN_NV_ID_LPHY_AGC_BASE_TABLE_B40                = 0xD65A,

        EN_NV_ID_LPHY_AGC_BASE_TABLE_B38                = 0xD69A,

        EN_NV_ID_LPHY_AGC_BASE_TABLE_B7                 = 0xD71A,*/


        EN_NV_ID_LPHY_DSP_VERSION_INFO                  = 0xD50B,
        EN_NV_ID_LPHY_DSP_CONFIG_INFO                   = 0xD50C,
        EN_NV_ID_MULTIMODE_DSP_COMMON_CONFIG_INFO       = 0xD50D,

        EN_NV_RX_BT_LEVEL_MAP_TABLE                     = 0xD3e1,




        /* modify by   for 所有band begin*/

        /* BEGIN: Added by  , 2015/5/8   PN:V7R5_AMPR*/
        /* use one ID for AMPR_NS */
        EN_NV_ID_LTE_TX_AMPR_NS                          = 0xD5d0,
        EN_NV_ID_LTE_TX_TAS_COM_PARA                     = 0xD5D1,

/*      EN_NV_ID_LTE_TX_AMPR_BNon26                      = 0xf878,
        EN_NV_ID_LTE_TX_AMPR_BNon28                      = 0xf478,
        EN_NV_ID_LTE_TX_AMPR_NS05                        = 0xd759,
        EN_NV_ID_LTE_TX_AMPR_NS09                        = 0xd75a,
        EN_NV_ID_LTE_TX_AMPR_NS07                        = 0xd751,
        EN_NV_ID_LTE_TX_AMPR_NS08                        = 0xd752,
        EN_NV_ID_LTE_TX_AMPR_NS10                        = 0xd753,
        EN_NV_ID_LTE_TX_AMPR_NS11                        = 0xd754,
        EN_NV_ID_LTE_TX_AMPR_NS16                        = 0xd755,
        EN_NV_ID_LTE_TX_AMPR_NS19                        = 0xd756,
        EN_NV_ID_LTE_TX_AMPR_NS20                        = 0xd757,
        EN_NV_ID_LTE_TX_AMPR_NS21                        = 0xd758,*/
        /* END:   Added by  , 2015/5/8 */

        EN_NV_ID_LTE_PA_TEMP_DET_CH_B20 = 0xf900,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B40 = 0xf901,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B38 = 0xf902,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B41 = 0xf903,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B7  = 0xf904,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B3  = 0xf905,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B1  = 0xf906,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B5  = 0xf907,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B8  = 0xf908,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B19 = 0xf909,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B21 = 0xf90a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B2  = 0xf90b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B4  = 0xf90c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B6  = 0xf90d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B9  = 0xf90e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B10 = 0xf90f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B11 = 0xf910,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B12 = 0xf911,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B13 = 0xf912,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B14 = 0xf913,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B17 = 0xf914,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B18 = 0xf915,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B22 = 0xf916,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B23 = 0xf917,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B24 = 0xf918,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B25 = 0xf919,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B26 = 0xf91a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B33 = 0xf91b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B34 = 0xf91c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B35 = 0xf91d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B36 = 0xf91e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B37 = 0xf91f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B42 = 0xf920,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B43 = 0xf921,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B39 = 0xf922,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B28 = 0xf923,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B128= 0xf924,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B27 = 0xf925,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B29 = 0xf926,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B30 = 0xf927,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B44 = 0xf928,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B32 = 0xf929,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B140 = 0xf92a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B107 = 0xf92b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B108 = 0xf92c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B109 = 0xf92d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B110 = 0xf92e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B111 = 0xf92f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B112 = 0xf930,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B113 = 0xf931,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B114 = 0xf932,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B115 = 0xf933,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B116 = 0xf934,

        /* modify by   for 所有band end*/

        EN_NV_ID_FACTORY_END                            = 0x2fff,





        /*
        * TDS NV ID定义，TDS NV ID段: 0xEA00~0xEA6F
        */

        /*
        * TDS非前端band无关NV, EA00~EA07
        */
        EN_NV_ID_TDS_DRX_PARA                       = 0xEA00,
        EN_NV_ID_TDS_HP_SAR                         = 0xEA01,
        EN_NV_ID_TDS_HIGH_SPEED_NV_PARA             = 0xEA02,
        EN_NV_ID_TDS_TX_POWER_NV_PARA               = 0xEA03,
        EN_NV_ID_TDS_HKADC_CHN_PARA                 = 0xEA04,
        EN_NV_ID_TDS_TEST_STUB_PARA                 = 0xEA05,

        /* TDS前端各band公共NV, EA08~EA0F */
        //EN_NV_ID_TDS_RF_HW_BASIC_INFO               = 0xEA08,
        EN_NV_ID_TDS_RF_MIPI_INIT_CMD               = 0xEA09,
        EN_NV_ID_TDS_RF_MIPI_APT_CMD                = 0xEA0A,
        EN_NV_ID_TDS_RF_STC_CFG_TIME                = 0xEA0B,
        EN_NV_ID_TDS_RF_DYN_CFG_TIME                = 0xEA0C,


        /* TDS前端BAND34 NV, EA10~EA17*/
        EN_NV_ID_TDS_RF_BAND_CONFIG_B34             = 0xEA10,
        EN_NV_ID_TDS_RF_FEM_PATH_B34                = 0xEA11,
        EN_NV_ID_TDS_RF_BAND_EXT_LNA_PATH_B34       = 0xEA12,
        EN_NV_ID_TDS_RF_VRAMP_PARA_B34              = 0xEA13,
        EN_NV_ID_TDS_RF_NOTCH_CONFIG_B34            = 0xEA14,

        /* TDS前端BAND39 NV, EA18~EA1F*/
        EN_NV_ID_TDS_RF_BAND_CONFIG_B39             = 0xEA18,
        EN_NV_ID_TDS_RF_FEM_PATH_B39                = 0xEA19,
        EN_NV_ID_TDS_RF_BAND_EXT_LNA_PATH_B39       = 0xEA1A,
        EN_NV_ID_TDS_RF_VRAMP_PARA_B39              = 0xEA1B,
        EN_NV_ID_TDS_RF_NOTCH_CONFIG_B39            = 0xEA1C,


        /*
        * TDS 共享AGC表NV段, 0xEA20~0xEA2F
        */
        EN_NV_ID_TDS_AGC_DEFAULT_GAIN                   = 0xEA20,
        EN_NV_ID_TDS_AGC_SWITCH_THR                     = 0xEA21,
        EN_NV_ID_TDS_AGC_RF_CTRL_CODE                   = 0xEA22,



        /*
        * Band34校准相关NV段, 0xEA30~0xEA47
        */
        //校准用固定NV
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B34             = 0xEA30,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B34               = 0xEA31,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B34                = 0xEA32,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B34                  = 0xEA33,
        EN_NV_ID_TDS_RX_CAL_FREQ_B34                    = 0xEA34,
        EN_NV_ID_TDS_AGC_USED_TABLE_B34                 = 0xEA35,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B34                  = 0xEA36,
        EN_NV_ID_TDS_PA_TEMP_DET_CHANNEL_STRU_B34       = 0xEA37,
        EN_NV_ID_TDS_APC_TABLE_LAB_STRU_B34             = 0xEA38,
        //校准更新NV
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B34           = 0xEA39,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B34         = 0xEA3A,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B34    = 0xEA3B,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B34                  = 0xEA3C,
        EN_NV_ID_TDS_DCOC_CAL_B34                       = 0xEA3D,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B34                    = 0xEA3E,
        EN_NV_ID_TDS_APC_TABLE_FAC_STRU_B34             = 0xEA3F,
        EN_NV_ID_TDS_AGC_CAL_RESULT_B34                 = 0xEA40,
        EN_NV_TDS_APC_CAL_TEMP_FAC_STRU_B34             = 0xEA41,
        EN_NV_ID_TDS_RX_CAL_TEMP_FAC_STRU_B34           = 0xEA42,
        EN_NV_ID_TDS_RX_FEM_IL_STRU_B34                 = 0xEA43,
        EN_NV_ID_TDS_TXIQ_CAL_PARA_STRU_B34             = 0xEA44,
        EN_NV_ID_TDS_MRX_FEM_INFO_B34                   = 0xEA45,

        /*
        * Band39校准相关NV段，0xEA48~0xEA5F
        */
        //校准用固定NV
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B39             = 0xEA48,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B39               = 0xEA49,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B39                = 0xEA4A,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B39                  = 0xEA4B,
        EN_NV_ID_TDS_RX_CAL_FREQ_B39                    = 0xEA4C,
        EN_NV_ID_TDS_AGC_USED_TABLE_B39                 = 0xEA4D,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B39                  = 0xEA4E,
        EN_NV_ID_TDS_PA_TEMP_DET_CHANNEL_STRU_B39       = 0xEA4F,
        EN_NV_ID_TDS_APC_TABLE_LAB_STRU_B39             = 0xEA50,
        //校准更新NV
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B39           = 0xEA51,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B39         = 0xEA52,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B39    = 0xEA53,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B39                  = 0xEA54,
        EN_NV_ID_TDS_DCOC_CAL_B39                       = 0xEA55,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B39                    = 0xEA56,
        EN_NV_ID_TDS_APC_TABLE_FAC_STRU_B39             = 0xEA57,
        EN_NV_ID_TDS_AGC_CAL_RESULT_B39                 = 0xEA58,
        EN_NV_TDS_APC_CAL_TEMP_FAC_STRU_B39             = 0xEA59,
        EN_NV_ID_TDS_RX_CAL_TEMP_FAC_STRU_B39           = 0xEA5A,
        EN_NV_ID_TDS_RX_FEM_IL_STRU_B39                 = 0xEA5B,
        EN_NV_ID_TDS_TXIQ_CAL_PARA_STRU_B39             = 0xEA5C,
        EN_NV_ID_TDS_MRX_FEM_INFO_B39                   = 0xEA5D,


        /*
        * TDS TAS NV段, 0xEA60~0xEA6F, 后续建议TAS统一设计一个NV以节省ID
        */
        EN_NV_ID_TDS_TAS_STRU_DEFAULT                   = 0xEA60,
        EN_NV_ID_TDS_TAS_RF_STRU_DEFAULT                = 0xEA61,
        EN_NV_ID_TDS_TAS_SEARCH_STRU_DEFAULT            = 0xEA62,

        EN_NV_ID_TDS_TAS_EXTRA_MODEM_GPIO               = 0xEA63,
        EN_NV_ID_TDS_TAS_DPDT_PROTECT_PARA              = 0xEA64,

        EN_NV_ID_TDS_TAS_HAPPY_STRU_DEFAULT             = 0xEA65,

        EN_NV_ID_TDS_TAS_BLIND_STRU_DEFAULT             = 0xEA66,
        EN_NV_ID_TDS_MAS_PARA_STRU_DEFAULT              = 0xEA67,
        EN_NV_ID_TL_MAS_GPIO_STRU_DEFAULT               = 0xEA68,
        EN_NV_ID_TDS_TAS_DPDT_MIPI_CTRL_WORD            = 0xEA69,









    };
#elif (!defined(TL_PHY_ASIC_K3V5))
    enum NV_TLPHY_ITEM_ID_ENUM
    {
        EN_NV_ID_AGC_PARA                               = 0xD3CD,

        EN_NV_ID_RFE_CONFIG_TIME                        = 0xD3cf,

        EN_NV_ID_ADC_OPEN_TIME                          = 0xD400,
        EN_NV_ID_RFIC_T1                                = 0xD401,
        EN_NV_ID_RFIC_T2                                = 0xD402,
        EN_NV_ID_RFIC_T3                                = 0xD403,
        EN_NV_ID_RFIC_T4                                = 0xD404,
        EN_NV_ID_RFIC_T7                                = 0xD405,
        EN_NV_ID_RFIC_T8                                = 0xD406,
        EN_NV_ID_RFIC_T9                                = 0xD407,
        EN_NV_ID_RFIC_T10                               = 0xD408,
        EN_NV_ID_PA_OPEN_TIME                           = 0xD409,

        /* modify by   for 所有band begin*/

        EN_NV_ID_FTM_CAND_CELL_LIST_B19                 = 0xD900,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B19            = 0xD901,
        EN_NV_ID_LTE_TX_CAL_LIST_B19            = 0xD902,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B19          = 0xD903,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B19          = 0xD904,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B19                = 0xD905,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B19               = 0xD906,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B19               = 0xD907,
        EN_NV_ID_LTE_TX_TAS_B19                         = 0xD908,

        EN_NV_ID_TEMP_SENSOR_TABLE_B19                  = 0xD90f,
        EN_NV_ID_HI6360_AGC_PARA_B19                    = 0xD918,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B19        = 0xD91b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B19        = 0xD91c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B19                  = 0xD91d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B19                   = 0xD91e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B19              = 0xD91f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B19         = 0xD920,
        EN_NV_ID_LTE_TX_ATTEN_B19                       = 0xD921,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B19             = 0xD922,
        EN_NV_ID_TX_FILTER_CMP_STRU_B19                 = 0xD923,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B19               = 0xD924,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B19       = 0xD925,

        EN_NV_ID_LTE_TX_APT_PARA_B19                    = 0xD926,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B19            = 0xD927,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B19             = 0xD928,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B19             = 0xD929,
        EN_NV_ID_LTE_TX_MPR_B19                         = 0xD92a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B19                     = 0xD92b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B19                        = 0xD92c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B19         = 0xD92d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B19                    = 0xD92e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B19                    = 0xD92f,

        /*add for V9R1_6361 Begin*/
        EN_NV_ID_LTE_TX_ANT_SEL_B19                      = 0Xd930,
        EN_NV_ID_LTE_TX_BODY_SAR_B19                     = 0xD931,
        EN_NV_ID_TX_PA_TEMP_COMP_B19                    = 0xD932,
        EN_NV_ID_EXT_LNA_AGC_PARA_B19                      = 0xD933,
        EN_NV_ID_CALB_AGC_B19                              = 0xD934,
        //EN_NV_ID_TX_ATTEN_TABLE_B19                     = 0xD933,
        //EN_NV_ID_POWERDET_VOLTAGE_B19                   = 0xD934,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B19                = 0xD935,
        EN_NV_ID_LTE_TX_PD_PARA_B19                      = 0xD936,
        EN_NV_ID_TX_ET_BAND_PARA_B19                      = 0xD937,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B19           = 0xD938,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B19               = 0xD939,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B19                   = 0xD93a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B19                   = 0xD93b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B19                         = 0xD93c,
        /* END:   Added , 2015/10/29 */
        /*hrl*/
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B19              = 0xD93d,
        EN_NV_ID_LTE_TX_MPR_64QAM_B19                    = 0xD93f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B21                 = 0xD940,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B21            = 0xD941,
        EN_NV_ID_LTE_TX_CAL_LIST_B21            = 0xD942,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B21          = 0xD943,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B21          = 0xD944,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B21                = 0xD945,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B21               = 0xD946,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B21               = 0xD947,
        EN_NV_ID_LTE_TX_TAS_B21                         = 0xD948,

        EN_NV_ID_TEMP_SENSOR_TABLE_B21                  = 0xD94f,
        EN_NV_ID_HI6360_AGC_PARA_B21                    = 0xD958,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B21        = 0xD95b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B21        = 0xD95c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B21                  = 0xD95d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B21                   = 0xD95e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B21              = 0xD95f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B21         = 0xD960,
        EN_NV_ID_LTE_TX_ATTEN_B21                       = 0xD961,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B21             = 0xD962,
        EN_NV_ID_TX_FILTER_CMP_STRU_B21          = 0xD963,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B21           = 0xD964,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B21           = 0xD965,

        EN_NV_ID_LTE_TX_APT_PARA_B21                    = 0xD966,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B21            = 0xD967,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B21             = 0xD968,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B21             = 0xD969,
        EN_NV_ID_LTE_TX_MPR_B21                         = 0xD96a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B21                     = 0xD96b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B21                        = 0xD96c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B21         = 0xD96d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B21                   = 0xD96e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B21                   = 0xD96f,
        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B21                     = 0xD970,
        EN_NV_ID_LTE_TX_BODY_SAR_B21                      = 0xD971,
        EN_NV_ID_TX_PA_TEMP_COMP_B21                      = 0xD972,
        EN_NV_ID_EXT_LNA_AGC_PARA_B21                     = 0xD973,
        EN_NV_ID_CALB_AGC_B21                             = 0xD974,

        //EN_NV_ID_TX_ATTEN_TABLE_B21                        = 0xD973,
        //EN_NV_ID_POWERDET_VOLTAGE_B21                        = 0xD974,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B21               = 0xD975,
        EN_NV_ID_LTE_TX_PD_PARA_B21                     = 0xD976,
        EN_NV_ID_TX_ET_BAND_PARA_B21                    = 0xD977,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B21          = 0xD978,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B21              = 0xD979,
        /* BEGIN: Added by  , 2015/5/14    V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B21                   = 0xD97a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B21                   = 0xD97b,
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B21                         = 0xD97c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B21             = 0xD97d,
        /* END:   Added by  , 2015/5/14 */
        EN_NV_ID_LTE_TX_MPR_64QAM_B21                   = 0xD97f,
        EN_NV_ID_FTM_CAND_CELL_LIST_B2                  = 0xD980,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B2             = 0xD981,
        EN_NV_ID_LTE_TX_CAL_LIST_B2                     = 0xD982,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B2           = 0xD983,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B2           = 0xD984,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B2                 = 0xD985,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B2                = 0xD986,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B2                = 0xD987,
        EN_NV_ID_LTE_TX_TAS_B2                          = 0xD988,

        EN_NV_ID_TEMP_SENSOR_TABLE_B2                   = 0xD98f,
        EN_NV_ID_HI6360_AGC_PARA_B2                     = 0xD998,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B2         = 0xD99b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B2         = 0xD99c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B2                   = 0xD99d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B2                    = 0xD99e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B2               = 0xD99f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B2          = 0xD9a0,
        EN_NV_ID_LTE_TX_ATTEN_B2                        = 0xD9a1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B2              = 0xD9a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B2                  = 0xD9a3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B2            = 0xD9a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B2            = 0xD9a5,

        EN_NV_ID_LTE_TX_APT_PARA_B2                     = 0xD9a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B2             = 0xD9a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B2              = 0xD9a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B2              = 0xD9a9,
        EN_NV_ID_LTE_TX_MPR_B2                          = 0xD9aa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B2                      = 0xD9ab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B2                         = 0xD9ac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B2         = 0xD9ad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B2                   = 0xD9ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B2                   = 0xD9af,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B2                     = 0xD9b0,
        EN_NV_ID_LTE_TX_BODY_SAR_B2                      = 0xD9b1,
        EN_NV_ID_TX_PA_TEMP_COMP_B2                      = 0xD9b2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B2                     = 0xD9b3,
        EN_NV_ID_CALB_AGC_B2                             = 0xD9b4,
        //EN_NV_ID_TX_ATTEN_TABLE_B2                        = 0xD9b3,
        //EN_NV_ID_POWERDET_VOLTAGE_B2                        = 0xD9b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B2                = 0xD9b5,
        EN_NV_ID_LTE_TX_PD_PARA_B2                      = 0xD9b6,
        EN_NV_ID_TX_ET_BAND_PARA_B2                     = 0xD9b7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B2           = 0xD9b8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B2               = 0xD9b9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B2                    = 0xD9ba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B2                    = 0xD9bb,
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B2                          = 0xD9bc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B2              = 0xD9bd,
        EN_NV_ID_LTE_TX_MPR_64QAM_B2                    = 0xD9bf,

        /* END:   Added by  , 2015/5/14 */
        EN_NV_ID_FTM_CAND_CELL_LIST_B4                  = 0xD9c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B4             = 0xD9c1,
        EN_NV_ID_LTE_TX_CAL_LIST_B4                     = 0xD9c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B4           = 0xD9c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B4           = 0xD9c4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B4                 = 0xD9c5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B4                = 0xD9c6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B4                = 0xD9c7,
        EN_NV_ID_LTE_TX_TAS_B4                          = 0xD9c8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B4                   = 0xD9cf,
        EN_NV_ID_HI6360_AGC_PARA_B4                     = 0xD9d8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B4         = 0xD9db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B4         = 0xD9dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B4                   = 0xD9dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B4                    = 0xD9de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B4               = 0xD9df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B4          = 0xD9e0,
        EN_NV_ID_LTE_TX_ATTEN_B4                        = 0xD9e1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B4              = 0xD9e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B4                  = 0xD9e3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B4          = 0xD9e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B4        = 0xD9e5,

        EN_NV_ID_LTE_TX_APT_PARA_B4                     = 0xD9e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B4             = 0xD9e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B4              = 0xD9e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B4              = 0xD9e9,
        EN_NV_ID_LTE_TX_MPR_B4                          = 0xD9ea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B4                      = 0xD9eb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B4                         = 0xD9ec,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B4         = 0xD9ed,

        EN_NV_ID_LTE_TX_AMPR_NS05_B4                   = 0xD9ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B4                   = 0xD9ef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B4                     = 0xD9f0,
        EN_NV_ID_LTE_TX_BODY_SAR_B4                     = 0xD9f1,
        EN_NV_ID_TX_PA_TEMP_COMP_B4                     = 0xD9f2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B4                    = 0xD9f3,
        EN_NV_ID_CALB_AGC_B4                            = 0xD9f4,
        //EN_NV_ID_TX_ATTEN_TABLE_B4                      = 0xD9f3,
        //EN_NV_ID_POWERDET_VOLTAGE_B4                  = 0xD9f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B4                = 0xD9f5,
        EN_NV_ID_LTE_TX_PD_PARA_B4                      = 0xD9f6,
        EN_NV_ID_TX_ET_BAND_PARA_B4                     = 0xD9f7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B4           = 0xD9f8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B4               = 0xD9f9,
        /* BEGIN: Added by  , 2015/5/14   V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B4                    = 0xD9fa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B4                    = 0xD9fb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B4                         = 0xD9fc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B4              = 0xD9fd,
        EN_NV_ID_LTE_TX_MPR_64QAM_B4                    = 0xD9ff,

        EN_NV_ID_FTM_CAND_CELL_LIST_B6                  = 0xDa00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B6             = 0xDa01,
        EN_NV_ID_LTE_TX_CAL_LIST_B6             = 0xDa02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B6           = 0xDa03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B6           = 0xDa04,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B6                 = 0xDa05,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B6                = 0xDa06,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B6                = 0xDa07,
        EN_NV_ID_LTE_TX_TAS_B6                          = 0xDa08,

        EN_NV_ID_TEMP_SENSOR_TABLE_B6                   = 0xDa0f,
        EN_NV_ID_HI6360_AGC_PARA_B6                     = 0xDa18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B6         = 0xDa1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B6         = 0xDa1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B6                   = 0xDa1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B6                    = 0xDa1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B6               = 0xDa1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B6          = 0xDa20,
        EN_NV_ID_LTE_TX_ATTEN_B6                        = 0xDa21,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B6           = 0xDa22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B6           = 0xDa23,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B6            = 0xDa24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B6            = 0xDa25,

        EN_NV_ID_LTE_TX_APT_PARA_B6                     = 0xDa26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B6             = 0xDa27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B6              = 0xDa28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B6              = 0xDa29,
        EN_NV_ID_LTE_TX_MPR_B6                          = 0xDa2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B6                      = 0xDa2b,
        /* add  tx end */

        /* add by  AMPR_REDUDTION begin */
        EN_NV_ID_LTE_TX_AMPR_B6                         = 0xDa2c,
        /* add by  AMPR_REDUDTION END */
        EN_NV_ID_LTE_OTHER_COMP_B6         = 0xDa2d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B6                   = 0xDa2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B6                   = 0xDa2f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B6                      = 0xDa30,
        EN_NV_ID_LTE_TX_BODY_SAR_B6                     = 0xDa31,
        EN_NV_ID_TX_PA_TEMP_COMP_B6                     = 0xDa32,
        EN_NV_ID_EXT_LNA_AGC_PARA_B6                    = 0xDa33,
        EN_NV_ID_CALB_AGC_B6                            = 0xDa34,
        //EN_NV_ID_TX_ATTEN_TABLE_B6                      = 0xDa33,
        //EN_NV_ID_POWERDET_VOLTAGE_B6                  = 0xDa34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B6                = 0xDa35,
        EN_NV_ID_LTE_TX_PD_PARA_B6                      = 0xDa36,
        EN_NV_ID_TX_ET_BAND_PARA_B6                     = 0xDa37,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B6           = 0xDa38,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B6               = 0xDa39,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B6                    = 0xDa3a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B6                    = 0xDa3b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B6                          = 0xDa3c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B6              = 0xDa3d,
        EN_NV_ID_LTE_TX_MPR_64QAM_B6                    = 0xDa3f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B9                  = 0xDa40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B9             = 0xDa41,
        EN_NV_ID_LTE_TX_CAL_LIST_B9                     = 0xDa42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B9           = 0xDa43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B9           = 0xDa44,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B9                 = 0xDa45,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B9                = 0xDa46,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B9                = 0xDa47,
        EN_NV_ID_LTE_TX_TAS_B9                          = 0xDa48,

        EN_NV_ID_TEMP_SENSOR_TABLE_B9                   = 0xDa4f,
        EN_NV_ID_HI6360_AGC_PARA_B9                     = 0xDa58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B9         = 0xDa5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B9         = 0xDa5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B9                   = 0xDa5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B9                    = 0xDa5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B9               = 0xDa5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B9          = 0xDa60,
        EN_NV_ID_LTE_TX_ATTEN_B9                        = 0xDa61,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B9         = 0xDa62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B9                  = 0xDa63,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B9          = 0xDa64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B9        = 0xDa65,

        EN_NV_ID_LTE_TX_APT_PARA_B9                     = 0xDa66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B9             = 0xDa67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B9              = 0xDa68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B9              = 0xDa69,
        EN_NV_ID_LTE_TX_MPR_B9                          = 0xDa6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B9                      = 0xDa6b,
        /* add  tx end */

        /* add by  AMPR_REDUDTION begin */
        EN_NV_ID_LTE_TX_AMPR_B9                         = 0xDa6c,
        /* add by  AMPR_REDUDTION END */
        EN_NV_ID_LTE_OTHER_COMP_B9         = 0xDa6d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B9                   = 0xDa6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B9                   = 0xDa6f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B9                     = 0xDa70,
        EN_NV_ID_LTE_TX_BODY_SAR_B9                     = 0xDa71,
        EN_NV_ID_TX_PA_TEMP_COMP_B9                     = 0xDa72,
        EN_NV_ID_EXT_LNA_AGC_PARA_B9                    = 0xDa73,
        EN_NV_ID_CALB_AGC_B9                            = 0xDa74,
        //EN_NV_ID_TX_ATTEN_TABLE_B9                      = 0xDa73,
        //EN_NV_ID_POWERDET_VOLTAGE_B9                  = 0xDa74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B9                = 0xDa75,
        EN_NV_ID_LTE_TX_PD_PARA_B9                      = 0xDa76,
        EN_NV_ID_TX_ET_BAND_PARA_B9                     = 0xDa77,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B9           = 0xDa78,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B9               = 0xDa79,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B9                    = 0xDa7a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B9                    = 0xDa7b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B9                          = 0xDa7c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B9              = 0xDa7d,
        EN_NV_ID_LTE_TX_MPR_64QAM_B9                    = 0xDa7f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B10                 = 0xDa80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B10            = 0xDa81,
        EN_NV_ID_LTE_TX_CAL_LIST_B10                    = 0xDa82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B10          = 0xDa83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B10         = 0xDa84,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B10                = 0xDa85,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B10               = 0xDa86,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B10               = 0xDa87,
        EN_NV_ID_LTE_TX_TAS_B10                         = 0xDa88,

        EN_NV_ID_TEMP_SENSOR_TABLE_B10                  = 0xDa8f,
        EN_NV_ID_HI6360_AGC_PARA_B10                    = 0xDa98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B10        = 0xDa9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B10       = 0xDa9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B10                  = 0xDa9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B10                   = 0xDa9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B10              = 0xDa9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B10         = 0xDaa0,
        EN_NV_ID_LTE_TX_ATTEN_B10                       = 0xDaa1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B10             = 0xDaa2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B10                 = 0xDaa3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B10         = 0xDaa4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B10       = 0xDaa5,

        EN_NV_ID_LTE_TX_APT_PARA_B10                    = 0xDaa6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B10            = 0xDaa7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B10             = 0xDaa8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B10             = 0xDaa9,
        EN_NV_ID_LTE_TX_MPR_B10                         = 0xDaaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B10                     = 0xDaab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B10                        = 0xDaac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B10         = 0xDaad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B10                   = 0xDaae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B10                   = 0xDaaf,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B10                    = 0xDab0,
        EN_NV_ID_LTE_TX_BODY_SAR_B10                    = 0xDab1,
        EN_NV_ID_TX_PA_TEMP_COMP_B10                    = 0xDab2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B10                   = 0xDab3,
        EN_NV_ID_CALB_AGC_B10                           = 0xDab4,
        //EN_NV_ID_TX_ATTEN_TABLE_B10                     = 0xDab3,
        //EN_NV_ID_POWERDET_VOLTAGE_B10                 = 0xDab4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B10               = 0xDab5,
        EN_NV_ID_LTE_TX_PD_PARA_B10                     = 0xDab6,
        EN_NV_ID_TX_ET_BAND_PARA_B10                    = 0xDab7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B10          = 0xDab8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B10              = 0xDab9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B10                   = 0xDaba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B10                   = 0xDabb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B10                         = 0xDabc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B10              = 0xDabd,
        EN_NV_ID_LTE_TX_MPR_64QAM_B10                    = 0xDabf,

        EN_NV_ID_FTM_CAND_CELL_LIST_B11                 = 0xDac0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B11            = 0xDac1,
        EN_NV_ID_LTE_TX_CAL_LIST_B11                    = 0xDac2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B11          = 0xDac3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B11         = 0xDac4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B11                = 0xDac5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B11               = 0xDac6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B11               = 0xDac7,
        EN_NV_ID_LTE_TX_TAS_B11                         = 0xDac8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B11                  = 0xDacf,
        EN_NV_ID_HI6360_AGC_PARA_B11                    = 0xDad8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B11        = 0xDadb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B11       = 0xDadc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B11                  = 0xDadd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B11                   = 0xDade,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B11              = 0xDadf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B11         = 0xDae0,
        EN_NV_ID_LTE_TX_ATTEN_B11                       = 0xDae1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B11             = 0xDae2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B11                 = 0xDae3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B11         = 0xDae4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B11       = 0xDae5,

        EN_NV_ID_LTE_TX_APT_PARA_B11                    = 0xDae6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B11            = 0xDae7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B11             = 0xDae8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B11             = 0xDae9,
        EN_NV_ID_LTE_TX_MPR_B11                         = 0xDaea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B11                     = 0xDaeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B11                        = 0xDaec,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B11         = 0xDaed,

        EN_NV_ID_LTE_TX_AMPR_NS05_B11                   = 0xDaee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B11                   = 0xDaef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B11                    = 0xDaf0,
        EN_NV_ID_LTE_TX_BODY_SAR_B11                    = 0xDaf1,
        EN_NV_ID_TX_PA_TEMP_COMP_B11                    = 0xDaf2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B11                   = 0xDaf3,
        EN_NV_ID_CALB_AGC_B11                           = 0xDaf4,
        //EN_NV_ID_TX_ATTEN_TABLE_B11                     = 0xDaf3,
        //EN_NV_ID_POWERDET_VOLTAGE_B11                 = 0xDaf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B11               = 0xDaf5,
        EN_NV_ID_LTE_TX_PD_PARA_B11                     = 0xDaf6,
        EN_NV_ID_TX_ET_BAND_PARA_B11                    = 0xDaf7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B11          = 0xDaf8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B11              = 0xDaf9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B11                   = 0xDafa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B11                   = 0xDafb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B11                         = 0xDafc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B11              = 0xDafd,
        EN_NV_ID_LTE_TX_MPR_64QAM_B11                    = 0xDaff,

        EN_NV_ID_FTM_CAND_CELL_LIST_B12                 = 0xDb00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B12            = 0xDb01,
        EN_NV_ID_LTE_TX_CAL_LIST_B12                    = 0xDb02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B12          = 0xDb03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B12         = 0xDb04,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B12                = 0xDb05,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B12               = 0xDb06,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B12               = 0xDb07,
        EN_NV_ID_LTE_TX_TAS_B12                         = 0xDb08,

        EN_NV_ID_TEMP_SENSOR_TABLE_B12                  = 0xDb0f,
        EN_NV_ID_HI6360_AGC_PARA_B12                    = 0xDb18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B12        = 0xDb1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B12       = 0xDb1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B12                  = 0xDb1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B12                   = 0xDb1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B12              = 0xDb1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B12         = 0xDb20,
        EN_NV_ID_LTE_TX_ATTEN_B12                       = 0xDb21,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B12             = 0xDb22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B12                 = 0xDb23,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B12         = 0xDb24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B12       = 0xDb25,

        EN_NV_ID_LTE_TX_APT_PARA_B12                    = 0xDb26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B12            = 0xDb27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B12             = 0xDb28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B12             = 0xDb29,
        EN_NV_ID_LTE_TX_MPR_B12                         = 0xDb2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B12                     = 0xDb2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B12                        = 0xDb2c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B12         = 0xDb2d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B12                   = 0xDb2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B12                   = 0xDb2f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B12                    = 0xDb30,
        EN_NV_ID_LTE_TX_BODY_SAR_B12                    = 0xDb31,
        EN_NV_ID_TX_PA_TEMP_COMP_B12                    = 0xDb32,
        EN_NV_ID_EXT_LNA_AGC_PARA_B12                   = 0xDb33,
        EN_NV_ID_CALB_AGC_B12                           = 0xDb34,
        //EN_NV_ID_TX_ATTEN_TABLE_B12                     = 0xDb33,
        //EN_NV_ID_POWERDET_VOLTAGE_B12                 = 0xDb34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B12               = 0xDb35,
        EN_NV_ID_LTE_TX_PD_PARA_B12                     = 0xDb36,
        EN_NV_ID_TX_ET_BAND_PARA_B12                    = 0xDb37,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B12          = 0xDb38,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B12              = 0xDb39,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B12                   = 0xDb3a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B12                   = 0xDb3b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B12                         = 0xDb3c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B12              = 0xDb3d,
        EN_NV_ID_LTE_TX_MPR_64QAM_B12                    = 0xDb3f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B13                 = 0xDb40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B13            = 0xDb41,
        EN_NV_ID_LTE_TX_CAL_LIST_B13                    = 0xDb42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B13          = 0xDb43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B13         = 0xDb44,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B13                = 0xDb45,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B13               = 0xDb46,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B13               = 0xDb47,
        EN_NV_ID_LTE_TX_TAS_B13                         = 0xDb48,

        EN_NV_ID_TEMP_SENSOR_TABLE_B13                  = 0xDb4f,
        EN_NV_ID_HI6360_AGC_PARA_B13                    = 0xDb58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B13        = 0xDb5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B13       = 0xDb5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B13                  = 0xDb5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B13                  = 0xDb5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B13             = 0xDb5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B13         = 0xDb60,
        EN_NV_ID_LTE_TX_ATTEN_B13                       = 0xDb61,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B13             = 0xDb62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B13                 = 0xDb63,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B13         = 0xDb64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B13       = 0xDb65,

        EN_NV_ID_LTE_TX_APT_PARA_B13                    = 0xDb66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B13            = 0xDb67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B13             = 0xDb68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B13             = 0xDb69,
        EN_NV_ID_LTE_TX_MPR_B13                         = 0xDb6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B13                     = 0xDb6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B13                        = 0xDb6c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B13         = 0xDb6d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B13                   = 0xDb6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B13                   = 0xDb6f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B13                    = 0xDb70,
        EN_NV_ID_LTE_TX_BODY_SAR_B13                    = 0xDb71,
        EN_NV_ID_TX_PA_TEMP_COMP_B13                    = 0xDb72,
        EN_NV_ID_EXT_LNA_AGC_PARA_B13                   = 0xDb73,
        EN_NV_ID_CALB_AGC_B13                           = 0xDb74,
        //EN_NV_ID_TX_ATTEN_TABLE_B13                     = 0xDb73,
        //EN_NV_ID_POWERDET_VOLTAGE_B13                 = 0xDb74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B13               = 0xDb75,
        EN_NV_ID_LTE_TX_PD_PARA_B13                     = 0xDb76,
        EN_NV_ID_TX_ET_BAND_PARA_B13                    = 0xDb77,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B13          = 0xDb78,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B13              = 0xDb79,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B13                   = 0xDb7a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B13                   = 0xDb7b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B13                         = 0xDb7c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B13              = 0xDb7d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B13                    = 0xDb7f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B14                 = 0xDb80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B14            = 0xDb81,
        EN_NV_ID_LTE_TX_CAL_LIST_B14                    = 0xDb82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B14          = 0xDb83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B14         = 0xDb84,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B14                = 0xDb85,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B14               = 0xDb86,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B14               = 0xDb87,
        EN_NV_ID_LTE_TX_TAS_B14                         = 0xDb88,

        EN_NV_ID_TEMP_SENSOR_TABLE_B14                  = 0xDb8f,
        EN_NV_ID_HI6360_AGC_PARA_B14                    = 0xDb98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B14        = 0xDb9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B14       = 0xDb9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B14                  = 0xDb9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B14                  = 0xDb9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B14             = 0xDb9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B14         = 0xDba0,
        EN_NV_ID_LTE_TX_ATTEN_B14                       = 0xDba1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B14             = 0xDba2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B14                 = 0xDba3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B14         = 0xDba4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B14       = 0xDba5,

        EN_NV_ID_LTE_TX_APT_PARA_B14                    = 0xDba6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B14            = 0xDba7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B14             = 0xDba8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B14             = 0xDba9,
        EN_NV_ID_LTE_TX_MPR_B14                         = 0xDbaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B14                     = 0xDbab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B14                        = 0xDbac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B14         = 0xDbad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B14                   = 0xDbae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B14                   = 0xDbaf,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B14                    = 0xDbb0,
        EN_NV_ID_LTE_TX_BODY_SAR_B14                    = 0xDbb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B14                    = 0xDbb2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B14                   = 0xDbb3,
        EN_NV_ID_CALB_AGC_B14                           = 0xDbb4,
        //EN_NV_ID_TX_ATTEN_TABLE_B14                     = 0xDbb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B14                 = 0xDbb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B14               = 0xDbb5,
        EN_NV_ID_LTE_TX_PD_PARA_B14                     = 0xDbb6,
        EN_NV_ID_TX_ET_BAND_PARA_B14                    = 0xDbb7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B14          = 0xDbb8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B14              = 0xDbb9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B14                   = 0xDbba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B14                   = 0xDbbb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B14                         = 0xDbbc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B14              = 0xDbbd,
        EN_NV_ID_LTE_TX_MPR_64QAM_B14                    = 0xDbbf,

        EN_NV_ID_FTM_CAND_CELL_LIST_B17                 = 0xDbc0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B17            = 0xDbc1,
        EN_NV_ID_LTE_TX_CAL_LIST_B17                    = 0xDbc2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B17          = 0xDbc3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B17         = 0xDbc4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B17                = 0xDbc5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B17               = 0xDbc6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B17               = 0xDbc7,
        EN_NV_ID_LTE_TX_TAS_B17                         = 0xDbc8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B17                  = 0xDbcf,
        EN_NV_ID_HI6360_AGC_PARA_B17                    = 0xDbd8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B17        = 0xDbdb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B17       = 0xDbdc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B17                  = 0xDbdd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B17                  = 0xDbde,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B17             = 0xDbdf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B17         = 0xDbe0,
        EN_NV_ID_LTE_TX_ATTEN_B17                       = 0xDbe1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B17             = 0xDbe2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B17                 = 0xDbe3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B17         = 0xDbe4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B17       = 0xDbe5,

        EN_NV_ID_LTE_TX_APT_PARA_B17                    = 0xDbe6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B17            = 0xDbe7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B17             = 0xDbe8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B17             = 0xDbe9,
        EN_NV_ID_LTE_TX_MPR_B17                         = 0xDbea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B17                     = 0xDbeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B17                        = 0xDbec,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B17         = 0xDbed,

        EN_NV_ID_LTE_TX_AMPR_NS05_B17                   = 0xDbee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B17                   = 0xDbef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B17                    = 0xDbf0,
        EN_NV_ID_LTE_TX_BODY_SAR_B17                    = 0xDbf1,
        EN_NV_ID_TX_PA_TEMP_COMP_B17                    = 0xDbf2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B17                   = 0xDbf3,
        EN_NV_ID_CALB_AGC_B17                           = 0xDbf4,
        //EN_NV_ID_TX_ATTEN_TABLE_B17                        = 0xDbf3,
        //EN_NV_ID_POWERDET_VOLTAGE_B17                 = 0xDbf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B17               = 0xDbf5,
        EN_NV_ID_LTE_TX_PD_PARA_B17                     = 0xDbf6,
        EN_NV_ID_TX_ET_BAND_PARA_B17                    = 0xDbf7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B17          = 0xDbf8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B17              = 0xDbf9,
         /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B17                   = 0xDbfa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B17                   = 0xDbfb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B17                         = 0xDbfc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B17              = 0xDbfd,
        EN_NV_ID_LTE_TX_MPR_64QAM_B17                    = 0xDbff,

        EN_NV_ID_FTM_CAND_CELL_LIST_B18                 = 0xDc00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B18            = 0xDc01,
        EN_NV_ID_LTE_TX_CAL_LIST_B18                    = 0xDc02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B18          = 0xDc03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B18         = 0xDc04,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B18                = 0xDc05,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B18               = 0xDc06,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B18               = 0xDc07,
        EN_NV_ID_LTE_TX_TAS_B18                         = 0xDc08,

        EN_NV_ID_TEMP_SENSOR_TABLE_B18                  = 0xDc0f,
        EN_NV_ID_HI6360_AGC_PARA_B18                    = 0xDc18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B18        = 0xDc1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B18       = 0xDc1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B18                  = 0xDc1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B18                  = 0xDc1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B18             = 0xDc1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B18         = 0xDc20,
        EN_NV_ID_LTE_TX_ATTEN_B18                       = 0xDc21,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B18             = 0xDc22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B18                 = 0xDc23,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B18         = 0xDc24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B18       = 0xDc25,

        EN_NV_ID_LTE_TX_APT_PARA_B18                    = 0xDc26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B18            = 0xDc27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B18             = 0xDc28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B18             = 0xDc29,
        EN_NV_ID_LTE_TX_MPR_B18                         = 0xDc2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B18                     = 0xDc2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B18                        = 0xDc2c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B18         = 0xDc2d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B18                   = 0xDc2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B18                   = 0xDc2f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B18                    = 0xDc30,
        EN_NV_ID_LTE_TX_BODY_SAR_B18                    = 0xDc31,
        EN_NV_ID_TX_PA_TEMP_COMP_B18                    = 0xDc32,
        EN_NV_ID_EXT_LNA_AGC_PARA_B18                   = 0xDc33,
        EN_NV_ID_CALB_AGC_B18                           = 0xDc34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B18               = 0xDc35,
        EN_NV_ID_LTE_TX_PD_PARA_B18                     = 0xDc36,
        EN_NV_ID_TX_ET_BAND_PARA_B18                    = 0xDc37,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B18          = 0xDc38,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B18              = 0xDc39,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B18                   = 0xDc3a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B18                   = 0xDc3b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B18                         = 0xDc3c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B18              = 0xDc3d,
        EN_NV_ID_LTE_TX_MPR_64QAM_B18                    = 0xDc3f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B22                 = 0xDc40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B22            = 0xDc41,
        EN_NV_ID_LTE_TX_CAL_LIST_B22                    = 0xDc42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B22          = 0xDc43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B22         = 0xDc44,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B22                = 0xDc45,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B22               = 0xDc46,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B22               = 0xDc47,
        EN_NV_ID_LTE_TX_TAS_B22                         = 0xDc48,

        EN_NV_ID_TEMP_SENSOR_TABLE_B22                  = 0xDc4f,
        EN_NV_ID_HI6360_AGC_PARA_B22                    = 0xDc58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B22        = 0xDc5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B22       = 0xDc5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B22                  = 0xDc5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B22                  = 0xDc5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B22             = 0xDc5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B22         = 0xDc60,
        EN_NV_ID_LTE_TX_ATTEN_B22                       = 0xDc61,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B22        = 0xDc62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B22                 = 0xDc63,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B22           = 0xDc64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B22           = 0xDc65,

        EN_NV_ID_LTE_TX_APT_PARA_B22                    = 0xDc66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B22            = 0xDc67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B22             = 0xDc68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B22             = 0xDc69,
        EN_NV_ID_LTE_TX_MPR_B22                         = 0xDc6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B22                     = 0xDc6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B22                        = 0xDc6c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B22         = 0xDc6d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B22                   = 0xDc6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B22                   = 0xDc6f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B22                    = 0xDc70,
        EN_NV_ID_LTE_TX_BODY_SAR_B22                    = 0xDc71,
        EN_NV_ID_TX_PA_TEMP_COMP_B22                    = 0xDc72,
        EN_NV_ID_EXT_LNA_AGC_PARA_B22                   = 0xDc73,
        EN_NV_ID_CALB_AGC_B22                           = 0xDc74,
        //EN_NV_ID_TX_ATTEN_TABLE_B22                        = 0xDc73,
        //EN_NV_ID_POWERDET_VOLTAGE_B22                        = 0xDc74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B22               = 0xDc75,
        EN_NV_ID_LTE_TX_PD_PARA_B22                     = 0xDC36,
        EN_NV_ID_TX_ET_BAND_PARA_B22                    = 0xDc77,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B22          = 0xDc78,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B22              = 0xDc79,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B22                   = 0xDc7a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B22                   = 0xDc7b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B22                         = 0xDc7c,
        /* END:   Added , 2015/10/29 */
         EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B22              = 0xDc7d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B22                    = 0xDc7f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B23                 = 0xDc80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B23            = 0xDc81,
        EN_NV_ID_LTE_TX_CAL_LIST_B23            = 0xDc82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B23          = 0xDc83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B23         = 0xDc84,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B23                = 0xDc85,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B23               = 0xDc86,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B23               = 0xDc87,
        EN_NV_ID_LTE_TX_TAS_B23                         = 0xDc88,

        EN_NV_ID_TEMP_SENSOR_TABLE_B23                  = 0xDc8f,
        EN_NV_ID_HI6360_AGC_PARA_B23                    = 0xDc98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B23        = 0xDc9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B23       = 0xDc9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B23                  = 0xDc9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B23                   = 0xDc9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B23              = 0xDc9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B23         = 0xDca0,
        EN_NV_ID_LTE_TX_ATTEN_B23                       = 0xDca1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B23            = 0xDca2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B23                = 0xDca3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B23           = 0xDca4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B23           = 0xDca5,

        EN_NV_ID_LTE_TX_APT_PARA_B23                    = 0xDca6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B23            = 0xDca7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B23             = 0xDca8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B23             = 0xDca9,
        EN_NV_ID_LTE_TX_MPR_B23                         = 0xDcaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B23                     = 0xDcab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B23                        = 0xDcac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B23         = 0xDcad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B23                   = 0xDcae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B23                   = 0xDcaf,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B23                    = 0xDcb0,
        EN_NV_ID_LTE_TX_BODY_SAR_B23                    = 0xDcb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B23                    = 0xDcb2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B23                   = 0xDcb3,
        EN_NV_ID_CALB_AGC_B23                           = 0xDcb4,
        //EN_NV_ID_TX_ATTEN_TABLE_B23                        = 0xDcb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B23                        = 0xDcb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B23               = 0xDcb5,
        EN_NV_ID_LTE_TX_PD_PARA_B23                     = 0xDcb6,
        EN_NV_ID_TX_ET_BAND_PARA_B23                    = 0xDcb7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B23          = 0xDcb8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B23              = 0xDcb9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B23                   = 0xDcba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B23                   = 0xDcbb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B23                         = 0xDcbc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B23              = 0xDcbd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B23                    = 0xDcbf,

        EN_NV_ID_FTM_CAND_CELL_LIST_B24                 = 0xDcc0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B24            = 0xDcc1,
        EN_NV_ID_LTE_TX_CAL_LIST_B24            = 0xDcc2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B24          = 0xDcc3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B24         = 0xDcc4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B24                = 0xDcc5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B24               = 0xDcc6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B24               = 0xDcc7,
        EN_NV_ID_LTE_TX_TAS_B24                         = 0xDcc8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B24                  = 0xDccf,
        EN_NV_ID_HI6360_AGC_PARA_B24                    = 0xDcd8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B24        = 0xDcdb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B24       = 0xDcdc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B24                  = 0xDcdd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B24                   = 0xDcde,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B24              = 0xDcdf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B24         = 0xDce0,
        EN_NV_ID_LTE_TX_ATTEN_B24                       = 0xDce1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B24          = 0xDce2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B24          = 0xDce3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B24           = 0xDce4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B24           = 0xDce5,

        EN_NV_ID_LTE_TX_APT_PARA_B24                    = 0xDce6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B24            = 0xDce7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B24             = 0xDce8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B24             = 0xDce9,
        EN_NV_ID_LTE_TX_MPR_B24                         = 0xDcea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B24                     = 0xDceb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B24                        = 0xDcec,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B24         = 0xDced,

        EN_NV_ID_LTE_TX_AMPR_NS05_B24                   = 0xDcee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B24                   = 0xDcef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B24                    = 0xDcf0,
        EN_NV_ID_LTE_TX_BODY_SAR_B24                    = 0xDcf1,
        EN_NV_ID_TX_PA_TEMP_COMP_B24                    = 0xDcf2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B24                   = 0xDcf3,
        EN_NV_ID_CALB_AGC_B24                           = 0xDcf4,
        //EN_NV_ID_TX_ATTEN_TABLE_B24                        = 0xDcf3,
        //EN_NV_ID_POWERDET_VOLTAGE_B24                        = 0xDcf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B24               = 0xDcf5,
        EN_NV_ID_LTE_TX_PD_PARA_B24                     = 0xDcf6,
        EN_NV_ID_TX_ET_BAND_PARA_B24                    = 0xDcf7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B24          = 0xDcf8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B24              = 0xDcf9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B24                   = 0xDcfa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B24                   = 0xDcfb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B24                         = 0xDcfc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B24              = 0xDcfd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B24                    = 0xDcff,

        EN_NV_ID_FTM_CAND_CELL_LIST_B25                 = 0xDd00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B25            = 0xDd01,
        EN_NV_ID_LTE_TX_CAL_LIST_B25            = 0xDd02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B25          = 0xDd03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B25         = 0xDd04,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B25                = 0xDd05,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B25               = 0xDd06,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B25               = 0xDd07,
        EN_NV_ID_LTE_TX_TAS_B25                         = 0xDd08,

        EN_NV_ID_TEMP_SENSOR_TABLE_B25                  = 0xDd0f,
        EN_NV_ID_HI6360_AGC_PARA_B25                    = 0xDd18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B25        = 0xDd1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B25       = 0xDd1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B25                  = 0xDd1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B25                   = 0xDd1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B25              = 0xDd1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B25         = 0xDd20,
        EN_NV_ID_LTE_TX_ATTEN_B25                       = 0xDd21,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B25          = 0xDd22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B25          = 0xDd23,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B25           = 0xDd24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B25           = 0xDd25,

        EN_NV_ID_LTE_TX_APT_PARA_B25                    = 0xDd26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B25            = 0xDd27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B25             = 0xDd28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B25             = 0xDd29,
        EN_NV_ID_LTE_TX_MPR_B25                         = 0xDd2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B25                     = 0xDd2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B25                        = 0xDd2c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B25         = 0xDd2d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B25                   = 0xDd2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B25                   = 0xDd2f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B25                    = 0xDd30,
        EN_NV_ID_LTE_TX_BODY_SAR_B25                    = 0xDd31,
        EN_NV_ID_TX_PA_TEMP_COMP_B25                    = 0xDd32,
        EN_NV_ID_EXT_LNA_AGC_PARA_B25                   = 0xDd33,
        EN_NV_ID_CALB_AGC_B25                           = 0xDd34,
        //EN_NV_ID_TX_ATTEN_TABLE_B25                        = 0xDd33,
        //EN_NV_ID_POWERDET_VOLTAGE_B25                        = 0xDd34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B25               = 0xDd35,
        EN_NV_ID_LTE_TX_PD_PARA_B25                     = 0xDd36,
        EN_NV_ID_TX_ET_BAND_PARA_B25                    = 0xDd37,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B25          = 0xDd38,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B25              = 0xDd39,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B25                   = 0xDd3a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B25                   = 0xDd3b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B25                         = 0xDd3c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B25              = 0xDd3d,
        EN_NV_ID_LTE_TX_MPR_64QAM_B25                   = 0xDd3f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B33                 = 0xDd40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B33            = 0xDd41,
        EN_NV_ID_LTE_TX_CAL_LIST_B33            = 0xDd42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B33          = 0xDd43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B33         = 0xDd44,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B33                = 0xDd45,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B33               = 0xDd46,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B33               = 0xDd47,
        EN_NV_ID_LTE_TX_TAS_B33                         = 0xDd48,

        EN_NV_ID_TEMP_SENSOR_TABLE_B33                  = 0xDd4f,
        EN_NV_ID_HI6360_AGC_PARA_B33                    = 0xDd58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B33        = 0xDd5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B33       = 0xDd5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B33                  = 0xDd5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B33                   = 0xDd5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B33              = 0xDd5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B33         = 0xDd60,
        EN_NV_ID_LTE_TX_ATTEN_B33                       = 0xDd61,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B33          = 0xDd62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B33          = 0xDd63,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B33           = 0xDd64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B33           = 0xDd65,

        EN_NV_ID_LTE_TX_APT_PARA_B33                    = 0xDd66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B33            = 0xDd67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B33             = 0xDd68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B33             = 0xDd69,
        EN_NV_ID_LTE_TX_MPR_B33                         = 0xDd6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B33                     = 0xDd6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B33                        = 0xDd6c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B33         = 0xDd6d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B33                   = 0xDd6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B33                   = 0xDd6f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B33                    = 0xDd70,
        EN_NV_ID_LTE_TX_BODY_SAR_B33                    = 0xDd71,
        EN_NV_ID_TX_PA_TEMP_COMP_B33                    = 0xDd72,
        EN_NV_ID_EXT_LNA_AGC_PARA_B33                   = 0xDd73,
        EN_NV_ID_CALB_AGC_B33                           = 0xDd74,
        //EN_NV_ID_TX_ATTEN_TABLE_B33                        = 0xDd73,
        //EN_NV_ID_POWERDET_VOLTAGE_B33                        = 0xDd74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B33               = 0xDd75,
        EN_NV_ID_LTE_TX_PD_PARA_B33                     = 0xDd76,
        EN_NV_ID_TX_ET_BAND_PARA_B33                    = 0xDd77,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B33          = 0xDd78,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B33              = 0xDd79,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B33                   = 0xDd7a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B33                   = 0xDd7b,
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B33                         = 0xDd7c,
        /* END:   Added , 2015/10/29 */
        /* END:   Added by  , 2015/5/14 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B33              = 0xDd7d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B33                    = 0xDd7f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B34                 = 0xDd80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B34            = 0xDd81,
        EN_NV_ID_LTE_TX_CAL_LIST_B34            = 0xDd82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B34          = 0xDd83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B34         = 0xDd84,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B34                = 0xDd85,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B34               = 0xDd86,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B34               = 0xDd87,
        EN_NV_ID_LTE_TX_TAS_B34                         = 0xDd88,

        EN_NV_ID_TEMP_SENSOR_TABLE_B34                  = 0xDd8f,
        EN_NV_ID_HI6360_AGC_PARA_B34                    = 0xDd98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B34        = 0xDd9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B34       = 0xDd9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B34                  = 0xDd9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B34                   = 0xDd9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B34              = 0xDd9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B34         = 0xDda0,
        EN_NV_ID_LTE_TX_ATTEN_B34                       = 0xDda1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B34          = 0xDda2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B34          = 0xDda3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B34           = 0xDda4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B34           = 0xDda5,

        EN_NV_ID_LTE_TX_APT_PARA_B34                    = 0xDda6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B34            = 0xDda7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B34             = 0xDda8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B34             = 0xDda9,
        EN_NV_ID_LTE_TX_MPR_B34                         = 0xDdaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B34                     = 0xDdab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B34                        = 0xDdac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B34         = 0xDdad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B34                   = 0xDdae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B34                   = 0xDdaf,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B34                    = 0xDdb0,
        EN_NV_ID_LTE_TX_BODY_SAR_B34                    = 0xDdb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B34                    = 0xDdb2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B34                   = 0xDdb3,
        EN_NV_ID_CALB_AGC_B34                           = 0xDdb4,
        //EN_NV_ID_TX_ATTEN_TABLE_B34                        = 0xDdb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B34                        = 0xDdb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B34               = 0xDdb5,
        EN_NV_ID_LTE_TX_PD_PARA_B34                     = 0xDdb6,
        EN_NV_ID_TX_ET_BAND_PARA_B34                    = 0xDdb7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B34          = 0xDdb8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B34              = 0xDdb9,
         /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B34                   = 0xDdba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B34                   = 0xDdbb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B34                         = 0xDdbc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B34              = 0xDdbd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B34                    = 0xDdbf,

        EN_NV_ID_FTM_CAND_CELL_LIST_B35                 = 0xDdc0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B35            = 0xDdc1,
        EN_NV_ID_LTE_TX_CAL_LIST_B35            = 0xDdc2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B35          = 0xDdc3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B35         = 0xDdc4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B35                = 0xDdc5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B35               = 0xDdc6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B35               = 0xDdc7,
        EN_NV_ID_LTE_TX_TAS_B35                         = 0xDdc8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B35                  = 0xDdcf,
        EN_NV_ID_HI6360_AGC_PARA_B35                    = 0xDdd8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B35        = 0xDddb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B35       = 0xDddc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B35                  = 0xDddd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B35                   = 0xDdde,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B35              = 0xDddf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B35         = 0xDde0,
        EN_NV_ID_LTE_TX_ATTEN_B35                       = 0xDde1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B35          = 0xDde2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B35          = 0xDde3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B35           = 0xDde4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B35           = 0xDde5,

        EN_NV_ID_LTE_TX_APT_PARA_B35                    = 0xDde6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B35            = 0xDde7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B35             = 0xDde8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B35             = 0xDde9,
        EN_NV_ID_LTE_TX_MPR_B35                         = 0xDdea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B35                     = 0xDdeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B35                        = 0xDdec,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B35         = 0xDded,

        EN_NV_ID_LTE_TX_AMPR_NS05_B35                   = 0xDdee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B35                   = 0xDdef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B35                    = 0xDdf0,
        EN_NV_ID_LTE_TX_BODY_SAR_B35                    = 0xDdf1,
        EN_NV_ID_TX_PA_TEMP_COMP_B35                    = 0xDdf2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B35                   = 0xDdf3,
        EN_NV_ID_CALB_AGC_B35                           = 0xDdf4,
        //EN_NV_ID_TX_ATTEN_TABLE_B35                        = 0xDdf3,
        //EN_NV_ID_POWERDET_VOLTAGE_B35                        = 0xDdf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B35               = 0xDdf5,
        EN_NV_ID_LTE_TX_PD_PARA_B35                     = 0xDdf6,
        EN_NV_ID_TX_ET_BAND_PARA_B35                    = 0xDdf7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B35          = 0xDdf8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B35              = 0xDdf9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B35                   = 0xDdfa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B35                   = 0xDdfb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B35                         = 0xDdfc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B35              = 0xDdfd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B35                    = 0xDdff,

        EN_NV_ID_FTM_CAND_CELL_LIST_B36                 = 0xDe00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B36            = 0xDe01,
        EN_NV_ID_LTE_TX_CAL_LIST_B36            = 0xDe02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B36          = 0xDe03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B36         = 0xDe04,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B36                = 0xDe05,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B36               = 0xDe06,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B36               = 0xDe07,
        EN_NV_ID_LTE_TX_TAS_B36                         = 0xDe08,

        EN_NV_ID_TEMP_SENSOR_TABLE_B36                  = 0xDe0f,
        EN_NV_ID_HI6360_AGC_PARA_B36                    = 0xDe18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B36        = 0xDe1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B36       = 0xDe1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B36                  = 0xDe1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B36                   = 0xDe1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B36              = 0xDe1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B36         = 0xDe20,
        EN_NV_ID_LTE_TX_ATTEN_B36                       = 0xDe21,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B36          = 0xDe22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B36          = 0xDe23,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B36           = 0xDe24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B36           = 0xDe25,

        EN_NV_ID_LTE_TX_APT_PARA_B36                    = 0xDe26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B36            = 0xDe27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B36             = 0xDe28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B36             = 0xDe29,
        EN_NV_ID_LTE_TX_MPR_B36                         = 0xDe2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B36                     = 0xDe2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B36                        = 0xDe2c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B36         = 0xDe2d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B36                   = 0xDe2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B36                   = 0xDe2f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B36                    = 0xDe30,
        EN_NV_ID_LTE_TX_BODY_SAR_B36                    = 0xDe31,
        EN_NV_ID_TX_PA_TEMP_COMP_B36                    = 0xDe32,
        EN_NV_ID_EXT_LNA_AGC_PARA_B36                   = 0xDe33,
        EN_NV_ID_CALB_AGC_B36                           = 0xDe34,
        //EN_NV_ID_TX_ATTEN_TABLE_B36                        = 0xDe33,
        //EN_NV_ID_POWERDET_VOLTAGE_B36                        = 0xDe34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B36               = 0xDe35,
        EN_NV_ID_LTE_TX_PD_PARA_B36                     = 0xDe36,
        EN_NV_ID_TX_ET_BAND_PARA_B36                    = 0xDe37,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B36          = 0xDe38,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B36              = 0xDe39,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B36                   = 0xDe3a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B36                   = 0xDe3b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B36                         = 0xDe3c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B36              = 0xDe3d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B36                    = 0xDe3f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B37                 = 0xDe40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B37            = 0xDe41,
        EN_NV_ID_LTE_TX_CAL_LIST_B37            = 0xDe42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B37          = 0xDe43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B37         = 0xDe44,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B37                = 0xDe45,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B37               = 0xDe46,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B37               = 0xDe47,
        EN_NV_ID_LTE_TX_TAS_B37                         = 0xDe48,

        EN_NV_ID_TEMP_SENSOR_TABLE_B37                  = 0xDe4f,
        EN_NV_ID_HI6360_AGC_PARA_B37                    = 0xDe58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B37        = 0xDe5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B37       = 0xDe5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B37                  = 0xDe5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B37                   = 0xDe5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B37              = 0xDe5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B37         = 0xDe60,
        EN_NV_ID_LTE_TX_ATTEN_B37                       = 0xDe61,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B37          = 0xDe62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B37          = 0xDe63,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B37           = 0xDe64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B37           = 0xDe65,

        EN_NV_ID_LTE_TX_APT_PARA_B37                    = 0xDe66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B37            = 0xDe67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B37             = 0xDe68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B37             = 0xDe69,
        EN_NV_ID_LTE_TX_MPR_B37                         = 0xDe6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B37                     = 0xDe6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B37                        = 0xDe6c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B37         = 0xDe6d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B37                   = 0xDe6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B37                   = 0xDe6f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B37                    = 0xDe70,
        EN_NV_ID_LTE_TX_BODY_SAR_B37                    = 0xDe71,
        EN_NV_ID_TX_PA_TEMP_COMP_B37                    = 0xDe72,
        EN_NV_ID_EXT_LNA_AGC_PARA_B37                   = 0xDe73,
        EN_NV_ID_CALB_AGC_B37                           = 0xDe74,
        //EN_NV_ID_TX_ATTEN_TABLE_B37                        = 0xDe73,
        //EN_NV_ID_POWERDET_VOLTAGE_B37                        = 0xDe74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B37               = 0xDe75,
        EN_NV_ID_LTE_TX_PD_PARA_B37                     = 0xDe76,
        EN_NV_ID_TX_ET_BAND_PARA_B37                    = 0xDe77,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B37          = 0xDe78,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B37              = 0xDe79,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B37                   = 0xDe7a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B37                   = 0xDe7b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B37                         = 0xDe7c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B37              = 0xDe7d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B37                    = 0xDe7f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B42                 = 0xDe80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B42            = 0xDe81,
        EN_NV_ID_LTE_TX_CAL_LIST_B42                    = 0xDe82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B42          = 0xDe83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B42         = 0xDe84,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B42                = 0xDe85,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B42               = 0xDe86,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B42               = 0xDe87,
        EN_NV_ID_LTE_TX_TAS_B42                         = 0xDe88,

        EN_NV_ID_TEMP_SENSOR_TABLE_B42                  = 0xDe8f,
        EN_NV_ID_HI6360_AGC_PARA_B42                    = 0xDe98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B42        = 0xDe9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B42       = 0xDe9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B42                  = 0xDe9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B42                   = 0xDe9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B42              = 0xDe9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B42         = 0xDea0,
        EN_NV_ID_LTE_TX_ATTEN_B42                       = 0xDea1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B42          = 0xDea2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B42                 = 0xDea3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B42           = 0xDea4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B42       = 0xDea5,

        EN_NV_ID_LTE_TX_APT_PARA_B42                    = 0xDea6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B42            = 0xDea7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B42             = 0xDea8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B42             = 0xDea9,
        EN_NV_ID_LTE_TX_MPR_B42                         = 0xDeaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B42                     = 0xDeab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B42                        = 0xDeac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B42         = 0xDead,

        EN_NV_ID_LTE_TX_AMPR_NS05_B42                   = 0xDeae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B42                   = 0xDeaf,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B42                    = 0xDeb0,
        EN_NV_ID_LTE_TX_BODY_SAR_B42                    = 0xDeb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B42                    = 0xDeb2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B42                   = 0xDeb3,
        EN_NV_ID_CALB_AGC_B42                           = 0xDeb4,
        //EN_NV_ID_TX_ATTEN_TABLE_B42                        = 0xDeb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B42                        = 0xDeb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B42               = 0xDeb5,
        EN_NV_ID_LTE_TX_PD_PARA_B42                     =0xDeb6,
        EN_NV_ID_TX_ET_BAND_PARA_B42                    = 0xDeb7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B42          = 0xDeb8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B42              = 0xDeb9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B42                   = 0xDeba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B42                   = 0xDebb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B42                         = 0xDebc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B42              = 0xDebd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B42                    = 0xDebf,

        EN_NV_ID_FTM_CAND_CELL_LIST_B43                 = 0xDec0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B43            = 0xDec1,
        EN_NV_ID_LTE_TX_CAL_LIST_B43            = 0xDec2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B43          = 0xDec3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B43         = 0xDec4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B43                = 0xDec5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B43               = 0xDec6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B43               = 0xDec7,
        EN_NV_ID_LTE_TX_TAS_B43                         = 0xDec8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B43                  = 0xDecf,
        EN_NV_ID_HI6360_AGC_PARA_B43                    = 0xDed8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B43        = 0xDedb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B43       = 0xDedc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B43                  = 0xDedd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B43                   = 0xDede,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B43              = 0xDedf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B43         = 0xDee0,
        EN_NV_ID_LTE_TX_ATTEN_B43                       = 0xDee1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B43          = 0xDee2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B43          = 0xDee3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B43           = 0xDee4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B43           = 0xDee5,

        EN_NV_ID_LTE_TX_APT_PARA_B43                    = 0xDee6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B43            = 0xDee7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B43             = 0xDee8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B43             = 0xDee9,
        EN_NV_ID_LTE_TX_MPR_B43                         = 0xDeea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B43                     = 0xDeeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B43                        = 0xDeec,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B43         = 0xDeed,

        EN_NV_ID_LTE_TX_AMPR_NS05_B43                   = 0xDeee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B43                   = 0xDeef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B43                    = 0xDef0,
        EN_NV_ID_LTE_TX_BODY_SAR_B43                    = 0xDef1,
        EN_NV_ID_TX_PA_TEMP_COMP_B43                    = 0xDef2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B43                   = 0xDef3,
        EN_NV_ID_CALB_AGC_B43                           = 0xDef4,
        //EN_NV_ID_TX_ATTEN_TABLE_B43                        = 0xDef3,
        //EN_NV_ID_POWERDET_VOLTAGE_B43                        = 0xDef4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B43               = 0xDef5,
        EN_NV_ID_LTE_TX_PD_PARA_B43                     = 0xDef6,
        EN_NV_ID_TX_ET_BAND_PARA_B43                    = 0xDef7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B43          = 0xDef8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B43              = 0xDef9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B43                   = 0xDefa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B43                   = 0xDefb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B43                         = 0xDefc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B43              = 0xDefd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B43                    = 0xDeff,

        EN_NV_ID_FTM_CAND_CELL_LIST_B39                 = 0xDf00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B39            = 0xDf01,
        EN_NV_ID_LTE_TX_CAL_LIST_B39                    = 0xDf02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B39          = 0xDf03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B39         = 0xDf04,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B39                = 0xDf05,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B39               = 0xDf06,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B39               = 0xDf07,
        EN_NV_ID_LTE_TX_TAS_B39                         = 0xDf08,

        EN_NV_ID_TEMP_SENSOR_TABLE_B39                  = 0xDf0f,
        EN_NV_ID_HI6360_AGC_PARA_B39                    = 0xDf18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B39        = 0xDf1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B39       = 0xDf1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B39                  = 0xDf1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B39                   = 0xDf1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B39              = 0xDf1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B39         = 0xDf20,
        EN_NV_ID_LTE_TX_ATTEN_B39                       = 0xDf21,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B39          = 0xDf22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B39          = 0xDf23,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B39           = 0xDf24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B39           = 0xDf25,

        EN_NV_ID_LTE_TX_APT_PARA_B39                    = 0xDf26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B39            = 0xDf27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B39             = 0xDf28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B39             = 0xDf29,
        EN_NV_ID_LTE_TX_MPR_B39                         = 0xDf2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B39                     = 0xDf2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B39                        = 0xDf2c,
        /* add by  AMPR  END */

        /* modify by   for 所有band end*/
        EN_NV_ID_LTE_OTHER_COMP_B39         = 0xDf2d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B39                   = 0xDf2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B39                   = 0xDf2f,
        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B39                    = 0xDf30,
        EN_NV_ID_LTE_TX_BODY_SAR_B39                    = 0xDf31,
        EN_NV_ID_TX_PA_TEMP_COMP_B39                    = 0xDf32,
        EN_NV_ID_EXT_LNA_AGC_PARA_B39                   = 0xDf33,
        EN_NV_ID_CALB_AGC_B39                           = 0xDf34,
        //EN_NV_ID_TX_ATTEN_TABLE_B39                        = 0xDf33,
        //EN_NV_ID_POWERDET_VOLTAGE_B39                        = 0xDf34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B39               = 0xDf35,
        EN_NV_ID_LTE_TX_PD_PARA_B39                     = 0xDf36,
        EN_NV_ID_TX_ET_BAND_PARA_B39                    = 0xDf37,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B39          = 0xDf38,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B39              = 0xDf39,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B39                   = 0xDf3a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B39                   = 0xDf3b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B39                         = 0xDf3c,
        /* END:   Added , 2015/10/29 */
        /* modify by   for 所有band end*/
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B39              = 0xDf3d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B39                    = 0xDf3f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B20                 = 0xD600,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B20            = 0xD601,
        EN_NV_ID_LTE_TX_CAL_LIST_B20            = 0xD602,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B20          = 0xD603,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B20          = 0xD604,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B20               = 0xD605,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B20               = 0xD606,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B20               = 0xD607,
        EN_NV_ID_LTE_TX_TAS_B20                         = 0xD608,

        EN_NV_ID_TEMP_SENSOR_TABLE_B20                  = 0xD60f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B20        = 0xD61b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B20        = 0xD61c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B20                  = 0xD61d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B20                   = 0xD61e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B20              = 0xD61f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B20         = 0xD620,
        EN_NV_ID_LTE_TX_ATTEN_B20                       = 0xD621,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B20          = 0xD622,
        EN_NV_ID_TX_FILTER_CMP_STRU_B20          = 0xD623,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B20           = 0xD624,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B20           = 0xD625,
        EN_NV_ID_LTE_TX_APT_PARA_B20                    = 0xD626,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B20            = 0xD627,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B20             = 0xD628,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B20             = 0xD629,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B20                         = 0xD62a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B20                     = 0xD62b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B20                        = 0xD62c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B20         = 0xD62d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B20                   = 0xD62e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B20                   = 0xD62f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B20                    = 0xD630,
        EN_NV_ID_LTE_TX_BODY_SAR_B20                    = 0xD631,
        EN_NV_ID_TX_PA_TEMP_COMP_B20                    = 0xD632,
        EN_NV_ID_EXT_LNA_AGC_PARA_B20                   = 0xD633,
        EN_NV_ID_CALB_AGC_B20                           = 0xD634,
        //EN_NV_ID_TX_ATTEN_TABLE_B20                        = 0xD633,
        //EN_NV_ID_POWERDET_VOLTAGE_B20                        = 0xD634,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B20               = 0xD635,
        EN_NV_ID_LTE_TX_PD_PARA_B20                     = 0xD636,
        EN_NV_ID_TX_ET_BAND_PARA_B20                    = 0xD637,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B20          = 0xD638,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B20              = 0xD639,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B20                   = 0xD63a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B20                   = 0xD63b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B20                         = 0xD63c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B20              = 0xD63d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B20                    = 0xD63f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B40                 = 0xD640,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B40            = 0xD641,
        EN_NV_ID_LTE_TX_CAL_LIST_B40            = 0xD642,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B40          = 0xD643,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B40          = 0xD644,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B40                = 0xD645,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B40               = 0xD646,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B40               = 0xD647,
        EN_NV_ID_LTE_TX_TAS_B40                         = 0xD648,

        EN_NV_ID_TEMP_SENSOR_TABLE_B40                  = 0xD64f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B40        = 0xD65b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B40        = 0xD65c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B40                  = 0xD65d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B40                   = 0xD65e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B40              = 0xD65f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B40         = 0xD660,
        EN_NV_ID_LTE_TX_ATTEN_B40                       = 0xD661,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B40          = 0xD662,
        EN_NV_ID_TX_FILTER_CMP_STRU_B40          = 0xD663,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B40           = 0xD664,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B40           = 0xD665,
        EN_NV_ID_LTE_TX_APT_PARA_B40                    = 0xD666,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B40            = 0xD667,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B40             = 0xD668,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B40             = 0xD669,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B40                         = 0xD66a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B40                     = 0xD66b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B40                        = 0xD66c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B40         = 0xD66d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B40                   = 0xD66e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B40                   = 0xD66f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B40                    = 0xD670,
        EN_NV_ID_LTE_TX_BODY_SAR_B40                    = 0xD671,
        EN_NV_ID_TX_PA_TEMP_COMP_B40                    = 0xD672,
        EN_NV_ID_EXT_LNA_AGC_PARA_B40                   = 0xD673,
        EN_NV_ID_CALB_AGC_B40                           = 0xD674,
        //EN_NV_ID_TX_ATTEN_TABLE_B40                        = 0xD673,
        //EN_NV_ID_POWERDET_VOLTAGE_B40                        = 0xD674,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B40               = 0xD675,
        EN_NV_ID_LTE_TX_PD_PARA_B40                     = 0xD676,
        EN_NV_ID_TX_ET_BAND_PARA_B40                    = 0xD677,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B40          = 0xD678,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B40              = 0xD679,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B40                   = 0xD67a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B40                   = 0xD67b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B40                         = 0xD67c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B40              = 0xD67d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B40                    = 0xD67f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B140                 = 0xE540,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B140            = 0xE541,
        EN_NV_ID_LTE_TX_CAL_LIST_B140            = 0xE542,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B140          = 0xE543,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B140          = 0xE544,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B140                = 0xE545,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B140               = 0xE546,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B140               = 0xE547,
        EN_NV_ID_LTE_TX_TAS_B140                         = 0xE548,

        EN_NV_ID_TEMP_SENSOR_TABLE_B140                  = 0xE54f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B140        = 0xE55b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B140        = 0xE55c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B140                  = 0xE55d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B140                   = 0xE55e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B140              = 0xE55f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B140         = 0xE560,
        EN_NV_ID_LTE_TX_ATTEN_B140                       = 0xE561,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B140          = 0xE562,
        EN_NV_ID_TX_FILTER_CMP_STRU_B140          = 0xE563,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B140           = 0xE564,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B140           = 0xE565,
        EN_NV_ID_LTE_TX_APT_PARA_B140                    = 0xE566,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B140            = 0xE567,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B140             = 0xE568,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B140             = 0xE569,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B140                         = 0xE56a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B140                     = 0xE56b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B140                        = 0xE56c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B140         = 0xE56d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B140                   = 0xE56e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B140                   = 0xE56f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B140                    = 0xE570,
        EN_NV_ID_LTE_TX_BODY_SAR_B140                   = 0xE571,
        EN_NV_ID_TX_PA_TEMP_COMP_B140                   = 0xE572,
        EN_NV_ID_EXT_LNA_AGC_PARA_B140                   = 0xE573,
        EN_NV_ID_CALB_AGC_B140                           = 0xE574,
        //EN_NV_ID_TX_ATTEN_TABLE_B140                        = 0xE573,
        //EN_NV_ID_POWERDET_VOLTAGE_B140                        = 0xE574,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B140               = 0xE575,
        EN_NV_ID_LTE_TX_PD_PARA_B140                     = 0xE576,
        EN_NV_ID_TX_ET_BAND_PARA_B140                    = 0xE577,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B140          = 0xE578,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B140              = 0xE579,
		/* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
	    EN_NV_ID_LTE_TX_AMPR_NS03_B140                   = 0xE57a,
	    EN_NV_ID_LTE_TX_AMPR_NS22_B140                   = 0xE57b,
	    /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B140                         = 0xE57c,
        /* END:   Added , 2015/10/29 */
	    EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B140             = 0xE57d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B140                    = 0xE57f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B38                 = 0xD680,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B38            = 0xD681,
        EN_NV_ID_LTE_TX_CAL_LIST_B38            = 0xD682,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B38          = 0xD683,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B38          = 0xD684,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B38                = 0xD685,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B38               = 0xD686,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B38               = 0xD687,
        EN_NV_ID_LTE_TX_TAS_B38                         = 0xD688,

        EN_NV_ID_TEMP_SENSOR_TABLE_B38                  = 0xD68f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B38        = 0xD69b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B38        = 0xD69c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B38                  = 0xD69d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B38                   = 0xD69e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B38              = 0xD69f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B38         = 0xD6a0,
        EN_NV_ID_LTE_TX_ATTEN_B38                       = 0xD6a1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B38          = 0xD6a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B38          = 0xD6a3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B38           = 0xD6a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B38           = 0xD6a5,
        EN_NV_ID_LTE_TX_APT_PARA_B38                    = 0xD6a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B38            = 0xD6a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B38             = 0xD6a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B38             = 0xD6a9,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B38                         = 0xD6aa,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B38                     = 0xD6ab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B38                        = 0xD6ac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B38         = 0xD6ad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B38                   = 0xD6ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B38                   = 0xD6af,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B38                    = 0xD6b0,
        EN_NV_ID_LTE_TX_BODY_SAR_B38                    = 0xD6b1,
        EN_NV_ID_TX_PA_TEMP_COMP_B38                    = 0xD6b2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B38                   = 0xD6b3,
        EN_NV_ID_CALB_AGC_B38                           = 0xD6b4,
        //EN_NV_ID_TX_ATTEN_TABLE_B38                        = 0xD6b3,
        //EN_NV_ID_POWERDET_VOLTAGE_B38                        = 0xD6b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B38               = 0xD6b5,
        EN_NV_ID_LTE_TX_PD_PARA_B38                     = 0xD6b6,
        EN_NV_ID_TX_ET_BAND_PARA_B38                    = 0xD6b7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B38          = 0xD6b8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B38              = 0xD6b9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B38                   = 0xD6ba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B38                   = 0xD6bb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B38                         = 0xD6bc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B38             = 0xD6bd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B38                    = 0xD6bf,

        EN_NV_ID_FTM_CAND_CELL_LIST_B41                 = 0xD6c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B41            = 0xD6c1,
        EN_NV_ID_LTE_TX_CAL_LIST_B41            = 0xD6c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B41          = 0xD6c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B41          = 0xD6c4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B41                = 0xD6c5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B41               = 0xD6c6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B41               = 0xD6c7,
        EN_NV_ID_LTE_TX_TAS_B41                         = 0xD6c8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B41                  = 0xD6cf,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B41        = 0xD6Db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B41        = 0xD6Dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B41                  = 0xD6Dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B41                   = 0xD6De,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B41              = 0xD6Df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B41         = 0xD6E0,
        EN_NV_ID_LTE_TX_ATTEN_B41                       = 0xD6E1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B41          = 0xD6E2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B41          = 0xD6E3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B41           = 0xD6E4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B41           = 0xD6E5,
        EN_NV_ID_LTE_TX_APT_PARA_B41                    = 0xD6E6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B41            = 0xD6E7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B41             = 0xD6E8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B41             = 0xD6E9,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B41                         = 0xD6ea,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B41                     = 0xD6eb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B41                        = 0xD6ec,
        /* add by  AMPR END */
        EN_NV_ID_LTE_OTHER_COMP_B41         = 0xD6ed,

        EN_NV_ID_LTE_TX_AMPR_NS05_B41                   = 0xD6ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B41                   = 0xD6ef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B41                    = 0xD6f0,
        EN_NV_ID_LTE_TX_BODY_SAR_B41                    = 0xD6f1,
        EN_NV_ID_TX_PA_TEMP_COMP_B41                    = 0xD6f2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B41                   = 0xD6f3,
        EN_NV_ID_CALB_AGC_B41                           = 0xD6f4,
        //EN_NV_ID_TX_ATTEN_TABLE_B41                    = 0xD6f3,
        //EN_NV_ID_POWERDET_VOLTAGE_B41                   = 0xD6f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B41               = 0xD6f5,
        EN_NV_ID_LTE_TX_PD_PARA_B41                     = 0xD6f6,
        EN_NV_ID_TX_ET_BAND_PARA_B41                    = 0xD6f7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B41          = 0xD6f8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B41              = 0xD6f9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B41                   = 0xD6fa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B41                   = 0xD6fb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B41                         = 0xD6fc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B41              = 0xD6fd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B41                    = 0xD6ff,

        EN_NV_ID_FTM_CAND_CELL_LIST_B7                  = 0xD700,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B7             = 0xD701,
        EN_NV_ID_LTE_TX_CAL_LIST_B7             = 0xD702,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B7           = 0xD703,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B7           = 0xD704,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B7                 = 0xD705,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B7                = 0xD706,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B7                = 0xD707,
        EN_NV_ID_LTE_TX_TAS_B7                          = 0xD708,

        EN_NV_ID_TEMP_SENSOR_TABLE_B7                   = 0xD70f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B7         = 0xD71b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B7         = 0xD71c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B7                   = 0xD71d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B7                    = 0xD71e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B7               = 0xD71f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B7          = 0xD720,
        EN_NV_ID_LTE_TX_ATTEN_B7                        = 0xD721,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B7           = 0xD722,
        EN_NV_ID_TX_FILTER_CMP_STRU_B7           = 0xD723,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B7            = 0xD724,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B7            = 0xD725,
        EN_NV_ID_LTE_TX_APT_PARA_B7                     = 0xD726,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B7             = 0xD727,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B7              = 0xD728,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B7              = 0xD729,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B7                          = 0xD72a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B7                      = 0xD72b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B7                         = 0xD72c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B7         = 0xD72d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B7                   = 0xD72e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B7                   = 0xD72f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B7                     = 0xD730,
        EN_NV_ID_LTE_TX_BODY_SAR_B7                    = 0xD731,
        EN_NV_ID_TX_PA_TEMP_COMP_B7                    = 0xD732,
        EN_NV_ID_EXT_LNA_AGC_PARA_B7                   = 0xD733,
        EN_NV_ID_CALB_AGC_B7                           = 0xD734,
        //EN_NV_ID_TX_ATTEN_TABLE_B7                    = 0xD733,
        //EN_NV_ID_POWERDET_VOLTAGE_B7                  = 0xD734,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B7                = 0xD735,
        EN_NV_ID_LTE_TX_PD_PARA_B7                      = 0xD736,
        EN_NV_ID_TX_ET_BAND_PARA_B7                     = 0xD737,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B7           = 0xD738,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B7               = 0xD739,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B7                   = 0xD73a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B7                   = 0xD73b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B7                          = 0xD73c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B7              = 0xD73d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B7                    = 0xD73f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B3                  = 0xD800,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B3             = 0xD801,
        EN_NV_ID_LTE_TX_CAL_LIST_B3                     = 0xD802,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B3           = 0xD803,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B3           = 0xD804,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B3                 = 0xD805,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B3                = 0xD806,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B3                = 0xD807,
        EN_NV_ID_LTE_TX_TAS_B3                          = 0xD808,

        EN_NV_ID_TEMP_SENSOR_TABLE_B3                   = 0xD80f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B3         = 0xD81b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B3         = 0xD81c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B3                   = 0xD81d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B3                    = 0xD81e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B3               = 0xD81f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B3          = 0xD820,
        EN_NV_ID_LTE_TX_ATTEN_B3                        = 0xD821,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B3           = 0xD822,
        EN_NV_ID_TX_FILTER_CMP_STRU_B3           = 0xD823,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B3            = 0xD824,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B3            = 0xD825,
        EN_NV_ID_LTE_TX_APT_PARA_B3                     = 0xD826,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B3             = 0xD827,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B3              = 0xD828,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B3              = 0xD829,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B3                          = 0xD82a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B3                      = 0xD82b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B3                         = 0xD82c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B3         = 0xD82d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B3                   = 0xD82e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B3                   = 0xD82f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B3                     = 0xD830,
        EN_NV_ID_LTE_TX_BODY_SAR_B3                    = 0xD831,
        EN_NV_ID_TX_PA_TEMP_COMP_B3                    = 0xD832,
        EN_NV_ID_EXT_LNA_AGC_PARA_B3                   = 0xD833,
        EN_NV_ID_CALB_AGC_B3                           = 0xD834,
        //EN_NV_ID_TX_ATTEN_TABLE_B3                        = 0xD833,
        //EN_NV_ID_POWERDET_VOLTAGE_B3                        = 0xD834,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B3                = 0xD835,
        EN_NV_ID_LTE_TX_PD_PARA_B3                      = 0xD836,
        EN_NV_ID_TX_ET_BAND_PARA_B3                     = 0xD837,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B3           = 0xD838,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B3               = 0xD839,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B3                   = 0xD83a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B3                   = 0xD83b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B3                          = 0xD83c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B3              = 0xD83d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B3                    = 0xD83f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B1                  = 0xD840,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B1             = 0xD841,
        EN_NV_ID_LTE_TX_CAL_LIST_B1             = 0xD842,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B1           = 0xD843,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B1           = 0xD844,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B1                 = 0xD845,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B1                = 0xD846,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B1                = 0xD847,
        EN_NV_ID_LTE_TX_TAS_B1                          = 0xD848,

        EN_NV_ID_TEMP_SENSOR_TABLE_B1                   = 0xD84f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B1         = 0xD85b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B1         = 0xD85c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B1                   = 0xD85d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B1                    = 0xD85e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B1               = 0xD85f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B1          = 0xD860,
        EN_NV_ID_LTE_TX_ATTEN_B1                        = 0xD861,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B1           = 0xD862,
        EN_NV_ID_TX_FILTER_CMP_STRU_B1           = 0xD863,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B1            = 0xD864,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B1            = 0xD865,
        EN_NV_ID_LTE_TX_APT_PARA_B1                     = 0xD866,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B1             = 0xD867,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B1              = 0xD868,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B1              = 0xD869,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B1                          = 0xD86a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B1                      = 0xD86b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B1                         = 0xD86c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B1         = 0xD86d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B1                   = 0xD86e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B1                   = 0xD86f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B1                    = 0xD870,
        EN_NV_ID_LTE_TX_BODY_SAR_B1                    = 0xD871,
        EN_NV_ID_TX_PA_TEMP_COMP_B1                    = 0xD872,
        EN_NV_ID_EXT_LNA_AGC_PARA_B1                   = 0xD873,
        EN_NV_ID_CALB_AGC_B1                           = 0xD874,
        //EN_NV_ID_TX_ATTEN_TABLE_B1                        = 0xD873,
        //EN_NV_ID_POWERDET_VOLTAGE_B1                        = 0xD874,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B1                = 0xD875,
        EN_NV_ID_LTE_TX_PD_PARA_B1                      = 0xD876,
        EN_NV_ID_TX_ET_BAND_PARA_B1                     = 0xD877,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B1           = 0xD878,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B1               = 0xD879,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B1                    = 0xD87a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B1                    = 0xD87b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B1                          = 0xD87c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B1              = 0xD87d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B1                    = 0xD87f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B5                  = 0xD880,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B5             = 0xD881,
        EN_NV_ID_LTE_TX_CAL_LIST_B5                     = 0xD882,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B5           = 0xD883,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B5           = 0xD884,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B5                 = 0xD885,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B5                = 0xD886,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B5                = 0xD887,
        EN_NV_ID_LTE_TX_TAS_B5                          = 0xD888,

        EN_NV_ID_TEMP_SENSOR_TABLE_B5                   = 0xD88f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B5         = 0xD89b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B5         = 0xD89c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B5                   = 0xD89d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B5                    = 0xD89e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B5               = 0xD89f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B5          = 0xD8a0,
        EN_NV_ID_LTE_TX_ATTEN_B5                        = 0xD8a1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B5           = 0xD8a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B5           = 0xD8a3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B5            = 0xD8a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B5            = 0xD8a5,
        EN_NV_ID_LTE_TX_APT_PARA_B5                     = 0xD8a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B5             = 0xD8a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B5              = 0xD8a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B5              = 0xD8a9,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B5                          = 0xD8aa,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B5                      = 0xD8ab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B5                         = 0xD8ac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B5         = 0xD8ad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B5                   = 0xD8ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B5                   = 0xD8af,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B5                     = 0xD8b0,
        EN_NV_ID_LTE_TX_BODY_SAR_B5                    = 0xD8b1,
        EN_NV_ID_TX_PA_TEMP_COMP_B5                    = 0xD8b2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B5                   = 0xD8b3,
        EN_NV_ID_CALB_AGC_B5                           = 0xD8b4,
        //EN_NV_ID_TX_ATTEN_TABLE_B5                        = 0xD8b3,
        //EN_NV_ID_POWERDET_VOLTAGE_B5                        = 0xD8b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B5                = 0xD8b5,
        EN_NV_ID_LTE_TX_PD_PARA_B5                      = 0xD8b6,
        EN_NV_ID_TX_ET_BAND_PARA_B5                     = 0xD8b7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B5           = 0xD8b8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B5               = 0xD8b9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B5                    = 0xD8ba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B5                    = 0xD8bb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B5                          = 0xD8bc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B5              = 0xD8bd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B5                    = 0xD8bf,

        EN_NV_ID_FTM_CAND_CELL_LIST_B8                  = 0xD8c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B8             = 0xD8c1,
        EN_NV_ID_LTE_TX_CAL_LIST_B8                     = 0xD8c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B8           = 0xD8c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B8           = 0xD8c4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B8                 = 0xD8c5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B8                = 0xD8c6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B8                = 0xD8c7,
        EN_NV_ID_LTE_TX_TAS_B8                          = 0xD8c8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B8                   = 0xD8cf,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B8         = 0xD8db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B8         = 0xD8dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B8                   = 0xD8dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B8                    = 0xD8de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B8               = 0xD8df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B8          = 0xD8e0,
        EN_NV_ID_LTE_TX_ATTEN_B8                        = 0xD8e1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B8           = 0xD8e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B8                  = 0xD8e3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B8            = 0xD8e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B8            = 0xD8e5,
        EN_NV_ID_LTE_TX_APT_PARA_B8                     = 0xD8e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B8             = 0xD8e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B8              = 0xD8e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B8              = 0xD8e9,

        /* modify  mpr begin */
        EN_NV_ID_LTE_TX_MPR_B8                          = 0xD8ea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B8                      = 0xD8eb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B8                         = 0xD8ec,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B8         = 0xD8ed,

        EN_NV_ID_LTE_TX_AMPR_NS05_B8                   = 0xD8ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B8                   = 0xD8ef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B8                    = 0xD8f0,
        EN_NV_ID_LTE_TX_BODY_SAR_B8                    = 0xD8f1,
        EN_NV_ID_TX_PA_TEMP_COMP_B8                    = 0xD8f2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B8                   = 0xD8f3,
        EN_NV_ID_CALB_AGC_B8                           = 0xD8f4,
        //EN_NV_ID_TX_ATTEN_TABLE_B8                        = 0xD8f3,
        //EN_NV_ID_POWERDET_VOLTAGE_B8                        = 0xD8f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B8                = 0xD8f5,
        EN_NV_ID_LTE_TX_PD_PARA_B8                      = 0xD8f6,
        EN_NV_ID_TX_ET_BAND_PARA_B8                     = 0xD8f7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B8           = 0xD8f8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B8               = 0xD8f9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B8                    = 0xD8fa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B8                    = 0xD8fb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B8                          = 0xD8fc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B8              = 0xD8fd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B8                    = 0xD8ff,

    /*Band28 相关NV 项*/

        EN_NV_ID_FTM_CAND_CELL_LIST_B28                  = 0xDf40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B28             = 0xDf41,
        EN_NV_ID_LTE_TX_CAL_LIST_B28             = 0xDf42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B28           = 0xDf43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B28           = 0xDf44,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B28                 = 0xDf45,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B28                = 0xDf46,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B28                = 0xDf47,
        EN_NV_ID_LTE_TX_TAS_B28                          = 0xDf48,

        EN_NV_ID_TEMP_SENSOR_TABLE_B28                   = 0xDf4f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B28         = 0xDf5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B28         = 0xDf5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B28                   = 0xDf5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B28                    = 0xDf5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B28               = 0xDf5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B28          = 0xDf60,
        EN_NV_ID_LTE_TX_ATTEN_B28                        = 0xDf61,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B28           = 0xDf62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B28           = 0xDf63,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B28            = 0xDf64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B28            = 0xDf65,
        EN_NV_ID_LTE_TX_APT_PARA_B28                     = 0xDf66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B28             = 0xDf67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B28             = 0xDf68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B28             = 0xDf69,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B28                          = 0xDf6a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B28                      = 0xDf6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B28                         = 0xDf6c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B28         = 0xDf6d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B28                   = 0xDf6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B28                   = 0xDf6f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B28                    = 0xDf70,
        EN_NV_ID_LTE_TX_BODY_SAR_B28                    = 0xDf71,
        EN_NV_ID_TX_PA_TEMP_COMP_B28                    = 0xDf72,
        EN_NV_ID_EXT_LNA_AGC_PARA_B28                   = 0xDf73,
        EN_NV_ID_CALB_AGC_B28                           = 0xDf74,
        //EN_NV_ID_TX_ATTEN_TABLE_B28                        = 0xDf73,
        //EN_NV_ID_POWERDET_VOLTAGE_B28                        = 0xDf74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B28               = 0xDf75,
        EN_NV_ID_LTE_TX_PD_PARA_B28                     = 0xDf76,
        EN_NV_ID_TX_ET_BAND_PARA_B28                    = 0xDf77,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B28          = 0xDf78,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B28              = 0xDf79,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B28                    = 0xDf7a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B28                    = 0xDf7b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B28                          = 0xDf7c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B28              = 0xDf7d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B28                    = 0xDf7f,

    /*新增Band26 相关NV 项*/
        EN_NV_ID_FTM_CAND_CELL_LIST_B26                 = 0xE380,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B26            = 0xE381,
        EN_NV_ID_LTE_TX_CAL_LIST_B26                    = 0xE382,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B26          = 0xE383,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B26          = 0xE384,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B26                = 0xE385,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B26               = 0xE386,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B26               = 0xE387,
        EN_NV_ID_LTE_TX_TAS_B26                         = 0xE388,

        EN_NV_ID_TEMP_SENSOR_TABLE_B26                  = 0xE38f,
        EN_NV_ID_HI6360_AGC_PARA_B26                    = 0xE398,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B26        = 0xE39b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B26        = 0xE39c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B26                  = 0xE39d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B26                   = 0xE39e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B26              = 0xE39f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B26         = 0xE3a0,
        EN_NV_ID_LTE_TX_ATTEN_B26                       = 0xE3a1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B26          = 0xE3a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B26          = 0xE3a3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B26           = 0xE3a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B26           = 0xE3a5,

        EN_NV_ID_LTE_TX_APT_PARA_B26                    = 0xE3a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B26            = 0xE3a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B26             = 0xE3a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B26             = 0xE3a9,
        EN_NV_ID_LTE_TX_MPR_B26                         = 0xE3aa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B26                     = 0xE3ab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B26                        = 0xE3ac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B26         = 0xE3ad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B26                   = 0xE3ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B26                   = 0xE3af,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B26                    = 0xE3b0,
        EN_NV_ID_LTE_TX_BODY_SAR_B26                    = 0xE3b1,
        EN_NV_ID_TX_PA_TEMP_COMP_B26                    = 0xE3b2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B26                   = 0xE3b3,
        EN_NV_ID_CALB_AGC_B26                           = 0xE3b4,
        //EN_NV_ID_TX_ATTEN_TABLE_B26                        = 0xE3b3,
        //EN_NV_ID_POWERDET_VOLTAGE_B26                        = 0xE3b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B26               = 0xE3b5,
        EN_NV_ID_LTE_TX_PD_PARA_B26                     = 0xE3b6,
        EN_NV_ID_TX_ET_BAND_PARA_B26                    = 0xE3b7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B26          = 0xE3b8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B26              = 0xe3b9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B26                   = 0xe3ba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B26                   = 0xe3bb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B26                          = 0xe3bc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B26              = 0xe37d,
        EN_NV_ID_LTE_TX_MPR_64QAM_B26                   = 0xE37f,
    /* 非标频段begin */
           /* EN_NV_ID_FTM_CAND_CELL_LIST_BNon1                 = 0xDf40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon1            = 0xDf41,
        EN_NV_ID_LTE_TX_CALIBRATION_FREQ_BNon1            = 0xDf42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon1          = 0xDf43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT37_BNon1         = 0xDf44,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon1                  = 0xDf4f,
        EN_NV_ID_HI6360_AGC_PARA_BNon1                    = 0xDf58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon1        = 0xDf5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT37_BNon1       = 0xDf5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon1                  = 0xDf5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon1                   = 0xDf5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon1              = 0xDf5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon1         = 0xDf60,
        EN_NV_ID_LTE_TX_ATTEN_BNon1                       = 0xDf61,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon1          = 0xDf62,
        EN_NV_ID_LTE_TX_CAL_HIGHGAIN_POWER_BNon1          = 0xDf63,
        EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon1           = 0xDf64,
        EN_NV_ID_LTE_TX_CAL_LOWGAIN_POWER_BNon1           = 0xDf65,
        EN_NV_ID_LTE_TX_APT_PARA_BNon1                    = 0xDf66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon1            = 0xDf67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon1             = 0xDf68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon1             = 0xDf69,
        EN_NV_ID_LTE_TX_MPR_BNon1                         = 0xDf6a,
        EN_NV_ID_LTE_ANT_SAR_BNon1                     = 0xDf6b,
    */
        /* add by  AMPR begin */
      //  EN_NV_ID_LTE_TX_AMPR_BNon1                        = 0xDf6c,
        /* add by  AMPR  END */
      //  EN_NV_ID_LTE_OTHER_COMP_BNon1         = 0xDf6d,

      //  EN_NV_ID_LTE_TX_AMPR_NS05_BNon1                   = 0xDf6e,
      //  EN_NV_ID_LTE_TX_AMPR_NS09_BNon1                   = 0xDf6f,

        /*add for V9R1_6361 Begin*/

       // EN_NV_ID_TX_RF_BB_ATT_BNon1                       = 0xDf70,
       // EN_NV_ID_TX_RF_BIAS_BNon1                         = 0xDf71,
       // EN_NV_ID_TX_PA_TEMP_COMP_BNon1                   = 0xDf72,
       // EN_NV_ID_TX_ATTEN_TABLE_BNon1                        = 0xDf73,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon1                        = 0xDf74,
        /*add for V9R1_6361 End*/
       // EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon1               = 0xDf75,
    /*BEGIN    modify for B28全频段特性*/
    /*Band128 相关NV项*/
        EN_NV_ID_FTM_CAND_CELL_LIST_B128                  = 0xDf80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B128             = 0xDf81,
        EN_NV_ID_LTE_TX_CAL_LIST_B128             = 0xDf82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B128           = 0xDf83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B128           = 0xDf84,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B128                 = 0xDf85,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B128                = 0xDf86,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B128                = 0xDf87,
        EN_NV_ID_LTE_TX_TAS_B128                          = 0xDf88,

        EN_NV_ID_TEMP_SENSOR_TABLE_B128                    = 0xDf8f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B128         = 0xDf9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B128         = 0xDf9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B128                    = 0xDf9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B128                     = 0xDf9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B128                = 0xDf9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B128          = 0xDfa0,
        EN_NV_ID_LTE_TX_ATTEN_B128                          = 0xDfa1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B128           = 0xDfa2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B128           = 0xDfa3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B128            = 0xDfa4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B128            = 0xDfa5,
        EN_NV_ID_LTE_TX_APT_PARA_B128                      = 0xDfa6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B128             = 0xDfa7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B128              = 0xDfa8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B128              = 0xDfa9,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B128                           = 0xDfaa,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B128                          = 0xDfab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B128                         = 0xDfac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B128         = 0xDfad,

        EN_NV_ID_LTE_TX_AMPR_NS05_B128                    = 0xDfae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B128                    = 0xDfaf,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B128                    = 0xDfb0,
        EN_NV_ID_LTE_TX_BODY_SAR_B128                        = 0xDfb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B128                        = 0xDfb2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B128                       = 0xDfb3,
        EN_NV_ID_CALB_AGC_B128                               = 0xDfb4,
        //EN_NV_ID_TX_ATTEN_TABLE_B128                       = 0xDfb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B128                     = 0xDfb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B128                 = 0xDfb5,
        EN_NV_ID_LTE_TX_PD_PARA_B128                       = 0xDfb6,
        EN_NV_ID_TX_ET_BAND_PARA_B128                      = 0xDfb7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B128            = 0xDfb8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B128                = 0xDfb9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B128                    = 0xDfba,
        EN_NV_ID_LTE_TX_AMPR_NS22_B128                    = 0xDfbb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B128                          = 0xDfbc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B128              = 0xDfbd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B128                    = 0xDfbf,

    /*END    modify for B28全频段特性*/
        /* modify by   for 所有band end*/


        EN_NV_ID_FTM_CAND_CELL_LIST_B29                  = 0xDfc0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B29             = 0xDfc1,
        EN_NV_ID_LTE_TX_CAL_LIST_B29                     = 0xDfc2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B29           = 0xDfc3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B29           = 0xDfc4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B29                 = 0xDfc5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B29                = 0xDfc6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B29                = 0xDfc7,
        EN_NV_ID_LTE_TX_TAS_B29                          = 0xDfc8,

        EN_NV_ID_TEMP_SENSOR_TABLE_B29                   = 0xDfcf,
        EN_NV_ID_HI6360_AGC_PARA_B29                     = 0xDfd8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B29         = 0xDfdb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B29         = 0xDfdc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B29                   = 0xDfdd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B29                    = 0xDfde,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B29               = 0xDfdf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B29          = 0xDfe0,
        EN_NV_ID_LTE_TX_ATTEN_B29                        = 0xDfe1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B29              = 0xDfe2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B29                  = 0xDfe3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B29                = 0xDfe4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B29        = 0xDfe5,
        EN_NV_ID_LTE_TX_APT_PARA_B29                     = 0xDfe6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B29             = 0xDfe7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B29              = 0xDfe8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B29              = 0xDfe9,

        /* modify  mpr begin */
        EN_NV_ID_LTE_TX_MPR_B29                          = 0xDfea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B29                         = 0xDfeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B29                         = 0xDfec,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B29                      = 0xDfed,

        EN_NV_ID_LTE_TX_AMPR_NS05_B29                   = 0xDfee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B29                   = 0xDfef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B29                    = 0xDff0,
        EN_NV_ID_LTE_TX_BODY_SAR_B29                      = 0xDff1,
        EN_NV_ID_TX_PA_TEMP_COMP_B29                      = 0xDff2,
        EN_NV_ID_EXT_LNA_AGC_PARA_B29                     = 0xDff3,
        EN_NV_ID_CALB_AGC_B29                             = 0xDff4,
        //EN_NV_ID_TX_ATTEN_TABLE_B29                        = 0xDff3,
        //EN_NV_ID_POWERDET_VOLTAGE_B29                        = 0xDff4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B29                = 0xDff5,
        EN_NV_ID_LTE_TX_PD_PARA_B29                      = 0xDff6,
        EN_NV_ID_TX_ET_BAND_PARA_B29                     = 0xDff7,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B29           = 0xDff8,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B29               = 0xDff9,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B29                    = 0xDffa,
        EN_NV_ID_LTE_TX_AMPR_NS22_B29                    = 0xDffb,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B29                          = 0xDffc,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B29              = 0xDffd,
            EN_NV_ID_LTE_TX_MPR_64QAM_B29                    = 0xDfff,



        EN_NV_ID_FTM_CAND_CELL_LIST_B32                  = 0xe000,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B32             = 0xe001,
        EN_NV_ID_LTE_TX_CAL_LIST_B32                     = 0xe002,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B32           = 0xe003,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B32           = 0xe004,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B32                 = 0xe005,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B32                = 0xe006,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B32                = 0xe007,
        EN_NV_ID_LTE_TX_TAS_B32                          = 0xe008,

        EN_NV_ID_TEMP_SENSOR_TABLE_B32                   = 0xe00f,
        EN_NV_ID_HI6360_AGC_PARA_B32                     = 0xe018,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B32         = 0xe01b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B32         = 0xe01c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B32                   = 0xe01d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B32                    = 0xe01e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B32               = 0xe01f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B32          = 0xe020,
        EN_NV_ID_LTE_TX_ATTEN_B32                        = 0xe021,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B32              = 0xe022,
        EN_NV_ID_TX_FILTER_CMP_STRU_B32                  = 0xe023,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B32                = 0xe024,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B32        = 0xe025,
        EN_NV_ID_LTE_TX_APT_PARA_B32                     = 0xe026,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B32             = 0xe027,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B32              = 0xe028,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B32              = 0xe029,

        /* modify  mpr begin */
        EN_NV_ID_LTE_TX_MPR_B32                          = 0xe02a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B32                         = 0xe02b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B32                         = 0xe02c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B32                      = 0xe02d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B32                   = 0xe02e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B32                   = 0xe02f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B32                    = 0xe030,
        EN_NV_ID_LTE_TX_BODY_SAR_B32                     = 0xe031,
        EN_NV_ID_TX_PA_TEMP_COMP_B32                     = 0xe032,
        EN_NV_ID_EXT_LNA_AGC_PARA_B32                    = 0xe033,
        EN_NV_ID_CALB_AGC_B32                            = 0xe034,
        //EN_NV_ID_TX_ATTEN_TABLE_B32                        = 0xe033,
        //EN_NV_ID_POWERDET_VOLTAGE_B32                        = 0xe034,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B32                = 0xe035,
        EN_NV_ID_LTE_TX_PD_PARA_B32                      = 0xe036,
        EN_NV_ID_TX_ET_BAND_PARA_B32                     = 0xe037,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B32           = 0xe038,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B32               = 0xe039,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B32                    = 0xe03a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B32                    = 0xe03b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B32                          = 0xe03c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B32              = 0xe03d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B32                    = 0xe03f,

        EN_NV_ID_FTM_CAND_CELL_LIST_B30                  = 0xe040,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B30             = 0xe041,
        EN_NV_ID_LTE_TX_CAL_LIST_B30                     = 0xe042,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B30           = 0xe043,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B30           = 0xe044,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_B30                 = 0xe045,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_B30                = 0xe046,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_B30                = 0xe047,
        EN_NV_ID_LTE_TX_TAS_B30                          = 0xe048,

        EN_NV_ID_TEMP_SENSOR_TABLE_B30                   = 0xe04f,
        EN_NV_ID_HI6360_AGC_PARA_B30                     = 0xe058,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B30         = 0xe05b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B30          = 0xe05c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B30                    = 0xe05d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B30                     = 0xe05e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B30                = 0xe05f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B30           = 0xe060,
        EN_NV_ID_LTE_TX_ATTEN_B30                         = 0xe061,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_B30               = 0xe062,
        EN_NV_ID_TX_FILTER_CMP_STRU_B30                   = 0xe063,
        EN_NV_ID_LTE_RESERVED_NV_STRU_B30                 = 0xe064,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B30         = 0xe065,
        EN_NV_ID_LTE_TX_APT_PARA_B30                      = 0xe066,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B30              = 0xe067,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B30               = 0xe068,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B30               = 0xe069,

        /* modify  mpr begin */
        EN_NV_ID_LTE_TX_MPR_B30                           = 0xe06a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B30                          = 0xe06b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B30                          = 0xe06c,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_B30                       = 0xe06d,

        EN_NV_ID_LTE_TX_AMPR_NS05_B30                     = 0xe06e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B30                     = 0xe06f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_B30                       = 0xe070,
        EN_NV_ID_LTE_TX_BODY_SAR_B30                      = 0xe071,
        EN_NV_ID_TX_PA_TEMP_COMP_B30                      = 0xe072,
        EN_NV_ID_EXT_LNA_AGC_PARA_B30                     = 0xe073,
        EN_NV_ID_CALB_AGC_B30                             = 0xe074,
        //EN_NV_ID_TX_ATTEN_TABLE_B30                     = 0xe073,
        //EN_NV_ID_POWERDET_VOLTAGE_B30                   = 0xe074,

        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B30                 = 0xe075,
        EN_NV_ID_LTE_TX_PD_PARA_B30                       = 0xe076,
        EN_NV_ID_TX_ET_BAND_PARA_B30                      = 0xe077,
        EN_NV_ID_LTE_CAL_PATH_ANT_NUM_PARA_B30            = 0xe078,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B30                = 0xe079,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B30                     = 0xe07a,
        EN_NV_ID_LTE_TX_AMPR_NS22_B30                     = 0xe07b,
        /* END:   Added by  , 2015/5/14 */
        /* BEGIN: Added , 2015/10/29   PN:HP 降SAR特性开发*/
        EN_NV_ID_LTE_HP_SAR_B30                           = 0xe07c,
        /* END:   Added , 2015/10/29 */
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B30               = 0xe07d,
            EN_NV_ID_LTE_TX_MPR_64QAM_B30                    = 0xe07f,






        EN_NV_ID_FTM_CAND_CELL_LIST_BNon6                 = 0xe080,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon6            = 0xe081,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon6            = 0xe082,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon6          = 0xe083,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon6         = 0xe084,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon6                = 0xe085,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon6               = 0xe086,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon6               = 0xe087,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon6                  = 0xe08f,
        EN_NV_ID_HI6360_AGC_PARA_BNon6                    = 0xe098,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon6        = 0xe09b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon6       = 0xe09c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon6                  = 0xe09d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon6                   = 0xe09e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon6              = 0xe09f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon6         = 0xe0a0,
        EN_NV_ID_LTE_TX_ATTEN_BNon6                       = 0xe0a1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon6          = 0xe0a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon6          = 0xe0a3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon6           = 0xe0a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon6           = 0xe0a5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon6                    = 0xe0a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon6            = 0xe0a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon6             = 0xe0a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon6             = 0xe0a9,
        EN_NV_ID_LTE_TX_MPR_BNon6                         = 0xe0aa,
        EN_NV_ID_LTE_ANT_SAR_BNon6                     = 0xe0ab,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon6                        = 0xe0ac,
        /* add by  AMPR  END */
        EN_NV_ID_LTE_OTHER_COMP_BNon6         = 0xe0ad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon6                   = 0xe0ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon6                   = 0xe0af,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon6                    = 0xe0b0,
        //EN_NV_ID_TX_RF_BB_ATT_BNon6                       = 0xe0b0,
        //EN_NV_ID_TX_RF_BIAS_BNon6                         = 0xe0b1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon6                     = 0xe0b2,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon6                    = 0xe0b3,
        EN_NV_ID_CALB_AGC_BNon6                            = 0xe0b4,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon6                        = 0xe0b3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon6                        = 0xe0b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon6               = 0xe0b5,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon7                 = 0xe0c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon7            = 0xe0c1,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon7            = 0xe0c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon7          = 0xe0c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon7         = 0xe0c4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon7                = 0xe0c5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon7               = 0xe0c6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon7               = 0xe0c7,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon7                  = 0xe0cf,
        EN_NV_ID_HI6360_AGC_PARA_BNon7                    = 0xe0d8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon7        = 0xe0db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon7       = 0xe0dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon7                  = 0xe0dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon7                   = 0xe0de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon7              = 0xe0df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon7         = 0xe0e0,
        EN_NV_ID_LTE_TX_ATTEN_BNon7                       = 0xe0e1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon7          = 0xe0e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon7          = 0xe0e3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon7           = 0xe0e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon7           = 0xe0e5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon7                    = 0xe0e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon7            = 0xe0e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon7             = 0xe0e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon7             = 0xe0e9,
        EN_NV_ID_LTE_TX_MPR_BNon7                         = 0xe0ea,
        EN_NV_ID_LTE_ANT_SAR_BNon7                     = 0xe0eb,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon7                        = 0xe0ec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon7         = 0xe0ed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon7                   = 0xe0ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon7                   = 0xe0ef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon7                    = 0xe0f0,
        //EN_NV_ID_TX_RF_BB_ATT_BNon7                       = 0xe0f0,
        //EN_NV_ID_TX_RF_BIAS_BNon7                         = 0xe0f1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon7                      = 0xe0f2,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon7                     = 0xe0f3,
        EN_NV_ID_CALB_AGC_BNon7                             = 0xe0f4,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon7                        = 0xe0f3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon7                        = 0xe0f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon7               = 0xe0f5,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon8                 = 0xe100,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon8            = 0xe101,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon8            = 0xe102,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon8          = 0xe103,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon8         = 0xe104,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon8                = 0xe105,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon8               = 0xe106,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon8               = 0xe107,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon8                  = 0xe10f,
        EN_NV_ID_HI6360_AGC_PARA_BNon8                    = 0xe118,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon8        = 0xe11b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon8       = 0xe11c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon8                  = 0xe11d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon8                   = 0xe11e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon8              = 0xe11f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon8         = 0xe120,
        EN_NV_ID_LTE_TX_ATTEN_BNon8                       = 0xe121,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon8          = 0xe122,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon8          = 0xe123,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon8           = 0xe124,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon8           = 0xe125,

        EN_NV_ID_LTE_TX_APT_PARA_BNon8                    = 0xe126,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon8            = 0xe127,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon8             = 0xe128,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon8             = 0xe129,
        EN_NV_ID_LTE_TX_MPR_BNon8                         = 0xe12a,
        EN_NV_ID_LTE_ANT_SAR_BNon8                     = 0xe12b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon8                        = 0xe12c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon8         = 0xe12d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon8                   = 0xe12e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon8                   = 0xe12f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon8                    = 0xe130,
        //EN_NV_ID_TX_RF_BB_ATT_BNon8                       = 0xe130,
        //EN_NV_ID_TX_RF_BIAS_BNon8                         = 0xe131,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon8                      = 0xe132,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon8                     = 0xe133,
        EN_NV_ID_CALB_AGC_BNon8                             = 0xe134,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon8                        = 0xe133,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon8                        = 0xe134,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon8               = 0xe135,

        /* 非标频段begin */
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon9                 = 0xe140,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon9            = 0xe141,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon9            = 0xe142,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon9          = 0xe143,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon9         = 0xe144,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon9                = 0xe145,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon9               = 0xe146,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon9               = 0xe147,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon9                  = 0xe14f,
        EN_NV_ID_HI6360_AGC_PARA_BNon9                    = 0xe158,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon9        = 0xe15b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon9       = 0xe15c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon9                  = 0xe15d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon9                   = 0xe15e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon9              = 0xe15f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon9         = 0xe160,
        EN_NV_ID_LTE_TX_ATTEN_BNon9                       = 0xe161,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon9          = 0xe162,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon9          = 0xe163,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon9           = 0xe164,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon9           = 0xe165,
        EN_NV_ID_LTE_TX_APT_PARA_BNon9                    = 0xe166,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon9            = 0xe167,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon9             = 0xe168,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon9             = 0xe169,
        EN_NV_ID_LTE_TX_MPR_BNon9                         = 0xe16a,
        EN_NV_ID_LTE_ANT_SAR_BNon9                        = 0xe16b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon9                        = 0xe16c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon9         = 0xe16d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon9                   = 0xe16e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon9                   = 0xe16f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon9                    = 0xe170,
        //EN_NV_ID_TX_RF_BB_ATT_BNon9                       = 0xe170,
        //EN_NV_ID_TX_RF_BIAS_BNon9                         = 0xe171,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon9                      = 0xe172,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon9                     = 0xe173,
        EN_NV_ID_CALB_AGC_BNon9                             = 0xe174,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon9                        = 0xe173,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon9                        = 0xe174,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon9               = 0xe175,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon10                 = 0xe180,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon10            = 0xe181,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon10            = 0xe182,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon10          = 0xe183,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon10         = 0xe184,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon10                = 0xe185,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon10               = 0xe186,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon10               = 0xe187,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon10                  = 0xe18f,
        EN_NV_ID_HI6360_AGC_PARA_BNon10                    = 0xe198,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon10        = 0xe19b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon10       = 0xe19c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon10                  = 0xe19d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon10                   = 0xe19e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon10              = 0xe19f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon10         = 0xe1a0,
        EN_NV_ID_LTE_TX_ATTEN_BNon10                       = 0xe1a1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon10          = 0xe1a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon10          = 0xe1a3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon10           = 0xe1a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon10           = 0xe1a5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon10                    = 0xe1a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon10            = 0xe1a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon10             = 0xe1a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon10             = 0xe1a9,
        EN_NV_ID_LTE_TX_MPR_BNon10                         = 0xe1aa,
        EN_NV_ID_LTE_ANT_SAR_BNon10                     = 0xe1ab,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon10                        = 0xe1ac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon10         = 0xe1ad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon10                   = 0xe1ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon10                   = 0xe1af,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon10                    = 0xe1b0,
        //EN_NV_ID_TX_RF_BB_ATT_BNon10                       = 0xe1b0,
        //EN_NV_ID_TX_RF_BIAS_BNon10                         = 0xe1b1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon10                      = 0xe1b2,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon10                     = 0xe1b3,
        EN_NV_ID_CALB_AGC_BNon10                             = 0xe1b4,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon10                        = 0xe1b3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon10                        = 0xe1b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon10               = 0xe1b5,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon11                 = 0xe1c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon11            = 0xe1c1,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon11            = 0xe1c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon11          = 0xe1c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon11         = 0xe1c4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon11                = 0xe1c5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon11               = 0xe1c6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon11               = 0xe1c7,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon11                  = 0xe1cf,
        EN_NV_ID_HI6360_AGC_PARA_BNon11                    = 0xe1d8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon11        = 0xe1db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon11       = 0xe1dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon11                  = 0xe1dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon11                   = 0xe1de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon11              = 0xe1df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon11         = 0xe1e0,
        EN_NV_ID_LTE_TX_ATTEN_BNon11                       = 0xe1e1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon11          = 0xe1e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon11          = 0xe1e3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon11           = 0xe1e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon11           = 0xe1e5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon11                    = 0xe1e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon11            = 0xe1e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon11             = 0xe1e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon11             = 0xe1e9,
        EN_NV_ID_LTE_TX_MPR_BNon11                         = 0xe1ea,
        EN_NV_ID_LTE_ANT_SAR_BNon11                     = 0xe1eb,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon11                        = 0xe1ec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon11         = 0xe1ed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon11                   = 0xe1ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon11                   = 0xe1ef,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon11                    = 0xe1f0,
        //EN_NV_ID_TX_RF_BB_ATT_BNon11                       = 0xe1f0,
        //EN_NV_ID_TX_RF_BIAS_BNon11                         = 0xe1f1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon11                      = 0xe1f2,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon11                     = 0xe1f3,
        EN_NV_ID_CALB_AGC_BNon11                             = 0xe1f4,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon11                        = 0xe1f3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon11                        = 0xe1f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon11               = 0xe1f5,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon12                 = 0xe200,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon12            = 0xe201,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon12            = 0xe202,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon12          = 0xe203,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon12         = 0xe204,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon12                = 0xe205,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon12               = 0xe206,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon12               = 0xe207,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon12                  = 0xe20f,
        EN_NV_ID_HI6360_AGC_PARA_BNon12                    = 0xe218,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon12        = 0xe21b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon12       = 0xe21c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon12                  = 0xe21d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon12                   = 0xe21e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon12              = 0xe21f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon12         = 0xe220,
        EN_NV_ID_LTE_TX_ATTEN_BNon12                       = 0xe221,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon12          = 0xe222,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon12          = 0xe223,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon12           = 0xe224,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon12           = 0xe225,

        EN_NV_ID_LTE_TX_APT_PARA_BNon12                    = 0xe226,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon12            = 0xe227,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon12             = 0xe228,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon12             = 0xe229,
        EN_NV_ID_LTE_TX_MPR_BNon12                         = 0xe22a,
        EN_NV_ID_LTE_ANT_SAR_BNon12                     = 0xe22b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon12                        = 0xe22c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon12         = 0xe22d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon12                   = 0xe22e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon12                   = 0xe22f,


        EN_NV_ID_LTE_TX_ANT_SEL_BNon12                    = 0xe230,
        //EN_NV_ID_TX_RF_BB_ATT_BNon12                       = 0xe230,
        //EN_NV_ID_TX_RF_BIAS_BNon12                         = 0xe231,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon12                      = 0xe232,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon12                     = 0xe233,
        EN_NV_ID_CALB_AGC_BNon12                             = 0xe234,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon12                        = 0xe233,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon12                        = 0xe234,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon12               = 0xe235,
        /* 非标频段begin */
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon13                 = 0xe240,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon13            = 0xe241,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon13            = 0xe242,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon13          = 0xe243,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon13         = 0xe244,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon13                = 0xe245,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon13               = 0xe246,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon13               = 0xe247,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon13                  = 0xe24f,
        EN_NV_ID_HI6360_AGC_PARA_BNon13                    = 0xe258,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon13        = 0xe25b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon13       = 0xe25c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon13                  = 0xe25d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon13                   = 0xe25e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon13              = 0xe25f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon13         = 0xe260,
        EN_NV_ID_LTE_TX_ATTEN_BNon13                       = 0xe261,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon13          = 0xe262,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon13          = 0xe263,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon13           = 0xe264,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon13           = 0xe265,
        EN_NV_ID_LTE_TX_APT_PARA_BNon13                    = 0xe266,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon13            = 0xe267,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon13             = 0xe268,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon13             = 0xe269,
        EN_NV_ID_LTE_TX_MPR_BNon13                         = 0xe26a,
        EN_NV_ID_LTE_ANT_SAR_BNon13                     = 0xe26b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon13                        = 0xe26c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon13         = 0xe26d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon13                   = 0xe26e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon13                   = 0xe26f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon13                    = 0xe270,
        //EN_NV_ID_TX_RF_BB_ATT_BNon13                       = 0xe270,
        //EN_NV_ID_TX_RF_BIAS_BNon13                         = 0xe271,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon13                      = 0xe272,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon13                     = 0xe273,
        EN_NV_ID_CALB_AGC_BNon13                             = 0xe274,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon13                        = 0xe273,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon13                        = 0xe274,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon13               = 0xe275,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon14                 = 0xe280,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon14            = 0xe281,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon14            = 0xe282,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon14          = 0xe283,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon14         = 0xe284,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon14                = 0xe285,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon14               = 0xe286,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon14               = 0xe287,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon14                  = 0xe28f,
        EN_NV_ID_HI6360_AGC_PARA_BNon14                    = 0xe298,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon14        = 0xe29b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon14       = 0xe29c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon14                  = 0xe29d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon14                   = 0xe29e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon14              = 0xe29f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon14         = 0xe2a0,
        EN_NV_ID_LTE_TX_ATTEN_BNon14                       = 0xe2a1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon14          = 0xe2a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon14          = 0xe2a3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon14           = 0xe2a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon14           = 0xe2a5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon14                    = 0xe2a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon14            = 0xe2a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon14             = 0xe2a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon14             = 0xe2a9,
        EN_NV_ID_LTE_TX_MPR_BNon14                         = 0xe2aa,
        EN_NV_ID_LTE_ANT_SAR_BNon14                     = 0xe2ab,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon14                        = 0xe2ac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon14         = 0xe2ad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon14                   = 0xe2ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon14                   = 0xe2af,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon14                    = 0xe2b0,
        //EN_NV_ID_TX_RF_BB_ATT_BNon14                       = 0xe2b0,
        //EN_NV_ID_TX_RF_BIAS_BNon14                         = 0xe2b1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon14                      = 0xe2b2,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon14                     = 0xe2b3,
        EN_NV_ID_CALB_AGC_BNon14                             = 0xe2b4,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon14                        = 0xe2b3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon14                        = 0xe2b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon14               = 0xe2b5,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon15                 = 0xe2c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon15            = 0xe2c1,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon15            = 0xe2c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon15          = 0xe2c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon15         = 0xe2c4,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon15                = 0xe2c5,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon15               = 0xe2c6,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon15               = 0xe2c7,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon15                  = 0xe2cf,
        EN_NV_ID_HI6360_AGC_PARA_BNon15                    = 0xe2d8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon15        = 0xe2db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon15       = 0xe2dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon15                  = 0xe2dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon15                   = 0xe2de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon15              = 0xe2df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon15         = 0xe2e0,
        EN_NV_ID_LTE_TX_ATTEN_BNon15                       = 0xe2e1,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon15          = 0xe2e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon15          = 0xe2e3,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon15           = 0xe2e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon15           = 0xe2e5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon15                    = 0xe2e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon15            = 0xe2e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon15             = 0xe2e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon15             = 0xe2e9,
        EN_NV_ID_LTE_TX_MPR_BNon15                         = 0xe2ea,
        EN_NV_ID_LTE_ANT_SAR_BNon15                     = 0xe2eb,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon15                        = 0xe2ec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon15         = 0xe2ed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon15                   = 0xe2ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon15                   = 0xe2ef,

        EN_NV_ID_LTE_TX_ANT_SEL_BNon15                    = 0xe2f0,
        //EN_NV_ID_TX_RF_BB_ATT_BNon15                       = 0xe2f0,
        //EN_NV_ID_TX_RF_BIAS_BNon15                         = 0xe2f1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon15                      = 0xe2f2,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon15                     = 0xe2f3,
        EN_NV_ID_CALB_AGC_BNon15                             = 0xe2f4,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon15                        = 0xe2f3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon15                        = 0xe2f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon15               = 0xe2f5,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B15                      = 0xe2f6,
        EN_NV_ID_LTE_TX_AMPR_NS22_B15                      = 0xe2f7,
        /* END:   Added by  , 2015/5/14 */

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon16                 = 0xe300,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon16            = 0xe301,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon16            = 0xe302,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon16          = 0xe303,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_BNon16         = 0xe304,
        EN_NV_ID_TX_PA_TEMP_COMP_LOW_BNon16                = 0xe305,
        EN_NV_ID_TX_PA_TEMP_COMP_HIGH_BNon16               = 0xe306,
        EN_NV_ID_TX_PA_TEMP_COMP_CTRL_BNon16               = 0xe307,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon16                  = 0xe30f,
        EN_NV_ID_HI6360_AGC_PARA_BNon16                    = 0xe318,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon16        = 0xe31b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_BNon16       = 0xe31c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon16                  = 0xe31d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon16                   = 0xe31e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon16              = 0xe31f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon16         = 0xe320,
        EN_NV_ID_LTE_TX_ATTEN_BNon16                       = 0xe321,
        EN_NV_ID_LTE_TXIQ_CAL_PARA_STRU_BNon16          = 0xe322,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon16          = 0xe323,
        EN_NV_ID_LTE_RESERVED_NV_STRU_BNon16           = 0xe324,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon16           = 0xe325,

        EN_NV_ID_LTE_TX_APT_PARA_BNon16                    = 0xe326,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon16            = 0xe327,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon16             = 0xe328,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon16             = 0xe329,
        EN_NV_ID_LTE_TX_MPR_BNon16                         = 0xe32a,
        EN_NV_ID_LTE_ANT_SAR_BNon16                     = 0xe32b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon16                        = 0xe32c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon16         = 0xe32d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon16                   = 0xe32e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon16                   = 0xe32f,

        /*add for V9R1_6361 Begin*/

        EN_NV_ID_LTE_TX_ANT_SEL_BNon16                    = 0xe220,
        //EN_NV_ID_TX_RF_BB_ATT_BNon16                       = 0xe330,
        //EN_NV_ID_TX_RF_BIAS_BNon16                         = 0xe331,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon16                      = 0xe332,
        EN_NV_ID_EXT_LNA_AGC_PARA_BNon16                     = 0xe333,
        EN_NV_ID_CALB_AGC_BNon16                             = 0xe334,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon16                        = 0xe333,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon16                        = 0xe334,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon16               = 0xe335,
        /* BEGIN: Added by  , 2015/5/14   问题单号:V7R5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_NS03_B16                      = 0xe336,
        EN_NV_ID_LTE_TX_AMPR_NS22_B16                      = 0xe337,
        /* END:   Added by  , 2015/5/14 */

        /* 非标频段end */
        EN_NV_ID_MODEM_END                              = 0xE4ff,
        /*Modem end 0xE4FF00*/

        EN_NV_ID_TCXO_DYN_CONFIG                        = 0x1401,

        /*begin: add by   for DSP NV,<2012-6-11>*/
    #if 1
        EN_NV_ID_DSP2RF_CFG                             = 0xe400,
        EN_NV_ID_RF_ADVANCE_TIME                        = 0xe401,
        EN_NV_ID_RF_AGC_PARA_CODE                       = 0xe402,
        EN_NV_ID_RF_AGC_PARA_UPTHRGAIN                  = 0xe403,
        EN_NV_ID_RF_AGC_PARA_DOWNTHRGAIN                = 0xe404,
        EN_NV_ID_RF_AGC_PARA_TOTALGAIN                  = 0xe405,
        EN_NV_ID_RF_ACS_PARA                            = 0xe406,
        EN_NV_ID_AGC_CAL_PARA                           = 0xe407,
        EN_NV_ID_RF_APC_PARA                            = 0xe408,
        EN_NV_ID_MAXPOWER                               = 0xE4e0,
        EN_NV_ID_TX_APCOFF_REDUCE                       = 0xE4e1,
        EN_NV_ID_TX_CAL_FREQ_LIST_0                     = 0xE4e3,
        EN_NV_ID_TX_CAL_FREQ_LIST_1                     = 0xE4e9,
        EN_NV_ID_TX_CAL_FREQ_LIST_2                     = 0xE4ef,
        EN_NV_ID_RX_CAL_FREQ_LIST_0                     = 0xE4e4,
        EN_NV_ID_RX_CAL_FREQ_LIST_1                     = 0xE4ea,
        EN_NV_ID_RX_CAL_FREQ_LIST_2                     = 0xE4f0,
        EN_NV_ID_TEMP_SEMSOR_0                          = 0xE4e5,
        EN_NV_ID_TEMP_SEMSOR_1                          = 0xE4eb,
        EN_NV_ID_TEMP_SEMSOR_2                          = 0xE4f1,
        EN_NV_ID_TX_TEMP_COMP_0                         = 0xE4e6,
        EN_NV_ID_TX_TEMP_COMP_1                         = 0xE4ec,
        EN_NV_ID_TX_TEMP_COMP_2                         = 0xE4f2,
        EN_NV_ID_RX_TEMP_COMP_0                         = 0xE4e7,
        EN_NV_ID_RX_TEMP_COMP_1                         = 0xE4ed,
        EN_NV_ID_RX_TEMP_COMP_2                         = 0xE4f3,
        EN_NV_ID_PA_POWER_0                             = 0xF8f0,
        EN_NV_ID_PA_POWER_1                             = 0xF8f5,
        EN_NV_ID_PA_POWER_2                             = 0xF8fa,
        EN_NV_ID_TX_APC_COMP_0                          = 0xF8f1,
        EN_NV_ID_TX_APC_COMP_1                          = 0xF8f6,
        EN_NV_ID_TX_APC_COMP_2                          = 0xF8fb,
        EN_NV_ID_TX_APC_FREQ_COMP_0                     = 0xF8f2,
        EN_NV_ID_TX_APC_FREQ_COMP_1                     = 0xF8f7,
        EN_NV_ID_TX_APC_FREQ_COMP_2                     = 0xF8fc,
        EN_NV_ID_RX_AGC_COMP_0                          = 0xF8f3,
        EN_NV_ID_RX_AGC_COMP_1                          = 0xF8f8,
        EN_NV_ID_RX_AGC_COMP_2                          = 0xF8fd,
        EN_NV_ID_RX_AGC_FREQ_COMP_0                     = 0xF8f4,
        EN_NV_ID_RX_AGC_FREQ_COMP_1                     = 0xF8f9,
        EN_NV_ID_RX_AGC_FREQ_COMP_2                     = 0xF8fe,
        EN_NV_ID_TX_PA_LEVEL_THRE                     = 0xE409,
        EN_NV_ID_US_TCXO_INIT                         = 0xE900,
        EN_NV_ID_DRX_PARA                             = 0xE40a,
    #endif
        /*end: add by   for DSP NV,<2012-6-11>*/
        /*mipi apt begin*/
        EN_TDS_NV_MIPI_APT                            = 0xE40c,
        /*mipi apt end*/

        /*begin: add by   for V9R1 DSPNV*/
        EN_NV_ID_TDS_RFIC_CFG_STRU_DEFAULT              = 0xe499,
        EN_NV_ID_TDS_LINECTRL_ALLOT_BY_HARDWARE_STRU_DEFAULT = 0xe49a,
        EN_NV_ID_TDS_RF_ADVANCE_TIME_STU_DEFAULT        = 0xe49b,
        EN_NV_ID_TDS_MIPI_CTRL_DEFAULT              = 0xe49c,

        /*  k3v3+tas 20140915 begin*/
        EN_NV_ID_TDS_TAS_STRU_DEFAULT                   = 0xe49d,
        EN_NV_ID_TDS_TAS_RF_STRU_DEFAULT                = 0xe49e,
        EN_NV_ID_TDS_TAS_SEARCH_STRU_DEFAULT            = 0xe49f,

        EN_NV_ID_TDS_TAS_EXTRA_MODEM_GPIO               = 0xe480,
        EN_NV_ID_TDS_TAS_DPDT_PROTECT_PARA              = 0xe481,

        EN_NV_ID_TDS_TAS_HAPPY_STRU_DEFAULT             = 0xe482,

        EN_NV_ID_TDS_TAS_BLIND_STRU_DEFAULT             = 0xe484,
        EN_NV_ID_TDS_MAS_PARA_STRU_DEFAULT              = 0xe485,
        EN_NV_ID_TL_MAS_GPIO_STRU_DEFAULT               = 0xe486,
        EN_NV_ID_TDS_TAS_DPDT_MIPI_CTRL_WORD            = 0xe487,
        /* +tas 20140915 end*/

        /*Band34*/
        /*modem nv*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B34             = 0xe4a0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B34               = 0xe4a1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B34                = 0xe4a2,
        //EN_NV_ID_TDS_TX_RF_BIAS_B34             = 0xe4a3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_B34              = 0xe4a4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B34                  = 0xe4a5,
        EN_NV_ID_TDS_RX_CAL_FREQ_B34                    = 0xe4a6,
        EN_NV_ID_TDS_AGC_BAND_PARA_B34                  = 0xe4a7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B34                  = 0xe4a8,
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_B34                 = 0xe4a9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_B34                    = 0xe4aa,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_B34                   = 0xe4ab,
        EN_NV_ID_TDS_PA_TEMP_DET_CHANNEL_STRU_B34       = 0xe4ac,
        /*工厂nv*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B34           = 0xf8a0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B34         = 0xf8a1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B34    = 0xf8a2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B34                  = 0xf8a3,
        EN_NV_ID_TDS_DCOC_CAL_B34                       = 0xf8a4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B34                    = 0xf8a5,
        EN_NV_ID_TDS_APC_TABLE_STRU_B34                 = 0xf8a6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_B34                = 0xf8a7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_B34          = 0xf8a8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_B34          = 0xf8a9,
        /*Band39*/
        /*modem nv*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B39             = 0xe4b0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B39               = 0xe4b1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B39                = 0xe4b2,
        //EN_NV_ID_TDS_TX_RF_BIAS_B39             = 0xe4b3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_B39              = 0xe4b4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B39                  = 0xe4b5,
        EN_NV_ID_TDS_RX_CAL_FREQ_B39                    = 0xe4b6,
        EN_NV_ID_TDS_AGC_BAND_PARA_B39                  = 0xe4b7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B39                  = 0xe4b8,
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_B39                 = 0xe4b9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_B39                    = 0xe4ba,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_B39                   = 0xe4bb,
        EN_NV_ID_TDS_PA_TEMP_DET_CHANNEL_STRU_B39       = 0xe4bc,
        /*工厂nv*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B39           = 0xf8b0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B39         = 0xf8b1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B39    = 0xf8b2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B39                  = 0xf8b3,
        EN_NV_ID_TDS_DCOC_CAL_B39                       = 0xf8b4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B39                    = 0xf8b5,
        EN_NV_ID_TDS_APC_TABLE_STRU_B39                     = 0xf8b6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_B39                = 0xf8b7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_B39              = 0xf8b8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_B39              = 0xf8b9,

        /*Band40*/
        /*modem nv*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B40             = 0xe4c0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B40               = 0xe4c1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B40                = 0xe4c2,
        //EN_NV_ID_TDS_TX_RF_BIAS_B40                       = 0xe4c3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_B40              = 0xe4c4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B40                  = 0xe4c5,
        EN_NV_ID_TDS_RX_CAL_FREQ_B40                    = 0xe4c6,
        EN_NV_ID_TDS_AGC_BAND_PARA_B40                  = 0xe4c7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B40                  = 0xe4c8,
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_B40                 = 0xe4c9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_B40                    = 0xe4ca,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_B40                   = 0xe4cb,

        /*Band140*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B140            = 0xE5c0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B140              = 0xE5c1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B140               = 0xE5c2,
        //EN_NV_ID_TDS_TX_RF_BIAS_B140                      = 0xE5c3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_B140             = 0xE5c4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B140                 = 0xE5c5,
        EN_NV_ID_TDS_RX_CAL_FREQ_B140                   = 0xE5c6,
        EN_NV_ID_TDS_AGC_BAND_PARA_B140                 = 0xE5c7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B140                 = 0xE5c8,
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_B140                = 0xE5c9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_B140                   = 0xE5ca,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_B140                  = 0xE5cb,

        /*工厂nv*/
        /*Band40*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B40           = 0xf8c0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B40         = 0xf8c1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B40    = 0xf8c2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B40                  = 0xf8c3,
        EN_NV_ID_TDS_DCOC_CAL_B40                       = 0xf8c4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B40                    = 0xf8c5,
        EN_NV_ID_TDS_APC_TABLE_STRU_B40                 = 0xf8c6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_B40                = 0xf8c7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_B40              = 0xf8c8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_B40              = 0xf8c9,

        /*Band140*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B140           = 0xFac0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B140         = 0xFac1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B140    = 0xFac2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B140                  = 0xFac3,
        EN_NV_ID_TDS_DCOC_CAL_B140                       = 0xFac4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B140                    = 0xFac5,
        EN_NV_ID_TDS_APC_TABLE_STRU_B140                 = 0xFac6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_B140                = 0xFac7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_B140              = 0xFac8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_B140              = 0xFac9,

        /*Band_Reserved*/
        /*modem nv*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_BRESERVED       = 0xe4d0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_BRESERVED                 = 0xe4d1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_BRESERVED          = 0xe4d2,
        //EN_NV_ID_TDS_TX_RF_BIAS_BRESERVED       = 0xe4d3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_BRESERVED            = 0xe4d4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_BRESERVED            = 0xe4d5,
        EN_NV_ID_TDS_RX_CAL_FREQ_BRESERVED              = 0xe4d6,
        EN_NV_ID_TDS_AGC_BAND_PARA_BRESERVED            = 0xe4d7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_BRESERVED            = 0xe4d8,
/* BEGIN: Added by  , 2016/2/26   PN:Chicago Ext LNA Feature*/
        EN_NV_ID_TDS_AGC_GAIN_CALBR_B34                 = 0xe4d9,
        EN_NV_ID_TDS_AGC_GAIN_CALBR_B39                 = 0xe4da,
        EN_NV_ID_TDS_EXT_LNA_CFG_PARA                   = 0xe4db,
        EN_NV_ID_TDS_EXT_LNA_AGC_PARA                   = 0xe4dc,
        EN_NV_ID_TDS_FRE_ERR                            = 0xe4dd,
/* END:   Added by  , 2016/2/26 */
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_BRESERVED       = 0xe4d9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_BRESERVED          = 0xe4da,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_BRESERVED         = 0xe4db,
        /*工厂nv*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_BRESERVED             = 0xf8d0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_BRESERVED       = 0xf8d1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_BRESERVED      = 0xf8d2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_BRESERVED            = 0xf8d3,
        EN_NV_ID_TDS_DCOC_CAL_BRESERVED                 = 0xf8d4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_BRESERVED              = 0xf8d5,
        EN_NV_ID_TDS_APC_TABLE_STRU_BRESERVED               = 0xf8d6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_BRESERVED      = 0xf8d7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_BRESERVED        = 0xf8a8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_BRESERVED        = 0xf8a9,

        /*end: add by   for V9R1 DSPNV*/

        EN_NV_ID_LTE_TCXO_INIT_FREQ                     = 0xe900,
        EN_NV_ID_TDS_NV_TUNER_PARA                      = 0xe901,
        EN_NV_ID_TDS_DRX_PARA                           = 0xe902,
        EN_NV_ID_TDS_VRAMP_PARA                         = 0xe903,
/* BEGIN: Added , 2015/11/5   PN:HP 降SAR特性开发*/
        EN_NV_ID_TDS_HP_SAR                             = 0xe904,
/* END:   Added , 2015/11/5 */

        EN_NV_ID_ANT_MODEM_LOSS_B20                     = 0xeb00,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B20    =    0xeb01,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B20    =    0xeb02,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B20    =    0xeb03,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B20    =    0xeb04,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B20    =    0xeb05,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B20    =    0xeb06,
        EN_NV_ID_DPD_FAC_LUT_03_B20    =    0xeb07,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B20    =    0xeb08,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B20         = 0xeb0d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B20         = 0xeb0e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B20       = 0xeb0f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B20       = 0xeb10,
        EN_NV_ID_LTE_IP2_CAL_B20                        = 0xeb11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B20      = 0xeb12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B20       = 0xeb13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B20       = 0xeb14,
        //EN_NV_ID_LTE_PA_POWER_B20                       = 0xeb15,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B20        = 0xeb16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B20         = 0xeb17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B20         = 0xeb18,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B20                    = 0xeb19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B20                  = 0xeb1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B20                  = 0xeb1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B20                       = 0xeb1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B20                       = 0xeb1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B20               = 0xeb1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B20              = 0xeb1f,



        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B20               = 0xeb25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B20              = 0xeb26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B20             = 0xeb27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B20          = 0xeb28,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B20     = 0xeb29,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B20   = 0xeb2a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B20  = 0xeb2b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B20  = 0xeb2c,*/
        EN_NV_ID_DPD_LAB_PARA_B20                      = 0xeb2d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B20                  = 0xeb2e,
        EN_NV_ID_DPD_FAC_RESULT_B20                 = 0xeb2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B20                   = 0xeb30,
        EN_NV_ID_DPD_FAC_LUT_00_B20                    = 0xeb31,
        EN_NV_ID_DPD_FAC_LUT_01_B20                    = 0xeb32,
        EN_NV_ID_DPD_FAC_LUT_02_B20                           = 0xeb33,
        EN_NV_ID_RF_CA_RCCODE_B20                        = 0xeb34,


        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B40                     = 0xeb40,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B40    =    0xeb41,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B40    =    0xeb42,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B40    =    0xeb43,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B40    =    0xeb44,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B40    =    0xeb45,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B40    =    0xeb46,
        EN_NV_ID_DPD_FAC_LUT_03_B40    =    0xeb47,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B40    =    0xeb48,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B40         = 0xeb4D,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B40         = 0xeb4E,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B40       = 0xeb4F,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B40       = 0xeb50,
        EN_NV_ID_LTE_IP2_CAL_B40                        = 0xeb51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B40      = 0xeb52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B40       = 0xeb53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B40       = 0xeb54,
        //EN_NV_ID_LTE_PA_POWER_B40                       = 0xeb55,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B40        = 0xeb56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B40         = 0xeb57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B40         = 0xeb58,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B40                    = 0xeb59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B40                  = 0xeb5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B40                  = 0xeb5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B40                       = 0xeb5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B40                       = 0xeb5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B40               = 0xeb5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B40              = 0xeb5f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B40               = 0xeb65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B40              = 0xeb66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B40             = 0xeb67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B40          = 0xeb68,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B40     = 0xeb69,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B40   = 0xeb6a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B40  = 0xeb6b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B40  = 0xeb6c,*/
        EN_NV_ID_DPD_LAB_PARA_B40                   = 0xeb6d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B40             = 0xeb6e,
        EN_NV_ID_DPD_FAC_RESULT_B40                 = 0xeb6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B40                   = 0xeb70,
        EN_NV_ID_DPD_FAC_LUT_00_B40                    = 0xeb71,
        EN_NV_ID_DPD_FAC_LUT_01_B40                    = 0xeb72,
        EN_NV_ID_DPD_FAC_LUT_02_B40                           = 0xeb73,
        EN_NV_ID_RF_CA_RCCODE_B40                        = 0xeb74,

        EN_NV_ID_ANT_MODEM_LOSS_B140                     = 0xEc40,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B140    =    0xEc41,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B140    =    0xEc42,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B140    =    0xEc43,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B140    =    0xEc44,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B140    =    0xEc45,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B140    =    0xEc46,
        EN_NV_ID_DPD_FAC_LUT_03_B140    =    0xEc47,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B140    =    0xEc48,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B140         = 0xEc4D,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B140         = 0xEc4E,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B140       = 0xEc4F,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B140       = 0xEc50,
        EN_NV_ID_LTE_IP2_CAL_B140                        = 0xEc51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B140      = 0xEc52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B140       = 0xEc53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B140       = 0xEc54,
        //EN_NV_ID_LTE_PA_POWER_B140                       = 0xEc55,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B140        = 0xEc56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B140         = 0xEc57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B140         = 0xEc58,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B140                    = 0xEc59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B140                  = 0xEc5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B140                  = 0xEc5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B140                       = 0xEc5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B140                       = 0xEc5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B140               = 0xEc5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B140              = 0xEc5f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B140               = 0xEc65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B140              = 0xEc66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B140             = 0xEc67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B140          = 0xEc68,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B140     = 0xEc69,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B140   = 0xEc6a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B140  = 0xEc6b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B140  = 0xEc6c,*/
        EN_NV_ID_DPD_LAB_PARA_B140                   = 0xEc6d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B140             = 0xEc6e,
        EN_NV_ID_DPD_FAC_RESULT_B140                 = 0xEc6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B140                   = 0xEc70,
        EN_NV_ID_DPD_FAC_LUT_00_B140                    = 0xEc71,
        EN_NV_ID_DPD_FAC_LUT_01_B140                    = 0xEc72,
        EN_NV_ID_DPD_FAC_LUT_02_B140                           = 0xEc73,
        EN_NV_ID_RF_CA_RCCODE_B140                        = 0xEc74,

        EN_NV_ID_ANT_MODEM_LOSS_B38                     = 0xeb80,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B38    =    0xeb81,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B38    =    0xeb82,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B38    =    0xeb83,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B38    =    0xeb84,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B38    =    0xeb85,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B38    =    0xeb86,
        EN_NV_ID_DPD_FAC_LUT_03_B38    =    0xeb87,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B38    =    0xeb88,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B38         = 0xeb8d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B38         = 0xeb8e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B38       = 0xeb8f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B38       = 0xeb90,
        EN_NV_ID_LTE_IP2_CAL_B38                        = 0xeb91,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B38      = 0xeb92,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B38       = 0xeb93,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B38       = 0xeb94,
        //EN_NV_ID_LTE_PA_POWER_B38                       = 0xeb95,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B38        = 0xeb96,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B38         = 0xeb97,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B38         = 0xeb98,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B38                    = 0xeb99,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B38                  = 0xeb9a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B38                  = 0xeb9b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B38                       = 0xeb9c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B38                       = 0xeb9d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B38               = 0xeb9e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B38              = 0xeb9f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B38                = 0xeba5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B38               = 0xeba6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B38              = 0xeba7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B38           = 0xeba8,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B38         = 0xeba9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B38         = 0xebaa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B38       = 0xebab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B38       = 0xebac,*/
        EN_NV_ID_DPD_LAB_PARA_B38                  = 0xebad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B38                  = 0xebae,
        EN_NV_ID_DPD_FAC_RESULT_B38                 = 0xebaf,
        EN_NV_ID_LTE_ET_GAIN_COMP_B38                   = 0xebb0,
        EN_NV_ID_DPD_FAC_LUT_00_B38                    = 0xebb1,
        EN_NV_ID_DPD_FAC_LUT_01_B38                    = 0xebb2,
        EN_NV_ID_DPD_FAC_LUT_02_B38                           = 0xebb3,
        EN_NV_ID_RF_CA_RCCODE_B38                        = 0xebb4,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B41                     = 0xebc0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B41    =    0xebc1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B41    =    0xebc2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B41    =    0xebc3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B41    =    0xebc4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B41    =    0xebc5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B41    =    0xebc6,
        EN_NV_ID_DPD_FAC_LUT_03_B41    =    0xebc7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B41    =    0xebc8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B41         = 0xebcd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B41         = 0xebce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B41       = 0xebcf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B41       = 0xebd0,
        EN_NV_ID_LTE_IP2_CAL_B41                        = 0xebd1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B41      = 0xebd2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B41       = 0xebd3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B41       = 0xebd4,
        //EN_NV_ID_LTE_PA_POWER_B41                       = 0xebd5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B41        = 0xebd6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B41         = 0xebd7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B41         = 0xebd8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B41                    = 0xebd9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B41                  = 0xebda,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B41                  = 0xebdb,
        EN_NV_ID_LTE_APC_TABLE_STRU_B41                       = 0xebdc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B41                       = 0xebdd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B41               = 0xebde,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B41              = 0xebdf,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B41                = 0xebe5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B41               = 0xebe6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B41              = 0xebe7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B41           = 0xebe8,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B41    = 0xebe9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B41    = 0xebea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B41  = 0xebeb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B41  = 0xebec,*/
        EN_NV_ID_DPD_LAB_PARA_B41                  = 0xebed,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B41                  = 0xebee,
        EN_NV_ID_DPD_FAC_RESULT_B41                 = 0xebef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B41                   = 0xebf0,
        EN_NV_ID_DPD_FAC_LUT_00_B41                    = 0xebf1,
        EN_NV_ID_DPD_FAC_LUT_01_B41                    = 0xebf2,
        EN_NV_ID_DPD_FAC_LUT_02_B41                           = 0xebf3,
        EN_NV_ID_RF_CA_RCCODE_B41                        = 0xebf4,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B7                      = 0xec00,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B7    =    0xec01,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B7    =    0xec02,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B7    =    0xec03,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B7    =    0xec04,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B7    =    0xec05,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B7    =    0xec06,
        EN_NV_ID_DPD_FAC_LUT_03_B7    =    0xec07,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B7    =    0xec08,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B7          = 0xec0D,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B7          = 0xec0E,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B7        = 0xec0F,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B7        = 0xec10,
        EN_NV_ID_LTE_IP2_CAL_B7                         = 0xec11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B7       = 0xec12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B7        = 0xec13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B7        = 0xec14,
        //EN_NV_ID_LTE_PA_POWER_B7                        = 0xec15,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B7         = 0xec16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B7          = 0xec17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B7          = 0xec18,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B7                    = 0xec19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B7                  = 0xec1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B7                  = 0xec1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B7                       = 0xec1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B7                       = 0xec1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B7               = 0xec1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B7              = 0xec1f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B7                = 0xec25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B7               = 0xec26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B7              = 0xec27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B7           = 0xec28,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B7    = 0xec29,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B7    = 0xec2a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B7  = 0xec2b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B7  = 0xec2c,*/
        EN_NV_ID_DPD_LAB_PARA_B7                  = 0xec2d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B7                  = 0xec2e,
        EN_NV_ID_DPD_FAC_RESULT_B7                 = 0xec2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B7                   = 0xec30,
        EN_NV_ID_DPD_FAC_LUT_00_B7                    = 0xec31,
        EN_NV_ID_DPD_FAC_LUT_01_B7                    = 0xec32,
        EN_NV_ID_DPD_FAC_LUT_02_B7                           = 0xec33,
        EN_NV_ID_RF_CA_RCCODE_B7                        = 0xec34,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B3                      = 0xed00,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B3    =    0xed01,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B3    =    0xed02,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B3    =    0xed03,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B3    =    0xed04,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B3    =    0xed05,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B3    =    0xed06,
        EN_NV_ID_DPD_FAC_LUT_03_B3    =    0xed07,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B3    =    0xed08,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B3          = 0xed0D,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B3          = 0xed0E,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B3        = 0xed0F,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B3        = 0xed10,
        EN_NV_ID_LTE_IP2_CAL_B3                         = 0xed11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B3       = 0xed12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B3        = 0xed13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B3        = 0xed14,
        //EN_NV_ID_LTE_PA_POWER_B3                        = 0xed15,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B3         = 0xed16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B3          = 0xed17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B3          = 0xed18,
         /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B3                    = 0xed19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B3                  = 0xed1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B3                  = 0xed1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B3                       = 0xed1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B3                       = 0xed1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B3               = 0xed1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B3              = 0xed1f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B3                = 0xed25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B3               = 0xed26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B3              = 0xed27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B3           = 0xed28,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B3    = 0xed29,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B3    = 0xed2a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B3  = 0xed2b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B3  = 0xed2c,*/
        EN_NV_ID_DPD_LAB_PARA_B3                  = 0xed2d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B3                  = 0xed2e,
        EN_NV_ID_DPD_FAC_RESULT_B3                 = 0xed2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B3                    = 0xed30,
        EN_NV_ID_DPD_FAC_LUT_00_B3                     = 0xed31,
        EN_NV_ID_DPD_FAC_LUT_01_B3                     = 0xed32,
        EN_NV_ID_DPD_FAC_LUT_02_B3                           = 0xed33,
        EN_NV_ID_RF_CA_RCCODE_B3                        = 0xed34,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B1                      = 0xed40,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B1    =    0xed41,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B1    =    0xed42,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B1    =    0xed43,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B1    =    0xed44,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B1    =    0xed45,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B1    =    0xed46,
        EN_NV_ID_DPD_FAC_LUT_03_B1    =    0xed47,
        ////EN_NV_ID_LTE_DPD_LAB_STRU_B1    =    0xed48,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B1          = 0xed4D,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B1          = 0xed4E,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B1        = 0xed4F,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B1        = 0xed50,
        EN_NV_ID_LTE_IP2_CAL_B1                         = 0xed51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B1       = 0xed52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B1        = 0xed53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B1        = 0xed54,
        //EN_NV_ID_LTE_PA_POWER_B1                        = 0xed55,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B1         = 0xed56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B1          = 0xed57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B1          = 0xed58,
         /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B1                    = 0xed59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B1                  = 0xed5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B1                  = 0xed5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B1                       = 0xed5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B1                       = 0xed5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B1               = 0xed5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B1              = 0xed5f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B1                = 0xed65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B1               = 0xed66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B1              = 0xed67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B1           = 0xed68,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B1    = 0xed69,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B1    = 0xed6a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B1  = 0xed6b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B1  = 0xed6c,*/
        EN_NV_ID_DPD_LAB_PARA_B1                      = 0xed6d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B1                 = 0xed6e,
        EN_NV_ID_DPD_FAC_RESULT_B1                     = 0xed6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B1                   = 0xed70,
        EN_NV_ID_DPD_FAC_LUT_00_B1                     = 0xed71,
        EN_NV_ID_DPD_FAC_LUT_01_B1                    = 0xed72,
        EN_NV_ID_DPD_FAC_LUT_02_B1                           = 0xed73,
        EN_NV_ID_RF_CA_RCCODE_B1                        = 0xed74,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B5                      = 0xed80,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B5    =    0xed81,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B5    =    0xed82,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B5    =    0xed83,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B5    =    0xed84,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B5    =    0xed85,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B5    =    0xed86,
        EN_NV_ID_DPD_FAC_LUT_03_B5    =    0xed87,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B5    =    0xed88,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B5          = 0xed8D,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B5          = 0xed8E,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B5        = 0xed8F,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B5        = 0xed90,
        EN_NV_ID_LTE_IP2_CAL_B5                         = 0xed91,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B5       = 0xed92,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B5        = 0xed93,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B5        = 0xed94,
        //EN_NV_ID_LTE_PA_POWER_B5                        = 0xed95,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B5         = 0xed96,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B5          = 0xed97,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B5          = 0xed98,
         /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B5                    = 0xed99,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B5                  = 0xed9a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B5                  = 0xed9b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B5                       = 0xed9c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B5                       = 0xed9d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B5               = 0xed9e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B5              = 0xed9f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B5                = 0xedA5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B5               = 0xedA6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B5              = 0xedA7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B5           = 0xedA8,


        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B5      = 0xedA9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B5    = 0xedAa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B5  = 0xedAb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B5  = 0xedAc,*/
        EN_NV_ID_DPD_LAB_PARA_B5                  = 0xedad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B5                  = 0xedae,
        EN_NV_ID_DPD_FAC_RESULT_B5                 = 0xedaf,
        EN_NV_ID_LTE_ET_GAIN_COMP_B5                   = 0xedb0,
        EN_NV_ID_DPD_FAC_LUT_00_B5                    = 0xedb1,
        EN_NV_ID_DPD_FAC_LUT_01_B5                    = 0xedb2,
        EN_NV_ID_DPD_FAC_LUT_02_B5                           = 0xedb3,
        EN_NV_ID_RF_CA_RCCODE_B5                        = 0xedb4,

        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B8                      = 0xedc0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B8    =    0xedc1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B8    =    0xedc2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B8    =    0xedc3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B8    =    0xedc4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B8    =    0xedc5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B8    =    0xedc6,
        EN_NV_ID_DPD_FAC_LUT_03_B8    =    0xedc7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B8    =    0xedc8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B8          = 0xedcD,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B8          = 0xedcE,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B8        = 0xedcF,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B8        = 0xedd0,
        EN_NV_ID_LTE_IP2_CAL_B8                         = 0xedd1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B8       = 0xedd2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B8        = 0xedd3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B8        = 0xedd4,
        //EN_NV_ID_LTE_PA_POWER_B8                        = 0xedd5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B8         = 0xedd6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B8          = 0xedd7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B8          = 0xedd8,
         /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B8                    = 0xedd9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B8                  = 0xedda,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B8                  = 0xeddb,
        EN_NV_ID_LTE_APC_TABLE_STRU_B8                       = 0xeddc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B8                       = 0xeddd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B8               = 0xedde,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B8              = 0xeddf,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B8                = 0xede5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B8               = 0xede6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B8              = 0xede7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B8           = 0xede8,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B8    = 0xede9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B8    = 0xedea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B8  = 0xedeb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B8  = 0xedec,*/
        EN_NV_ID_DPD_LAB_PARA_B8                  = 0xeded,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B8                  = 0xedee,
        EN_NV_ID_DPD_FAC_RESULT_B8                 = 0xedef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B8                   = 0xedf0,
        EN_NV_ID_DPD_FAC_LUT_00_B8                    = 0xedf1,
        EN_NV_ID_DPD_FAC_LUT_01_B8                    = 0xedf2,
        EN_NV_ID_DPD_FAC_LUT_02_B8                           = 0xedf3,
        EN_NV_ID_RF_CA_RCCODE_B8                        = 0xedf4,
        /*add for V9R1_6361 End*/

        /* modify by   for 所有band begin*/
        EN_NV_ID_ANT_MODEM_LOSS_B19                     = 0xEE00,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B19    =    0xEE01,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B19    =    0xEE02,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B19    =    0xEE03,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B19    =    0xEE04,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B19    =    0xEE05,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B19    =    0xEE06,
        EN_NV_ID_DPD_FAC_LUT_03_B19    =    0xEE07,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B19    =    0xEE08,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B19         = 0xEE0d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B19         = 0xEE0e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B19       = 0xEE0f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B19       = 0xEE10,
        EN_NV_ID_LTE_IP2_CAL_B19                        = 0xEE11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B19      = 0xEE12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B19       = 0xEE13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B19       = 0xEE14,
        //EN_NV_ID_LTE_PA_POWER_B19                       = 0xEE15,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B19        = 0xEE16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B19         = 0xEE17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B19         = 0xEE18,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B19                     = 0xEE19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B19                   = 0xEE1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B19                   = 0xEE1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B19                        = 0xEE1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B19                        = 0xEE1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B19                = 0xEE1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B19               = 0xEE1f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B19                  = 0xee25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B19                 = 0xee26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B19                = 0xee27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B19             = 0xee28,
        /*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B19      = 0xee29,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B19      = 0xee2a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B19    = 0xee2b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B19    = 0xee2c,*/
        EN_NV_ID_DPD_LAB_PARA_B19                  = 0xee2d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B19                  = 0xee2e,
        EN_NV_ID_DPD_FAC_RESULT_B19                 = 0xee2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B19                   = 0xee30,
        EN_NV_ID_DPD_FAC_LUT_00_B19                    = 0xee31,
        EN_NV_ID_DPD_FAC_LUT_01_B19                    = 0xee32,
        EN_NV_ID_DPD_FAC_LUT_02_B19                           = 0xee33,
        EN_NV_ID_RF_CA_RCCODE_B19                        = 0xee34,

        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B21                     = 0xEE40,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B21    =    0xEE41,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B21    =    0xEE42,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B21    =    0xEE43,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B21    =    0xEE44,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B21    =    0xEE45,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B21    =    0xEE46,
        EN_NV_ID_DPD_FAC_LUT_03_B21    =    0xEE47,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B21    =    0xEE48,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B21         = 0xEE4d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B21         = 0xEE4e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B21       = 0xEE4f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B21       = 0xEE50,
        EN_NV_ID_LTE_IP2_CAL_B21                        = 0xEE51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B21      = 0xEE52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B21       = 0xEE53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B21       = 0xEE54,
        //EN_NV_ID_LTE_PA_POWER_B21                       = 0xEE55,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B21        = 0xEE56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B21         = 0xEE57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B21         = 0xEE58,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B21                     = 0xEE59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B21                   = 0xEE5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B21                   = 0xEE5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B21                        = 0xEE5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B21                        = 0xEE5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B21                = 0xEE5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B21               = 0xEE5f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B21                  = 0xee65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B21                 = 0xee66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B21                = 0xee67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B21             = 0xee68,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B21      = 0xee69,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B21      = 0xee6a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B21    = 0xee6b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B21    = 0xee6c,
        EN_NV_ID_DPD_LAB_PARA_B21                  = 0xee6d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B21                  = 0xee6e,
        EN_NV_ID_DPD_FAC_RESULT_B21                 = 0xee6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B21                   = 0xee70,
        EN_NV_ID_DPD_FAC_LUT_00_B21                    = 0xee71,
        EN_NV_ID_DPD_FAC_LUT_01_B21                    = 0xee72,
        EN_NV_ID_DPD_FAC_LUT_02_B21                           = 0xee73,
        EN_NV_ID_RF_CA_RCCODE_B21                        = 0xee74,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B2                      = 0xEE80,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B2    =    0xEE81,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B2    =    0xEE82,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B2    =    0xEE83,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B2    =    0xEE84,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B2    =    0xEE85,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B2    =    0xEE86,
        EN_NV_ID_DPD_FAC_LUT_03_B2    =    0xEE87,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B2    =    0xEE88,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B2          = 0xEE8d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B2          = 0xEE8e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B2        = 0xEE8f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B2        = 0xEE90,
        EN_NV_ID_LTE_IP2_CAL_B2                         = 0xEE91,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B2       = 0xEE92,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B2        = 0xEE93,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B2        = 0xEE94,
        //EN_NV_ID_LTE_PA_POWER_B2                        = 0xEE95,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B2         = 0xEE96,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B2          = 0xEE97,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B2          = 0xEE98,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B2                     = 0xEE99,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B2                   = 0xEE9a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B2                   = 0xEE9b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B2                        = 0xEE9c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B2                        = 0xEE9d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B2                = 0xEE9e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B2               = 0xEE9f,
        /*add for V9R1_6361 End*/



        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B2                = 0xEEa5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B2               = 0xEEa6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B2              = 0xEEa7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B2           = 0xEEa8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B2    = 0xEEa9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B2      = 0xEEaa,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B2  = 0xEEab,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B2  = 0xEEac,
        EN_NV_ID_DPD_LAB_PARA_B2                  = 0xEEad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B2                  = 0xEEae,
        EN_NV_ID_DPD_FAC_RESULT_B2                 = 0xEEaf,
        EN_NV_ID_LTE_ET_GAIN_COMP_B2                   = 0xEEb0,
        EN_NV_ID_DPD_FAC_LUT_00_B2                    = 0xEEb1,
        EN_NV_ID_DPD_FAC_LUT_01_B2                    = 0xEEb2,
        EN_NV_ID_DPD_FAC_LUT_02_B2                           = 0xEEb3,
        EN_NV_ID_RF_CA_RCCODE_B2                        = 0xEEb4,

        EN_NV_ID_ANT_MODEM_LOSS_B4                      = 0xEEc0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B4    =    0xEEc1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B4    =    0xEEc2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B4    =    0xEEc3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B4    =    0xEEc4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B4    =    0xEEc5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B4    =    0xEEc6,
        EN_NV_ID_DPD_FAC_LUT_03_B4    =    0xEEc7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B4    =    0xEEc8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B4          = 0xEEcd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B4          = 0xEEce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B4        = 0xEEcf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B4        = 0xEEd0,
        EN_NV_ID_LTE_IP2_CAL_B4                         = 0xEEd1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B4       = 0xEEd2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B4        = 0xEEd3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B4        = 0xEEd4,
        //EN_NV_ID_LTE_PA_POWER_B4                        = 0xEEd5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B4         = 0xEEd6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B4          = 0xEEd7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B4          = 0xEEd8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B4                     = 0xEEd9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B4                   = 0xEEda,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B4                   = 0xEEdb,
        EN_NV_ID_LTE_APC_TABLE_STRU_B4                        = 0xEEdc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B4                        = 0xEEdd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B4                = 0xEEde,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B4               = 0xEEdf,
        /*add for V9R1_6361 End*/

        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B4                = 0xEEe5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B4               = 0xEEe6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B4              = 0xEEe7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B4           = 0xEEe8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B4    = 0xEEe9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B4      = 0xEEea,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B4  = 0xEEeb,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B4  = 0xEEec,
        EN_NV_ID_DPD_LAB_PARA_B4                  = 0xEEed,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B4                  = 0xEEee,
        EN_NV_ID_DPD_FAC_RESULT_B4                 = 0xEEef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B4                   = 0xEEf0,
        EN_NV_ID_DPD_FAC_LUT_00_B4                    = 0xEEf1,
        EN_NV_ID_DPD_FAC_LUT_01_B4                    = 0xEEf2,
        EN_NV_ID_DPD_FAC_LUT_02_B4                           = 0xEEf3,
        EN_NV_ID_RF_CA_RCCODE_B4                        = 0xEEf4,

        EN_NV_ID_ANT_MODEM_LOSS_B6                      = 0xEf00,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B6    =    0xEf01,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B6    =    0xEf02,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B6    =    0xEf03,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B6    =    0xEf04,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B6    =    0xEf05,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B6    =    0xEf06,
        EN_NV_ID_DPD_FAC_LUT_03_B6    =    0xEf07,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B6    =    0xEf08,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B6          = 0xEf0d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B6          = 0xEf0e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B6        = 0xEf0f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B6        = 0xEf10,
        EN_NV_ID_LTE_IP2_CAL_B6                         = 0xEf11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B6       = 0xEf12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B6        = 0xEf13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B6        = 0xEf14,
        //EN_NV_ID_LTE_PA_POWER_B6                        = 0xEf15,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B6         = 0xEf16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B6          = 0xEf17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B6          = 0xEf18,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B6                     = 0xEf19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B6                   = 0xEf1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B6                   = 0xEf1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B6                        = 0xEf1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B6                        = 0xEf1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B6                = 0xEf1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B6               = 0xEf1f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B6                = 0xEf25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B6               = 0xEf26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B6              = 0xEf27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B6           = 0xEf28,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B6    = 0xEf29,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B6      = 0xEf2a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B6  = 0xEf2b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B6  = 0xEf2c,
        EN_NV_ID_DPD_LAB_PARA_B6                  = 0xEf2d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B6                  = 0xEf2e,
        EN_NV_ID_DPD_FAC_RESULT_B6                 = 0xEf2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B6                   = 0xEf30,
        EN_NV_ID_DPD_FAC_LUT_00_B6                    = 0xEf31,
        EN_NV_ID_DPD_FAC_LUT_01_B6                    = 0xEf32,
        EN_NV_ID_DPD_FAC_LUT_02_B6                           = 0xEf33,
        EN_NV_ID_RF_CA_RCCODE_B6                        = 0xEf34,

        EN_NV_ID_ANT_MODEM_LOSS_B9                      = 0xEf40,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B9    =    0xEf41,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B9    =    0xEf42,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B9    =    0xEf43,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B9    =    0xEf44,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B9    =    0xEf45,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B9    =    0xEf46,
        EN_NV_ID_DPD_FAC_LUT_03_B9    =    0xEf47,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B9    =    0xEf48,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B9          = 0xEf4d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B9          = 0xEf4e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B9        = 0xEf4f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B9        = 0xEf50,
        EN_NV_ID_LTE_IP2_CAL_B9                         = 0xEf51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B9       = 0xEf52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B9        = 0xEf53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B9        = 0xEf54,
        //EN_NV_ID_LTE_PA_POWER_B9                        = 0xEf55,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B9         = 0xEf56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B9          = 0xEf57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B9          = 0xEf58,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B9                     = 0xEf59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B9                   = 0xEf5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B9                   = 0xEf5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B9                        = 0xEf5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B9                        = 0xEf5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B9                = 0xEf5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B9               = 0xEf5f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B9                = 0xEf65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B9               = 0xEf66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B9              = 0xEf67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B9           = 0xEf68,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B9    = 0xEf69,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B9      = 0xEf6a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B9  = 0xEf6b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B9  = 0xEf6c,
        EN_NV_ID_DPD_LAB_PARA_B9                  = 0xEf6d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B9                  = 0xEf6e,
        EN_NV_ID_DPD_FAC_RESULT_B9                 = 0xEf6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B9                   = 0xEf70,
        EN_NV_ID_DPD_FAC_LUT_00_B9                    = 0xEf71,
        EN_NV_ID_DPD_FAC_LUT_01_B9                    = 0xEf72,
        EN_NV_ID_DPD_FAC_LUT_02_B9                           = 0xEf73,
        EN_NV_ID_RF_CA_RCCODE_B9                        = 0xEf74,

        EN_NV_ID_ANT_MODEM_LOSS_B10                     = 0xEf80,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B10    =    0xEf81,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B10    =    0xEf82,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B10    =    0xEf83,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B10    =    0xEf84,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B10    =    0xEf85,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B10    =    0xEf86,
        EN_NV_ID_DPD_FAC_LUT_03_B10    =    0xEf87,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B10    =    0xEf88,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B10         = 0xEf8d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B10         = 0xEf8e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B10       = 0xEf8f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B10       = 0xEf90,
        EN_NV_ID_LTE_IP2_CAL_B10                        = 0xEf91,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B10      = 0xEf92,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B10       = 0xEf93,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B10       = 0xEf94,
        //EN_NV_ID_LTE_PA_POWER_B10                       = 0xEf95,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B10        = 0xEf96,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B10         = 0xEf97,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B10         = 0xEf98,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B10                    = 0xEf99,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B10                  = 0xEf9a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B10                  = 0xEf9b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B10                       = 0xEf9c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B10                       = 0xEf9d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B10               = 0xEf9e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B10              = 0xEf9f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B10                = 0xEfa5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B10               = 0xEfa6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B10              = 0xEfa7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B10           = 0xEfa8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B10   = 0xEfa9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B10     = 0xEfaa,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B10  = 0xEfab,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B10  = 0xEfac,
        EN_NV_ID_DPD_LAB_PARA_B10                  = 0xEfad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B10                  = 0xEfae,
        EN_NV_ID_DPD_FAC_RESULT_B10                 = 0xEfaf,
        EN_NV_ID_LTE_ET_GAIN_COMP_B10                   = 0xEfb0,
        EN_NV_ID_DPD_FAC_LUT_00_B10                    = 0xEfb1,
        EN_NV_ID_DPD_FAC_LUT_01_B10                    = 0xEfb2,
        EN_NV_ID_DPD_FAC_LUT_02_B10                           = 0xEfb3,
        EN_NV_ID_RF_CA_RCCODE_B10                        = 0xEfb4,





        EN_NV_ID_ANT_MODEM_LOSS_B11                     = 0xEfc0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B11    =    0xEfc1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B11    =    0xEfc2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B11    =    0xEfc3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B11    =    0xEfc4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B11    =    0xEfc5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B11    =    0xEfc6,
        EN_NV_ID_DPD_FAC_LUT_03_B11    =    0xEfc7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B11    =    0xEfc8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B11         = 0xEfcd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B11         = 0xEfce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B11       = 0xEfcf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B11       = 0xEfd0,
        EN_NV_ID_LTE_IP2_CAL_B11                        = 0xEfd1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B11      = 0xEfd2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B11       = 0xEfd3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B11       = 0xEfd4,
        //EN_NV_ID_LTE_PA_POWER_B11                       = 0xEfd5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B11        = 0xEfd6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B11         = 0xEfd7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B11         = 0xEfd8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B11                    = 0xEfd9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B11                  = 0xEfda,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B11                  = 0xEfdb,
        EN_NV_ID_LTE_APC_TABLE_STRU_B11                       = 0xEfdc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B11                       = 0xEfdd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B11               = 0xEfde,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B11              = 0xEfdf,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B11                = 0xEfe5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B11               = 0xEfe6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B11              = 0xEfe7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B11           = 0xEfe8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B11   = 0xEfe9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B11     = 0xEfea,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B11  = 0xEfeb,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B11  = 0xEfec,
        EN_NV_ID_DPD_LAB_PARA_B11                  = 0xEfed,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B11                  = 0xEfee,
        EN_NV_ID_DPD_FAC_RESULT_B11                 = 0xEfef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B11                   = 0xEff0,
        EN_NV_ID_DPD_FAC_LUT_00_B11                    = 0xEff1,
        EN_NV_ID_DPD_FAC_LUT_01_B11                    = 0xEff2,
        EN_NV_ID_DPD_FAC_LUT_02_B11                           = 0xEff3,
        EN_NV_ID_RF_CA_RCCODE_B11                        = 0xEff4,

        EN_NV_ID_ANT_MODEM_LOSS_B12                     = 0xf000,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B12    =    0xf001,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B12    =    0xf002,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B12    =    0xf003,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B12    =    0xf004,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B12    =    0xf005,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B12    =    0xf006,
        EN_NV_ID_DPD_FAC_LUT_03_B12    =    0xf007,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B12    =    0xf008,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B12         = 0xf00d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B12         = 0xf00e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B12       = 0xf00f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B12       = 0xf010,
        EN_NV_ID_LTE_IP2_CAL_B12                        = 0xf011,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B12      = 0xf012,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B12       = 0xf013,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B12       = 0xf014,
        //EN_NV_ID_LTE_PA_POWER_B12                       = 0xf015,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B12        = 0xf016,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B12         = 0xf017,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B12         = 0xf018,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B12                    = 0xf019,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B12                  = 0xf01a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B12                  = 0xf01b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B12                       = 0xf01c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B12                       = 0xf01d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B12               = 0xf01e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B12              = 0xf01f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B12                = 0xf025,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B12               = 0xf026,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B12              = 0xf027,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B12           = 0xf028,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B12   = 0xf029,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B12     = 0xf02a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B12  = 0xf02b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B12  = 0xf02c,
        EN_NV_ID_DPD_LAB_PARA_B12                  = 0xf02d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B12                  = 0xf02e,
        EN_NV_ID_DPD_FAC_RESULT_B12                 = 0xf02f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B12                   = 0xf030,
        EN_NV_ID_DPD_FAC_LUT_00_B12                    = 0xf031,
        EN_NV_ID_DPD_FAC_LUT_01_B12                    = 0xf032,
        EN_NV_ID_DPD_FAC_LUT_02_B12                           = 0xf033,
        EN_NV_ID_RF_CA_RCCODE_B12                        = 0xf034,

        EN_NV_ID_ANT_MODEM_LOSS_B13                     = 0xf040,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B13    =    0xf041,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B13    =    0xf042,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B13    =    0xf043,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B13    =    0xf044,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B13    =    0xf045,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B13    =    0xf046,
        EN_NV_ID_DPD_FAC_LUT_03_B13    =    0xf047,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B13    =    0xf048,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B13         = 0xf04d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B13         = 0xf04e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B13       = 0xf04f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B13       = 0xf050,
        EN_NV_ID_LTE_IP2_CAL_B13                        = 0xf051,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B13      = 0xf052,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B13       = 0xf053,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B13       = 0xf054,
        //EN_NV_ID_LTE_PA_POWER_B13                       = 0xf055,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B13        = 0xf056,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B13         = 0xf057,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B13         = 0xf058,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B13                    = 0xf059,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B13                  = 0xf05a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B13                  = 0xf05b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B13                       = 0xf05c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B13                       = 0xf05d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B13               = 0xf05e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B13              = 0xf05f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B13                = 0xf065,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B13               = 0xf066,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B13              = 0xf067,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B13           = 0xf068,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B13   = 0xf069,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B13   = 0xf06a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B13  = 0xf06b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B13  = 0xf06c,
        EN_NV_ID_DPD_LAB_PARA_B13                  = 0xf06d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B13                  = 0xf06e,
        EN_NV_ID_DPD_FAC_RESULT_B13                 = 0xf06f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B13                   = 0xf070,
        EN_NV_ID_DPD_FAC_LUT_00_B13                    = 0xf071,
        EN_NV_ID_DPD_FAC_LUT_01_B13                    = 0xf072,
        EN_NV_ID_DPD_FAC_LUT_02_B13                           = 0xf073,
        EN_NV_ID_RF_CA_RCCODE_B13                        = 0xf074,


        EN_NV_ID_ANT_MODEM_LOSS_B14                     = 0xf080,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B14    =    0xf081,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B14    =    0xf082,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B14    =    0xf083,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B14    =    0xf084,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B14    =    0xf085,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B14    =    0xf086,
        EN_NV_ID_DPD_FAC_LUT_03_B14    =    0xf087,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B14    =    0xf088,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B14         = 0xf08d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B14         = 0xf08e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B14       = 0xf08f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B14       = 0xf090,
        EN_NV_ID_LTE_IP2_CAL_B14                        = 0xf091,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B14      = 0xf092,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B14       = 0xf093,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B14       = 0xf094,
        //EN_NV_ID_LTE_PA_POWER_B14                       = 0xf095,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B14        = 0xf096,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B14         = 0xf097,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B14         = 0xf098,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B14                    = 0xf099,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B14                  = 0xf09a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B14                  = 0xf09b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B14                       = 0xf09c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B14                       = 0xf09d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B14               = 0xf09e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B14              = 0xf09f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B14                = 0xf0a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B14               = 0xf0a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B14              = 0xf0a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B14           = 0xf0a8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B14   = 0xf0a9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B14   = 0xf0aa,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B14  = 0xf0ab,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B14  = 0xf0ac,
        EN_NV_ID_DPD_LAB_PARA_B14                  = 0xf0ad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B14                  = 0xf0ae,
        EN_NV_ID_DPD_FAC_RESULT_B14                 = 0xf0af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B14                   = 0xf0b0,
        EN_NV_ID_DPD_FAC_LUT_00_B14                    = 0xf0b1,
        EN_NV_ID_DPD_FAC_LUT_01_B14                    = 0xf0b2,
        EN_NV_ID_DPD_FAC_LUT_02_B14                           = 0xf0b3,
        EN_NV_ID_RF_CA_RCCODE_B14                        = 0xf0b4,






        EN_NV_ID_ANT_MODEM_LOSS_B17                     = 0xf0c0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B17    =    0xf0c1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B17    =    0xf0c2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B17    =    0xf0c3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B17    =    0xf0c4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B17    =    0xf0c5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B17    =    0xf0c6,
        EN_NV_ID_DPD_FAC_LUT_03_B17    =    0xf0c7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B17    =    0xf0c8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B17         = 0xf0cd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B17         = 0xf0ce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B17       = 0xf0cf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B17       = 0xf0d0,
        EN_NV_ID_LTE_IP2_CAL_B17                        = 0xf0d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B17      = 0xf0d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B17       = 0xf0d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B17       = 0xf0d4,
        //EN_NV_ID_LTE_PA_POWER_B17                       = 0xf0d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B17        = 0xf0d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B17         = 0xf0d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B17         = 0xf0d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B17                    = 0xf0d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B17                  = 0xf0da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B17                  = 0xf0db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B17                       = 0xf0dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B17                       = 0xf0dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B17               = 0xf0de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B17              = 0xf0df,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B17                = 0xf0e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B17               = 0xf0e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B17              = 0xf0e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B17           = 0xf0e8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B17   = 0xf0e9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B17   = 0xf0ea,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B17  = 0xf0eb,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B17  = 0xf0ec,
        EN_NV_ID_DPD_LAB_PARA_B17                  = 0xf0ed,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B17                  = 0xf0ee,
        EN_NV_ID_DPD_FAC_RESULT_B17                 = 0xf0ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B17                   = 0xf0f0,
        EN_NV_ID_DPD_FAC_LUT_00_B17                    = 0xf0f1,
        EN_NV_ID_DPD_FAC_LUT_01_B17                    = 0xf0f2,
        EN_NV_ID_DPD_FAC_LUT_02_B17                           = 0xf0f3,
        EN_NV_ID_RF_CA_RCCODE_B17                        = 0xf0f4,





        EN_NV_ID_ANT_MODEM_LOSS_B18                     = 0xf100,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B18    =    0xf101,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B18    =    0xf102,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B18    =    0xf103,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B18    =    0xf104,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B18    =    0xf105,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B18    =    0xf106,
        EN_NV_ID_DPD_FAC_LUT_03_B18    =    0xf107,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B18    =    0xf108,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B18         = 0xf10d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B18         = 0xf10e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B18       = 0xf10f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B18       = 0xf110,
        EN_NV_ID_LTE_IP2_CAL_B18                        = 0xf111,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B18      = 0xf112,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B18       = 0xf113,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B18       = 0xf114,
        //EN_NV_ID_LTE_PA_POWER_B18                       = 0xf115,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B18        = 0xf116,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B18         = 0xf117,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B18         = 0xf118,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B18                    = 0xf119,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B18                  = 0xf11a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B18                  = 0xf11b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B18                       = 0xf11c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B18                       = 0xf11d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B18               = 0xf11e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B18              = 0xf11f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B18                = 0xf125,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B18               = 0xf126,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B18              = 0xf127,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B18           = 0xf128,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B18   = 0xf129,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B18   = 0xf12a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B18  = 0xf12b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B18  = 0xf12c,
        EN_NV_ID_DPD_LAB_PARA_B18                  = 0xf12d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B18                  = 0xf12e,
        EN_NV_ID_DPD_FAC_RESULT_B18                 = 0xf12f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B18                   = 0xf130,
        EN_NV_ID_DPD_FAC_LUT_00_B18                    = 0xf131,
        EN_NV_ID_DPD_FAC_LUT_01_B18                    = 0xf132,
        EN_NV_ID_DPD_FAC_LUT_02_B18                           = 0xf133,
        EN_NV_ID_RF_CA_RCCODE_B18                        = 0xf134,





        EN_NV_ID_ANT_MODEM_LOSS_B22                     = 0xf140,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B22    =    0xf141,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B22    =    0xf142,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B22    =    0xf143,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B22    =    0xf144,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B22    =    0xf145,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B22    =    0xf146,
        EN_NV_ID_DPD_FAC_LUT_03_B22    =    0xf147,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B22    =    0xf148,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B22         = 0xf14d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B22         = 0xf14e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B22       = 0xf14f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B22       = 0xf150,
        EN_NV_ID_LTE_IP2_CAL_B22                        = 0xf151,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B22      = 0xf152,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B22       = 0xf153,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B22       = 0xf154,
        //EN_NV_ID_LTE_PA_POWER_B22                       = 0xf155,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B22        = 0xf156,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B22         = 0xf157,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B22         = 0xf158,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B22                    = 0xf159,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B22                  = 0xf15a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B22                  = 0xf15b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B22                       = 0xf15c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B22                       = 0xf15d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B22               = 0xf15e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B22              = 0xf15f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B22                = 0xf165,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B22               = 0xf166,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B22              = 0xf167,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B22           = 0xf168,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B22   = 0xf169,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B22   = 0xf16a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B22  = 0xf16b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B22  = 0xf16c,
        EN_NV_ID_DPD_LAB_PARA_B22                  = 0xf16d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B22                  = 0xf16e,
        EN_NV_ID_DPD_FAC_RESULT_B22                 = 0xf16f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B22                   = 0xf170,
        EN_NV_ID_DPD_FAC_LUT_00_B22                    = 0xf171,
        EN_NV_ID_DPD_FAC_LUT_01_B22                    = 0xf172,
        EN_NV_ID_DPD_FAC_LUT_02_B22                           = 0xf173,
        EN_NV_ID_RF_CA_RCCODE_B22                        = 0xf174,





        EN_NV_ID_ANT_MODEM_LOSS_B23                     = 0xf180,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B23    =    0xf181,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B23    =    0xf182,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B23    =    0xf183,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B23    =    0xf184,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B23    =    0xf185,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B23    =    0xf186,
        EN_NV_ID_DPD_FAC_LUT_03_B23    =    0xf187,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B23    =    0xf188,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B23         = 0xf18d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B23         = 0xf18e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B23       = 0xf18f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B23       = 0xf190,
        EN_NV_ID_LTE_IP2_CAL_B23                        = 0xf191,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B23      = 0xf192,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B23       = 0xf193,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B23       = 0xf194,
        //EN_NV_ID_LTE_PA_POWER_B23                       = 0xf195,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B23        = 0xf196,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B23         = 0xf197,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B23         = 0xf198,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B23                    = 0xf199,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B23                  = 0xf19a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B23                  = 0xf19b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B23                       = 0xf19c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B23                       = 0xf19d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B23               = 0xf19e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B23              = 0xf19f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B23                = 0xf1a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B23               = 0xf1a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B23              = 0xf1a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B23           = 0xf1a8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B23   = 0xf1a9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B23   = 0xf1aa,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B23  = 0xf1ab,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B23  = 0xf1ac,
        EN_NV_ID_DPD_LAB_PARA_B23                  = 0xf1ad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B23                  = 0xf1ae,
        EN_NV_ID_DPD_FAC_RESULT_B23                 = 0xf1af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B23                   = 0xf1b0,
        EN_NV_ID_DPD_FAC_LUT_00_B23                    = 0xf1b1,
        EN_NV_ID_DPD_FAC_LUT_01_B23                    = 0xf1b2,
        EN_NV_ID_DPD_FAC_LUT_02_B23                           = 0xf1b3,
        EN_NV_ID_RF_CA_RCCODE_B23                        = 0xf1b4,

        EN_NV_ID_ANT_MODEM_LOSS_B24                     = 0xf1c0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B24    =    0xf1c1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B24    =    0xf1c2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B24    =    0xf1c3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B24    =    0xf1c4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B24    =    0xf1c5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B24    =    0xf1c6,
        EN_NV_ID_DPD_FAC_LUT_03_B24    =    0xf1c7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B24    =    0xf1c8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B24         = 0xf1cd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B24         = 0xf1ce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B24       = 0xf1cf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B24       = 0xf1d0,
        EN_NV_ID_LTE_IP2_CAL_B24                        = 0xf1d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B24      = 0xf1d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B24       = 0xf1d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B24       = 0xf1d4,
        //EN_NV_ID_LTE_PA_POWER_B24                       = 0xf1d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B24        = 0xf1d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B24         = 0xf1d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B24         = 0xf1d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B24                    = 0xf1d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B24                  = 0xf1da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B24                  = 0xf1db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B24                       = 0xf1dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B24                       = 0xf1dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B24               = 0xf1de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B24              = 0xf1df,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B24                = 0xf1e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B24               = 0xf1e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B24              = 0xf1e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B24           = 0xf1e8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B24   = 0xf1e9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B24   = 0xf1ea,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B24  = 0xf1eb,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B24  = 0xf1ec,
        EN_NV_ID_DPD_LAB_PARA_B24                  = 0xf1ed,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B24                  = 0xf1ee,
        EN_NV_ID_DPD_FAC_RESULT_B24                 = 0xf1ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B24                   = 0xf1f0,
        EN_NV_ID_DPD_FAC_LUT_00_B24                    = 0xf1f1,
        EN_NV_ID_DPD_FAC_LUT_01_B24                    = 0xf1f2,
        EN_NV_ID_DPD_FAC_LUT_02_B24                           = 0xf1f3,
        EN_NV_ID_RF_CA_RCCODE_B24                        = 0xf1f4,

        EN_NV_ID_ANT_MODEM_LOSS_B25                     = 0xf200,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B25    =    0xf201,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B25    =    0xf202,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B25    =    0xf203,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B25    =    0xf204,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B25    =    0xf205,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B25    =    0xf206,
        EN_NV_ID_DPD_FAC_LUT_03_B25    =    0xf207,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B25    =    0xf208,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B25         = 0xf20d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B25         = 0xf20e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B25       = 0xf20f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B25       = 0xf210,
        EN_NV_ID_LTE_IP2_CAL_B25                        = 0xf211,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B25      = 0xf212,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B25       = 0xf213,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B25       = 0xf214,
        //EN_NV_ID_LTE_PA_POWER_B25                       = 0xf215,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B25        = 0xf216,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B25         = 0xf217,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B25         = 0xf218,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B25                    = 0xf219,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B25                  = 0xf21a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B25                  = 0xf21b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B25                       = 0xf21c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B25                       = 0xf21d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B25               = 0xf21e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B25              = 0xf21f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B25                = 0xf225,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B25               = 0xf226,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B25              = 0xf227,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B25           = 0xf228,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B25   = 0xf229,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B25   = 0xf22a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B25  = 0xf22b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B25  = 0xf22c,
        EN_NV_ID_DPD_LAB_PARA_B25                  = 0xf22d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B25                  = 0xf22e,
        EN_NV_ID_DPD_FAC_RESULT_B25                 = 0xf22f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B25                   = 0xf230,
        EN_NV_ID_DPD_FAC_LUT_00_B25                    = 0xf231,
        EN_NV_ID_DPD_FAC_LUT_01_B25                    = 0xf232,
        EN_NV_ID_DPD_FAC_LUT_02_B25                           = 0xf233,
        EN_NV_ID_RF_CA_RCCODE_B25                        = 0xf234,

        EN_NV_ID_ANT_MODEM_LOSS_B33                     = 0xf240,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B33    =    0xf241,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B33    =    0xf242,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B33    =    0xf243,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B33    =    0xf244,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B33    =    0xf245,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B33    =    0xf246,
        EN_NV_ID_DPD_FAC_LUT_03_B33    =    0xf247,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B33    =    0xf248,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B33         = 0xf24d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B33         = 0xf24e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B33       = 0xf24f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B33       = 0xf250,
        EN_NV_ID_LTE_IP2_CAL_B33                        = 0xf251,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B33      = 0xf252,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B33       = 0xf253,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B33       = 0xf254,
        //EN_NV_ID_LTE_PA_POWER_B33                       = 0xf255,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B33        = 0xf256,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B33         = 0xf257,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B33         = 0xf258,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B33                    = 0xf259,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B33                  = 0xf25a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B33                  = 0xf25b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B33                       = 0xf25c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B33                       = 0xf25d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B33               = 0xf25e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B33              = 0xf25f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B33                = 0xf265,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B33               = 0xf266,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B33              = 0xf267,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B33           = 0xf268,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B33   = 0xf269,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B33   = 0xf26a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B33  = 0xf26b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B33  = 0xf26c,
        EN_NV_ID_DPD_LAB_PARA_B33                  = 0xf26d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B33                  = 0xf26e,
        EN_NV_ID_DPD_FAC_RESULT_B33                 = 0xf26f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B33                   = 0xf270,
        EN_NV_ID_DPD_FAC_LUT_00_B33                    = 0xf271,
        EN_NV_ID_DPD_FAC_LUT_01_B33                    = 0xf272,
        EN_NV_ID_DPD_FAC_LUT_02_B33                           = 0xf273,
        EN_NV_ID_RF_CA_RCCODE_B33                        = 0xf274,





        EN_NV_ID_ANT_MODEM_LOSS_B34                     = 0xf280,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B34    =    0xf281,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B34    =    0xf282,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B34    =    0xf283,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B34    =    0xf284,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B34    =    0xf285,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B34    =    0xf286,
        EN_NV_ID_DPD_FAC_LUT_03_B34    =    0xf287,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B34    =    0xf288,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B34         = 0xf28d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B34         = 0xf28e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B34       = 0xf28f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B34       = 0xf290,
        EN_NV_ID_LTE_IP2_CAL_B34                        = 0xf291,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B34      = 0xf292,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B34       = 0xf293,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B34       = 0xf294,
        //EN_NV_ID_LTE_PA_POWER_B34                       = 0xf295,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B34        = 0xf296,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B34         = 0xf297,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B34         = 0xf298,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B34                    = 0xf299,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B34                  = 0xf29a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B34                  = 0xf29b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B34                       = 0xf29c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B34                       = 0xf29d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B34               = 0xf29e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B34              = 0xf29f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B34                = 0xf2a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B34               = 0xf2a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B34              = 0xf2a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B34           = 0xf2a8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B34   = 0xf2a9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B34   = 0xf2aa,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B34  = 0xf2ab,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B34  = 0xf2ac,
        EN_NV_ID_DPD_LAB_PARA_B34                  = 0xf2ad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B34                  = 0xf2ae,
        EN_NV_ID_DPD_FAC_RESULT_B34                 = 0xf2af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B34                   = 0xf2b0,
        EN_NV_ID_DPD_FAC_LUT_00_B34                    = 0xf2b1,
        EN_NV_ID_DPD_FAC_LUT_01_B34                    = 0xf2b2,
        EN_NV_ID_DPD_FAC_LUT_02_B34                           = 0xf2b3,
        EN_NV_ID_RF_CA_RCCODE_B34                        = 0xf2b4,

        EN_NV_ID_ANT_MODEM_LOSS_B35                     = 0xf2c0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B35    =    0xf2c1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B35    =    0xf2c2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B35    =    0xf2c3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B35    =    0xf2c4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B35    =    0xf2c5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B35    =    0xf2c6,
        EN_NV_ID_DPD_FAC_LUT_03_B35    =    0xf2c7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B35    =    0xf2c8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B35         = 0xf2cd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B35         = 0xf2ce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B35       = 0xf2cf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B35       = 0xf2d0,
        EN_NV_ID_LTE_IP2_CAL_B35                        = 0xf2d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B35      = 0xf2d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B35       = 0xf2d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B35       = 0xf2d4,
        //EN_NV_ID_LTE_PA_POWER_B35                       = 0xf2d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B35        = 0xf2d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B35         = 0xf2d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B35         = 0xf2d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B35                    = 0xf2d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B35                  = 0xf2da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B35                  = 0xf2db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B35                       = 0xf2dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B35                       = 0xf2dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B35               = 0xf2de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B35              = 0xf2df,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B35                = 0xf2e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B35               = 0xf2e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B35              = 0xf2e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B35           = 0xf2e8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B35   = 0xf2e9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B35   = 0xf2ea,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B35  = 0xf2eb,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B35  = 0xf2ec,
        EN_NV_ID_DPD_LAB_PARA_B35                  = 0xf2ed,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B35                  = 0xf2ee,
        EN_NV_ID_DPD_FAC_RESULT_B35                 = 0xf2ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B35                   = 0xf2f0,
        EN_NV_ID_DPD_FAC_LUT_00_B35                    = 0xf2f1,
        EN_NV_ID_DPD_FAC_LUT_01_B35                    = 0xf2f2,
        EN_NV_ID_DPD_FAC_LUT_02_B35                           = 0xf2f3,
        EN_NV_ID_RF_CA_RCCODE_B35                        = 0xf2f4,

        EN_NV_ID_ANT_MODEM_LOSS_B36                     = 0xf300,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B36    =    0xf301,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B36    =    0xf302,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B36    =    0xf303,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B36    =    0xf304,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B36    =    0xf305,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B36    =    0xf306,
        EN_NV_ID_DPD_FAC_LUT_03_B36    =    0xf307,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B36    =    0xf308,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B36         = 0xf30d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B36         = 0xf30e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B36       = 0xf30f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B36       = 0xf310,
        EN_NV_ID_LTE_IP2_CAL_B36                        = 0xf311,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B36      = 0xf312,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B36       = 0xf313,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B36       = 0xf314,
        //EN_NV_ID_LTE_PA_POWER_B36                       = 0xf315,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B36        = 0xf316,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B36         = 0xf317,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B36         = 0xf318,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B36                    = 0xf319,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B36                  = 0xf31a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B36                  = 0xf31b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B36                       = 0xf31c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B36                       = 0xf31d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B36               = 0xf31e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B36              = 0xf31f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B36                = 0xf325,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B36               = 0xf326,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B36              = 0xf327,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B36           = 0xf328,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B36   = 0xf329,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B36   = 0xf32a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B36  = 0xf32b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B36  = 0xf32c,
        EN_NV_ID_DPD_LAB_PARA_B36                  = 0xf32d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B36                  = 0xf32e,
        EN_NV_ID_DPD_FAC_RESULT_B36                 = 0xf32f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B36                   = 0xf330,
        EN_NV_ID_DPD_FAC_LUT_00_B36                    = 0xf331,
        EN_NV_ID_DPD_FAC_LUT_01_B36                    = 0xf332,
        EN_NV_ID_DPD_FAC_LUT_02_B36                           = 0xf333,
        EN_NV_ID_RF_CA_RCCODE_B36                        = 0xf334,

        EN_NV_ID_ANT_MODEM_LOSS_B37                     = 0xf340,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B37    =    0xf341,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B37    =    0xf342,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B37    =    0xf343,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B37    =    0xf344,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B37    =    0xf345,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B37    =    0xf346,
        EN_NV_ID_DPD_FAC_LUT_03_B37    =    0xf347,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B37    =    0xf348,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B37         = 0xf34d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B37         = 0xf34e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B37       = 0xf34f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B37       = 0xf350,
        EN_NV_ID_LTE_IP2_CAL_B37                        = 0xf351,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B37      = 0xf352,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B37       = 0xf353,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B37       = 0xf354,
        //EN_NV_ID_LTE_PA_POWER_B37                       = 0xf355,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B37        = 0xf356,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B37         = 0xf357,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B37         = 0xf358,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B37                    = 0xf359,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B37                  = 0xf35a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B37                  = 0xf35b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B37                       = 0xf35c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B37                       = 0xf35d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B37               = 0xf35e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B37              = 0xf35f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B37                = 0xf365,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B37               = 0xf366,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B37              = 0xf367,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B37           = 0xf368,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B37   = 0xf369,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B37   = 0xf36a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B37  = 0xf36b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B37  = 0xf36c,
        EN_NV_ID_DPD_LAB_PARA_B37                  = 0xf36d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B37                  = 0xf36e,
        EN_NV_ID_DPD_FAC_RESULT_B37                 = 0xf36f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B37                   = 0xf370,
        EN_NV_ID_DPD_FAC_LUT_00_B37                    = 0xf371,
        EN_NV_ID_DPD_FAC_LUT_01_B37                    = 0xf372,
        EN_NV_ID_DPD_FAC_LUT_02_B37                           = 0xf373,
        EN_NV_ID_RF_CA_RCCODE_B37                        = 0xf374,

        EN_NV_ID_ANT_MODEM_LOSS_B42                     = 0xf380,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B42    =    0xf381,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B42    =    0xf382,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B42    =    0xf383,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B42    =    0xf384,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B42    =    0xf385,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B42    =    0xf386,
        EN_NV_ID_DPD_FAC_LUT_03_B42    =    0xf387,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B42    =    0xf388,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B42         = 0xf38d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B42         = 0xf38e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B42       = 0xf38f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B42       = 0xf390,
        EN_NV_ID_LTE_IP2_CAL_B42                        = 0xf391,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B42      = 0xf392,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B42       = 0xf393,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B42       = 0xf394,
        //EN_NV_ID_LTE_PA_POWER_B42                       = 0xf395,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B42        = 0xf396,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B42         = 0xf397,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B42         = 0xf398,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B42                    = 0xf399,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B42                  = 0xf39a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B42                  = 0xf39b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B42                       = 0xf39c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B42                       = 0xf39d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B42               = 0xf39e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B42              = 0xf39f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B42                = 0xf3a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B42               = 0xf3a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B42              = 0xf3a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B42           = 0xf3a8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B42   = 0xf3a9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B42   = 0xf3aa,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B42  = 0xf3ab,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B42  = 0xf3ac,
        EN_NV_ID_DPD_LAB_PARA_B42                  = 0xf3ad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B42                  = 0xf3ae,
        EN_NV_ID_DPD_FAC_RESULT_B42                 = 0xf3af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B42                   = 0xf3b0,
        EN_NV_ID_DPD_FAC_LUT_00_B42                    = 0xf3b1,
        EN_NV_ID_DPD_FAC_LUT_01_B42                    = 0xf3b2,
        EN_NV_ID_DPD_FAC_LUT_02_B42                           = 0xf3b3,
        EN_NV_ID_RF_CA_RCCODE_B42                        = 0xf3b4,

        EN_NV_ID_ANT_MODEM_LOSS_B43                     = 0xf3c0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B43    =    0xf3c1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B43    =    0xf3c2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B43    =    0xf3c3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B43    =    0xf3c4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B43    =    0xf3c5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B43    =    0xf3c6,
        EN_NV_ID_DPD_FAC_LUT_03_B43    =    0xf3c7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B43    =    0xf3c8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B43         = 0xf3cd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B43         = 0xf3ce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B43       = 0xf3cf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B43       = 0xf3d0,
        EN_NV_ID_LTE_IP2_CAL_B43                        = 0xf3d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B43      = 0xf3d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B43       = 0xf3d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B43       = 0xf3d4,
        //EN_NV_ID_LTE_PA_POWER_B43                       = 0xf3d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B43        = 0xf3d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B43         = 0xf3d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B43         = 0xf3d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B43                    = 0xf3d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B43                  = 0xf3da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B43                  = 0xf3db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B43                       = 0xf3dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B43                       = 0xf3dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B43               = 0xf3de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B43              = 0xf3df,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B43                = 0xf3e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B43               = 0xf3e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B43              = 0xf3e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B43           = 0xf3e8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B43   = 0xf3e9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B43   = 0xf3ea,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B43  = 0xf3eb,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B43  = 0xf3ec,
        EN_NV_ID_DPD_LAB_PARA_B43                  = 0xf3ed,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B43                  = 0xf3ee,
        EN_NV_ID_DPD_FAC_RESULT_B43                 = 0xf3ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B43                   = 0xf3f0,
        EN_NV_ID_DPD_FAC_LUT_00_B43                    = 0xf3f1,
        EN_NV_ID_DPD_FAC_LUT_01_B43                    = 0xf3f2,
        EN_NV_ID_DPD_FAC_LUT_02_B43                           = 0xf3f3,
        EN_NV_ID_RF_CA_RCCODE_B43                        = 0xf3f4,

        EN_NV_ID_ANT_MODEM_LOSS_B39                     = 0xf400,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B39    =    0xf401,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B39    =    0xf402,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B39    =    0xf403,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B39    =    0xf404,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B39    =    0xf405,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B39    =    0xf406,
        EN_NV_ID_DPD_FAC_LUT_03_B39    =    0xf407,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B39    =    0xf408,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B39         = 0xf40d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B39         = 0xf40e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B39       = 0xf40f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B39       = 0xf410,
        EN_NV_ID_LTE_IP2_CAL_B39                        = 0xf411,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B39      = 0xf412,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B39       = 0xf413,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B39       = 0xf414,
        //EN_NV_ID_LTE_PA_POWER_B39                       = 0xf415,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B39        = 0xf416,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B39         = 0xf417,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B39         = 0xf418,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B39                    = 0xf419,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B39                  = 0xf41a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B39                  = 0xf41b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B39                       = 0xf41c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B39                       = 0xf41d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B39               = 0xf41e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B39              = 0xf41f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B39                = 0xf425,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B39               = 0xf426,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B39              = 0xf427,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B39           = 0xf428,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B39   = 0xf429,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B39     = 0xf42a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B39  = 0xf42b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B39  = 0xf42c,
        EN_NV_ID_DPD_LAB_PARA_B39                  = 0xf42d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B39                  = 0xf42e,
        EN_NV_ID_DPD_FAC_RESULT_B39                 = 0xf42f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B39                   = 0xf430,
        EN_NV_ID_DPD_FAC_LUT_00_B39                    = 0xf431,
        EN_NV_ID_DPD_FAC_LUT_01_B39                    = 0xf432,
        EN_NV_ID_DPD_FAC_LUT_02_B39                           = 0xf433,
        EN_NV_ID_RF_CA_RCCODE_B39                        = 0xf434,

    /*BAND28 生产NV*/
        EN_NV_ID_ANT_MODEM_LOSS_B28                     = 0xf440,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B28    =    0xf441,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B28    =    0xf442,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B28    =    0xf443,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B28    =    0xf444,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B28    =    0xf445,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B28    =    0xf446,
        EN_NV_ID_DPD_FAC_LUT_03_B28    =    0xf447,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B28    =    0xf448,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B28         = 0xf44D,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B28         = 0xf44E,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B28     = 0xf44F,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B28     = 0xf450,
        EN_NV_ID_LTE_IP2_CAL_B28                        = 0xf451,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B28    = 0xf452,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B28              = 0xf453,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B28             = 0xf454,
        //EN_NV_ID_LTE_PA_POWER_B28                     = 0xf455,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B28        = 0xf456,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B28         = 0xf457,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B28         = 0xf458,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B28                   = 0xf459,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B28                 = 0xf45a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B28                 = 0xf45b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B28                 = 0xf45c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B28               = 0xf45d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B28         = 0xf45e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B28             = 0xf45f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B28               = 0xf465,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B28              = 0xf466,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B28             = 0xf467,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B28          = 0xf468,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B28   = 0xf469,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B28     = 0xf46a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B28  = 0xf46b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B28  = 0xf46c,
        EN_NV_ID_DPD_LAB_PARA_B28                  = 0xf46d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B28                  = 0xf46e,
        EN_NV_ID_DPD_FAC_RESULT_B28                 = 0xf46f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B28                   = 0xf470,
        EN_NV_ID_DPD_FAC_LUT_00_B28                    = 0xf471,
        EN_NV_ID_DPD_FAC_LUT_01_B28                    = 0xf472,
        EN_NV_ID_DPD_FAC_LUT_02_B28                           = 0xf473,
        EN_NV_ID_RF_CA_RCCODE_B28                        = 0xf474,

    /*
        EN_NV_ID_ANT_MODEM_LOSS_BNon1                     = 0xf440,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon1    =    0xf441,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon1    =    0xf442,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon1    =    0xf443,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon1    =    0xf444,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon1    =    0xf445,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon1    =    0xf446,
        EN_NV_ID_DPD_FAC_LUT_03_BNon1    =    0xf447,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon1    =    0xf448,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon1         = 0xf44d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon1         = 0xf44e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon1       = 0xf44f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon1       = 0xf450,
        EN_NV_ID_LTE_IP2_CAL_BNon1                        = 0xf451,
        EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon1      = 0xf452,
        EN_NV_ID_LTE_TX_APC_MIDGAIN_FREQ_COMP_BNon1       = 0xf453,
        EN_NV_ID_LTE_TX_APC_LOWGAIN_FREQ_COMP_BNon1       = 0xf454,
        EN_NV_ID_LTE_PA_POWER_BNon1                       = 0xf455,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon1        = 0xf456,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon1         = 0xf457,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon1         = 0xf458,
        */
        /*add for V9R1_6361 Begin*/
    /*    EN_NV_ID_IIP2_CAL_TABLE_BNon1                    = 0xf459,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon1                 = 0xf45a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon1                  = 0xf45b,
        EN_NV_ID_TX_APC_GAIN_BNon1                       = 0xf45c,
        EN_NV_ID_RF_TXIQ_CAL_BNon1                       = 0xf45d,
        EN_NV_ID_PA_POWER_DIFFERENCE_BNon1               = 0xf45e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon1              = 0xf45f,
        */
        /*add for V9R1_6361 End*/
    /*BEGIN     modify for B28全频段特性*/
       /*BAND128 生产NV*/
        EN_NV_ID_ANT_MODEM_LOSS_B128                        = 0xf480,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B128    =    0xf481,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B128    =    0xf482,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B128    =    0xf483,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B128    =    0xf484,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B128    =    0xf485,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B128    =    0xf486,
        EN_NV_ID_DPD_FAC_LUT_03_B128    =    0xf487,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B128    =    0xf488,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B128        = 0xf48D,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B128        = 0xf48E,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B128        = 0xf48F,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B128        = 0xf490,
        EN_NV_ID_LTE_IP2_CAL_B128                       = 0xf491,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B128       = 0xf492,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B128     = 0xf493,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B128        = 0xf494,
        //EN_NV_ID_LTE_PA_POWER_B128                        = 0xf495,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B128       = 0xf496,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B128        = 0xf497,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B128        = 0xf498,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B128                        = 0xf499,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B128                  = 0xf49a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B128                  = 0xf49b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B128                        = 0xf49c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B128                      = 0xf49d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B128                = 0xf49e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B128              = 0xf49f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B128                    = 0xf4a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B128               = 0xf4a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B128              = 0xf4a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B128           = 0xf4a8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B128    = 0xf4a9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B128  = 0xf4aa,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B128 = 0xf4ab,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B128 = 0xf4ac,
        EN_NV_ID_DPD_LAB_PARA_B128                   = 0xf4ad,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B128                   = 0xf4ae,
        EN_NV_ID_DPD_FAC_RESULT_B128                  = 0xf4af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B128                   = 0xf4b0,
        EN_NV_ID_DPD_FAC_LUT_00_B128                    = 0xf4b1,
        EN_NV_ID_DPD_FAC_LUT_01_B128                    = 0xf4b2,
        EN_NV_ID_DPD_FAC_LUT_02_B128                           = 0xf4b3,
        EN_NV_ID_RF_CA_RCCODE_B128                        = 0xf4b4,
    /*END     modify for B28全频段特性*/


    /*BAND29 生产NV*/
        EN_NV_ID_ANT_MODEM_LOSS_B29                        = 0xf4c0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B29    =    0xf4c1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B29    =    0xf4c2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B29    =    0xf4c3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B29    =    0xf4c4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B29    =    0xf4c5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B29    =    0xf4c6,
        EN_NV_ID_DPD_FAC_LUT_03_B29    =    0xf4c7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B29    =    0xf4c8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B29        = 0xf4cd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B29        = 0xf4ce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B29        = 0xf4cf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B29        = 0xf4d0,
        EN_NV_ID_LTE_IP2_CAL_B29                       = 0xf4d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B29       = 0xf4d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B29     = 0xf4d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B29        = 0xf4d4,
        //EN_NV_ID_LTE_PA_POWER_B29                        = 0xf4d5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B29       = 0xf4d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B29        = 0xf4d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B29        = 0xf4d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B29                        = 0xf4d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B29                  = 0xf4da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B29                  = 0xf4db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B29                        = 0xf4dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B29                      = 0xf4dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B29                = 0xf4de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B29              = 0xf4df,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B29                    = 0xf4e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B29               = 0xf4e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B29              = 0xf4e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B29           = 0xf4e8,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B29    = 0xf4e9,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B29  = 0xf4ea,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B29 = 0xf4eb,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B29 = 0xf4ec,
        EN_NV_ID_DPD_LAB_PARA_B29                   = 0xf4ed,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B29                   = 0xf4ee,
        EN_NV_ID_DPD_FAC_RESULT_B29                  = 0xf4ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B29                   = 0xf4f0,
        EN_NV_ID_DPD_FAC_LUT_00_B29                    = 0xf4f1,
        EN_NV_ID_DPD_FAC_LUT_01_B29                    = 0xf4f2,
        EN_NV_ID_DPD_FAC_LUT_02_B29                           = 0xf4f3,
        EN_NV_ID_RF_CA_RCCODE_B29                        = 0xf4f4,


      /*BAND32 生产NV*/
        EN_NV_ID_ANT_MODEM_LOSS_B32                        = 0xf500,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B32    =    0xf501,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B32    =    0xf502,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B32    =    0xf503,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B32    =    0xf504,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B32    =    0xf505,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B32    =    0xf506,
        EN_NV_ID_DPD_FAC_LUT_03_B32    =    0xf507,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B32    =    0xf508,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B32        = 0xf50d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B32        = 0xf50e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B32        = 0xf50f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B32        = 0xf510,
        EN_NV_ID_LTE_IP2_CAL_B32                       = 0xf511,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B32       = 0xf512,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B32     = 0xf513,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B32        = 0xf514,
        //EN_NV_ID_LTE_PA_POWER_B32                        = 0xf515,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B32       = 0xf516,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B32        = 0xf517,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B32        = 0xf518,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B32                        = 0xf519,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B32                  = 0xf51a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B32                  = 0xf51b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B32                        = 0xf51c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B32                      = 0xf51d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B32                = 0xf51e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B32              = 0xf51f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B32                    = 0xf525,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B32               = 0xf526,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B32              = 0xf527,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B32           = 0xf528,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B32    = 0xf529,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B32  = 0xf52a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B32 = 0xf52b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B32 = 0xf52c,
        EN_NV_ID_DPD_LAB_PARA_B32                   = 0xf52d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B32                   = 0xf52e,
        EN_NV_ID_DPD_FAC_RESULT_B32                  = 0xf52f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B32                   = 0xf530,
        EN_NV_ID_DPD_FAC_LUT_00_B32                    = 0xf531,
        EN_NV_ID_DPD_FAC_LUT_01_B32                    = 0xf532,
        EN_NV_ID_DPD_FAC_LUT_02_B32                           = 0xf533,
        EN_NV_ID_RF_CA_RCCODE_B32                        = 0xf534,

    /*BAND30 生产NV*/
        EN_NV_ID_ANT_MODEM_LOSS_B30                     = 0xf540,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B30    =    0xf541,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B30    =    0xf542,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B30    =    0xf543,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B30    =    0xf544,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B30    =    0xf545,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B30    =    0xf546,
        EN_NV_ID_DPD_FAC_LUT_03_B30    =    0xf547,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B30    =    0xf548,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B30             = 0xf54d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B30             = 0xf54e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B30             = 0xf54f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B30             = 0xf550,
        EN_NV_ID_LTE_IP2_CAL_B30                        = 0xf551,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B30    = 0xf552,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B30              = 0xf553,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B30             = 0xf554,
        //EN_NV_ID_LTE_PA_POWER_B30                     = 0xf555,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B30        = 0xf556,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B30         = 0xf557,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B30         = 0xf558,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B30                     = 0xf559,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B30                   = 0xf55a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B30                   = 0xf55b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B30                 = 0xf55c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B30               = 0xf55d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B30         = 0xf55e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B30               = 0xf55f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B30                 = 0xf565,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B30                = 0xf566,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B30               = 0xf567,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B30            = 0xf568,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B30     = 0xf569,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B30    = 0xf56a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B30  = 0xf56b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B30  = 0xf56c,
        EN_NV_ID_DPD_LAB_PARA_B30                    = 0xf56d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B30                    = 0xf56e,
        EN_NV_ID_DPD_FAC_RESULT_B30                   = 0xf56f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B30                    = 0xf570,
        EN_NV_ID_DPD_FAC_LUT_00_B30                     = 0xf571,
        EN_NV_ID_DPD_FAC_LUT_01_B30                     = 0xf572,
        EN_NV_ID_DPD_FAC_LUT_02_B30                           = 0xf573,
        EN_NV_ID_RF_CA_RCCODE_B30                        = 0xf574,


        EN_NV_ID_ANT_MODEM_LOSS_BNon6                     = 0xf580,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon6    =    0xf581,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon6    =    0xf582,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon6    =    0xf583,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon6    =    0xf584,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon6    =    0xf585,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon6    =    0xf586,
        EN_NV_ID_DPD_FAC_LUT_03_BNon6    =    0xf587,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon6    =    0xf588,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon6         = 0xf58d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon6         = 0xf58e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon6       = 0xf58f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon6       = 0xf590,
        EN_NV_ID_LTE_IP2_CAL_BNon6                        = 0xf591,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon6      = 0xf592,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon6       = 0xf593,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon6       = 0xf594,
        //EN_NV_ID_LTE_PA_POWER_BNon6                       = 0xf595,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon6        = 0xf596,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon6         = 0xf597,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon6         = 0xf598,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon6                    = 0xf599,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon6                 = 0xf59a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon6                  = 0xf59b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon6                       = 0xf59c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon6                       = 0xf59d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon6               = 0xf59e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon6              = 0xf59f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_ANT_MODEM_LOSS_BNon7                     = 0xf5c0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon7    =    0xf5c1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon7    =    0xf5c2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon7    =    0xf5c3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon7    =    0xf5c4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon7    =    0xf5c5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon7    =    0xf5c6,
        EN_NV_ID_DPD_FAC_LUT_03_BNon7    =    0xf5c7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon7    =    0xf5c8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon7         = 0xf5cd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon7         = 0xf5ce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon7       = 0xf5cf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon7       = 0xf5d0,
        EN_NV_ID_LTE_IP2_CAL_BNon7                        = 0xf5d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon7      = 0xf5d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon7       = 0xf5d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon7       = 0xf5d4,
        //EN_NV_ID_LTE_PA_POWER_BNon7                       = 0xf5d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon7        = 0xf5d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon7         = 0xf5d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon7         = 0xf5d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon7                    = 0xf5d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon7                 = 0xf5da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon7                  = 0xf5db,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon7                       = 0xf5dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon7                       = 0xf5dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon7               = 0xf5de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon7              = 0xf5df,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon8                     = 0xf600,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon8    =    0xf601,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon8    =    0xf602,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon8    =    0xf603,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon8    =    0xf604,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon8    =    0xf605,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon8    =    0xf606,
        EN_NV_ID_DPD_FAC_LUT_03_BNon8    =    0xf607,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon8    =    0xf608,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon8         = 0xf60d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon8         = 0xf60e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon8       = 0xf60f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon8       = 0xf610,
        EN_NV_ID_LTE_IP2_CAL_BNon8                        = 0xf611,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon8      = 0xf612,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon8       = 0xf613,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon8       = 0xf614,
        //EN_NV_ID_LTE_PA_POWER_BNon8                       = 0xf615,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon8        = 0xf616,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon8         = 0xf617,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon8         = 0xf618,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon8                    = 0xf619,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon8                 = 0xf61a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon8                  = 0xf61b,
        EN_NV_ID_TX_APC_GAIN_BNon8                       = 0xf61c,
        EN_NV_ID_RF_TXIQ_CAL_BNon8                       = 0xf61d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon8               = 0xf61e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon8              = 0xf61f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon9                     = 0xf640,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon9    =    0xf641,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon9    =    0xf642,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon9    =    0xf643,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon9    =    0xf644,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon9    =    0xf645,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon9    =    0xf646,
        EN_NV_ID_DPD_FAC_LUT_03_BNon9    =    0xf647,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon9    =    0xf648,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon9         = 0xf64d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon9         = 0xf64e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon9       = 0xf64f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon9       = 0xf650,
        EN_NV_ID_LTE_IP2_CAL_BNon9                        = 0xf651,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon9      = 0xf652,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon9       = 0xf653,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon9       = 0xf654,
        //EN_NV_ID_LTE_PA_POWER_BNon9                       = 0xf655,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon9        = 0xf656,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon9         = 0xf657,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon9         = 0xf658,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon9                    = 0xf659,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon9                 = 0xf65a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon9                  = 0xf65b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon9                       = 0xf65c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon9                       = 0xf65d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon9               = 0xf65e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon9              = 0xf65f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon10                     = 0xf680,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon10    =    0xf681,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon10    =    0xf682,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon10    =    0xf683,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon10    =    0xf684,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon10    =    0xf685,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon10    =    0xf686,
        EN_NV_ID_DPD_FAC_LUT_03_BNon10    =    0xf687,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon10    =    0xf688,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon10         = 0xf68d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon10         = 0xf68e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon10       = 0xf68f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon10       = 0xf690,
        EN_NV_ID_LTE_IP2_CAL_BNon10                        = 0xf691,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon10      = 0xf692,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon10       = 0xf693,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon10       = 0xf694,
        //EN_NV_ID_LTE_PA_POWER_BNon10                       = 0xf695,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon10        = 0xf696,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon10         = 0xf697,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon10         = 0xf698,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon10                   = 0xf699,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon10                = 0xf69a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon10                 = 0xf69b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon10                      = 0xf69c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon10                      = 0xf69d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon10              = 0xf69e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon10             = 0xf69f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon11                     = 0xf6c0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon11    =    0xf6c1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon11    =    0xf6c2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon11    =    0xf6c3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon11    =    0xf6c4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon11    =    0xf6c5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon11    =    0xf6c6,
        EN_NV_ID_DPD_FAC_LUT_03_BNon11    =    0xf6c7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon11    =    0xf6c8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon11         = 0xf6cd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon11         = 0xf6ce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon11       = 0xf6cf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon11       = 0xf6d0,
        EN_NV_ID_LTE_IP2_CAL_BNon11                        = 0xf6d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon11      = 0xf6d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon11       = 0xf6d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon11       = 0xf6d4,
        //EN_NV_ID_LTE_PA_POWER_BNon11                       = 0xf6d5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon11        = 0xf6d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon11         = 0xf6d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon11         = 0xf6d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon11                   = 0xf6d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon11                = 0xf6da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon11                 = 0xf6db,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon11                      = 0xf6dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon11                      = 0xf6dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon11              = 0xf6de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon11             = 0xf6df,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon12                     = 0xf700,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon12    =    0xf701,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon12    =    0xf702,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon12    =    0xf703,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon12    =    0xf704,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon12    =    0xf705,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon12    =    0xf706,
        EN_NV_ID_DPD_FAC_LUT_03_BNon12    =    0xf707,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon12    =    0xf708,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon12         = 0xf70d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon12         = 0xf70e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon12       = 0xf70f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon12       = 0xf710,
        EN_NV_ID_LTE_IP2_CAL_BNon12                        = 0xf711,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon12      = 0xf712,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon12       = 0xf713,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon12       = 0xf714,
        //EN_NV_ID_LTE_PA_POWER_BNon12                       = 0xf715,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon12        = 0xf716,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon12         = 0xf717,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon12         = 0xf718,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon12                   = 0xf719,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon12                = 0xf71a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon12                 = 0xf71b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon12                      = 0xf71c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon12                      = 0xf71d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon12              = 0xf71e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon12             = 0xf71f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon13                     = 0xf740,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon13    =    0xf741,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon13    =    0xf742,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon13    =    0xf743,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon13    =    0xf744,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon13    =    0xf745,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon13    =    0xf746,
        EN_NV_ID_DPD_FAC_LUT_03_BNon13    =    0xf747,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon13    =    0xf748,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon13         = 0xf74d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon13         = 0xf74e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon13       = 0xf74f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon13       = 0xf750,
        EN_NV_ID_LTE_IP2_CAL_BNon13                        = 0xf751,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon13      = 0xf752,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon13       = 0xf753,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon13       = 0xf754,
        //EN_NV_ID_LTE_PA_POWER_BNon13                       = 0xf755,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon13        = 0xf756,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon13         = 0xf757,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon13         = 0xf758,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon13                   = 0xf759,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon13                = 0xf75a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon13                 = 0xf75b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon13                      = 0xf75c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon13                      = 0xf75d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon13              = 0xf75e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon13             = 0xf75f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon14                     = 0xf780,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon14    =    0xf781,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon14    =    0xf782,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon14    =    0xf783,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon14    =    0xf784,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon14    =    0xf785,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon14    =    0xf786,
        EN_NV_ID_DPD_FAC_LUT_03_BNon14    =    0xf787,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon14    =    0xf788,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon14         = 0xf78d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon14         = 0xf78e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon14       = 0xf78f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon14       = 0xf790,
        EN_NV_ID_LTE_IP2_CAL_BNon14                        = 0xf791,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon14      = 0xf792,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon14       = 0xf793,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon14       = 0xf794,
        //EN_NV_ID_LTE_PA_POWER_BNon14                       = 0xf795,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon14        = 0xf796,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon14         = 0xf797,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon14         = 0xf798,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon14                   = 0xf799,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon14                = 0xf79a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon14                 = 0xf79b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon14                      = 0xf79c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon14                      = 0xf79d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon14              = 0xf79e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon14             = 0xf79f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon15                     = 0xf7c0,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon15    =    0xf7c1,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon15    =    0xf7c2,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon15    =    0xf7c3,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon15    =    0xf7c4,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon15    =    0xf7c5,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon15    =    0xf7c6,
        EN_NV_ID_DPD_FAC_LUT_03_BNon15    =    0xf7c7,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon15    =    0xf7c8,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon15         = 0xf7cd,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon15         = 0xf7ce,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon15       = 0xf7cf,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon15       = 0xf7d0,
        EN_NV_ID_LTE_IP2_CAL_BNon15                        = 0xf7d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon15      = 0xf7d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon15       = 0xf7d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon15       = 0xf7d4,
        //EN_NV_ID_LTE_PA_POWER_BNon15                       = 0xf7d5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon15        = 0xf7d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon15         = 0xf7d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon15         = 0xf7d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon15                   = 0xf7d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon15                = 0xf7da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon15                 = 0xf7db,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon15                      = 0xf7dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon15                      = 0xf7dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon15              = 0xf7de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon15             = 0xf7df,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon16                     = 0xf800,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_BNon16    =    0xf801,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_BNon16    =    0xf802,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_BNon16    =    0xf803,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_BNon16    =    0xf804,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_BNon16    =    0xf805,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_BNon16    =    0xf806,
        EN_NV_ID_DPD_FAC_LUT_03_BNon16    =    0xf807,
        //EN_NV_ID_LTE_DPD_LAB_STRU_BNon16    =    0xf808,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_BNon16         = 0xf80d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_BNon16         = 0xf80e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_BNon16       = 0xf80f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_BNon16       = 0xf810,
        EN_NV_ID_LTE_IP2_CAL_BNon16                        = 0xf811,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon16      = 0xf812,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon16       = 0xf813,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon16       = 0xf814,
        //EN_NV_ID_LTE_PA_POWER_BNon16                       = 0xf815,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon16        = 0xf816,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon16         = 0xf817,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon16         = 0xf818,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon16                   = 0xf819,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon16                = 0xf81a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon16                 = 0xf81b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon16                      = 0xf81c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon16                      = 0xf81d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon16              = 0xf81e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon16             = 0xf81f,
        /*add for V9R1_6361 End*/
        /* v7r5 band26 */
        EN_NV_ID_ANT_MODEM_LOSS_B26                     = 0xf820,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B26    =    0xf821,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B26    =    0xf822,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B26    =    0xf823,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B26    =    0xf824,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B26    =    0xf825,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B26    =    0xf826,
        EN_NV_ID_DPD_FAC_LUT_03_B26    =    0xf827,
        //EN_NV_ID_LTE_DPD_LAB_STRU_B26    =    0xf828,
        EN_NV_ID_LTE_RX_CHAN0_FREQ_COMP_B26            = 0xf82d,
        EN_NV_ID_LTE_RX_CHAN1_FREQ_COMP_B26         = 0xf82e,
        EN_NV_ID_LTE_RX_CHAN2_FREQ_COMP_B26       = 0xf82f,
        EN_NV_ID_LTE_RX_CHAN3_FREQ_COMP_B26       = 0xf830,
        EN_NV_ID_LTE_IP2_CAL_B26                        = 0xf831,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B26      = 0xf832,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B26       = 0xf833,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B26       = 0xf834,
        //EN_NV_ID_LTE_PA_POWER_B26                       = 0xf835,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B26        = 0xf836,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B26         = 0xf837,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B26         = 0xf838,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B26                    = 0xf839,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B26                  = 0xf83a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B26                  = 0xf83b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B26                       = 0xf83c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B26                     = 0xf83d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B26               = 0xf83e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B26              = 0xf83f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B26                = 0xf845,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B26               = 0xf846,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B26              = 0xf847,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B26           = 0xf848,
        ///*EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B26   = 0xf849,
        //EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B26   = 0xf84a,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B26  = 0xf84b,
        //EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B26  = 0xf84c,
        EN_NV_ID_DPD_LAB_PARA_B26                  = 0xf84d,
        EN_NV_ID_DPD_LAB_PIN_OFFSET_B26                  = 0xf84e,
        EN_NV_ID_DPD_FAC_RESULT_B26                 = 0xf84f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B26                   = 0xf850,
        EN_NV_ID_DPD_FAC_LUT_00_B26                    = 0xf851,
        EN_NV_ID_DPD_FAC_LUT_01_B26                    = 0xf852,
        EN_NV_ID_DPD_FAC_LUT_02_B26                           = 0xf853,
        EN_NV_ID_RF_CA_RCCODE_B26                        = 0xf854,


        /* modify by   for 所有band end*/

        EN_NV_ID_TIMING_PARA                            = 0xD3C0,
        EN_NV_ID_EMU_FAKECELL_PARA                      = 0xD3C1,
        EN_NV_ID_CQI_PARA                               = 0xD3C2,
        EN_NV_ID_ANTCORR_PARA                           = 0xD3C3,
        EN_NV_ID_RLM_PARA                               = 0xD3C4,
        EN_NV_ID_AFC_PARA                               = 0xD3C5,
        EN_NV_ID_IRC_PUB_PARA                           = 0xD3C6,
        EN_NV_ID_CHE_PARA                               = 0xD3C7,
        EN_NV_ID_VITERBI_PARA                           = 0xD3C8,
        EN_NV_ID_TURBO_PARA                             = 0xD3C9,
        EN_NV_ID_DEM_LIST_PARA                          = 0xD3CA,
        EN_NV_ID_AD9361_UL_PARA                         = 0xD3CB,
        EN_NV_ID_HI6360_UL_PARA                         = 0xD3CC,
        EN_NV_ID_EMU_PARA                               = 0xD3CE,

        /* modify by   begin */
        EN_NV_ID_PHY_FUNC_VERIFY_SWITCH_PARA            = 0xD3E0,
        /* modify by   end */

        /*tcx0*/
        EN_NV_ID_TCXO_DYNAMIC_CONFIG_PARA               = 0xD3E2,

        /* add by  2012-6-8 for TX_FILTER begin */
        EN_NV_ID_TX_FILTER_CMP                          = 0xD3E3,
        /* add by  2012-6-8 for TX_FILTER end */

        EN_NV_ID_LPHY_PD_COMM_PARA                      = 0xD3E5,
        /* add by   2014-9-18 for single band and ca begin */
        EN_NV_ID_RFIC_INIT_PARA                               = 0xD41f,
#if 1
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_000                  = 0xD420,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_001                  = 0xD421,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_002                  = 0xD422,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_003                  = 0xD423,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_004                  = 0xD424,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_005                  = 0xD425,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_006                  = 0xD426,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_007                  = 0xD427,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_008                  = 0xD428,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_009                  = 0xD429,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_010                  = 0xD42a,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_011                  = 0xD42b,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_012                  = 0xD42c,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_013                  = 0xD42d,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_014                  = 0xD42e,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_015                  = 0xD42f,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_016                  = 0xD430,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_017                  = 0xD431,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_018                  = 0xD432,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_019                  = 0xD433,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_020                  = 0xD434,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_021                  = 0xD435,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_022                  = 0xD436,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_023                  = 0xD437,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_024                  = 0xD438,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_025                  = 0xD439,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_026                  = 0xD43a,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_027                  = 0xD43b,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_028                  = 0xD43c,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_029                  = 0xD43d,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_030                  = 0xD43e,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_031                  = 0xD43f,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_032                  = 0xD440,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_033                  = 0xD441,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_034                  = 0xD442,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_035                  = 0xD443,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_036                  = 0xD444,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_037                  = 0xD445,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_038                  = 0xD446,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_039                  = 0xD447,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_040                  = 0xD448,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_041                  = 0xD449,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_042                  = 0xD44a,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_043                  = 0xD44b,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_044                  = 0xD44c,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_045                  = 0xD44d,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_046                  = 0xD44e,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_047                  = 0xD44f,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_048                  = 0xD450,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_049                  = 0xD451,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_050                  = 0xD452,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_051                  = 0xD453,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_052                  = 0xD454,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_053                  = 0xD455,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_054                  = 0xD456,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_055                  = 0xD457,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_056                  = 0xD458,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_057                  = 0xD459,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_058                  = 0xD45a,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_059                  = 0xD45b,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_060                  = 0xD45c,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_061                  = 0xD45d,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_062                  = 0xD45e,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_063                  = 0xD45f,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_064                  = 0xD460,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_065                  = 0xD461,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_066                  = 0xD462,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_067                  = 0xD463,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_068                  = 0xD464,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_069                  = 0xD465,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_070                  = 0xD466,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_071                  = 0xD467,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_072                  = 0xD468,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_073                  = 0xD469,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_074                  = 0xD46a,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_075                  = 0xD46b,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_076                  = 0xD46c,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_077                  = 0xD46d,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_078                  = 0xD46e,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_079                  = 0xD46f,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_080                  = 0xD470,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_081                  = 0xD471,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_082                  = 0xD472,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_083                  = 0xD473,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_084                  = 0xD474,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_085                  = 0xD475,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_086                  = 0xD476,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_087                  = 0xD477,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_088                  = 0xD478,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_089                  = 0xD479,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_090                  = 0xD47a,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_091                  = 0xD47b,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_092                  = 0xD47c,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_093                  = 0xD47d,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_094                  = 0xD47e,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_095                  = 0xD47f,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_096                  = 0xD480,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_097                  = 0xD481,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_098                  = 0xD482,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_099                  = 0xD483,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_100                  = 0xD484,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_101                  = 0xD485,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_102                  = 0xD486,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_103                  = 0xD487,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_104                  = 0xD488,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_105                  = 0xD489,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_106                  = 0xD48a,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_107                  = 0xD48b,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_108                  = 0xD48c,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_109                  = 0xD48d,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_110                  = 0xD48e,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_111                  = 0xD48f,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_112                  = 0xD490,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_113                  = 0xD491,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_114                  = 0xD492,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_115                  = 0xD493,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_116                  = 0xD494,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_117                  = 0xD495,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_118                  = 0xD496,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_119                  = 0xD497,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_120                  = 0xD498,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_121                  = 0xD499,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_122                  = 0xD49a,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_123                  = 0xD49b,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_124                  = 0xD49c,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_125                  = 0xD49d,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_126                  = 0xD49e,
        EN_NV_ID_BAND_COMB_FE_PARA_INDEX_127                  = 0xD49f,
#endif
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_000           = 0xD4A0,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_001           = 0xD4A1,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_002           = 0xD4A2,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_003           = 0xD4A3,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_004           = 0xD4A4,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_005           = 0xD4A5,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_006           = 0xD4A6,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_007           = 0xD4A7,
#if 1
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_008           = 0xD4A8,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_009           = 0xD4A9,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_010           = 0xD4Aa,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_011           = 0xD4Ab,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_012           = 0xD4Ac,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_013           = 0xD4Ad,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_014           = 0xD4Ae,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_015           = 0xD4Af,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_016           = 0xD4B0,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_017           = 0xD4B1,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_018           = 0xD4B2,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_019           = 0xD4B3,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_020           = 0xD4B4,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_021           = 0xD4B5,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_022           = 0xD4B6,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_023           = 0xD4B7,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_024           = 0xD4B8,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_025           = 0xD4B9,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_026           = 0xD4BA,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_027           = 0xD4BB,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_028           = 0xD4BC,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_029           = 0xD4BD,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_030           = 0xD4BE,
        EN_NV_ID_SINGLE_BAND_FE_PARA_BAND_INDEX_031           = 0xD4BF,
#endif
        /* add by   2014-9-18 for single band and ca end */
        EN_NV_FE_TIMER_STRU_PARA                                          = 0xD4C0,
        /* BEGIN: PN:Chicago Ext Lna Feature  added by   2016/02/25 */
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_000           = 0xD4D0,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_001           = 0xD4D1,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_002           = 0xD4D2,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_003           = 0xD4D3,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_004           = 0xD4D4,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_005           = 0xD4D5,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_006           = 0xD4D6,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_007           = 0xD4D7,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_008           = 0xD4D8,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_009           = 0xD4D9,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_010           = 0xD4Da,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_011           = 0xD4Db,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_012           = 0xD4Dc,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_013           = 0xD4Dd,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_014           = 0xD4De,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_015           = 0xD4Df,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_016           = 0xD4E0,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_017           = 0xD4E1,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_018           = 0xD4E2,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_019           = 0xD4E3,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_020           = 0xD4E4,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_021           = 0xD4E5,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_022           = 0xD4E6,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_023           = 0xD4E7,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_024           = 0xD4E8,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_025           = 0xD4E9,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_026           = 0xD4EA,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_027           = 0xD4EB,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_028           = 0xD4EC,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_029           = 0xD4ED,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_030           = 0xD4EE,
        EN_NV_ID_LTE_EXT_LNA_PARA_BAND_INDEX_031           = 0xD4EF,
        /* END: PN:Chicago Ext Lna Feature  added by   2016/02/25 */
        /* BEGIN: Added by  , 2013/3/22 */
        #if defined(LPHY_FEATURE_DSDS_20)
        EN_NV_DSDS_RX_CH_STRU_PARA                      = 0xD4C1,
        EN_NV_BAND_SPUR_STRU_PARA                       = 0xD4C2,
        #endif
        #if defined(LPHY_K3V6)
        EN_NV_THRL_DYNAMIC_SPUR_PARA                    = 0xD4C3,
        #endif
        EN_NV_ID_LTE_COM_ANT_SAR_PARA                   = 0xD4FA,
        EN_NV_ID_LTE_BODY_SAR_WIRED_FLAG                = 0xD4FB,
        EN_NV_ID_LVRAMP_PARA                            = 0xD4FC,
        EN_NV_ID_FE_NOTCH_INFO                          = 0xD4FF, /*notch*/
        EN_NV_ID_FE_BASIC_INFO                          = 0xD500,
        EN_NV_ID_FE_RFIC_INIT                           = 0xD501,
        EN_NV_ID_FE_COMM_CONFIG                         = 0xD502,
        EN_NV_ID_PBAND_INFO                             = 0xD503,
        EN_NV_ID_SBAND_INFO                             = 0xD504,
        EN_NV_ID_PBAND_MIPI_INFO                        = 0xD505,
        /* END:   Added by  , 2013/3/22 */
        EN_NV_ID_CA_TUNER_INFO                          = 0xD506,

        EN_NV_ID_APC_GAIN_DEFALUT                       = 0xD507,
        EN_NV_ID_PA_POWER_DIFFERENCE_DEFALUT            = 0xD508,
        EN_NV_ID_DSP_NV_PARA_SIZE                       = 0xD509,
        EN_NV_ID_MIPIDEV_INIT                           = 0xD50A,
        EN_NV_ID_LPHY_ET_COMM_PARA                   = 0xD50B,
        EN_NV_ID_LPHY_ET_VENDOR_PARA_TI              = 0xD50C,
        EN_NV_ID_LPHY_ET_VENDOR_PARA_R2              = 0xD50D,
        EN_NV_ID_LPHY_ET_VENDOR_PARA_QU              = 0xD50E,
        EN_NV_ID_RF_CA_RCCAL_CFG                      = 0xD50F,

        /*begin added by   2014/06/23*/
        EN_NV_ID_LPHY_LWCOEX_INIT_PARA                  = 0xD510,
        /*end added by   2014/06/23*/

        /*begin added by   2015/12/26*/
        EN_NV_ID_LPHY_LWCOEX_FEATURE_SELECT_STRU    = 0xD527,
        /*end added by   2015/12/26*/


        EN_NV_ID_ASU_PARA                           = 0xD511,
        EN_NV_SINGLE_XO_DEFINE                      = 0xD51d,
        NV_ID_PA_TYPE_FLAG_PARA                     = 0xD51f,

        /* 准入控制NV */
        EN_NV_ID_ACCESS_PARA_PARA                           = 0xD5F0,

#if 1 //defined (FEATURE_TLPHY_SINGLE_XO) || defined (FEATURE_TLPHY_TCXO_OVER_TEMP_PROTECT))
        EN_NV_ID_TL_COMM_NV_PARA_SIZE                = 0xD512,
        EN_NV_ID_DCXO_C_TRIM_DEFAULT                 = 0xD513,
        EN_NV_ID_DCXO_C2_FIX_DEFAULT                 = 0xD514,
        EN_NV_ID_XO_INIT_FREQUENCY                   = 0xD515,
        EN_NV_ID_DCXO_C_TRIM                         = 0xD516,
        EN_NV_ID_DCXO_C2_FIX                         = 0xD517,
        EN_NV_ID_DCXO_TEMP_COMP_POLY_COEF            = 0xD518,
        EN_NV_ID_DCXO_TEMP_COMP_THRESHOLD            = 0xD519,
        EN_NV_ID_DCXO_FREQ_VS_TEMP_ARRAY             = 0xD51a,
        EN_NV_ID_DCXO_TEMP_READ_PERIOD               = 0xD51b,
        EN_NV_ID_DCXO_TEMP_COMP_POLY_ALL             = 0xD51c,
        EN_NV_ID_XO_AGING_INFO                       = 0xF93F,
#endif
        EN_NV_ID_PA_MIPI_ADDED                       = 0xD520,
        EN_NV_ID_CONVERT_FACTOR                      = 0xD521,
        EN_NV_ID_TL_PA_GPIO_CTRL                       = 0xd530,
        EN_NV_APT_PDM_HOLD_NTX_STRU                    = 0xD531,
        /*mipi apt begin*/
        EN_NV_LPHY_MIPI_APT_PARA                       = 0xD525,

        /*mipi apt end*/

        EN_NV_LTE_DCXO_PPM_UPDATE_THRESHOLD_STRU       = 0xD523,
        EN_NV_LTE_DCXO_PPM_VAR_THRESHOLD_STRU          = 0xD526,

        /*begin: add for feature k3v3+tas 2015/02/25*/
        EN_NV_ID_TAS_GPIO_PARA                           = 0xD5d0,

        EN_NV_ID_TAS_DPDT_PROTECT_PARA                   = 0xD5d1,
        EN_NV_ID_LTE_TAS_BS_RSSI_THD_PARA                = 0xD5d2,
        EN_NV_ID_TAS_BLIND_SW_THD_PARA                   = 0xD5d3,
        EN_NV_ID_TAS_CLG_MODE_GPIO_MAP                   = 0xD5d4,
        EN_NV_ID_TAS_HAPPY_THD_PARA                      = 0xD5d5,
        EN_NV_ID_MAS_THR_PARA                            = 0xD5d6,
        /*end: add for feature tas 2015/02/25*/

        /*begin: add for band29 and band32 RX  2015/12/25*/
        EN_NV_LPHY_SINGLE_RX_BAND_CAL_PARA               = 0xD5d7,
        /*end: add for band29 and band32 RX  2015/12/25*/
        EN_NV_BT_DBB_REDUCE_STRU                        = 0xD5d8,
        EN_NV_ET_APT_MIPI_CMD_LAB_PARA                  = 0xD5D9,
        EN_NV_ET_SELECT_WORK_MODE_LAB_PARA              = 0xD5DA,
        //EN_NV_DPD_SELECT_WORK_MODE_LAB_PARA             = 0xD5DC,

        EN_NV_ID_TAS_ACCESS_THR_PARA                     = 0xD5db,
        EN_NV_LPHY_NS_DBB_REDUCE_PARA                    = 0xD5dc,
        EN_NV_ID_LTE_TAS_DPDT_MIPI_CTRL_WORD             = 0xD5dd,
        EN_NV_ID_LTE_TXTAS_PARA                          = 0xD5de,
        EN_NV_LPHY_NS_PA_BIAS_CONFIG                     = 0xD5df,

        EN_NV_ID_HI6360_AGC_PARA_B20                    = 0xD618,
        EN_NV_ID_AD9361_AGC_PARA_B20                    = 0xD619,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B20                = 0xD61A,

        EN_NV_ID_HI6360_AGC_PARA_B41                    = 0xD6d8,
        EN_NV_ID_AD9361_AGC_PARA_B41                    = 0xD6d9,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B41                = 0xD6da,

        EN_NV_ID_HI6360_AGC_PARA_B40                    = 0xD658,
        EN_NV_ID_AD9361_AGC_PARA_B40                    = 0xD659,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B40                = 0xD65A,
        EN_NV_ID_HI6360_AGC_PARA_B140                    = 0xE558,
        EN_NV_ID_AD9361_AGC_PARA_B140                    = 0xE559,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B140                = 0xE55A,

        EN_NV_ID_HI6360_AGC_PARA_B38                    = 0xD698,
        EN_NV_ID_AD9361_AGC_PARA_B38                    = 0xD699,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B38                = 0xD69A,

        EN_NV_ID_HI6360_AGC_PARA_B7                     = 0xD718,
        EN_NV_ID_AD9361_AGC_PARA_B7                     = 0xD719,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B7                 = 0xD71A,

        EN_NV_ID_HI6360_AGC_PARA_B3                     = 0xD817,
        EN_NV_ID_HI6360_AGC_PARA_B1                     = 0xD858,
        EN_NV_ID_HI6360_AGC_PARA_B5                     = 0xD898,
        EN_NV_ID_HI6360_AGC_PARA_B8                     = 0xD8d8,
        EN_NV_ID_HI6360_AGC_PARA_B28                    = 0xDf58,/**/
    /*BEGIN     modify for B28全频段特性*/
        EN_NV_ID_HI6360_AGC_PARA_B128                   = 0xDf98,
    /*END     modify for B28全频段特性*/

        EN_NV_ID_LPHY_DSP_VERSION_INFO                  = 0xD818,
        EN_NV_ID_LPHY_DSP_CONFIG_INFO                   = 0xD819,
        EN_NV_ID_MULTIMODE_DSP_COMMON_CONFIG_INFO       = 0xD81A,

        EN_NV_RX_BT_LEVEL_MAP_TABLE                     = 0xD3e1,

        EN_NV_ID_TDS_HIGH_SPEED_NV_PARA                = 0xD580,
        EN_NV_ID_TDS_TX_POWER_NV_PARA                  = 0xD581,
        #if defined(LPHY_ASIC_K3V6)
        EN_NV_ID_TDS_TEST_MODE_TX_POWER_NV_PARA        = 0xD582,
        #endif
        /* modify by   for 所有band begin*/

        /* BEGIN: Added by  , 2015/5/8   PN:V7R5_AMPR*/

        EN_NV_ID_LTE_TX_AMPR_BNon26                      = 0xf878,
        EN_NV_ID_LTE_TX_AMPR_BNon28                      = 0xf478,
        /*与band无关*/
        EN_NV_ID_LTE_TX_AMPR_NS07                        = 0xd751,
        EN_NV_ID_LTE_TX_AMPR_NS08                        = 0xd752,
        EN_NV_ID_LTE_TX_AMPR_NS10                        = 0xd753,
        EN_NV_ID_LTE_TX_AMPR_NS11                        = 0xd754,
        EN_NV_ID_LTE_TX_AMPR_NS16                        = 0xd755,
        EN_NV_ID_LTE_TX_AMPR_NS19                        = 0xd756,
        EN_NV_ID_LTE_TX_AMPR_NS20                        = 0xd757,
        EN_NV_ID_LTE_TX_AMPR_NS21                        = 0xd758,
        /* END:   Added by  , 2015/5/8 */

        EN_NV_ID_LTE_PA_TEMP_DET_CH_B20 = 0xf900,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B40 = 0xf901,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B38 = 0xf902,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B41 = 0xf903,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B7  = 0xf904,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B3  = 0xf905,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B1  = 0xf906,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B5  = 0xf907,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B8  = 0xf908,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B19 = 0xf909,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B21 = 0xf90a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B2  = 0xf90b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B4  = 0xf90c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B6  = 0xf90d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B9  = 0xf90e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B10 = 0xf90f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B11 = 0xf910,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B12 = 0xf911,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B13 = 0xf912,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B14 = 0xf913,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B17 = 0xf914,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B18 = 0xf915,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B22 = 0xf916,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B23 = 0xf917,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B24 = 0xf918,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B25 = 0xf919,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B26 = 0xf91a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B33 = 0xf91b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B34 = 0xf91c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B35 = 0xf91d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B36 = 0xf91e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B37 = 0xf91f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B42 = 0xf920,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B43 = 0xf921,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B39 = 0xf922,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B28 = 0xf923,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B128= 0xf924,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B27 = 0xf925,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B29 = 0xf926,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B30 = 0xf927,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B44 = 0xf928,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B32 = 0xf929,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon6 = 0xf92a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon7 = 0xf92b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon8 = 0xf92c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon9 = 0xf92d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon10 = 0xf92e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon11 = 0xf92f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon12 = 0xf930,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon13 = 0xf931,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon14 = 0xf932,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon15 = 0xf933,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon16 = 0xf934,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B140   = 0xf935,

        /* modify by   for 所有band end*/

        EN_NV_ID_FACTORY_END                            = 0x2fff
    };

#else
    enum NV_TLPHY_ITEM_ID_ENUM
    {
    #if 1/*def LPHY_NV_ALL_BAND*/

        /*begin:Add by   for k3v3+*/
            NV_BAND_FE_PARA_STRU_INDEX_000               = 0xD590,
            NV_BAND_FE_PARA_STRU_INDEX_001               = 0xD591,
            NV_BAND_FE_PARA_STRU_INDEX_002               = 0xD592,
            NV_BAND_FE_PARA_STRU_INDEX_003               = 0xD593,
            NV_BAND_FE_PARA_STRU_INDEX_004               = 0xD594,
            NV_BAND_FE_PARA_STRU_INDEX_005               = 0xD595,
            NV_BAND_FE_PARA_STRU_INDEX_006               = 0xD596,
            NV_BAND_FE_PARA_STRU_INDEX_007               = 0xD597,
            NV_BAND_FE_PARA_STRU_INDEX_008               = 0xD598,
            NV_BAND_FE_PARA_STRU_INDEX_009               = 0xD599,
            NV_BAND_FE_PARA_STRU_INDEX_010               = 0xD59A,
            NV_BAND_FE_PARA_STRU_INDEX_011               = 0xD59B,
            NV_BAND_FE_PARA_STRU_INDEX_012               = 0xD59C,
            NV_BAND_FE_PARA_STRU_INDEX_013               = 0xD59D,
            NV_BAND_FE_PARA_STRU_INDEX_014               = 0xD59E,
            NV_BAND_FE_PARA_STRU_INDEX_015               = 0xD59F,
            NV_BAND_FE_PARA_STRU_INDEX_016               = 0xD5A0,
            NV_BAND_FE_PARA_STRU_INDEX_017               = 0xD5A1,
            NV_BAND_FE_PARA_STRU_INDEX_018               = 0xD5A2,
            NV_BAND_FE_PARA_STRU_INDEX_019               = 0xD5A3,
            NV_BAND_FE_PARA_STRU_INDEX_020               = 0xD5A4,
            NV_BAND_FE_PARA_STRU_INDEX_021               = 0xD5A5,
            NV_BAND_FE_PARA_STRU_INDEX_022               = 0xD5A6,
            NV_BAND_FE_PARA_STRU_INDEX_023               = 0xD5A7,
            NV_BAND_FE_PARA_STRU_INDEX_024               = 0xD5A8,
            NV_BAND_FE_PARA_STRU_INDEX_025               = 0xD5A9,
            NV_BAND_FE_PARA_STRU_INDEX_026               = 0xD5AA,
            NV_BAND_FE_PARA_STRU_INDEX_027               = 0xD5AB,
            NV_BAND_FE_PARA_STRU_INDEX_028               = 0xD5AC,
            NV_BAND_FE_PARA_STRU_INDEX_029               = 0xD5AD,
            NV_BAND_FE_PARA_STRU_INDEX_030               = 0xD5AE,
            NV_BAND_FE_PARA_STRU_INDEX_031               = 0xD5AF,
        /*end:Add by   for k3v3+*/
    #endif

        /*begin: add for feature k3v3+tas 2015/02/25*/
        EN_NV_ID_TAS_GPIO_PARA                           = 0xD5d0,

        EN_NV_ID_TAS_DPDT_PROTECT_PARA                   = 0xD5d1,
        EN_NV_ID_LTE_TAS_BS_RSSI_THD_PARA                = 0xD5d2,
        EN_NV_ID_TAS_BLIND_SW_THD_PARA                   = 0xD5d3,
        EN_NV_ID_TAS_CLG_MODE_GPIO_MAP                   = 0xD5d4,
        EN_NV_ID_TAS_HAPPY_THD_PARA                      = 0xD5d5,
        /*end: add for feature tas 2015/02/25*/

        EN_NV_ID_MAS_THR_PARA                            = 0xD5d6,
        EN_NV_LPHY_SINGLE_RX_BAND_CAL_PARA               = 0xD5d7,

        EN_NV_LPHY_NS_DBB_REDUCE_PARA                    = 0xD5d9,
        EN_NV_LPHY_NS_PA_BIAS_CONFIG                     = 0xD5da,
        EN_NV_ID_TAS_ACCESS_THR_PARA                     = 0xD5db,
        EN_NV_ID_LTE_TAS_DPDT_MIPI_CTRL_WORD             = 0xD5dd,

        EN_NV_ID_AGC_PARA                               = 0xD3CD,
        EN_NV_ID_EMU_PARA                               = 0xD3CE,
        EN_NV_ID_RFE_CONFIG_TIME                        = 0xD3cf,

        EN_NV_ID_ADC_OPEN_TIME                          = 0xD400,
        EN_NV_ID_RFIC_T1                                = 0xD401,
        EN_NV_ID_RFIC_T2                                = 0xD402,
        EN_NV_ID_RFIC_T3                                = 0xD403,
        EN_NV_ID_RFIC_T4                                = 0xD404,
        EN_NV_ID_RFIC_T7                                = 0xD405,
        EN_NV_ID_RFIC_T8                                = 0xD406,
        EN_NV_ID_RFIC_T9                                = 0xD407,
        EN_NV_ID_RFIC_T10                               = 0xD408,
        EN_NV_ID_PA_OPEN_TIME                           = 0xD409,

        /* modify by   for 所有band begin*/

        EN_NV_ID_FTM_CAND_CELL_LIST_B19                 = 0xD900,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B19            = 0xD901,
        EN_NV_ID_LTE_TX_CAL_LIST_B19            = 0xD902,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B19          = 0xD903,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B19          = 0xD904,
        EN_NV_ID_TEMP_SENSOR_TABLE_B19                  = 0xD90f,
        EN_NV_ID_HI6360_AGC_PARA_B19                    = 0xD918,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B19        = 0xD91b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B19        = 0xD91c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B19                  = 0xD91d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B19                   = 0xD91e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B19              = 0xD91f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B19         = 0xD920,
        EN_NV_ID_LTE_TX_ATTEN_B19                       = 0xD921,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B19          = 0xD922,
        EN_NV_ID_TX_FILTER_CMP_STRU_B19          = 0xD923,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B19           = 0xD924,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B19           = 0xD925,

        EN_NV_ID_LTE_TX_APT_PARA_B19                    = 0xD926,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B19            = 0xD927,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B19             = 0xD928,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B19             = 0xD929,
        EN_NV_ID_LTE_TX_MPR_B19                         = 0xD92a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B19                     = 0xD92b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B19                        = 0xD92c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B19         = 0xD92d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B19                    = 0xD92e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B19                    = 0xD92f,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B19                       = 0xD930,
        //EN_NV_ID_TX_RF_BIAS_B19                         = 0xD931,
        EN_NV_ID_TX_PA_TEMP_COMP_B19                    = 0xD932,
        //EN_NV_ID_TX_ATTEN_TABLE_B19                     = 0xD933,
        //EN_NV_ID_POWERDET_VOLTAGE_B19                   = 0xD934,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B19                = 0xD935,
        EN_NV_ID_LTE_TX_PD_PARA_B19                      = 0xD936,
        EN_NV_ID_TX_ET_BAND_PARA_B19                      = 0xD937,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B19             = 0xD938,
        EN_NV_ID_LTE_DPD_LAB_STRU_B19                    = 0xD939,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B19 = 0xD93a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B19   = 0xD93b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B19            = 0xD93c,
        /*hrl*/
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B19              = 0xD93d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B19               = 0xD93e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B21                 = 0xD940,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B21            = 0xD941,
        EN_NV_ID_LTE_TX_CAL_LIST_B21            = 0xD942,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B21          = 0xD943,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B21          = 0xD944,
        EN_NV_ID_TEMP_SENSOR_TABLE_B21                  = 0xD94f,
        EN_NV_ID_HI6360_AGC_PARA_B21                    = 0xD958,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B21        = 0xD95b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B21        = 0xD95c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B21                  = 0xD95d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B21                   = 0xD95e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B21              = 0xD95f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B21         = 0xD960,
        EN_NV_ID_LTE_TX_ATTEN_B21                       = 0xD961,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B21          = 0xD962,
        EN_NV_ID_TX_FILTER_CMP_STRU_B21          = 0xD963,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B21           = 0xD964,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B21           = 0xD965,

        EN_NV_ID_LTE_TX_APT_PARA_B21                    = 0xD966,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B21            = 0xD967,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B21             = 0xD968,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B21             = 0xD969,
        EN_NV_ID_LTE_TX_MPR_B21                         = 0xD96a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B21                     = 0xD96b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B21                        = 0xD96c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B21         = 0xD96d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B21                   = 0xD96e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B21                   = 0xD96f,
        /*add for  * End*/
        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B21                       = 0xD970,
        //EN_NV_ID_TX_RF_BIAS_B21                         = 0xD971,
        EN_NV_ID_TX_PA_TEMP_COMP_B21                   = 0xD972,
        //EN_NV_ID_TX_ATTEN_TABLE_B21                        = 0xD973,
        //EN_NV_ID_POWERDET_VOLTAGE_B21                        = 0xD974,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B21               = 0xD975,
        EN_NV_ID_LTE_TX_PD_PARA_B21                     = 0xD976,
        EN_NV_ID_TX_ET_BAND_PARA_B21                     = 0xD977,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B21             = 0xD978,
        EN_NV_ID_LTE_DPD_LAB_STRU_B21                    = 0xD979,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B21 = 0xD97a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B21   = 0xD97b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B21            = 0xD97c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B21              = 0xD97D,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B21               = 0xD97e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B2                  = 0xD980,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B2             = 0xD981,
        EN_NV_ID_LTE_TX_CAL_LIST_B2                     = 0xD982,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B2           = 0xD983,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B2           = 0xD984,
        EN_NV_ID_TEMP_SENSOR_TABLE_B2                   = 0xD98f,
        EN_NV_ID_HI6360_AGC_PARA_B2                     = 0xD998,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B2         = 0xD99b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B2         = 0xD99c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B2                   = 0xD99d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B2                    = 0xD99e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B2               = 0xD99f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B2          = 0xD9a0,
        EN_NV_ID_LTE_TX_ATTEN_B2                        = 0xD9a1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B2           = 0xD9a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B2                  = 0xD9a3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B2            = 0xD9a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B2            = 0xD9a5,

        EN_NV_ID_LTE_TX_APT_PARA_B2                     = 0xD9a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B2             = 0xD9a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B2              = 0xD9a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B2              = 0xD9a9,
        EN_NV_ID_LTE_TX_MPR_B2                          = 0xD9aa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B2                      = 0xD9ab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B2                         = 0xD9ac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B2         = 0xD9ad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B2                   = 0xD9ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B2                   = 0xD9af,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B2                       = 0xD9b0,
        //EN_NV_ID_TX_RF_BIAS_B2                         = 0xD9b1,
        EN_NV_ID_TX_PA_TEMP_COMP_B2                   = 0xD9b2,
        //EN_NV_ID_TX_ATTEN_TABLE_B2                        = 0xD9b3,
        //EN_NV_ID_POWERDET_VOLTAGE_B2                        = 0xD9b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B2                = 0xD9b5,
        EN_NV_ID_LTE_TX_PD_PARA_B2                      = 0xD9b6,
        EN_NV_ID_TX_ET_BAND_PARA_B2                     = 0xD9b7,

        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B2             = 0xD9b8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B2                    = 0xD9b9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B2 = 0xD9ba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B2   = 0xD9bb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B2            = 0xD9bc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B2              = 0xD9bd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B2               = 0xD9be,
        EN_NV_ID_FTM_CAND_CELL_LIST_B4                  = 0xD9c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B4             = 0xD9c1,
        EN_NV_ID_LTE_TX_CAL_LIST_B4                     = 0xD9c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B4           = 0xD9c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT4_B4           = 0xD9c4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B4                   = 0xD9cf,
        EN_NV_ID_HI6360_AGC_PARA_B4                     = 0xD9d8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B4         = 0xD9db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT4_B4         = 0xD9dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B4                   = 0xD9dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B4                    = 0xD9de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B4               = 0xD9df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B4          = 0xD9e0,
        EN_NV_ID_LTE_TX_ATTEN_B4                        = 0xD9e1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B4         = 0xD9e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B4                  = 0xD9e3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B4          = 0xD9e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B4        = 0xD9e5,

        EN_NV_ID_LTE_TX_APT_PARA_B4                     = 0xD9e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B4             = 0xD9e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B4              = 0xD9e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B4              = 0xD9e9,
        EN_NV_ID_LTE_TX_MPR_B4                          = 0xD9ea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B4                      = 0xD9eb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B4                         = 0xD9ec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B4         = 0xD9ed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B4                   = 0xD9ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B4                   = 0xD9ef,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B4                      = 0xD9f0,
        //EN_NV_ID_TX_RF_BIAS_B4                        = 0xD9f1,
        EN_NV_ID_TX_PA_TEMP_COMP_B4                   = 0xD9f2,
        //EN_NV_ID_TX_ATTEN_TABLE_B4                      = 0xD9f3,
        //EN_NV_ID_POWERDET_VOLTAGE_B4                  = 0xD9f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B4                = 0xD9f5,
        EN_NV_ID_LTE_TX_PD_PARA_B4                      = 0xD9f6,
        EN_NV_ID_TX_ET_BAND_PARA_B4                     = 0xD9f7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B4             = 0xD9f8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B4                    = 0xD9f9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B4 = 0xD9fa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B4   = 0xD9fb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B4            = 0xD9fc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B4              = 0xD9fd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B4               = 0xD9fe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B6                  = 0xDa00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B6             = 0xDa01,
        EN_NV_ID_LTE_TX_CAL_LIST_B6             = 0xDa02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B6           = 0xDa03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT6_B6           = 0xDa04,
        EN_NV_ID_TEMP_SENSOR_TABLE_B6                   = 0xDa0f,
        EN_NV_ID_HI6360_AGC_PARA_B6                     = 0xDa18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B6         = 0xDa1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT6_B6         = 0xDa1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B6                   = 0xDa1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B6                    = 0xDa1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B6               = 0xDa1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B6          = 0xDa20,
        EN_NV_ID_LTE_TX_ATTEN_B6                        = 0xDa21,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B6           = 0xDa22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B6           = 0xDa23,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B6            = 0xDa24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B6            = 0xDa25,

        EN_NV_ID_LTE_TX_APT_PARA_B6                     = 0xDa26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B6             = 0xDa27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B6              = 0xDa28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B6              = 0xDa29,
        EN_NV_ID_LTE_TX_MPR_B6                          = 0xDa2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B6                      = 0xDa2b,
        /* add  tx end */

        /* add by  AMPR_REDUDTION begin */
        EN_NV_ID_LTE_TX_AMPR_B6                         = 0xDa2c,
        /* add by  AMPR_REDUDTION END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B6         = 0xDa2d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B6                   = 0xDa2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B6                   = 0xDa2f,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B6                      = 0xDa30,
        //EN_NV_ID_TX_RF_BIAS_B6                        = 0xDa31,
        EN_NV_ID_TX_PA_TEMP_COMP_B6                   = 0xDa32,
        //EN_NV_ID_TX_ATTEN_TABLE_B6                      = 0xDa33,
        //EN_NV_ID_POWERDET_VOLTAGE_B6                  = 0xDa34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B6                = 0xDa35,
        EN_NV_ID_LTE_TX_PD_PARA_B6                      = 0xDa36,
        EN_NV_ID_TX_ET_BAND_PARA_B6                     = 0xDa37,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B6             = 0xDa38,
        EN_NV_ID_LTE_DPD_LAB_STRU_B6                    = 0xDa39,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B6 = 0xDa3a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B6   = 0xDa3b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B6           = 0xDa3c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B6              = 0xDa3d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B6               = 0xDa3e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B9                  = 0xDa40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B9             = 0xDa41,
        EN_NV_ID_LTE_TX_CAL_LIST_B9                     = 0xDa42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B9           = 0xDa43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT9_B9           = 0xDa44,
        EN_NV_ID_TEMP_SENSOR_TABLE_B9                   = 0xDa4f,
        EN_NV_ID_HI6360_AGC_PARA_B9                     = 0xDa58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B9         = 0xDa5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT9_B9         = 0xDa5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B9                   = 0xDa5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B9                    = 0xDa5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B9               = 0xDa5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B9          = 0xDa60,
        EN_NV_ID_LTE_TX_ATTEN_B9                        = 0xDa61,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B9         = 0xDa62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B9                  = 0xDa63,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B9          = 0xDa64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B9        = 0xDa65,

        EN_NV_ID_LTE_TX_APT_PARA_B9                     = 0xDa66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B9             = 0xDa67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B9              = 0xDa68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B9              = 0xDa69,
        EN_NV_ID_LTE_TX_MPR_B9                          = 0xDa6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B9                      = 0xDa6b,
        /* add  tx end */

        /* add by  AMPR_REDUDTION begin */
        EN_NV_ID_LTE_TX_AMPR_B9                         = 0xDa6c,
        /* add by  AMPR_REDUDTION END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B9         = 0xDa6d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B9                   = 0xDa6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B9                   = 0xDa6f,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B9                      = 0xDa70,
        //EN_NV_ID_TX_RF_BIAS_B9                        = 0xDa71,
        EN_NV_ID_TX_PA_TEMP_COMP_B9                   = 0xDa72,
        //EN_NV_ID_TX_ATTEN_TABLE_B9                      = 0xDa73,
        //EN_NV_ID_POWERDET_VOLTAGE_B9                  = 0xDa74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B9                = 0xDa75,
        EN_NV_ID_LTE_TX_PD_PARA_B9                      = 0xDa76,
        EN_NV_ID_TX_ET_BAND_PARA_B9                     = 0xDa77,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B9             = 0xDa78,
        EN_NV_ID_LTE_DPD_LAB_STRU_B9                    = 0xDa79,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B9 = 0xDa7a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B9   = 0xDa7b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B9            = 0xDa7c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B9              = 0xDa7d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B9               = 0xDa7e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B10                 = 0xDa80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B10            = 0xDa81,
        EN_NV_ID_LTE_TX_CAL_LIST_B10                    = 0xDa82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B10          = 0xDa83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT10_B10         = 0xDa84,
        EN_NV_ID_TEMP_SENSOR_TABLE_B10                  = 0xDa8f,
        EN_NV_ID_HI6360_AGC_PARA_B10                    = 0xDa98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B10        = 0xDa9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT10_B10       = 0xDa9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B10                  = 0xDa9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B10                   = 0xDa9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B10              = 0xDa9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B10         = 0xDaa0,
        EN_NV_ID_LTE_TX_ATTEN_B10                       = 0xDaa1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B10        = 0xDaa2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B10                 = 0xDaa3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B10         = 0xDaa4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B10       = 0xDaa5,

        EN_NV_ID_LTE_TX_APT_PARA_B10                    = 0xDaa6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B10            = 0xDaa7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B10             = 0xDaa8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B10             = 0xDaa9,
        EN_NV_ID_LTE_TX_MPR_B10                         = 0xDaaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B10                     = 0xDaab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B10                        = 0xDaac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B10         = 0xDaad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B10                   = 0xDaae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B10                   = 0xDaaf,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B10                     = 0xDab0,
        //EN_NV_ID_TX_RF_BIAS_B10                       = 0xDab1,
        EN_NV_ID_TX_PA_TEMP_COMP_B10                   = 0xDab2,
        //EN_NV_ID_TX_ATTEN_TABLE_B10                     = 0xDab3,
        //EN_NV_ID_POWERDET_VOLTAGE_B10                 = 0xDab4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B10               = 0xDab5,
        EN_NV_ID_LTE_TX_PD_PARA_B10                     = 0xDab6,
        EN_NV_ID_TX_ET_BAND_PARA_B10                    = 0xDab7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B10             = 0xDab8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B10                    = 0xDab9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B10 = 0xDaba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B10   = 0xDabb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B10            = 0xDabc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B10              = 0xDabd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B10               = 0xDabe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B11                 = 0xDac0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B11            = 0xDac1,
        EN_NV_ID_LTE_TX_CAL_LIST_B11                    = 0xDac2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B11          = 0xDac3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT11_B11         = 0xDac4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B11                  = 0xDacf,
        EN_NV_ID_HI6360_AGC_PARA_B11                    = 0xDad8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B11        = 0xDadb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT11_B11       = 0xDadc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B11                  = 0xDadd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B11                   = 0xDade,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B11              = 0xDadf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B11         = 0xDae0,
        EN_NV_ID_LTE_TX_ATTEN_B11                       = 0xDae1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B11        = 0xDae2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B11                 = 0xDae3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B11         = 0xDae4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B11       = 0xDae5,

        EN_NV_ID_LTE_TX_APT_PARA_B11                    = 0xDae6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B11            = 0xDae7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B11             = 0xDae8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B11             = 0xDae9,
        EN_NV_ID_LTE_TX_MPR_B11                         = 0xDaea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B11                     = 0xDaeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B11                        = 0xDaec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B11         = 0xDaed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B11                   = 0xDaee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B11                   = 0xDaef,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B11                     = 0xDaf0,
        //EN_NV_ID_TX_RF_BIAS_B11                       = 0xDaf1,
        EN_NV_ID_TX_PA_TEMP_COMP_B11                   = 0xDaf2,
        //EN_NV_ID_TX_ATTEN_TABLE_B11                     = 0xDaf3,
        //EN_NV_ID_POWERDET_VOLTAGE_B11                 = 0xDaf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B11               = 0xDaf5,
        EN_NV_ID_LTE_TX_PD_PARA_B11                     = 0xDaf6,
        EN_NV_ID_TX_ET_BAND_PARA_B11                    = 0xDaf7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B11             = 0xDaf8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B11                    = 0xDaf9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B11 = 0xDafa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B11   = 0xDafb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B11            = 0xDafc,
            EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B11              = 0xDafd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B11               = 0xDafe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B12                 = 0xDb00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B12            = 0xDb01,
        EN_NV_ID_LTE_TX_CAL_LIST_B12                    = 0xDb02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B12          = 0xDb03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT12_B12         = 0xDb04,
        EN_NV_ID_TEMP_SENSOR_TABLE_B12                  = 0xDb0f,
        EN_NV_ID_HI6360_AGC_PARA_B12                    = 0xDb18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B12        = 0xDb1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT12_B12       = 0xDb1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B12                  = 0xDb1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B12                   = 0xDb1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B12              = 0xDb1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B12         = 0xDb20,
        EN_NV_ID_LTE_TX_ATTEN_B12                       = 0xDb21,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B12        = 0xDb22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B12                 = 0xDb23,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B12         = 0xDb24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B12       = 0xDb25,

        EN_NV_ID_LTE_TX_APT_PARA_B12                    = 0xDb26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B12            = 0xDb27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B12             = 0xDb28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B12             = 0xDb29,
        EN_NV_ID_LTE_TX_MPR_B12                         = 0xDb2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B12                     = 0xDb2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B12                        = 0xDb2c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B12         = 0xDb2d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B12                   = 0xDb2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B12                   = 0xDb2f,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B12                     = 0xDb30,
        //EN_NV_ID_TX_RF_BIAS_B12                       = 0xDb31,
        EN_NV_ID_TX_PA_TEMP_COMP_B12                   = 0xDb32,
        //EN_NV_ID_TX_ATTEN_TABLE_B12                     = 0xDb33,
        //EN_NV_ID_POWERDET_VOLTAGE_B12                 = 0xDb34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B12               = 0xDb35,
        EN_NV_ID_LTE_TX_PD_PARA_B12                     = 0xDb36,
        EN_NV_ID_TX_ET_BAND_PARA_B12                    = 0xDb37,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B12             = 0xDb38,
        EN_NV_ID_LTE_DPD_LAB_STRU_B12                    = 0xDb39,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B12 = 0xDb3a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B12   = 0xDb3b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B12            = 0xDb3c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B12              = 0xDb3d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B12               = 0xDb3e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B13                 = 0xDb40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B13            = 0xDb41,
        EN_NV_ID_LTE_TX_CAL_LIST_B13                    = 0xDb42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B13          = 0xDb43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT13_B13         = 0xDb44,
        EN_NV_ID_TEMP_SENSOR_TABLE_B13                  = 0xDb4f,
        EN_NV_ID_HI6360_AGC_PARA_B13                    = 0xDb58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B13        = 0xDb5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT13_B13       = 0xDb5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B13                  = 0xDb5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B13                  = 0xDb5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B13             = 0xDb5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B13         = 0xDb60,
        EN_NV_ID_LTE_TX_ATTEN_B13                       = 0xDb61,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B13        = 0xDb62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B13                 = 0xDb63,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B13         = 0xDb64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B13       = 0xDb65,

        EN_NV_ID_LTE_TX_APT_PARA_B13                    = 0xDb66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B13            = 0xDb67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B13             = 0xDb68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B13             = 0xDb69,
        EN_NV_ID_LTE_TX_MPR_B13                         = 0xDb6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B13                     = 0xDb6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B13                        = 0xDb6c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B13         = 0xDb6d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B13                   = 0xDb6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B13                   = 0xDb6f,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B13                     = 0xDb70,
        //EN_NV_ID_TX_RF_BIAS_B13                       = 0xDb71,
        EN_NV_ID_TX_PA_TEMP_COMP_B13                   = 0xDb72,
        //EN_NV_ID_TX_ATTEN_TABLE_B13                     = 0xDb73,
        //EN_NV_ID_POWERDET_VOLTAGE_B13                 = 0xDb74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B13               = 0xDb75,
        EN_NV_ID_LTE_TX_PD_PARA_B13                     = 0xDb76,
        EN_NV_ID_TX_ET_BAND_PARA_B13                    = 0xDb77,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B13             = 0xDb78,
        EN_NV_ID_LTE_DPD_LAB_STRU_B13                    = 0xDb79,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B13 = 0xDb7a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B13   = 0xDb7b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B13            = 0xDb7c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B13              = 0xDb7d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B13               = 0xDb7e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B14                 = 0xDb80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B14            = 0xDb81,
        EN_NV_ID_LTE_TX_CAL_LIST_B14                    = 0xDb82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B14          = 0xDb83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT14_B14         = 0xDb84,
        EN_NV_ID_TEMP_SENSOR_TABLE_B14                  = 0xDb8f,
        EN_NV_ID_HI6360_AGC_PARA_B14                    = 0xDb98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B14        = 0xDb9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT14_B14       = 0xDb9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B14                  = 0xDb9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B14                  = 0xDb9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B14             = 0xDb9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B14         = 0xDba0,
        EN_NV_ID_LTE_TX_ATTEN_B14                       = 0xDba1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B14        = 0xDba2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B14                 = 0xDba3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B14         = 0xDba4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B14       = 0xDba5,

        EN_NV_ID_LTE_TX_APT_PARA_B14                    = 0xDba6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B14            = 0xDba7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B14             = 0xDba8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B14             = 0xDba9,
        EN_NV_ID_LTE_TX_MPR_B14                         = 0xDbaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B14                     = 0xDbab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B14                        = 0xDbac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B14         = 0xDbad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B14                   = 0xDbae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B14                   = 0xDbaf,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B14                     = 0xDbb0,
        //EN_NV_ID_TX_RF_BIAS_B14                       = 0xDbb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B14                   = 0xDbb2,
        //EN_NV_ID_TX_ATTEN_TABLE_B14                     = 0xDbb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B14                 = 0xDbb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B14               = 0xDbb5,
        EN_NV_ID_LTE_TX_PD_PARA_B14                     = 0xDbb6,
        EN_NV_ID_TX_ET_BAND_PARA_B14                    = 0xDbb7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B14             = 0xDbb8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B14                    = 0xDbb9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B14 = 0xDbba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B14   = 0xDbbb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B14            = 0xDbbc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B14              = 0xDbbd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B14               = 0xDbbe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B17                 = 0xDbc0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B17            = 0xDbc1,
        EN_NV_ID_LTE_TX_CAL_LIST_B17                    = 0xDbc2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B17          = 0xDbc3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT17_B17         = 0xDbc4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B17                  = 0xDbcf,
        EN_NV_ID_HI6360_AGC_PARA_B17                    = 0xDbd8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B17        = 0xDbdb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT17_B17       = 0xDbdc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B17                  = 0xDbdd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B17                  = 0xDbde,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B17             = 0xDbdf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B17         = 0xDbe0,
        EN_NV_ID_LTE_TX_ATTEN_B17                       = 0xDbe1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B17        = 0xDbe2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B17                 = 0xDbe3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B17         = 0xDbe4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B17       = 0xDbe5,

        EN_NV_ID_LTE_TX_APT_PARA_B17                    = 0xDbe6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B17            = 0xDbe7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B17             = 0xDbe8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B17             = 0xDbe9,
        EN_NV_ID_LTE_TX_MPR_B17                         = 0xDbea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B17                     = 0xDbeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B17                        = 0xDbec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B17         = 0xDbed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B17                   = 0xDbee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B17                   = 0xDbef,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B17                     = 0xDbf0,
        //EN_NV_ID_TX_RF_BIAS_B17                       = 0xDbf1,
        EN_NV_ID_TX_PA_TEMP_COMP_B17                   = 0xDbf2,
        //EN_NV_ID_TX_ATTEN_TABLE_B17                        = 0xDbf3,
        //EN_NV_ID_POWERDET_VOLTAGE_B17                 = 0xDbf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B17               = 0xDbf5,
        EN_NV_ID_LTE_TX_PD_PARA_B17                     = 0xDbf6,
        EN_NV_ID_TX_ET_BAND_PARA_B17                    = 0xDbf7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B17             = 0xDbf8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B17                    = 0xDbf9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B17 = 0xDbfa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B17   = 0xDbfb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B17            = 0xDbfc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B17              = 0xDbfd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B17               = 0xDbfe,

       EN_NV_ID_FTM_CAND_CELL_LIST_B18                  = 0xDc00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B18             = 0xDc01,
        EN_NV_ID_LTE_TX_CAL_LIST_B18             = 0xDc02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B18           = 0xDc03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B18           = 0xDc05,
        EN_NV_ID_TEMP_SENSOR_TABLE_B18                   = 0xDc0f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B18         = 0xDc1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B18         = 0xDc1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B18                   = 0xDc1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B18                    = 0xDc1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B18               = 0xDc1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B18          = 0xDc20,
        EN_NV_ID_LTE_TX_ATTEN_B18                        = 0xDc21,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B18           = 0xDc22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B18           = 0xDc23,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B18            = 0xDc24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B18            = 0xDc25,
        EN_NV_ID_LTE_TX_APT_PARA_B18                     = 0xDc26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B18             = 0xDc27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B18              = 0xDc28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B18              = 0xDc29,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B18                          = 0xDc2a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B18                      = 0xDc2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B18                         = 0xDc2c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B18         = 0xDc2d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B18                   = 0xDc2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B18                   = 0xDc2f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B18                      = 0xDc30,
        //EN_NV_ID_TX_RF_BIAS_B18                        = 0xDc31,
        EN_NV_ID_TX_PA_TEMP_COMP_B18                   = 0xDc32,
        //EN_NV_ID_TX_ATTEN_TABLE_B18                    = 0xDc33,
        //EN_NV_ID_POWERDET_VOLTAGE_B18                  = 0xDc34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B18               = 0xDc35,
        EN_NV_ID_LTE_TX_PD_PARA_B18                      = 0xDc36,
        EN_NV_ID_TX_ET_BAND_PARA_B18                    = 0xDc37,

        /*ET_DPD相关的NV_ID号*/
        /*LAB相关*/
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B18             = 0xDc38,
        EN_NV_ID_LTE_DPD_LAB_STRU_B18                    = 0xDc39,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B18 = 0xDc3a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B18   = 0xDc3b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B18            = 0xDc3c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B18              = 0xDc3d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B18               = 0xDc3e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B22                 = 0xDc40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B22            = 0xDc41,
        EN_NV_ID_LTE_TX_CAL_LIST_B22                    = 0xDc42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B22          = 0xDc43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT22_B22         = 0xDc44,
        EN_NV_ID_TEMP_SENSOR_TABLE_B22                  = 0xDc4f,
        EN_NV_ID_HI6360_AGC_PARA_B22                    = 0xDc58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B22        = 0xDc5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT22_B22       = 0xDc5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B22                  = 0xDc5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B22                  = 0xDc5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B22             = 0xDc5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B22         = 0xDc60,
        EN_NV_ID_LTE_TX_ATTEN_B22                       = 0xDc61,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B22        = 0xDc62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B22                 = 0xDc63,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B22           = 0xDc64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B22           = 0xDc65,

        EN_NV_ID_LTE_TX_APT_PARA_B22                    = 0xDc66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B22            = 0xDc67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B22             = 0xDc68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B22             = 0xDc69,
        EN_NV_ID_LTE_TX_MPR_B22                         = 0xDc6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B22                     = 0xDc6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B22                        = 0xDc6c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B22         = 0xDc6d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B22                   = 0xDc6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B22                   = 0xDc6f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B22                       = 0xDc70,
        //EN_NV_ID_TX_RF_BIAS_B22                         = 0xDc71,
        EN_NV_ID_TX_PA_TEMP_COMP_B22                   = 0xDc72,
        //EN_NV_ID_TX_ATTEN_TABLE_B22                        = 0xDc73,
        //EN_NV_ID_POWERDET_VOLTAGE_B22                        = 0xDc74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B22               = 0xDc75,
        EN_NV_ID_LTE_TX_PD_PARA_B22                     = 0xDC36,
        EN_NV_ID_TX_ET_BAND_PARA_B22                    = 0xDc77,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B22             = 0xDc78,
        EN_NV_ID_LTE_DPD_LAB_STRU_B22                    = 0xDc79,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B22 = 0xDc7a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B22   = 0xDc7b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B22            = 0xDc7c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B22              = 0xDc7d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B22               = 0xDc7e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B23                 = 0xDc80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B23            = 0xDc81,
        EN_NV_ID_LTE_TX_CAL_LIST_B23            = 0xDc82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B23          = 0xDc83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT23_B23         = 0xDc84,
        EN_NV_ID_TEMP_SENSOR_TABLE_B23                  = 0xDc8f,
        EN_NV_ID_HI6360_AGC_PARA_B23                    = 0xDc98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B23        = 0xDc9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT23_B23       = 0xDc9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B23                  = 0xDc9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B23                   = 0xDc9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B23              = 0xDc9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B23         = 0xDca0,
        EN_NV_ID_LTE_TX_ATTEN_B23                       = 0xDca1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B23          = 0xDca2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B23          = 0xDca3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B23           = 0xDca4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B23           = 0xDca5,

        EN_NV_ID_LTE_TX_APT_PARA_B23                    = 0xDca6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B23            = 0xDca7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B23             = 0xDca8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B23             = 0xDca9,
        EN_NV_ID_LTE_TX_MPR_B23                         = 0xDcaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B23                     = 0xDcab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B23                        = 0xDcac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B23         = 0xDcad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B23                   = 0xDcae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B23                   = 0xDcaf,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B23                       = 0xDcb0,
        //EN_NV_ID_TX_RF_BIAS_B23                         = 0xDcb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B23                   = 0xDcb2,
        //EN_NV_ID_TX_ATTEN_TABLE_B23                        = 0xDcb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B23                        = 0xDcb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B23               = 0xDcb5,
        EN_NV_ID_LTE_TX_PD_PARA_B23                     = 0xDcb6,
        EN_NV_ID_TX_ET_BAND_PARA_B23                    = 0xDcb7,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B23             = 0xDcb8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B23                    = 0xDcb9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B23 = 0xDcba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B23   = 0xDcbb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B23            = 0xDcbc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B23              = 0xDcbd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B23               = 0xDcbe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B24                 = 0xDcc0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B24            = 0xDcc1,
        EN_NV_ID_LTE_TX_CAL_LIST_B24            = 0xDcc2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B24          = 0xDcc3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT24_B24         = 0xDcc4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B24                  = 0xDccf,
        EN_NV_ID_HI6360_AGC_PARA_B24                    = 0xDcd8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B24        = 0xDcdb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT24_B24       = 0xDcdc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B24                  = 0xDcdd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B24                   = 0xDcde,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B24              = 0xDcdf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B24         = 0xDce0,
        EN_NV_ID_LTE_TX_ATTEN_B24                       = 0xDce1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B24          = 0xDce2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B24          = 0xDce3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B24           = 0xDce4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B24           = 0xDce5,

        EN_NV_ID_LTE_TX_APT_PARA_B24                    = 0xDce6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B24            = 0xDce7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B24             = 0xDce8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B24             = 0xDce9,
        EN_NV_ID_LTE_TX_MPR_B24                         = 0xDcea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B24                     = 0xDceb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B24                        = 0xDcec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B24         = 0xDced,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B24                   = 0xDcee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B24                   = 0xDcef,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B24                       = 0xDcf0,
        //EN_NV_ID_TX_RF_BIAS_B24                         = 0xDcf1,
        EN_NV_ID_TX_PA_TEMP_COMP_B24                   = 0xDcf2,
        //EN_NV_ID_TX_ATTEN_TABLE_B24                        = 0xDcf3,
        //EN_NV_ID_POWERDET_VOLTAGE_B24                        = 0xDcf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B24               = 0xDcf5,
        EN_NV_ID_LTE_TX_PD_PARA_B24                     = 0xDcf6,
        EN_NV_ID_TX_ET_BAND_PARA_B24                    = 0xDcf7,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B24             = 0xDcf8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B24                    = 0xDcf9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B24 = 0xDcfa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B24   = 0xDcfb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B24            = 0xDcfc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B24              = 0xDcfd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B24               = 0xDcfe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B25                  = 0xDd00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B25             = 0xDd01,
        EN_NV_ID_LTE_TX_CAL_LIST_B25             = 0xDd02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B25           = 0xDd03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B25           = 0xDd04,
        EN_NV_ID_TEMP_SENSOR_TABLE_B25                   = 0xDd0f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B25         = 0xDd1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B25         = 0xDd1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B25                   = 0xDd1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B25                    = 0xDd1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B25               = 0xDd1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B25          = 0xDd20,
        EN_NV_ID_LTE_TX_ATTEN_B25                        = 0xDd21,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B25           = 0xDd22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B25           = 0xDd23,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B25            = 0xDd24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B25            = 0xDd25,
        EN_NV_ID_LTE_TX_APT_PARA_B25                     = 0xDd26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B25             = 0xDd27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B25              = 0xDd28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B25              = 0xDd29,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B25                          = 0xDd2a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B25                      = 0xDd2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B25                         = 0xDd2c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B25         = 0xDd2d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B25                   = 0xDd2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B25                   = 0xDd2f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B25                      = 0xDd30,
        //EN_NV_ID_TX_RF_BIAS_B25                        = 0xDd31,
        EN_NV_ID_TX_PA_TEMP_COMP_B25                   = 0xDd32,
        //EN_NV_ID_TX_ATTEN_TABLE_B25                    = 0xDd33,
        //EN_NV_ID_POWERDET_VOLTAGE_B25                  = 0xDd34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B25               = 0xDd35,
        EN_NV_ID_LTE_TX_PD_PARA_B25                      = 0xDd36,
        EN_NV_ID_TX_ET_BAND_PARA_B25                    = 0xDd37,

        /*ET_DPD相关的NV_ID号*/
        /*LAB相关*/
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B25             = 0xDd38,
        EN_NV_ID_LTE_DPD_LAB_STRU_B25                    = 0xDd39,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B25 = 0xDd3a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B25   = 0xDd3b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B25            = 0xDd3c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B25              = 0xDd3d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B25               = 0xDd3e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B26                  = 0xe340,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B26             = 0xe341,
        EN_NV_ID_LTE_TX_CAL_LIST_B26             = 0xe342,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B26           = 0xe343,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B26           = 0xe344,
        EN_NV_ID_TEMP_SENSOR_TABLE_B26                   = 0xe34f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B26         = 0xe35b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B26         = 0xe35c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B26                   = 0xe35d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B26                    = 0xe35e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B26               = 0xe35f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B26          = 0xe360,
        EN_NV_ID_LTE_TX_ATTEN_B26                        = 0xe361,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B26           =0xe362,
        EN_NV_ID_TX_FILTER_CMP_STRU_B26           = 0xe363,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B26            = 0xe364,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B26            = 0xe365,
        EN_NV_ID_LTE_TX_APT_PARA_B26                     = 0xe366,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B26             = 0xe367,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B26              = 0xe368,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B26              = 0xe369,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B26                          = 0xe36a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B26                      = 0xe36b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B26                         = 0xe36c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B26         = 0xe36d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B26                   = 0xe36e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B26                   = 0xe36f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B26                      =0xe370,
        //EN_NV_ID_TX_RF_BIAS_B26                        = 0xe371,
        EN_NV_ID_TX_PA_TEMP_COMP_B26                   = 0xe372,
        //EN_NV_ID_TX_ATTEN_TABLE_B26                    = 0xe373,
        //EN_NV_ID_POWERDET_VOLTAGE_B26                  = 0xe374,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B26               = 0xe375,
        EN_NV_ID_LTE_TX_PD_PARA_B26                      = 0xe376,
        EN_NV_ID_TX_ET_BAND_PARA_B26                    = 0xe377,

        /*ET_DPD相关的NV_ID号*/
        /*LAB相关*/
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B26             =0xe378,
        EN_NV_ID_LTE_DPD_LAB_STRU_B26                    = 0xe379,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B26 = 0xe37a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B26   = 0xe37b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B26            = 0xe37c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B26              = 0xe37d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B26               = 0xe37e,





        EN_NV_ID_FTM_CAND_CELL_LIST_B33                 = 0xDd40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B33            = 0xDd41,
        EN_NV_ID_LTE_TX_CAL_LIST_B33            = 0xDd42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B33          = 0xDd43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT33_B33         = 0xDd44,
        EN_NV_ID_TEMP_SENSOR_TABLE_B33                  = 0xDd4f,
        EN_NV_ID_HI6360_AGC_PARA_B33                    = 0xDd58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B33        = 0xDd5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT33_B33       = 0xDd5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B33                  = 0xDd5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B33                   = 0xDd5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B33              = 0xDd5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B33         = 0xDd60,
        EN_NV_ID_LTE_TX_ATTEN_B33                       = 0xDd61,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B33          = 0xDd62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B33          = 0xDd63,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B33           = 0xDd64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B33           = 0xDd65,

        EN_NV_ID_LTE_TX_APT_PARA_B33                    = 0xDd66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B33            = 0xDd67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B33             = 0xDd68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B33             = 0xDd69,
        EN_NV_ID_LTE_TX_MPR_B33                         = 0xDd6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B33                     = 0xDd6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B33                        = 0xDd6c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B33         = 0xDd6d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B33                   = 0xDd6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B33                   = 0xDd6f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B33                       = 0xDd70,
        //EN_NV_ID_TX_RF_BIAS_B33                         = 0xDd71,
        EN_NV_ID_TX_PA_TEMP_COMP_B33                   = 0xDd72,
        //EN_NV_ID_TX_ATTEN_TABLE_B33                        = 0xDd73,
        //EN_NV_ID_POWERDET_VOLTAGE_B33                        = 0xDd74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B33               = 0xDd75,
        EN_NV_ID_LTE_TX_PD_PARA_B33                     = 0xDd76,
        EN_NV_ID_TX_ET_BAND_PARA_B33                    = 0xDd77,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B33             = 0xDd78,
        EN_NV_ID_LTE_DPD_LAB_STRU_B33                    = 0xDd79,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B33 = 0xDd7a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B33   = 0xDd7b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B33            = 0xDd7c,
            EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B33              = 0xDd7d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B33               = 0xDd7e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B34                 = 0xDd80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B34            = 0xDd81,
        EN_NV_ID_LTE_TX_CAL_LIST_B34            = 0xDd82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B34          = 0xDd83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT34_B34         = 0xDd84,
        EN_NV_ID_TEMP_SENSOR_TABLE_B34                  = 0xDd8f,
        EN_NV_ID_HI6360_AGC_PARA_B34                    = 0xDd98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B34        = 0xDd9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT34_B34       = 0xDd9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B34                  = 0xDd9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B34                   = 0xDd9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B34              = 0xDd9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B34         = 0xDda0,
        EN_NV_ID_LTE_TX_ATTEN_B34                       = 0xDda1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B34          = 0xDda2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B34          = 0xDda3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B34           = 0xDda4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B34           = 0xDda5,

        EN_NV_ID_LTE_TX_APT_PARA_B34                    = 0xDda6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B34            = 0xDda7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B34             = 0xDda8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B34             = 0xDda9,
        EN_NV_ID_LTE_TX_MPR_B34                         = 0xDdaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B34                     = 0xDdab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B34                        = 0xDdac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B34         = 0xDdad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B34                   = 0xDdae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B34                   = 0xDdaf,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B34                       = 0xDdb0,
        //EN_NV_ID_TX_RF_BIAS_B34                         = 0xDdb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B34                   = 0xDdb2,
        //EN_NV_ID_TX_ATTEN_TABLE_B34                        = 0xDdb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B34                        = 0xDdb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B34               = 0xDdb5,
        EN_NV_ID_LTE_TX_PD_PARA_B34                     = 0xDdb6,
        EN_NV_ID_TX_ET_BAND_PARA_B34                    = 0xDdb7,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B34             = 0xDdb8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B34                    = 0xDdb9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B34 = 0xDdba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B34   = 0xDdbb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B34            = 0xDdbc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B34              = 0xDdbd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B34               = 0xDdbe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B35                 = 0xDdc0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B35            = 0xDdc1,
        EN_NV_ID_LTE_TX_CAL_LIST_B35            = 0xDdc2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B35          = 0xDdc3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT35_B35         = 0xDdc4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B35                  = 0xDdcf,
        EN_NV_ID_HI6360_AGC_PARA_B35                    = 0xDdd8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B35        = 0xDddb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT35_B35       = 0xDddc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B35                  = 0xDddd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B35                   = 0xDdde,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B35              = 0xDddf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B35         = 0xDde0,
        EN_NV_ID_LTE_TX_ATTEN_B35                       = 0xDde1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B35          = 0xDde2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B35          = 0xDde3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B35           = 0xDde4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B35           = 0xDde5,

        EN_NV_ID_LTE_TX_APT_PARA_B35                    = 0xDde6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B35            = 0xDde7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B35             = 0xDde8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B35             = 0xDde9,
        EN_NV_ID_LTE_TX_MPR_B35                         = 0xDdea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B35                     = 0xDdeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B35                        = 0xDdec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B35         = 0xDded,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B35                   = 0xDdee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B35                   = 0xDdef,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B35                       = 0xDdf0,
        //EN_NV_ID_TX_RF_BIAS_B35                         = 0xDdf1,
        EN_NV_ID_TX_PA_TEMP_COMP_B35                   = 0xDdf2,
        //EN_NV_ID_TX_ATTEN_TABLE_B35                        = 0xDdf3,
        //EN_NV_ID_POWERDET_VOLTAGE_B35                        = 0xDdf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B35               = 0xDdf5,
        EN_NV_ID_LTE_TX_PD_PARA_B35                     = 0xDdf6,
        EN_NV_ID_TX_ET_BAND_PARA_B35                    = 0xDdf7,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B35             = 0xDdf8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B35                    = 0xDdf9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B35 = 0xDdfa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B35   = 0xDdfb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B35            = 0xDdfc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B35              = 0xDdfd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B35               = 0xDdfe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B36                 = 0xDe00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B36            = 0xDe01,
        EN_NV_ID_LTE_TX_CAL_LIST_B36            = 0xDe02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B36          = 0xDe03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT36_B36         = 0xDe04,
        EN_NV_ID_TEMP_SENSOR_TABLE_B36                  = 0xDe0f,
        EN_NV_ID_HI6360_AGC_PARA_B36                    = 0xDe18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B36        = 0xDe1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT36_B36       = 0xDe1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B36                  = 0xDe1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B36                   = 0xDe1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B36              = 0xDe1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B36         = 0xDe20,
        EN_NV_ID_LTE_TX_ATTEN_B36                       = 0xDe21,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B36          = 0xDe22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B36          = 0xDe23,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B36           = 0xDe24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B36           = 0xDe25,

        EN_NV_ID_LTE_TX_APT_PARA_B36                    = 0xDe26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B36            = 0xDe27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B36             = 0xDe28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B36             = 0xDe29,
        EN_NV_ID_LTE_TX_MPR_B36                         = 0xDe2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B36                     = 0xDe2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B36                        = 0xDe2c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B36         = 0xDe2d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B36                   = 0xDe2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B36                   = 0xDe2f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B36                       = 0xDe30,
        //EN_NV_ID_TX_RF_BIAS_B36                         = 0xDe31,
        EN_NV_ID_TX_PA_TEMP_COMP_B36                   = 0xDe32,
        //EN_NV_ID_TX_ATTEN_TABLE_B36                        = 0xDe33,
        //EN_NV_ID_POWERDET_VOLTAGE_B36                        = 0xDe34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B36               = 0xDe35,
        EN_NV_ID_LTE_TX_PD_PARA_B36                     = 0xDe36,
        EN_NV_ID_TX_ET_BAND_PARA_B36                    = 0xDe37,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B36             = 0xDe38,
        EN_NV_ID_LTE_DPD_LAB_STRU_B36                    = 0xDe39,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B36 = 0xDe3a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B36   = 0xDe3b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B36            = 0xDe3c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B36              = 0xDe3d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B36               = 0xDe3e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B37                 = 0xDe40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B37            = 0xDe41,
        EN_NV_ID_LTE_TX_CAL_LIST_B37            = 0xDe42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B37          = 0xDe43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT37_B37         = 0xDe44,
        EN_NV_ID_TEMP_SENSOR_TABLE_B37                  = 0xDe4f,
        EN_NV_ID_HI6360_AGC_PARA_B37                    = 0xDe58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B37        = 0xDe5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT37_B37       = 0xDe5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B37                  = 0xDe5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B37                   = 0xDe5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B37              = 0xDe5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B37         = 0xDe60,
        EN_NV_ID_LTE_TX_ATTEN_B37                       = 0xDe61,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B37          = 0xDe62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B37          = 0xDe63,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B37           = 0xDe64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B37           = 0xDe65,

        EN_NV_ID_LTE_TX_APT_PARA_B37                    = 0xDe66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B37            = 0xDe67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B37             = 0xDe68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B37             = 0xDe69,
        EN_NV_ID_LTE_TX_MPR_B37                         = 0xDe6a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B37                     = 0xDe6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B37                        = 0xDe6c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B37         = 0xDe6d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B37                   = 0xDe6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B37                   = 0xDe6f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B37                       = 0xDe70,
        //EN_NV_ID_TX_RF_BIAS_B37                         = 0xDe71,
        EN_NV_ID_TX_PA_TEMP_COMP_B37                   = 0xDe72,
        //EN_NV_ID_TX_ATTEN_TABLE_B37                        = 0xDe73,
        //EN_NV_ID_POWERDET_VOLTAGE_B37                        = 0xDe74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B37               = 0xDe75,
        EN_NV_ID_LTE_TX_PD_PARA_B37                     = 0xDe76,
        EN_NV_ID_TX_ET_BAND_PARA_B37                    = 0xDe77,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B37             = 0xDe78,
        EN_NV_ID_LTE_DPD_LAB_STRU_B37                    = 0xDe79,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B37 = 0xDe7a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B37   = 0xDe7b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B37            = 0xDe7c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B37              = 0xDe7d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B37               = 0xDe7e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B42                 = 0xDe80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B42            = 0xDe81,
        EN_NV_ID_LTE_TX_CAL_LIST_B42                    = 0xDe82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B42          = 0xDe83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT42_B42         = 0xDe84,
        EN_NV_ID_TEMP_SENSOR_TABLE_B42                  = 0xDe8f,
        EN_NV_ID_HI6360_AGC_PARA_B42                    = 0xDe98,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B42        = 0xDe9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT42_B42       = 0xDe9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B42                  = 0xDe9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B42                   = 0xDe9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B42              = 0xDe9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B42         = 0xDea0,
        EN_NV_ID_LTE_TX_ATTEN_B42                       = 0xDea1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B42          = 0xDea2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B42                 = 0xDea3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B42           = 0xDea4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B42       = 0xDea5,

        EN_NV_ID_LTE_TX_APT_PARA_B42                    = 0xDea6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B42            = 0xDea7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B42             = 0xDea8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B42             = 0xDea9,
        EN_NV_ID_LTE_TX_MPR_B42                         = 0xDeaa,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B42                     = 0xDeab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B42                        = 0xDeac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B42         = 0xDead,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B42                   = 0xDeae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B42                   = 0xDeaf,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B42                       = 0xDeb0,
        //EN_NV_ID_TX_RF_BIAS_B42                         = 0xDeb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B42                   = 0xDeb2,
        //EN_NV_ID_TX_ATTEN_TABLE_B42                        = 0xDeb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B42                        = 0xDeb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B42               = 0xDeb5,
        EN_NV_ID_LTE_TX_PD_PARA_B42                     =0xDeb6,
        EN_NV_ID_TX_ET_BAND_PARA_B42                    = 0xDeb7,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B42             = 0xDeb8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B42                    = 0xDeb9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B42 = 0xDeba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B42   = 0xDebb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B42            = 0xDebc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B42              = 0xDebd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B42               = 0xDebe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B43                 = 0xDec0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B43            = 0xDec1,
        EN_NV_ID_LTE_TX_CAL_LIST_B43            = 0xDec2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B43          = 0xDec3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT43_B43         = 0xDec4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B43                  = 0xDecf,
        EN_NV_ID_HI6360_AGC_PARA_B43                    = 0xDed8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B43        = 0xDedb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT43_B43       = 0xDedc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B43                  = 0xDedd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B43                   = 0xDede,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B43              = 0xDedf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B43         = 0xDee0,
        EN_NV_ID_LTE_TX_ATTEN_B43                       = 0xDee1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B43          = 0xDee2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B43          = 0xDee3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B43           = 0xDee4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B43           = 0xDee5,

        EN_NV_ID_LTE_TX_APT_PARA_B43                    = 0xDee6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B43            = 0xDee7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B43             = 0xDee8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B43             = 0xDee9,
        EN_NV_ID_LTE_TX_MPR_B43                         = 0xDeea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B43                     = 0xDeeb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B43                        = 0xDeec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B43         = 0xDeed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B43                   = 0xDeee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B43                   = 0xDeef,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B43                       = 0xDef0,
        //EN_NV_ID_TX_RF_BIAS_B43                         = 0xDef1,
        EN_NV_ID_TX_PA_TEMP_COMP_B43                   = 0xDef2,
        //EN_NV_ID_TX_ATTEN_TABLE_B43                        = 0xDef3,
        //EN_NV_ID_POWERDET_VOLTAGE_B43                        = 0xDef4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B43               = 0xDef5,
        EN_NV_ID_LTE_TX_PD_PARA_B43                     = 0xDef6,
        EN_NV_ID_TX_ET_BAND_PARA_B43                     = 0xDef7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B43             = 0xDef8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B43                    = 0xDef9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B43 = 0xDefa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B43   = 0xDefb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B43            = 0xDefc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B43              = 0xDefd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B43               = 0xDefe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B39                 = 0xDf00,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B39            = 0xDf01,
        EN_NV_ID_LTE_TX_CAL_LIST_B39            = 0xDf02,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B39          = 0xDf03,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT36_B39         = 0xDf04,
        EN_NV_ID_TEMP_SENSOR_TABLE_B39                  = 0xDf0f,
        EN_NV_ID_HI6360_AGC_PARA_B39                    = 0xDf18,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B39        = 0xDf1b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT36_B39       = 0xDf1c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B39                  = 0xDf1d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B39                   = 0xDf1e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B39              = 0xDf1f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B39         = 0xDf20,
        EN_NV_ID_LTE_TX_ATTEN_B39                       = 0xDf21,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B39          = 0xDf22,
        EN_NV_ID_TX_FILTER_CMP_STRU_B39          = 0xDf23,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B39           = 0xDf24,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B39           = 0xDf25,

        EN_NV_ID_LTE_TX_APT_PARA_B39                    = 0xDf26,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B39            = 0xDf27,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B39             = 0xDf28,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B39             = 0xDf29,
        EN_NV_ID_LTE_TX_MPR_B39                         = 0xDf2a,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B39                     = 0xDf2b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B39                        = 0xDf2c,
        /* add by  AMPR  END */

        /* modify by   for 所有band end*/
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B39         = 0xDf2d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B39                   = 0xDf2e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B39                   = 0xDf2f,
        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B39                       = 0xDf30,
        //EN_NV_ID_TX_RF_BIAS_B39                         = 0xDf31,
        EN_NV_ID_TX_PA_TEMP_COMP_B39                   = 0xDf32,
        //EN_NV_ID_TX_ATTEN_TABLE_B39                        = 0xDf33,
        //EN_NV_ID_POWERDET_VOLTAGE_B39                        = 0xDf34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B39               = 0xDf35,
        EN_NV_ID_LTE_TX_PD_PARA_B39                     = 0xDf36,
        EN_NV_ID_TX_ET_BAND_PARA_B39                    = 0xDf37,

        /* modify by   for 所有band end*/
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B39             = 0xDf38,
        EN_NV_ID_LTE_DPD_LAB_STRU_B39                    = 0xDf39,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B39 = 0xDf3a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B39   = 0xDf3b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B39            = 0xDf3c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B39              = 0xDf3d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B39               = 0xDf3e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B20                 = 0xD600,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B20            = 0xD601,
        EN_NV_ID_LTE_TX_CAL_LIST_B20            = 0xD602,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B20          = 0xD603,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B20          = 0xD604,
        EN_NV_ID_TEMP_SENSOR_TABLE_B20                  = 0xD60f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B20        = 0xD61b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B20        = 0xD61c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B20                  = 0xD61d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B20                   = 0xD61e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B20              = 0xD61f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B20         = 0xD620,
        EN_NV_ID_LTE_TX_ATTEN_B20                       = 0xD621,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B20          = 0xD622,
        EN_NV_ID_TX_FILTER_CMP_STRU_B20          = 0xD623,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B20           = 0xD624,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B20           = 0xD625,
        EN_NV_ID_LTE_TX_APT_PARA_B20                    = 0xD626,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B20            = 0xD627,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B20             = 0xD628,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B20             = 0xD629,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B20                         = 0xD62a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B20                     = 0xD62b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B20                        = 0xD62c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B20         = 0xD62d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B20                   = 0xD62e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B20                   = 0xD62f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B20                       = 0xD630,
        //EN_NV_ID_TX_RF_BIAS_B20                         = 0xD631,
        EN_NV_ID_TX_PA_TEMP_COMP_B20                   = 0xD632,
        //EN_NV_ID_TX_ATTEN_TABLE_B20                        = 0xD633,
        //EN_NV_ID_POWERDET_VOLTAGE_B20                        = 0xD634,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B20               = 0xD635,
        EN_NV_ID_LTE_TX_PD_PARA_B20                     = 0xD636,
        EN_NV_ID_TX_ET_BAND_PARA_B20                    = 0xD637,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B20             = 0xD638,
        EN_NV_ID_LTE_DPD_LAB_STRU_B20                    = 0xD639,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B20 = 0xD63a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B20   = 0xD63b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B20            = 0xD63c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B20              = 0xD63d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B20               = 0xD63e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B40                 = 0xD640,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B40            = 0xD641,
        EN_NV_ID_LTE_TX_CAL_LIST_B40            = 0xD642,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B40          = 0xD643,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B40          = 0xD644,
        EN_NV_ID_TEMP_SENSOR_TABLE_B40                  = 0xD64f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B40        = 0xD65b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B40        = 0xD65c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B40                  = 0xD65d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B40                   = 0xD65e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B40              = 0xD65f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B40         = 0xD660,
        EN_NV_ID_LTE_TX_ATTEN_B40                       = 0xD661,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B40          = 0xD662,
        EN_NV_ID_TX_FILTER_CMP_STRU_B40          = 0xD663,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B40           = 0xD664,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B40           = 0xD665,
        EN_NV_ID_LTE_TX_APT_PARA_B40                    = 0xD666,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B40            = 0xD667,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B40             = 0xD668,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B40             = 0xD669,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B40                         = 0xD66a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B40                     = 0xD66b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B40                        = 0xD66c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B40         = 0xD66d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B40                   = 0xD66e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B40                   = 0xD66f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B40                       = 0xD670,
        //EN_NV_ID_TX_RF_BIAS_B40                         = 0xD671,
        EN_NV_ID_TX_PA_TEMP_COMP_B40                   = 0xD672,
        //EN_NV_ID_TX_ATTEN_TABLE_B40                        = 0xD673,
        //EN_NV_ID_POWERDET_VOLTAGE_B40                        = 0xD674,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B40               = 0xD675,
        EN_NV_ID_LTE_TX_PD_PARA_B40                     = 0xD676,
        EN_NV_ID_TX_ET_BAND_PARA_B40                    = 0xD677,
        /*add for K3V5 START 20141211*/
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B40             = 0xD678,
        EN_NV_ID_LTE_DPD_LAB_STRU_B40                    = 0xD679,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B40 = 0xD67a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B40   = 0xD67b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B40            = 0xD67c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B40              = 0xD67d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B40               = 0xD67e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B38                 = 0xD680,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B38            = 0xD681,
        EN_NV_ID_LTE_TX_CAL_LIST_B38            = 0xD682,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B38          = 0xD683,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B38          = 0xD684,
        EN_NV_ID_TEMP_SENSOR_TABLE_B38                  = 0xD68f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B38        = 0xD69b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B38        = 0xD69c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B38                  = 0xD69d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B38                   = 0xD69e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B38              = 0xD69f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B38         = 0xD6a0,
        EN_NV_ID_LTE_TX_ATTEN_B38                       = 0xD6a1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B38          = 0xD6a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B38          = 0xD6a3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B38           = 0xD6a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B38           = 0xD6a5,
        EN_NV_ID_LTE_TX_APT_PARA_B38                    = 0xD6a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B38            = 0xD6a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B38             = 0xD6a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B38             = 0xD6a9,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B38                         = 0xD6aa,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B38                     = 0xD6ab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B38                        = 0xD6ac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B38         = 0xD6ad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B38                   = 0xD6ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B38                   = 0xD6af,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B38                       = 0xD6b0,
        //EN_NV_ID_TX_RF_BIAS_B38                         = 0xD6b1,
        EN_NV_ID_TX_PA_TEMP_COMP_B38                   = 0xD6b2,
        //EN_NV_ID_TX_ATTEN_TABLE_B38                        = 0xD6b3,
        //EN_NV_ID_POWERDET_VOLTAGE_B38                        = 0xD6b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B38               = 0xD6b5,
        EN_NV_ID_LTE_TX_PD_PARA_B38                     = 0xD6b6,
        EN_NV_ID_TX_ET_BAND_PARA_B38                    = 0xD6b7,

        /*add for K3V5 START 20141211*/
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B38            = 0xD6b8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B38                   = 0xD6b9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B38 = 0xD6ba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B38  = 0xD6bb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B38           = 0xD6bc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B38             = 0xD6bd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B38              = 0xD6be,

        EN_NV_ID_FTM_CAND_CELL_LIST_B41                 = 0xD6c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B41            = 0xD6c1,
        EN_NV_ID_LTE_TX_CAL_LIST_B41            = 0xD6c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B41          = 0xD6c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B41          = 0xD6c4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B41                  = 0xD6cf,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B41        = 0xD6Db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B41        = 0xD6Dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B41                  = 0xD6Dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B41                   = 0xD6De,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B41              = 0xD6Df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B41         = 0xD6E0,
        EN_NV_ID_LTE_TX_ATTEN_B41                       = 0xD6E1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B41          = 0xD6E2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B41          = 0xD6E3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B41           = 0xD6E4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B41           = 0xD6E5,
        EN_NV_ID_LTE_TX_APT_PARA_B41                    = 0xD6E6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B41            = 0xD6E7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B41             = 0xD6E8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B41             = 0xD6E9,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B41                         = 0xD6ea,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B41                     = 0xD6eb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B41                        = 0xD6ec,
        /* add by  AMPR END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B41         = 0xD6ed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B41                   = 0xD6ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B41                   = 0xD6ef,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B41                       = 0xD6f0,
        //EN_NV_ID_TX_RF_BIAS_B41                         = 0xD6f1,
        EN_NV_ID_TX_PA_TEMP_COMP_B41                   = 0xD6f2,
        //EN_NV_ID_TX_ATTEN_TABLE_B41                    = 0xD6f3,
        //EN_NV_ID_POWERDET_VOLTAGE_B41                   = 0xD6f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B41               = 0xD6f5,
        EN_NV_ID_LTE_TX_PD_PARA_B41                     = 0xD6f6,
        EN_NV_ID_TX_ET_BAND_PARA_B41                    = 0xD6f7,
        /* add for K3V5 START  */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B41             = 0xD6f8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B41                    = 0xD6f9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B41 = 0xD6fa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B41   = 0xD6fb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B41            = 0xD6fc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B41              = 0xD6fd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B41               = 0xD6fe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B7                  = 0xD700,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B7             = 0xD701,
        EN_NV_ID_LTE_TX_CAL_LIST_B7             = 0xD702,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B7           = 0xD703,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B7           = 0xD704,
        EN_NV_ID_TEMP_SENSOR_TABLE_B7                   = 0xD70f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B7         = 0xD71b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B7         = 0xD71c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B7                   = 0xD71d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B7                    = 0xD71e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B7               = 0xD71f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B7          = 0xD720,
        EN_NV_ID_LTE_TX_ATTEN_B7                        = 0xD721,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B7           = 0xD722,
        EN_NV_ID_TX_FILTER_CMP_STRU_B7           = 0xD723,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B7            = 0xD724,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B7            = 0xD725,
        EN_NV_ID_LTE_TX_APT_PARA_B7                     = 0xD726,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B7             = 0xD727,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B7              = 0xD728,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B7              = 0xD729,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B7                          = 0xD72a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B7                      = 0xD72b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B7                         = 0xD72c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B7         = 0xD72d,

        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B7                   = 0xD72e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B7                   = 0xD72f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B7                      = 0xD730,
        //EN_NV_ID_TX_RF_BIAS_B7                        = 0xD731,
        EN_NV_ID_TX_PA_TEMP_COMP_B7                   = 0xD732,
        //EN_NV_ID_TX_ATTEN_TABLE_B7                    = 0xD733,
        //EN_NV_ID_POWERDET_VOLTAGE_B7                  = 0xD734,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B7               = 0xD735,
        EN_NV_ID_LTE_TX_PD_PARA_B7                      = 0xD736,
        EN_NV_ID_TX_ET_BAND_PARA_B7                    = 0xD737,

        /*ET_DPD相关的NV_ID号*/
        /*LAB相关*/
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B7             = 0xD738,
        EN_NV_ID_LTE_DPD_LAB_STRU_B7                    = 0xD739,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B7 = 0xD73a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B7   = 0xD73b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B7            = 0xD73c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B7              = 0xD73d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B7               = 0xD73e,



        EN_NV_ID_FTM_CAND_CELL_LIST_B3                  = 0xD800,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B3             = 0xD801,
        EN_NV_ID_LTE_TX_CAL_LIST_B3                     = 0xD802,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B3           = 0xD803,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B3           = 0xD804,
        EN_NV_ID_TEMP_SENSOR_TABLE_B3                   = 0xD80f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B3         = 0xD81b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B3         = 0xD81c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B3                   = 0xD81d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B3                    = 0xD81e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B3               = 0xD81f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B3          = 0xD820,
        EN_NV_ID_LTE_TX_ATTEN_B3                        = 0xD821,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B3           = 0xD822,
        EN_NV_ID_TX_FILTER_CMP_STRU_B3           = 0xD823,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B3            = 0xD824,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B3            = 0xD825,
        EN_NV_ID_LTE_TX_APT_PARA_B3                     = 0xD826,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B3             = 0xD827,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B3              = 0xD828,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B3              = 0xD829,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B3                          = 0xD82a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B3                      = 0xD82b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B3                         = 0xD82c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B3         = 0xD82d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B3                   = 0xD82e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B3                   = 0xD82f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B3                       = 0xD830,
        //EN_NV_ID_TX_RF_BIAS_B3                         = 0xD831,
        EN_NV_ID_TX_PA_TEMP_COMP_B3                   = 0xD832,
        //EN_NV_ID_TX_ATTEN_TABLE_B3                        = 0xD833,
        //EN_NV_ID_POWERDET_VOLTAGE_B3                        = 0xD834,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B3               = 0xD835,
        EN_NV_ID_LTE_TX_PD_PARA_B3                      = 0xD836,
        EN_NV_ID_TX_ET_BAND_PARA_B3                     = 0xD837,
        /*add for K3V5 START  */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B3             = 0xD838,
        EN_NV_ID_LTE_DPD_LAB_STRU_B3                    = 0xD839,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B3 = 0xD83a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B3   = 0xD83b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B3            = 0xD83c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B3              = 0xD83d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B3               = 0xD83e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B1                  = 0xD840,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B1             = 0xD841,
        EN_NV_ID_LTE_TX_CAL_LIST_B1             = 0xD842,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B1           = 0xD843,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B1           = 0xD844,
        EN_NV_ID_TEMP_SENSOR_TABLE_B1                   = 0xD84f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B1         = 0xD85b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B1         = 0xD85c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B1                   = 0xD85d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B1                    = 0xD85e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B1               = 0xD85f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B1          = 0xD860,
        EN_NV_ID_LTE_TX_ATTEN_B1                        = 0xD861,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B1           = 0xD862,
        EN_NV_ID_TX_FILTER_CMP_STRU_B1           = 0xD863,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B1            = 0xD864,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B1            = 0xD865,
        EN_NV_ID_LTE_TX_APT_PARA_B1                     = 0xD866,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B1             = 0xD867,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B1              = 0xD868,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B1              = 0xD869,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B1                          = 0xD86a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B1                      = 0xD86b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B1                         = 0xD86c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B1         = 0xD86d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B1                   = 0xD86e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B1                   = 0xD86f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B1                       = 0xD870,
        //EN_NV_ID_TX_RF_BIAS_B1                         = 0xD871,
        EN_NV_ID_TX_PA_TEMP_COMP_B1                   = 0xD872,
        //EN_NV_ID_TX_ATTEN_TABLE_B1                        = 0xD873,
        //EN_NV_ID_POWERDET_VOLTAGE_B1                        = 0xD874,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B1                = 0xD875,
        EN_NV_ID_LTE_TX_PD_PARA_B1                      = 0xD876,
        EN_NV_ID_TX_ET_BAND_PARA_B1                     = 0xD877,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B1             = 0xD878,
        EN_NV_ID_LTE_DPD_LAB_STRU_B1                    = 0xD879,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B1 = 0xD87a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B1   = 0xD87b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B1            = 0xD87c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B1              = 0xD87d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B1               = 0xD87e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B5                  = 0xD880,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B5             = 0xD881,
        EN_NV_ID_LTE_TX_CAL_LIST_B5             = 0xD882,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B5           = 0xD883,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B5           = 0xD884,
        EN_NV_ID_TEMP_SENSOR_TABLE_B5                   = 0xD88f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B5         = 0xD89b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B5         = 0xD89c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B5                   = 0xD89d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B5                    = 0xD89e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B5               = 0xD89f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B5          = 0xD8a0,
        EN_NV_ID_LTE_TX_ATTEN_B5                        = 0xD8a1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B5           = 0xD8a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B5           = 0xD8a3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B5            = 0xD8a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B5            = 0xD8a5,
        EN_NV_ID_LTE_TX_APT_PARA_B5                     = 0xD8a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B5             = 0xD8a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B5              = 0xD8a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B5              = 0xD8a9,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B5                          = 0xD8aa,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B5                      = 0xD8ab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B5                         = 0xD8ac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B5         = 0xD8ad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B5                   = 0xD8ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B5                   = 0xD8af,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B5                       = 0xD8b0,
        //EN_NV_ID_TX_RF_BIAS_B5                         = 0xD8b1,
        EN_NV_ID_TX_PA_TEMP_COMP_B5                   = 0xD8b2,
        //EN_NV_ID_TX_ATTEN_TABLE_B5                        = 0xD8b3,
        //EN_NV_ID_POWERDET_VOLTAGE_B5                        = 0xD8b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B5               = 0xD8b5,
        EN_NV_ID_LTE_TX_PD_PARA_B5                      = 0xD8b6,
        EN_NV_ID_TX_ET_BAND_PARA_B5                     = 0xD8b7,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B5             = 0xD8b8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B5                    = 0xD8b9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B5 = 0xD8ba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B5   = 0xD8bb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B5            = 0xD8bc,
            EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B5              = 0xD8bd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B5               = 0xD8be,

        EN_NV_ID_FTM_CAND_CELL_LIST_B8                  = 0xD8c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B8             = 0xD8c1,
        EN_NV_ID_LTE_TX_CAL_LIST_B8                     = 0xD8c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B8           = 0xD8c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B8           = 0xD8c4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B8                   = 0xD8cf,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B8         = 0xD8db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B8         = 0xD8dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B8                   = 0xD8dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B8                    = 0xD8de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B8               = 0xD8df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B8          = 0xD8e0,
        EN_NV_ID_LTE_TX_ATTEN_B8                        = 0xD8e1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B8           = 0xD8e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B8                  = 0xD8e3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B8            = 0xD8e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B8            = 0xD8e5,
        EN_NV_ID_LTE_TX_APT_PARA_B8                     = 0xD8e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B8             = 0xD8e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B8              = 0xD8e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B8              = 0xD8e9,

        /* modify  mpr begin */
        EN_NV_ID_LTE_TX_MPR_B8                          = 0xD8ea,
        /* modify  mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B8                      = 0xD8eb,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B8                         = 0xD8ec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B8         = 0xD8ed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B8                   = 0xD8ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B8                   = 0xD8ef,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B8                       = 0xD8f0,
        //EN_NV_ID_TX_RF_BIAS_B8                         = 0xD8f1,
        EN_NV_ID_TX_PA_TEMP_COMP_B8                   = 0xD8f2,
        //EN_NV_ID_TX_ATTEN_TABLE_B8                        = 0xD8f3,
        //EN_NV_ID_POWERDET_VOLTAGE_B8                        = 0xD8f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B8               = 0xD8f5,
        EN_NV_ID_LTE_TX_PD_PARA_B8                      = 0xD8f6,
        EN_NV_ID_TX_ET_BAND_PARA_B8                     = 0xD8f7,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B8             = 0xD8f8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B8                    = 0xD8f9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B8 = 0xD8fa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B8   = 0xD8fb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B8            = 0xD8fc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B8              = 0xD8fd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B8               = 0xD8fe,

    /*Band28 相关NV 项*/
        EN_NV_ID_FTM_CAND_CELL_LIST_B28                  = 0xDf40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B28             = 0xDf41,
        EN_NV_ID_LTE_TX_CAL_LIST_B28             = 0xDf42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B28           = 0xDf43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B28           = 0xDf44,
        EN_NV_ID_TEMP_SENSOR_TABLE_B28                   = 0xDf4f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B28         = 0xDf5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B28         = 0xDf5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B28                   = 0xDf5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B28                    = 0xDf5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B28               = 0xDf5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B28          = 0xDf60,
        EN_NV_ID_LTE_TX_ATTEN_B28                        = 0xDf61,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B28           = 0xDf62,
        EN_NV_ID_TX_FILTER_CMP_STRU_B28           = 0xDf63,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B28            = 0xDf64,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B28            = 0xDf65,
        EN_NV_ID_LTE_TX_APT_PARA_B28                     = 0xDf66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B28             = 0xDf67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B28             = 0xDf68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B28             = 0xDf69,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B28                          = 0xDf6a,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B28                      = 0xDf6b,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B28                         = 0xDf6c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B28         = 0xDf6d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B28                   = 0xDf6e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B28                   = 0xDf6f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B28                      = 0xDf70,
        //EN_NV_ID_TX_RF_BIAS_B28                         = 0xDf71,
        EN_NV_ID_TX_PA_TEMP_COMP_B28                   = 0xDf72,
        //EN_NV_ID_TX_ATTEN_TABLE_B28                        = 0xDf73,
        //EN_NV_ID_POWERDET_VOLTAGE_B28                        = 0xDf74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B28               = 0xDf75,
        EN_NV_ID_LTE_TX_PD_PARA_B28                     = 0xDf76,
        EN_NV_ID_TX_ET_BAND_PARA_B28                    = 0xDf77,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B28             = 0xDf78,
        EN_NV_ID_LTE_DPD_LAB_STRU_B28                    = 0xDf79,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B28 = 0xDf7a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B28   = 0xDf7b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B28            = 0xDf7c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B28              = 0xDf7d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B28               = 0xDf7e,
    /* 非标频段begin */
           /* EN_NV_ID_FTM_CAND_CELL_LIST_BNon1                 = 0xDf40,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon1            = 0xDf41,
        EN_NV_ID_LTE_TX_CALIBRATION_FREQ_BNon1            = 0xDf42,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon1          = 0xDf43,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT37_BNon1         = 0xDf44,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon1                  = 0xDf4f,
        EN_NV_ID_HI6360_AGC_PARA_BNon1                    = 0xDf58,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon1        = 0xDf5b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT37_BNon1       = 0xDf5c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon1                  = 0xDf5d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon1                   = 0xDf5e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon1              = 0xDf5f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon1         = 0xDf60,
        EN_NV_ID_LTE_TX_ATTEN_BNon1                       = 0xDf61,
        EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_BNon1          = 0xDf62,
        EN_NV_ID_LTE_TX_CAL_HIGHGAIN_POWER_BNon1          = 0xDf63,
        EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon1           = 0xDf64,
        EN_NV_ID_LTE_TX_CAL_LOWGAIN_POWER_BNon1           = 0xDf65,
        EN_NV_ID_LTE_TX_APT_PARA_BNon1                    = 0xDf66,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon1            = 0xDf67,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon1             = 0xDf68,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon1             = 0xDf69,
        EN_NV_ID_LTE_TX_MPR_BNon1                         = 0xDf6a,
        EN_NV_ID_LTE_ANT_SAR_BNon1                     = 0xDf6b,
    */
        /* add by  AMPR begin */
      //  EN_NV_ID_LTE_TX_AMPR_BNon1                        = 0xDf6c,
        /* add by  AMPR  END */
        /*add for  * begin*/
      //  EN_NV_ID_LTE_OTHER_COMP_BNon1         = 0xDf6d,
        /*add for  * end*/

        /*add for  * Begin*/
      //  EN_NV_ID_LTE_TX_AMPR_NS05_BNon1                   = 0xDf6e,
      //  EN_NV_ID_LTE_TX_AMPR_NS09_BNon1                   = 0xDf6f,

        /*add for V9R1_6361 Begin*/

       // EN_NV_ID_TX_RF_BB_ATT_BNon1                       = 0xDf70,
       // EN_NV_ID_TX_RF_BIAS_BNon1                         = 0xDf71,
       // EN_NV_ID_TX_PA_TEMP_COMP_BNon1                   = 0xDf72,
       // EN_NV_ID_TX_ATTEN_TABLE_BNon1                        = 0xDf73,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon1                        = 0xDf74,
        /*add for V9R1_6361 End*/
       // EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon1               = 0xDf75,
    /*BEGIN     modify for B28全频段特性*/
    /*Band128 相关NV项*/
        EN_NV_ID_FTM_CAND_CELL_LIST_B128                  = 0xDf80,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B128             = 0xDf81,
        EN_NV_ID_LTE_TX_CAL_LIST_B128             = 0xDf82,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B128           = 0xDf83,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B128           = 0xDf84,
        EN_NV_ID_TEMP_SENSOR_TABLE_B128                    = 0xDf8f,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B128         = 0xDf9b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B128         = 0xDf9c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B128                    = 0xDf9d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B128                     = 0xDf9e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B128                = 0xDf9f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B128          = 0xDfa0,
        EN_NV_ID_LTE_TX_ATTEN_B128                          = 0xDfa1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B128           = 0xDfa2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B128           = 0xDfa3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B128            = 0xDfa4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B128            = 0xDfa5,
        EN_NV_ID_LTE_TX_APT_PARA_B128                      = 0xDfa6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B128             = 0xDfa7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B128              = 0xDfa8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B128              = 0xDfa9,

        /* modify mpr begin */
        EN_NV_ID_LTE_TX_MPR_B128                           = 0xDfaa,
        /* modify mpr end */

        /* add  tx begin */
        EN_NV_ID_LTE_ANT_SAR_B128                          = 0xDfab,
        /* add  tx end */

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_B128                         = 0xDfac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_B128         = 0xDfad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_B128                    = 0xDfae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B128                    = 0xDfaf,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_B128                         = 0xDfb0,
        //EN_NV_ID_TX_RF_BIAS_B128                           = 0xDfb1,
        EN_NV_ID_TX_PA_TEMP_COMP_B128                      = 0xDfb2,
        //EN_NV_ID_TX_ATTEN_TABLE_B128                       = 0xDfb3,
        //EN_NV_ID_POWERDET_VOLTAGE_B128                     = 0xDfb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B128                = 0xDfb5,
        EN_NV_ID_LTE_TX_PD_PARA_B128                       = 0xDfb6,
        EN_NV_ID_TX_ET_BAND_PARA_B128                      = 0xDfb7,
        /*add for K3V5 START 20141211 ++  for B128 missing */
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B128             = 0xDfb8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B128                    = 0xDfb9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B128 = 0xDfba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B128   = 0xDfbb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B128            = 0xDfbc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B128              = 0xDfbd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B128               = 0xDfbe,

    /*END     modify for B28全频段特性*/
        /* modify by   for 所有band end*/

        EN_NV_ID_FTM_CAND_CELL_LIST_B27                    = 0xDfc0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B27               = 0xDfc1,
        EN_NV_ID_LTE_TX_CAL_LIST_B27                       = 0xDfc2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B27             = 0xDfc3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B27             = 0xDfc4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B27                     = 0xDfcf,
        EN_NV_ID_HI6360_AGC_PARA_B27                       = 0xDfd8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B27           = 0xDfdb,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B27           = 0xDfdc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B27                     = 0xDfdd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B27                      = 0xDfde,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B27                 = 0xDfdf,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B27            = 0xDfe0,
        EN_NV_ID_LTE_TX_ATTEN_B27                          = 0xDfe1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B27           = 0xDfe2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B27                    = 0xDfe3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B27            = 0xDfe4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B27          = 0xDfe5,
        EN_NV_ID_LTE_TX_APT_PARA_B27                       = 0xDfe6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B27               = 0xDfe7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B27                = 0xDfe8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B27                = 0xDfe9,
        EN_NV_ID_LTE_TX_MPR_B27                            = 0xDfea,
        EN_NV_ID_LTE_ANT_SAR_B27                           = 0xDfeb,
        EN_NV_ID_LTE_TX_AMPR_B27                           = 0xDfec,
        EN_NV_ID_LTE_OTHER_COMP_B27                        = 0xDfed,
        EN_NV_ID_LTE_TX_AMPR_NS05_B27                      = 0xDfee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B27                      = 0xDfef,
        //EN_NV_ID_TX_RF_BB_ATT_B27                        = 0xDff0,
        //EN_NV_ID_TX_RF_BIAS_B27                          = 0xDff1,
        EN_NV_ID_TX_PA_TEMP_COMP_B27                       = 0xDff2,
        //EN_NV_ID_TX_ATTEN_TABLE_B27                      = 0xDff3,
        //EN_NV_ID_POWERDET_VOLTAGE_B27                    = 0xDff4,
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B27                  = 0xDff5,
        EN_NV_ID_LTE_TX_PD_PARA_B27                        = 0xDff6,
        EN_NV_ID_TX_ET_BAND_PARA_B27                       = 0xDff7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B27               = 0xDff8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B27                      = 0xDff9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B27 = 0xDffa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B27     = 0xDffb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B27              = 0xDffc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B27                = 0xDffd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B27                 = 0xDffe,

        EN_NV_ID_FTM_CAND_CELL_LIST_B29                    = 0xe000,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B29               = 0xe001,
        EN_NV_ID_LTE_TX_CAL_LIST_B29                       = 0xe002,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B29             = 0xe003,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B29             = 0xe004,
        EN_NV_ID_TEMP_SENSOR_TABLE_B29                     = 0xe00f,
        EN_NV_ID_HI6360_AGC_PARA_B29                       = 0xe018,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B29           = 0xe01b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B29           = 0xe01c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B29                     = 0xe01d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B29                      = 0xe01e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B29                 = 0xe01f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B29            = 0xe020,
        EN_NV_ID_LTE_TX_ATTEN_B29                          = 0xe021,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B29           = 0xe022,
        EN_NV_ID_TX_FILTER_CMP_STRU_B29                    = 0xe023,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B29            = 0xe024,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B29          = 0xe025,
        EN_NV_ID_LTE_TX_APT_PARA_B29                       = 0xe026,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B29               = 0xe027,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B29                = 0xe028,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B29                = 0xe029,
        EN_NV_ID_LTE_TX_MPR_B29                            = 0xe02a,
        EN_NV_ID_LTE_ANT_SAR_B29                           = 0xe02b,
        EN_NV_ID_LTE_TX_AMPR_B29                           = 0xe02c,
        EN_NV_ID_LTE_OTHER_COMP_B29                        = 0xe02d,
        EN_NV_ID_LTE_TX_AMPR_NS05_B29                      = 0xe02e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B29                      = 0xe02f,
        //EN_NV_ID_TX_RF_BB_ATT_B29                        = 0xe030,
        //EN_NV_ID_TX_RF_BIAS_B29                          = 0xe031,
        EN_NV_ID_TX_PA_TEMP_COMP_B29                       = 0xe032,
        //EN_NV_ID_TX_ATTEN_TABLE_B29                      = 0xe033,
        //EN_NV_ID_POWERDET_VOLTAGE_B29                    = 0xe034,
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B29                  = 0xe035,
        EN_NV_ID_LTE_TX_PD_PARA_B29                        = 0xe036,
        EN_NV_ID_TX_ET_BAND_PARA_B29                       = 0xe037,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B29               = 0xe038,
        EN_NV_ID_LTE_DPD_LAB_STRU_B29                      = 0xe039,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B29 = 0xe03a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B29     = 0xe03b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B29              = 0xe03c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B29                = 0xe03d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B29                 = 0xe03e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B30                    = 0xe040,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B30               = 0xe041,
        EN_NV_ID_LTE_TX_CAL_LIST_B30                       = 0xe042,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B30             = 0xe043,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B30             = 0xe044,
        EN_NV_ID_TEMP_SENSOR_TABLE_B30                     = 0xe04f,
        EN_NV_ID_HI6360_AGC_PARA_B30                       = 0xe058,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B30           = 0xe05b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B30           = 0xe05c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B30                     = 0xe05d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B30                      = 0xe05e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B30                 = 0xe05f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B30            = 0xe060,
        EN_NV_ID_LTE_TX_ATTEN_B30                          = 0xe061,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B30           = 0xe062,
        EN_NV_ID_TX_FILTER_CMP_STRU_B30                    = 0xe063,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B30            = 0xe064,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B30          = 0xe065,
        EN_NV_ID_LTE_TX_APT_PARA_B30                       = 0xe066,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B30               = 0xe067,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B30                = 0xe068,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B30                = 0xe069,
        EN_NV_ID_LTE_TX_MPR_B30                            = 0xe06a,
        EN_NV_ID_LTE_ANT_SAR_B30                           = 0xe06b,
        EN_NV_ID_LTE_TX_AMPR_B30                           = 0xe06c,
        EN_NV_ID_LTE_OTHER_COMP_B30                        = 0xe06d,
        EN_NV_ID_LTE_TX_AMPR_NS05_B30                      = 0xe06e,
        EN_NV_ID_LTE_TX_AMPR_NS09_B30                      = 0xe06f,
        //EN_NV_ID_TX_RF_BB_ATT_B30                        = 0xe070,
        //EN_NV_ID_TX_RF_BIAS_B30                          = 0xe071,
        EN_NV_ID_TX_PA_TEMP_COMP_B30                       = 0xe072,
        //EN_NV_ID_TX_ATTEN_TABLE_B30                      = 0xe073,
        //EN_NV_ID_POWERDET_VOLTAGE_B30                    = 0xe074,
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B30                  = 0xe075,
        EN_NV_ID_LTE_TX_PD_PARA_B30                        = 0xe076,
        EN_NV_ID_TX_ET_BAND_PARA_B30                       = 0xe077,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B30               = 0xe078,
        EN_NV_ID_LTE_DPD_LAB_STRU_B30                      = 0xe079,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B30 = 0xe07a,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B30     = 0xe07b,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B30              = 0xe07c,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B30                = 0xe07d,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B30                 = 0xe07e,

        EN_NV_ID_FTM_CAND_CELL_LIST_B44                    = 0xe080,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B44               = 0xe081,
        EN_NV_ID_LTE_TX_CAL_LIST_B44                       = 0xe082,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B44             = 0xe083,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B44             = 0xe084,
        EN_NV_ID_TEMP_SENSOR_TABLE_B44                     = 0xe08f,
        EN_NV_ID_HI6360_AGC_PARA_B44                       = 0xe098,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B44           = 0xe09b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B44           = 0xe09c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B44                     = 0xe09d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B44                      = 0xe09e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B44                 = 0xe09f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B44            = 0xe0a0,
        EN_NV_ID_LTE_TX_ATTEN_B44                          = 0xe0a1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B44           = 0xe0a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B44                    = 0xe0a3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B44            = 0xe0a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B44          = 0xe0a5,
        EN_NV_ID_LTE_TX_APT_PARA_B44                       = 0xe0a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B44               = 0xe0a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B44                = 0xe0a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B44                = 0xe0a9,
        EN_NV_ID_LTE_TX_MPR_B44                            = 0xe0aa,
        EN_NV_ID_LTE_ANT_SAR_B44                           = 0xe0ab,
        EN_NV_ID_LTE_TX_AMPR_B44                           = 0xe0ac,
        EN_NV_ID_LTE_OTHER_COMP_B44                        = 0xe0ad,
        EN_NV_ID_LTE_TX_AMPR_NS05_B44                      = 0xe0ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_B44                      = 0xe0af,
        //EN_NV_ID_TX_RF_BB_ATT_B44                        = 0xe0b0,
        //EN_NV_ID_TX_RF_BIAS_B44                          = 0xe0b1,
        EN_NV_ID_TX_PA_TEMP_COMP_B44                       = 0xe0b2,
        //EN_NV_ID_TX_ATTEN_TABLE_B44                      = 0xe0b3,
        //EN_NV_ID_POWERDET_VOLTAGE_B44                    = 0xe0b4,
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B44                  = 0xe0b5,
        EN_NV_ID_LTE_TX_PD_PARA_B44                        = 0xe0b6,
        EN_NV_ID_TX_ET_BAND_PARA_B44                       = 0xe0b7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B44               = 0xe0b8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B44                      = 0xe0b9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B44 = 0xe0ba,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B44     = 0xe0bb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B44              = 0xe0bc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B44                = 0xe0bd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B44                 = 0xe0be,


        EN_NV_ID_FTM_CAND_CELL_LIST_B32                    = 0xe0c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_B32               = 0xe0c1,
        EN_NV_ID_LTE_TX_CAL_LIST_B32                       = 0xe0c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_B32             = 0xe0c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT2_B32             = 0xe0c4,
        EN_NV_ID_TEMP_SENSOR_TABLE_B32                     = 0xe0cf,
        EN_NV_ID_HI6360_AGC_PARA_B32                       = 0xe0d8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_B32           = 0xe0db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT2_B32           = 0xe0dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_B32                     = 0xe0dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_B32                      = 0xe0de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_B32                 = 0xe0df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_B32            = 0xe0e0,
        EN_NV_ID_LTE_TX_ATTEN_B32                          = 0xe0e1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_B32           = 0xe0e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_B32                    = 0xe0e3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_B32            = 0xe0e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_B32          = 0xe0e5,
        EN_NV_ID_LTE_TX_APT_PARA_B32                       = 0xe0e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_B32               = 0xe0e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_B32                = 0xe0e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_B32                = 0xe0e9,
        EN_NV_ID_LTE_TX_MPR_B32                            = 0xe0ea,
        EN_NV_ID_LTE_ANT_SAR_B32                           = 0xe0eb,
        EN_NV_ID_LTE_TX_AMPR_B32                           = 0xe0ec,
        EN_NV_ID_LTE_OTHER_COMP_B32                        = 0xe0ed,
        EN_NV_ID_LTE_TX_AMPR_NS05_B32                      = 0xe0ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_B32                      = 0xe0ef,
        //EN_NV_ID_TX_RF_BB_ATT_B32                        = 0xe0f0,
        //EN_NV_ID_TX_RF_BIAS_B32                          = 0xe0f1,
        EN_NV_ID_TX_PA_TEMP_COMP_B32                       = 0xe0f2,
        //EN_NV_ID_TX_ATTEN_TABLE_B32                      = 0xe0f3,
        //EN_NV_ID_POWERDET_VOLTAGE_B32                    = 0xe0f4,
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_B32                  = 0xe0f5,
        EN_NV_ID_LTE_TX_PD_PARA_B32                        = 0xe0f6,
        EN_NV_ID_TX_ET_BAND_PARA_B32                       = 0xe0f7,
        EN_NV_ID_LTE_ETM_ET_APT_LAB_STRU_B32               = 0xe0f8,
        EN_NV_ID_LTE_DPD_LAB_STRU_B32                      = 0xe0f9,
        EN_NV_ID_LTE_ETM_SEMI_STATIC_MIPI_CMD_LAB_STRU_B32 = 0xe0fa,
        EN_NV_ID_LTE_ETM_DYNAMIC_MIPI_CMD_LAB_STRU_B32     = 0xe0fb,
        EN_NV_ID_LTE_ET_DPD_COMP_LAB_STRU_B32              = 0xe0fc,
        EN_NV_ID_LTE_HRL_SPUR_INFO_STRU_B32                = 0xe0fd,
        EN_NV_ID_LTE_TAS2_DPDT_ANT_SAR_B32                 = 0xe0fe,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon8                 = 0xe100,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon8            = 0xe101,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon8            = 0xe102,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon8          = 0xe103,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT36_BNon8         = 0xe104,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon8                  = 0xe10f,
        EN_NV_ID_HI6360_AGC_PARA_BNon8                    = 0xe118,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon8        = 0xe11b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT36_BNon8       = 0xe11c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon8                  = 0xe11d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon8                   = 0xe11e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon8              = 0xe11f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon8         = 0xe120,
        EN_NV_ID_LTE_TX_ATTEN_BNon8                       = 0xe121,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_BNon8          = 0xe122,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon8          = 0xe123,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon8           = 0xe124,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon8           = 0xe125,

        EN_NV_ID_LTE_TX_APT_PARA_BNon8                    = 0xe126,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon8            = 0xe127,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon8             = 0xe128,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon8             = 0xe129,
        EN_NV_ID_LTE_TX_MPR_BNon8                         = 0xe12a,
        EN_NV_ID_LTE_ANT_SAR_BNon8                     = 0xe12b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon8                        = 0xe12c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon8         = 0xe12d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon8                   = 0xe12e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon8                   = 0xe12f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_BNon8                       = 0xe130,
        //EN_NV_ID_TX_RF_BIAS_BNon8                         = 0xe131,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon8                   = 0xe132,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon8                        = 0xe133,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon8                        = 0xe134,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon8               = 0xe135,

        /* 非标频段begin */
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon9                 = 0xe140,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon9            = 0xe141,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon9            = 0xe142,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon9          = 0xe143,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT37_BNon9         = 0xe144,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon9                  = 0xe14f,
        EN_NV_ID_HI6360_AGC_PARA_BNon9                    = 0xe158,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon9        = 0xe15b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT37_BNon9       = 0xe15c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon9                  = 0xe15d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon9                   = 0xe15e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon9              = 0xe15f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon9         = 0xe160,
        EN_NV_ID_LTE_TX_ATTEN_BNon9                       = 0xe161,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_BNon9          = 0xe162,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon9          = 0xe163,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon9           = 0xe164,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon9           = 0xe165,
        EN_NV_ID_LTE_TX_APT_PARA_BNon9                    = 0xe166,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon9            = 0xe167,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon9             = 0xe168,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon9             = 0xe169,
        EN_NV_ID_LTE_TX_MPR_BNon9                         = 0xe16a,
        EN_NV_ID_LTE_ANT_SAR_BNon9                        = 0xe16b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon9                        = 0xe16c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon9         = 0xe16d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon9                   = 0xe16e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon9                   = 0xe16f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_BNon9                       = 0xe170,
        //EN_NV_ID_TX_RF_BIAS_BNon9                         = 0xe171,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon9                   = 0xe172,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon9                        = 0xe173,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon9                        = 0xe174,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon9               = 0xe175,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon10                 = 0xe180,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon10            = 0xe181,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon10            = 0xe182,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon10          = 0xe183,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT42_BNon10         = 0xe184,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon10                  = 0xe18f,
        EN_NV_ID_HI6360_AGC_PARA_BNon10                    = 0xe198,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon10        = 0xe19b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT42_BNon10       = 0xe19c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon10                  = 0xe19d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon10                   = 0xe19e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon10              = 0xe19f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon10         = 0xe1a0,
        EN_NV_ID_LTE_TX_ATTEN_BNon10                       = 0xe1a1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_BNon10          = 0xe1a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon10          = 0xe1a3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon10           = 0xe1a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon10           = 0xe1a5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon10                    = 0xe1a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon10            = 0xe1a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon10             = 0xe1a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon10             = 0xe1a9,
        EN_NV_ID_LTE_TX_MPR_BNon10                         = 0xe1aa,
        EN_NV_ID_LTE_ANT_SAR_BNon10                     = 0xe1ab,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon10                        = 0xe1ac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon10         = 0xe1ad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon10                   = 0xe1ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon10                   = 0xe1af,
        /*add for  * End*/

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_BNon10                       = 0xe1b0,
        //EN_NV_ID_TX_RF_BIAS_BNon10                         = 0xe1b1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon10                   = 0xe1b2,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon10                        = 0xe1b3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon10                        = 0xe1b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon10               = 0xe1b5,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon11                 = 0xe1c0,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon11            = 0xe1c1,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon11            = 0xe1c2,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon11          = 0xe1c3,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT43_BNon11         = 0xe1c4,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon11                  = 0xe1cf,
        EN_NV_ID_HI6360_AGC_PARA_BNon11                    = 0xe1d8,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon11        = 0xe1db,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT43_BNon11       = 0xe1dc,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon11                  = 0xe1dd,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon11                   = 0xe1de,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon11              = 0xe1df,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon11         = 0xe1e0,
        EN_NV_ID_LTE_TX_ATTEN_BNon11                       = 0xe1e1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_BNon11          = 0xe1e2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon11          = 0xe1e3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon11           = 0xe1e4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon11           = 0xe1e5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon11                    = 0xe1e6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon11            = 0xe1e7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon11             = 0xe1e8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon11             = 0xe1e9,
        EN_NV_ID_LTE_TX_MPR_BNon11                         = 0xe1ea,
        EN_NV_ID_LTE_ANT_SAR_BNon11                     = 0xe1eb,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon11                        = 0xe1ec,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon11         = 0xe1ed,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon11                   = 0xe1ee,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon11                   = 0xe1ef,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_BNon11                       = 0xe1f0,
        //EN_NV_ID_TX_RF_BIAS_BNon11                         = 0xe1f1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon11                   = 0xe1f2,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon11                        = 0xe1f3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon11                        = 0xe1f4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon11               = 0xe1f5,
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon12                 = 0xe200,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon12            = 0xe201,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon12            = 0xe202,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon12          = 0xe203,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT36_BNon12         = 0xe204,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon12                  = 0xe20f,
        EN_NV_ID_HI6360_AGC_PARA_BNon12                    = 0xe218,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon12        = 0xe21b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT36_BNon12       = 0xe21c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon12                  = 0xe21d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon12                   = 0xe21e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon12              = 0xe21f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon12         = 0xe220,
        EN_NV_ID_LTE_TX_ATTEN_BNon12                       = 0xe221,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_BNon12          = 0xe222,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon12          = 0xe223,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon12           = 0xe224,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon12           = 0xe225,

        EN_NV_ID_LTE_TX_APT_PARA_BNon12                    = 0xe226,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon12            = 0xe227,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon12             = 0xe228,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon12             = 0xe229,
        EN_NV_ID_LTE_TX_MPR_BNon12                         = 0xe22a,
        EN_NV_ID_LTE_ANT_SAR_BNon12                     = 0xe22b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon12                        = 0xe22c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon12         = 0xe22d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon12                   = 0xe22e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon12                   = 0xe22f,


        //EN_NV_ID_TX_RF_BB_ATT_BNon12                       = 0xe230,
        //EN_NV_ID_TX_RF_BIAS_BNon12                         = 0xe231,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon12                   = 0xe232,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon12                        = 0xe233,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon12                        = 0xe234,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon12               = 0xe235,
        /* 非标频段begin */
        EN_NV_ID_FTM_CAND_CELL_LIST_BNon13                 = 0xe240,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon13            = 0xe241,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon13            = 0xe242,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon13          = 0xe243,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT37_BNon13         = 0xe244,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon13                  = 0xe24f,
        EN_NV_ID_HI6360_AGC_PARA_BNon13                    = 0xe258,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon13        = 0xe25b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT37_BNon13       = 0xe25c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon13                  = 0xe25d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon13                   = 0xe25e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon13              = 0xe25f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon13         = 0xe260,
        EN_NV_ID_LTE_TX_ATTEN_BNon13                       = 0xe261,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_BNon13          = 0xe262,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon13          = 0xe263,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon13           = 0xe264,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon13           = 0xe265,
        EN_NV_ID_LTE_TX_APT_PARA_BNon13                    = 0xe266,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon13            = 0xe267,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon13             = 0xe268,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon13             = 0xe269,
        EN_NV_ID_LTE_TX_MPR_BNon13                         = 0xe26a,
        EN_NV_ID_LTE_ANT_SAR_BNon13                     = 0xe26b,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon13                        = 0xe26c,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon13         = 0xe26d,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon13                   = 0xe26e,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon13                   = 0xe26f,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_BNon13                       = 0xe270,
        //EN_NV_ID_TX_RF_BIAS_BNon13                         = 0xe271,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon13                   = 0xe272,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon13                        = 0xe273,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon13                        = 0xe274,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon13               = 0xe275,

        EN_NV_ID_FTM_CAND_CELL_LIST_BNon14                 = 0xe280,
        EN_NV_ID_LTE_RX_CALIBRATION_FREQ_BNon14            = 0xe281,
        EN_NV_ID_LTE_TX_CAL_LIST_BNon14            = 0xe282,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT1_BNon14          = 0xe283,
        EN_NV_ID_LTE_RX_AGC_BLK_TABLE_ANT42_BNon14         = 0xe284,
        EN_NV_ID_TEMP_SENSOR_TABLE_BNon14                  = 0xe28f,
        EN_NV_ID_HI6360_AGC_PARA_BNon14                    = 0xe298,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT1_BNon14        = 0xe29b,
        EN_NV_ID_LTE_RX_AGC_NOBLK_TABLE_ANT42_BNon14       = 0xe29c,
        EN_NV_ID_LTE_AGC_TEMP_COMP_BNon14                  = 0xe29d,
        EN_NV_ID_LTE_IP2_CAL_CHAN_BNon14                   = 0xe29e,
        EN_NV_ID_LTE_IP2_CAL_THRESHOLD_BNon14              = 0xe29f,
        EN_NV_ID_LTE_TX_APC_GAIN_THRESHHOLD_BNon14         = 0xe2a0,
        EN_NV_ID_LTE_TX_ATTEN_BNon14                       = 0xe2a1,
        //EN_NV_ID_LTE_TX_APC_TEMP_COMP_STRU_BNon14          = 0xe2a2,
        EN_NV_ID_TX_FILTER_CMP_STRU_BNon14          = 0xe2a3,
        //EN_NV_ID_LTE_TX_CAL_MIDGAIN_POWER_BNon14           = 0xe2a4,
        EN_NV_ID_LTE_TX_CAL_RESULT_TABLE_STRU_BNon14           = 0xe2a5,
        EN_NV_ID_LTE_TX_APT_PARA_BNon14                    = 0xe2a6,
        EN_NV_ID_LTE_TX_APT_PDM_HIGH_TBL_BNon14            = 0xe2a7,
        EN_NV_ID_LTE_TX_APT_PDM_MID_TBL_BNon14             = 0xe2a8,
        EN_NV_ID_LTE_TX_APT_PDM_LOW_TBL_BNon14             = 0xe2a9,
        EN_NV_ID_LTE_TX_MPR_BNon14                         = 0xe2aa,
        EN_NV_ID_LTE_ANT_SAR_BNon14                     = 0xe2ab,

        /* add by  AMPR begin */
        EN_NV_ID_LTE_TX_AMPR_BNon14                        = 0xe2ac,
        /* add by  AMPR  END */
        /*add for  * begin*/
        EN_NV_ID_LTE_OTHER_COMP_BNon14         = 0xe2ad,
        /*add for  * end*/

        /*add for  * Begin*/
        EN_NV_ID_LTE_TX_AMPR_NS05_BNon14                   = 0xe2ae,
        EN_NV_ID_LTE_TX_AMPR_NS09_BNon14                   = 0xe2af,

        /*add for V9R1_6361 Begin*/

        //EN_NV_ID_TX_RF_BB_ATT_BNon14                       = 0xe2b0,
        //EN_NV_ID_TX_RF_BIAS_BNon14                         = 0xe2b1,
        EN_NV_ID_TX_PA_TEMP_COMP_BNon14                   = 0xe2b2,
        //EN_NV_ID_TX_ATTEN_TABLE_BNon14                        = 0xe2b3,
        //EN_NV_ID_POWERDET_VOLTAGE_BNon14                        = 0xe2b4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_TX_UL_ONE_RB_MPR_BNon14               = 0xe2b5,

        /* 非标频段end */
        EN_NV_ID_MODEM_END                              = 0xE4ff,
        /*Modem end 0xE4FF00*/

        EN_NV_ID_TCXO_DYN_CONFIG                        = 0x1401,

        EN_NV_ID_LTE_PA_TEMP_DET_CH_B20 = 0xf900,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B40 = 0xf901,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B38 = 0xf902,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B41 = 0xf903,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B7  = 0xf904,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B3  = 0xf905,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B1  = 0xf906,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B5  = 0xf907,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B8  = 0xf908,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B19 = 0xf909,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B21 = 0xf90a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B2  = 0xf90b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B4  = 0xf90c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B6  = 0xf90d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B9  = 0xf90e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B10 = 0xf90f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B11 = 0xf910,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B12 = 0xf911,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B13 = 0xf912,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B14 = 0xf913,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B17 = 0xf914,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B18 = 0xf915,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B22 = 0xf916,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B23 = 0xf917,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B24 = 0xf918,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B25 = 0xf919,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B26 = 0xf91a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B33 = 0xf91b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B34 = 0xf91c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B35 = 0xf91d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B36 = 0xf91e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B37 = 0xf91f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B42 = 0xf920,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B43 = 0xf921,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B39 = 0xf922,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B28 = 0xf923,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B128= 0xf924,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B27 = 0xf925,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B29 = 0xf926,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B30 = 0xf927,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B44 = 0xf928,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_B32 = 0xf929,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon6 = 0xf92a,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon7 = 0xf92b,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon8 = 0xf92c,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon9 = 0xf92d,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon10 = 0xf92e,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon11 = 0xf92f,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon12 = 0xf930,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon13 = 0xf931,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon14 = 0xf932,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon15 = 0xf933,
        EN_NV_ID_LTE_PA_TEMP_DET_CH_BNon16 = 0xf934,

        /*begin: add by   for DSP NV,<2012-6-11>*/
    #if 1
        EN_NV_ID_DSP2RF_CFG                             = 0xe400,
        EN_NV_ID_RF_ADVANCE_TIME                        = 0xe401,
        EN_NV_ID_RF_AGC_PARA_CODE                       = 0xe402,
        EN_NV_ID_RF_AGC_PARA_UPTHRGAIN                  = 0xe403,
        EN_NV_ID_RF_AGC_PARA_DOWNTHRGAIN                = 0xe404,
        EN_NV_ID_RF_AGC_PARA_TOTALGAIN                  = 0xe405,
        EN_NV_ID_RF_ACS_PARA                            = 0xe406,
        EN_NV_ID_AGC_CAL_PARA                           = 0xe407,
        EN_NV_ID_RF_APC_PARA                            = 0xe408,
        EN_NV_ID_MAXPOWER                               = 0xE4e0,
        EN_NV_ID_TX_APCOFF_REDUCE                       = 0xE4e1,
        EN_NV_ID_TX_CAL_FREQ_LIST_0                     = 0xE4e3,
        EN_NV_ID_TX_CAL_FREQ_LIST_1                     = 0xE4e9,
        EN_NV_ID_TX_CAL_FREQ_LIST_2                     = 0xE4ef,
        EN_NV_ID_RX_CAL_FREQ_LIST_0                     = 0xE4e4,
        EN_NV_ID_RX_CAL_FREQ_LIST_1                     = 0xE4ea,
        EN_NV_ID_RX_CAL_FREQ_LIST_2                     = 0xE4f0,
        EN_NV_ID_TEMP_SEMSOR_0                          = 0xE4e5,
        EN_NV_ID_TEMP_SEMSOR_1                          = 0xE4eb,
        EN_NV_ID_TEMP_SEMSOR_2                          = 0xE4f1,
        EN_NV_ID_TX_TEMP_COMP_0                         = 0xE4e6,
        EN_NV_ID_TX_TEMP_COMP_1                         = 0xE4ec,
        EN_NV_ID_TX_TEMP_COMP_2                         = 0xE4f2,
        EN_NV_ID_RX_TEMP_COMP_0                         = 0xE4e7,
        EN_NV_ID_RX_TEMP_COMP_1                         = 0xE4ed,
        EN_NV_ID_RX_TEMP_COMP_2                         = 0xE4f3,
        EN_NV_ID_PA_POWER_0                             = 0xF8f0,
        EN_NV_ID_PA_POWER_1                             = 0xF8f5,
        EN_NV_ID_PA_POWER_2                             = 0xF8fa,
        EN_NV_ID_TX_APC_COMP_0                          = 0xF8f1,
        EN_NV_ID_TX_APC_COMP_1                          = 0xF8f6,
        EN_NV_ID_TX_APC_COMP_2                          = 0xF8fb,
        EN_NV_ID_TX_APC_FREQ_COMP_0                     = 0xF8f2,
        EN_NV_ID_TX_APC_FREQ_COMP_1                     = 0xF8f7,
        EN_NV_ID_TX_APC_FREQ_COMP_2                     = 0xF8fc,
        EN_NV_ID_RX_AGC_COMP_0                          = 0xF8f3,
        EN_NV_ID_RX_AGC_COMP_1                          = 0xF8f8,
        EN_NV_ID_RX_AGC_COMP_2                          = 0xF8fd,
        EN_NV_ID_RX_AGC_FREQ_COMP_0                     = 0xF8f4,
        EN_NV_ID_RX_AGC_FREQ_COMP_1                     = 0xF8f9,
        EN_NV_ID_RX_AGC_FREQ_COMP_2                     = 0xF8fe,
        EN_NV_ID_TX_PA_LEVEL_THRE                     = 0xE409,
        EN_NV_ID_US_TCXO_INIT                         = 0xE900,
        EN_NV_ID_DRX_PARA                             = 0xE40a,
    #endif
        /*end: add by   for DSP NV,<2012-6-11>*/


        EN_TDS_NV_ID_TERMINAL_TO_DSP      = 0xE40b,
        EN_TDS_NV_LPHY_MIPI_APT           = 0xE40c,
        EN_TDS_NV_ETM_MIPI_PARA           = 0xE40d,



        /*begin: add by   for V9R1 DSPNV*/
        EN_NV_ID_TDS_RFIC_CFG_STRU_DEFAULT              = 0xe499,
        EN_NV_ID_TDS_LINECTRL_ALLOT_BY_HARDWARE_STRU_DEFAULT = 0xe49a,
        EN_NV_ID_TDS_RF_ADVANCE_TIME_STU_DEFAULT        = 0xe49b,
        EN_NV_ID_TDS_MIPI_CTRL_DEFAULT              = 0xe49c,

        /*  +tas 20140915 begin*/
        EN_NV_ID_TDS_TAS_STRU_DEFAULT                   = 0xe49d,
        EN_NV_ID_TDS_TAS_RF_STRU_DEFAULT                = 0xe49e,
        EN_NV_ID_TDS_TAS_SEARCH_STRU_DEFAULT            = 0xe49f,

        EN_NV_ID_TDS_TAS_EXTRA_MODEM_GPIO               = 0xe480,
        EN_NV_ID_TDS_TAS_DPDT_PROTECT_PARA              = 0xe481,

        EN_NV_ID_TDS_TAS_HAPPY_STRU_DEFAULT             = 0xe482,

        EN_NV_ID_TDS_TAS_BLIND_STRU_DEFAULT             = 0xe484,
        EN_NV_ID_TDS_MAS_PARA_STRU_DEFAULT              = 0xe485,
        EN_NV_ID_TL_MAS_GPIO_STRU_DEFAULT               = 0xe486,
        EN_NV_ID_TDS_TAS_DPDT_MIPI_CTRL_WORD            = 0xe487,
        /* +tas 20140915 end*/

        /*Band34*/
        /*modem nv*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B34             = 0xe4a0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B34               = 0xe4a1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B34                = 0xe4a2,
        //EN_NV_ID_TDS_TX_RF_BIAS_B34                   = 0xe4a3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_B34              = 0xe4a4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B34                  = 0xe4a5,
        EN_NV_ID_TDS_RX_CAL_FREQ_B34                    = 0xe4a6,
        EN_NV_ID_TDS_AGC_BAND_PARA_B34                  = 0xe4a7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B34                  = 0xe4a8,
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_B34                 = 0xe4a9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_B34                    = 0xe4aa,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_B34                   = 0xe4ab,
        EN_NV_ID_TDS_PA_TEMP_DET_CHANNEL_STRU_B34       = 0xe4ac,
        /*工厂nv*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B34           = 0xf8a0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B34         = 0xf8a1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B34    = 0xf8a2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B34                  = 0xf8a3,
        EN_NV_ID_TDS_DCOC_CAL_B34                       = 0xf8a4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B34                    = 0xf8a5,
        EN_NV_ID_TDS_APC_TABLE_STRU_B34                 = 0xf8a6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_B34                = 0xf8a7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_B34          = 0xf8a8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_B34          = 0xf8a9,
        /*Band39*/
        /*modem nv*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B39             = 0xe4b0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B39               = 0xe4b1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B39                = 0xe4b2,
        //EN_NV_ID_TDS_TX_RF_BIAS_B39                   = 0xe4b3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_B39              = 0xe4b4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B39                  = 0xe4b5,
        EN_NV_ID_TDS_RX_CAL_FREQ_B39                    = 0xe4b6,
        EN_NV_ID_TDS_AGC_BAND_PARA_B39                  = 0xe4b7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B39                  = 0xe4b8,
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_B39                 = 0xe4b9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_B39                    = 0xe4ba,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_B39                   = 0xe4bb,
        EN_NV_ID_TDS_PA_TEMP_DET_CHANNEL_STRU_B39       = 0xe4bc,
        /*工厂nv*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B39           = 0xf8b0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B39         = 0xf8b1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B39    = 0xf8b2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B39                  = 0xf8b3,
        EN_NV_ID_TDS_DCOC_CAL_B39                       = 0xf8b4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B39                    = 0xf8b5,
        EN_NV_ID_TDS_APC_TABLE_STRU_B39                     = 0xf8b6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_B39                = 0xf8b7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_B39              = 0xf8b8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_B39              = 0xf8b9,

        /*Band40*/
        /*modem nv*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_B40             = 0xe4c0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_B40                   = 0xe4c1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_B40                    = 0xe4c2,
        //EN_NV_ID_TDS_TX_RF_BIAS_B40                   = 0xe4c3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_B40                  = 0xe4c4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_B40                  = 0xe4c5,
        EN_NV_ID_TDS_RX_CAL_FREQ_B40                    = 0xe4c6,
        EN_NV_ID_TDS_AGC_BAND_PARA_B40                  = 0xe4c7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_B40                  = 0xe4c8,
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_B40                 = 0xe4c9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_B40                    = 0xe4ca,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_B40                   = 0xe4cb,
        /*工厂nv*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_B40                   = 0xf8c0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_B40             = 0xf8c1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_B40                = 0xf8c2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_B40                  = 0xf8c3,
        EN_NV_ID_TDS_DCOC_CAL_B40                       = 0xf8c4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_B40                    = 0xf8c5,
        EN_NV_ID_TDS_APC_TABLE_STRU_B40                     = 0xf8c6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_B40                = 0xf8c7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_B40              = 0xf8c8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_B40              = 0xf8c9,

        /*Band_Reserved*/
        /*modem nv*/
        EN_NV_ID_TDS_TEMPERATURE_SENSOR_BRESERVED       = 0xe4d0,
        EN_NV_ID_TDS_TX_CAL_LIST_STRU_BRESERVED                 = 0xe4d1,
        EN_NV_ID_TDS_TX_PA_TEMP_COMP_BRESERVED          = 0xe4d2,
        //EN_NV_ID_TDS_TX_RF_BIAS_BRESERVED                 = 0xe4d3,
        EN_NV_ID_TDS_TX_RF_BB_ATT_STRU_BRESERVED            = 0xe4d4,
        EN_NV_ID_TDS_PA_LEVEL_THRE_BRESERVED            = 0xe4d5,
        EN_NV_ID_TDS_RX_CAL_FREQ_BRESERVED              = 0xe4d6,
        EN_NV_ID_TDS_AGC_BAND_PARA_BRESERVED            = 0xe4d7,
        EN_NV_ID_TDS_AGC_TEMP_COMP_BRESERVED            = 0xe4d8,
        //EN_NV_ID_TDS_TX_RF_BB_MAX_ATT_BRESERVED       = 0xe4d9,
        //EN_NV_ID_TDS_TX_CAL_BB_ATT_BRESERVED          = 0xe4da,
        //EN_NV_ID_TDS_TX_PA_CAL_FREQ_BRESERVED         = 0xe4db,
        /*工厂nv*/
        EN_NV_ID_TDS_TX_RF_FREQ_COMP_STRU_BRESERVED             = 0xf8d0,
        EN_NV_ID_TDS_DEFAULT_POW_TABLE_STRU_BRESERVED       = 0xf8d1,
        EN_NV_ID_TDS_NV_TX_CAL_RESULT_TABLE_STRU_BRESERVED      = 0xf8d2,
        EN_NV_ID_TDS_AGC_FREQ_COMP_BRESERVED            = 0xf8d3,
        EN_NV_ID_TDS_DCOC_CAL_BRESERVED                 = 0xf8d4,
        EN_NV_ID_TDS_RF_TXIQ_CAL_BRESERVED              = 0xf8d5,
        EN_NV_ID_TDS_APC_TABLE_STRU_BRESERVED               = 0xf8d6,
        //EN_NV_ID_TDS_TX_CAL_PA_GAIN_BB_BRESERVED      = 0xf8d7,
        //EN_NV_ID_TDS_PA_MID_TX_FREQ_COMP_BRESERVED        = 0xf8a8,
        //EN_NV_ID_TDS_PA_LOW_TX_FREQ_COMP_BRESERVED        = 0xf8a9,

        /*end: add by   for V9R1 DSPNV*/

        EN_NV_ID_LTE_TCXO_INIT_FREQ                     = 0xe900,
        EN_NV_ID_TDS_NV_TUNER_PARA                      = 0xe901,
        EN_NV_ID_TDS_DRX_PARA                           = 0xe902,
        EN_NV_ID_TDS_VRAMP_PARA                         = 0xe903,

        EN_NV_ID_ANT_MODEM_LOSS_B20                     = 0xeb00,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B20         = 0xeb0d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B20         = 0xeb0e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B20       = 0xeb0f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B20       = 0xeb10,
        EN_NV_ID_LTE_IP2_CAL_B20                        = 0xeb11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B20      = 0xeb12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B20       = 0xeb13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B20       = 0xeb14,
        //EN_NV_ID_LTE_PA_POWER_B20                       = 0xeb15,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B20        = 0xeb16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B20         = 0xeb17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B20         = 0xeb18,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B20                    = 0xeb19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B20                  = 0xeb1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B20                  = 0xeb1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B20                       = 0xeb1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B20                       = 0xeb1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B20               = 0xeb1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B20              = 0xeb1f,



        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B20               = 0xeb25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B20              = 0xeb26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B20             = 0xeb27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B20          = 0xeb28,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B20   = 0xeb29,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B20   = 0xeb2a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B20  = 0xeb2b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B20  = 0xeb2c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B20                  = 0xeb2d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B20                  = 0xeb2e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B20                 = 0xeb2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B20                   = 0xeb30,
        EN_NV_ID_ET_FINEDLY_PARA_B20                    = 0xeb31,
        EN_NV_ID_ET_VOFFSET_GAIN_B20                    = 0xeb32,
        EN_NV_ID_ET_EVDELY_B20                           = 0xeb33,
        EN_NV_ID_RF_CA_RCCODE_B20                        = 0xeb34,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B20             = 0xeb35,
        EN_NV_ID_LTE_DPD_FAC_STRU_B20                    = 0xeb36,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B20            = 0xeb37,


        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B40                     = 0xeb40,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B40         = 0xeb4D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B40         = 0xeb4E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B40       = 0xeb4F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B40       = 0xeb50,
        EN_NV_ID_LTE_IP2_CAL_B40                        = 0xeb51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B40      = 0xeb52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B40       = 0xeb53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B40       = 0xeb54,
        //EN_NV_ID_LTE_PA_POWER_B40                       = 0xeb55,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B40        = 0xeb56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B40         = 0xeb57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B40         = 0xeb58,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B40                    = 0xeb59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B40                  = 0xeb5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B40                  = 0xeb5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B40                       = 0xeb5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B40                       = 0xeb5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B40               = 0xeb5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B40              = 0xeb5f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B40               = 0xeb65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B40              = 0xeb66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B40             = 0xeb67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B40          = 0xeb68,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B40   = 0xeb69,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B40   = 0xeb6a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B40  = 0xeb6b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B40  = 0xeb6c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B40                  = 0xeb6d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B40                  = 0xeb6e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B40                 = 0xeb6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B40                   = 0xeb70,
        EN_NV_ID_ET_FINEDLY_PARA_B40                    = 0xeb71,
        EN_NV_ID_ET_VOFFSET_GAIN_B40                    = 0xeb72,
        EN_NV_ID_ET_EVDELY_B40                           = 0xeb73,
        EN_NV_ID_RF_CA_RCCODE_B40                        = 0xeb74,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B40             = 0xeb75,
        EN_NV_ID_LTE_DPD_FAC_STRU_B40                    = 0xeb76,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B40            = 0xeb77,

        EN_NV_ID_ANT_MODEM_LOSS_B38                     = 0xeb80,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B38         = 0xeb8d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B38         = 0xeb8e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B38       = 0xeb8f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B38       = 0xeb90,
        EN_NV_ID_LTE_IP2_CAL_B38                        = 0xeb91,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B38      = 0xeb92,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B38       = 0xeb93,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B38       = 0xeb94,
        //EN_NV_ID_LTE_PA_POWER_B38                       = 0xeb95,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B38        = 0xeb96,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B38         = 0xeb97,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B38         = 0xeb98,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B38                    = 0xeb99,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B38                  = 0xeb9a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B38                  = 0xeb9b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B38                       = 0xeb9c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B38                       = 0xeb9d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B38               = 0xeb9e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B38              = 0xeb9f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B38                = 0xeba5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B38               = 0xeba6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B38              = 0xeba7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B38           = 0xeba8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B38         = 0xeba9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B38         = 0xebaa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B38       = 0xebab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B38       = 0xebac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B38                  = 0xebad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B38                  = 0xebae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B38                 = 0xebaf,
        EN_NV_ID_LTE_ET_GAIN_COMP_B38                   = 0xebb0,
        EN_NV_ID_ET_FINEDLY_PARA_B38                    = 0xebb1,
        EN_NV_ID_ET_VOFFSET_GAIN_B38                    = 0xebb2,
        EN_NV_ID_ET_EVDELY_B38                           = 0xebb3,
        EN_NV_ID_RF_CA_RCCODE_B38                        = 0xebb4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B38            = 0xebb5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B38                   = 0xebb6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B38           = 0xebb7,

        EN_NV_ID_ANT_MODEM_LOSS_B41                     = 0xebc0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B41         = 0xebcd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B41         = 0xebce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B41       = 0xebcf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B41       = 0xebd0,
        EN_NV_ID_LTE_IP2_CAL_B41                        = 0xebd1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B41      = 0xebd2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B41       = 0xebd3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B41       = 0xebd4,
        //EN_NV_ID_LTE_PA_POWER_B41                       = 0xebd5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B41        = 0xebd6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B41         = 0xebd7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B41         = 0xebd8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B41                    = 0xebd9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B41                  = 0xebda,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B41                  = 0xebdb,
        EN_NV_ID_LTE_APC_TABLE_STRU_B41                       = 0xebdc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B41                       = 0xebdd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B41               = 0xebde,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B41              = 0xebdf,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B41                = 0xebe5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B41               = 0xebe6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B41              = 0xebe7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B41           = 0xebe8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B41    = 0xebe9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B41    = 0xebea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B41  = 0xebeb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B41  = 0xebec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B41                  = 0xebed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B41                  = 0xebee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B41                 = 0xebef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B41                   = 0xebf0,
        EN_NV_ID_ET_FINEDLY_PARA_B41                    = 0xebf1,
        EN_NV_ID_ET_VOFFSET_GAIN_B41                    = 0xebf2,
        EN_NV_ID_ET_EVDELY_B41                           = 0xebf3,
        EN_NV_ID_RF_CA_RCCODE_B41                        = 0xebf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B41             = 0xebf5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B41                    = 0xebf6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B41            = 0xebf7,

        EN_NV_ID_ANT_MODEM_LOSS_B7                      = 0xec00,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B7          = 0xec0D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B7          = 0xec0E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B7        = 0xec0F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B7        = 0xec10,
        EN_NV_ID_LTE_IP2_CAL_B7                         = 0xec11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B7       = 0xec12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B7        = 0xec13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B7        = 0xec14,
        //EN_NV_ID_LTE_PA_POWER_B7                        = 0xec15,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B7         = 0xec16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B7          = 0xec17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B7          = 0xec18,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B7                    = 0xec19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B7                  = 0xec1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B7                  = 0xec1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B7                       = 0xec1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B7                       = 0xec1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B7               = 0xec1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B7              = 0xec1f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B7                = 0xec25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B7               = 0xec26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B7              = 0xec27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B7           = 0xec28,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B7    = 0xec29,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B7    = 0xec2a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B7  = 0xec2b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B7  = 0xec2c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B7                  = 0xec2d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B7                  = 0xec2e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B7                 = 0xec2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B7                   = 0xec30,
        EN_NV_ID_ET_FINEDLY_PARA_B7                    = 0xec31,
        EN_NV_ID_ET_VOFFSET_GAIN_B7                    = 0xec32,
        EN_NV_ID_ET_EVDELY_B7                           = 0xec33,
        EN_NV_ID_RF_CA_RCCODE_B7                        = 0xec34,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B7             = 0xec35,
        EN_NV_ID_LTE_DPD_FAC_STRU_B7                    = 0xec36,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B7            = 0xec37,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_B3                      = 0xed00,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B3          = 0xed0D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B3          = 0xed0E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B3        = 0xed0F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B3        = 0xed10,
        EN_NV_ID_LTE_IP2_CAL_B3                         = 0xed11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B3       = 0xed12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B3        = 0xed13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B3        = 0xed14,
        //EN_NV_ID_LTE_PA_POWER_B3                        = 0xed15,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B3         = 0xed16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B3          = 0xed17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B3          = 0xed18,
         /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B3                    = 0xed19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B3                  = 0xed1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B3                  = 0xed1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B3                       = 0xed1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B3                       = 0xed1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B3               = 0xed1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B3              = 0xed1f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B3                = 0xed25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B3               = 0xed26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B3              = 0xed27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B3           = 0xed28,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B3    = 0xed29,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B3    = 0xed2a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B3  = 0xed2b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B3  = 0xed2c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B3                  = 0xed2d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B3                  = 0xed2e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B3                 = 0xed2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B3                    = 0xed30,
        EN_NV_ID_ET_FINEDLY_PARA_B3                     = 0xed31,
        EN_NV_ID_ET_VOFFSET_GAIN_B3                     = 0xed32,
        EN_NV_ID_ET_EVDELY_B3                           = 0xed33,
        EN_NV_ID_RF_CA_RCCODE_B3                        = 0xed34,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B3             = 0xed35,
        EN_NV_ID_LTE_DPD_FAC_STRU_B3                    = 0xed36,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B3            = 0xed37,

        EN_NV_ID_ANT_MODEM_LOSS_B1                      = 0xed40,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B1          = 0xed4D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B1          = 0xed4E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B1        = 0xed4F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B1        = 0xed50,
        EN_NV_ID_LTE_IP2_CAL_B1                         = 0xed51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B1       = 0xed52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B1        = 0xed53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B1        = 0xed54,
        //EN_NV_ID_LTE_PA_POWER_B1                        = 0xed55,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B1         = 0xed56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B1          = 0xed57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B1          = 0xed58,
         /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B1                    = 0xed59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B1                  = 0xed5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B1                  = 0xed5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B1                       = 0xed5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B1                       = 0xed5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B1               = 0xed5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B1              = 0xed5f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B1                = 0xed65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B1               = 0xed66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B1              = 0xed67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B1           = 0xed68,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B1    = 0xed69,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B1    = 0xed6a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B1  = 0xed6b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B1  = 0xed6c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B1                  = 0xed6d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B1                  = 0xed6e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B1                 = 0xed6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B1                   = 0xed70,
        EN_NV_ID_ET_FINEDLY_PARA_B1                    = 0xed71,
        EN_NV_ID_ET_VOFFSET_GAIN_B1                    = 0xed72,
        EN_NV_ID_ET_EVDELY_B1                           = 0xed73,
        EN_NV_ID_RF_CA_RCCODE_B1                        = 0xed74,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B1             = 0xed75,
        EN_NV_ID_LTE_DPD_FAC_STRU_B1                    = 0xed76,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B1            = 0xed77,

        EN_NV_ID_ANT_MODEM_LOSS_B5                      = 0xed80,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B5          = 0xed8D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B5          = 0xed8E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B5        = 0xed8F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B5        = 0xed90,
        EN_NV_ID_LTE_IP2_CAL_B5                         = 0xed91,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B5       = 0xed92,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B5        = 0xed93,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B5        = 0xed94,
        //EN_NV_ID_LTE_PA_POWER_B5                        = 0xed95,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B5         = 0xed96,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B5          = 0xed97,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B5          = 0xed98,
         /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B5                    = 0xed99,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B5                  = 0xed9a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B5                  = 0xed9b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B5                       = 0xed9c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B5                       = 0xed9d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B5               = 0xed9e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B5              = 0xed9f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B5                = 0xedA5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B5               = 0xedA6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B5              = 0xedA7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B5           = 0xedA8,


        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B5    = 0xedA9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B5    = 0xedAa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B5  = 0xedAb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B5  = 0xedAc,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B5                  = 0xedad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B5                  = 0xedae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B5                 = 0xedaf,
        EN_NV_ID_LTE_ET_GAIN_COMP_B5                   = 0xedb0,
        EN_NV_ID_ET_FINEDLY_PARA_B5                    = 0xedb1,
        EN_NV_ID_ET_VOFFSET_GAIN_B5                    = 0xedb2,
        EN_NV_ID_ET_EVDELY_B5                           = 0xedb3,
        EN_NV_ID_RF_CA_RCCODE_B5                        = 0xedb4,

        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B5             = 0xedb5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B5                    = 0xedb6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B5            = 0xedb7,

        EN_NV_ID_ANT_MODEM_LOSS_B8                      = 0xedc0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B8          = 0xedcD,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B8          = 0xedcE,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B8        = 0xedcF,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B8        = 0xedd0,
        EN_NV_ID_LTE_IP2_CAL_B8                         = 0xedd1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B8       = 0xedd2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B8        = 0xedd3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B8        = 0xedd4,
        //EN_NV_ID_LTE_PA_POWER_B8                        = 0xedd5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B8         = 0xedd6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B8          = 0xedd7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B8          = 0xedd8,
         /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B8                    = 0xedd9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B8                  = 0xedda,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B8                  = 0xeddb,
        EN_NV_ID_LTE_APC_TABLE_STRU_B8                       = 0xeddc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B8                       = 0xeddd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B8               = 0xedde,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B8              = 0xeddf,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B8                = 0xede5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B8               = 0xede6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B8              = 0xede7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B8           = 0xede8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B8    = 0xede9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B8    = 0xedea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B8  = 0xedeb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B8  = 0xedec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B8                  = 0xeded,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B8                  = 0xedee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B8                 = 0xedef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B8                   = 0xedf0,
        EN_NV_ID_ET_FINEDLY_PARA_B8                    = 0xedf1,
        EN_NV_ID_ET_VOFFSET_GAIN_B8                    = 0xedf2,
        EN_NV_ID_ET_EVDELY_B8                           = 0xedf3,
        EN_NV_ID_RF_CA_RCCODE_B8                        = 0xedf4,
        /*add for V9R1_6361 End*/
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B8             = 0xedf5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B8                    = 0xedf6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B8            = 0xedf7,

        /* modify by   for 所有band begin*/
        EN_NV_ID_ANT_MODEM_LOSS_B19                     = 0xEE00,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B19         = 0xEE0d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B19         = 0xEE0e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B19       = 0xEE0f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B19       = 0xEE10,
        EN_NV_ID_LTE_IP2_CAL_B19                        = 0xEE11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B19      = 0xEE12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B19       = 0xEE13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B19       = 0xEE14,
        //EN_NV_ID_LTE_PA_POWER_B19                       = 0xEE15,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B19        = 0xEE16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B19         = 0xEE17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B19         = 0xEE18,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B19                     = 0xEE19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B19                   = 0xEE1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B19                   = 0xEE1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B19                        = 0xEE1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B19                        = 0xEE1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B19                = 0xEE1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B19               = 0xEE1f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B19                  = 0xee25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B19                 = 0xee26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B19                = 0xee27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B19             = 0xee28,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B19      = 0xee29,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B19      = 0xee2a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B19    = 0xee2b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B19    = 0xee2c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B19                  = 0xee2d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B19                  = 0xee2e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B19                 = 0xee2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B19                   = 0xee30,
        EN_NV_ID_ET_FINEDLY_PARA_B19                    = 0xee31,
        EN_NV_ID_ET_VOFFSET_GAIN_B19                    = 0xee32,
        EN_NV_ID_ET_EVDELY_B19                           = 0xee33,
        EN_NV_ID_RF_CA_RCCODE_B19                        = 0xee34,

        /*add for V9R1_6361 End*/

        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B19             = 0xee35,
        EN_NV_ID_LTE_DPD_FAC_STRU_B19                    = 0xee36,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B19            = 0xee37,
        EN_NV_ID_ANT_MODEM_LOSS_B21                     = 0xEE40,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B21         = 0xEE4d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B21         = 0xEE4e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B21       = 0xEE4f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B21       = 0xEE50,
        EN_NV_ID_LTE_IP2_CAL_B21                        = 0xEE51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B21      = 0xEE52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B21       = 0xEE53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B21       = 0xEE54,
        //EN_NV_ID_LTE_PA_POWER_B21                       = 0xEE55,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B21        = 0xEE56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B21         = 0xEE57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B21         = 0xEE58,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B21                     = 0xEE59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B21                   = 0xEE5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B21                   = 0xEE5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B21                        = 0xEE5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B21                        = 0xEE5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B21                = 0xEE5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B21               = 0xEE5f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B21                  = 0xee65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B21                 = 0xee66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B21                = 0xee67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B21             = 0xee68,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B21      = 0xee69,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B21      = 0xee6a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B21    = 0xee6b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B21    = 0xee6c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B21                  = 0xee6d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B21                  = 0xee6e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B21                 = 0xee6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B21                   = 0xee70,
        EN_NV_ID_ET_FINEDLY_PARA_B21                    = 0xee71,
        EN_NV_ID_ET_VOFFSET_GAIN_B21                    = 0xee72,
        EN_NV_ID_ET_EVDELY_B21                           = 0xee73,
        EN_NV_ID_RF_CA_RCCODE_B21                        = 0xee74,
        /*add for V9R1_6361 End*/

        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B21             = 0xee75,
        EN_NV_ID_LTE_DPD_FAC_STRU_B21                    = 0xee76,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B21            = 0xee77,

        EN_NV_ID_ANT_MODEM_LOSS_B2                      = 0xEE80,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B2          = 0xEE8d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B2          = 0xEE8e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B2        = 0xEE8f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B2        = 0xEE90,
        EN_NV_ID_LTE_IP2_CAL_B2                         = 0xEE91,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B2       = 0xEE92,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B2        = 0xEE93,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B2        = 0xEE94,
        //EN_NV_ID_LTE_PA_POWER_B2                        = 0xEE95,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B2         = 0xEE96,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B2          = 0xEE97,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B2          = 0xEE98,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B2                     = 0xEE99,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B2                   = 0xEE9a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B2                   = 0xEE9b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B2                        = 0xEE9c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B2                        = 0xEE9d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B2                = 0xEE9e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B2               = 0xEE9f,
        /*add for V9R1_6361 End*/



        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B2                = 0xEEa5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B2               = 0xEEa6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B2              = 0xEEa7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B2           = 0xEEa8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B2    = 0xEEa9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B2    = 0xEEaa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B2  = 0xEEab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B2  = 0xEEac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B2                  = 0xEEad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B2                  = 0xEEae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B2                 = 0xEEaf,
        EN_NV_ID_LTE_ET_GAIN_COMP_B2                   = 0xEEb0,
        EN_NV_ID_ET_FINEDLY_PARA_B2                    = 0xEEb1,
        EN_NV_ID_ET_VOFFSET_GAIN_B2                    = 0xEEb2,
        EN_NV_ID_ET_EVDELY_B2                           = 0xEEb3,
        EN_NV_ID_RF_CA_RCCODE_B2                        = 0xEEb4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B2             = 0xEEb5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B2                    = 0xEEb6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B2            = 0xEEb7,

        EN_NV_ID_ANT_MODEM_LOSS_B4                      = 0xEEc0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B4          = 0xEEcd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B4          = 0xEEce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B4        = 0xEEcf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B4        = 0xEEd0,
        EN_NV_ID_LTE_IP2_CAL_B4                         = 0xEEd1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B4       = 0xEEd2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B4        = 0xEEd3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B4        = 0xEEd4,
        //EN_NV_ID_LTE_PA_POWER_B4                        = 0xEEd5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B4         = 0xEEd6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B4          = 0xEEd7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B4          = 0xEEd8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B4                     = 0xEEd9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B4                   = 0xEEda,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B4                   = 0xEEdb,
        EN_NV_ID_LTE_APC_TABLE_STRU_B4                        = 0xEEdc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B4                        = 0xEEdd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B4                = 0xEEde,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B4               = 0xEEdf,
        /*add for V9R1_6361 End*/

        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B4                = 0xEEe5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B4               = 0xEEe6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B4              = 0xEEe7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B4           = 0xEEe8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B4    = 0xEEe9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B4    = 0xEEea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B4  = 0xEEeb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B4  = 0xEEec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B4                  = 0xEEed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B4                  = 0xEEee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B4                 = 0xEEef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B4                   = 0xEEf0,
        EN_NV_ID_ET_FINEDLY_PARA_B4                    = 0xEEf1,
        EN_NV_ID_ET_VOFFSET_GAIN_B4                    = 0xEEf2,
        EN_NV_ID_ET_EVDELY_B4                           = 0xEEf3,
        EN_NV_ID_RF_CA_RCCODE_B4                        = 0xEEf4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B4             = 0xEEf5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B4                    = 0xEEf6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B4            = 0xEEf7,

        EN_NV_ID_ANT_MODEM_LOSS_B6                      = 0xEf00,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B6          = 0xEf0d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B6          = 0xEf0e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B6        = 0xEf0f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B6        = 0xEf10,
        EN_NV_ID_LTE_IP2_CAL_B6                         = 0xEf11,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B6       = 0xEf12,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B6        = 0xEf13,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B6        = 0xEf14,
        //EN_NV_ID_LTE_PA_POWER_B6                        = 0xEf15,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B6         = 0xEf16,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B6          = 0xEf17,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B6          = 0xEf18,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B6                     = 0xEf19,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B6                   = 0xEf1a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B6                   = 0xEf1b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B6                        = 0xEf1c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B6                        = 0xEf1d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B6                = 0xEf1e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B6               = 0xEf1f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B6                = 0xEf25,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B6               = 0xEf26,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B6              = 0xEf27,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B6           = 0xEf28,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B6    = 0xEf29,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B6    = 0xEf2a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B6  = 0xEf2b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B6  = 0xEf2c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B6                  = 0xEf2d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B6                  = 0xEf2e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B6                 = 0xEf2f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B6                   = 0xEf30,
        EN_NV_ID_ET_FINEDLY_PARA_B6                    = 0xEf31,
        EN_NV_ID_ET_VOFFSET_GAIN_B6                    = 0xEf32,
        EN_NV_ID_ET_EVDELY_B6                           = 0xEf33,
        EN_NV_ID_RF_CA_RCCODE_B6                        = 0xEf34,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B6             = 0xEf35,
        EN_NV_ID_LTE_DPD_FAC_STRU_B6                    = 0xEf36,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B6            = 0xEf37,

        EN_NV_ID_ANT_MODEM_LOSS_B9                      = 0xEf40,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B9          = 0xEf4d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B9          = 0xEf4e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B9        = 0xEf4f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B9        = 0xEf50,
        EN_NV_ID_LTE_IP2_CAL_B9                         = 0xEf51,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B9       = 0xEf52,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B9        = 0xEf53,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B9        = 0xEf54,
        //EN_NV_ID_LTE_PA_POWER_B9                        = 0xEf55,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B9         = 0xEf56,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B9          = 0xEf57,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B9          = 0xEf58,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B9                     = 0xEf59,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B9                   = 0xEf5a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B9                   = 0xEf5b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B9                        = 0xEf5c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B9                        = 0xEf5d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B9                = 0xEf5e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B9               = 0xEf5f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B9                = 0xEf65,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B9               = 0xEf66,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B9              = 0xEf67,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B9           = 0xEf68,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B9    = 0xEf69,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B9    = 0xEf6a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B9  = 0xEf6b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B9  = 0xEf6c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B9                  = 0xEf6d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B9                  = 0xEf6e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B9                 = 0xEf6f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B9                   = 0xEf70,
        EN_NV_ID_ET_FINEDLY_PARA_B9                    = 0xEf71,
        EN_NV_ID_ET_VOFFSET_GAIN_B9                    = 0xEf72,
        EN_NV_ID_ET_EVDELY_B9                           = 0xEf73,
        EN_NV_ID_RF_CA_RCCODE_B9                        = 0xEf74,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B9             = 0xEf75,
        EN_NV_ID_LTE_DPD_FAC_STRU_B9                    = 0xEf76,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B9            = 0xEf77,

        EN_NV_ID_ANT_MODEM_LOSS_B10                     = 0xEf80,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B10         = 0xEf8d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B10         = 0xEf8e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B10       = 0xEf8f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B10       = 0xEf90,
        EN_NV_ID_LTE_IP2_CAL_B10                        = 0xEf91,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B10      = 0xEf92,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B10       = 0xEf93,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B10       = 0xEf94,
        //EN_NV_ID_LTE_PA_POWER_B10                       = 0xEf95,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B10        = 0xEf96,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B10         = 0xEf97,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B10         = 0xEf98,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B10                    = 0xEf99,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B10                  = 0xEf9a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B10                  = 0xEf9b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B10                       = 0xEf9c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B10                       = 0xEf9d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B10               = 0xEf9e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B10              = 0xEf9f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B10                = 0xEfa5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B10               = 0xEfa6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B10              = 0xEfa7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B10           = 0xEfa8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B10   = 0xEfa9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B10   = 0xEfaa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B10  = 0xEfab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B10  = 0xEfac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B10                  = 0xEfad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B10                  = 0xEfae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B10                 = 0xEfaf,
        EN_NV_ID_LTE_ET_GAIN_COMP_B10                   = 0xEfb0,
        EN_NV_ID_ET_FINEDLY_PARA_B10                    = 0xEfb1,
        EN_NV_ID_ET_VOFFSET_GAIN_B10                    = 0xEfb2,
        EN_NV_ID_ET_EVDELY_B10                           = 0xEfb3,
        EN_NV_ID_RF_CA_RCCODE_B10                        = 0xEfb4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B10             = 0xEfb5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B10                    = 0xEfb6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B10            = 0xEfb7,





        EN_NV_ID_ANT_MODEM_LOSS_B11                     = 0xEfc0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B11         = 0xEfcd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B11         = 0xEfce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B11       = 0xEfcf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B11       = 0xEfd0,
        EN_NV_ID_LTE_IP2_CAL_B11                        = 0xEfd1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B11      = 0xEfd2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B11       = 0xEfd3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B11       = 0xEfd4,
        //EN_NV_ID_LTE_PA_POWER_B11                       = 0xEfd5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B11        = 0xEfd6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B11         = 0xEfd7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B11         = 0xEfd8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B11                    = 0xEfd9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B11                  = 0xEfda,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B11                  = 0xEfdb,
        EN_NV_ID_LTE_APC_TABLE_STRU_B11                       = 0xEfdc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B11                       = 0xEfdd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B11               = 0xEfde,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B11              = 0xEfdf,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B11                = 0xEfe5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B11               = 0xEfe6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B11              = 0xEfe7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B11           = 0xEfe8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B11   = 0xEfe9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B11   = 0xEfea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B11  = 0xEfeb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B11  = 0xEfec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B11                  = 0xEfed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B11                  = 0xEfee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B11                 = 0xEfef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B11                   = 0xEff0,
        EN_NV_ID_ET_FINEDLY_PARA_B11                    = 0xEff1,
        EN_NV_ID_ET_VOFFSET_GAIN_B11                    = 0xEff2,
        EN_NV_ID_ET_EVDELY_B11                           = 0xEff3,
        EN_NV_ID_RF_CA_RCCODE_B11                        = 0xEff4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B11             = 0xEff5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B11                    = 0xEff6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B11            = 0xEff7,

        EN_NV_ID_ANT_MODEM_LOSS_B12                     = 0xf000,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B12         = 0xf00d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B12         = 0xf00e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B12       = 0xf00f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B12       = 0xf010,
        EN_NV_ID_LTE_IP2_CAL_B12                        = 0xf011,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B12      = 0xf012,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B12       = 0xf013,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B12       = 0xf014,
        //EN_NV_ID_LTE_PA_POWER_B12                       = 0xf015,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B12        = 0xf016,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B12         = 0xf017,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B12         = 0xf018,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B12                    = 0xf019,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B12                  = 0xf01a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B12                  = 0xf01b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B12                       = 0xf01c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B12                       = 0xf01d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B12               = 0xf01e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B12              = 0xf01f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B12                = 0xf025,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B12               = 0xf026,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B12              = 0xf027,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B12           = 0xf028,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B12   = 0xf029,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B12   = 0xf02a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B12  = 0xf02b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B12  = 0xf02c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B12                  = 0xf02d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B12                  = 0xf02e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B12                 = 0xf02f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B12                   = 0xf030,
        EN_NV_ID_ET_FINEDLY_PARA_B12                    = 0xf031,
        EN_NV_ID_ET_VOFFSET_GAIN_B12                    = 0xf032,
        EN_NV_ID_ET_EVDELY_B12                           = 0xf033,
        EN_NV_ID_RF_CA_RCCODE_B12                        = 0xf034,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B12             = 0xf035,
        EN_NV_ID_LTE_DPD_FAC_STRU_B12                    = 0xf036,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B12            = 0xf037,

        EN_NV_ID_ANT_MODEM_LOSS_B13                     = 0xf040,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B13         = 0xf04d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B13         = 0xf04e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B13       = 0xf04f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B13       = 0xf050,
        EN_NV_ID_LTE_IP2_CAL_B13                        = 0xf051,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B13      = 0xf052,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B13       = 0xf053,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B13       = 0xf054,
        //EN_NV_ID_LTE_PA_POWER_B13                       = 0xf055,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B13        = 0xf056,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B13         = 0xf057,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B13         = 0xf058,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B13                    = 0xf059,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B13                  = 0xf05a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B13                  = 0xf05b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B13                       = 0xf05c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B13                       = 0xf05d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B13               = 0xf05e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B13              = 0xf05f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B13                = 0xf065,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B13               = 0xf066,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B13              = 0xf067,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B13           = 0xf068,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B13   = 0xf069,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B13   = 0xf06a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B13  = 0xf06b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B13  = 0xf06c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B13                  = 0xf06d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B13                  = 0xf06e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B13                 = 0xf06f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B13                   = 0xf070,
        EN_NV_ID_ET_FINEDLY_PARA_B13                    = 0xf071,
        EN_NV_ID_ET_VOFFSET_GAIN_B13                    = 0xf072,
        EN_NV_ID_ET_EVDELY_B13                           = 0xf073,
        EN_NV_ID_RF_CA_RCCODE_B13                        = 0xf074,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B13             = 0xf075,
        EN_NV_ID_LTE_DPD_FAC_STRU_B13                    = 0xf076,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B13            = 0xf077,


        EN_NV_ID_ANT_MODEM_LOSS_B14                     = 0xf080,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B14         = 0xf08d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B14         = 0xf08e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B14       = 0xf08f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B14       = 0xf090,
        EN_NV_ID_LTE_IP2_CAL_B14                        = 0xf091,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B14      = 0xf092,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B14       = 0xf093,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B14       = 0xf094,
        //EN_NV_ID_LTE_PA_POWER_B14                       = 0xf095,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B14        = 0xf096,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B14         = 0xf097,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B14         = 0xf098,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B14                    = 0xf099,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B14                  = 0xf09a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B14                  = 0xf09b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B14                       = 0xf09c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B14                       = 0xf09d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B14               = 0xf09e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B14              = 0xf09f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B14                = 0xf0a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B14               = 0xf0a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B14              = 0xf0a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B14           = 0xf0a8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B14   = 0xf0a9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B14   = 0xf0aa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B14  = 0xf0ab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B14  = 0xf0ac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B14                  = 0xf0ad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B14                  = 0xf0ae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B14                 = 0xf0af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B14                   = 0xf0b0,
        EN_NV_ID_ET_FINEDLY_PARA_B14                    = 0xf0b1,
        EN_NV_ID_ET_VOFFSET_GAIN_B14                    = 0xf0b2,
        EN_NV_ID_ET_EVDELY_B14                           = 0xf0b3,
        EN_NV_ID_RF_CA_RCCODE_B14                        = 0xf0b4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B14             = 0xf0b5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B14                    = 0xf0b6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B14            = 0xf0b7,






        EN_NV_ID_ANT_MODEM_LOSS_B17                     = 0xf0c0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B17         = 0xf0cd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B17         = 0xf0ce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B17       = 0xf0cf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B17       = 0xf0d0,
        EN_NV_ID_LTE_IP2_CAL_B17                        = 0xf0d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B17      = 0xf0d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B17       = 0xf0d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B17       = 0xf0d4,
        //EN_NV_ID_LTE_PA_POWER_B17                       = 0xf0d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B17        = 0xf0d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B17         = 0xf0d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B17         = 0xf0d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B17                    = 0xf0d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B17                  = 0xf0da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B17                  = 0xf0db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B17                       = 0xf0dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B17                       = 0xf0dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B17               = 0xf0de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B17              = 0xf0df,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B17                = 0xf0e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B17               = 0xf0e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B17              = 0xf0e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B17           = 0xf0e8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B17   = 0xf0e9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B17   = 0xf0ea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B17  = 0xf0eb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B17  = 0xf0ec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B17                  = 0xf0ed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B17                  = 0xf0ee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B17                 = 0xf0ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B17                   = 0xf0f0,
        EN_NV_ID_ET_FINEDLY_PARA_B17                    = 0xf0f1,
        EN_NV_ID_ET_VOFFSET_GAIN_B17                    = 0xf0f2,
        EN_NV_ID_ET_EVDELY_B17                           = 0xf0f3,
        EN_NV_ID_RF_CA_RCCODE_B17                        = 0xf0f4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B17             = 0xf0f5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B17                    = 0xf0f6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B17            = 0xf0f7,


        EN_NV_ID_ANT_MODEM_LOSS_B18                      = 0xf100,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B18          = 0xf10D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B18          = 0xf10E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B18        = 0xf10F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B18        = 0xf110,
        EN_NV_ID_LTE_IP2_CAL_B18                         = 0xf111,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B18       = 0xf112,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B18        = 0xf113,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B18        = 0xf114,
        //EN_NV_ID_LTE_PA_POWER_B18                        = 0xf115,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B18         = 0xf116,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B18          = 0xf117,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B18          = 0xf118,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B18                    = 0xf119,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B18                  = 0xf11a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B18                  = 0xf11b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B18                       = 0xf11c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B18                       = 0xf11d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B18               = 0xf11e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B18              = 0xf11f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B18                = 0xf125,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B18               = 0xf126,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B18              = 0xf127,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B18           = 0xf128,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B18    = 0xf129,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B18    = 0xf12a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B18  = 0xf12b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B18  = 0xf12c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B18                  = 0xf12d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B18                  = 0xf12e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B18                 = 0xf12f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B18                   = 0xf130,
        EN_NV_ID_ET_FINEDLY_PARA_B18                    = 0xf131,
        EN_NV_ID_ET_VOFFSET_GAIN_B18                    = 0xf132,
        EN_NV_ID_ET_EVDELY_B18                           = 0xf133,
        EN_NV_ID_RF_CA_RCCODE_B18                        = 0xf134,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B18             = 0xf135,
        EN_NV_ID_LTE_DPD_FAC_STRU_B18                    = 0xf136,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B18            = 0xf137,
        /*add for V9R1_6361 End*/





        EN_NV_ID_ANT_MODEM_LOSS_B22                     = 0xf140,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B22         = 0xf14d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B22         = 0xf14e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B22       = 0xf14f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B22       = 0xf150,
        EN_NV_ID_LTE_IP2_CAL_B22                        = 0xf151,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B22      = 0xf152,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B22       = 0xf153,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B22       = 0xf154,
        //EN_NV_ID_LTE_PA_POWER_B22                       = 0xf155,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B22        = 0xf156,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B22         = 0xf157,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B22         = 0xf158,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B22                    = 0xf159,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B22                  = 0xf15a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B22                  = 0xf15b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B22                       = 0xf15c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B22                       = 0xf15d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B22               = 0xf15e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B22              = 0xf15f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B22                = 0xf165,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B22               = 0xf166,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B22              = 0xf167,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B22           = 0xf168,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B22   = 0xf169,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B22   = 0xf16a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B22  = 0xf16b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B22  = 0xf16c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B22                  = 0xf16d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B22                  = 0xf16e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B22                 = 0xf16f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B22                   = 0xf170,
        EN_NV_ID_ET_FINEDLY_PARA_B22                    = 0xf171,
        EN_NV_ID_ET_VOFFSET_GAIN_B22                    = 0xf172,
        EN_NV_ID_ET_EVDELY_B22                           = 0xf173,
        EN_NV_ID_RF_CA_RCCODE_B22                        = 0xf174,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B22            = 0xf175,
        EN_NV_ID_LTE_DPD_FAC_STRU_B22                   = 0xf176,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B22           = 0xf177,





        EN_NV_ID_ANT_MODEM_LOSS_B23                     = 0xf180,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B23         = 0xf18d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B23         = 0xf18e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B23       = 0xf18f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B23       = 0xf190,
        EN_NV_ID_LTE_IP2_CAL_B23                        = 0xf191,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B23      = 0xf192,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B23       = 0xf193,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B23       = 0xf194,
        //EN_NV_ID_LTE_PA_POWER_B23                       = 0xf195,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B23        = 0xf196,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B23         = 0xf197,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B23         = 0xf198,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B23                    = 0xf199,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B23                  = 0xf19a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B23                  = 0xf19b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B23                       = 0xf19c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B23                       = 0xf19d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B23               = 0xf19e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B23              = 0xf19f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B23                = 0xf1a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B23               = 0xf1a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B23              = 0xf1a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B23           = 0xf1a8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B23   = 0xf1a9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B23   = 0xf1aa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B23  = 0xf1ab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B23  = 0xf1ac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B23                  = 0xf1ad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B23                  = 0xf1ae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B23                 = 0xf1af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B23                   = 0xf1b0,
        EN_NV_ID_ET_FINEDLY_PARA_B23                    = 0xf1b1,
        EN_NV_ID_ET_VOFFSET_GAIN_B23                    = 0xf1b2,
        EN_NV_ID_ET_EVDELY_B23                           = 0xf1b3,
        EN_NV_ID_RF_CA_RCCODE_B23                        = 0xf1b4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B23             = 0xf1b5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B23                    = 0xf1b6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B23            = 0xf1b7,

        EN_NV_ID_ANT_MODEM_LOSS_B24                     = 0xf1c0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B24         = 0xf1cd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B24         = 0xf1ce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B24       = 0xf1cf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B24       = 0xf1d0,
        EN_NV_ID_LTE_IP2_CAL_B24                        = 0xf1d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B24      = 0xf1d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B24       = 0xf1d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B24       = 0xf1d4,
        //EN_NV_ID_LTE_PA_POWER_B24                       = 0xf1d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B24        = 0xf1d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B24         = 0xf1d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B24         = 0xf1d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B24                    = 0xf1d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B24                  = 0xf1da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B24                  = 0xf1db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B24                       = 0xf1dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B24                       = 0xf1dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B24               = 0xf1de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B24              = 0xf1df,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B24                = 0xf1e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B24               = 0xf1e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B24              = 0xf1e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B24           = 0xf1e8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B24   = 0xf1e9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B24   = 0xf1ea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B24  = 0xf1eb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B24  = 0xf1ec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B24                  = 0xf1ed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B24                  = 0xf1ee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B24                 = 0xf1ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B24                   = 0xf1f0,
        EN_NV_ID_ET_FINEDLY_PARA_B24                    = 0xf1f1,
        EN_NV_ID_ET_VOFFSET_GAIN_B24                    = 0xf1f2,
        EN_NV_ID_ET_EVDELY_B24                           = 0xf1f3,
        EN_NV_ID_RF_CA_RCCODE_B24                        = 0xf1f4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B24             = 0xf1f5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B24                    = 0xf1f6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B24            = 0xf1f7,

        EN_NV_ID_ANT_MODEM_LOSS_B25                      = 0xf200,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B25          = 0xf20D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B25          = 0xf20E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B25        = 0xf20F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B25        = 0xf210,
        EN_NV_ID_LTE_IP2_CAL_B25                         = 0xf211,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B25       = 0xf212,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B25        = 0xf213,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B25        = 0xf214,
        //EN_NV_ID_LTE_PA_POWER_B25                        = 0xf215,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B25         = 0xf216,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B25          = 0xf217,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B25          = 0xf218,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B25                    = 0xf219,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B25                  = 0xf21a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B25                  = 0xf21b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B25                       = 0xf21c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B25                       = 0xf21d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B25               = 0xf21e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B25              = 0xf21f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B25                = 0xf225,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B25               = 0xf226,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B25              = 0xf227,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B25           = 0xf228,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B25    = 0xf229,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B25    = 0xf22a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B25  = 0xf22b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B25  = 0xf22c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B25                  = 0xf22d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B25                  = 0xf22e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B25                 = 0xf22f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B25                   = 0xf230,
        EN_NV_ID_ET_FINEDLY_PARA_B25                    = 0xf231,
        EN_NV_ID_ET_VOFFSET_GAIN_B25                    = 0xf232,
        EN_NV_ID_ET_EVDELY_B25                           = 0xf233,
        EN_NV_ID_RF_CA_RCCODE_B25                        = 0xf234,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B25             = 0xf235,
        EN_NV_ID_LTE_DPD_FAC_STRU_B25                    = 0xf236,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B25            = 0xf237,

        EN_NV_ID_ANT_MODEM_LOSS_B26                      = 0xf840,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B26          = 0xf84D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B26          = 0xf84E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B26        = 0xf84F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B26        = 0xf850,
        EN_NV_ID_LTE_IP2_CAL_B26                         = 0xf851,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B26       = 0xf852,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B26        = 0xf853,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B26        = 0xf854,
        //EN_NV_ID_LTE_PA_POWER_B26                        = 0xf855,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B26         = 0xf856,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B26          = 0xf857,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B26          = 0xf858,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B26                    = 0xf859,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B26                  = 0xf85a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B26                  = 0xf85b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B26                       = 0xf85c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B26                       = 0xf85d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B26               = 0xf85e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B26              = 0xf85f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B26                = 0xf865,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B26               = 0xf866,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B26              = 0xf867,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B26           = 0xf868,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B26    = 0xf869,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B26    = 0xf86a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B26  = 0xf86b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B26  = 0xf86c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B26                  = 0xf86d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B26                  = 0xf86e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B26                 = 0xf86f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B26                   = 0xf870,
        EN_NV_ID_ET_FINEDLY_PARA_B26                    = 0xf871,
        EN_NV_ID_ET_VOFFSET_GAIN_B26                    = 0xf872,
        EN_NV_ID_ET_EVDELY_B26                           = 0xf873,
        EN_NV_ID_RF_CA_RCCODE_B26                        = 0xf874,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B26             = 0xf875,
        EN_NV_ID_LTE_DPD_FAC_STRU_B26                    = 0xf876,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B26            = 0xf877,
        /* BEGIN: Added by  , 2015/6/3   ????:K3V5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_BNon26                      = 0xf878,
        /* END:   Added by  , 2015/6/3 */


        EN_NV_ID_ANT_MODEM_LOSS_B33                     = 0xf240,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B33         = 0xf24d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B33         = 0xf24e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B33       = 0xf24f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B33       = 0xf250,
        EN_NV_ID_LTE_IP2_CAL_B33                        = 0xf251,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B33      = 0xf252,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B33       = 0xf253,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B33       = 0xf254,
        //EN_NV_ID_LTE_PA_POWER_B33                       = 0xf255,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B33        = 0xf256,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B33         = 0xf257,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B33         = 0xf258,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B33                    = 0xf259,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B33                  = 0xf25a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B33                  = 0xf25b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B33                       = 0xf25c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B33                       = 0xf25d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B33               = 0xf25e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B33              = 0xf25f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B33                = 0xf265,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B33               = 0xf266,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B33              = 0xf267,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B33           = 0xf268,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B33   = 0xf269,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B33   = 0xf26a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B33  = 0xf26b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B33  = 0xf26c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B33                  = 0xf26d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B33                  = 0xf26e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B33                 = 0xf26f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B33                   = 0xf270,
        EN_NV_ID_ET_FINEDLY_PARA_B33                    = 0xf271,
        EN_NV_ID_ET_VOFFSET_GAIN_B33                    = 0xf272,
        EN_NV_ID_ET_EVDELY_B33                           = 0xf273,
        EN_NV_ID_RF_CA_RCCODE_B33                        = 0xf274,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B33             = 0xf275,
        EN_NV_ID_LTE_DPD_FAC_STRU_B33                    = 0xf276,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B33            = 0xf277,





        EN_NV_ID_ANT_MODEM_LOSS_B34                     = 0xf280,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B34         = 0xf28d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B34         = 0xf28e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B34       = 0xf28f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B34       = 0xf290,
        EN_NV_ID_LTE_IP2_CAL_B34                        = 0xf291,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B34      = 0xf292,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B34       = 0xf293,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B34       = 0xf294,
        //EN_NV_ID_LTE_PA_POWER_B34                       = 0xf295,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B34        = 0xf296,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B34         = 0xf297,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B34         = 0xf298,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B34                    = 0xf299,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B34                  = 0xf29a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B34                  = 0xf29b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B34                       = 0xf29c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B34                       = 0xf29d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B34               = 0xf29e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B34              = 0xf29f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B34                = 0xf2a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B34               = 0xf2a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B34              = 0xf2a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B34           = 0xf2a8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B34   = 0xf2a9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B34   = 0xf2aa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B34  = 0xf2ab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B34  = 0xf2ac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B34                  = 0xf2ad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B34                  = 0xf2ae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B34                 = 0xf2af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B34                   = 0xf2b0,
        EN_NV_ID_ET_FINEDLY_PARA_B34                    = 0xf2b1,
        EN_NV_ID_ET_VOFFSET_GAIN_B34                    = 0xf2b2,
        EN_NV_ID_ET_EVDELY_B34                           = 0xf2b3,
        EN_NV_ID_RF_CA_RCCODE_B34                        = 0xf2b4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B34             = 0xf2b5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B34                    = 0xf2b6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B34            = 0xf2b7,

        EN_NV_ID_ANT_MODEM_LOSS_B35                     = 0xf2c0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B35         = 0xf2cd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B35         = 0xf2ce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B35       = 0xf2cf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B35       = 0xf2d0,
        EN_NV_ID_LTE_IP2_CAL_B35                        = 0xf2d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B35      = 0xf2d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B35       = 0xf2d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B35       = 0xf2d4,
        //EN_NV_ID_LTE_PA_POWER_B35                       = 0xf2d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B35        = 0xf2d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B35         = 0xf2d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B35         = 0xf2d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B35                    = 0xf2d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B35                  = 0xf2da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B35                  = 0xf2db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B35                       = 0xf2dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B35                       = 0xf2dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B35               = 0xf2de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B35              = 0xf2df,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B35                = 0xf2e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B35               = 0xf2e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B35              = 0xf2e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B35           = 0xf2e8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B35   = 0xf2e9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B35   = 0xf2ea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B35  = 0xf2eb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B35  = 0xf2ec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B35                  = 0xf2ed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B35                  = 0xf2ee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B35                 = 0xf2ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B35                   = 0xf2f0,
        EN_NV_ID_ET_FINEDLY_PARA_B35                    = 0xf2f1,
        EN_NV_ID_ET_VOFFSET_GAIN_B35                    = 0xf2f2,
        EN_NV_ID_ET_EVDELY_B35                           = 0xf2f3,
        EN_NV_ID_RF_CA_RCCODE_B35                        = 0xf2f4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B35             = 0xf2f5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B35                    = 0xf2f6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B35            = 0xf2f7,

        EN_NV_ID_ANT_MODEM_LOSS_B36                     = 0xf300,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B36         = 0xf30d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B36         = 0xf30e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B36       = 0xf30f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B36       = 0xf310,
        EN_NV_ID_LTE_IP2_CAL_B36                        = 0xf311,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B36      = 0xf312,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B36       = 0xf313,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B36       = 0xf314,
        //EN_NV_ID_LTE_PA_POWER_B36                       = 0xf315,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B36        = 0xf316,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B36         = 0xf317,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B36         = 0xf318,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B36                    = 0xf319,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B36                  = 0xf31a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B36                  = 0xf31b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B36                       = 0xf31c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B36                       = 0xf31d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B36               = 0xf31e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B36              = 0xf31f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B36                = 0xf325,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B36               = 0xf326,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B36              = 0xf327,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B36           = 0xf328,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B36   = 0xf329,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B36   = 0xf32a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B36  = 0xf32b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B36  = 0xf32c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B36                  = 0xf32d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B36                  = 0xf32e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B36                 = 0xf32f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B36                   = 0xf330,
        EN_NV_ID_ET_FINEDLY_PARA_B36                    = 0xf331,
        EN_NV_ID_ET_VOFFSET_GAIN_B36                    = 0xf332,
        EN_NV_ID_ET_EVDELY_B36                           = 0xf333,
        EN_NV_ID_RF_CA_RCCODE_B36                        = 0xf334,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B36             = 0xf335,
        EN_NV_ID_LTE_DPD_FAC_STRU_B36                    = 0xf336,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B36            = 0xf337,

        EN_NV_ID_ANT_MODEM_LOSS_B37                     = 0xf340,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B37         = 0xf34d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B37         = 0xf34e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B37       = 0xf34f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B37       = 0xf350,
        EN_NV_ID_LTE_IP2_CAL_B37                        = 0xf351,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B37      = 0xf352,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B37       = 0xf353,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B37       = 0xf354,
        //EN_NV_ID_LTE_PA_POWER_B37                       = 0xf355,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B37        = 0xf356,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B37         = 0xf357,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B37         = 0xf358,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B37                    = 0xf359,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B37                  = 0xf35a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B37                  = 0xf35b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B37                       = 0xf35c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B37                       = 0xf35d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B37               = 0xf35e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B37              = 0xf35f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B37                = 0xf365,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B37               = 0xf366,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B37              = 0xf367,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B37           = 0xf368,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B37   = 0xf369,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B37   = 0xf36a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B37  = 0xf36b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B37  = 0xf36c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B37                  = 0xf36d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B37                  = 0xf36e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B37                 = 0xf36f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B37                   = 0xf370,
        EN_NV_ID_ET_FINEDLY_PARA_B37                    = 0xf371,
        EN_NV_ID_ET_VOFFSET_GAIN_B37                    = 0xf372,
        EN_NV_ID_ET_EVDELY_B37                           = 0xf373,
        EN_NV_ID_RF_CA_RCCODE_B37                        = 0xf374,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B37             = 0xf375,
        EN_NV_ID_LTE_DPD_FAC_STRU_B37                    = 0xf376,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B37            = 0xf377,

        EN_NV_ID_ANT_MODEM_LOSS_B42                     = 0xf380,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B42         = 0xf38d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B42         = 0xf38e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B42       = 0xf38f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B42       = 0xf390,
        EN_NV_ID_LTE_IP2_CAL_B42                        = 0xf391,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B42      = 0xf392,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B42       = 0xf393,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B42       = 0xf394,
        //EN_NV_ID_LTE_PA_POWER_B42                       = 0xf395,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B42        = 0xf396,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B42         = 0xf397,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B42         = 0xf398,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B42                    = 0xf399,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B42                  = 0xf39a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B42                  = 0xf39b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B42                       = 0xf39c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B42                       = 0xf39d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B42               = 0xf39e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B42              = 0xf39f,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B42                = 0xf3a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B42               = 0xf3a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B42              = 0xf3a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B42           = 0xf3a8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B42   = 0xf3a9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B42   = 0xf3aa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B42  = 0xf3ab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B42  = 0xf3ac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B42                  = 0xf3ad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B42                  = 0xf3ae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B42                 = 0xf3af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B42                   = 0xf3b0,
        EN_NV_ID_ET_FINEDLY_PARA_B42                    = 0xf3b1,
        EN_NV_ID_ET_VOFFSET_GAIN_B42                    = 0xf3b2,
        EN_NV_ID_ET_EVDELY_B42                           = 0xf3b3,
        EN_NV_ID_RF_CA_RCCODE_B42                        = 0xf3b4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B42             = 0xf3b5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B42                    = 0xf3b6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B42            = 0xf3b7,

        EN_NV_ID_ANT_MODEM_LOSS_B43                     = 0xf3c0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B43         = 0xf3cd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B43         = 0xf3ce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B43       = 0xf3cf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B43       = 0xf3d0,
        EN_NV_ID_LTE_IP2_CAL_B43                        = 0xf3d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B43      = 0xf3d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B43       = 0xf3d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B43       = 0xf3d4,
        //EN_NV_ID_LTE_PA_POWER_B43                       = 0xf3d5,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B43        = 0xf3d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B43         = 0xf3d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B43         = 0xf3d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B43                    = 0xf3d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B43                  = 0xf3da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B43                  = 0xf3db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B43                       = 0xf3dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B43                       = 0xf3dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B43               = 0xf3de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B43              = 0xf3df,
        /*add for V9R1_6361 End*/


        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B43                = 0xf3e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B43               = 0xf3e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B43              = 0xf3e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B43           = 0xf3e8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B43   = 0xf3e9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B43   = 0xf3ea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B43  = 0xf3eb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B43  = 0xf3ec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B43                  = 0xf3ed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B43                  = 0xf3ee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B43                 = 0xf3ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B43                   = 0xf3f0,
        EN_NV_ID_ET_FINEDLY_PARA_B43                    = 0xf3f1,
        EN_NV_ID_ET_VOFFSET_GAIN_B43                    = 0xf3f2,
        EN_NV_ID_ET_EVDELY_B43                           = 0xf3f3,
        EN_NV_ID_RF_CA_RCCODE_B43                        = 0xf3f4,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B43             = 0xf3f5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B43                    = 0xf3f6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B43            = 0xf3f7,

        EN_NV_ID_ANT_MODEM_LOSS_B39                     = 0xf400,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B39         = 0xf40d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B39         = 0xf40e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B39       = 0xf40f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B39       = 0xf410,
        EN_NV_ID_LTE_IP2_CAL_B39                        = 0xf411,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B39      = 0xf412,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B39       = 0xf413,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B39       = 0xf414,
        //EN_NV_ID_LTE_PA_POWER_B39                       = 0xf415,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B39        = 0xf416,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B39         = 0xf417,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B39         = 0xf418,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B39                    = 0xf419,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B39                  = 0xf41a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B39                  = 0xf41b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B39                       = 0xf41c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B39                       = 0xf41d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B39               = 0xf41e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B39              = 0xf41f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B39                = 0xf425,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B39               = 0xf426,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B39              = 0xf427,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B39           = 0xf428,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B39   = 0xf429,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B39   = 0xf42a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B39  = 0xf42b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B39  = 0xf42c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B39                  = 0xf42d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B39                  = 0xf42e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B39                 = 0xf42f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B39                   = 0xf430,
        EN_NV_ID_ET_FINEDLY_PARA_B39                    = 0xf431,
        EN_NV_ID_ET_VOFFSET_GAIN_B39                    = 0xf432,
        EN_NV_ID_ET_EVDELY_B39                           = 0xf433,
        EN_NV_ID_RF_CA_RCCODE_B39                        = 0xf434,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B39             = 0xf435,
        EN_NV_ID_LTE_DPD_FAC_STRU_B39                    = 0xf436,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B39            = 0xf437,

    /*BAND28 生产NV*/
        EN_NV_ID_ANT_MODEM_LOSS_B28                     = 0xf440,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B28         = 0xf44D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B28         = 0xf44E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B28       = 0xf44F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B28       = 0xf450,
        EN_NV_ID_LTE_IP2_CAL_B28                        = 0xf451,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B28    = 0xf452,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B28              = 0xf453,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B28             = 0xf454,
        //EN_NV_ID_LTE_PA_POWER_B28                     = 0xf455,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B28        = 0xf456,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B28         = 0xf457,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B28         = 0xf458,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B28                   = 0xf459,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B28                 = 0xf45a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B28                 = 0xf45b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B28                 = 0xf45c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B28               = 0xf45d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B28         = 0xf45e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B28             = 0xf45f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B28               = 0xf465,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B28              = 0xf466,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B28             = 0xf467,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B28          = 0xf468,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B28   = 0xf469,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B28   = 0xf46a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B28  = 0xf46b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B28  = 0xf46c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B28                  = 0xf46d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B28                  = 0xf46e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B28                 = 0xf46f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B28                   = 0xf470,
        EN_NV_ID_ET_FINEDLY_PARA_B28                    = 0xf471,
        EN_NV_ID_ET_VOFFSET_GAIN_B28                    = 0xf472,
        EN_NV_ID_ET_EVDELY_B28                           = 0xf473,
        EN_NV_ID_RF_CA_RCCODE_B28                        = 0xf474,
        /*add for K3V5 START 20141211 */
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B28             = 0xf475,
        EN_NV_ID_LTE_DPD_FAC_STRU_B28                    = 0xf476,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B28            = 0xf477,
        /* BEGIN: Added by  , 2015/6/3   ????:K3V5_AMPR*/
        EN_NV_ID_LTE_TX_AMPR_BNon28                      = 0xf478,
        /* END:   Added by  , 2015/6/3 */

    /*
        EN_NV_ID_ANT_MODEM_LOSS_BNon1                     = 0xf440,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_BNon1         = 0xf44d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_BNon1         = 0xf44e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_BNon1       = 0xf44f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_BNon1       = 0xf450,
        EN_NV_ID_LTE_IP2_CAL_BNon1                        = 0xf451,
        EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon1      = 0xf452,
        EN_NV_ID_LTE_TX_APC_MIDGAIN_FREQ_COMP_BNon1       = 0xf453,
        EN_NV_ID_LTE_TX_APC_LOWGAIN_FREQ_COMP_BNon1       = 0xf454,
        EN_NV_ID_LTE_PA_POWER_BNon1                       = 0xf455,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon1        = 0xf456,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon1         = 0xf457,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon1         = 0xf458,
        */
        /*add for V9R1_6361 Begin*/
    /*    EN_NV_ID_IIP2_CAL_TABLE_BNon1                    = 0xf459,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon1                 = 0xf45a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon1                  = 0xf45b,
        EN_NV_ID_TX_APC_GAIN_BNon1                       = 0xf45c,
        EN_NV_ID_RF_TXIQ_CAL_BNon1                       = 0xf45d,
        EN_NV_ID_PA_POWER_DIFFERENCE_BNon1               = 0xf45e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon1              = 0xf45f,
        */
        /*add for V9R1_6361 End*/
    /*BEGIN     modify for B28全频段特性*/
       /*BAND128 生产NV*/
        EN_NV_ID_ANT_MODEM_LOSS_B128                        = 0xf480,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B128        = 0xf48D,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B128        = 0xf48E,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B128      = 0xf48F,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B128      = 0xf490,
        EN_NV_ID_LTE_IP2_CAL_B128                       = 0xf491,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B128       = 0xf492,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B128     = 0xf493,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B128        = 0xf494,
        //EN_NV_ID_LTE_PA_POWER_B128                        = 0xf495,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B128       = 0xf496,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B128        = 0xf497,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B128        = 0xf498,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_B128                        = 0xf499,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B128                  = 0xf49a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B128                  = 0xf49b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B128                        = 0xf49c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B128                      = 0xf49d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B128                = 0xf49e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B128              = 0xf49f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B128                    = 0xf4a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B128               = 0xf4a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B128              = 0xf4a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B128           = 0xf4a8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B128    = 0xf4a9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B128    = 0xf4aa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B128 = 0xf4ab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B128 = 0xf4ac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B128                   = 0xf4ad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B128                   = 0xf4ae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B128                  = 0xf4af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B128                   = 0xf4b0,
        EN_NV_ID_ET_FINEDLY_PARA_B128                    = 0xf4b1,
        EN_NV_ID_ET_VOFFSET_GAIN_B128                    = 0xf4b2,
        EN_NV_ID_ET_EVDELY_B128                           = 0xf4b3,
        EN_NV_ID_RF_CA_RCCODE_B128                        = 0xf4b4,
        /*add for K3V5 START 20141211 ++  for B128 missing*/
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B128             = 0xf4b5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B128                    = 0xf4b6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B128            = 0xf4b7,
    /*END     modify for B28全频段特性*/

        EN_NV_ID_ANT_MODEM_LOSS_B27                        = 0xf4c0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B27            = 0xf4cd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B27            = 0xf4ce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B27          = 0xf4cf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B27          = 0xf4d0,
        EN_NV_ID_LTE_IP2_CAL_B27                           = 0xf4d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B27       = 0xf4d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B27                   = 0xf4d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B27              = 0xf4d4,
        //EN_NV_ID_LTE_PA_POWER_B27                        = 0xf4d5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B27           = 0xf4d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B27            = 0xf4d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B27            = 0xf4d8,
        EN_NV_ID_IIP2_CAL_TABLE_B27                      = 0xf4d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B27                      = 0xf4da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B27                      = 0xf4db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B27                    = 0xf4dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B27                  = 0xf4dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B27            = 0xf4de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B27                = 0xf4df,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B27                    = 0xf4e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B27                   = 0xf4e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B27                  = 0xf4e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B27               = 0xf4e8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B27        = 0xf4e9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B27        = 0xf4ea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B27      = 0xf4eb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B27      = 0xf4ec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B27                      = 0xf4ed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B27                      = 0xf4ee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B27                     = 0xf4ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B27                      = 0xf4f0,
        EN_NV_ID_ET_FINEDLY_PARA_B27                       = 0xf4f1,
        EN_NV_ID_ET_VOFFSET_GAIN_B27                       = 0xf4f2,
        EN_NV_ID_ET_EVDELY_B27                             = 0xf4f3,
        EN_NV_ID_RF_CA_RCCODE_B27                          = 0xf4f4,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B27               = 0xf4f5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B27                      = 0xf4f6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B27              = 0xf4f7,

        EN_NV_ID_ANT_MODEM_LOSS_B29                        = 0xf500,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B29            = 0xf50d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B29            = 0xf50e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B29          = 0xf50f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B29          = 0xf510,
        EN_NV_ID_LTE_IP2_CAL_B29                           = 0xf511,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B29       = 0xf512,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B29                 = 0xf513,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B29                = 0xf514,
        //EN_NV_ID_LTE_PA_POWER_B29                        = 0xf515,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B29           = 0xf516,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B29            = 0xf517,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B29            = 0xf518,
        EN_NV_ID_IIP2_CAL_TABLE_B29                        = 0xf519,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B29                      = 0xf51a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B29                      = 0xf51b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B29                    = 0xf51c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B29                  = 0xf51d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B29            = 0xf51e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B29                  = 0xf51f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B29                    = 0xf525,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B29                   = 0xf526,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B29                  = 0xf527,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B29               = 0xf528,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B29        = 0xf529,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B29        = 0xf52a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B29      = 0xf52b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B29      = 0xf52c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B29                      = 0xf52d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B29                      = 0xf52e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B29                     = 0xf52f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B29                      = 0xf530,
        EN_NV_ID_ET_FINEDLY_PARA_B29                       = 0xf531,
        EN_NV_ID_ET_VOFFSET_GAIN_B29                       = 0xf532,
        EN_NV_ID_ET_EVDELY_B29                             = 0xf533,
        EN_NV_ID_RF_CA_RCCODE_B29                          = 0xf534,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B29               = 0xf535,
        EN_NV_ID_LTE_DPD_FAC_STRU_B29                      = 0xf536,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B29              = 0xf537,

        EN_NV_ID_ANT_MODEM_LOSS_B30                        = 0xf540,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B30            = 0xf54d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B30            = 0xf54e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B30          = 0xf54f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B30          = 0xf550,
        EN_NV_ID_LTE_IP2_CAL_B30                           = 0xf551,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B30       = 0xf552,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B30                 = 0xf553,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B30                = 0xf554,
        //EN_NV_ID_LTE_PA_POWER_B30                        = 0xf555,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B30           = 0xf556,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B30            = 0xf557,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B30            = 0xf558,
        EN_NV_ID_IIP2_CAL_TABLE_B30                        = 0xf559,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B30                      = 0xf55a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B30                      = 0xf55b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B30                    = 0xf55c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B30                  = 0xf55d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B30            = 0xf55e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B30                  = 0xf55f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B30                    = 0xf565,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B30                   = 0xf566,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B30                  = 0xf567,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B30               = 0xf568,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B30        = 0xf569,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B30        = 0xf56a,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B30      = 0xf56b,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B30      = 0xf56c,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B30                      = 0xf56d,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B30                      = 0xf56e,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B30                     = 0xf56f,
        EN_NV_ID_LTE_ET_GAIN_COMP_B30                      = 0xf570,
        EN_NV_ID_ET_FINEDLY_PARA_B30                       = 0xf571,
        EN_NV_ID_ET_VOFFSET_GAIN_B30                       = 0xf572,
        EN_NV_ID_ET_EVDELY_B30                             = 0xf573,
        EN_NV_ID_RF_CA_RCCODE_B30                          = 0xf574,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B30               = 0xf575,
        EN_NV_ID_LTE_DPD_FAC_STRU_B30                      = 0xf576,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B30              = 0xf577,

        EN_NV_ID_ANT_MODEM_LOSS_B44                        = 0xf580,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B44            = 0xf58d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B44            = 0xf58e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B44          = 0xf58f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B44          = 0xf590,
        EN_NV_ID_LTE_IP2_CAL_B44                           = 0xf591,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B44       = 0xf592,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B44                 = 0xf593,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B44                = 0xf594,
        //EN_NV_ID_LTE_PA_POWER_B44                        = 0xf595,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B44           = 0xf596,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B44            = 0xf597,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B44            = 0xf598,
        EN_NV_ID_IIP2_CAL_TABLE_B44                        = 0xf599,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B44                      = 0xf59a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B44                      = 0xf59b,
        EN_NV_ID_LTE_APC_TABLE_STRU_B44                    = 0xf59c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B44                  = 0xf59d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B44            = 0xf59e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B44                  = 0xf59f,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B44                    = 0xf5a5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B44                   = 0xf5a6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B44                  = 0xf5a7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B44               = 0xf5a8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B44        = 0xf5a9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B44        = 0xf5aa,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B44      = 0xf5ab,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B44      = 0xf5ac,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B44                      = 0xf5ad,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B44                      = 0xf5ae,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B44                     = 0xf5af,
        EN_NV_ID_LTE_ET_GAIN_COMP_B44                      = 0xf5b0,
        EN_NV_ID_ET_FINEDLY_PARA_B44                       = 0xf5b1,
        EN_NV_ID_ET_VOFFSET_GAIN_B44                       = 0xf5b2,
        EN_NV_ID_ET_EVDELY_B44                             = 0xf5b3,
        EN_NV_ID_RF_CA_RCCODE_B44                          = 0xf5b4,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B44               = 0xf5b5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B44                      = 0xf5b6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B44              = 0xf5b7,


        EN_NV_ID_ANT_MODEM_LOSS_B32                        = 0xf5c0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_B32            = 0xf5cd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_B32            = 0xf5ce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_B32          = 0xf5cf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_B32          = 0xf5d0,
        EN_NV_ID_LTE_IP2_CAL_B32                           = 0xf5d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_B32       = 0xf5d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_B32                 = 0xf5d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_B32                = 0xf5d4,
        //EN_NV_ID_LTE_PA_POWER_B32                        = 0xf5d5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_B32           = 0xf5d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_B32            = 0xf5d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_B32            = 0xf5d8,
        EN_NV_ID_IIP2_CAL_TABLE_B32                        = 0xf5d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_B32                      = 0xf5da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_B32                      = 0xf5db,
        EN_NV_ID_LTE_APC_TABLE_STRU_B32                    = 0xf5dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_B32                  = 0xf5dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_B32            = 0xf5de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_B32                  = 0xf5df,
        EN_NV_ID_LTE_TX_PD_AUTO_CAL_B32                    = 0xf5e5,
        EN_NV_ID_LTE_TX_PD_PWR_TABLE_B32                   = 0xf5e6,
        EN_NV_ID_LTE_TX_PD_VOLT_TABLE_B32                  = 0xf5e7,
        EN_NV_ID_LTE_TX_PD_TEMPCMP_TABLE_B32               = 0xf5e8,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT1_B32        = 0xf5e9,
        EN_NV_ID_LTE_AGC_SCC_BLK_FREQ_COMP_ANT2_B32        = 0xf5ea,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT1_B32      = 0xf5eb,
        EN_NV_ID_LTE_AGC_SCC_NOBLK_FREQ_COMP_ANT2_B32      = 0xf5ec,
        EN_NV_ID_ET_LUT_TBL_LOWCH_B32                      = 0xf5ed,
        EN_NV_ID_ET_LUT_TBL_MIDCH_B32                      = 0xf5ee,
        EN_NV_ID_ET_LUT_TBL_HIGHCH_B32                     = 0xf5ef,
        EN_NV_ID_LTE_ET_GAIN_COMP_B32                      = 0xf5f0,
        EN_NV_ID_ET_FINEDLY_PARA_B32                       = 0xf5f1,
        EN_NV_ID_ET_VOFFSET_GAIN_B32                       = 0xf5f2,
        EN_NV_ID_ET_EVDELY_B32                             = 0xf5f3,
        EN_NV_ID_RF_CA_RCCODE_B32                          = 0xf5f4,
        EN_NV_ID_LTE_ETM_ET_APT_FAC_STRU_B32               = 0xf5f5,
        EN_NV_ID_LTE_DPD_FAC_STRU_B32                      = 0xf5f6,
        EN_NV_ID_LTE_ET_DPD_COMP_FAC_STRU_B32              = 0xf5f7,

        EN_NV_ID_ANT_MODEM_LOSS_BNon8                     = 0xf600,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_BNon8         = 0xf60d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_BNon8         = 0xf60e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_BNon8       = 0xf60f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_BNon8       = 0xf610,
        EN_NV_ID_LTE_IP2_CAL_BNon8                        = 0xf611,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon8      = 0xf612,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon8       = 0xf613,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon8       = 0xf614,
        //EN_NV_ID_LTE_PA_POWER_BNon8                       = 0xf615,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon8        = 0xf616,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon8         = 0xf617,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon8         = 0xf618,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon8                    = 0xf619,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon8                 = 0xf61a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon8                  = 0xf61b,
        EN_NV_ID_TX_APC_GAIN_BNon8                       = 0xf61c,
        EN_NV_ID_RF_TXIQ_CAL_BNon8                       = 0xf61d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon8               = 0xf61e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon8              = 0xf61f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon9                     = 0xf640,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_BNon9         = 0xf64d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_BNon9         = 0xf64e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_BNon9       = 0xf64f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_BNon9       = 0xf650,
        EN_NV_ID_LTE_IP2_CAL_BNon9                        = 0xf651,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon9      = 0xf652,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon9       = 0xf653,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon9       = 0xf654,
        //EN_NV_ID_LTE_PA_POWER_BNon9                       = 0xf655,

        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon9        = 0xf656,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon9         = 0xf657,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon9         = 0xf658,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon9                    = 0xf659,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon9                 = 0xf65a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon9                  = 0xf65b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon9                       = 0xf65c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon9                       = 0xf65d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon9               = 0xf65e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon9              = 0xf65f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon10                     = 0xf680,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_BNon10         = 0xf68d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_BNon10         = 0xf68e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_BNon10       = 0xf68f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_BNon10       = 0xf690,
        EN_NV_ID_LTE_IP2_CAL_BNon10                        = 0xf691,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon10      = 0xf692,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon10       = 0xf693,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon10       = 0xf694,
        //EN_NV_ID_LTE_PA_POWER_BNon10                       = 0xf695,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon10        = 0xf696,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon10         = 0xf697,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon10         = 0xf698,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon10                   = 0xf699,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon10                = 0xf69a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon10                 = 0xf69b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon10                      = 0xf69c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon10                      = 0xf69d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon10              = 0xf69e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon10             = 0xf69f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon11                     = 0xf6c0,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_BNon11         = 0xf6cd,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_BNon11         = 0xf6ce,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_BNon11       = 0xf6cf,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_BNon11       = 0xf6d0,
        EN_NV_ID_LTE_IP2_CAL_BNon11                        = 0xf6d1,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon11      = 0xf6d2,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon11       = 0xf6d3,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon11       = 0xf6d4,
        //EN_NV_ID_LTE_PA_POWER_BNon11                       = 0xf6d5,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon11        = 0xf6d6,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon11         = 0xf6d7,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon11         = 0xf6d8,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon11                   = 0xf6d9,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon11                = 0xf6da,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon11                 = 0xf6db,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon11                      = 0xf6dc,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon11                      = 0xf6dd,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon11              = 0xf6de,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon11             = 0xf6df,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon12                     = 0xf700,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_BNon12         = 0xf70d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_BNon12         = 0xf70e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_BNon12       = 0xf70f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_BNon12       = 0xf710,
        EN_NV_ID_LTE_IP2_CAL_BNon12                        = 0xf711,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon12      = 0xf712,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon12       = 0xf713,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon12       = 0xf714,
        //EN_NV_ID_LTE_PA_POWER_BNon12                       = 0xf715,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon12        = 0xf716,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon12         = 0xf717,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon12         = 0xf718,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon12                   = 0xf719,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon12                = 0xf71a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon12                 = 0xf71b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon12                      = 0xf71c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon12                      = 0xf71d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon12              = 0xf71e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon12             = 0xf71f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon13                     = 0xf740,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_BNon13         = 0xf74d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_BNon13         = 0xf74e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_BNon13       = 0xf74f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_BNon13       = 0xf750,
        EN_NV_ID_LTE_IP2_CAL_BNon13                        = 0xf751,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon13      = 0xf752,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon13       = 0xf753,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon13       = 0xf754,
        //EN_NV_ID_LTE_PA_POWER_BNon13                       = 0xf755,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon13        = 0xf756,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon13         = 0xf757,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon13         = 0xf758,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon13                   = 0xf759,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon13                = 0xf75a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon13                 = 0xf75b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon13                      = 0xf75c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon13                      = 0xf75d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon13              = 0xf75e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon13             = 0xf75f,
        /*add for V9R1_6361 End*/

        EN_NV_ID_ANT_MODEM_LOSS_BNon14                     = 0xf780,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT1_BNon14         = 0xf78d,
        EN_NV_ID_LTE_AGC_BLK_FREQ_COMP_ANT2_BNon14         = 0xf78e,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT1_BNon14       = 0xf78f,
        EN_NV_ID_LTE_AGC_NOBLK_FREQ_COMP_ANT2_BNon14       = 0xf790,
        EN_NV_ID_LTE_IP2_CAL_BNon14                        = 0xf791,
        //EN_NV_ID_LTE_TX_APC_HIGHGAIN_FREQ_COMP_BNon14      = 0xf792,
        EN_NV_ID_LTE_HD3_CAL_PARA_STRU_BNon14       = 0xf793,
        EN_NV_ID_LTE_HD3_CAL_RSULT_STRU_BNon14       = 0xf794,
        //EN_NV_ID_LTE_PA_POWER_BNon14                       = 0xf795,
        EN_NV_ID_LTE_TX_APT_COMP_MC_HIGH_TBL_BNon14        = 0xf796,
        EN_NV_ID_LTE_TX_APT_COMP_MC_MID_TBL_BNon14         = 0xf797,
        EN_NV_ID_LTE_TX_APT_COMP_MC_LOW_TBL_BNon14         = 0xf798,
        /*add for V9R1_6361 Begin*/
        EN_NV_ID_IIP2_CAL_TABLE_BNon14                   = 0xf799,
        EN_NV_ID_RF_DCOC_CAL_ANT1_BNon14                = 0xf79a,
        EN_NV_ID_RF_DCOC_CAL_ANT2_BNon14                 = 0xf79b,
        EN_NV_ID_LTE_APC_TABLE_STRU_BNon14                      = 0xf79c,
        EN_NV_ID_LTE_RF_TXIQ_CAL_STRU_BNon14                      = 0xf79d,
        EN_NV_ID_LTE_DEFAULT_POW_TABLE_STRU_BNon14              = 0xf79e,
        EN_NV_ID_TX_RF_FREQ_COMP_STRU_BNon14             = 0xf79f,
        /*add for V9R1_6361 End*/

        /* modify by   for 所有band end*/

        EN_NV_ID_TIMING_PARA                            = 0xD3C0,
        EN_NV_ID_EMU_FAKECELL_PARA                      = 0xD3C1,
        EN_NV_ID_CQI_PARA                               = 0xD3C2,
        EN_NV_ID_ANTCORR_PARA                           = 0xD3C3,
        EN_NV_ID_RLM_PARA                               = 0xD3C4,
        EN_NV_ID_AFC_PARA                               = 0xD3C5,
        EN_NV_ID_IRC_PUB_PARA                           = 0xD3C6,
        EN_NV_ID_CHE_PARA                               = 0xD3C7,
        EN_NV_ID_VITERBI_PARA                           = 0xD3C8,
        EN_NV_ID_TURBO_PARA                             = 0xD3C9,
        EN_NV_ID_DEM_LIST_PARA                          = 0xD3CA,
        EN_NV_ID_AD9361_UL_PARA                         = 0xD3CB,
        EN_NV_ID_HI6360_UL_PARA                         = 0xD3CC,

        /* modify by   begin */
        EN_NV_ID_PHY_FUNC_VERIFY_SWITCH_PARA            = 0xD3E0,
        /* modify by   end */

        /*tcx0*/
        EN_NV_ID_TCXO_DYNAMIC_CONFIG_PARA               = 0xD3E2,

        /* add by  2012-6-8 for TX_FILTER begin */
        EN_NV_ID_TX_FILTER_CMP                          = 0xD3E3,
        /* add by  2012-6-8 for TX_FILTER end */

        EN_NV_ID_LPHY_PD_COMM_PARA                      = 0xD3E5,

        /* BEGIN: Added by  , 2013/3/22 */
        EN_NV_ID_LTE_COM_ANT_SAR_PARA                   = 0xD4FA,
        EN_NV_ID_LTE_BODY_SAR_WIRED_FLAG                = 0xD4FB,
        EN_NV_ID_LVRAMP_PARA                            = 0xD4FC,
        EN_NV_ID_FE_NOTCH_INFO                          = 0xD4FF, /*notch*/
        EN_NV_ID_FE_BASIC_INFO                          = 0xD500,
        EN_NV_ID_FE_RFIC_INIT                           = 0xD501,
        EN_NV_ID_FE_COMM_CONFIG                         = 0xD502,
        EN_NV_ID_6362_RFIC_INIT                          = 0xD503,
        //EN_NV_ID_SBAND_INFO                             = 0xD504,
        //EN_NV_ID_PBAND_MIPI_INFO                        = 0xD505,
        /* END:   Added by  , 2013/3/22 */
        EN_NV_ID_CA_TUNER_INFO                          = 0xD506,

        EN_NV_ID_APC_GAIN_DEFALUT                       = 0xD507,
        EN_NV_ID_PA_POWER_DIFFERENCE_DEFALUT            = 0xD508,
        EN_NV_ID_DSP_NV_PARA_SIZE                       = 0xD509,
        EN_NV_ID_MIPIDEV_INIT                           = 0xD50A,
        EN_NV_ID_LPHY_ET_COMM_PARA                   = 0xD50B,
        EN_NV_ID_LPHY_ET_VENDOR_PARA_TI              = 0xD50C,
        EN_NV_ID_LPHY_ET_VENDOR_PARA_R2              = 0xD50D,
        EN_NV_ID_LPHY_ET_VENDOR_PARA_QU              = 0xD50E,
        EN_NV_ID_RF_CA_RCCAL_CFG                      = 0xD50F,

        /*begin added by   2014/06/23*/
        EN_NV_ID_LPHY_LWCOEX_INIT_PARA                  = 0xD510,
        /*end added by   2014/06/23*/


        EN_NV_ID_ASU_PARA                           = 0xD511,
        EN_NV_SINGLE_XO_DEFINE                      = 0xD51d,

        /* 准入控制NV */
        EN_NV_ID_ACCESS_PARA_PARA                           = 0xD5F0,

#if (defined (FEATURE_TLPHY_SINGLE_XO) || defined (FEATURE_TLPHY_TCXO_OVER_TEMP_PROTECT))
        EN_NV_ID_TL_COMM_NV_PARA_SIZE                = 0xD512,
        EN_NV_ID_DCXO_C_TRIM_DEFAULT                 = 0xD513,
        EN_NV_ID_DCXO_C2_FIX_DEFAULT                 = 0xD514,
        EN_NV_ID_XO_INIT_FREQUENCY                   = 0xD515,
        EN_NV_ID_DCXO_C_TRIM                         = 0xD516,
        EN_NV_ID_DCXO_C2_FIX                         = 0xD517,
        EN_NV_ID_DCXO_TEMP_COMP_POLY_COEF            = 0xD518,
        EN_NV_ID_DCXO_TEMP_COMP_THRESHOLD            = 0xD519,
        EN_NV_ID_DCXO_FREQ_VS_TEMP_ARRAY             = 0xD51a,
        EN_NV_ID_DCXO_TEMP_READ_PERIOD               = 0xD51b,
        EN_NV_ID_DCXO_TEMP_COMP_POLY_ALL             = 0xD51c,
        EN_NV_ID_XO_AGING_INFO                       = 0xF93F,
#endif
        EN_NV_ET_SELECT_WORK_MODE                    = 0xD51E,
        EN_NV_DPD_SELECT_WORK_MODE                   = 0xD51F,
        EN_NV_MIPI_CMD_MODE                          = 0xD520,

        EN_NV_ID_CONVERT_FACTOR                      = 0xD521,

        EN_NV_TERMINAL_TO_DSP                          = 0xD522,

        EN_NV_LTE_DCXO_PPM_UPDATE_THRESHOLD_STRU       = 0xD523,

        EN_NV_RF_LTE_BAND_EXTERNAL_LNA_CFG_PARA        = 0xD524,
        EN_NV_LPHY_MIPI_APT_PARA                       = 0xD525,
        EN_NV_LTE_DCXO_PPM_VAR_THRESHOLD_STRU          = 0xD526,

        EN_NV_ID_TL_PA_GPIO_CTRL                       = 0xd530,
        EN_NV_APT_PDM_HOLD_NTX_STRU                    = 0xD531,

        EN_NV_ID_TDS_HIGH_SPEED_NV_PARA                = 0xD580,
        EN_NV_ID_TDS_TX_POWER_NV_PARA                  = 0xD581,

        EN_NV_ID_HI6360_AGC_PARA_B20                    = 0xD618,
        EN_NV_ID_AD9361_AGC_PARA_B20                    = 0xD619,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B20                = 0xD61A,

        EN_NV_ID_HI6360_AGC_PARA_B41                    = 0xD6d8,
        EN_NV_ID_AD9361_AGC_PARA_B41                    = 0xD6d9,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B41                = 0xD6da,

        EN_NV_ID_HI6360_AGC_PARA_B40                    = 0xD658,
        EN_NV_ID_AD9361_AGC_PARA_B40                    = 0xD659,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B40                = 0xD65A,

        EN_NV_ID_HI6360_AGC_PARA_B38                    = 0xD698,
        EN_NV_ID_AD9361_AGC_PARA_B38                    = 0xD699,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B38                = 0xD69A,

        EN_NV_ID_HI6360_AGC_PARA_B7                     = 0xD718,
        EN_NV_ID_AD9361_AGC_PARA_B7                     = 0xD719,
        EN_NV_ID_LPHY_AGC_BASE_TABLE_B7                 = 0xD71A,

        EN_NV_ID_HI6360_AGC_PARA_B3                     = 0xD817,
        EN_NV_ID_HI6360_AGC_PARA_B1                     = 0xD858,
        EN_NV_ID_HI6360_AGC_PARA_B5                     = 0xD898,
        EN_NV_ID_HI6360_AGC_PARA_B8                     = 0xD8d8,
        EN_NV_ID_HI6360_AGC_PARA_B26                    = 0xe358,
        EN_NV_ID_HI6360_AGC_PARA_B25                     = 0xdd18,
        EN_NV_ID_HI6360_AGC_PARA_B18                     = 0xDc18,
        EN_NV_ID_HI6360_AGC_PARA_B28                    = 0xDf58,/**/
    /*BEGIN     modify for B28全频段特性*/
        EN_NV_ID_HI6360_AGC_PARA_B128                   = 0xDf98,
    /*END     modify for B28全频段特性*/

        EN_NV_ID_LPHY_DSP_VERSION_INFO                  = 0xD818,
        EN_NV_ID_LPHY_DSP_CONFIG_INFO                   = 0xD819,
        EN_NV_ID_MULTIMODE_DSP_COMMON_CONFIG_INFO       = 0xD81A,

        EN_NV_RX_BT_LEVEL_MAP_TABLE                     = 0xD3e1,

        EN_NV_ID_FACTORY_END                            = 0x2fff
    };






    /*****************************************************************************
       5 STRUCT
    *****************************************************************************/




    /*****************************************************************************
      6 UNION
    *****************************************************************************/


    /*****************************************************************************
      7 Extern Global Variable
    *****************************************************************************/


    /*****************************************************************************
      8 Fuction Extern
    *****************************************************************************/


    /*****************************************************************************
      9 OTHERS
    *****************************************************************************/









    
    /*
#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif
    */

#endif

    typedef unsigned long NV_TLPHY_ITEM_ID_ENUM_UINT32;



/*****************************************************************************
   5 STRUCT
*****************************************************************************/




/*****************************************************************************
  6 UNION
*****************************************************************************/


/*****************************************************************************
  7 Extern Global Variable
*****************************************************************************/


/*****************************************************************************
  8 Fuction Extern
*****************************************************************************/


/*****************************************************************************
  9 OTHERS
*****************************************************************************/









/* this file is included by drv */
/*
#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif
*/



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of lt_phy_nv_define.h */

