/*
 * hisi_pcm_hifi.c -- ALSA SoC HISI PCM HIFI driver
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/sched/rt.h>
#include <linux/ion.h>
#include <linux/hisi/hisi_ion.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/hwdep.h>
#include "drv_mailbox_cfg.h"

#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif

#include "hifi_lpp.h"
#include "hisi_pcm_hifi.h"
#include "hisi_snd_log.h"
#include "hisi_pcm_ion.h"

/*lint -e750 -e785 -e838 -e749 -e747 -e611 -e570 -e647 -e574*/

#define UNUSED_PARAMETER(x) (void)(x)

#define HISI_PCM_HIFI "hi6210-hifi"
#define HISI_PCM_ION_CLIENT_NAME    "hisi_pcm_ion"

/*
 * PLAYBACK SUPPORT FORMATS
 * BITS : 8/16/24  18/20
 * LITTLE_ENDIAN / BIG_ENDIAN
 * MONO / STEREO
 * UNSIGNED / SIGNED
 */
#define HISI_PCM_PB_FORMATS  (SNDRV_PCM_FMTBIT_S8 | \
		SNDRV_PCM_FMTBIT_U8 | \
		SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S16_BE | \
		SNDRV_PCM_FMTBIT_U16_LE | \
		SNDRV_PCM_FMTBIT_U16_BE | \
		SNDRV_PCM_FMTBIT_S24_LE | \
		SNDRV_PCM_FMTBIT_S24_BE | \
		SNDRV_PCM_FMTBIT_U24_LE | \
		SNDRV_PCM_FMTBIT_U24_BE)

/*
 * PLAYBACK SUPPORT RATES
 * 8/11.025/16/22.05/32/44.1/48/88.2/96kHz
 */
#define HISI_PCM_PB_RATES    (SNDRV_PCM_RATE_8000_48000 | \
		SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_176400 | \
		SNDRV_PCM_RATE_192000 | \
		SNDRV_PCM_RATE_384000)

#define HISI_PCM_PB_MIN_CHANNELS  ( 1 )
#define HISI_PCM_PB_MAX_CHANNELS  ( 2 )
/* Assume the FIFO size */
#define HISI_PCM_PB_FIFO_SIZE     ( 16 )

/* CAPTURE SUPPORT FORMATS : SIGNED 16/24bit */
#define HISI_PCM_CP_FORMATS  ( SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

/* CAPTURE SUPPORT RATES : 48/96kHz */
#define HISI_PCM_CP_RATES    ( SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 )

#define HISI_PCM_CP_MIN_CHANNELS  ( 1 )
#define HISI_PCM_CP_MAX_CHANNELS  ( 6 )
/* Assume the FIFO size */
#define HISI_PCM_CP_FIFO_SIZE     ( 32 )
#define HISI_PCM_MODEM_RATES      ( SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000)
#define HISI_PCM_BT_RATES         ( SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 )
#define HISI_PCM_FM_RATES         ( SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 )

#define HISI_PCM_MAX_BUFFER_SIZE  ( 192 * 1024 )    /* 0x30000 */
#define HISI_PCM_BUFFER_SIZE_MM   ( 32 * 1024 )
#define HISI_PCM_MIN_BUFFER_SIZE  ( 32 )
#define HISI_PCM_MAX_PERIODS      ( 32 )
#define HISI_PCM_MIN_PERIODS      ( 2 )
#define HISI_PCM_WORK_DELAY_1MS   ( 33 )    /* 33 equals 1ms */
#define HISI_PCM_CYC_SUB(Cur, Pre, CycLen)                    \
	(((Cur) < (Pre)) ? (((CycLen) - (Pre)) + (Cur)) : ((Cur) - (Pre)))

#ifndef OK
#define OK              0
#endif
#ifndef ERROR
#define ERROR           -1
#endif

#undef NULL
#define NULL ((void *)0)

#define ALSA_TIMEOUT_MILLISEC 40
#define CHECK_FLAG_TIMEOUT    (160)     /* 5ms */
#define CHECK_UPDATE_TIMEOUT  (350)     /* 10ms */

PCM_DMA_BUF_CONFIG  g_pcm_dma_buf_config[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {
	{/*normal*/
		{PCM_DMA_BUF_0_PLAYBACK_BASE, PCM_DMA_BUF_0_PLAYBACK_LEN},
		{PCM_DMA_BUF_0_CAPTURE_BASE, PCM_DMA_BUF_0_CAPTURE_LEN}
	},
	{ {0, 0}, {0, 0} },/*modem*/
	{ {0, 0}, {0, 0} },/*fm*/
	{ {0, 0}, {0, 0} },/*offload*/
	{/*direct*/
		{PCM_DMA_BUF_1_PLAYBACK_BASE, PCM_DMA_BUF_1_PLAYBACK_LEN},
		{PCM_DMA_BUF_1_CAPTURE_BASE, PCM_DMA_BUF_1_CAPTURE_LEN}
	},
	{/*lowlatency*/
		{PCM_DMA_BUF_2_PLAYBACK_BASE, PCM_DMA_BUF_2_PLAYBACK_LEN},
		{PCM_DMA_BUF_2_CAPTURE_BASE, PCM_DMA_BUF_2_CAPTURE_LEN}
	},
	{/*mmap*/
		{PCM_DMA_BUF_3_PLAYBACK_BASE, PCM_DMA_BUF_3_PLAYBACK_LEN},
		{PCM_DMA_BUF_3_CAPTURE_BASE, PCM_DMA_BUF_3_CAPTURE_LEN}
	},
};

/* supported sample rates */
static const unsigned int freq[] = {
	8000,   11025,  12000,  16000,
	22050,  24000,  32000,  44100,
	48000,  88200,  96000,  176400,
	192000, 384000,
};

static const struct snd_soc_component_driver hisi_pcm_component = {
	.name   = HISI_PCM_HIFI,
};

static u64 hisi_pcm_dmamask           = (u64)(0xffffffff);

static struct snd_soc_dai_driver hisi_pcm_dai[] =
{
	{
		.name = "hi6210-mm",
		.playback = {
			.stream_name  = "hi6210-mm Playback",
			.channels_min = HISI_PCM_PB_MIN_CHANNELS,
			.channels_max = HISI_PCM_PB_MAX_CHANNELS,
			.rates        = HISI_PCM_PB_RATES,
			.formats      = HISI_PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "hi6210-mm Capture",
			.channels_min = HISI_PCM_CP_MIN_CHANNELS,
			.channels_max = HISI_PCM_CP_MAX_CHANNELS,
			.rates        = HISI_PCM_CP_RATES,
			.formats      = HISI_PCM_CP_FORMATS
		},
	},
	{
		.name = "hi6210-modem",
		.playback = {
			.stream_name  = "hi6210-modem Playback",
			.channels_min = HISI_PCM_PB_MIN_CHANNELS,
			.channels_max = HISI_PCM_PB_MAX_CHANNELS,
			.rates        = HISI_PCM_MODEM_RATES,
			.formats      = HISI_PCM_PB_FORMATS
		},
	},
	{
		.name = "hi6210-fm",
		.playback = {
			.stream_name  = "hi6210-fm Playback",
			.channels_min = HISI_PCM_PB_MIN_CHANNELS,
			.channels_max = HISI_PCM_PB_MAX_CHANNELS,
			.rates        = HISI_PCM_FM_RATES,
			.formats      = HISI_PCM_PB_FORMATS
		},
	},
	{
		.name = "hi6210-lpp",
		.playback = {
			.stream_name  = "hi6210-lpp Playback",
			.channels_min = HISI_PCM_PB_MIN_CHANNELS,
			.channels_max = HISI_PCM_PB_MAX_CHANNELS,
			.rates        = HISI_PCM_PB_RATES,
			.formats      = HISI_PCM_PB_FORMATS
		},
	},
	{
		.name = "hi6210-direct",
		.playback = {
			.stream_name  = "hi6210-direct Playback",
			.channels_min = HISI_PCM_PB_MIN_CHANNELS,
			.channels_max = HISI_PCM_PB_MAX_CHANNELS,
			.rates        = HISI_PCM_PB_RATES,
			.formats      = HISI_PCM_PB_FORMATS
		},
	},
	{
		.name = "hi6210-fast",
		.playback = {
			.stream_name  = "hi6210-fast Playback",
			.channels_min = HISI_PCM_PB_MIN_CHANNELS,
			.channels_max = HISI_PCM_PB_MAX_CHANNELS,
			.rates        = HISI_PCM_PB_RATES,
			.formats      = HISI_PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "hi6210-fast Capture",
			.channels_min = HISI_PCM_CP_MIN_CHANNELS,
			.channels_max = HISI_PCM_CP_MAX_CHANNELS,
			.rates        = HISI_PCM_CP_RATES,
			.formats      = HISI_PCM_CP_FORMATS
		},
	},
	{
		.name = "hisi-pcm-mmap",
		.playback = {
			.stream_name  = "hisi-pcm-mmap Playback",
			.channels_min = HISI_PCM_PB_MIN_CHANNELS,
			.channels_max = HISI_PCM_PB_MAX_CHANNELS,
			.rates        = HISI_PCM_PB_RATES,
			.formats      = HISI_PCM_PB_FORMATS
		},
		.capture = {
			.stream_name  = "hisi-pcm-mmap Capture",
			.channels_min = HISI_PCM_CP_MIN_CHANNELS,
			.channels_max = HISI_PCM_CP_MAX_CHANNELS,
			.rates        = HISI_PCM_CP_RATES,
			.formats      = HISI_PCM_CP_FORMATS
		},
	},
};

/* define the capability of playback channel */
static const struct snd_pcm_hardware hisi_pcm_hardware_playback =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min     = HISI_PCM_PB_MIN_CHANNELS,
	.channels_max     = HISI_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.period_bytes_min = HISI_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.periods_min      = HISI_PCM_MIN_PERIODS,
	.periods_max      = HISI_PCM_MAX_PERIODS,
	.fifo_size        = HISI_PCM_PB_FIFO_SIZE,
};

