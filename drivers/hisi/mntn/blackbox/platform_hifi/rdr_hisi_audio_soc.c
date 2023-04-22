/*
 * audio soc rdr.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/kthread.h>
#include <linux/thread_info.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_ap_hook.h>
#include <linux/interrupt.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kmod.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/uaccess.h>
#include <linux/wakelock.h>
#include <linux/of_irq.h>
#include <linux/version.h>
#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_field.h"

#include "rdr_hisi_audio_adapter.h"
#include "rdr_hisi_audio_soc.h"

#include "hifi_lpp.h"
#include "hifi_om.h"
#include "usbaudio_ioctl.h"
#include <linux/hisi/usb/hisi_hifi_usb.h>
#include <dsm_audio/dsm_audio.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/remoteproc.h>
#include <linux/hisi/hisi_drmdriver.h>
#include <linux/of_reserved_mem.h>
#include <linux/hisi/hisi_load_image.h>
#include <linux/hisi/hisi_partition.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/hisi/hisi_mm.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include "teek_client_id.h"

/*lint -e750 -e838 -e730 -e715*/
#define RDR_HIFI_DUMP_ADDR				(HIFI_DUMP_BIN_ADDR)
#define RDR_HIFI_DUMP_SIZE				(HIFI_DUMP_BIN_SIZE)
#define RDR_COMMENT_LEN					(128UL)

#define HIFI_BSS_SEC					(2)
#define SOC_WDT_TIMEOUT_IRQ_NUM			(245U)
#define OM_SOC_LOG_PATH					"sochifi_logs"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
#define WATCHDOG_DTS_COMP_NAME "hisilicon,sochifi-watchdog"
#endif

#define DRV_WATCHDOG_CONTROL            (DRV_WATCHDOG_BASE_ADDR + 0x008)
#define DRV_WATCHDOG_INTCLR             (DRV_WATCHDOG_BASE_ADDR + 0x00C)
#define DRV_WATCHDOG_LOCK               (DRV_WATCHDOG_BASE_ADDR + 0xC00)
#define DRV_WATCHDOG_UNLOCK_NUM	        (0x1ACCE551)
#define DRV_WATCHDOG_LOCK_NUM           (0x0)
#define DRV_WATCHDOG_CONTROL_DISABLE    (0x0)
#define DRV_WATCHDOG_INTCLR_NUM         (0x4455) /* Generally speaking, any number is okay */

#define HIFI_DIE_NOTIFY_LPM3			((0 << 24) | (16 << 16) | (3 << 8) | (1 << 0))
										/*	bit 24-31 OBJ_AP
											bit 16-23 OBJ_PSCI
											bit 8-15  CMD_SETTING
											bit 0-7   TYPE_POWER */

#define CFG_DSP_NMI                     (0x3C)               /*DSP NMI ,bit0-bit15*/
#define CFG_MMBUF_REMAP_EN              (0x130)              /*mmbuf remap enable，9bit*/
#define CFG_OCRAM_REMAP_EN              (0x13C)              /*ocram remap enable，9bit*/
#define ASP_CFG_BASE                    (SOC_ACPU_ASP_CFG_BASE_ADDR)

struct rdr_soc_des_s {
	uint32_t modid;
	uint32_t wdt_irq_num;
	char *pathname;
	uint32_t *control_addr;
	uint32_t *lock_addr;
	uint32_t *intclr_addr;
	pfn_cb_dump_done dumpdone_cb;

	struct semaphore dump_sem;
	struct semaphore handler_sem;
	struct wake_lock rdr_wl;
	struct task_struct *kdump_task;
	struct task_struct *khandler_task;
};
static struct rdr_soc_des_s soc_des;

#define DTS_COMP_HIFICMA_NAME    "hisilicon,hifi-cma"
#define HIFI_CMA_IMAGE_SIZE    (0x600000UL)
struct hisi_hifi_cma_struct {
    struct device *device;
};

struct hisi_hifi_cma_struct hificma_dev;

/*mem_dyn*/
char *mem_dyn_type[] =
{
	"UCOM_MEM_DYN_TYPE_DDR",
	"UCOM_MEM_DYN_TYPE_TCM",
	"UCOM_MEM_DYN_TYPE_OCB",
	"UCOM_MEM_DYN_TYPE_USB_160K",
	"UCOM_MEM_DYN_TYPE_USB_96K",
	"UCOM_MEM_DYN_TYPE_BUTT"
};

char *mem_dyn_om_enable[] =
{
	"UCOM_MEM_DYN_OM_ENABLE_NO",
	"UCOM_MEM_DYN_OM_ENABLE_YES",
	"UCOM_MEM_DYN_OM_ENABLE_BUTT",
};

/* flag */
struct flag_info{
	char flag_name[32];
	unsigned int flag_addr_offset;
};

struct flag_info sochifi_flag[] = {
	{"DRV_DSP_PANIC_MARK",         (DRV_DSP_PANIC_MARK - HIFI_FLAG_DATA_ADDR)},
	{"DRV_DSP_UART_LOG_LEVEL",     (DRV_DSP_UART_LOG_LEVEL - HIFI_FLAG_DATA_ADDR)},
	{"DRV_DSP_UART_TO_MEM_CUR",    (DRV_DSP_UART_TO_MEM_CUR_ADDR - HIFI_FLAG_DATA_ADDR)},
	{"DRV_DSP_EXCEPTION_NO",       (DRV_DSP_EXCEPTION_NO - HIFI_FLAG_DATA_ADDR)},
	{"DRV_DSP_IDLE_COUNT",         (DRV_DSP_IDLE_COUNT_ADDR - HIFI_FLAG_DATA_ADDR)},
	{"DRV_DSP_LOADED_INDICATE",    (DRV_DSP_LOADED_INDICATE - HIFI_FLAG_DATA_ADDR)},
	{"DRV_DSP_POWER_STATUS",       (DRV_DSP_POWER_STATUS_ADDR - HIFI_FLAG_DATA_ADDR)},
	{"DRV_DSP_NMI_FLAG",           (DRV_DSP_NMI_FLAG_ADDR - HIFI_FLAG_DATA_ADDR)}
};

#define FLAG_ROW_LEN (64)
#define FLAG_COMMENT_LEN (128)
#define PARSE_FLAG_LOG_SIZE (FLAG_ROW_LEN * ARRAY_SIZE(sochifi_flag) + FLAG_COMMENT_LEN)


static void hisi_rdr_nmi_notify_hifi(void)
{
	unsigned int value;
	void __iomem *rdr_aspcfg_base = NULL;

	rdr_aspcfg_base = ioremap(ASP_CFG_BASE, (unsigned long)SZ_4K);
	if (!rdr_aspcfg_base) {
		BB_PRINT_ERR("%s():rdr_aspcfg_base ioremap error\n", __func__);
		return;
	}

	value = (unsigned int)readl(rdr_aspcfg_base + CFG_DSP_NMI);
	value &= ~(0x1 << 0);
	writel(value, (rdr_aspcfg_base + CFG_DSP_NMI));

	value |= (0x1 << 0);
	writel(value, (rdr_aspcfg_base + CFG_DSP_NMI));
	udelay(1);
	writel(0x0, (rdr_aspcfg_base + CFG_DSP_NMI));

	iounmap(rdr_aspcfg_base);

	BB_PRINT_PN("%s\n", __func__);
	return;/*lint !e438*/
}/*lint !e550*/


