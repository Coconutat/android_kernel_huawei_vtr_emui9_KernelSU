/*
*Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*/
#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/compiler.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/clk.h>
#include <linux/hwspinlock.h>
#include <linux/semaphore.h>
#include <linux/hisi/hi64xx_hifi_misc.h>

#include <sound/core.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include "mlib_static_ringbuffer.h"
#include "soundtrigger_dma_drv.h"
#include "soundtrigger_log.h"
#include "slimbus.h"
#include "hifi_lpp.h"
#include "asp_dma.h"
#include "soundtrigger_event.h"
#include "soundtrigger_socdsp_mailbox.h"
#include "soundtrigger_socdsp_pcm.h"

/*lint -e846 -e516 -e514 -e866 -e30 -e84*/

#define DRV_NAME									"soundtrigger_dma_drv"

#define COMP_SOUNDTRIGGER_DMA_DRV_NAME				"hisilicon,soundtrigger_dma_drv"

#define SOUNDTRIGGER_HWLOCK_ID						(5)

#define HISILICON_SOUNDTRIGGER_DMA_DRIVER_VERSION	"1.0"

#define MAX_MSG_SIZE			(1024)

#define RINGBUFFER_SIZE 		(61440) 									/* accoring to 6402 ringbuffer size, which will be totally transmited to AP */

#define FAST_FRAME_LENGTH		(1920)										/* time: 10ms */

#define FAST_TRAN_RATE			(192000)

#define HI6402_NORMAL_FRAME_LENGTH 	(160)										/* time: 10ms */

#define HI6402_NORMAL_TRAN_RATE		(16000)

#define HI6402_4SMARTPA_NORMAL_FRAME_LENGTH 	(480)										/* time: 10ms */

#define HI6402_4SMARTPA_NORMAL_TRAN_RATE		(48000)

#define HI6403_NORMAL_FRAME_LENGTH 	(480)										/* time: 10ms */

#define HI6403_NORMAL_TRAN_RATE		(48000)
#define BYTE_COUNT				(4) 										/* each sampling point has 4 bytes */

#define VALID_BYTE_COUNT		(2) 										/* each sampling point only has 2 bytes valid data */

#define VALID_BYTE_COUNT_FAST	(4)

#define FAST_HEAD_FRAME_COUNT	(1) 										/* head frame count of fast channel, which is full of 0x5A5A */

#define FAST_TAIL_FRAME_COUNT	(1) 										/* tail  frame count of fast channel, which is full of 0x5A5A */

#define NORMAL_HEAD_FRAME_COUNT (2) 										/* head frame count of normal channel, which is full of 0x5A5A */

#define HI6402_FAST_TRAN_COUNT 		(RINGBUFFER_SIZE / (FAST_FRAME_LENGTH * VALID_BYTE_COUNT) + FAST_HEAD_FRAME_COUNT + FAST_TAIL_FRAME_COUNT)	/* 16 + 2=18 */
#define FAST_TRAN_COUNT 		(RINGBUFFER_SIZE / (FAST_FRAME_LENGTH * VALID_BYTE_COUNT_FAST) + FAST_HEAD_FRAME_COUNT + FAST_TAIL_FRAME_COUNT)	/* 8 + 2=10 */

#define HI6402_FAST_BUFFER_SIZE		(FAST_FRAME_LENGTH * VALID_BYTE_COUNT_FAST * HI6402_FAST_TRAN_COUNT)
#define FAST_BUFFER_SIZE		(FAST_FRAME_LENGTH * VALID_BYTE_COUNT_FAST * FAST_TRAN_COUNT)													/* 1920x2x18=69120 */

#define HEAD_FRAME_WORD 		(0x5A5A)																									/* key word that fill with head/tail frame */

#define RINGBUF_FRAME_LEN		(160)

#define RINGBUF_FRAME_COUNT		(200)

#define RINGBUF_HEAD_SIZE		(20)

#define NORMAL_BUFFER_SIZE		(RINGBUF_FRAME_LEN * VALID_BYTE_COUNT * RINGBUF_FRAME_COUNT + RINGBUF_HEAD_SIZE)								/* according to mlib_ringbuffer.c */

#define TIMEOUT_CLOSE_DMA_MS		(60000)

//#define STEREO_TYPE
	#define DMA_PORT_NUM		(1)

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif

typedef struct
{
	uint32_t  port; 		/* slimbus port address */
	uint32_t  config;		/* dma config number */
	uint32_t  channel;		/* dma channel number */
}DRV_DMA_CONFIG_STRU;

struct soundtrigger_pcm_config {
	uint32_t channels;		/* stereo type: channel = 2, mono type: channel = 1 */
	uint32_t rate;			/* sampling rate */
	uint32_t frame_len; 	/* frame length */
	uint32_t byte_count;	/* each sampling point contain byte number	*/
};

struct soundtrigger_pcm_info {
	struct soundtrigger_pcm_config	pcm_cfg;
	void							*buffer[DMA_PORT_NUM][PCM_SWAP_BUFFER_NUM]; 				/* swap buffer */
	void							*buffer_physical_addr[DMA_PORT_NUM][PCM_SWAP_BUFFER_NUM];	/* swap buffer physical addr */
	uint32_t						buffer_size;												/* one swap buffer size, each sampling point contains 4 bytes */
	uint32_t						channel[DMA_PORT_NUM];										/* dma channel number */
	struct dma_lli_cfg				*dma_cfg[DMA_PORT_NUM][PCM_SWAP_BUFFER_NUM];
	void							*lli_dma_physical_addr[DMA_PORT_NUM][PCM_SWAP_BUFFER_NUM];
};

struct fast_tran_info {
	uint32_t	fast_frame_find_flag;			/* flag to decide whether get first frame; before first frame, all input data is unuse */
	uint32_t	fast_start_addr;				/* address of valid data in the first frame */
	uint16_t	fast_head_frame_word;			/* word of head frame which is full of unuse data, such as 0x5A5A */
	uint32_t	fast_head_frame_size;
	uint16_t	fast_buffer[FAST_BUFFER_SIZE];	/* buffer to store all fast transmit data, including head frame */
	uint32_t	fast_buffer_size;
	int32_t 	dma_tran_count; 				/* tnumber of valid fast transmission */
	int32_t 	dma_tran_total_count;			/* total number of fast transmit */
	int32_t 	fast_complete_flag; 			/* fast tansmit complete flag */
	int32_t 	fast_read_complete_flag;		/* flag to decide whether HAL read cpmlete */
	uint32_t	irq_count_left; 				/* left channel interrupt count */
	uint32_t	irq_count_right;				/* left channel interrupt count */
	uint32_t	read_count_left;				/* left channel read count */
	uint32_t	read_count_right;				/* right channel read count */
};

struct normal_tran_info {
	uint32_t	normal_frame_find_flag; 		/* flag to decide whether get first frame; before first frame, all input data is unuse */
	uint32_t	normal_start_addr;				/* address of valid data in the first frame */
	uint32_t	normal_first_frame_read_flag;	/* flag to decide whether HAL read first frame */
	uint16_t	normal_head_frame_word; 		/*word of head frame which is full of unuse data, such as 0xA5A5 */
	uint32_t	normal_head_frame_size;
	uint16_t	normal_buffer[NORMAL_BUFFER_SIZE];/* ringbuffer which stores normal transmit data , including head frame */
	uint32_t	normal_buffer_size;
	uint32_t	normal_tran_count;				/* tnumber of valid normal transmission */
	uint32_t	irq_count_left; 				/* left channel interrupt count */
	uint32_t	irq_count_right;				/* left channel interrupt count */
	uint32_t	read_count_left;				/* left channel read count */
	uint32_t	read_count_right;				/* right channel read count */
};

struct soundtrigger_dma_drv_info {
	struct hwspinlock	*hwlock;
	spinlock_t			lock;
	struct resource 	*res;
	void __iomem		*reg_base_addr;
	void __iomem		*v_slimbus_base_reg_addr;
	struct regulator	*asp_ip;
	struct device		*dev;
	uint32_t			dma_int_fast_flag;
	uint32_t			dma_int_nomal_flag;
	uint32_t			dma_alloc_flag;
	uint32_t			soundtrigger_dma_drv_state;
	int32_t 			fm_status;
	uint32_t			is_dma_enable;
	uint32_t			is_slimbus_enable;
	enum codec_hifi_type type;
	struct clk *asp_subsys_clk;
	struct soundtrigger_pcm_info	st_pcm_info[SOUNDTRIGGER_PCM_CHAN_NUM];
	struct fast_tran_info			st_fast_tran_info;
	struct normal_tran_info 		st_normal_tran_info;

	struct workqueue_struct 		*soundtrigger_delay_wq;
	struct delayed_work 			soundtrigger_delay_dma_fast_left_work;		/* delay work of interrupt response for fast transmit left channel */
	struct delayed_work 			soundtrigger_delay_dma_fast_right_work; 	/* delay work of interrupt response for fast transmit right channel */
	struct delayed_work 			soundtrigger_delay_dma_normal_left_work;	/* delay work of interrupt response for normal transmit left channel */
	struct delayed_work 			soundtrigger_delay_dma_normal_right_work;	/* delay work of interrupt response for normal transmit right channel */

	struct workqueue_struct 		*soundtrigger_delay_close_dma_wq;
	struct delayed_work 			soundtrigger_delay_close_dma_timeout_work;	/* delay work of close dma when timeout */

	struct wake_lock st_wake_lock;
	struct mutex ioctl_mutex;
};

static uint32_t hi6402_normal_frame_length = HI6402_NORMAL_FRAME_LENGTH;
static uint32_t hi6402_normal_tran_rate = HI6402_NORMAL_TRAN_RATE;


DRV_DMA_CONFIG_STRU hi6403_soundtrigger_dma_fast_cfg[2] = {
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x1280), .config = 0x433220a7, .channel = DMA_FAST_LEFT_CH_NUM},	/*hi6403 fast data left channel*/
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x12c0), .config = 0x43322077, .channel = DMA_FAST_RIGHT_CH_NUM},	/*hi6403 fast data right channel*/
};

