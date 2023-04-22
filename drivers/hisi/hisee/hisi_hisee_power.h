#ifndef __HISI_HISEE_POWER_H__
#define __HISI_HISEE_POWER_H__

#define HISEE_POWERCTRL_TIMEOUT_ERROR    (-4000)
#define HISEE_POWERCTRL_NOTIFY_ERROR     (-4001)
#define HISEE_POWERCTRL_RETRY_FAILURE_ERROR     (-4002)
#define HISEE_POWERCTRL_FLOW_ERROR       (-4003)

#define HISEE_IPC_TXBUF_SIZE (3)

/* Lpm3 communication id */
#define HISEE_LPM3_CMD        IPC_CMD(OBJ_AP, OBJ_LPM3, CMD_NOTIFY, 0)
#define HISEE_LPM3_ACK_MAGIC  0xaaccbbdd

#define HISEE_PWROFF_LOCK	(1)

#define HISEE_COS_UPGRADE_RUNNING_FLG (0x87654321u)

/* timeout of thread exit when waiting semaphore, 30s */
#define HISEE_THREAD_WAIT_TIMEOUT (msecs_to_jiffies(30000))

#define TIMEOUT_MAX_LEN            (32)

typedef enum {
	NFC_SERVICE = 0,
#ifdef CONFIG_HISEE_SUPPORT_INSE_ENCRYPT
	INSE_ENCRYPT = 1,/* ID for pin code , 3D face and fingerprint. */
#endif
	MAX_TIMEOUT_ID,
}hisee_timeout_vote_id_type;

/* the para to lpm3 throug atf */
typedef enum _HISEE_POWER_OPERATION {
	HISEE_POWER_OFF = 0x01000100,
	HISEE_POWER_ON_BOOTING = 0x01000101,
	HISEE_POWER_ON_UPGRADE = 0x01000102,
	HISEE_POWER_ON_UPGRADE_SM = 0x01000103,
	HISEE_POWER_ON_BOOTING_MISC = 0x01000104,
	HISEE_POWER_MAX_OP,
} hisee_power_operation;

/* the powerctrl command */
typedef enum {
	HISEE_POWER_CMD_ON = 0x01000200,
	HISEE_POWER_CMD_OFF = 0x01000201,
} hisee_power_cmd;

/* the power vote record method */
typedef enum {
	HISEE_POWER_VOTE_RECORD_CNT = 0x01000300,
	HISEE_POWER_VOTE_RECORD_PRO = 0x01000301,
} hisee_power_vote_record;

/* the power status */
typedef enum {
	HISEE_POWER_STATUS_ON = 0x01000400,
	HISEE_POWER_STATUS_OFF = 0x01000401,
} hisee_power_status;

typedef union {
	unsigned long value;
	struct {
		unsigned int huawei_wallet:8;
		unsigned int u_shield:8;
		unsigned int chip_test_and_upgrade:8;
		unsigned int unknow_id:8;
		unsigned int time_out:8;
#ifdef CONFIG_HISEE_SUPPORT_INSE_ENCRYPT
		unsigned int inse_encrypt:8;
		unsigned int reserved:16;
#else
		unsigned int reserved:24;
#endif
	} status;
}hisee_power_vote_status;

typedef struct _TIMER_ENTRY_LIST {
	struct list_head list;
	struct timer_list timer;
	atomic_t handled;
} timer_entry_list;

extern unsigned int g_cos_id;

int hisee_suspend(struct platform_device *pdev, struct pm_message state);
void hisee_power_ctrl_init(void);
ssize_t hisee_check_ready_show(struct device *dev, struct device_attribute *attr, char *buf);
int wait_hisee_ready(hisee_state ready_state, unsigned int timeout_ms);

/* buf is the process id */
int hisee_get_cosid_processid(void *buf, unsigned int *cos_id, unsigned int *process_id);
int hisee_poweron_booting_func(void *buf, int para);
int hisee_poweron_upgrade_func(void *buf, int para);
int hisee_poweroff_func(void *buf, int para);
int hisee_poweron_timeout_func(void *buf, int para);
hisee_power_status hisee_get_power_status(void);
#ifdef CONFIG_HISI_SMX_PROCESS
int smx_process(hisee_power_operation op_type, unsigned int op_cosid, int power_cmd);
#endif
#ifdef CONFIG_HISEE_NFC_IRQ_SWITCH
int hisee_nfc_irq_switch_func(void * buf, int para);
#endif
#endif
