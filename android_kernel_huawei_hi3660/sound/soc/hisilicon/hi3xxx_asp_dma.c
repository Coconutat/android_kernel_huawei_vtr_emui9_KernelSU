/*
 * hi3xxx_asp_dmac.c -- ALSA SoC HI3630 ASP DMA driver
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define LOG_TAG "hi3xxx_asp_dmac"

#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/compiler.h>
#include <linux/regulator/consumer.h>
#include <linux/hwspinlock.h>
#include <linux/pm_runtime.h>
#include <sound/core.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>

#include "hi3630_log.h"
#include "hi3630_asp_common.h"
#include "hi3xxx_asp_dma.h"
#include "asp_dma.h"
#include "hisi_pcm_hifi.h"
#include "hifi_lpp.h"
#include "huawei_platform/log/imonitor.h"
#include "huawei_platform/log/imonitor_keys.h"

/*lint -e749 -e64 -e647 -e429*/

#define PCM_PORTS_MAX           (4) /* max ports num for all stream type - same as soc hifi */
#define PCM_PORTS_NUM           (2) /* actual ports num supported by asp dma driver */
#define DMA_CHANNEL_MAX         (16)
#define CODEC_NAME_HI6402       "hi6402-codec"
#define CODEC_NAME_HI6403ES     "hi6403es-codec"
#define CODEC_NAME_HI6403       "hi6403-codec"

#undef CONFIG_PM_RUNTIME

#define MONITOR_DMA_EXCEPTION
#ifdef MONITOR_DMA_EXCEPTION
	struct workqueue_struct *dma_exception_workqueue = NULL;
	struct delayed_work dma_exception_work;
	static unsigned long dmaPeriodErrTimeNs = 0;
	static unsigned int dmaPeriodErrCount = 0;
	static unsigned int dmaPeriodIntervalMs = 0;
	static bool triggerSendEvent = false;
#endif

#define HI3XXX_PCM_DMA_BUF_DOWN_SIZE         (PCM_DMA_BUF_PLAYBACK_LEN - 1024) // 75k for low latency playback
#define HI3XXX_PCM_DMA_LLI_DOWN_SIZE         (1024) // 1k
#define HI3XXX_PCM_DMA_BUF_DOWN_BASE_ADDR    (PCM_DMA_BUF_2_PLAYBACK_BASE)
#define HI3XXX_PCM_DMA_LLI_DOWN_BASE_ADDR    (HI3XXX_PCM_DMA_BUF_DOWN_BASE_ADDR + HI3XXX_PCM_DMA_BUF_DOWN_SIZE)

#define HI3XXX_PCM_DMA_BUF_UP_SIZE           (PCM_DMA_BUF_PLAYBACK_LEN - 1024) // 75k for low latency capture
#define HI3XXX_PCM_DMA_LLI_UP_SIZE           (1024) // 1k
#define HI3XXX_PCM_DMA_BUF_UP_BASE_ADDR      (PCM_DMA_BUF_2_CAPTURE_BASE)
#define HI3XXX_PCM_DMA_LLI_UP_BASE_ADDR      (HI3XXX_PCM_DMA_BUF_UP_BASE_ADDR + HI3XXX_PCM_DMA_BUF_UP_SIZE)

static PCM_DMA_BUF_CONFIG  hi3xxx_pcm_dma_buf_array[PCM_STREAM_MAX] =
{
	{HI3XXX_PCM_DMA_BUF_DOWN_BASE_ADDR, HI3XXX_PCM_DMA_BUF_DOWN_SIZE},
	{HI3XXX_PCM_DMA_BUF_UP_BASE_ADDR, HI3XXX_PCM_DMA_BUF_UP_SIZE},
};

static PCM_DMA_BUF_CONFIG  hi3xxx_pcm_dma_lli_array[PCM_STREAM_MAX] =
{
	{HI3XXX_PCM_DMA_LLI_DOWN_BASE_ADDR, HI3XXX_PCM_DMA_LLI_DOWN_SIZE},
	{HI3XXX_PCM_DMA_LLI_UP_BASE_ADDR, HI3XXX_PCM_DMA_LLI_UP_SIZE},
};

enum hi3xxx_asp_dmac_status {
	STATUS_DMAC_STOP = 0,
	STATUS_DMAC_RUNNING,
};

enum {
	PCM_PORTS_VOICE_UL,
	PCM_PORTS_VOICE_DL,
	PCM_PORTS_AUDIO_UL,
	PCM_PORTS_AUDIO_DL,
	PCM_PORTS_EXMODEM_UL,
	PCM_PORTS_EXMODEM_DL,
	PCM_PORTS_VOICE_SMARTPA_UL,
	PCM_PORTS_AUDIO_SMARTPA_UL,
	PCM_PORTS_VIRTUAL,
	PCM_PORTS_BUTT,
};

unsigned int pcm_port_tbl_6402[PCM_PORTS_BUTT][PCM_PORTS_MAX] =
{
	{2,3,10},       //PCM_DEVICE_VOICE_UL_PORT_NUM
	{8,9},          //PCM_DEVICE_VOICE_DL_PORT_NUM
	{2,3,10},       //PCM_DEVICE_AUDIO_UL_PORT_NUM
	{0,1},          //PCM_DEVICE_AUDIO_DL_PORT_NUM
	{2,3,10},       //PCM_EXMODEM_UL_PORT_NUM
	{8,9},          //PCM_EXMODEM_DL_PORT_NUM
	{14},           //PCM_VOICE_SMARTPA_UL_PORT_NUM
	{15},           //PCM_AUDIO_SMARTPA_UL_PORT_NUM
};

