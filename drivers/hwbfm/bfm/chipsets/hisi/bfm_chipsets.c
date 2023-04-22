/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_chipsets.c

    @brief: define the chipsets's interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/atomic.h>
#include <linux/types.h>
#include <linux/hisi/kirin_partition.h>
#include <hisi_partition.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/rdr_pub.h>
#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/common/bfmr_common.h>
#include <chipset_common/bfmr/bfm/chipsets/bfm_chipsets.h>
#include <chipset_common/hwbfm/hw_boot_fail_core.h>
#include "bfm_hisi_dmd_info_priv.h"
#include <linux/notifier.h>
#include <linux/kthread.h>
#include <linux/mfd/hisi_pmic_mntn.h>


/*----local macroes------------------------------------------------------------------*/

#define BFM_HISI_BFI_PART_NAME "bootfail_info"
#define BFM_HISI_BFI_BACKUP_PART_NAME PART_HISITEST0
#define BFM_HISI_LOG_PART_MOUINT_POINT "/splash2"
#define BFM_HISI_LOG_ROOT_PATH "/splash2/reliability/boot_fail"
#define BFM_HISI_LOG_UPLOADING_PATH BFM_HISI_LOG_ROOT_PATH "/" BFM_UPLOADING_DIR_NAME
#define BFM_BFI_MAGIC_NUMBER ((unsigned int)0x42464958) /*BFIX*/

/* the bfi part has been devided into many pieces whose size are 4096 */
#define BFM_BFI_HEADER_SIZE (BFMR_SIZE_4K)
#define BFM_BFI_MEMBER_SIZE (BFMR_SIZE_4K)
#define BFM_BFI_RECORD_TOTAL_COUNT ((int)20)

#define BFM_HISI_BL1_BOOTFAIL_LOG_NAME "xloader_log"
#define BFM_HISI_BL2_BOOTFAIL_LOG_NAME "fastboot_log"
#define BFM_HISI_KERNEL_BOOTFAIL_LOG_NAME "last_kmsg"
#define BFM_HISI_RAMOOPS_BOOTFAIL_LOG_NAME "pmsg-ramoops-0"

#define BFM_HISI_WAIT_FOR_LOG_PART_TIMEOUT (40)
#define BFM_BFI_PART_MAX_COUNT (2)

#define BFM_MAX_U32 (0xFFFFFFFFU)
#define BFM_HISI_PWR_KEY_PRESS_KEYWORD "power key press interrupt"
#define BFM_BOOT_SUCCESS_TIME_IN_KENREL ((long long)60) /* unit: second */
#define BFM_DATAREADY_PATH "/proc/data-ready"


/*----local prototypes----------------------------------------------------------------*/

typedef struct bfmr_stagecode_to_hisi_keypoint
{
    bfmr_detail_boot_stage_e bfmr_stagecode;
    unsigned int hisi_keypoint;
} bfmr_stagecode_to_hisi_keypoint_t;

typedef struct bfmr_bootfail_errno_to_hisi_modid
{
    bfmr_bootfail_errno_e bootfail_errno;
    unsigned int hisi_modid;
} bfmr_bootfail_errno_to_hisi_modid_t;

typedef struct bfmr_bootfail_errno_to_hisi_reboot_reason
{
    bfmr_bootfail_errno_e bootfail_errno;
    unsigned int hisi_reboot_reason;
} bfmr_bootfail_errno_to_hisi_reboot_reason_t;

typedef struct
{
    char *bl1_log_start;
    unsigned int bl1_log_len;
    char *bl2_log_start;
    unsigned int bl2_log_len;
    char *kernel_log_start;
    unsigned int kernel_log_len;
    char *applogcat_log_start;
    unsigned int applogcat_log_len;
    bfmr_detail_boot_stage_e bfmr_stagecode;
    bfmr_bootfail_errno_e bootfail_errno;
    bool save_log_after_reboot;
} bfm_dfx_log_read_param_t;

typedef enum
{
    MODID_BFM_START = HISI_BB_MOD_BFM_START,
    MODID_BFM_BOOT_TIMEOUT,
    MODID_BFM_NATIVE_START,
    MODID_BFM_NATIVE_SECURITY_FAIL,
    MODID_BFM_NATIVE_CRITICAL_SERVICE_FAIL_TO_START,
    MODID_BFM_NATIVE_END,
    MODID_BFM_NATIVE_DATA_START,
    MODID_BFM_NATIVE_SYS_MOUNT_FAIL,
    MODID_BFM_NATIVE_DATA_MOUNT_FAILED_AND_ERASED,
    MODID_BFM_NATIVE_DATA_MOUNT_RO, /* added by qidechun */
    MODID_BFM_NATIVE_DATA_NOSPC, /* added by qidechun, NOSPC means data partition is full */
    MODID_BFM_NATIVE_VENDOR_MOUNT_FAIL,
    MODID_BFM_NATIVE_PANIC,
    MODID_BFM_NATIVE_DATA_END,
    MODID_BFM_FRAMEWORK_LEVEL_START,
    MODID_BFM_FRAMEWORK_SYS_SERVICE_LOAD_FAIL,
    MODID_BFM_FRAMEWORK_PREBOOT_BROADCAST_FAIL,
    MODID_BFM_FRAMEWORK_VM_OAT_FILE_DAMAGED,
    MODID_BFM_FRAMEWORK_LEVEL_END,
    MODID_BFM_FRAMEWORK_PACKAGE_MANAGER_SETTING_FILE_DAMAGED,
    MODID_BFM_BOOTUP_SLOWLY,
    MODID_BFM_HARDWARE_FAULT,
    MODID_BFM_END              = HISI_BB_MOD_BFM_END
} bfm_hisi_modid_for_bfmr;

typedef struct
{
    long long integer_part;
    long long decimal_part;
} bfm_kernel_print_time_t;

struct persistent_ram_buffer
{
    uint32_t sig;
    unsigned int start;
    unsigned int size;
    uint8_t data[0];
};

typedef struct fastboot_log_header
{
    unsigned int magic;
    unsigned int lastlog_start;
    unsigned int lastlog_offset;
    unsigned int log_start;
    unsigned int log_offset;
} fastboot_log_header_t;

typedef struct
{
    bfmr_hardware_fault_type_e bfmr_fault_type;
    unsigned long hisi_fault_type;
} bfmr_hw_fault_map_table_t;


/*----local variables-----------------------------------------------------------------*/

static bfmr_stagecode_to_hisi_keypoint_t s_bfmr_stagecode_hisi_keypoint_map_tbl[] = {
    {BL1_STAGE_START, STAGE_XLOADER_START},
    {BL1_PL1_STAGE_DDR_INIT_FAIL, STAGE_XLOADER_DDR_INIT_FAIL},
    {BL1_PL1_STAGE_DDR_INIT_OK, STAGE_XLOADER_DDR_INIT_OK},
    {BL1_PL1_STAGE_EMMC_INIT_FAIL, STAGE_XLOADER_EMMC_INIT_FAIL},
    {BL1_PL1_STAGE_EMMC_INIT_OK, STAGE_XLOADER_EMMC_INIT_OK},
    {BL1_PL1_STAGE_RD_VRL_FAIL, STAGE_XLOADER_RD_VRL_FAIL},
    {BL1_PL1_STAGE_CHECK_VRL_ERROR, STAGE_XLOADER_CHECK_VRL_ERROR},
    {BL1_PL1_STAGE_READ_FASTBOOT_FAIL, STAGE_XLOADER_READ_FASTBOOT_FAIL},
    {BL1_PL1_STAGE_LOAD_HIBENCH_FAIL, STAGE_XLOADER_LOAD_HIBENCH_FAIL},
    {BL1_PL1_STAGE_SEC_VERIFY_FAIL, STAGE_XLOADER_SEC_VERIFY_FAIL},
    {BL1_PL1_STAGE_VRL_CHECK_ERROR, STAGE_XLOADER_VRL_CHECK_ERROR},
    {BL1_PL1_SECURE_VERIFY_ERROR, STAGE_XLOADER_SECURE_VERIFY_ERROR},
    {BL1_STAGE_END, STAGE_XLOADER_END},

    {BL2_STAGE_START, STAGE_FASTBOOT_START},
    {BL2_STAGE_EMMC_INIT_START, STAGE_FASTBOOT_EMMC_INIT_START},
    {BL2_STAGE_EMMC_INIT_FAIL, STAGE_FASTBOOT_EMMC_INIT_FAIL},
    {BL2_STAGE_EMMC_INIT_OK, STAGE_FASTBOOT_EMMC_INIT_OK},
    {BL2_PL1_STAGE_DDR_INIT_START, STAGE_FASTBOOT_DDR_INIT_START},
    {BL2_PL1_STAGE_DISPLAY_INIT_START, STAGE_FASTBOOT_DISPLAY_INIT_START},
    {BL2_PL1_STAGE_PRE_BOOT_INIT_START, STAGE_FASTBOOT_PRE_BOOT_INIT_START},
    {BL2_PL1_STAGE_LD_OTHER_IMGS_START, STAGE_FASTBOOT_LD_OTHER_IMGS_START},
    {BL2_PL1_STAGE_LD_KERNEL_IMG_START, STAGE_FASTBOOT_LD_KERNEL_IMG_START},
    {BL2_PL1_STAGE_BOOT_KERNEL_START, STAGE_FASTBOOT_BOOT_KERNEL_START},
    {BL2_STAGE_END, STAGE_FASTBOOT_END},

    {KERNEL_EARLY_INITCALL, STAGE_KERNEL_EARLY_INITCALL},
    {KERNEL_PURE_INITCALL, STAGE_KERNEL_PURE_INITCALL},
    {KERNEL_CORE_INITCALL_SYNC, STAGE_KERNEL_CORE_INITCALL},
    {KERNEL_CORE_INITCALL_SYNC, STAGE_KERNEL_CORE_INITCALL_SYNC},
    {KERNEL_POSTCORE_INITCALL, STAGE_KERNEL_POSTCORE_INITCALL},
    {KERNEL_POSTCORE_INITCALL_SYNC, STAGE_KERNEL_POSTCORE_INITCALL_SYNC},
    {KERNEL_ARCH_INITCALL, STAGE_KERNEL_ARCH_INITCALL},
    {KERNEL_ARCH_INITCALLC, STAGE_KERNEL_ARCH_INITCALLC},
    {KERNEL_SUBSYS_INITCALL, STAGE_KERNEL_SUBSYS_INITCALL},
    {KERNEL_SUBSYS_INITCALL_SYNC, STAGE_KERNEL_SUBSYS_INITCALL_SYNC},
    {KERNEL_FS_INITCALL, STAGE_KERNEL_FS_INITCALL},
    {KERNEL_FS_INITCALL_SYNC, STAGE_KERNEL_FS_INITCALL_SYNC},

    {KERNEL_ROOTFS_INITCALL, STAGE_KERNEL_ROOTFS_INITCALL},
    {KERNEL_DEVICE_INITCALL, STAGE_KERNEL_DEVICE_INITCALL},
    {KERNEL_DEVICE_INITCALL_SYNC, STAGE_KERNEL_DEVICE_INITCALL_SYNC},
    {KERNEL_LATE_INITCALL, STAGE_KERNEL_LATE_INITCALL},
    {KERNEL_LATE_INITCALL_SYNC, STAGE_KERNEL_LATE_INITCALL_SYNC},
    {KERNEL_CONSOLE_INITCALL, STAGE_KERNEL_CONSOLE_INITCALL},
    {KERNEL_SECURITY_INITCALL, STAGE_KERNEL_SECURITY_INITCALL},
    {KERNEL_BOOTANIM_COMPLETE, STAGE_KERNEL_BOOTANIM_COMPLETE},

    {STAGE_INIT_START, STAGE_INIT_INIT_START},
    {STAGE_ON_EARLY_INIT, STAGE_INIT_ON_EARLY_INIT},
    {STAGE_ON_INIT, STAGE_INIT_ON_INIT},
    {STAGE_ON_EARLY_FS, STAGE_INIT_ON_EARLY_FS},
    {STAGE_ON_FS, STAGE_INIT_ON_FS},
    {STAGE_ON_POST_FS, STAGE_INIT_ON_POST_FS},
    {STAGE_ON_POST_FS_DATA, STAGE_INIT_ON_POST_FS_DATA},
    {STAGE_ON_EARLY_BOOT, STAGE_INIT_ON_EARLY_BOOT},
    {STAGE_ON_BOOT, STAGE_INIT_ON_BOOT},

    {STAGE_ZYGOTE_START, STAGE_ANDROID_ZYGOTE_START},
    {STAGE_VM_START, STAGE_ANDROID_VM_START},
    {STAGE_PHASE_WAIT_FOR_DEFAULT_DISPLAY, STAGE_ANDROID_PHASE_WAIT_FOR_DEFAULT_DISPLAY},
    {STAGE_PHASE_LOCK_SETTINGS_READY, STAGE_ANDROID_PHASE_LOCK_SETTINGS_READY},
    {STAGE_PHASE_SYSTEM_SERVICES_READY, STAGE_ANDROID_PHASE_SYSTEM_SERVICES_READY},
    {STAGE_PHASE_ACTIVITY_MANAGER_READY, STAGE_ANDROID_PHASE_ACTIVITY_MANAGER_READY},
    {STAGE_PHASE_THIRD_PARTY_APPS_CAN_START, STAGE_ANDROID_PHASE_THIRD_PARTY_APPS_CAN_START},

    {STAGE_BOOT_SUCCESS, STAGE_ANDROID_BOOT_SUCCESS},
};

