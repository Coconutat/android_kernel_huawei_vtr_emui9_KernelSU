/**********************************************************
 * Filename:    erecovery_transtation.c
 *
 * Discription: kernel process of erecovery event
 *
 * Copyright:
 *
 * Author:
 *
**********************************************************/
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/aio.h>
#include <uapi/linux/uio.h>
#include <asm/ioctls.h>
#include <linux/errno.h>
#include "erecovery_transtation.h"
#include <chipset_common/hwerecovery/erecovery.h>
#include <erecovery_common.h>
#ifdef GTEST
#define static
#define inline
#endif

#define ERECOVERY_EVENT_SIZE     sizeof(erecovery_write_event)
#define ERECOVERY_RING_BUF_SIZE_MAX             (ERECOVERY_TOTAL_BUF_SIZE - ERECOVERY_EVENT_SIZE)

typedef struct {
    uint32_t    w;
    uint32_t    r;
} erecovery_trans_pos;

static DEFINE_MUTEX(erecovery_mutex);
static wait_queue_head_t    erecovery_wq;
static char*     erecovery_trans_buf;
static erecovery_trans_pos *        erecovery_pos;

static inline uint32_t erecovery_get_pos(uint32_t pos, uint32_t len)
{
    uint32_t new_pos = pos + len;
    if (new_pos >= ERECOVERY_RING_BUF_SIZE_MAX) {
        new_pos -= ERECOVERY_RING_BUF_SIZE_MAX;
    }
    return new_pos;
}

static int erecovery_write_from_kernel(uint32_t pos, void* k_data, uint32_t len)
{
    uint32_t part_len;

    if (NULL == k_data || pos >= ERECOVERY_RING_BUF_SIZE_MAX) {
        ERECOVERY_ERROR("pos: %d, dst:%p, len:0x%x\n", pos, k_data, len);
        return -1;
    }
    part_len = min((size_t)len, (size_t)(ERECOVERY_RING_BUF_SIZE_MAX - pos));
    memcpy(erecovery_trans_buf + pos, k_data, part_len);
    if (part_len < len) {
        memcpy(erecovery_trans_buf, k_data + part_len, len - part_len);
    }
    return 0;
}

static inline int erecovery_is_full(uint32_t len)
{
    if (erecovery_pos->w > erecovery_pos->r) {
        return (erecovery_pos->w + len - erecovery_pos->r >= ERECOVERY_RING_BUF_SIZE_MAX);
    } else if (erecovery_pos->w < erecovery_pos->r) {
        return (erecovery_pos->w + len >= erecovery_pos->r);
    } else {
        return 0;
    }
}

static inline int erecovery_is_empty(void)
{
    return (erecovery_pos->w == erecovery_pos->r);
}


static int erecovery_read_one_event(void __user *u_data)
{
    uint32_t pos, part_len;

    if (erecovery_is_empty()) {
        ERECOVERY_ERROR("buffer empty\n");
        return -1;
    }
    // get read_event pos
    pos = erecovery_get_pos(erecovery_pos->r, 0);
    // copy data to user
    part_len = min(ERECOVERY_EVENT_SIZE, (size_t)(ERECOVERY_RING_BUF_SIZE_MAX - pos));
    if (copy_to_user(u_data, erecovery_trans_buf + pos, part_len)) {
        ERECOVERY_ERROR("copy_to_user error\n");
        return -1;
    }

    if (part_len < ERECOVERY_EVENT_SIZE){
        if (copy_to_user(u_data + part_len, erecovery_trans_buf, ERECOVERY_EVENT_SIZE - part_len)) {
            ERECOVERY_ERROR("copy_to_user error\n");
            return -1;
        }
    }
    // change r
    erecovery_pos->r = erecovery_get_pos(erecovery_pos->r, ERECOVERY_EVENT_SIZE);
    return 0;
}

static long erecovery_write_event_internal(void* kernel_event)
{
    int ret;
    if (!kernel_event) {
        ERECOVERY_ERROR("param error\n");
        return -1;
    }
    erecovery_write_event *we = kernel_event;
    if(mutex_lock_interruptible(&erecovery_mutex)) {
        return -1;
    }
    if (erecovery_is_full(sizeof(erecovery_write_event))) {
        ERECOVERY_ERROR("erecovery buffer full");
        goto end;
    }
    // write into ring buffer
    ret = erecovery_write_from_kernel(erecovery_pos->w, (char*)we, sizeof(erecovery_write_event));
    if (ret) {
        goto end;
    }
    erecovery_pos->w = erecovery_get_pos(erecovery_pos->w, sizeof(erecovery_write_event));
    mutex_unlock(&erecovery_mutex);
    wake_up_interruptible(&erecovery_wq);
    return 0;
end:
    mutex_unlock(&erecovery_mutex);
    return -1;
}


long erecovery_write_event_user(void __user *argp)
{
    erecovery_write_event *we;
    int ret;
    if(!argp) {
        ERECOVERY_ERROR("param error!");
        return -1;
    }
    we = (erecovery_write_event*)vmalloc(ERECOVERY_EVENT_SIZE);
    if (!we) {
        return -1;
    }
    if (copy_from_user((void*)we, argp, sizeof(erecovery_write_event))) {
        ERECOVERY_ERROR("can not copy data from user space\n");
        vfree(we);
        return -1;
    }
    ret = erecovery_write_event_internal(we);
    vfree(we);
    return ret;
}


long erecovery_write_event_kernel(void *argp)
{
    return erecovery_write_event_internal(argp);
}


long erecovery_read_event(void __user *argp)
{
    if (!argp) {
        ERECOVERY_ERROR("param error\n");
        return -1;
    }
    if(mutex_lock_interruptible(&erecovery_mutex)) {
        return -1;
    }
    if (erecovery_is_empty()) {
        mutex_unlock(&erecovery_mutex);
        wait_event_interruptible(erecovery_wq, erecovery_pos->w != erecovery_pos->r);
        if(mutex_lock_interruptible(&erecovery_mutex)) {
            return -1;
        }
    }
    if (erecovery_read_one_event(argp)) {
        ERECOVERY_ERROR("etrans read buffer failed\n");
        mutex_unlock(&erecovery_mutex);
        return -1;
    }

    mutex_unlock(&erecovery_mutex);
    return 0;
}


static int __init erecovery_trans_init(void)
{
    int ret = 0;
    erecovery_trans_buf = (char*)vmalloc(ERECOVERY_TOTAL_BUF_SIZE);
    if (!erecovery_trans_buf) {
        ERECOVERY_ERROR("transtion not init\n");
        ret = -ENOMEM;
        goto _error;
    }
    memset(erecovery_trans_buf, 0, ERECOVERY_TOTAL_BUF_SIZE);
    ERECOVERY_INFO("malloc buf: %p, size: 0x%x\n", erecovery_trans_buf, ERECOVERY_TOTAL_BUF_SIZE);
    erecovery_pos = (erecovery_trans_pos*)(erecovery_trans_buf+ERECOVERY_TOTAL_BUF_SIZE-sizeof(erecovery_trans_pos));
    ERECOVERY_INFO("pos: %p\n", erecovery_pos);
    init_waitqueue_head(&erecovery_wq);
    return 0;

_error:
    if (erecovery_trans_buf) {
        vfree(erecovery_trans_buf);
    }

    return ret;
}

static void __exit erecovery_trans_exit(void)
{
    if (erecovery_trans_buf) {
        vfree(erecovery_trans_buf);
    }
}

module_init(erecovery_trans_init);
module_exit(erecovery_trans_exit);
MODULE_LICENSE("GPL");
