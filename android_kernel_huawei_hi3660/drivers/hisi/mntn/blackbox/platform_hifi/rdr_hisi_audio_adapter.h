/*
 * audio rdr adpter.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __RDR_HISI_AUDIO_ADAPTER_H__
#define __RDR_HISI_AUDIO_ADAPTER_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/hisi/rdr_pub.h>

#define RDR_FNAME_LEN					128UL
/* rdr modid for hifi from 0x84000000(HISI_BB_MOD_HIFI_START) to 0x84ffffff(HISI_BB_MOD_HIFI_END) */
#define RDR_AUDIO_MODID_START          HISI_BB_MOD_HIFI_START

#define RDR_AUDIO_SOC_MODID_START      RDR_AUDIO_MODID_START
#define RDR_AUDIO_SOC_WD_TIMEOUT_MODID 0x84000001U
#define RDR_AUDIO_SOC_MODID_END        0x8400000fU

#define RDR_AUDIO_CODEC_MODID_START      0x84000010U
#define RDR_AUDIO_CODEC_WD_TIMEOUT_MODID 0x84000011U
#define RDR_AUDIO_CODEC_MODID_END        0x8400001FU

#define RDR_AUDIO_REBOOT_MODID_START 0x84000020U
#define RDR_AUDIO_NOC_MODID          0x84000021U
#define RDR_AUDIO_REBOOT_MODID_END   0x8400002FU

#define RDR_AUDIO_MODID_END          HISI_BB_MOD_HIFI_END

enum core_type {
	CODECDSP,
	SOCHIFI
};

#define RDR_CODECDSP_STACK_TO_MEM_SIZE    (512)
#define PARSER_CODEC_TRACE_SIZE           (2300)
#define RDR_CODECDSP_CPUVIEW_TO_MEM_SIZE  (512)
#define RDR_SOCHIFI_STACK_TO_MEM_SIZE     (4096)
#define PARSER_SOCHIFI_TRACE_SIZE         (15920)
#define CODEC_CPUVIEW_DETAIL_MAX_NUM      (100)
#define SOCHIFI_CPUVIEW_DETAIL_MAX_NUM    (1024)
#define CPUVIEW_ONE_INFO_LEN              (128)
#define PARSER_CODEC_CPUVIEW_LOG_SIZE     (CODEC_CPUVIEW_DETAIL_MAX_NUM * CPUVIEW_ONE_INFO_LEN)   /* store codec cpuview buff size */
#define PARSER_SOCHIFI_CPUVIEW_LOG_SIZE	  (SOCHIFI_CPUVIEW_DETAIL_MAX_NUM * CPUVIEW_ONE_INFO_LEN + 180)  /* store sochifi cpuview buff size */
#define SOCHIFI_ORIGINAL_CPUVIEW_SIZE     (sizeof(struct cpuview_details) + sizeof(struct cpuview_slice_record)*SOCHIFI_CPUVIEW_DETAIL_MAX_NUM)

struct cpuview_slice_record
{
	unsigned int target:2;
	unsigned int target_id:5;
	unsigned int action:1;
	unsigned int time_stamp:24;
};

struct cpuview_details
{
	unsigned short curr_idx;
	unsigned short rpt_idx;
	struct cpuview_slice_record records[0];
};

typedef int (*LOG_PARSE_FUNC)(char *, unsigned int, char *, unsigned int, unsigned int);
struct parse_log {
	unsigned int original_offset;
	unsigned int original_log_size;
	unsigned int parse_log_size;
	LOG_PARSE_FUNC parse_func;
};

int rdr_audio_write_file(char *name, char *data, u32 size);
int parse_hifi_cpuview(char *original_buf, unsigned int original_buf_size,
	char *parsed_buf, unsigned int parsed_buf_size, unsigned int core_type);
int parse_hifi_trace(char *original_data, unsigned int original_data_size,
	char *parsed_data, unsigned int parsed_data_size, unsigned int core_type);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