DRV_DMA_CONFIG_STRU hi6403_soundtrigger_dma_normal_cfg[2] = {
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x1080), .config = 0x43322027, .channel = DMA_NORMAL_LEFT_CH_NUM},	/*hi6403 normal data left channel*/
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x10c0), .config = 0x433220b7, .channel = DMA_NORMAL_RIGHT_CH_NUM},	/*hi6403 normal data right channel*/
};

DRV_DMA_CONFIG_STRU hi6402_soundtrigger_dma_fast_cfg[2] = {
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x1180), .config = 0x43322067, .channel = DMA_FAST_LEFT_CH_NUM},	/*hi6402 fast data left channel*/
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x11c0), .config = 0x43322077, .channel = DMA_FAST_RIGHT_CH_NUM},	/*hi6402 fast data right channel*/
};

DRV_DMA_CONFIG_STRU hi6402_soundtrigger_dma_normal_cfg[2] = {
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x1280), .config = 0x433220a7, .channel = DMA_NORMAL_LEFT_CH_NUM},	/*hi6402 normal data left channel*/
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x12c0), .config = 0x433220b7, .channel = DMA_NORMAL_RIGHT_CH_NUM},	/*hi6402 normal data right channel*/
};

DRV_DMA_CONFIG_STRU hi6402_soundtrigger_dma_fast_cfg_4smartpa[2] = {
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x1280), .config = 0x433220a7, .channel = DMA_FAST_LEFT_CH_NUM}, /*hi6402 fast data left channel*/
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x12c0), .config = 0x433220b7, .channel = DMA_FAST_RIGHT_CH_NUM}, /*hi6402 fast data right channel*/
};

DRV_DMA_CONFIG_STRU hi6402_soundtrigger_dma_normal_cfg_4smartpa[2] = {
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x1080), .config = 0x43322027, .channel = DMA_NORMAL_LEFT_CH_NUM}, /*hi6402 normal data left channel*/
	{.port = (HI3xxx_SLIMBUS_BASE_REG + 0x10c0), .config = 0x43322037, .channel = DMA_NORMAL_RIGHT_CH_NUM}, /*hi6402 normal data right channel*/
};

DRV_DMA_CONFIG_STRU *hi640X_soundtrigger_dma_cfg_4smartpa[CODEC_HI640X_MAX][SOUNDTRIGGER_PCM_CHAN_NUM] = {
	{hi6402_soundtrigger_dma_fast_cfg_4smartpa,hi6402_soundtrigger_dma_normal_cfg_4smartpa,},
	{hi6403_soundtrigger_dma_fast_cfg,hi6403_soundtrigger_dma_normal_cfg,},
};

DRV_DMA_CONFIG_STRU *hi640X_soundtrigger_dma_cfg_default[CODEC_HI640X_MAX][SOUNDTRIGGER_PCM_CHAN_NUM] = {
	{hi6402_soundtrigger_dma_fast_cfg,hi6402_soundtrigger_dma_normal_cfg,},
	{hi6403_soundtrigger_dma_fast_cfg,hi6403_soundtrigger_dma_normal_cfg,},
};

DRV_DMA_CONFIG_STRU * (*hi640X_soundtrigger_dma_cfg)[SOUNDTRIGGER_PCM_CHAN_NUM] = hi640X_soundtrigger_dma_cfg_default;



struct soundtrigger_pcm_config hi640X_pcm_cfg[CODEC_HI640X_MAX][SOUNDTRIGGER_PCM_CHAN_NUM] = {
	{
		/*hi6402_pcm_fast_cfg*/
		{
			.channels = DMA_PORT_NUM,
			.rate = FAST_TRAN_RATE,
			.frame_len = FAST_FRAME_LENGTH,
			.byte_count = BYTE_COUNT,
		},
		/*hi6402_pcm_normal_cfg*/
		{
			.channels = DMA_PORT_NUM,
			.rate = HI6402_NORMAL_TRAN_RATE,
			.frame_len = HI6402_NORMAL_FRAME_LENGTH,
			.byte_count = BYTE_COUNT,
		},
	},
	{
		/*hi6403_pcm_fast_cfg*/
		{
			.channels = DMA_PORT_NUM,
			.rate = FAST_TRAN_RATE,
			.frame_len = FAST_FRAME_LENGTH,
			.byte_count = BYTE_COUNT,
		},
		/*hi6403_pcm_normal_cfg*/
		{
			.channels = DMA_PORT_NUM,
			.rate = HI6403_NORMAL_TRAN_RATE,
			.frame_len = HI6403_NORMAL_FRAME_LENGTH,
			.byte_count = BYTE_COUNT,
		},
	},
};

struct soundtrigger_dma_drv_info *g_dma_drv_info = NULL;

static int32_t soundtrigger_dmac_irq_handler(unsigned short int_type, unsigned long para, unsigned int dma_channel);

static int get_input_param(unsigned int usr_para_size,
				  void __user *usr_para_addr,
				  unsigned int *krn_para_size,
				  void **krn_para_addr)
{
	void *para_in = NULL;
	unsigned int para_size_in = 0;

	if (!usr_para_addr) {
		loge("usr_para_addr NULL\n");
		goto ERR;
	}

	if ((usr_para_size == 0) || (usr_para_size > MAX_MSG_SIZE)) {
		loge("usr_para_size invalid %d, max %d.\n", usr_para_size, MAX_MSG_SIZE);
		goto ERR;
	}

	para_size_in = roundup(usr_para_size, 4);

	para_in = kzalloc(para_size_in, GFP_KERNEL);
	if (para_in == NULL) {
		loge("kzalloc fail\n");
		goto ERR;
	}

	if (copy_from_user(para_in , (void __user *)usr_para_addr, usr_para_size)) {
		loge("copy_from_user fail\n");
		goto ERR;
	}

	*krn_para_size = para_size_in;
	*krn_para_addr = para_in;

	return 0;

ERR:
	if (para_in) {
		kfree(para_in);
		para_in = NULL;
	}
	return -EIO;
}

static void param_free(void **krn_para_addr)
{
	if (*krn_para_addr) {
		kfree(*krn_para_addr);
		*krn_para_addr = NULL;
	} else {
		loge("krn_para_addr to free is NULL\n");
	}

	return;
}

static void dma_dump_addr_info(struct soundtrigger_dma_drv_info *dma_drv_info)
{
	uint32_t i;
	uint32_t j;
	uint32_t k;
	struct soundtrigger_pcm_info *pcm_info = NULL;
	void    *soundtrigger_addr = (void *)CODEC_DSP_SOUNDTRIGGER_BASE_ADDR;

	logi("dma config soundtrigger soundtrigger_addr[%pK]\n", soundtrigger_addr);

	for (i = 0; i < ARRAY_SIZE(dma_drv_info->st_pcm_info); i++) {
		pcm_info = &(dma_drv_info->st_pcm_info[i]);
		for (j = 0; j < ARRAY_SIZE(pcm_info->channel); j++) {
			for (k = 0; k < ARRAY_SIZE(pcm_info->dma_cfg[j]); k++) {
				logi("dma soundtrigger info: dma_num:%d, buffer_num:%d,"
					"buffer_physical_addr:%pK, buffer:%pK,"
					"lli_dma_physical_addr:%pK, dma_cfg:%pK\n",
					j, k,
					pcm_info->buffer_physical_addr[j][k],
					pcm_info->buffer[j][k],
					pcm_info->lli_dma_physical_addr[j][k],
					pcm_info->dma_cfg[j][k]);

				logi("a count:0x%x, src addr:0x%pK, dest addr:0x%pK, config:0x%x\n",
					pcm_info->dma_cfg[j][k]->a_count,
					(void *)(unsigned long)(pcm_info->dma_cfg[j][k]->src_addr),
					(void *)(unsigned long)(pcm_info->dma_cfg[j][k]->des_addr),
					pcm_info->dma_cfg[j][k]->config);
			}
		}
	}
	return;
}


static inline int32_t slimbus_register_read(struct soundtrigger_dma_drv_info *dma_drv_info,uint32_t reg)
{
	unsigned long flag = 0;
	int32_t ret = 0;

	BUG_ON(NULL == dma_drv_info);

	if (hwspin_lock_timeout_irqsave(dma_drv_info->hwlock, HWLOCK_WAIT_TIME, &flag)) {
		loge("hwspinlock timeout\n");
		return 0;
	}

	ret = readl(dma_drv_info->v_slimbus_base_reg_addr + reg);

	hwspin_unlock_irqrestore(dma_drv_info->hwlock, &flag);
	return ret;
}

static void pcm_valid_data_get(uint32_t *input_buffer, uint16_t *output_buffer, int32_t frame_count)
{
	int32_t count = 0;

	BUG_ON(NULL == input_buffer);
	BUG_ON(NULL == output_buffer);

	for (count = 0; count < frame_count; count++) {
		output_buffer[count] = input_buffer[count]>>16;
	}
}

static void pcm_48K_mono_to_16K_mono(uint16_t *input_buffer, uint16_t *output_buffer, int32_t output_len)
{
	int32_t count = 0;

	BUG_ON(NULL == input_buffer);
	BUG_ON(NULL == output_buffer);

	for (count = 0; count < output_len; count++) {
		output_buffer[count] = input_buffer[3 * count];/*lint !e679*/
	}
}

