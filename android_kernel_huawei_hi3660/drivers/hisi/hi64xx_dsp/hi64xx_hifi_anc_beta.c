/*
 * hi64xx_hifi_om.c -- om module for hi64xx
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "hi64xx_hifi_interface.h"
#include "hi64xx_hifi_debug.h"
#include "hi64xx_hifi_om.h"
#include "hi64xx_hifi_misc.h"
#include "../../huawei_platform/log/imonitor.h"
#include "../../huawei_platform/log/imonitor_keys.h"


/*lint -e655 -e838 -e730 -e754 -e747 -e731*/
#define CODEC_DSP_ANC_ERR_BASE_ID        916000000
#define CODEC_DSP_SMARTPA_ERR_BASE_ID    916000100
#define HOOK_PATH_BETA_CLUB "/data/log/codec_dsp/beta_club/"
#define FULL_PERMISSION true
#define SAFE_PERMISSION false
#define EVENTLEVEL E916000001_EVENTLEVEL_INT
#define EVENTMODULE E916000001_EVENTMODULE_VARCHAR

//This data structure holds statistics data of 5 adaptive parameters.
//After reading, the statistics of the 5 adaptive parameters will reset.
typedef struct{
    int         maxFcQ9;		//Q9, in Hz,           valid floating-point range = [0, 4194304)
    int         minFcQ9;        //Q9, in Hz,           valid floating-point range = [0, 4194304)

    int         maxQcQ29;       //Q29,                 valid floating-point range = [0, 4)
    int         minQcQ29;       //Q29,                 valid floating-point range = [0, 4)

    int         maxRdcQ27;      //Q27,                 valid floating-point range = [0, 16)
    int         minRdcQ27;      //Q27,                 valid floating-point range = [0, 16)

    int         maxTempQ19;     //Q19,
    int         minTempQ19;		//Q19
    float       avgTemp;       //floating-point,

    int         maxExcurQ27;    //Q27,                 valid floating-point range = [0, 16)
    int         minExcurQ27;    //Q27,                 valid floating-point range = [0, 16)

    int         maxAbsInputQ0;  //Q0
    int         maxAbsOutputQ0; //Q0

    int         maxAbsIDataQ0;  //Q0
    int         maxAbsVDataQ0;  //Q0

    int         spker_short_circuit_warning;
    int         spker_open_circuit_warning;

    float       elapsed_frames;
    int         exc_limiter_overshoots;
} Dsm_adp_statistics;

static struct om_hook_para g_anc_hook_para_full_perms[4] = {
	{HOOK_POS_ANC_PCM_REF, 2, 1, 16000},
	{HOOK_POS_ANC_PCM_ERR, 2, 1, 16000},
	{HOOK_POS_ANC_PCM_ANTI, 2, 1, 16000},
	{HOOK_POS_ANC_BETA_CSINFO, 2, 1, 8000}
};

static struct om_hook_para g_anc_hook_para_safe_perms[1] = {
	{HOOK_POS_ANC_BETA_CSINFO, 2, 1, 8000}
};

bool voice_record_permission = SAFE_PERMISSION;

void anc_beta_set_voice_hook_switch(unsigned short permission)
{
	voice_record_permission = (1 == permission) ? FULL_PERMISSION : SAFE_PERMISSION;
}

int anc_beta_start_hook(void)
{
	struct krn_param_io_buf krn_param;
	struct om_start_hook_msg msg;
	struct om_hook_para *anc_hook_para;
	unsigned char *buf_in;
	unsigned int buf_size_in;
	int ret = 0;

	HI64XX_DSP_INFO("ANC detection start hook, permission %d(1:inc voice)\n", voice_record_permission);
	msg.msg_id = ID_AP_DSP_HOOK_START;
	if (voice_record_permission) {
		msg.para_size = sizeof(g_anc_hook_para_full_perms);
		anc_hook_para = g_anc_hook_para_full_perms;
	} else {
		msg.para_size = sizeof(g_anc_hook_para_safe_perms);
		anc_hook_para = g_anc_hook_para_safe_perms;
	}

	buf_in = (unsigned char *)kzalloc(sizeof(msg) + msg.para_size, GFP_ATOMIC);
	if (!buf_in) {
		HI64XX_DSP_ERROR("buf_in alloc fail\n");
		return -ENOMEM;
	}
	memcpy(buf_in, &msg, sizeof(msg));
	memcpy(buf_in + sizeof(msg), anc_hook_para, (unsigned long)msg.para_size);

	buf_size_in = sizeof(msg) + msg.para_size;

	krn_param.buf_in = buf_in;
	krn_param.buf_size_in = buf_size_in;

	hi64xx_hifi_om_set_sponsor(OM_SPONSOR_BETA);
	hi64xx_hifi_om_set_bw(OM_BANDWIDTH_6144);
	ret = hi64xx_func_start_hook(&krn_param);

	kfree(buf_in);

	return ret;
}

int anc_beta_stop_hook(void)
{
	struct krn_param_io_buf krn_param;
	struct om_start_hook_msg msg;
	struct om_hook_para *anc_hook_para;
	unsigned char *buf_in;
	unsigned int buf_size_in;
	int ret = 0;

	HI64XX_DSP_INFO("ANC detection stop hook, permission %d(1:inc voice)\n", voice_record_permission);
	msg.msg_id = ID_AP_DSP_HOOK_START;
	if (voice_record_permission) {
		msg.para_size = sizeof(g_anc_hook_para_full_perms);
		anc_hook_para = g_anc_hook_para_full_perms;
	} else {
		msg.para_size = sizeof(g_anc_hook_para_safe_perms);
		anc_hook_para = g_anc_hook_para_safe_perms;
	}

	buf_in = (unsigned char *)kzalloc(sizeof(msg) + msg.para_size, GFP_ATOMIC);
	if (!buf_in) {
		HI64XX_DSP_ERROR("buf_in alloc fail\n");
		return -ENOMEM;
	}
	memcpy(buf_in, &msg, sizeof(msg));
	memcpy(buf_in + sizeof(msg), anc_hook_para, (unsigned long)msg.para_size);

	buf_size_in = sizeof(msg) + msg.para_size;

	krn_param.buf_in = buf_in;
	krn_param.buf_size_in = buf_size_in;

	ret = hi64xx_func_stop_hook(&krn_param);

	kfree(buf_in);

	return ret;
}

