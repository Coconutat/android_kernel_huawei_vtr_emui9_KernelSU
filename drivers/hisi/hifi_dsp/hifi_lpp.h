/*
 * hifi misc driver.
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __HIFI_LPP_H__
#define __HIFI_LPP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/list.h>
#include "global_ddr_map.h"
#include "soc_acpu_baseaddr_interface.h"

/* mailbox mail_len max */
#define MAIL_LEN_MAX	(512)

#ifndef OK
#define OK			 0
#endif

#define ERROR		(-1)
#define BUSY		(-2)
#define NOMEM		(-3)
#define INVAILD 	(-4)
#define COPYFAIL	(-0xFFF)

/* IOCTL入参和出参的SIZE限制 */
#ifndef CLT_VOICE
#define SIZE_LIMIT_PARAM		(256)
#else
#define SIZE_LIMIT_PARAM		(512)
#endif

/* AP侧NV项大小参数 */
#define NVPARAM_COUNT        600         //HIFI NV size is 600

#define NVPARAM_NUMBER       258         //256+2, nv_data(256) + nv_id(2)
#define NVPARAM_START        2           //head protect_number 0x5a5a5a5a
#define NVPARAM_TAIL         2           //tail protect_number 0x5a5a5a5a
#define NVPARAM_TOTAL_SIZE   ((NVPARAM_NUMBER * NVPARAM_COUNT + NVPARAM_START + NVPARAM_TAIL) * sizeof(unsigned short))

/* HIFI专用区 */
#ifdef CONFIG_HISI_FAMA
#define HIFI_UNSEC_BASE_ADDR   (HISI_RESERVED_HIFI_DATA_PHYMEM_BASE_FAMA)
#define ASP_FAMA_PHY_ADDR_DIFF (HISI_RESERVED_HIFI_DATA_PHYMEM_BASE_FAMA - HISI_RESERVED_HIFI_DATA_PHYMEM_BASE)
#else
#define HIFI_UNSEC_BASE_ADDR   (HISI_RESERVED_HIFI_DATA_PHYMEM_BASE)
#define ASP_FAMA_PHY_ADDR_DIFF (0)
#endif

#ifdef CONFIG_HIFI_IPC_3660
#define HIFI_UNSEC_REGION_SIZE              (HISI_RESERVED_HIFI_DATA_PHYMEM_SIZE)
#define HIFI_MUSIC_DATA_SIZE                (0x132000 + 0x32000)
#define PCM_PLAY_BUFF_SIZE                  (0x200000 - 0x32000)
#else
#define HIFI_UNSEC_REGION_SIZE              (0x380000)
#define HIFI_MUSIC_DATA_SIZE                (0x132000)
#define PCM_PLAY_BUFF_SIZE                  (0x100000)
#endif
#define HIFI_AP_NV_DATA_SIZE                (0x4BC00)
#define DRV_DSP_UART_TO_MEM_SIZE            (0x7f000)
#define DRV_DSP_UART_TO_MEM_RESERVE_SIZE    (0x100)
#define DRV_DSP_STACK_TO_MEM_SIZE           (0x1000)
#define HIFI_ICC_DEBUG_SIZE                 (0x13000)
#define HIFI_FLAG_DATA_SIZE                 (0x1000)
#define HIFI_SEC_HEAD_SIZE                  (0x1000)