unsigned int pcm_port_tbl_6403es[PCM_PORTS_BUTT][PCM_PORTS_MAX] =
{
	{10, 11, 12, 13},      //PCM_DEVICE_VOICE_UL_PORT_NUM
	{8, 9},                //PCM_DEVICE_VOICE_DL_PORT_NUM
	{2, 3, 10, 11},       //PCM_DEVICE_AUDIO_UL_PORT_NUM
	{0, 1},                //PCM_DEVICE_AUDIO_DL_PORT_NUM
	{10, 11, 12, 13},     //PCM_EXMODEM_UL_PORT_NUM
	{8, 9},               //PCM_EXMODEM_DL_PORT_NUM
	{14, 15},                 //PCM_VOICE_SMARTPA_UL_PORT_NUM
	{4, 7},                 //PCM_AUDIO_SMARTPA_UL_PORT_NUM
};

unsigned int pcm_port_tbl_6403[PCM_PORTS_BUTT][PCM_PORTS_MAX] =
{
	{10, 11, 12, 13},      //PCM_DEVICE_VOICE_UL_PORT_NUM
	{8, 9},                //PCM_DEVICE_VOICE_DL_PORT_NUM
	{2, 3, 12, 13},       //PCM_DEVICE_AUDIO_UL_PORT_NUM
	{0, 1},                //PCM_DEVICE_AUDIO_DL_PORT_NUM
	{10, 11, 12, 13},     //PCM_EXMODEM_UL_PORT_NUM
	{8, 9},               //PCM_EXMODEM_DL_PORT_NUM
	{14, 15},               //PCM_VOICE_SMARTPA_UL_PORT_NUM
	{4, 7},                 //PCM_AUDIO_SMARTPA_UL_PORT_NUM
};

struct pcm_ports {
	unsigned int ports_tbl_idx; /* device ports table index */
	unsigned int ports_cnt; /* ports number */
};

struct dma_config {
	unsigned int port;        /* port addr */
	unsigned int config;      /* dma config */
	unsigned int channel;     /* dma channel */
};

struct dma_config dma_cfg_slimbus_6402[DMA_CHANNEL_MAX] = {
	{0xe8051000,0x83322007,5},
	{0xe8051040,0x83322017,6},
	{0xe8051080,0x43322027,7},
	{0xe80510c0,0x43322037,8},
	{0xe8051100,0x83322047,DMA_CHANNEL_MAX}, /* no use*/
	{0xe8051140,0x83322057,DMA_CHANNEL_MAX}, /* no use*/
	{0xe8051180,0x43322067,DMA_CHANNEL_MAX}, /* no use*/
	{0xe80511c0,0x43322077,DMA_CHANNEL_MAX}, /* no use*/
	{0xe8051200,0x83322087,1},
	{0xe8051240,0x83322097,2},
	{0xe8051280,0x433220a7,3},
	{0xe80512c0,0x433220b7,4},
	{0xe8051300,0x833220c7,9},
	{0xe8051340,0x833220d7,10},
	{0xe8051380,0x433220e7,9},
	{0xe80513c0,0x433220f7,11},
};

struct dma_config dma_cfg_slimbus_6403[DMA_CHANNEL_MAX] = {
	{0xe8051000,0x83322007,5}, /* dport0, d1 */
	{0xe8051040,0x83322017,6}, /* dport1, d2 */
	{0xe8051080,0x43322027,7}, /* dport2, u1 */
	{0xe80510c0,0x43322037,8}, /* dport3, u2 */
	{0xe8051100,0x43322047,12},  /* dport4, d3 */
	{0xe8051140,0x83322057,DMA_CHANNEL_MAX},  /* dport5, d4 */
	{0xe8051180,0x43322067,DMA_CHANNEL_MAX},  /* dport6, u9 */
	{0xe80511c0,0x43322077,15},  /* dport7, u10 */
	{0xe8051200,0x83322087,1}, /* dport8, d5 */
	{0xe8051240,0x83322097,2}, /* dport9, d6 */
	{0xe8051280,0x433220a7,3}, /* dport10, u5 */
	{0xe80512c0,0x433220b7,4}, /* dport11, u6 */
	{0xe8051300,0x433220c7,9}, /* dport12, u3 */
	{0xe8051340,0x433220d7,11}, /* dport13, u4 */
	{0xe8051380,0x433220e7,0}, /* dport14, u7, voice pa */
	{0xe80513c0,0x433220f7,13}, /* dport15, u8, audio pa */
};

struct hi3xxx_asp_dmac_data {
	struct resource		*res;
	void __iomem		*reg_base_addr;
	struct regulator	*asp_ip;
	struct device		*dev;
};

struct hi3xxx_asp_dmac_runtime_data {
	spinlock_t lock;
	struct hi3xxx_asp_dmac_data *pdata;
	struct mutex mutex;
	enum hi3xxx_asp_dmac_status status;
	unsigned int sampleRate;
	unsigned int dma_addr;
	unsigned int period_size;
	unsigned int period_cur;
	unsigned int dma_buf_next[PCM_PORTS_NUM];
	unsigned int irq_cnt;
	unsigned long preIrqTimeNs;
	unsigned long prePeriodTimeNs;
	struct pcm_ports ports;
	struct dma_config dma_cfg[PCM_PORTS_NUM];
	struct dma_lli_cfg *pdma_lli_cfg[PCM_PORTS_NUM];
	unsigned int lli_dma_addr[PCM_PORTS_NUM];
	unsigned int (*pcm_port_tbl)[PCM_PORTS_MAX];
	struct dma_config *dma_cfg_tbl;
};