static bfmr_bootfail_errno_to_hisi_modid_t s_bfmr_errno_to_hisi_modid_map_tbl[] = {
    {KERNEL_BOOT_TIMEOUT, MODID_BFM_BOOT_TIMEOUT},

    {SYSTEM_MOUNT_FAIL, (unsigned int)MODID_BFM_NATIVE_SYS_MOUNT_FAIL},
    {SECURITY_FAIL, MODID_BFM_NATIVE_SECURITY_FAIL},
    {CRITICAL_SERVICE_FAIL_TO_START, MODID_BFM_NATIVE_CRITICAL_SERVICE_FAIL_TO_START},
    {DATA_MOUNT_FAILED_AND_ERASED, MODID_BFM_NATIVE_DATA_MOUNT_FAILED_AND_ERASED},
    {DATA_MOUNT_RO, MODID_BFM_NATIVE_DATA_MOUNT_RO},
    {DATA_NOSPC, MODID_BFM_NATIVE_DATA_NOSPC},
    {VENDOR_MOUNT_FAIL, MODID_BFM_NATIVE_VENDOR_MOUNT_FAIL},
    {NATIVE_PANIC, MODID_BFM_NATIVE_PANIC},

    {SYSTEM_SERVICE_LOAD_FAIL, MODID_BFM_FRAMEWORK_SYS_SERVICE_LOAD_FAIL},
    {PREBOOT_BROADCAST_FAIL, MODID_BFM_FRAMEWORK_PREBOOT_BROADCAST_FAIL},
    {VM_OAT_FILE_DAMAGED, MODID_BFM_FRAMEWORK_VM_OAT_FILE_DAMAGED},
    {PACKAGE_MANAGER_SETTING_FILE_DAMAGED, MODID_BFM_FRAMEWORK_PACKAGE_MANAGER_SETTING_FILE_DAMAGED},
    {BOOTUP_SLOWLY, MODID_BFM_BOOTUP_SLOWLY},
    {BFM_HARDWARE_FAULT, MODID_BFM_HARDWARE_FAULT},

    //TBD: add more bootErrNo from kernel and application level.
};

struct rdr_exception_info_s s_rdr_excetion_info_for_bfmr[] = {
    {
        {0, 0}, (u32)MODID_BFM_BOOT_TIMEOUT, (u32)MODID_BFM_BOOT_TIMEOUT, RDR_ERR,
        RDR_REBOOT_NOW, RDR_AP | RDR_BFM, RDR_AP, RDR_AP,
        (u32)RDR_REENTRANT_DISALLOW, (u32)BFM_S_BOOT_TIMEOUT, 0, (u32)RDR_UPLOAD_YES, "bfm", "bfm-boot-TO",
        0, 0, 0
    },

    {
        {0, 0}, (u32)MODID_BFM_NATIVE_START, (u32)MODID_BFM_NATIVE_END, RDR_ERR,
        RDR_REBOOT_NOW, RDR_AP | RDR_BFM, RDR_AP, RDR_AP,
        (u32)RDR_REENTRANT_DISALLOW, (u32)BFM_S_NATIVE_BOOT_FAIL, 0, (u32)RDR_UPLOAD_YES, "bfm", "bfm-boot-TO",
        0, 0, 0
    },

    {
        {0, 0}, (u32)MODID_BFM_NATIVE_DATA_START, (u32)MODID_BFM_NATIVE_DATA_END, RDR_ERR,
        RDR_REBOOT_NOW, RDR_AP | RDR_BFM, RDR_AP, RDR_AP,
        (u32)RDR_REENTRANT_DISALLOW,(u32) BFM_S_NATIVE_DATA_FAIL, 0, (u32)RDR_UPLOAD_YES, "bfm", "bfm-boot-TO",
        0, 0, 0
    },

    {
        {0, 0}, (u32)MODID_BFM_FRAMEWORK_LEVEL_START, (u32)MODID_BFM_FRAMEWORK_LEVEL_END, RDR_ERR,
        RDR_REBOOT_NOW, RDR_AP | RDR_BFM, RDR_AP, RDR_AP,
        (u32)RDR_REENTRANT_DISALLOW, (u32)BFM_S_FW_BOOT_FAIL, 0, (u32)RDR_UPLOAD_YES, "bfm", "bfm-boot-TO",
        0, 0, 0
    },

    {
        {0, 0}, (u32)MODID_BFM_FRAMEWORK_PACKAGE_MANAGER_SETTING_FILE_DAMAGED, (u32)MODID_BFM_FRAMEWORK_PACKAGE_MANAGER_SETTING_FILE_DAMAGED, RDR_WARN,
        RDR_REBOOT_NO, RDR_AP | RDR_BFM, 0, RDR_AP,
        (u32)RDR_REENTRANT_DISALLOW, 0, 0, (u32)RDR_UPLOAD_YES, "bfm", "bfm-boot-TO",
        0, 0, 0
    },
    {
        {0, 0}, (u32)MODID_BFM_HARDWARE_FAULT, (u32)MODID_BFM_HARDWARE_FAULT, RDR_ERR,
        RDR_REBOOT_NOW, RDR_AP | RDR_BFM, RDR_AP, RDR_AP,
        (u32)RDR_REENTRANT_DISALLOW, (u32)BFM_S_BOOT_TIMEOUT, 0, (u32)RDR_UPLOAD_YES, "bfm", "bfm-boot-TO",
        0, 0, 0
    },
};

static DEFINE_SEMAPHORE(s_process_bottom_layer_boot_fail_sem);
static char *s_bottom_layer_log_buf = NULL;
static bfm_process_bootfail_param_t s_process_bootfail_param;
static bfm_bootfail_log_saving_param_t s_bootfail_log_saving_param;
static bfm_process_ocp_excp_param_t s_process_ocp_excp_param;
static bool s_chipsets_is_bootup_successfully = false;
static struct semaphore s_process_ocp_sem;
static bfmr_hw_fault_map_table_t s_hw_fault_map_table[] = {
    {HW_FAULT_OCP, HISI_PMIC_OCP_EVENT},
};

static bool s_is_recovery_mode = false;
static bool s_power_off_charge = false;


/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes--------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/

static bfmr_detail_boot_stage_e bfm_hisi_keypoint_to_bfmr_stagecode(unsigned int key_point);
static unsigned int bfm_bfmr_stagecode_to_hisi_keypoint(bfmr_detail_boot_stage_e bfmr_stagecode);
static int bfm_get_bfi_part_full_path(char *path_buf, unsigned int path_buf_len);
static int bfm_open_bfi_part_at_latest_bfi_info(int *fd);
static void bfm_dump_callback(u32 modid,
    u32 etype,
    u64 coreid,
    char *log_path,
    pfn_cb_dump_done pfn_cb);
