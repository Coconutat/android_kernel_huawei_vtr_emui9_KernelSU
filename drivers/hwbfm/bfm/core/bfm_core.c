/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_core.c

    @brief: define the core's external public enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/dirent.h>
#include <linux/statfs.h>
#include <chipset_common/bfmr/bfm/core/bfm_core.h>
#include <chipset_common/bfmr/bfm/chipsets/bfm_chipsets.h>
#include <chipset_common/bfmr/bfm/core/bfm_timer.h>
#include <chipset_common/bfmr/bfm/core/bfm_stage_info.h>
#include <chipset_common/bfmr/bfr/core/bfr_core.h>
#include <chipset_common/bfmr/abns/abns_shutdown.h>

#include <linux/rtc.h>
#include <linux/cpu.h>



/*----local macroes------------------------------------------------------------------*/

#define bfmr_llseek no_llseek
#define BFM_BFI_FILE_NAME "bootFail_info.txt"
#define BFM_RECOVERY_FILE_NAME "recovery_info.txt"
#define BFM_RECOVERY_METHOD_FIELD "rcvMethod:"
#define BFM_RECOVERY_SUCCESS_FIELD "rcvResult:1\r\n"
#define BFM_RECOVERY_RESULT_FIELD "rcvResult:"
#define BFM_LOG_END_TAG_PER_LINE "\r\n"
#define BFM_BYTES_REDUNDANT_ON_LOG_PART (4 * 1024)
#define BFM_DONE_FILE_NAME "DONE"
#define BFM_TIME_FIELD_NAME "time:"
#define BFM_BFI_FILE_CONTENT_FORMAT \
    "time%s:%s\r\n" \
    "bootFailErrno%s:%s\r\n" \
    "boot_stage%s:%s_%s\r\n" \
    "isSystemRooted%s:%d\r\n" \
    "isUserPerceptible%s:%d\r\n" \
    "SpaceLeftOnData%s:%lldMB [%lld%%]\r\n" \
    "iNodesUsedOnData%s:%lld [%lld%%]\r\n" \
    "\r\n" \
    "time%s:0x%llx\r\n" \
    "bootFailErrno%s:0x%x\r\n" \
    "boot_stage%s:0x%x\r\n" \
    "isSystemRooted%s:%d\r\n" \
    "isUserPerceptible%s:%d\r\n"\
    "dmdErrNo%s:%d\r\n"\
    "bootFailDetail%s:%s\r\n"\
    "bootup_time:%dS\r\n" \
    "isBootUpSuccessfully%s:%s\r\n" \
    "RebootType%s:0x%x\r\n" \
    "\r\n"\
    "the bootlock field in cmdline is: [%s] this time\r\n"

#define BFM_RCV_FILE_CONTENT_FORMAT \
    "rcvMethod%s:%s\r\n" \
    "rcvResult%s:%s\r\n" \
    "\r\n" \
    "rcvMethod%s:%d\r\n" \
    "rcvResult%s:%d\r\n"

#define BFM_RECOVERY_SUCCESS_STR "success"
#define BFM_RECOVERY_SUCCESS_INT_VALUE 1
#define BFM_RECOVERY_FAIL_STR "fail"
#define BFM_RECOVERY_FAIL_INT_VALUE 0
#define BFM_PBL_LOG_MAX_LEN (BFMR_SIZE_1K)
#define BFM_BOOLTOADER1_LOG_MAX_LEN (BFMR_SIZE_1K)
#define BFM_BOOLTOADER2_LOG_MAX_LEN (128 * BFMR_SIZE_1K)
#define BFM_KMSG_LOG_MAX_LEN (BFMR_SIZE_1K * BFMR_SIZE_1K)
#define BFM_KMSG_TEXT_LOG_MAX_LEN (512 * BFMR_SIZE_1K)
#define BFM_LOG_RAMOOPS_LOG_MAX_LEN (128 * BFMR_SIZE_1K)
#define BFM_APP_BETA_LOGCAT_LOG_MAX_LEN (1024 * BFMR_SIZE_1K)
#define BFM_CRITICAL_PROCESS_CRASH_LOG_MAX_LEN (128 * BFMR_SIZE_1K)
#define BFM_VM_TOMBSTONES_LOG_MAX_LEN (256 * BFMR_SIZE_1K)
#define BFM_VM_CRASH_LOG_MAX_LEN (128 * BFMR_SIZE_1K)
#define BFM_VM_WATCHDOG_LOG_MAX_LEN (256 * BFMR_SIZE_1K)
#define BFM_NORMAL_FRAMEWORK_BOOTFAIL_LOG_MAX_LEN (256 * BFMR_SIZE_1K)
#define BFM_FIXED_FRAMEWORK_BOOTFAIL_LOG_MAX_LEN (256 * BFMR_SIZE_1K)
#define BFM_BOOTFAIL_INFO_LOG_MAX_LEN (BFMR_SIZE_1K)
#define BFM_RECOVERY_INFO_LOG_MAX_LEN (BFMR_SIZE_1K)
#define BFM_BASIC_LOG_MAX_LEN (BFM_BOOTFAIL_INFO_LOG_MAX_LEN + BFM_RECOVERY_INFO_LOG_MAX_LEN)
#define BFM_BFMR_TEMP_BUF_LOG_MAX_LEN (BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN\
    + BFM_KMSG_LOG_MAX_LEN + BFM_LOG_RAMOOPS_LOG_MAX_LEN)

#define BFM_TOMBSTONE_FILE_NAME_TAG "tombstone"
#define BFM_SYSTEM_SERVER_CRASH_FILE_NAME_TAG "system_server_crash"
#define BFM_SYSTEM_SERVER_WATCHDOG_FILE_NAME_TAG "system_server_watchdog"
#define BFM_SAVE_LOG_INTERVAL_FOR_EACH_LOG (1000)
#define BFM_US_PERSECOND (1000000)
#define BFM_SAVE_LOG_MAX_TIME (6000)

#define BFMR_RECOVERT_MODE_KEYWORD_1 "rebooting into recovery mode"
#define BFMR_RECOVERT_MODE_KEYWORD_2 "exited 4 times in 4 minutes"
#define BFM_WAIT_DATA_PART_TIME_OUT (5)
#define BFM_TEXT_LOG_SEPRATOR_WITHOUT_FIRST_NEWLINE "================time:%s================\r\n"
#define BFM_TEXT_LOG_SEPRATOR_WITH_FIRST_NEWLINE "\n" BFM_TEXT_LOG_SEPRATOR_WITHOUT_FIRST_NEWLINE
#define BFM_USER_NOT_SENSIBLE_BOOTFAIL_MAX_COUNT (2)
#define BFM_USER_MAX_TOLERANT_BOOTTIME_IN_SECOND (60)
#define BFM_BOOTUP_SLOWLY_THRESHOLD_IN_SECOND (5 * 60)

/*----local prototypes----------------------------------------------------------------*/

typedef struct
{
    bfmr_log_type_e log_type;
    unsigned int buf_len;
    char *buf;
} bfm_log_type_buffer_info_t;

typedef struct
{
    bfmr_bootfail_errno_e bootfail_errno;
    char *desc;
} bfm_boot_fail_no_desc_t;

typedef struct
{
    bfmr_boot_stage_e boot_stage;
    long long log_size_allowed;
} bfm_log_size_param_t;

/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes--------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/

static int __init early_parse_bfmr_enable_flag(char *p);
static long bfmr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int bfmr_open(struct inode *inode, struct file *file);
static ssize_t bfmr_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
static ssize_t bfmr_write(struct file *file, const char *data, size_t len, loff_t *ppos);
static int bfmr_release(struct inode *inode, struct file *file);
static int bfm_update_info_for_each_log(void);
static int bfm_notify_boot_success(void *param);
static int bfm_lookup_dir_by_create_time(const char *root,
    char *log_path,
    size_t log_path_len,
    int find_oldest_log);
static int bfm_find_newest_log(char *log_path, size_t log_path_len);
static void bfm_wait_for_compeletion_of_processing_boot_fail(void);

