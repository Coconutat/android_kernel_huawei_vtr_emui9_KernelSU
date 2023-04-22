/*
* Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
* foss@huawei.com
*
* If distributed as part of the Linux kernel, the following license terms
* apply:
*
* * This program is free software; you can redistribute it and/or modify
* * it under the terms of the GNU General Public License version 2 and
* * only version 2 as published by the Free Software Foundation.
* *
* * This program is distributed in the hope that it will be useful,
* * but WITHOUT ANY WARRANTY; without even the implied warranty of
* * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* * GNU General Public License for more details.
* *
* * You should have received a copy of the GNU General Public License
* * along with this program; if not, write to the Free Software
* * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*
* Otherwise, the following license terms apply:
*
* * Redistribution and use in source and binary forms, with or without
* * modification, are permitted provided that the following conditions
* * are met:
* * 1) Redistributions of source code must retain the above copyright
* *    notice, this list of conditions and the following disclaimer.
* * 2) Redistributions in binary form must reproduce the above copyright
* *    notice, this list of conditions and the following disclaimer in the
* *    documentation and/or other materials provided with the distribution.
* * 3) Neither the name of Huawei nor the names of its contributors may
* *    be used to endorse or promote products derived from this software
* *    without specific prior written permission.
*
* * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "NetMgrCtrlVcom.h"
#include "NetMgrCtrlInterface.h"


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_NET_MGR_CTRL_VCOM_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
NM_CTRL_CTX_STRU    g_stNmCtrlCtx;                          /*设备结构体*/

static const struct file_operations g_stNmCtrlCdevFops =
{
    .owner   = THIS_MODULE,
    .read    = NM_CTRL_Read,
    .open    = NM_CTRL_Open,
    .release = NM_CTRL_Release,
    .poll    = NM_CTRL_Poll,
};

/*****************************************************************************
  3 函数实现
*****************************************************************************/

void NM_CTRL_SendMsg(void* pDataBuffer, unsigned int len)
{
    /*lint --e{429}*/
    /* 屏蔽error 429(警告pstListEntry内存没有释放，此处pstListEntry内存在read函数中释放，所以该告警屏蔽) */
    NM_CTRL_CDEV_DATA_STRU             *pstListEntry    = VOS_NULL_PTR;
    unsigned long                       flags           = 0;

    NM_CTRL_PRINT_INFO("Enter NM_CTRL_SendMsg len(%d).\n", len);

    if (len > NM_MSG_BUFFER_SIZE)
    {
       NM_CTRL_PRINT_ERR("NM_CTRL_SendMsg the length(%d) received exceed the MAX NM_MSG_BUFFER_SIZE.\n", len);
       return;
    }

    /* 分配链表内存kmalloc，用于存储数据 */
    pstListEntry = (NM_CTRL_CDEV_DATA_STRU *)kmalloc(sizeof(NM_CTRL_CDEV_DATA_STRU) + len, GFP_KERNEL);
    if (VOS_NULL_PTR == pstListEntry)
    {
        NM_CTRL_PRINT_ERR("NM_CTRL_SendMsg: kmalloc data failed.\n");
        return;
    }

    memset(pstListEntry, 0, sizeof(NM_CTRL_CDEV_DATA_STRU) + len);

    pstListEntry->ulLen = len;
    memcpy(pstListEntry->aucData, pDataBuffer, len);

    NM_CTRL_PRINT_INFO("NM_CTRL_SendMsg: list addr %pK data addr %pK", pstListEntry, pstListEntry->aucData);

    /* 获取信号量 */
    spin_lock_irqsave(&(g_stNmCtrlCtx.stListLock), flags);

    /* 挂接到链表末尾 */
    list_add_tail(&(pstListEntry->stMsgList), &(g_stNmCtrlCtx.stListHead));

    g_stNmCtrlCtx.ulDataFlg = true;

    /* 释放信号量 */
    spin_unlock_irqrestore(&(g_stNmCtrlCtx.stListLock), flags);

    wake_up_interruptible(&(g_stNmCtrlCtx.stReadInq));

    return;
}