static bool pcm_start_addr_find(uint32_t pcm_index, uint16_t *input_buffer, uint32_t input_length, uint32_t *start_addr)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;
	struct fast_tran_info *fast_info = &(dma_drv_info->st_fast_tran_info);
	struct normal_tran_info *normal_info = &(dma_drv_info->st_normal_tran_info);
	uint32_t index = 0;
	static uint32_t s_fast_count;
	static uint32_t s_normal_count;

	if (pcm_index == SOUNDTRIGGER_PCM_FAST) {
		if ((input_buffer[0] != fast_info->fast_head_frame_word) && (input_buffer[input_length - 1] != fast_info->fast_head_frame_word)) {
			logw("don't have fast head frame, frame word:0x%x, first data:0x%x, last data:0x%x.\n",
				fast_info->fast_head_frame_word, input_buffer[0], input_buffer[input_length - 1]);
			return false;
		}

		for (index = 0; index < input_length; index++) {
			if (input_buffer[index] == fast_info->fast_head_frame_word) {
				s_fast_count++;
			} else {
				if (s_fast_count > (input_length * FAST_HEAD_FRAME_COUNT / 2)) {
					*start_addr = index;
					s_fast_count = 0;
					logi("fast channel find head, index:%d.\n", index);
					return true;
				}
			}
		}
	}

	if (pcm_index == SOUNDTRIGGER_PCM_NORMAL) {
		if ((input_buffer[0] != normal_info->normal_head_frame_word) && (input_buffer[input_length - 1] != normal_info->normal_head_frame_word))
			return false;

		for (index = 0; index < input_length; index++) {
			if (input_buffer[index] == normal_info->normal_head_frame_word) {
				s_normal_count++;
			} else {
				if (s_normal_count > (input_length * NORMAL_HEAD_FRAME_COUNT / 2)) {
					*start_addr = index;
					s_normal_count = 0;
					logi("normal channel find head.normal config:%x, fast config:%x,"
						"fast src:%x, fast dst:%x, fast size:%x\n",
						_dmac_reg_read(ASP_DMA_CX_CONFIG(DMA_NORMAL_LEFT_CH_NUM)),
						_dmac_reg_read(ASP_DMA_CX_CONFIG(DMA_FAST_LEFT_CH_NUM)),
						_dmac_reg_read(ASP_DMA_CX_SRC_ADDR(DMA_FAST_LEFT_CH_NUM)),
						_dmac_reg_read(ASP_DMA_CX_DES_ADDR(DMA_FAST_LEFT_CH_NUM)),
						_dmac_reg_read(ASP_DMA_CX_CNT0(DMA_FAST_LEFT_CH_NUM)));
					dma_dump_addr_info(dma_drv_info);
					return true;
				}
				s_normal_count = 0;
			}
		}
	}
	return false;
}

static void pcm_fast_buffer_check(struct fast_tran_info *fast_info)
{
	uint32_t word_count;
	uint32_t index;
	uint16_t *fast_valid_buffer = fast_info->fast_buffer + fast_info->fast_start_addr;
	uint32_t fast_valid_buffer_len = RINGBUFFER_SIZE / VALID_BYTE_COUNT;

	for (word_count = 0; word_count < fast_valid_buffer_len; word_count++) {
		if (fast_valid_buffer[fast_valid_buffer_len - 1 - word_count] != fast_info->fast_head_frame_word)
			break;
	}

	for (index = 0; index < word_count; index++) {
		fast_valid_buffer[fast_valid_buffer_len - 1 - index] = fast_valid_buffer[fast_valid_buffer_len - 1 - word_count];
		logi("fast tran slimbus miss sampling point[%d].\n", word_count);
	}
}

static int32_t dma_config_clear(struct soundtrigger_dma_drv_info *dma_drv_info)
{
	uint32_t i;
	uint32_t j;
	uint32_t k;
	struct   soundtrigger_pcm_info *pcm_info;

	if (!dma_drv_info) {
		return -ENOENT;
	}

	logi("dma config clear\n");

	for (i = 0; i < ARRAY_SIZE(dma_drv_info->st_pcm_info); i++) {
		pcm_info = &(dma_drv_info->st_pcm_info[i]);
		for (j = 0; j < ARRAY_SIZE(pcm_info->channel); j++) {
			for (k = 0; k < ARRAY_SIZE(pcm_info->dma_cfg[j]); k++) {
				if ((pcm_info->buffer[j][k]) != NULL) {
					if (dma_drv_info->type == CODEC_HI6402) {
						dma_free_coherent(dma_drv_info->dev,
							(unsigned long)pcm_info->buffer_size,
							pcm_info->buffer[j][k],
							(dma_addr_t)pcm_info->buffer_physical_addr[j][k]);
					} else {
						iounmap(pcm_info->buffer[j][k]);
					}
					pcm_info->buffer[j][k] = NULL;
				}
				if ((pcm_info->dma_cfg[j][k]) != NULL){
					if (dma_drv_info->type == CODEC_HI6402) {
						dma_free_coherent(dma_drv_info->dev,
							sizeof(struct dma_lli_cfg),
							pcm_info->dma_cfg[j][k],
							(dma_addr_t)pcm_info->lli_dma_physical_addr[j][k]);
					} else {
						iounmap(pcm_info->dma_cfg[j][k]);
					}
					pcm_info->dma_cfg[j][k] = NULL;
				}
			}
		}
	}

	return 0;
}


static int32_t soundtrigger_pcm_init(struct soundtrigger_pcm_info *pcm_info,
		uint32_t hifi_type, uint32_t pcm_channel, uint64_t *cfg_addr)
{
	uint32_t i;
	uint32_t j;
	uint32_t buff_size;
	int32_t err;

	if (!g_dma_drv_info)
		return -ENOENT;

	buff_size = ARRAY_SIZE(pcm_info->dma_cfg[0]);
	for (i = 0; i < ARRAY_SIZE(pcm_info->channel); i++) {
		pcm_info->channel[i] =
			(hi640X_soundtrigger_dma_cfg[hifi_type][pcm_channel] + i)->channel;
		for (j = 0; j < buff_size; j++) {
			if (g_dma_drv_info->type == CODEC_HI6402) {
				pcm_info->buffer[i][j] = (void *)dma_alloc_coherent(g_dma_drv_info->dev,
					(unsigned long)pcm_info->buffer_size,
					(dma_addr_t *)&(pcm_info->buffer_physical_addr[i][j]),
					GFP_KERNEL);
				if (pcm_info->buffer[i][j] == NULL) {
					err = -ENOMEM;
					return err;
				}
				memset(pcm_info->buffer[i][j], 0, (unsigned long)pcm_info->buffer_size);
				pcm_info->dma_cfg[i][j] =
					(struct dma_lli_cfg *)dma_alloc_coherent(g_dma_drv_info->dev,
					sizeof(struct dma_lli_cfg),
					(dma_addr_t *)&(pcm_info->lli_dma_physical_addr[i][j]),
					GFP_KERNEL);
				if (pcm_info->dma_cfg[i][j] == NULL) {
					err = -ENOMEM;
					return err;
				}
				memset(pcm_info->dma_cfg[i][j], 0, sizeof(struct dma_lli_cfg));
			} else {
				/*remap the soundtrigger address*/
				pcm_info->buffer_physical_addr[i][j] = (void *)(*cfg_addr);
				pcm_info->buffer[i][j] =
					ioremap_wc((phys_addr_t)(*cfg_addr), (uint64_t)(pcm_info->buffer_size));
				memset(pcm_info->buffer[i][j], 0, (uint64_t)(pcm_info->buffer_size));
				*cfg_addr += pcm_info->buffer_size;

				pcm_info->lli_dma_physical_addr[i][j] = (void *)(*cfg_addr);
				pcm_info->dma_cfg[i][j] =
					ioremap_wc((phys_addr_t)(*cfg_addr), sizeof(struct dma_lli_cfg));
				memset(pcm_info->dma_cfg[i][j], 0,
					sizeof(struct dma_lli_cfg));
				*cfg_addr += sizeof(struct dma_lli_cfg);
			}

			/* set dma config */
			pcm_info->dma_cfg[i][j]->config =
				(hi640X_soundtrigger_dma_cfg[hifi_type][pcm_channel] + i)->config;
			pcm_info->dma_cfg[i][j]->src_addr =
				(hi640X_soundtrigger_dma_cfg[hifi_type][pcm_channel] + i)->port;

			pcm_info->dma_cfg[i][j]->a_count = pcm_info->buffer_size;
		}
	}
	return 0;
}

static void soundtrigger_pcm_adjust(struct soundtrigger_pcm_info *pcm_info)
{
	uint32_t i;
	uint32_t j;
	uint32_t next_addr;
	uint32_t real_pos;
	uint32_t buff_size;

	buff_size = ARRAY_SIZE(pcm_info->dma_cfg[0]);
	for (i = 0; i < ARRAY_SIZE(pcm_info->channel); i++) {
		for (j = 0; j < buff_size; j++) {
			real_pos = (j + 1) % buff_size;
			next_addr = (uint32_t)(uint64_t)pcm_info->lli_dma_physical_addr[i][real_pos];
			pcm_info->dma_cfg[i][j]->des_addr = (uint32_t)(uint64_t)pcm_info->buffer_physical_addr[i][j];
			pcm_info->dma_cfg[i][j]->lli = DRV_DMA_LLI_LINK(next_addr);
		}
	}
	return;
}

static int32_t dma_config_set(struct soundtrigger_dma_drv_info *dma_drv_info)
{
	int32_t err = 0;
	uint32_t pcm_channel;
	uint32_t hifi_type;
	uint32_t pcm_num;
	struct soundtrigger_pcm_info *pcm_info = NULL;
	uint64_t cfg_addr = CODEC_DSP_SOUNDTRIGGER_BASE_ADDR;

	if (!dma_drv_info ) {
		err = -ENOENT;
		goto err_exit;
	}

	hifi_type = dma_drv_info->type;
	pcm_num = ARRAY_SIZE(dma_drv_info->st_pcm_info);

	for (pcm_channel = 0; pcm_channel < pcm_num; pcm_channel++) {
		pcm_info = &(dma_drv_info->st_pcm_info[pcm_channel]);
		memcpy(&(pcm_info->pcm_cfg), &hi640X_pcm_cfg[hifi_type][pcm_channel],
			sizeof(struct soundtrigger_pcm_config));
		pcm_info->buffer_size =
			pcm_info->pcm_cfg.frame_len * pcm_info->pcm_cfg.byte_count;

		err = soundtrigger_pcm_init(pcm_info, hifi_type, pcm_channel, &cfg_addr);
		if (err) {
			loge("dma config fail, err:[%d].\n", err);
			goto err_exit;
		}

		soundtrigger_pcm_adjust(pcm_info);
	}

	dma_dump_addr_info(dma_drv_info);
	return 0;

err_exit:
	dma_config_clear(dma_drv_info);

	loge("dma config fail, err:[%d].\n", err);
	return err;
}