static void bfm_delete_oldest_log(long long bytes_need_this_time);
static long long bfm_get_basic_space_for_each_bootfail_log(bfmr_bootfail_errno_e bootfail_errno);
static long long bfm_get_extra_space_for_each_bootfail_log(bfm_process_bootfail_param_t *pparam);
static int bfm_save_extra_bootfail_logs(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_bl1_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_bl2_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_kernel_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_ramoops_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_native_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_framework_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static bool bfm_is_log_existed(unsigned long long rtc_time, unsigned int bootfail_errno);
static int bfm_capture_and_save_bootfail_log(bfm_process_bootfail_param_t *pparam);
static void bfm_process_after_save_bootfail_log(void);
static char* bfm_get_boot_fail_no_desc(bfmr_bootfail_errno_e bootfail_errno, bfm_process_bootfail_param_t *pparam);
static int bfm_save_bootfail_info_txt(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_save_recovery_info_txt(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static char *bfm_get_file_name(char *file_full_path);
static int bfm_process_upper_layer_boot_fail(void *param);
static int bfm_process_bottom_layer_boot_fail(void *param);
static int bfmr_capture_and_save_bottom_layer_boot_fail_log(void);
/**
    @function: static int bfmr_capture_and_save_log(bfmr_log_type_e type, bfmr_log_dst_t *dst)
    @brief: capture and save log, initialised in core.

    @param: src [in] log src info.
    @param: dst [in] infomation about the media of saving log.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in bootloader and kernel.
*/
static int bfmr_capture_and_save_log(bfmr_log_src_t *src, bfmr_log_dst_t *dst, bfm_process_bootfail_param_t *pparam);
static unsigned long long bfm_get_system_time(void);
static int bfm_get_fs_state(const char *pmount_point, struct statfs *pstatfsbuf);
static bool bfm_is_user_sensible_boot_fail(bfm_bootfail_log_info_t *pbootfail_log_info);
static void bfm_merge_bootfail_logs(bfm_bootfail_log_info_t *pbootfail_log_info);
static void bfm_delete_user_unsensible_bootfail_logs(bfm_bootfail_log_info_t *pbootfail_log_info);
static int bfm_traverse_log_root_dir(bfm_bootfail_log_info_t *pbootfail_log_info);
static void bfm_user_space_process_read_its_own_file(bfm_process_bootfail_param_t *pparam);

/*----local variables-----------------------------------------------------------------*/

static const struct file_operations s_bfmr_fops = {
    .owner	 = THIS_MODULE,
    .unlocked_ioctl = bfmr_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = bfmr_ioctl,
#endif
    .open = bfmr_open,
    .read = bfmr_read,
    .write = bfmr_write,
    .release = bfmr_release,
    .llseek = bfmr_llseek,
};

static struct miscdevice s_bfmr_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = BFMR_DEV_NAME,
    .fops = &s_bfmr_fops,
};


static DEFINE_MUTEX(s_process_boot_stage_mutex);
static DEFINE_MUTEX(s_process_boot_fail_mutex);
static DECLARE_COMPLETION(s_process_boot_fail_comp);

/* modifiy this array if you add new log type */
static bfm_log_type_buffer_info_t s_log_type_buffer_info[LOG_TYPE_MAX_COUNT] = {
    {LOG_TYPE_BOOTLOADER_1, BFM_BOOLTOADER1_LOG_MAX_LEN, NULL},
    {LOG_TYPE_BOOTLOADER_2, BFM_BOOLTOADER2_LOG_MAX_LEN, NULL},
    {LOG_TYPE_BFMR_TEMP_BUF, BFM_BFMR_TEMP_BUF_LOG_MAX_LEN, NULL},
    {LOG_TYPE_ANDROID_KMSG, BFM_KMSG_LOG_MAX_LEN, NULL},
    {LOG_TYPE_RAMOOPS, BFM_LOG_RAMOOPS_LOG_MAX_LEN, NULL},
    {LOG_TYPE_BETA_APP_LOGCAT, BFM_APP_BETA_LOGCAT_LOG_MAX_LEN, NULL},
    {LOG_TYPE_CRITICAL_PROCESS_CRASH, BFM_CRITICAL_PROCESS_CRASH_LOG_MAX_LEN, NULL},
    {LOG_TYPE_VM_TOMBSTONES, BFM_VM_TOMBSTONES_LOG_MAX_LEN, NULL},
    {LOG_TYPE_VM_CRASH, BFM_VM_CRASH_LOG_MAX_LEN, NULL},
    {LOG_TYPE_VM_WATCHDOG, BFM_VM_WATCHDOG_LOG_MAX_LEN, NULL},
    {LOG_TYPE_NORMAL_FRAMEWORK_BOOTFAIL_LOG, BFM_NORMAL_FRAMEWORK_BOOTFAIL_LOG_MAX_LEN, NULL},
    {LOG_TYPE_FIXED_FRAMEWORK_BOOTFAIL_LOG, BFM_FIXED_FRAMEWORK_BOOTFAIL_LOG_MAX_LEN, NULL},
    {LOG_TYPE_TEXT_KMSG, BFM_KMSG_TEXT_LOG_MAX_LEN, NULL},
};

/* modifiy this array according to the log count captured for each bootfail */
static bfm_log_size_param_t s_log_size_param[] = {
    {(PBL_STAGE << 24), BFM_PBL_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(BL1_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(BL2_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(KERNEL_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN + BFM_KMSG_LOG_MAX_LEN
        + BFM_LOG_RAMOOPS_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(NATIVE_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN + BFM_KMSG_LOG_MAX_LEN
        + BFM_LOG_RAMOOPS_LOG_MAX_LEN + BFM_VM_TOMBSTONES_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(ANDROID_FRAMEWORK_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN
        + BFM_KMSG_LOG_MAX_LEN + BFM_LOG_RAMOOPS_LOG_MAX_LEN + BFM_VM_WATCHDOG_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
};

/* modifiy this array if you add new boot fail no */
static bfm_boot_fail_no_desc_t s_bootfail_errno_desc[] =
{
    {BL1_DDR_INIT_FAIL, "bl1 ddr init failed"},
    {BL1_EMMC_INIT_FAIL, "bl1 emmc init failed"},
    {BL1_BL2_LOAD_FAIL, "load image image failed"},
    {BL1_BL2_VERIFY_FAIL, "verify image failed"},
    {BL2_EMMC_INIT_FAIL, "bl2 emmc init failed"},
    {BL1_WDT, "bl1 wdt"},
    {BL1_VRL_LOAD_FAIL, "bl1 vrl load error"},
    {BL1_VRL_VERIFY_FAIL, "bl1 vrl verify image error"},
    {BL1_ERROR_GROUP_BOOT, "bl1 group boot error"},
    {BL1_ERROR_GROUP_BUSES, "bl1 group buses error"},
    {BL1_ERROR_GROUP_BAM, "bl1 group bam"},
    {BL1_ERROR_GROUP_BUSYWAIT, "bl1 group busy wait"},
    {BL1_ERROR_GROUP_CLK, "bl1 group clock"},
    {BL1_ERROR_GROUP_CRYPTO, "bl1 group crypto"},
    {BL1_ERROR_GROUP_DAL, "bl1 group dal"},
    {BL1_ERROR_GROUP_DEVPROG, "bl1 group devprog"},
    {BL1_ERROR_GROUP_DEVPROG_DDR, "bl1 group devprog ddr"},
    {BL1_ERROR_GROUP_EFS, "bl1 group efs"},
    {BL1_ERROR_GROUP_EFS_LITE, "bl1 group efs lite"},
    {BL1_ERROR_GROUP_HOTPLUG, "bl1 group hot-plug"},
    {BL1_ERROR_GROUP_HSUSB, "bl1 group high speed usb"},
    {BL1_ERROR_GROUP_ICB, "bl1 group icb"},
    {BL1_ERROR_GROUP_LIMITS, "bl1 group limits"},
    {BL1_ERROR_GROUP_MHI, "bl1 group mhi"},
    {BL1_ERROR_GROUP_PCIE, "bl1 group pcie"},
    {BL1_ERROR_GROUP_PLATFOM, "bl1 group platform"},
    {BL1_ERROR_GROUP_PMIC, "bl1 group pmic"},
    {BL1_ERROR_GROUP_POWER, "bl1 group power"},
    {BL1_ERROR_GROUP_PRNG, "bl1 group prng"},
    {BL1_ERROR_GROUP_QUSB, "bl1 group qusb"},
    {BL1_ERROR_GROUP_SECIMG, "bl1 group secimg"},
    {BL1_ERROR_GROUP_SECBOOT, "bl1 group secboot"},
    {BL1_ERROR_GROUP_SECCFG, "bl1 group seccfg"},
    {BL1_ERROR_GROUP_SMEM, "bl1 group smem"},
    {BL1_ERROR_GROUP_SPMI, "bl1 group spmi"},
    {BL1_ERROR_GROUP_SUBSYS, "bl1 group subsystem"},
    {BL1_ERROR_GROUP_TLMM, "bl1 group tlmm"},
    {BL1_ERROR_GROUP_TSENS, "bl1 group tsensor"},
    {BL1_ERROR_GROUP_HWENGINES, "bl1 group hwengines"},
    {BL1_ERROR_GROUP_IMAGE_VERSION, "bl1 group image version"},
    {BL1_ERROR_GROUP_SECURITY, "bl1 group system security"},
    {BL1_ERROR_GROUP_STORAGE, "bl1 group storage"},
    {BL1_ERROR_GROUP_SYSTEMDRIVERS, "bl1 group system drivers"},
    {BL1_ERROR_GROUP_EXCEPTIONS, "bl1 group exceptions"},
    {BL1_ERROR_GROUP_MPROC, "bl1 group mppoc"},
    {BL2_PANIC, "bl2 panic"},
    {BL2_WDT, "bl2 wdt"},
    {BL2_PL1_OCV_ERROR, "ocv error"},
    {BL2_PL1_BAT_TEMP_ERROR, "battery temperature error"},
    {BL2_PL1_MISC_ERROR, "misc part dmaged"},
    {BL2_PL1_DTIMG_ERROR, "dt image dmaged"},
    {BL2_PL1_LOAD_OTHER_IMGS_ERRNO, "image dmaged"},
    {BL2_PL1_KERNEL_IMG_ERROR, "kernel image verify failed"},
    {BL2_MMC_INIT_FAILED, "bl2 mmc init error"},
    {BL2_QSEECOM_START_ERROR, "bl2 qseecom start error"},
    {BL2_RPMB_INIT_FAILED, "bl2 rpmb init failed"},
    {BL2_LOAD_SECAPP_FAILED, "bl2 load secapp failed"},
    {BL2_ABOOT_DLKEY_DETECTED, "bl2 dlkey failed"},
    {BL2_ABOOT_NORMAL_BOOT_FAIL, "bl2 aboot normal boot failed"},
    {BL2_BR_POWERON_BY_SMPL, "bl2 poweron by smpl"},
    {BL2_FASTBOOT_S_LOADLPMCU_FAIL, "bl2 load lpmcu failed"},
    {BL2_FASTBOOT_S_IMG_VERIFY_FAIL, "bl2 image verify failed"},
    {BL2_FASTBOOT_S_SOC_TEMP_ERR, "bl2 soc tmp err"},
    {BL2_PL1_BAT_LOW_POWER, "battery low power"},
    {KERNEL_AP_PANIC, "kernel ap panic"},
    {KERNEL_EMMC_INIT_FAIL, "kernel emm init failed"},
    {KERNEL_AP_WDT, "kernel ap wdt"},
    {KERNEL_PRESS10S, "kernel press10s"},
    {KERNEL_BOOT_TIMEOUT, "kernel boot timeout"},
    {KERNEL_AP_COMBINATIONKEY, "kernel ap combinationkey"},
    {KERNEL_AP_S_ABNORMAL, "kernel ap abnormal"},
    {KERNEL_AP_S_TSENSOR0, "kernel ap tsensor0"},
    {KERNEL_AP_S_TSENSOR1, "kernel ap tsensor1"},
    {KERNEL_LPM3_S_GLOBALWDT, "kernel lpm3 global wdt"},
    {KERNEL_G3D_S_G3DTSENSOR, "kernel g3d g3dtsensor"},
    {KERNEL_LPM3_S_LPMCURST, "kernel lpm3 lp mcu reset"},
    {KERNEL_CP_S_CPTSENSOR, "kernel cp cpt sensor"},
    {KERNEL_IOM3_S_IOMCURST, "kernel iom3 io mcu reset"},
    {KERNEL_ASP_S_ASPWD, "kernel asp as pwd"},
    {KERNEL_CP_S_CPWD, "kernel cp cp pwd"},
    {KERNEL_IVP_S_IVPWD, "kernel ivp iv pwd"},
    {KERNEL_ISP_S_ISPWD, "kernel isp is pwd"},
    {KERNEL_AP_S_DDR_UCE_WD, "kernel ap ddr uce wd"},
    {KERNEL_AP_S_DDR_FATAL_INTER, "kernel ap ddr fatal inter"},
    {KERNEL_AP_S_DDR_SEC, "kernel ap ddr sec"},
    {KERNEL_AP_S_MAILBOX, "kernel ap mailbox"},
    {KERNEL_CP_S_MODEMDMSS, "kernel cp modem dmss"},
    {KERNEL_CP_S_MODEMNOC, "kernel cp modem noc"},
    {KERNEL_CP_S_MODEMAP, "kernel cp modem ap"},
    {KERNEL_CP_S_EXCEPTION, "kernel cp exception"},
    {KERNEL_CP_S_RESETFAIL, "kernel cp reset failed"},
    {KERNEL_CP_S_NORMALRESET, "kernel cp normal reset"},
    {KERNEL_CP_S_ABNORMAL, "kernel cp abnormal"},
    {KERNEL_LPM3_S_EXCEPTION, "kernel lpm3 exception"},
    {KERNEL_HIFI_S_EXCEPTION, "kernel hisi exception"},
    {KERNEL_HIFI_S_RESETFAIL, "kernel hisi reset failed"},
    {KERNEL_ISP_S_EXCEPTION, "kernel isp exception"},
    {KERNEL_IVP_S_EXCEPTION, "kernel ivp exception"},
    {KERNEL_IOM3_S_EXCEPTION, "kernel iom3 exception"},
    {KERNEL_TEE_S_EXCEPTION, "kernel tee exception"},
    {KERNEL_MMC_S_EXCEPTION, "kernel mmc exception"},
    {KERNEL_CODECHIFI_S_EXCEPTION, "kernel codec hifi exception"},
    {KERNEL_CP_S_RILD_EXCEPTION, "kernel cp rild exception"},
    {KERNEL_CP_S_3RD_EXCEPTION, "kernel cp 3rd exception"},
    {KERNEL_IOM3_S_USER_EXCEPTION, "kernel iom3 user exception"},
    {KERNEL_MODEM_PANIC, "kernel modem panic"},
    {KERNEL_VENUS_PANIC, "kernel venus panic"},
    {KERNEL_WCNSS_PANIC, "kernel wcnss panic"},
    {KERNEL_SENSOR_PANIC, "kernel sensor panic"},
    {KERNEL_OCBC_S_WD, "kernel ocbc wd"},
    {KERNEL_AP_S_NOC, "kernel noc"},
    {KERNEL_AP_S_RESUME_SLOWY, "kernel resume slowly"},
    {KERNEL_AP_S_F2FS, "kernel f2fs abnormal"},
    {KERNLE_AP_S_BL31_PANIC, "kernel bl31 panic"},
    {KERNLE_HISEE_S_EXCEPTION, "kernel hisee exception"},
    {KERNEL_AP_S_PMU, "kernel ap pmu"},
    {KERNEL_AP_S_SMPL, "kernel ap smpl"},
    {KERNLE_AP_S_SCHARGER, "kernel charger abnormal"},
    {SYSTEM_MOUNT_FAIL, "system part mount failed"},
    {SECURITY_FAIL, "security failed"},
    {CRITICAL_SERVICE_FAIL_TO_START, "critical service start failed"},
    {DATA_MOUNT_FAILED_AND_ERASED, "data part mount failed"},
    {DATA_MOUNT_RO, "data part mounted ro"},
    {DATA_NOSPC, "no space on data part"},
    {VENDOR_MOUNT_FAIL, "vendor part mount failed"},
    {NATIVE_PANIC, "native critical fail"},
    {SYSTEM_SERVICE_LOAD_FAIL, "system service load failed"},
    {PREBOOT_BROADCAST_FAIL, "preboot broadcast failed"},
    {VM_OAT_FILE_DAMAGED, "ota file damaged"},
    {PACKAGE_MANAGER_SETTING_FILE_DAMAGED, "package manager setting file damaged"},
    {BFM_HARDWARE_FAULT, "hardware-fault"},
    {BOOTUP_SLOWLY, "bootup slowly"},
    {POWEROFF_ABNORMAL, "power off abnormally"},
};


static bfmr_bootfail_errno_e s_sensible_bootfail_with_reboot_recovery[] = {
    KERNEL_PRESS10S,
    KERNEL_BOOT_TIMEOUT,
    CRITICAL_SERVICE_FAIL_TO_START,
    POWEROFF_ABNORMAL,
    KERNEL_AP_WDT,
    KERNEL_LPM3_S_GLOBALWDT,
    KERNEL_LPM3_S_LPMCURST,
    BL2_WDT,
};

static char *s_valid_log_name[] = {
    BFM_BFI_FILE_NAME,
    BFM_RECOVERY_FILE_NAME,
    BFM_CRITICAL_PROCESS_CRASH_LOG_NAME,
    BFM_TOMBSTONE_LOG_NAME,
    BFM_SYSTEM_SERVER_CRASH_LOG_NAME,
    BFM_SYSTEM_SERVER_WATCHDOG_LOG_NAME,
    BFM_BL1_LOG_FILENAME,
    BFM_BL2_LOG_FILENAME,
    BFM_KERNEL_LOG_FILENAME,
    BFM_ALT_BL1_LOG_FILENAME,
    BFM_ALT_BL2_LOG_FILENAME,
    BFM_ALT_KERNEL_LOG_FILENAME,
    BFM_FRAMEWORK_BOOTFAIL_LOG_FILE_NAME,
    BFM_PMSG_LOG_FILENAME,
};

static bool s_is_bootup_successfully = false;

static bool s_is_process_boot_success = false;

#define SIG_TO_INIT      40
#define SIG_INT_VALUE 1234


/*----function definitions--------------------------------------------------------------*/


void bfm_send_signal_to_init(void)
{
    int pid = 1;
    int ret;
    struct siginfo info;
    struct task_struct *t;

    info.si_signo = SIG_TO_INIT;
    info.si_code = SI_QUEUE;
    info.si_int = SIG_INT_VALUE;

    rcu_read_lock();
    t = find_task_by_vpid(pid);
    if (t == NULL) {
        BFMR_PRINT_ERR("Init dump: no such pid\n");
        rcu_read_unlock();
    }
    else {
        rcu_read_unlock();
        ret = send_sig_info(SIG_TO_INIT, &info, t);
        if (ret < 0) {
            BFMR_PRINT_ERR("Init dump: error sending signal\n");
        } else {
            BFMR_PRINT_ERR("Init dump: sending signal success\n");
        }
    }
}


static char* bfm_get_boot_fail_no_desc(bfmr_bootfail_errno_e bootfail_errno, bfm_process_bootfail_param_t *pparam)
{
    int i = 0;
    int count = sizeof(s_bootfail_errno_desc) / sizeof(s_bootfail_errno_desc[0]);

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam.\n");
        return "unknown";
    }

    for (i = 0; i < count; i++)
    {
        if (bootfail_errno == s_bootfail_errno_desc[i].bootfail_errno)
        {
            return s_bootfail_errno_desc[i].desc;
        }
    }

    return "unknown";
}

static int bfm_get_fs_state(const char *pmount_point, struct statfs *pstatfsbuf)
{
    mm_segment_t old_fs;
    int ret = -1;

    if (unlikely((NULL == pmount_point) || (NULL == pstatfsbuf)))
    {
        BFMR_PRINT_INVALID_PARAMS("pmount_point or pstatfsbuf.\n");
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    memset((void *)pstatfsbuf, 0, sizeof(struct statfs));
    ret = sys_statfs(pmount_point, pstatfsbuf);
    set_fs(old_fs);

    return ret;
}

static int bfm_save_bootfail_info_txt(bfmr_log_dst_t *pdst, bfmr_log_src_t *psrc, bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;
    size_t bytes_formatted = 0;
    char *pdata = NULL;
    struct statfs statfsbuf = {0};
    char record_count_str[BFM_MAX_INT_NUMBER_LEN] = {'\0'};

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam.\n");
        return -1;
    }

    pdata = (char *)bfmr_malloc(BFMR_TEMP_BUF_LEN);
    if (NULL == pdata)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -1;
    }
    memset(pdata, 0, BFMR_TEMP_BUF_LEN);

    ret = bfmr_wait_for_part_mount_with_timeout("/data", BFM_WAIT_DATA_PART_TIME_OUT);
    if (0 == ret)
    {
        ret = bfm_get_fs_state("/data", &statfsbuf);
    }
    snprintf(record_count_str, sizeof(record_count_str) - 1, (0 == pparam->bootfail_log_info.log_dir_count) ? ("%s") : ("_%d"),
        (0 == pparam->bootfail_log_info.log_dir_count) ? ("") : (pparam->bootfail_log_info.log_dir_count + 1));/*lint !e679 */
    bytes_formatted = snprintf(pdata, BFMR_TEMP_BUF_LEN - 1, BFM_BFI_FILE_CONTENT_FORMAT,
        record_count_str, bfmr_convert_rtc_time_to_asctime(pparam->bootfail_time),
        record_count_str, bfm_get_boot_fail_no_desc(pparam->bootfail_errno, pparam),
        record_count_str, bfm_get_platform_name(),
        bfm_get_boot_stage_name((unsigned int)pparam->boot_stage),
        record_count_str, pparam->is_system_rooted,
        record_count_str, pparam->is_user_sensible,
        record_count_str, (0 == ret) ? ((long long)(statfsbuf.f_bavail * statfsbuf.f_bsize) / (long long)(BFMR_SIZE_1K * BFMR_SIZE_1K)) : (0L),
        (0 == ret) ? ((statfsbuf.f_blocks <= 0) ? (0LL) : ((long long)(100 * statfsbuf.f_bavail) / (long long)statfsbuf.f_blocks)) : (0LL),
        record_count_str, (0 == ret) ? ((long long)(statfsbuf.f_files - statfsbuf.f_ffree)) : (0L),
        (0 == ret) ? ((statfsbuf.f_files <= 0) ? (0LL) : ((long long)(100 * (statfsbuf.f_files - statfsbuf.f_ffree)) / (long long)statfsbuf.f_files)): (0LL),
        record_count_str, pparam->bootfail_time,
        record_count_str, (unsigned int)pparam->bootfail_errno,
        record_count_str, (unsigned int)pparam->boot_stage,
        record_count_str, pparam->is_system_rooted,
        record_count_str, pparam->is_user_sensible,
        record_count_str, pparam->dmd_num,
        record_count_str, pparam->excepInfo,
        (unsigned int)pparam->bootup_time,
        record_count_str, pparam->is_bootup_successfully ? "yes" : "no",
        record_count_str, pparam->reboot_type,
        bfm_get_bootlock_value_from_cmdline());

    if ((bytes_formatted < (BFMR_TEMP_BUF_LEN - 1)) &&
        ((BOOTUP_SLOWLY == pparam->bootfail_errno) || (KERNEL_BOOT_TIMEOUT == pparam->bootfail_errno)))
    {
        BFMR_PRINT_KEY_INFO("Record boot stages information.\n");
        bytes_formatted += bfm_save_stages_info_txt((char*)(pdata + bytes_formatted), (BFMR_TEMP_BUF_LEN - bytes_formatted));
    }

    switch (pdst->type)
    {
    case DST_FILE:
        {
            ret = bfmr_save_log(pparam->bootfail_log_dir, BFM_BFI_FILE_NAME, (void *)pdata, bytes_formatted, 0);
            if (0 != ret)
            {
                BFMR_PRINT_ERR("save [%s] failed!\n", BFM_BFI_FILE_NAME);
            }
            break;
        }
    case DST_RAW_PART:
        {
            bfmr_save_log_to_raw_part(pdst->dst_info.raw_part.raw_part_name,
                pdst->dst_info.raw_part.offset,
                (void *)pdata, bytes_formatted);
            psrc->log_type = LOG_TYPE_BFM_BFI_LOG;
            strncpy(psrc->src_log_file_path, BFM_BFI_FILE_NAME, BFMR_MAX_PATH);
            bfmr_update_raw_log_info(psrc, pdst, bytes_formatted);
            break;
        }
    case DST_MEMORY_BUFFER:
    default:
        {
            bfmr_save_log_to_mem_buffer(pdst->dst_info.buffer.addr, pdst->dst_info.buffer.len, (void *)pdata, bytes_formatted);
            break;
        }
    }

    bfmr_free(pdata);

    return ret;
}


static int bfm_get_recovery_result(bfr_suggested_recovery_method_e suggested_recovery_method)
{
    return (DO_NOTHING == suggested_recovery_method)
        ? (int)(BFM_RECOVERY_SUCCESS_INT_VALUE)
        : (int)(BFM_RECOVERY_FAIL_INT_VALUE);
}


static int bfm_save_recovery_info_txt(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;
    char *pdata = NULL;
    char record_count_str[BFM_MAX_INT_NUMBER_LEN] = {'\0'};

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam.\n");
        return -1;
    }

    pdata = (char *)bfmr_malloc(BFMR_TEMP_BUF_LEN);
    if (NULL == pdata)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -1;
    }
    memset(pdata, 0, BFMR_TEMP_BUF_LEN);

    snprintf(record_count_str, sizeof(record_count_str) - 1, (DO_NOTHING == pparam->suggested_recovery_method)
        ? ("%s") : ("_%d"), (DO_NOTHING == pparam->suggested_recovery_method)
        ? ("") : (pparam->bootfail_log_info.log_dir_count + 1));/*lint !e679 */
    (void)snprintf(pdata, BFMR_TEMP_BUF_LEN - 1, BFM_RCV_FILE_CONTENT_FORMAT,
        record_count_str, bfr_get_recovery_method_desc(pparam->recovery_method),
        record_count_str, (BFM_RECOVERY_SUCCESS_INT_VALUE == bfm_get_recovery_result(pparam->suggested_recovery_method))
        ?  BFM_RECOVERY_SUCCESS_STR : BFM_RECOVERY_FAIL_STR,
        record_count_str, pparam->recovery_method,
        record_count_str, bfm_get_recovery_result(pparam->suggested_recovery_method));
    switch (pdst->type)
    {
    case DST_FILE:
        {
            ret = bfmr_save_log(pparam->bootfail_log_dir, BFM_RECOVERY_FILE_NAME, (void *)pdata, strlen(pdata), 0);
            if (0 != ret)
            {
                BFMR_PRINT_ERR("save [%s] failed!\n", BFM_RECOVERY_FILE_NAME);
            }
            break;
        }
    case DST_RAW_PART:
        {
            bfmr_save_log_to_raw_part(pdst->dst_info.raw_part.raw_part_name,
                pdst->dst_info.raw_part.offset,
                (void *)pdata, strlen(pdata));
            psrc->log_type = LOG_TYPE_BFM_RECOVERY_LOG;
            strncpy(psrc->src_log_file_path, BFM_RECOVERY_FILE_NAME, BFMR_MAX_PATH);
            bfmr_update_raw_log_info(psrc, pdst, strlen(pdata));
            break;
        }
    case DST_MEMORY_BUFFER:
    default:
        {
            bfmr_save_log_to_mem_buffer(pdst->dst_info.buffer.addr, pdst->dst_info.buffer.len, (void *)pdata, strlen(pdata));
            break;
        }
    }

__out:
    bfmr_free(pdata);

    return ret;
}


int bfm_get_log_count(char *bfmr_log_root_path)
{
    int fd = -1;
    int num;
    int log_count = 0;
    void *buf = NULL;
    char *full_path = NULL;
    struct linux_dirent64 *dirp;
    mm_segment_t oldfs;

    if (unlikely((NULL == bfmr_log_root_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("bfmr_log_root_path.\n");
        return 0;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    fd = sys_open(bfmr_log_root_path, O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed!\n", bfmr_log_root_path);
        goto __out;
    }

    buf = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    dirp = buf;
    num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    while (num > 0)
    {
        while (num > 0)
        {
            bfm_stat_t st;
            int ret;

            memset(full_path, 0, BFMR_MAX_PATH);
            snprintf(full_path, BFMR_MAX_PATH - 1, "%s/%s", bfmr_log_root_path, dirp->d_name);
            if ((0 == strcmp(dirp->d_name, ".")) || (0 == strcmp(dirp->d_name, "..")))
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                continue;
            }

            memset((void *)&st, 0, sizeof(bfm_stat_t));
            ret = bfm_sys_lstat(full_path, &st);
            if ((0 == ret)
                && (S_ISDIR(st.st_mode))
                && (0 != strcmp(dirp->d_name, BFM_UPLOADING_DIR_NAME)))/*lint !e421 */
            {
                log_count++;
            }

            num -= dirp->d_reclen;
            dirp = (void *)dirp + dirp->d_reclen;
        }

        dirp = buf;
        memset(buf, 0, BFMR_MAX_PATH);
        num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }
    set_fs(oldfs);

    BFMR_PRINT_DBG("Log count: %d\n", log_count);

    bfmr_free(buf);
    bfmr_free(full_path);

    return log_count;
}


void bfm_delete_dir(char *log_path)
{
    int fd = -1;
    void *buf = NULL;
    char *full_path = NULL;
    struct linux_dirent64 *dirp;
    int num;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);/*lint !e501 */
    fd = sys_open(log_path, O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed![ret = %d]\n", log_path, fd);
        goto __out;
    }

    buf =(char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(buf, 0, BFMR_MAX_PATH);

    full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(full_path, 0, BFMR_MAX_PATH);

    dirp = buf;
    num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    while (num > 0)
    {
        while (num > 0)
        {
            bfm_stat_t st;
            int ret;

            memset(full_path, 0, BFMR_MAX_PATH);
            memset((void *)&st, 0, sizeof(bfm_stat_t));
            snprintf(full_path, BFMR_MAX_PATH - 1, "%s/%s", log_path, dirp->d_name);
            if ((0 != strcmp(dirp->d_name, ".")) && (0 != strcmp(dirp->d_name, "..")))/*lint !e421 */
            {
                ret = bfm_sys_lstat(full_path, &st);
                if (0 == ret)
                {
                    if (S_ISDIR(st.st_mode))
                    {
                        sys_rmdir(full_path);
                    }
                    else
                    {
                        sys_unlink(full_path);
                    }
                }
            }
            num -= dirp->d_reclen;
            dirp = (void *)dirp + dirp->d_reclen;
        }
        dirp = buf;
        memset(buf, 0, BFMR_MAX_PATH);
        num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }

    /* remove the log path */
    sys_rmdir(log_path);
    set_fs(oldfs);

    bfmr_free(buf);
    bfmr_free(full_path);
}


static void bfm_delete_oldest_log(long long bytes_need_this_time)
{
    int log_count = 0;
    char *log_path = NULL;
    long long available_space = 0LL;
    bool should_delete_oldest_log = false;

    log_count = bfm_get_log_count(bfm_get_bfmr_log_root_path());
    BFMR_PRINT_KEY_INFO("There are %d logs in %s\n", log_count, bfm_get_bfmr_log_root_path());
    if (log_count < BFM_LOG_MAX_COUNT)
    {
        available_space = bfmr_get_fs_available_space(bfm_get_bfmr_log_part_mount_point());
        if (available_space < bytes_need_this_time)
        {
            should_delete_oldest_log = true;
        }
    }
    else
    {
        should_delete_oldest_log = true;
    }

    if (!should_delete_oldest_log)
    {
        goto __out;
    }

    log_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == log_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(log_path, 0, BFMR_MAX_PATH);

    (void)bfm_lookup_dir_by_create_time(bfm_get_bfmr_log_root_path(), log_path, BFMR_MAX_PATH, 1);
    if (0 != strlen(log_path))
    {
        bfm_delete_dir(log_path);
    }

__out:
    bfmr_free(log_path);
}


static void bfm_process_after_save_bootfail_log(void)
{
    BFMR_PRINT_KEY_INFO("restart system now!\n");
    kernel_restart(NULL);
    return;
}


static long long bfm_get_basic_space_for_each_bootfail_log(bfmr_bootfail_errno_e bootfail_errno)
{
    int i = 0;
    int count = sizeof(s_log_size_param) / sizeof(s_log_size_param[0]);

    for (i = 0; i < count; i++)
    {
        if (bfmr_get_boot_stage_from_bootfail_errno(bootfail_errno) == s_log_size_param[i].boot_stage)
        {
            return s_log_size_param[i].log_size_allowed;
        }
    }

    return (long long)(BFM_BOOTFAIL_INFO_LOG_MAX_LEN + BFM_RECOVERY_INFO_LOG_MAX_LEN);
}


static long long bfm_get_extra_space_for_each_bootfail_log(bfm_process_bootfail_param_t *pparam)
{
    long long bytes_need = 0LL;

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam.\n");
        return 0LL;
    }

    if (0 != strcmp(pparam->addl_info.log_path, BFM_FRAMEWORK_BOOTFAIL_LOG_PATH))
    {
        bytes_need = (long long)(bfmr_get_file_length(BFM_LOGCAT_FILE_PATH)
            + bfmr_get_file_length(BFM_FRAMEWORK_BOOTFAIL_LOG_PATH)
            + bfmr_get_file_length(pparam->addl_info.log_path));
    }
    else
    {
        bytes_need = (long long)(bfmr_get_file_length(BFM_LOGCAT_FILE_PATH)
            + bfmr_get_file_length(pparam->addl_info.log_path)); 
    }

    return bytes_need;
}


static int bfm_save_extra_bootfail_logs(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;
    char src_path[BFMR_SIZE_256] = {0};

    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst or psrc or pparam.\n");
        return -1;
    }

    /* 1. save logcat */
    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, BFM_LOGCAT_FILE_NAME);
    if (bfm_get_symbol_link_path(BFM_LOGCAT_FILE_PATH, src_path, sizeof(src_path)))
    {
        (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
            "%s/%s.gz", pparam->bootfail_log_dir, BFM_LOGCAT_FILE_NAME_KEYWORD);
    }
    psrc->log_type = LOG_TYPE_BETA_APP_LOGCAT;
    ret = bfmr_capture_and_save_log(psrc, pdst, pparam);

    /* 2. save framework bootfail log */
    if ((pparam->boot_stage >= ANDROID_FRAMEWORK_STAGE_START)
        && (0 != strcmp(pparam->addl_info.log_path, BFM_FRAMEWORK_BOOTFAIL_LOG_PATH)))
    {
        memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
        (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
            "%s/%s", pparam->bootfail_log_dir, BFM_FRAMEWORK_BOOTFAIL_LOG_FILE_NAME);
        psrc->log_type = LOG_TYPE_FIXED_FRAMEWORK_BOOTFAIL_LOG;
        ret = bfmr_capture_and_save_log(psrc, pdst, pparam);
        bfmr_change_file_ownership(BFM_FRAMEWORK_BOOTFAIL_LOG_PATH, pparam->user_space_log_uid,
            pparam->user_space_log_gid);
    }

    return ret;
}


static int bfm_capture_and_save_bl1_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst or psrc or pparam.\n");
        return -1;
    }

    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_bl1_bootfail_log_name());
    psrc->log_type = LOG_TYPE_BOOTLOADER_1;

    return bfmr_capture_and_save_log(psrc, pdst, pparam);
}