int NM_CTRL_Open(struct inode *node, struct file *filp)
{
    int                                 ret = 0;

    filp->private_data = g_stNmCtrlCtx.pstNmCtrlDev;

    NM_CTRL_PRINT_INFO("Enter NM_CTRL_open.\n");

    return ret;
}


int NM_CTRL_Release(struct inode *node, struct file *filp)
{
    LIST_HEAD_STRU                     *pstNextPtr  = VOS_NULL_PTR;
    LIST_HEAD_STRU                     *pstCurPtr   = VOS_NULL_PTR;
    NM_CTRL_CDEV_DATA_STRU             *pstCurEntry = VOS_NULL_PTR;
    unsigned long                       flags       = 0;
    int                                 ret         = 0;

    /* 获取信号量 */
    spin_lock_irqsave(&(g_stNmCtrlCtx.stListLock), flags);

    list_for_each_safe(pstCurPtr, pstNextPtr, &(g_stNmCtrlCtx.stListHead))
    {
        pstCurEntry = list_entry(pstCurPtr, NM_CTRL_CDEV_DATA_STRU, stMsgList);
        list_del(&(pstCurEntry->stMsgList));
        kfree(pstCurEntry);
    }

    g_stNmCtrlCtx.ulDataFlg = false;

    /* 释放信号量 */
    spin_unlock_irqrestore(&(g_stNmCtrlCtx.stListLock), flags);

    NM_CTRL_PRINT_INFO("Enter NM_CTRL_release.\n");

    return ret;
}


ssize_t NM_CTRL_Read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    NM_CTRL_CDEV_DATA_STRU             *pstCurEntry = VOS_NULL_PTR;
    unsigned long                       flags       = 0;
    int                                 ret         = 0;

    NM_CTRL_PRINT_INFO("Enter NM_CTRL_read size_t(%lu).\n", (unsigned long)size);

    if (filp->f_flags & O_NONBLOCK)
    {
        NM_CTRL_PRINT_INFO("NM_CTRL_read: NONBLOCK\n");
        return -EAGAIN;
    }

    if (wait_event_interruptible(g_stNmCtrlCtx.stReadInq, g_stNmCtrlCtx.ulDataFlg))
    {
        NM_CTRL_PRINT_ERR("NM_CTRL_read wait_event_interruptible error\n");
        return -ERESTARTSYS;
    }

    /* 获取信号量 */
    spin_lock_irqsave(&(g_stNmCtrlCtx.stListLock), flags);

    /* 读取数据链表 */
    if (!list_empty(&(g_stNmCtrlCtx.stListHead)))
    {
        pstCurEntry = list_first_entry(&(g_stNmCtrlCtx.stListHead), NM_CTRL_CDEV_DATA_STRU, stMsgList);

        if (size < pstCurEntry->ulLen)
        {
            NM_CTRL_PRINT_ERR("NM_CTRL_read size is less than the send len\n");
        }

        if (size > pstCurEntry->ulLen)
        {
            NM_CTRL_PRINT_WARN("NM_CTRL_read size is more than the send len\n");
            size = pstCurEntry->ulLen;
        }

        NM_CTRL_PRINT_INFO("NM_CTRL_Read: list addr %pK data addr %pK", pstCurEntry, pstCurEntry->aucData);

        /* read data to user space */
        if (copy_to_user(buf, (void*)(pstCurEntry->aucData), (unsigned long)size))
        {
            ret = -EFAULT;
            NM_CTRL_PRINT_ERR("NM_CTRL_read copy_to_user err\n");
        }
        else
        {
           ret = (int)pstCurEntry->ulLen;
           list_del((LIST_HEAD_STRU *)pstCurEntry);
           kfree(pstCurEntry);
           NM_CTRL_PRINT_INFO("NM_CTRL_read has read %d bytes.\n", ret);
        }
    }

    /* 判断链表是否为空 list_empty(g_stNmCtrlCdevp->data)；如果是空，false；如果非空，true; */
    if (list_empty(&(g_stNmCtrlCtx.stListHead)))
        g_stNmCtrlCtx.ulDataFlg = false;
    else
        g_stNmCtrlCtx.ulDataFlg = true;

    /* 释放信号量 */
    spin_unlock_irqrestore(&(g_stNmCtrlCtx.stListLock), flags);

    return ret;
}


