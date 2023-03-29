/*
 * Copyright @ Huawei Technologies Co., Ltd. 1998-2017. All rights reserved.
 *
 * Author:
 * Markku Kylänpää <markku.kylanpaa@vtt.fi>
 * yudong<yudong2@huawei.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 * 2017-11-27 modify by yudong<yudong2@huawei.com>
 */

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/mm_types.h>
#include <linux/types.h>
#include <linux/syscalls.h>
#include <linux/namei.h>
#include <linux/seq_file.h>
#include <linux/rwsem.h>
#include <linux/fs.h>
#include <linux/security.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/dcache.h>
#include <linux/vmalloc.h>
#include <teek_client_api.h>
#include <libhwsecurec/securec.h>
#include "tpm.h"

#define TPMTEE_ERROR (-1)

//#define TPMTEE_PRINT_DEBUG 1
#define TPMTEE_TAG  "TPM"
#define tpmtee_error(fmt, args...) pr_err("%s: %s: " fmt "\n", \
            TPMTEE_TAG, __func__, ## args)
#define tpmtee_warning(fmt, args...) pr_warn("%s: %s: " fmt "\n", \
            TPMTEE_TAG, __func__, ## args)
#define tpmtee_info(fmt, args...) pr_info("%s: %s: " fmt "\n", \
            TPMTEE_TAG, __func__, ## args)
#ifdef TPMTEE_PRINT_DEBUG
#define tpmtee_debug(fmt,args...)  pr_info("[DEBUG]%s: %s: " fmt "\n", \
            TPMTEE_TAG, __func__, ## args)
#else
#define tpmtee_debug(fmt, ...)
#endif

/* Minimum size for send message */
#define MIN_SIZE_FOR_SEND                       10

/* Minimum size for receive message */
#define MIN_SIZE_FOR_RECV                        6

#define TPM_SIGNAL_POWER_ON                      1
#define TPM_SEND_COMMAND                         8
#define TPM_SIGNAL_NV_ON                        11

/* task_tpm UUID*/
static const TEEC_UUID tee_uuid ={ \
    0x1abbf8e5, \
    0x0add, \
    0x443d, \
    { \
         0x84, 0xf7, 0x1f, 0xf1, 0x08, 0x0e, 0x12, 0x58 \
    } \
};

/* Communications buffers */
static uint8_t *indata;
static uint8_t *outdata;
static uint8_t locality_buf[8];
static uint32_t uid = 0;
static uint8_t *package_name = "/dev/tpm0";

static int tee_initialized;
static int session_initialized;
static TEEC_Context tee_ctx;
static TEEC_Session tee_sess;
static int tee_have_data = 0;
static size_t tee_data_size = 0;

static const uint8_t tpm2_startup_cmd[] = {
    0x80, 0x01, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x01, 0x44, 0x00, 0x00
};

#define HISI_TRUSTBOOT
#ifdef HISI_TRUSTBOOT
//#define TRUST_BOOT_PCR_INDEX 15
#define TRUST_BOOT_NAMELEN 36
#define TRUST_BOOT_DIGEST_SIZE 32

#define TPM_SIGNAL_CMD_GET_TRUSTBOOT_HASHS            99

struct img_info {
       char name[TRUST_BOOT_NAMELEN];
       char hash[TRUST_BOOT_DIGEST_SIZE];
};
#define SEC_IMG_MAX 5
struct trusted_hash_struct {
    unsigned int count;
    unsigned int size;
    struct img_info secimg[SEC_IMG_MAX];
};
static struct trusted_hash_struct TrustBootHashs = {0};
static unsigned char  TrustBootHashsPcrIndex[SEC_IMG_MAX] = {0};
static int hisi_trustboot_initialized  = false;
#endif

#define USE_TEEX_RPMB
#ifdef USE_TEEX_RPMB
#define TPM_SIGNAL_CMD_RPMB_STAT_READY 98
#define RPMB_DRIVER_IS_NOT_READY 0
#define RPMB_DRIVER_IS_READY 1

static int tpmta_rpmb_initialized  = false;
extern int get_rpmb_init_status_quick(void);
int tpmtee_get_rpmb_status(void)
{
    return get_rpmb_init_status_quick();
}
#endif

