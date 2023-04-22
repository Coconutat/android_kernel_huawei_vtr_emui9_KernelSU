/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfr_core.c

    @brief: define the core's external public enum/macros/interface for BFR (Boot Fail Recovery)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <chipset_common/bfmr/common/bfmr_common.h>
#include <chipset_common/bfmr/bfr/chipsets/bfr_chipsets.h>
#include <chipset_common/bfmr/bfr/core/bfr_core.h>


/*----local macroes------------------------------------------------------------------*/

#define BFR_RECORD_COPIES_MAX_COUNT (2)
#define BFR_RRECORD_PART_MAX_COUNT (2)
#define BFR_RECOVERY_RECORD_READ_MAX_COUNT (5)
#define BF_0_TIME (0)
#define BF_1_TIME (1)
#define BF_2_TIME (2)
#define BF_3_TIME (3)
#define BF_4_TIME (4)
#define bfr_is_package_install_successfully(running_status_code) (RMRSC_ERECOVERY_INSTALL_PACKAGES_SUCCESS == running_status_code)
#define bfr_is_factory_reset_done(factory_reset_flag) (BFR_FACTORY_RESET_DONE == factory_reset_flag)
#define bfr_is_boot_fail_recovery_successfully(result) (BOOT_FAIL_RECOVERY_SUCCESS == result)


/*----local prototypes----------------------------------------------------------------*/

typedef struct bfr_misc_message
{
    char command[32];
    char status[32];
    char recovery[1024];
} bfr_misc_message_t;

typedef struct bfr_recovery_method_select_param
{
    unsigned int cur_boot_fail_stage;
    unsigned int cur_boot_fail_no;
    bfr_suggested_recovery_method_e suggested_recovery_method;
    int latest_valid_recovery_record_count;
    int recovery_failed_times_total;
    int recovery_failed_times_in_bottom_layer;
    int recovery_failed_times_in_application;
    int bootfail_with_safe_mode_failed_times;
    bool need_not_parse_recovery_method_further;
    bool has_fixed_recovery_method;
    bfr_recovery_method_e fixed_recovery_method;
    bfr_recovery_method_e recovery_method;
    bfr_recovery_record_t latest_recovery_record[BFR_RECOVERY_RECORD_READ_MAX_COUNT];
    char *bootfail_detail_info;
} bfr_recovery_method_select_param_t;

typedef struct
{
    bfr_recovery_method_e recovery_method;
    char *desc;
} bfr_recovery_method_desc_t;

typedef struct
{
    unsigned int bootfail_errno;
    bool has_safemode_recovery_method;
} bfr_safemode_recovery_param_t;


/*----local variables-----------------------------------------------------------------*/

