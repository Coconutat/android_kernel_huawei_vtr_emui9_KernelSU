/*
 * audio rdr adpter.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/thread_info.h>
#include <linux/notifier.h>
#include <linux/syscalls.h>
#include <linux/errno.h>

#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_ap_hook.h>

#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_hisi_audio_adapter.h"
#include "rdr_hisi_audio_codec.h"
#include "rdr_hisi_audio_soc.h"

/*lint -e730*/


struct rdr_audio_des_s {
	struct rdr_register_module_result audio_info;

	char soc_pathname[RDR_FNAME_LEN];
	char codec_pathname[RDR_FNAME_LEN];
};
static struct rdr_audio_des_s s_rdr_audio_des;

enum {
	TASK = 0,
	INTERRUPT,
	REGION,
	TARGET_BUTT
};

struct cpuview_task_info {
	unsigned int task_no;
	char task_name[32];
};

static struct cpuview_task_info cpuview_codec_task_info[] =
{
	{0, "FID_RT"},
	{1, "FID_NORMAL"},
	{2, "FID_LOW"},
	{3, "IDLE"},
	{4, "UNDEFINE"},
};

static struct cpuview_task_info cpuview_soc_task_info[] =
{
	{0, "FID_RT"},
	{1, "FID_NORMAL"},
	{2, "FID_LOW"},
	{3, "UNDEFINE"},
	{4, "IDLE"},
};

struct cpuview_interrupt_info {
	unsigned int interrupt_no;
	char interrupt_name[32];
};

static struct cpuview_interrupt_info cpuview_soc_int_info[] =
{
	{0, "NMI_0"},
	{1, "SoftINT_1"},
	{2, "RPROC_2"},
	{3, "AP2HIFI_3"},
	{4, "INT_4"},
	{5, "HIFI_Timer0_5"},
	{6, "HIFI_Timer1_6"},
	{7, "INT_7"},
	{8, "ASP_Timer0_8"},
	{9, "ASP_Timer1_9"},
	{10, "INT_10"},
	{11, "INT_11"},
	{12, "INT_12"},
	{13, "INT_13"},
	{14, "INT_14"},
	{15, "OCBC_15"},
	{16, "INT_16"},
	{17, "DMAC_S_17"},
	{18, "DMAC_NS_18"},
	{19, "INT_19"},
	{20, "INT_20"},
	{21, "INT_21"},
	{22, "WatchDog_22"},
	{23, "LPM3_2_HIFI_23"},
	{24, "INT_24"},
	{25, "CCORE_2_HIFI_25"},
	{26, "BBE16_2_HIFI_26"},
	{27, "INT_27"},
	{28, "INT_28"},
	{29, "WriteErr_29"},
	{30, "INT_30"},
	{31, "INT_31"},
};

static struct cpuview_interrupt_info cpuview_codec_int_info[] =
{
	{0, "NMI_0"},
	{1, "SoftINT_1"},
	{2, "INT_2"},
	{3, "INT_3"},
	{4, "INT_4"},
	{5, "HIFI_Timer0_5"},
	{6, "HIFI_Timer1_6"},
	{7, "INT_7"},
	{8, "INT_8"},
	{9, "INT_9"},
	{10, "INT_10"},
	{11, "INT_11"},
	{12, "DMAC_12"},
	{13, "MAD_13"},
	{14, "CMD_14"},
	{15, "CFG_CLK_SW_15"},
	{16, "Timer0_0_16"},
	{17, "Timer0_1_17"},
	{18, "Timer0_1_18"},
	{19, "Timer0_1_19"},
	{20, "GPIO0_20"},
	{21, "GPIO0_21"},
	{22, "GPIO0_22"},
	{23, "GPIO0_23"},
	{24, "Dlock_24"},
	{25, "UART_25"},
	{26, "CFG_26"},
	{27, "INT_27"},
	{28, "INT_28"},
	{29, "WriteErr_29"},
	{30, "INT_30"},
	{31, "INT_31"},
};

struct cpuview_region_info {
	unsigned int region_no;
	char region_name[64];
};

