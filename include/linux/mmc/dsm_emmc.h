#ifdef CONFIG_HUAWEI_EMMC_DSM
#ifndef LINUX_MMC_DSM_EMMC_H
#define LINUX_MMC_DSM_EMMC_H

#include <dsm/dsm_pub.h>

/* define a 1024 size of array as buffer */
#define EMMC_DSM_BUFFER_SIZE  2048
extern unsigned int emmc_dsm_real_upload_size;
#define MSG_MAX_SIZE 200
/* eMMC card ext_csd lengh */
#define EMMC_EXT_CSD_LENGHT 512

/*
debug version 0x00 , just for test that it could report;
deta version  0x02 ;
*/
#define EMMC_LIFE_TIME_TRIGGER_LEVEL_FOR_DEBUG         0x00
#define EMMC_LIFE_TIME_TRIGGER_LEVEL_FOR_BETA            0x02
#define EMMC_LIFE_TIME_TRIGGER_LEVEL_FOR_USER            0x05
#define DEVICE_LIFE_TRIGGER_LEVEL  EMMC_LIFE_TIME_TRIGGER_LEVEL_FOR_BETA
/* Error code, decimal[5]: 0 is input, 1 is output, 2 I&O
 * decimal[4:3]: 10 is for eMMC,
 * decimal[2:1]: for different error code.
 */
enum DSM_EMMC_ERR
{
	DSM_EMMC_INIT_ERROR = 928002020,                    /* 20300 */
	DSM_EMMC_TUNING_ERROR = 928002000,                  /* 20301 */
	DSM_EMMC_READ_ERR = 928002008,                      /* 20302 */
	DSM_EMMC_WRITE_ERR = 928002009,                     /* 20303 */
	DSM_EMMC_DATA0_BUSY_ERROR = 928002021,              /* 20304 */
	DSM_EMMC_PRE_EOL_INFO_ERR = 928002002,              /* 20305 */
	DSM_EMMC_LIFE_TIME_EST_ERR = 928002003,             /* 20306 */
	DSM_EMMC_THROUGHPUT_MONITOR_ERROR = 928002022,      /* 20307 */
	DSM_STORAGE_EXT4_ERROR_NO = 928002023,              /* 20308 */
	DSM_SYSTEM_W_ERR = 928002006,                       /* 20309 */
	DSM_EMMC_ERASE_ERR = 928002004,                     /* 20310 */
	DSM_EMMC_VDET_ERR = 928002001,                      /* 20311 */
	DSM_EMMC_SEND_CXD_ERR = 928002005,                  /* 20312 */
	DSM_EMMC_SET_BUS_WIDTH_ERR = 928002015,             /* 20313 */
	DSM_EMMC_RSP_ERR  = 928002011,                      /* 20314 */
	DSM_EMMC_RW_TIMEOUT_ERR  = 928002024,               /* 20315 */
	DSM_EMMC_HOST_ERR  = 928002016,                     /* 20316 */
	DSM_EMMC_URGENT_BKOPS  = 928002012,                 /* 20317 */
	DSM_EMMC_DYNCAP_NEEDED  = 928002013,                /* 20318 */
	DSM_EMMC_SYSPOOL_EXHAUSTED  = 928002025,            /* 20319 */
	DSM_EMMC_PACKED_FAILURE  = 928002014,               /* 20320 */
	DSM_EMMC_DATA_CRC = 928002030,
	DSM_EMMC_COMMAND_CRC = 928002031,
};


struct emmc_dsm_log {
	char emmc_dsm_log[EMMC_DSM_BUFFER_SIZE];
	struct mutex lock;	/* mutex */
};

extern struct dsm_client *emmc_dclient;

/*buffer for transffering to device radar*/
extern struct emmc_dsm_log g_emmc_dsm_log;
extern int dsm_emmc_get_log(void *card, int code, char * err_msg);
extern void dsm_emmc_init(void);
extern int dsm_emmc_get_life_time(struct mmc_card *card);

/*Transfer the msg to device radar*/
#define DSM_EMMC_LOG(card, no, fmt, a...) \
	do { \
		char msg[MSG_MAX_SIZE]; \
		snprintf(msg, MSG_MAX_SIZE-1, fmt, ## a); \
		mutex_lock(&g_emmc_dsm_log.lock); \
		if(dsm_emmc_get_log((card), (no), (msg))){ \
			if(!dsm_client_ocuppy(emmc_dclient)) { \
				dsm_client_copy(emmc_dclient,g_emmc_dsm_log.emmc_dsm_log, emmc_dsm_real_upload_size + 1); \
				dsm_client_notify(emmc_dclient, no); } \
		} \
		mutex_unlock(&g_emmc_dsm_log.lock); \
	}while(0)

#endif /* LINUX_MMC_DSM_EMMC_H */
#endif /* CONFIG_HUAWEI_EMMC_DSM */