static void hisi_rdr_remap_init(void)
{
	void __iomem *rdr_aspcfg_base = NULL;
	unsigned int read_val;

	rdr_aspcfg_base = ioremap(ASP_CFG_BASE, (unsigned long)SZ_4K);
	if (!rdr_aspcfg_base) {
		BB_PRINT_ERR("%s():rdr_aspcfg_base ioremap error\n", __func__);
		return;
	}

	read_val = (unsigned int)readl(rdr_aspcfg_base + CFG_MMBUF_REMAP_EN);
	read_val &= (0x1<<9);
	if (read_val != 0)
		writel(0x0, (rdr_aspcfg_base + CFG_MMBUF_REMAP_EN));

	read_val = (unsigned int)readl(rdr_aspcfg_base + CFG_OCRAM_REMAP_EN);/*lint !e548*/
	read_val &= (0x1<<9);
	if (read_val != 0)
		writel(0x0, (rdr_aspcfg_base + CFG_OCRAM_REMAP_EN));

	iounmap(rdr_aspcfg_base);/*lint !e548*/

	BB_PRINT_PN("%s\n", __func__);
	return;
}

static bool is_dsp_power_on(void)
{
	unsigned int *power_status_addr = NULL;
	int power_status = 0;
	bool is_power_on = false;

	power_status_addr = (unsigned int *)ioremap_wc(DRV_DSP_POWER_STATUS_ADDR, (unsigned long)0x4);
	if (NULL == power_status_addr) {
		BB_PRINT_ERR("%s():DRV_DSP_POWER_STATUS_ADDR ioremap failed\n", __func__);
		return false;
	}

	power_status = readl(power_status_addr);
	if (DRV_DSP_POWER_ON == power_status)
		is_power_on = true;
	else if (DRV_DSP_POWER_OFF == power_status)
		is_power_on = false;
	else
		BB_PRINT_ERR("Get dsp power status error.[0x%x]\n", power_status);

	iounmap(power_status_addr);

	return is_power_on;
}

static bool is_nmi_complete(void)
{
	unsigned int *nmi_flag_addr = NULL;
	bool is_complete = false;

	nmi_flag_addr = (unsigned int *)ioremap_wc(DRV_DSP_NMI_FLAG_ADDR, 0x4ul);
	if (NULL == nmi_flag_addr) {
		BB_PRINT_ERR("%s():DRV_DSP_NMI_FLAG_ADDR ioremap failed\n", __func__);
		return false;
	}

	if ((unsigned int)DRV_DSP_NMI_COMPLETE == (unsigned int)readl(nmi_flag_addr))
		is_complete = true;
	else
		is_complete = false;

	iounmap(nmi_flag_addr);

	return is_complete;
}

static int parse_sochifi_innerlog(char *original_data, unsigned int original_data_size,
	char *parsed_data, unsigned int parsed_data_size, unsigned int core_type)
{
	int i;
	int ret = 0;
	unsigned int index;
	struct innerlog_obj *innerlog;

	if (NULL == original_data || NULL == parsed_data) {
		BB_PRINT_ERR("input data buffer is null\n");
		return -EINVAL;
	}

	memset(parsed_data, 0, parsed_data_size);/* unsafe_function_ignore: memset */
	snprintf(parsed_data, parsed_data_size, "\n\n/*********[innerlog info begin]*********/\n\n");

	innerlog = (struct innerlog_obj *)original_data;
	index = innerlog->curr_idx;
	if (index < OM_LOG_INNER_MAX_NUM) {
		const char *title = "enLogID | uwTimeStamp | uhwFileID | uhwLineID |   uwVal1          |         uwVal2          |         uwVal3 \n";

		snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data), "%s", title);
		for (i = 0; i < OM_LOG_INNER_MAX_NUM; i++) {
			snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
				"0x%-8x 0x%-13x %-11d %-10d 0x%-17x       0x%-17x       0x%-17x \n",
				innerlog->records[index].enlogid,
				innerlog->records[index].time_stamp,
				innerlog->records[index].fileid,
				innerlog->records[index].lineid,
				innerlog->records[index].value1,
				innerlog->records[index].value2,
				innerlog->records[index].value3);
			index++;
			if (index >= OM_LOG_INNER_MAX_NUM) {
				index = 0;
			}
		}
	} else {
		ret = -EINVAL;
		BB_PRINT_ERR("innerlog info invalid\n");
	}
	snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
		"\n\n/*********[innerlog info end]*********/\n\n");

	return ret;
}

static int parse_sochifi_dynmem(char *original_data, unsigned int original_data_size,
	char *parsed_data, unsigned int parsed_data_size, unsigned int core_type)
{
	int i;
	struct mem_dyn_ctrl *dynmem;
	struct mem_dyn_status *status;
	struct mem_dyn_node *nodes;

	if (NULL == original_data || NULL == parsed_data) {
		BB_PRINT_ERR("input data buffer is null\n");
		return -EINVAL;
	}

	memset(parsed_data, 0, parsed_data_size);/* unsafe_function_ignore: memset */
	snprintf(parsed_data, parsed_data_size, "\n\n/*********[dynmem info begin]*********/\n\n");
	dynmem = (struct mem_dyn_ctrl *)original_data;

	for (i = 0; i < UCOM_MEM_DYN_TYPE_BUTT; i++ ) {
		snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
			"astStatus[%s]\n", mem_dyn_type[i]);
		status = &dynmem->status[i];

		if (status->enable >= UCOM_MEM_DYN_OM_ENABLE_BUTT) {
		    snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
		    	" |- enEnable: ERROR:0x%-10x\n", status->enable);
		} else {
		    snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
		    	" |- enEnable: %-28s\n", mem_dyn_om_enable[status->enable]);
		}

		snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
			" |- uwTotalSize: %-10u\n |- uwUsedRate: %-10u\n |- astMemTrace\n     |- uwCurrUsedRate: %-10u\n"\
			"|- uwTimeStamp: 0x%-10x\n",
			status->total_size,
			status->used_rate,
			status->mem_trace.curr_used_rate,
			status->mem_trace.time_stamp);
	}

	for (i= 0; i < UCOM_MEM_DYN_NODE_MAX_NUM; i++) {
		snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
			"astNodes[%d]\n", i + 1);
		nodes = &dynmem->nodes[i];
		snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
			" |- stBlk\n      uwAddr: 0x%08x    uwSize: %-10u    uwFileId: %-10u    uwLineId: %-10u\n",
			nodes->blk.addr, nodes->blk.size, nodes->blk.fileid, nodes->blk.lineid);
		snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
			" |- pstNext: 0x%08x\n |- pstPrev: 0x%08x\n", nodes->next, nodes->prev);
	}

	snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
		"\n\n/*********[dynmem info end]*********/\n\n");

	return 0;
}