static bfr_enter_erecovery_reason_map_t s_enter_erecovery_reason_map[] =
{
    {{BL1_ERRNO_START, BL1_PL1_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_BOOTLOADER_BOOT_FAIL},
    {{BL1_PL1_START, BL1_PL2_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_BOOTLOADER_BOOT_FAIL},
    {{BL1_PL2_START, BL2_ERRNO_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_BOOTLOADER_BOOT_FAIL},
    {{BL2_ERRNO_START, BL2_PL1_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_BOOTLOADER_BOOT_FAIL},
    {{BL2_PL1_START, BL2_PL2_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_BOOTLOADER_BOOT_FAIL},
    {{BL2_PL2_START, KERNEL_ERRNO_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_BOOTLOADER_BOOT_FAIL},
    {{KERNEL_ERRNO_START, KERNEL_PL1_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_KERNEL_BOOT_FAIL},
    {{KERNEL_PL1_START, KERNEL_PL2_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_KERNEL_BOOT_FAIL},
    {{KERNEL_PL2_START, NATIVE_ERRNO_START - 1}, ENTER_ERECOVERY_REASON_BECAUSE_KERNEL_BOOT_FAIL},
    {{SYSTEM_MOUNT_FAIL, SYSTEM_MOUNT_FAIL}, ENTER_ERECOVERY_BECAUSE_SYSTEM_MOUNT_FAILED},
    {{SECURITY_FAIL, SECURITY_FAIL}, ENTER_ERECOVERY_BECAUSE_SECURITY_FAIL},
    {{CRITICAL_SERVICE_FAIL_TO_START, CRITICAL_SERVICE_FAIL_TO_START}, ENTER_ERECOVERY_BECAUSE_KEY_PROCESS_START_FAILED},
    {{DATA_MOUNT_FAILED_AND_ERASED, DATA_MOUNT_FAILED_AND_ERASED}, ENTER_ERECOVERY_BECAUSE_DATA_MOUNT_FAILED},
    {{DATA_MOUNT_RO, DATA_MOUNT_RO}, ENTER_ERECOVERY_BECAUSE_DATA_MOUNT_RO},
    {{VENDOR_MOUNT_FAIL, VENDOR_MOUNT_FAIL}, ENTER_ERECOVERY_BECAUSE_VENDOR_MOUNT_FAILED},
    {{NATIVE_ERRNO_START, PACKAGE_MANAGER_SETTING_FILE_DAMAGED}, ENTER_ERECOVERY_BECAUSE_APP_BOOT_FAIL},
};

#if defined(CONFIG_USE_AB_SYSTEM)
static bfr_recovery_policy_e s_fixed_recovery_policy[] =
{
    {
        SYSTEM_MOUNT_FAIL, 1,
        {
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY}
        },
    },
};
#else
static bfr_recovery_policy_e s_fixed_recovery_policy[] =
{
    {
        SYSTEM_MOUNT_FAIL, 1,
        {
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY}
        },
    },
    {
        VENDOR_MOUNT_FAIL, 1,
        {
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY}
        },
    },
    {
        CRITICAL_SERVICE_FAIL_TO_START, 1,
        {
            {1, FRM_REBOOT},
            {1, FRM_GOTO_ERECOVERY_FACTORY_RESET},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
        },
    },
    {
        DATA_MOUNT_FAILED_AND_ERASED, 1,
        {
            {1, FRM_REBOOT},
            {1, FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
        },
    },
    {
        DATA_MOUNT_RO, 1,
        {
            {1, FRM_REBOOT},
            {1, FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
        },
    },
    {
        DATA_NOSPC, 1,
        {
            {1, FRM_GOTO_ERECOVERY_DEL_FILES_FOR_NOSPC},
            {1, FRM_GOTO_ERECOVERY_FACTORY_RESET},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
        },
    },
    {
        FRK_USER_DATA_DAMAGED, 1,
        {
            {1, FRM_REBOOT},
            {1, FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA},
            {1, FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA},
            {1, FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA},
        },
    },
    {
        KERNEL_BOOT_TIMEOUT, 1,
        {
            {1, FRM_REBOOT},
            {1, FRM_GOTO_ERECOVERY_FACTORY_RESET},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
            {1, FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY},
        },
    },
    {
        FRK_USER_DATA_DAMAGED, 1,
        {
            {1, FRM_REBOOT},
            {1, FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA},
            {1, FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA},
            {1, FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA},
        },
    },
    {
        BFM_HARDWARE_FAULT, 1,
        {
            {1, FRM_REBOOT},
            {1, FRM_DO_NOTHING},
            {1, FRM_DO_NOTHING},
            {1, FRM_DO_NOTHING},
        },
    }
};
#endif

static bfr_recovery_record_param_t s_rrecord_param[BFR_RECORD_COPIES_MAX_COUNT];
static bfr_recovery_method_desc_t s_recovery_method_desc[] =
{
    {FRM_DO_NOTHING, "do nothing"},
    {FRM_REBOOT, "reboot"},
    {FRM_GOTO_B_SYSTEM, "goto B system"},
    {FRM_GOTO_ERECOVERY_DEL_FILES_FOR_BF, "del files in /data part"},
    {FRM_GOTO_ERECOVERY_DEL_FILES_FOR_NOSPC, "del files in /data part because of no space"},
    {FRM_GOTO_ERECOVERY_FACTORY_RESET, "recommend user to do factory reset"},
    {FRM_GOTO_ERECOVERY_FORMAT_DATA_PART, "recommend user to format /data part"},
    {FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY, "download and recovery"},
    {FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_DEL_FILES, "recommend user to del files in /data after download and recovery"},
    {FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_FACTORY_RESET, "recommend user to do factory reset after download and recovery"},
    {FRM_NOTIFY_USER_RECOVERY_FAILURE, "recovery failed, the boot fail fault can't be recoveried by BFRM"},
    {FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA, "recommend user to do low-level formatting data"},
    {FRM_ENTER_SAFE_MODE, "enter android safe mode to uninstall the third party APK"},
    {FRM_FACTORY_RESET_AFTER_DOWNLOAD_RECOVERY, "factory reset after download and recovery"},
};

static bfr_safemode_recovery_param_t s_safemode_recovery_param[] = {
    {KERNEL_AP_PANIC, true},
    {KERNEL_PRESS10S, true},
    {KERNEL_AP_WDT, true},
    {KERNEL_AP_S_ABNORMAL, true},
    {KERNEL_BOOT_TIMEOUT, true},
    {CRITICAL_SERVICE_FAIL_TO_START, true},
};


/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes--------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/

#if defined(CONFIG_USE_AB_SYSTEM)
static int bfr_another_system_is_ok(void);
static void bfr_goto_another_system(void);
#endif
static bool bfr_boot_fail_has_fixed_recovery_method(bfr_recovery_method_select_param_t *pselect_param);
static bfr_recovery_method_running_status_e bfr_init_recovery_method_running_status(
    bfr_recovery_method_e recovery_method);
static bfr_recovery_method_run_result_e bfr_init_recovery_method_run_result(
    bfr_recovery_method_e recovery_method);
static int bfr_select_recovery_method_with_safe_mode(
    bfr_recovery_method_select_param_t *pselect_param);
static int bfr_select_recovery_method_without_safe_mode(
    bfr_recovery_method_select_param_t *pselect_param);
static bfr_boot_fail_stage_e bfr_get_main_boot_fail_stage(unsigned int boot_fail_stage);
static int bfr_run_recovery_method(bfr_recovery_method_select_param_t *pselect_param);
static int bfr_set_misc_msg_for_erecovery(void);
static int bfr_init_local_recovery_record_param(void);
static int bfr_release_local_recovery_record_param(void);
static int bfr_read_recovery_record_to_local_buf(void);
static int bfr_verify_recovery_record(void);
static int bfr_read_and_verify_recovery_record(void);
static int bfr_init_recovery_record_header(void);
static int bfr_read_recovery_record(bfr_recovery_record_t *precord,
    int record_count_to_read,
    int *record_count_actually_read);
static int bfr_create_recovery_record(bfr_recovery_record_t *precord);
static int bfr_renew_recovery_record(bfr_recovery_record_t *precord);
static bool bfr_bootfail_has_safe_mode_recovery_method(unsigned int bootfail_errno);
static bool bfr_need_factory_reset_after_download_recovery(bfr_recovery_method_select_param_t *pselect_param);
static unsigned int bfr_get_bootfail_uptime(void);


/*----function definitions--------------------------------------------------------------*/

#if defined(CONFIG_USE_AB_SYSTEM)
static int bfr_another_system_is_ok(void)
{
    return 1;
}


static void bfr_goto_another_system(void)
{
    return;
}
#endif


static bool bfr_bootfail_has_safe_mode_recovery_method(unsigned int bootfail_errno)
{
    unsigned int i = 0;
    unsigned int count = sizeof(s_safemode_recovery_param) / sizeof(s_safemode_recovery_param[0]);

    for (i = 0; i < count; i++)
    {
        if (bootfail_errno == s_safemode_recovery_param[i].bootfail_errno)
        {
            return s_safemode_recovery_param[i].has_safemode_recovery_method;
        }
    }

    return false;
}


static bool bfr_boot_fail_has_fixed_recovery_method(bfr_recovery_method_select_param_t *pselect_param)
{
    int count = sizeof(s_fixed_recovery_policy) / sizeof(s_fixed_recovery_policy[0]);
    int i = 0;

    if (unlikely(NULL == pselect_param))
    {
        BFMR_PRINT_INVALID_PARAMS("pselect_param.\n");
        return false;
    }

    if (BFM_HARDWARE_FAULT == pselect_param->cur_boot_fail_no)
    {
        bfmr_get_hw_fault_info_param_t *pfault_info_param = NULL;

        pselect_param->need_not_parse_recovery_method_further = true;
        pselect_param->fixed_recovery_method = FRM_DO_NOTHING;
        if (NULL == pselect_param->bootfail_detail_info)
        {
            return true;
        }

        pfault_info_param = (bfmr_get_hw_fault_info_param_t *)bfmr_malloc(sizeof(bfmr_get_hw_fault_info_param_t));
        if (NULL == pfault_info_param)
        {
            BFMR_PRINT_ERR("bfmr_malloc failed!\n");
            return true;
        }
        memset(pfault_info_param, 0, sizeof(bfmr_get_hw_fault_info_param_t));

        pfault_info_param->fault_stage = bfmr_is_boot_success(pselect_param->cur_boot_fail_stage)
            ? HW_FAULT_STAGE_AFTER_BOOT_SUCCESS : HW_FAULT_STAGE_DURING_BOOTUP;
        memcpy(pfault_info_param->hw_excp_info.ocp_excp_info.ldo_num, pselect_param->bootfail_detail_info,
            BFMR_MIN(sizeof(pfault_info_param->hw_excp_info.ocp_excp_info.ldo_num) - 1, strlen(pselect_param->bootfail_detail_info)));
        switch (bfr_get_hardware_fault_times(pfault_info_param))
        {
        case 0:
            {
                pselect_param->recovery_method =  bfmr_is_boot_success(pselect_param->cur_boot_fail_stage) ? FRM_DO_NOTHING : FRM_REBOOT;
                break;
            }
        case 1:
            {
                pselect_param->recovery_method = FRM_DO_NOTHING;
                break;
            }
        default:
            {
                pselect_param->recovery_method = FRM_DO_NOTHING;
                break;
            }
        }

        /* release memory */
        bfmr_free(pfault_info_param);

        pselect_param->fixed_recovery_method = pselect_param->recovery_method;
        return true;
    }

    for (i = 0; i < count; i++)
    {
        if (pselect_param->cur_boot_fail_no != s_fixed_recovery_policy[i].boot_fail_no)
        {
            continue;
        }

        if (0 != s_fixed_recovery_policy[i].has_fixed_policy)
        {
            int method_count = sizeof(s_fixed_recovery_policy[i].param) / sizeof(s_fixed_recovery_policy[i].param[0]);

            if ((pselect_param->recovery_failed_times_in_application <= 0) || (pselect_param->recovery_failed_times_in_application > method_count))
            {
                pselect_param->fixed_recovery_method = FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY;
                BFMR_PRINT_KEY_INFO("ErrNo: %x failed_times_in_application: %d recovery_method: %d\n", (unsigned int)pselect_param->cur_boot_fail_no,
                    pselect_param->recovery_failed_times_in_application, pselect_param->fixed_recovery_method);
                return true;
            }

            if (0 != s_fixed_recovery_policy[i].param[pselect_param->recovery_failed_times_in_application - 1].enable_this_method)
            {
                pselect_param->fixed_recovery_method = s_fixed_recovery_policy[i].param[
                    pselect_param->recovery_failed_times_in_application - 1].recovery_method;
            }
            else
            {
                pselect_param->fixed_recovery_method = FRM_DO_NOTHING;
            }

            BFMR_PRINT_KEY_INFO("ErrNo: %x recovery_method: %x\n", pselect_param->cur_boot_fail_no, pselect_param->fixed_recovery_method);

            return true;
        }
        else
        {
            break;
        }
    }

    return false;
}


static unsigned int bfr_get_enter_erecovery_reason(unsigned int boot_fail_no)
{
    int i = 0;
    int count = sizeof(s_enter_erecovery_reason_map) / sizeof(s_enter_erecovery_reason_map[0]);

    for (i = 0; i < count; i++)
    {
        if ((boot_fail_no >= s_enter_erecovery_reason_map[i].range.start)
            && boot_fail_no <= s_enter_erecovery_reason_map[i].range.end)
        {
            return s_enter_erecovery_reason_map[i].enter_erecovery_reason;
        }
    }

    return ENTER_ERECOVERY_UNKNOWN;
}


static int bfr_set_misc_msg(bfr_misc_cmd_e cmd_type)
{
    int ret = -1;
    bfr_misc_message_t *pmsg = NULL;
    char *dev_path = NULL;
    const char *cmd_str = NULL;

    dev_path = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN + 1);
    if (NULL == dev_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)dev_path, 0, BFMR_DEV_FULL_PATH_MAX_LEN + 1);

    pmsg = (bfr_misc_message_t *)bfmr_malloc(sizeof(bfr_misc_message_t));
    if (NULL == pmsg)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pmsg, 0, sizeof(bfr_misc_message_t));

    ret = bfmr_get_device_full_path(BFR_MISC_PART_NAME, dev_path, BFMR_DEV_FULL_PATH_MAX_LEN);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("get full path of [%s] failed!\n", BFR_MISC_PART_NAME);
        goto __out;
    }

    switch (cmd_type)
    {
    case BFR_MISC_CMD_ERECOVERY:
        {
            cmd_str = BFR_ENTER_ERECOVERY_CMD;
            break;
        }
    case BFR_MISC_CMD_RECOVERY:
        {
            cmd_str = BFR_ENTER_RECOVERY_CMD;
            break;
        }
    default:
        {
            goto __out;
        }
    }

    memcpy(pmsg->command, (void *)cmd_str, BFMR_MIN(
        strlen(cmd_str), sizeof(pmsg->command) - 1));
    ret = bfmr_write_emmc_raw_part(dev_path, 0x0, (char *)pmsg, sizeof(bfr_misc_message_t));
    if (0 != ret)
    {
        BFMR_PRINT_ERR("write misc cmd failed!\n");
        goto __out;
    }

__out:
    if (NULL != pmsg)
    {
        bfmr_free(pmsg);
    }

    if (NULL != dev_path)
    {
        bfmr_free(dev_path);
    }

    return ret;
}


static int bfr_set_misc_msg_for_erecovery(void)
{
    return bfr_set_misc_msg(BFR_MISC_CMD_ERECOVERY);
}


static bool bfr_need_factory_reset_after_download_recovery(bfr_recovery_method_select_param_t *pselect_param)
{
    if (NULL == pselect_param)
    {
        BFMR_PRINT_INVALID_PARAMS("pselect_param.\n");
        return false;
    }

    if (pselect_param->latest_valid_recovery_record_count <= 0)
    {
        BFMR_PRINT_ERR("recovery record count: [%d]\n", pselect_param->latest_valid_recovery_record_count);
        return false;
    }

    BFMR_PRINT_KEY_INFO("recovery_method: %d, running_status_code: %d, recovery_result: %x, factory_reset_flag:%x\n",
        pselect_param->latest_recovery_record[pselect_param->latest_valid_recovery_record_count - 1].recovery_method,
        pselect_param->latest_recovery_record[pselect_param->latest_valid_recovery_record_count - 1].running_status_code,
        pselect_param->latest_recovery_record[pselect_param->latest_valid_recovery_record_count - 1].recovery_result,
        pselect_param->latest_recovery_record[pselect_param->latest_valid_recovery_record_count - 1].factory_reset_flag);
    if (bfr_is_download_recovery_method(pselect_param->latest_recovery_record[
        pselect_param->latest_valid_recovery_record_count - 1].recovery_method)
        && bfr_is_package_install_successfully(pselect_param->latest_recovery_record[
        pselect_param->latest_valid_recovery_record_count - 1].running_status_code)
        && !bfr_is_boot_fail_recovery_successfully(pselect_param->latest_recovery_record[
        pselect_param->latest_valid_recovery_record_count - 1].recovery_result)
        && !bfr_is_factory_reset_done(pselect_param->latest_recovery_record[
        pselect_param->latest_valid_recovery_record_count - 1].factory_reset_flag))
    {
        return true;
    }

    return false;
}


static int bfr_parse_boot_fail_info(bfr_recovery_method_select_param_t *pselect_param)
{
    int i = 0;

    if (NULL == pselect_param)
    {
        BFMR_PRINT_INVALID_PARAMS("pselect_param.\n");
        return -1;
    }

    if (bfr_need_factory_reset_after_download_recovery(pselect_param))
    {
        pselect_param->recovery_method = FRM_FACTORY_RESET_AFTER_DOWNLOAD_RECOVERY;
        pselect_param->need_not_parse_recovery_method_further = true;
        return 0;
    }

    /* record count is >0 */
    for (i= 0; i < pselect_param->latest_valid_recovery_record_count; i++)
    {
        if (BOOT_FAIL_RECOVERY_SUCCESS == pselect_param->latest_recovery_record[i].recovery_result)
        {
            pselect_param->recovery_failed_times_total = 0;
            pselect_param->recovery_failed_times_in_bottom_layer = 0;
            pselect_param->bootfail_with_safe_mode_failed_times = 0;
        }
        else
        {
            pselect_param->recovery_failed_times_total++;
            if (pselect_param->latest_recovery_record[i].boot_fail_stage < NATIVE_STAGE_START)
            {
                pselect_param->recovery_failed_times_in_bottom_layer++;
            }

            /* check the count of boot fail has safe mode or not */
            if (bfr_bootfail_has_safe_mode_recovery_method(pselect_param->latest_recovery_record[i].boot_fail_no))
            {
                pselect_param->bootfail_with_safe_mode_failed_times++;
            }
        }
    }

    pselect_param->recovery_failed_times_total++;
    if (pselect_param->cur_boot_fail_stage < NATIVE_STAGE_START)
    {
        pselect_param->recovery_failed_times_in_bottom_layer++;
    }

    pselect_param->recovery_failed_times_in_application = pselect_param->recovery_failed_times_total
        - pselect_param->recovery_failed_times_in_bottom_layer;
    BFMR_PRINT_KEY_INFO("bf_total_times:%d bf_bottom_layer_times: %d bf_app_times: %d\n",
        pselect_param->recovery_failed_times_total, pselect_param->recovery_failed_times_in_bottom_layer,
        pselect_param->recovery_failed_times_in_application);

    /* It is the first failure */
    pselect_param->has_fixed_recovery_method = bfr_boot_fail_has_fixed_recovery_method(pselect_param);
    if (0 == pselect_param->latest_valid_recovery_record_count)
    {
        BFMR_PRINT_KEY_INFO("System has no valid recovery record, the boot fail occurs in [%s]\n",
            (pselect_param->cur_boot_fail_stage < NATIVE_STAGE_START) ? ("Bottom layer") : ("APP"));
        pselect_param->recovery_method = (pselect_param->has_fixed_recovery_method)
            ? (pselect_param->fixed_recovery_method) : (FRM_REBOOT);
        pselect_param->need_not_parse_recovery_method_further = true;
        BFMR_PRINT_KEY_INFO("recovery_method: %d\n", pselect_param->recovery_method);
    }

    return 0;
}

static int bfr_select_recovery_method_with_safe_mode(
    bfr_recovery_method_select_param_t *pselect_param)
{
    if (NULL == pselect_param)
    {
        BFMR_PRINT_INVALID_PARAMS("pselect_param.\n");
        return -1;
    }

    if (0 != bfr_parse_boot_fail_info(pselect_param))
    {
        BFMR_PRINT_ERR("Failed to parse boot fail info!\n");
        return -1;
    }

    if (pselect_param->need_not_parse_recovery_method_further)
    {
        return 0;
    }

    if (bfr_bootfail_has_safe_mode_recovery_method(pselect_param->cur_boot_fail_no))
    {
        switch (pselect_param->recovery_failed_times_total)
        {
        case BF_1_TIME:
            {
                pselect_param->recovery_method = FRM_REBOOT;
                break;
            }
        case BF_2_TIME:
            {
                switch (pselect_param->bootfail_with_safe_mode_failed_times)
                {
                case BF_1_TIME:
                    {
                        pselect_param->recovery_method = FRM_ENTER_SAFE_MODE;
                        break;
                    }
                default:
                    {
                        pselect_param->recovery_method = FRM_REBOOT;
                        break;
                    }
                }
                break;
            }
        case BF_3_TIME:
            {
                switch (pselect_param->bootfail_with_safe_mode_failed_times)
                {
                case BF_0_TIME:
                    {
                        pselect_param->recovery_method = FRM_REBOOT;
                        break;
                    }
                case BF_1_TIME:
                    {
                        pselect_param->recovery_method = FRM_ENTER_SAFE_MODE;
                        break;
                    }
                case BF_2_TIME:
                default:
                    {
                        pselect_param->recovery_method = FRM_GOTO_ERECOVERY_FACTORY_RESET;
                        break;
                    }
                }
                break;
            }
        case BF_4_TIME:
        default:
            {
                pselect_param->recovery_method = FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY;
                break;
            }
        }
    }
    else
    {
        switch (pselect_param->recovery_failed_times_total)
        {
        case BF_1_TIME:
        case BF_2_TIME:
            {
                pselect_param->recovery_method = (pselect_param->has_fixed_recovery_method)
                    ? (pselect_param->fixed_recovery_method) : (FRM_REBOOT);
                break;
            }
        case BF_3_TIME:
            {
                pselect_param->recovery_method = (pselect_param->has_fixed_recovery_method)
                    ? (pselect_param->fixed_recovery_method) : (FRM_GOTO_ERECOVERY_FACTORY_RESET);
                break;
            }
        case BF_4_TIME:
        default:
            {
                pselect_param->recovery_method = (pselect_param->has_fixed_recovery_method)
                    ? (pselect_param->fixed_recovery_method) : FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY;
                break;
            }
        }
    }

    return 0;
}


static int bfr_select_recovery_method_without_safe_mode(
    bfr_recovery_method_select_param_t *pselect_param)
{
    if (NULL == pselect_param)
    {
        BFMR_PRINT_INVALID_PARAMS("pselect_param.\n");
        return -1;
    }

    if (0 != bfr_parse_boot_fail_info(pselect_param))
    {
        BFMR_PRINT_ERR("Failed to parse boot fail info!\n");
        return -1;
    }

    if (pselect_param->need_not_parse_recovery_method_further)
    {
        return 0;
    }

    switch (pselect_param->recovery_failed_times_in_bottom_layer)
    {
    case BF_0_TIME:
        {
            if (pselect_param->has_fixed_recovery_method)
            {
                BFMR_PRINT_KEY_INFO("[Bottom layer has no boot fail] APP has fixed recovery method: [%d]\n",
                    pselect_param->fixed_recovery_method);
                pselect_param->recovery_method = pselect_param->fixed_recovery_method;
                break;
            }

            switch (pselect_param->recovery_failed_times_in_application)
            {
            case BF_1_TIME:
                {
                    BFMR_PRINT_KEY_INFO("[Bottom layer has no boot fail] FRM_REBOOT\n");
                    pselect_param->recovery_method = FRM_REBOOT;
                    break;
                }
            case BF_2_TIME:
                {
                    BFMR_PRINT_KEY_INFO("[Bottom layer has no boot fail] FRM_GOTO_ERECOVERY_DEL_FILES_FOR_BF\n");
                    pselect_param->recovery_method = FRM_GOTO_ERECOVERY_DEL_FILES_FOR_BF;
                    break;
                }
            case BF_3_TIME:
                {
                    BFMR_PRINT_KEY_INFO("[Bottom layer has no boot fail] FRM_GOTO_ERECOVERY_FACTORY_RESET\n");
                    pselect_param->recovery_method = FRM_GOTO_ERECOVERY_FACTORY_RESET;
                    break;
                }
            case BF_4_TIME:
            default:
                {
                    BFMR_PRINT_KEY_INFO("[Bottom layer has no boot fail] FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY\n");
                    pselect_param->recovery_method = FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY;
                    break;
                }
            }
            break;
        }
    case BF_1_TIME:
    case BF_2_TIME:
        {
            if (pselect_param->has_fixed_recovery_method)
            {
                BFMR_PRINT_KEY_INFO("[Bottom layer has 1 or 2 boot fail] APP has fixed recovery method\n");
                pselect_param->recovery_method = pselect_param->fixed_recovery_method;
                break;
            }

            switch (pselect_param->recovery_failed_times_in_application)
            {
            case BF_0_TIME:
                {
                    BFMR_PRINT_KEY_INFO("[APP has no Bootfail] FRM_REBOOT\n");
                    pselect_param->recovery_method = FRM_REBOOT;
                    break;
                }
            case BF_1_TIME:
                {
                    BFMR_PRINT_KEY_INFO("[Bottom layer has boot fail] FRM_REBOOT\n");
                    pselect_param->recovery_method = FRM_REBOOT;
                    break;
                }
            case BF_2_TIME:
                {
                    BFMR_PRINT_KEY_INFO("[Bottom layer has boot fail] FRM_GOTO_ERECOVERY_DEL_FILES_FOR_BF\n");
                    pselect_param->recovery_method = FRM_GOTO_ERECOVERY_DEL_FILES_FOR_BF;
                    break;
                }
            case BF_3_TIME:
            default:
                {
                    BFMR_PRINT_KEY_INFO("[Bottom layer has boot fail] FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_FACTORY_RESET\n");
                    pselect_param->recovery_method = FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_FACTORY_RESET;
                    break;
                }
            }
            break;
        }
    case BF_3_TIME:
    default:
        {
            BFMR_PRINT_KEY_INFO("[APP has no boot fail] FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY\n");
            pselect_param->recovery_method = FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY;
            break;
        }
    }

    return 0;
}


static bfr_boot_fail_stage_e bfr_get_main_boot_fail_stage(unsigned int boot_fail_stage)
{
    if (bfmr_is_bl1_stage(boot_fail_stage) || bfmr_is_bl2_stage(boot_fail_stage))
    {
        return BFS_BOOTLOADER;
    }
    else if (bfmr_is_kernel_stage(boot_fail_stage))
    {
        return BFS_KERNEL;
    }
    else
    {
        return BFS_APP;
    }
}


static int bfr_run_recovery_method(bfr_recovery_method_select_param_t *pselect_param)
{
    if (NULL == pselect_param)
    {
        BFMR_PRINT_INVALID_PARAMS("pselect_param.\n");
        return -1;
    }

    switch (pselect_param->recovery_method)
    {
#if defined(CONFIG_USE_AB_SYSTEM)
    case FRM_GOTO_B_SYSTEM:
        {
            bfr_goto_another_system();
            break;
        }
#endif
    case FRM_GOTO_ERECOVERY_DEL_FILES_FOR_BF:
    case FRM_GOTO_ERECOVERY_DEL_FILES_FOR_NOSPC:
    case FRM_GOTO_ERECOVERY_FACTORY_RESET:
    case FRM_GOTO_ERECOVERY_FORMAT_DATA_PART:
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_DEL_FILES:
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_FACTORY_RESET:
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY:
    case FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA:
    case FRM_FACTORY_RESET_AFTER_DOWNLOAD_RECOVERY:
        {
            bfmr_rrecord_misc_msg_param_t reason_param;

            memset((void *)&reason_param, 0, sizeof(reason_param));
            memcpy((void *)reason_param.command, (void *)BFR_ENTER_ERECOVERY_CMD,
                strlen(BFR_ENTER_ERECOVERY_CMD)+1);/*[false alarm]:strlen*/
            reason_param.enter_erecovery_reason = EER_BOOT_FAIL_SOLUTION;
            reason_param.enter_erecovery_reason_number = bfr_get_enter_erecovery_reason(pselect_param->cur_boot_fail_no);
            reason_param.boot_fail_stage_for_erecovery = bfr_get_main_boot_fail_stage(pselect_param->cur_boot_fail_stage);
            reason_param.recovery_method = (unsigned int)pselect_param->recovery_method;
            reason_param.boot_fail_no = (unsigned int)pselect_param->cur_boot_fail_no;
            (void)bfmr_write_rrecord_misc_msg(&reason_param);
            (void)bfr_set_misc_msg_for_erecovery();
            break;
        }
    case FRM_NOTIFY_USER_RECOVERY_FAILURE:
        {
            break;
        }
    case FRM_ENTER_SAFE_MODE:
        {
            bfmr_rrecord_misc_msg_param_t misc_msg;

            BFMR_PRINT_KEY_INFO("FRM_ENTER_SAFE_MODE!\n");
            memset((void *)&misc_msg, 0, sizeof(misc_msg));
            memcpy((void *)misc_msg.command, (void *)BFR_ENTER_SAFE_MODE_CMD,
                strlen(BFR_ENTER_SAFE_MODE_CMD)+1);/*[false alarm]:strlen*/
            (void)bfmr_write_rrecord_misc_msg(&misc_msg);
            break;
        }
    default:
        {
            return 0;
        }
    }

    return 0;
}


static bfr_recovery_method_running_status_e bfr_init_recovery_method_running_status(
    bfr_recovery_method_e recovery_method)
{
    switch (recovery_method)
    {
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY:
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_DEL_FILES:
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_FACTORY_RESET:
    case FRM_GOTO_ERECOVERY_LOWLEVEL_FORMAT_DATA:
    case FRM_FACTORY_RESET_AFTER_DOWNLOAD_RECOVERY:
    case FRM_GOTO_ERECOVERY_FACTORY_RESET:
    case FRM_GOTO_ERECOVERY_FORMAT_DATA_PART:
    case FRM_GOTO_ERECOVERY_DEL_FILES_FOR_BF:
    case FRM_GOTO_ERECOVERY_DEL_FILES_FOR_NOSPC:
        {
            return RMRSC_ERECOVERY_BOOT_FAILED;
        }
    default:
        {
            break;
        }
    }

    return RMRSC_EXEC_COMPLETED;
}


static bfr_recovery_method_run_result_e bfr_init_recovery_method_run_result(
    bfr_recovery_method_e recovery_method)
{
    switch (recovery_method)
    {
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY:
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_DEL_FILES:
    case FRM_GOTO_ERECOVERY_DOWNLOAD_RECOVERY_AND_FACTORY_RESET:
        {
            return RMRR_FAILED;
        }
    default:
        {
            break;
        }
    }

    return RMRR_SUCCESS;
}


static int bfr_init_local_recovery_record_param(void)
{
    int i = 0;
    int count = sizeof(s_rrecord_param) / sizeof(s_rrecord_param[0]);

    /* return right now if the memory of the local param has been inited */
    if (NULL != s_rrecord_param[0].buf)
    {
        return 0;
    }

    memset((void *)&s_rrecord_param, 0, sizeof(s_rrecord_param));
    for (i = 0; i < count; i++)
    {
        s_rrecord_param[i].buf = (unsigned char *)bfmr_malloc(BFR_EACH_RECORD_PART_SIZE);
        if (NULL == s_rrecord_param[i].buf)
        {
            BFMR_PRINT_ERR("bfmr_malloc failed!\n");
            bfr_release_local_recovery_record_param();
            return -1;
        }
        memset((void *)s_rrecord_param[i].buf, 0, BFR_EACH_RECORD_PART_SIZE);
        s_rrecord_param[i].buf_size = BFR_EACH_RECORD_PART_SIZE;
        s_rrecord_param[i].part_offset = (0 == i)
            ? (BFR_RRECORD_FIRST_PART_OFFSET) : ((1 == i)
            ? (BFR_RRECORD_SECOND_PART_OFFSET) : (BFR_RRECORD_THIRD_PART_OFFSET));
    }

    return 0;
}


static int bfr_release_local_recovery_record_param(void)
{
    int i = 0;
    int count = sizeof(s_rrecord_param) / sizeof(s_rrecord_param[0]);

    for (i = 0; i < count; i++)
    {
        bfmr_free(s_rrecord_param[i].buf);
        s_rrecord_param[i].buf = NULL;        
    }

    return 0;
}


static int bfr_read_recovery_record_to_local_buf(void)
{
    int i = 0;
    int ret = -1;
    int count = sizeof(s_rrecord_param) / sizeof(s_rrecord_param[0]);
    char *dev_path = NULL;
    static bool recovery_record_has_been_read = false;

    if (recovery_record_has_been_read)
    {
        return 0;
    }

    /* 1. read recovery record here firstly */
    if (0 != bfr_init_local_recovery_record_param())
    {
        BFMR_PRINT_ERR("Failed to init local recovery record read param!\n");
        goto __out;
    }

    /* 2. get path of the rrecord path */
    ret = bfr_get_full_path_of_rrecord_part(&dev_path);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("find the full path of rrecord part failed!\n");
        goto __out;
    }

    /* 3. read recovery record to local */
    for (i = 0; i < count; i++)
    {
        ret = bfmr_read_emmc_raw_part(dev_path, s_rrecord_param[i].part_offset,
            s_rrecord_param[i].buf, s_rrecord_param[i].buf_size);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("read [%s] failed!\n", dev_path);
            break;
        }
    }

    /* 4. update the lcoal static flag */
    recovery_record_has_been_read = true;

__out:
    if (NULL != dev_path)
    {
        bfmr_free(dev_path);
    }

    return ret;
}


static int bfr_verify_recovery_record(void)
{
    bfr_recovery_record_header_t *pfirst_header = NULL;
    bfr_recovery_record_header_t *psecond_header = NULL;
    int ret = -1;
    int i = 0;
    int valid_record_idx = -1;
    int count = sizeof(s_rrecord_param) / sizeof(s_rrecord_param[0]);
    int record_damaged = 0;
    char *dev_path = NULL;

    /* 1. check the magic number */
    pfirst_header = (bfr_recovery_record_header_t *)s_rrecord_param[0].buf;
    psecond_header = (bfr_recovery_record_header_t *)s_rrecord_param[1].buf;
    if ((BFR_RRECORD_MAGIC_NUMBER != pfirst_header->magic_number)
        && (BFR_RRECORD_MAGIC_NUMBER != psecond_header->magic_number))
    {
        /* maybe it is the first time teh rrecord part hae been used */
        return bfr_init_recovery_record_header();
    }

    /* 2. check the crc32 value */
    for (i = 0; i < count; i++)
    {
        unsigned int local_crc32 = 0x0;
        bfr_recovery_record_header_t *pheader = (bfr_recovery_record_header_t *)s_rrecord_param[i].buf;

        local_crc32 = bfmr_get_crc32(s_rrecord_param[i].buf + (unsigned int)sizeof(pheader->crc32),
            s_rrecord_param[i].buf_size - (unsigned int)sizeof(pheader->crc32));
        if (local_crc32 != pheader->crc32)
        {
            BFMR_PRINT_ERR("CRC check failed! orig_crc32:0x%08x local_crc32: 0x%08x\n",
                pheader->crc32, local_crc32);
            s_rrecord_param[i].record_damaged = 1;
            record_damaged++;
        }
        else
        {
            s_rrecord_param[i].record_damaged = 0;
            valid_record_idx = i;
        }
    }

    /* 3. no valid record has been found, init the record header and return here */
    if (valid_record_idx < 0)
    {
        BFMR_PRINT_ERR("There is no valid recovery record!\n");
        return bfr_init_recovery_record_header();
    }

    /* 4. malloc memory for the full path buffer of rrecord */
    if (0 != record_damaged)
    {
        ret = bfr_get_full_path_of_rrecord_part(&dev_path);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("find the full path of rrecord part failed!\n");
            goto __out;
        }
    }

    /* 5. correct the recovery record */
    ret = 0; /* set the value of ret as 0 here because the maybe all the parts are valid */
    for (i = 0; i < count; i++)
    {
        if (0 == s_rrecord_param[i].record_damaged)
        {
            continue;
        }

        memcpy((void *)s_rrecord_param[i].buf, (void *)s_rrecord_param[valid_record_idx].buf,
            s_rrecord_param[valid_record_idx].buf_size);
        ret = bfmr_write_emmc_raw_part(dev_path, s_rrecord_param[i].part_offset,
            s_rrecord_param[i].buf, s_rrecord_param[i].buf_size);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("write data to [%s] failed!\n", dev_path);
            break;
        }
        s_rrecord_param[i].record_damaged = 0;
    }