/* define the capability of capture channel */
static const struct snd_pcm_hardware hisi_pcm_hardware_capture =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.rates            = SNDRV_PCM_RATE_48000,
	.channels_min     = HISI_PCM_CP_MIN_CHANNELS,
	.channels_max     = HISI_PCM_CP_MAX_CHANNELS,
	.buffer_bytes_max = HISI_PCM_MAX_BUFFER_SIZE,
	.period_bytes_min = HISI_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = HISI_PCM_MAX_BUFFER_SIZE,
	.periods_min      = HISI_PCM_MIN_PERIODS,
	.periods_max      = HISI_PCM_MAX_PERIODS,
	.fifo_size        = HISI_PCM_CP_FIFO_SIZE,
};

/* define the capability of playback channel for direct*/
static const struct snd_pcm_hardware hisi_pcm_hardware_direct_playback =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S24_LE,
	.channels_min     = HISI_PCM_PB_MIN_CHANNELS,
	.channels_max     = HISI_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_DIRECT_LEN,
	.period_bytes_min = HISI_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_DIRECT_LEN,
	.periods_min      = HISI_PCM_MIN_PERIODS,
	.periods_max      = HISI_PCM_MAX_PERIODS,
	.fifo_size        = HISI_PCM_PB_FIFO_SIZE,
};

/* define the capability of playback channel for Modem */
static const struct snd_pcm_hardware hisi_pcm_hardware_modem_playback =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_BLOCK_TRANSFER
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min     = HISI_PCM_PB_MIN_CHANNELS,
	.channels_max     = HISI_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = HISI_PCM_MAX_BUFFER_SIZE,
	.period_bytes_min = HISI_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = HISI_PCM_MAX_BUFFER_SIZE,
	.periods_min      = HISI_PCM_MIN_PERIODS,
	.periods_max      = HISI_PCM_MAX_PERIODS,
	.fifo_size        = HISI_PCM_PB_FIFO_SIZE,
};

/* define the capability of playback channel for lowlatency */
static const struct snd_pcm_hardware hisi_pcm_hardware_lowlatency_playback =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min     = HISI_PCM_PB_MIN_CHANNELS,
	.channels_max     = HISI_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.period_bytes_min = HISI_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.periods_min      = HISI_PCM_MIN_PERIODS,
	.periods_max      = HISI_PCM_MAX_PERIODS,
	.fifo_size        = HISI_PCM_PB_FIFO_SIZE,
};

/* define the capability of capture channel for lowlatency */
static const struct snd_pcm_hardware hisi_pcm_hardware_lowlatency_capture =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.rates            = SNDRV_PCM_RATE_48000,
	.channels_min     = HISI_PCM_CP_MIN_CHANNELS,
	.channels_max     = HISI_PCM_CP_MAX_CHANNELS,
	.buffer_bytes_max = HISI_PCM_MAX_BUFFER_SIZE,
	.period_bytes_min = HISI_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = HISI_PCM_MAX_BUFFER_SIZE,
	.periods_min      = HISI_PCM_MIN_PERIODS,
	.periods_max      = HISI_PCM_MAX_PERIODS,
	.fifo_size        = HISI_PCM_CP_FIFO_SIZE,
};

/* define the capability of playback channel for mmap device */
static const struct snd_pcm_hardware hisi_pcm_hardware_mmap_playback = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min     = HISI_PCM_PB_MIN_CHANNELS,
	.channels_max     = HISI_PCM_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_MMAP_MAX_SIZE,
	.period_bytes_min = HISI_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_MMAP_MAX_SIZE,
	.periods_min      = HISI_PCM_MIN_PERIODS,
	.periods_max      = HISI_PCM_MAX_PERIODS,
	.fifo_size        = HISI_PCM_PB_FIFO_SIZE,
};

/* define the capability of capture channel for mmap device */
static const struct snd_pcm_hardware hisi_pcm_hardware_mmap_capture = {
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.rates            = SNDRV_PCM_RATE_48000,
	.channels_min     = HISI_PCM_CP_MIN_CHANNELS,
	.channels_max     = HISI_PCM_CP_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_MMAP_MAX_SIZE,
	.period_bytes_min = HISI_PCM_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_MMAP_MAX_SIZE,
	.periods_min      = HISI_PCM_MIN_PERIODS,
	.periods_max      = HISI_PCM_MAX_PERIODS,
	.fifo_size        = HISI_PCM_CP_FIFO_SIZE,
};

