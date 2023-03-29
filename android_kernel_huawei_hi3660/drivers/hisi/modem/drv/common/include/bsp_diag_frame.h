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

#ifndef _BSP_DIAG_FRAME_H
#define _BSP_DIAG_FRAME_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <osl_types.h>

/* DIAG_SERVICE_HEAD_STRU:ssid4b */
typedef enum _diag_frame_ssid_type
{
    DIAG_FRAME_SSID_APP_CPU   = 0x1,
    DIAG_FRAME_SSID_MODEM_CPU,
    DIAG_FRAME_SSID_LTE_DSP,
    DIAG_FRAME_SSID_LTE_BBP,
    DIAG_FRAME_SSID_GU_DSP,
    DIAG_FRAME_SSID_HIFI,
    DIAG_FRAME_SSID_TDS_DSP,
    DIAG_FRAME_SSID_TDS_BBP,
    DIAG_FRAME_SSID_MCU,
    DIAG_FRAME_SSID_GPU,
    DIAG_FRAME_SSID_GU_BBP,
    DIAG_FRAME_SSID_IOM3,
    DIAG_FRAME_SSID_ISP,
    DIAG_FRAME_SSID_X_DSP,
    DIAG_FRAME_SSID_BUTT

}diag_frame_ssid_type;


/* DIAG_SERVICE_HEAD_STRU:sessionid8b */
#define DIAG_FRAME_MSP_SERVICE_SESSION_ID        (0x1) /* 标识Service与Client之间的连接,固定为1*/

/* DIAG_SERVICE_HEAD_STRU:mt2b */
typedef enum _diag_frame_msgtype_type
{
    DIAG_FRAME_MT_RSV   = 0x0,
    DIAG_FRAME_MT_REQ   = 0x1,
    DIAG_FRAME_MT_CNF   = 0x2,
    DIAG_FRAME_MT_IND   = 0x3

}diag_frame_msgtype_type;


/* MSP_DIAG_STID_STRU:pri4b */
typedef enum _diag_frame_message_type
{
    DIAG_FRAME_MSG_TYPE_RSV   = 0x0,
    DIAG_FRAME_MSG_TYPE_MSP   = 0x1,
    DIAG_FRAME_MSG_TYPE_PS    = 0x2,
    DIAG_FRAME_MSG_TYPE_PHY   = 0x3,
    DIAG_FRAME_MSG_TYPE_BBP   = 0x4,
    DIAG_FRAME_MSG_TYPE_HSO   = 0x5,
    DIAG_FRAME_MSG_TYPE_BSP   = 0x9,
    DIAG_FRAME_MSG_TYPE_ISP   = 0xa,
    DIAG_FRAME_MSG_TYPE_AUDIO = 0xc,
    DIAG_FRAME_MSG_TYPE_APP   = 0xe,
    DIAG_FRAME_MSG_TYPE_BUTT

}diag_frame_message_type;

/* MSP_DIAG_STID_STRU:mode4b */
typedef enum _diag_frame_mode_type
{
    DIAG_FRAME_MODE_LTE  = 0x0,
    DIAG_FRAME_MODE_TDS  = 0x1,
    DIAG_FRAME_MODE_GSM  = 0x2,
    DIAG_FRAME_MODE_UMTS = 0x3,
    DIAG_FRAME_MODE_1X   = 0x4,
    DIAG_FRAME_MODE_HRPD = 0x5,
    DIAG_FRAME_MODE_COMM = 0xf

}diag_frame_mode_type;


/* MSP_DIAG_STID_STRU:pri4b */
typedef enum _diag_frame_msg_sub_type
{
    DIAG_FRAME_MSG_CMD        = 0x0,
    DIAG_FRAME_MSG_AIR     = 0x1,
    DIAG_FRAME_MSG_LAYER   = 0x2,
    DIAG_FRAME_MSG_PRINT   = 0x3,
    DIAG_FRAME_MSG_EVENT   = 0x4,
    DIAG_FRAME_MSG_USER    = 0x5,
    DIAG_FRAME_MSG_VOLTE   = 0x6,
    DIAG_FRAME_MSG_STRUCT  = 0x7,
    DIAG_FRAME_MSG_DOT     = 0x8,
    DIAG_FRAME_MSG_DSP_PRINT   = 0x9,
    DIAG_FRAME_MSG_CNF     = 0xa,
    DIAG_FRAME_MSG_IND     = 0xb,
    DIAG_FRAME_MSG_STAT    = 0x1f

}diag_frame_msg_sub_type;