static int bfm_capture_and_save_bl2_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst or psrc or pparam.\n");
        return -1;
    }

    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_bl2_bootfail_log_name());
    psrc->log_type = LOG_TYPE_BOOTLOADER_2;

    return bfmr_capture_and_save_log(psrc, pdst, pparam);
}


static int bfm_capture_and_save_kernel_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    bool file_existed = false;

    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst or psrc or pparam.\n");
        return -1;
    }

    /* capture last kmsg of zip type on Beta version */
    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    file_existed = bfmr_is_file_existed(BFM_BETA_KMSG_LOG_PATH);
    BFMR_PRINT_KEY_INFO("[%s] %s!\n", BFM_BETA_KMSG_LOG_PATH, (file_existed) ? ("exists") : ("doesn't exist"));
    if (bfm_is_beta_version() && file_existed)
    {
        (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
            "%s/%s.gz", pparam->bootfail_log_dir, BFM_KERNEL_LOG_GZ_FILENAME_KEYWORD);
        psrc->log_type = LOG_TYPE_ANDROID_KMSG;
        (void)bfmr_capture_and_save_log(psrc, pdst, pparam);
    }

    /* capture last kmsg of text type */
    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_kernel_bootfail_log_name());
    psrc->log_type = LOG_TYPE_TEXT_KMSG;

    return bfmr_capture_and_save_log(psrc, pdst, pparam);
}