static const struct snd_pcm_hardware *hisi_pcm_hw_info[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {
	{&hisi_pcm_hardware_playback, &hisi_pcm_hardware_capture}, /* PCM_DEVICE_NORMAL */
	{NULL, NULL}, /* PCM_DEVICE_MODEM */
	{NULL, NULL}, /* PCM_DEVICE_FM */
	{NULL, NULL}, /* PCM_DEVICE_OFFLOAD */
	{&hisi_pcm_hardware_direct_playback, NULL}, /* PCM_DEVICE_DIRECT */
	{&hisi_pcm_hardware_lowlatency_playback, &hisi_pcm_hardware_lowlatency_capture}, /* PCM_DEVICE_LOW_LATENCY */
	{&hisi_pcm_hardware_mmap_playback, &hisi_pcm_hardware_mmap_capture}, /* PCM_DEVICE_MMAP */
};

struct pcm_thread_stream_info {
	/* communction paras */
	volatile uint32_t finish_flag;          /* is hifi updated play buffer */
	volatile uint32_t data_buf_offset;      /* new frame data addr offset */
	volatile uint32_t data_buf_size;        /* new frame buffer size */
	/* om info */
	volatile uint32_t set_time;             /* when kernel set new buffer */
	volatile uint32_t get_time;             /* when hifi get new buffer */
	volatile uint32_t check_get_time;       /* thread check finish_flag changed time */
	volatile uint32_t last_get_time;        /* last update buffer time */
	volatile uint32_t data_avail;           /* alsa queued space */
};

struct pcm_hifi_share_data {
	struct pcm_thread_stream_info stream_share_data[PCM_STREAM_MAX];
};

struct pcm_thread_info {
	struct task_struct *pcm_run_thread;
	struct pcm_hifi_share_data *hifi_share_data;  /* communication share mem with hifi */
	atomic_t using_thread_cnt;	/* record the number of substreams using thread */
	struct hisi_pcm_runtime_data *runtime_data[PCM_DEVICE_TOTAL][PCM_STREAM_MAX];
};

struct hisi_pcm_data {
	struct pcm_thread_info thread_info;	/* currently only lowlatency use */
};

extern int mailbox_get_timestamp(void);
static int hisi_pcm_notify_set_buf(struct snd_pcm_substream *substream);
static irq_rt_t hisi_pcm_notify_recv_isr(void *usr_para, void *mail_handle, unsigned int mail_len);
static irq_rt_t hisi_pcm_isr_handle(struct snd_pcm_substream *substream);

static bool _is_valid_pcm_device(int pcm_device)
{
	switch (pcm_device) {
	case PCM_DEVICE_NORMAL:
	case PCM_DEVICE_DIRECT:
	case PCM_DEVICE_LOW_LATENCY:
	case PCM_DEVICE_MMAP:
		return true;
	default:
		return false;
	}
}

static bool _is_valid_substream(struct snd_pcm_substream *substream)
{
	int pcm_device = 0;
	int pcm_mode = 0;

	if (substream == NULL) {
		loge("substream is NULL\n");
		return false;
	}

	if (substream->runtime == NULL) {
		if (hifi_misc_get_platform_type() == HIFI_DSP_PLATFORM_ASIC) {
			loge("substream runtime is NULL\n");
		}
		return false;
	}

	pcm_device = substream->pcm->device;
	pcm_mode = substream->stream;

	if (!_is_valid_pcm_device(pcm_device)) {
		loge("pcm_device: %d is not support\n", pcm_device);
		return false;
	}

	if ((pcm_mode != SNDRV_PCM_STREAM_PLAYBACK) &&
		(pcm_mode != SNDRV_PCM_STREAM_CAPTURE)) {
		loge("pcm_mode: %d is invalid\n", pcm_mode);
		return false;
	}

	return true;
}

static bool _is_pcm_device_using_thread(uint16_t pcm_device)
{
	switch (pcm_device) {
	case PCM_DEVICE_LOW_LATENCY:
		return true;
	default:
		return false;
	}
}

static void _dump_thread_info(struct pcm_thread_info *thread_info,
	struct snd_pcm_substream *substream, const char *str)
{
	uint32_t pcm_mode = substream->stream;
	struct pcm_hifi_share_data *share_data = thread_info->hifi_share_data;
	struct pcm_thread_stream_info *stream_info = &share_data->stream_share_data[pcm_mode];

	if (stream_info == NULL) {
		loge("pcm mode: %d, thread_share_data is null!\n", pcm_mode);
		return;
	}

	logw("%s: finish_flag-%d, get_time-%d, set_time:%d, last_get_time-%d, check_get_time-%d\n",
		str,
		stream_info->finish_flag,
		stream_info->get_time,
		stream_info->set_time,
		stream_info->last_get_time,
		stream_info->check_get_time);
}

static bool _is_one_frame_finished(struct pcm_thread_info *thread_info,
	struct snd_pcm_substream *substream)
{
	int ret = false;
	uint32_t pcm_mode = substream->stream;
	struct pcm_hifi_share_data *share_data = thread_info->hifi_share_data;
	struct pcm_thread_stream_info *stream_info = &share_data->stream_share_data[pcm_mode];
	const char *timeout_str[PCM_STREAM_MAX] = {
		"thread check play timeout",
		"thread check capture timeout"};
	int get_data_interval = 0;

	if (stream_info == NULL) {
		loge("pcm thread share data is null!\n");
		return false;
	}

	if (stream_info->finish_flag) {
		stream_info->finish_flag = false;
		stream_info->check_get_time = mailbox_get_timestamp();

		get_data_interval = stream_info->check_get_time - stream_info->get_time;
		if (get_data_interval > CHECK_FLAG_TIMEOUT) {
			_dump_thread_info(thread_info, substream, timeout_str[pcm_mode]);
		}
		ret = true;
	}

	return ret;
}

static void _thread_update_frame(struct pcm_thread_info *thread_info,
	bool *should_schedule)
{
	uint32_t stream = 0;
	uint32_t device = 0;
	struct hisi_pcm_runtime_data *prtd = NULL;

	for (device = 0; device < PCM_DEVICE_TOTAL; device++) {
		for (stream = 0; stream < PCM_STREAM_MAX; stream++) {
			if (!_is_pcm_device_using_thread(device))
				continue;

			prtd = thread_info->runtime_data[device][stream];
			if (prtd == NULL)
				continue;

			if (prtd->status != STATUS_RUNNING)
				continue;

			if (_is_one_frame_finished(thread_info, prtd->substream)) {
				hisi_pcm_isr_handle(prtd->substream);
				*should_schedule = false;
			}
		}
	}
}

static int pcm_check_frame_thread(void *data)
{
	struct pcm_thread_info *thread_info = data;
	bool should_schedule = false;

	while (!kthread_should_stop()) {
		/* if playback & capture stream are all closed, thread go to sleep,
		 * and wait to be wakeup until next trigger start.
		 */
		if (atomic_read(&thread_info->using_thread_cnt) == 0) {
			logi("set lowlatency check frame thread to sleep state!\n");
			set_current_state(TASK_INTERRUPTIBLE); /*lint !e446 !e666*/
			schedule();
		}

		should_schedule = true;

		_thread_update_frame(thread_info, &should_schedule);

		if (should_schedule)
			usleep_range(1000, 1100);
	}

	return 0;
}

static void pcm_thread_stop(struct snd_pcm *pcm)
{
	struct snd_soc_pcm_runtime *rtd = pcm->private_data;
	struct hisi_pcm_data *pdata = snd_soc_platform_get_drvdata(rtd->platform);
	struct pcm_thread_info *thread_info = &pdata->thread_info;
	bool is_stream_open = pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream_opened ||
		pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream_opened;

	if (is_stream_open)
		return;

	if (thread_info->pcm_run_thread) {
		kthread_stop(thread_info->pcm_run_thread);
		thread_info->pcm_run_thread = NULL;

		if (thread_info->hifi_share_data) {
			iounmap(thread_info->hifi_share_data);
			thread_info->hifi_share_data = NULL;
		}

		logi("pcm thread has stopped.\n");
	}
}

static int pcm_thread_start(struct snd_pcm *pcm)
{
	int ret;
	struct sched_param param = {.sched_priority = MAX_RT_PRIO - 1};
	struct pcm_hifi_share_data *thread_share_data = NULL;
	struct snd_soc_pcm_runtime *rtd = pcm->private_data;
	struct hisi_pcm_data *pdata = snd_soc_platform_get_drvdata(rtd->platform);
	struct pcm_thread_info *thread_info = &pdata->thread_info;

	if (thread_info->pcm_run_thread) {
		logi("pcm thread has already running, not need init.\n");
		return 0;
	}

	/* share memory remap */
	thread_share_data = ioremap_wc(DRV_DSP_PCM_THREAD_DATA_ADDR, sizeof(struct pcm_hifi_share_data));
	if (!thread_share_data) {
		loge("pcm share mem iormap failed!\n");
		return -ENOMEM;
	}
	memset(thread_share_data, 0, sizeof(*thread_share_data));/* unsafe_function_ignore: memset */

	/* create pcm thread for communication with hifi */
	thread_info->pcm_run_thread = kthread_create(pcm_check_frame_thread,
		(void *)thread_info, "pcm_run_thread");
	if (IS_ERR(thread_info->pcm_run_thread)) {
		loge("create check frame thread failed!\n");
		thread_info->pcm_run_thread = NULL;
		iounmap(thread_share_data);
		thread_share_data = NULL;
		return -ENOMEM;
	}

	thread_info->hifi_share_data = thread_share_data;

	/* set highest rt prio */
	ret = sched_setscheduler(thread_info->pcm_run_thread, SCHED_FIFO, &param);
	if (ret)
		loge("set thread schedule priority failed\n");

	/* do not wakeup process in this stage, wakeup when lowlatency stream start */
	logi("create pcm_run_thread success\n");

	return 0;
}

static int pcm_set_share_data(
	struct snd_pcm_substream *substream, uint32_t data_addr, uint32_t data_len)
{
	int pcm_mode = substream->stream;
	struct snd_soc_pcm_runtime *soc_prtd = substream->private_data;
	struct hisi_pcm_data *pdata = dev_get_drvdata(soc_prtd->platform->dev);
	struct pcm_thread_info *thread_info = &pdata->thread_info;
	struct pcm_hifi_share_data *share_data = thread_info->hifi_share_data;
	struct pcm_thread_stream_info *stream_info = &share_data->stream_share_data[pcm_mode];
	const char *timeout_str[PCM_STREAM_MAX] = {
		"set play addr timeout",
		"set capture addr timeout"};
	int set_data_interval = 0;

	if (stream_info == NULL) {
		loge("pcm mode: %d, thread_share_data is null!\n", pcm_mode);
		return -EINVAL;
	}

	stream_info->data_buf_offset = data_addr;
	stream_info->data_buf_size = data_len;
	stream_info->last_get_time = stream_info->set_time;
	stream_info->set_time = mailbox_get_timestamp();

	set_data_interval = stream_info->set_time - stream_info->last_get_time;
	if (set_data_interval > CHECK_UPDATE_TIMEOUT)
		_dump_thread_info(thread_info, substream, timeout_str[pcm_mode]);

	return 0;
}

static int hisi_pcm_alloc_mmap_share_buf(struct snd_pcm_substream *substream,
	uint32_t buf_size)
{
	int ret = -EINVAL;
	struct hisi_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct snd_soc_pcm_runtime *soc_prtd = substream->private_data;
	struct device *dev = soc_prtd->platform->dev;
	struct hisi_pcm_ion_buf *ion_buf = NULL;
	int pcm_device = substream->pcm->device;
	int pcm_mode = substream->stream;

	if (buf_size > PCM_DMA_BUF_MMAP_MAX_SIZE) {
		loge("buf_size %d error\n", buf_size);
		return ret;
	}

	ion_buf = kzalloc(sizeof(*ion_buf), GFP_KERNEL);
	if (!ion_buf) {
		loge("ion_buf malloc fail\n");
		ret = -ENOMEM;
		goto err_ret;
	}


	/* create ion mem client for mmap pcm device */
	ion_buf->client = hisi_ion_client_create(HISI_PCM_ION_CLIENT_NAME);
	if (IS_ERR_OR_NULL(ion_buf->client)) {
		loge("failed to create ion client\n");
		goto err_ion_client;
	}

	ion_buf->buf_size = buf_size;
	ion_buf->handle = ion_alloc(ion_buf->client, ion_buf->buf_size,
		PAGE_SIZE, ION_HEAP(ION_MISC_HEAP_ID), 0);
	if (IS_ERR_OR_NULL(ion_buf->handle)) {
		loge("failed to alloc ion memory(size:%d)\n", ion_buf->buf_size);
		goto err_ion_alloc;
	}

	/* get share buffer phy address */
	ret = hisi_pcm_ion_phys(ion_buf->client, ion_buf->handle, dev,
		(ion_phys_addr_t *)&ion_buf->phy_addr);
	if (ret) {
		loge("failed to get ion phys\n");
		goto err_ion_addr;
	}

	/* get buffer virt address */
	ion_buf->buf_addr = ion_map_kernel(ion_buf->client, ion_buf->handle);
	if (!ion_buf->buf_addr) {
		loge("device:%d mode:%d failed to map ion memory\n",
			pcm_device, pcm_mode);
		goto err_ion_addr;
	}

	memset((void *)ion_buf->buf_addr, 0, ion_buf->buf_size);/* unsafe_function_ignore: memset */

	prtd->ion_buf = ion_buf;

	return ret;

err_ion_addr:
	ion_free(ion_buf->client, ion_buf->handle);
err_ion_alloc:
	ion_client_destroy(ion_buf->client);
	ion_buf->handle = NULL;
	ion_buf->client = NULL;
err_ion_client:
	kfree(ion_buf);
	ion_buf = NULL;
err_ret:
	return ret;
}

static void hisi_pcm_free_mmap_share_buf(struct snd_pcm_substream *substream)
{
	struct hisi_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct hisi_pcm_ion_buf *ion_buf = prtd->ion_buf;

	if (ion_buf && ion_buf->buf_addr) {
		if (!ion_buf->client || !ion_buf->handle) {
			loge("ion buf client or handle is invalid\n");
			return;
		}

		ion_unmap_kernel(ion_buf->client, ion_buf->handle);
		ion_free(ion_buf->client, ion_buf->handle);

		ion_client_destroy(ion_buf->client);
		ion_buf->handle = NULL;
		ion_buf->buf_addr = NULL;

		kfree(prtd->ion_buf);
		prtd->ion_buf = NULL;
	}
}

/* get shared fd info for mmap device's ion share buffer */
static int hisi_pcm_mmap_shared_fd(struct snd_pcm_substream *substream,
			   struct hisi_pcm_mmap_fd *mmap_fd)
{
	struct hisi_pcm_runtime_data *prtd = NULL;
	struct hisi_pcm_ion_buf *ion_buf = NULL;

	if (!substream->runtime) {
		loge("substream runtime is null\n");
		return -EFAULT;
	}

	if (!mmap_fd) {
		loge("mmap fd is null\n");
		return -EFAULT;
	}

	prtd = substream->runtime->private_data;
	if (!prtd) {
		loge("prtd is null\n");
		return -EINVAL;
	}

	ion_buf = prtd->ion_buf;
	if (!ion_buf) {
		loge("ion_buf is null\n");
		return -EINVAL;
	}

	mmap_fd->shared_fd = ion_share_dma_buf_fd(ion_buf->client, ion_buf->handle);
	if (mmap_fd->shared_fd >= 0) {
		mmap_fd->buf_size = ion_buf->buf_size;
		mmap_fd->stream_dir = substream->stream;
	} else {
		loge("get shared fd %d error\n", mmap_fd->shared_fd);
		return -EFAULT;
	}

	return 0;
}

static int hisi_pcm_hwdep_ioctl_shared_fd(struct snd_pcm *pcm,
	unsigned long arg)
{
	struct hisi_pcm_mmap_fd __user *_mmap_fd = NULL;
	struct hisi_pcm_mmap_fd mmap_fd = {0};
	struct snd_pcm_substream *substream = NULL;
	int32_t pcm_mode = -1;

	_mmap_fd = (struct hisi_pcm_mmap_fd __user *)arg;
	if (get_user(pcm_mode, (int32_t __user *)&(_mmap_fd->stream_dir))) {
		loge("copying mmap_fd from user fail\n");
		return -EFAULT;
	}

	if (pcm_mode != SNDRV_PCM_STREAM_PLAYBACK && pcm_mode != SNDRV_PCM_STREAM_CAPTURE) {
		loge("stream invalid mode: %d\n", pcm_mode);
		return -EINVAL;
	}

	substream = pcm->streams[pcm_mode].substream;
	if (!substream) {
		loge("substream is invalid\n");
		return -ENODEV;
	}

	if (hisi_pcm_mmap_shared_fd(substream, &mmap_fd) < 0) {
		loge("get mmap buffer fd fail\n");
		return -EFAULT;
	}

	logd("device: %d mode: %d - [shared_fd: %d, buf_size: %d]\n",
		pcm->device, pcm_mode, mmap_fd.shared_fd, mmap_fd.buf_size);

	if (put_user(mmap_fd.shared_fd, &_mmap_fd->shared_fd) || /*lint !e1058*/
		put_user(mmap_fd.buf_size, &_mmap_fd->buf_size)) {   /*lint !e1058*/
		loge("copying shared fd info fail\n");
		return -EFAULT;
	}

	return 0;
}

/* add custom pcm ioctl cmd here */
static int hisi_pcm_hwdep_ioctl(struct snd_hwdep *hw, struct file *file,
			       unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct snd_pcm *pcm = hw->private_data;

	switch (cmd) {
	case HISI_PCM_IOCTL_MMAP_SHARED_FD:
		return hisi_pcm_hwdep_ioctl_shared_fd(pcm, arg);
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int hisi_pcm_add_hwdep_dev(struct snd_soc_pcm_runtime *runtime)
{
	struct snd_hwdep *hwdep = NULL;
	int ret = 0;
	char id[] = "MMAP_xx";

	snprintf(id, sizeof(id), "MMAP_%d", runtime->pcm->device);
	logd("add hwdep node for pcm device %d\n", runtime->pcm->device);

	ret = snd_hwdep_new(runtime->card->snd_card,
			   &id[0], runtime->pcm->device, &hwdep);
	if (!hwdep || ret < 0) {
		loge("hwdep intf failed to create %s - hwdep\n", id);
		return ret;
	}

	hwdep->iface = HISI_SND_HWDEP_IFACE_PCM;
	hwdep->private_data = runtime->pcm;
	hwdep->ops.ioctl = hisi_pcm_hwdep_ioctl;

	return 0;
}

static int map_hifi_share_buffer(struct snd_pcm_substream *substream)
{
	struct hisi_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct pcm_ap_hifi_buf *buf = &prtd->hifi_buf;
	int pcm_device = substream->pcm->device;
	int pcm_mode = substream->stream;

	if (pcm_device != PCM_DEVICE_MMAP)
		return 0;

	buf->addr = g_pcm_dma_buf_config[pcm_device][pcm_mode].pcm_dma_buf_base;
	buf->bytes = g_pcm_dma_buf_config[pcm_device][pcm_mode].pcm_dma_buf_len;
	buf->area = ioremap(buf->addr, buf->bytes);

	if (!buf->area) {
		loge("ap-hifi buf area error\n");
		return -ENOMEM;
	}

	return 0;
}

static void unmap_hifi_share_buffer(struct snd_pcm_substream *substream)
{
	struct hisi_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct pcm_ap_hifi_buf *buf = &prtd->hifi_buf;
	int pcm_device = substream->pcm->device;

	if (pcm_device != PCM_DEVICE_MMAP)
		return;

	if (!buf->area)
		return;

	iounmap(buf->area);

	buf->area = NULL;
	buf->addr = 0;
}

static irq_rt_t hisi_pcm_isr_handle(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hisi_pcm_runtime_data *prtd = NULL;
	snd_pcm_uframes_t rt_period_size = 0;
	unsigned int num_period = 0;
	snd_pcm_uframes_t avail = 0;
	unsigned short pcm_mode = 0;
	int pcm_device = 0;

	IN_FUNCTION;

	if (substream == NULL) {
		loge("substream is null\n");
		return IRQ_HDD_PTR;
	}

	if (substream->runtime == NULL) {
		loge("runtime is null\n");
		return IRQ_HDD_PTR;
	}

	pcm_mode = (unsigned short)substream->stream;
	pcm_device = substream->pcm->device;
	prtd    = (struct hisi_pcm_runtime_data *)substream->runtime->private_data;
	rt_period_size  = substream->runtime->period_size;
	num_period      = substream->runtime->periods;

	if (!prtd) {
		loge("prtd is null\n");
		return IRQ_HDD_PTR;
	}

	if (STATUS_RUNNING != prtd->status) {
		logd("dma status %d error\n", prtd->status);
		return IRQ_HDD_STATUS;
	}

	if (pcm_device == PCM_DEVICE_MMAP) {
		switch (pcm_mode) {
		case SNDRV_PCM_STREAM_PLAYBACK:
			prtd->frame_counter += rt_period_size;
			substream->runtime->control->appl_ptr = prtd->frame_counter + rt_period_size;
			break;
		case SNDRV_PCM_STREAM_CAPTURE:
			prtd->frame_counter += rt_period_size;
			substream->runtime->control->appl_ptr = prtd->frame_counter - rt_period_size;
			break;
		}
		substream->runtime->control->appl_ptr = substream->runtime->control->appl_ptr % substream->runtime->boundary;
	}

	if (SNDRV_PCM_STREAM_CAPTURE == pcm_mode) {
		avail = (snd_pcm_uframes_t)snd_pcm_capture_hw_avail(substream->runtime);
	} else {
		avail = (snd_pcm_uframes_t)snd_pcm_playback_hw_avail(substream->runtime);
	}

	if (avail < rt_period_size) {
		if (hifi_misc_get_platform_type() == HIFI_DSP_PLATFORM_ASIC)
			logw("pcm_mode:%d: avail(%d) < rt_period_size(%d)\n", pcm_mode, (int)avail, (int)rt_period_size);
		return IRQ_HDD_SIZE;
	}

	spin_lock(&prtd->lock);
	prtd->period_cur = (prtd->period_cur + 1) % num_period;
	spin_unlock(&prtd->lock);

	snd_pcm_period_elapsed(substream);

	ret = hisi_pcm_notify_set_buf(substream);
	if (ret < 0) {
		loge("pcm_mode:%d, pcm_device:%d: notify set buf fail, ret(%d)\n",
			pcm_mode, pcm_device, ret);
		return IRQ_HDD_ERROR;
	}

	spin_lock(&prtd->lock);
	prtd->period_next = (prtd->period_next + 1) % num_period;
	spin_unlock(&prtd->lock);

	OUT_FUNCTION;

	return IRQ_HDD;
}

static int hisi_pcm_mailbox_send_data(void *pmsg_body, unsigned int msg_len,
		unsigned int msg_priority)
{
	unsigned int ret = 0;
	static unsigned int err_count = 0;

	ret = DRV_MAILBOX_SENDMAIL(MAILBOX_MAILCODE_ACPU_TO_HIFI_AUDIO, pmsg_body, msg_len);
	if (MAILBOX_OK != ret) {
		if (err_count % 50 == 0) {
#ifdef CONFIG_HUAWEI_DSM
			audio_dsm_report_info(AUDIO_CODEC, DSM_PCM_DRV_UPDATE_PCM_BUFF_DELAY, "update pcm buffer delay");
#endif
		}
		err_count++;
	} else {
		err_count = 0;
	}

	return (int)ret;
}

static int hisi_pcm_notify_set_buf(struct snd_pcm_substream *substream)
{
	int ret = 0;
	unsigned int period_size;
	struct hifi_channel_set_buffer msg_body = {0};
	unsigned short pcm_mode = (unsigned short)substream->stream;
	int pcm_device = substream->pcm->device;
	struct hisi_pcm_runtime_data *prtd = (struct hisi_pcm_runtime_data *)substream->runtime->private_data;
	struct hisi_pcm_ion_buf *ion_buf = NULL;
	uint32_t data_phy_addr = 0;
	uint32_t data_offset_addr = 0;

	IN_FUNCTION;

	if (NULL == prtd) {
		loge("prtd is null, error\n");
		return -EINVAL;
	}

	if ((SNDRV_PCM_STREAM_PLAYBACK != pcm_mode) && (SNDRV_PCM_STREAM_CAPTURE != pcm_mode)) {
		loge("pcm mode %d invalid\n", pcm_mode);
		return -EINVAL;
	}

	/* transfer frame data between ion buffer and hifi buffer */
	ion_buf = prtd->ion_buf;
	if (pcm_device == PCM_DEVICE_MMAP) {
		if (pcm_mode == SNDRV_PCM_STREAM_PLAYBACK)
			memcpy(prtd->hifi_buf.area, ion_buf->buf_addr, ion_buf->buf_size);
		else
			memcpy(ion_buf->buf_addr, prtd->hifi_buf.area, ion_buf->buf_size);
	}

	period_size = prtd->period_size;
	if (pcm_device == PCM_DEVICE_MMAP) {
		data_phy_addr = prtd->hifi_buf.addr + prtd->period_next * period_size;
	} else {
		data_phy_addr = substream->runtime->dma_addr + prtd->period_next * period_size;
	}
	data_offset_addr = data_phy_addr - PCM_DMA_BUF_0_PLAYBACK_BASE;

	msg_body.msg_type   = (unsigned short)HI_CHN_MSG_PCM_SET_BUF;
	msg_body.pcm_mode   = pcm_mode;
	msg_body.pcm_device = (unsigned short)pcm_device;
	msg_body.data_addr = data_offset_addr;
	msg_body.data_len  = period_size;

	if (STATUS_RUNNING != prtd->status) {
		logd("pcm status %d error\n", prtd->status);
		return -EINVAL;
	}

	if (prtd->using_thread_flag) {
		ret = pcm_set_share_data(substream, msg_body.data_addr, msg_body.data_len);
	} else {
		ret = hisi_pcm_mailbox_send_data(&msg_body, sizeof(msg_body), 0);
	}
	if (ret != OK)
		ret = -EBUSY;

	OUT_FUNCTION;

	return ret;
}

static void print_pcm_timeout(unsigned int pre_time, const char *print_type, unsigned int time_delay)
{
	unsigned int  delay_time;
	unsigned int  curr_time;

	if (hifi_misc_get_platform_type() != HIFI_DSP_PLATFORM_ASIC) {
		return;
	}

	curr_time = (unsigned int)mailbox_get_timestamp();
	delay_time = curr_time - pre_time;

	if (delay_time > (HISI_PCM_WORK_DELAY_1MS * time_delay)) {
		logw("[%d]:%s, delaytime %u.\n", mailbox_get_timestamp(), print_type, delay_time);
	}
}

static long get_snd_current_millisec(void)
{
	struct timeval last_update;
	long curr_time;

	do_gettimeofday(&last_update);
	curr_time = last_update.tv_sec * 1000 + last_update.tv_usec / 1000;
	return curr_time;
}

void snd_pcm_print_timeout(struct snd_pcm_substream *substream, unsigned int timeout_type)
{
	long delay_time;
	long curr_time;
	static unsigned int timeout_count[SND_TIMEOUT_TYPE_MAX] = {0};
	const char *timeout_str[SND_TIMEOUT_TYPE_MAX] = {
		"pcm write interval timeout",
		"pcm write proc timeout",
		"pcm read interval timeout",
		"pcm read proc timeout"};

	if (hifi_misc_get_platform_type() != HIFI_DSP_PLATFORM_ASIC) {
		return;
	}

	if (substream == NULL) {
		loge("substream is null\n");
		return;
	}

	if (timeout_type >= SND_TIMEOUT_TYPE_MAX) {
		return;
	}

	curr_time = get_snd_current_millisec();
	delay_time = curr_time - substream->runtime->pre_time;

	if (delay_time > ALSA_TIMEOUT_MILLISEC && (substream->runtime->pre_time != 0)) {
		timeout_count[timeout_type]++;

		if ((timeout_count[timeout_type] == 1) || (timeout_count[timeout_type] % 20 == 0)) {
			logw("%s, delay time: %ld ms.\n", timeout_str[timeout_type], delay_time);
		}
	} else {
		timeout_count[timeout_type] = 0;
	}

	if (timeout_type == SND_TIMEOUT_TYPE_WRITE_INTERVAL
		|| timeout_type == SND_TIMEOUT_TYPE_READ_INTERVAL) {
		substream->runtime->pre_time = curr_time;
	}
}
EXPORT_SYMBOL(snd_pcm_print_timeout);

void snd_pcm_reset_pre_time(struct snd_pcm_substream *substream)
{
	if (substream == NULL) {
		loge("substream is null\n");
		return;
	}

	substream->runtime->pre_time = 0;
}
EXPORT_SYMBOL(snd_pcm_reset_pre_time);

static irq_rt_t hisi_pcm_notify_recv_isr(void *usr_para, void *mail_handle, unsigned int mail_len)
{
	struct snd_pcm_substream * substream    = NULL;
	struct hisi_pcm_runtime_data *prtd        = NULL;
	struct hifi_chn_pcm_period_elapsed mail_buf;
	unsigned int mail_size          = mail_len;
	unsigned int ret_mail           = MAILBOX_OK;
	irq_rt_t ret                    = IRQ_NH;
	unsigned int start_time = 0;
	const char *print_type[2] = {"recv pcm msg timeout", "process pcm msg timeout"};

	UNUSED_PARAMETER(usr_para);

	start_time = (unsigned int)mailbox_get_timestamp();
	memset(&mail_buf, 0, sizeof(struct hifi_chn_pcm_period_elapsed));/* unsafe_function_ignore: memset */

	/*get the data from mailbox*/

	ret_mail = DRV_MAILBOX_READMAILDATA(mail_handle, (unsigned char*)&mail_buf, &mail_size);
	if ((ret_mail != MAILBOX_OK)
		|| (mail_size == 0)
			|| (mail_size > sizeof(struct hifi_chn_pcm_period_elapsed)))
	{
		loge("Empty point or data length error! size: %d  ret_mail:%d sizeof(struct hifi_chn_pcm_period_elapsed):%lu\n", mail_size, ret_mail, sizeof(struct hifi_chn_pcm_period_elapsed));
		return IRQ_NH_MB;
	}

	substream = INT_TO_ADDR(mail_buf.substream_l32,mail_buf.substream_h32);
	if (!_is_valid_substream(substream))
		return IRQ_NH_OTHERS;

	prtd = (struct hisi_pcm_runtime_data *)substream->runtime->private_data;
	if (NULL == prtd) {
		loge("prtd is NULL\n");
		return IRQ_NH_OTHERS;
	}
	if (STATUS_STOP == prtd->status) {
		logi("process has stopped\n");
		return IRQ_NH_OTHERS;
	}

	switch(mail_buf.msg_type) {
		case HI_CHN_MSG_PCM_PERIOD_ELAPSED:
			/* check if elapsed msg is timeout */
			print_pcm_timeout(mail_buf.msg_timestamp, print_type[0], 10);
			ret = hisi_pcm_isr_handle(substream);
			if (ret == IRQ_NH)
				loge("mb msg handle err, ret : %d\n", ret);
			break;
		case HI_CHN_MSG_PCM_PERIOD_STOP:
			if (STATUS_STOPPING == prtd->status) {
				prtd->status = STATUS_STOP;
				logi("device %d mode %d stop now !\n", substream->pcm->device, mail_buf.pcm_mode);
			}
			break;
		default:
			loge("msg_type 0x%x\n", mail_buf.msg_type);
			break;
	}
	/* check if isr proc is timeout */
	print_pcm_timeout(start_time, print_type[1], 20);

	return ret;
}

static int hisi_pcm_notify_isr_register(irq_hdl_t pisr)
{
	int ret                     = 0;
	unsigned int mailbox_ret    = MAILBOX_OK;

	if (NULL == pisr) {
		loge("pisr==NULL!\n");
		ret = ERROR;
	} else {
		mailbox_ret = DRV_MAILBOX_REGISTERRECVFUNC(MAILBOX_MAILCODE_HIFI_TO_ACPU_AUDIO, (void *)pisr, NULL);
		if (MAILBOX_OK != mailbox_ret) {
			ret = ERROR;
			loge("ret : %d,0x%x\n", ret, MAILBOX_MAILCODE_HIFI_TO_ACPU_AUDIO);
		}
	}

	return ret;
}

static int hisi_pcm_notify_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	int ret = 0;
	struct hifi_chn_pcm_open open_msg = {0};
	struct hifi_chn_pcm_hw_params hw_params_msg  = {0};
	unsigned int params_value = 0;
	unsigned int infreq_index = 0;

	IN_FUNCTION;

	open_msg.msg_type = (unsigned short)HI_CHN_MSG_PCM_OPEN;
	hw_params_msg.msg_type = (unsigned short)HI_CHN_MSG_PCM_HW_PARAMS;
	hw_params_msg.pcm_mode = open_msg.pcm_mode = (unsigned short)substream->stream;
	hw_params_msg.pcm_device = open_msg.pcm_device = (unsigned short)substream->pcm->device;

	/* check channels  : mono or stereo */
	params_value = params_channels(params);
	if ((HISI_PCM_CP_MIN_CHANNELS <= params_value) && (HISI_PCM_CP_MAX_CHANNELS >= params_value)) {
		open_msg.config.channels = params_value;
		hw_params_msg.channel_num = params_value;
	} else {
		loge("DAC not support %d channels\n", params_value);
		return -EINVAL;
	}

	/* check samplerate */
	params_value = params_rate(params);
	logi("rate is %d\n", params_value);

	for (infreq_index = 0; infreq_index < ARRAY_SIZE(freq); infreq_index++) {
		if(params_value == freq[infreq_index])
			break;
	}

	if (ARRAY_SIZE(freq) <= infreq_index) {
		loge("rate %d not support\n", params_value);
		return -EINVAL;
	}

	open_msg.config.rate = params_value;

	hw_params_msg.sample_rate = params_value;

	/* check format */
	params_value = (unsigned int)params_format(params);
	if (params_value == SNDRV_PCM_FORMAT_S24_LE) {
		params_value = PCM_FORMAT_S24_LE_LA;
	} else {
		params_value = PCM_FORMAT_S16_LE;
	}

	hw_params_msg.format = params_value;

	open_msg.config.format = params_value;
	open_msg.config.period_size = params_period_size(params);
	open_msg.config.period_count = params_periods(params);

	ret = hisi_pcm_mailbox_send_data(&open_msg, sizeof(open_msg), 0);

	/* send hw_params */
	ret += hisi_pcm_mailbox_send_data(&hw_params_msg, sizeof(hw_params_msg), 0);

	OUT_FUNCTION;

	return ret;
}

static int hisi_pcm_notify_hw_free(struct snd_pcm_substream *substream)
{
	int ret = 0;

	UNUSED_PARAMETER(substream);

	return ret;
}

static int hisi_pcm_notify_prepare(struct snd_pcm_substream *substream)
{
	int ret = OK;

	UNUSED_PARAMETER(substream);

	return ret;
}

static int hisi_pcm_notify_trigger(int cmd, struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hifi_chn_pcm_trigger msg_body = {0};
	uint32_t period_size = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct hisi_pcm_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *soc_prtd = substream->private_data;
	struct hisi_pcm_data *pdata = dev_get_drvdata(soc_prtd->platform->dev);
	struct pcm_thread_info *thread_info = &pdata->thread_info;
	struct hisi_pcm_ion_buf *ion_buf = NULL;
	int pcm_mode = substream->stream;
	int pcm_device = substream->pcm->device;
	uint32_t data_phy_addr = 0;
	uint32_t data_offset_addr = 0;

	IN_FUNCTION;

	if (NULL == prtd) {
		loge("prtd is null\n");
		return -EINVAL;
	}

	msg_body.msg_type   	= (unsigned short)HI_CHN_MSG_PCM_TRIGGER;
	msg_body.pcm_mode   	= (unsigned short)pcm_mode;
	msg_body.pcm_device   	= (unsigned short)pcm_device;
	msg_body.tg_cmd     	= (unsigned short)cmd;
	msg_body.substream_l32  = GET_LOW32(substream);
	msg_body.substream_h32  = GET_HIG32(substream);

	if ((SNDRV_PCM_TRIGGER_START == cmd)
		|| (SNDRV_PCM_TRIGGER_RESUME == cmd)
		|| (SNDRV_PCM_TRIGGER_PAUSE_RELEASE == cmd)) {

		/* transfer frame data between ion buffer and hifi buffer */
		ion_buf = prtd->ion_buf;
		if (pcm_device == PCM_DEVICE_MMAP) {
			if (pcm_mode == SNDRV_PCM_STREAM_PLAYBACK)
				memcpy(prtd->hifi_buf.area, ion_buf->buf_addr, ion_buf->buf_size);
			else
				memcpy(ion_buf->buf_addr, prtd->hifi_buf.area, ion_buf->buf_size);
		}

		period_size = prtd->period_size;
		if (pcm_device == PCM_DEVICE_MMAP) {
			data_phy_addr = prtd->hifi_buf.addr + prtd->period_next * period_size;
		} else {
			data_phy_addr = runtime->dma_addr + prtd->period_next * period_size;
		}
		data_offset_addr = data_phy_addr - PCM_DMA_BUF_0_PLAYBACK_BASE;

		msg_body.data_addr = data_offset_addr;
		msg_body.data_len  = period_size;
	}

	/* update share memory info */
	if (prtd->using_thread_flag && (cmd == SNDRV_PCM_TRIGGER_START)) {
		ret = wake_up_process(thread_info->pcm_run_thread);
		logi("lowlatency check frame thread wake up %s!\n", ret ? "success" : "fail");

		ret = pcm_set_share_data(substream, msg_body.data_addr, msg_body.data_len);
		if (ret)
			loge("set share data fail, ret:%d!\n", ret);
	}

	ret = hisi_pcm_mailbox_send_data(&msg_body, sizeof(msg_body), 0);

	OUT_FUNCTION;

	return ret;
}

static int hisi_pcm_notify_open(struct snd_pcm_substream *substream)
{
	int ret = 0;

	UNUSED_PARAMETER(substream);

	return ret;
}

static int hisi_pcm_notify_close(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hifi_chn_pcm_close msg_body  = {0};

	IN_FUNCTION;

	msg_body.msg_type = (unsigned short)HI_CHN_MSG_PCM_CLOSE;
	msg_body.pcm_mode = (unsigned short)substream->stream;
	msg_body.pcm_device = (unsigned short)substream->pcm->device;
	ret = hisi_pcm_mailbox_send_data(&msg_body, sizeof(msg_body), 0);
	if (ret)
		ret = -EBUSY;

	OUT_FUNCTION;

	return ret;
}

static int hisi_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	int ret = 0;
	struct hisi_pcm_runtime_data *prtd = substream->runtime->private_data;
	size_t bytes = params_buffer_bytes(params);
	int device = substream->pcm->device;
	struct snd_dma_buffer *dma_buf = &substream->dma_buffer;
	struct hisi_pcm_ion_buf *ion_buf;

	if (!_is_valid_pcm_device(device)) {
		return ret;
	}

	if (NULL == prtd) {
		loge("prtd is null\n");
		return -EINVAL;
	}

	if (device == PCM_DEVICE_MMAP) {
		ret = map_hifi_share_buffer(substream);
		if (ret) {
			loge("prealloc hifi share buffer fail, ret %d\n", ret);
			return ret;
		}

		ret = hisi_pcm_alloc_mmap_share_buf(substream, bytes);
		if (ret) {
			loge("alloc mmap share buffer size %zd fail, ret %d\n", bytes, ret);
			unmap_hifi_share_buffer(substream);
			return ret;
		}

		ion_buf = prtd->ion_buf;
		dma_buf->dev.type = SNDRV_DMA_TYPE_DEV;
		dma_buf->dev.dev = substream->pcm->card->dev;
		dma_buf->private_data = NULL;
		dma_buf->area = ion_buf->buf_addr;
		dma_buf->addr = ion_buf->phy_addr;
		dma_buf->bytes = ion_buf->buf_size;
		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	} else {
		ret = snd_pcm_lib_malloc_pages(substream, bytes);
		if (ret < 0) {
			loge("snd_pcm_lib_malloc_pages ret : %d\n", ret);
			return ret;
		}
	}

	spin_lock(&prtd->lock);
	prtd->period_size = params_period_bytes(params);
	prtd->period_next = 0;
	spin_unlock(&prtd->lock);

	ret = hisi_pcm_notify_hw_params(substream, params);
	if (ret < 0) {
		loge("pcm mode %d notify hw_params error\n", substream->stream);
		if (device != PCM_DEVICE_MMAP)
			snd_pcm_lib_free_pages(substream);
		else
			unmap_hifi_share_buffer(substream);
	}

	return ret;
}

static int hisi_pcm_hw_free(struct snd_pcm_substream *substream)
{
	int ret = 0;
	int i   = 0;
	struct hisi_pcm_runtime_data *prtd = NULL;
	int device = substream->pcm->device;

	prtd = (struct hisi_pcm_runtime_data *)substream->runtime->private_data;

	if (!_is_valid_pcm_device(device)) {
		return ret;
	}

	if (NULL == prtd) {
		loge("prtd is null\n");
		return -EINVAL;
	}

	for(i = 0; i < 30 ; i++) {  /* wait for dma ok */
		if (STATUS_STOP == prtd->status) {
			break;
		} else {
			msleep(10);
		}
	}
	if (30 == i) {
		logi("timeout for waiting for stop info from other\n");
	}

	ret = hisi_pcm_notify_hw_free(substream);
	if (ret < 0) {
		loge("free fail device %d\n", substream->pcm->device);
	}

	if (device != PCM_DEVICE_MMAP)
		ret = snd_pcm_lib_free_pages(substream);

	return ret;
}

static int hisi_pcm_prepare(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hisi_pcm_runtime_data *prtd = (struct hisi_pcm_runtime_data *)substream->runtime->private_data;
	int device = substream->pcm->device;

	if (!_is_valid_pcm_device(device))
		return ret;

	if (NULL == prtd) {
		loge("prtd is null\n");
		return -EINVAL;
	}

	/* init prtd */
	spin_lock(&prtd->lock);
	prtd->status        = STATUS_STOP;
	prtd->period_next   = 0;
	prtd->period_cur    = 0;
	prtd->frame_counter = 0;
	spin_unlock(&prtd->lock);

	ret = hisi_pcm_notify_prepare(substream);

	return ret;
}

static int hisi_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct hisi_pcm_runtime_data *prtd = (struct hisi_pcm_runtime_data *)substream->runtime->private_data;
	unsigned int num_periods = runtime->periods;
	int device = substream->pcm->device;
	struct hisi_pcm_ion_buf *ion_buf = NULL;

	if (!_is_valid_pcm_device(device)) {
		return ret;
	}

	if (NULL == prtd) {
		loge("prtd is null\n");
		return -EINVAL;
	}

	logi("device %d mode %d trigger %d \n", substream->pcm->device, substream->stream, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = hisi_pcm_notify_trigger(cmd, substream);
		if (ret < 0) {
			loge("trigger %d failed, ret : %d\n", cmd, ret);
		} else {
			spin_lock(&prtd->lock);
			prtd->status = STATUS_RUNNING;
			prtd->period_next = (prtd->period_next + 1) % num_periods;
			prtd->frame_counter = 0;
			spin_unlock(&prtd->lock);
		}
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		spin_lock(&prtd->lock);
		prtd->status = STATUS_STOPPING;
		spin_unlock(&prtd->lock);

		ret = hisi_pcm_notify_trigger(cmd, substream);
		if (ret < 0) {
			loge("hisi_pcm_notify_pcm_trigger ret : %d\n", ret);
		}

		if (device == PCM_DEVICE_MMAP) {
			ion_buf = prtd->ion_buf;
			if (ion_buf && ion_buf->buf_addr)
				memset(ion_buf->buf_addr, 0, ion_buf->buf_size);/* unsafe_function_ignore: memset */
		}
		break;
	default:
		loge("trigger cmd error : %d\n", cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static snd_pcm_uframes_t hisi_pcm_pointer(struct snd_pcm_substream *substream)
{
	long frame = 0L;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct hisi_pcm_runtime_data *prtd = runtime->private_data;
	int device = substream->pcm->device;

	if (!_is_valid_pcm_device(device)) {
		return (snd_pcm_uframes_t)frame;
	}

	if (NULL == prtd) {
		loge("prtd is null\n");
		return -EINVAL;
	}

	frame = bytes_to_frames(runtime, prtd->period_cur * prtd->period_size);
	if (frame >= runtime->buffer_size)
		frame = 0;

	return (snd_pcm_uframes_t)frame;
}

static int hisi_pcm_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hisi_pcm_runtime_data *prtd = NULL;
	uint32_t pcm_mode = substream->stream;
	uint32_t pcm_device = substream->pcm->device;
	const struct snd_pcm_hardware *hw_info = NULL;
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_pcm_runtime *rtd = pcm->private_data;
	struct hisi_pcm_data *pdata = snd_soc_platform_get_drvdata(rtd->platform);
	struct pcm_thread_info *thread_info = &pdata->thread_info;

	logi("device %d, mode %d open\n", substream->pcm->device, substream->stream);

	if (WARN_ON(pcm_device >= PCM_DEVICE_TOTAL || pcm_mode >= PCM_STREAM_MAX))
		return -EINVAL;

	hw_info = hisi_pcm_hw_info[pcm_device][pcm_mode];
	if (!hw_info) {
		logw("pcm device: %d, pcm_mode:%d hw_info is not found, set default para\n", pcm_device, pcm_mode);
		return snd_soc_set_runtime_hwparams(substream, &hisi_pcm_hardware_modem_playback);
	}

	snd_soc_set_runtime_hwparams(substream, hw_info);

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (prtd == NULL) {
		loge("Failed to allocate memory for hisi_pcm_runtime_data\n");
		return -ENOMEM;
	}

	prtd->substream = substream;
	spin_lock_init(&prtd->lock);

	thread_info->runtime_data[pcm_device][pcm_mode] = prtd;

	/* init substream private data */
	spin_lock(&prtd->lock);
	prtd->period_cur  = 0;
	prtd->period_next = 0;
	prtd->period_size = 0;
	prtd->frame_counter = 0;
	prtd->status = STATUS_STOP;
	prtd->using_thread_flag = _is_pcm_device_using_thread(pcm_device);
	substream->runtime->private_data = prtd;
	spin_unlock(&prtd->lock);

	if (prtd->using_thread_flag)
		atomic_inc(&thread_info->using_thread_cnt);

	ret = hisi_pcm_notify_open(substream);
	if (ret) {
		loge("notify open fail, ret %d\n", ret);
		goto fail_open;
	}

	return 0;

fail_open:
	kfree(prtd);
	return ret;
}

static int hisi_pcm_close(struct snd_pcm_substream *substream)
{
	int ret = 0;
	int device = substream->pcm->device;
	struct snd_pcm *pcm = substream->pcm;
	struct snd_soc_pcm_runtime *rtd = pcm->private_data;
	struct hisi_pcm_data *pdata = snd_soc_platform_get_drvdata(rtd->platform);
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct hisi_pcm_runtime_data *prtd = runtime->private_data;
	struct pcm_thread_info *thread_info = NULL;

	if (!_is_valid_pcm_device(device))
		return 0;

	if (prtd == NULL) {
		loge("prtd is null\n");
		return -EINVAL;
	}

	if (pdata == NULL) {
		loge("platform data is null\n");
		return -EINVAL;
	}

	thread_info = &pdata->thread_info;
	if (prtd->using_thread_flag) {
		atomic_dec(&thread_info->using_thread_cnt);
	}

	logi("device %d, mode %d close\n", device, substream->stream);
	ret = hisi_pcm_notify_close(substream);
	if (ret)
		loge("pcm notify hifi close fail, ret %d\n", ret);

	if (device == PCM_DEVICE_MMAP) {
		hisi_pcm_free_mmap_share_buf(substream);
		unmap_hifi_share_buffer(substream);
	}

	kfree(prtd);
	runtime->private_data = NULL;

	if (substream->stream < PCM_STREAM_MAX) {
		thread_info->runtime_data[device][substream->stream] = NULL;
	}

	return ret;
}

static int hisi_pcm_ioctl(struct snd_pcm_substream *substream,
	unsigned int cmd, void *arg)
{
	struct hisi_pcm_runtime_data *prtd = substream->runtime->private_data;
	struct hisi_pcm_ion_buf *ion_buf = NULL;
	int pcm_device = substream->pcm->device;

	switch (cmd) {
	case SNDRV_PCM_IOCTL1_RESET:
		if (pcm_device == PCM_DEVICE_MMAP) {
			ion_buf = prtd->ion_buf;
			if (ion_buf && ion_buf->buf_addr)
				memset(ion_buf->buf_addr, 0, ion_buf->buf_size);/* unsafe_function_ignore: memset */
		}
		break;
	default:
		break;
	}

	return snd_pcm_lib_ioctl(substream, cmd, arg);
}

/* define all pcm ops of hisi pcm */
static struct snd_pcm_ops hisi_pcm_ops = {
	.open       = hisi_pcm_open,
	.close      = hisi_pcm_close,
	.ioctl      = hisi_pcm_ioctl,
	.hw_params  = hisi_pcm_hw_params,
	.hw_free    = hisi_pcm_hw_free,
	.prepare    = hisi_pcm_prepare,
	.trigger    = hisi_pcm_trigger,
	.pointer    = hisi_pcm_pointer,
};

static int preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;

	if ((pcm->device >= PCM_DEVICE_TOTAL) || (stream >= PCM_STREAM_MAX)) {
		loge("Invalid argument: device %d stream %d\n", pcm->device, stream);
		return -EINVAL;
	}

	if (pcm->device == PCM_DEVICE_MMAP)
		return 0;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->addr = g_pcm_dma_buf_config[pcm->device][stream].pcm_dma_buf_base;
	buf->bytes = g_pcm_dma_buf_config[pcm->device][stream].pcm_dma_buf_len;
	buf->area = ioremap(buf->addr, buf->bytes);

	if (!buf->area) {
		loge("dma buf area error\n");
		return -ENOMEM;
	}

	return 0;
}

static void free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	IN_FUNCTION;

	if (pcm->device == PCM_DEVICE_MMAP)
		return;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		iounmap(buf->area);

		buf->area = NULL;
		buf->addr = 0;
	}
}

