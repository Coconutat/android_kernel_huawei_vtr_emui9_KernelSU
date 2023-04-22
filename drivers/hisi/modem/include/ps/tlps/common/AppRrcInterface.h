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

#ifndef __APPRRCINTERFACE_H__
#define __APPRRCINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#include  "vos.h"
#include  "PsTypeDef.h"
#include  "LPSCommon.h"

#if (VOS_OS_VER != VOS_WIN32)
#pragma pack(4)
#else
#pragma pack(push, 4)
#endif

/*****************************************************************************
  2 macro
*****************************************************************************/
#define RRC_APP_CELL_MAX_COUNT           (32)
#define RRC_APP_MAX_NUM_OF_MCC           (3)

/*typedef DT_CMD_ENUM_UINT8 APP_CMD_ENUM_UINT8;*/
/*typedef DT_RESULT_ENUM_UINT32 APP_RESULT_ENUM_UINT32;*/

/* RRC发起随机接入到MAC收到rar */
#define PS_OM_EST_PTL_SND_MACRA_TO_RCV_RAR_TIME      (11)           /* 11ms T4-T1 */
/* 从mac收到rar到发送消息3时间 */
#define PS_OM_EST_PTL_RCV_RAR_TO_SND_MSG3_TIME       (3)            /* 2.5ms T5-T4*/
/* 发送消息3到收到setup消息 */
#define PS_OM_EST_PTL_SND_MSG3_TO_RCV_SETUP_TIME     (73)           /* 28.5ms+2*Ts1c  T6-T5*/
/* 处理setup消息协议时间 */
#define PS_OM_EST_PTL_SETUP_TO_CMPL_TIME             (3)             /* 3ms T7-T6*/
/* 发起随机接入到发送setup cmpl协议时间 */
#define PS_OM_EST_PTL_RRC_SND_MACRA_TO_CMPL_TIME     (92)            /* 47.5ms+2*Ts1c--(50.5~92.5ms) T7-T1*/
/* 建链用户面时延协议值 */
#define PS_OM_EST_PTL_USER_PLANE_TIME                (106)            /* 61ms+2*Ts1c+Ts1u--66~106ms T13-T1*/

/* 小区搜索协议时间 */
#define PS_OM_REEST_PTL_SEARCH_CELL_TIME             (100)            /* <100ms */
/* 系统消息更新协议时间 */
#define PS_OM_REEST_PTL_RCV_SI_TIME                  (1280)           /* <1280ms */
/* 检测到链路失败到在新小区上发起随机接入协议时间 */
#define PS_OM_REEST_PTL_RCV_REEST_IND_TO_MACRA       (1500)           /* <1500ms */

/*  物理层切换开始到结束协议时间 */
#define PS_OM_HO_PTL_HO_REQ_TO_CNF                   (1)              /* <1ms T6-T5*/

/* 随机接入请求到收到Cnf的协议时间 */
#define PS_OM_HO_PTL_RRC_MACRA_REQ_TO_MACRA_CNF      (11)             /* 2.5 + 1+ 7.5ms T8-T7*/

/* 发送切换完成消息到收到确认消息协议时间 */
#define PS_OM_HO_PTL_SND_CMPL_TO_CMPL_CNF            (13)             /* 13ms T10-T9*/

/* 收到切换重配消息到发起随机接入协议时间 */
#define PS_OM_HO_PTL_RCV_RBCFG_TO_MACRA_REQ          (50)             /* 50ms T7-T4*/

/* 收到切换重配置消息到回复数传用户面时延协议时间 */
#define PS_OM_HO_PTL_USER_PLANE_TIME                 (100)            /* 100ms T11-T4*/

/* add for AT&T LRRC DAM begin */
/* 受限列表中保存最大的条目数 */
#define RRC_APP_MAX_LIMITED_ITEM_COUNT         (32)
/* add for AT&T LRRC DAM end */
/* 新增测量消息中上报小区最大个数 */
#define LRRC_LCSEL_INTRA_CELL_REPORT_NUM 4
#define LRRC_LCSEL_INTER_CELL_REPORT_NUM 4
#define LRRC_LCSEL_UTRAN_CELL_REPORT_NUM 4
#define LRRC_LCSEL_INTER_FREQ_REPORT_NUM 9

#define LRRC_SCELL_MAX_NUM 3
#define LRRC_INTRA_CELL_MAX_NUM 16
#define LRRC_INTER_CELL_MAX_NUM 16
#define LRRC_UTRAN_CELL_MAX_NUM 16
#define LRRC_GERAN_CELL_MAX_NUM 16

/* UE支持的UTRAN频点的最大测量数目 */
#define LRRC_TRRC_PHY_MAX_SUPPORT_CARRIER_NUM               9

#define LRRC_GURRC_GERAN_ARFCN_MAX_NUM                      32

/* 存放BAND信息最大的数组 */
#define LRRC_APP_MAX_BAND_IND_ARRAY_NUM                     (8)

#define LRRC_APP_LWCLASH_MAX_SCELL_NUM                      (4)


/*****************************************************************************
  3 Massage Declare
*****************************************************************************/
/*****************************************************************************
 枚举名    : APP_RRC_MSG_ID_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : RRC <-> APP 接口消息ID
*****************************************************************************/
enum APP_RRC_MSG_ID_ENUM
{
    /* RRC发给APP的原语 */
    ID_APP_RRC_TRANSPARENT_CMD_REQ      = (PS_MSG_ID_APP_TO_RRC_BASE + 0x00),   /* _H2ASN_MsgChoice APP_RRC_TRANSPARENT_CMD_REQ_STRU */
    ID_APP_RRC_CELL_MEAS_RPT_REQ        = (PS_MSG_ID_APP_TO_RRC_BASE + 0x01),   /* _H2ASN_MsgChoice APP_RRC_CELL_MEAS_RPT_REQ_STRU */
    ID_APP_RRC_TIME_DELAY_RPT_REQ       = (PS_MSG_ID_APP_TO_RRC_BASE + 0x02),   /* _H2ASN_MsgChoice APP_RRC_TIME_DELAY_RPT_REQ_STRU */
    ID_APP_RRC_INQ_CAMP_CELL_INFO_REQ   = (PS_MSG_ID_APP_TO_RRC_BASE + 0x03),   /* _H2ASN_MsgChoice APP_RRC_INQ_CAMP_CELL_INFO_REQ_STRU */
    ID_APP_RRC_LOCK_WORK_INFO_REQ       = (PS_MSG_ID_APP_TO_RRC_BASE + 0x04),   /* _H2ASN_MsgChoice APP_RRC_LOCK_WORK_INFO_REQ_STRU */
    ID_APP_RRC_CSQ_REQ                  = (PS_MSG_ID_APP_TO_RRC_BASE + 0x05),   /* _H2ASN_MsgChoice APP_RRC_CSQ_REQ_STRU */
    ID_APP_RRC_PTL_STATE_QUERY_REQ      = (PS_MSG_ID_APP_TO_RRC_BASE + 0x06),   /* _H2ASN_MsgChoice APP_RRC_PTL_STATE_QUERY_REQ_STRU */
    ID_APP_RRC_CELL_INFO_QUERY_REQ      = (PS_MSG_ID_APP_TO_RRC_BASE + 0x07),   /* _H2ASN_MsgChoice APP_RRC_CELL_INFO_QUERY_REQ_STRU */
    ID_APP_RRC_LWCLASH_REQ              = (PS_MSG_ID_APP_TO_RRC_BASE + 0x08),   /* _H2ASN_MsgChoice APP_RRC_PTL_LWCLASH_REQ_STRU */
    /* DT begin */
    ID_APP_RRC_SERVING_CELL_INFO_QUERY_REQ   = (PS_MSG_ID_APP_TO_RRC_BASE + 0x09),   /* _H2ASN_MsgChoice APP_RRC_CELL_INFO_QUERY_REQ_STRU */
    ID_APP_RRC_CSEL_INFO_QUERY_REQ           = (PS_MSG_ID_APP_TO_RRC_BASE + 0x0a),   /* _H2ASN_MsgChoice APP_RRC_CELL_INFO_QUERY_REQ_STRU */
    ID_APP_RRC_UE_CAP_INFO_QUERY_REQ         = (PS_MSG_ID_APP_TO_RRC_BASE + 0x0b),   /* _H2ASN_MsgChoice APP_RRC_CELL_INFO_QUERY_REQ_STRU */
    ID_APP_RRC_AC_BARRING_INFO_QUERY_REQ     = (PS_MSG_ID_APP_TO_RRC_BASE + 0x0c),   /* _H2ASN_MsgChoice APP_RRC_CELL_INFO_QUERY_REQ_STRU */
    ID_APP_RRC_DRX_INFO_QUERY_REQ            = (PS_MSG_ID_APP_TO_RRC_BASE + 0x0d),   /* _H2ASN_MsgChoice APP_RRC_CELL_INFO_QUERY_REQ_STRU */

    /* DT end */
    ID_APP_RRC_NMR_REQ                  = (PS_MSG_ID_APP_TO_RRC_BASE + 0x0e),   /* _H2ASN_MsgChoice APP_RRC_NMR_REQ_STRU */
    ID_APP_RRC_CELLID_REQ               = (PS_MSG_ID_APP_TO_RRC_BASE + 0x0f),   /* _H2ASN_MsgChoice APP_RRC_CELLID_REQ_STRU */

    /* dcom-resel-cfg */
    ID_APP_LRRC_RESEL_OFFSET_CFG_NTF         = (PS_MSG_ID_APP_TO_RRC_BASE + 0x10),   /* _H2ASN_MsgChoice APP_LRRC_RESEL_OFFSET_CFG_NTF_STRU  */
    ID_APP_LRRC_CON_TO_IDLE_NTF          = (PS_MSG_ID_APP_TO_RRC_BASE + 0x11),   /* _H2ASN_MsgChoice APP_LRRC_CON_TO_IDLE_NTF_STRU  */
    /* fast-dorm-cfg */
    ID_APP_LRRC_FAST_DORM_CFG_NTF          = (PS_MSG_ID_APP_TO_RRC_BASE + 0x12),   /* _H2ASN_MsgChoice APP_LRRC_FAST_DORM_CFG_NTF_STRU  */
    ID_APP_LRRC_GET_NCELL_INFO_REQ      = (PS_MSG_ID_APP_TO_RRC_BASE + 0x13),   /* _H2ASN_MsgChoice APP_LRRC_GET_NCELL_INFO_REQ_STRU  */

    /* begin:V7R2-DT 移植强制切换，重选和禁止限制小区接入等功能,2014/3/26 */
    /* Prob Begin */
    ID_APP_LRRC_INQ_TCFG_TXPOWER_REQ    = (PS_MSG_ID_APP_TO_RRC_BASE + 0x14),   /* _H2ASN_MsgChoice APP_RRC_INQ_TCFG_TXPOWER_REQ_STRU */
    /* Prob End */
    ID_APP_RRC_FORCE_HOANDCSEL_REQ        = (PS_MSG_ID_APP_TO_RRC_BASE + 0x15),/*_H2ASN_MsgChoice APP_RRC_FORCE_HOANDCSEL_REQ_STRU*/
    ID_APP_RRC_BARCELL_ACCESS_REQ        = (PS_MSG_ID_APP_TO_RRC_BASE + 0x16),/*_H2ASN_MsgChoice APP_RRC_BARCELL_ACCESS_REQ_STRU*/
    /* end:V7R2-DT 移植强制切换，重选和禁止限制小区接入等功能,2014/3/26 */
    ID_APP_LRRC_SET_UE_REL_VERSION_REQ      = (PS_MSG_ID_APP_TO_RRC_BASE + 0x17), /* _H2ASN_MsgChoice APP_LRRC_SET_UE_REL_VERSION_REQ_STRU  */

