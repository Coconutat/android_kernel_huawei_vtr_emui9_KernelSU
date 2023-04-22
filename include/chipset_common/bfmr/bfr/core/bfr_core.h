/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfr_core.h

    @brief: define the core external public enum/macros/interface for BFR (Boot Fail Recovery)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

#ifndef BFR_CORE_H
#define BFR_CORE_H


/*----includes-----------------------------------------------------------------------*/

#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/common/bfmr_common.h>


/*----c++ support--------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif


/*----export prototypes---------------------------------------------------------------*/

typedef enum bfr_recovery_method
{
    /*
    * in case that the boot fail has been processed in frmawork layer, the BFMR should
    * set the suggested recovery method as BFR_DO_NOTHING when invoke function
    * try_to_recovery
    */
    FRM_DO_NOTHING = 0,

    /*
    * this is the basic recovery method for the most boot fail
    */
    FRM_REBOOT,

    /*
    * if the phone has A/B system and the B system is valid, when A system can not boot successfully
    * the BFR will use B system to do bootup
    */
    FRM_GOTO_B_SYSTEM,

    /*
    * this recovery method is used for the native and framework normal boot fail when
    * the reboot action can not recovery the boot fail.
    */
    FRM_GOTO_ERECOVERY_DEL_FILES_FOR_BF,

    /*
    * this recovery method is used when the available space of data partition is 
    * below the threshold, which will resulting in boot fail
    */
    FRM_GOTO_ERECOVERY_DEL_FILES_FOR_NOSPC,

    /*
    * if deleting files in data can not recovery the boot fail, the further recovery
    * method is suggesting the user to do factory data reset.
    */
    FRM_GOTO_ERECOVERY_FACTORY_RESET,

    /*
    * this recovery method is used for the extreme case that the action of
    * mounting data partirion will result in panic in kernel
    */
    FRM_GOTO_ERECOVERY_FORMAT_DATA_PART,

    /* download the latest OTA full package to do recovery */
    FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY,

    /*
    * download the latest OTA full package to do recovery
    * and in the end delete the key files in data partition
    */
    FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_DEL_FILES,

    /*
    * download the latest OTA full package to do recovery
    * and in the end suggest the user to do factory reset
    */
    FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_FACTORY_RESET,

    /* if the recovery can not bootup successfully for ever, if we should notify the user in time*/
    FRM_NOTIFY_USER_RECOVERY_FAILURE,

    FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA,

    FRM_ENTER_SAFE_MODE,

    FRM_FACTORY_RESET_AFTER_DOWNLOAD_RECOVERY,

    FRM_RECOVERY_METHOD_MAX_COUNT
} bfr_recovery_method_e;

typedef enum SUGGESTED_RECOVERY_METHOD
{
    /*
    * if you can not determine the recovery method for a boot fail, transfer
    * NO_SUGGESTION to the BFR.
    */
    NO_SUGGESTION = 0x0,
    DO_NOTHING = 0x1,
} bfr_suggested_recovery_method_e;

typedef struct bfr_fixed_recovery_method_param
{
    /* 0 - disable; 1 - enable */
    int enable_this_method;
    bfr_recovery_method_e recovery_method;
} bfr_fixed_recovery_method_param_t;

typedef struct bfr_recovery_policy
{
    bfmr_bootfail_errno_e boot_fail_no;

    /* if the boot fail no has fixed recovery method. 1 - has one, 0 - has none */
    int has_fixed_policy;
    bfr_fixed_recovery_method_param_t param[4];
} bfr_recovery_policy_e;

typedef enum bfr_enter_erecovery_reason
{
    /* enter erecovery because of boot fail  */
    EER_BOOT_FAIL_SOLUTION = 0,

    /* enter erecovery because of user pressing the VolumeUp key when power on the phone */
    EER_PRESS_VOLUMEUP_KEY,

    /* enter erecovery because of user pressing the VolumeUp key when verify failed */
    EER_VERIFY_BOOT,

    EER_MAX_COUNT
} bfr_enter_erecovery_reason_e;

typedef enum bfr_boot_fail_stage
{
    /* boot fail occurs on bootloader stage */
    BFS_BOOTLOADER = 0,

    /* boot fail occurs on kernel stage */
    BFS_KERNEL,

    /* boot fail occurs on native or framework stage */
    BFS_APP,
} bfr_boot_fail_stage_e;

typedef struct bfr_boot_fail_no_range
{
    unsigned int start;
    unsigned int end;
} bfr_boot_fail_no_range_t;

typedef struct bfr_enter_erecovery_reason_map
{
    bfr_boot_fail_no_range_t range;

    /* this will be used by erecovery to communicate with BI server */
    unsigned int enter_erecovery_reason;
} bfr_enter_erecovery_reason_map_t;

