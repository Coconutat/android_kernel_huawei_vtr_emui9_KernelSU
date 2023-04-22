/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_chipsets.c

    @brief: define the chipsets's interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/common/bfmr_common.h>
#include <chipset_common/bfmr/bfm/chipsets/bfm_chipsets.h>
#include <chipset_common/bfmr/bfm/chipsets/qcom/bfm_qcom.h>


/*----local macroes------------------------------------------------------------------*/

#define BFM_QCOM_LOG_PART_MOUINT_POINT "/log"
#define BFM_QCOM_LOG_ROOT_PATH "/log/reliability/boot_fail"
#define BFM_QCOM_LOG_UPLOADING_PATH BFM_QCOM_LOG_ROOT_PATH "/" BFM_UPLOADING_DIR_NAME
#define BFM_QCOM_BL1_BOOTFAIL_LOG_NAME "sbl1.log"
#define BFM_QCOM_BL2_BOOTFAIL_LOG_NAME "lk.log"
#define BFM_QCOM_KERNEL_BOOTFAIL_LOG_NAME "kmsg.log"
#define BFM_QCOM_RAMOOPS_BOOTFAIL_LOG_NAME "pmsg-ramoops-0"
#define BFM_QCOM_WAIT_FOR_LOG_PART_TIMEOUT (40)

#define BFM_QCOM_RAW_PART_NAME "bootfail_info"
#define BFM_QCOM_RAW_LOG_MAGIC (0x12345678)
#define BFM_QCOM_RAW_PART_OFFSET (512*1024)
#define BFM_KMSG_TMP_BUF_LEN (3*1024*512)
#define BFM_BOOT_UP_60_SECOND (60)
#define BFM_TIME_S_TO_NS (1000*1000*1000)


/*----local prototypes----------------------------------------------------------------*/


/*----local variables-----------------------------------------------------------------*/


/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes--------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/