    /* begin:add for 路测融合 */
    ID_DT_LRRC_MEAS_REPORT_REQ         = (PS_MSG_ID_APP_TO_RRC_BASE + 0x18),
    ID_DT_LRRC_SYNC_REPORT_REQ         = (PS_MSG_ID_APP_TO_RRC_BASE + 0x19),
    /* end:add for 路测融合 */

    /* begin: add for at&t ue cap display */
    ID_APP_LRRC_GET_UE_CAP_INFO_REQ    = (PS_MSG_ID_APP_TO_RRC_BASE + 0x1a),   /* _H2ASN_MsgChoice APP_LRRC_GET_UE_CAP_INFO_REQ_STRU */
    /* end:   add for at&t ue cap display */

    ID_APP_LRRC_SET_TLPS_PRINT2LAYER_REQ    = (PS_MSG_ID_RRC_TO_APP_BASE + 0x18),

    /* APP发给RRC的原语 */
    ID_RRC_APP_TRANSPARENT_CMD_CNF      = (PS_MSG_ID_RRC_TO_APP_BASE + 0x00),   /* _H2ASN_MsgChoice RRC_APP_TRANSPARENT_CMD_CNF_STRU */
    ID_RRC_APP_TRANSPARENT_CMD_IND      = (PS_MSG_ID_RRC_TO_APP_BASE + 0x01),   /* _H2ASN_MsgChoice NULL */
    ID_RRC_APP_CELL_MEAS_RPT_CNF        = (PS_MSG_ID_RRC_TO_APP_BASE + 0x02),   /* _H2ASN_MsgChoice RRC_APP_CELL_MEAS_RPT_CNF_STRU */
    ID_RRC_APP_CELL_MEAS_RPT_IND        = (PS_MSG_ID_RRC_TO_APP_BASE + 0x03),   /* _H2ASN_MsgChoice RRC_APP_CELL_MEAS_REPORT_IND_STRU */
    ID_RRC_APP_TIME_DELAY_RPT_CNF       = (PS_MSG_ID_RRC_TO_APP_BASE + 0x04),   /* _H2ASN_MsgChoice RRC_APP_TIME_DELAY_RPT_CNF_STRU */
    ID_RRC_APP_TIME_DELAY_RPT_IND       = (PS_MSG_ID_RRC_TO_APP_BASE + 0x05),   /* _H2ASN_MsgChoice RRC_APP_TIME_DELAY_RPT_IND_STRU */
    ID_RRC_APP_INQ_CAMP_CELL_INFO_CNF   = (PS_MSG_ID_RRC_TO_APP_BASE + 0x06),   /* _H2ASN_MsgChoice RRC_APP_INQ_CAMP_CELL_INFO_CNF_STRU */
    ID_RRC_APP_INQ_CAMP_CELL_INFO_IND   = (PS_MSG_ID_RRC_TO_APP_BASE + 0x07),   /* _H2ASN_MsgChoice RRC_APP_INQ_CAMP_CELL_INFO_IND_STRU */
    ID_RRC_APP_LOCK_WORK_INFO_CNF       = (PS_MSG_ID_RRC_TO_APP_BASE + 0x08),   /* _H2ASN_MsgChoice RRC_APP_LOCK_WORK_INFO_CNF_STRU */
    ID_RRC_APP_RPT_DBG_INFO_IND         = (PS_MSG_ID_RRC_TO_APP_BASE + 0x09),   /* _H2ASN_MsgChoice RRC_APP_DEBUG_INFO_STRU */
    ID_RRC_APP_CSQ_CNF                  = (PS_MSG_ID_RRC_TO_APP_BASE + 0x0a),   /* _H2ASN_MsgChoice RRC_APP_CSQ_CNF_STRU */
    ID_RRC_APP_CSQ_IND                  = (PS_MSG_ID_RRC_TO_APP_BASE + 0x0b),   /* _H2ASN_MsgChoice RRC_APP_CSQ_IND_STRU */
    ID_RRC_APP_PTL_STATE_QUERY_CNF      = (PS_MSG_ID_RRC_TO_APP_BASE + 0x0c),   /* _H2ASN_MsgChoice RRC_APP_PTL_STATE_QUERY_CNF_STRU */
    ID_RRC_APP_PTL_STATE_CHANGE_IND     = (PS_MSG_ID_RRC_TO_APP_BASE + 0x0d),   /* _H2ASN_MsgChoice RRC_APP_PTL_STATE_CHANGE_IND_STRU */
    ID_RRC_APP_CELL_INFO_QUERY_CNF      = (PS_MSG_ID_RRC_TO_APP_BASE + 0x0e),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_LWCLASH_CNF              = (PS_MSG_ID_RRC_TO_APP_BASE + 0x0f),   /* _H2ASN_MsgChoice RRC_APP_PTL_LWCLASH_IND_STRU */
    ID_RRC_APP_LWCLASH_IND              = (PS_MSG_ID_RRC_TO_APP_BASE + 0x10),   /* _H2ASN_MsgChoice RRC_APP_PTL_LWCLASH_CNF_STRU */
    /* DT begin */
    ID_RRC_APP_SERVING_CELL_INFO_QUERY_CNF    = (PS_MSG_ID_RRC_TO_APP_BASE + 0x11),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_SERVING_CELL_INFO_IND    = (PS_MSG_ID_RRC_TO_APP_BASE + 0x12),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_CSEL_INFO_QUERY_CNF      = (PS_MSG_ID_RRC_TO_APP_BASE + 0x13),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_LTE_CSEL_INFO_IND        = (PS_MSG_ID_RRC_TO_APP_BASE + 0x14),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */

    ID_RRC_APP_UE_CAP_INFO_QUERY_CNF      = (PS_MSG_ID_RRC_TO_APP_BASE + 0x15),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_UE_CAP_INFO_IND            = (PS_MSG_ID_RRC_TO_APP_BASE + 0x16),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_AC_BARRING_INFO_QUERY_CNF  = (PS_MSG_ID_RRC_TO_APP_BASE + 0x17),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_AC_BARRING_INFO_IND        = (PS_MSG_ID_RRC_TO_APP_BASE + 0x18),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */

    ID_RRC_APP_UTRA_CSEL_INFO_IND         = (PS_MSG_ID_RRC_TO_APP_BASE + 0x19),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_GERAN_CSEL_INFO_IND        = (PS_MSG_ID_RRC_TO_APP_BASE + 0x1a),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_DRX_INFO_QUERY_CNF         = (PS_MSG_ID_RRC_TO_APP_BASE + 0x1b),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    ID_RRC_APP_DRX_INFO_IND               = (PS_MSG_ID_RRC_TO_APP_BASE + 0x1c),   /* _H2ASN_MsgChoice RRC_APP_CELL_INFO_QUERY_CNF_STRU */
    /* DT end */
    ID_RRC_APP_NMR_CNF                    = (PS_MSG_ID_RRC_TO_APP_BASE + 0x1d),   /* _H2ASN_MsgChoice RRC_APP_NMR_CNF_STRU */
    ID_RRC_APP_CELLID_CNF                 = (PS_MSG_ID_RRC_TO_APP_BASE + 0x1e),   /* _H2ASN_MsgChoice RRC_APP_CELLID_CNF_STRU */
    ID_RRC_APP_GET_NCELL_INFO_CNF       = (PS_MSG_ID_RRC_TO_APP_BASE + 0x1f), /*_H2ASN_MsgChoice LRRC_APP_GET_NCELL_INFO_CNF_STRU*/

    ID_LRRC_APP_SET_UE_REL_VERSION_CNF      = (PS_MSG_ID_RRC_TO_APP_BASE + 0x40),/* _H2ASN_MsgChoice LRRC_APP_SET_UE_REL_VERSION_CNF_STRU  */
   /* begin:V7R2-DT add dt cnf msg,2014/5/21 */
    ID_APP_RRC_FORCE_HOANDCSEL_CNF        = (PS_MSG_ID_RRC_TO_APP_BASE + 0x41),/* _H2ASN_MsgChoice RRC_APP_FORCE_HOANDCSEL_CNF_STRU */
    ID_APP_RRC_BARCELL_ACCESS_CNF        = (PS_MSG_ID_RRC_TO_APP_BASE + 0x42),/* _H2ASN_MsgChoice RRC_APP_BARCELL_ACCESS_CNF_STRU */
    /* end:V7R2-DT add dt cnf msg,2014/5/21 */

    /*Print2Layer */
    ID_LRRC_APP_SET_TLPS_PRINT2LAYER_CNF = (PS_MSG_ID_RRC_TO_APP_BASE + 0x47),

    /* begin:add for 路测融合 */
    ID_LRRC_DT_MEAS_REPORT_CNF   = (PS_MSG_ID_RRC_TO_APP_BASE + 0x48),
    ID_LRRC_DT_SYNC_REPORT_CNF   = (PS_MSG_ID_RRC_TO_APP_BASE + 0x49),
    ID_LRRC_DT_MEAS_INFO_IND     = (0x988),                    /* 根据对外接口，路测测量Ind消息id为0x988，等于id_lrrc_lphy_conn_meas_ind */
    ID_LRRC_DT_SYNC_INFO_IND     = (0x904),                    /* 根据对外接口，路测测量Ind消息id为0x904，等于id_lrrc_lphy_sync_info_ind */
    ID_LRRC_DT_OUT_OF_SYNC_INFO_IND = (0x90f),                 /* 根据对外接口，路测测量Ind消息id为0x90f，等于id_lrrc_lphy_out_of_sync_info_ind */
    /* end:add for 路测融合 */

    /* V7R2-DT, report pcell and scell basic information, 2014-3-25 begin*/
    ID_RRC_APP_SERVING_CELL_CA_INFO_IND          = (PS_MSG_ID_RRC_TO_DT_BASE + 0x12),
    /* V7R2-DT, report pcell and scell basic information, 2014-3-25 begin*/

    ID_LRRC_APP_GET_UE_CAP_INFO_CNF       = (PS_MSG_ID_RRC_TO_APP_BASE + 0x4a), /* _H2ASN_MsgChoice LRRC_APP_GET_UE_CAP_INFO_CNF_STRU */

    ID_LRRC_APP_FGI_INFO_IND              = (PS_MSG_ID_RRC_TO_APP_BASE + 0x4b), /* _H2ASN_MsgChoice LRRC_APP_FGI_INFO_IND_STRU */

    ID_APP_RRC_MSG_ID_BUTT
};
typedef VOS_UINT32    APP_RRC_MSG_ID_ENUM_UINT32;

/*****************************************************************************
  4 Enum
*****************************************************************************/
/*****************************************************************************
 结构名    : DT_CMD_ENUM
 结构说明  : DT命令枚举
*****************************************************************************/
enum DT_CMD_ENUM
{
    DT_CMD_STOP,        /*停止*/
    DT_CMD_START,       /*启动*/
    DT_CMD_BUTT
};
typedef VOS_UINT8 DT_CMD_ENUM_UINT8;