static int parse_sochifi_flag(char *original_data, unsigned int original_data_size,
	char *parsed_data, unsigned int parsed_data_size, unsigned int core_type)
{
	unsigned int i = 0;

	if (NULL == original_data || NULL == parsed_data) {
		BB_PRINT_ERR("%s invalid param:base_buf:%s,dump_flag_addr:%s,dump_flag_size:%d\n",
			__func__, original_data, parsed_data, (unsigned int)parsed_data_size);
		return -EINVAL;
	}

	memset(parsed_data, 0, parsed_data_size);/* unsafe_function_ignore: memset */
	snprintf(parsed_data, parsed_data_size, "\n\n/*********[flag info begin]*********/\n\n");

	for (i = 0; i < ARRAY_SIZE(sochifi_flag); i++) {
		snprintf(parsed_data + strlen(parsed_data),	parsed_data_size - strlen(parsed_data),
			"%-26s 0x%08x\n", sochifi_flag[i].flag_name,
			*((unsigned int*)(original_data + sochifi_flag[i].flag_addr_offset)));
	}

	snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data),
		"\n\n/*********[flag info end]*********/\n\n");

	return 0;
}

struct parse_log parse_sochifi_log[] = {
	{HIFI_OM_LOG_SIZE + DRV_DSP_UART_TO_MEM_SIZE, DRV_DSP_STACK_TO_MEM_SIZE, PARSER_SOCHIFI_TRACE_SIZE, parse_hifi_trace},
	{RDR_FALG_OFFSET, HIFI_FLAG_DATA_SIZE, PARSE_FLAG_LOG_SIZE, parse_sochifi_flag},
	{0, SOCHIFI_ORIGINAL_CPUVIEW_SIZE, PARSER_SOCHIFI_CPUVIEW_LOG_SIZE, parse_hifi_cpuview},
	{SOCHIFI_ORIGINAL_CPUVIEW_SIZE, SOCHIFI_ORIGINAL_INNERLOG_SIZE, PARSE_INNERLOG_SIZE, parse_sochifi_innerlog},
	{SOCHIFI_ORIGINAL_CPUVIEW_SIZE + SOCHIFI_ORIGINAL_INNERLOG_SIZE, SOCHIFI_ORIGINAL_DYNMEM_SIZE, PARSE_MEM_DYN_LOG_SIZE, parse_sochifi_dynmem},
};

static int dump_hifi_ddr(char *filepath)
{
	char *buf = NULL;
	char *full_text = NULL;
	char *parse_text = NULL;
	unsigned int i = 0;
	int ret = 0;
	char xn[RDR_FNAME_LEN] = {0};
	char comment[RDR_COMMENT_LEN] = {0};
	int count = 0;
	size_t hifi_log_size = 0;
	size_t full_text_size = 0;

	if (NULL == filepath) {
		BB_PRINT_ERR("%s error: filepath is NULL\n", __func__);
		return -ENOENT;
	}

	while (count < 10) {
		if (is_nmi_complete()) {
			break;
		} else {
			count++;
			msleep(1);
		}
	}

	if (10 == count) {
		BB_PRINT_ERR("NMI process is uncomplete in hifi, om log maybe inaccurate\n");
		snprintf(comment, RDR_COMMENT_LEN, "NMI process is uncomplete in hifi, OM log maybe inaccurate");
	} else {
		snprintf(comment, RDR_COMMENT_LEN, "OM log is fine.");
	}

	snprintf(xn, RDR_FNAME_LEN, "%s%s/hifi_ddr.bin", filepath, OM_SOC_LOG_PATH); /* [false alarm]: RDR_FNAME_LEN As expected */

	buf = (char *)ioremap_wc(RDR_HIFI_DUMP_ADDR, RDR_HIFI_DUMP_SIZE);
	if (!buf) {
		BB_PRINT_ERR("%s error: ioremap hifi dump addr fail\n", __func__);
		return -ENOMEM;
	}

	hifi_log_size = COMPILE_TIME_BUFF_SIZE + strlen("\n\n") + (DRV_DSP_UART_TO_MEM_SIZE - DRV_DSP_UART_TO_MEM_RESERVE_SIZE);
	full_text_size = hifi_log_size;
	for (i = 0; i < ARRAY_SIZE(parse_sochifi_log); i++) {
		full_text_size += parse_sochifi_log[i].parse_log_size;
	}
	full_text = vzalloc(full_text_size);
	if (NULL == full_text) {
		BB_PRINT_ERR("%s error: alloc full_text failed\n", __func__);
		ret = -ENOMEM;
		goto error;
	}

	/*hifi log*/
	memcpy(full_text + strlen(full_text), buf + HIFI_OM_LOG_SIZE, COMPILE_TIME_BUFF_SIZE);
	memcpy(full_text + strlen(full_text), "\n\n", strlen("\n\n"));
	memcpy(full_text + COMPILE_TIME_BUFF_SIZE + strlen("\n\n"), buf + HIFI_OM_LOG_SIZE + DRV_DSP_UART_TO_MEM_RESERVE_SIZE,
		DRV_DSP_UART_TO_MEM_SIZE - DRV_DSP_UART_TO_MEM_RESERVE_SIZE);

	/*start parse hifi log*/
	BB_PRINT_PN("start parse hifi log\n");
	parse_text = full_text + hifi_log_size;
	for (i = 0; i < ARRAY_SIZE(parse_sochifi_log); i++) {
		if (hifi_log_size + strlen(parse_text) + parse_sochifi_log[i].parse_log_size > full_text_size) {
			BB_PRINT_ERR("log size more than the full_text_size\n");
			break;
		}

		ret = parse_sochifi_log[i].parse_func(buf + parse_sochifi_log[i].original_offset,
			parse_sochifi_log[i].original_log_size,
			parse_text + strlen(parse_text),
			parse_sochifi_log[i].parse_log_size,
			SOCHIFI);
		if (ret)
			BB_PRINT_ERR("%s error: parser module %u failed\n", __func__, i);
	}

	BB_PRINT_PN("end parser hifi log\n");
	ret = rdr_audio_write_file(xn, full_text, hifi_log_size + strlen(parse_text));/*lint !e747*/
	if (ret)
		BB_PRINT_ERR("rdr:dump %s fail\n", xn);

	vfree(full_text);
error:
	iounmap(buf);

	return ret;
}