/*lint -e715*/
static int32_t dma_start(struct st_fast_status * fast_status)
{
	int32_t err;
	uint32_t pcm_channel;
	uint32_t dma_channel;
	uint32_t dma_port_num;
	uint32_t track_type;
	struct soundtrigger_pcm_info *pcm_info = NULL;
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;
	slimbus_track_param_t  slimbus_params;
	slimbus_device_type_t device_type = SLIMBUS_DEVICE_NUM;
	int ret = 0;

	if (!dma_drv_info) {
		err = -ENOENT;
		goto err_exit;
	}

	if (dma_drv_info->soundtrigger_dma_drv_state != DMA_DRV_INIT) {
		err = -EAGAIN;
		goto err_exit;
	}

	if (dma_drv_info->is_dma_enable) {
		err = -EAGAIN;
		goto err_exit;
	}

	memset(&slimbus_params, 0x0, sizeof(slimbus_params));
	/* slimbus config */
	slimbus_params.rate = FAST_TRAN_RATE;
	logi("fast slimbus_params.rate[%d]\n",slimbus_params.rate);

	if (CODEC_HI6402 == dma_drv_info->type) {
		if (hi6402_normal_tran_rate == HI6402_4SMARTPA_NORMAL_TRAN_RATE)
			slimbus_params.channels = 1;
		else
			slimbus_params.channels = 2;
		device_type = SLIMBUS_DEVICE_HI6402;
		track_type = SLIMBUS_TRACK_SOUND_TRIGGER;
	} else if (CODEC_HI6403 == dma_drv_info->type) {
		slimbus_params.channels = 1;
		device_type = SLIMBUS_DEVICE_HI6403;
		track_type = SLIMBUS_TRACK_SOUND_TRIGGER;
	}
	else {
		err = -EINVAL;
		loge( "device type is err %d\n", device_type);
		goto err_exit;
	}

	(void)hi64xx_request_pll_resource(HI_FREQ_SCENE_FASTTRANS);
	msleep(2);
	ret = slimbus_track_activate(device_type, track_type, &slimbus_params);
	logi("dma start request pll resource and switch to codec\n");
	if (ret != 0) {
		logi("soundtrigger activate fail\n");
		hi64xx_release_pll_resource(HI_FREQ_SCENE_FASTTRANS);
		return ret;
	}
	dma_drv_info->is_slimbus_enable = 1;

	wake_lock(&dma_drv_info->st_wake_lock);

	if (!queue_delayed_work(dma_drv_info->soundtrigger_delay_close_dma_wq,
				&dma_drv_info->soundtrigger_delay_close_dma_timeout_work, msecs_to_jiffies(TIMEOUT_CLOSE_DMA_MS))) {
		loge("close dma timeout lost msg\n");
	}

	for (pcm_channel = 0; pcm_channel < ARRAY_SIZE(dma_drv_info->st_pcm_info); pcm_channel++) {
		pcm_info = &(dma_drv_info->st_pcm_info[pcm_channel]);

		for (dma_port_num = 0; dma_port_num < ARRAY_SIZE(pcm_info->channel); dma_port_num++) {
			dma_channel = pcm_info->channel[dma_port_num];
			asp_dma_config((unsigned short)dma_channel, pcm_info->dma_cfg[dma_port_num][0], soundtrigger_dmac_irq_handler, (unsigned long)0);
			asp_dma_start((unsigned short)dma_channel, pcm_info->dma_cfg[dma_port_num][0]);
		}
	}
	dma_drv_info->is_dma_enable = 1;

	return ret;/*lint !e454*/

err_exit:
	return err;
}
/*lint +e715*/

static void dma_stop(int32_t pcm_channel)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;
	struct soundtrigger_pcm_info *pcm_info = &(dma_drv_info->st_pcm_info[pcm_channel]);
	uint32_t dma_port_num;
	uint32_t dma_channel;

	for (dma_port_num = 0; dma_port_num < ARRAY_SIZE(pcm_info->channel); dma_port_num++) {
		dma_channel = pcm_info->channel[dma_port_num];
		asp_dma_stop((unsigned short)dma_channel);
	}
}

static int32_t dma_open(struct st_fast_status * fast_status)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;
	struct fast_tran_info *fast_info = &(dma_drv_info->st_fast_tran_info);
	struct normal_tran_info *normal_info = &(dma_drv_info->st_normal_tran_info);
	int32_t err;

	if (dma_drv_info->soundtrigger_dma_drv_state != DMA_DRV_NO_INIT)
		return -EAGAIN;

	/*get fast buffer*/
	memset(fast_info->fast_buffer, 0x00, sizeof(uint16_t) * FAST_BUFFER_SIZE);

	fast_info->fast_read_complete_flag = READ_NOT_COMPLETE;
	fast_info->dma_tran_count = 0;
	fast_info->fast_complete_flag = FAST_TRAN_NOT_COMPLETE;
	fast_info->fast_head_frame_word = HEAD_FRAME_WORD;
	if (CODEC_HI6402 == dma_drv_info->type) {
		fast_info->fast_buffer_size = HI6402_FAST_BUFFER_SIZE;
		fast_info->fast_head_frame_size = FAST_FRAME_LENGTH * VALID_BYTE_COUNT;
		fast_info->dma_tran_total_count = HI6402_FAST_TRAN_COUNT;
	} else {
		fast_info->fast_buffer_size = FAST_BUFFER_SIZE;
		fast_info->fast_head_frame_size = FAST_FRAME_LENGTH * VALID_BYTE_COUNT_FAST;
		fast_info->dma_tran_total_count = FAST_TRAN_COUNT;
	}
	fast_info->fast_frame_find_flag = FRAME_NOT_FIND;
	fast_info->irq_count_left = 0;
	fast_info->irq_count_right = 0;
	fast_info->read_count_left = 0;
	fast_info->read_count_right = 0;

	/*get normal buffer*/
	Static_RingBuffer_Init(normal_info->normal_buffer, (RINGBUF_FRAME_LEN * VALID_BYTE_COUNT), RINGBUF_FRAME_COUNT);

	normal_info->normal_buffer_size = NORMAL_BUFFER_SIZE;
	normal_info->normal_head_frame_word = HEAD_FRAME_WORD;
	if (CODEC_HI6402 == dma_drv_info->type) {
		normal_info->normal_head_frame_size = hi6402_normal_frame_length * VALID_BYTE_COUNT;
	} else if (CODEC_HI6403 == dma_drv_info->type) {
		normal_info->normal_head_frame_size = HI6403_NORMAL_FRAME_LENGTH * VALID_BYTE_COUNT;
	}
	else {
		loge( "device type is err %d\n", dma_drv_info->type);
		return -EINVAL;
	}

	normal_info->normal_frame_find_flag = FRAME_NOT_FIND;
	normal_info->normal_first_frame_read_flag = READ_NOT_COMPLETE;
	normal_info->normal_tran_count = 0;
	normal_info->irq_count_left = 0;
	normal_info->irq_count_right = 0;
	normal_info->read_count_left = 0;
	normal_info->read_count_right = 0;

	dma_drv_info->fm_status = fast_status->fm_status;

	if (dma_drv_info->dma_alloc_flag == 0) {
		err = dma_config_set(dma_drv_info);
		if (err)
			goto err_exit;
		dma_drv_info->dma_alloc_flag = 1;
	}

	dma_drv_info->dma_int_fast_flag = 0;
	dma_drv_info->dma_int_nomal_flag = 0;

	logi("soundtrigger_dma open aspclk:%d, --\n", clk_get_enable_count(dma_drv_info->asp_subsys_clk));

	err = clk_prepare_enable(dma_drv_info->asp_subsys_clk);
	if (err) {
		loge( "clk prepare enable failed, err:[%d].\n", err);
		goto err_exit;
	}

	logi("soundtrigger_dma open aspclk:%d, ++\n", clk_get_enable_count(dma_drv_info->asp_subsys_clk));

	dma_drv_info->soundtrigger_dma_drv_state = DMA_DRV_INIT;
	return 0;

err_exit:
	loge( "dma open fail, err:[%d].\n", err);
	return err;
}

