#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/list.h>
#include <linux/atomic.h>
#include <linux/ion.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/hisi/hisi-iommu.h>
#include <linux/hisi/ion-iommu.h>
#include "ivp.h"
#include "ivp_log.h"
#include "ivp_core.h"
#include "ivp_platform.h"
#include "libhwsecurec/securec.h"
//lint -save -e785 -e64 -e715 -e838 -e747 -e712 -e737 -e64 -e30 -e438 -e713 -e713
//lint -save -e529 -e838 -e438 -e774 -e826 -e775 -e730 -e730 -e528 -specific(-e528)
//lint -save -e753 -specific(-e753) -e1058

#define DEFAULT_MSG_SIZE    (32)
#define MAX_FD_NUM          (DEFAULT_MSG_SIZE/sizeof(unsigned int) - 3)   //head + fdnum + sharefd= 3, now 5

#define     CMD_SELECTALGO          (0x10)
#define     CMD_ALOGPARAM           (0x11)
#define     CMD_ALGOINIT            (0x12)
#define     CMD_ALGORUN             (0x13)
#define     CMD_ALGOEXIT            (0x14)
#define     CMD_MEMORYALLOC         (0x40)
#define     CMD_MEMORYFREE          (0x41)
#define     CMD_SHARE_MEM_ALLOC     (0x42)
#define     CMD_DUMPIMAGE           (0x60)
#define     CMD_LOADIMAGE           (0x61)
#define     CMD_RPC_INVOKE          (0x68)
#define     CMD_LOOPBACK            (0x7F)

struct ivp_ipc_packet {
    char *buff;
    size_t len;
    struct list_head list;
};

struct ivp_ipc_queue {
    struct list_head head;

    spinlock_t rw_lock;
    struct semaphore r_lock;

    atomic_t flush;
};

struct ivp_ipc_device {
    struct miscdevice device;

    struct notifier_block recv_nb;
    struct ivp_ipc_queue recv_queue;

    atomic_t accessible;

    rproc_id_t recv_ipc;
    rproc_id_t send_ipc;
};

extern struct ivp_ipc_device ivp_ipc_dev;
extern struct ivp_device ivp_dev;
static struct mutex ivp_ipc_ion_mutex;
static struct mutex ivp_ipc_read_mutex;

static const struct of_device_id ivp_ipc_of_descriptor[] = {
        {.compatible = "hisilicon,hisi-ivp-ipc",},
        {},
};
MODULE_DEVICE_TABLE(of, hisi_mdev_of_match);

/*======================================================================
 * IPC Pakcet Operations
 * include get, put, remove, replace
 ======================================================================*/
static inline struct ivp_ipc_packet *ivp_ipc_alloc_packet(size_t len)
{
    struct ivp_ipc_packet *packet = NULL;

    packet = kzalloc(sizeof(struct ivp_ipc_packet), GFP_ATOMIC);
    if (packet == NULL) {
        ivp_err("malloc packet fail.");
        return NULL;
    }

    packet->buff = kzalloc(sizeof(char) * len, GFP_ATOMIC);
    if (packet->buff == NULL) {
        ivp_err("malloc packet buf fail.");
        kfree(packet);
        return NULL;
    }

    packet->len = len;

    return packet;
}

static inline void ivp_ipc_free_packet(struct ivp_ipc_packet *packet)
{
    if (packet == NULL) {
        ivp_err("packet is NULL.");
        return;
    }

    if (packet->buff != NULL) {
        kfree(packet->buff);
        packet->buff = NULL;
    }
    kfree(packet);
}

static inline void ivp_ipc_init_queue(struct ivp_ipc_queue *queue)
{
    spin_lock_init(&queue->rw_lock);
    sema_init(&queue->r_lock, 0);
    atomic_set(&queue->flush, 1);
    INIT_LIST_HEAD(&queue->head);
}

static struct ivp_ipc_packet *ivp_ipc_get_packet(struct ivp_ipc_queue *queue)
{
    struct ivp_ipc_packet *packet = NULL;

    ivp_dbg("get packet");
    spin_lock_irq(&queue->rw_lock);
    packet = list_first_entry_or_null(&queue->head, struct ivp_ipc_packet, list);
    spin_unlock_irq(&queue->rw_lock);