unsigned int NM_CTRL_Poll(struct file* filp, poll_table *wait)
{
    unsigned int                        mask = 0;

    NM_CTRL_PRINT_INFO("Enter NM_CTRL_poll.\n");

    /*put the queue into poll_table */
    poll_wait(filp, &(g_stNmCtrlCtx.stReadInq), wait);

    if (true == g_stNmCtrlCtx.ulDataFlg)
    {
       mask |= POLLIN | POLLRDNORM;
       NM_CTRL_PRINT_INFO("NM_CTRL_Poll notify read  process.\n");
    }

    return mask;
}


void NM_CTRL_Setup(struct cdev * dev)
{
    int                                 err = 0;
    dev_t                               devno;

    NM_CTRL_PRINT_INFO("Enter NM_CTRL_Setup.");

    devno = MKDEV(g_stNmCtrlCtx.ulMajorNum, 0);

    /* Init the device */
    cdev_init( dev, &g_stNmCtrlCdevFops );
    dev->owner = THIS_MODULE;
    dev->ops = &g_stNmCtrlCdevFops;

    /* add the device into devices list */
    err = cdev_add( dev, devno, 1 );

    if (err)
    {
        NM_CTRL_PRINT_ERR("NM_CTRL_Setup Error %d adding ACT Device Failed", err);
        return;
    }
}


int __init NM_CTRL_Init(VOS_VOID)
{
    int                                 ret = 0;
    dev_t                               devno;
    static struct class                *pstNmCtrlClass = VOS_NULL_PTR;

    NM_CTRL_PRINT_INFO("Enter NM_CTRL_Init.");

    memset(&g_stNmCtrlCtx, 0, sizeof(NM_CTRL_CTX_STRU));

    ret = alloc_chrdev_region(&devno, 0, 1, NM_CTRL_DEVICE_NAME);

    NM_CTRL_PRINT_INFO("NM_CTRL_Init alloc_chrdev_region ret (%d) devno (%d).", ret, devno);

    if (ret < 0)
    {
        return ret;
    }

    g_stNmCtrlCtx.ulMajorNum = MAJOR(devno);

    /* alloc the reource */
    g_stNmCtrlCtx.pstNmCtrlDev = (struct cdev *)kmalloc(sizeof(struct cdev), GFP_KERNEL);

    if (!g_stNmCtrlCtx.pstNmCtrlDev)
    {
        ret = -ENOMEM;
        NM_CTRL_PRINT_ERR("NM_CTRL_Init: kmalloc failed.\n");
        unregister_chrdev_region(devno, 1);
        return ret;
    }

    NM_CTRL_Setup(g_stNmCtrlCtx.pstNmCtrlDev);

    /* create the device node */
    pstNmCtrlClass = class_create(THIS_MODULE, NM_CTRL_DEVICE_NAME);

    if (IS_ERR(pstNmCtrlClass))
    {
       NM_CTRL_PRINT_ERR("NM_CTRL_Init: failed to create class.\n");
       ret = -EFAULT;
    }
    else
    {
       device_create(pstNmCtrlClass, NULL, MKDEV(g_stNmCtrlCtx.ulMajorNum, 0), NULL, NM_CTRL_DEVICE_NAME);
    }

    /* 初始化 */
    INIT_LIST_HEAD(&(g_stNmCtrlCtx.stListHead));

    spin_lock_init(&(g_stNmCtrlCtx.stListLock));

    /* init wait queue */
    init_waitqueue_head(&(g_stNmCtrlCtx.stReadInq));

    NM_CTRL_PRINT_INFO("Exit NM_CTRL_Init.");

    return ret;
}


module_init(NM_CTRL_Init);