static void bfm_reset_callback(u32 modid, u32 etype, u64 coreid);
static int bfm_register_callbacks_to_rdr(void);
static void bfm_reregister_exceptions_to_rdr(unsigned int modid, unsigned int exec_type);
static void bfm_register_exceptions_to_rdr(void);
static unsigned int bfm_get_hisi_modid_according_to_bootfail_errno(bfmr_bootfail_errno_e bootfail_errno);
static int bfm_read_bfi_part_header(bfm_bfi_header_info_t *pbfi_header_info);
static int bfm_update_bfi_part_header(bfm_bfi_header_info_t *pbfi_header_info);
static int bfm_get_rtc_time_of_latest_bootail_log_from_dfx_part(u64 *prtc_time);
static void bfm_save_log_to_bfi_part(u32 hisi_modid, bfm_process_bootfail_param_t *param);
static int bfm_read_latest_bfi_info(bfm_bfi_member_info_t *pbfi_memeber, unsigned int bfi_memeber_len);
static int bfm_update_latest_bfi_info(bfm_bfi_member_info_t *pbfi_memeber, unsigned int bfi_memeber_len);
static unsigned int bfm_read_file(char *buf, unsigned int buf_len, char *src_log_path, int copy_from_begin);
static unsigned int bfmr_capture_tombstone(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_vm_crash(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_vm_watchdog(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_normal_framework_bootfail_log(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_logcat_on_beta_version(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_beta_kmsg(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_critical_process_crash_log(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_fixed_framework_bootfail_log(char *buf, unsigned int buf_len, char *src_log_file_path);
static int bfm_save_bootfail_log_to_fs_immediately(
    bfm_process_bootfail_param_t *process_bootfail_param,
    bfm_bootfail_log_saving_param_t *psave_param,
    struct dfx_head_info *pdfx_head_info,
    int save_bottom_layer_bootfail_log);
static int bfm_check_validity_of_bootfail_log_in_dfx(bfm_bfi_member_info_t *pbfi_info, u64 rtc_time);
static int bfm_read_log_in_dfx_part(void);
static void bfm_release_buffer_saving_dfx_log(void);
static bool bfmr_userdata_is_ready(void);
static int bfm_add_bfi_info(bfm_bfi_member_info_t *pbfi_memeber, unsigned int bfi_memeber_len);
static int bfm_update_save_flag_in_bfi(bfm_bfi_member_info_t *pbfi_info);
static void bfm_sort_dfx_log_by_create_time(struct dfx_head_info *pdfx_head_info, char **paddr_buf, size_t addr_count);
static int bfm_process_ocp_callback(struct notifier_block *self, unsigned long val, void *data);


/*----function definitions--------------------------------------------------------------*/

static bfmr_detail_boot_stage_e bfm_hisi_keypoint_to_bfmr_stagecode(unsigned int hisi_keypoint)
{
    unsigned int i = 0;
    unsigned int count = (sizeof(s_bfmr_stagecode_hisi_keypoint_map_tbl)
        / sizeof(s_bfmr_stagecode_hisi_keypoint_map_tbl[0]));

    for (i = 0; i < count; i++)
    {
        if (hisi_keypoint == s_bfmr_stagecode_hisi_keypoint_map_tbl[i].hisi_keypoint)
        {
            return s_bfmr_stagecode_hisi_keypoint_map_tbl[i].bfmr_stagecode;
        }
    }

    BFMR_PRINT_ERR("hisi_keypoint: 0x%x has no bfmr_stagecode!\n", hisi_keypoint);

    return -1;
}


static unsigned int bfm_bfmr_stagecode_to_hisi_keypoint(bfmr_detail_boot_stage_e bfmr_stagecode)
{
    unsigned int i = 0;
    unsigned int count = (sizeof(s_bfmr_stagecode_hisi_keypoint_map_tbl)
        / sizeof(s_bfmr_stagecode_hisi_keypoint_map_tbl[0]));
    
    for (i = 0; i < count; i++)
    {
        if (s_bfmr_stagecode_hisi_keypoint_map_tbl[i].bfmr_stagecode == bfmr_stagecode)
        {
            return s_bfmr_stagecode_hisi_keypoint_map_tbl[i].hisi_keypoint;
        }
    }

    BFMR_PRINT_ERR("bfmr_stagecode: 0x%x has no hisi_keypoint!\n", bfmr_stagecode);

    return -1;
}


int bfm_get_boot_stage(bfmr_detail_boot_stage_e *pbfmr_bootstage)
{
    unsigned int hisi_keypoint;

    if (unlikely(NULL == pbfmr_bootstage))
    {
        BFMR_PRINT_INVALID_PARAMS("pbfmr_bootstage.\n");
        return -1;
    }

    hisi_keypoint = get_boot_keypoint();
    *pbfmr_bootstage = bfm_hisi_keypoint_to_bfmr_stagecode(hisi_keypoint);
    BFMR_PRINT_DBG("hisi_keypoint: 0x%x, pbfmr_bootstage: 0x%x\n", hisi_keypoint, *pbfmr_bootstage);

    return 0;
}


int bfm_set_boot_stage(bfmr_detail_boot_stage_e bfmr_bootstage)
{
    unsigned int hisi_keypoint = bfm_bfmr_stagecode_to_hisi_keypoint(bfmr_bootstage);
    set_boot_keypoint(hisi_keypoint);
    BFMR_PRINT_KEY_INFO("bfmr_bootstage: 0x%x hisi_keypoint: 0x%x\n", bfmr_bootstage, hisi_keypoint);

    return 0;
}


static unsigned int bfm_read_file(char *buf, unsigned int buf_len, char *src_log_path, int copy_from_begin)
{
    int fd_src = -1;
    char *ptemp = NULL;
    long src_file_len = 0L;
    mm_segment_t old_fs;
    long bytes_to_read = 0L;
    long bytes_read = 0L;
    long seek_pos = 0L;
    unsigned long bytes_read_total = 0;

    if (unlikely((NULL == buf) || (NULL == src_log_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("buf or src_log_path.\n");
        return 0;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    /* get the length of the source file */
    src_file_len = bfmr_get_file_length(src_log_path);
    if (src_file_len <= 0)
    {
        BFMR_PRINT_ERR("length of [%s] is: %ld!\n", src_log_path, src_file_len);
        goto __out;
    }

    fd_src = sys_open(src_log_path, O_RDONLY, 0);
    if (fd_src < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed![ret = %d]\n", src_log_path, fd_src);
        goto __out;
    }

    /* lseek for reading the latest log */
    seek_pos = (0 != copy_from_begin) ? (0L) : ((src_file_len <= (long)buf_len)
        ? (0L) : (src_file_len - (long)buf_len));
    if (sys_lseek(fd_src, (unsigned int)seek_pos, SEEK_SET) < 0)
    {
        BFMR_PRINT_ERR("sys_lseek [%s] failed!\n", src_log_path);
        goto __out;
    }

    /* read data from the user space source file */
    ptemp = buf;
    bytes_to_read = BFMR_MIN(src_file_len, (long)buf_len);
    while (bytes_to_read > 0)
    {
        bytes_read = bfmr_full_read(fd_src, ptemp, bytes_to_read);
        if (bytes_read < 0)
        {
            BFMR_PRINT_ERR("bfmr_full_read [%s] failed!\n", src_log_path);
            goto __out;
        }
        bytes_to_read -= bytes_read;
        ptemp += bytes_read;
        bytes_read_total += bytes_read;
    }

__out:
    if (fd_src >= 0)
    {
        sys_close(fd_src);
    }

    set_fs(old_fs);

    return bytes_read_total;
}


unsigned int bfmr_capture_log_from_src_file(char *buf, unsigned int buf_len, char *src_log_path)
{
    return bfm_read_file(buf, buf_len, src_log_path, 0);
}


static unsigned int bfmr_capture_tombstone(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfm_read_file(buf, buf_len, src_log_file_path, 1);
}


static unsigned int bfmr_capture_vm_crash(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfm_read_file(buf, buf_len, src_log_file_path, 0);
}


static unsigned int bfmr_capture_vm_watchdog(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfm_read_file(buf, buf_len, src_log_file_path, 0);
}


static unsigned int bfmr_capture_normal_framework_bootfail_log(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfm_read_file(buf, buf_len, src_log_file_path, 1);
}


static unsigned int bfmr_capture_logcat_on_beta_version(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    char src_path[BFMR_SIZE_256] = {0};

    if (bfm_get_symbol_link_path(src_log_file_path, src_path, sizeof(src_path)))
    {
        src_log_file_path = src_path;
    }

    return bfm_read_file(buf, buf_len, src_log_file_path, 0);
}


static unsigned int bfmr_capture_beta_kmsg(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    char src_path[BFMR_SIZE_256] = {0};

    if (bfm_get_symbol_link_path(src_log_file_path, src_path, sizeof(src_path)))
    {
        src_log_file_path = src_path;
    }

    return bfm_read_file(buf, buf_len, src_log_file_path, 0);
}


static unsigned int bfmr_capture_critical_process_crash_log(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfm_read_file(buf, buf_len, src_log_file_path, 0);
}


static unsigned int bfmr_capture_fixed_framework_bootfail_log(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfm_read_file(buf, buf_len, src_log_file_path, 1);
}


static unsigned int bfm_copy_user_log(char *buf, unsigned int buf_len, bfm_process_bootfail_param_t *pparam, bool from_begin)
{
    unsigned int bytes_captured = 0U;

    if (unlikely((NULL == buf) || (NULL == pparam) || (NULL == pparam->user_space_log_buf)))
    {
        BFMR_PRINT_INVALID_PARAMS("invalid parameter!\n");
        return 0U;
    }

    bytes_captured = BFMR_MIN(buf_len, pparam->user_space_log_read_len);
    memcpy(buf, pparam->user_space_log_buf + (from_begin ? 0 : ((pparam->user_space_log_read_len <= buf_len)
        ? (0) : (pparam->user_space_log_read_len - buf_len))), bytes_captured);
    BFMR_PRINT_KEY_INFO("Copy data from previous read data!buf_len: %u, file len: %ld!\n", buf_len, pparam->user_space_log_read_len);
    pparam->user_space_log_read_len = 0L;

    return bytes_captured;
}


/**
    @function: unsigned int bfmr_capture_log_from_system(char *buf, unsigned int buf_len, bfmr_log_src_t *src, int timeout)
    @brief: capture log from system.

    @param: buf [out], buffer to save log.
    @param: buf_len [in], length of buffer.
    @param: src [in], src log info.
    @param: timeout [in], timeout value of capturing log.

    @return: the length of the log has been captured.

    @note:
*/
unsigned int bfmr_capture_log_from_system(char *buf, unsigned int buf_len, bfmr_log_src_t *src, int timeout)
{
    unsigned int bytes_captured = 0U;
    bfm_dfx_log_read_param_t *pdfx_log_read_param = NULL;
    bfm_process_bootfail_param_t *pparam = NULL;

    if (unlikely((NULL == buf) || (NULL == src)))
    {
        BFMR_PRINT_INVALID_PARAMS("buf or src.\n");
        return 0U;
    }

    pparam = (bfm_process_bootfail_param_t *)src->log_save_additional_context;
    switch (src->log_type)
    {
    case LOG_TYPE_BOOTLOADER_1:
        {
            /* if your platform is not hisi, break or return 0 here */
            if (NULL == src->log_save_context)
            {
                break;
            }

            pdfx_log_read_param = (bfm_dfx_log_read_param_t *)src->log_save_context;
            if (pdfx_log_read_param->bl1_log_len != 0)
            {
                bytes_captured = BFMR_MIN(pdfx_log_read_param->bl1_log_len, buf_len);
                memcpy((void *)buf, (void *)pdfx_log_read_param->bl1_log_start, bytes_captured);
            }
            else
            {
                BFMR_PRINT_KEY_INFO("There is no BL1 boot fail log!\n");
            }
            break;
        }
    case LOG_TYPE_BOOTLOADER_2:
        {
            /* if your platform is not hisi, break or return 0 here */
            if (NULL == src->log_save_context)
            {
                break;
            }

            pdfx_log_read_param = (bfm_dfx_log_read_param_t *)src->log_save_context;
            if (pdfx_log_read_param->bl2_log_len != 0)
            {
                bytes_captured = BFMR_MIN(pdfx_log_read_param->bl2_log_len, buf_len);
                memcpy((void *)buf, (void *)pdfx_log_read_param->bl2_log_start, bytes_captured);
            }
            else
            {
                BFMR_PRINT_KEY_INFO("There is no BL2 boot fail log!\n");
            }
            break;
        }
    case LOG_TYPE_BFMR_TEMP_BUF:
        {
            /* if your platform is not hisi, break or return 0 here */
            down(&s_process_bottom_layer_boot_fail_sem);
            if (NULL != s_bottom_layer_log_buf)
            {
                bytes_captured = BFMR_MIN(DFX_USED_SIZE, buf_len);
                memcpy((void *)buf, (void *)s_bottom_layer_log_buf, bytes_captured);
                bfm_release_buffer_saving_dfx_log();
            }
            else
            {
                BFMR_PRINT_KEY_INFO("there is no bottom layer boot fail log!\n");
            }
            up(&s_process_bottom_layer_boot_fail_sem);
            break;
        }
    case LOG_TYPE_ANDROID_KMSG:
        {
            if (!bfm_is_beta_version())
            {
                break;
            }

            pdfx_log_read_param = (bfm_dfx_log_read_param_t *)src->log_save_context;
            if (NULL == pdfx_log_read_param)
            {
                break;
            }

            BFMR_PRINT_KEY_INFO("Save kmsg %s from android_logs instead!\n", (pdfx_log_read_param->save_log_after_reboot)
                ? ("after reboot") : ("now"));
            bytes_captured = bfmr_capture_beta_kmsg(buf, buf_len, (pdfx_log_read_param->save_log_after_reboot)
                ? (BFM_BETA_OLD_KMSG_LOG_PATH) : (BFM_BETA_KMSG_LOG_PATH));
            break;
        }
    case LOG_TYPE_TEXT_KMSG:
        {
            /* get text kmsg on commercial version */
            if (NULL == src->log_save_context)
            {
                break;
            }

            pdfx_log_read_param = (bfm_dfx_log_read_param_t *)src->log_save_context;
            if (pdfx_log_read_param->kernel_log_len != 0)
            {
                bytes_captured = BFMR_MIN(pdfx_log_read_param->kernel_log_len, buf_len);
                memcpy((void *)buf, (void *)pdfx_log_read_param->kernel_log_start, bytes_captured);
            }
            else
            {
                BFMR_PRINT_KEY_INFO("There is no Kernel boot fail log!\n");
            }
            break;
        }
    case LOG_TYPE_RAMOOPS:
        {
            if (NULL == src->log_save_context)
            {
                break;
            }

            pdfx_log_read_param = (bfm_dfx_log_read_param_t *)src->log_save_context;
            if (pdfx_log_read_param->applogcat_log_len != 0)
            {
                bytes_captured = BFMR_MIN(pdfx_log_read_param->applogcat_log_len, buf_len);
                memcpy((void *)buf, (void *)pdfx_log_read_param->applogcat_log_start, bytes_captured);
            }
            else
            {
                BFMR_PRINT_KEY_INFO("There is no applogcat boot fail log!\n");
            }
            break;
        }
    case LOG_TYPE_BETA_APP_LOGCAT:
        {
            if (bfm_is_beta_version())
            {
                pdfx_log_read_param = (bfm_dfx_log_read_param_t *)src->log_save_context;
                if (NULL == pdfx_log_read_param)
                {
                    break;
                }

                BFMR_PRINT_KEY_INFO("Save logcat %s from android_logs instead!\n", (pdfx_log_read_param->save_log_after_reboot)
                    ? ("after reboot") : ("now"));
                bytes_captured = bfmr_capture_logcat_on_beta_version(buf, buf_len, (pdfx_log_read_param->save_log_after_reboot)
                    ? (BFM_OLD_LOGCAT_FILE_PATH) : (BFM_LOGCAT_FILE_PATH));
            }
            else
            {
                bytes_captured = 0U;
            }
            break;
        }
    case LOG_TYPE_CRITICAL_PROCESS_CRASH:
        {
            bytes_captured = (NULL == pparam || pparam->user_space_log_read_len <= 0) ? bfmr_capture_critical_process_crash_log(
                buf, buf_len, src->src_log_file_path) : (bfm_copy_user_log(buf, buf_len, pparam, false));
            break;
        }
    case LOG_TYPE_VM_TOMBSTONES:
        {
            bytes_captured = (NULL == pparam || pparam->user_space_log_read_len <= 0) ? bfmr_capture_tombstone(
                buf, buf_len, src->src_log_file_path) : (bfm_copy_user_log(buf, buf_len, pparam, true));
            break;
        }
    case LOG_TYPE_VM_CRASH:
        {
            bytes_captured = (NULL == pparam || pparam->user_space_log_read_len <= 0) ? bfmr_capture_vm_crash(
                buf, buf_len, src->src_log_file_path) : (bfm_copy_user_log(buf, buf_len, pparam, false));
            break;
        }
    case LOG_TYPE_VM_WATCHDOG:
        {
            bytes_captured = (NULL == pparam || pparam->user_space_log_read_len <= 0) ? bfmr_capture_vm_watchdog(
                buf, buf_len, src->src_log_file_path) : (bfm_copy_user_log(buf, buf_len, pparam, false));
            break;
        }
    case LOG_TYPE_NORMAL_FRAMEWORK_BOOTFAIL_LOG:
        {
            bytes_captured = (NULL == pparam || pparam->user_space_log_read_len <= 0) ? bfmr_capture_normal_framework_bootfail_log(
                buf, buf_len, src->src_log_file_path) : (bfm_copy_user_log(buf, buf_len, pparam, true));
            break;
        }
    case LOG_TYPE_FIXED_FRAMEWORK_BOOTFAIL_LOG:
        {
            bytes_captured = (NULL == pparam || pparam->user_space_log_read_len <= 0) ? bfmr_capture_fixed_framework_bootfail_log(
                buf, buf_len, BFM_FRAMEWORK_BOOTFAIL_LOG_PATH) : (bfm_copy_user_log(buf, buf_len, pparam, true));
            break;
        }
    default:
        {
            BFMR_PRINT_ERR("Invalid log type: [%d]\n", (int)(src->log_type));
            break;
        }
    }

    return bytes_captured;
}


static int bfm_check_validity_of_bootfail_log_in_dfx(bfm_bfi_member_info_t *pbfi_info, u64 rtc_time)
{
    char *pdata_buf = NULL;
    char *bfi_part_full_path = NULL;
    long bytes_read = 0L;
    long seek_result = 0L;
    mm_segment_t old_fs;
    int ret = -1;
    int fd = -1;
    int i = 0;

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    bfi_part_full_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN + 1);
    if (NULL == bfi_part_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)bfi_part_full_path, 0, (BFMR_DEV_FULL_PATH_MAX_LEN + 1));

    pdata_buf = (char *)bfmr_malloc(BFM_BFI_MEMBER_SIZE);
    if (NULL == pdata_buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pdata_buf, 0, BFM_BFI_MEMBER_SIZE);

    ret = bfm_get_bfi_part_full_path(bfi_part_full_path, BFMR_DEV_FULL_PATH_MAX_LEN);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("failed ot get full path of bfi part!\n");
        goto __out;
    }

    ret = -1;
    fd = sys_open(bfi_part_full_path, O_RDONLY, BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed [ret = %d]\n", bfi_part_full_path, fd);
        goto __out;
    }

    /* seek the header in the beginning of the BFI part */
    seek_result = sys_lseek(fd, BFM_BFI_HEADER_SIZE, SEEK_SET);
    if (seek_result != (long)BFM_BFI_HEADER_SIZE)
    {
        BFMR_PRINT_ERR("sys_lseek [%s] failed!, seek_results: %ld ,it must be: %ld\n",
            bfi_part_full_path, seek_result, (long)BFM_BFI_HEADER_SIZE);
        goto __out;
    }

    for (i = 0; i < BFM_BFI_RECORD_TOTAL_COUNT; i++)
    {
        memset((void *)pdata_buf, 0, BFM_BFI_MEMBER_SIZE);
        bytes_read = bfmr_full_read(fd, pdata_buf, BFM_BFI_MEMBER_SIZE);
        if (bytes_read < 0)
        {
            BFMR_PRINT_ERR("bfmr_full_read [%s] failed! [ret = %ld]\n", bfi_part_full_path, bytes_read);
            goto  __out;
        }

        memcpy((void *)pbfi_info, (void *)pdata_buf, sizeof(bfm_bfi_member_info_t));
        if ((pbfi_info->rtcValue == rtc_time) && (rtc_time != 0ULL))
        {
            BFMR_PRINT_KEY_INFO("RTC time in BFI[%d]: %llx, RTC time in DFX: %llx\n", i, pbfi_info->rtcValue, rtc_time);
            ret = 0;
            break;
        }
    }

    BFMR_PRINT_KEY_INFO("get bfi info %s!\n", (0 == ret) ? ("successfully") : ("failed"));

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }

    set_fs(old_fs);

    bfmr_free(bfi_part_full_path);
    bfmr_free(pdata_buf);

    return ret;
}


static int bfm_update_save_flag_in_bfi(bfm_bfi_member_info_t *pbfi_info)
{
    bfm_bfi_member_info_t *pbfi_info_local = NULL;
    char *bfi_part_full_path = NULL;
    long bytes_read = 0L;
    long bytes_write = 0L;
    long seek_result = 0L;
    mm_segment_t old_fs;
    int ret = -1;
    int fd = -1;
    int i = 0;

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    bfi_part_full_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN + 1);
    if (NULL == bfi_part_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)bfi_part_full_path, 0, (BFMR_DEV_FULL_PATH_MAX_LEN + 1));

    pbfi_info_local = (bfm_bfi_member_info_t *)bfmr_malloc(BFM_BFI_MEMBER_SIZE);
    if (NULL == pbfi_info_local)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pbfi_info_local, 0, BFM_BFI_MEMBER_SIZE);

    ret = bfm_get_bfi_part_full_path(bfi_part_full_path, BFMR_DEV_FULL_PATH_MAX_LEN);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("failed ot get full path of bfi part!\n");
        goto __out;
    }

    ret = -1;
    fd = sys_open(bfi_part_full_path, O_RDWR, BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed [ret = %d]\n", bfi_part_full_path, fd);
        goto __out;
    }

    /* seek the header in the beginning of the BFI part */
    seek_result = sys_lseek(fd, BFM_BFI_HEADER_SIZE, SEEK_SET);
    if (seek_result != (long)BFM_BFI_HEADER_SIZE)
    {
        BFMR_PRINT_ERR("sys_lseek [%s] failed!, seek_results: %ld ,it must be: %ld\n",
            bfi_part_full_path, seek_result, (long)BFM_BFI_HEADER_SIZE);
        goto __out;
    }

    for (i = 0; i < BFM_BFI_RECORD_TOTAL_COUNT; i++)
    {
        memset((void *)pbfi_info_local, 0, BFM_BFI_MEMBER_SIZE);
        bytes_read = bfmr_full_read(fd, (char *)pbfi_info_local, BFM_BFI_MEMBER_SIZE);
        if (bytes_read < 0)
        {
            BFMR_PRINT_ERR("bfmr_full_read [%s] failed! [ret = %ld]\n", bfi_part_full_path, bytes_read);
            goto  __out;
        }

        if (pbfi_info->rtcValue == pbfi_info_local->rtcValue)
        {
            seek_result = sys_lseek(fd, (long)(BFM_BFI_HEADER_SIZE + i * BFM_BFI_MEMBER_SIZE), SEEK_SET);/*lint !e647 */
            if (seek_result != (long)(BFM_BFI_HEADER_SIZE + i * BFM_BFI_MEMBER_SIZE))/*lint !e647 */
            {
                BFMR_PRINT_ERR("sys_lseek [%s] failed!, seek_results: %ld ,it must be: %ld\n",
                    bfi_part_full_path, seek_result, (long)(BFM_BFI_HEADER_SIZE + i * BFM_BFI_MEMBER_SIZE));/*lint !e647 */
                goto __out;
            }

            bytes_write = bfmr_full_write(fd, (char *)pbfi_info, BFM_BFI_MEMBER_SIZE);
            if (bytes_write <= 0L)
            {
                BFMR_PRINT_ERR("bfmr_full_write [%s] failed! [ret = %ld]\n", bfi_part_full_path, bytes_write);
                goto  __out;
            }
            ret = 0;
            break;
        }
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }
    set_fs(old_fs);
    bfmr_free(bfi_part_full_path);
    bfmr_free(pbfi_info_local);
    BFMR_PRINT_KEY_INFO("update log-saving flag in bfi part %s!\n", (0 == ret) ? ("successfully") : ("failed"));

    return ret;
}


static void bfm_sort_dfx_log_by_create_time(struct dfx_head_info *pdfx_head_info, char **paddr_buf, size_t addr_count)
{
    int log_max_count = 0;
    int i = 0;
    int j = 0;

    if (unlikely((NULL == pdfx_head_info) || (NULL == paddr_buf)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdfx_head_info or paddr_buf.\n");
        return;
    }

    log_max_count = BFMR_MIN(addr_count, BFMR_MIN(TOTAL_NUMBER, pdfx_head_info->total_number));/*lint !e574 */
    for (i = 0; i < log_max_count; i++)
    {
        paddr_buf[i] = (char *)((char *)pdfx_head_info + pdfx_head_info->every_number_addr[i]);
    }

    for (i = 0; i < log_max_count; i++)
    {
        for (j = 0; j < (log_max_count - i - 1); j++)
        {
            if (((struct every_number_info *)(paddr_buf[j + 1]))->rtc_time < ((struct every_number_info *)(paddr_buf[j]))->rtc_time)/*lint !e679 */
            {
                char *ptemp_addr = paddr_buf[j];
                paddr_buf[j] = paddr_buf[j + 1];/*lint !e679 */
                paddr_buf[j + 1] = ptemp_addr;/*lint !e679 */
            }
        }
    }
}


static int bfm_save_bootfail_log_to_fs_immediately(
    bfm_process_bootfail_param_t *process_bootfail_param,
    bfm_bootfail_log_saving_param_t *psave_param,
    struct dfx_head_info *pdfx_head_info,
    int save_bottom_layer_bootfail_log)
{
    int ret = -1;
    int log_idx = -1;
    int log_max_count = 0;
    struct every_number_info *pdfx_log_info;
    bfm_bfi_member_info_t *pbfi_info = NULL;
    bfm_dfx_log_read_param_t *pdfx_log_read_param = NULL;
    char *paddr_buf[TOTAL_NUMBER] = {0};

    BFMR_PRINT_ENTER();
    if (unlikely((NULL == process_bootfail_param)
        || (NULL == psave_param)
        || (NULL == (void *)psave_param->capture_and_save_bootfail_log)
        || (NULL == pdfx_head_info)))
    {
        BFMR_PRINT_INVALID_PARAMS("psave_param or pdfx_head_info or pdfx_head_info->capture_and_save_bootfail_log is NULL\n");
        return -1;
    }

    pbfi_info = (bfm_bfi_member_info_t *)bfmr_malloc(sizeof(bfm_bfi_member_info_t));
    if (NULL == pbfi_info)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pbfi_info, 0, (sizeof(bfm_bfi_member_info_t)));

    pdfx_log_read_param = (bfm_dfx_log_read_param_t *)bfmr_malloc(sizeof(bfm_dfx_log_read_param_t));
    if (NULL == pdfx_log_read_param)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pdfx_log_read_param, 0, sizeof(bfm_dfx_log_read_param_t));

    /* parse and save boot fail log saved in DFX part */
    log_idx = 0;
    log_max_count = BFMR_MIN(TOTAL_NUMBER, pdfx_head_info->total_number);
    memset(paddr_buf, 0, sizeof(paddr_buf));
    bfm_sort_dfx_log_by_create_time(pdfx_head_info, &(paddr_buf[0]), sizeof(paddr_buf) / sizeof(paddr_buf[0]));
    while (log_idx < log_max_count)
    {
        pdfx_log_info = (struct every_number_info *)paddr_buf[log_idx];
        BFMR_PRINT_KEY_INFO("reboot_type: %llx\n", pdfx_log_info->reboot_type);
        memset((void *)pbfi_info, 0, (sizeof(bfm_bfi_member_info_t)));
        if (0 != bfm_check_validity_of_bootfail_log_in_dfx(pbfi_info, pdfx_log_info->rtc_time))
        {
            BFMR_PRINT_ERR("the log in dfx is invalid, whose index is [%d], continue to check the next one\n", log_idx);
            goto __next;
        }

        if (0 != pbfi_info->is_log_saved)
        {
            BFMR_PRINT_ERR("the log in dfx has been saved, continue to check the next one\n");
            goto __next;
        }

        memset((void *)pdfx_log_read_param, 0, sizeof(bfm_dfx_log_read_param_t));
        process_bootfail_param->save_bottom_layer_bootfail_log = ((bfmr_detail_boot_stage_e)pbfi_info->bfmStageCode
            >= NATIVE_STAGE_START) ? (0) : (save_bottom_layer_bootfail_log);
        process_bootfail_param->bootfail_errno = pbfi_info->bfmErrNo;
        process_bootfail_param->bootfail_time = pbfi_info->rtcValue;
        process_bootfail_param->bootup_time =  pbfi_info->bootup_time;
        process_bootfail_param->recovery_method = (bfr_recovery_method_e)pbfi_info->rcvMethod;
        process_bootfail_param->boot_stage = (bfmr_detail_boot_stage_e)pbfi_info->bfmStageCode;
        process_bootfail_param->is_user_sensible = (int)pbfi_info->isUserPerceptiable;
        process_bootfail_param->is_system_rooted = (int)pbfi_info->isSystemRooted;
        process_bootfail_param->dmd_num = pbfi_info->dmdErrNo;
        process_bootfail_param->is_bootup_successfully = pbfi_info->is_bootup_successfully;
        process_bootfail_param->suggested_recovery_method = pbfi_info->suggested_recovery_method;
        process_bootfail_param->reboot_type = pbfi_info->reboot_type;
        strncpy(process_bootfail_param->excepInfo, pbfi_info->excepInfo, sizeof(pbfi_info->excepInfo) -1);
        process_bootfail_param->log_save_context = (void *)pdfx_log_read_param;
        pdfx_log_read_param->bl1_log_start = NULL; /* TODO: captue BL1 bootfail log on HISI */
        pdfx_log_read_param->bl1_log_len = 0U; /* TODO: captue BL1 bootfail log on HISI */
        pdfx_log_read_param->bl2_log_start = (char *)pdfx_log_info + pdfx_log_info->fastbootlog_start_addr;
        pdfx_log_read_param->bl2_log_len = pdfx_log_info->fastbootlog_size;
        pdfx_log_read_param->kernel_log_start = (char *)pdfx_log_info + pdfx_log_info->last_kmsg_start_addr;
        pdfx_log_read_param->kernel_log_len = pdfx_log_info->last_kmsg_size;
        if (bfm_is_beta_version())
        {
            pdfx_log_read_param->applogcat_log_start = (char *)pdfx_log_info + pdfx_log_info->last_applog_start_addr;
            pdfx_log_read_param->applogcat_log_len = (unsigned int)pdfx_log_info->last_applog_size;
        }
        else
        {
            pdfx_log_read_param->applogcat_log_start = NULL;
            pdfx_log_read_param->applogcat_log_len = 0U;
        }
        pdfx_log_read_param->save_log_after_reboot = process_bootfail_param->save_log_after_reboot;
        ret = psave_param->capture_and_save_bootfail_log(process_bootfail_param);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("Save boot fail log failed!\n");
        }
        else
        {
            pbfi_info->is_log_saved = ~0;
            (void)bfm_update_save_flag_in_bfi(pbfi_info);
        }

__next:
        log_idx++;
    }

__out:
    bfmr_free(pbfi_info);
    bfmr_free(pdfx_log_read_param);
    BFMR_PRINT_EXIT();

    return ret;
}