static struct cpuview_region_info cpuview_soc_region_info[] =
{
	{0, "UCOM_WFI"},
	{1, "UCOM_DRF"},
	{2, "UCOM_POWERDOWN"},
	{3, "VOS_INIT"},
	{4, "TEXT_CHECK"},
	{5, "TASK_PROC"},
	{6, "TASK_SWITCH_FLL"},
	{7, "UNDEFINE"},
	{8, "AUDIO_PCM_UPDATE_BUFF_PLAY"},
	{9, "AUDIO_PCM_UPDATE_BUFF_CAPTURE"},
	{10, "AUDIO_DSP_IN_EFFECT_PROCESS"},
	{11, "AUDIO_DSP_OUT_EFFECT_PROCESS"},
	{12, "UNDEFINE"},
	{13, "UNDEFINE"},
	{14, "AUDIO_PLAYER_DECODE"},
	{15, "AUDIO_PLAYER_SRC"},
	{16, "AUDIO_PLAYER_DTS"},
	{17, "WAKEUP_PCM_UPDATE_BUFF"},
	{18, "WAKEUP_DECODE_PROC"},
	{19, "WAKEUP_DECODE_MSG"},
	{20, "UNDEFINE"},
	{21, "UNDEFINE"},
	{22, "UNDEFINE"},
	{23, "UNDEFINE"},
	{24, "VOICE_PROC_MICIN"},
	{25, "VOICE_PROC_SPKOUT"},
	{26, "VOICE_ENCODE"},
	{27, "VOICE_DECODE"},
	{28, "UNDEFINE"},
};

static struct cpuview_region_info cpuview_codec_region_info[] =
{
	{0, "ANC_MLIB_PROC"},
	{1, "PLL_SWITCH_WFI"},
	{2, "PA_MLIB_PROC"},
	{3, "INIT"},
	{4, "FIRST_WFI"},
	{5, "IDLE_WFI"},
	{6, "WAKEUP_MLIB_PROC"},
	{7, "SEND_PLL_SW_CNF"},
	{8, "RECEIVE_MSG"},
	{9, "SEND_MSG_CNF"},
	{10, "SEND_PWRON_CNF"},
	{11, "HISI_WAKEUP_DECODE_PROC"},
	{12, "HISI_WAKEUP_DECODE_MSG"},
};

struct cpuview_total_info {
	struct cpuview_task_info * task_info;
	struct cpuview_interrupt_info * int_info;
	struct cpuview_region_info *region_info;
	unsigned int task_num;
	unsigned int int_num;
	unsigned int region_num;
};

static void parse_single_cpuview_info(char *info_buf, const unsigned int buf_len,
	struct cpuview_slice_record *record, struct cpuview_total_info *total_info)
{
	memset(info_buf, 0, buf_len);/* unsafe_function_ignore: memset */

	switch(record->target) {
	case TASK:
		if (record->target_id < total_info->task_num) {
			snprintf(info_buf, buf_len, "%-15s %-40s", "TASK",
				total_info->task_info[record->target_id].task_name);
		} else {
			snprintf(info_buf, buf_len, "%-15s %-40s", "TASK", "UNDEFINE");
		}

		break;
	case INTERRUPT:
		if (record->target_id < total_info->int_num) {
			snprintf(info_buf, buf_len, "%-15s %-40s", "INT",
				total_info->int_info[record->target_id].interrupt_name);
		} else {
			snprintf(info_buf, buf_len, "%-15s %-40s", "INT", "UNDEFINE");
		}

		break;
	case REGION:
		if (record->target_id < total_info->region_num) {
			snprintf(info_buf, buf_len, "%-15s %-40s", "REGION",
				total_info->region_info[record->target_id].region_name);
		} else {
			snprintf(info_buf, buf_len, "%-15s %-40s", "REGION", "UNDEFINE");
		}

		break;
	default:
		snprintf(info_buf, buf_len, "%-55s %u","UNDEFINE TARGET", record->target_id);
		break;
	}

	snprintf(info_buf + strlen(info_buf), buf_len - strlen(info_buf), "%-15s %-11u",
		record->action == 0 ? "ENTER" : "EXIT",
		record->time_stamp);

	return;
}