static int32_t dma_close(void)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;
	struct fast_tran_info *fast_info = NULL;
	struct normal_tran_info *normal_info = NULL;
	struct soundtrigger_pcm_info *fast_pcm_info = NULL;
	struct soundtrigger_pcm_info *normal_pcm_info = NULL;
	slimbus_device_type_t device_type = SLIMBUS_DEVICE_NUM;
	uint32_t track_type;
	int32_t err;

	if (!dma_drv_info) {
		err = -ENOENT;
		goto null_exit;
	}

	if (dma_drv_info->soundtrigger_dma_drv_state != DMA_DRV_INIT) {
		err = -EAGAIN;
		goto err_exit;
	}

	dma_drv_info->soundtrigger_dma_drv_state = DMA_DRV_NO_INIT;

	fast_info = &(dma_drv_info->st_fast_tran_info);
	normal_info = &(dma_drv_info->st_normal_tran_info);
	fast_pcm_info = &(dma_drv_info->st_pcm_info[SOUNDTRIGGER_PCM_FAST]);
	normal_pcm_info = &(dma_drv_info->st_pcm_info[SOUNDTRIGGER_PCM_NORMAL]);

	if (dma_drv_info->is_dma_enable) {
		if (fast_info->fast_read_complete_flag != FAST_TRAN_COMPLETE) {
			dma_stop(SOUNDTRIGGER_PCM_FAST);
		}
		dma_stop(SOUNDTRIGGER_PCM_NORMAL);
		dma_drv_info->is_dma_enable = 0;
	}

	if (dma_drv_info->is_slimbus_enable) {
		if (CODEC_HI6402 == dma_drv_info->type) {
			device_type = SLIMBUS_DEVICE_HI6402;
			track_type = SLIMBUS_TRACK_SOUND_TRIGGER;
		} else if (CODEC_HI6403 == dma_drv_info->type) {
			device_type = SLIMBUS_DEVICE_HI6403;
			track_type = SLIMBUS_TRACK_SOUND_TRIGGER;
		}
		else {
			loge("device type is err %d\n", dma_drv_info->type);
			err = -EINVAL;
			goto err_exit;
		}
		slimbus_track_deactivate(device_type, track_type, NULL);
		msleep(2);
		hi64xx_release_pll_resource(HI_FREQ_SCENE_FASTTRANS);
		logi("soundtrigger dma release pll resource and switch to soc\n");
		dma_drv_info->is_slimbus_enable = 0;
	}

	cancel_delayed_work(&dma_drv_info->soundtrigger_delay_dma_fast_left_work);
	cancel_delayed_work(&dma_drv_info->soundtrigger_delay_dma_fast_right_work);
	cancel_delayed_work(&dma_drv_info->soundtrigger_delay_dma_normal_left_work);
	cancel_delayed_work(&dma_drv_info->soundtrigger_delay_dma_normal_right_work);
	flush_workqueue(dma_drv_info->soundtrigger_delay_wq);

	fast_info->irq_count_left = 0;
	fast_info->irq_count_right = 0;
	fast_info->read_count_left = 0;
	fast_info->read_count_right = 0;
	fast_info->dma_tran_count = 0;
	fast_info->dma_tran_total_count = 0;

	normal_info->irq_count_left = 0;
	normal_info->irq_count_right = 0;
	normal_info->read_count_left = 0;
	normal_info->read_count_right = 0;
	normal_info->normal_tran_count = 0;

	/* memset fast and free normal channel pcm buffer */
	memset(fast_info->fast_buffer, 0x00, sizeof(uint16_t) * FAST_BUFFER_SIZE);

	Static_RingBuffer_DeInit();

	if (wake_lock_active(&dma_drv_info->st_wake_lock))
		wake_unlock(&dma_drv_info->st_wake_lock);/*lint !e455*/

	logi("soundtrigger_dma close aspclk:%d, ++\n", clk_get_enable_count(dma_drv_info->asp_subsys_clk));

	clk_disable_unprepare(dma_drv_info->asp_subsys_clk);

	logi("soundtrigger_dma close aspclk:%d, --\n", clk_get_enable_count(dma_drv_info->asp_subsys_clk));

	return 0;
err_exit:
	if (wake_lock_active(&dma_drv_info->st_wake_lock))
		wake_unlock(&dma_drv_info->st_wake_lock);/*lint !e455*/
null_exit:
	logw( "dma close fail, err:[%d].\n", err);
	return err;
}
/*lint -e715*/
static int32_t dma_fops_open(struct inode *finode, struct file *fd)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;

	if (!dma_drv_info)
		return -ENOENT;

	return 0;
}

static int32_t dma_fops_release(struct inode *finode, struct file *fd)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;

	if (!dma_drv_info)
		return -ENOENT;

	return 0;
}
/*lint +e715*/

static int32_t dma_get_max_read_len(enum codec_hifi_type codec_type, size_t *max_read_len, size_t count)
{
	if (CODEC_HI6402 == codec_type) {
		*max_read_len = (RINGBUFFER_SIZE > (HI6402_NORMAL_FRAME_LENGTH * VALID_BYTE_COUNT)) ?
			RINGBUFFER_SIZE : (HI6402_NORMAL_FRAME_LENGTH * VALID_BYTE_COUNT);
	} else if (CODEC_HI6403 == codec_type) {
		*max_read_len = (RINGBUFFER_SIZE > (HI6403_NORMAL_FRAME_LENGTH * VALID_BYTE_COUNT)) ?
			RINGBUFFER_SIZE : (HI6403_NORMAL_FRAME_LENGTH * VALID_BYTE_COUNT);
	}
	else {
		loge("codec type = %d invalid .\n", codec_type);
		return -EINVAL;
	}

	if (count < *max_read_len) {
		loge("user buffer too short, need %zu\n", *max_read_len);
		return -EINVAL;
	}

	return 0;
}

static ssize_t dma_fops_read(struct file *file, char __user *buffer, size_t count, loff_t *f_ops)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;
	struct fast_tran_info *fast_info = &(dma_drv_info->st_fast_tran_info);
	struct normal_tran_info *normal_info = &(dma_drv_info->st_normal_tran_info);
	int32_t rest_bytes = 0;
	int32_t ret = 0;
	size_t max_read_len = 0;
	uint16_t *pcm_buffer = NULL;
	static uint16_t static_buffer[RINGBUF_FRAME_LEN];

	if (dma_drv_info == NULL || fast_info == NULL || normal_info == NULL) {
		loge("pointer is NULL.\n");
		return -EINVAL;
	}

	if(!buffer) {
		loge("user input buffer is invalid\n");
		return -EINVAL;
	}

	if (dma_drv_info->soundtrigger_dma_drv_state == DMA_DRV_NO_INIT) {
		loge("soundtrigger dma aleady closed\n");
		return -EINVAL;
	}

	ret = dma_get_max_read_len(dma_drv_info->type, &max_read_len, count);
	if (ret < 0) {
		loge("get max read len error ret = %d \n", ret);
		return ret;
	}

	if (fast_info->fast_complete_flag == FAST_TRAN_NOT_COMPLETE) {
		//loge("fast channel not complete.\n");
		return -EAGAIN;
	}

	if((0 == dma_drv_info->dma_int_fast_flag) || (0 == dma_drv_info->dma_int_nomal_flag)){
		loge("normal config: %x, fast config: %x, PORT6_REG_0: %x, PORT6_REG_1: %x, PORT10_REG_0: %x, PORT10_REG_1: %x\n"
			, _dmac_reg_read(ASP_DMA_CX_CONFIG(DMA_NORMAL_LEFT_CH_NUM))
			, _dmac_reg_read(ASP_DMA_CX_CONFIG(DMA_FAST_LEFT_CH_NUM))
			, slimbus_register_read(dma_drv_info, HI3xxx_SLIMBUS_PORT6_REG_0)
			, slimbus_register_read(dma_drv_info, HI3xxx_SLIMBUS_PORT6_REG_1)
			, slimbus_register_read(dma_drv_info, HI3xxx_SLIMBUS_PORT10_REG_0)
			, slimbus_register_read(dma_drv_info, HI3xxx_SLIMBUS_PORT10_REG_1));
		dma_drv_info->dma_int_fast_flag = 1;
		dma_drv_info->dma_int_nomal_flag = 1;
	}

	/* first, hal not read fast data, we copy fast data to user */
	if (fast_info->fast_read_complete_flag == READ_NOT_COMPLETE) {
		pcm_fast_buffer_check(fast_info);
		rest_bytes = copy_to_user(buffer,fast_info->fast_buffer + fast_info->fast_start_addr, max_read_len);
		fast_info->fast_read_complete_flag = READ_COMPLETE;
		if (rest_bytes) {
			loge("fast chan copy to user space fail, rest bytes[%d].\n", rest_bytes);
			return -EINVAL;
		} else {
			return RINGBUFFER_SIZE;
		}
	} else {
	/* second, hal already read fast data, we copy normal data to user */
		if (!Static_RingBuffer_IsEmpty()) {
			if (dma_drv_info->st_normal_tran_info.normal_start_addr > RINGBUF_FRAME_LEN) {
				loge("normal_start_addr > RINGBUF_FRAME_LEN, failed.\n");
				return -EINVAL;
			}
			pcm_buffer = (uint16_t *)kzalloc(RINGBUF_FRAME_LEN * VALID_BYTE_COUNT, GFP_KERNEL);
			if (!pcm_buffer) {
				loge("pcm_buffer kzalloc failed.\n");
				return -EINVAL;
			}

			Static_RingBuffer_Get(pcm_buffer);

			if (normal_info->normal_first_frame_read_flag == READ_NOT_COMPLETE) {
				normal_info->normal_first_frame_read_flag = READ_COMPLETE;
				memcpy(static_buffer, pcm_buffer + dma_drv_info->st_normal_tran_info.normal_start_addr,
					(RINGBUF_FRAME_LEN - dma_drv_info->st_normal_tran_info.normal_start_addr) * VALID_BYTE_COUNT);/*lint !e647*/
				kzfree(pcm_buffer);
			} else {
				memcpy(static_buffer + (RINGBUF_FRAME_LEN - dma_drv_info->st_normal_tran_info.normal_start_addr),
					pcm_buffer, dma_drv_info->st_normal_tran_info.normal_start_addr * VALID_BYTE_COUNT);/*lint !e647*/
				rest_bytes = copy_to_user(buffer, static_buffer, RINGBUF_FRAME_LEN * VALID_BYTE_COUNT);
				memcpy(static_buffer, pcm_buffer + dma_drv_info->st_normal_tran_info.normal_start_addr,
					(RINGBUF_FRAME_LEN - dma_drv_info->st_normal_tran_info.normal_start_addr) * VALID_BYTE_COUNT);/*lint !e647*/
				kzfree(pcm_buffer);
				if (rest_bytes) {
					loge("normal chan copy to user space fail, rest bytes[%d].\n", rest_bytes);
					return -EINVAL;
				} else {
					return (RINGBUF_FRAME_LEN * VALID_BYTE_COUNT);
				}
			}
		}
	}
	return -EINVAL;
}/*lint !e715*/

