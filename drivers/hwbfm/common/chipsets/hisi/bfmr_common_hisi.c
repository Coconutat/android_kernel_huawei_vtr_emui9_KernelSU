/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfmr_common_hisi.c

    @brief: define the common interface for BFMR (Boot Fail Monitor and Recovery) on HISI

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/hisi/kirin_partition.h>
#include <hisi_partition.h>
#include <chipset_common/bfmr/common/bfmr_common.h>


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
    int ret = -1;

    if (unlikely((NULL == dev_name) || (NULL == path_buf)))
    {
        BFMR_PRINT_INVALID_PARAMS("dev_name or path_buf.\n");
        return -1;
    }

    ret = flash_find_ptn(dev_name, path_buf);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("find full path for part: [%s] failed!\n", dev_name);
        return -1;
    }

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

#ifdef CONFIG_HISI_TIME
    ts = hisi_getcurtime();/* ns */
#else
    ts = sched_clock();
#endif

    return (unsigned int)(ts / 1000 / 1000 / 1000);
}