    return packet;
}

static int ivp_ipc_add_packet(struct ivp_ipc_queue *queue, void *data, size_t len)
{
    struct ivp_ipc_packet *new_packet = NULL;
    int ret = 0;

    len = (len>DEFAULT_MSG_SIZE) ? DEFAULT_MSG_SIZE : len;
    new_packet = ivp_ipc_alloc_packet(DEFAULT_MSG_SIZE);
    if (NULL == new_packet) {
        ivp_err("new packet NULL");
        ret = -ENOMEM;
        goto ipc_exit;
    }

    memcpy_s(new_packet->buff, DEFAULT_MSG_SIZE, data, len);

    spin_lock_irq(&queue->rw_lock);
    list_add_tail(&new_packet->list, &queue->head);
    spin_unlock_irq(&queue->rw_lock);

    up(&queue->r_lock);

ipc_exit:
    return ret;
}
/*only remove one packet, the caller guarantees mutual exclusion*/
static void ivp_ipc_remove_one_packet(struct ivp_ipc_queue *queue, struct ivp_ipc_packet *packet)
{
    if (packet) {
        list_del(&packet->list);
    }
    ivp_ipc_free_packet(packet);
}

static void ivp_ipc_remove_packet(struct ivp_ipc_queue *queue, struct ivp_ipc_packet *packet)
{
    spin_lock_irq(&queue->rw_lock);
    ivp_ipc_remove_one_packet(queue, packet);
    spin_unlock_irq(&queue->rw_lock);
}

static void ivp_ipc_remove_all_packet(struct ivp_ipc_queue *queue)
{
    struct list_head *p = NULL, *n = NULL;

    spin_lock_irq(&queue->rw_lock);
    list_for_each_safe(p, n, &queue->head) {
        struct ivp_ipc_packet *packet = list_entry(p, struct ivp_ipc_packet, list);
        ivp_ipc_remove_one_packet(queue, packet);
    }
    spin_unlock_irq(&queue->rw_lock);
}

static int ivp_ipc_open(struct inode *inode, struct file *file)
{
    struct ivp_ipc_device *pdev = &ivp_ipc_dev;
    int ret = 0;

    if (!atomic_dec_and_test(&pdev->accessible)) {
        ivp_err("maybe ivp ipc dev has been opened!");
        atomic_inc(&pdev->accessible);
        return -EBUSY;
    }

    ivp_dbg("enter");
    ret = nonseekable_open(inode, file);
    if (ret != 0) {
        atomic_inc(&pdev->accessible);
        return ret;
    }
    file->private_data = (void *)&ivp_ipc_dev;

    if (unlikely(1 != atomic_read(&ivp_ipc_dev.recv_queue.flush))) {
        ivp_warn("Flush was not set when first open! %u",
                  atomic_read(&(ivp_ipc_dev.recv_queue.flush)));
    }
    atomic_set(&ivp_ipc_dev.recv_queue.flush, 0);

    if (unlikely(!list_empty(&ivp_ipc_dev.recv_queue.head))) {
        ivp_warn("queue is not Empty!");
        ivp_ipc_remove_all_packet(&ivp_ipc_dev.recv_queue);
    }

    sema_init(&(ivp_ipc_dev.recv_queue.r_lock), 0);

    return ret;
}
static int ivp_trans_sharefd_to_phyaddr(unsigned int* buff)
{
    int ret;
    unsigned int i;
    unsigned int share_fd = 0;
    unsigned int fd_num = 0;
    unsigned int sec_fd_num = 0;
    unsigned int nosec_fd_num = 0;
    struct ion_client *ivp_ipc_fd_client;
    struct ion_handle *ivp_ion_fd_handle;
    ion_phys_addr_t ion_phy_addr = 0x0 ;
    mutex_lock(&ivp_ipc_ion_mutex);
    ivp_ipc_fd_client = hisi_ion_client_create("ivp_ipc_fd_client");
    if (IS_ERR(ivp_ipc_fd_client)) {
        ivp_err("ivp_ipc_fd_client create failed!\n");
        goto err_create_client;
    }
    //the second field is sharefd number according to the algo arg struct
    buff++;
    fd_num = *buff++;
    sec_fd_num = fd_num&0xFFFF;
    nosec_fd_num = (fd_num >> 16)&0xFFFF;
    /*fd_num indicate the followed shared_fd number, it should not exceed the
    buffer size(32), buff size = one cmd + one fdnum + fdnum*shard_fd + ..*/
    if (((sec_fd_num+nosec_fd_num) > MAX_FD_NUM)||(0 == sec_fd_num))
    {
        ivp_err("ion buff number maybe wrong, num=%d\n", fd_num);
        goto err_ion_buff_num;
    }
    //trans sec buff phyaddr, phyaddr = phyaddr_begin+offset
    share_fd = *buff++;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0))
    ivp_ion_fd_handle = ion_import_dma_buf(ivp_ipc_fd_client, share_fd);
