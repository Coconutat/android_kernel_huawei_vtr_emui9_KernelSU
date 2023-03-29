#ifndef	__HISI_HISEE_H__
#define	__HISI_HISEE_H__
#include <linux/device.h>
#include <linux/wakelock.h>
#include <linux/hisi/hisi_rproc.h>
#include "hisi_hisee_fs.h"
#ifdef CONFIG_HISI_HISEE_SUPPORT_CASDKEY
#include "hisi_casdcert.h"
#endif

/* Hisee module general error code*/
#define HISEE_TRUE           (1)
#define HISEE_FALSE          (0)
#define HISEE_OK             (0)
#define HISEE_ERROR          (-10002)
#define HISEE_NO_RESOURCES   (-10003)
#define HISEE_INVALID_PARAMS (-6)
#define HISEE_IS_UPGRADING   (-7)
#define HISEE_CMA_DEVICE_INIT_ERROR (-10005)
#define HISEE_IOCTL_NODE_CREATE_ERROR (-10006)
#define HISEE_POWER_NODE_CREATE_ERROR (-10007)
#define HISEE_THREAD_CREATE_ERROR     (-10008)
#define HISEE_RPMB_MODULE_INIT_ERROR  (-11)
#define HISEE_BULK_CLK_INIT_ERROR     (-10010)
#define HISEE_BULK_CLK_ENABLE_ERROR   (-10011)
#define HISEE_POWERDEBUG_NODE_CREATE_ERROR   (-10012)
#ifdef CONFIG_HISI_HISEE_SHUTDOWN_SWIPE
#define HISEE_PDSWIPE_RECORD_NODE_CREATE_ERROR  (-10013)
#define HISEE_PDSWIPE_EXC_NODE_CREATE_ERROR     (-10014)
#endif
#define HISEE_AT_RESULT_NODE_CREATE_ERROR       (-10015)
#define HISEE_FACTORY_STATE_ERROR     (-10021)

#define HISEE_OTP1_WRITE_FAIL    (-10023)
#define HISEE_POWER_CTRL_FAIL   (-10024)

/* Hisee module specific error code*/
#define HISEE_RPMB_KEY_WRITE_ERROR (-1000)
#define HISEE_RPMB_KEY_READ_ERROR  (-1001)
#define HISEE_RPMB_KEY_UNREADY_ERROR  (-1010)

#define HISEE_FIRST_SMC_CMD_ERROR  (-5000)
#define HISEE_SMC_CMD_TIMEOUT_ERROR  (-5001)
#define HISEE_SMC_CMD_PROCESS_ERROR  (-5002)

#define HISEE_GET_HISEE_VALUE_ERROR    (-7000)
#define HISEE_SET_HISEE_VALUE_ERROR    (-7001)
#define HISEE_SET_HISEE_STATE_ERROR    (-7002)


#ifdef CONFIG_HISI_SMX_PROCESS
#define SMX_PROCESS_STEP1_ACTION        (0x01000101)
#define SMX_PROCESS_STEP1_ID            (0)
#define SMX_PROCESS_STEP1_CMD           (0x01000200)

#define SMX_PROCESS_STEP2_ACTION        (0x01000100)
#define SMX_PROCESS_STEP2_ID            (0)
#define SMX_PROCESS_STEP2_CMD           (0x01000201)

#define SMX_PROCESS_UNSUPPORT           (0x5A5A0001)
#define SMX_PROCESS_SUPPORT_AND_OK      (0x5A5A0002)
#define SMX_PROCESS_SUPPORT_BUT_ERROR   (0x5A5A0003)
#define SMX_PROCESS_INVALID_PARAMS      (0x5A5A0004)
#define SMX_PROCESS_CLK_ENABLE_ERROR    (0x5A5A0005)

#define SMX_PROCESS_FN_MAIN_SERVICE_CMD (0xc5000020)
#endif

#define HISEE_WAIT_READY_TIMEOUT     (-9001)

#ifdef CONFIG_HISI_HISEE_SUPPORT_CASDKEY
#define HISI_CASD_ERROR                 (-19000)
#define HISI_CASD_AT_MISMATCH_ERROR     (-19001)
#define HISI_CASD_AT_HEADER_ERROR       (-19002)
#define HISI_CASD_LENGTH_ERROR          (-19003)
#define HISI_CASD_DATA_PARAM_ERROR      (-19004)
#define HISI_CASD_HASH_ERROR            (-19005)
#endif

#define HISEE_DEVICE_NAME    "hisee"
#define HISEE_DEFAULT_COSID  (0)


/* ATF service id */
#define HISEE_FN_MAIN_SERVICE_CMD    (0xc5000020)
#define HISEE_FN_CHANNEL_TEST_CMD    (0xc5000040)