/*****************************************************************************
 结构名    : DT_RESULT_ENUM
 结构说明  : DT操作结果枚举
*****************************************************************************/
enum DT_RESULT_ENUM
{
    DT_RESULT_SUCC = 0,
    DT_RESULT_FAIL,
    DT_RESULT_NOT_SUPPORT_MEASURE,                /*当前不支持的测量*/

    /**********各自模块分别在下面增加需要的结果值********/
    /*RRC模式的结果值定义Begin*/
    DT_RESULT_RRC_BEGIN = 0x1000,

    /*RRC模式的结果值定义End*/

    /*NAS模式的结果值定义Begin*/
    DT_RESULT_NAS_BEGIN                 = 0x2000,
    DT_RESULT_NAS_PLMN_LOCK             = 0x2001,
    DT_RESULT_NAS_PLMN_UNLOCK           = 0x2002,

    /*NAS模式的结果值定义End*/

    /*L2模式的结果值定义Begin*/
   DT_RESULT_L2_BEGIN = 0x3000,

    /*L2模式的结果值定义End*/

   /*RRU模式的结果值定义Begin*/
   DT_RESULT_RRU_BEGIN = 0x4000,

   /*RRU模式的结果值定义End*/

    DT_RESULT_BUTT
};
typedef VOS_UINT32 DT_RESULT_ENUM_UINT32;

typedef DT_CMD_ENUM_UINT8       APP_CMD_ENUM_UINT8;
typedef DT_RESULT_ENUM_UINT32   APP_RESULT_ENUM_UINT32;
/*****************************************************************************
 枚举名    : APP_RRC_CELL_RPT_PERIOD_ENUM
 枚举说明  :
*****************************************************************************/
enum APP_RRC_CELL_RPT_PERIOD_ENUM
{
    APP_RRC_CELL_REPORT_PERIOD_300MS = 0,
    APP_RRC_CELL_REPORT_PERIOD_600MS,
    APP_RRC_CELL_REPORT_PERIOD_900MS,
    APP_RRC_CELL_REPORT_PERIOD_1200MS,
    APP_RRC_CELL_REPORT_PERIOD_BUTT
};
typedef VOS_UINT8 APP_RRC_CELL_RPT_PERIOD_ENUM_UINT8;

/*****************************************************************************
 枚举名    : APP_LATENCY_TYPE_ENUM
 枚举说明  :
*****************************************************************************/
enum APP_LATENCY_TYPE_ENUM
{
    APP_LATENCY_EST =0,               /*连接建立时延*/
    APP_LATENCY_HO,                   /*切换时延*/
    APP_LATENCY_REEST,                /*重建时延*/
    APP_LATENCY_BUTT
};
typedef VOS_UINT8 APP_LATENCY_TYPE_ENUM_UINT8;

/*****************************************************************************
 枚举名    : APP_RRC_LOCK_WORK_INFO_TYPE_ENUM
 枚举说明  : 锁定的类型
*****************************************************************************/
enum APP_RRC_LOCK_WORK_INFO_TYPE_ENUM
{
    RRC_APP_LOCK_FREQPOINT,              /* 频点锁定 */
    RRC_APP_LOCK_FREQANDCELL,            /* 频点和小区联合锁定 */
    RRC_APP_LOCK_FREQBAND,               /* 频带锁定 */
    RRC_APP_LOCK_BUTT
};
typedef VOS_UINT8 APP_RRC_LOCK_WORK_INFO_TYPE_ENUM_UINT8;
/*****************************************************************************
 枚举名    : APP_DELAY_TYPE_ENUM
 枚举说明  :
*****************************************************************************/
enum APP_DELAY_TYPE_ENUM
{
    APP_DELAY_CONTROL_PLANE =0,     /*控制面时延*/
    APP_DELAY_USER_PLANE,           /*用户面时延*/
    APP_DELAY_HO,                   /*切换时延*/
    APP_DELAY_BUTT
};
typedef VOS_UINT8 APP_DELAY_TYPE_ENUM_UINT8;
/*****************************************************************************
 枚举名    : APP_CAMPED_FLAG_ENUM
 枚举说明  :
*****************************************************************************/
enum APP_CAMPED_FLAG_ENUM
{
    APP_CAMPED,                 /* 已驻留 */
    APP_NOT_CAMPED,             /* 未驻留 */
    APP_CAMPED_BUTT
};
typedef VOS_UINT8 APP_CAMPED_FLAG_ENUM_UINT8;
/*****************************************************************************
 枚举名    : APP_DELAY_TYPE_ENUM
 枚举说明  :
*****************************************************************************/
enum APP_STATE_FLAG_ENUM
{
    APP_STATE_NOT_IN_RANGE,                 /* 不在冲突范围内,状态2 */
    APP_STATE_IN_RANGE,                     /* 在冲突范围内,状态1*/
    APP_STATE_BUTT
};
typedef VOS_UINT8 APP_STATE_FLAG_ENUM_UINT8;

/* begin: add for at&t ue cap display */
/*****************************************************************************
 枚举名    : LRRC_CA_CC_NUM_ENUM
 枚举说明  :
*****************************************************************************/
enum LRRC_CA_CC_NUM_ENUM
{
    LRRC_CA_CC_NUM_2,                             /* 代表最大支持CA CC数是2CC */
    LRRC_CA_CC_NUM_3,                             /* 代表最大支持CA CC数是3CC */
    LRRC_CA_CC_NUM_4,                             /* 代表最大支持CA CC数是4CC */
    LRRC_CA_CC_NUM_BUTT
};
typedef VOS_UINT8 LRRC_CA_CC_NUM_ENUM_UINT8;
/* end:   add for at&t ue cap display */

/*****************************************************************************
   5 STRUCT
*****************************************************************************/
/*****************************************************************************
 结构名    : APP_OM_MSG_STRU
 结构说明  : APP(后台工具)与OM交互的消息体
*****************************************************************************/
typedef struct
{
     VOS_MSG_HEADER                     /*VOS头
 */
     VOS_UINT32          ulMsgId;
     APP_MSG_HEADER                     /*APP头
 */
     VOS_UINT8           aucPara[4];    /* 参数内容 */
}APP_OM_MSG_STRU;

/*****************************************************************************
 结构名    : APP_PLMN_ID_STRU
 结构说明  :
    MCC, Mobile country code (aucPlmnId[0], aucPlmnId[1] bits 1 to 4)
    MNC, Mobile network code (aucPlmnId[2], aucPlmnId[1] bits 5 to 8).

    The coding of this field is the responsibility of each administration but BCD
    coding shall be used. The MNC shall consist of 2 or 3 digits. For PCS 1900 for NA,
    Federal regulation mandates that a 3-digit MNC shall be used. However a network
    operator may decide to use only two digits in the MNC over the radio interface.
    In this case, bits 5 to 8 of octet 4 shall be coded as "1111". Mobile equipment
    shall accept MNC coded in such a way.

    ---------------------------------------------------------------------------
                 ||(BIT8)|(BIT7)|(BIT6)|(BIT5)|(BIT4)|(BIT3)|(BIT2)|(BIT1)
    ---------------------------------------------------------------------------
    aucPlmnId[0] ||    MCC digit 2            |           MCC digit 1
    ---------------------------------------------------------------------------
    aucPlmnId[1] ||    MNC digit 3            |           MCC digit 3
    ---------------------------------------------------------------------------
    aucPlmnId[2] ||    MNC digit 2            |           MNC digit 1
    ---------------------------------------------------------------------------

    AT命令：
    at+cops=1,2,"mcc digit 1, mcc digit 2, mcc digit 3, mnc digit 1, mnc digit 2, mnc

digit 3",2 :

    e.g.
    at+cops=1,2,"789456",2 :
    --------------------------------------------------------------------------------
    (mcc digit 1)|(mcc digit 2)|(mcc digit 3)|(mnc digit 1)|(mnc digit 2)|(mnc digit 3)
    --------------------------------------------------------------------------------
       7         |     8       |      9      |     4       |      5      |     6
    --------------------------------------------------------------------------------

    在aucPlmnId[3]中的存放格式:
    ---------------------------------------------------------------------------
                 ||(BIT8)|(BIT7)|(BIT6)|(BIT5)|(BIT4)|(BIT3)|(BIT2)|(BIT1)
    ---------------------------------------------------------------------------
    aucPlmnId[0] ||    MCC digit 2 = 8        |           MCC digit 1 = 7
    ---------------------------------------------------------------------------
    aucPlmnId[1] ||    MNC digit 3 = 6        |           MCC digit 3 = 9
    ---------------------------------------------------------------------------
    aucPlmnId[2] ||    MNC digit 2 = 5        |           MNC digit 1 = 4
    ---------------------------------------------------------------------------

*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucPlmnId[3];
    VOS_UINT8                           ucRsv;
} APP_PLMN_ID_STRU;

/*****************************************************************************
 结构名    : APP_RRC_CELL_MEAS_RPT_REQ_STRU
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgID;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_CMD_ENUM_UINT8                  enCommand;
    APP_RRC_CELL_RPT_PERIOD_ENUM_UINT8  enPeriod;
    VOS_UINT8                           aucReserved[2];
}APP_RRC_CELL_MEAS_RPT_REQ_STRU;

/*****************************************************************************
 结构名    : RRC_APP_CELL_MEAS_RPT_CNF_STRU
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
}RRC_APP_CELL_MEAS_RPT_CNF_STRU;

/*****************************************************************************
 结构名    :RRC_APP_CELL_MEAS_RSLT_STRU
 结构说明  :小区测量结果
*****************************************************************************/
typedef struct
{
    VOS_UINT16 usCellId;
    VOS_INT16  sRSRP;
    VOS_INT16  sRSRQ;
    VOS_INT16  sRSSI;
}RRC_APP_CELL_MEAS_RSLT_STRU;

/*****************************************************************************
 结构名    :RRC_APP_CELL_MEAS_REPORT_IND_STRU
 结构说明  :RRC通过此原语周期上报小区的能量测量结果。
            当前存在同频测量时，RRC上报小区的测量结果；不存在，不上报数据；
            如果当前不存在同频测量，但是网络侧之后发送了同频测量，RRC根据之前的OM设置进行上报小区测量结果。
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    VOS_UINT8                           ucCellCnt;                              /* 小区数量 */
    VOS_UINT8                           aucReserved[3];
    RRC_APP_CELL_MEAS_RSLT_STRU         astCellMeasInd[RRC_APP_CELL_MAX_COUNT];
}RRC_APP_CELL_MEAS_REPORT_IND_STRU;