#define HIFI_AP_MAILBOX_TOTAL_SIZE          (0x10000)
#define CODEC_DSP_OM_DMA_BUFFER_SIZE        (0xF000)
#define CODEC_DSP_OM_DMA_CONFIG_SIZE        (0x80)
#define CODEC_DSP_SOUNDTRIGGER_TOTAL_SIZE   (0xF000)
#define HIFI_PCM_UPLOAD_BUFFER_SIZE         (0x2000)
#define HIFI_AUDIO_EFFECT_PARAM_BUFF_SIZE   (0xC000)
#define HIFI_USB_DRIVER_SHARE_MEM_SIZE      (0x20000)
#ifdef CONFIG_HIFI_IPC_3660
#define HISI_DP_AUDIO_DATA_BUFF_SIZE        (0x40000)
#else
#define HISI_DP_AUDIO_DATA_BUFF_SIZE        (0x0)
#endif
#define HISI_AP_AUDIO_PA_BUFF_SIZE          (0x1800)
#define HISI_AP_AUDIO_WAKEUP_RINGBUFEER_SIZE   (0xF000)
#define HISI_AP_AUDIO_WAKEUP_CAPTURE_SIZE   (0x1000)
#define HISI_AP_AUDIO_WAKEUP_MODEL_SIZE  (0x1000)
#define HIFI_UNSEC_RESERVE_SIZE             (0x4AC00)
#define HIFI_PCM_THREAD_DATA_SIZE           (0x48)

#define HIFI_MUSIC_DATA_LOCATION        (HIFI_UNSEC_BASE_ADDR)
#define PCM_PLAY_BUFF_LOCATION          (HIFI_MUSIC_DATA_LOCATION + HIFI_MUSIC_DATA_SIZE)
#define DRV_DSP_UART_TO_MEM             (PCM_PLAY_BUFF_LOCATION + PCM_PLAY_BUFF_SIZE)
#define DRV_DSP_STACK_TO_MEM            (DRV_DSP_UART_TO_MEM + DRV_DSP_UART_TO_MEM_SIZE)
#define HIFI_ICC_DEBUG_LOCATION         (DRV_DSP_STACK_TO_MEM + DRV_DSP_STACK_TO_MEM_SIZE)
#define HIFI_FLAG_DATA_ADDR             (HIFI_ICC_DEBUG_LOCATION + HIFI_ICC_DEBUG_SIZE)
#define HIFI_SEC_HEAD_BACKUP            (HIFI_FLAG_DATA_ADDR + HIFI_FLAG_DATA_SIZE)
#define HIFI_AP_NV_DATA_ADDR            (HIFI_SEC_HEAD_BACKUP + HIFI_SEC_HEAD_SIZE)
#define HIFI_AP_MAILBOX_BASE_ADDR       (HIFI_AP_NV_DATA_ADDR + HIFI_AP_NV_DATA_SIZE)
#define CODEC_DSP_OM_DMA_BUFFER_ADDR    (HIFI_AP_MAILBOX_BASE_ADDR + HIFI_AP_MAILBOX_TOTAL_SIZE)
#define CODEC_DSP_OM_DMA_CONFIG_ADDR    (CODEC_DSP_OM_DMA_BUFFER_ADDR + CODEC_DSP_OM_DMA_BUFFER_SIZE)
#define CODEC_DSP_SOUNDTRIGGER_BASE_ADDR (CODEC_DSP_OM_DMA_CONFIG_ADDR + CODEC_DSP_OM_DMA_CONFIG_SIZE)
#define HIFI_PCM_UPLOAD_BUFFER_ADDR     (CODEC_DSP_SOUNDTRIGGER_BASE_ADDR + CODEC_DSP_SOUNDTRIGGER_TOTAL_SIZE)
#define HIFI_AUDIO_EFFECT_PARAM_ADDR    (HIFI_PCM_UPLOAD_BUFFER_ADDR + HIFI_PCM_UPLOAD_BUFFER_SIZE)
#define HIFI_USB_DRIVER_SHARE_MEM_ADDR  (HIFI_AUDIO_EFFECT_PARAM_ADDR + HIFI_AUDIO_EFFECT_PARAM_BUFF_SIZE)
#define HISI_DP_AUDIO_DATA_LOCATION     (HIFI_USB_DRIVER_SHARE_MEM_ADDR + HIFI_USB_DRIVER_SHARE_MEM_SIZE)
#define HISI_AP_AUDIO_PA_ADDR           (HISI_DP_AUDIO_DATA_LOCATION + HISI_DP_AUDIO_DATA_BUFF_SIZE)
#define HISI_AP_AUDIO_WAKEUP_RINGBUFFER_ADDR           (HISI_AP_AUDIO_PA_ADDR + HISI_AP_AUDIO_PA_BUFF_SIZE)
#define HISI_AP_AUDIO_WAKEUP_CAPTURE_ADDR (HISI_AP_AUDIO_WAKEUP_RINGBUFFER_ADDR + HISI_AP_AUDIO_WAKEUP_RINGBUFEER_SIZE)
#define HISI_AP_AUDIO_WAKEUP_MODEL_ADDR           (HISI_AP_AUDIO_WAKEUP_CAPTURE_ADDR + HISI_AP_AUDIO_WAKEUP_CAPTURE_SIZE)
#define HIFI_UNSEC_RESERVE_ADDR         (HISI_AP_AUDIO_WAKEUP_MODEL_ADDR + HISI_AP_AUDIO_WAKEUP_MODEL_SIZE)