__out:
    if (NULL != dev_path)
    {
        bfmr_free(dev_path);
    }

    return ret;
}


static int bfr_init_recovery_record_header(void)
{
    bfr_recovery_record_header_t *pheader = NULL;
    unsigned int header_size = (unsigned int)sizeof(bfr_recovery_record_header_t);
    int i = 0;
    int ret = -1;
    int count = sizeof(s_rrecord_param) / sizeof(s_rrecord_param[0]);
    char *dev_path = NULL;

    ret = bfr_get_full_path_of_rrecord_part(&dev_path);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("find the full path of rrecord part failed!\n");
        goto __out;
    }

    for (i = 0; i < count; i++)
    {
        memset((void *)s_rrecord_param[i].buf, 0, s_rrecord_param[i].buf_size);
        s_rrecord_param[i].record_damaged = 0;
        pheader = (bfr_recovery_record_header_t *)s_rrecord_param[i].buf;
        pheader->magic_number = BFR_RRECORD_MAGIC_NUMBER;
        pheader->safe_mode_enable_flag = (bfr_safe_mode_has_been_enabled()) ? (BFR_SAFE_MODE_ENABLE_FLAG) : (0x0);
        pheader->record_count = (s_rrecord_param[i].buf_size - header_size) / sizeof(bfr_recovery_record_t);
        pheader->crc32 = bfmr_get_crc32(s_rrecord_param[i].buf + (unsigned int)sizeof(pheader->crc32),
            s_rrecord_param[i].buf_size - (unsigned int)sizeof(pheader->crc32));
        ret = bfmr_write_emmc_raw_part(dev_path, s_rrecord_param[i].part_offset,
            (char *)s_rrecord_param[i].buf, s_rrecord_param[i].buf_size);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("Write part [%s] failed [ret = %d]!\n", dev_path, ret);
            break;
        }
    }