/**
    @function: unsigned int bfm_parse_and_save_bottom_layer_bootfail_log(
        bfm_process_bootfail_param_t *process_bootfail_param,
        char *buf,
        unsigned int buf_len)
    @brief: parse and save bottom layer bootfail log.

    @param: process_bootfail_param[in], bootfail process params.
    @param: buf [in], buffer to save log.
    @param: buf_len [in], length of buffer.

    @return: 0 - success, -1 - failed.

    @note: HISI must realize this function in detail, the other platform can return 0 when enter this function
*/
int bfm_parse_and_save_bottom_layer_bootfail_log(
    bfm_process_bootfail_param_t *process_bootfail_param,
    char *buf,
    unsigned int buf_len)
{
    int ret = -1;
    struct dfx_head_info *pdfx_head_info = (struct dfx_head_info *)buf;

    if (unlikely((NULL == process_bootfail_param) || (NULL == buf)))
    {
        BFMR_PRINT_INVALID_PARAMS("process_bootfail_param or buf.\n");
        return -1;
    }

    /* 1. check the validity of source log */
    BFMR_PRINT_KEY_INFO("There is %u bytes bottom layer log to be parsed! its size shoule be: %u\n",
        buf_len, bfm_get_dfx_log_length());
    down(&s_process_bottom_layer_boot_fail_sem);
    pdfx_head_info = (struct dfx_head_info *)buf;
    BFMR_PRINT_ERR("Info of dfx part, magic: 0x%08x, it must be: 0x%08x; it can save up to %d logs!\n",
        (unsigned int)pdfx_head_info->magic, (unsigned int)DFX_MAGIC_NUMBER, (int)TOTAL_NUMBER);
    up(&s_process_bottom_layer_boot_fail_sem);

    /* 2. wait for the log part */
    BFMR_PRINT_SIMPLE_INFO("============ wait for log part start =============\n");
    if (0 != bfmr_wait_for_part_mount_with_timeout(bfm_get_bfmr_log_part_mount_point(),
        BFM_HISI_WAIT_FOR_LOG_PART_TIMEOUT))
    {
        BFMR_PRINT_ERR("[%s] is not ready, the boot fail logs can't be saved!\n", bfm_get_bfmr_log_part_mount_point());
        goto __out;
    }
    BFMR_PRINT_SIMPLE_INFO("============ wait for log part end =============\n");

    process_bootfail_param->save_log_after_reboot = true;
    ret = bfm_save_bootfail_log_to_fs_immediately(process_bootfail_param,
        &s_bootfail_log_saving_param, pdfx_head_info, 1);
    BFMR_PRINT_KEY_INFO("Save boot fail log %s!\n", (0 == ret) ? ("successfully") : ("failed"));

__out:
    return ret; 
}