#define HIFI_OM_LOG_SIZE                (0xA000)
#define HIFI_OM_LOG_ADDR                (DRV_DSP_UART_TO_MEM - HIFI_OM_LOG_SIZE)
#define HIFI_DUMP_BIN_SIZE              (HIFI_AP_MAILBOX_BASE_ADDR + HIFI_AP_MAILBOX_TOTAL_SIZE - HIFI_OM_LOG_ADDR)
#define HIFI_DUMP_BIN_ADDR              (HIFI_OM_LOG_ADDR)

#define DRV_DSP_PANIC_MARK              (HIFI_FLAG_DATA_ADDR)
#define DRV_DSP_UART_LOG_LEVEL          (DRV_DSP_PANIC_MARK + 4)
#define DRV_DSP_UART_TO_MEM_CUR_ADDR    (DRV_DSP_UART_LOG_LEVEL + 4)
#define DRV_DSP_EXCEPTION_NO            (DRV_DSP_UART_TO_MEM_CUR_ADDR + 4)
#define DRV_DSP_IDLE_COUNT_ADDR         (DRV_DSP_EXCEPTION_NO + 4)
#define DRV_DSP_LOADED_INDICATE         (DRV_DSP_IDLE_COUNT_ADDR + 4)
#define DRV_DSP_KILLME_ADDR             (DRV_DSP_LOADED_INDICATE + 4)
#define DRV_DSP_WRITE_MEM_PRINT_ADDR    (DRV_DSP_KILLME_ADDR + 4)
#define DRV_DSP_POWER_STATUS_ADDR       (DRV_DSP_WRITE_MEM_PRINT_ADDR + 4)
#define DRV_DSP_NMI_FLAG_ADDR           (DRV_DSP_POWER_STATUS_ADDR + 4)
#define DRV_DSP_SOCP_FAMA_CONFIG_ADDR   (DRV_DSP_NMI_FLAG_ADDR + 4)
#define DRV_DSP_FLAG_ALP_NOC_CHECK      (DRV_DSP_SOCP_FAMA_CONFIG_ADDR + sizeof(struct drv_fama_config))
#define DRV_DSP_SLT_FLAG_ADDR           (DRV_DSP_FLAG_ALP_NOC_CHECK + 4)
#define DRV_DSP_PCM_THREAD_DATA_ADDR    (DRV_DSP_SLT_FLAG_ADDR + 4)
#define DRV_DSP_FLAG_DATA_RESERVED      (DRV_DSP_PCM_THREAD_DATA_ADDR + HIFI_PCM_THREAD_DATA_SIZE) //TODO: add new flag data here

#define DRV_DSP_POWER_ON                (0x55AA55AA)
#define DRV_DSP_POWER_OFF               (0x55FF55FF)
#define DRV_DSP_KILLME_VALUE            (0xA5A55A5A)
#define DRV_DSP_NMI_COMPLETE            (0xB5B5B5B5)
#define DRV_DSP_NMI_INIT                (0xA5A5A5A5)
#define DRV_DSP_SOCP_FAMA_HEAD_MAGIC    (0x5A5A5A5A)
#define DRV_DSP_SOCP_FAMA_REAR_MAGIC    (0xA5A5A5A5)
#define DRV_DSP_FAMA_ON					(0x1)
#define DRV_DSP_FAMA_OFF				(0x0)