__out:
    if (NULL != dev_path)
    {
        bfmr_free(dev_path);
    }

    return ret;
}


static int bfr_read_and_verify_recovery_record(void)
{
    int ret = -1;
    static bool recovery_record_has_been_read_and_verified = false;

    if (recovery_record_has_been_read_and_verified)
    {
        return 0;
    }

     /* 1. read recovery record to local buffer */
    ret = bfr_read_recovery_record_to_local_buf();
    if (0 != ret)
    {
        BFMR_PRINT_ERR("bfr_read_recovery_record_to_local_buf failed!\n");
        return -1;
    }

    /* 2. verify the recovery record */
    ret = bfr_verify_recovery_record();
    if (0 != ret)
    {
        BFMR_PRINT_ERR("bfr_verify_recovery_record failed!\n");
        return -1;
    }

    /* 3. update the local flag */
    recovery_record_has_been_read_and_verified = true;

    return ret;
}


static int bfr_read_recovery_record(bfr_recovery_record_t *precord,
    int record_count_to_read,
    int *record_count_actually_read)
{
    unsigned int buf_size = 0;
    unsigned int header_size = (unsigned int)sizeof(bfr_recovery_record_header_t);
    unsigned int record_size = (unsigned int)sizeof(bfr_recovery_record_t);
    bfr_recovery_record_header_t *precord_header = NULL;

    if (unlikely((NULL == precord) || (NULL == record_count_actually_read)))
    {
        BFMR_PRINT_INVALID_PARAMS("precord or record_count_actually_read.\n");
        return -1;
    }

    /* 1. init the record_count_actually_read as 0 */
    *record_count_actually_read = 0;

    /* 2. read and verify the recovery record */
    if (0 != bfr_read_and_verify_recovery_record())
    {
        BFMR_PRINT_ERR("Failed to read and verify the recovery record!\n");
        return -1;
    }

    /* 3. read recovery record */
    precord_header = (bfr_recovery_record_header_t *)s_rrecord_param[0].buf;
    *record_count_actually_read = BFMR_MIN(precord_header->boot_fail_count,
        BFMR_MIN(precord_header->record_count, record_count_to_read));
    buf_size = (*record_count_actually_read) * sizeof(bfr_recovery_record_t);
    if (precord_header->next_record_idx >= *record_count_actually_read)
    {
        memcpy((void *)precord, (void *)(s_rrecord_param[0].buf
            + header_size + (precord_header->next_record_idx
            - *record_count_actually_read) * record_size), buf_size);/*lint !e679 */
    }
    else
    {
        unsigned int count_in_the_end = *record_count_actually_read - precord_header->next_record_idx;
        unsigned int count_in_the_begin = precord_header->next_record_idx;

        memcpy((void *)precord, (void *)(s_rrecord_param[0].buf + header_size
            + (precord_header->record_count - count_in_the_end) * record_size), count_in_the_end * record_size); /*lint !e647 !e679 */
        memcpy((void *)((char *)precord + count_in_the_end * record_size),
            (void *)(s_rrecord_param[0].buf + header_size), count_in_the_begin * record_size);
    }

    return 0;
}