static int bfm_capture_and_save_ramoops_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst or psrc or pparam.\n");
        return -1;
    }

    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_ramoops_bootfail_log_name());
    psrc->log_type = LOG_TYPE_RAMOOPS;

    return bfmr_capture_and_save_log(psrc, pdst, pparam);
}


static char *bfm_get_file_name(char *file_full_path)
{
    char *ptemp = NULL;

    if (unlikely((NULL == file_full_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("file_full_path.\n");
        return NULL;
    }

    ptemp = strrchr(file_full_path, '/');
    if (NULL == ptemp)
    {
        return file_full_path;
    }

    return (ptemp + 1);
}


static int bfm_capture_and_save_native_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;

    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst, psrc, pparam.\n");
        return -1;
    }

    if (0 == strlen(pparam->addl_info.log_path))
    {
        BFMR_PRINT_KEY_INFO("user log path hasn't been set!\n");
        return -1;
    }
    else
    {
        memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
        if (NULL != strstr(pparam->addl_info.log_path, BFM_TOMBSTONE_FILE_NAME_TAG))
        {
            (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
                "%s/%s", pparam->bootfail_log_dir, BFM_TOMBSTONE_LOG_NAME);
            psrc->log_type = LOG_TYPE_VM_TOMBSTONES;
        }
        else if (NULL != strstr(pparam->addl_info.log_path, BFM_SYSTEM_SERVER_CRASH_FILE_NAME_TAG))
        {
            (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
                "%s/%s", pparam->bootfail_log_dir, BFM_SYSTEM_SERVER_CRASH_LOG_NAME);
            psrc->log_type = LOG_TYPE_VM_CRASH;
        }
        else if (NULL != strstr(pparam->addl_info.log_path, BFM_SYSTEM_SERVER_WATCHDOG_FILE_NAME_TAG))
        {
            (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
                "%s/%s", pparam->bootfail_log_dir, BFM_SYSTEM_SERVER_WATCHDOG_LOG_NAME);
            psrc->log_type = LOG_TYPE_VM_WATCHDOG;
        }
        else if (NULL != strstr(pparam->addl_info.log_path, BFM_CRITICAL_PROCESS_CRASH_LOG_NAME))
        {
            (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
                "%s/%s", pparam->bootfail_log_dir, BFM_CRITICAL_PROCESS_CRASH_LOG_NAME);
            psrc->log_type = LOG_TYPE_CRITICAL_PROCESS_CRASH;
        }
        else
        {
            BFMR_PRINT_KEY_INFO("Invalid user bootfail log!\n");
            return -1;
        }
    }

    ret = bfmr_capture_and_save_log(psrc, pdst, pparam);
    bfmr_change_file_ownership(pparam->addl_info.log_path, pparam->user_space_log_uid, pparam->user_space_log_gid);

    /* remove critical process crash log in /cache */
    if (NULL != strstr(pparam->addl_info.log_path, BFM_CRITICAL_PROCESS_CRASH_LOG_NAME))
    {
        bfmr_unlink_file(pparam->addl_info.log_path);
    }

    return ret;
}


static int bfm_capture_and_save_framework_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;

    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst or psrc or pparam.\n");
        return -1;
    }

    if (0 == strlen(pparam->addl_info.log_path))
    {
        BFMR_PRINT_KEY_INFO("user log path hasn't been set!\n");
        return -1;
    }

    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_file_name(pparam->addl_info.log_path));
    psrc->log_type = LOG_TYPE_NORMAL_FRAMEWORK_BOOTFAIL_LOG;
    ret = bfmr_capture_and_save_log(psrc, pdst, pparam);
    bfmr_change_file_ownership(pparam->addl_info.log_path, pparam->user_space_log_uid, pparam->user_space_log_gid);

    return ret;
}


static bool bfm_is_log_existed(unsigned long long rtc_time, unsigned int bootfail_errno)
{
    char *log_full_path = NULL;
    bool ret = false;

    log_full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == log_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)log_full_path, 0, BFMR_MAX_PATH);

    snprintf(log_full_path, BFMR_MAX_PATH - 1, "%s/" BFM_BOOTFAIL_LOG_DIR_NAME_FORMAT,
        bfm_get_bfmr_log_root_path(), bfmr_convert_rtc_time_to_asctime(rtc_time), bootfail_errno);
    ret = bfmr_is_dir_existed(log_full_path);
    BFMR_PRINT_KEY_INFO("[%s] %s\n", log_full_path, (ret) ? ("exists!") : ("does't exist!"));

__out:
    bfmr_free(log_full_path);

    return ret;
}


static int bfm_capture_and_save_bootfail_log(bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;
    bfmr_log_dst_t dst;
    bfmr_log_src_t src;

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam.\n");
        return -1;
    }

    memset((void *)&dst, 0, sizeof(dst));
    memset((void *)&src, 0, sizeof(src));
    dst.type = pparam->dst_type;
    src.log_save_additional_context = (void *)pparam;
    switch (dst.type)
    {
    case DST_FILE:
        {
            dst.dst_info.filename = pparam->bootfail_log_path;
            src.src_log_file_path = pparam->addl_info.log_path;
            src.log_save_context = pparam->log_save_context;
            if (!bfmr_is_part_mounted_rw(bfm_get_bfmr_log_part_mount_point()))
            {
                BFMR_PRINT_ERR("the log part hasn't been mounted to: %s\n", bfm_get_bfmr_log_part_mount_point());
                goto __out;
            }

            /* create log root path */
            (void)bfmr_create_log_path(bfm_get_bfmr_log_root_path());

            /* create uploading path */
            (void)bfmr_create_log_path(bfm_get_bfmr_log_uploading_path());

            /* check if the log is existed or not */
            if (bfm_is_log_existed(pparam->bootfail_time, (unsigned int)pparam->bootfail_errno))
            {
                ret = 0;
                goto __out;
            }

            /* delete oldest log */
            if (0 == pparam->save_bottom_layer_bootfail_log)
            {
                bfm_delete_oldest_log(bfm_get_basic_space_for_each_bootfail_log(pparam->bootfail_errno)
                    + bfm_get_extra_space_for_each_bootfail_log(pparam));
            }
            else
            {
                bfm_delete_oldest_log(bfm_get_basic_space_for_each_bootfail_log(pparam->bootfail_errno));    
            }

            /* traverse all the bootfail logs */
            bfm_traverse_log_root_dir(&(pparam->bootfail_log_info));

            /* create boot fail log dir */
            (void)snprintf(pparam->bootfail_log_dir, sizeof(pparam->bootfail_log_dir) - 1,
                "%s/" BFM_BOOTFAIL_LOG_DIR_NAME_FORMAT, bfm_get_bfmr_log_root_path(),
                bfmr_convert_rtc_time_to_asctime(pparam->bootfail_time), (unsigned int)pparam->bootfail_errno);
            bfmr_create_log_path(pparam->bootfail_log_dir);

            break;
        }
    case DST_RAW_PART:
        {
            dst.dst_info.raw_part.raw_part_name = bfm_get_raw_part_name();
            dst.dst_info.raw_part.offset += bfm_get_raw_part_offset();
            src.src_log_file_path = pparam->addl_info.log_path;
            (void)snprintf(pparam->bootfail_log_dir, sizeof(pparam->bootfail_log_dir) - 1,
                "%s/" BFM_BOOTFAIL_LOG_DIR_NAME_FORMAT, bfm_get_bfmr_log_root_path(),
                bfmr_convert_rtc_time_to_asctime(pparam->bootfail_time), (unsigned int)pparam->bootfail_errno);
            bfmr_alloc_and_init_raw_log_info(pparam, &dst);
            src.log_save_context = pparam->log_save_context;
            break;
        }
    case DST_MEMORY_BUFFER:
        {
            break;
        }
    default:
        {
            BFMR_PRINT_ERR("Invalid dst type: %d\n", dst.type);
            goto __out;
        }
    }

    /* save bootFail_info.txt */
    bfm_save_bootfail_info_txt(&dst, &src, pparam);

    /* save recovery_info.txt */
    bfm_save_recovery_info_txt(&dst, &src, pparam);

    ret = 0;
    switch (bfmr_get_boot_stage_from_bootfail_errno(pparam->bootfail_errno))
    {
    case BL1_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ BL1, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            break;
        }
    case BL2_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ BL2, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            break;
        }
    case KERNEL_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ Kernel, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_kernel_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_ramoops_bootfail_log(&dst, &src, pparam);
            break;
        }
    case NATIVE_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ Native, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_kernel_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_ramoops_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_native_bootfail_log(&dst, &src, pparam);
            break;
        }
    case ANDROID_FRAMEWORK_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ Framework, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_kernel_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_ramoops_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_native_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_framework_bootfail_log(&dst, &src, pparam);
            break;
        }
    default:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ Unknown stage, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_kernel_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_ramoops_bootfail_log(&dst, &src, pparam);
            break;
        }
    }

    /* save extra logs such as: logcat */
    if (0 ==pparam->save_bottom_layer_bootfail_log)
    {
        bfm_save_extra_bootfail_logs(&dst, &src, pparam);
    }

    if (DST_RAW_PART == dst.type)
    {
        bfmr_save_and_free_raw_log_info(pparam);
    }