#define HIFI_IMAGE_OCRAMBAK_SIZE        (0x30000)
#define HIFI_SEC_HEAD_SIZE              (0x1000)
#ifdef CONFIG_HIFI_IPC_3660
#define HIFI_SEC_REGION_SIZE            (HISI_RESERVED_HIFI_PHYMEM_SIZE)
#ifdef CONFIG_HIFI_MEMORY_21M
#define HIFI_RUN_SIZE                   (0xF00000)
#else
#define HIFI_RUN_SIZE                   (0xB00000)
#endif
#define HIFI_IMAGE_TCMBAK_SIZE          (0x34000)

#define HIFI_RUN_ITCM_SIZE              (0x9000)
#define HIFI_RUN_DTCM_SIZE              (0x28000)

#ifdef CONFIG_HISI_FAMA
#define HIFI_SEC_BASE_ADDR              (HISI_RESERVED_HIFI_PHYMEM_BASE_FAMA)
#else
#define HIFI_SEC_BASE_ADDR              (HISI_RESERVED_HIFI_PHYMEM_BASE)
#endif

#define HIFI_RUN_LOCATION               (HIFI_SEC_BASE_ADDR)
#define HIFI_IMAGE_OCRAMBAK_LOCATION    (HIFI_RUN_LOCATION + HIFI_RUN_SIZE)
#define HIFI_IMAGE_TCMBAK_LOCATION      (HIFI_IMAGE_OCRAMBAK_LOCATION + HIFI_IMAGE_OCRAMBAK_SIZE)
#define HIFI_SEC_HEAD_LOCATION          (HIFI_IMAGE_TCMBAK_LOCATION + HIFI_IMAGE_TCMBAK_SIZE)
#define HIFI_SEC_RESERVE_ADDR           (HIFI_SEC_HEAD_LOCATION + HIFI_SEC_HEAD_SIZE)

#define DRV_WATCHDOG_BASE_ADDR          (SOC_ACPU_ASP_WD_BASE_ADDR)
#else
#define HIFI_SEC_REGION_SIZE            (0xB80000)
#define HIFI_RUN_SIZE                   (0xB00000)
#define HIFI_IMAGE_TCMBAK_SIZE          (0x1E000)
#define HIFI_SEC_RESERVE_SIZE           (0x31000)

#define HIFI_RUN_ITCM_SIZE              (0x6000)
#define HIFI_RUN_DTCM_SIZE              (0x18000)

#define HIFI_SEC_BASE_ADDR              (HIFI_UNSEC_BASE_ADDR + HIFI_UNSEC_REGION_SIZE) /* austin dallas */
#define HIFI_IMAGE_OCRAMBAK_LOCATION    (HIFI_SEC_BASE_ADDR)
#define HIFI_IMAGE_TCMBAK_LOCATION      (HIFI_IMAGE_OCRAMBAK_LOCATION + HIFI_IMAGE_OCRAMBAK_SIZE)
#define HIFI_SEC_HEAD_LOCATION          (HIFI_IMAGE_TCMBAK_LOCATION + HIFI_IMAGE_TCMBAK_SIZE)
#define HIFI_SEC_RESERVE_ADDR           (HIFI_SEC_HEAD_LOCATION + HIFI_SEC_HEAD_SIZE)
#define HIFI_RUN_LOCATION               (HIFI_SEC_RESERVE_ADDR + HIFI_SEC_RESERVE_SIZE)

#define DRV_WATCHDOG_BASE_ADDR          (SOC_ACPU_ASP_Watchdog_BASE_ADDR)
#endif