/*****************************************************************************
 结构名    : APP_RRC_TIME_DELAY_RPT_REQ_STRU
 结构说明  : 要求上报控制面时延。控制面时延：信令RB建立的开销时间，即发起连接建立请求到RB1建立成功的时间。
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_CMD_ENUM_UINT8                  enCommand;
    APP_DELAY_TYPE_ENUM_UINT8           enDelayType;
    VOS_UINT8                           aucReserved[2];
}APP_RRC_TIME_DELAY_RPT_REQ_STRU;

/*****************************************************************************
 结构名    :RRC_APP_TIME_DELAY_RPT_CNF_STRU
 结构说明  :对原语APP_RRC_TIME_DELAY_RPT_REQ_STRU的回复
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_DELAY_TYPE_ENUM_UINT8           enDelayType;
    VOS_UINT8                           aucReserved[3];
    APP_RESULT_ENUM_UINT32              enResult;
}RRC_APP_TIME_DELAY_RPT_CNF_STRU;

/*****************************************************************************
 结构名    :APP_RRC_INQ_CAMP_CELL_INFO_REQ_STRU
 结构说明  :获取当前驻留小区的ID、频点等信息的查询请求，或者停止上报驻留小区信息的请求。
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_CMD_ENUM_UINT8                  enCommand;
    VOS_UINT8                           aucReserved[3];
}APP_RRC_INQ_CAMP_CELL_INFO_REQ_STRU;

/*****************************************************************************
 结构名    :RRC_APP_INQ_CAMP_CELL_INFO_CNF_STRU
 结构说明  :对原语APP_RRC_INQ_CAMP_CELL_INFO_REQ_STRU的回复
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
}RRC_APP_INQ_CAMP_CELL_INFO_CNF_STRU;

/*****************************************************************************
 结构名    :RRC_APP_INQ_CAMP_CELL_INFO_IND_STRU
 结构说明  :上报当前驻留小区信息
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    PS_BOOL_ENUM_UINT8                  enValueFlag;
    VOS_UINT8                           aucReserved[3];
    VOS_UINT16                          usCellId;
    VOS_UINT16                          usFreqInfo;
}RRC_APP_INQ_CAMP_CELL_INFO_IND_STRU;

/*****************************************************************************
 结构名    :APP_RRC_FREQ_BAND_STRU
 结构说明  :频带范围
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usLowBand;                              /* 频点下限 */
    VOS_UINT16                          usHighBand;                             /* 频点上限 */
}APP_RRC_FREQ_BAND_STRU;

/*****************************************************************************
 结构名    :APP_RRC_CELL_INFO_STRU
 结构说明  :小区信息
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCellId;                               /* 小区ID */
    VOS_UINT16                          usFreqPoint;                            /* 频点信息 */
}APP_RRC_CELL_INFO_STRU;

/*****************************************************************************
 结构名    :APP_RRC_LOCK_INFO_STRU
 结构说明  :锁定的相关信息
*****************************************************************************/
typedef struct
{
    APP_RRC_LOCK_WORK_INFO_TYPE_ENUM_UINT8  enLockType;                         /* 锁定类型 */
    VOS_UINT8                               ucBandInd;
    union
    {
        VOS_UINT16                      usFreqPoint;                            /* 锁定的频点 */
        APP_RRC_CELL_INFO_STRU          stFreqAndCell;                          /* 锁定的小区ID和频点 */
        APP_RRC_FREQ_BAND_STRU          stFreqBand;                             /* 锁定频带的上下限 */
    }u;
}APP_RRC_LOCK_INFO_STRU;

/*****************************************************************************
 结构名    :APP_RRC_LOCK_WORK_INFO_REQ_STRU
 结构说明  :路测下发的锁定请求的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32 ulMsgID;                                     /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_CMD_ENUM_UINT8                  enCommand;                              /* 锁定还是解锁 */
    VOS_UINT8                           aucReserved[3];                         /* 保留, 此处是2, 是因为下一个是单字节 */
    APP_RRC_LOCK_INFO_STRU              stLockInfo;
}APP_RRC_LOCK_WORK_INFO_REQ_STRU;

/*****************************************************************************
 结构名    :RRC_APP_LOCK_WORK_INFO_CNF_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :对原语APP_RRC_LOCK_WORK_INFO_REQ进行回复
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
}RRC_APP_LOCK_WORK_INFO_CNF_STRU;

/* begin: add for at&t ue cap display */
/*****************************************************************************
 结构名    :APP_LRRC_GET_UE_CAP_INFO_REQ_STRU
 结构说明  :HIDS下发的获取关键UE能力信息的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgID;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    VOS_UINT8                           aucReserved[128];                         /* 保留 */
}APP_LRRC_GET_UE_CAP_INFO_REQ_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_UE_CAP_INFO_STRU
 结构说明  : LRRC与APP之间上报的UE能力关键信元
*****************************************************************************/
typedef struct
{
    /* ulFGI01To32: APP解析的bit位如下: short DRX:FGI4, C-DRX:FGI5, L-W,PSHO:FGI8,
       L-G,PSHO:FGI9, CSFB w/measurements(REL9):FGI22, TTI Bundling:FGI28, SPS:FGI29 */
    VOS_UINT32                          ulFGI01To32;                            /* fgi bit位从最高位至最低位对应FGI1至FGI32 */
    VOS_UINT32                          ulFGI33To64;                            /* fgi bit位从最高位至最低位对应FGI33至FGI64 */
    VOS_UINT32                          ulFGI101To132;                          /* fgi bit位从最高位至最低位对应FGI101至FGI132 */
    VOS_UINT32                          ulFGIReserved;                          /* fgi bit保留位，保留32bit以便于协议扩展 */

    /* 从最高bit开始计数，为profile0x0001,以此类推，如果值为1，表示支持，0表示
       不支持;分别对应profile0x0001,profile0x0002,profile0x0003,profile0x0004,
       profile0x0006,profile0x0101,profile0x0103,profile0x0104 */
    VOS_UINT16                          usROHC;

    VOS_UINT8                           ucCategory;
    LRRC_CA_CC_NUM_ENUM_UINT8           enCaCCNum;                              /* 最大支持的CA CC数 */


    VOS_UINT32                          ulBandNum;                                   /* usBandNum 取值范围[1,256] */
    VOS_UINT32                          aulBandInd[LRRC_APP_MAX_BAND_IND_ARRAY_NUM]; /* aulBandInd,每个BIT，1:代表支持，0代表不支持，
                                                                                        ulBandInd[0]中第一个BIT代表BAND1,以此类推; */
    VOS_UINT32                          aul256Qam[LRRC_APP_MAX_BAND_IND_ARRAY_NUM]; /* aul256Qam,每个BIT，1:代表支持，0代表不支持，
                                                                                        aul256Qam[0]中第一个BIT，以此类推; */
    PS_BOOL_ENUM_UINT8                  enUlMimo4Layer;                           /* 是否支持上行4层 */
    PS_BOOL_ENUM_UINT8                  enDlMimo4Layer;                           /* 是否支持下行4层 */
    VOS_UINT8                           aucReserved[2];
}LRRC_APP_UE_CAP_INFO_STRU;

/*****************************************************************************
 结构名    :LRRC_APP_GET_UE_CAP_INFO_CNF_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :对原语APP_LRRC_GET_UE_CAP_INFO_REQ_STRU进行回复
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
    LRRC_APP_UE_CAP_INFO_STRU           stUeCapInfo;

    VOS_UINT8                           aucReserved[32];   /* 用来指示后续是否有Ind指示
                                                             ，目前主要用于扩展CA组合，因为
                                                             CA组合占用空间很大 */
}LRRC_APP_GET_UE_CAP_INFO_CNF_STRU;
/* end:   add for at&t ue cap display */

/*****************************************************************************
 结构名    : APP_RRC_TRANSPARENT_CMD_REQ
 协议表格  :
 ASN.1描述 :
 结构说明  :透明命令头
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32 ulMsgId;                                     /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    VOS_UINT16 usTransCmdNum;
    VOS_UINT16 usParaSize;
    VOS_UINT8  aucPara[4];
}APP_RRC_TRANSPARENT_CMD_REQ_STRU;


/*****************************************************************************
 结构名    : RRC_APP_TRANSPARENT_CMD_CNF_STRU
 结构说明  : PS->OMT的透明命令执行结果数据结构(
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32          ulMsgId;                            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    VOS_UINT8           aucTransCmdCnf[4];                 /* 透明命令结果码流，可变 */
}RRC_APP_TRANSPARENT_CMD_CNF_STRU;

/*****************************************************************************
 结构名    :PS_APP_HO_LATENCY_DETAIL_STRU
 结构说明  :切换时延关键点的上报
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          T4_ulRrcRcvHoRecfg;
    VOS_UINT32                          ulRrcRcvNasRabmRsp;
    VOS_UINT32                          ulRrcRcvCmmHoCnf;
    VOS_UINT32                          ulRrcRcvStopPdcpCnf;
    VOS_UINT32                          T5_ulRrcSndDspHoReq;
    VOS_UINT32                          T6_ulRrcRcvDspHoCnf;
    VOS_UINT32                          T7_ulRrcSndMacRaReq;
    VOS_UINT32                          T12_ulMacSndPhyAccessReq;
    VOS_UINT32                          T13_ulMacRcvRar;
    VOS_UINT32                          T8_ulRrcRcvMacRaCnf;
    VOS_UINT32                          ulRrcSndSmcSecuCfg;
    VOS_UINT32                          T9_ulRrcSndRecfgCmp;
    VOS_UINT32                          T10_ulRrcRcvAmDataCnf;
    VOS_UINT32                          ulRrcRcvCqiSrsCnf;
    VOS_UINT32                          ulRrcRcvPdcpContineCnf;
    VOS_UINT32                          T11_ulRrcRcvRabmStatusRsp;
}PS_APP_HO_LATENCY_DETAIL_STRU;
/*****************************************************************************
 结构名    :PS_APP_HO_COMP_DETAIL_STRU
 结构说明  :切换时延与协议对比
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulRealTestT6T5;
    VOS_UINT32                          ulPtlT6T5;          /* 1ms */
    VOS_UINT32                          ulRealTestT8T7;
    VOS_UINT32                          ulPtlT8T7;          /* 2.5 + 1+ 7.5ms */
    VOS_UINT32                          ulRealTestT10T9;
    VOS_UINT32                          ulPtlT10T9;         /* 13ms */
    VOS_UINT32                          ulRealTestT7T4;
    VOS_UINT32                          ulPtlT7T4;          /* 50ms */
    VOS_UINT32                          ulRealTestT11T4;
    VOS_UINT32                          ulPtlT11T4;         /* 100ms */
}PS_APP_HO_COMP_DETAIL_STRU;
/*****************************************************************************
 结构名    :PS_APP_EST_LATENCY_DETAIL_STRU
 结构说明  :建链时延关键点的上报
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          T1_ulRrcSndMacRaReq;
    VOS_UINT32                          T2_ulMacSndPhyAccessReq;
    VOS_UINT32                          T4_ulMacRcvRar;
    VOS_UINT32                          ulRrcRcvMacRaCnf;
    VOS_UINT32                          T5_ulMacSndMsg3;
    VOS_UINT32                          T6_ulRrcRcvSetUp;
    VOS_UINT32                          T7_ulRrcSndEstCmp;
    VOS_UINT32                          ulRrcRcvSecCmd;
    VOS_UINT32                          ulRrcSndSecCmdCmp;
    VOS_UINT32                          ulRrcRcvCapEnq;
    VOS_UINT32                          ulRrcSndCapInfo;
    VOS_UINT32                          ulRrcRcvRecfg;
    VOS_UINT32                          T13_ulRrcSndRecfgCmp;
}PS_APP_EST_LATENCY_DETAIL_STRU;
/*****************************************************************************
 结构名    :PS_APP_EST_COMP_STRU
 结构说明  :建链时延与协议对比
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulRealTestT4T1;
    VOS_UINT32                          ulPtlT4T1;              /* 5s + 6ms */
    VOS_UINT32                          ulRealTestT5T4;
    VOS_UINT32                          ulPtlT5T4;              /* 2.5ms */
    VOS_UINT32                          ulTestRealT6T5;
    VOS_UINT32                          ulPtlT6T5;              /* 28.5ms+2*Ts1c */
    VOS_UINT32                          ulRealTestT7T6;
    VOS_UINT32                          ulPtlT7T6;              /* 3ms */
    VOS_UINT32                          ulReaTestlT7T1;
    VOS_UINT32                          ulPtlT7T1;              /* 47.5ms+2*Ts1c */
    VOS_UINT32                          ulReaTestlT13T1;
    VOS_UINT32                          ulPtlT13T1;             /* 61ms+2*Ts1c+Ts1u */
}PS_APP_EST_COMP_DETAIL_STRU;
/*****************************************************************************
 结构名    :PS_APP_REEST_LATENCY_DETAIL_STRU
 结构说明  :重建时延关键点的上报
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          T2_ulRrcRcvReestInd;
    VOS_UINT32                          T3_ulRrcSndCellSearchReq;
    VOS_UINT32                          T4_ulRrcRcvCellSearchInd;
    VOS_UINT32                          T5_ulRrcRcvSi;
    VOS_UINT32                          T6_ulRrcSndCampReq;
    VOS_UINT32                          T7_ulRrcRcvCampCnf;
    VOS_UINT32                          T10_ulRrcSndMacRaReq;
    VOS_UINT32                          ulMacSndPhyAccessReq;
    VOS_UINT32                          ulMacRcvRar;
    VOS_UINT32                          ulRrcRcvMacRaCnf;
    VOS_UINT32                          ulMacSndMsg3;
    VOS_UINT32                          ulRrcRcvReest;
    VOS_UINT32                          ulRrcSndReestCmpl;
}PS_APP_REEST_LATENCY_DETAIL_STRU;

/*****************************************************************************
 结构名    :PS_APP_REEST_COMP_DETAIL_STRU
 结构说明  :重建时延与协议对比
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulRealTestT4T3;
    VOS_UINT32                          ulPtlT4T3;              /* 100ms */
    VOS_UINT32                          ulRealTestT5T4;
    VOS_UINT32                          ulPtlT5T4;              /* 1280ms */
    VOS_UINT32                          ulRealTestT10T2;
    VOS_UINT32                          ulPtlT10T2;             /* 1500ms */

}PS_APP_REEST_COMP_DETAIL_STRU;
/*****************************************************************************
 结构名    :RRC_APP_TIME_LATENCY_RPT_IND_STRU
 结构说明  :时延时间的上报
*****************************************************************************/
typedef struct
{
    VOS_UINT32                                  T7T4_ulHoCPlaneRealLatency;
    VOS_UINT32                                  ulHoCPlanePtlLatency;
    VOS_UINT32                                  T11T4_ulHoUPlaneRealLatency;
    VOS_UINT32                                  ulHoUPlanePtlLatency;
    PS_APP_HO_LATENCY_DETAIL_STRU               stHoLatencyDetail;
    PS_APP_HO_COMP_DETAIL_STRU                  stHoCompDetail;
}PS_APP_HO_LATENCY_STRU;