static int tpmtee_init_tpmtasession(TEEC_Session *session, TEEC_Context *context)
{
    TEEC_Result result;
    TEEC_Operation op;
    uint32_t err_origin;

    result = TEEK_InitializeContext(NULL, context);
    if (result != TEEC_SUCCESS) {
        tpmtee_error("context init failed res=0x%x", result);
        return result;
    } else {
        tpmtee_debug( "TPM TA context initialized");
    }

    memset_s(&op,  sizeof(op), 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
                     TEEC_MEMREF_TEMP_INPUT,
                     TEEC_MEMREF_TEMP_INPUT);
    op.started = 1;
    op.params[2].tmpref.buffer = (void*)&uid;
    op.params[2].tmpref.size = sizeof(uid);
    op.params[3].tmpref.buffer = (void*)package_name;
    op.params[3].tmpref.size = strlen(package_name) + 1;
    result = TEEK_OpenSession(context, session, &tee_uuid,
                   TEEC_LOGIN_IDENTIFY, NULL, &op, &err_origin);
    if (result != TEEC_SUCCESS) {
        tpmtee_error("session init failed res=0x%x orig=0x%x",result, err_origin);
        goto err_sess;
    }
    tpmtee_debug( "TPM TA session created");
    return 0;

err_sess:
    tpmtee_error("init TPM TA session error");
#ifdef TPMTEE_PRINT_DEBUG
    dump_stack();
#endif
    TEEK_FinalizeContext(context);
    return result;
}

static int tpmtee_init_globel_session(void)
{
    int ret = 0;
    if (session_initialized) {
        return 0;
    }
    ret = tpmtee_init_tpmtasession(&tee_sess,&tee_ctx);
    if (ret) {
        tpmtee_error("tpmtee init globel session failed,ret = %d",ret);
        return ret;
    }else{
        tpmtee_warning("tpmtee init globel session succ!");
        session_initialized = true;
    }
    return 0;
}

static int tpmtee_check_tpmta_initialized(TEEC_Session *session)
{
    TEEC_Operation op;
    TEEC_Result result;
    uint32_t err_origin;

    if (!session_initialized) {
        result = tpmtee_init_globel_session();
         if (result) {
            tpmtee_error("init globel session failed %d", result);
            return result;
        }
    }

    if (!tee_initialized) {
        memset_s(&op, sizeof(op), 0, sizeof(op));
        op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
                         TEEC_NONE, TEEC_NONE);
        op.started = 1;
        tpmtee_warning("tmptee_send: POWER ON");
        result = TEEK_InvokeCommand(session, TPM_SIGNAL_POWER_ON, &op, &err_origin);
        if (result) {
            tpmtee_error("tmptee_send: POWER_ON failed %d", result);
            return result;
        }
        tpmtee_warning("tmptee_send: NV ON");
        result = TEEK_InvokeCommand(session, TPM_SIGNAL_NV_ON, &op, &err_origin);
        if (result) {
            tpmtee_error("tmptee_send: NV_ON failed %d", result);
            return result;
        }
        tpmtee_warning("tmptee_send: STARTUP");
        memset_s(&op, sizeof(op), 0, sizeof(op));
        op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                         TEEC_MEMREF_TEMP_INPUT,
                         TEEC_MEMREF_TEMP_OUTPUT,
                         TEEC_NONE);
        op.started = 1;

        /* locality, 1byte */
        op.params[0].tmpref.buffer = locality_buf;
        op.params[0].tmpref.size = 1;

        /* input buffer */
        op.params[1].tmpref.buffer = (void*)tpm2_startup_cmd;
        op.params[1].tmpref.size = sizeof(tpm2_startup_cmd);

        /* output buffer */
        op.params[2].tmpref.buffer = (void*)outdata;
        op.params[2].tmpref.size = TPM_BUFSIZE;

        result = TEEK_InvokeCommand(session, TPM_SEND_COMMAND, &op, &err_origin);
        if (result) {
            tpmtee_error("tmptee_send: STARTUP failed %d", result);
            return result;
        }
        tee_initialized = true;
    }