static int hisi_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;

	if (!card->dev->dma_mask) {
		logi("dev->dma_mask not set\n");
		card->dev->dma_mask = &hisi_pcm_dmamask;
	}

	if (!card->dev->coherent_dma_mask) {
		logi("dev->coherent_dma_mask not set\n");
		card->dev->coherent_dma_mask = hisi_pcm_dmamask;
	}

	logi("PLATFORM machine set pcm-device %d\n", pcm->device);

	if (!_is_valid_pcm_device(pcm->device)) {
		logi("just alloc space for pcm device %d\n", pcm->device);
		return 0;
	}

	/* register callback */
	ret = hisi_pcm_notify_isr_register((void*)hisi_pcm_notify_recv_isr);
	if (ret) {
		loge("notify isr register error : %d\n", ret);
		return ret;
	}

	/* init lowlatency stream thread */
	if (_is_pcm_device_using_thread(pcm->device)) {
		ret = pcm_thread_start(pcm);
		if (ret) {
			loge("pcm thread start fail, ret %d\n", ret);
			return ret;
		}
	}

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
		if (ret) {
			loge("playback preallocate dma buffer fail\n");
			goto err;
		}
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
		if (ret) {
			loge("capture preallocate dma buffer fail\n");
			goto err;
		}
	}

	if (pcm->device == PCM_DEVICE_MMAP) {
		/* add a hwdep node for pcm device */
		ret = hisi_pcm_add_hwdep_dev(rtd);
		if (ret) {
			loge("add hw dep node for pcm device %d fail, ret %d\n", pcm->device, ret);
			goto err;
		}
	}

	return 0;