/**
    @function: int bfmr_save_log_to_fs(char *dst_file_path, char *buf, unsigned int log_len, int append)
    @brief: save log to file system.

    @param: dst_file_path [in], full path of the dst log file.
    @param: buf [in], buffer saving the boto fail log.
    @param: log_len [in], length of the log.
    @param: append [in], 0 - not append, 1 - append.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_save_log_to_fs(char *dst_file_path, char *buf, unsigned int log_len, int append)
{
    int ret = -1;
    int fd = -1;
    mm_segment_t fs = 0;
    long bytes_written = 0L;

    if (unlikely(NULL == dst_file_path || NULL == buf))
    {
        BFMR_PRINT_INVALID_PARAMS("dst_file_path or buf.\n");
        return -1;
    }

    if (0U == log_len)
    {
        BFMR_PRINT_KEY_INFO("There is no need to save log whose length is: %u\n", log_len);
        return 0;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    /* 1. open file for writing, please note the parameter-append */
    fd = sys_open(dst_file_path, O_CREAT | O_RDWR | ((0 == append) ? (0U) : O_APPEND), BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed, fd: %d\n", dst_file_path, fd);
        goto __out;
    }

    /* 2. seek file */
    ret = sys_lseek(fd, 0, ((0 == append) ? (SEEK_SET) : (SEEK_END)));
    if (ret < 0)
    {
        BFMR_PRINT_ERR("sys_lseek [%s] failed, ret: %d\n", dst_file_path, ret);
        goto __out;
    }

    /* 3. write file */
    bytes_written = bfmr_full_write(fd, buf, log_len);
    if ((long)log_len != bytes_written)
    {
        BFMR_PRINT_ERR("bfmr_full_write [%s] failed, log_len: %ld bytes_written: %ld\n",
            dst_file_path, (long)log_len, bytes_written);
        goto __out;
    }

    /* 4. change own and mode for the file */
    bfmr_change_own_mode(dst_file_path, BFMR_AID_ROOT, BFMR_AID_SYSTEM, BFMR_FILE_LIMIT);

    /* 5. write successfully, modify the value of ret */
    ret = 0;

__out:
    if (fd >= 0)
    {
        sys_fsync(fd);
        sys_close(fd);
    }

    set_fs(fs);

    return ret;
}


/**
    @function: int bfmr_save_log_to_raw_part(char *raw_part_name, unsigned long long offset, char *buf, unsigned int log_len)
    @brief: save log to raw partition.

    @param: raw_part_name [in], such as: rrecord/recovery/boot, not the full path of the device.
    @param: offset [in], offset from the beginning of the "raw_part_name".
    @param: buf [in], buffer saving log.
    @param: buf_len [in], length of the log which is <= the length of buf.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_save_log_to_raw_part(char *raw_part_name, unsigned long long offset, char *buf, unsigned int log_len)
{
    int ret = -1;
    int fd = -1;
    char *pdev_full_path = NULL;

    if (unlikely(NULL == raw_part_name || NULL == buf))
    {
        BFMR_PRINT_INVALID_PARAMS("raw_part_name or buf.\n");
        return -1;
    }

    pdev_full_path = (char *)bfmr_malloc((BFMR_DEV_FULL_PATH_MAX_LEN + 1) * sizeof(char));
    if (NULL == pdev_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pdev_full_path, 0, ((BFMR_DEV_FULL_PATH_MAX_LEN + 1) * sizeof(char)));

    ret = bfmr_get_device_full_path(raw_part_name, pdev_full_path, BFMR_DEV_FULL_PATH_MAX_LEN);
    if (0 != ret)
    {
        goto __out;
    }

    ret = bfmr_write_emmc_raw_part(pdev_full_path, offset, buf, (unsigned long long)log_len);
    if (0 != ret)
    {
        ret = -1;
        BFMR_PRINT_ERR("sys_open [%s] failed fd: %d!\n", pdev_full_path, fd);
        goto __out;
    }

__out:
    bfmr_free(pdev_full_path);

    return ret;
}


/**
    @function: int bfmr_save_log_to_mem_buffer(char *dst_buf, unsigned int dst_buf_len, char *src_buf, unsigned int log_len)
    @brief: save log to memory buffer.

    @param: dst_buf [in] dst buffer.
    @param: dst_buf_len [in], length of the dst buffer.
    @param: src_buf [in] ,source buffer saving log.
    @param: log_len [in], length of the buffer.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_save_log_to_mem_buffer(char *dst_buf, unsigned int dst_buf_len, char *src_buf, unsigned int log_len)
{
    if (unlikely(NULL == dst_buf || NULL == src_buf))
    {
        BFMR_PRINT_INVALID_PARAMS("dst_buf or src_buf.\n");
        return -1;
    }

    memcpy(dst_buf, src_buf, BFMR_MIN(dst_buf_len, log_len));

    return 0;
}


/**
    @function: char* bfm_get_bfmr_log_root_path(void)
    @brief: get log root path

    @param: none

    @return: the path of bfmr log's root path.

    @note:
*/
char* bfm_get_bfmr_log_root_path(void)
{
    return (char *)BFM_HISI_LOG_ROOT_PATH;
}


char* bfm_get_bfmr_log_uploading_path(void)
{
    return (char *)BFM_HISI_LOG_UPLOADING_PATH;
}


char* bfm_get_bfmr_log_part_mount_point(void)
{
    return (char *)BFM_HISI_LOG_PART_MOUINT_POINT;
}