#define HISEE_ATF_ACK_SUCCESS 0xaabbccaa
#define HISEE_ATF_ACK_FAILURE 0xaabbcc55

#define HISEE_SM_MODE_MAGIC            (0xffeebbaa)
#define HISEE_DM_MODE_MAGIC            (0xffeebb55)
#define DELAY_BETWEEN_STEPS            (50)
#define DELAY_FOR_HISEE_POWERON_UPGRADE	(200)
#define DELAY_FOR_HISEE_POWEROFF		(50)
#define DELAY_FOR_HISEE_POWERON_BOOTING    (200)

#define HISEE_ATF_MESSAGE_HEADER_LEN   (16)
#define HISEE_ATF_COS_APPDATA_TIMEOUT  (15000)
#define HISEE_ATF_WRITE_RPMBKEY_TIMEOUT (1000)
#define HISEE_ATF_OTP_TIMEOUT        (10000)
#define HISEE_ATF_COS_TIMEOUT        (30000)
#define HISEE_ATF_SLOADER_TIMEOUT    (30000)
#ifndef CONFIG_HISEE_SUPPORT_MULTI_COS
#define HISEE_ATF_MISC_TIMEOUT    (30000)
#else
#define HISEE_ATF_MISC_TIMEOUT    (100000)
#endif
#define HISEE_ATF_APPLICATION_TIMEOUT    (60000)
#define HISEE_ATF_GENERAL_TIMEOUT        (30000)
#define HISEE_ATF_NVM_FORMAT_TIMEOUT        (60000)
#define HISEE_FPGA_ATF_COS_TIMEOUT        (3000000)

#define SIZE_1K              (1024)
#define SIZE_4K              (4 * SIZE_1K)
#define SIZE_1M              (1024 * SIZE_1K)


#define SMC_TEST_RESULT_SIZE      (256)

#define HISEE_COS_PATCH_FREE_CNT		(1)

#define HISEE_MAX_IMG_SIZE    (640 * SIZE_1K)
#define HISEE_SHARE_BUFF_SIZE round_up((HISEE_MAX_IMG_SIZE + HISEE_ATF_MESSAGE_HEADER_LEN), PAGE_SIZE)
#define HISEE_CMD_NAME_LEN    (128)
#define HISEE_BUF_SHOW_LEN    (128)
#define HISEE_ERROR_DESCRIPTION_MAX  (64)
#define HISEE_APDU_DATA_LEN_MAX      (261)

#define HISEE_IOCTL_CMD_MAX_LEN (2048) /* max str len for AT^HISEE=CASD cmd */

#define HISEE_FACTORY_TEST_VERSION	(0x12345678)
#define HISEE_SERVICE_WORK_VERSION	(0)

#define MAX_CMD_BUFF_PARAM_LEN  (4)

/* hisee apdu cmd type */
#define HISEE_SET_KEY	(0)
#define HISEE_DEL_TEST_APPLET	(1)

#define HISEE_CHAR_NEWLINE (10)
#define HISEE_CHAR_SPACE (32)

/* hisee NFC_IRQ mode */
#ifdef CONFIG_HISEE_NFC_IRQ_SWITCH
#define HISEE_NFC_IRQ_ADDR    	ioremap(SOC_SCTRL_SCBAKDATA22_ADDR(SOC_ACPU_SCTRL_BASE_ADDR), 4)
#define HISEE_NFC_IRQ_DISABLE_BIT		(31)
#endif
/* hisee lcs mode */
#define HISEE_STATE_ADDR    	ioremap(SOC_SCTRL_SCBAKDATA10_ADDR(SOC_ACPU_SCTRL_BASE_ADDR), 4)
#define HISEE_LCS_DM_BIT    	(13)


#define SMX_PROCESS_0    (0x5A5AA5A5)
#define SMX_PROCESS_1    (0xA5A55A5A)

/* check ret is ok or otherwise goto err_process*/
#define check_result_and_goto(ret, lable) \
do { \
	if (HISEE_OK != (ret)) { \
		pr_err("hisee:%s() run failed,line=%d.\n", __func__, __LINE__);\
		goto lable; \
	} \
} while (0)

#define check_and_print_result()  \
do {\
	if (ret != HISEE_OK)\
		pr_err("hisee:%s() run failed\n", __func__);\
	else\
		pr_err("hisee:%s() run success\n", __func__);\
} while (0)

#define check_and_print_result_with_cosid()  \
do {\
	if (HISEE_OK != ret)\
		pr_err("hisee:%s() run failed,cos_id=%d.\n", __func__, cos_id);\
	else\
		pr_err("hisee:%s() run success,cos_id=%d\n", __func__, cos_id);\
} while (0)

