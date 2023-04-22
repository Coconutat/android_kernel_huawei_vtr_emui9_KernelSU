/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfmr_common_qcom.c

    @brief: define the common interface for BFMR (Boot Fail Monitor and Recovery) on QCOM

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <chipset_common/bfmr/common/bfmr_common.h>
#include <linux/rtc.h>


/*----local macroes------------------------------------------------------------------*/


/*----local prototypes----------------------------------------------------------------*/


/*----local variables-----------------------------------------------------------------*/


/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes--------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/


/*----function definitions--------------------------------------------------------------*/

/**
    @function: int bfm_get_device_full_path(char *dev_name, char *path_buf, unsigned int path_buf_len)
    @brief: get full path of the "dev_name".

    @param: dev_name [in] device name such as: boot/recovery/rrecord.
    @param: path_buf [out] buffer will store the full path of "dev_name".
    @param: path_buf_len [in] length of the path_buf.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_get_device_full_path(char *dev_name, char *path_buf, unsigned int path_buf_len)
{
    if (unlikely((NULL == dev_name) || (NULL == path_buf)))
    {
        BFMR_PRINT_INVALID_PARAMS("dev_name: %p, path_buf: %p\n", dev_name, path_buf);
        return -1;
    }

    snprintf(path_buf, path_buf_len - 1, "/dev/block/bootdevice/by-name/%s", dev_name);

    return 0;
}


/**
    @function: unsigned int bfmr_get_bootup_time(void)
    @brief: get bootup time.

    @param:

    @return: bootup time(seconds).

    @note:
*/
unsigned int bfmr_get_bootup_time(void)
{
    u64 ts = 0;

    ts = sched_clock();
    return (unsigned int)(ts / 1000 / 1000 / 1000);
}

