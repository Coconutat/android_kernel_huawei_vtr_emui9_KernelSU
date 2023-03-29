/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_core.h

    @brief: define the core's external public enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

#ifndef BFM_CORE_H
#define BFM_CORE_H


/*----includes-----------------------------------------------------------------------*/

#include <linux/ioctl.h>
#include <linux/types.h>
#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/common/bfmr_common.h>
#include <chipset_common/bfmr/bfr/core/bfr_core.h>
#include <chipset_common/bfmr/bfm/core/bfm_timer.h>


/*----c++ support-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif


/*----export prototypes---------------------------------------------------------------*/

struct bfmr_dev_path
{
    char dev_name[BFMR_SIZE_64];
    char dev_path[BFMR_SIZE_1K];
};

struct bfmr_boot_fail_info
{
    int boot_fail_no;
    int suggested_recovery_method;
    bfmr_bootfail_addl_info_t addl_info;
};


/*----export macroes-----------------------------------------------------------------*/

#define BFMR_IOCTL_BASE 'B'
#define BFMR_GET_TIMER_STATUS _IOR(BFMR_IOCTL_BASE, 1, int)
#define BFMR_ENABLE_TIMER _IOW(BFMR_IOCTL_BASE, 2, int)
#define BFMR_DISABLE_TIMER _IOW(BFMR_IOCTL_BASE, 3, int)
#define BFMR_GET_TIMER_TIMEOUT_VALUE _IOR(BFMR_IOCTL_BASE, 4, int)
#define BFMR_SET_TIMER_TIMEOUT_VALUE _IOW(BFMR_IOCTL_BASE, 5, int)
#define BFMR_GET_BOOT_STAGE  _IOR(BFMR_IOCTL_BASE, 6, int)
#define BFMR_SET_BOOT_STAGE  _IOW(BFMR_IOCTL_BASE, 7, int)
#define BFMR_PROCESS_BOOT_FAIL _IOW(BFMR_IOCTL_BASE, 8, struct bfmr_boot_fail_info)
#define BFMR_GET_DEV_PATH _IOR(BFMR_IOCTL_BASE, 9, struct bfmr_dev_path)
#define BFMR_ENABLE_CTRL _IOW(BFMR_IOCTL_BASE, 10, int)
#define BFMR_ACTION_TIMER_CTL _IOW(BFMR_IOCTL_BASE, 11, struct action_ioctl_data)


/*----global variables----------------------------------------------------------------*/


/*----export function prototypes--------------------------------------------------------*/

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
    bfmr_bootfail_addl_info_t *paddl_info);

/**
    @function: int bfmr_get_boot_stage(bfmr_detail_boot_stage_e *pboot_stage)
    @brief: get current boot stage during boot process.

    @param: pboot_stage [out], buffer storing the boot stage.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_get_boot_stage(bfmr_detail_boot_stage_e *pboot_stage);

/**
    @function: int bfmr_set_boot_stage(bfmr_detail_boot_stage_e boot_stage)
    @brief: get current boot stage during boot process.

    @param: boot_stage [in], boot stage to be set.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_set_boot_stage(bfmr_detail_boot_stage_e boot_stage);

/**
    @function: int bfmr_get_timer_state(int *state)
    @brief: get state of the timer.

    @param: state [out], the state of the boot timer.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
        2. if *state == 0, the boot timer is disabled, if *state == 1, the boot timer is enbaled.
*/
int bfmr_get_timer_state(int *state);

/**
    @function: int bfm_enable_timer(void)
    @brief: enbale timer.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_enable_timer(void);

/**
    @function: int bfm_disable_timer(void)
    @brief: disable timer.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_disable_timer(void);

/**
    @function: int bfm_set_timer_timeout_value(unsigned int timeout)
    @brief: set timeout value of the kernel timer. Note: the timer which control the boot procedure is in the kernel.

    @param: timeout [in] timeout value (unit: msec).

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_set_timer_timeout_value(unsigned int timeout);

/**
    @function: int bfm_get_timer_timeout_value(unsigned int *timeout)
    @brief: get timeout value of the kernel timer. Note: the timer which control the boot procedure is in the kernel.

    @param: timeout [in] buffer will store the timeout value (unit: msec).

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_get_timer_timeout_value(unsigned int *timeout);

/**
    @function: int bfm_init(void)
    @brief: init bfm module in kernel.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in bootloader and kernel.
*/
int bfm_init(void);

int bfm_get_log_count(char *bfmr_log_root_path);
void bfm_delete_dir(char *log_path);

//send sgnal to init, it will show init task trace.
void bfm_send_signal_to_init(void);


#ifdef __cplusplus
}
#endif

#endif

