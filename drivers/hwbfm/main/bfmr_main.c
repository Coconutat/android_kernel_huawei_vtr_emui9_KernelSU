/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfmr_main.c

    @brief: It is the main entry for BFMR(Boot Fail Monitor and Recovery)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <chipset_common/bfmr/common/bfmr_common.h>
#include <chipset_common/bfmr/bfm/core/bfm_core.h>
#include <chipset_common/bfmr/bfr/core/bfr_core.h>


/*----local macroes-----------------------------------------------------------------*/


/*----local prototypes---------------------------------------------------------------*/


/*----local variables------------------------------------------------------------------*/

static char s_boot_lock_info[32] = {0};
static int s_is_bfmr_enabled = 0;
static int s_is_bfr_enabled = 0;
static DEFINE_SEMAPHORE(s_bfmr_enable_ctl_sem);


/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes---------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/


/*----function definitions--------------------------------------------------------------*/

static int __init bfm_early_parse_bootlock_cmdline(char *p)
{
    BFMR_PRINT_KEY_INFO(BFMR_BOOTLOCK_FIELD_NAME "=%s\n", p);
    memset((void *)s_boot_lock_info, 0, sizeof(s_boot_lock_info));
    snprintf(s_boot_lock_info, sizeof(s_boot_lock_info) - 1, BFMR_BOOTLOCK_FIELD_NAME "=%s", p);

    return 0;
}
early_param(BFMR_BOOTLOCK_FIELD_NAME, bfm_early_parse_bootlock_cmdline);


static int __init early_parse_bfmr_enable_flag(char *p)
{
    if (NULL != p)
    {
        if (0 == strncmp(p, "1", strlen("1")))
        {
            s_is_bfmr_enabled = 1;
        }
        else
        {
            s_is_bfmr_enabled = 0;
        }

        BFMR_PRINT_KEY_INFO(BFMR_ENABLE_FIELD_NAME "=%s\n", p);
    }

    return 0;
}
early_param(BFMR_ENABLE_FIELD_NAME, early_parse_bfmr_enable_flag);

static int __init early_parse_bfr_enable_flag(char *p)
{
    if (NULL != p)
    {
        if (0 == strncmp(p, "1", strlen("1")))
        {
            s_is_bfr_enabled = 1;
        }
        else
        {
            s_is_bfr_enabled = 0;
        }

        BFMR_PRINT_KEY_INFO(BFR_ENABLE_FIELD_NAME "=%s\n", p);
    }

    return 0;
}
early_param(BFR_ENABLE_FIELD_NAME, early_parse_bfr_enable_flag);


bool bfmr_has_been_enabled(void)
{
    int bfmr_enable_flag = 0;
    down(&s_bfmr_enable_ctl_sem);
    bfmr_enable_flag = s_is_bfmr_enabled;
    up(&s_bfmr_enable_ctl_sem);
    return (0 == bfmr_enable_flag) ? (false) : (true);
}


bool bfr_has_been_enabled(void)
{
    return (0 == s_is_bfr_enabled) ? (false) : (true);
}


void bfmr_enable_ctl(int enable_flag)
{
    down(&s_bfmr_enable_ctl_sem);
    s_is_bfmr_enabled = enable_flag;
    up(&s_bfmr_enable_ctl_sem);
}


char* bfm_get_bootlock_value_from_cmdline(void)
{
    return s_boot_lock_info;
}


/**
    @function: int __init bfmr_init(void)
    @brief: init bfmr module in kernel.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in bootloader and kernel.
*/
int __init bfmr_init(void)
{
    bfmr_common_init();
    bfm_init();
    bfr_init();

    return 0;
}

static void __exit bfmr_exit(void)
{
    return ;
}

fs_initcall(bfmr_init);
module_exit(bfmr_exit);
MODULE_LICENSE("GPL");

