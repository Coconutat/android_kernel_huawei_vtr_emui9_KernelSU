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

#ifndef __NETMGRCTRLVCOM_H__
#define __NETMGRCTRLVCOM_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "v_typdef.h"
#include "PsTypeDef.h"
#include "product_config.h"
#include "TafTypeDef.h"

#if (VOS_OS_VER == VOS_LINUX)
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#else
#include "PsCommonDef.h"
#include "Linuxstub.h"
#endif

#include "hi_list.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define NM_CTRL_DEVICE_NAME             "nmctrlvcom"

#define NM_CTRL_PRINT_INFO(fmt, ...) \
    printk(KERN_ERR fmt, ##__VA_ARGS__)
/* printk(KERN_INFO fmt, ##__VA_ARGS__) */


#define NM_CTRL_PRINT_WARN(fmt, ...) \
    printk(KERN_ERR fmt, ##__VA_ARGS__)
/* printk(KERN_WARNING fmt, ##__VA_ARGS__) */

#define NM_CTRL_PRINT_ERR(fmt, ...) \
    printk(KERN_ERR fmt, ##__VA_ARGS__)

#define NM_CTRL_GET_MAJOR_NUM()         (g_stNmCtrlCtx.ulMajorNum)
#define NM_CTRL_SET_MAJOR_NUM(major)    (g_stNmCtrlCtx.ulMajorNum = (major))

#define NM_CTRL_GET_DATA_FLG()          (g_stNmCtrlCtx.ulDataFlg)
#define NM_CTRL_SET_DATA_FLG(flg)       (g_stNmCtrlCtx.ulDataFlg = (flg))


#if (VOS_OS_VER == VOS_WIN32 || defined(_lint))
typedef struct msp_list_header  LIST_HEAD_STRU;
#else
typedef struct list_head        LIST_HEAD_STRU;
#endif

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef struct
 {
    LIST_HEAD_STRU                      stMsgList;
    unsigned int                        ulLen;
    unsigned int                        ulReserv;
    unsigned char                       aucData[];
}NM_CTRL_CDEV_DATA_STRU;

typedef struct
{
    struct cdev                        *pstNmCtrlDev;            /* cdev结构体，与字符设备对应 */
    wait_queue_head_t                   stReadInq;
    spinlock_t                          stListLock;             /* 新增一个成员，定义一个spin锁,访问链表时需要加锁 */
    LIST_HEAD_STRU                      stListHead;
    unsigned int                        ulMajorNum;
    unsigned int                        ulDataFlg;
}NM_CTRL_CTX_STRU;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

extern int __init NM_CTRL_Init(VOS_VOID);
extern int NM_CTRL_Open(struct inode *node, struct file *filp);
extern unsigned int NM_CTRL_Poll(struct file* filp, poll_table *wait);
extern ssize_t NM_CTRL_Read(struct file *filp, char __user *buf, size_t size, loff_t *ppos);
extern int NM_CTRL_Release(struct inode *node, struct file *filp);
extern void NM_CTRL_SendMsg(void* pDataBuffer, unsigned int len);
extern void NM_CTRL_Setup(struct cdev * dev);



#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of NetMgrCtrlVcom.h */