/*****************************************************************************
 结构名    :PS_APP_REEST_LATENCY_STRU
 结构说明  :时延时间的上报
*****************************************************************************/
typedef struct
{
    VOS_UINT32                                  T10T2_ulReestRealLatency;
    VOS_UINT32                                  ulReestPtlLatency;
    PS_APP_REEST_LATENCY_DETAIL_STRU            stReestLatencyDetail;
    PS_APP_REEST_COMP_DETAIL_STRU               stReestCompDetail;
}PS_APP_REEST_LATENCY_STRU;

/*****************************************************************************
 结构名    :PS_APP_EST_LATENCY_STRU
 结构说明  :时延时间的上报
*****************************************************************************/
typedef struct
{
    VOS_UINT32                                  T13T1_ulEstUPlaneRealLatency;
    VOS_UINT32                                  ulEstUPlanePtlLatency;
    VOS_UINT32                                  T7T1_ulEstCPlaneRealLatency;
    VOS_UINT32                                  ulEstCPlanePtlLatency;
    PS_APP_EST_LATENCY_DETAIL_STRU              stEstLatencyDetail;
    PS_APP_EST_COMP_DETAIL_STRU                 stEstCompDetail;
}PS_APP_EST_LATENCY_STRU;

/*****************************************************************************
 结构名    :RRC_APP_TIME_LATENCY_RPT_IND_STRU
 结构说明  :时延时间的上报
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;

    APP_LATENCY_TYPE_ENUM_UINT8         enLatencyType;
    VOS_UINT8                           aucReserved[3];
union
    {
        PS_APP_HO_LATENCY_STRU             stHoElapse;
        PS_APP_REEST_LATENCY_STRU          stReestElapse;
        PS_APP_EST_LATENCY_STRU            stEstElapse;
    }u;
} RRC_APP_TIME_DELAY_RPT_IND_STRU;

/*****************************************************************************
 结构名    :APP_RRC_CSQ_REQ_STRU
 结构说明  :CSQ查询请求
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    VOS_UINT16                          usSwt;              /* 0: 停止周期上报 1: 查询RSSI，不启动IND上报  2: 启动周期上报RSSI */
    VOS_UINT16                          usPeriod;
    VOS_UINT16                          usNdb;              /* 范围 0-5 dbm*/
    VOS_UINT16                          usMs;               /* 范围 1-20 s*/
}APP_RRC_CSQ_REQ_STRU;

/*****************************************************************************
 结构名    :RRC_APP_CSQ_CNF_STRU
 结构说明  :CSQ查询请求响应
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usRI;                /*RI值*/
    VOS_UINT16                          ausCQI[2];           /* CQI两个码字 */
    VOS_UINT8                           aucRes[2];

}APP_RRC_CQI_INFO_STRU;

/*****************************************************************************
 结构名    :RRC_APP_CSQ_CNF_STRU
 结构说明  :CSQ查询请求响应
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
    VOS_INT16                           sRsrp;              /* RSRP测量值范围：(-141,-44) */
    VOS_INT16                           sRsrq;              /* RSRQ测量值范围：(-40, -6) */
    VOS_INT16                           sRssi;              /* RSSI测量值 */
    VOS_UINT16                          usBer;              /* 误码率 */
    VOS_INT32                           lSINR;              /* SINR  RS_SNR */
    APP_RRC_CQI_INFO_STRU               stCQI;              /* CQI两个码字 */
}RRC_APP_CSQ_CNF_STRU;

/*****************************************************************************
 结构名    :RRC_APP_CSQ_IND_STRU
 结构说明  :RRC上报给APP的周期CSQ查询指示
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_INT16                           sRsrp;              /* RSRP测量值范围：(-141,-44) */
    VOS_INT16                           sRsrq;              /* RSRQ测量值范围：(-40, -6) */
    VOS_INT16                           sRssi;              /* RSSI测量值 */
    VOS_UINT16                          usBer;              /* 误码率 */
    VOS_INT32                           lSINR;              /* SINR  RS_SNR */
    APP_RRC_CQI_INFO_STRU               stCQI;              /* CQI两个码字 */
}RRC_APP_CSQ_IND_STRU;

/*****************************************************************************
 结构名    :APP_RRC_PTL_STATE_QUERY_REQ_STRU
 结构说明  :协议状态查询请求
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_CMD_ENUM_UINT8                  enCommand;
    APP_RRC_CELL_RPT_PERIOD_ENUM_UINT8  enPeriod;
    VOS_UINT8                           aucReserved[2];

}APP_RRC_PTL_STATE_QUERY_REQ_STRU;

/*****************************************************************************
 结构名    : RRC_APP_PTL_STATE_QUERY_CNF_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC上报的协议状态
*****************************************************************************/
/*V7R2-DT ,2014/4/25, begin*/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;             /* MSP直接将此ID做为CmdID发给Prob */
    APP_RESULT_ENUM_UINT32              enResult;
    //VOS_UINT32                          ulCurrentState;     /* RRC协议状态, 0:表示IDLE态 1:表示CONNECTED 2:表示协议状态无效 */
}RRC_APP_PTL_STATE_QUERY_CNF_STRU;
/*V7R2-DT ,2014/4/25, end*/

/*****************************************************************************
 结构名    : RRC_APP_PTL_STATE_CHANGE_IND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC上报的协议状态
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    VOS_UINT32                          ulCurrentState;     /* RRC协议状态, 0:表示IDLE态 1:表示CONNECTED 2:表示协议状态无效 */
}RRC_APP_PTL_STATE_CHANGE_IND_STRU;
/* DT begin */
/*****************************************************************************
 结构名    : RRC_APP_PTL_STATE_CHANGE_IND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC上报的协议状态
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    VOS_UINT32                          ulCurrentState;     /* RRC协议状态, 0:表示IDLE态 1:表示CONNECTED 2:表示协议状态无效 */
}RRC_APP_DT_PTL_STATE_IND_STRU;

/* DT end */

/*****************************************************************************
 结构名    :APP_RRC_PTL_STATE_QUERY_REQ_STRU
 结构说明  :小区基本信息查询请求
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
}APP_RRC_CELL_INFO_QUERY_REQ_STRU;

/*****************************************************************************
 结构名    : RRC_APP_PTL_STATE_QUERY_CNF_STRU
 结构说明  : RRC回复的小区基本信息
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    VOS_UINT16                          usFreq;             /* 0xFFFF为无效,单位：100KHz */
    VOS_UINT16                          usPci;              /* 0xFFFF为无效,范围：(0,503) */
    VOS_UINT8                           ucDlBandWidth;      /* 0xff为无效,范围:(0,5): (0 : 1.4M , 1 : 3M , 2 : 5M ,3 : 10M , 4 : 15M, 5 : 20M) */
    VOS_UINT8                           aucReserved[3];
}RRC_APP_CELL_INFO_QUERY_CNF_STRU;

/* DT begin */
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgID;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_CMD_ENUM_UINT8                  enCommand;
    APP_RRC_CELL_RPT_PERIOD_ENUM_UINT8  enPeriod;
    VOS_UINT8                           aucReserved[2];
}APP_RRC_INQ_CMD_REQ_STRU;

typedef APP_RRC_INQ_CMD_REQ_STRU APP_RRC_INQ_SERVING_CELL_INFO_REQ_STRU;

typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgID;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
}APP_RRC_INQ_CMD_CNF_HEADER_STRU;
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgID;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
}APP_RRC_INQ_CMD_IND_HEADER_STRU;

/* DT end */

/* begin:V7R2-DT 移植强制切换、重选和禁止限制小区接入等功能,2014/3/26 */
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
}RRC_APP_FORCE_HOANDCSEL_CNF_STRU;
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32 ulMsgID;                                     /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    DT_CMD_ENUM_UINT8                   enCmd;              /* FORCE HO/CSEL FLAG */
    VOS_UINT8                           ucRsv[3];
    VOS_UINT32                          ulPci;              /* PHY Cell ID */
}APP_RRC_FORCE_HOANDCSEL_REQ_STRU;
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
}RRC_APP_BARCELL_ACCESS_CNF_STRU;
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32 ulMsgID;                                     /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    DT_CMD_ENUM_UINT8                   enCmd;              /* Bar Cell Access FLAG */
    VOS_UINT8                           ucRsv[3];
}APP_RRC_BARCELL_ACCESS_REQ_STRU;