int parse_hifi_cpuview(char *original_buf, unsigned int original_buf_size,
	char *parsed_buf, unsigned int parsed_buf_size, unsigned int core_type)
{
	int ret = 0;
	unsigned short index;
	struct cpuview_details *details = NULL;
	struct cpuview_total_info total_info;
	unsigned int cpuview_info_num;
	unsigned int original_data_size, parsed_data_size;

	if (!original_buf || !parsed_buf) {
		BB_PRINT_ERR("input data buffer is null\n");
		return -ENOMEM;
	}

	if (core_type == CODECDSP) {
		cpuview_info_num = CODEC_CPUVIEW_DETAIL_MAX_NUM;
		parsed_data_size = PARSER_CODEC_CPUVIEW_LOG_SIZE;
		total_info.int_info = cpuview_codec_int_info;
		total_info.int_num = ARRAY_SIZE(cpuview_codec_int_info);
		total_info.region_info = cpuview_codec_region_info;
		total_info.region_num = ARRAY_SIZE(cpuview_codec_region_info);
		total_info.task_info = cpuview_codec_task_info;
		total_info.task_num = ARRAY_SIZE(cpuview_codec_task_info);
	} else if (core_type == SOCHIFI){
		cpuview_info_num = SOCHIFI_CPUVIEW_DETAIL_MAX_NUM;
		parsed_data_size = PARSER_SOCHIFI_CPUVIEW_LOG_SIZE;
		total_info.int_info = cpuview_soc_int_info;
		total_info.int_num = ARRAY_SIZE(cpuview_soc_int_info);
		total_info.region_info = cpuview_soc_region_info;
		total_info.region_num = ARRAY_SIZE(cpuview_soc_region_info);
		total_info.task_info = cpuview_soc_task_info;
		total_info.task_num = ARRAY_SIZE(cpuview_soc_task_info);
	} else {
		BB_PRINT_ERR("input core type error, %d\n", core_type);
		return -EINVAL;
	}

	original_data_size = sizeof(struct cpuview_details) + cpuview_info_num * sizeof(struct cpuview_slice_record);
	if (original_buf_size < original_data_size || parsed_buf_size < parsed_data_size) {
		BB_PRINT_ERR("input buf size error, original_buf_size:%u, parsed_buf_size:%u\n",
			original_buf_size, parsed_buf_size);
		return -EINVAL;
	}

	memset(parsed_buf, 0, parsed_buf_size);/* unsafe_function_ignore: memset */
	details = (struct cpuview_details *)original_buf;
	index = details->curr_idx;
	snprintf(parsed_buf, parsed_data_size, "\n\n/*********[cpuview info begin]*********/\n\n");

	if (index < cpuview_info_num) {
		unsigned int i;
		char single_info[CPUVIEW_ONE_INFO_LEN];
		struct cpuview_slice_record *slice;

		snprintf(parsed_buf + strlen(parsed_buf), parsed_data_size - strlen(parsed_buf),
			"Target         Target Id                                ACTION          OrigTS \n");

		for (i = 0; i < cpuview_info_num; i++) {
			slice = details->records + index;
			parse_single_cpuview_info(single_info, CPUVIEW_ONE_INFO_LEN, slice, &total_info);

			snprintf(parsed_buf + strlen(parsed_buf), parsed_data_size - strlen(parsed_buf),
				"%s \n", single_info);

			index++;
			if (index == cpuview_info_num) {
				index = 0;
			}
		}
	} else {
		BB_PRINT_ERR("record index error, %d\n", index);
		ret = -EINVAL;
	}

	snprintf(parsed_buf + strlen(parsed_buf), parsed_data_size - strlen(parsed_buf),
		"\n\n/*********[cpuview info end]*********/\n\n");

	return ret;
}