#ifdef USE_TEEX_RPMB
    if (!tpmta_rpmb_initialized) {
        if (tpmtee_get_rpmb_status() == RPMB_DRIVER_IS_READY) {
            memset_s(&op, sizeof(op), 0, sizeof(op));
            op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE,
                                                                        TEEC_NONE, TEEC_NONE);
            op.started = 1;
            tpmtee_warning("tmptee_send: RPMB STAT READY");
            result = TEEK_InvokeCommand(session, TPM_SIGNAL_CMD_RPMB_STAT_READY, &op, &err_origin);
            if (result) {
                tpmtee_error("tmptee_send: RPMB STAT READY failed %d", result);
            }else{
                tpmta_rpmb_initialized = true;
            }
        }
    }
#endif
#ifdef HISI_TRUSTBOOT
    if (!hisi_trustboot_initialized) {
        memset_s(&op, sizeof(op), 0, sizeof(op));
        op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_OUTPUT, TEEC_MEMREF_TEMP_OUTPUT,
                         TEEC_NONE, TEEC_NONE);
        op.started = 1;
        op.params[0].tmpref.buffer = &TrustBootHashs;
        op.params[0].tmpref.size = sizeof(TrustBootHashs);
        op.params[1].tmpref.buffer = &TrustBootHashsPcrIndex;
        op.params[1].tmpref.size = sizeof(TrustBootHashsPcrIndex);
        tpmtee_warning("tmptee_send: TPM_TA_GET_TRUSTBOOT_HASHS");
        result = TEEK_InvokeCommand(session, TPM_SIGNAL_CMD_GET_TRUSTBOOT_HASHS, &op, &err_origin);
        if (result) {
            tpmtee_error("tmptee_send: TPM_TA_GET_TRUSTBOOT_HASHS failed ret=%d", result);
        }else{
            hisi_trustboot_initialized = true;
        }
    }
#endif
    return 0;
}
static int tpmtee_send_to_tpmta(struct tpm_chip *chip, u8 *buf, size_t count,TEEC_Session *send_session)
{
    int rc;
    uint32_t err_origin;
    TEEC_Operation op;

    tpmtee_debug("tmptee_send: buf=%p size=%zd", buf, count);
#ifdef TPMTEE_PRINT_DEBUG
    print_hex_dump(KERN_ERR, "tpmsend: ", DUMP_PREFIX_ADDRESS,
               16, 1, buf, count, true);
#endif
    /* Sanity check for TPM commands */
    if (count < MIN_SIZE_FOR_SEND) {
        tpmtee_error("short send %zd", count);
        return -EIO;
    }
    if (count > TPM_BUFSIZE) {
        tpmtee_error("long send %zd", count);
        return -EIO;
    }
    rc = tpmtee_check_tpmta_initialized(send_session);
    if(rc){
        tpmtee_error("check tpmta initialized error!");
        return -EIO;
    }

    memset_s(&op, sizeof(op), 0, sizeof(op));
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
                     TEEC_MEMREF_TEMP_INPUT,
                     TEEC_MEMREF_TEMP_OUTPUT,
                     TEEC_NONE);
    op.started = 1;

    /* locality, 1byte */
    op.params[0].tmpref.buffer = locality_buf;
    op.params[0].tmpref.size = 1;

    /* input buffer */
    memcpy_s(indata, TPM_BUFSIZE, buf, count);
    op.params[1].tmpref.buffer = (void*)indata;
    op.params[1].tmpref.size = count;

    /* output buffer */
    op.params[2].tmpref.buffer = (void*)outdata;
    op.params[2].tmpref.size = TPM_BUFSIZE;

    tpmtee_debug("tmptee_send: SEND");
    rc = TEEK_InvokeCommand(send_session, TPM_SEND_COMMAND, &op, &err_origin);
    if (rc)
        goto err_invoke;
    tee_have_data = true;
    tee_data_size = op.params[2].tmpref.size;
    tpmtee_debug("tmptee_send: return size=%zd",tee_data_size);
    return count;

err_invoke:
    tpmtee_error("send data to tpmta error");
#ifdef TPMTEE_PRINT_DEBUG
    dump_stack();
#endif
    return -EIO;
}

static int tpmtee_send_normal(struct tpm_chip *chip, u8 *buf, size_t count)
{
    TEEC_Session session;
    TEEC_Context context;
    int ret = 0;
    ret = tpmtee_init_tpmtasession(&session,&context);
    if (ret) {
        tpmtee_error("tpmtee init normal session failed,ret = %d",ret);
        return ret;
    }
    ret = tpmtee_send_to_tpmta(chip, buf,count,&session);
    if (ret < 0) {
        tpmtee_error("tpmtee send to tpmta failed,ret = %d",ret);
    }

    TEEK_CloseSession(&session);
    TEEK_FinalizeContext(&context);
    return ret;
}