__out:
    return ret;
}


static int bfm_process_upper_layer_boot_fail(void *param)
{
    bfm_process_bootfail_param_t *pparam =  (bfm_process_bootfail_param_t*)param;

    if (unlikely(NULL == param))
    {
        BFMR_PRINT_INVALID_PARAMS("param.\n");
        goto __out;
    }

    if ((0 != strlen(pparam->addl_info.log_path)) && (pparam->user_space_log_read_len <= 0))
    {
        uid_t uid = 0;
        gid_t gid = 0;

        /* if the user space log's original ownership has not been gotten correctly, we can not change its onwership here */
        if ((0 == bfmr_get_uid_gid(&uid, &gid)) && (-1 != (int)pparam->user_space_log_uid)
            && (-1 != (int)pparam->user_space_log_gid))
        {
            bfmr_change_file_ownership(pparam->addl_info.log_path, uid, gid);
        }
    }

    /* 1. this case means the boot fail has been resolved */
    if (DO_NOTHING == pparam->suggested_recovery_method)
    {
        BFMR_PRINT_KEY_INFO("suggested recovery method is: \"DO_NOTHING\"\n");
        (void)bfm_capture_and_save_do_nothing_bootfail_log(pparam);
        goto __out;
    }

    /* 2. process boot fail furtherly in platform */
    (void)bfm_platform_process_boot_fail(pparam);

    if (0 == pparam->bootfail_can_only_be_processed_in_platform)
    {
        /* 3. capture and save log for most boot fail errno */
        bfm_capture_and_save_bootfail_log(pparam);

        /* 4. post process after save bootfail log */
        bfm_process_after_save_bootfail_log();
    }

__out:

    if (NULL != pparam)
    {
        if (NULL != pparam->user_space_log_buf)
        {
            bfmr_free(pparam->user_space_log_buf);
        }
        bfmr_free(param);
    }
    msleep(BFM_SAVE_LOG_INTERVAL_FOR_EACH_LOG);
    complete(&s_process_boot_fail_comp);

    return 0;
}


static void bfm_wait_for_compeletion_of_processing_boot_fail(void)
{
    while (1)
    {
        msleep_interruptible(BFM_BLOCK_CALLING_PROCESS_INTERVAL);
    }
}


static void bfm_user_space_process_read_its_own_file(bfm_process_bootfail_param_t *pparam)
{
    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam.\n");
        return;
    }

    pparam->user_space_log_len = bfmr_get_file_length(pparam->addl_info.log_path);
    if (pparam->user_space_log_len > 0)
    {
        pparam->user_space_log_buf = (char *)bfmr_malloc(pparam->user_space_log_len + 1);
        if (NULL != pparam->user_space_log_buf)
        {
            memset(pparam->user_space_log_buf, 0, pparam->user_space_log_len + 1);
            pparam->user_space_log_read_len = bfmr_full_read_with_file_path(pparam->addl_info.log_path,
                pparam->user_space_log_buf, pparam->user_space_log_len);
            BFMR_PRINT_KEY_INFO("Read file [%s] [%ld] Bytes successsfully, its length is [%ld] Bytes!\n",
                pparam->addl_info.log_path, pparam->user_space_log_read_len, pparam->user_space_log_len);
        }
    }
}


/**
    @function: int boot_fail_err(bfmr_bootfail_errno_e bootfail_errno,
        bfr_suggested_recovery_method_e suggested_recovery_method,
        bfmr_bootfail_addl_info_t *paddl_info)
    @brief: save the log and do proper recovery actions when meet with error during system booting process.

    @param: bootfail_errno [in], boot fail error no.
    @param: suggested_recovery_method [in], suggested recovery method, if you don't know, please transfer NO_SUGGESTION for it
    @param: paddl_info [in], saving additional info such as log path and so on.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int boot_fail_err(bfmr_bootfail_errno_e bootfail_errno,
    bfr_suggested_recovery_method_e suggested_recovery_method,
    bfmr_bootfail_addl_info_t *paddl_info)
{
    bfmr_detail_boot_stage_e boot_stage;
    bfm_process_bootfail_param_t *pparam = NULL;
    static bool s_is_comp_init = false;

    BFMR_PRINT_ENTER();
    if (!bfmr_has_been_enabled())
    {
        BFMR_PRINT_KEY_INFO("BFMR is disabled!\n");
        return 0;
    }

    mutex_lock(&s_process_boot_fail_mutex);
    if (!s_is_comp_init)
    {
        complete(&s_process_boot_fail_comp);
        s_is_comp_init = true;
    }

    bfmr_get_boot_stage(&boot_stage);
    if (bfmr_is_boot_success(boot_stage))
    {
        if ((CRITICAL_SERVICE_FAIL_TO_START != bootfail_errno) && (BOOTUP_SLOWLY != bootfail_errno)
            && (BFM_HARDWARE_FAULT != bootfail_errno))
        {
            BFMR_PRINT_ERR("Error: can't set errno [%x] after device boot success!\n", (unsigned int)bootfail_errno);
            goto __out;
        }

        BFMR_PRINT_KEY_INFO("%s", (CRITICAL_SERVICE_FAIL_TO_START == bootfail_errno)
            ? ("critical process work abnormally after boot success") : ("bootup slowly!\n"));
    }

    if (!bfr_has_been_enabled())
    {
        BFMR_PRINT_ERR("BFR has been disabled, so set suggested_recovery_method = DO_NOTHING!\n");
        suggested_recovery_method = DO_NOTHING;
    }

    if (DO_NOTHING == suggested_recovery_method)
    {
        if (wait_for_completion_timeout(&s_process_boot_fail_comp, msecs_to_jiffies(BFM_SAVE_LOG_MAX_TIME)) == 0)
        {
            BFMR_PRINT_KEY_INFO("last boot_err is in processing, this error skip for DO_NOTHING!\n");
            goto __out;
        }
    }
    else
    {
        wait_for_completion(&s_process_boot_fail_comp);
    }

    pparam = (bfm_process_bootfail_param_t*)bfmr_malloc(sizeof(bfm_process_bootfail_param_t));
    if (NULL == pparam)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pparam, 0, sizeof(bfm_process_bootfail_param_t));

    pparam->bootfail_errno = bootfail_errno;
    pparam->boot_stage = boot_stage;
    pparam->is_user_sensible = (suggested_recovery_method != DO_NOTHING) ? (1) : (0);
    pparam->bootfail_errno = bootfail_errno;
    pparam->suggested_recovery_method = suggested_recovery_method;
    pparam->user_space_log_uid = (uid_t)-1;
    pparam->user_space_log_gid = (gid_t)-1;
    pparam->is_bootup_successfully = s_is_bootup_successfully;
    if (NULL != paddl_info)
    {
        memcpy(&pparam->addl_info, paddl_info, sizeof(pparam->addl_info));
        (void)bfmr_get_file_ownership(pparam->addl_info.log_path, &pparam->user_space_log_uid, &pparam->user_space_log_gid);
        bfm_user_space_process_read_its_own_file(pparam);
    }
    pparam->bootfail_time = bfm_get_system_time();
    pparam->bootup_time = bfmr_get_bootup_time();
    pparam->is_user_sensible = bfm_is_user_sensible_bootfail(pparam->bootfail_errno, pparam->suggested_recovery_method),
    pparam->is_system_rooted = bfm_is_system_rooted();
    pparam->bootfail_can_only_be_processed_in_platform = 0;
    pparam->capture_and_save_bootfail_log = bfm_capture_and_save_bootfail_log;
    kthread_run(bfm_process_upper_layer_boot_fail, (void *)pparam, "bfm_process_upper_layer_boot_fail");
    if (!bfr_has_been_enabled())
    {
        wait_for_completion(&s_process_boot_fail_comp);
        complete(&s_process_boot_fail_comp);
    }

    if (DO_NOTHING != suggested_recovery_method)
    {
        bfm_wait_for_compeletion_of_processing_boot_fail();
    }
__out:
    mutex_unlock(&s_process_boot_fail_mutex);
    BFMR_PRINT_EXIT();

    return 0;/*lint !e429 */
}



/**
    @function: int bfmr_set_boot_stage(bfmr_detail_boot_stage_e boot_stage)
    @brief: get current boot stage during boot process.

    @param: boot_stage [in], boot stage to be set.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_set_boot_stage(bfmr_detail_boot_stage_e boot_stage)
{
    mutex_lock(&s_process_boot_stage_mutex);
    (void)bfm_set_boot_stage(boot_stage);

    //record the start time of boot stage, only before bootup successful
    if (false == s_is_bootup_successfully)
    {
        bfm_stage_info_t stage_info = {0};

        stage_info.stage      = boot_stage;
        stage_info.start_time = bfmr_get_bootup_time();

        bfm_add_stage_info(&stage_info);
    }

    if (bfmr_is_boot_success(boot_stage))
    {
        BFMR_PRINT_KEY_INFO("boot success!\n");
        bfm_stop_boot_timer();
        kthread_run(bfm_notify_boot_success, NULL, "bfm_notify_boot_success");
    }
    mutex_unlock(&s_process_boot_stage_mutex);

    return 0;
}


/**
    @function: int bfmr_get_boot_stage(bfmr_detail_boot_stage_e *pboot_stage)
    @brief: get current boot stage during boot process.

    @param: pboot_stage [out], buffer storing the boot stage.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_get_boot_stage(bfmr_detail_boot_stage_e *pboot_stage)
{
    int ret = -1;
    bfmr_detail_boot_stage_e boot_stage;

    mutex_lock(&s_process_boot_stage_mutex);
    ret = bfm_get_boot_stage(&boot_stage);
    *pboot_stage = boot_stage;
    mutex_unlock(&s_process_boot_stage_mutex);

    return ret;
}


static int bfm_update_bootfail_info_for_each_log(bfm_bootfail_log_info_t *pbootfail_log_info)
{
    return 0;
}


static int bfm_update_recovery_info_for_each_log(bfm_bootfail_log_info_t *pbootfail_log_info)
{
    char *pdata = NULL;
    char record_count_str[BFM_MAX_INT_NUMBER_LEN] = {'\0'};
    char time[BFM_MAX_INT_NUMBER_LEN] = {'\0'};
    int i = 0;
    int bytes_formatted = 0;
    char c = '\0';

    if (unlikely(NULL == pbootfail_log_info))
    {
        BFMR_PRINT_INVALID_PARAMS("preal_recovery_info.\n");
        return -1;
    }

    pdata = bfmr_malloc(BFMR_SIZE_4K + sizeof(char));
    if (NULL == pdata)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(pdata, 0, BFMR_SIZE_4K + sizeof(char));

    /* format the recovery info */
    for (i = 0; i < pbootfail_log_info->real_recovery_info.record_count; i++)
    {
        snprintf(record_count_str, sizeof(record_count_str) - 1, (pbootfail_log_info->real_recovery_info.record_count == (i + 1))
            ? ("%s") : ("_%d"), (pbootfail_log_info->real_recovery_info.record_count == (i + 1)) ? ("") : (i + 1));
        snprintf(time, sizeof(time) - 1, "0x%08x", pbootfail_log_info->real_recovery_info.boot_fail_rtc_time[i]);
        bytes_formatted += snprintf(pdata + bytes_formatted, BFMR_SIZE_4K - bytes_formatted, (1 == pbootfail_log_info->real_recovery_info.record_count)
            ? ("%s" BFM_RCV_FILE_CONTENT_FORMAT) : (BFM_TEXT_LOG_SEPRATOR_WITHOUT_FIRST_NEWLINE BFM_RCV_FILE_CONTENT_FORMAT),
            (1 == pbootfail_log_info->real_recovery_info.record_count) ? ("") : (time),
            record_count_str, bfr_get_recovery_method_desc(pbootfail_log_info->real_recovery_info.recovery_method[i]),
            record_count_str, (((pbootfail_log_info->real_recovery_info.record_count == (i + 1))
            || (bfmr_is_boot_success(pbootfail_log_info->real_recovery_info.boot_fail_stage[i])))
            ? BFM_RECOVERY_SUCCESS_STR : BFM_RECOVERY_FAIL_STR),
            record_count_str, pbootfail_log_info->real_recovery_info.recovery_method[i],
            record_count_str, (((pbootfail_log_info->real_recovery_info.record_count == (i + 1))
            || (bfmr_is_boot_success(pbootfail_log_info->real_recovery_info.boot_fail_stage[i])))
            ? (BFM_RECOVERY_SUCCESS_INT_VALUE) : (BFM_RECOVERY_FAIL_INT_VALUE)));
    }

    /* save recovery info */
    if (0 != bfmr_save_log(pbootfail_log_info->bootfail_logs[0].log_dir, BFM_RECOVERY_FILE_NAME, (void *)pdata, strlen(pdata), 0))
    {
        BFMR_PRINT_ERR("Failed to update recovery_info.txt!\n");
    }

    /* Create DONE file for each file */
    bfmr_save_log(pbootfail_log_info->bootfail_logs[0].log_dir, BFM_DONE_FILE_NAME, (void *)&c, sizeof(char), 0);