void anc_beta_generate_path(hook_pos pos, char *base_path, char *full_path, unsigned long full_path_len)
{
	switch (pos) {
	case HOOK_POS_ANC_PCM_REF:
		snprintf(full_path, full_path_len - 1, "%sanc_pcm_ref.data", base_path);
		break;
	case HOOK_POS_ANC_PCM_ERR:
		snprintf(full_path, full_path_len - 1, "%sanc_pcm_err.data", base_path);
		break;
	case HOOK_POS_ANC_PCM_ANTI:
		snprintf(full_path, full_path_len - 1, "%sanc_pcm_anti.data", base_path);
		break;
	case HOOK_POS_ANC_BETA_CSINFO:
		snprintf(full_path, full_path_len - 1, "%sanc_cs_info.log", base_path);
		break;
	default:
	 	break;
      }
}

int anc_beta_log_upload(void* data)
{
	MLIB_ANC_DFT_INFO *info = (MLIB_ANC_DFT_INFO *)data;
	struct imonitor_eventobj *obj;
	unsigned int event_id;
	int ret;

	event_id = CODEC_DSP_ANC_ERR_BASE_ID + info->err_class;
	obj = imonitor_create_eventobj(event_id);

	imonitor_set_param(obj, EVENTLEVEL, info->err_level);
	if (voice_record_permission) {
		imonitor_set_param(obj, EVENTMODULE, (long)"ANC_inc_voice");
	} else {
		imonitor_set_param(obj, EVENTMODULE, (long)"ANC_no_voice");
	}
	if (LOW_ACTTIME_RATE == info->err_class) {
		imonitor_set_param(obj, E916000004_PROBABILITY_TINYINT, *(unsigned int *)(info->details + 24));
	}
	if (PROCESS_PATH_ERR == info->err_class) {
		char   rsn[8] = {0};
		snprintf(rsn, 7, "%d", *(unsigned int *)(info->details + 28));
		imonitor_set_param(obj, E916000005_CAUSECASE_VARCHAR, (long)rsn);
	}
	ret = imonitor_send_event(obj);
	imonitor_destroy_eventobj(obj);

	return ret;
}

int dsm_beta_dump_file(void* data, bool create_dir)
{
	MLIB_DSM_DFT_INFO *info = (MLIB_DSM_DFT_INFO *)data;
	char fullname[HOOK_PATH_MAX_LENGTH] = {0};

	if (create_dir)
		hi64xx_hifi_create_hook_dir(HOOK_PATH_BETA_CLUB);

	snprintf(fullname, sizeof(fullname) - 1, "%s%s", HOOK_PATH_BETA_CLUB, "pa_om_info.log");

	if( info->msgSize > sizeof(MLIB_DSM_DFT_INFO)) {
		hi64xx_hifi_dump_to_file((char *)info, sizeof(MLIB_DSM_DFT_INFO), fullname);
		HI64XX_DSP_ERROR("message size is wrong\n");
		return 0;
	}
	hi64xx_hifi_dump_to_file((char *)info,  info->msgSize, fullname);

	return 0;
}

int dsm_beta_log_upload(void* data)
{
	MLIB_DSM_DFT_INFO *info = (MLIB_DSM_DFT_INFO *)data;
	struct imonitor_eventobj *obj;
	unsigned int event_id;
	Dsm_adp_statistics *dsm_info;
	int ret;

	event_id = CODEC_DSP_SMARTPA_ERR_BASE_ID + info->errClass;
	obj = imonitor_create_eventobj(event_id);
	imonitor_set_param(obj, E916000101_EVENTLEVEL_INT, info->errLevel);
	imonitor_set_param(obj, E916000101_EVENTMODULE_VARCHAR, (long)"SmartPa");
	if (DSM_OM_ERR_TYPE_PROC == info->errClass) {
		dsm_info = (Dsm_adp_statistics *)info->errInfo;
		imonitor_set_param(obj, E916000101_ERRCODE_INT, info->errCode);
		imonitor_set_param(obj, E916000101_MAXRDC_INT, dsm_info->maxRdcQ27);
		imonitor_set_param(obj, E916000101_MINRDC_INT, dsm_info->minRdcQ27);
		imonitor_set_param(obj, E916000101_MAXTEMP_INT, dsm_info->maxTempQ19);
		imonitor_set_param(obj, E916000101_MINTEMP_INT, dsm_info->minTempQ19);
		imonitor_set_param(obj, E916000101_MAXAMP_INT, dsm_info->maxExcurQ27);
		imonitor_set_param(obj, E916000101_MINAMP_INT, dsm_info->minExcurQ27);
	} else {
		char   numStr[8] = {0};
		snprintf(numStr, 7, "%d", info->errLineNum);
		imonitor_set_param(obj, E916000102_ERRPOSTAG_VARCHAR, (long)numStr);
	}
	ret = imonitor_send_event(obj);
	imonitor_destroy_eventobj(obj);

	return ret;
}