struct tpmtee_async_send_work {
    struct tpm_chip *chip;
    u8 *buf;
    size_t count;
    struct work_struct work;
};

#define WAIT_TPMQUEUE_TIMES (200)
static struct workqueue_struct *tpmtee_workqueue;
static bool tpmtee_work_queue_IsWorking = false;

static void tpmtee_async_send_do_work(struct work_struct *do_wrok)
{
    int ret = 0;
    struct tpmtee_async_send_work *tpmtee_work_node = container_of(do_wrok, struct tpmtee_async_send_work, work);

    ret = tpmtee_send_normal(tpmtee_work_node->chip, tpmtee_work_node->buf,tpmtee_work_node->count);
    if (ret < 0) {
        tpmtee_error("tpmtee send to tpmta failed,ret = %d",ret);
    }

    tpmtee_work_queue_IsWorking = false;
    kfree(tpmtee_work_node);
    tpmtee_debug("tpmtee  async send do work finished,ret = [%d]",ret);
    return;
}

static bool tpmtee_cmd_can_send_workqueue(u8 *buf, size_t count)
{
    struct tpm_input_header *ptpm_cmd_header;
    if (count < sizeof(struct tpm_input_header)) {
        return false;
    }
    ptpm_cmd_header = (struct tpm_input_header *)buf;
    if (ptpm_cmd_header->ordinal == cpu_to_be32(TPM2_CC_PCR_EXTEND)) {
        tpmtee_warning("this cmd can send workqueue , cmd = [%#x]",ptpm_cmd_header->ordinal);
        return true;
    }
    return false;
}

static int tpmtee_send_by_workqueue(struct tpm_chip *chip, u8 *buf, size_t count)
{
    int i = 0;
    struct tpmtee_async_send_work *tpmtee_work_node;

    tpmtee_debug("init_globel_sessionadd work queue begin!");
    tpmtee_work_node = kmalloc(sizeof(struct tpmtee_async_send_work), GFP_KERNEL);
    if (!tpmtee_work_node) {
        tpmtee_error("alloc tpmtee_work_node mem failed");
        return -ENOMEM;
    }

    tpmtee_work_node->chip = chip;
    tpmtee_work_node->buf  = buf;
    tpmtee_work_node->count = count;
    tpmtee_work_queue_IsWorking = true;
    INIT_WORK(&tpmtee_work_node->work, tpmtee_async_send_do_work);

    if (0 == queue_work(tpmtee_workqueue, &tpmtee_work_node->work)) {
        tpmtee_error("send data async add work queue failed!");
        kfree(tpmtee_work_node);
        return TPMTEE_ERROR;
    }

    for (i = 0;i < WAIT_TPMQUEUE_TIMES; i++) {
        if (!tpmtee_work_queue_IsWorking) {
            break;
        }
        msleep(10);
    }
    tpmtee_debug("tpmtee work queue wait finished! i=[%d]",i);
    if (i == WAIT_TPMQUEUE_TIMES) {
        tpmtee_error("wait tpmtee work queue  finish failed,tpmtee must return!");
        return TPMTEE_ERROR;//lint !e429
    }

    tpmtee_debug("tpmtee send_data_async success! pls receive");
    return count;//lint !e429
}

int tpmtee_send(struct tpm_chip *chip, u8 *buf, size_t count)
{
    if (tpmtee_cmd_can_send_workqueue(buf,count)) {
        return tpmtee_send_by_workqueue(chip,buf, count);
    }
    return tpmtee_send_normal(chip,buf, count);
}
/* Receive data from (pseudo) TPM chip */
static int tpmtee_recv(struct tpm_chip *chip, u8 *buf, size_t count)
{
    if (!tee_have_data)
        return -EIO;
    if (count < MIN_SIZE_FOR_RECV)
        return -EIO;
    if (tee_data_size > count)
        return -EIO;
    if (tee_data_size > TPM_BUFSIZE)
        return -EIO;

    tpmtee_debug( "tmptee_recv: buf=%p size=%zd rsp-size=%zd",
         buf, count, tee_data_size);
#ifdef TPMTEE_PRINT_DEBUG
    print_hex_dump(KERN_DEBUG, "tpmrecv: ", DUMP_PREFIX_ADDRESS,
               16, 1, buf, tee_data_size, true);
#endif

    tee_have_data = 0;
    memcpy_s(buf, count, outdata, tee_data_size);

    return tee_data_size;
}