static int32_t soundtrigger_dma_fops(uint32_t cmd, struct st_fast_status *fast_status)
{
	int32_t err = 0;

	if (!fast_status) {
		loge("pointer is NULL.\n");
		return -EINVAL;
	}
	if (!g_dma_drv_info) {
		loge("g_dma_drv_info get error\n");
		return -EINVAL;
	}
	if (cmd == SOUNDTRIGGER_CMD_DMA_CLOSE) {
		if (cancel_delayed_work_sync(&g_dma_drv_info->soundtrigger_delay_close_dma_timeout_work))
			logi("cancel timeout dma_close work success\n");
	}
	mutex_lock(&g_dma_drv_info->ioctl_mutex);
	switch(cmd) {
	case SOUNDTRIGGER_CMD_DMA_READY:
		err = dma_open(fast_status);
		logi("dma open, ret[%d]\n", err);
		break;
	case SOUNDTRIGGER_CMD_DMA_OPEN:
		err = dma_start(fast_status);
		logi("dma start, ret[%d]\n", err);
		break;
	case SOUNDTRIGGER_CMD_DMA_CLOSE:
		err = dma_close();
		logi("dma close, ret[%d]\n", err);
		break;
	default:
		loge("invalid value, ret[%d]\n", err);
		err = -ENOTTY;
		break;
	}
	mutex_unlock(&g_dma_drv_info->ioctl_mutex);
	return err;
}

static long dma_fops_ioctl(struct file *fd, uint32_t cmd, unsigned long arg)
{
	int32_t err = 0;
	struct soundtrigger_io_sync_param param;
	struct krn_param_io_buf krn_param;
	struct st_fast_status * fast_status;

	if (!(void __user *)arg) {
		loge("INPUT ERROR: arg is NULL\n");
		err = -EINVAL;
		return (long)err;
	}

	if (copy_from_user(&param, (void __user *)arg, sizeof(struct soundtrigger_io_sync_param))) {
		loge("copy_from_user fail.\n");
		err = -EIO;
		return (long)err;
	}

	err = get_input_param(param.para_size_in,
					 INT_TO_ADDR(param.para_in_l, param.para_in_h),
					 &krn_param.buf_size_in,
					 (void **)&krn_param.buf_in);
	if (err) {
		loge("INPUT ERROR: input param is not valid!\n");
		err = -EINVAL;
		return (long)err;
	}

	fast_status = (struct st_fast_status * )krn_param.buf_in;

	err = soundtrigger_dma_fops(cmd, fast_status);

	param_free((void **)&(krn_param.buf_in));

	return (long)err;
}/*lint !e715*/

static long dma_fops_ioctl32(struct file *fd, uint32_t cmd, unsigned long arg)
{
	void __user *user_arg = (void __user*)compat_ptr(arg);

	return dma_fops_ioctl(fd, cmd, (unsigned long)user_arg);
}

void hi64xx_soundtrigger_dma_close(void)
{
	struct st_fast_status fast_status = {0};
	(void)soundtrigger_dma_fops(SOUNDTRIGGER_CMD_DMA_CLOSE, &fast_status);
}

static void hi64xx_soundtrigger_close_soc_dma(void)
{
	if (!g_dma_drv_info)
		return;
	mutex_lock(&g_dma_drv_info->ioctl_mutex);
	(void)dma_close();
	mutex_unlock(&g_dma_drv_info->ioctl_mutex);
}

void dma_fast_left_workfunc(struct work_struct *work)
{
	uint32_t *left_buffer = NULL;
	uint16_t *temp_buffer = NULL;
	uint32_t start_addr = 0;
	static uint32_t om_fast_count = 0;
	struct soundtrigger_dma_drv_info *dma_drv_info = NULL;
	struct soundtrigger_pcm_info *pcm_info = NULL;
	struct fast_tran_info *fast_info = NULL;

	om_fast_count++;
	if (om_fast_count == 50) {
		om_fast_count = 0;
		logi("fast dma irq come.\n");
	}

	dma_drv_info = container_of(work, struct soundtrigger_dma_drv_info, soundtrigger_delay_dma_fast_left_work.work);
	if (!dma_drv_info) {
		loge("dma_drv_info get error\n");
		return;
	}

	if (dma_drv_info->soundtrigger_dma_drv_state == DMA_DRV_NO_INIT) {
		loge("drv is not open, work queue don't process\n");
		return;
	}

	pcm_info = &(dma_drv_info->st_pcm_info[SOUNDTRIGGER_PCM_FAST]);

	fast_info = &(dma_drv_info->st_fast_tran_info);
	if (fast_info->fast_complete_flag == FAST_TRAN_COMPLETE)
		return;

	if (pcm_info->buffer_size == 0) {
		loge( "pcm info buffer_size is 0.\n");
		return;
	}

	left_buffer = (uint32_t *)kzalloc(pcm_info->buffer_size, GFP_KERNEL);
	if (!left_buffer) {
		loge("left_buffer kzalloc failed\n");
		return;
	}

	if (CODEC_HI6402 == dma_drv_info->type) {
		temp_buffer = (uint16_t *)kzalloc(FAST_FRAME_LENGTH * VALID_BYTE_COUNT, GFP_KERNEL);
		if (!temp_buffer) {
			kzfree(left_buffer);
			loge("temp_buffer kzalloc failed.\n");
			return;
		}
	} else {
		temp_buffer = NULL;
	}

	if (dma_drv_info->st_fast_tran_info.read_count_left >= dma_drv_info->st_fast_tran_info.irq_count_left) {
		loge("read_count_left[%d] out of range irq_count_left[%d]error\n",
					dma_drv_info->st_fast_tran_info.read_count_left,
					dma_drv_info->st_fast_tran_info.irq_count_left);
		goto exit;
	}

	if (dma_drv_info->st_fast_tran_info.read_count_left >= FAST_CHANNEL_TIMEOUT_READ_COUNT) {
		loge( "dma fast channel timeout.\n");
		dma_stop(SOUNDTRIGGER_PCM_FAST);
		dma_stop(SOUNDTRIGGER_PCM_NORMAL);
		goto exit;
	} else {
		memcpy(left_buffer, (uint32_t *)pcm_info->buffer[0][dma_drv_info->st_fast_tran_info.read_count_left % PCM_SWAP_BUFFER_NUM], pcm_info->buffer_size);
		dma_drv_info->st_fast_tran_info.read_count_left++;
		if (CODEC_HI6402 == dma_drv_info->type) {
			pcm_valid_data_get(left_buffer, temp_buffer, FAST_FRAME_LENGTH);

			if (dma_drv_info->st_fast_tran_info.fast_frame_find_flag == FRAME_NOT_FIND) {
				if (pcm_start_addr_find(SOUNDTRIGGER_PCM_FAST, temp_buffer, FAST_FRAME_LENGTH, &start_addr)){
					dma_drv_info->st_fast_tran_info.fast_start_addr = start_addr;
					memcpy(dma_drv_info->st_fast_tran_info.fast_buffer + FAST_FRAME_LENGTH * dma_drv_info->st_fast_tran_info.dma_tran_count,/*lint !e679*/
							temp_buffer, FAST_FRAME_LENGTH * VALID_BYTE_COUNT);/*lint !e668*/
					dma_drv_info->st_fast_tran_info.fast_frame_find_flag = FRAME_FIND;
					dma_drv_info->st_fast_tran_info.fast_start_addr = start_addr;
					dma_drv_info->st_fast_tran_info.dma_tran_count++;
				}
			} else {
				memcpy(dma_drv_info->st_fast_tran_info.fast_buffer + FAST_FRAME_LENGTH * dma_drv_info->st_fast_tran_info.dma_tran_count,/*lint !e679*/
						temp_buffer, FAST_FRAME_LENGTH * VALID_BYTE_COUNT);/*lint !e668*/
				dma_drv_info->st_fast_tran_info.dma_tran_count++;
			}
		} else {
			if (dma_drv_info->st_fast_tran_info.fast_frame_find_flag == FRAME_NOT_FIND) {
				if (pcm_start_addr_find(SOUNDTRIGGER_PCM_FAST, (uint16_t *)left_buffer, FAST_FRAME_LENGTH, &start_addr)){
					dma_drv_info->st_fast_tran_info.fast_start_addr = start_addr;
					memcpy(dma_drv_info->st_fast_tran_info.fast_buffer + FAST_FRAME_LENGTH * dma_drv_info->st_fast_tran_info.dma_tran_count * 2,/*lint !e679*/
							left_buffer, FAST_FRAME_LENGTH * VALID_BYTE_COUNT_FAST);
					dma_drv_info->st_fast_tran_info.fast_frame_find_flag = FRAME_FIND;
					dma_drv_info->st_fast_tran_info.fast_start_addr = start_addr;
					dma_drv_info->st_fast_tran_info.dma_tran_count++;
				}
			} else {
				memcpy(dma_drv_info->st_fast_tran_info.fast_buffer + FAST_FRAME_LENGTH * dma_drv_info->st_fast_tran_info.dma_tran_count * 2,/*lint !e679*/
						left_buffer, FAST_FRAME_LENGTH * VALID_BYTE_COUNT_FAST);
				dma_drv_info->st_fast_tran_info.dma_tran_count++;
			}
		}

		if (fast_info->dma_tran_count == fast_info->dma_tran_total_count) {
			dma_stop(SOUNDTRIGGER_PCM_FAST);
			fast_info->fast_complete_flag = FAST_TRAN_COMPLETE;
		}
	}

exit:
	if (temp_buffer != NULL) {
		kzfree(temp_buffer);
	}
	kzfree(left_buffer);
}