__out:
    bfmr_free(pdata);

    return 0;
}


static bool bfm_is_user_sensible_boot_fail(bfm_bootfail_log_info_t *pbootfail_log_info)
{
    int i = 0;
    int j = 0;
    int count = (int)(sizeof(s_sensible_bootfail_with_reboot_recovery) / sizeof(s_sensible_bootfail_with_reboot_recovery[0]));

    if ((0 == pbootfail_log_info->real_recovery_info.record_count) && (pbootfail_log_info->log_dir_count > 0))
    {
        /* Maybe DO_NOTHING log */
        return true;
    }

    /* 2, if bootfail times > 2, it must be a user sensible bootfail */
    if (pbootfail_log_info->real_recovery_info.record_count > BFM_USER_NOT_SENSIBLE_BOOTFAIL_MAX_COUNT)
    {
        return true;
    }

    /* 3, if the recovery method of bootfail is not REBOOT, it must be user sensible */
    for (i = 0; i < pbootfail_log_info->real_recovery_info.record_count; i++)
    {
        if (FRM_REBOOT != pbootfail_log_info->real_recovery_info.recovery_method_original[i])
        {
            BFMR_PRINT_KEY_INFO("The original recovery method of bootfail [%x] is: %d, not \"FRM_REBOOT\"!, it is user sensible\n",
		pbootfail_log_info->real_recovery_info.boot_fail_no[i],
		pbootfail_log_info->real_recovery_info.recovery_method_original[i]);
            return true;
        }
    }

    /* 4, some time even if the recovery method is FRM_REBOOT, it is also user sensible such as: PRESS10S */
    for (i = 0; i < count; i++)
    {
        for (j = 0; j < pbootfail_log_info->real_recovery_info.record_count; j++)
        {
            if ((unsigned int)s_sensible_bootfail_with_reboot_recovery[i] == pbootfail_log_info->real_recovery_info.boot_fail_no[j])
            {
                BFMR_PRINT_KEY_INFO("The bootfail [%x] is user sensible!\n", pbootfail_log_info->real_recovery_info.boot_fail_no[j]);
                return true;
            }
        }
    }

    /* 5, if the bootfail occurs @ NATIVE/FRAMEWORK stage, it maybe a user sensible bootfail with high probability */
    for (i = 0; i < pbootfail_log_info->real_recovery_info.record_count; i++)
    {
        if (NATIVE_STAGE_START <= pbootfail_log_info->real_recovery_info.boot_fail_stage[i])
        {
            BFMR_PRINT_KEY_INFO("The bootfail [%x] occurs @%x stage, it is user sensible!\n",
                pbootfail_log_info->real_recovery_info.boot_fail_no[i], pbootfail_log_info->real_recovery_info.boot_fail_stage[i]);
            return true;
        }
    }

    /* 6, if the bootfail occurs @ NATIVE/FRAMEWORK stage, it maybe a user sensible bootfail with high probability */
    for (i = 0; i < pbootfail_log_info->real_recovery_info.record_count; i++)
    {
        if (BFM_USER_MAX_TOLERANT_BOOTTIME_IN_SECOND < pbootfail_log_info->real_recovery_info.boot_fail_time[i])
        {
            BFMR_PRINT_KEY_INFO("The bootfail [%x] occurs @%u second, it is user sensible!\n",
                pbootfail_log_info->real_recovery_info.boot_fail_no[i], pbootfail_log_info->real_recovery_info.boot_fail_time[i]);
            return true;
        }
    }

    return false;
}


static void bfm_merge_bootfail_logs(bfm_bootfail_log_info_t *pbootfail_log_info)
{
    int i = 0;
    int j = 0;
    int log_count = (int)(sizeof(s_valid_log_name) / sizeof(s_valid_log_name[0]));
    char *dst_file_path = NULL;
    char *src_file_path = NULL;
    char *pdata = NULL;
    char *pstart = NULL;
    long src_file_len = 0L;
    long offset = 0L;
    long buf_len = 0L;
    char time[BFM_MAX_INT_NUMBER_LEN] = {0};

    if (unlikely(NULL == pbootfail_log_info))
    {
        BFMR_PRINT_INVALID_PARAMS("pbootfail_log_info.\n");
        return;
    }

    if (1 >= pbootfail_log_info->log_dir_count)
    {
        return;
    }

    dst_file_path = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == dst_file_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    src_file_path = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == src_file_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    for (i = 0; i < pbootfail_log_info->log_dir_count; i++)
    {
        for (j = 0; j < log_count; j++)
        {
            memset(dst_file_path, 0, BFMR_MAX_PATH);
            memset(src_file_path, 0, BFMR_MAX_PATH);
            snprintf(dst_file_path, BFMR_MAX_PATH - 1, "%s/%s", pbootfail_log_info->bootfail_logs[0].log_dir, s_valid_log_name[j]);
            snprintf(src_file_path, BFMR_MAX_PATH - 1, "%s/%s", pbootfail_log_info->bootfail_logs[i].log_dir, s_valid_log_name[j]);

            /* continue if the src file doesn't exist */
            if (!bfmr_is_file_existed(src_file_path))
            {
                continue;
            }

            /* get file length of src file */
            src_file_len = bfmr_get_file_length(src_file_path);
            if (src_file_len <= 0L)
            {
                BFMR_PRINT_ERR("the length of [%s] is :%ld\n", src_file_path, src_file_len);
                continue;
            }

            /* allocate mem */
            buf_len = src_file_len + strlen(BFM_TEXT_LOG_SEPRATOR_WITH_FIRST_NEWLINE) + strlen(time) + 1;
            pdata = (char *)bfmr_malloc(buf_len);
            if (NULL == pdata)
            {
                BFMR_PRINT_ERR("bfmr_malloc failed!\n");
                continue;
            }
            memset(pdata, 0, buf_len);

            /* read src file */
            snprintf(time, sizeof(time) - 1, "0x%08x", pbootfail_log_info->real_recovery_info.boot_fail_rtc_time[i]);
            offset = snprintf(pdata, buf_len, BFM_TEXT_LOG_SEPRATOR_WITH_FIRST_NEWLINE, time);
            src_file_len = bfmr_full_read_with_file_path(src_file_path, pdata + offset, src_file_len);
            if (src_file_len <= 0)
            {
                BFMR_PRINT_ERR("read [%s] failed!\n", src_file_path);
                bfmr_free(pdata);
                continue;
            }
            pstart = ((('\n' == pdata[offset + src_file_len - 1]) || (0 == i)) ? (pdata + 1) : (pdata));
            buf_len = ((('\n' == pdata[offset + src_file_len - 1]) || (0 == i)) ? (offset + src_file_len - 1) : (offset + src_file_len));
            bfmr_save_log(pbootfail_log_info->bootfail_logs[0].log_dir, s_valid_log_name[j], pstart, buf_len, (0 == i) ? (0) : 1);
            bfmr_free(pdata);
        }

        if (i > 0)
        {
            bfm_delete_dir(pbootfail_log_info->bootfail_logs[i].log_dir);
        }
    }

__out:
    bfmr_free(dst_file_path);
    bfmr_free(src_file_path);
}


static void bfm_delete_user_unsensible_bootfail_logs(bfm_bootfail_log_info_t *pbootfail_log_info)
{
    int i = 0;

    if (unlikely(NULL == pbootfail_log_info))
    {
        BFMR_PRINT_INVALID_PARAMS("pbootfail_log_info.\n");
        return;
    }

    for (i = 0; i < pbootfail_log_info->log_dir_count; i++)
    {
        bfm_delete_dir(pbootfail_log_info->bootfail_logs[i].log_dir);
    }
}


static int bfm_traverse_log_root_dir(bfm_bootfail_log_info_t *pbootfail_log_info)
{
    int i = 0;
    int fd = -1;
    int num;
    size_t log_idx = 0;
    size_t log_max_count = 0;
    void *buf = NULL;
    char *full_path = NULL;
    struct linux_dirent64 *dirp;
    mm_segment_t oldfs;

    if (unlikely(NULL == pbootfail_log_info))
    {
        BFMR_PRINT_INVALID_PARAMS("pbootfail_log_info.\n");
        return -1;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    fd = sys_open(bfm_get_bfmr_log_root_path(), O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed!\n", bfm_get_bfmr_log_root_path());
        goto __out;
    }

    buf = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    full_path = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    log_max_count = sizeof(pbootfail_log_info->bootfail_logs) / sizeof(pbootfail_log_info->bootfail_logs[0]);
    dirp = buf;
    num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    while (num > 0)
    {
        while ((num > 0) && (log_idx < log_max_count))
        {
            bfm_stat_t st;
            int ret;

            if ((0 == strcmp(dirp->d_name, ".")) || (0 == strcmp(dirp->d_name, ".."))
                || (0 == strcmp(dirp->d_name, BFM_UPLOADING_DIR_NAME)))/*lint !e421 */
            {
                goto __continue;
            }

            memset(pbootfail_log_info->bootfail_logs[log_idx].log_dir, 0, sizeof(pbootfail_log_info->bootfail_logs[log_idx].log_dir));
            snprintf(pbootfail_log_info->bootfail_logs[log_idx].log_dir, sizeof(pbootfail_log_info->bootfail_logs[log_idx].log_dir) - 1,
                "%s/%s", bfm_get_bfmr_log_root_path(), dirp->d_name);
            memset((void *)&st, 0, sizeof(bfm_stat_t));
            ret = bfm_sys_lstat(pbootfail_log_info->bootfail_logs[log_idx].log_dir, &st);
            if ((0 != ret) || (!S_ISDIR(st.st_mode)))
            {
                BFMR_PRINT_ERR("newlstat %s failed or %s isn't a dir!\n", pbootfail_log_info->bootfail_logs[log_idx].log_dir,
                    pbootfail_log_info->bootfail_logs[log_idx].log_dir);
                goto __continue;
            }

            /* check if the log belongs to the last bootfail or not */
            memset(full_path, 0, BFMR_MAX_PATH);
            snprintf(full_path, BFMR_MAX_PATH - 1, "%s/%s", pbootfail_log_info->bootfail_logs[log_idx].log_dir, BFM_DONE_FILE_NAME);
            ret = bfm_sys_lstat(full_path, &st);
            log_idx = (0 == ret) ? (log_idx) : (log_idx + 1);

__continue:
            num -= dirp->d_reclen;
            dirp = (void *)dirp + dirp->d_reclen;
        }

        if (log_idx >= log_max_count)
        {
            BFMR_PRINT_ERR("extent max count: %d!\n", (int)log_max_count);
            break;
        }
        dirp = buf;
        memset(buf, 0, BFMR_MAX_PATH);
        num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    }

    /* save log count */
    pbootfail_log_info->log_dir_count = log_idx;

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }
    set_fs(oldfs);
    bfmr_free(buf);
    bfmr_free(full_path);
    BFMR_PRINT_ERR("There're %d valid bootfail logs:\n", pbootfail_log_info->log_dir_count);
    for (i = 0; i < pbootfail_log_info->log_dir_count; i++)
    {
        BFMR_PRINT_SIMPLE_INFO("%s\n", pbootfail_log_info->bootfail_logs[i].log_dir);
    }

    return 0;
}