static void tpmtee_cancel(struct tpm_chip *chip)
{
    dev_err(&chip->dev, "tmptee_cancel\n");
}

static u8 tpmtee_status(struct tpm_chip *chip)
{
#ifdef TPMTEE_PRINT_DEBUG
    dev_err(&chip->dev, "tmptee_status\n");
#endif
    return 0;
}

static const struct tpm_class_ops tpmtee = {
    .recv = tpmtee_recv,
    .send = tpmtee_send,
    .cancel = tpmtee_cancel,
    .status = tpmtee_status,
    .req_complete_mask = 0,
    .req_complete_val = 0,
};

static struct platform_device *pdev;

static void tpmtee_remove(struct device *dev)
{
    struct tpm_chip *chip = dev_get_drvdata(dev);

    if (chip) {
        dev_info(&chip->dev, "tmptee_remove\n");
        tpm_chip_unregister(chip);
        /* tpm_remove_hardware(chip->dev); */
    }
}
/*add by yudong
tpm in tee do not suspend the tpm device for reasons:
    1. unecessary
    2. device suspend stack called from system_server,
    if use tpm_pm_suspend,we must add TEE Authentication for process "system_server"
    it is not easy
*/
int tpmtee_pm_suspend(struct device *dev)
{
    struct tpm_chip *chip = dev_get_drvdata(dev);

    if (chip == NULL)
        return -ENODEV;

    //dev_err(chip->pdev, "tpmtee_pm_suspend,dump_stack: \n");
    //dump_stack();
    return 0;
}
int tpmtee_pm_resume(struct device *dev)
{
    struct tpm_chip *chip = dev_get_drvdata(dev);

    if (chip == NULL)
        return -ENODEV;

    //dev_err(chip->pdev, "tpmtee_pm_resume,dump_stack: \n");
    //dump_stack();
    return 0;
}

static SIMPLE_DEV_PM_OPS(tpmtee_pm, tpmtee_pm_suspend, tpmtee_pm_resume);
//static SIMPLE_DEV_PM_OPS(tpmtee_pm, tpm_pm_suspend, tpm_pm_resume);

static struct platform_driver tpmtee_drv = {
    .driver          = {
        .name    = "tpmtee",
        .owner   = THIS_MODULE,
        .pm      = &tpmtee_pm,
    },
};

static int is_bad(void *p)
{
    if (!p)
        return 1;
    if (IS_ERR(p) && (PTR_ERR(p) != -ENODEV))
        return 1;
    return 0;
}

#ifdef HISI_TRUSTBOOT  //trustboot code by yudong
void tpmtee_print_digest(struct seq_file *m, u8 *digest, u32 size)
{
    u32 i;

    for (i = 0; i < size; i++)
        seq_printf(m, "%02x", *(digest + i));
}

static int tpmtee_trustboot_log_dump(struct seq_file *s, void *p)
{
    unsigned int i;

    if ((u32)TrustBootHashs.count > SEC_IMG_MAX) {
        tpmtee_error("tpmtee trustboot log dump error, TrustBootHashs  count= 0x%x", TrustBootHashs.count);
        return 1;
    }
    for (i = 0; i < TrustBootHashs.count; i++) {
        /*1st: PCR index*/
        seq_printf(s, "%2d ", TrustBootHashsPcrIndex[i]);
        /* 2nd: SHA256 template hash */
        tpmtee_print_digest(s, TrustBootHashs.secimg[i].hash, TRUST_BOOT_DIGEST_SIZE);

        /* 3th:  template name */
        tpmtee_debug("tpmtee trustboot log dump template name = [%s]", TrustBootHashs.secimg[i].name);
        seq_printf(s, " %s", TrustBootHashs.secimg[i].name);
        seq_puts(s, "\n");
    }
    return 0;
}

static int tpmtee_trustboot_log_open(struct inode *inode, struct file *file)
{
    return single_open(file, &tpmtee_trustboot_log_dump, NULL);
}