static int save_icc_channel_fifo(void)
{
	int i = 0;
	struct icc_channel_fifo *dst_fifo = NULL;
	struct icc_channel_fifo *src_fifo = NULL;
	struct icc_dbg *icc_dbg_info = NULL;

	icc_dbg_info = (struct icc_dbg *)ioremap_wc(HIFI_ICC_DEBUG_LOCATION,
						(unsigned long)HIFI_ICC_DEBUG_SIZE);
	if (!icc_dbg_info) {
		BB_PRINT_ERR("%s(): ioremap hifi icc_dbg_info fail\n", __func__);
		return -ENOMEM;
	}

	if ((ICC_DEBUG_PROTECT_WORD1 != icc_dbg_info->protectword1)
		|| (ICC_DEBUG_PROTECT_WORD2 != icc_dbg_info->protectword2)) {
		BB_PRINT_ERR("%s(): check ICC_DEBUG_PROTECT_WORD fail [0x%x, 0x%x], do not save icc fifo\n",
			__func__, icc_dbg_info->protectword1, icc_dbg_info->protectword2);
		iounmap(icc_dbg_info);
		return -EINVAL;
	}

	for (i = 0; i < 2; i++) {
		struct icc_channel *channel = &icc_dbg_info->icc_channels[i];
		/* 保存接收fifo */
		if (0 != channel->fifo_recv) {
			dst_fifo = (struct icc_channel_fifo *)(((char *)&(channel->fifo_recv)) + *(unsigned int *)(&channel->fifo_recv));/*lint !e826*/
			src_fifo = (struct icc_channel_fifo *)ioremap_wc(*((unsigned int *)dst_fifo->data) + ICC_SHARE_FAMA_ADDR_OFFSET,
						dst_fifo->size + sizeof(struct icc_channel_fifo));
			if (!src_fifo) {
				BB_PRINT_ERR("rdr: remap hifi recv src_fifo fail\n");
				iounmap(icc_dbg_info);
				return -ENOMEM;
			}

			/* 拷贝fifo通道数据 */
			memcpy((void *)((char *)dst_fifo + sizeof(struct icc_channel_fifo)),
					(void *)((char *)src_fifo + sizeof(struct icc_channel_fifo)),
					(unsigned long)dst_fifo->size);
			/* 更新Fifo头信息 */
			dst_fifo->magic = src_fifo->magic;
			dst_fifo->read  = src_fifo->read;
			dst_fifo->write = src_fifo->write;
			iounmap(src_fifo);
			src_fifo = NULL;
		}

		/* 保存发送fifo */
		if (0 != channel->fifo_send) {
			dst_fifo = (struct icc_channel_fifo *)((char *)&(channel->fifo_send) + *(unsigned int *)(&channel->fifo_send));/*lint !e826*/
			src_fifo = (struct icc_channel_fifo *)ioremap_wc(*((unsigned int *)dst_fifo->data) + ICC_SHARE_FAMA_ADDR_OFFSET,
						dst_fifo->size + sizeof(struct icc_channel_fifo));

			if (!src_fifo) {
				BB_PRINT_ERR("rdr: remap hifi send src_fifo fail\n");
				iounmap(icc_dbg_info);
				return -ENOMEM;
			}
			/* 拷贝fifo通道数据 */
			memcpy((void *)((char *)dst_fifo + sizeof(struct icc_channel_fifo)),
						(void *)((char *)src_fifo + sizeof(struct icc_channel_fifo)),
						(unsigned long)(dst_fifo->size));
			/* 更新Fifo头信息 */
			dst_fifo->magic = src_fifo->magic;
			dst_fifo->read = src_fifo->read;
			dst_fifo->write = src_fifo->write;

			iounmap(src_fifo);
			src_fifo = NULL;
		}
	}
	iounmap(icc_dbg_info);

	return 0;
}


static void dump_hifi(char *filepath)
{
	int ret = 0;

	BUG_ON(NULL == filepath);

	ret = save_icc_channel_fifo();
	BB_PRINT_PN("rdr:%s()save_icc_channel_fifo,%s\n", __func__, ret ? "fail" : "success");

	ret = dump_hifi_ddr(filepath);
	BB_PRINT_PN("rdr:%s():dump hifi ddr, %s\n", __func__, ret ? "fail" : "success");

	if ((soc_des.modid >= (unsigned int)RDR_AUDIO_SOC_MODID_START) && (soc_des.modid <= RDR_AUDIO_SOC_MODID_END)) {
		audio_dsm_report_info(AUDIO_CODEC, DSM_SOC_HIFI_RESET, "DSM_SOC_HIFI_RESET\n");
	}

	return;
}

static int reset_hifi_sec(void)
{
	struct drv_hifi_sec_ddr_head *head;
	char *sec_head = NULL;
	char *sec_addr = NULL;
	unsigned int i;
	int ret = 0;

	sec_head = (char *)ioremap_wc(HIFI_SEC_HEAD_BACKUP, (unsigned long)HIFI_SEC_HEAD_SIZE);
	if (!sec_head) {
		ret = -ENOMEM;
		goto error;
	}
	head = (struct drv_hifi_sec_ddr_head *)sec_head;/*lint !e826*/

	BB_PRINT_PN("sections_num = 0x%x\n", head->sections_num);

	for (i = 0; i < head->sections_num; i++) {
		if (head->sections[i].type == HIFI_BSS_SEC) {
			BB_PRINT_PN("sec_id = %d, type = 0x%x, src_addr = 0x%pK, des_addr = 0x%pK, size = %d\n",
					i,
					head->sections[i].type,
					(void *)(unsigned long)(head->sections[i].src_addr),
					(void *)(unsigned long)(head->sections[i].des_addr),
					head->sections[i].size);
			sec_addr = (char *)ioremap_wc((phys_addr_t)head->sections[i].des_addr,
							(unsigned long)head->sections[i].size);
			if (NULL == sec_addr) {
				ret = -ENOMEM;
				goto error1;
			}

			memset(sec_addr, 0x0, (unsigned long)head->sections[i].size);
			iounmap(sec_addr);
			sec_addr = NULL;
		}
	}

error1:
	iounmap(sec_head);
error:
	return ret;
}

static int dump_thread(void *arg)
{
	BB_PRINT_START();

	while (!kthread_should_stop()) {
		if (down_interruptible(&soc_des.dump_sem)) {
			BB_PRINT_ERR("dump_thread down sem fail\n");
			continue;
		}

		if (RDR_AUDIO_NOC_MODID == soc_des.modid && is_dsp_power_on()) {
			BB_PRINT_PN("notify hifi save tcm\n");
			hisi_rdr_nmi_notify_hifi();
		}

		BB_PRINT_DBG("begin to dump soc hifi log\n");
		dump_hifi(soc_des.pathname);
		BB_PRINT_DBG("end dump soc hifi log\n");

		if (soc_des.dumpdone_cb) {
			BB_PRINT_DBG("begin dump soc hifi done callback, modid: 0x%x\n", soc_des.modid);
			soc_des.dumpdone_cb(soc_des.modid, (unsigned long long)RDR_HIFI);
			BB_PRINT_DBG("end dump soc hifi done callback\n");
		}
	}

	BB_PRINT_END();

	return 0;
}

static int irq_handler_thread(void *arg)
{
	BB_PRINT_START();

	while (!kthread_should_stop()) {
		if (down_interruptible(&soc_des.handler_sem)) {
			BB_PRINT_ERR("irq_handler_thread down sem fail\n");
			continue;
		}

		BB_PRINT_PN("%s():[sochifi timestamp: %u]sochifi watchdog coming\n", __func__, HIFI_STAMP);
		usbaudio_ctrl_hifi_reset_inform();
		hifi_usb_hifi_reset_inform();
		if (is_dsp_power_on()) {
			hisi_rdr_nmi_notify_hifi();
			hisi_rdr_remap_init();
		} else {
			BB_PRINT_ERR("hifi is power off, do not send nmi & remap\n");
		}

		hifireset_runcbfun(DRV_RESET_CALLCBFUN_RESET_BEFORE);

		BB_PRINT_PN("enter rdr process for sochifi watchdog\n");
		rdr_system_error(RDR_AUDIO_SOC_WD_TIMEOUT_MODID, 0, 0);
		BB_PRINT_PN("exit rdr process for sochifi watchdog\n");
	}

	BB_PRINT_END();

	return 0;
}