static int bfm_open_bfi_part_at_latest_bfi_info(int *fd)
{
    int ret = -1;
    mm_segment_t old_fs;
    bfm_bfi_header_info_t *pbfi_header = NULL;
    int bytes_read = 0;
    int cur_number;
    int offset;
    int seek_result = -1;
    char *bfi_dev_path = NULL;

    if (unlikely(NULL == fd))
    {
        BFMR_PRINT_INVALID_PARAMS("fd.\n");
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    /* 1. find the full path of bfi part */
    bfi_dev_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN);
    if (NULL == bfi_dev_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(bfi_dev_path, 0, BFMR_DEV_FULL_PATH_MAX_LEN);

    if (0 != bfm_get_bfi_part_full_path(bfi_dev_path, BFMR_DEV_FULL_PATH_MAX_LEN))
    {
        BFMR_PRINT_ERR("get full path of bfi part failed!\n");
        goto __out;
    }

    pbfi_header = (bfm_bfi_header_info_t *)bfmr_malloc(BFM_BFI_HEADER_SIZE);
    if (NULL == pbfi_header)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    *fd = sys_open(bfi_dev_path, O_RDWR, BFMR_FILE_LIMIT);
    if (*fd < 0)
    {
        BFMR_PRINT_ERR("sys_open: [%s] failed![ret=%d]\n", bfi_dev_path, *fd);
        goto __out;
    }

    /* 2. read header of bfi */
    memset(pbfi_header, 0, BFM_BFI_HEADER_SIZE);
    bytes_read = bfmr_full_read(*fd, (char *)pbfi_header, BFM_BFI_HEADER_SIZE);
    if (bytes_read != BFM_BFI_HEADER_SIZE)
    {
        BFMR_PRINT_ERR("read [%s] fail![%d]\n", bfi_dev_path, bytes_read);
        goto __out;
    }

    /* 3. read out latest member info */
    cur_number = pbfi_header->cur_number;
    cur_number = ((0 == cur_number) ? (BFM_BFI_RECORD_TOTAL_COUNT - 1) : (cur_number - 1));
    offset = BFM_BFI_HEADER_SIZE + cur_number * pbfi_header->every_number_size;
    seek_result = sys_lseek(*fd, offset, SEEK_SET);
    if (seek_result != offset)
    {
        BFMR_PRINT_ERR("lseek [%s] fail[%d]\n", bfi_dev_path, ret);
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    set_fs(old_fs);
    bfmr_free(bfi_dev_path);
    bfmr_free(pbfi_header);

    return ret;
}


static int bfm_read_latest_bfi_info(bfm_bfi_member_info_t *pbfi_memeber, unsigned int bfi_memeber_len)
{
    int ret = -1;
    mm_segment_t old_fs;
    int fd = -1;
    long bytes_read = 0;

    if (unlikely(NULL == pbfi_memeber))
    {
        BFMR_PRINT_INVALID_PARAMS("pbfi_memeber.\n");
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    if (0 != bfm_open_bfi_part_at_latest_bfi_info(&fd))
    {
        BFMR_PRINT_ERR("open bfi part failed!\n");
        goto __out;
    }

    bytes_read = bfmr_full_read(fd, (char*)pbfi_memeber, bfi_memeber_len);
    if (bytes_read != (long)bfi_memeber_len)
    {
        BFMR_PRINT_ERR("bfmr_full_read bfi part failed!bytes_read: %ld it should be: %ld\n",
            bytes_read, (long)bfi_memeber_len);
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }

    set_fs(old_fs);

    return ret;
}


static int bfm_update_latest_bfi_info(bfm_bfi_member_info_t *pbfi_memeber, unsigned int bfi_memeber_len)
{
    int ret = -1;
    int fd = -1;
    long bytes_write = 0;
    mm_segment_t old_fs;

    if (unlikely(NULL == pbfi_memeber))
    {
        BFMR_PRINT_INVALID_PARAMS("pbfi_memeber.\n");
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);/*lint !e501 */
    if (0 != bfm_open_bfi_part_at_latest_bfi_info(&fd))
    {
        BFMR_PRINT_ERR("open bfi part failed!\n");
        goto __out;
    }

    bytes_write = bfmr_full_write(fd, (char*)pbfi_memeber, bfi_memeber_len);
    if (bytes_write != (long)bfi_memeber_len)
    {
        BFMR_PRINT_ERR("write bfi part failed!bytes_write: %ld, it should be: %ld\n",
            bytes_write, (long)bfi_memeber_len);
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    if (fd >= 0)
    {
        sys_fsync(fd);
        sys_close(fd);
    }

    set_fs(old_fs);

    return ret;
}


static int bfm_get_bfi_part_full_path(char *path_buf, unsigned int path_buf_len)
{
    int ret = -1;
    int i = 0;
    bool find_full_path_of_rrecord_part = false;
    char *bfi_part_names[BFM_BFI_PART_MAX_COUNT] = {BFM_HISI_BFI_PART_NAME,
        BFM_HISI_BFI_BACKUP_PART_NAME};
    int count = sizeof(bfi_part_names) / sizeof(bfi_part_names[0]);

    if (unlikely(NULL == path_buf))
    {
        BFMR_PRINT_INVALID_PARAMS("path_buf.\n");
        return -1;
    }

    for (i = 0; i < count; i++)
    {
        memset((void *)path_buf, 0, path_buf_len);

        ret = bfmr_get_device_full_path(bfi_part_names[i], path_buf, path_buf_len);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("get full path for device [%s] failed!\n", bfi_part_names[i]);
            continue;
        }

        find_full_path_of_rrecord_part = true;
        break;
    }

    if (!find_full_path_of_rrecord_part)
    {
        ret = -1;
    }
    else
    {
        ret = 0;
    }

    return ret;
}/*lint !e429 */


int bfm_capture_and_save_do_nothing_bootfail_log(bfm_process_bootfail_param_t *param)
{
    int ret = -1;
    unsigned int modid;
    bfm_bfi_member_info_t *pbfi_memeber = NULL;

    if (unlikely(NULL == param))
    {
        BFMR_PRINT_INVALID_PARAMS("param.\n");
        return -1;
    }

    /* save log to dfx part */
    systemerror_save_log2dfx(param->reboot_type);
    modid = bfm_get_hisi_modid_according_to_bootfail_errno(param->bootfail_errno);
    BFMR_PRINT_KEY_INFO("modid: 0x%x bootfail_errno: 0x%x\n", modid, param->bootfail_errno);
    if (-1 == (int)modid)
    {
        BFMR_PRINT_ERR("modid: is 0x%x\n", modid);
        return -1;
    }

    /* save log to bfi part */
    bfm_save_log_to_bfi_part(modid, param);
    (void)bfm_read_log_in_dfx_part();

    /* save log from dfx and bfi part to log part */
    ret = bfm_save_bootfail_log_to_fs_immediately(param, &s_bootfail_log_saving_param,
        (struct dfx_head_info *)s_bottom_layer_log_buf, 0);
    if (0 != ret)
    {
        bfm_release_buffer_saving_dfx_log();
        BFMR_PRINT_ERR("Failed to save boot fail log to fs!\n");
        return -1;
    }

    /* release buffer */
    bfm_release_buffer_saving_dfx_log();

    /* read latest bfi info */
    pbfi_memeber = (bfm_bfi_member_info_t *)bfmr_malloc(BFM_BFI_MEMBER_SIZE);
    if (NULL == pbfi_memeber)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(pbfi_memeber, 0, BFM_BFI_MEMBER_SIZE);/*lint !e669 */

    ret = bfm_read_latest_bfi_info(pbfi_memeber, BFM_BFI_MEMBER_SIZE);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("bfm_read_latest_bfi_info failed!\n");
        goto __out;
    }

    /*  update the RTC time to an invalid value in case of saving log repeatedly */
    BFMR_PRINT_ERR("pbfi_memeber->rtcValue: %llx\n", pbfi_memeber->rtcValue);
    pbfi_memeber->rtcValue = (unsigned int)(-1);
    ret = bfm_update_latest_bfi_info(pbfi_memeber, BFM_BFI_MEMBER_SIZE);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("update latest bfi info\n");
        goto __out;
    }

__out:
    bfmr_free(pbfi_memeber);

    return ret;
}

static bool bfmr_userdata_is_ready(void)
{
    char data[BFM_MAX_INT_NUMBER_LEN] = {0};
    int userdata_is_ready = 0;

    (void)bfmr_full_read_with_file_path(BFM_DATAREADY_PATH, data, sizeof(char));
    userdata_is_ready = (int)simple_strtol(data, NULL, 10);
    BFMR_PRINT_KEY_INFO("userdata_is_ready: %d\n", userdata_is_ready);

    return (0 != userdata_is_ready);
}

/**
    @function: int bfm_platform_process_boot_fail(bfm_process_bootfail_param_t *param)
    @brief: process boot fail in chipsets module

    @param: param [int] params for further process boot fail in chipsets

    @return: 0 - succeeded; -1 - failed.

    @note: realize this function according to diffirent platforms
*/
int bfm_platform_process_boot_fail(bfm_process_bootfail_param_t *param)
{
    unsigned int modid;

    if (unlikely(NULL == param))
    {
        BFMR_PRINT_INVALID_PARAMS("param.\n");
        return -1;
    }

    param->bootfail_can_only_be_processed_in_platform = 1;
    modid = bfm_get_hisi_modid_according_to_bootfail_errno(param->bootfail_errno);
    BFMR_PRINT_KEY_INFO("modid: 0x%x bootfail_errno: 0x%x\n", modid, param->bootfail_errno);
    if (-1 == (int)modid)
    {
        BFMR_PRINT_ERR("modid: is 0x%x\n", modid);
        return -1;
    }

    if (!bfmr_is_part_ready_without_timeout(PART_DFX)
        || !bfmr_is_part_ready_without_timeout(PART_BOOTFAIL_INFO)
        || !bfmr_is_part_ready_without_timeout(PART_RRECORD))
    {
        panic("bootfail: dfx or bootfail_info or rrecord is not ready, so panic....");
        while(1);
    }

    param->reboot_type = BFM_S_NATIVE_BOOT_FAIL;
    memset((void *)&s_process_bootfail_param, 0, sizeof(bfm_process_bootfail_param_t));
    memcpy((void *)&s_process_bootfail_param, (void *)param, sizeof(bfm_process_bootfail_param_t));
    switch (param->bootfail_errno)
    {
    case SYSTEM_MOUNT_FAIL:
    case DATA_MOUNT_FAILED_AND_ERASED:
    case DATA_MOUNT_RO:
    case DATA_NOSPC:
    case VENDOR_MOUNT_FAIL:
    case NATIVE_PANIC:
        {
            BFMR_PRINT_KEY_INFO("Call \"rdr_syserr_process_for_ap\" to process boot fail!\n");
            rdr_syserr_process_for_ap(modid, 0, 0);
            break;
        }
    default:
        {
            bfm_reregister_exceptions_to_rdr(modid, BFM_S_NATIVE_DATA_FAIL);
            BFMR_PRINT_KEY_INFO("Call \"rdr_syserr_process_for_ap\" to process boot fail in case of dead lock!\n");
            rdr_syserr_process_for_ap(modid, 0, 0);
            break;
        }
    }

    return 0;
}

int bfm_update_platform_logs(bfm_bootfail_log_info_t *pbootfail_log_info)
{
    return 0;
}

/**
    @function: int bfm_platform_process_boot_success(void)
    @brief: process boot success in chipsets module

    @param: none

    @return: 0 - succeeded; -1 - failed.

    @note: HISI must realize this function in detail, the other platform can return 0 when enter this function
*/
int bfm_platform_process_boot_success(void)
{
    int ret = -1;
    bfm_bfi_member_info_t *pbfi_memeber = NULL;

    /* 1. read latest bfi info */
    pbfi_memeber = (bfm_bfi_member_info_t *)bfmr_malloc(BFM_BFI_MEMBER_SIZE);
    if (NULL == pbfi_memeber)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pbfi_memeber, 0, BFM_BFI_MEMBER_SIZE);

    ret = bfm_read_latest_bfi_info(pbfi_memeber, BFM_BFI_MEMBER_SIZE);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("bfm_read_latest_bfi_info failed!\n");
        goto __out;
    }

    /* 2. update the result into success */
    pbfi_memeber->rcvResult = 1;
    ret = bfm_update_latest_bfi_info(pbfi_memeber, BFM_BFI_MEMBER_SIZE);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("update latest bfi info\n");
        goto __out;
    }

__out:
    bfmr_free(pbfi_memeber);
    s_chipsets_is_bootup_successfully = true;
    return ret;
}


static unsigned int bfm_get_hisi_modid_according_to_bootfail_errno(bfmr_bootfail_errno_e bootfail_errno)
{
    unsigned int i;
    unsigned int size = sizeof(s_bfmr_errno_to_hisi_modid_map_tbl) / sizeof(s_bfmr_errno_to_hisi_modid_map_tbl[0]);

    for (i = 0; i < size; i++)
    {
        if (bootfail_errno == s_bfmr_errno_to_hisi_modid_map_tbl[i].bootfail_errno) 
        {
            return s_bfmr_errno_to_hisi_modid_map_tbl[i].hisi_modid;
        }
    }

    BFMR_PRINT_ERR("bootfail_errno: 0x%x has no hisi_modid!\n", bootfail_errno);

    return (unsigned int)-1;
}


static int bfm_read_bfi_part_header(bfm_bfi_header_info_t *pbfi_header_info)
{
    int ret = -1;
    int fd = -1;
    mm_segment_t old_fs;
    char *bfi_dev_path = NULL;
    long bytes_read = 0L;

    if (unlikely(NULL == pbfi_header_info))
    {
        BFMR_PRINT_INVALID_PARAMS("pbfi_header_info.\n");
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);/*lint !e501 */

    /* 1. get the full path of bfi part */
    bfi_dev_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN);
    if (NULL == bfi_dev_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(bfi_dev_path, 0, BFMR_DEV_FULL_PATH_MAX_LEN);

    if (0 != bfm_get_bfi_part_full_path(bfi_dev_path, BFMR_DEV_FULL_PATH_MAX_LEN))
    {
        BFMR_PRINT_ERR("get full path of bfi part failed!\n");
        goto __out;
    }

    /* 2. open bfi part  */
    fd = sys_open(bfi_dev_path, O_RDONLY, BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed![fd = %d]\n", bfi_dev_path, fd);
        goto __out;
    }

    /* 3. read header of bfi part */
    bytes_read = bfmr_full_read(fd, (char *)pbfi_header_info, sizeof(bfm_bfi_header_info_t));
    if (bytes_read != (long)sizeof(bfm_bfi_header_info_t))
    {
        BFMR_PRINT_ERR("bfmr_full_read [%s] failed!bytes_read: %ld, it should be: %ld\n",
            bfi_dev_path, bytes_read, (long)sizeof(bfm_bfi_header_info_t));
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }

    set_fs(old_fs);

    bfmr_free(bfi_dev_path);

    return ret;
}