err:
	if (_is_pcm_device_using_thread(pcm->device)) {
		pcm_thread_stop(pcm);
	}

	free_dma_buffers(pcm);

	return ret;
}

static void hisi_pcm_free(struct snd_pcm *pcm)
{
	IN_FUNCTION;
	logi("hisi_pcm_free pcm-device %d\n", pcm->device);

	if (_is_pcm_device_using_thread(pcm->device))
		pcm_thread_stop(pcm);

	free_dma_buffers(pcm);

	OUT_FUNCTION;
}

struct snd_soc_platform_driver hisi_pcm_platform = {
	.ops      = &hisi_pcm_ops,
	.pcm_new  =  hisi_pcm_new,
	.pcm_free =  hisi_pcm_free,
};

/*lint -e429*/
static int  hisi_pcm_platform_probe(struct platform_device *pdev)
{
	int ret = -ENODEV;
	struct device *dev = &pdev->dev;
	struct hisi_pcm_data *pdata = NULL;
	struct pcm_thread_info *pcm_thread_para = NULL;

	IN_FUNCTION;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		loge("allocate hisi pcm platform data fail\n");
		goto probe_failed;
	}

	pcm_thread_para = &pdata->thread_info;
	atomic_set(&pcm_thread_para->using_thread_cnt, 0);

	platform_set_drvdata(pdev, pdata);

	ret = snd_soc_register_component(&pdev->dev, &hisi_pcm_component,
			hisi_pcm_dai, ARRAY_SIZE(hisi_pcm_dai));
	if (ret) {
		loge("snd_soc_register_dai return %d\n", ret);
		goto probe_failed;
	}

	dev_set_name(&pdev->dev, HISI_PCM_HIFI);
	ret = snd_soc_register_platform(&pdev->dev, &hisi_pcm_platform);
	if (ret) {
		loge("snd_soc_register_platform return %d\n", ret);
		snd_soc_unregister_component(&pdev->dev);
		goto probe_failed;
	}

	OUT_FUNCTION;

	return ret;

probe_failed:
	OUT_FUNCTION;
	return ret;
}
/*lint +e429*/

static int hisi_pcm_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static const struct of_device_id hisi_pcm_hifi_match_table[] = {
	{.compatible = HISI_PCM_HIFI, },
	{ },
};
static struct platform_driver hisi_pcm_platform_driver = {
	.driver = {
		.name  = HISI_PCM_HIFI,
		.owner = THIS_MODULE,
		.of_match_table = hisi_pcm_hifi_match_table,
	},
	.probe  = hisi_pcm_platform_probe,
	.remove = hisi_pcm_platform_remove,
};

static int __init hisi_pcm_init(void)
{
	IN_FUNCTION;
	return platform_driver_register(&hisi_pcm_platform_driver);
}
module_init(hisi_pcm_init);

static void __exit hisi_pcm_exit(void)
{
	platform_driver_unregister(&hisi_pcm_platform_driver);
}
module_exit(hisi_pcm_exit);

MODULE_AUTHOR("S00212991");
MODULE_DESCRIPTION("HISI HIFI platform driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hifi");

