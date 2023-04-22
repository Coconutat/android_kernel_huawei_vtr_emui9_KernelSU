
/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_trace_stagetime.c

    @brief: define the basic external enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: YangJie ID: 202466

    @date: 2018-04-12

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
#include <linux/dirent.h>
#include <linux/statfs.h>
#include <chipset_common/bfmr/bfm/chipsets/bfm_chipsets.h>
#include <chipset_common/bfmr/bfm/core/bfm_stage_info.h>


/*----local macroes-----------------------------------------------------------------*/
#define BFM_BOOTUP_STAGE_INFO_FORMAT \
        "start:%12s_%s(%ds)  end:%12s_%s(%ds)  elapse:%ds\r\n"

#define TITLE_BFMR_BOOTUP_STAGE_INFO "booup stage info"

/*----local prototypes---------------------------------------------------------------*/

typedef struct
{
    bfmr_boot_stage_e boot_stage;
    char *desc;
} bfm_boot_stage_no_desc_t;

static char *bfm_get_boot_stage_no_desc(bfmr_boot_stage_e boot_stage);
static size_t bfm_format_stages_info(char *bs_data, unsigned int bs_data_buffer_len);

static DEFINE_MUTEX(s_record_stage_mutex);
/*----local variables------------------------------------------------------------------*/

/*if you need bootstage description before native stage, modify this array*/
static bfm_boot_stage_no_desc_t s_boot_stage_desc[] =
{
    {STAGE_INIT_START,                           "stage_init_start"},
    {STAGE_RESIZE_FS_START,                      "stage_resize_fs_start"},
    {STAGE_RESIZE_FS_END,                        "stage_resize_fs_end"},
    {STAGE_CHECK_FS_START,                       "stage_check_fs_start"},
    {STAGE_CHECK_FS_END,                         "stage_check_fs_end"},
    {STAGE_ON_EARLY_INIT,                        "stage_on_early_init"},
    {STAGE_ON_INIT,                              "stage_on_init"},
    {STAGE_ON_EARLY_FS,                          "stage_on_early_fs"},
    {STAGE_ON_FS,                                "stage_on_fs"},
    {STAGE_ON_POST_FS,                           "stage_on_post_fs"},
    {STAGE_ON_POST_FS_DATA,                      "stage_on_post_fs_data"},
    {STAGE_RESTORECON_START,                     "stage_restorecon_start"},
    {STAGE_RESTORECON_END,                       "stage_restorecon_end"},
    {STAGE_ON_EARLY_BOOT,                        "stage_on_early_boot"},
    {STAGE_ON_BOOT,                              "stage_on_boot"},
    {STAGE_ZYGOTE_START,                         "stage_zygote_start"},
    {STAGE_VM_START,                             "stage_vm_start"},
    {STAGE_PHASE_WAIT_FOR_DEFAULT_DISPLAY,       "stage_phase_wait_for_default_display"},
    {STAGE_PHASE_LOCK_SETTINGS_READY,            "stage_phase_lock_settings_ready"},
    {STAGE_PHASE_SYSTEM_SERVICES_READY,          "stage_phase_system_services_ready"},
    {STAGE_PHASE_ACTIVITY_MANAGER_READY,         "stage_phase_activity_manager_ready"},
    {STAGE_PHASE_THIRD_PARTY_APPS_CAN_START,     "stage_phase_third_party_apps_can_start"},
    {STAGE_FRAMEWORK_JAR_DEXOPT_START,           "stage_framework_jar_dexopt_start"},
    {STAGE_FRAMEWORK_JAR_DEXOPT_END,             "stage_framework_jar_dexopt_end"},
    {STAGE_APP_DEXOPT_START,                     "stage_app_dexopt_start"},
    {STAGE_APP_DEXOPT_END,                       "stage_app_dexopt_end"},
    {STAGE_PHASE_BOOT_COMPLETED,                 "stage_phase_boot_completed"},
    {STAGE_SYSTEM_SERVER_DEX2OAT_START,          "stage_system_server_dex2oat_start"},
    {STAGE_SYSTEM_SERVER_DEX2OAT_END,            "stage_system_server_dex2oat_end"},
    {STAGE_SYSTEM_SERVER_INIT_START,             "stage_system_server_init_start"},
    {STAGE_SYSTEM_SERVER_INIT_END,               "stage_system_server_init_end"},
    {STAGE_BOOT_SUCCESS ,                        "stage_boot_success"},
};


/*----global variables-----------------------------------------------------------------*/
static bfm_stages_t * s_stages_info = NULL;

/*----global function prototypes---------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/


/*----function definitions--------------------------------------------------------------*/

static char* bfm_get_boot_stage_no_desc(bfmr_boot_stage_e boot_stage)
{
    int count = sizeof(s_boot_stage_desc) / sizeof(s_boot_stage_desc[0]);
    int i = 0;
    for (i = 0; i < count; i++)
    {
        if (boot_stage == s_boot_stage_desc[i].boot_stage)
        {
            return s_boot_stage_desc[i].desc;
        }
    }
    return "unknown_stage";
}

/**
    @function: bfmr_save_stages_info_txt
    @brief: append the stages info into bs_data

    @param: pdata[inout], the buffer to record information
    @param: pdata_size[in], the size of bs_data

    @return:the length to need print

    @note:
*/
size_t  bfm_save_stages_info_txt(char *buffer, unsigned int buffer_len)
{
     size_t bytes_print = 0;

     if (NULL == buffer || buffer_len <= 0)
     {
        BFMR_PRINT_ERR("Buffer is not valid!\n");
        return 0;
     }

    bytes_print = bfm_format_stages_info(buffer, buffer_len);
    if (bytes_print <= 0)
    {
        BFMR_PRINT_ERR(" not add any stage info into bootfail_info.txt!\n");
    }

    bfm_deinit_stage_info();

    return bytes_print;
}
EXPORT_SYMBOL(bfm_save_stages_info_txt);