void rdr_audio_soc_dump(u32 modid, char *pathname, pfn_cb_dump_done pfb)
{
	BUG_ON(NULL == pathname);
	BUG_ON(NULL == pfb);

	BB_PRINT_START();

	soc_des.modid = modid;
	soc_des.dumpdone_cb = pfb;
	soc_des.pathname = pathname;

	up(&soc_des.dump_sem);

	BB_PRINT_END();

	return;
}

static int hisi_rdr_ipc_notify_lpm3(u32 *msg, int len)
{
	int ret = 0;
	int i;

	for (i = 0; i < len; i++)
		BB_PRINT_PN("rdr:[ap2lpm3 notifiy] msg[%d] = 0x%x\n", i, msg[i]);

	ret = RPROC_ASYNC_SEND(HISI_RPROC_LPM3_MBX17, msg, len);
	if (ret)
		BB_PRINT_ERR("%s(): send mesg to lpm3 fail\n", __func__);

	return ret;
}

static bool is_config_cma_secure_by_atf(void)
{
	struct hisi_hifi_cma_struct *dev = &hificma_dev;
	const char *cma_sec_config;

	if (!of_property_read_string(dev->device->of_node, "cma-sec-config", &cma_sec_config)) {
		if (!strncmp(cma_sec_config, "atf", strlen("atf"))) {
			BB_PRINT_PN("rdr:%s(): config cma secure by atf\n", __func__);
			return true;
		}
	} else {
		BB_PRINT_ERR("read cma-sec-config err\n");
	}

	BB_PRINT_PN("rdr:%s(): config cma secure not by atf\n", __func__);
	return false;
}

static int rdr_hifi_cma_alloc(phys_addr_t *hifi_addr)
{
	struct hisi_hifi_cma_struct *dev = &hificma_dev;
	phys_addr_t phys;
	struct page *page;
	unsigned int align_val = 0;

	if (of_property_read_u32(dev->device->of_node, "hisi-align", &align_val)) {
		BB_PRINT_ERR("read align val err\n");
		return -EINVAL;
	}

	page = dma_alloc_from_contiguous(dev->device, HIFI_CMA_IMAGE_SIZE >> PAGE_SHIFT, align_val);
	if (!page) {
		BB_PRINT_ERR("dma_alloc_from_contiguous (hifi) alloc err!\n");
		return -ENOMEM;
	}
	phys = page_to_phys(page);
	if (phys & (phys_addr_t)((0x1UL << align_val) - 1)) {
		BB_PRINT_ERR("align error, align val %d\n", align_val);
		return -EINVAL;
	}

	__dma_unmap_area(phys_to_virt(phys), HIFI_CMA_IMAGE_SIZE, DMA_BIDIRECTIONAL);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	change_secpage_range(phys, (unsigned long)phys_to_virt(phys),
		HIFI_CMA_IMAGE_SIZE, __pgprot(PROT_DEVICE_nGnRE));
#else
	create_mapping_late(phys, (unsigned long)phys_to_virt(phys),
		HIFI_CMA_IMAGE_SIZE, __pgprot(PROT_DEVICE_nGnRE));
#endif

	if (is_config_cma_secure_by_atf()) {
		if (-1 == atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID,
								phys, HIFI_CMA_IMAGE_SIZE,
								ACCESS_REGISTER_FN_SUB_ID_DDR_HIFI_SEC_OPEN)) {
			BB_PRINT_ERR("atfd_hisi_service_access_register_smc (hifi) set err!\n");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
			change_secpage_range(phys, (unsigned long)phys_to_virt(phys),
				HIFI_CMA_IMAGE_SIZE, __pgprot(PROT_NORMAL));
#else
			create_mapping_late(phys, (unsigned long)phys_to_virt(phys),
				HIFI_CMA_IMAGE_SIZE, __pgprot(PROT_NORMAL));
#endif
			(void)dma_release_from_contiguous(dev->device,
				phys_to_page(phys),
				(int)HIFI_CMA_IMAGE_SIZE >> PAGE_SHIFT);
			return -EACCES;
		}
	}

	*hifi_addr = phys;
	BB_PRINT_PN("rdr:%s(): address 0x%pK\n", __func__, (void *)phys);
	return 0;
}

static int rdr_hifi_cma_free(phys_addr_t addr)
{
	struct hisi_hifi_cma_struct *dev = (struct hisi_hifi_cma_struct *)&hificma_dev;

	if (addr == 0) {
		BB_PRINT_ERR("addr is null\n");
		return -EINVAL;
	}

	if (is_config_cma_secure_by_atf()) {
		if (-1 == atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID,
								addr, HIFI_CMA_IMAGE_SIZE,
								ACCESS_REGISTER_FN_SUB_ID_DDR_HIFI_SEC_CLOSE)) {
			BB_PRINT_ERR("atfd_hisi_service_access_register_smc (hifi) clear err!\n");
			return -EACCES;
		}
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	change_secpage_range(addr, (unsigned long)phys_to_virt(addr),
		HIFI_CMA_IMAGE_SIZE, __pgprot(PROT_NORMAL));
#else
	create_mapping_late(addr, (unsigned long)phys_to_virt(addr),
		HIFI_CMA_IMAGE_SIZE, __pgprot(PROT_NORMAL));
#endif
	(void)dma_release_from_contiguous(dev->device,
		phys_to_page(addr),
		(int)HIFI_CMA_IMAGE_SIZE >> PAGE_SHIFT);

	return 0;
}

static int get_hifi_image_size(unsigned int *size)
{
	int fd = 0;
	int cnt = 0;
	int ret = 0;
	char path[RDR_FNAME_LEN + 1] = {0};
	struct drv_hifi_image_head hifi_head = {{0}};
	mm_segment_t old_fs;

	if (flash_find_ptn(PART_FW_HIFI, path) < 0) {
		BB_PRINT_ERR("partion_name(hifi) is not in partion table!\n");
		return -EINVAL;
	}

	old_fs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/

	fd = (int)sys_open(path, O_RDWR, 0);
	if (fd < 0) {
		BB_PRINT_ERR("rdr:%s():open %s failed, return [%d]\n", __func__, path, fd);
		set_fs(old_fs);
		return -ENOENT;
	}

	cnt = (int)sys_read((unsigned int)fd, (char *)&hifi_head, sizeof(struct drv_hifi_image_head));
	if (cnt != (int)sizeof(struct drv_hifi_image_head)) {
		BB_PRINT_ERR("rdr:%s():read %s failed, return [%d]\n", __func__, path, cnt);
		ret = -EINVAL;
	}

	sys_close((unsigned int)fd);
	set_fs(old_fs);
	*size = hifi_head.image_size;
	BB_PRINT_PN("rdr:%s(): get_hifi_image_size %d\n", __func__, hifi_head.image_size);

	return ret;
}