/* end:V7R2-DT 移植强制切换、重选和禁止限制小区接入等功能,2014/3/26 */

/*****************************************************************************
 结构名    : APP_RRC_MSG_DATA
 协议表格  :
 ASN.1描述 :
 结构说明  : APP_RRC_MSG_DATA数据结构,以下为RRC专用
*****************************************************************************/
typedef struct
{
    APP_RRC_MSG_ID_ENUM_UINT32          enMsgID;        /*_H2ASN_MsgChoice_Export APP_RRC_MSG_ID_ENUM_UINT32*/
    VOS_UINT8                           aucMsg[4];
    /***************************************************************************
        _H2ASN_MsgChoice_When_Comment          APP_RRC_MSG_ID_ENUM_UINT32
    ****************************************************************************/
}APP_RRC_MSG_DATA;

/*_H2ASN_Length UINT32*/

/*****************************************************************************
 结构名    : AppRrcInterface_MSG
 协议表格  :
 ASN.1描述 :
 结构说明  : AppRrcInterface_MSG数据结构,以下为RRC专用
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    APP_RRC_MSG_DATA                    stMsgData;
}AppRrcInterface_MSG;
/*********************************************************
 枚举名    : LRRC_LPHY_LTE_BAND_WIDTH_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  :
**********************************************************/
enum RRC_APP_BAND_WIDTH_ENUM
{
    RRC_APP_LTE_BAND_WIDTH_1D4M = 0,
    RRC_APP_LTE_BAND_WIDTH_3M,
    RRC_APP_LTE_BAND_WIDTH_5M,
    RRC_APP_LTE_BAND_WIDTH_10M,
    RRC_APP_LTE_BAND_WIDTH_15M,
    RRC_APP_LTE_BAND_WIDTH_20M,
    RRC_APP_LTE_BAND_WIDTH_BUTT
};
typedef VOS_UINT16 RRC_APP_BAND_WIDTH_ENUM_UINT16;

/*****************************************************************************
 结构名    : APP_RRC_LWCLASH_REQ_STRU
 结构说明  :协议状态查询请求
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                      /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
} APP_RRC_LWCLASH_REQ_STRU;
/*****************************************************************************
 结构名    : RRC_APP_LWCLASH_PARA_STRU
结构说明  : RRC上报的消息
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              usUlFreq;          /*上行中心频点单位:100Khz*/
    VOS_UINT16                              usDlFreq;          /*下行中心频点 单位:100Khz*/
    RRC_APP_BAND_WIDTH_ENUM_UINT16          usUlBandwidth;     /*上行带宽 */
    RRC_APP_BAND_WIDTH_ENUM_UINT16          usDlBandwidth;     /*上行带宽 */
    APP_CAMPED_FLAG_ENUM_UINT8              enCamped;          /*是否驻留 */
    APP_STATE_FLAG_ENUM_UINT8               enState;           /*是否为冲突状态 */
    VOS_UINT8                               usBand;            /*频带指示 */
    VOS_UINT8                               aucResv[1];
} RRC_APP_LWCLASH_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_APP_SCELL_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_UINT32                              ulUlFreq;           /*上行中心频率 单位:100Khz*/
    VOS_UINT32                              ulDlFreq;           /*下行中心频率 单位:100Khz*/
    RRC_APP_BAND_WIDTH_ENUM_UINT16          usUlBandwidth;      /*上行带宽 */
    RRC_APP_BAND_WIDTH_ENUM_UINT16          usDlBandwidth;      /*下行带宽 */
}RRC_APP_SCELL_INFO_STRU;

/*****************************************************************************
 结构名    : RRC_APP_LWCLASH_CNF_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                      /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;         /* MSP直接将此ID做为CmdID发给Prob */
    RRC_APP_LWCLASH_PARA_STRU           stLWClashPara;
    VOS_UINT32                          ulScellNum;
    RRC_APP_SCELL_INFO_STRU             stScellInfo[LRRC_APP_LWCLASH_MAX_SCELL_NUM];
} RRC_APP_LWCLASH_CNF_STRU;
/*****************************************************************************
 结构名    : RRC_APP_LWCLASH_IND_STRU
结构说明  : RRC上报的消息
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                      /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;        /*_H2ASN_Skip*/
    RRC_APP_LWCLASH_PARA_STRU           stLWClashPara;
    VOS_UINT32                          ulScellNum;
    RRC_APP_SCELL_INFO_STRU             stScellInfo[LRRC_APP_LWCLASH_MAX_SCELL_NUM];
} RRC_APP_LWCLASH_IND_STRU;

/*****************************************************************************
 结构名    :APP_RRC_NMR_REQ_STRU
 结构说明  :NMR查询请求
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
}APP_RRC_NMR_REQ_STRU;

/*****************************************************************************
 结构名    :RRC_APP_NMR_CGI_STRU
 结构说明  :NMR结构体定义
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usMncNum;           /*指示 MNC 个数*/
    VOS_UINT16                          usMcc;
    VOS_UINT16                          usMnc;
    VOS_UINT8                           aucRes[2];
} RRC_APP_PLMN_ID_STRU;

/*****************************************************************************
 结构名    :RRC_APP_NMR_CGI_STRU
 结构说明  :NMR结构体定义
*****************************************************************************/
typedef struct
{
    RRC_APP_PLMN_ID_STRU                stPlmnId;
    VOS_UINT32                          sCellId;            /* 范围：(0,503) */
} RRC_APP_CELL_GLOBAL_ID_STRU;

/*****************************************************************************
 结构名    :RRC_APP_NMR_CGI_STRU
 结构说明  :NMR结构体定义
*****************************************************************************/
typedef struct
{
    RRC_APP_CELL_GLOBAL_ID_STRU         stCellGloId;        /* CellGlobalId */
    VOS_UINT16                          usTAC;
    VOS_UINT16                          usPci;              /* 0xFFFF为无效,范围：(0,503) */
    VOS_INT16                           sRsrp;              /* RSRP测量值范围：(-141,-44) */
    VOS_INT16                           sRsrq;              /* RSRQ测量值范围：(-40, -6) */
    VOS_UINT16                          usTa;               /* TA值*/
    VOS_UINT8                           aucRes[2];
} RRC_APP_LTE_CELL_INFO_STRU;

/*****************************************************************************
 结构名    :RRC_APP_NMR_CNF_STRU
 结构说明  :NMR查询请求响应
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
    RRC_APP_LTE_CELL_INFO_STRU          stLteCelInfo;
}RRC_APP_NMR_CNF_STRU;

/*****************************************************************************
 结构名    :APP_RRC_CELLID_REQ_STRU
 结构说明  :小区基本信息查询请求
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
}APP_RRC_CELLID_REQ_STRU;

/*****************************************************************************
 结构名    : RRC_APP_CELLID_CNF_STRU
 结构说明  : RRC回复的小区基本信息
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    APP_RESULT_ENUM_UINT32              enResult;
    APP_PLMN_ID_STRU                    stPlmnId;
    VOS_UINT32                          ulCi;               /* 0xFFFF为无效, */
    VOS_UINT16                          usPci;              /* 0xFFFF为无效,范围：(0,503) */
    VOS_UINT16                          usTAC;             /* TAC */
}RRC_APP_CELLID_CNF_STRU;

/*****************************************************************************
  6 UNION
*****************************************************************************/


/*****************************************************************************
  7 Extern Global Variable
*****************************************************************************/
/*****************************************************************************
 结构名    : APP_LPS_MSG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulMsgId;      /*消息ID*/
    VOS_UINT32                          ulSize;       /*消息体的长度*/
    VOS_UINT8                           aucValue[4];  /*消息体有效内容的指针*/
}APP_LPS_MSG_STRU;

/* BEGIN DTS_DCM_IDLE_RESELECTION_CFG 2012-12-08 Add*/
/*****************************************************************************
 结构名    : APP_LRRC_RESEL_OFFSET_CFG_NTF_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                         /*_H2ASN_Skip*/
    VOS_UINT32             ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER

    VOS_UINT32             ulOpId;
    VOS_UINT32             ulFlag;
}APP_LRRC_RESEL_OFFSET_CFG_NTF_STRU;

/* dcom-resel-cfg */
/*****************************************************************************
 结构名    : APP_LRRC_CON_TO_IDLE_NTF_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                         /*_H2ASN_Skip*/
    VOS_UINT32             ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32              ulOpId;
    VOS_INT32              ulReserv;
}APP_LRRC_CON_TO_IDLE_NTF_STRU;

/* fast-dorm-cfg */

/*****************************************************************************
 枚举名    : LRRC_LPDCP_FAST_DORMANCY_CMD_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : 指示PDCP启动或停止FAST_DORMANCY
*****************************************************************************/
enum APP_LRRC_FAST_DORM_ENUM
{
    APP_LRRC_FAST_DORMANCY_STOP         = 0,                                  /* 停止FAST_DORMANCY */
    APP_LRRC_FAST_DORMANCY_START,                                             /* 启动FAST_DORMANCY */
    APP_LRRC_FAST_DORMANCY_BUTT
};
typedef VOS_UINT32 APP_LRRC_FAST_DORM_ENUM_UINT32;

/*****************************************************************************
 结构名    : APP_LRRC_FAST_DORM_CFG_NTF_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                         /*_H2ASN_Skip*/
    VOS_UINT32             ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;

    APP_LRRC_FAST_DORM_ENUM_UINT32      ulFlag;
    VOS_UINT32            ulTimerLen;   /* unite: s */
}APP_LRRC_FAST_DORM_CFG_NTF_STRU;



/*****************************************************************************
 结构名    : ID_APP_LRRC_SET_UE_REL_VERSION_REQ
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                         /*_H2ASN_Skip*/
    VOS_UINT32             ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;
    VOS_UINT32              ulMode;     /* 2:LTE , 1:TDS */
    VOS_UINT32              ulVersion;  /* 9 - 14 */
}APP_LRRC_SET_UE_REL_VERSION_REQ_STRU;

/*****************************************************************************
 结构名    : ID_LRRC_APP_SET_UE_REL_VERSION_CNF
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                      /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;        /*_H2ASN_Skip*/
    APP_MSG_HEADER
    VOS_UINT32                          ulOpId;         /* MSP直接将此ID做为CmdID发给Prob */
    APP_RESULT_ENUM_UINT32              enResult;
} LRRC_APP_SET_UE_REL_VERSION_CNF_STRU;