int parse_hifi_trace(char *original_data, unsigned int original_data_size, char *parsed_data, unsigned int parsed_data_size, unsigned int core_type)
{
	unsigned int i;
	unsigned int stack_depth;
	unsigned int stack_top;
	unsigned int *stack;

	if (NULL == original_data || NULL == parsed_data) {
	    BB_PRINT_ERR("input data buffer is null\n");
	    return -EINVAL;
	}

	memset(parsed_data, 0, parsed_data_size);/* unsafe_function_ignore: memset */
	snprintf(parsed_data, parsed_data_size, "\n\n/*********[trace info begin]*********/\n\n");
	stack = (unsigned int *)original_data + 8;
	stack_top = *((unsigned int *)original_data + 4);
	stack_depth = (original_data_size - 0x20)/4;

	snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data), "panic addr:0x%08x, cur_pc:0x%08x, pre_pc:0x%08x, cause:0x%08x\n",
		*(unsigned int *)original_data, *((unsigned int *)original_data + 1), *((unsigned int *)original_data + 2), *((unsigned int *)original_data + 3));
	for (i = 0; i < stack_depth; i += 4) {
		snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data), "addr:%08x    %08x    %08x    %08x    %08x\n",
			stack_top+i*4, *(stack + i), *(stack + 1 + i), *(stack + 2 + i), *(stack + 3 + i));
	}

	snprintf(parsed_data + strlen(parsed_data), parsed_data_size - strlen(parsed_data), "\n\n/*********[trace info end]*********/\n\n");

	return 0;
}


static int create_dir(char *path)
{
	long fd;

	BUG_ON(NULL == path);

	fd = sys_access(path, 0);
	if (fd) {
		BB_PRINT_PN("need create dir %s\n", path);
		fd = sys_mkdir(path, 0770);
		if (fd < 0) {
			BB_PRINT_ERR("create dir %s failed, fd:%ld\n", path, fd);
			return -EBADF;
		}
		BB_PRINT_PN("create dir %s successed, fd: %ld\n", path, fd);
	}

	/* change dir limit root-system */
	if (bbox_chown((const char *)path, ROOT_UID, SYSTEM_GID, false)) {/*lint !e747*/
		BB_PRINT_ERR("[%s], chown %s dir failed\n", __func__, path);
	}

	return 0;
}

static int rdr_audio_create_dir(const char *path)
{
	char cur_path[RDR_FNAME_LEN];
	int index = 0;

	memset(cur_path, 0, RDR_FNAME_LEN);
	if (*path != '/')
		return -ENOTDIR;

	cur_path[index++] = *path++;

	while (*path != '\0') {
		if (*path == '/') {
			if (create_dir(cur_path))
				return -ENOENT;
		}
		cur_path[index] = *path;
		path++;
		index++;
	}

	return 0;
}

static int rdr_audio_loopwrite_open(char *name, unsigned int *pfd)
{
	int ret;
	int fd;

	BUG_ON(NULL == pfd);
	BUG_ON(NULL == name);

	ret = rdr_audio_create_dir(name);
	if (ret) {
		BB_PRINT_ERR("create dir fail, name: %s, ret: %d\n", name, ret);
		return ret;
	}

	/* sys_access() return 0:success, return ~0:error */
	if (!(sys_access(name, 0))) {
		ret = (int)sys_unlink(name);
		if (ret) {
			BB_PRINT_ERR("remove file fail, name: %s, ret: %d\n", name, ret);
			return ret;
		}
	}

	fd = (int)sys_open(name, O_CREAT | O_RDWR, 0660);
	if (fd < 0) {
		BB_PRINT_ERR("create and open file fail, name: %s, fd: %d\n", name, fd);
		return -EBADF;
	}

	*pfd = (unsigned int)fd;

	return ret;
}

static int rdr_audio_loopwrite_append(unsigned int fd, void *address, u32 length)
{
	long bytes;

	BUG_ON(NULL == address);
	BUG_ON(0 == length);

	bytes = sys_write(fd, address, (unsigned long)length);
	if (bytes != length) {
		BB_PRINT_ERR("write data failed, ret: %ld\n", bytes);
		return -EINVAL;
	}

	return 0;
}

static void rdr_audio_loopwrite_close(unsigned int fd)
{
	long ret;

	ret = sys_fsync(fd);
	if (ret < 0)
		pr_err("[%s]sys_fsync failed, ret is %ld\n", __func__, ret);

	ret = sys_close(fd);
	if (ret)
		BB_PRINT_ERR("close file failed, ret: %ld\n", ret);

	return;
}