typedef enum bfr_recovery_method_running_status
{
    RMRSC_EXEC_COMPLETED = 0,
    RMRSC_ERECOVERY_BOOT_FAILED = 1,
    RMRSC_ERECOVERY_BOOT_SUCCESS,
    RMRSC_ERECOVERY_START_DEL_FILES,
    RMRSC_ERECOVERY_DEL_FILES_SUCCESS,
    RMRSC_ERECOVERY_DEL_FILES_FAILED,
    RMRSC_ERECOVERY_START_WIFI,
    RMRSC_ERECOVERY_START_WIFI_SUCCESS,
    RMRSC_ERECOVERY_START_WIFI_FAILED,
    RMRSC_ERECOVERY_SATRT_QUERY_PACKAGES,
    RMRSC_ERECOVERY_QUERY_PACKAGES_SUCCESS,
    RMRSC_ERECOVERY_QUERY_PACKAGES_FAILED,
    RMRSC_ERECOVERY_START_DOWNLOAD_PACKAGES,
    RMRSC_ERECOVERY_DOWNLOAD_PACKAGES_SUCCESS,
    RMRSC_ERECOVERY_DOWNLOAD_PACKAGES_FAILED,
    RMRSC_ERECOVERY_START_DOWNLOAD_UPDATE_AUTH_FILES,
    RMRSC_ERECOVERY_DOWNLOAD_UPDATE_AUTH_FILES_SUCCESS,
    RMRSC_ERECOVERY_DOWNLOAD_UPDATE_AUTH_FILES_FAILED,
    RMRSC_ERECOVERY_START_INSTALL_PACKAGES,
    RMRSC_ERECOVERY_INSTALL_PACKAGES_SUCCESS,
    RMRSC_ERECOVERY_INSTALL_PACKAGES_FAILED,
    RMRSC_ERECOVERY_MOUNT_USERDATA_FAILED,
    RMRSC_ERECOVERY_MOUNT_USERDATA_SUCCESS,
} bfr_recovery_method_running_status_e;

typedef enum bfr_recovery_method_run_result
{
    RMRR_SUCCESS = 0,
    RMRR_FAILED,
} bfr_recovery_method_run_result_e;

typedef enum bfr_boot_fail_recovery_result
{
    BOOT_FAIL_RECOVERY_SUCCESS = ((unsigned int)0xA5A5A5A5),
    BOOT_FAIL_RECOVERY_FAILURE = ((unsigned int)0x5A5A5A5A),
} bfr_boot_fail_recovery_result_e;

typedef struct bfr_recovery_record_header
{
    unsigned int crc32;
    unsigned int magic_number;
    int boot_fail_count;
    int last_record_idx;
    int next_record_idx;
    int record_count; /* record total count can be saved in the rrecord partition*/
    int record_count_before_boot_success; /* this field shouid be set as 0 when boot success */
    unsigned int safe_mode_enable_flag; /* if this fiels is 0x656e736d means safe mode has been enabled */
    char reserved[96];
} bfr_recovery_record_header_t;

typedef struct bfr_recovery_record
{
    unsigned long long boot_fail_detected_time;
    unsigned int boot_fail_stage;
    unsigned int boot_fail_no;
    unsigned int recovery_method;
    bfr_recovery_method_running_status_e running_status_code;
    bfr_recovery_method_run_result_e method_run_result;
    bfr_boot_fail_recovery_result_e recovery_result;
    unsigned int factory_reset_flag;
    unsigned int recovery_method_original;
    unsigned int boot_fail_time; /* boot time when bootfail happened */
    char bootfail_detail[16];
    char reserved[68];
} bfr_recovery_record_t;

typedef struct bfr_recovery_record_param
{
    unsigned char *buf;
    unsigned int buf_size;
    unsigned int part_offset;
    int record_damaged; /* 0 - good, 1 - damaged */
    int record_count_before_boot_success; /* this field shouid be set as 0 when boot success */
} bfr_recovery_record_param_t;

typedef struct
{
    unsigned int boot_fail_rtc_time[BFM_LOG_MAX_COUNT];
    unsigned int boot_fail_time[BFM_LOG_MAX_COUNT];
    unsigned int boot_fail_no[BFM_LOG_MAX_COUNT];
    unsigned int boot_fail_stage[BFM_LOG_MAX_COUNT];
    unsigned int recovery_method[BFM_LOG_MAX_COUNT];
    unsigned int recovery_method_original[BFM_LOG_MAX_COUNT];
    int record_count;
} bfr_real_recovery_info_t;

typedef enum bfr_misc_cmd
{
    BFR_MISC_CMD_ERECOVERY = 0,
    BFR_MISC_CMD_RECOVERY,
    BFR_MISC_CMD_MAX_COUNT,
} bfr_misc_cmd_e;


/*----export macroes-----------------------------------------------------------------*/

