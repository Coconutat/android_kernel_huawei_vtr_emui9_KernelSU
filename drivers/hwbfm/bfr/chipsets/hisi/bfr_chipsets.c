/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfr_chipsets.c

    @brief: define the chipsets' external public enum/macros/interface for BFR (Boot Fail Recovery)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <chipset_common/bfmr/bfr/chipsets/bfr_chipsets.h>
#include <chipset_common/bfmr/common/bfmr_common.h>


/*----local macroes------------------------------------------------------------------*/

#define BFR_RRECORD_BACKUP_PART_NAME "reserved2"


/*----local prototypes----------------------------------------------------------------*/


/*----local variables-----------------------------------------------------------------*/


/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes--------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/


/*----function definitions--------------------------------------------------------------*/

bool bfr_safe_mode_has_been_enabled(void)
{
    return true;
}


int bfr_get_full_path_of_rrecord_part(char **path_buf)
{
    int ret = -1;
    int i = 0;
    bool find_full_path_of_rrecord_part = false;
    char *rrecord_names[BFR_RRECORD_PART_MAX_COUNT] = {BFR_RRECORD_PART_NAME,
        BFR_RRECORD_BACKUP_PART_NAME};
    int count = sizeof(rrecord_names) / sizeof(rrecord_names[0]);

    if (unlikely(NULL == path_buf))
    {
        BFMR_PRINT_INVALID_PARAMS("path_buf.\n");
        return -1;
    }

    *path_buf = (char *)bfmr_malloc(BFMR_DEV_FULL_PATH_MAX_LEN + 1);
    if (NULL == *path_buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -1;
    }

    for (i = 0; i < count; i++)
    {
        memset((void *)*path_buf, 0, BFMR_DEV_FULL_PATH_MAX_LEN + 1);

        ret = bfmr_get_device_full_path(rrecord_names[i], *path_buf, BFMR_DEV_FULL_PATH_MAX_LEN);
        if (0 != ret)
        {
            BFMR_PRINT_ERR("get full path for device [%s] failed!\n", rrecord_names[i]);
            continue;
        }

        find_full_path_of_rrecord_part = true;
        break;
    }

    if (!find_full_path_of_rrecord_part)
    {
        ret = -1;
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    return ret;
}