static const struct of_device_id hi3xxx_asp_dmac_of_match[] = {
	{
		.compatible = "hisilicon,hi3xxx-pcm-asp-dma",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, hi3xxx_asp_dmac_of_match);

static const struct snd_pcm_hardware hi3xxx_asp_dmac_hardware = {
	.info			= SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_INTERLEAVED,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE |
				  SNDRV_PCM_FMTBIT_S16_BE |
				  SNDRV_PCM_FMTBIT_S24_LE |
				  SNDRV_PCM_FMTBIT_S24_BE,
	.period_bytes_min	= 32,
	.period_bytes_max	= 16 * 1024,
	.periods_min		= 2,
	.periods_max		= 32,
	.buffer_bytes_max	= 128 * 1024,
};

/*
*dma operations
*/

/*dma config lli node*/
static void asp_dma_set_lli_node(struct snd_pcm_substream *substream, unsigned int port_index, unsigned int lli_index)
{
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;
	unsigned int ports_cnt = prtd->ports.ports_cnt;
	unsigned int dma_size = prtd->period_size/ports_cnt;
	unsigned int dma_buf_num = substream->runtime->periods * ports_cnt;

	pr_debug("[%s:%d] set dma buf[%d/%d] dma_size: %d\n", __FUNCTION__, __LINE__, prtd->dma_buf_next[port_index], dma_buf_num, dma_size);

	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		prtd->pdma_lli_cfg[port_index][lli_index].src_addr = prtd->dma_addr + prtd->dma_buf_next[port_index] * dma_size;
	} else {
		prtd->pdma_lli_cfg[port_index][lli_index].des_addr = prtd->dma_addr + prtd->dma_buf_next[port_index] * dma_size;
	}

	prtd->dma_buf_next[port_index] = (prtd->dma_buf_next[port_index] + ports_cnt) % dma_buf_num;
}

static void asp_dma_lli_cfg(struct snd_pcm_substream *substream, unsigned int port_index)
{
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;
	unsigned int lli_index;
	unsigned int next_addr 		 = 0x0;
	unsigned int config			 = 0x0;
	unsigned int dma_lli_num 	 = DMA_LLI_NUM;
	unsigned int tx_dma_addr = 0X0;
	unsigned int rx_dma_addr = 0X0;
	unsigned int ports_cnt = prtd->ports.ports_cnt;
	unsigned int dma_size = prtd->period_size/ports_cnt;

	config = prtd->dma_cfg[port_index].config;
	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		tx_dma_addr = prtd->dma_cfg[port_index].port;
	} else {
		rx_dma_addr = prtd->dma_cfg[port_index].port;
	}

	memset(prtd->pdma_lli_cfg[port_index], 0, dma_lli_num * sizeof(struct dma_lli_cfg));/* unsafe_function_ignore: memset */
	for (lli_index = 0; lli_index < dma_lli_num; lli_index++) {
		next_addr = (unsigned int)(prtd->lli_dma_addr[port_index] + sizeof(struct dma_lli_cfg) * (lli_index + 1));
		prtd->pdma_lli_cfg[port_index][lli_index].lli 		= next_addr | DMA_LLI_ENABLE;
		prtd->pdma_lli_cfg[port_index][lli_index].config 	= config;
		prtd->pdma_lli_cfg[port_index][lli_index].des_addr 	= tx_dma_addr;
		prtd->pdma_lli_cfg[port_index][lli_index].src_addr 	= rx_dma_addr;
		prtd->pdma_lli_cfg[port_index][lli_index].a_count 	= dma_size;
		/* reset the src or dest addr of one dma list */
		asp_dma_set_lli_node(substream, port_index, lli_index);
	}

	prtd->pdma_lli_cfg[port_index][dma_lli_num - 1].lli = prtd->lli_dma_addr[port_index] | DMA_LLI_ENABLE;

	for (lli_index = 0; lli_index < dma_lli_num; lli_index++) {
		pr_debug("[%s:%d] port[%d] node[%d]  lli = 0x%x, count = %d, src_addr = 0x%pK, des_addr = 0x%pK, config = 0x%x\n",
									__FUNCTION__, __LINE__, port_index, lli_index,
									prtd->pdma_lli_cfg[port_index][lli_index].lli,
									prtd->pdma_lli_cfg[port_index][lli_index].a_count,
									(void *)(unsigned long)(prtd->pdma_lli_cfg[port_index][lli_index].src_addr),
									(void *)(unsigned long)(prtd->pdma_lli_cfg[port_index][lli_index].des_addr),
									prtd->pdma_lli_cfg[port_index][lli_index].config
									);
	}
}

static bool error_interrupt_handle(struct hi3xxx_asp_dmac_runtime_data *prtd, unsigned short int_type, unsigned int dma_channel)
{
	unsigned int count = 0;

	switch (int_type) {
		case ASP_DMA_INT_TYPE_ERR1:
			pr_err("[%s:%d] dmac channel %d interrupt config error happend\n", __FUNCTION__, __LINE__, dma_channel);
			count++;
			break;

		case ASP_DMA_INT_TYPE_ERR2:
			pr_err("[%s:%d] dmac channel %d interrupt transit error happend\n", __FUNCTION__, __LINE__, dma_channel);
			count++;
			break;

		case ASP_DMA_INT_TYPE_ERR3:
			pr_err("[%s:%d] dmac channel %d interrupt read lli error happend\n", __FUNCTION__, __LINE__, dma_channel);
			count++;
			break;

		case ASP_DMA_INT_TYPE_TC1:
			pr_info("[%s:%d] dmac channel %d transit finished\n", __FUNCTION__, __LINE__, dma_channel);
			break;

		/*dma lli transit finish interrupt*/
		case ASP_DMA_INT_TYPE_TC2:
			break;

		default:
			pr_info("[%s:%d] dmac interrupt error type[%d]\n", __FUNCTION__, __LINE__, int_type);
			count++;
			break;
	}

	if (unlikely(0 < count))
		return true;

	return false;
}