void dma_fast_right_workfunc(struct work_struct *work)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = container_of(work, struct soundtrigger_dma_drv_info,
		soundtrigger_delay_dma_fast_right_work.work);

	if (!dma_drv_info) {
		loge("dma_drv_info NULL.\n");
		return;
	}

	if (dma_drv_info->soundtrigger_dma_drv_state == DMA_DRV_NO_INIT) {
		loge("drv is not open, work queue don't process\n");
		return;
	}

	if (dma_drv_info->st_fast_tran_info.read_count_right >= dma_drv_info->st_fast_tran_info.irq_count_right)
		return;
	else
		dma_drv_info->st_fast_tran_info.read_count_right++;

	/* we do nothing here */
}

static uint16_t *alloc_temp_buffer(enum codec_hifi_type codec_type)
{
	uint16_t *temp_buf = NULL;

	if (CODEC_HI6402 == codec_type) {
		temp_buf = (uint16_t *)kzalloc(HI6402_NORMAL_FRAME_LENGTH * VALID_BYTE_COUNT, GFP_KERNEL);
	} else if (CODEC_HI6403 == codec_type) {
		temp_buf = (uint16_t *)kzalloc(HI6403_NORMAL_FRAME_LENGTH * VALID_BYTE_COUNT, GFP_KERNEL);
	}
	else {
		temp_buf = NULL;
	}

	return temp_buf;
}

void dma_normal_left_workfunc(struct work_struct *work)
{
	uint32_t *left_buffer = NULL;
	uint16_t *temp_buf = NULL;
	uint16_t *ring_buf = NULL;
	uint32_t start_addr = 0;
	static uint32_t om_normal_count = 0;
	struct soundtrigger_pcm_info *pcm_info = NULL;
	struct soundtrigger_dma_drv_info *dma_drv_info = NULL;

	om_normal_count++;
	if (om_normal_count == 50) {
		om_normal_count = 0;
		logi("normal dma irq come.\n");
	}

	dma_drv_info = container_of(work, struct soundtrigger_dma_drv_info, soundtrigger_delay_dma_normal_left_work.work);
	if (!dma_drv_info) {
		loge("dma_drv_info NULL\n");
		return;
	}

	if (dma_drv_info->soundtrigger_dma_drv_state == DMA_DRV_NO_INIT) {
		loge("drv is not open, work queue don't process\n");
		return;
	}

	pcm_info = &(dma_drv_info->st_pcm_info[SOUNDTRIGGER_PCM_NORMAL]);

	if (pcm_info->buffer_size == 0) {
		loge( "pcm info buffer_size is 0.\n");
		return;
	}

	left_buffer = (uint32_t *)kzalloc(pcm_info->buffer_size, GFP_KERNEL);
	if (!left_buffer) {
		loge("normal left_buffer kzalloc failed\n");
		return;
	}

	temp_buf = alloc_temp_buffer(dma_drv_info->type);
	if (temp_buf == NULL) {
		loge("normal temp_buf kzalloc failed\n");
		kzfree(left_buffer);
		return;
	}

	ring_buf = (uint16_t *)kzalloc(RINGBUF_FRAME_LEN * VALID_BYTE_COUNT, GFP_KERNEL);
	if (ring_buf == NULL) {
		loge("normal ring_buf kzalloc failed\n");
		kzfree(left_buffer);
		kzfree(temp_buf);
		return;
	}

	if (dma_drv_info->st_normal_tran_info.read_count_left >= dma_drv_info->st_normal_tran_info.irq_count_left) {
		goto exit;
	} else {
		memcpy(left_buffer, (uint32_t *)pcm_info->buffer[0][dma_drv_info->st_normal_tran_info.read_count_left % PCM_SWAP_BUFFER_NUM], pcm_info->buffer_size);
		dma_drv_info->st_normal_tran_info.read_count_left++;
		pcm_valid_data_get(left_buffer, temp_buf, pcm_info->buffer_size / BYTE_COUNT);
		if (CODEC_HI6402 == dma_drv_info->type) {
			if (hi6402_normal_tran_rate == HI6402_4SMARTPA_NORMAL_TRAN_RATE)
				pcm_48K_mono_to_16K_mono(temp_buf, ring_buf, RINGBUF_FRAME_LEN);
			else
				memcpy(ring_buf, temp_buf, RINGBUF_FRAME_LEN * VALID_BYTE_COUNT);
		} else {
			pcm_48K_mono_to_16K_mono(temp_buf, ring_buf, RINGBUF_FRAME_LEN);
		}
		if (dma_drv_info->st_normal_tran_info.normal_frame_find_flag == FRAME_NOT_FIND) {
			if (pcm_start_addr_find(SOUNDTRIGGER_PCM_NORMAL, ring_buf, RINGBUF_FRAME_LEN, &start_addr)) {
				Static_RingBuffer_Put(ring_buf);
				dma_drv_info->st_normal_tran_info.normal_frame_find_flag = FRAME_FIND;
				dma_drv_info->st_normal_tran_info.normal_start_addr = start_addr;
				dma_drv_info->st_normal_tran_info.normal_tran_count++;
			}
		} else {
			Static_RingBuffer_Put(ring_buf);
			dma_drv_info->st_normal_tran_info.normal_tran_count++;
		}
	}

	if ((dma_drv_info->st_normal_tran_info.read_count_left - dma_drv_info->st_normal_tran_info.normal_tran_count)
		>= NORMAL_CHANNEL_TIMEOUT_READ_COUNT) {
		loge( "dma normal channel timeout.\n");
		dma_stop(SOUNDTRIGGER_PCM_FAST);
		dma_stop(SOUNDTRIGGER_PCM_NORMAL);
		goto exit;
	}

exit:
	kzfree(ring_buf);
	kzfree(temp_buf);
	kzfree(left_buffer);

}

void dma_normal_right_workfunc(struct work_struct *work)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = container_of(work, struct soundtrigger_dma_drv_info,
		soundtrigger_delay_dma_normal_right_work.work);
	if (!dma_drv_info ) {
		loge("dma_drv_info NULL.\n");
		return;
	}

	if (dma_drv_info->soundtrigger_dma_drv_state == DMA_DRV_NO_INIT) {
		loge("drv is not open, work queue don't process\n");
		return;
	}

	if (dma_drv_info->st_normal_tran_info.read_count_right >= dma_drv_info->st_normal_tran_info.irq_count_right)
		return;
	else
		dma_drv_info->st_normal_tran_info.read_count_right++;

	/* we do nothing here */
}

static int32_t soundtrigger_dmac_irq_handler(unsigned short int_type, unsigned long para, unsigned int dma_channel)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;
	if (!dma_drv_info)
		return IRQ_FINISH;

	switch (int_type) {
		case ASP_DMA_INT_TYPE_ERR1:
			loge("dma interrupt setting error[dmac channel %d]\n", dma_channel);
			return IRQ_ERR;

		case ASP_DMA_INT_TYPE_ERR2:
			loge("dma interrupt transmission error[dmac channel %d]\n", dma_channel);
			return IRQ_ERR;

		case ASP_DMA_INT_TYPE_ERR3:
			loge("dma interrupt lli error[dmac channel %d]\n", dma_channel);
			return IRQ_ERR;

		case ASP_DMA_INT_TYPE_TC1:
			loge("dma interrupt transmission finish[dmac channel %d]\n", dma_channel);
			return IRQ_FINISH;

		/*dma lli node transit finish interrupt*/
		case ASP_DMA_INT_TYPE_TC2:
			break;

		default:
			loge("dma interrupt error int_type[%d]\n", int_type);
			return IRQ_ERR;
	}

	switch(dma_channel) {
		case DMA_FAST_LEFT_CH_NUM:
			(dma_drv_info->st_fast_tran_info.irq_count_left)++;
			dma_drv_info->dma_int_fast_flag = 1;
			if (!queue_delayed_work(dma_drv_info->soundtrigger_delay_wq,
				&dma_drv_info->soundtrigger_delay_dma_fast_left_work, msecs_to_jiffies(0))) {
				loge("fast left lost msg\n");
			}
			return IRQ_FINISH;

		case DMA_FAST_RIGHT_CH_NUM:
			(dma_drv_info->st_fast_tran_info.irq_count_right)++;
			queue_delayed_work(dma_drv_info->soundtrigger_delay_wq,
				&dma_drv_info->soundtrigger_delay_dma_fast_right_work, msecs_to_jiffies(0));
			return IRQ_FINISH;

		case DMA_NORMAL_LEFT_CH_NUM:
			(dma_drv_info->st_normal_tran_info.irq_count_left)++;
			dma_drv_info->dma_int_nomal_flag= 1;
			queue_delayed_work(dma_drv_info->soundtrigger_delay_wq,
				&dma_drv_info->soundtrigger_delay_dma_normal_left_work, msecs_to_jiffies(0));
			return IRQ_FINISH;

		case DMA_NORMAL_RIGHT_CH_NUM:
			(dma_drv_info->st_normal_tran_info.irq_count_right)++;
			queue_delayed_work(dma_drv_info->soundtrigger_delay_wq,
				&dma_drv_info->soundtrigger_delay_dma_normal_right_work, msecs_to_jiffies(0));
			return IRQ_FINISH;

		default:
			loge("dma interrupt error dma_channel[%d]\n", dma_channel);
			return IRQ_ERR;
	}
}/*lint !e715*/

static void close_dma_timeout_workfunc(struct work_struct *work)
{
	UNUSED_PARAMETER(work);

	logi("timeout close dma\n");
	hi64xx_soundtrigger_close_soc_dma();
	hi64xx_soundtrigger_close_codec_dma();
}

static const struct file_operations soundtrigger_dma_drv_fops = {
	.owner			= THIS_MODULE,
	.open			= dma_fops_open,
	.release		= dma_fops_release,
	.read			= dma_fops_read,
	.unlocked_ioctl = dma_fops_ioctl,
	.compat_ioctl	= dma_fops_ioctl32,
};

static struct miscdevice soundtrigger_dma_drv_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= DRV_NAME,
	.fops	= &soundtrigger_dma_drv_fops,
};

