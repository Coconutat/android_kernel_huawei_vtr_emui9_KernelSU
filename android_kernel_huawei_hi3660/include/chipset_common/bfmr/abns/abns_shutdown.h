

/*define boot stage in kernel*/
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/rtc.h>
#include <linux/statfs.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/common/bfmr_common.h>
#include <chipset_common/bfmr/bfm/chipsets/bfm_chipsets.h>
#include <chipset_common/bfmr/bfm/core/bfm_core.h>

#define ABNS_INFO_MAGIC           0xABABBABA
#define BOOT_FLAG_MANUAL_SHUTDOWN 0x5A5AFFFF


#define BOOT_FLAG_BL1      0xA1A5FFFF
#define BOOT_FLAG_BL2      0xA2A5FFFF
#define BOOT_FLAG_KERNEL   0xA3A5FFFF
#define BOOT_FLAG_NATIVE   0xA4A5FFFF
#define BOOT_FLAG_FRAMWORK 0xA5A5FFFF
#define BOOT_FLAG_SUCC     0xA8A5FFFF

#define ABNS_TRUE 1
#define ABNS_FLASE 0


#define ABNS_PARAM_STR_LEN 32
#define TIME_STR_LEN 32
#define ABNS_LOG_PATH_LEN 256
#define ABNS_LOG_MAX_COUNT BFM_LOG_MAX_COUNT
#define ABNS_LOG_ROOT_PATH     "/splash2/abns_shutdown"
#define ABNS_HISTORY_FILE_NAME "abns_history.txt"
#define ABNS_WAIT_FOR_LOG_PART_TIMEOUT 180
#define ABNS_INFO_FILE_NAME    "abns_info.txt"
#define ABNS_A_LOG_DIR_NAME_FORMAT "%s/abns_%s"
#define ABNS_A_LOG_DIR_NAME_LEN 19
#define MAXSIZES_PATH 256


#define ABNS_A_LOG_FASTBOOT      "fastboot_log"
#define ABNS_A_LOG_LAST_FASTBOOT "last_fastboot_log"
#define ABNS_A_LOG_KMEGSLOG      "kmsgcat-log"

#define HISI_RSV_PART_NAME_STR "/dev/block/bootdevice/by-name/reserved2"

#define ABNS_TYPE_CMDLINE_STR    "abns_type"
#define ABNS_SWTYPE_CMDLINE_STR  "androidboot.swtype"
#define ABNS_RUNMODE_CMDLINE_STR "androidboot.mode"
#define ABNS_VERIFIEDMD_CMDLINE_STR "androidboot.verifiedbootstate"
#define ABNS_UNKNOWN_STR          "UNKNOWN"

#define fastboot_log_path        "/proc/balong/log/fastboot_log"
#define last_fastboot_log_path   "/proc/balong/log/last_fastboot_log"
#define kmesglog_path            "/data/log/android_logs/kmsgcat-log"


#define ABNS_INFO_FILE_CONTENT_FORMAT \
    "time           :%s\r\n" \
    "abns_magic     :0x%X\r\n" \
    "abns_stage     :0x%X\r\n" \
    "abns_tpye      :%d\r\n" \
    "abns_tpye_name :%s\r\n" \
    "sw_mode        :%s\r\n" \
    "run_mode       :%s\r\n" \
    "verifiedbootmod:%s\r\n"


#define ABNS_PRINT_ERR BFMR_PRINT_ERR
#define ABNS_PRINT_KEY_INFO BFMR_PRINT_KEY_INFO
#define ABNS_PRINT_INVALID_PARAMS BFMR_PRINT_INVALID_PARAMS
#define ABNS_PRINT_ENTER BFMR_PRINT_ENTER
#define ABNS_PRINT_EXIT BFMR_PRINT_EXIT

#define abns_write_emmc_raw_part bfmr_write_emmc_raw_part
#define abns_read_emmc_raw_part bfmr_read_emmc_raw_part
#define abns_create_dir bfmr_create_dir
#define abns_delete_dir bfm_delete_dir
#define abns_create_log_path bfmr_create_log_path
#define abns_capture_log_from_src_file bfmr_capture_log_from_src_file
#define abns_get_proc_file_length bfmr_get_proc_file_length
#define abns_get_file_length bfmr_get_file_length
#define abns_save_log_file bfmr_save_log
#define abns_get_log_count bfm_get_log_count
#define abns_wait_for_part_mount_with_timeout bfmr_wait_for_part_mount_with_timeout
#define abns_get_bfmr_log_part_mount_point bfm_get_bfmr_log_part_mount_point



/*Define abnormal shutdown type in kernel*/
typedef enum
{
    ABNS_NORMAL = 0,
    ABNS_PRESS10S,
    ABNS_LTPOWEROFF,
    ABNS_PMU_EXP,
    ABNS_PMU_SMPL,//statiblity issue log has save by reboot_reason.c
    ABNS_UNKOWN,
    ABNS_BAT_LOW,
    ABNS_BAT_TEMP_HIGH,
    ABNS_BAT_TEMP_LOW,
    ABNS_CHIPSETS_TEMP_HIGH,
} ABNS_ERR_TYPE;


struct abns_word{
    unsigned char name[ABNS_PARAM_STR_LEN];
    unsigned int num;
};

typedef struct abns_boot_flag{
    unsigned int magic;
    unsigned int boot_flag;
}ABNS_BTFG_T;


/*Define ABNS_INFO struct*/
typedef struct {
    unsigned char  asctime[TIME_STR_LEN];  //current android time
    unsigned int   abns_magic;
    unsigned int   abns_stage;
    unsigned int   abns_tpye;
    unsigned char  abns_tpye_name[ABNS_PARAM_STR_LEN];
    unsigned char* sw_mode; //normal or recovery
    unsigned char* run_mode; //normal or recovery
    unsigned char* verifiedbootmod;
    unsigned int   is_info_valid;
    struct work_struct abns_work;
} ABNS_INFO_T;


#define BOOT_FLAG_FAIL 0x5A5A5A5A
#define BOOT_FLAG_OK 0xA5A5A5A5

int bfmr_set_abns_flag(unsigned int abns_flag);
int bfmr_clean_abns_flag(unsigned int abns_flag);