static int bfr_create_recovery_record(bfr_recovery_record_t *precord)
{
    int i = 0;
    int ret = -1;
    int count = sizeof(s_rrecord_param) / sizeof(s_rrecord_param[0]);
    bfr_recovery_record_header_t *pheader = NULL;
    unsigned int header_size = (unsigned int)sizeof(bfr_recovery_record_header_t);
    unsigned int record_size = (unsigned int)sizeof(bfr_recovery_record_t);
    char *dev_path = NULL;

    if (0 != bfr_get_full_path_of_rrecord_part(&dev_path))
    {
        BFMR_PRINT_ERR("find the full path of rrecord part failed!\n");
        goto __out;
    }

    for (i = 0; i < count; i++)
    {
        pheader = (bfr_recovery_record_header_t *)s_rrecord_param[i].buf;
        memcpy((void *)(s_rrecord_param[i].buf + header_size
            + pheader->next_record_idx * record_size), (void *)precord, record_size);
        pheader->boot_fail_count++;
        pheader->next_record_idx++;
        pheader->record_count_before_boot_success++;
        if (pheader->next_record_idx >= pheader->record_count)
        {
            pheader->next_record_idx = 0;
            pheader->last_record_idx = pheader->record_count - 1;
        }
        else
        {
            pheader->last_record_idx = pheader->next_record_idx - 1;
        }
        pheader->safe_mode_enable_flag = (bfr_safe_mode_has_been_enabled()) ? (BFR_SAFE_MODE_ENABLE_FLAG) : (0x0);
        pheader->crc32 = bfmr_get_crc32(s_rrecord_param[i].buf + (unsigned int)sizeof(pheader->crc32),
            s_rrecord_param[i].buf_size - (unsigned int)sizeof(pheader->crc32));

        /* 1. write header */
        ret = bfmr_write_emmc_raw_part(dev_path, (unsigned long long)s_rrecord_param[i].part_offset,
            (char *)pheader, (unsigned long long)header_size);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("Write recovery record header to [%s] failed!\n", dev_path);
            goto __out;
        }

        /* 2. write record */
        ret = bfmr_write_emmc_raw_part(dev_path, (unsigned long long)((unsigned long long)s_rrecord_param[i].part_offset
            + (unsigned long long)header_size + pheader->last_record_idx * (unsigned long long)record_size),
            (char *)precord, (unsigned long long)record_size);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("Write recovery record to [%s] failed!\n", dev_path);
            goto __out;
        }
    }