static bool is_reload_hifi_by_secos(void)
{
	struct hisi_hifi_cma_struct *dev = &hificma_dev;
	const char * enable_status;

	if (!of_property_read_string(dev->device->of_node, "enable-status", &enable_status)) {
		if (!strncmp(enable_status, "true", strlen("true")))
			return true;
	} else {
		BB_PRINT_ERR("read enable-status err\n");
	}

	return false;
}

static int reload_hifi_by_secos(void)
{
	struct load_image_info loadinfo = {0};
	phys_addr_t hifi_addr = 0;
	char partion_name[10] = PART_FW_HIFI;
	int ret = 0;
	int free_ret = 0;

	ret = rdr_hifi_cma_alloc(&hifi_addr);
	if (ret != 0) {
		BB_PRINT_ERR("rdr:%s(): alloc cma buf err:%d", __func__, ret);
		return ret;
	}

	//TODO:invoke sec_os
	loadinfo.ecoretype = HIFI;
	loadinfo.image_addr = (unsigned long)hifi_addr;
	loadinfo.partion_name = partion_name;
	ret = get_hifi_image_size(&loadinfo.image_size);
	if (ret != 0) {
		BB_PRINT_ERR("rdr:%s(): get_hifi_image_size fail:%d, ret:%d\n",
			__func__, loadinfo.image_size, ret);
		goto end;
	}
	ret = bsp_load_and_verify_image(&loadinfo);
	if (ret < 0) {
		BB_PRINT_ERR("rdr:%s(): bsp_load_and_verify_image pre fail:%d\n",
				__func__, ret);
		goto end;
	}
	BB_PRINT_PN("rdr:%s(): bsp_load_and_verify_image success\n", __func__);

end:
	free_ret = rdr_hifi_cma_free(hifi_addr);
	if (free_ret != 0) {
		BB_PRINT_ERR("rdr:%s(): free cma buffer error:%d\n", __func__, free_ret);
		ret = free_ret;
	}

	return ret;
}

static int reset_hifi(void)
{
	unsigned int *hifi_power_status_addr = NULL;
	unsigned int msg = 0xdead;
	int ret = 0;

	BB_PRINT_START();

	if (is_reload_hifi_by_secos()) {
		BB_PRINT_PN("rdr:%s(): reload hifi by secos\n", __func__);
		ret = reload_hifi_by_secos();
		if (ret != 0) {
			BB_PRINT_ERR("%s(): secos reload hifi fail, ret:%d, reboot now\n", __func__, ret);
			BUG_ON(1);
		}
	}

	hifi_power_status_addr = ioremap_wc(DRV_DSP_POWER_STATUS_ADDR, (unsigned long)0x4);
	if (NULL == hifi_power_status_addr) {
		BB_PRINT_ERR("%s():DRV_DSP_POWER_STATUS_ADDR ioremap failed\n", __func__);
	} else {
		writel(DRV_DSP_POWER_OFF, hifi_power_status_addr);
		iounmap(hifi_power_status_addr);
	}

	msg = HIFI_DIE_NOTIFY_LPM3;
	ret = hisi_rdr_ipc_notify_lpm3(&msg, 1);
	BB_PRINT_PN("rdr:%s(): power off hifi %s\n", __func__, ret ? "fail" : "success\n");

	BB_PRINT_END();

	return ret;
}

/*Link used for hifi reset*/
struct sreset_mgr_lli *g_pmgr_hifireset_data;

/*****************************************************************************
 函 数 名  : reset_link_insert
 功能描述  : 将数据插入链表
 输入参数  :
			struct sreset_mgr_lli *plink, 链表指针
			struct sreset_mgr_lli *punit，待插入的节点指针
 输出参数  : 无
 返 回 值  : int
*****************************************************************************/
struct sreset_mgr_lli *reset_link_insert(struct sreset_mgr_lli *plink, struct sreset_mgr_lli *punit)
{
	struct sreset_mgr_lli *phead = plink;
	struct sreset_mgr_lli *ppose = plink;
	struct sreset_mgr_lli *ptail = plink;

	if (NULL == plink || NULL == punit) {
		BB_PRINT_ERR("%s: input params are not legitimate\n", __func__);
		return NULL;
	}

	while (NULL != ppose) {
		/*根据优先级插入到链表中*/
		if (ppose->cbfuninfo.priolevel > punit->cbfuninfo.priolevel) {
			if (phead == ppose) {
				punit->pnext = ppose;
				phead = punit;
			} else {
				ptail->pnext = punit;
				punit->pnext = ppose;
			}
			break;
		}
		ptail = ppose;
		ppose = ppose->pnext;
	}

	if (NULL == ppose)
		ptail->pnext = punit;

	return phead;
}

/*****************************************************************************
 函 数 名  : reset_do_regcbfunc
 功能描述  : 用于其它组件注册回调函数，处理Modem复位前后相关数据。
 输入参数  :
			struct sreset_mgr_lli *plink,管理链表，注意，允许为空.
			const char *pname, 组件注册的名字
			pdrv_reset_cbfun cbfun,    组件注册的回调函数
			int userdata,组件的私有数据
			Int Priolevel, 回调函数调用优先级 0-49，其中0-9 保留。
 输出参数  : 无
 返 回 值  : int
*****************************************************************************/
struct sreset_mgr_lli *reset_do_regcbfunc(struct sreset_mgr_lli *plink, const char *pname, pdrv_reset_cbfun pcbfun, int userdata, int priolevel)
{
	struct sreset_mgr_lli  *phead = plink;
	struct sreset_mgr_lli  *pmgr_unit = NULL;

	/*判断入参是否合法，不合法返回错误*/
	if ((NULL == pname)
		|| (NULL == pcbfun)
		|| (priolevel < RESET_CBFUNC_PRIO_LEVEL_LOWT || priolevel > RESET_CBFUNC_PRIO_LEVEL_HIGH)) {
		BB_PRINT_ERR("fail in ccore reset regcb,fail, name 0x%s, cbfun 0x%pK, prio %d\n", pname, pcbfun, priolevel);
		return NULL;
	}

	/*分配空间*/
	pmgr_unit = kmalloc(sizeof(*pmgr_unit), GFP_KERNEL);
	if (NULL != pmgr_unit) {
		memset((void *)pmgr_unit, 0, (sizeof(*pmgr_unit)));
		/*赋值*/
		strncpy(pmgr_unit->cbfuninfo.name, pname, (unsigned long)DRV_MODULE_NAME_LEN);
		pmgr_unit->cbfuninfo.priolevel = priolevel;
		pmgr_unit->cbfuninfo.userdata = userdata;
		pmgr_unit->cbfuninfo.cbfun = pcbfun;
	} else {
		BB_PRINT_ERR("pmgr_unit malloc fail!\n");
		return NULL;
	}