#else
    ivp_ion_fd_handle = ion_import_dma_buf_fd(ivp_ipc_fd_client, share_fd);
#endif
    if (IS_ERR(ivp_ion_fd_handle)){
        ivp_err("%d, ion_import_dma_buf failed!\n", __LINE__);
        goto err_import_handle;
    }
    ret = ivp_ion_phys(ivp_ipc_fd_client, ivp_ion_fd_handle, (dma_addr_t *)&ion_phy_addr);
    if (ret < 0){
        ivp_err("%d, ion_phys failed, result=%d\n", __LINE__, ret);
        goto err_ion_phys;
    }
    for (i = 0; i < sec_fd_num; i++)
    {
        *buff++ += ion_phy_addr;
    }
    ion_free(ivp_ipc_fd_client, ivp_ion_fd_handle);

    //trans nosec buff phyaddr
    for (i = 0; i < nosec_fd_num; i++)
    {
        share_fd = *buff;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0))
        ivp_ion_fd_handle = ion_import_dma_buf(ivp_ipc_fd_client, share_fd);
#else
        ivp_ion_fd_handle = ion_import_dma_buf_fd(ivp_ipc_fd_client, share_fd);
#endif
        if (IS_ERR(ivp_ion_fd_handle)){
            ivp_err("%d, ion_import_dma_buf failed!\n", __LINE__);
            goto err_import_handle;
        }
        ret = ivp_ion_phys(ivp_ipc_fd_client, ivp_ion_fd_handle, (dma_addr_t *)&ion_phy_addr);
        if (ret < 0){
            ivp_err("%d, ion_phys failed, result=%d\n", __LINE__, ret);
            goto err_ion_phys;
        }
        *buff++ = ion_phy_addr;
        ion_free(ivp_ipc_fd_client, ivp_ion_fd_handle);
    }
    ion_client_destroy(ivp_ipc_fd_client);
    mutex_unlock(&ivp_ipc_ion_mutex);
    return ret;

err_ion_phys:
    ion_free(ivp_ipc_fd_client, ivp_ion_fd_handle);
err_ion_buff_num:
err_import_handle:
    ion_client_destroy(ivp_ipc_fd_client);
err_create_client:
    mutex_unlock(&ivp_ipc_ion_mutex);

    return -EFAULT;
}
/******************************************************************************************
 *  len:   msg len. data len is (len * sizeof(msg))
 ** ***************************************************************************************/
static int ivp_ipc_recv_notifier(struct notifier_block *nb, unsigned long len, void *data)
{
    struct ivp_ipc_device *pdev = NULL;
    int ret = 0;

    if (NULL == data || NULL == nb) {
        ivp_err("data or nb is NULL");
        return -EINVAL;
    }

    if (0 >= len) {
        ivp_err("len equals to or less than 0");
        return -EINVAL;
    }

    pdev = container_of(nb, struct ivp_ipc_device, recv_nb);
    if (NULL == pdev) {
        ivp_err("pdev NULL");
        return -EINVAL;
    }

    if (1 == atomic_read(&(pdev->recv_queue.flush))) {
        ivp_err("flushed.No longer receive msg.");
        ret = -ECANCELED;

    } else {
        len *= 4;
        ret = ivp_ipc_add_packet(&pdev->recv_queue, data, len);
    }

    return ret;
}

