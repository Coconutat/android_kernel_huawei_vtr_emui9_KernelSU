#ifndef __HISEE_MNTN_H__
#define __HISEE_MNTN_H__

#include <linux/hisi/ipc_msg.h>

/* used for translation from original value to exception module id */
typedef struct {
	u32		irq_value;
	u32		module_value;
} hisee_exc_trans_s;

typedef struct {
	uint64_t addr;
	uint64_t max_size;
	uint64_t real_size;
} hlog_header;
typedef enum {
	MODID_HISEE_START = (unsigned int)HISI_BB_MOD_HISEE_START,
	MODID_HISEE_EXC_SENSOR_CTRL = MODID_HISEE_START,
	MODID_HISEE_EXC_SIC,
	MODID_HISEE_EXC_MED_ROM,
	MODID_HISEE_EXC_MED_RAM,
	MODID_HISEE_EXC_OTPC,
	MODID_HISEE_EXC_HARD,
	MODID_HISEE_EXC_IPC_MAILBOX,
	MODID_HISEE_EXC_MPU,
	MODID_HISEE_EXC_BUS,
	MODID_HISEE_EXC_TIMER,
	MODID_HISEE_EXC_SEC_EXTERN,
	MODID_HISEE_EXC_WDG,
	MODID_HISEE_EXC_SYSALARM,
	MODID_HISEE_EXC_NV_COUNTER,
	MODID_HISEE_EXC_SWP,
	MODID_HISEE_EXC_COS,
	MODID_HISEE_EXC_BB,
	MODID_HISEE_EXC_MNTN_COS,
	MODID_HISEE_EXC_MNTN_COS_RESET,
	MODID_HISEE_EXC_LIBC,
	MODID_HISEE_EXC_NVM,

	MODID_HISEE_EXC_SECENG_TRNG,
	MODID_HISEE_EXC_SECENG_TRIM,
	MODID_HISEE_EXC_SECENG_SCE,
	MODID_HISEE_EXC_SECENG_RSA,
	MODID_HISEE_EXC_SECENG_SM2,
	MODID_HISEE_EXC_SECENG_KM,
	MODID_HISEE_EXC_SECENG_SCRAMBLING,
	MODID_HISEE_EXC_BOTTOM,

	/*only for exception records that are not happened to hisee itself, starts*/
	MODID_SIMULATE_EXC_RPMB_KO,
	MODID_SIMULATE_EXC_PD_SWIPE,
	/*only for exception records that are not happened to hisee itself, ends*/
	/* Please don't remove this comment,
	These have to leave for E_HISEE_EXC_TYPE defined in Lpm3 module code*/

	MODID_HISEE_EXC_ALARM0 = 0x87000100,
	MODID_HISEE_EXC_ALARM1,
	MODID_HISEE_EXC_AS2AP_IRQ,
	MODID_HISEE_EXC_DS2AP_IRQ,
	MODID_HISEE_EXC_SENC2AP_IRQ,	/*used for chicago and boston es*/
	/*added for boston cs, starts*/
	MODID_HISEE_EXC_SENC2AP_IRQ0,
	MODID_HISEE_EXC_SENC2AP_IRQ1,
	/*added for boston cs, ends*/
	MODID_HISEE_EXC_LOCKUP,
	MODID_HISEE_EXC_EH2H_SLV,
	MODID_HISEE_EXC_TSENSOR1,
#ifdef CONFIG_HISI_HISEE_MNTN_RESET_IRQ_SEPARATE
	MODID_HISEE_EXC_RST,
#endif

	/*Please your type above !!!*/
	MODID_HISEE_EXC_UNKNOWN,
	MODID_HISEE_EXC_END = HISI_BB_MOD_HISEE_END
} hisee_module_id;

typedef enum {
	HISEE_MSG_START,
	/* lpm3 -> kernel hisee mntn driver */
	HISEE_MSG_EXCEPTION,
	HISEE_MSG_IRQ,
	HISEE_MSG_SYNC_TIME,
	HISEE_MSG_LOG_OUT,
	HISEE_MSG_VOTE_RES,
	/* kernel hisee mntn driver -> lpm3 */
	HISEE_MNTN_TYPE,
	HISEE_TYPE_END
} hisee_type;