/*****************************************************************************
 结构名    : LRRC_DAM_BAR_LIST_ITEM_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usFreqInfo;
    VOS_UINT16                          usCellId;
    VOS_UINT8                           ucBandInd;
    VOS_UINT8                           aucReserved[3];
    VOS_UINT32                          ulTimeOutTimeInMs;
    VOS_UINT32                          ulRemainTimeOutInMs;
}LRRC_DAM_BAR_LIST_ITEM_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_DAM_BAR_LIST_DBG_INFO_IND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    VOS_UINT16                          usItemCount;        /* Bar List中条目个数 */
    VOS_UINT16                          usRev;              /* 保留 */
    LRRC_DAM_BAR_LIST_ITEM_STRU         astDamBarListItem[RRC_APP_MAX_LIMITED_ITEM_COUNT];
}LRRC_APP_DAM_BAR_LIST_DBG_INFO_IND_STRU;
/* add for AT&T LRRC DAM end */
/*****************************************************************************
 结构名    : LRRC_APP_IDLE_SERVE_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          enCurrReselPrior;    /* 当前使用的重选优先级，移动性转换的时候 */
    VOS_UINT16                          enBandInd;           /* 频带指示 */
    VOS_UINT16                          usFreqInfo;          /* 服务小区频点 */
    VOS_UINT16                          usCellId;            /* 服务小区ID */
    VOS_INT16                           sRsrp;               /* RSRP测量值 */
    VOS_INT16                           sRsrq;               /* RSRQ测量值 */
    VOS_INT16                           sRssi;               /* RSSI测量值 */
    VOS_INT16                           sSValue;             /*计算得到的SP值*/
    VOS_INT16                           sSqual;              /*计算得到的SQ值*/
    VOS_INT16                           sQRxLevMin;          /*准则RSRP评估参数*/
    VOS_INT16                           sPCompensation;      /* 根据 P-Max 和 UE Max Tx power 推算出来的 PCompensation */
    VOS_INT16                           sQqualMin;           /* S准则RSRQ评估参数 */
    VOS_INT16                           sSIntraSearchP;      /* 启动同频测量的rsrp阈值参数 */
    VOS_INT16                           sSIntraSearchQ;      /* 启动同频测量的rsrq阈值参数 */
    VOS_INT16                           sSNonIntraSearchP;   /* 启动同优先级和低优先级异频测量的rsrp阈值参数 */
    VOS_INT16                           sSNonIntraSearchQ;   /* 启动同优先级和低优先级异频测量的rsrq阈值参数 */
    VOS_INT16                           sThreshServingLowP;  /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                           sThreshServingLowQ;  /* 重选评估时使用R9参数 */
} LRRC_APP_IDLE_SERVE_INFO_STRU;


/*****************************************************************************
 结构名    : LRRC_APP_IDLE_INTRA_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCellId;
    VOS_INT16                           sSValue;            /*计算得到的SP值*/
    VOS_INT16                           sSqual;             /*计算得到的SQ值*/
    VOS_INT16                           sRsrp;              /* RSRP测量值 */
    VOS_INT16                           sRsrq;              /* RSRQ测量值 */
    VOS_INT16                           sRssi;              /* RSSI测量值 */
}LRRC_APP_IDLE_CELL_MEAS_RESULT_STRU;

typedef struct
{
    VOS_UINT16                          enCurrReselPrior;    /* 当前使用的重选优先级，移动性转换的时候 */
    VOS_UINT16                          enBandInd;           /* 频带指示 */
    VOS_UINT16                          usFreqInfo;          /*频点*/
    VOS_UINT16                          usTotalCellNum;      /*小区总数*/
    VOS_UINT16                          usDetectedCellNum;   /*检测到的小区数目*/
    VOS_INT16                           sQRxLevMin;          /*计算SP值用到的最低接入电平*/
    VOS_INT16                           sPCompensation;      /* 根据 P-Max 和 UE Max Tx power 推算出来的 PCompensation */
    VOS_INT16                           sQqualMin;           /* S准则RSRQ评估参数 */
    VOS_INT16                           sSIntraSearchP;      /* 启动同频测量的rsrp阈值参数 */
    VOS_INT16                           sSIntraSearchQ;      /* 启动同频测量的rsrq阈值参数 */
    LRRC_APP_IDLE_CELL_MEAS_RESULT_STRU astIntraCellMesRslt[LRRC_LCSEL_INTRA_CELL_REPORT_NUM];
}LRRC_APP_IDLE_INTRA_INFO_STRU;



/*****************************************************************************
 结构名    : LRRC_APP_IDLE_INTER_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              enCurrReselPrior;    /* 当前使用的重选优先级，移动性转换的时候 */
    VOS_UINT16                              usFreqInfo;
    VOS_UINT16                              enBandInd;           /* 频带指示 */
    VOS_INT16                               sThreshLowP;         /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                               sThreshLowQ;         /* 重选评估时使用R9参数 */
    VOS_INT16                               sThreshHighP;        /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                               sThreshHighQ;        /* 重选评估时使用R9参数 */
    VOS_UINT16                              usTotalCellNum;      /*小区总数*/
    VOS_UINT16                              usDetectedCellNum;   /*检测到的小区数目*/
    LRRC_APP_IDLE_CELL_MEAS_RESULT_STRU     astInterCellMesRslt[LRRC_LCSEL_INTER_CELL_REPORT_NUM]; /* SCELL放在第一个的位置 */
}LRRC_APP_IDLE_SINGLE_FREQ_MEAS_RESULT_STRU;

typedef struct
{
    VOS_UINT16                                   usInterFreqNum;
    VOS_INT16                                    sSNonIntraSearchP;   /* 启动同优先级和低优先级异频测量的rsrp阈值参数 */
    VOS_INT16                                    sSNonIntraSearchQ;   /* 启动同优先级和低优先级异频测量的rsrq阈值参数 */
    VOS_INT16                                    sServValue;          /*计算得到的SP值*/
    VOS_INT16                                    sServqual;           /*计算得到的SQ值*/
    VOS_INT16                                    sThreshServingLowP;  /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                                    sThreshServingLowQ;  /* 重选评估时使用R9参数 */
    LRRC_APP_IDLE_SINGLE_FREQ_MEAS_RESULT_STRU   astInterFreqMesRslt[LRRC_LCSEL_INTER_FREQ_REPORT_NUM];
}LRRC_APP_IDLE_INTER_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_IDLE_UTRAN_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              usPrimaryCode;
    VOS_INT16                               sRscp;               /* 精度1/8 */
    VOS_INT16                               sEcN0;               /* 精度1/8 */
    VOS_INT16                               sSValue;             /*计算得到的SP值*/
    VOS_INT16                               sSqual;              /*计算得到的SQ值*/
}LRRC_APP_UTRAN_CELL_MEAS_RESULT_STRU;

typedef struct
{
    VOS_UINT16                              enCurrReselPrior;    /* 当前使用的重选优先级，移动性转换的时候 */
    VOS_UINT16                              usArfcn;
    VOS_INT16                               sRssi;               /* 精度1/8 */
    VOS_INT16                               sQRxLevMin;          /*计算SP值用到的最低接入电平*/
    VOS_INT16                               sPCompensation;      /* 根据 P-Max 和 UE Max Tx power 推算出来的 PCompensation */
    VOS_INT16                               sQqualMin;           /* S准则RSRQ评估参数 */
    VOS_INT16                               sThreshLowP;         /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                               sThreshLowQ;         /* 重选评估时使用R9参数 */
    VOS_INT16                               sThreshHighP;        /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                               sThreshHighQ;        /* 重选评估时使用R9参数 */
    VOS_UINT16                              usCellNum;
    LRRC_APP_UTRAN_CELL_MEAS_RESULT_STRU    astUtranCellMesRslt[LRRC_LCSEL_UTRAN_CELL_REPORT_NUM];
}LRRC_APP_UTRAN_SIGNLE_FREQ_MEAS_RESULT_STRU;

typedef struct
{
    VOS_UINT16                                      usArfcnNum;
    VOS_INT16                                       sSNonIntraSearchP;   /* 启动同优先级和低优先级异频测量的rsrp阈值参数 */
    VOS_INT16                                       sSNonIntraSearchQ;   /* 启动同优先级和低优先级异频测量的rsrq阈值参数 */
    VOS_INT16                                       sServValue;          /*计算得到的SP值*/
    VOS_INT16                                       sServqual;           /*计算得到的SQ值*/
    VOS_INT16                                       sThreshServingLowP;  /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                                       sThreshServingLowQ;  /* 重选评估时蔛褂肦9参数 */
    LRRC_APP_UTRAN_SIGNLE_FREQ_MEAS_RESULT_STRU     astUtranFreqMesRslt[LRRC_TRRC_PHY_MAX_SUPPORT_CARRIER_NUM];
}LRRC_APP_IDLE_UTRAN_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_IDLE_UTRAN_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                              enCurrReselPrior;   /* 当前使用的重选优先级，移动性转换的时候 */
    VOS_UINT16                              usArfcn;
    VOS_UINT16                              enBandInd;          /* 频点指示 */
    VOS_INT16                               sRssi;              /* 精度1/8 */
    VOS_UINT16                              usNcc;
    VOS_UINT16                              usBcc;
    VOS_INT16                               sSValue;             /*计算得到的SP值*/
    VOS_INT16                               sQRxLevMin;          /*计算SP值用到的最低接入电平*/
    VOS_INT16                               sPCompensation;      /* 根据 P-Max 和 UE Max Tx power 推算出来的 PCompensation */
    VOS_INT16                               sThreshLowP;         /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                               sThreshHighP;        /* 针对低优先级小区重选的服务小区质量门限 */
}LRRC_APP_GERAN_SIGNLE_FREQ_MEAS_RESULT_STRU;

typedef struct
{
    VOS_UINT16                                    usArfcnNum;
    VOS_INT16                                     sSNonIntraSearchP;   /* 启动同优先级和低优先级异频测量的rsrp阈值参数 */
    VOS_INT16                                     sSNonIntraSearchQ;   /* 启动同优先级和低优先级异频测量的rsrq阈值参数 */
    VOS_INT16                                     sServValue;          /*计算得到的SP值*/
    VOS_INT16                                     sServqual;           /*计算得到的SQ值*/
    VOS_INT16                                     sThreshServingLowP;  /* 针对低优先级小区重选的服务小区质量门限 */
    VOS_INT16                                     sThreshServingLowQ;  /* 重选评估时使用R9参数 */
    LRRC_APP_GERAN_SIGNLE_FREQ_MEAS_RESULT_STRU   astGeranFreqMesRslt[LRRC_GURRC_GERAN_ARFCN_MAX_NUM];
}LRRC_APP_IDLE_GERAN_INFO_STRU;

/*begin: measurement report 2015-08-18 */
/*****************************************************************************
 结构名    : LRRC_APP_CELL_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCellId;
    VOS_UINT16                          usFreqInfo;
    VOS_INT16                           sRsrp;              /* RSRP测量值 */
    VOS_INT16                           sRsrq;              /* RSRQ测量值 */
    VOS_INT16                           sRssi;              /* RSSI测量值 */
} LRRC_APP_CELL_MEAS_RSLT_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_UTRAN_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usArfcn;
    VOS_UINT16                          usPrimaryCode;
    VOS_INT16                           sRscp;               /* 精度1/8 */
    VOS_INT16                           sEcN0;               /* 精度1/8 */
    VOS_INT16                           sRssi;               /* 精度1/8 */
    VOS_UINT16                          UtranType;
} LRRC_APP_UTRAN_CELL_RSLT_STRU;
/*****************************************************************************
 枚举名     :LRRC_GRR_BANDINDICATOR_ENUM
 协议表格  :
 ASN.1描述   :
 枚举说明 : 2G小区频带指示
*****************************************************************************/
enum LRRC_APP_GERAN_BANDIND_ENUM
{
    DCS1800                          = 0,
    PCS1900                          = 1,

