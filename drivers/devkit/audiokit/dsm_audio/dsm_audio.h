#ifndef __DSM_AUDIO_H__
#define __DSM_AUDIO_H__
#include <dsm/dsm_pub.h>

#ifdef CONFIG_HUAWEI_DSM
#define DSM_AUDIO_BUF_SIZE		(4096)   /*Byte*/
#define DSM_AUDIO_NAME			"dsm_audio_info"

#define DSM_SMARTPA_BUF_SIZE	(1024)   /*Byte*/
#define DSM_SMARTPA_NAME		"dsm_smartpa"

#define DSM_ANC_HS_BUF_SIZE		(1024)
#define DSM_ANC_HS_NAME			"dsm_anc_hs"


enum audio_device_type {
	AUDIO_CODEC = 0,
	AUDIO_SMARTPA,
	AUDIO_ANC_HS,
	AUDIO_DEVICE_MAX
};
int audio_dsm_report_num(enum audio_device_type device_type, int error_no, unsigned int mesg_no);
int audio_dsm_report_info(enum audio_device_type device_type, int error_no, char *fmt, ...);

/* Debug info */
#define ERROR_LEVEL	 1
#define INFO_LEVEL	  1
#define DEBUG_LEVEL	 0

#if INFO_LEVEL
#define dsm_logi(fmt, ...) pr_info(DSM_AUDIO_NAME"[I]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define dsm_logi(fmt, ...)
#endif

#if DEBUG_LEVEL
#define dsm_logd(fmt, ...) pr_info(DSM_AUDIO_NAME"[D]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define dsm_logd(fmt, ...)
#endif

#if ERROR_LEVEL
#define dsm_loge(fmt, ...) pr_err(DSM_AUDIO_NAME"[E]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define dsm_loge(fmt, ...)
#endif

#endif
#endif