__out:
    if (NULL != dev_path)
    {
        bfmr_free(dev_path);
    }

    return ret;
}


int bfr_get_hardware_fault_times(bfmr_get_hw_fault_info_param_t *pfault_info_param)
{
    int i = 0;
    bfr_recovery_record_header_t *pheader = NULL;
    bfr_recovery_record_t *precord = NULL;
    int fault_times = 0;
    int record_count = 0;
    char *bootfail_detail = NULL;

    if (unlikely((NULL == pfault_info_param)))
    {
        BFMR_PRINT_INVALID_PARAMS("pfault_info_param.\n");
        return -1;
    }

    /* 1. read and verify the recovery record */
    if (0 != bfr_read_and_verify_recovery_record())
    {
        BFMR_PRINT_ERR("Failed to read and verify the recovery record!\n");
        return -1;
    }

    pheader = (bfr_recovery_record_header_t *)(s_rrecord_param[0].buf);
    if (NULL == pheader)
    {
        BFMR_PRINT_ERR("the local rrecord has not been read successfully!\n");
        return -1;
    }

    record_count = BFMR_MIN(pheader->boot_fail_count, pheader->record_count);
    for (i = 0; i < record_count; i++)
    {
        precord = (bfr_recovery_record_t *)(s_rrecord_param[0].buf + sizeof(bfr_recovery_record_header_t) + i * sizeof(bfr_recovery_record_t));
        if (BFM_HARDWARE_FAULT != precord->boot_fail_no)
        {
            continue;
        }

        bootfail_detail = precord->bootfail_detail;
        bootfail_detail[sizeof(precord->bootfail_detail) - 1] = '\0';
        switch (pfault_info_param->fault_stage)
        {
        case HW_FAULT_STAGE_DURING_BOOTUP:
            {
                if (STAGE_BOOT_SUCCESS == precord->boot_fail_stage)
                {
                    break;
                }

                if (0 == strcmp(bootfail_detail, pfault_info_param->hw_excp_info.ocp_excp_info.ldo_num))
                {
                    fault_times++;
                }
                break;
            }
        case HW_FAULT_STAGE_AFTER_BOOT_SUCCESS:
            {
                if (STAGE_BOOT_SUCCESS != precord->boot_fail_stage)
                {
                    break;
                }

                if (0 == strcmp(bootfail_detail, pfault_info_param->hw_excp_info.ocp_excp_info.ldo_num))
                {
                    fault_times++;
                }
                break;
            }
        default:
            {
                break;
            }
        }
    }

    return fault_times;
}