/*msg definition Kernel->lpm3->hisee, so keep the same as defined in lpm3 and hisee, starts*/
typedef enum {
	HISEE_GROUP_START,
	HISEE_MNTN_CTRL,

	HISEE_SYNC_TIME,
	HISEE_RESET,
	HISEE_TEST,
	HISEE_GET_VOTE,
	HISEE_GROUP_END
} hisee_msg_group;

typedef enum {
	HISEE_STATUS_START,
	HISEE_TEST_IRQ,
	HISEE_TEST_EXCEPTION,
	HISEE_TEST_RECORD,
	HISEE_TEST_RUN_COS,
	HISEE_TEST_PRINT_ERR,
	HISEE_TEST_PRINT_PLAIN,
	HISEE_TEST_PRINT_CRYPTO,
	HISEE_STATUS_END
} hisee_msg_data;
/*ends*/
/*used to define control level, keep the same as define in hisee*/
typedef enum {
    HISEE_MNTN_DISABLE = 0x3C5AA5C3,
    HISEE_MNTN_USR = 0xA53CC3A5,
    HISEE_MNTN_BETA = 0xA5C33C5A,
    HISEE_MNTN_ENG = 0xC3A55A3C,
} hisee_mntn_ctrl;


typedef enum {
	HISEE_SMC_INIT = 1,
	HISEE_SMC_GET_LOG,	/*save all log data when hisee reset*/
	HISEE_SMC_LOG_OUT,		/*get log print of hisee when it is running*/
	HISEE_SMC_GET_VOTE		/*get vote value of hisee pwr state*/
} hisee_mntn_smc_cmd;

typedef enum {
	HISEE_STATE_INVALID,
	HISEE_STATE_READY,
	HISEE_STATE_LOG_OUT,
	HISEE_STATE_HISEE_EXC,
} hisee_mntn_state;

/*keep the same as define in kernel*/
#define	HISEE_MNTN_LOG_CTRL_OUT_2_KERNEL	0x5A5A0000
#define	HISEE_MNTN_LOG_CTRL_NOT_2_KERNEL	0xA5A50000
#define	HISEE_DMD_START	DSM_HISEE_BASE_ERROR_NO
#define	HISEE_DMD_END	(DSM_HISEE_COS_IMAGE_UPGRADE_ERROR_NO + 1000)

#define HISEE_MNTN_ID             (0xc500cc00u)

#define HISEE_MNTN_PATH_MAXLEN         128
#define HISEE_LOG_FLIENAME "hisee_log"
#define HISEE_PRINTLOG_FLIENAME "hisee_printlog"
#define HISEE_FILE_PERMISSION 0660
#define EFUSE_LENGTH 8
#define EFUSE_READ_TIMEOUT 1000

#define HISEE_MNTN_PRINT_COS_VER_MS 3000
#define HISEE_MNTN_PRINT_COS_VER_MAXTRY 10

#define LPM3_HISEE_MNTN IPC_CMD(OBJ_AP, OBJ_INSE, CMD_INQUIRY, HISEE_MNTN_TYPE)
#define HISEE_EXCEPTION IPC_CMD(OBJ_LPM3, OBJ_INSE, CMD_NOTIFY, HISEE_MSG_EXCEPTION)
#define HISEE_IRQ IPC_CMD(OBJ_LPM3, OBJ_INSE, CMD_NOTIFY, HISEE_MSG_IRQ)
#define HISEE_TIME IPC_CMD(OBJ_LPM3, OBJ_INSE, CMD_NOTIFY, HISEE_MSG_SYNC_TIME)
#define HISEE_LOG_OUT IPC_CMD(OBJ_LPM3, OBJ_INSE, CMD_NOTIFY, HISEE_MSG_LOG_OUT)
#define HISEE_VOTE_RES IPC_CMD(OBJ_LPM3, OBJ_INSE, CMD_NOTIFY, HISEE_MSG_VOTE_RES)
/*Must keep the same as the one define in atf!!!*/
#define HISEE_MNTN_LOG_OUT_MAGIC	(0x5a7e5a7e)

#define HISEE_DMD_BUFF_SIZE		1024

extern char *rdr_get_timestamp(void);
extern void rdr_count_size(void);
extern u64 rdr_get_logsize(void);
extern int rdr_dir_size(char *path, bool recursion);
extern int get_efuse_hisee_value(unsigned char *pu8Buffer, unsigned int u32Length, unsigned int timeout);
extern void hisee_mntn_print_cos_info(void);
extern void hisee_mntn_update_local_ver_info(void);
#endif
