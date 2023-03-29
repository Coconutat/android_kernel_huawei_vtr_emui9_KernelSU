/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_chipsets.h

    @brief: define the chipsets' external public enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

#ifndef BFM_CHIPSETS_H
#define BFM_CHIPSETS_H


/*----includes-----------------------------------------------------------------------*/

#include <chipset_common/bfmr/common/bfmr_common.h>
#include <chipset_common/bfmr/bfr/core/bfr_core.h>


/*----c++ support-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif


/*----export prototypes---------------------------------------------------------------*/

/* this stuct is defined for HISI currently */
typedef struct bfi_head_info
{
    unsigned int magic;
    unsigned int total_number;
    unsigned int cur_number;
    unsigned int every_number_size;
} bfm_bfi_header_info_t;
       
/* this stuct is defined for HISI currently */
typedef struct bfi_every_number_Info
{
    unsigned long long rtcValue;
    unsigned int bfmErrNo;
    unsigned int bfmStageCode;
    unsigned int isSystemRooted;
    unsigned int isUserPerceptiable;
    unsigned int rcvMethod;
    unsigned int rcvResult;
    unsigned int dmdErrNo;
    char excepInfo[BFMR_SIZE_128];
    unsigned int bootup_time;
    unsigned int is_log_saved;
    unsigned int suggested_recovery_method;
    unsigned int is_bootup_successfully;
    unsigned int reboot_type;
} bfm_bfi_member_info_t;

/* this stuct is defined for qcom currently */
typedef struct bfm_record_Info
{
    unsigned int magic;
    unsigned int bfmErrNo;
    unsigned long long bootfail_time;
    unsigned int boot_stage;
    unsigned int suggested_recovery_method;
    unsigned int recovery_method;
    unsigned int total_log_lenth;
    unsigned int log_lenth[LOG_TYPE_MAX_COUNT];
    char log_dir[BFMR_SIZE_128];
    char log_name[LOG_TYPE_MAX_COUNT][BFMR_SIZE_128];
} bfm_record_info_t;

typedef struct
{
    char log_dir[BFMR_MAX_PATH];
} bfm_single_bootfail_log_info_t;

typedef struct
{
    bfm_single_bootfail_log_info_t bootfail_logs[BFM_LOG_MAX_COUNT];
    bfr_real_recovery_info_t real_recovery_info;
    int log_dir_count;
} bfm_bootfail_log_info_t;

typedef struct bfm_process_bootfail_param
{
    bfmr_bootfail_errno_e bootfail_errno;
    bfmr_detail_boot_stage_e boot_stage;
    unsigned long long bootfail_time;
    bfr_suggested_recovery_method_e suggested_recovery_method;
    bfr_recovery_method_e recovery_method;

    /* for hisi platform */
    unsigned int reboot_type;

    /* 1 - the user can sense the presence of the boot fail, 0 - can not sense the fault */
    int is_user_sensible;

    /* 1 - the system has been rooted, 0 - the system has not been rooted */
    int is_system_rooted;

    /* 1 - save bottom layer bootfail log, just for HISI now, 0 - save upper layer bootfail log */
    int save_bottom_layer_bootfail_log;

    /* 1 - process boot fail in chipsets, just for HISI currently, 0 - for non-HISI platform, this field is 0 */
    int bootfail_can_only_be_processed_in_platform;

    int dmd_num;

    /* save log to: log part, raw patition, memory etc. */
    int dst_type;

    /* log dir such as: /log/boot_fail/bootfail_20161010101010_0x03000005 */
    char bootfail_log_dir[BFMR_MAX_PATH];

    /* additional info such as: /cache/critical_process_crash.txt */
    bfmr_bootfail_addl_info_t addl_info;

    /* varified log path such as: /log/boot_fail/bootfail_20161010101010_0x03000005/xloader_log */
    char bootfail_log_path[BFMR_MAX_PATH];

    /* for HISI currently */
    void *log_save_context;

    /*for bfm_chipsets call bfm_core's function to save log*/
    void *capture_and_save_bootfail_log;

    char critical_process_name[BFMR_SIZE_128];

    bool save_log_after_reboot;

    uid_t user_space_log_uid;
    gid_t user_space_log_gid;

    char *user_space_log_buf;
    long user_space_log_len;
    long user_space_log_read_len;
    char excepInfo[BFMR_SIZE_128];
    bfm_bootfail_log_info_t bootfail_log_info;
    unsigned int bootup_time;
    bool is_bootup_successfully;
} bfm_process_bootfail_param_t;