typedef enum _diag_frame_modem_id
{
    DIAG_FRAME_MODEM_0 = 0x0,
    DIAG_FRAME_MODEM_1 = 0x1,
    DIAG_FRAME_MODEM_2 = 0x2
}diag_frame_modem_id;



/* DIAG_SERVICE_HEAD_STRU:sid8b */
typedef enum _diag_frame_sid_type
{
    DIAG_FRAME_MSP_SID_DEFAULT   = 0x0,
    DIAG_FRAME_MSP_SID_AT_SERVICE,
    DIAG_FRAME_MSP_SID_DIAG_SERVICE,
    DIAG_FRAME_MSP_SID_DATA_SERVICE,
    DIAG_FRAME_MSP_SID_NV_SERVICE,
    DIAG_FRAME_MSP_SID_USIM_SERVICE,
    DIAG_FRAME_MSP_SID_DM_SERVICE,
    DIAG_FRAME_MSP_SID_CBT_SERVICE,
    DIAG_FRAME_MSP_SID_BUTT
}diag_frame_sid_type;

typedef struct
{
    u32 u32module;                        /* 打印信息所在的模块ID */
    u32 u32level;                         /* 输出级别 */
    u32 u32no;                            /* IND标号 */
    //u8  sztext[APPLOG_MAX_USER_BUFF_LEN];   /* 所有打印文本内容，可能包括文件和行号,以'\0'结尾 */
} diag_print_head_stru;
/* 描述 :一级头: service头 */
typedef struct
{
    u32 ulModule;     /* 源模块ID*/
    u32 ulMsgId;      /* ID*/
    u32 ulNo;         /* 序号*/
    u8  aucDta[0];    /* 用户数据缓存区*/
} diag_trans_head_stru;
/* 描述 :一级头: service头 */

typedef struct
{
    u32    sid8b       :8;
    u32    mdmid3b     :3;
    u32    rsv1b       :1;
    u32    ssid4b      :4;
    u32    sessionid8b :8;
    u32    mt2b        :2;
    u32    index4b     :4;
    u32    eof1b       :1;
    u32    ff1b        :1;
    u32    MsgTransId;
    u8     aucTimeStamp[8];
}diag_service_head_stru;

/* 描述 :二级头: DIAG消息头 */
typedef struct
{
    u32    cmdid19b:19;
    u32    sec5b   :5;
    u32    mode4b  :4;
    u32    pri4b   :4;
} diag_stid_stru;


/* 描述 :三级头: 工具软件信息头，用于REQ/CNF消息 */
typedef struct
{
    u32 ulAuid;         /* 原AUID*/
    u32 ulSn;           /* HSO分发，插件命令管理*/
    u8  ucData[0];      /* 参数的数据*/
}MSP_DIAG_DATA_REQ_STRU;


/* 描述 :整体帧结构 */
typedef struct
{
    diag_service_head_stru  stService;

    union
    {
        u32                 u32CmdId;           /* 结构化ID */
        diag_stid_stru      stID;
    };

    u32                     u32MsgLen;
    u8                      aucData[0];
}diag_frame_head_stru;


typedef struct
{
    u32                  u32HisiMagic;   /*"HISI"*/
    u32                  u32DataLen;      /*数据长度*/
    u8                   aucData[0];
}diag_socp_head_stru;

typedef struct
{
    u32  ulAuid;                     /* 原AUID*/
    u32  ulSn;                       /* HSO分发，插件命令管理*/
    u32  ulRet;
}DIAG_BSP_COMM_CNF_STRU;

#define DIAG_SOCP_HEAD_MAGIC               (0x48495349) /*HISI*/

#define DIAG_SOCP_HEAD_SIZE         (sizeof(diag_socp_head_stru))
#define DIAG_FRAME_HEAD_SIZE        (sizeof(diag_frame_head_stru))
#define DIAG_PRINT_HEAD_SIZE        (sizeof(diag_print_head_stru))
#define DIAG_TRANS_HEAD_SIZE        (sizeof(diag_trans_head_stru))

//#ifdef CONFIG_DIAG_FRAME
void bsp_diag_fill_socp_head(diag_socp_head_stru * socp_packet, u32 len);
void bsp_diag_frame_head_init(diag_frame_head_stru* diag_frame);
//#endif


#ifdef __cplusplus
}
#endif

#endif /* end of _BSP_DIAG_FRAME_H*/