static int hi3xxx_asp_dmac_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;
	unsigned long bytes = params_buffer_bytes(params);
	unsigned long buffer_size = params_buffer_size(params);
	unsigned int lli_dma_addr = 0;
	unsigned int channels = params_channels(params);
	unsigned int ports_cnt = prtd->ports.ports_cnt;
	int stream = substream->stream;
	unsigned int port_index;
	int ret = 0;

	if (channels > 2) {
		pr_err("[%s:%d] channels %d not support!", __FUNCTION__, __LINE__, channels);
		ret = -EPERM;
		return ret;
	}

	ret = snd_pcm_lib_malloc_pages(substream, bytes);
	if (ret < 0) {
		pr_err("[%s:%d] snd_pcm_lib_malloc_pages ret is %d", __FUNCTION__, __LINE__, ret);
		return ret;
	}

	mutex_lock(&prtd->mutex);

	for (port_index = 0; port_index < ports_cnt; port_index++) {
		lli_dma_addr = hi3xxx_pcm_dma_lli_array[stream].pcm_dma_buf_base + (port_index * DMA_LLI_NUM * sizeof(struct dma_lli_cfg)) - ASP_FAMA_PHY_ADDR_DIFF;
		prtd->pdma_lli_cfg[port_index] = (struct dma_lli_cfg *)ioremap(hi3xxx_pcm_dma_lli_array[stream].pcm_dma_buf_base + (port_index * DMA_LLI_NUM * sizeof(struct dma_lli_cfg)), DMA_LLI_NUM * sizeof(struct dma_lli_cfg));
		if (NULL == prtd->pdma_lli_cfg[port_index]) {
			pr_err("[%s:%d] prtd->pdma_lli_cfg dma alloc coherent error!", __FUNCTION__, __LINE__);
			ret = -ENOMEM;
			goto err_out;
		}

		prtd->lli_dma_addr[port_index] = lli_dma_addr;
	}
	prtd->period_size = params_period_bytes(params);
	prtd->sampleRate = params_rate(params);

	pr_info("[%s:%d] dma buffer bytes: %lu(size: %lu), prtd->period_size: %u, sampleRate: %u\n", __FUNCTION__, __LINE__, bytes, buffer_size, prtd->period_size, prtd->sampleRate);

	mutex_unlock(&prtd->mutex);
	return ret;

err_out:
	pr_err("[%s:%d] hw params error, ret : %d\n", __FUNCTION__, __LINE__, ret);
	for (port_index = 0; port_index < ports_cnt; port_index++) {
		if (prtd->pdma_lli_cfg[port_index]) {
			iounmap(prtd->pdma_lli_cfg[port_index]);
			prtd->pdma_lli_cfg[port_index] = NULL;
		}
	}
	mutex_unlock(&prtd->mutex);

	snd_pcm_lib_free_pages(substream);

	return ret;
}

static int hi3xxx_asp_dmac_hw_free(struct snd_pcm_substream *substream)
{
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;
	unsigned int ports_cnt = prtd->ports.ports_cnt;
	unsigned int port_index;
	int ret = 0;

	mutex_lock(&prtd->mutex);

	for (port_index = 0; port_index < ports_cnt; port_index++) {
		if (prtd->pdma_lli_cfg[port_index]) {
			iounmap(prtd->pdma_lli_cfg[port_index]);
			prtd->pdma_lli_cfg[port_index] = NULL;
		}
	}

	mutex_unlock(&prtd->mutex);

	ret = snd_pcm_lib_free_pages(substream);

	return ret;
}

static int hi3xxx_intr_dmac_check_channel_irq(struct hi3xxx_asp_dmac_runtime_data *prtd, unsigned int dma_channel)
{
	unsigned long currIrqTimeNs;
	unsigned int irqIntervalMs;

	/* don't check if we just use one dma channel*/
	if (prtd->ports.ports_cnt == 1) {
		return 0;
	}

	if (prtd->preIrqTimeNs == 0) {
		prtd->preIrqTimeNs = hisi_getcurtime();
	} else {
		currIrqTimeNs = hisi_getcurtime();
		irqIntervalMs = (currIrqTimeNs - prtd->preIrqTimeNs)/1000000;
		prtd->preIrqTimeNs = currIrqTimeNs;
		if (irqIntervalMs > 2) {
			pr_warn("[%s:%d] dma channel irq error interval %d ms\n", __FUNCTION__, __LINE__,  irqIntervalMs);
			return -EFAULT;
		}
	}

	return 0;
}

static int hi3xxx_intr_dmac_check_period_irq(struct hi3xxx_asp_dmac_runtime_data *prtd, unsigned int intervalNormalMs, unsigned int dma_channel)
{
	unsigned long currPeriodTimeNs;
	unsigned int periodIntervalMs;
	int ret;

	ret = hi3xxx_intr_dmac_check_channel_irq(prtd, dma_channel);
	if (ret == 0) {
		prtd->preIrqTimeNs = 0;
	}

	if (prtd->prePeriodTimeNs == 0) {
		prtd->prePeriodTimeNs = hisi_getcurtime();
	} else {
		currPeriodTimeNs = hisi_getcurtime();
		periodIntervalMs = (currPeriodTimeNs - prtd->prePeriodTimeNs)/1000000;
		prtd->prePeriodTimeNs = currPeriodTimeNs;
		if (periodIntervalMs < intervalNormalMs / 2) {
			pr_err("[%s:%d] dma period irq error interval %d ms\n", __FUNCTION__, __LINE__,  periodIntervalMs);
		}

		if (periodIntervalMs > intervalNormalMs * 2) {
#ifdef MONITOR_DMA_EXCEPTION
			dmaPeriodErrCount++;
			if (dmaPeriodIntervalMs < periodIntervalMs) {
				dmaPeriodIntervalMs = periodIntervalMs;
			}
			cancel_delayed_work(&dma_exception_work);
			queue_delayed_work(dma_exception_workqueue, &dma_exception_work, msecs_to_jiffies(100));
#endif

			pr_err("[%s:%d] dma period irq error interval %d ms\n", __FUNCTION__, __LINE__,  periodIntervalMs);
			ret = -EFAULT;
		}
	}

	return ret;
}

static int hi3xxx_intr_dmac_check(struct hi3xxx_asp_dmac_runtime_data *prtd, unsigned int intervalNormalMs, unsigned int dma_channel)
{
	prtd->irq_cnt++;
	if (prtd->irq_cnt < prtd->ports.ports_cnt) {
		return hi3xxx_intr_dmac_check_channel_irq(prtd, dma_channel);
	}
	prtd->irq_cnt = 0;

	return hi3xxx_intr_dmac_check_period_irq(prtd, intervalNormalMs, dma_channel);
}