typedef int (*bfmr_capture_and_save_bootfail_log)(bfm_process_bootfail_param_t *param);
typedef int (*bfmr_process_ocp_excp)(bfmr_hardware_fault_type_e fault_type, ocp_excp_info_t *pexcp_info);

typedef struct
{
    bfmr_capture_and_save_bootfail_log capture_and_save_bootfail_log;
} bfm_bootfail_log_saving_param_t;

typedef struct
{
    bfmr_process_ocp_excp process_ocp_excp;
    bfm_ocp_excp_info_t ocp_excp_info[10];
} bfm_process_ocp_excp_param_t;

typedef struct
{
    bfm_bootfail_log_saving_param_t log_saving_param;
    bfmr_process_ocp_excp process_ocp_excp;
} bfm_chipsets_init_param_t;

typedef struct bfm_ocp_boot_fail_test_param
{
    int sleep_time; /* ms */
    int ldo_count; /* the total count of ldos to be test, such as 3 */
    int ldo_initial_idx; /* the beginning idx of the ldo, such as 21 */
} bfm_ocp_boot_fail_test_param_t;


/*----export macroes-----------------------------------------------------------------*/

#define BFM_CRITICAL_PROCESS_CRASH_LOG_NAME "critical_process_crash.txt"
#define BFM_TOMBSTONE_LOG_NAME "tombstone_00"
#define BFM_SYSTEM_SERVER_CRASH_LOG_NAME "system_server_crash@0.txt"
#define BFM_SYSTEM_SERVER_WATCHDOG_LOG_NAME "system_server_watchdog@0.txt.gz"
#define BFM_LOGCAT_FILE_PATH "/data/log/android_logs/applogcat-log"
#define BFM_OLD_LOGCAT_FILE_PATH "/data/log/android_logs/applogcat-log.1"
#define BFM_LOGCAT_FILE_NAME "applogcat-log"
#define BFM_LOGCAT_FILE_NAME_KEYWORD "applogcat-log"
#define BFM_FRAMEWORK_BOOTFAIL_LOG_PATH "/data/anr/framework_boot_fail.log"
#define BFM_FRAMEWORK_BOOTFAIL_LOG_FILE_NAME "framework_boot_fail.log"
#define BFM_BETA_KMSG_LOG_PATH "/data/log/android_logs/kmsgcat-log"
#define BFM_BETA_OLD_KMSG_LOG_PATH "/data/log/android_logs/kmsgcat-log.1"
#define BFM_BL1_LOG_FILENAME "xloader_log"
#define BFM_BL2_LOG_FILENAME "fastboot_log"
#define BFM_KERNEL_LOG_FILENAME "last_kmsg"
#define BFM_ALT_BL1_LOG_FILENAME "sbl1.log"
#define BFM_ALT_BL2_LOG_FILENAME "lk.log"
#define BFM_ALT_KERNEL_LOG_FILENAME "kmsg.log"
#define BFM_PMSG_LOG_FILENAME "pmsg-ramoops-0"
#define BFM_KERNEL_LOG_GZ_FILENAME_KEYWORD "last_kmsg.android"
#define BFM_BOOT_FAIL_LOGCAT_FILE_MAX_SIZE ((unsigned int)512 * 1024)
#define BFM_UPLOADING_DIR_NAME "uploading"
#define BFM_BOOTFAIL_LOG_DIR_NAME_FORMAT "bootfail_%s_0x%08x"
#define BFM_BLOCK_CALLING_PROCESS_INTERVAL (3600 * 1000)


/*----global variables----------------------------------------------------------------*/


