

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

#ifndef __CHR_DEVS_H__
#define __CHR_DEVS_H__

/*****************************************************************************
  1 头文件包含
*****************************************************************************/

#include <linux/debugfs.h>

/*****************************************************************************
  2 CHR性能配置
*****************************************************************************/

/*****************************************************************************
  3 宏定义
*****************************************************************************/

#define CHR_DEV_KMSG_PLAT           "chrKmsgPlat"

#define CHR_LOG_ENABLE              (1)
#define CHR_LOG_DISABLE             (0)
#define CHR_ERRNO_QUEUE_MAX_LEN     (20)
#define CHR_DEV_FRAME_START         (0x7E)
#define CHR_DEV_FRAME_END           (0x7E)
#define CHR_READ_SEMA               (1)

#ifdef CHR_DEBUG
#define CHR_DBG(s, args...)          printk(KERN_DEBUG KBUILD_MODNAME ":D]chr %s]" s,__func__, ## args)
#else
#define CHR_DBG(s, args...)
#endif
#define CHR_ERR(s, args...)          printk(KERN_ERR KBUILD_MODNAME ":E]chr %s]" s,__func__, ## args)
#define CHR_WARNING(s, args...)      printk(KERN_WARNING KBUILD_MODNAME ":W]chr %s]" s,__func__, ## args)
#define CHR_INFO(s, args...)         printk(KERN_DEBUG KBUILD_MODNAME ":I]chr %s]" s,__func__, ## args)

#define CHR_MAGIC                   'C'
#define CHR_MAX_NR                  1
#define chr_ERRNO_WRITE_NR          1
#define CHR_ERRNO_WRITE             _IOW(CHR_MAGIC, 1, int32)
/*****************************************************************************
  3 数据类型定义
*****************************************************************************/
typedef unsigned char               uint8;
typedef char                        int8;
typedef unsigned short              uint16;
typedef short                       int16;
typedef unsigned int                uint32;
typedef int                         int32;
typedef unsigned long               uint64;
typedef long                        int64;

/*****************************************************************************
  4 枚举类型定义
*****************************************************************************/
typedef enum chr_dev_index{
    CHR_INDEX_KMSG_PLAT = 0,
    CHR_INDEX_KMSG_WIFI,
    CHR_INDEX_APP_WIFI,
    CHR_INDEX_APP_GNSS,
    CHR_INDEX_APP_BT,
#ifdef CONFIG_CHR_OTHER_DEVS
    CHR_INDEX_APP_FM,
    CHR_INDEX_APP_NFC,
    CHR_INDEX_APP_IR,
#endif
    CHR_INDEX_MUTT,
}CHR_DEV_INDEX;

typedef enum chr_LogPriority{
    CHR_LOG_DEBUG = 0,
    CHR_LOG_INFO,
    CHR_LOG_WARN,
    CHR_LOG_ERROR,
    CHR_LOG_MUTT,
}CHR_LOGPRIORITY;

typedef enum chr_LogTag{
    CHR_LOG_TAG_PLAT = 0,
    CHR_LOG_TAG_WIFI,
    CHR_LOG_TAG_GNSS,
    CHR_LOG_TAG_BT,
#ifdef CONFIG_CHR_OTHER_DEVS
    CHR_LOG_TAG_FM,
    CHR_LOG_TAG_NFC,
    CHR_LOG_TAG_IR,
#endif
    CHR_LOG_TAG_MUTT,
}CHR_LOG_TAG;

enum return_type
{
    CHR_SUCC = 0,
    CHR_EFAIL,
};

/*****************************************************************************
  5 结构体定义
*****************************************************************************/
typedef struct {
    wait_queue_head_t       errno_wait;
    struct sk_buff_head     errno_queue;
    struct semaphore        errno_sem;
}CHR_EVENT;

typedef struct
{
    uint8  framehead;
    uint8  reserved[3];
    uint32 errno;
    uint8  frametail;
}CHR_DEV_EXCEPTION_STRU;

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