static ssize_t ivp_ipc_read(struct file *file,
                                   char __user *buff,
                                   size_t size,
                                   loff_t *off)
{
    struct ivp_ipc_device *pdev = (struct ivp_ipc_device *) file->private_data;
    struct ivp_ipc_queue *queue = &pdev->recv_queue;
    struct ivp_ipc_packet *packet = NULL;
    ssize_t ret = 0;

    if (NULL == buff) {
        ivp_err("buff is null!");
        return -EINVAL;
    }

    if (DEFAULT_MSG_SIZE != size) {
        ivp_err("Size should be 32Byte.size:%lu", size);
        return -EINVAL;
    }

    //Block until IVPCore send new Msg
    if (down_interruptible(&queue->r_lock)) {
        ivp_err("interrupted.");
        return -ERESTARTSYS;
    }

    if (1 == atomic_read(&queue->flush)) {
        ivp_err("flushed.");
        return -ECANCELED;
    }
    mutex_lock(&ivp_ipc_read_mutex);
    packet = ivp_ipc_get_packet(queue);
    if (packet != NULL) {
        if (copy_to_user(buff, packet->buff, size)) {
            ivp_err("copy to user fail.");
            ret = -EFAULT;
            goto OUT;
        }

    } else {
        ivp_err("get packet NULL");
        mutex_unlock(&ivp_ipc_read_mutex);
        return -EINVAL;
    }
    ivp_info("send ipc cmd 0x%x,0x%x,0x%x,0x%x", packet->buff[0], packet->buff[1], packet->buff[2], packet->buff[3]);

    *off += size;
    ret = size;

OUT:
    ivp_ipc_remove_packet(queue, packet);
    mutex_unlock(&ivp_ipc_read_mutex);
    return ret;
}

static ssize_t ivp_ipc_write(struct file *file,
                                    const char __user *buff,
                                    size_t size,
                                    loff_t *off)
{
    struct ivp_ipc_device *pdev = (struct ivp_ipc_device *)file->private_data;
    char *tmp_buff = NULL;
    ssize_t ret = 0;

    if (NULL == buff) {
        ivp_err("buff is null!");
        return -EINVAL;
    }

    if (size != DEFAULT_MSG_SIZE) {
        ivp_err("size %lu not %d.", size, DEFAULT_MSG_SIZE);
        return -EINVAL;
    }

    tmp_buff = kzalloc((unsigned long)DEFAULT_MSG_SIZE, GFP_KERNEL);
    if (tmp_buff == NULL) {
        ivp_err("malloc buf failed.");
        return -ENOMEM;
    }

    if (copy_from_user(tmp_buff, buff, size)) {
        ivp_err("copy from user fail.");
        ret = -EFAULT;
        goto OUT;
    }
    //trans ion fd to phyaddr
    if (is_ivp_in_secmode()) {
        //the third char is ipc cmd,the first char is msg index
        if ((CMD_ALGORUN == (tmp_buff[3] & 0x7F)) && (0 == tmp_buff[0])) {
            ret = ivp_trans_sharefd_to_phyaddr((unsigned int *)tmp_buff);
            if (ret < 0) {
                ivp_err("ivp trans fd fail! ret=%ld\n", ret);
                goto OUT;
            }
        }
    }
    if (0 == tmp_buff[0])
    {
        ivp_info("receive ipc cmd 0x%x",(tmp_buff[3] & 0x7F));
    }
    ret = RPROC_ASYNC_SEND(pdev->send_ipc, (rproc_msg_t *) tmp_buff, size/sizeof(rproc_msg_len_t));
    if (ret) {
        ivp_err("ipc send fail [%ld].", ret);
        goto OUT;
    }

    *off += size;
    ret = size;

OUT:
    kfree(tmp_buff);
    return ret;
}

static int ivp_ipc_release(struct inode *inode, struct file *file)
{
    struct ivp_ipc_device *pdev = (struct ivp_ipc_device *)file->private_data;

    if (atomic_read(&pdev->accessible) != 0) {
        ivp_err("maybe ivp dev not opened!");
        return -1;
    }

    ivp_info("enter");
    //drop all packet
    mutex_lock(&ivp_ipc_read_mutex);
    ivp_ipc_remove_all_packet(&pdev->recv_queue);
    mutex_unlock(&ivp_ipc_read_mutex);

    atomic_inc(&pdev->accessible);

    return 0;
}