/*----export function prototypes--------------------------------------------------------*/

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
unsigned int bfmr_capture_log_from_system(char *buf, unsigned int buf_len, bfmr_log_src_t *src, int timeout);

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
    unsigned int buf_len);

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
int bfmr_save_log_to_fs(char *dst_file_path, char *buf, unsigned int log_len, int append);

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
int bfmr_save_log_to_raw_part(char *raw_part_name, unsigned long long offset, char *buf, unsigned int log_len);

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
int bfmr_save_log_to_mem_buffer(char *dst_buf, unsigned int dst_buf_len, char *src_buf, unsigned int log_len);

/**
    @function: char* bfm_get_bfmr_log_root_path(void)
    @brief: get log root path

    @param: none

    @return: the path of bfmr log's root path.

    @note:
*/
char* bfm_get_bfmr_log_root_path(void);
char* bfm_get_bfmr_log_uploading_path(void);
char* bfm_get_bfmr_log_part_mount_point(void);
int bfm_get_boot_stage(bfmr_detail_boot_stage_e *pbfmr_bootstage);
int bfm_set_boot_stage(bfmr_detail_boot_stage_e bfmr_bootstage);
int bfm_capture_and_save_do_nothing_bootfail_log(bfm_process_bootfail_param_t *param);

/**
    @function: int bfm_platform_process_boot_fail(bfm_process_bootfail_param_t *param)
    @brief: process boot fail in chipsets module

    @param: param [int] params for further process boot fail in chipsets

    @return: 0 - succeeded; -1 - failed.

    @note: realize this function according to diffirent platforms
*/
int bfm_platform_process_boot_fail(bfm_process_bootfail_param_t *param);

/**
    @function: int bfm_platform_process_boot_success(void)
    @brief: process boot success in chipsets module

    @param: none

    @return: 0 - succeeded; -1 - failed.

    @note: HISI must realize this function in detail, the other platform can return 0 when enter this function
*/
int bfm_platform_process_boot_success(void);
int bfm_is_system_rooted(void);
int bfm_is_user_sensible_bootfail(bfmr_bootfail_errno_e bootfail_errno,
    bfr_suggested_recovery_method_e suggested_recovery_method);
char* bfm_get_bl1_bootfail_log_name(void);
char* bfm_get_bl2_bootfail_log_name(void);
char* bfm_get_kernel_bootfail_log_name(void);
char* bfm_get_ramoops_bootfail_log_name(void);
char* bfm_get_platform_name(void);
unsigned int bfm_get_dfx_log_length(void);
int bfm_update_platform_logs(bfm_bootfail_log_info_t *pbootfail_log_info);
/**
    @function: int bfmr_copy_data_from_dfx_to_bfmr_tmp_buffer(void)
    @brief: copy dfx data to local buffer

    @param: none

    @return: 0 - succeeded; -1 - failed.

    @note: HISI must realize this function in detail, the other platform can return when enter this function
*/
void bfmr_copy_data_from_dfx_to_bfmr_tmp_buffer(void);
char* bfm_get_raw_part_name(void);
int bfm_get_raw_part_offset(void);
void bfmr_alloc_and_init_raw_log_info(bfm_process_bootfail_param_t *pparam, bfmr_log_dst_t *pdst);
void bfmr_save_and_free_raw_log_info(bfm_process_bootfail_param_t *pparam);
void bfmr_update_raw_log_info(bfmr_log_src_t *psrc, bfmr_log_dst_t *pdst, unsigned int bytes_read);
unsigned int bfmr_capture_log_from_src_file(char *buf, unsigned int buf_len, char *src_log_path);

/*
* param must be the type of bfm_ocp_boot_fail_test_param_t
* usage:
* bfm_ocp_boot_fail_test_param_t *ptest_param = (bfm_ocp_boot_fail_test_param_t *)vmalloc(sizeof(bfm_ocp_boot_fail_test_param_t));
* if (NULL != ptest_param)
* {
*     ptest_param->ldo_count = 3;
*     ptest_param->ldo_initial_idx = 21;
*     ptest_param->sleep_time = 45000;
*     bfm_process_ocp_boot_fail_test((void *)ptest_param);
*     vfree(ptest_param);
* }
*/
int bfm_process_ocp_boot_fail_test(void *param);
void bfm_set_valid_long_press_flag(void);
int bfm_chipsets_init(bfm_chipsets_init_param_t *param);

#ifdef __cplusplus
}
#endif

#endif

