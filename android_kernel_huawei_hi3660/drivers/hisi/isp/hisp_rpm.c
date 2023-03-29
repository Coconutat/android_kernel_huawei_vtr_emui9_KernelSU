/*
 * Histarisp rpmsg client driver
 *
 * Copyright (c) 2013- Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*lint -e666  -e529 -e438 -e713 -e715 -e559 -e626 -e719 -e846 -e514 -e778 -e866 -e84 -e437 -esym(666,*) -esym(529,*) -esym(438,*) -esym(713,*) -esym(715,*) -esym(559,*) -esym(626,*) -esym(719,*) -esym(846,*) -esym(514,*) -esym(778,*) -esym(866,*) -esym(84,*) -esym(437,*)*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/rpmsg.h>
#include <linux/completion.h>
#include <uapi/linux/histarisp.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include <linux/ion.h>
#include <linux/dma-buf.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/version.h>
#include <linux/miscdevice.h>

static int debug_mask = 0x3;
#define rpmsg_err(fmt, args...) \
    do {                         \
        if (debug_mask & 0x01)   \
            printk(KERN_INFO "Rpmsg HISI Err: [%s] " fmt, __func__, ##args); \
    } while (0)
#define rpmsg_info(fmt, args...)  \
    do {                         \
        if (debug_mask & 0x02)   \
            printk(KERN_INFO "Rpmsg HISI Info: [%s] " fmt, __func__, ##args);  \
    } while (0)
#define rpmsg_dbg(fmt, args...)  \
    do {                         \
        if (debug_mask & 0x04) \
            printk(KERN_INFO "Rpmsg HISI Debug: [%s] " fmt, __func__, ##args); \
    } while (0)

/**
 * struct rpmsg_hdr - common header for all rpmsg messages
 * @src: source address
 * @dst: destination address
 * @reserved: reserved for future use
 * @len: length of payload (in bytes)
 * @flags: message flags
 * @data: @len bytes of message payload data
 *
 * Every message sent(/received) on the rpmsg bus begins with this header.
 */
struct rpmsg_hdr {
	u32 src;
	u32 dst;
	u32 reserved;
	u16 len;
	u16 flags;
	u8 data[0];
} __packed;


struct rpmsg_hisi_service {
    struct rpmsg_device *rpdev;
};

struct hisp_rpmsgrefs_s {
    atomic_t sendin_refs;
    atomic_t sendx_refs;
    atomic_t recvtask_refs;
    atomic_t recvthread_refs;
    atomic_t recvin_refs;
    atomic_t recvcb_refs;
    atomic_t recvdone_refs;
    atomic_t rdr_refs;
};
struct hisp_rpmsgrefs_s *hisp_rpmsgrefs = NULL;
static struct rpmsg_hisi_service hisi_isp_serv;
struct completion channel_sync;
struct rpmsg_channel_info chinfo = {
    .src = RPMSG_ADDR_ANY,
};

int rpmsg_client_debug = INVALID_CLIENT;
static void hisp_rpmsg_rdr_save(unsigned int num, unsigned int type, void *data)
{
    return;
}
int hisp_rpmsg_rdr_init(void)
{
    struct hisp_rpmsgrefs_s *dev = NULL;
    dev = kzalloc(sizeof(struct hisp_rpmsgrefs_s), GFP_KERNEL);
    if (!dev) {
        pr_err("%s: kzalloc failed, size.0x%lx\n", __func__,sizeof(struct hisp_rpmsgrefs_s));
        return -ENOMEM;
    }
    pr_info("%s: kzalloc size.0x%lx\n", __func__,sizeof(struct hisp_rpmsgrefs_s));
    hisp_rpmsgrefs = dev;
    return 0;
}
int hisp_rpmsg_rdr_deinit(void)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return -ENOMEM;
    }
    kfree(dev);
    hisp_rpmsgrefs = NULL;
    return 0;
}

void hisp_sendin(void *data)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;

    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return;
    }
    atomic_inc(&dev->sendin_refs);
    atomic_inc(&dev->rdr_refs);
    hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),RPMSG_CAMERA_SEND_MSG,data);
}

void hisp_sendx(void)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return;
    }
    atomic_inc(&dev->sendx_refs);
    atomic_inc(&dev->rdr_refs);
    hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),RPMSG_SEND_MSG_TO_MAILBOX,NULL);
}

void hisp_recvtask(void)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return;
    }
    atomic_inc(&dev->recvtask_refs);
    atomic_inc(&dev->rdr_refs);
    hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),RPMSG_RECV_MAILBOX_FROM_ISPCPU,NULL);
}

void hisp_recvthread(void)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return;
    }
    atomic_inc(&dev->recvthread_refs);
    atomic_inc(&dev->rdr_refs);
    hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),RPMSG_ISP_THREAD_RECVED,NULL);
}