// cppcheck-suppress *
#define set_errno_and_return(err) \
	ret = err;\
	/*lint -save -e1058 */atomic_set(&g_hisee_errno, ret);/*lint -restore */\
	return ret

#define hisee_mdelay(n)  msleep(n)
#define hisee_delay(n)   msleep((n) * 1000)

#ifdef CONFIG_HISEE_NFC_IRQ_SWITCH
#define HISEE_NFC_IRQ_SWITCH_CMD_MAX_LEN   (4)
typedef enum _HISEE_NFC_IRQ_CFG_STATE {
    NFC_IRQ_CFG_ON = 0x5A5AA5A5,
    NFC_IRQ_CFG_OFF = 0xA5A55A5A
} hisee_nfc_irq_cfg_state;
#endif

typedef enum _HISEE_NFC_CFG_MESSAGE {
	DISABLE_NFC_IRQ = 0x02000100,
	SET_NFC_IRQ = 0x02000101,
} hisee_nfc_cfg_message;

typedef struct _HISEE_ERRCODE_ITEM_DES {
	int err_code;	/* see error code definition */
	char err_description[HISEE_ERROR_DESCRIPTION_MAX]; /* error code description */
} hisee_errcode_item_des;

typedef struct _HISEE_DRIVER_FUNCTION {
	char *function_name;	/* function cmd string */
	int (*function_ptr)(void *buf, int para); /* function cmd process */
} hisee_driver_function;

#define SE_HISEE_MISC_NO_UPRGADE (0xCCAAAACC)

typedef enum _HISEE_STATE {
	HISEE_STATE_POWER_DOWN = 0,
	HISEE_STATE_POWER_UP   = 1,
	HISEE_STATE_MISC_READY = 2,
	HISEE_STATE_COS_READY  = 3,
	HISEE_STATE_POWER_DOWN_DOING = 4,
	HISEE_STATE_POWER_UP_DOING   = 5,
#ifdef CONFIG_HISEE_SUPPORT_OVERSEA
	HISEE_STATE_MISC_UPGRADE_DONE = 6,
#endif
	HISEE_STATE_MAX,
} hisee_state;

typedef enum  _HISEE_COS_IMGID_TYPE {
	COS_IMG_ID_0 = 0,
	COS_IMG_ID_1 = 1,
	COS_IMG_ID_2 = 2,
	COS_IMG_ID_3 = 3,
	COS_IMG_ID_4 = 4,
	COS_IMG_ID_5 = 5,
	COS_IMG_ID_6 = 6,
	COS_IMG_ID_7 = 7,
	MAX_COS_IMG_ID,
} hisee_cos_imgid_type;

typedef enum  _HISEE_COS_PROCESS_TYPE {
	COS_PROCESS_WALLET = 0,	/* huawei wallet */
	COS_PROCESS_U_SHIELD,	/* u shiled */
	COS_PROCESS_CHIP_TEST,	/* chip test */
	COS_PROCESS_UNKNOWN,      /* default id */
	COS_PROCESS_TIMEOUT,	/* timeout :must be the last valid id, restricted in function hisee_poweroff_func*/
#ifdef CONFIG_HISEE_SUPPORT_INSE_ENCRYPT
	COS_PROCESS_INSE_ENCRYPT = 5, /* inse encypt: pin, bio, file etc. */
#endif
	MAX_POWER_PROCESS_ID,
} hisee_cos_process_type;
/* TODO: modify the factory flow */
#define COS_PROCESS_UPGRADE COS_PROCESS_CHIP_TEST

typedef enum  _HISEE_PLATFORM_TYPE {
	HISEE_PLATFORM_3660 = 0,
	HISEE_PLATFORM_3670_ES,
	HISEE_PLATFORM_3670_CS,
	HISEE_PLATFORM_MAX_ID = 0xFFFF,
} hisee_platform_type;