static size_t bfm_format_stages_info(char *bs_data, unsigned int bs_data_buffer_len)
{
    size_t     bs_data_size = 0;
    size_t     bytes_record = 0;
    unsigned int       stage_index    = 0;
    bfm_stage_info_t *pre_stage_node = NULL;
    bfm_stage_info_t *cur_stage_node = NULL;

    mutex_lock(&s_record_stage_mutex);

    if(NULL == s_stages_info)
    {
        BFMR_PRINT_ERR("stage info not initialized!\n");
        mutex_unlock(&s_record_stage_mutex);
        return 0;
    }

    if(0 == s_stages_info->stage_count)
    {
        BFMR_PRINT_ERR("not record bootup stage info\n");
        mutex_unlock(&s_record_stage_mutex);
        return 0;
    }

    //set boot stage info title
    bs_data_size = snprintf(bs_data, bs_data_buffer_len - 1, "\r\n%s:\r\n", TITLE_BFMR_BOOTUP_STAGE_INFO);

    for (; stage_index < s_stages_info->stage_count; stage_index++)
    {
        pre_stage_node = cur_stage_node;
        cur_stage_node = &(s_stages_info->stage_info_list[stage_index]);

        if ((NULL == pre_stage_node) || (NULL == cur_stage_node))
        {
           continue;
        }

        if (bs_data_size >= bs_data_buffer_len -1)
        {
            bs_data_size = bs_data_buffer_len -1;
            bs_data[bs_data_buffer_len -1] = '\0';

            BFMR_PRINT_ERR("string truncation ouccured, bs_data_size:(%lu)\n", bs_data_size);
            break;
        }

        bytes_record = snprintf(bs_data + bs_data_size, bs_data_buffer_len - bs_data_size -1, BFM_BOOTUP_STAGE_INFO_FORMAT,
                                bfm_get_boot_stage_name(pre_stage_node->stage),
                                bfm_get_boot_stage_no_desc(pre_stage_node->stage),
                                pre_stage_node->start_time,
                                bfm_get_boot_stage_name(cur_stage_node->stage),
                                bfm_get_boot_stage_no_desc(cur_stage_node->stage),
                                cur_stage_node->start_time,
                                (cur_stage_node->start_time - pre_stage_node->start_time));

        bs_data_size += bytes_record;
    }

    mutex_unlock(&s_record_stage_mutex);
    return bs_data_size;
}

/**
    @function: bfm_add_stage_info
    @brief: record the stage information into list

    @param: pStage[in], the stageinfo pointer

    @return:none

    @note:
*/
void bfm_add_stage_info(bfm_stage_info_t *pStage)
{
    int cur_stage_index = 0;

    mutex_lock(&s_record_stage_mutex);

    if (NULL == pStage)
    {
        BFMR_PRINT_ERR("pStage is null!\n");
        mutex_unlock(&s_record_stage_mutex);
        return;
    }

    if (NULL == s_stages_info)
    {
        BFMR_PRINT_ERR("s_stages_info is null!\n");
        mutex_unlock(&s_record_stage_mutex);
        return;
    }

    cur_stage_index = s_stages_info->stage_count;
    if (cur_stage_index < BMFR_MAX_BOOTUP_STAGE_COUNT)
    {
        bfm_stage_info_t *current_stage_node = (bfm_stage_info_t *)&(s_stages_info->stage_info_list[cur_stage_index]);

        current_stage_node->start_time = pStage->start_time;
        current_stage_node->stage      = pStage->stage;

        s_stages_info->stage_count++;
    }
    else
    {
        BFMR_PRINT_KEY_INFO("the stages info stack is full\n");
    }

    mutex_unlock(&s_record_stage_mutex);
}
EXPORT_SYMBOL(bfm_add_stage_info);

/**
    @function: int bfm_init_stage_info(void)
    @brief: init trace time module resource

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in kernel.
*/
int bfm_init_stage_info(void)
{
    mutex_lock(&s_record_stage_mutex);

    if (NULL == s_stages_info)
    {
        s_stages_info = (bfm_stages_t *)bfmr_malloc(sizeof(bfm_stages_t));
        if (NULL == s_stages_info)
        {
            BFMR_PRINT_KEY_INFO("malloc s_stages_info failed\n");
            mutex_unlock(&s_record_stage_mutex);
            return -1;
        }

        memset((void *)s_stages_info, 0, sizeof(bfm_stages_t));
    }

    mutex_unlock(&s_record_stage_mutex);
    return 0;
}
EXPORT_SYMBOL(bfm_init_stage_info);

/**
    @function: void bfm_deinit_stage_info(void)
    @brief: release trace time module resource

    @param: none.

    @return:none

    @note: it need be called after bootsuccess and saved the buffer into txt.
*/
void bfm_deinit_stage_info(void)
{
  mutex_lock(&s_record_stage_mutex);

  if (NULL != s_stages_info)
  {
      bfmr_free(s_stages_info);
      s_stages_info = NULL;
  }

  mutex_unlock(&s_record_stage_mutex);
}
EXPORT_SYMBOL(bfm_deinit_stage_info);