static const struct file_operations tpmtee_trustboot_log_ops = {
    .open = tpmtee_trustboot_log_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

#endif
static int __init tpmtee_init(void)
{
    int rc = 0;
    int err;
    struct tpm_chip *chip;
    struct dentry *tpmtee_dir;

#ifdef HISI_TRUSTBOOT  //trustboot code by yudong
    struct dentry *trustboot_measurements;
#endif
    tpmtee_info("tpmtee_init begin!");
    err = platform_driver_register(&tpmtee_drv);
    if (err)
        return err;

    pdev = platform_device_alloc("tpm_tmpl0", -1);
    if (!pdev) {
        rc = -ENOMEM;
        goto err_unreg_drv;
    }

    pdev->num_resources = 0;
    pdev->dev.driver = &tpmtee_drv.driver;
    pdev->dev.release = tpmtee_remove;

    rc = platform_device_add(pdev);
    if (rc < 0)
        goto err_put_dev;

    chip = tpmm_chip_alloc(&pdev->dev, &tpmtee);
    if (IS_ERR(chip)) {
        rc = -ENODEV;
        goto err_rel_reg;
    }
    chip->flags = TPM_CHIP_FLAG_TPM2;

    rc = tpm_chip_register(chip);
    if (rc)
        goto err_rel_reg;

    indata = (u8*)kmalloc(TPM_BUFSIZE, GFP_KERNEL);
    if (!indata) {
        rc = -ENOMEM;
        dev_err(&chip->dev, "tmptee_init malloc indata error\n");
        goto err_rel_reg;
    }
    outdata = (u8*)kmalloc(TPM_BUFSIZE, GFP_KERNEL);
    if (!outdata) {
        rc = -ENOMEM;
        dev_err(&chip->dev, "tmptee_init malloc outdata error\n");
        goto err_rel_reg;
    }
    tpmtee_dir = securityfs_create_dir(dev_name(&chip->dev), NULL);
    if (is_bad(tpmtee_dir)) {
        rc = -ENODEV;
        dev_err(&chip->dev, "tmptee_init create tpm0 securityfs error\n");
        goto err_sec_tpmdir;
    }
#ifdef HISI_TRUSTBOOT
    trustboot_measurements = securityfs_create_file("trustboot_measurements",S_IRUSR | S_IRGRP, tpmtee_dir,
                        NULL, &tpmtee_trustboot_log_ops);
    if (is_bad(trustboot_measurements)) {
        rc = -ENODEV;
        dev_err(&chip->dev, "tmptee_init create trustboot_measurements securityfs error\n");
        goto err_trustboot_logfile;
    }
#endif
    /* init workqueue */
    tpmtee_workqueue = create_singlethread_workqueue("TPM");
    if (!tpmtee_workqueue) {
        dev_err(&chip->dev, "tmptee_init create tpm workqueue error\n");
        goto error_tpmworkqueue;
    }
    dev_err(&chip->dev, "tmptee_init complete\n");
    return 0;

error_tpmworkqueue:
    if (tpmtee_workqueue != NULL) {
        destroy_workqueue(tpmtee_workqueue);
        tpmtee_workqueue = NULL;
    }
#ifdef HISI_TRUSTBOOT  //trustboot code by yudong
err_trustboot_logfile:
    if (trustboot_measurements != NULL) {
        securityfs_remove(trustboot_measurements);
        trustboot_measurements = NULL;
    }
#endif
err_sec_tpmdir:
    if (tpmtee_dir != NULL) {
        securityfs_remove(tpmtee_dir);
        tpmtee_dir = NULL;
    }
err_rel_reg:
    platform_device_del(pdev);
    if (indata != NULL) {
        kfree(indata);
    }
    if (outdata != NULL) {
        kfree(outdata);
    }
err_put_dev:
    platform_device_put(pdev);
err_unreg_drv:
    platform_driver_unregister(&tpmtee_drv);
    return rc;
}

static void __exit tpmtee_cleanup(void)
{
    if (pdev) {
        tpmtee_remove(&pdev->dev);
        platform_device_unregister(pdev);
    }

    platform_driver_unregister(&tpmtee_drv);
}

module_init(tpmtee_init);
module_exit(tpmtee_cleanup);

MODULE_AUTHOR("Markku Kylänpää (markku.kylanpaa@vtt.fi)");
MODULE_DESCRIPTION("TPM TEE Driver");
MODULE_VERSION("2.0");
MODULE_LICENSE("GPL");