	/*第一次调用该函数，链表为空*/
	if (NULL == phead) {
		phead = pmgr_unit;
	} else {
	/*根据优先级插入链表*/
		phead = reset_link_insert(phead, pmgr_unit);
	}

	return phead;
}

/*****************************************************************************
 函 数 名  : hifireset_regcbfunc
 功能描述  : 用于其它组件注册回调函数，处理HIFI复位前后相关数据。
 输入参数  :
			const char *pname, 组件注册的名字
			pdrv_reset_cbfun cbfun,    组件注册的回调函数
			int userdata,组件的私有数据
			Int Priolevel, 回调函数调用优先级 0-49，其中0-9 保留。
 输出参数  : 无
 返 回 值  : int
*****************************************************************************/
int hifireset_regcbfunc(const char *pname, pdrv_reset_cbfun pcbfun, int userdata, int priolevel)
{
	g_pmgr_hifireset_data = reset_do_regcbfunc(g_pmgr_hifireset_data, pname, pcbfun, userdata, priolevel);
	BB_PRINT_PN("%s registered a cbfun for hifi reset\n", pname);

	return 0;
}

/*****************************************************************************
 函 数 名  :  hifireset_doruncbfun
 功能描述  : HIFI复位前后调用回调函数的函数。由于圈复杂度超标，所以这里封装函数
 输入参数  : enum DRV_RESET_CALLCBFUN_MOMENT eparam, 0 表示HIFI复位前；非零表示复位后。

 输出参数  : 无
 返 回 值  : 0, 成功，非0，失败
*****************************************************************************/
int hifireset_doruncbfun(const char *pname, enum DRV_RESET_CALLCBFUN_MOMENT eparam)
{
	int  iresult = BSP_RESET_OK;
	struct sreset_mgr_lli  *phead = g_pmgr_hifireset_data;

	BUG_ON(NULL == pname);

	/*不判断模块名字,按顺序执行*/
	if (strncmp(pname, RESET_CBFUN_IGNORE_NAME, strlen(RESET_CBFUN_IGNORE_NAME)) == 0) {
		while (NULL != phead) {
			if (NULL != phead->cbfuninfo.cbfun) {
				iresult = phead->cbfuninfo.cbfun(eparam, phead->cbfuninfo.userdata);

				if (BSP_RESET_OK != iresult) {
					/*如果返回失败，记录下组件名字,返回值*/
					BB_PRINT_ERR("fail to run cbfun of %s, at %d return %d\n", phead->cbfuninfo.name, eparam, iresult);
					break;
				}
				BB_PRINT_PN("run %s cb function 0x%pK\n", phead->cbfuninfo.name, phead->cbfuninfo.cbfun);
			}
			phead = phead->pnext;
		}
	} else {
	/*需要判断模块名字，执行指定的回调函数*/
		while (NULL != phead) {
			if (strncmp(pname, phead->cbfuninfo.name, strlen(phead->cbfuninfo.name)) == 0
				&& NULL != phead->cbfuninfo.cbfun) {
				iresult  = phead->cbfuninfo.cbfun(eparam, phead->cbfuninfo.userdata);
				BB_PRINT_PN("run %s cb function 0x%pK\n", phead->cbfuninfo.name, phead->cbfuninfo.cbfun);
				break;
			}
			phead = phead->pnext;
		}
	}

	if (BSP_RESET_OK != iresult) {
		if (NULL != phead) {
			BB_PRINT_ERR("fail to run cbfun of %s, at %d, return %d\n", phead->cbfuninfo.name, eparam, iresult);
		} else {
			BB_PRINT_ERR("fail to run cbfun, but phead or pname is null\n");
		}
	}

	return iresult;
}

/*****************************************************************************
 函 数 名  :  hifireset _runcbfun
 功能描述  : HIFI复位前后调用回调函数的函数。
 输入参数  : enum DRV_RESET_CALLCBFUN_MOMENT eparam, 0 表示HIFI复位前；非零表示复位后。

 输出参数  : 无
 返 回 值  : 0, 成功，非0，失败
*****************************************************************************/
int hifireset_runcbfun(enum DRV_RESET_CALLCBFUN_MOMENT eparam)
{
	int  iresult = 0;

	if (DRV_RESET_CALLCBFUN_RESET_BEFORE == eparam) {
		/*遍历回调函数链表，调用NAS的回调*/
		iresult = hifireset_doruncbfun("NAS_AT", eparam);
		if (BSP_RESET_OK != iresult) {
			/*如果返回失败，记录下组建name, 返回值，保存到文件*/
			goto return_error;
		}
	} else {
		/*遍历回调函数链表，调用回调函数*/
		iresult = hifireset_doruncbfun(RESET_CBFUN_IGNORE_NAME, eparam);
		if (BSP_RESET_OK != iresult)
			goto return_error;
	}
	BB_PRINT_PN("end of run cb functions for hifi reset at %d, %d\n", eparam, iresult);
	return BSP_RESET_OK;
return_error:
	return BSP_RESET_ERROR;
}

void rdr_audio_soc_reset(u32 modid, u32 etype, u64 coreid)
{
	int ret = 0;

	BB_PRINT_START();

	ret = reset_hifi_sec();
	BB_PRINT_PN("rdr:%s():reset hifi sec,%s\n", __func__, ret ? "fail" : "success\n");

	ret = reset_hifi();
	if (ret) {
		wake_unlock(&soc_des.rdr_wl);/*lint !e455*/
		BB_PRINT_ERR("rdr:%s():reset hifi error\n", __func__);
		return;
	}

/* todo :sochifi_watchdog_send_event undefined in dallas */
	sochifi_watchdog_send_event();
	hifireset_runcbfun(DRV_RESET_CALLCBFUN_RESET_AFTER);

	wake_unlock(&soc_des.rdr_wl);/*lint !e455*/

	BB_PRINT_END();

	return;
}

static irqreturn_t soc_wtd_irq_handler(int irq, void *data)
{
	writel(DRV_WATCHDOG_UNLOCK_NUM, soc_des.lock_addr);
	writel(DRV_WATCHDOG_INTCLR_NUM, soc_des.intclr_addr);
	writel(DRV_WATCHDOG_CONTROL_DISABLE, soc_des.control_addr);
	writel(DRV_WATCHDOG_LOCK_NUM, soc_des.lock_addr);

	wake_lock(&soc_des.rdr_wl);

	up(&soc_des.handler_sem);

	return IRQ_HANDLED;/*lint !e454*/
}

static unsigned int rdr_get_hifi_watchdog_irq_num(void)
{
	unsigned int irq_num = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	struct device_node *dev_node = NULL;

	dev_node = of_find_compatible_node(NULL, NULL, WATCHDOG_DTS_COMP_NAME);
	if (!dev_node) {
		BB_PRINT_ERR("rdr:%s():find device node(sochifi-watchdog) by compatible failed\n", __func__);
		return 0;
	}

	irq_num = irq_of_parse_and_map(dev_node, 0);
	if (0 == irq_num) {
		BB_PRINT_ERR("rdr:%s:irq parse and map failed, irq:%u\n", __func__, irq_num);
		return 0;
	}
#else
	irq_num = SOC_WDT_TIMEOUT_IRQ_NUM;
#endif

	return irq_num;
}