static int hi3xxx_intr_dmac_handle(unsigned short int_type, unsigned long para, unsigned int dma_channel)
{
	struct snd_pcm_substream *substream = (struct snd_pcm_substream *)para;
	struct snd_pcm_runtime *runtime = NULL;
	struct hi3xxx_asp_dmac_runtime_data *prtd = NULL;
	snd_pcm_uframes_t avail = 0;
	unsigned int rt_period_size = 0;
	unsigned int num_period = 0;
	unsigned int intervalNormalMs = 0;
	unsigned char *log_tag = NULL;

	if (NULL == substream) {
		pr_err("[%s:%d] substream is NULL\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	runtime = substream->runtime;
	BUG_ON(NULL == runtime);
	prtd = runtime->private_data;
	BUG_ON(NULL == prtd);

	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		log_tag = (unsigned char *)"StreamPlayback";
	} else {
		log_tag = (unsigned char *)"StreamCapture";
	}

	if (error_interrupt_handle(prtd, int_type, dma_channel)) {
		pr_err("[%s:%d][%s] IRQ_ERROR! dma_channel: %d\n", __FUNCTION__, __LINE__, log_tag, dma_channel);
		return -EFAULT;
	}

	/* DMA IS STOPPED */
	if (STATUS_DMAC_STOP == prtd->status) {
		pr_err("[%s:%d][%s] dma stopped, ignore this irq! dma_channel: %d\n", __FUNCTION__, __LINE__, log_tag, dma_channel);
		return -EPERM;
	}

	rt_period_size = runtime->period_size;
	num_period = runtime->periods;
	intervalNormalMs = (rt_period_size * 1000) / prtd->sampleRate;

	spin_lock(&prtd->lock);
	if (0 > hi3xxx_intr_dmac_check(prtd, intervalNormalMs, dma_channel)) {
		pr_err("[%s:%d][%s] IRQ_TIME_ERROR! dma_channel: %d\n", __FUNCTION__, __LINE__, log_tag, dma_channel);
		spin_unlock(&prtd->lock);
		return -EFAULT;
	}

	if (prtd->irq_cnt != 0) {
		pr_debug("[%s:%d][%s] IRQ_COMED! dma_channel: %d\n", __FUNCTION__, __LINE__, log_tag, dma_channel);
		spin_unlock(&prtd->lock);
		return 0;
	}

	prtd->period_cur = (prtd->period_cur + 1) % num_period;
	spin_unlock(&prtd->lock);

	snd_pcm_period_elapsed(substream);

	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		avail = snd_pcm_playback_hw_avail(runtime);
	} else {
		avail = snd_pcm_capture_hw_avail(runtime);
	}

	pr_debug("[%s:%d][%s] IRQ_COMED! dma_channel: %d, avail size: %ld, rt_period_size: %d\n", __FUNCTION__, __LINE__, log_tag, dma_channel, avail, rt_period_size);
	if (avail < rt_period_size) {
		pr_info("[%s:%d][%s] There is no avail data\n", __FUNCTION__, __LINE__, log_tag);
	}

	return 0;
}

static void hi3xxx_asp_dmac_get_dma_cfg(struct snd_pcm_substream *substream, unsigned int port_index)
{
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;
	struct dma_config *portDmaCfg = &(prtd->dma_cfg[port_index]);
	unsigned int portNum = prtd->pcm_port_tbl[prtd->ports.ports_tbl_idx][port_index];

	portDmaCfg->port = prtd->dma_cfg_tbl[portNum].port;
	portDmaCfg->config = prtd->dma_cfg_tbl[portNum].config;
	portDmaCfg->channel = prtd->dma_cfg_tbl[portNum].channel;
	pr_info("[%s:%d] port = 0x%x, config = 0x%x, channel = %u\n", __FUNCTION__, __LINE__,
		portDmaCfg->port, portDmaCfg->config, portDmaCfg->channel);
}

static int hi3xxx_asp_dmac_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;
	unsigned int ports_cnt = prtd->ports.ports_cnt;
	unsigned int port_index;
	callback_t callback;

	mutex_lock(&prtd->mutex);

	prtd->status = STATUS_DMAC_STOP;
	for (port_index = 0; port_index < ports_cnt; port_index++) {
		prtd->dma_buf_next[port_index] = port_index;
	}
	prtd->period_cur = 0;
	prtd->dma_addr = runtime->dma_addr - ASP_FAMA_PHY_ADDR_DIFF;

	callback = hi3xxx_intr_dmac_handle;

	for (port_index = 0; port_index < ports_cnt; port_index++) {
		hi3xxx_asp_dmac_get_dma_cfg(substream, port_index);
		asp_dma_lli_cfg(substream, port_index);
		asp_dma_config(prtd->dma_cfg[port_index].channel, prtd->pdma_lli_cfg[port_index], callback, (unsigned long)substream);
	}

	mutex_unlock(&prtd->mutex);

	return 0;
}