unsigned int bfmr_capture_log_from_src_file(char *buf, unsigned int buf_len, char *src_log_path);
static unsigned int bfmr_capture_tombstone(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_vm_crash(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_vm_watchdog(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_normal_framework_bootfail_log(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_logcat_on_beta_version(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_critical_process_crash_log(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfmr_capture_fixed_framework_bootfail_log(char *buf, unsigned int buf_len, char *src_log_file_path);
static unsigned int bfm_capture_kmsg_log(char *buf, unsigned int buf_len);


/*----function definitions--------------------------------------------------------------*/

int bfm_get_boot_stage(bfmr_detail_boot_stage_e *pbfmr_bootstage)
{
    unsigned int hisi_keypoint;

    if (unlikely(NULL == pbfmr_bootstage))
    {
        BFMR_PRINT_INVALID_PARAMS("pbfmr_bootstage: %p\n", pbfmr_bootstage);
        return -1;
    }

    *pbfmr_bootstage = qcom_get_boot_stage();
    return 0;
}


int bfm_set_boot_stage(bfmr_detail_boot_stage_e bfmr_bootstage)
{
    qcom_set_boot_stage(bfmr_bootstage);
    return 0;
}


unsigned int bfmr_capture_log_from_src_file(char *buf, unsigned int buf_len, char *src_log_path)
{
    int fd_src = -1;
    char *ptemp = NULL;
    long src_file_len = 0L;
    mm_segment_t old_fs;
    long bytes_to_read = 0L;
    long bytes_read = 0L;
    long seek_pos = 0L;
    unsigned long bytes_read_total = 0;

    if (unlikely((NULL == buf) || (NULL == src_log_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("buf: %p src_file_path: %p\n", buf, src_log_path);
        return 0;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    /* get the length of the source file */
    src_file_len = bfmr_get_file_length(src_log_path);
    if (src_file_len <= 0)
    {
        BFMR_PRINT_ERR("length of [%s] is: %ld!\n", src_log_path, src_file_len);
        goto __out;
    }

    fd_src = sys_open(src_log_path, O_RDONLY, 0);
    if (fd_src < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed![ret = %d]\n", src_log_path, fd_src);
        goto __out;
    }

    /* lseek for reading the latest log */
    seek_pos = (src_file_len <= (long)buf_len)
        ? (0L) : (src_file_len - (long)buf_len);
    if (sys_lseek(fd_src, (unsigned int)seek_pos, SEEK_SET) < 0)
    {
        BFMR_PRINT_ERR("sys_lseek [%s] failed!\n", src_log_path);
        goto __out;
    }

    /* read data from the user space source file */
    ptemp = buf;
    bytes_to_read = BFMR_MIN(src_file_len, (long)buf_len);
    while (bytes_to_read > 0)
    {
        bytes_read = bfmr_full_read(fd_src, ptemp, bytes_to_read);
        if (bytes_read < 0)
        {
            BFMR_PRINT_ERR("bfmr_full_read [%s] failed!\n", src_log_path);
            goto __out;
        }
        bytes_to_read -= bytes_read;
        ptemp += bytes_read;
        bytes_read_total += bytes_read;
    }

__out:
    if (fd_src >= 0)
    {
        sys_close(fd_src);
    }

    set_fs(old_fs);

    return bytes_read_total;
}


static unsigned int bfmr_capture_tombstone(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfmr_capture_log_from_src_file(buf, buf_len, src_log_file_path);
}


static unsigned int bfmr_capture_vm_crash(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfmr_capture_log_from_src_file(buf, buf_len, src_log_file_path);
}


static unsigned int bfmr_capture_vm_watchdog(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfmr_capture_log_from_src_file(buf, buf_len, src_log_file_path);
}


static unsigned int bfmr_capture_normal_framework_bootfail_log(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfmr_capture_log_from_src_file(buf, buf_len, src_log_file_path);
}


static unsigned int bfmr_capture_logcat_on_beta_version(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    mm_segment_t old_fs;
    char src_path[BFMR_SIZE_256] = {0};

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    if (sys_readlink(src_log_file_path, src_path, sizeof(src_path) - 1) > 0)
    {
        src_log_file_path = src_path;
    }
    set_fs(old_fs);

    return bfmr_capture_log_from_src_file(buf, buf_len, src_log_file_path);
}


static unsigned int bfmr_capture_critical_process_crash_log(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfmr_capture_log_from_src_file(buf, buf_len, src_log_file_path);
}


static unsigned int bfmr_capture_fixed_framework_bootfail_log(char *buf, unsigned int buf_len, char *src_log_file_path)
{
    return bfmr_capture_log_from_src_file(buf, buf_len, src_log_file_path);
}

static unsigned int bfm_capture_kmsg_log(char *buf, unsigned int buf_len)
{
    int ret;
    char *kmsg_tmp;

    if (NULL == buf)
      return 0;

    kmsg_tmp = bfmr_malloc(BFM_KMSG_TMP_BUF_LEN);

    if(NULL == kmsg_tmp)
    {
        ret = kmsg_print_to_ddr(buf,(int)buf_len);
        ret = ((ret > 0)?ret:0);
    }
    else
    {
        ret = kmsg_print_to_ddr(kmsg_tmp, BFM_KMSG_TMP_BUF_LEN);
        ret = ((ret > 0)?ret:0);
        if(ret > buf_len)
        {
            memcpy(buf, kmsg_tmp+ret-buf_len, buf_len);
            ret = buf_len;
        }
        else
        {
            memcpy(buf, kmsg_tmp, ret);
        }
        bfmr_free(kmsg_tmp);
    }
    return (unsigned int)ret;
}


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
unsigned int bfmr_capture_log_from_system(char *buf, unsigned int buf_len, bfmr_log_src_t *src, int timeout)
{
    unsigned int bytes_captured = 0U;

    if (unlikely((NULL == buf) || (NULL == src)))
    {
        BFMR_PRINT_INVALID_PARAMS("buf: %p, src: %p\n", buf, src);
        return 0U;
    }

    switch (src->log_type)
    {
    case LOG_TYPE_BOOTLOADER_1:
        {
            break;
        }
    case LOG_TYPE_BOOTLOADER_2:
        {
            break;
        }
    case LOG_TYPE_BFMR_TEMP_BUF:
        {
            break;
        }
    case LOG_TYPE_TEXT_KMSG:
        {
            bytes_captured = bfm_capture_kmsg_log(buf, buf_len);
            break;
        }
    case LOG_TYPE_RAMOOPS:
        {
            break;
        }
    case LOG_TYPE_BETA_APP_LOGCAT:
        {
            if (bfm_is_beta_version())
            {
                bytes_captured = bfmr_capture_logcat_on_beta_version(buf, buf_len, BFM_LOGCAT_FILE_PATH);
            }
            break;
        }
    case LOG_TYPE_CRITICAL_PROCESS_CRASH:
        {
            bytes_captured = bfmr_capture_critical_process_crash_log(buf, buf_len, src->src_log_file_path);
            break;
        }
    case LOG_TYPE_VM_TOMBSTONES:
        {
            bytes_captured = bfmr_capture_tombstone(buf, buf_len, src->src_log_file_path);
            break;
        }
    case LOG_TYPE_VM_CRASH:
        {
            bytes_captured = bfmr_capture_vm_crash(buf, buf_len, src->src_log_file_path);
            break;
        }
    case LOG_TYPE_VM_WATCHDOG:
        {
            bytes_captured = bfmr_capture_vm_watchdog(buf, buf_len, src->src_log_file_path);
            break;
        }
    case LOG_TYPE_NORMAL_FRAMEWORK_BOOTFAIL_LOG:
        {
            bytes_captured = bfmr_capture_normal_framework_bootfail_log(buf, buf_len, src->src_log_file_path);
            break;
        }
    case LOG_TYPE_FIXED_FRAMEWORK_BOOTFAIL_LOG:
        {
            bytes_captured = bfmr_capture_fixed_framework_bootfail_log(buf, buf_len, BFM_FRAMEWORK_BOOTFAIL_LOG_PATH);
            break;
        }
    default:
        {
            BFMR_PRINT_ERR("Invalid log type: [%d]\n", (int)(src->log_type));
            break;
        }
    }

    return bytes_captured;
}


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
    unsigned int buf_len)
{
    if (unlikely((NULL == process_bootfail_param) || (NULL == buf)))
    {
        BFMR_PRINT_INVALID_PARAMS("psave_param: %p, buf: %p\n", process_bootfail_param, buf);
        return -1;
    }

    return 0;
}


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
int bfmr_save_log_to_fs(char *dst_file_path, char *buf, unsigned int log_len, int append)
{
    int ret = -1;
    int fd = -1;
    mm_segment_t fs = 0;
    long bytes_written = 0L;

    if (unlikely(NULL == dst_file_path || NULL == buf))
    {
        BFMR_PRINT_INVALID_PARAMS("dst_file_path: %p, buf: %p\n", dst_file_path, buf);
        return -1;
    }

    if (0U == log_len)
    {
        BFMR_PRINT_KEY_INFO("There is no need to save log whose length is: %u\n", log_len);
        return 0;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);

    /* 1. open file for writing, please note the parameter-append */
    fd = sys_open(dst_file_path, O_CREAT | O_RDWR | ((0 == append) ? (0U) : O_APPEND), BFMR_FILE_LIMIT);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed, fd: %d\n", dst_file_path, fd);
        goto __out;
    }

    /* 2. write file */
    bytes_written = bfmr_full_write(fd, buf, log_len);
    if ((long)log_len != bytes_written)
    {
        BFMR_PRINT_ERR("bfmr_full_write [%s] failed, log_len: %ld bytes_written: %ld\n",
            dst_file_path, (long)log_len, bytes_written);
        goto __out;
    }

    /* 3. change own and mode for the file */
    bfmr_change_own_mode(dst_file_path, BFMR_AID_ROOT, BFMR_AID_SYSTEM, BFMR_FILE_LIMIT);

    /* 4. write successfully, modify the value of ret */
    ret = 0;

__out:
    if (fd >= 0)
    {
        sys_fsync(fd);
        sys_close(fd);
    }

    set_fs(fs);

    return ret;
}


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
int bfmr_save_log_to_raw_part(char *raw_part_name, unsigned long long offset, char *buf, unsigned int log_len)
{
    int ret = -1;
    char *pdev_full_path = NULL;

    if(!log_len)
    {
        BFMR_PRINT_INVALID_PARAMS("raw_part_name: %p, buf: %p,loglen = %d\n", raw_part_name, buf,log_len);
        return 0;
    }

    if (unlikely(NULL == raw_part_name || NULL == buf))
    {
        BFMR_PRINT_INVALID_PARAMS("raw_part_name: %p, buf: %p\n", raw_part_name, buf);
        return -1;
    }

    pdev_full_path = (char *)bfmr_malloc((BFMR_DEV_FULL_PATH_MAX_LEN + 1) * sizeof(char));
    if (NULL == pdev_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pdev_full_path, 0, ((BFMR_DEV_FULL_PATH_MAX_LEN + 1) * sizeof(char)));

    ret = bfmr_get_device_full_path(raw_part_name, pdev_full_path, BFMR_DEV_FULL_PATH_MAX_LEN);
    if (0 != ret)
    {
        goto __out;
    }

    ret = bfmr_write_emmc_raw_part(pdev_full_path, offset, buf, (unsigned long long)log_len);
    if (0 != ret)
    {
        ret = -1;
        BFMR_PRINT_ERR("write [%s] failed ret: %d!\n", pdev_full_path, ret);
        goto __out;
    }

__out:
    bfmr_free(pdev_full_path);

    return ret;
}


/**
    @function: int bfmr_read_log_from_raw_part(char *raw_part_name, unsigned long long offset, char *buf, unsigned int buf_size)
    @brief: save log to raw partition.

    @param: raw_part_name [in], such as: rrecord/recovery/boot, not the full path of the device.
    @param: offset [in], offset from the beginning of the "raw_part_name".
    @param: buf [in], buffer saving log.
    @param: buf_size [in], length of the log which is <= the length of buf.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_read_log_from_raw_part(char *raw_part_name, unsigned long long offset, char *buf, unsigned int buf_size)
{
    int ret = -1;
    char *pdev_full_path = NULL;

    if(!buf_size)
    {
        BFMR_PRINT_INVALID_PARAMS("raw_part_name: %p, buf: %p,loglen = %d\n", raw_part_name, buf, buf_size);
        return 0;
    }

    if (unlikely(NULL == raw_part_name || NULL == buf))
    {
        BFMR_PRINT_INVALID_PARAMS("raw_part_name: %p, buf: %p\n", raw_part_name, buf);
        return -1;
    }

    pdev_full_path = (char *)bfmr_malloc((BFMR_DEV_FULL_PATH_MAX_LEN + 1) * sizeof(char));
    if (NULL == pdev_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pdev_full_path, 0, ((BFMR_DEV_FULL_PATH_MAX_LEN + 1) * sizeof(char)));

    ret = bfmr_get_device_full_path(raw_part_name, pdev_full_path, BFMR_DEV_FULL_PATH_MAX_LEN);
    if (0 != ret)
    {
        goto __out;
    }

    ret = bfmr_read_emmc_raw_part(pdev_full_path, offset, buf, (unsigned long long)buf_size);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("Read [%s] failed ret: %d!\n", pdev_full_path, ret);
        goto __out;
    }

__out:
    bfmr_free(pdev_full_path);

    return ret;
}

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
int bfmr_save_log_to_mem_buffer(char *dst_buf, unsigned int dst_buf_len, char *src_buf, unsigned int log_len)
{
    if (unlikely(NULL == dst_buf || NULL == src_buf))
    {
        BFMR_PRINT_INVALID_PARAMS("dst_buf: %p, src_buf: %p\n", dst_buf, src_buf);
        return -1;
    }

    memcpy(dst_buf, src_buf, BFMR_MIN(dst_buf_len, log_len));

    return 0;
}


/**
    @function: char* bfm_get_bfmr_log_root_path(void)
    @brief: get log root path

    @param: none

    @return: the path of bfmr log's root path.

    @note:
*/
char* bfm_get_bfmr_log_root_path(void)
{
    return (char *)BFM_QCOM_LOG_ROOT_PATH;
}


char* bfm_get_bfmr_log_uploading_path(void)
{
    return (char *)BFM_QCOM_LOG_UPLOADING_PATH;
}


char* bfm_get_bfmr_log_part_mount_point(void)
{
    return (char *)BFM_QCOM_LOG_PART_MOUINT_POINT;
}


int bfm_capture_and_save_do_nothing_bootfail_log(bfm_process_bootfail_param_t *param)
{
    int ret = 0;
    bfmr_capture_and_save_bootfail_log capture_and_save_bootfail_log_tmp;

    if (unlikely(NULL == param))
    {
        BFMR_PRINT_INVALID_PARAMS("param: %p\n", param);
        return -1;
    }

    capture_and_save_bootfail_log_tmp = (bfmr_capture_and_save_bootfail_log)(param->capture_and_save_bootfail_log);
    if (!capture_and_save_bootfail_log_tmp)
    {
        BFMR_PRINT_INVALID_PARAMS("capture_and_save_bootfail_log_tmp : %p\n", capture_and_save_bootfail_log_tmp);
        return -1;
    }

    switch (param->bootfail_errno)
    {
    case KERNEL_PRESS10S:
        {
            /*save_log_to_raw_part,then forward save log to log part in sbl1 and try_to_recovery*/
            param->bootfail_time = (unsigned long long)bfm_hctosys(param->bootfail_time);
            param->dst_type = DST_RAW_PART;
            param->recovery_method = FRM_REBOOT;
            param->bootup_time = sched_clock()/(BFM_TIME_S_TO_NS);
            if(param->bootup_time < BFM_BOOT_UP_60_SECOND){
                BFMR_PRINT_ERR("KERNEL_PRESS10S at param->bootup_time =%d\n", param->bootup_time);
                return ret;
            }
            (void)capture_and_save_bootfail_log_tmp(param);
            break;
        }
    default:
        {
            if (0 != bfmr_wait_for_part_mount_without_timeout(bfm_get_bfmr_log_part_mount_point()))
            {
                BFMR_PRINT_ERR("log part [%s] is not ready\n", BFM_QCOM_LOG_PART_MOUINT_POINT);
                ret = -1;
            }
            else
            {
                param->recovery_method = FRM_DO_NOTHING;
                (void)capture_and_save_bootfail_log_tmp(param);
            }
            break;
        }
    }

    return ret;
}


/**
    @function: int bfm_platform_process_boot_fail(bfm_process_bootfail_param_t *param)
    @brief: process boot fail in chipsets module

    @param: param [int] params for further process boot fail in chipsets

    @return: 0 - succeeded; -1 - failed.

    @note: realize this function according to diffirent platforms
*/
int bfm_platform_process_boot_fail(bfm_process_bootfail_param_t *param)
{
    if (unlikely(NULL == param))
    {
        BFMR_PRINT_INVALID_PARAMS("param: %p\n", param);
        return -1;
    }

    /*if log is not ready, so we think error must happend in kernel, so panic*/
    if (0 != bfmr_wait_for_part_mount_without_timeout(bfm_get_bfmr_log_part_mount_point()))
    {
        BFMR_PRINT_ERR("log part [%s] is not ready\n", BFM_QCOM_LOG_PART_MOUINT_POINT);
        qcom_set_boot_fail_flag(param->bootfail_errno);
        panic("bfm_plat:bootfail happened NO=0x%08x!!\n",param->bootfail_errno);
        param->bootfail_can_only_be_processed_in_platform = 1;
    }
    else
    {
        param->bootfail_time = (unsigned long long)bfm_hctosys(param->bootfail_time);
        param->bootfail_can_only_be_processed_in_platform = 0;
        param->recovery_method = try_to_recovery(param->bootfail_time,
            param->bootfail_errno, param->boot_stage, param->suggested_recovery_method, NULL);
    }

    return 0;
}


int bfm_update_platform_logs(bfm_bootfail_log_info_t *pbootfail_log_info)
{
    return 0;
}


/**
    @function: int bfm_platform_process_boot_success(void)
    @brief: process boot success in chipsets module

    @param: none

    @return: 0 - succeeded; -1 - failed.

    @note: HISI must realize this function in detail, the other platform can return 0 when enter this function
*/
int bfm_platform_process_boot_success(void)
{
    int ret;
    bfm_record_info_t *brit = bfmr_malloc(sizeof(bfm_record_info_t));

    if (NULL == brit)
    {
      ret = -1;
      goto __out;
    }

    ret = bfmr_read_log_from_raw_part(BFM_QCOM_RAW_PART_NAME,
                              BFM_QCOM_RAW_PART_OFFSET,
                              brit, sizeof(bfm_record_info_t));
    if (ret)
    {
      ret = -1;
      goto __out;
    }

    if (BFM_QCOM_RAW_LOG_MAGIC != brit->magic)
    {
      BFMR_PRINT_KEY_INFO("raw part is not have bootfail log\n");
      ret = 0;
      goto __out;
    }

    brit->magic = 0;
    ret = bfmr_save_log_to_raw_part(BFM_QCOM_RAW_PART_NAME,
                              BFM_QCOM_RAW_PART_OFFSET,
                              brit, sizeof(bfm_record_info_t));
    if (ret)
    {
      ret = -1;
      goto __out;
    }
    ret = 0;
    BFMR_PRINT_KEY_INFO("raw part's bf log is cleared successfuly\n");

__out:
    if (NULL != brit)
    {
        bfmr_free(brit);
    }
    return ret;
}


int bfm_is_system_rooted(void)
{
    char *bootlock_value = bfm_get_bootlock_value_from_cmdline();

    if (NULL == bootlock_value)
    {
        return 0;
    }

    if (0 == strcmp(bootlock_value, "bootlock=locked"))
    {
        return 0;
    }

    return 1;
}


int bfm_is_user_sensible_bootfail(bfmr_bootfail_errno_e bootfail_errno,
    bfr_suggested_recovery_method_e suggested_recovery_method)

{
    return (DO_NOTHING == suggested_recovery_method) ? 0 : 1;
}


char* bfm_get_bl1_bootfail_log_name(void)
{
    return BFM_QCOM_BL1_BOOTFAIL_LOG_NAME;
}


char* bfm_get_bl2_bootfail_log_name(void)
{
    return BFM_QCOM_BL2_BOOTFAIL_LOG_NAME;
}


char* bfm_get_kernel_bootfail_log_name(void)
{
    return BFM_QCOM_KERNEL_BOOTFAIL_LOG_NAME;
}


char* bfm_get_ramoops_bootfail_log_name(void)
{
    return BFM_QCOM_RAMOOPS_BOOTFAIL_LOG_NAME;
}


char* bfm_get_platform_name(void)
{
    return "qcom";
}


unsigned int bfm_get_dfx_log_length(void)
{
    return (unsigned int)0;
}

/***********************************************
 *
 *     write log to raw partion start
 *
************************************************/
char* bfm_get_raw_part_name(void)
{
    return BFM_QCOM_RAW_PART_NAME;
}

int bfm_get_raw_part_offset(void)
{
    return BFM_QCOM_RAW_PART_OFFSET;
}

static char *qcom_get_file_name(char *file_full_path)
{
    char *ptemp = NULL;

    if (unlikely((NULL == file_full_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("file_full_path: %p\n", file_full_path);
        return NULL;
    }

    ptemp = strrchr(file_full_path, '/');
    if (NULL == ptemp)
    {
        return file_full_path;
    }

    return (ptemp + 1);
}

void bfmr_alloc_and_init_raw_log_info(bfm_process_bootfail_param_t *pparam, bfmr_log_dst_t *pdst)
{
    bfm_record_info_t *brit;

    pparam->log_save_context = bfmr_malloc(sizeof(bfm_record_info_t));
    if (NULL != pparam->log_save_context)
    {
        BFMR_PRINT_KEY_INFO("start+++\n");
        memset(pparam->log_save_context, 0, sizeof(bfm_record_info_t));
        brit = (bfm_record_info_t *)(pparam->log_save_context);

        brit->magic = BFM_QCOM_RAW_LOG_MAGIC;
        brit->bfmErrNo = pparam->bootfail_errno;
        brit->boot_stage = pparam->boot_stage;
        brit->bootfail_time = pparam->bootfail_time;
        brit->suggested_recovery_method = pparam->suggested_recovery_method;
        brit->recovery_method = pparam->recovery_method;

        strncpy(brit->log_dir, pparam->bootfail_log_dir, BFMR_SIZE_128 - 1);
        brit->total_log_lenth += sizeof(bfm_record_info_t);
        pdst->dst_info.raw_part.offset += sizeof(bfm_record_info_t);
        printk("brit->total_log_lenth %d\n",brit->total_log_lenth);
        BFMR_PRINT_KEY_INFO("end+++\n");
    }
    return;
}

void bfmr_save_and_free_raw_log_info(bfm_process_bootfail_param_t *pparam)
{
    bfm_record_info_t *brit;
    if (NULL != pparam->log_save_context)
    {
        BFMR_PRINT_KEY_INFO("start+++\n");
        brit = (bfm_record_info_t *)(pparam->log_save_context);
        bfmr_save_log_to_raw_part(BFM_QCOM_RAW_PART_NAME,
                                  BFM_QCOM_RAW_PART_OFFSET,
                                  brit, sizeof(bfm_record_info_t));

        printk("brit->total_log_lenth %d\n",brit->total_log_lenth);
        bfmr_free(pparam->log_save_context);
        pparam->log_save_context = NULL;
        BFMR_PRINT_KEY_INFO("end+++\n");
    }

    return;
}

void bfmr_update_raw_log_info(bfmr_log_src_t *psrc, bfmr_log_dst_t *pdst, unsigned int bytes_read)
{
    bfm_record_info_t *brit;

    if(!psrc || !psrc->log_save_context || !pdst)
      return;

    BFMR_PRINT_KEY_INFO("start+++logtype %d\n",psrc->log_type);
    brit = (bfm_record_info_t *)(psrc->log_save_context);

    /*get file name*/
    switch (psrc->log_type)
    {
    case LOG_TYPE_BOOTLOADER_1:
        {
            strncpy(brit->log_name[psrc->log_type], BFM_QCOM_BL1_BOOTFAIL_LOG_NAME, BFMR_SIZE_128-1);
            break;
        }
    case LOG_TYPE_BOOTLOADER_2:
        {
            strncpy(brit->log_name[psrc->log_type], BFM_QCOM_BL2_BOOTFAIL_LOG_NAME, BFMR_SIZE_128-1);
            break;
        }
    case LOG_TYPE_TEXT_KMSG:
        {
            strncpy(brit->log_name[psrc->log_type], BFM_QCOM_KERNEL_BOOTFAIL_LOG_NAME, BFMR_SIZE_128-1);
            break;
        }
    case LOG_TYPE_RAMOOPS:
        {
            strncpy(brit->log_name[psrc->log_type], BFM_QCOM_RAMOOPS_BOOTFAIL_LOG_NAME, BFMR_SIZE_128-1);
            break;
        }
    case LOG_TYPE_BETA_APP_LOGCAT:
        {
            strncpy(brit->log_name[psrc->log_type], BFM_LOGCAT_FILE_NAME, BFMR_SIZE_128-1);
            break;
        }
    case LOG_TYPE_FIXED_FRAMEWORK_BOOTFAIL_LOG:
        {
            strncpy(brit->log_name[psrc->log_type], BFM_FRAMEWORK_BOOTFAIL_LOG_FILE_NAME, BFMR_SIZE_128-1);
            break;
        }
    case LOG_TYPE_CRITICAL_PROCESS_CRASH:
    case LOG_TYPE_VM_TOMBSTONES:
    case LOG_TYPE_VM_CRASH:
    case LOG_TYPE_VM_WATCHDOG:
    case LOG_TYPE_NORMAL_FRAMEWORK_BOOTFAIL_LOG:
    case LOG_TYPE_BFM_BFI_LOG:
    case LOG_TYPE_BFM_RECOVERY_LOG:
        {
            strncpy(brit->log_name[psrc->log_type], qcom_get_file_name(psrc->src_log_file_path), BFMR_SIZE_128-1);
            break;
        }
    case LOG_TYPE_BFMR_TEMP_BUF:
    default:
        {
            BFMR_PRINT_ERR("Invalid log type: [%d]\n", (int)(psrc->log_type));
            break;
        }
    }

    /*  update relation struct  */
    pdst->dst_info.raw_part.offset += bytes_read;
    brit->log_lenth[psrc->log_type] = bytes_read;

    brit->total_log_lenth += bytes_read;
    printk("brit->total_log_lenth %d\n",brit->total_log_lenth);
    BFMR_PRINT_KEY_INFO("end+++\n");
    return;
}

/***********************************************
 *
 *     write log to raw partion end
 *
************************************************/

/**
    @function: int bfmr_copy_data_from_dfx_to_bfmr_tmp_buffer(void)
    @brief: copy dfx data to local buffer

    @param: none

    @return: 0 - succeeded; -1 - failed.

    @note: HISI must realize this function in detail, the other platform can return when enter this function
*/
void bfmr_copy_data_from_dfx_to_bfmr_tmp_buffer(void)
{
    return;
}


int bfm_chipsets_init(bfm_chipsets_init_param_t *param)
{
    int ret = 0;

    if (unlikely((NULL == param)))
    {
        BFMR_PRINT_KEY_INFO("param or param->log_saving_param is NULL\n");
        return -1;
    }

    qcom_hwboot_fail_init();
    return 0;
}