int rdr_audio_write_file(char *name, char *data, u32 size)
{
	int ret;
	unsigned int fd = 0;
	mm_segment_t oldfs;

	BUG_ON(NULL == name);
	BUG_ON(NULL == data);
	BUG_ON(0 == size);

	oldfs = get_fs();/*lint !e501*/
	set_fs(KERNEL_DS);/*lint !e501*/

	ret = rdr_audio_loopwrite_open(name, &fd);
	if (ret) {
		BB_PRINT_ERR("open %s fail\n", name);
		set_fs(oldfs);
		return ret;
	}

	ret = rdr_audio_loopwrite_append(fd, data, size);
	if (ret)
		BB_PRINT_ERR("write %s fail\n", name);

	/* change file limit root-system */
	if (bbox_chown((const char *)name, ROOT_UID, SYSTEM_GID, false)) {/*lint !e747*/
		BB_PRINT_ERR("[%s], chown %s file failed\n", __func__, name);
	}

	rdr_audio_loopwrite_close(fd);

	set_fs(oldfs);

	return ret;
}

void rdr_audio_dump_log(u32 modid, u32 etype, u64 coreid,
				char *pathname, pfn_cb_dump_done pfn_cb)
{
	if (!pathname) {
		BB_PRINT_ERR("input path name is NULL\n");
		return;
	}

	if (!pfn_cb) {
		BB_PRINT_ERR("input dump done cb is NULL\n");
		return;
	}

	BB_PRINT_DBG(" ====================================\n");
	BB_PRINT_DBG(" modid:          [0x%x]\n",   modid);
	BB_PRINT_DBG(" coreid:         [0x%llx]\n", coreid);
	BB_PRINT_DBG(" exce tpye:      [0x%x]\n",   etype);
	BB_PRINT_DBG(" path name:      [%s]\n",     pathname);
	BB_PRINT_DBG(" dump start:     [0x%pK]\n", (void *)s_rdr_audio_des.audio_info.log_addr);
	BB_PRINT_DBG(" dump len:       [%d]\n",     s_rdr_audio_des.audio_info.log_len);
	BB_PRINT_DBG(" nve:            [0x%llx]\n", s_rdr_audio_des.audio_info.nve);
	BB_PRINT_DBG(" callback:       [0x%pK]\n",   pfn_cb);
	BB_PRINT_DBG(" ====================================\n");

	if (((modid >= (unsigned int)RDR_AUDIO_SOC_MODID_START) && (modid <= RDR_AUDIO_SOC_MODID_END))
		|| ((modid >= RDR_AUDIO_REBOOT_MODID_START) && (modid <= RDR_AUDIO_REBOOT_MODID_END))) {
		snprintf(s_rdr_audio_des.soc_pathname, RDR_FNAME_LEN, "%s", pathname);
		rdr_audio_soc_dump(modid, s_rdr_audio_des.soc_pathname, pfn_cb);
	} else if ((modid >= RDR_AUDIO_CODEC_MODID_START) && (modid <= RDR_AUDIO_CODEC_MODID_END)) {
		snprintf(s_rdr_audio_des.codec_pathname, RDR_FNAME_LEN, "%s", pathname);
		rdr_audio_codec_dump(modid, s_rdr_audio_des.codec_pathname, pfn_cb);
	} else if (modid >= (unsigned int)HISI_BB_MOD_CP_START && modid <= (unsigned int)HISI_BB_MOD_CP_END) {
		snprintf(s_rdr_audio_des.soc_pathname, RDR_FNAME_LEN, "%s", pathname);
		BB_PRINT_PN("modem reset soc hifi dump = %s, begin\n", s_rdr_audio_des.soc_pathname);
		rdr_audio_soc_dump(modid, s_rdr_audio_des.soc_pathname, pfn_cb);
		BB_PRINT_PN("modem reset soc hifi dump = %s, end\n", s_rdr_audio_des.soc_pathname);
	} else {
		BB_PRINT_ERR("mod id is invalide: 0x%x[soc:0x%x - 0x%x, codec: 0x%x - 0x%x]\n"
						, modid
						, RDR_AUDIO_SOC_MODID_START, RDR_AUDIO_SOC_MODID_END
						, RDR_AUDIO_CODEC_MODID_START, RDR_AUDIO_CODEC_MODID_END);
	}

	return;
}