static int hi3xxx_asp_dmac_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;
	unsigned int ports_cnt = prtd->ports.ports_cnt;
	unsigned int port_index;
	unsigned int lli_index;

	pr_info("[%s:%d] stream = %d, cmd = %d \n", __FUNCTION__, __LINE__, substream->stream, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		spin_lock(&prtd->lock);
		prtd->status = STATUS_DMAC_RUNNING;
		spin_unlock(&prtd->lock);
		for (port_index = 0; port_index < ports_cnt; port_index++) {
			ret = asp_dma_start(prtd->dma_cfg[port_index].channel, prtd->pdma_lli_cfg[port_index]);
			if (ret < 0) {
				pr_err("[%s:%d] dma channel %d start failed\n", __FUNCTION__, __LINE__, prtd->dma_cfg[port_index].channel);
				return ret;
			}
		}
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		for (port_index = 0; port_index < ports_cnt; port_index++) {
			for (lli_index = 0; lli_index < DMA_LLI_NUM; lli_index++) {
				prtd->pdma_lli_cfg[port_index][lli_index].lli = 0x0;
			}
			asp_dma_stop(prtd->dma_cfg[port_index].channel);
		}
		spin_lock(&prtd->lock);
		prtd->irq_cnt = 0;
		prtd->preIrqTimeNs = 0;
		prtd->prePeriodTimeNs = 0;
		prtd->status = STATUS_DMAC_STOP;
		spin_unlock(&prtd->lock);
		break;

	default:
		pr_err("[%s:%d] cmd %d is invalid", __FUNCTION__, __LINE__, cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static snd_pcm_uframes_t hi3xxx_asp_dmac_pointer(struct snd_pcm_substream *substream)
{
	snd_pcm_uframes_t offset = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;
	unsigned int period_cur = 0;
	unsigned int period_size = 0;

	spin_lock(&prtd->lock);
	period_cur = prtd->period_cur;
	period_size = prtd->period_size;
	spin_unlock(&prtd->lock);

	offset = bytes_to_frames(runtime, period_cur * period_size);

	if (offset >= runtime->buffer_size)
		offset = 0;

	return offset;
}

static int hi3xxx_asp_dmac_mmap(struct snd_pcm_substream *substream,
				struct vm_area_struct *vma)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = NULL;

	runtime = substream->runtime;
	if (NULL == runtime) {
		pr_err("[%s:%d] runtime is invalid! \n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	ret = dma_mmap_writecombine(substream->pcm->card->dev,
			vma,
			runtime->dma_area,
			runtime->dma_addr,
			runtime->dma_bytes);

	return ret;
}

static int hi3xxx_asp_dmac_regulator_enable(struct hi3xxx_asp_dmac_data *pdata)
{
	int ret = 0;

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_get_sync(pdata->dev);
#else
	ret = regulator_enable(pdata->asp_ip);
	if (ret != 0) {
		pr_err("[%s:%d] couldn't enable regulators %d\n", __FUNCTION__, __LINE__, ret);
	}
#endif

	return ret;
}

static void hi3xxx_asp_dmac_regulator_disable(struct hi3xxx_asp_dmac_data *pdata)
{
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_mark_last_busy(pdata->dev);
	pm_runtime_put_autosuspend(pdata->dev);
#else
	if (regulator_disable(pdata->asp_ip) != 0) {
		pr_err("[%s:%d] Failed: regulator_disable\n", __FUNCTION__, __LINE__);
	}
#endif
}

static int hi3xxx_asp_dmac_get_codec(struct snd_soc_pcm_runtime *rtd, struct hi3xxx_asp_dmac_runtime_data *prtd)
{
	const char *codec_name = rtd->codec->component.name;

	if (!codec_name) {
		pr_err("[%s:%d] couldn't get codec_name\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}

	pr_info("[%s:%d] codec_name : %s\n", __FUNCTION__, __LINE__, codec_name);
	if (!strncmp(codec_name, CODEC_NAME_HI6402, strlen(CODEC_NAME_HI6402))) {
		prtd->pcm_port_tbl = pcm_port_tbl_6402;
		prtd->dma_cfg_tbl = dma_cfg_slimbus_6402;
	} else if (!strncmp(codec_name, CODEC_NAME_HI6403ES, strlen(CODEC_NAME_HI6403ES))) {
		prtd->pcm_port_tbl = pcm_port_tbl_6403es;
		prtd->dma_cfg_tbl = dma_cfg_slimbus_6403;
	} else if (!strncmp(codec_name, CODEC_NAME_HI6403, strlen(CODEC_NAME_HI6403))) {
		prtd->pcm_port_tbl = pcm_port_tbl_6403;
		prtd->dma_cfg_tbl = dma_cfg_slimbus_6403;
	} else {
		pr_err("[%s:%d] couldn't support codec %s\n", __FUNCTION__, __LINE__, codec_name);
		return -ENODEV;
	}

	return 0;
}

static int hi3xxx_asp_dmac_open(struct snd_pcm_substream *substream)
{
	struct hi3xxx_asp_dmac_runtime_data *prtd = NULL;
	struct snd_soc_pcm_runtime *rtd = NULL;
	struct hi3xxx_asp_dmac_data *pdata = NULL;
	struct snd_pcm *pcm = NULL ;
	int ret = 0;

	pcm = substream->pcm;
	prtd = kzalloc(sizeof(struct hi3xxx_asp_dmac_runtime_data), GFP_KERNEL);
	if (NULL == prtd) {
		pr_err("[%s:%d] kzalloc hi3xxx_asp_dmac_runtime_data error!\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	mutex_init(&prtd->mutex);
	spin_lock_init(&prtd->lock);

	rtd = (struct snd_soc_pcm_runtime *)substream->private_data;
	pdata = (struct hi3xxx_asp_dmac_data *)snd_soc_platform_get_drvdata(rtd->platform);

	if (NULL == pdata) {
		kfree(prtd);
		return -ENOMEM;
	}

	ret = hi3xxx_asp_dmac_regulator_enable(pdata);
	if (ret != 0) {
		goto err_bulk;
	}

	spin_lock(&prtd->lock);

	prtd->pdata = pdata;
	substream->runtime->private_data = prtd;

	ret = hi3xxx_asp_dmac_get_codec(rtd, prtd);
	if (ret < 0) {
		spin_unlock(&prtd->lock);
		hi3xxx_asp_dmac_regulator_disable(pdata);
		goto err_bulk;
	}

	prtd->ports.ports_cnt = PCM_PORTS_NUM;
	if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
		/* lowlatency playback stream use the same port as voice call*/
		if (pcm->device == PCM_DEVICE_LOW_LATENCY) {
			prtd->ports.ports_tbl_idx = PCM_PORTS_VOICE_DL;
		} else {
			prtd->ports.ports_tbl_idx = PCM_PORTS_AUDIO_DL;
		}
		pr_info("[%s:%d] SNDRV_PCM_STREAM_PLAYBACK pcm_device: %d, ports_tbl_idx: %d, ports_cnt: %d\n",
				__FUNCTION__, __LINE__, pcm->device, prtd->ports.ports_tbl_idx, prtd->ports.ports_cnt);
	} else {
		prtd->ports.ports_tbl_idx = PCM_PORTS_AUDIO_UL;
		pr_info("[%s:%d] SNDRV_PCM_STREAM_CAPTURE pcm_device: %d, ports_tbl_idx: %d, ports_cnt: %d\n",
				__FUNCTION__, __LINE__, pcm->device, prtd->ports.ports_tbl_idx, prtd->ports.ports_cnt);
	}

	spin_unlock(&prtd->lock);

	ret = snd_soc_set_runtime_hwparams(substream, &hi3xxx_asp_dmac_hardware);

	return ret;

err_bulk:
	kfree(prtd);
	return ret;
}

static int hi3xxx_asp_dmac_close(struct snd_pcm_substream *substream)
{
	struct hi3xxx_asp_dmac_runtime_data *prtd = substream->runtime->private_data;

	if (NULL == prtd) {
		pr_err("[%s:%d] prtd is NULL\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	BUG_ON(NULL == prtd->pdata);

	hi3xxx_asp_dmac_regulator_disable(prtd->pdata);

	kfree(prtd);
	substream->runtime->private_data = NULL;

	return 0;
}

static struct snd_pcm_ops hi3xxx_asp_dmac_ops = {
	.open	    = hi3xxx_asp_dmac_open,
	.close	    = hi3xxx_asp_dmac_close,
	.ioctl      = snd_pcm_lib_ioctl,
	.hw_params  = hi3xxx_asp_dmac_hw_params,
	.hw_free    = hi3xxx_asp_dmac_hw_free,
	.prepare    = hi3xxx_asp_dmac_prepare,
	.trigger    = hi3xxx_asp_dmac_trigger,
	.pointer    = hi3xxx_asp_dmac_pointer,
	.mmap       = hi3xxx_asp_dmac_mmap,
};

static unsigned long long hi3xxx_pcm_dmamask = DMA_BIT_MASK(32);

static int preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream 	*substream = pcm->streams[stream].substream;
	struct snd_dma_buffer 		*buf = &substream->dma_buffer;

	if ((pcm->device >= PCM_DEVICE_TOTAL) ||(stream >= PCM_STREAM_MAX)) {
		loge("Invalid argument  : device %d stream %d \n", pcm->device, stream);
		return -EINVAL;
	}

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->addr = hi3xxx_pcm_dma_buf_array[stream].pcm_dma_buf_base;
	buf->bytes = hi3xxx_pcm_dma_buf_array[stream].pcm_dma_buf_len;
	buf->area = ioremap_wc(buf->addr, buf->bytes);

	if (!buf->area) {
		loge("[%s:%d] error\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	return 0;
}

static void dma_free_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;

	substream = pcm->streams[stream].substream;
	if (!substream)
		return;

	buf = &substream->dma_buffer;
	if (!buf->area)
		return;

	iounmap((void __iomem *)buf->area);

	buf->area = NULL;
	buf->addr = 0;
}

static int hi3xxx_asp_dmac_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm  *pcm  = rtd->pcm;
	int ret = 0;

	BUG_ON(NULL == card);
	BUG_ON(NULL == pcm);

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &hi3xxx_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
		if (ret) {
			pr_err("[%s:%d] playback error : %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		}
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
		if (ret) {
			pr_err("[%s:%d] capture error : %d\n", __FUNCTION__, __LINE__, ret);
			return ret;
		}
	}

	return ret;
}

static void hi3xxx_asp_dmac_free(struct snd_pcm *pcm)
{
	BUG_ON(NULL == pcm);
	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		dma_free_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
	}
	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		dma_free_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
	}
}

static struct snd_soc_platform_driver hi3xxx_pcm_asp_dmac_platform = {
	.ops		= &hi3xxx_asp_dmac_ops,
	.pcm_new	= hi3xxx_asp_dmac_new,
	.pcm_free	= hi3xxx_asp_dmac_free,
};

#ifdef MONITOR_DMA_EXCEPTION
static void  hi3xxx_asp_dmac_monitor_work_handle(struct work_struct *work)
{
	unsigned long currPeriodTimeNs = hisi_getcurtime();

	//first time or period > 60 seconds
	if ((dmaPeriodErrTimeNs == 0 || (currPeriodTimeNs - dmaPeriodErrTimeNs)/1000000 > 60 * 1000) &&  dmaPeriodErrCount != 0) {
		dmaPeriodErrTimeNs = currPeriodTimeNs;
		triggerSendEvent = true;
	} else {
		triggerSendEvent = false;
	}

	if (triggerSendEvent) {
		struct imonitor_eventobj *pEventObj;
		pEventObj = imonitor_create_eventobj(916010502);
		if (pEventObj) {
			imonitor_set_param(pEventObj, 0 /*EXCEPTION_EEVENTLEVEL_INT*/, (long)3 /*MEDIA_LOG_ERROR*/);
			imonitor_set_param(pEventObj, 1 /*EXCEPTION_SUBTYPE_INT*/, (long)0);
			imonitor_set_param(pEventObj, 2 /*EXCEPTION_REASON_VARCHAR*/, (long)"dma period err");
			imonitor_set_param(pEventObj, 4 /*EXCEPTION_COUNT_INT*/, (long)dmaPeriodErrCount);
			imonitor_set_param(pEventObj, 5 /*EXCEPTION_PARAINT_INT*/, (long)dmaPeriodIntervalMs);
			imonitor_send_event(pEventObj);
			imonitor_destroy_eventobj(pEventObj);
			//reset count and state
			pr_err("trigger_monitor_dma_exception %d \n", dmaPeriodErrCount);
			dmaPeriodErrCount = 0;
			dmaPeriodIntervalMs = 0;
			triggerSendEvent = false;
		}
	}
}
#endif

static int hi3xxx_asp_dmac_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct device *dev = &pdev->dev;
	struct hi3xxx_asp_dmac_data *pdata = NULL;

	if (!dev) {
		pr_err("[%s:%d] platform_device has no device\n", __FUNCTION__, __LINE__);
		return -ENOENT;
	}

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		pr_err("[%s:%d] cannot allocate hi3630 asp dma platform data\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	/* get resources */
	pdata->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!pdata->res) {
		pr_err("[%s:%d] platform_get_resource err\n", __FUNCTION__, __LINE__);
		ret = -ENOENT;
		goto get_res_failed;
	}

	if (!devm_request_mem_region(dev, pdata->res->start,
					 resource_size(pdata->res),
					 pdev->name)) {
		pr_err("[%s:%d] cannot claim register memory\n", __FUNCTION__, __LINE__);
		ret = -ENOENT;
		goto get_res_failed;
	}

	pdata->reg_base_addr = devm_ioremap(dev, pdata->res->start,
						resource_size(pdata->res));
	if (!pdata->reg_base_addr) {
		pr_err("[%s:%d] cannot map register memory\n", __FUNCTION__, __LINE__);
		ret = -ENOENT;
		goto ioremap_failed;
	}
	pr_info("[%s:%d] base = %pK , virt = %pK \n", __FUNCTION__, __LINE__, (void *)pdata->res->start, pdata->reg_base_addr);

	pdata->asp_ip = devm_regulator_get(dev, "asp-dmac");
	if (IS_ERR(pdata->asp_ip)) {
		pr_err("[%s:%d] couldn't get regulators %d\n", __FUNCTION__, __LINE__, ret);
		ret = -ENOENT;
		goto get_regulator_failed;
	}

	pdata->dev = dev;

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_set_autosuspend_delay(dev, 100); /* 100ms*/
	pm_runtime_use_autosuspend(dev);

	pm_runtime_enable(dev);
#endif

#ifdef MONITOR_DMA_EXCEPTION
	dma_exception_workqueue = create_singlethread_workqueue("dma_exception_workqueue");
	if (!dma_exception_workqueue) {
		pr_err("%s(%u) : dma_exception_workqueue create failed",__FUNCTION__, __LINE__);
		goto workqueue_create_failed;
	}
	INIT_DELAYED_WORK(&dma_exception_work, hi3xxx_asp_dmac_monitor_work_handle);
#endif

	platform_set_drvdata(pdev, pdata);

	dev_set_name(dev, "hi3xxx-pcm-asp-dma");

	ret = snd_soc_register_platform(dev, &hi3xxx_pcm_asp_dmac_platform);
	if (ret) {
		pr_err("[%s:%d] snd_soc_register_platform return %d\n", __FUNCTION__, __LINE__, ret);
		ret = -ENODEV;
		goto reg_platform_failed;
	}

	return 0;

reg_platform_failed:
#ifdef MONITOR_DMA_EXCEPTION
	destroy_workqueue(dma_exception_workqueue);
workqueue_create_failed:
#endif
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&pdev->dev);
#endif
get_regulator_failed:
	devm_iounmap(dev, pdata->reg_base_addr);
ioremap_failed:
	devm_release_mem_region(dev, pdata->res->start,
				resource_size(pdata->res));
get_res_failed:
	devm_kfree(dev, pdata);

	return ret;

}

static int hi3xxx_asp_dmac_remove(struct platform_device *pdev)
{
	struct hi3xxx_asp_dmac_data *pdata = dev_get_drvdata(&pdev->dev);

#ifdef MONITOR_DMA_EXCEPTION
	if (dma_exception_workqueue) {
		flush_workqueue(dma_exception_workqueue);
		destroy_workqueue(dma_exception_workqueue);
	}
#endif

	snd_soc_unregister_platform(&pdev->dev);
#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&pdev->dev);
#endif
	devm_iounmap(&pdev->dev, pdata->reg_base_addr);
	devm_release_mem_region(&pdev->dev, pdata->res->start,
				resource_size(pdata->res));
	devm_kfree(&pdev->dev, pdata);

	return 0;
}

#ifdef CONFIG_PM_RUNTIME
int hi3xxx_asp_dmac_runtime_suspend(struct device *dev)
{
	struct hi3xxx_asp_dmac_data *pdata = dev_get_drvdata(dev);

	BUG_ON(NULL == pdata);

	pr_info("[%s:%d] +\n", __FUNCTION__, __LINE__);

	regulator_disable(pdata->asp_ip);

	pr_info("[%s:%d] -\n", __FUNCTION__, __LINE__);

	return 0;
}

int hi3xxx_asp_dmac_runtime_resume(struct device *dev)
{
	struct hi3xxx_asp_dmac_data *pdata = dev_get_drvdata(dev);
	int ret;

	BUG_ON(NULL == pdata);

	pr_info("[%s:%d] +\n", __FUNCTION__, __LINE__);

	ret = regulator_enable(pdata->asp_ip);
	if (0 != ret)
		pr_err("[%s:%d] couldn't enable regulators %d\n", __FUNCTION__, __LINE__, ret);

	pr_info("[%s:%d] -\n", __FUNCTION__, __LINE__);

	return ret;
}

const struct dev_pm_ops hi3xxx_asp_dmac_pm_ops = {
	.runtime_suspend	= hi3xxx_asp_dmac_runtime_suspend,
	.runtime_resume		= hi3xxx_asp_dmac_runtime_resume,
};
#endif

static struct platform_driver hi3xxx_asp_dmac_driver = {
	.driver = {
		.name	= "hi3xxx-pcm-asp-dma",
		.owner	= THIS_MODULE,
		.of_match_table = hi3xxx_asp_dmac_of_match,
#ifdef CONFIG_PM_RUNTIME
		.pm	= &hi3xxx_asp_dmac_pm_ops,
#endif
	},
	.probe	= hi3xxx_asp_dmac_probe,
	.remove	= hi3xxx_asp_dmac_remove,
};
module_platform_driver(hi3xxx_asp_dmac_driver);

MODULE_AUTHOR("chengong <apollo.chengong@huawei.com>");
MODULE_DESCRIPTION("Hi3xxx ASP DMA Platform Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hi3xxx_asp_dmac");