static int bfm_update_info_for_each_log(void)
{
    bfm_bootfail_log_info_t *pbootfail_log_info = NULL;

    pbootfail_log_info = (bfm_bootfail_log_info_t *)bfmr_malloc(sizeof(bfm_bootfail_log_info_t));
    if (NULL == pbootfail_log_info)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(pbootfail_log_info, 0, sizeof(bfm_bootfail_log_info_t));

    /* traverse the log dir and save the path of valid bootfail log */
    bfm_traverse_log_root_dir(pbootfail_log_info);

    /* get real recovery info */
    if (0 != bfr_get_real_recovery_info(&(pbootfail_log_info->real_recovery_info)))
    {
        BFMR_PRINT_ERR("get real recovery info failed!\n");
        goto __out;
    }

    /* delete the user unsensible bootfail log */
    if (!bfm_is_user_sensible_boot_fail(pbootfail_log_info))
    {
        BFMR_PRINT_KEY_INFO("[%s] is a user unsensible bootfail log!\n", pbootfail_log_info->bootfail_logs[0].log_dir);
        (void)bfm_delete_user_unsensible_bootfail_logs(pbootfail_log_info);
        goto __out;
    }

    /* update bootfail info for each usefull log */
    bfm_merge_bootfail_logs(pbootfail_log_info);
    bfm_update_platform_logs(pbootfail_log_info);
    (void)bfm_update_bootfail_info_for_each_log(pbootfail_log_info);
    (void)bfm_update_recovery_info_for_each_log(pbootfail_log_info);

__out:
    bfmr_free(pbootfail_log_info);

    return 0;
}


static int bfm_lookup_dir_by_create_time(const char *root,
    char *log_path,
    size_t log_path_len,
    int find_oldest_log)
{
    int fd = -1;
    int num;
    int log_count = 0;
    void *buf = NULL;
    char *full_path = NULL;
    struct linux_dirent64 *dirp;
    mm_segment_t oldfs;
    unsigned long long special_time = 0;
    unsigned long long cur_time = 0;

    if (unlikely(NULL == log_path))
    {
        BFMR_PRINT_INVALID_PARAMS("log_path.\n");
        return -1;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);/*lint !e501 */

    memset((void *)log_path, 0, log_path_len);
    fd = sys_open(root, O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed!\n", root);
        goto __out;
    }

    buf = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    dirp = buf;
    num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    while (num > 0)
    {
        while (num > 0)
        {
            bfm_stat_t st;
            int ret;

            memset(full_path, 0, BFMR_MAX_PATH);
            snprintf(full_path, BFMR_MAX_PATH - 1, "%s/%s", root, dirp->d_name);
            if ((0 == strcmp(dirp->d_name, ".")) || (0 == strcmp(dirp->d_name, "..")))/*lint !e421*/
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                continue;
            }

            memset((void *)&st, 0, sizeof(bfm_stat_t));
            ret = bfm_sys_lstat(full_path, &st);
            if (0 != ret)
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                BFMR_PRINT_ERR("newlstat %s failed!\n", full_path);
                continue;
            }

            if (!S_ISDIR(st.st_mode))
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                BFMR_PRINT_ERR("%s is not a dir!\n", full_path);
                continue;
            }

            /* Note: We must exclude the uploaded dir */
            if (0 == strcmp(dirp->d_name, BFM_UPLOADING_DIR_NAME))
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                BFMR_PRINT_ERR("%s must be excluded!\n", full_path);
                continue;
            }

            cur_time = (unsigned long long)st.st_mtime * BFM_US_PERSECOND + st.st_mtime_nsec / 1000;
            if (0 == special_time)
            {
                strncpy(log_path, full_path, log_path_len - 1);
                special_time = cur_time;
            }
            else
            {
                if (0 != find_oldest_log)
                {
                    if (special_time > cur_time)
                    {
                        strncpy(log_path, full_path, log_path_len - 1);
                        special_time = cur_time;
                    }
                }
                else
                {
                    if (special_time < cur_time)
                    {
                        strncpy(log_path, full_path, log_path_len - 1);
                        special_time = cur_time;
                    }
                }
            }
            log_count++;
            num -= dirp->d_reclen;
            dirp = (void *)dirp + dirp->d_reclen;
        }
        dirp = buf;
        memset(buf, 0, BFMR_MAX_PATH);
        num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }
    set_fs(oldfs);

    BFMR_PRINT_KEY_INFO("Find %d log in %s, the %s log path is: %s\n", log_count, root,
        (0 == find_oldest_log) ? ("newest") : ("oldest"), log_path);

    bfmr_free(buf);
    bfmr_free(full_path);

    return 0;
}


static int bfm_find_newest_log(char *log_path, size_t log_path_len)
{
    int ret = -1;

    if (unlikely((NULL == log_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("log_path.\n");
        return -1;
    }

    ret = bfm_lookup_dir_by_create_time(bfm_get_bfmr_log_root_path(), log_path, log_path_len, 0);
    if (0 == strlen(log_path))
    {
        return -1;
    }
    else
    {
        BFMR_PRINT_ERR("The newest log is: %s\n", log_path);
    }

    return 0;
}


static int bfm_notify_boot_success(void *param)
{
    mutex_lock(&s_process_boot_fail_mutex);
    if (s_is_process_boot_success)
    {
        mutex_unlock(&s_process_boot_fail_mutex);
        BFMR_PRINT_ERR("s_is_process_boot_success has been set already!\n");
        return 0;
    }

    /* 1. notify boot success event to the BFR */
    boot_status_notify(1);
    /* 2. let platfrom process the boot success */
    bfm_platform_process_boot_success();

    /* 3. update recovery result in recovery_info.txt */
    bfm_update_info_for_each_log();
    s_is_process_boot_success = true;
    mutex_unlock(&s_process_boot_fail_mutex);

    /* 4. check bootup slowly event */
    unsigned int bootup_time = bfmr_get_bootup_time();
    if ((bootup_time >= BFM_BOOTUP_SLOWLY_THRESHOLD_IN_SECOND) && (!s_is_bootup_successfully))
    {
        BFMR_PRINT_ERR("bootup time[%dS] is too long.\n", bootup_time);
        boot_fail_err(BOOTUP_SLOWLY, DO_NOTHING, NULL);
    }
    else
    {
        //once if boot success, free boot stage info resource
        bfm_deinit_stage_info();
    }

    s_is_bootup_successfully = true;

    return 0;
}


/**
    @function: int bfmr_get_timer_state(int *state)
    @brief: get state of the timer.

    @param: state [out], the state of the boot timer.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
        2. if *state == 0, the boot timer is disabled, if *state == 1, the boot timer is enbaled.
*/
int bfmr_get_timer_state(int *state)
{
    return bfm_get_boot_timer_state(state);
}


/**
    @function: int bfm_enable_timer(void)
    @brief: enbale timer.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_enable_timer(void)
{
    return bfmr_resume_boot_timer();
}


/**
    @function: int bfm_disable_timer(void)
    @brief: disable timer.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_disable_timer(void)
{
    return bfm_suspend_boot_timer();
}


/**
    @function: int bfm_set_timer_timeout_value(unsigned int timeout)
    @brief: set timeout value of the kernel timer. Note: the timer which control the boot procedure is in the kernel.

    @param: timeout [in] timeout value (unit: msec).

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_set_timer_timeout_value(unsigned int timeout)
{
    return bfm_set_boot_timer_timeout_value(timeout);
}


/**
    @function: int bfm_get_timer_timeout_value(unsigned int *timeout)
    @brief: get timeout value of the kernel timer. Note: the timer which control the boot procedure is in the kernel.

    @param: timeout [in] buffer will store the timeout value (unit: msec).

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_get_timer_timeout_value(unsigned int *timeout)
{
    return bfm_get_boot_timer_timeout_value(timeout);
}


/**
    @function: int bfmr_action_timer_ctl(struct action_ioctl_data *pact_data)
    @brief: acttion timers opertion

    @param: pact_data  ationt ctl data

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
static int bfmr_action_timer_ctl(struct action_ioctl_data *pact_data)
{
    int ret = -EPERM;
    switch (pact_data->op)
    {
    case ACT_TIMER_START:
        {
            ret = bfm_action_timer_start(pact_data->action_name, pact_data->action_timer_timeout_value);
            break;
        }
    case ACT_TIMER_STOP:
        {
            ret = bfm_action_timer_stop(pact_data->action_name);
            break;
        }
    case ACT_TIMER_PAUSE:
        {
            ret = bfm_action_timer_pause(pact_data->action_name);
            break;
        }
    case ACT_TIMER_RESUME:
        {
            ret = bfm_action_timer_resume(pact_data->action_name);
            break;
        }
    default:
        {
            break;
        }
    }

    return ret;
}

static unsigned long long bfm_get_system_time(void)
{
    struct timeval tv = {0};

    do_gettimeofday(&tv);

    return (unsigned long long)tv.tv_sec;
}


/**
    @function: int bfmr_capture_and_save_log(bfmr_log_type_e type, bfmr_log_dst_t *dst)
    @brief: capture and save log, initialised in core.

    @param: src [in] log src info.
    @param: dst [in] infomation about the media of saving log.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in bootloader and kernel.
*/
static int bfmr_capture_and_save_log(bfmr_log_src_t *src, bfmr_log_dst_t *dst, bfm_process_bootfail_param_t *pparam)
{
    int i = 0;
    bool is_valid_log_type = false;
    int count = sizeof(s_log_type_buffer_info) / sizeof(s_log_type_buffer_info[0]);
    unsigned int bytes_read = 0;

    if (unlikely((NULL == src) || (NULL == dst) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("src or dst or pparam.\n");
        return -1;
    }

    for (i = 0; i < count; i++)
    {
        if (src->log_type != s_log_type_buffer_info[i].log_type)
        {
            continue;
        }

        s_log_type_buffer_info[i].buf = bfmr_malloc(s_log_type_buffer_info[i].buf_len + 1);
        if (NULL == s_log_type_buffer_info[i].buf)
        {
            BFMR_PRINT_ERR("bfmr_malloc failed!\n");
            return -1;
        }
        memset(s_log_type_buffer_info[i].buf, 0, s_log_type_buffer_info[i].buf_len + 1);
        is_valid_log_type = true;
        break;
    }

    if (!is_valid_log_type)
    {
        BFMR_PRINT_ERR("Invalid log type: [%d]\n", (int)(src->log_type));
        return -1;
    }

    src->save_log_after_reboot = pparam->save_log_after_reboot;
    bytes_read = bfmr_capture_log_from_system(s_log_type_buffer_info[i].buf,
        s_log_type_buffer_info[i].buf_len, src, 0);/*lint !e661 */
    if (bfmr_is_oversea_commercail_version())
    {
        BFMR_PRINT_ERR("Note: logs of oversea commercail version can't be saved!\n");
    }
    else
    {
        switch (dst->type)
        {
        case DST_FILE:
            {
                bfmr_save_log_to_fs(dst->dst_info.filename, s_log_type_buffer_info[i].buf, bytes_read, 0);/*lint !e661 */
                break;
            }
        case DST_RAW_PART:
            {
                if (dst->dst_info.raw_part.offset >=0)
                {
                    bfmr_save_log_to_raw_part(dst->dst_info.raw_part.raw_part_name,
                        (unsigned long long)dst->dst_info.raw_part.offset,
                        s_log_type_buffer_info[i].buf, bytes_read);/*lint !e661 */
                    bfmr_update_raw_log_info(src, dst, bytes_read);
                }
                else
                {
                    BFMR_PRINT_ERR("dst->dst_info.raw_part.offset is negative [%d]\n", dst->dst_info.raw_part.offset);
                }
                break;
            }
        case DST_MEMORY_BUFFER:
        default:
            {
                bfmr_save_log_to_mem_buffer(dst->dst_info.buffer.addr, dst->dst_info.buffer.len, s_log_type_buffer_info[i].buf, bytes_read);/*lint !e661 */
                break;
            }
        }
    }


    bfmr_free(s_log_type_buffer_info[i].buf);
    s_log_type_buffer_info[i].buf = NULL;

    return 0;
}


static long bfmr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned int timeout_value = -1;
    bfmr_detail_boot_stage_e boot_stage;
    struct bfmr_dev_path *dev_path = NULL;
    struct bfmr_boot_fail_info *pboot_fail_info = NULL;
    struct action_ioctl_data *pact_data = NULL;

    if (!bfmr_has_been_enabled())
    {
        BFMR_PRINT_KEY_INFO("BFMR is disabled!\n");
        return -EPERM;
    }

    if (NULL == (void *)arg)
    {
        BFMR_PRINT_INVALID_PARAMS("arg.\n");
        return -EINVAL;
    }

    switch (cmd)
    {
    case BFMR_GET_TIMER_STATUS:
        {
            int state;

            bfmr_get_timer_state(&state);
            if (0 != copy_to_user((int *)arg, &state, sizeof(state)))
            {
                BFMR_PRINT_ERR("copy_to_user failed!\n");
                ret = -EFAULT;
                break;
            }
            BFMR_PRINT_KEY_INFO("short timer stats is: %d\n", state);
            break;
        }
    case BFMR_ENABLE_TIMER:
        {
            bfmr_enable_timer();
            break;
        }
    case BFMR_DISABLE_TIMER:
        {
            bfmr_disable_timer();
            break;
        }
    case BFMR_GET_TIMER_TIMEOUT_VALUE:
        {
            bfmr_get_timer_timeout_value(&timeout_value);
            if (0 != copy_to_user((int *)arg, &timeout_value, sizeof(timeout_value)))
            {
                BFMR_PRINT_ERR("copy_to_user failed!\n");
                ret = -EFAULT;
                break;
            }
            BFMR_PRINT_KEY_INFO("short timer timeout value is: %d\n", timeout_value);
            break;
        }
    case BFMR_SET_TIMER_TIMEOUT_VALUE:
        {
            if (0 != copy_from_user(&timeout_value, (int *)arg, sizeof(timeout_value)))
            {
                BFMR_PRINT_ERR("copy_from_user failed!\n");
                ret = -EFAULT;
                break;
            }
            bfmr_set_timer_timeout_value(timeout_value);
            BFMR_PRINT_KEY_INFO("set short timer timeout value to: %d\n", timeout_value);
            break;
        }
    case BFMR_GET_BOOT_STAGE:
        {
            bfmr_get_boot_stage(&boot_stage);
            if (0 != copy_to_user((bfmr_detail_boot_stage_e *)arg, &boot_stage, sizeof(boot_stage)))
            {
                BFMR_PRINT_ERR("copy_to_user failed!\n");
                ret = -EFAULT;
                break;
            }
            BFMR_PRINT_KEY_INFO("bfmr_bootstage is: 0x%08x\n", (unsigned int)boot_stage);
            break;
        }
    case BFMR_SET_BOOT_STAGE:
        {
            bfmr_detail_boot_stage_e old_boot_stage;

            bfmr_get_boot_stage(&old_boot_stage);
            if (0 != copy_from_user(&boot_stage, (int *)arg, sizeof(boot_stage)))
            {
                BFMR_PRINT_ERR("copy_from_user failed!\n");
                ret = -EFAULT;
                break;
            }
            BFMR_PRINT_KEY_INFO("set bfmr_bootstage to: 0x%08x\n", (unsigned int)boot_stage);
            bfmr_set_boot_stage(boot_stage);

            break;
        }
    case BFMR_PROCESS_BOOT_FAIL:
        {
            pboot_fail_info = (struct bfmr_boot_fail_info *)bfmr_malloc(sizeof(struct bfmr_boot_fail_info));
            if (NULL == pboot_fail_info)
            {
                BFMR_PRINT_ERR("bfmr_malloc failed!\n");
                ret = -ENOMEM;
                break;
            }
            memset((void *)pboot_fail_info, 0, sizeof(struct bfmr_boot_fail_info));
            if (0 != copy_from_user(pboot_fail_info, ((struct bfmr_boot_fail_info *)arg), sizeof(struct bfmr_boot_fail_info)))
            {
                BFMR_PRINT_ERR("copy_from_user failed!\n");
                bfmr_free(pboot_fail_info);
                ret = -EFAULT;
                break;
            }

            pboot_fail_info->addl_info.log_path[sizeof(pboot_fail_info->addl_info.log_path) - 1] = '\0';
            BFMR_PRINT_KEY_INFO("bootfail_errno: 0x%08x, suggested_recovery_method: %d, log_file [%s]'s lenth:%ld\n",
                (unsigned int)pboot_fail_info->boot_fail_no, (int)pboot_fail_info->suggested_recovery_method,
                pboot_fail_info->addl_info.log_path, bfmr_get_file_length(pboot_fail_info->addl_info.log_path));
            (void)boot_fail_err(pboot_fail_info->boot_fail_no, pboot_fail_info->suggested_recovery_method, &pboot_fail_info->addl_info);
            bfmr_free(pboot_fail_info);
            break;
        }
    case BFMR_GET_DEV_PATH:
        {
            break;
        }
    case BFMR_ENABLE_CTRL:
        {
            int bfmr_enbale_flag;
            if (0 != copy_from_user(&bfmr_enbale_flag, (int *)arg, sizeof(int)))
            {
                BFMR_PRINT_KEY_INFO("Failed to copy bfmr enable flag from user!\n");
                ret = -EFAULT;
                break;
            }
            BFMR_PRINT_KEY_INFO("set bfmr enable flag: 0x%08x\n", (unsigned int)bfmr_enbale_flag);
            bfmr_enable_ctl(bfmr_enbale_flag);  
            break;
        }
    case BFMR_ACTION_TIMER_CTL:
        {
            pact_data = (struct action_ioctl_data *)bfmr_malloc(sizeof(struct action_ioctl_data));
            if (NULL == pact_data) {
                BFMR_PRINT_ERR("bfmr_malloc failed!\n");
                ret = -ENOMEM;
                break;
            }

            if (0 != copy_from_user(pact_data, arg, sizeof(struct action_ioctl_data))) {
                BFMR_PRINT_KEY_INFO("Failed to copy acttion_ioctl_data from user buffer!\n");
                bfmr_free(pact_data);
                ret = -EFAULT;
                break;
            }

            pact_data->action_name[BFMR_ACTION_NAME_LEN-1] = '\0';
            BFMR_PRINT_KEY_INFO("set bfmr action timer: %x, %s, %d\n",
                       pact_data->op, pact_data->action_name, pact_data->action_timer_timeout_value);
            ret = bfmr_action_timer_ctl(pact_data);
            bfmr_free(pact_data);
            break;
        }
    default:
        {
            BFMR_PRINT_ERR("Invalid CMD: 0x%x\n", cmd);
            ret = -EFAULT;
            break;
        }
    }

    return ret;
}


static int bfmr_open(struct inode *inode, struct file *file)
{
    return nonseekable_open(inode, file);
}


static ssize_t bfmr_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
    return len;
}


static ssize_t bfmr_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    return count;
}