void rdr_audio_reset(u32 modid, u32 etype, u64 coreid)
{
	BB_PRINT_DBG(" ====================================\n");
	BB_PRINT_DBG(" modid:		   [0x%x]\n",	modid);
	BB_PRINT_DBG(" coreid:		   [0x%llx]\n", coreid);
	BB_PRINT_DBG(" exce tpye:	   [0x%x]\n",	etype);
	BB_PRINT_DBG(" ====================================\n");

	if (modid >= (unsigned int)RDR_AUDIO_SOC_MODID_START && modid <= RDR_AUDIO_SOC_MODID_END) {
		rdr_audio_soc_reset(modid, etype, coreid);
	} else if (modid >= RDR_AUDIO_CODEC_MODID_START && modid <= RDR_AUDIO_CODEC_MODID_END) {
		rdr_audio_codec_reset(modid, etype, coreid);
	} else if (modid >= RDR_AUDIO_REBOOT_MODID_START && modid <= RDR_AUDIO_REBOOT_MODID_END){
		/* system will be reboot, do nothing here */
		BB_PRINT_ERR("audio NOC exception, system will be reboot\n");
	} else {
		BB_PRINT_ERR("mod id is invalide: 0x%x[soc:0x%x - 0x%x, codec: 0x%x - 0x%x]\n"
						, modid
						, RDR_AUDIO_SOC_MODID_START, RDR_AUDIO_SOC_MODID_END
						, RDR_AUDIO_CODEC_MODID_START, RDR_AUDIO_CODEC_MODID_END);
	}

	return;
}

static int rdr_audio_register_core(void)
{
	int ret;
	struct rdr_module_ops_pub module_ops;

	BB_PRINT_START();

	module_ops.ops_dump = rdr_audio_dump_log;
	module_ops.ops_reset = rdr_audio_reset;

	/* <0 error, >=0 success */
	ret = rdr_register_module_ops((u64)RDR_HIFI, &module_ops, &s_rdr_audio_des.audio_info);
	if (ret < 0) {
		BB_PRINT_ERR("rdr register hifi module ops error\n");
		ret = -EBUSY;
	} else {
		ret = 0;
	}

	BB_PRINT_END();

	return ret;
}

