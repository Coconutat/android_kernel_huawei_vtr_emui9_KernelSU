/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_core.h

    @brief: define the basic external enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

#ifndef BFM_TIMER_H
#define BFM_TIMER_H

/*----includes-----------------------------------------------------------------------*/


/*----c++ support-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif


/*----export prototypes---------------------------------------------------------------*/


/*----export macroes-----------------------------------------------------------------*/
#define BFMR_ACTION_NAME_LEN 64

/*----global variables----------------------------------------------------------------*/


/*----export prototypes--------------------------------------------------------------*/
typedef enum
{
    ACT_TIMER_START  = 0xbfac0000,
    ACT_TIMER_STOP   = 0xbfac0001,
    ACT_TIMER_PAUSE  = 0xbfac0002,
    ACT_TIMER_RESUME = 0xbfac0003,
} bfm_action_timer_op_e;

struct action_ioctl_data {
  bfm_action_timer_op_e op;
  char action_name[BFMR_ACTION_NAME_LEN];
  u32 action_timer_timeout_value;
};


/**
    @function: int bfm_get_boot_timer_state(int *state)
    @brief: get state of the boot timer.

    @param: state [out], the state of the boot timer.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
        2. if *state == 0, the boot timer is disabled, if *state == 1, the boot timer is enbaled.
*/
int bfm_get_boot_timer_state(int *state);

/**
    @function: int bfm_suspend_boot_timer(void)
    @brief: suspend the boot timer.

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfm_suspend_boot_timer(void);

/**
    @function: int bfmr_resume_boot_timer(void)
    @brief: resume the boot timer.

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfmr_resume_boot_timer(void);

/**
    @function: int bfm_set_boot_timer_timeout_value(unsigned int timeout_value)
    @brief: set timeout value of the boot timer.

    @param: timeout_value [int], timeout value to be set, unit:msec.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
*/
int bfm_set_boot_timer_timeout_value(unsigned int timeout_value);

/**
    @function: int bfm_get_boot_timer_timeout_value(unsigned int *ptimeout_value)
    @brief: get timeout value of the boot timer.

    @param: ptimeout_value [out], timeout value of the timer, unit:msec.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
*/
int bfm_get_boot_timer_timeout_value(unsigned int *ptimeout_value);

/**
    @function: int bfm_action_timer_start(char *act_name, unsigned int timeout_value)
    @brief:
    1. creat a new action timer
    2. stop the short boot timer

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfm_action_timer_start(char *act_name, unsigned int timeout_value);

/**
    @function: int bfm_action_timer_stop(char *act_name)
    @brief:
    1. stop & destroy the action timer
    2. resume the short boot timer if the action timer list is empty

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfm_action_timer_stop(char *act_name);

/**
    @function: int bfm_action_timer_pause(char *act_name)
    @brief:
    1. pause the action timer

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfm_action_timer_pause(char *act_name);

/**
    @function: int bfm_action_timer_resume(char *act_name)
    @brief:
    1. resume the action timer

    @param: none.

    @return: 0 - success, -1 - failed.

    @note:
*/
int bfm_action_timer_resume(char *act_name);

/**
    @function: int bfm_stop_boot_timer(void)
    @brief: stop the boot timer.

    @param: none.

    @return: 0 - success, -1 - failed.

    @note: this fuction only need be initialized in kernel.
*/
int bfm_stop_boot_timer(void);

/**
    @function: int bfm_init_boot_timer(void)
    @brief: init the boot timer.

    @param: none.

    @return: 0 - success, -1 - failed.

    @note: this fuction only need be initialized in kernel.
*/
int bfm_init_boot_timer(void);

#ifdef __cplusplus
}
#endif

#endif