#define BFR_ENTER_ERECOVERY_CMD  "boot-erecovery"
#define BFR_ENTER_RECOVERY_CMD  "boot-recovery"
#define BFR_ENTER_SAFE_MODE_CMD "boot-safemode"
#define BFR_MISC_PART_NAME "misc"
#define BFR_VALID_LONG_PRESS_LOG "valid_long_press"

/* 0x72656364-'r' 'e' 'c' 'd' */
#define BFR_RRECORD_MAGIC_NUMBER ((unsigned int)0x72656364)

#define BFR_ENTER_ERECOVERY_REASON_SIZE (sizeof(bfmr_rrecord_misc_msg_param_t))
#define BFR_EACH_RECORD_PART_SIZE ((unsigned int)0x100000)
#define BFR_ENTER_ERECOVERY_REASON_OFFSET (BFR_MISC_PART_OFFSET)
#define BFR_RRECORD_FIRST_PART_OFFSET (BFR_ENTER_ERECOVERY_REASON_SIZE)
#define BFR_RRECORD_SECOND_PART_OFFSET (BFR_RRECORD_FIRST_PART_OFFSET + BFR_EACH_RECORD_PART_SIZE)
#define BFR_RRECORD_THIRD_PART_OFFSET (BFR_RRECORD_SECOND_PART_OFFSET + BFR_EACH_RECORD_PART_SIZE)
#define BFR_SAFE_MODE_ENABLE_FLAG ((unsigned int)0x656e736d) /*ensm*/
#define BFR_FACTORY_RESET_DONE ((unsigned int)0x6672646e) /*frdn*/
#define BFR_FSCK_BEFORE_FORMAT_DATA ((unsigned int)0x6673666d) /* fsfm */
#define BFR_FORMAT_DATA_WITHOUT_FSCK ((unsigned int)0x666d6474) /* fmdt */
#define bfr_is_download_recovery_method(recovery_method) \
    (((unsigned int)FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY == recovery_method) \
    || ((unsigned int)FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_DEL_FILES == recovery_method) \
    || ((unsigned int)FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_FACTORY_RESET == recovery_method))


/*----global variables-----------------------------------------------------------------*/


/*----export function prototypes--------------------------------------------------------*/

#ifdef CONFIG_USE_BOOTFAIL_RECOVERY_SOLUTION
int bfr_get_hardware_fault_times(bfmr_get_hw_fault_info_param_t *pfault_info_param);
int bfr_get_real_recovery_info(bfr_real_recovery_info_t *preal_recovery_info);
char* bfr_get_recovery_method_desc(int recovery_method);

/**
    @function: void boot_status_notify(int boot_success)
    @brief: when the system bootup successfully, the BFM must call this
        function to notify the BFR, and the BFM was notified by the BFD.

    @param: boot_success.

    @return: none.

    @note: this fuction only need be initialized in kernel.
*/
void boot_status_notify(int boot_success);

/**
    @function: bfr_recovery_method_e try_to_recovery(
        unsigned long long boot_fail_detected_time,
        bfmr_bootfail_errno_e boot_fail_no,
        bfmr_detail_boot_stage_e boot_fail_stage,
        bfr_suggested_recovery_method_e suggested_recovery_method,
        char *args)
    @brief: do recovery for the boot fail.

    @param: boot_fail_detected_time [in], rtc time when boot fail was detected.
    @param: boot_fail_no [in], boot fail errno.
    @param: boot_fail_stage [in], the stage when boot fail happened.
    @param: suggested_recovery_method [in], suggested recovery method transfered by the BFD(Boot Fail Detection).
    @param: args [in], extra parametrs for recovery.

    @return: the recovery method selected by the BFR.

    @note:
*/
bfr_recovery_method_e try_to_recovery(
    unsigned long long boot_fail_detected_time,
    bfmr_bootfail_errno_e boot_fail_no,
    bfmr_detail_boot_stage_e boot_fail_stage,
    bfr_suggested_recovery_method_e suggested_recovery_method,
    char *args);

/**
    @function: int bfr_init(void)
    @brief: init BFR.

    @param: none.

    @return: none.

    @note:
*/
int bfr_init(void);
#else
static inline int bfr_get_hardware_fault_times(bfmr_get_hw_fault_info_param_t *pfault_info_param)
{
    return -1;
}

static inline int bfr_get_real_recovery_info(bfr_real_recovery_info_t *preal_recovery_info)
{
    return 0;
}

static inline char* bfr_get_recovery_method_desc(int recovery_method)
{
    return "";
}

static inline void boot_status_notify(int boot_success)
{
    return;
}

static inline bfr_recovery_method_e try_to_recovery(
    unsigned long long boot_fail_detected_time,
    bfmr_bootfail_errno_e boot_fail_no,
    bfmr_detail_boot_stage_e boot_fail_stage,
    bfr_suggested_recovery_method_e suggested_recovery_method,
    char *args)
{
    return FRM_DO_NOTHING;
}

static inline int bfr_init(void)
{
    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* BFR_CORE_H */