int rdr_audio_soc_init(void)
{
	int ret = 0;

	BB_PRINT_START();

	soc_des.modid = ~0;
	soc_des.wdt_irq_num = 0;
	soc_des.pathname = NULL;
	soc_des.lock_addr = NULL;
	soc_des.control_addr = NULL;
	soc_des.intclr_addr = NULL;
	soc_des.dumpdone_cb = NULL;

	sema_init(&soc_des.dump_sem, 0);
	sema_init(&soc_des.handler_sem, 0);
	wake_lock_init(&soc_des.rdr_wl, WAKE_LOCK_SUSPEND, "rdr_sochifi");
	soc_des.kdump_task = NULL;
	soc_des.khandler_task = NULL;

	soc_des.lock_addr = (u32 *)ioremap((unsigned long)DRV_WATCHDOG_LOCK, (unsigned long)0x4);
	if (!soc_des.lock_addr) {
		BB_PRINT_ERR("rdr: remap watchdog lock addr fail\n");
		ret = -ENOMEM;
		goto error;
	}

	soc_des.control_addr = (u32 *)ioremap((unsigned long)DRV_WATCHDOG_CONTROL, (unsigned long)0x4);
	if (!soc_des.control_addr) {
		BB_PRINT_ERR("rdr: remap watchdog control addr fail\n");
		ret = -ENOMEM;
		goto error;
	}

	soc_des.intclr_addr = (u32 *)ioremap((unsigned long)DRV_WATCHDOG_INTCLR, (unsigned long)0x4);
	if (!soc_des.intclr_addr) {
		BB_PRINT_ERR("rdr: remap watchdog interrupt clear addr fail\n");
		ret = -ENOMEM;
		goto error;
	}

	soc_des.kdump_task = kthread_run(dump_thread, NULL, "rdr_audio_soc_thread");
	if (!soc_des.kdump_task) {
		BB_PRINT_ERR("create rdr soc dump thead fail\n");
		ret = -EBUSY;
		goto error;
	}

	soc_des.khandler_task = kthread_run(irq_handler_thread, NULL, "rdr_audio_soc_wtd_irq_handler_thread");
	if (!soc_des.khandler_task) {
		BB_PRINT_ERR("create rdr soc wtd irq handler thead fail\n");
		ret = -EBUSY;
		goto error;
	}

	soc_des.wdt_irq_num = rdr_get_hifi_watchdog_irq_num();
	if (soc_des.wdt_irq_num == 0) {
		BB_PRINT_ERR("rdr: get hifi watchdog irq num fail\n");
		goto error;
	}

	ret = request_irq(soc_des.wdt_irq_num, soc_wtd_irq_handler, (unsigned long)0, "soc wdt handler", NULL);
	if (ret) {
		BB_PRINT_ERR("request_irq soc_wdt_irq_handler failed! return 0x%x\n", ret);
		goto error;
	}

	BB_PRINT_END();

	return ret;

error:
	if (soc_des.kdump_task != NULL) {
		kthread_stop(soc_des.kdump_task);
		up(&soc_des.dump_sem);
		soc_des.kdump_task = NULL;
	}

	if (soc_des.khandler_task != NULL) {
		kthread_stop(soc_des.khandler_task);
		up(&soc_des.handler_sem);
		soc_des.khandler_task = NULL;
	}

	wake_lock_destroy(&soc_des.rdr_wl);

	if (NULL != soc_des.lock_addr) {
		iounmap(soc_des.lock_addr);
		soc_des.lock_addr = NULL;
	}

	if (NULL != soc_des.control_addr) {
		iounmap(soc_des.control_addr);
		soc_des.control_addr = NULL;
	}

	if (NULL != soc_des.intclr_addr) {
		iounmap(soc_des.intclr_addr);
		soc_des.intclr_addr = NULL;
	}

	BB_PRINT_END();

	return ret;
}

void rdr_audio_soc_exit(void)
{
	BB_PRINT_START();

	if (soc_des.wdt_irq_num > 0)
		free_irq(soc_des.wdt_irq_num, NULL);

	if (soc_des.kdump_task != NULL) {
		kthread_stop(soc_des.kdump_task);
		up(&soc_des.dump_sem);
		soc_des.kdump_task = NULL;
	}

	if (soc_des.khandler_task != NULL) {
		kthread_stop(soc_des.khandler_task);
		up(&soc_des.handler_sem);
		soc_des.khandler_task = NULL;
	}

	wake_lock_destroy(&soc_des.rdr_wl);

	if (NULL != soc_des.lock_addr) {
		iounmap(soc_des.lock_addr);
		soc_des.lock_addr = NULL;
	}

	if (NULL != soc_des.control_addr) {
		iounmap(soc_des.control_addr);
		soc_des.control_addr = NULL;
	}

	if (NULL != soc_des.intclr_addr) {
		iounmap(soc_des.intclr_addr);
		soc_des.intclr_addr = NULL;
	}

	BB_PRINT_END();

	return;
}

static int hisi_hifi_cma_probe(struct platform_device *pdev)
{
	struct hisi_hifi_cma_struct *dev = (struct hisi_hifi_cma_struct *)&hificma_dev;
	int ret = 0;

	memset(dev, 0, sizeof(struct hisi_hifi_cma_struct));
	dev->device = &(pdev->dev);

	ret = of_reserved_mem_device_init(dev->device);
	if (ret) {
		dev->device = NULL;
		BB_PRINT_ERR("hifi cma device init failed! return 0x%x\n", ret);
	}

	return ret;
}

static int hisi_hifi_cma_remove(struct platform_device *pdev)
{
	struct hisi_hifi_cma_struct *dev = (struct hisi_hifi_cma_struct *)&hificma_dev;

	of_reserved_mem_device_release(dev->device);
	memset(dev, 0, sizeof(struct hisi_hifi_cma_struct));

	return 0;
}


static struct of_device_id hisi_hifi_cma_of_match[] = {
	{
		.compatible = DTS_COMP_HIFICMA_NAME,
		.data = NULL,
	},
	{ },/*lint !e785*/
};
MODULE_DEVICE_TABLE(of, hisi_hifi_cma_of_match);

static struct platform_driver hisi_hifi_cma_driver = {
	.driver = {
		.owner      = THIS_MODULE,/*lint !e64*/
		.name       = "hifi-cma",
		.of_match_table = of_match_ptr(hisi_hifi_cma_of_match),
	},/*lint !e785*/
	.probe  = hisi_hifi_cma_probe,
	.remove = hisi_hifi_cma_remove,
};/*lint !e785*/

static int __init hisi_hifi_cma_init(void)
{
	return platform_driver_register(&hisi_hifi_cma_driver);/*lint !e64*/
}
/*lint -e528 -e753*/
subsys_initcall(hisi_hifi_cma_init);

static void __exit hisi_hifi_cma_exit(void)
{
	platform_driver_unregister(&hisi_hifi_cma_driver);
}
module_exit(hisi_hifi_cma_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hisilicon hifi reset cma module");