    LRRC_APP_GERAN_BANDIND_BUTT      = 0xFFFF
};
typedef VOS_UINT16    LRRC_APP_GERAN_BANDIND_ENUM_UINT16;

/*****************************************************************************
 枚举名    : RRC_APP_PROTOCOL_STATE
 协议表格  :
 ASN.1描述 :
 枚举说明  : RRC协议状态类型
*****************************************************************************/
enum RRC_APP_PROTOCOL_STATE_ENUM
{
    RRC_MEAS_PROTOCOL_STATE_IDLE            = 0 ,
    RRC_MEAS_PROTOCOL_STATE_CONNECTED,
    RRC_MEAS_PROTOCOL_STATE_BUTT            = 0xFFFF
};
typedef VOS_UINT16 RRC_APP_PROTOCOL_STATE_ENUM_UINT16;

/*****************************************************************************
 枚举名     :LRRC_GRR_BANDINDICATOR_ENUM
 协议表格  :
 ASN.1描述   :
 枚举说明 : 2G小区频带指示
*****************************************************************************/
enum LRRC_GRR_BANDINDICATOR_ENUM
{
    LRRC_GRR_BANDINDICATOR_DCS1800                          = 0,
    LRRC_GRR_BANDINDICATOR_PCS1900                             ,

    LRRC_GRR_BANDINDICATOR_BUTT
};
typedef VOS_UINT16    LRRC_GRR_BANDINDICATOR_ENUM_UINT16;

/*****************************************************************************
 结构名    : LRRC_APP_UTRAN_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usArfcn;
    LRRC_APP_GERAN_BANDIND_ENUM_UINT16  enBandInd;          /* 频点指示 */
    VOS_INT16                           sRssi;              /* 精度1/8 */
} LRRC_APP_GERAN_CELL_RSLT_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_SERV_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    RRC_APP_PROTOCOL_STATE_ENUM_UINT16 enState;             /* 当前协议状态 */
    LRRC_APP_CELL_MEAS_RSLT_STRU       stServCellRslt;      /* 服务小区上报结果 */
} LRRC_APP_SERV_MEAS_RSLT_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_SCELL_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    LRRC_APP_CELL_MEAS_RSLT_STRU       astSCellRslt[LRRC_SCELL_MAX_NUM];          /* SCell上报结果 */
} LRRC_APP_SCELL_MEAS_RSLT_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_INTRA_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    LRRC_APP_CELL_MEAS_RSLT_STRU       astIntraCellRslt[LRRC_INTRA_CELL_MAX_NUM]; /* 同频小区上报结果 */
} LRRC_APP_INTRA_MEAS_RSLT_STRU;

/*****************************************************************************
 结构名    : LRRC_APP_INTER_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    LRRC_APP_CELL_MEAS_RSLT_STRU       astInterCellRslt[LRRC_INTER_CELL_MAX_NUM]; /* 异频小区上报结果 */
} LRRC_APP_INTER_MEAS_RSLT_STRU;
/*****************************************************************************
 结构名    : LRRC_APP_INTER_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    LRRC_APP_UTRAN_CELL_RSLT_STRU       astUtranCellRslt[LRRC_UTRAN_CELL_MAX_NUM]; /* 异频小区上报结果 */
} LRRC_APP_UTRAN_MEAS_RSLT_STRU;
/*****************************************************************************
 结构名    : LRRC_APP_INTER_MEAS_RSLT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈和APP间的接口消息的结构体
*****************************************************************************/
typedef struct
{
    LRRC_APP_GERAN_CELL_RSLT_STRU       astGeranCellRslt[LRRC_GERAN_CELL_MAX_NUM]; /* 异频小区上报结果 */
} LRRC_APP_GERAN_MEAS_RSLT_STRU;
/* end: measurement report 2015-08-18 */

/* add for Conn Meas Filter Optimize begin */
/*****************************************************************************
 结构名    : LRRC_CONN_MEAS_FILTER_INFO_IND_TYPE_ENUM
 结构说明  : LRRC上报滤波结果的类型
*****************************************************************************/
enum LRRC_CONN_MEAS_FILTER_INFO_IND_TYPE_ENUM
{
    LRRC_CONN_MEAS_FILTER_EUTRA_INFO,    /* _H2ASN_MsgChoice LRRC_CONN_MEAS_FILTER_EUTRA_INFO_STRU */
    LRRC_CONN_MEAS_FILTER_GERA_INFO,    /* _H2ASN_MsgChoice LRRC_CONN_MEAS_FILTER_GERA_INFO_STRU */
    LRRC_CONN_MEAS_FILTER_UTRA_INFO,    /* _H2ASN_MsgChoice LRRC_CONN_MEAS_FILTER_UTRA_INFO_STRU */
    LRRC_CONN_MEAS_FILTER_INFO_IND_TYPE_BUTT
};
typedef VOS_UINT32 LRRC_CONN_MEAS_FILTER_INFO_IND_TYPE_ENUM_UNIT32;

/*****************************************************************************
 结构名    : LRRC_CONN_MEAS_FILTER_EUTRA_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LRRC EUTRA滤波结果消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16    usCellId;    /* 小区id */
    VOS_INT16     sRsrp;       /* 滤波后的RSRP测量值 */
    VOS_INT16     sRsrq;       /* 滤波后的RSRQ测量值 */
    VOS_INT16     sRssi;       /* 滤波后的RSSI测量值 */
    VOS_UINT32    ulMeasTimeInterval;    /* 本次测量上报间隔 */
}LRRC_CONN_MEAS_FILTER_EUTRA_INFO_STRU;


/*****************************************************************************
 结构名    :LRRC_GRR_BSIC_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :CELL BAIC Info，协议36331 6.3.4
            usNcc(GSM Network Colour Code)  正常范围:(0..7), 8 表示无效值
            usBcc(GSM Base Station Colour Code)  正常范围:(0..7) , 8 表示无效值
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                              usNcc;
    VOS_UINT16                                              usBcc;
}LRRC_GRR_BSIC_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_CONN_MEAS_FILTER_GERA_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LRRC GERAN滤波结果消息的结构体
*****************************************************************************/
typedef struct
{
    PS_BOOL_ENUM_UINT8        enBsicVaild;    /* BSIC有效标志 */
    VOS_UINT8                 ucResv;         /* 保留位 */
    VOS_INT16                 sRssi;          /* 滤波后的Rssi */
    VOS_UINT16                usArfcn;            /* 频点 */
    LRRC_GRR_BANDINDICATOR_ENUM_UINT16     enBandInd;          /* 频点指示 */
    LRRC_GRR_BSIC_INFO_STRU   stBsic;         /* BSIC标志有效时的BSIC信息 */
    VOS_UINT32                ulMeasTimeInterval;    /* 本次测量上报间隔 */
}LRRC_CONN_MEAS_FILTER_GERA_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_CONN_MEAS_UTRA_TYPE_ENUM
 结构说明  : LRRC上报滤波结果的类型
*****************************************************************************/
enum LRRC_CONN_MEAS_UTRA_TYPE_ENUM
{
    LRRC_CONN_MEAS_FILTER_UTRA_FDD,    /*_H2ASN_Skip*/
    LRRC_CONN_MEAS_FILTER_UTRA_TDD,    /*_H2ASN_Skip*/
    LRRC_CONN_MEAS_FILTER_UTRA_TYPE_BUTT
};
typedef VOS_UINT8 LRRC_CONN_MEAS_UTRA_TYPE_ENUM_UINT8;

/*****************************************************************************
 结构名    : LRRC_CONN_MEAS_FILTER_UTRA_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LRRC FDD UTRA滤波结果消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT16                usCellId;       /* 小区id */
    LRRC_CONN_MEAS_UTRA_TYPE_ENUM_UINT8    enUtraType;    /* Utra的类型 */
    VOS_UINT8                 ucResv;     /* 保留位 */
    VOS_INT16                 sRscp;          /* 滤波后的Rscp */
    VOS_INT16                 sEcN0;          /* 滤波后的EcN0 */
    VOS_UINT32                ulMeasTimeInterval;    /* 本次测量上报间隔 */
}LRRC_CONN_MEAS_FILTER_UTRA_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_CONN_MEAS_FILTER_INFO_IND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LRRC滤波结果消息的结构体
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                          /*_H2ASN_Skip*/
    VOS_UINT32                          ulMsgId;            /*_H2ASN_Skip*/
    LRRC_CONN_MEAS_FILTER_INFO_IND_TYPE_ENUM_UNIT32       enUnionStructSelChoice;/*_H2ASN_Skip*/
    union  /* _H2ASN_Skip */
    {      /* _H2ASN_Skip */
        LRRC_CONN_MEAS_FILTER_EUTRA_INFO_STRU        stEutraConnMeasFilterInfo;   /* _H2ASN_Skip */
        LRRC_CONN_MEAS_FILTER_GERA_INFO_STRU        stGeraConnMeasFilterInfo;   /* _H2ASN_Skip */
        LRRC_CONN_MEAS_FILTER_UTRA_INFO_STRU        stUtraConnMeasFilterInfo;   /* _H2ASN_Skip */
    }u;  /* _H2ASN_Skip */
}LRRC_CONN_MEAS_FILTER_INFO_IND_STRU;
/* add for Conn Meas Filter Optimize end */

/*****************************************************************************
 结构名    : APP_OM_MSG_REDF_STRU
 结构说明  : APP(后台工具)与OM交互的消息体(和APP_OM_MSG_STRU相同)
*****************************************************************************/
typedef struct
{
     VOS_MSG_HEADER                     /*VOS头*/
     VOS_UINT32          ulMsgId;
     APP_MSG_HEADER                     /*APP头*/
     VOS_UINT8           aucPara[4];    /* 参数内容 */
}APP_OM_MSG_REDF_STRU;

extern PS_BOOL_ENUM_UINT8  LRRC_COMM_LoadDspAlready( VOS_VOID );

extern VOS_UINT32 LHPA_InitDsp( VOS_VOID );
extern VOS_VOID LHPA_DbgSendSetWorkMode_toMaterMode(VOS_VOID);
extern VOS_VOID LHPA_DbgSendSetWorkMode_toSlaveMode(VOS_VOID);
extern VOS_UINT32  RRC_RRC_LoadDsp( VOS_VOID );
extern VOS_VOID * LAPP_MemAlloc( VOS_UINT32 ulSize );
extern VOS_UINT32  LApp_MemFree(VOS_VOID *pAddr );
extern VOS_UINT32  LAppSndMsgToLPs(APP_LPS_MSG_STRU  *pstAppToPsMsg );
/* Modified for common CT, 2013-08-10, begin */
extern 	VOS_UINT32 LHPA_InitDsp_ForAT( VOS_VOID );
extern VOS_UINT32 LHPA_InitDspNvForLteTdsCBT(VOS_VOID);
extern VOS_UINT32 LHPA_LoadDspForLteCBT(VOS_VOID);
extern VOS_VOID   LHPA_DbgSetSlaveModeThenMasterMode(VOS_VOID);
/* Modified for common CT, 2013-08-10, end */

/*****************************************************************************
  8 Fuction Extern
*****************************************************************************/


/*****************************************************************************
  9 OTHERS
*****************************************************************************/










#if (VOS_OS_VER != VOS_WIN32)
#pragma pack()
#else
#pragma pack(pop)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of AppRrcInterface.h */