#define SIZE_PARAM_PRIV                         (206408) /*refer from function dsp_nv_init in dsp_soc_para_ctl.c  */
#define HIFI_SYS_MEM_ADDR                       (HIFI_RUN_LOCATION)
#define SYS_TIME_STAMP_REG                      (SOC_ACPU_SCTRL_BASE_ADDR + 0x534)

/* 接收HIFI消息，前部cmd_id占用的字节数 */
#define SIZE_CMD_ID 	   (8)

/* notice主动上报一次缓冲 */
#define REV_MSG_NOTICE_ID_MAX		2

#define ACPU_TO_HIFI_ASYNC_CMD	  0xFFFFFFFF

#define BUFFER_NUM	8
#define MAX_NODE_COUNT 10

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

typedef enum HIFI_MSG_ID_ {

	/*DTS command id from ap*/
	ID_AP_AUDIO_SET_DTS_ENABLE_CMD		= 0xDD36,
	ID_AP_AUDIO_SET_DTS_DEV_CMD			= 0xDD38,
	ID_AP_AUDIO_SET_DTS_GEQ_CMD			= 0xDD39,
	ID_AP_AUDIO_SET_AUDIO_EFFECT_PARAM_CMD    = 0xDD3A,
	ID_AP_AUDIO_SET_DTS_GEQ_ENABLE_CMD	= 0xDD3B,

	/* AP和HIFI的控制外置耳机HIFI codec交互消息 */
	ID_AP_AUDIO_SET_EXCODEC_ENABLE_CMD    = 0xDD3D,

	/* Voice Record */
	ID_AP_HIFI_VOICE_RECORD_START_CMD	= 0xDD40,
	ID_AP_HIFI_VOICE_RECORD_STOP_CMD	= 0xDD41,

	/* voicePP MSG_ID */
	ID_AP_VOICEPP_START_REQ				= 0xDD42,						/* 启动VOICEPP通话 */
	ID_VOICEPP_MSG_START				= ID_AP_VOICEPP_START_REQ,
	ID_VOICEPP_AP_START_CNF				= 0xDD43,
	ID_AP_VOICEPP_STOP_REQ				= 0xDD44,						/* 终止VOICEPP通话 */
	ID_VOICEPP_AP_STOP_CNF				= 0xDD45,
	ID_VOICEPP_MSG_END					= 0xDD4A,

	ID_AP_AUDIO_PLAY_START_REQ			= 0xDD51,/* AP启动Hifi audio player request命令 */
	ID_AUDIO_AP_PLAY_START_CNF			= 0xDD52,/* Hifi启动audio player后回复AP confirm命令 */
	ID_AP_AUDIO_PLAY_PAUSE_REQ			= 0xDD53,/* AP停止Hifi audio player request命令 */
	ID_AUDIO_AP_PLAY_PAUSE_CNF			= 0xDD54,/* Hifi停止audio player后回复AP confirm命令 */
	ID_AUDIO_AP_PLAY_DONE_IND			= 0xDD56,/* Hifi通知AP audio player一块数据播放完毕或者播放中断indication */
	ID_AP_AUDIO_PLAY_UPDATE_BUF_CMD 	= 0xDD57,/* AP通知Hifi新数据块更新command */
	ID_AP_AUDIO_PLAY_QUERY_TIME_REQ 	= 0xDD59,/* AP查询Hifi audio player播放进度request命令 */
	ID_AP_AUDIO_PLAY_WAKEUPTHREAD_REQ	= 0xDD5A,
	ID_AUDIO_AP_PLAY_QUERY_TIME_CNF 	= 0xDD60,/* Hifi回复AP audio player播放进度confirm命令 */
	ID_AP_AUDIO_PLAY_QUERY_STATUS_REQ	= 0xDD61,/* AP查询Hifi audio player播放状态request命令 */
	ID_AUDIO_AP_PLAY_QUERY_STATUS_CNF	= 0xDD62,/* Hifi回复AP audio player播放状态confirm命令 */
	ID_AP_AUDIO_PLAY_SEEK_REQ			= 0xDD63,/* AP seek Hifi audio player到某一位置request命令 */
	ID_AUDIO_AP_PLAY_SEEK_CNF			= 0xDD64,/* Hifi回复AP seek结果confirm命令 */
	ID_AP_AUDIO_PLAY_SET_VOL_CMD		= 0xDD70,/* AP设置音量命令 */
	ID_AP_AUDIO_RECORD_PCM_HOOK_CMD		= 0xDD7A,/* AP 通知HIFI开始抓取PCM数据 */
	ID_AUDIO_AP_UPDATE_PCM_BUFF_CMD      = 0xDD7C,
	ID_AP_AUDIO_DYN_EFFECT_GET_PARAM     = 0xDD7D,
	ID_AP_AUDIO_DYN_EFFECT_GET_PARAM_CNF = 0xDD7E,
	ID_AP_AUDIO_DYN_EFFECT_TRIGGER       = 0xDD7F,
	/* enhance msgid between ap and hifi */
	ID_AP_HIFI_ENHANCE_START_REQ		= 0xDD81,
	ID_HIFI_AP_ENHANCE_START_CNF		= 0xDD82,
	ID_AP_HIFI_ENHANCE_STOP_REQ 		= 0xDD83,
	ID_HIFI_AP_ENHANCE_STOP_CNF 		= 0xDD84,
	ID_AP_HIFI_ENHANCE_SET_DEVICE_REQ	= 0xDD85,
	ID_HIFI_AP_ENHANCE_SET_DEVICE_CNF	= 0xDD86,

	/* audio enhance msgid between ap and hifi */
	ID_AP_AUDIO_ENHANCE_SET_DEVICE_IND	= 0xDD91,
	ID_AP_AUDIO_MLIB_SET_PARA_IND		= 0xDD92,
	ID_AP_AUDIO_CMD_SET_SOURCE_CMD		= 0xDD95,
	ID_AP_AUDIO_CMD_SET_DEVICE_CMD		= 0xDD96,
	ID_AP_AUDIO_CMD_SET_MODE_CMD		= 0xDD97,
	ID_AP_AUDIO_CMD_SET_ANGLE_CMD		= 0xDD99,

	/* for 3mic */
	ID_AP_AUDIO_ROUTING_COMPLETE_REQ    = 0xDDC0,/*AP 通知HIFI 3Mic/4Mic 通路已建立*/
	ID_AUDIO_AP_DP_CLK_EN_IND           = 0xDDC1,/* HIFI 通知A核打开或关闭Codec DP时钟 */
	ID_AP_AUDIO_DP_CLK_STATE_IND        = 0xDDC2,/* A核通知HIFI ，Codec DP时钟状态( 打开或关闭) */
	ID_AUDIO_AP_OM_DUMP_CMD             = 0xDDC3,/* HIFI 通知A核dump日志 */
	ID_AUDIO_AP_FADE_OUT_REQ            = 0xDDC4,/* HIFI通知AP做淡出处理 */
	ID_AP_AUDIO_FADE_OUT_IND            = 0xDDC5,/* AP通知HIFI淡出完毕 */

	ID_AUDIO_AP_OM_CMD                  = 0xDDC9,
	ID_AP_AUDIO_STR_CMD                 = 0xDDCB,/* AP给HIFI发送一个字符串，具体含义hifi中解析 */
	ID_AUDIO_AP_VOICE_BSD_PARAM_CMD     = 0xDDCC,/* VOICE BSD 参数上报 */

	ID_AP_ENABLE_MODEM_LOOP_REQ         = 0xDDCD,/* the audio hal notify HIFI to start/stop  MODEM LOOP*/
	ID_AUDIO_AP_3A_CMD                  = 0xDDCE,
	ID_AP_ENABLE_AT_DSP_LOOP_REQ        = 0xDDCF,/* notify HIFI to start/stop hifidsp LOOP from slimbus to i2s */
	ID_AP_HIFI_REQUEST_VOICE_PARA_REQ   = 0xDF00, /*AP REQUEST VOICE MSG*/
	ID_HIFI_AP_REQUEST_VOICE_PARA_CNF   = 0xDF01, /*HIFI REPLAY VOICE MSG*/
	ID_AP_HIFI_REQUEST_SET_PARA_CMD     = 0xDF08,    /* HIFI SET PARAM MSG */
	ID_AP_HIFI_REQUEST_GET_PARA_CMD     = 0xDF09,    /* HIFI GET PARAM MSG */
	ID_AP_HIFI_REQUEST_GET_PARA_CNF     = 0xDF0A,    /* HIFI GET PARAM MSG */
	ID_HIFI_AP_BIGDATA_CMD              = 0xDF10,   /*bigdata*/
} HIFI_MSG_ID;