/* NOTE: thif func only can be invoked when write BFI */
static int bfm_update_bfi_part_header(bfm_bfi_header_info_t *pbfi_header_info)
{
    int ret = -1;
    int fd = -1;
    mm_segment_t old_fs;
    char *bfi_dev_path = NULL;
    long bytes_write = 0L;

    if (unlikely(NULL == pbfi_header_info))
    {
        BFMR_PRINT_INVALID_PARAMS("pbfi_header_info.\n");
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);/*lint !e501 */

    /* 1. get the full path of bfi part */
    bfi_dev_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN);
    if (NULL == bfi_dev_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(bfi_dev_path, 0, BFMR_DEV_FULL_PATH_MAX_LEN);

    if (0 != bfm_get_bfi_part_full_path(bfi_dev_path, BFMR_DEV_FULL_PATH_MAX_LEN))
    {
        BFMR_PRINT_ERR("get full path of bfi part failed!\n");
        goto __out;
    }

    /* 2. open bfi part  */
    fd = sys_open(bfi_dev_path, O_WRONLY, BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed![fd = %d]\n", bfi_dev_path, fd);
        goto __out;
    }

    /* 3. waite header of bfi part */
    bytes_write = bfmr_full_write(fd, (char *)pbfi_header_info, sizeof(bfm_bfi_header_info_t));
    if (bytes_write != (long)sizeof(bfm_bfi_header_info_t))
    {
        ret = -1;
        BFMR_PRINT_ERR("bfmr_full_write [%s] failed!bytes_write: %ld, it should be: %ld\n",
            bfi_dev_path, bytes_write, (long)sizeof(bfm_bfi_header_info_t));
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    if (fd >= 0)
    {
        sys_fsync(fd);
        sys_close(fd);
    }

    set_fs(old_fs);

    bfmr_free(bfi_dev_path);

    return ret;
}


static int bfm_get_rtc_time_of_latest_bootail_log_from_dfx_part(u64 *prtc_time)
{
    int ret = -1;
    int fd = -1;
    int cur_number;
    u64 latest_log_offset = 0ULL;
    u64 seek_result = 0ULL;
    long bytes_read = 0L;
    char *dfx_full_path = NULL;
    struct dfx_head_info *dfx_header = NULL;
    struct every_number_info *dfx_log_info = NULL;
    mm_segment_t old_fs;

    if (unlikely(NULL == prtc_time))
    {
        BFMR_PRINT_INVALID_PARAMS("prtc_time.\n");
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    dfx_full_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN);
    if (NULL == dfx_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(dfx_full_path, 0, BFMR_DEV_FULL_PATH_MAX_LEN);

    if (0 != bfmr_get_device_full_path(PART_DFX, dfx_full_path, BFMR_DEV_FULL_PATH_MAX_LEN))
    {
        BFMR_PRINT_ERR("get full path for device [%s] failed!\n", dfx_full_path);
        goto __out;
    }

    fd = sys_open(dfx_full_path, O_RDONLY, BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed! [ret = %d]\n", dfx_full_path, fd);
        goto __out;
    }

    dfx_header = (struct dfx_head_info *)bfmr_malloc(sizeof(struct dfx_head_info));
    if (NULL == dfx_header)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(dfx_header, 0, sizeof(struct dfx_head_info));

    bytes_read = bfmr_full_read(fd, (char *)dfx_header, sizeof(struct dfx_head_info));
    if (bytes_read != (long)sizeof(struct dfx_head_info))
    {
        BFMR_PRINT_ERR("bfmr_full_read [%s] failed!bytes_read: %ld, it should be: %ld\n",
            dfx_full_path, bytes_read, (long)sizeof(struct dfx_head_info));
        goto __out;
    }

    cur_number = dfx_header->cur_number;
    cur_number = (0 == cur_number) ? (TOTAL_NUMBER  - 1) : (cur_number - 1);
    latest_log_offset = dfx_header->every_number_addr[cur_number];
    seek_result = (u64)sys_lseek(fd, (off_t)latest_log_offset, SEEK_SET);
    if (seek_result != latest_log_offset)
    {
        BFMR_PRINT_ERR("sys_lseek failed!seek_result: %llu, it should be: %llu\n", seek_result, latest_log_offset);
        goto __out;
    }

    dfx_log_info = (struct every_number_info *)bfmr_malloc(sizeof(struct every_number_info));
    if (NULL == dfx_log_info)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(dfx_log_info, 0, sizeof(struct every_number_info));

    bytes_read = bfmr_full_read(fd, (char *)dfx_log_info, sizeof(struct every_number_info));
    if (bytes_read != (long)sizeof(struct every_number_info))
    {
        BFMR_PRINT_ERR("bfmr_full_read [%s] failed!bytes_read: %ld it should be: %ld\n",
            dfx_full_path, bytes_read, (long)sizeof(struct every_number_info));
        goto __out;
    }
    else
    {
        *prtc_time = dfx_log_info->rtc_time;
        BFMR_PRINT_ERR("latest log info: idx in dfx: %d, rtc time: %llx\n", cur_number, *prtc_time);
        ret = 0;
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }

    set_fs(old_fs);

    bfmr_free(dfx_full_path);
    bfmr_free(dfx_header);
    bfmr_free(dfx_log_info);

    return ret;
}


int bfm_is_system_rooted(void)
{
    char *bootlock_value = bfm_get_bootlock_value_from_cmdline();

    if (NULL == bootlock_value)
    {
        return 0;
    }

    if (0 == strcmp(bootlock_value, "bootlock=locked"))
    {
        return 0;
    }

    return 1;
}


int bfm_is_user_sensible_bootfail(bfmr_bootfail_errno_e bootfail_errno,
    bfr_suggested_recovery_method_e suggested_recovery_method)

{
    return (DO_NOTHING == suggested_recovery_method) ? 0 : 1;
}


char* bfm_get_bl1_bootfail_log_name(void)
{
    return BFM_HISI_BL1_BOOTFAIL_LOG_NAME;
}


char* bfm_get_bl2_bootfail_log_name(void)
{
    return BFM_HISI_BL2_BOOTFAIL_LOG_NAME;
}


char* bfm_get_kernel_bootfail_log_name(void)
{
    return BFM_HISI_KERNEL_BOOTFAIL_LOG_NAME;
}


char* bfm_get_ramoops_bootfail_log_name(void)
{
    return BFM_HISI_RAMOOPS_BOOTFAIL_LOG_NAME;
}


char* bfm_get_platform_name(void)
{
    return "hisi";
}


unsigned int bfm_get_dfx_log_length(void)
{
    return (unsigned int)DFX_USED_SIZE;
}


static int bfm_add_bfi_info(bfm_bfi_member_info_t *pbfi_memeber, unsigned int bfi_memeber_len)
{
    int ret = -1;
    int fd = -1;
    long bytes_write = 0;
    long bytes_read = 0;
    long seek_result = -1;
    long offset = 0L;
    char *bfi_dev_path = NULL;
    bfm_bfi_header_info_t *pbfi_header = NULL;
    mm_segment_t old_fs;

    if (unlikely(NULL == pbfi_memeber))
    {
        BFMR_PRINT_INVALID_PARAMS("pbfi_memeber.\n");
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);/*lint !e501 */

    /* 1. find the full path of bfi part */
    bfi_dev_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN);
    if (NULL == bfi_dev_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(bfi_dev_path, 0, BFMR_DEV_FULL_PATH_MAX_LEN);

    if (0 != bfm_get_bfi_part_full_path(bfi_dev_path, BFMR_DEV_FULL_PATH_MAX_LEN))
    {
        BFMR_PRINT_ERR("get full path of bfi part failed!\n");
        goto __out;
    }

    pbfi_header = (bfm_bfi_header_info_t *)bfmr_malloc(BFM_BFI_HEADER_SIZE);
    if (NULL == pbfi_header)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    fd = sys_open(bfi_dev_path, O_RDWR, BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("sys_open: [%s] failed![ret=%d]\n", bfi_dev_path, fd);
        goto __out;
    }

    /* 2. read header of bfi */
    memset(pbfi_header, 0, BFM_BFI_HEADER_SIZE);
    bytes_read = bfmr_full_read(fd, (char *)pbfi_header, BFM_BFI_HEADER_SIZE);
    if (bytes_read != BFM_BFI_HEADER_SIZE)
    {
        BFMR_PRINT_ERR("read [%s] fail![%ld]\n", bfi_dev_path, bytes_read);
        goto __out;
    }

    /* 3. seek to current write pos */
    offset = (long)BFM_BFI_HEADER_SIZE + (long)pbfi_header->cur_number * (long)pbfi_header->every_number_size;
    seek_result = sys_lseek(fd, offset, SEEK_SET);
    if (seek_result != offset)
    {
        BFMR_PRINT_ERR("lseek [%s] fail[%d]\n", bfi_dev_path, ret);
        goto __out;
    }

    /* 4. write bfi */
    bytes_write = bfmr_full_write(fd, (char*)pbfi_memeber, bfi_memeber_len);
    if (bytes_write != (long)bfi_memeber_len)
    {
        BFMR_PRINT_ERR("write bfi part failed!bytes_write: %ld, it should be: %ld\n",
            bytes_write, (long)bfi_memeber_len);
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    if (fd >= 0)
    {
        sys_fsync(fd);
        sys_close(fd);
    }
    bfmr_free(bfi_dev_path);
    bfmr_free(pbfi_header);
    set_fs(old_fs);

    return ret;
}


static const char *bfm_get_hardware_fault_type(bfmr_hardware_fault_type_e fault_type)
{
    switch (fault_type)
    {
    case HW_FAULT_OCP:
    default:
        {
            return "ocp";
        }
    }

    return "";
}


static void bfm_format_hardware_excp_detail_info(bfmr_hardware_fault_type_e fault_type,
    bfmr_detail_boot_stage_e boot_stage,
    const char *poriginal_detail,
    char *pdetail_info_out,
    size_t pdetail_info_out_buf_len)
{
    if (unlikely((NULL == poriginal_detail) || (NULL == pdetail_info_out)))
    {
        BFMR_PRINT_INVALID_PARAMS("poriginal_detail or pdetail_info_out.\n");
        return;
    }

    snprintf(pdetail_info_out, pdetail_info_out_buf_len - 1, "%s_%s_%s_%s",
        bfm_get_platform_name(), bfm_get_boot_stage_name(boot_stage),
        bfm_get_hardware_fault_type(fault_type), poriginal_detail);
}


static void bfm_save_log_to_bfi_part(u32 hisi_modid, bfm_process_bootfail_param_t *param)
{
    u64 rtc_time_of_latest_log;
    bfm_bfi_header_info_t *pbfi_header_info = NULL;
    bfm_bfi_member_info_t *pbfi_member_info = NULL;

    BFMR_PRINT_ENTER();
    if (unlikely(NULL == param))
    {
        BFMR_PRINT_INVALID_PARAMS("param.\n");
        return;
    }

    /* 1. read rtc time of latest bootail log */
    if (0 != bfm_get_rtc_time_of_latest_bootail_log_from_dfx_part(&rtc_time_of_latest_log))
    {
        BFMR_PRINT_ERR("read rtc time of latest bfmr log failed!\n");
        goto __out;
    }

    /* 2. read header of bfi part, NOTE: the header of BFI part is initialised in BOOTLOADER */
    pbfi_header_info = (bfm_bfi_header_info_t *)bfmr_malloc(sizeof(bfm_bfi_header_info_t));
    if (NULL == pbfi_header_info)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(pbfi_header_info, 0, sizeof(bfm_bfi_header_info_t));

    if (0 != bfm_read_bfi_part_header(pbfi_header_info))
    {
        BFMR_PRINT_ERR("read header of bfi part failed!\n");
        goto __out;
    }

    /* 3. format and write the latest bfi member info */
    pbfi_member_info = (bfm_bfi_member_info_t *)bfmr_malloc(sizeof(bfm_bfi_member_info_t));
    if (NULL == pbfi_member_info)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(pbfi_member_info, 0, sizeof(bfm_bfi_member_info_t));

    pbfi_member_info->rtcValue = rtc_time_of_latest_log;
    pbfi_member_info->bfmErrNo = (unsigned int)param->bootfail_errno;
    pbfi_member_info->bfmStageCode = (unsigned int)param->boot_stage;
    pbfi_member_info->isSystemRooted = bfm_is_system_rooted();
    pbfi_member_info->bootup_time = bfmr_get_bootup_time();
    pbfi_member_info->isUserPerceptiable = bfm_is_user_sensible_bootfail(
        pbfi_member_info->bfmErrNo, param->suggested_recovery_method);
    pbfi_member_info->rcvMethod = try_to_recovery(pbfi_member_info->rtcValue,
        pbfi_member_info->bfmErrNo, pbfi_member_info->bfmStageCode,
        param->suggested_recovery_method, param->addl_info.detail_info);
    param->recovery_method = pbfi_member_info->rcvResult;
    pbfi_member_info->rcvResult = 0;
    pbfi_member_info->suggested_recovery_method = param->suggested_recovery_method;
    pbfi_member_info->is_bootup_successfully = s_chipsets_is_bootup_successfully ? (~0) : (0U);
    pbfi_member_info->reboot_type = hisi_modid;
    if (BFM_HARDWARE_FAULT == param->bootfail_errno)
    {
        bfm_format_hardware_excp_detail_info(param->addl_info.hardware_fault_type, param->boot_stage,
            param->addl_info.detail_info, pbfi_member_info->excepInfo, sizeof(pbfi_member_info->excepInfo));
    }

    if (pbfi_member_info->bfmErrNo > KERNEL_ERRNO_START)
    {
        unsigned int  count = 0;
        char errName[BFMR_SIZE_128] = {0};

        if (0 == get_dmd_err_num(&pbfi_member_info->dmdErrNo, &count, errName))
        {
            BFMR_PRINT_ERR("DMD err info, errNo=%d; count=%d; errName=%s.\n",
                pbfi_member_info->dmdErrNo, count, errName);
        }
        else
        {
            pbfi_member_info->dmdErrNo = 0;
            BFMR_PRINT_ERR("DMD err info not found.\n");
        }
    }
    else
    {
        pbfi_member_info->dmdErrNo = 0;
    }

    BFMR_PRINT_KEY_INFO("boot fail info:\n"
        "rtcValue: 0x%llx\n"
        "bfmErrNo: 0x%08x\n"
        "bfmStageCode: 0x%08x\n"
        "bootup_time: %dS\n"
        "isSystemRooted: %u\n"
        "isUserPerceptiable: %u\n"
        "rcvMethod: %u\n"
        "rcvResult: %u\n",
        pbfi_member_info->rtcValue,
        pbfi_member_info->bfmErrNo,
        pbfi_member_info->bfmStageCode,
        pbfi_member_info->bootup_time,
        pbfi_member_info->isSystemRooted,
        pbfi_member_info->isUserPerceptiable,
        pbfi_member_info->rcvMethod,
        pbfi_member_info->rcvResult);
    if (0 != bfm_add_bfi_info(pbfi_member_info, sizeof(bfm_bfi_member_info_t)))
    {
        BFMR_PRINT_ERR("update latest bfi info failed!\n");
        goto __out;
    }

    /* 4. update the next write pos of header info in bfi part */
    pbfi_header_info->cur_number = (pbfi_header_info->cur_number + 1) % (BFM_BFI_RECORD_TOTAL_COUNT);
    if (0 != bfm_update_bfi_part_header(pbfi_header_info))
    {
        BFMR_PRINT_ERR("update bfi header failed!\n");
        goto __out;
    }

__out:
    bfmr_free(pbfi_header_info);
    bfmr_free(pbfi_member_info);

    BFMR_PRINT_EXIT();
}


static void bfm_dump_callback(u32 modid,
    u32 etype,
    u64 coreid,
    char *log_path,
    pfn_cb_dump_done pfn_cb)
{
    BFMR_PRINT_ENTER();
    if (BFM_S_NATIVE_DATA_FAIL == etype)
    {
        preempt_enable();
    }

    systemerror_save_log2dfx(etype);
    bfm_save_log_to_bfi_part(modid, (bfm_process_bootfail_param_t *)&s_process_bootfail_param);
    if (0 != bfmr_wait_for_part_mount_without_timeout(bfm_get_bfmr_log_part_mount_point()))
    {
        BFMR_PRINT_ERR("log part [%s] is not ready\n", BFM_HISI_LOG_ROOT_PATH);
    }
    else
    {
        (void)bfm_read_log_in_dfx_part();
        bfm_save_bootfail_log_to_fs_immediately((bfm_process_bootfail_param_t *)&s_process_bootfail_param,
            &s_bootfail_log_saving_param, (struct dfx_head_info *)s_bottom_layer_log_buf, 0);
    }

    if (NULL != pfn_cb)
    {
        pfn_cb(modid, coreid);
    }

    if (BFM_S_NATIVE_DATA_FAIL == etype)
    {
        preempt_disable();
    }
    BFMR_PRINT_EXIT();
}


static void bfm_reset_callback(u32 modid, u32 etype, u64 coreid)
{
    BFMR_PRINT_ENTER();
    BFMR_PRINT_EXIT();
}


static int bfm_register_callbacks_to_rdr(void)
{
    struct rdr_module_ops_pub s_soc_ops;
    struct rdr_register_module_result retinfo;
    int ret = 0;
    u64 coreid = RDR_BFM;

    BFMR_PRINT_ENTER();
    s_soc_ops.ops_dump = bfm_dump_callback;
    s_soc_ops.ops_reset = bfm_reset_callback;
    ret = rdr_register_module_ops(coreid, &s_soc_ops, &retinfo);
    if (0 != ret)
    {
        BFMR_PRINT_ERR( "rdr_register_module_ops failed!ret = [%d]\n", ret);
    }
    BFMR_PRINT_EXIT();

    return ret;
}


static void bfm_reregister_exceptions_to_rdr(unsigned int modid, unsigned int exec_type)
{
    int i = 0;
    int count = sizeof(s_rdr_excetion_info_for_bfmr) / sizeof(s_rdr_excetion_info_for_bfmr[0]);
    int ret = 0;

    BFMR_PRINT_KEY_INFO("unregister exception for [0x%08x] to rdr!\n", modid);
    ret = rdr_unregister_exception(modid);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("unregister exception for [0x%08x] failed!\n", modid);
        return;
    }

    for (i = 0; i < count; i++)
    {
        if ((modid >= s_rdr_excetion_info_for_bfmr[i].e_modid)
            && (modid <= s_rdr_excetion_info_for_bfmr[i].e_modid_end))
        {
            s_rdr_excetion_info_for_bfmr[i].e_exce_type = exec_type;
            ret = rdr_register_exception(&s_rdr_excetion_info_for_bfmr[i]);
            break;
        }
    }

    BFMR_PRINT_KEY_INFO("reregister exec type [%x] for mode: 0x%08x %s!\n", exec_type, modid,
        (0 == ret) ? ("failed") : ("successfully"));
}


static void bfm_register_exceptions_to_rdr(void)
{
    int i;
    int count = sizeof(s_rdr_excetion_info_for_bfmr) / sizeof(s_rdr_excetion_info_for_bfmr[0]);
    u32 ret;

    BFMR_PRINT_ENTER();
    BFMR_PRINT_KEY_INFO("register exception for BFMR to rdr as follows: \n");
    for (i = 0; i < count; i++)
    {
        BFMR_PRINT_SIMPLE_INFO("%d ", s_rdr_excetion_info_for_bfmr[i].e_exce_type);
        ret = rdr_register_exception(&s_rdr_excetion_info_for_bfmr[i]);
        if (0 == ret)
        {
            BFMR_PRINT_KEY_INFO("rdr_register_exception failed!, ret = [%d]\n", ret);
        }
    }
    BFMR_PRINT_SIMPLE_INFO("\n");
    BFMR_PRINT_EXIT();
}


static void bfm_release_buffer_saving_dfx_log(void)
{
    if (NULL != s_bottom_layer_log_buf)
    {
        bfmr_free(s_bottom_layer_log_buf);
        s_bottom_layer_log_buf = NULL;
    }
}


static int bfm_read_log_in_dfx_part(void)
{
    int i = 0;
    int fd = -1;
    int ret = -1;
    mm_segment_t old_fs = 0;
    long bytes_read = 0L;
    char *dfx_full_path = NULL;
    bool find_dfx_part = false;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    if (NULL != s_bottom_layer_log_buf)
    {
        bfmr_free(s_bottom_layer_log_buf);
        s_bottom_layer_log_buf = NULL;
    }

    s_bottom_layer_log_buf = (char *)bfmr_malloc((unsigned long)(DFX_USED_SIZE + 1));
    if (NULL == s_bottom_layer_log_buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)s_bottom_layer_log_buf, 0, (unsigned long)(DFX_USED_SIZE + 1));

    dfx_full_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN + 1);
    if (NULL == dfx_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;  
    }
    memset((void *)dfx_full_path, 0, BFMR_DEV_FULL_PATH_MAX_LEN + 1);

    while (i++ < (int)BFM_HISI_WAIT_FOR_LOG_PART_TIMEOUT)
    {
        (void)bfmr_get_device_full_path(PART_DFX, dfx_full_path, BFMR_DEV_FULL_PATH_MAX_LEN);
        if (bfmr_is_file_existed(dfx_full_path))
        {
            find_dfx_part = true;
            break;
        }
        msleep(1000);
    }

    if (!find_dfx_part)
    {
        BFMR_PRINT_ERR("Can not find the dfx part!\n");
        goto __out;
    }

    fd = sys_open(dfx_full_path, O_RDONLY, BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed! [ret = %d]\n", dfx_full_path, fd);
        goto __out;
    }

    bytes_read = bfmr_full_read(fd, s_bottom_layer_log_buf, DFX_USED_SIZE);
    if (bytes_read != (long)DFX_USED_SIZE)
    {
        BFMR_PRINT_ERR("bfmr_full_read [%s] failed! bytes_read: %ld, it must be: %ld\n",
            dfx_full_path, bytes_read, (long)DFX_USED_SIZE);
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }

    set_fs(old_fs);
    bfmr_free(dfx_full_path);

    return ret;
}