typedef enum {
	CMD_UPGRADE_SLOADER = 0,
	CMD_UPGRADE_OTP,
	CMD_UPGRADE_COS,
	CMD_UPGRADE_MISC,
#ifdef CONFIG_HICOS_MISCIMG_PATCH
	CMD_UPGRADE_COS_PATCH,
#endif
	CMD_UPGRADE_APPLET,
	CMD_PRESAVE_COS_APPDATA,
#ifdef CONFIG_HISEE_SUPPORT_MULTI_COS
	CMD_LOAD_ENCOS_DATA,
#endif
	CMD_WRITE_RPMB_KEY,
	CMD_SET_LCS_SM,
	CMD_SET_STATE,
	CMD_GET_STATE,
	CMD_APDU_RAWDATA,
	CMD_FACTORY_APDU_TEST,
	CMD_HISEE_CHANNEL_TEST,
	CMD_HISEE_VERIFY_KEY,
	CMD_HISEE_WRITE_CASD_KEY,
#ifdef CONFIG_HISEE_FACTORY_SECURITY_CHECK
	CMD_HISEE_FACTORY_CHECK,
#endif

	CMD_HISEE_POWER_ON = 0x30,
	CMD_HISEE_POWER_OFF,
#ifdef CONFIG_HISI_SMX_PROCESS
	CMD_SMX_PROCESS_STEP1,
	CMD_SMX_PROCESS_STEP2,
#endif
	CMD_HISEE_GET_EFUSE_VALUE = 0x40,
	CMD_FORMAT_RPMB = 0x51,
#ifdef CONFIG_HISEE_SUPPORT_OVERSEA
	CMD_WRITE_COS_CFG = 0x60,
	CMD_WRITE_SMX_CFG,
	CMD_SEND_MISC_CNT,
	CMD_GET_SMX_CFG,
	CMD_COS_VERSION_CFG,
#endif
	CMD_END,
} se_smc_cmd;

typedef struct _HISEE_WORK_STRUCT {
	char *buffer;
	phys_addr_t phy;
	unsigned int size;
} hisee_work_struct;

typedef enum {
	HISEE_FACTORY_TEST_FAIL = -1,
	HISEE_FACTORY_TEST_SUCCESS = 0,
	HISEE_FACTORY_TEST_RUNNING = 1,
	HISEE_FACTORY_TEST_NORUNNING = 2,
} hisee_factory_test_status;

/* message header between kernel and atf */
typedef struct _ATF_MESSAGE_HEADER {
/* atf cmd execute type, such as otp, cos, sloader at all, kernel set and atf read it*/
	unsigned int cmd;
/* atf cmd execute result indication, use a magic value to indicate success, atf set it and check in kernel*/
	unsigned int ack;
/* tell atf store the result to this buffer when doing channel test */
	unsigned int test_result_phy;
/* tell atf the size of buffer when doing channel test */
	unsigned int test_result_size;
} atf_message_header;

typedef struct _APDU_ACK_HEADER {
	unsigned int ack_len;
	unsigned char ack_buf[HISEE_APDU_DATA_LEN_MAX + 1];
} apdu_ack_header;

typedef struct _HISEE_MODULE_DATA {
	struct device *cma_device; /* cma memory allocator device */
	struct clk *hisee_clk;  /* buck 0 voltage hold at 0.8v */
	void *apdu_command_buff_virt;
	phys_addr_t apdu_command_buff_phy;
	struct semaphore atf_sem;    /* do sync for smc message between kernel and atf */
	hisee_img_header hisee_img_head; /* store the parsed result for hisee_img partition header */
	apdu_ack_header  apdu_ack; /* store the apdu response */
	struct mutex hisee_mutex; /* mutex for global resources */
	hisee_work_struct channel_test_item_result;
	unsigned int rpmb_is_ready; /* indicate the rpmb has been initialiazed */
	unsigned int smc_cmd_running; /*indicate the smc is running */
	int power_on_count;  /*indicate the number of hisee poweron*/
	hisee_factory_test_status factory_test_state; /*indicate the factory test status */
	struct wake_lock wake_lock;
#ifdef CONFIG_HISI_HISEE_SUPPORT_CASDKEY
	hisee_casd_at_data casd_data;
#endif
} hisee_module_data;

extern hisee_module_data g_hisee_data;
extern atomic_t g_hisee_errno;
extern unsigned int g_misc_version[];
extern int g_hisee_partition_byname_find;
extern unsigned int g_platform_id;
extern u32 g_hisee_cos_upgrade_time;
extern bool g_hisee_is_fpga;

#ifdef CONFIG_HICOS_MISCIMG_PATCH
extern atomic_t g_is_patch_free;
#endif

int get_hisee_lcs_mode(unsigned int *mode);
int set_hisee_lcs_sm_efuse(void);
noinline int atfd_hisee_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2);
int send_smc_process(atf_message_header *p_message_header, phys_addr_t phy_addr, unsigned int size,
							unsigned int timeout, se_smc_cmd smc_cmd);
void set_message_header(atf_message_header *header, unsigned int cmd_type);
int send_apdu_cmd(int type);
int hisee_lpmcu_send(rproc_msg_t msg_0, rproc_msg_t msg_1);
int cos_image_upgrade_by_self(void);
#ifdef CONFIG_HISEE_NFC_IRQ_SWITCH
void nfc_irq_cfg(hisee_nfc_irq_cfg_state flag);
#endif

void hisee_get_smx_cfg(unsigned int *p_smx_cfg);

#endif