static int bfmr_release(struct inode *inode, struct file *file)
{
    return 0;
}


static int bfm_process_bottom_layer_boot_fail(void *param)
{
    bfmr_log_src_t src;
    int i = 0;
    int ret = -1;
    int count = sizeof(s_log_type_buffer_info) / sizeof(s_log_type_buffer_info[0]);
    unsigned int bytes_read = 0U;
    bool find_log_type_buffer_info = false;

    mutex_lock(&s_process_boot_fail_mutex);
    memset((void *)&src, 0, sizeof(src));
    src.log_type = LOG_TYPE_BFMR_TEMP_BUF;
    for (i = 0; i < count; i++)
    {
        if (LOG_TYPE_BFMR_TEMP_BUF != s_log_type_buffer_info[i].log_type)
        {
            continue;
        }

        /* update the buf_len firstly */
        s_log_type_buffer_info[i].buf_len = bfm_get_dfx_log_length();
        s_log_type_buffer_info[i].buf = bfmr_malloc(s_log_type_buffer_info[i].buf_len + 1);
        if (NULL == s_log_type_buffer_info[i].buf)
        {
            BFMR_PRINT_ERR("bfmr_malloc failed!\n");
            goto __out;
        }
        memset(s_log_type_buffer_info[i].buf, 0, s_log_type_buffer_info[i].buf_len + 1);
        find_log_type_buffer_info = true;
        break;
    }
    if (!find_log_type_buffer_info)
    {
        BFMR_PRINT_ERR("Can not find log buffer info for \"LOG_TYPE_BFMR_TEMP_BUF\"\n");
        goto __out;
    }

    bytes_read = bfmr_capture_log_from_system(s_log_type_buffer_info[i].buf,
        s_log_type_buffer_info[i].buf_len, &src, 0);/*lint !e661*/
    if (0U == bytes_read)
    {
        ret = 0;
        BFMR_PRINT_ERR("There is no bottom layer bootfail log!\n");
        goto __out;
    }

    ret = bfm_parse_and_save_bottom_layer_bootfail_log(
        (bfm_process_bootfail_param_t *)param,
        s_log_type_buffer_info[i].buf,
        s_log_type_buffer_info[i].buf_len);/*lint !e661 */
    if (0 != ret)
    {
        BFMR_PRINT_ERR("Failed to save bottom layer bootfail log!\n");
        goto __out;
    }

__out:
    if (i < count)
    {
        if(NULL != s_log_type_buffer_info[i].buf)
        {
            bfmr_free(s_log_type_buffer_info[i].buf);
            s_log_type_buffer_info[i].buf = NULL;
        }
    }
    bfmr_free(param);
    mutex_unlock(&s_process_boot_fail_mutex);

    return ret;
}


static int bfmr_capture_and_save_bottom_layer_boot_fail_log(void)
{
    struct task_struct *tsk;
    bfm_process_bootfail_param_t *pparam = NULL;

    pparam = (bfm_process_bootfail_param_t *)bfmr_malloc(sizeof(bfm_process_bootfail_param_t));
    if (NULL == pparam)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -1;
    }
    memset((void *)pparam, 0, sizeof(bfm_process_bootfail_param_t));

    tsk = kthread_run(bfm_process_bottom_layer_boot_fail, (void *)pparam, "save_bl_bf_log");
    if (IS_ERR(tsk))
    {
        BFMR_PRINT_ERR("Failed to create thread to save bottom layer bootfail log!\n");
        return -1;
    }

    return 0;
}


/* this function can not be invoked in interrupt context */
static int bfm_process_ocp_excp(bfmr_hardware_fault_type_e fault_type, ocp_excp_info_t *pexcp_info)
{
    bfmr_bootfail_addl_info_t *paddl_info = NULL;
    bfmr_get_hw_fault_info_param_t *pfault_info_param = NULL;
    int fault_times = 0;

    if (unlikely(NULL == pexcp_info))
    {
        BFMR_PRINT_INVALID_PARAMS("pexcp_info.\n");
        return -1;
    }

    paddl_info = (bfmr_bootfail_addl_info_t *)bfmr_malloc(sizeof(bfmr_bootfail_addl_info_t));
    if (NULL == paddl_info)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(paddl_info, 0, sizeof(bfmr_bootfail_addl_info_t));

    pfault_info_param = (bfmr_get_hw_fault_info_param_t *)bfmr_malloc(sizeof(bfmr_get_hw_fault_info_param_t));
    if (NULL == pfault_info_param)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pfault_info_param, 0, sizeof(bfmr_get_hw_fault_info_param_t));

    pfault_info_param->fault_stage = HW_FAULT_STAGE_AFTER_BOOT_SUCCESS;
    memcpy(&pfault_info_param->hw_excp_info.ocp_excp_info, pexcp_info, sizeof(ocp_excp_info_t));
    memcpy(paddl_info->detail_info, pexcp_info->ldo_num, BFMR_MIN(sizeof(paddl_info->detail_info) - 1, strlen(pexcp_info->ldo_num)));
    paddl_info->hardware_fault_type = HW_FAULT_OCP;
    if (s_is_bootup_successfully)
    {
        mutex_lock(&s_process_boot_fail_mutex);
        fault_times = bfr_get_hardware_fault_times(pfault_info_param);
        mutex_unlock(&s_process_boot_fail_mutex);
        if (0 == fault_times)
        {
            boot_fail_err(BFM_HARDWARE_FAULT, DO_NOTHING, paddl_info);
        }
        else
        {
            BFMR_PRINT_KEY_INFO("BFM has processed OCP after boot-success one time!\n");
        }

        goto __out;
    }

    pfault_info_param->fault_stage = HW_FAULT_STAGE_DURING_BOOTUP;
    mutex_lock(&s_process_boot_fail_mutex);
    fault_times = bfr_get_hardware_fault_times(pfault_info_param);
    mutex_unlock(&s_process_boot_fail_mutex);
    switch (fault_times)
    {
    case 0:
        {
            BFMR_PRINT_KEY_INFO("%s has ocp once!\n", pexcp_info->ldo_num);
            boot_fail_err(BFM_HARDWARE_FAULT, NO_SUGGESTION, paddl_info);
            break;
        }
    case 1:
        {
            BFMR_PRINT_KEY_INFO("%s has ocp twice!\n", pexcp_info->ldo_num);
            boot_fail_err(BFM_HARDWARE_FAULT, DO_NOTHING, paddl_info);
            break;
        }
    default:
        {
            BFMR_PRINT_KEY_INFO("BFM has processed the same OCP during bootup!\n");
            break;
        }
    }

__out:
    bfmr_free(paddl_info);
    bfmr_free(pfault_info_param);

    return 0;
}


/**
    @function: int bfm_init(void)
    @brief: init bfm module in kernel.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in bootloader and kernel.
*/
int bfm_init(void)
{
    int ret = -1;
    bfm_chipsets_init_param_t chipsets_init_param;

    if (!bfmr_has_been_enabled())
    {
        BFMR_PRINT_KEY_INFO("BFMR is disabled!\n");
        return 0;
    }

    ret = misc_register(&s_bfmr_miscdev);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("misc_register failed, ret: %d.\n", ret);
        return ret;
    }

    (void)bfm_init_boot_timer();
    (void)bfm_init_stage_info();
    memset((void *)&chipsets_init_param, 0, sizeof(bfm_chipsets_init_param_t));
    chipsets_init_param.log_saving_param.capture_and_save_bootfail_log = bfm_capture_and_save_bootfail_log;
    chipsets_init_param.process_ocp_excp = bfm_process_ocp_excp;
    bfm_chipsets_init(&chipsets_init_param);
    bfmr_capture_and_save_bottom_layer_boot_fail_log();

    return 0;
}