int32_t hi64xx_soundtrigger_init(enum codec_hifi_type type)
{
	if (!g_dma_drv_info) {
		loge("device not init, default is hi6402.\n");
		return -ENOENT;
	}

	g_dma_drv_info->type = type;
	logi("codec hifi type:%d.\n", type);
	return 0;
}
/*lint -e429*/
static int32_t soundtrigger_dma_drv_probe (struct platform_device *pdev)
{
	struct device					 *dev			= &pdev->dev;
	struct soundtrigger_dma_drv_info *dma_drv_info	 = NULL;
	int32_t  err;
	g_dma_drv_info = NULL;

	logi("dma probe.\n");

	if (!dev) {
		loge("dev is null\n");
		return -EINVAL;
	}

	if (of_property_read_bool(dev->of_node, "hi6402-dma-cfg-4smartpa")) {
		logi("cust soundtrigger dma config for 4smartpa\n");
		hi6402_normal_frame_length = HI6402_4SMARTPA_NORMAL_FRAME_LENGTH;
		hi6402_normal_tran_rate = HI6402_4SMARTPA_NORMAL_TRAN_RATE;
		hi640X_soundtrigger_dma_cfg = hi640X_soundtrigger_dma_cfg_4smartpa;
		hi640X_pcm_cfg[CODEC_HI6402][SOUNDTRIGGER_PCM_NORMAL].rate = HI6402_4SMARTPA_NORMAL_TRAN_RATE;
		hi640X_pcm_cfg[CODEC_HI6402][SOUNDTRIGGER_PCM_NORMAL].frame_len = HI6402_4SMARTPA_NORMAL_FRAME_LENGTH;
	}

	err = misc_register(&soundtrigger_dma_drv_device);
	if (err) {
		loge("misc registe failed.\n");
		return -EBUSY;
	}

	dma_drv_info = devm_kzalloc(dev, sizeof(struct soundtrigger_dma_drv_info), GFP_KERNEL);
	if (!dma_drv_info) {
		err = -ENOMEM;
		loge("malloc failed.\n");
		goto err_out1;
	}

	memset(dma_drv_info,0x00,sizeof(struct soundtrigger_dma_drv_info));
	dma_drv_info->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!dma_drv_info->res) {
		err = -ENOENT;
		loge("get res error!\n");
		goto err_out1;
	}

	dma_drv_info->reg_base_addr = devm_ioremap(dev, dma_drv_info->res->start,
						resource_size(dma_drv_info->res));
	if (!dma_drv_info->reg_base_addr) {
		err = -ENOMEM;
		loge("remap asp dma addr failed.\n");
		goto err_out1;
	}

	dma_drv_info->v_slimbus_base_reg_addr = devm_ioremap(dev, HI3xxx_SLIMBUS_BASE_REG, HI3xxx_SLIMBUS_REG_SIZE);
	if (!dma_drv_info->v_slimbus_base_reg_addr)
	{
		err = -ENOENT;
		loge("slimbus ioremap error!\n");
		goto err_out1;
	}

	dma_drv_info->asp_ip = devm_regulator_get(dev, "asp-dmac");
	if (IS_ERR(dma_drv_info->asp_ip)) {
		err = -ENOENT;
		loge("regulator asp dmac failed.\n");
		goto err_out1;
	}

	dma_drv_info->asp_subsys_clk = devm_clk_get(dev, "clk_asp_subsys");
	if ( IS_ERR(dma_drv_info->asp_subsys_clk)) {
		err= PTR_ERR(dma_drv_info->asp_subsys_clk);
		loge( "asp subsys clk fail.\n");
		goto err_out1;
	}

	dma_drv_info->hwlock = hwspin_lock_request_specific(SOUNDTRIGGER_HWLOCK_ID);
	if (!dma_drv_info->hwlock) {
		err = -ENOENT;
		loge("get hwspin error!\n");
		goto err_out1;
	}

	dma_drv_info->soundtrigger_delay_wq = create_singlethread_workqueue("soundtrigger_delay_wq");
	if (!dma_drv_info->soundtrigger_delay_wq) {
		err = -ENOMEM;
		loge("create workqueue failed.\n");
		goto err_out2;
	}

	dma_drv_info->soundtrigger_delay_close_dma_wq =
			create_singlethread_workqueue("soundtrigger_delay_close_dma_wq");
	if (!dma_drv_info->soundtrigger_delay_close_dma_wq) {
		err = -ENOMEM;
		loge("create workqueue failed.\n");
		goto err_out3;
	}

	INIT_DELAYED_WORK(&dma_drv_info->soundtrigger_delay_dma_fast_left_work, dma_fast_left_workfunc);
	INIT_DELAYED_WORK(&dma_drv_info->soundtrigger_delay_dma_fast_right_work, dma_fast_right_workfunc);
	INIT_DELAYED_WORK(&dma_drv_info->soundtrigger_delay_dma_normal_left_work, dma_normal_left_workfunc);
	INIT_DELAYED_WORK(&dma_drv_info->soundtrigger_delay_dma_normal_right_work, dma_normal_right_workfunc);
	INIT_DELAYED_WORK(&dma_drv_info->soundtrigger_delay_close_dma_timeout_work, close_dma_timeout_workfunc);

	dma_drv_info->dev = dev;
	dma_drv_info->soundtrigger_dma_drv_state = DMA_DRV_NO_INIT;
	dma_drv_info->dma_alloc_flag = 0;
	dma_drv_info->is_dma_enable = 0;
	dma_drv_info->is_slimbus_enable = 0;
	wake_lock_init(&dma_drv_info->st_wake_lock, WAKE_LOCK_SUSPEND, "hisi-64xx-soundtrigger");
	platform_set_drvdata(pdev, dma_drv_info);
	mutex_init(&dma_drv_info->ioctl_mutex);
	spin_lock_init(&dma_drv_info->lock);

	g_dma_drv_info = dma_drv_info;

	logi("dma probe end.\n");

	/*if socdsp pcm init fail,codecdsp function is ok, so don't release codecdsp resource */
	soundtrigger_socdsp_pcm_init();
	soundtrigger_mailbox_init();

	return 0;

err_out3:
	if(g_dma_drv_info->soundtrigger_delay_wq) {
		flush_workqueue(g_dma_drv_info->soundtrigger_delay_wq);
		destroy_workqueue(g_dma_drv_info->soundtrigger_delay_wq);
	}
err_out2:
	if (hwspin_lock_free(dma_drv_info->hwlock))
		loge("free dma_drv_info->hwlock fail\n");

err_out1:
	misc_deregister(&soundtrigger_dma_drv_device);
	loge( "dma driver init fail.\n");
	return err;
}
/*lint +e429*/
static int32_t soundtrigger_dma_drv_remove(struct platform_device *pdev)
{
	struct soundtrigger_dma_drv_info *dma_drv_info = g_dma_drv_info;

	UNUSED_PARAMETER(pdev);

	if (!dma_drv_info) {
		loge("dma_drv_info NULL.\n");
		return -ENOENT;
	}

	if (hwspin_lock_free(dma_drv_info->hwlock))
		loge("free dma_drv_info->hwlock fail\n");

	dma_config_clear(dma_drv_info);

	dma_drv_info->dma_alloc_flag = 0;

	wake_lock_destroy(&g_dma_drv_info->st_wake_lock);
	mutex_destroy(&g_dma_drv_info->ioctl_mutex);

	misc_deregister(&soundtrigger_dma_drv_device);

	if(g_dma_drv_info->soundtrigger_delay_wq) {
		cancel_delayed_work(&g_dma_drv_info->soundtrigger_delay_dma_fast_left_work);
		cancel_delayed_work(&g_dma_drv_info->soundtrigger_delay_dma_fast_right_work);
		cancel_delayed_work(&g_dma_drv_info->soundtrigger_delay_dma_normal_left_work);
		cancel_delayed_work(&g_dma_drv_info->soundtrigger_delay_dma_normal_right_work);
		flush_workqueue(g_dma_drv_info->soundtrigger_delay_wq);
		destroy_workqueue(g_dma_drv_info->soundtrigger_delay_wq);
	}

	if(g_dma_drv_info->soundtrigger_delay_close_dma_wq) {
		cancel_delayed_work(&g_dma_drv_info->soundtrigger_delay_close_dma_timeout_work);
		flush_workqueue(g_dma_drv_info->soundtrigger_delay_close_dma_wq);
		destroy_workqueue(g_dma_drv_info->soundtrigger_delay_close_dma_wq);
	}

	soundtrigger_socdsp_pcm_deinit();
	soundtrigger_mailbox_deinit();

	logi( "dma remove.\n");
	return 0;
}

static const struct of_device_id soundtrigger_dma_match_table[] = {
	{ .compatible = COMP_SOUNDTRIGGER_DMA_DRV_NAME, },
	{},
};

MODULE_DEVICE_TABLE(of, soundtrigger_dma_match_table);

static struct platform_driver soundtrigger_dma_driver = {
	.driver 	= {
		.name	= "soundtrigger dma drviver",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(soundtrigger_dma_match_table),
	},
	.probe		= soundtrigger_dma_drv_probe,
	.remove 	= soundtrigger_dma_drv_remove,
};

static int32_t __init soundtrigger_dma_drv_init(void)
{
	logi( "soundtrigger dma drv init.\n");
	return platform_driver_register(&soundtrigger_dma_driver);
}
module_init(soundtrigger_dma_drv_init);

static void __exit soundtrigger_dma_drv_exit(void)
{
	platform_driver_unregister(&soundtrigger_dma_driver);
}
module_exit(soundtrigger_dma_drv_exit);

MODULE_AUTHOR("Yue Yu <yuyue321@hisilicon.com>");
MODULE_DESCRIPTION("Hisilicon (R) Soundtrigger DMA Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_VERSION(HISILICON_SOUNDTRIGGER_DMA_DRIVER_VERSION);