typedef enum HI6402_DP_CLK_STATE_ {
	HI6402_DP_CLK_OFF = 0x0,
	HI6402_DP_CLK_ON = 0x1,
} HI6402_DP_CLK_STATE;

enum hifi_dsp_platform_type {
	HIFI_DSP_PLATFORM_ASIC,
	HIFI_DSP_PLATFORM_FPGA,
};

/*处理hifi回复消息，记录cmd_id和数据*/
typedef struct {
	unsigned char *mail_buff;		/* 邮箱数据接收的buff */
	unsigned int mail_buff_len;
	unsigned int cmd_id;			/* 邮箱接收数据前4个字节是cmd_id */
	unsigned char *out_buff_ptr;	/* 指向mail_buff cmd_id后的位置 */
	unsigned int out_buff_len;
} rev_msg_buff;

struct recv_request {
	struct list_head recv_node;
	rev_msg_buff rev_msg;
};

struct misc_recmsg_param {
	unsigned short	msgID;
	unsigned short	playStatus;
};

struct common_hifi_cmd{
	unsigned short msg_id;
	unsigned short reserve;
	unsigned int   value;
};

struct dp_clk_request {
	struct list_head dp_clk_node;
	struct common_hifi_cmd dp_clk_msg;
};

typedef struct {
	unsigned short	down_cpu_utilization;    /* DDR下调cpu 占用率水线 */
	unsigned short	up_cpu_utilization;      /* DDR上调cpu 占用率水线 */
	unsigned short	ddr_freq;               /* 设置ddr频率*/
	unsigned short	is_vote_ddr;             /* 是否是当前投票的DDR频点 */
} audio_vote_ddr_freq_stru;