static int rdr_audio_register_exception(void)
{
	int ret = 0;
	struct rdr_exception_info_s einfo;

	BB_PRINT_START();

	/* ===sochifi exception register begin=== */
	memset(&einfo, 0, sizeof(struct rdr_exception_info_s));
	einfo.e_modid = (unsigned int)RDR_AUDIO_SOC_MODID_START;
	einfo.e_modid_end = RDR_AUDIO_SOC_MODID_END;
	einfo.e_process_priority = RDR_WARN;
	einfo.e_reboot_priority = RDR_REBOOT_NO;

	/* 标志位e_notify_core_mask中填0, RDR框架中会判断是否为0, 是0时不启动RDR框架中的导出LOG操作, 确认人:刘海龙 */
	einfo.e_notify_core_mask = RDR_HIFI;
	einfo.e_reset_core_mask = RDR_HIFI;
	einfo.e_reentrant = (unsigned int)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = SOCHIFI_S_EXCEPTION;
	einfo.e_upload_flag = (unsigned int)RDR_UPLOAD_YES;
	einfo.e_from_core = RDR_HIFI;
	memcpy(einfo.e_from_module, "RDR_SOCHIFI_WATCHDOG", sizeof("RDR_SOCHIFI_WATCHDOG"));
	memcpy(einfo.e_desc, "RDR_SOCHIFI watchdog timeout.",
			sizeof("RDR_SOCHIFI watchdog timeout."));

	/* error return 0, ok return modid */
	if (!rdr_register_exception(&einfo)) {
		BB_PRINT_ERR("regist audio soc exception fail\n");
		ret = -EBUSY;
	} else {
		ret = 0;
	}
	/* ===sochifi exception register end=== */

	/* ===codechifi exception register begin=== */
	memset(&einfo, 0, sizeof(struct rdr_exception_info_s));
	einfo.e_modid = RDR_AUDIO_CODEC_MODID_START;
	einfo.e_modid_end = RDR_AUDIO_CODEC_MODID_END;
	einfo.e_process_priority = RDR_WARN;
	einfo.e_reboot_priority = RDR_REBOOT_NO;
	einfo.e_notify_core_mask = RDR_HIFI;
	einfo.e_reset_core_mask = RDR_HIFI;
	einfo.e_reentrant = (unsigned int)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = CODECHIFI_S_EXCEPTION;
	einfo.e_upload_flag = (unsigned int)RDR_UPLOAD_YES;
	einfo.e_from_core = RDR_HIFI;
	memcpy(einfo.e_from_module, "RDR_CODECHIFI_WATCHDOG", sizeof("RDR_CODECHIFI_WATCHDOG"));
	memcpy(einfo.e_desc, "RDR_CODECHIFI watchdog timeout.",
			sizeof("RDR_CODECHIFI watchdog timeout."));

	/* error return 0, ok return modid */
	if (!rdr_register_exception(&einfo)) {
		BB_PRINT_ERR("register audio codec exception fail\n");
		ret = -EBUSY;
	} else {
		ret = 0;
	}
	/* ===codecshifi exception register end=== */

	/* ===shoule reboot exception register begin=== */
	memset(&einfo, 0, sizeof(struct rdr_exception_info_s));
	einfo.e_modid = RDR_AUDIO_REBOOT_MODID_START;
	einfo.e_modid_end = RDR_AUDIO_REBOOT_MODID_END;
	einfo.e_process_priority = RDR_WARN;
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	einfo.e_notify_core_mask = RDR_HIFI;
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_reentrant = (unsigned int)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = SOCHIFI_S_EXCEPTION;
	einfo.e_upload_flag = (unsigned int)RDR_UPLOAD_YES;
	einfo.e_from_core = RDR_HIFI;
	memcpy(einfo.e_from_module, "AUDIO_NOC_EXCEPTION", sizeof("AUDIO_NOC_EXCEPTION"));
	memcpy(einfo.e_desc, "NOC error due to audio", sizeof("NOC error due to audio"));

	/* error return 0, ok return modid */
	if (!rdr_register_exception(&einfo)) {
		BB_PRINT_ERR("register audio codec exception fail\n");
		ret = -EBUSY;
	} else {
		ret = 0;
	}
	/* ===shoule reboot exception register end=== */

	BB_PRINT_END();

	return ret;
}

static int rdr_audio_init_early(void)
{
	int ret;

	ret = rdr_audio_register_exception();
	if (ret) {
		BB_PRINT_ERR("rdr_hifi_register_exception fail\n");
		return ret;
	}

	ret = rdr_audio_register_core();
	if (ret) {
		BB_PRINT_ERR("rdr_hifi_register_core fail\n");
		return ret;
	}

	return ret;
}

static int __init rdr_audio_init(void)
{
	int ret;

	BB_PRINT_START();

	ret = rdr_audio_init_early();
	if (ret) {
		BB_PRINT_ERR("%s():init early fail\n", __func__);
		BB_PRINT_END();
		return ret;
	}

	memset(&s_rdr_audio_des.audio_info, 0, sizeof(struct rdr_register_module_result));
	memset(s_rdr_audio_des.soc_pathname, 0, RDR_FNAME_LEN);
	memset(s_rdr_audio_des.codec_pathname, 0, RDR_FNAME_LEN);

	ret = rdr_audio_soc_init();
	if (ret)
		BB_PRINT_ERR("init rdr soc hifi fail\n");

	ret = rdr_audio_codec_init();
	if (ret)
		BB_PRINT_ERR("init rdr codec hifi fail\n");

	BB_PRINT_END();

	return ret;
}

static void __exit rdr_audio_exit(void)
{
	rdr_audio_soc_exit();
	rdr_audio_codec_exit();
	return;
}

/*lint -e528 -e753*/
module_init(rdr_audio_init);
module_exit(rdr_audio_exit);

MODULE_LICENSE("GPL");