static int ivp_ipc_flush(struct ivp_ipc_device *pdev)
{
    //non block read.Make read return HAL.
    struct ivp_ipc_queue *queue = &pdev->recv_queue;
    int ret = 0;

    ivp_info("enter");
    atomic_set(&queue->flush, 1);
    up(&queue->r_lock);
    mutex_lock(&ivp_ipc_read_mutex);
    ivp_ipc_remove_all_packet(queue);
    mutex_unlock(&ivp_ipc_read_mutex);

    return ret;
}

static long ivp_ipc_ioctl(struct file *fd, unsigned int cmd, unsigned long args)
{
    struct ivp_ipc_device *pdev = (struct ivp_ipc_device *)fd->private_data;
    int ret = 0;
    ivp_info("cmd:%#x", cmd);

    ivp_dbg("IVP_IOCTL_IPC_FLUSH_ENABLE:%#lx", IVP_IOCTL_IPC_FLUSH_ENABLE);
    switch(cmd) {
    case IVP_IOCTL_IPC_FLUSH_ENABLE:
        ret = ivp_ipc_flush(pdev);
        break;

    default:
        ivp_err("invalid cmd, %#x", cmd);
        ret = -EINVAL;
        break;
    }

    return ret;
}

static long ivp_ipc_ioctl32(struct file *fd, unsigned int cmd, unsigned long args)
{
    void *user_ptr = compat_ptr(args);
    return ivp_ipc_ioctl(fd, cmd, (unsigned long)user_ptr);
}

static struct file_operations ivp_ipc_fops = {
    .owner = THIS_MODULE,
    .open = ivp_ipc_open,
    .read = ivp_ipc_read,
    .write = ivp_ipc_write,
    .release = ivp_ipc_release,
    .unlocked_ioctl = ivp_ipc_ioctl,
    .compat_ioctl = ivp_ipc_ioctl32,
};

struct ivp_ipc_device ivp_ipc_dev = {
    .device = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "ivp-ipc",
        .fops = &ivp_ipc_fops,
    },
    .send_ipc = HISI_RPROC_IVP_MBX25,
    .recv_ipc = HISI_RPROC_IVP_MBX5,
};

static inline void ivp_ipc_init_recv_nb(struct notifier_block *nb)
{
    nb->notifier_call = ivp_ipc_recv_notifier;
}

static int ivp_ipc_probe(struct platform_device *platform_pdev)
{
    struct ivp_ipc_device *pdev = &ivp_ipc_dev;
    int ret = 0;

    atomic_set(&pdev->accessible, 1);

    ret = misc_register(&pdev->device);
    if (ret < 0) {
        ivp_err("Failed to register misc device.");
        return ret;
    }
    mutex_init(&ivp_ipc_ion_mutex);
    mutex_init(&ivp_ipc_read_mutex);
    ret = RPROC_MONITOR_REGISTER((unsigned char)ivp_ipc_dev.recv_ipc, &ivp_ipc_dev.recv_nb);
    if (ret < 0) {
        ivp_err("Failed to create receiving notifier block");
        goto err_out;
    }

    ivp_ipc_init_queue(&pdev->recv_queue);

    ivp_ipc_init_recv_nb(&pdev->recv_nb);

    platform_set_drvdata(platform_pdev, pdev);

    return ret;

err_out:
    mutex_destroy(&ivp_ipc_ion_mutex);
    mutex_destroy(&ivp_ipc_read_mutex);
    misc_deregister(&pdev->device);
    return ret;
}

static int ivp_ipc_remove(struct platform_device *plat_devp)
{
    RPROC_MONITOR_UNREGISTER((unsigned char)ivp_ipc_dev.recv_ipc, &ivp_ipc_dev.recv_nb);
    mutex_destroy(&ivp_ipc_ion_mutex);
    mutex_destroy(&ivp_ipc_read_mutex);
    misc_deregister(&ivp_ipc_dev.device);
    return 0;
}

static struct platform_driver ivp_ipc_driver = {
    .probe = ivp_ipc_probe,
    .remove = ivp_ipc_remove,
    .driver = {
        .name = "ivp-ipc",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(ivp_ipc_of_descriptor),
    }, //lint -e785
}; //lint -e785

module_platform_driver(ivp_ipc_driver); //lint -e528 -e64
//MODULE_LICENSE("GPL");
//lint -restore