void hisp_recvin(void *data)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return;
    }
    atomic_inc(&dev->recvin_refs);
    atomic_inc(&dev->rdr_refs);
    hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),RPMSG_RECV_SINGLE_MSG,data);
}
void hisp_recvx(void *data)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return;
    }
    atomic_inc(&dev->recvcb_refs);
    atomic_inc(&dev->rdr_refs);
    hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),RPMSG_SINGLE_MSG_TO_CAMERA,data);
}
void hisp_recvdone(void *data)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return;
    }
    atomic_inc(&dev->recvdone_refs);
    atomic_inc(&dev->rdr_refs);
    hisp_rpmsg_rdr_save(atomic_read(&dev->rdr_refs),RPMSG_CAMERA_MSG_RECVED,data);
}

void hisp_rpmsgrefs_dump(void)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    pr_info("camera send info: sendin_refs.0x%x, sendx_refs.0x%x\n", atomic_read(&dev->sendin_refs),atomic_read(&dev->sendx_refs));
    pr_info("Rpmsg  recv info: recvtask_refs.0x%x, recvthread_refs.0x%x, recvin_refs.0x%x\n", atomic_read(&dev->recvtask_refs), atomic_read(&dev->recvthread_refs), atomic_read(&dev->recvin_refs));
    pr_info("camera recv info: recvcb_refs.0x%x, recvdone_refs.0x%x, total: rdr_refs.0x%x\n", atomic_read(&dev->recvcb_refs),atomic_read(&dev->recvdone_refs),atomic_read(&dev->rdr_refs));
}

void hisp_rpmsgrefs_reset(void)
{
    struct hisp_rpmsgrefs_s *dev = hisp_rpmsgrefs;
    if(dev == NULL) {
        pr_err("[%s] Failed: hisp_rpmsgrefs_s is null\n", __func__);
        return;
    }
    atomic_set(&dev->sendin_refs, 0);
    atomic_set(&dev->sendx_refs, 0);
    atomic_set(&dev->recvtask_refs, 0);
    atomic_set(&dev->recvthread_refs, 0);
    atomic_set(&dev->recvin_refs, 0);
    atomic_set(&dev->recvcb_refs, 0);
    atomic_set(&dev->recvdone_refs, 0);
    atomic_set(&dev->rdr_refs, 0);
}
void hisp_dump_rpmsg_with_id(const unsigned int message_id)
{
    return;
}

static int rpmsg_hisi_probe(struct rpmsg_device *rpdev)
{
    struct rpmsg_hisi_service *hisi_serv = &hisi_isp_serv;

    hisi_serv->rpdev = rpdev;

    rpmsg_info("new HISI connection srv channel: %u -> %u!\n",
                        rpdev->src, rpdev->dst);

    rpmsg_dbg("Exit ...\n");
    return 0;
}

static void rpmsg_hisi_remove(struct rpmsg_device *rpdev)
{
    rpmsg_info("Exit ...\n");
    return;
}

static int rpmsg_hisi_driver_cb(struct rpmsg_device *rpdev, void *data, int len, void *priv, u32 src)
{
    rpmsg_dbg("Enter ...\n");
    dev_warn(&rpdev->dev, "uhm, unexpected message\n");

    print_hex_dump(KERN_DEBUG, __func__, DUMP_PREFIX_NONE, 16, 1,
               data, len,  true);
    rpmsg_dbg("Exit ...\n");
    return 0;
}

static struct rpmsg_device_id rpmsg_hisi_id_table[] = {
    { .name    = "rpmsg-isp-debug" },
    { },
};
MODULE_DEVICE_TABLE(platform, rpmsg_hisi_id_table);
/*lint -save -e485*/
static struct rpmsg_driver rpmsg_hisi_driver = {
    .drv.name   = KBUILD_MODNAME,
    .drv.owner  = THIS_MODULE,
    .id_table   = rpmsg_hisi_id_table,
    .probe      = rpmsg_hisi_probe,
    .callback   = rpmsg_hisi_driver_cb,
    .remove     = rpmsg_hisi_remove,
};
/*lint -restore */


static int __init rpmsg_hisi_init(void)
{

    return register_rpmsg_driver(&rpmsg_hisi_driver);
}
module_init(rpmsg_hisi_init);

static void __exit rpmsg_hisi_exit(void)
{
    rpmsg_dbg("Enter ...\n");

    unregister_rpmsg_driver(&rpmsg_hisi_driver);


    rpmsg_dbg("Exit ...\n");
}
module_exit(rpmsg_hisi_exit);

MODULE_DESCRIPTION("HISI offloading rpmsg driver");
MODULE_LICENSE("GPL v2");