/**
    @function: int bfmr_copy_data_from_dfx_to_bfmr_tmp_buffer(void)
    @brief: copy dfx data to local buffer

    @param: none

    @return: 0 - succeeded; -1 - failed.

    @note: HISI must realize this function in detail, the other platform can return when enter this function
*/
void bfmr_copy_data_from_dfx_to_bfmr_tmp_buffer(void)
{
    if (0 != bfm_read_log_in_dfx_part())
    {
        BFMR_PRINT_ERR("Failed to read data from dfx part!\n");
    }
}


void save_hwbootfailInfo_to_file(void)
{
    BFMR_PRINT_ENTER();
    down(&s_process_bottom_layer_boot_fail_sem);
    bfmr_copy_data_from_dfx_to_bfmr_tmp_buffer();
    up(&s_process_bottom_layer_boot_fail_sem);
    BFMR_PRINT_EXIT();
}


bool bfmr_is_enabled(void)
{
    return bfmr_has_been_enabled();
}


/***********************************************************************
 *
 *if want save log to raw partion, you must implement below funcion.
 * currently, in bfm_core we can only support one type boot_fail log.
 *
 **********************************************************************/
char* bfm_get_raw_part_name(void)
{
    return "unsupport";
}


int bfm_get_raw_part_offset(void)
{
    return 0;
}


void bfmr_alloc_and_init_raw_log_info(bfm_process_bootfail_param_t *pparam, bfmr_log_dst_t *pdst)
{
    return;
}


void bfmr_save_and_free_raw_log_info(bfm_process_bootfail_param_t *pparam)
{
    return;
}


void bfmr_update_raw_log_info(bfmr_log_src_t *psrc, bfmr_log_dst_t *pdst, unsigned int bytes_read)
{
    return;
}


static struct notifier_block ocp_event_nb = {
    .notifier_call = bfm_process_ocp_callback,
    .priority	= INT_MAX,
};


static int bfm_process_ocp_boot_fail_func(void *param)
{
    BFMR_PRINT_KEY_INFO("param.\n");
    while (1)
    {
        int i = 0;

        if (0 != down_interruptible(&s_process_ocp_sem))
        {
            continue;
		}

        for (i = 0; i < (sizeof(s_process_ocp_excp_param.ocp_excp_info) / sizeof(s_process_ocp_excp_param.ocp_excp_info[0])); i++)
        {
            if (down_trylock(&(s_process_ocp_excp_param.ocp_excp_info[i].sem)))
            {
                continue;
            }

            if ((NULL != s_process_ocp_excp_param.process_ocp_excp)
                && (0 != strcmp(s_process_ocp_excp_param.ocp_excp_info[i].excp_info.ldo_num, "")))
            {
                s_process_ocp_excp_param.process_ocp_excp(s_process_ocp_excp_param.ocp_excp_info[i].fault_type,
                    &s_process_ocp_excp_param.ocp_excp_info[i].excp_info);
                memset(&s_process_ocp_excp_param.ocp_excp_info[i].excp_info, 0, sizeof(ocp_excp_info_t));
            }
            up(&(s_process_ocp_excp_param.ocp_excp_info[i].sem));
        }
    }

    return 0;
}



static int bfm_process_ocp_boot_fail(void)
{
    int ret = -1;
    struct task_struct *tsk;
    bfm_ocp_boot_fail_test_param_t *ptest_param = NULL;

    ret = hisi_pmic_mntn_register_notifier(&ocp_event_nb);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("ocp_event_nb register fail!\n");
        return -1;
    }

    tsk = kthread_run(bfm_process_ocp_boot_fail_func, NULL, "process_ocp");
    if (IS_ERR(tsk))
    {
        BFMR_PRINT_ERR("Failed to create thread to process OCP exception!\n");
        return -1;
    }

    return 0;
}

static bfmr_hardware_fault_type_e bfm_get_bfmr_fault_type(unsigned long hisi_fault_type)
{
    size_t i = 0;
    size_t count = sizeof(s_hw_fault_map_table) / sizeof(s_hw_fault_map_table[0]);

    for (i = 0; i < count; i++)
    {
        if (hisi_fault_type == s_hw_fault_map_table[i].hisi_fault_type)
        {
            return s_hw_fault_map_table[i].bfmr_fault_type;
        }
    }

    return HW_FAULT_OCP;
}

static int bfm_process_ocp_callback(struct notifier_block *self, unsigned long val, void *data)
{
    int i = 0;

    for (i = 0; i < (sizeof(s_process_ocp_excp_param.ocp_excp_info) / sizeof(s_process_ocp_excp_param.ocp_excp_info[0])); i++)
    {
        if (down_trylock(&(s_process_ocp_excp_param.ocp_excp_info[i].sem)))
        {
            BFMR_PRINT_ERR("There's an OCP is being processed!\n");
            continue;
        }

        if (0 == strcmp(s_process_ocp_excp_param.ocp_excp_info[i].excp_info.ldo_num, ""))
        {
            memset((void *)&s_process_ocp_excp_param.ocp_excp_info[i].excp_info, 0x0, sizeof(ocp_excp_info_t));
            memcpy((void *)&s_process_ocp_excp_param.ocp_excp_info[i].excp_info, data, sizeof(ocp_excp_info_t));
            s_process_ocp_excp_param.ocp_excp_info[i].fault_type = bfm_get_bfmr_fault_type(val);
            up(&(s_process_ocp_excp_param.ocp_excp_info[i].sem));
            break;
        }
        up(&(s_process_ocp_excp_param.ocp_excp_info[i].sem));
    }

    up(&s_process_ocp_sem);

    return NOTIFY_DONE;
}


static int __init bfm_early_parse_recovery_mode_cmdline(char *p)
{
    if (NULL != p)
    {
        s_is_recovery_mode = (0 == strncmp(p, "1", strlen("1"))) ? true : false;
    }

    return 0;
}
early_param("enter_recovery", bfm_early_parse_recovery_mode_cmdline);

static int __init bfm_early_parse_power_off_charge_cmdline(char *p)
{
    if (NULL != p)
    {
        s_power_off_charge = (0 == strncmp(p, "charger", strlen("charger"))) ? true : false;
    }

    return 0;
}
early_param("androidboot.mode", bfm_early_parse_power_off_charge_cmdline);


void bfm_set_valid_long_press_flag(void)
{
    bfmr_rrecord_misc_msg_param_t misc_msg;
    bfmr_detail_boot_stage_e bfmr_bootstage = STAGE_BOOT_SUCCESS;

    bfm_get_boot_stage(&bfmr_bootstage);
    if (!s_is_recovery_mode && !s_power_off_charge
        && (bfmr_get_bootup_time() > BFM_BOOT_SUCCESS_TIME_IN_KENREL)
        && (bfmr_bootstage < STAGE_BOOT_SUCCESS))
    {
        memset((void *)&misc_msg, 0, sizeof(misc_msg));
        memcpy(misc_msg.command, BFR_VALID_LONG_PRESS_LOG,
            BFMR_MIN(sizeof(misc_msg.command) - 1, strlen(BFR_VALID_LONG_PRESS_LOG)));
        (void)bfmr_write_rrecord_misc_msg(&misc_msg);
    }
    else
    {
        BFMR_PRINT_KEY_INFO("There's no need to save long press log\n");
    }
}


int bfm_chipsets_init(bfm_chipsets_init_param_t *param)
{
    int ret = 0;
    int i = 0;

    if (unlikely((NULL == param)))
    {
        BFMR_PRINT_KEY_INFO("param or param->log_saving_param is NULL\n");
        return -1;
    }

    BFMR_PRINT_ENTER();
    ret = bfm_register_callbacks_to_rdr();
    if (0 != ret)
    {
        BFMR_PRINT_ERR("bfm failed to register callbacks to rdr, ret = [%d], it maybe an error!\n", ret);
    }

    bfm_register_exceptions_to_rdr();
    memset((void *)&s_bootfail_log_saving_param, 0, sizeof(bfm_bootfail_log_saving_param_t));
    memcpy((void *)&s_bootfail_log_saving_param, (void *)&param->log_saving_param,
        sizeof(bfm_bootfail_log_saving_param_t));
    memset((void *)&s_process_ocp_excp_param, 0, sizeof(bfm_process_ocp_excp_param_t));
    memcpy((void *)&s_process_ocp_excp_param.process_ocp_excp, (void *)&param->process_ocp_excp,
        sizeof(param->process_ocp_excp));
    for (i = 0; i < sizeof(s_process_ocp_excp_param.ocp_excp_info) / sizeof(s_process_ocp_excp_param.ocp_excp_info[0]); i++)
    {
        sema_init(&s_process_ocp_excp_param.ocp_excp_info[i].sem, 1);
    }
	sema_init(&s_process_ocp_sem, 0);
    (void)bfm_process_ocp_boot_fail();
    BFMR_PRINT_EXIT();

    return 0;
}