typedef struct {
	unsigned short	enable_vote_ddr;         /* 是否使能动态DDR投票,设置为0表示只打印cpu占用率,不会投票 */
	unsigned short	ddr_freq_count;          /* ddr调频列表长度 */
	unsigned short	check_interval;         /* DDR投票cpu占用率检测间隔,单位毫秒 */
	unsigned short	report_interval;        /* cpu占用率上报间隔,单位秒 */
	audio_vote_ddr_freq_stru *pst_vote_ddr_freq;  /* DDR调频策略列表,可变长度,长度即uhwDdrFreqCount */
} audio_cpu_load_cfg_stru;

struct drv_fama_config {
	unsigned int head_magic;
	unsigned int flag;
	unsigned int rear_magic;
};

enum usbaudio_ioctl_type
{
	USBAUDIO_QUERY_INFO = 0,
	USBAUDIO_USB_POWER_RESUME,
	USBAUDIO_NV_ISREADY,
	USBAUDIO_MSG_MAX
};

struct usbaudio_ioctl_input {
	unsigned int msg_type;
	unsigned int input1;
	unsigned int input2;
};

int hifi_send_msg(unsigned int mailcode, void *data, unsigned int length);
void hifi_get_log_signal(void);
void hifi_release_log_signal(void);
void sochifi_watchdog_send_event(void);
unsigned long try_copy_from_user(void *to, const void __user *from, unsigned long n);
unsigned long try_copy_to_user(void __user *to, const void *from, unsigned long n);
enum hifi_dsp_platform_type hifi_misc_get_platform_type(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hifi_lpp.h */