int bfr_get_real_recovery_info(bfr_real_recovery_info_t *preal_recovery_info)
{
    int i = 0;
    int ret = -1;
    int offset = 0;
    int record_count_actually_read = 0;
    size_t count = 0;
    bfr_recovery_record_t *precovery_record = NULL;

    if (NULL == preal_recovery_info)
    {
        BFMR_PRINT_INVALID_PARAMS("preal_recovery_info.\n");
        return -1;
    }

    /* 1. read recovery record to local */
    count = (int)(sizeof(preal_recovery_info->recovery_method) / sizeof(preal_recovery_info->recovery_method[0]));
    precovery_record = (bfr_recovery_record_t *)bfmr_malloc(sizeof(bfr_recovery_record_t) * count);
    if (NULL == precovery_record)
    {
        BFMR_PRINT_KEY_INFO("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(precovery_record, 0, sizeof(bfr_recovery_record_t) * count);

    ret = bfr_read_recovery_record(precovery_record, count, &record_count_actually_read);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("Failed to read the recovery record!\n");
        goto __out;
    }

    memset(preal_recovery_info, 0, sizeof(bfr_real_recovery_info_t));
    preal_recovery_info->record_count = BFMR_MIN(s_rrecord_param[0].record_count_before_boot_success,
        record_count_actually_read); /* record_count_actually_read <= count of preal_recovery_info->recovery_method */
    if (preal_recovery_info->record_count <= 0)
    {
        BFMR_PRINT_ERR("There're no valid recovery record, record_count_before_boot_success:%d, record_count_actually_read:%d\n",
            s_rrecord_param[0].record_count_before_boot_success, record_count_actually_read);
        goto __out;
    }

    offset = (s_rrecord_param[0].record_count_before_boot_success < record_count_actually_read)
        ? (record_count_actually_read - s_rrecord_param[0].record_count_before_boot_success) : (0);
    for (i = 0; i < preal_recovery_info->record_count; i++)
    {
        preal_recovery_info->recovery_method[i] = precovery_record[i + offset].recovery_method;
        preal_recovery_info->recovery_method_original[i] = precovery_record[i + offset].recovery_method_original;/*lint !e679 */
        preal_recovery_info->boot_fail_no[i] = precovery_record[i + offset].boot_fail_no;/*lint !e679 */
        preal_recovery_info->boot_fail_stage[i] = precovery_record[i + offset].boot_fail_stage;/*lint !e679 */
        preal_recovery_info->boot_fail_time[i] = precovery_record[i + offset].boot_fail_time;/*lint !e679 */
        preal_recovery_info->boot_fail_rtc_time[i] = precovery_record[i + offset].boot_fail_detected_time;
    }
    BFMR_PRINT_KEY_INFO("There're %d recovery record\n", preal_recovery_info->record_count);

__out:
    if (NULL != precovery_record)
    {
        bfmr_free(precovery_record);
    }

    return ret;
}


/* this function need not be realized in bootloader, it should be realized in kernel and erecovery */
static int bfr_renew_recovery_record(bfr_recovery_record_t *precord)
{
    int i = 0;
    int ret = -1;
    int count = sizeof(s_rrecord_param) / sizeof(s_rrecord_param[0]);
    int valid_record_idx = -1;
    bfr_recovery_record_t recovery_record;
    unsigned int header_size = (unsigned int)sizeof(bfr_recovery_record_header_t);
    unsigned int record_size = (unsigned int)sizeof(bfr_recovery_record_t);
    unsigned int local_crc32 = 0x0;
    bool system_boot_fail_last_time = false;
    char *dev_path = NULL;

    if (NULL == precord)
    {
        BFMR_PRINT_INVALID_PARAMS("precord.\n");
        return -1;
    }

    /* 1. read recovery record to local */
    if (0 != bfr_read_and_verify_recovery_record())
    {
        BFMR_PRINT_ERR("Failed to read and verify the recovery record!\n");
        goto __out;
    }

    /* 2. get the full path of the valid rrecord part */
    if (0 != bfr_get_full_path_of_rrecord_part(&dev_path))
    {
        BFMR_PRINT_ERR("find the full path of rrecord part failed!\n");
        goto __out;
    }

    /* 3. chech if there're valid recovery record or not */
    if (0 == ((bfr_recovery_record_header_t *)s_rrecord_param[i].buf)->boot_fail_count)
    {
        BFMR_PRINT_KEY_INFO("There is no valid recovery record!\n");
        ret = 0;
        goto __out;
    }

    /* 4. update recovery record*/
    for (i = 0; i < count; i++)
    {
        int record_count_before_boot_success = 0;
        bfr_recovery_record_header_t *pheader = (bfr_recovery_record_header_t *)s_rrecord_param[i].buf;

        memset((void *)&recovery_record, 0, record_size);
        memcpy((void *)&recovery_record, (void *)(s_rrecord_param[i].buf + header_size
            + pheader->last_record_idx * record_size), record_size);/*lint !e679 */
        if (BOOT_FAIL_RECOVERY_SUCCESS != recovery_record.recovery_result)
        {
            system_boot_fail_last_time = true;
        }
        recovery_record.recovery_result = precord->recovery_result;
        memcpy((void *)(s_rrecord_param[i].buf + header_size + pheader->last_record_idx * record_size),
            (void *)&recovery_record, record_size);
        record_count_before_boot_success = pheader->record_count_before_boot_success;
        pheader->record_count_before_boot_success = (BOOT_FAIL_RECOVERY_SUCCESS == precord->recovery_result)
            ? (0) : (pheader->record_count_before_boot_success);
        pheader->crc32 = bfmr_get_crc32(s_rrecord_param[i].buf + (unsigned int)sizeof(pheader->crc32),
            s_rrecord_param[i].buf_size - (unsigned int)sizeof(pheader->crc32));
        s_rrecord_param[i].record_count_before_boot_success = record_count_before_boot_success;
    }

    /* 5. save recovery record */
    ret = 0;
    for (i = 0; i < count; i++)
    {
        if (system_boot_fail_last_time || (0 != s_rrecord_param[i].record_damaged))
        {
            ret = bfmr_write_emmc_raw_part(dev_path, s_rrecord_param[i].part_offset,
                (char *)s_rrecord_param[i].buf, s_rrecord_param[i].buf_size);
            if (0 != ret)
            {
                BFMR_PRINT_ERR("Write [%s] failed![errno: %d]\n", dev_path, ret);
                goto __out;
            }
        }
    }

__out:
    if (NULL != dev_path)
    {
        bfmr_free(dev_path);
    }

    return ret;
}


char* bfr_get_recovery_method_desc(int recovery_method)
{
    int i = 0;
    int count = sizeof(s_recovery_method_desc) / sizeof(s_recovery_method_desc[0]);

    for (i = 0; i < count; i++)
    {
        if ((bfr_recovery_method_e)recovery_method == s_recovery_method_desc[i].recovery_method)
        {
            return s_recovery_method_desc[i].desc;
        }
    }

    return "unknown";
}


/**
    @function: void boot_status_notify(int boot_success)
    @brief: when the system bootup successfully, the BFM must call this
        function to notify the BFR, and the BFM was notified by the BFD.

    @param: boot_success.

    @return: none.

    @note: this fuction only need be initialized in kernel.
*/
void boot_status_notify(int boot_success)
{
    bfr_recovery_record_t recovery_record;

    BFMR_PRINT_KEY_INFO("Recovery boot fail Successfully!\n");
    memset((void *)&recovery_record, 0, sizeof(recovery_record));
    recovery_record.recovery_result = (0 == boot_success) ? (BOOT_FAIL_RECOVERY_FAILURE)
        : (BOOT_FAIL_RECOVERY_SUCCESS);
    (void)bfr_renew_recovery_record(&recovery_record);
}


static unsigned int bfr_get_bootfail_uptime(void)
{
    long boottime = bfmr_get_bootup_time();

    return (0L == boottime) ? ((unsigned int)(-1)) : (unsigned int)boottime;
}


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
    char *args)
{
    int ret = -1;
    bfr_recovery_record_t cur_recovery_record;
    bfr_recovery_method_e recovery_method = FRM_DO_NOTHING;
    bfr_recovery_method_select_param_t *pselect_param = NULL;

    BFMR_PRINT_KEY_INFO("boot_fail_stage:%x, boot_fail_no: %x, suggested_recovery_method: %d!\n",
        boot_fail_stage, boot_fail_no, (int)suggested_recovery_method);

    /* 1. malloc memory here instead in case of stack overflow */
    pselect_param = (bfr_recovery_method_select_param_t *)bfmr_malloc(sizeof(bfr_recovery_method_select_param_t));
    if (NULL == pselect_param)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed for boot fail [0x%x]!\n", (unsigned int)boot_fail_no);
        return FRM_DO_NOTHING;
    }
    memset((void *)pselect_param, 0, sizeof(bfr_recovery_method_select_param_t)); /* YOU MUST DO THIS FIRSTLY */

    /* 2. read recovery record */
    ret = bfr_read_recovery_record(pselect_param->latest_recovery_record, sizeof(pselect_param->latest_recovery_record)
        / sizeof(pselect_param->latest_recovery_record[0]), &(pselect_param->latest_valid_recovery_record_count));
    if (0 != ret)
    {
        BFMR_PRINT_ERR("Failed to read recovery record for boot fail [0x%x]!\n", (unsigned int)boot_fail_no);
        goto __out;
    }

    /* 3. select recovery method */
    pselect_param->cur_boot_fail_stage = boot_fail_stage;
    pselect_param->cur_boot_fail_no = boot_fail_no;
    pselect_param->suggested_recovery_method = suggested_recovery_method;
    pselect_param->bootfail_detail_info = args;
    ret = bfr_safe_mode_has_been_enabled() ? bfr_select_recovery_method_with_safe_mode(pselect_param)
        : bfr_select_recovery_method_without_safe_mode(pselect_param);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("Failed to select recovery method for boot fail [0x%x]!\n", (unsigned int)boot_fail_no);
        goto __out;
    }

    /* 4. save recovery record */
    memset((void *)&cur_recovery_record, 0, sizeof(cur_recovery_record));
    cur_recovery_record.boot_fail_detected_time = boot_fail_detected_time;
    cur_recovery_record.boot_fail_stage = boot_fail_stage;
    cur_recovery_record.boot_fail_no = boot_fail_no;
    cur_recovery_record.recovery_method = (DO_NOTHING == suggested_recovery_method) ? FRM_DO_NOTHING : pselect_param->recovery_method;
    cur_recovery_record.running_status_code = bfr_init_recovery_method_running_status(pselect_param->recovery_method);
    cur_recovery_record.method_run_result = bfr_init_recovery_method_run_result(pselect_param->recovery_method);
    cur_recovery_record.recovery_result = BOOT_FAIL_RECOVERY_FAILURE;
    cur_recovery_record.recovery_method_original = pselect_param->recovery_method;
    cur_recovery_record.boot_fail_time = bfr_get_bootfail_uptime();
    if (NULL != args)
    {
        memcpy(cur_recovery_record.bootfail_detail, args, BFMR_MIN(sizeof(cur_recovery_record.bootfail_detail) - 1, strlen(args)));
    }
    ret = bfr_create_recovery_record(&cur_recovery_record);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("Failed to create recovery record for boot fail [0x%x]!\n", (unsigned int)boot_fail_no);
        goto __out;
    }

    /* 5. run recovery method. Note: reboot is executed by the caller now */
    ret = bfr_run_recovery_method(pselect_param);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("Failed to run recovery method for boot fail [0x%x]!\n", (unsigned int)boot_fail_no);
        goto __out;
    }

    /* 6. store the real recovery method selected just now in local variable */
    recovery_method = pselect_param->recovery_method;

__out:
    if (NULL != pselect_param)
    {
        bfmr_free(pselect_param);
    }

    /* 7. return recovery method to the caller */
    BFMR_PRINT_KEY_INFO("recovery_method: [%d] for boot fail [0x%x]!\n", recovery_method, (unsigned int)boot_fail_no);
    return recovery_method;
}


/**
    @function: int bfr_init(void)
    @brief: init BFR.

    @param: none.

    @return: none.

    @note:
*/
int bfr_init(void)
{
    memset((void *)&s_rrecord_param, 0, sizeof(s_rrecord_param));
    return 0;
}

