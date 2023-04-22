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


#ifndef __APPVCOMDEV_H__
#define __APPVCOMDEV_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "v_typdef.h"
#include "PsTypeDef.h"
#include "AtAppVcomInterface.h"
#include "product_config.h"

#if (VOS_OS_VER == VOS_LINUX)
#include "linux/module.h"
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <linux/wakelock.h>
#else
#include "Linuxstub.h"
#endif


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define USE_SEM                         (0)
#define APP_VCOM_MEM_CLEAR              (0x1)                                   /* 清零全局内存 */

#define APP_VCOM_MAX_NUM                 (APP_VCOM_DEV_INDEX_BUTT)                /*VCOM口数量*/
#define APP_VCOM_ERROR                   (-1)

#define APP_VCOM_BUILD_DEV_ID(dev, type) (((unsigned int)(dev) << 8) | ((unsigned int)(type) & 0x00ff))
#define APP_VCOM_BUILD_CMD_ID(dev, cmd)  (((unsigned int)(dev) << 8) | ((unsigned int)(cmd) & 0xffff))
#define APP_VCOM_SET_BIT(n)              (1U << n)

#define APP_VCOM_RD_WAKE_LOCK_NAME_LEN  (32)

/* 虚拟串口设备名称 */
#define APP_VCOM_DEV_NAME_0              "appvcom"

/*新增虚拟串口设备名称*/
#define APP_VCOM_DEV_NAME_1              "appvcom1"
#if (FEATURE_ON == FEATURE_VCOM_EXT)
#define APP_VCOM_DEV_NAME_2              "appvcom2"
#define APP_VCOM_DEV_NAME_3              "appvcom3"
#define APP_VCOM_DEV_NAME_4              "appvcom4"
#define APP_VCOM_DEV_NAME_5              "appvcom5"
#define APP_VCOM_DEV_NAME_6              "appvcom6"
#define APP_VCOM_DEV_NAME_7              "appvcom7"
#define APP_VCOM_DEV_NAME_8              "appvcom8"
#define APP_VCOM_DEV_NAME_9              "appvcom9"
#define APP_VCOM_DEV_NAME_10             "appvcom10"
#define APP_VCOM_DEV_NAME_11             "appvcom11"
#define APP_VCOM_DEV_NAME_12             "appvcom12"
#define APP_VCOM_DEV_NAME_13             "appvcom13"
#define APP_VCOM_DEV_NAME_14             "appvcom14"
#define APP_VCOM_DEV_NAME_15             "appvcom15"
#define APP_VCOM_DEV_NAME_16             "appvcom16"
#define APP_VCOM_DEV_NAME_17             "appvcom17"
#define APP_VCOM_DEV_NAME_18             "appvcom18"
#define APP_VCOM_DEV_NAME_19             "appvcom19"
#define APP_VCOM_DEV_NAME_20             "appvcom20"
#define APP_VCOM_DEV_NAME_21             "appvcom21"
#define APP_VCOM_DEV_NAME_22             "appvcom22"
#define APP_VCOM_DEV_NAME_23             "appvcom23"
#define APP_VCOM_DEV_NAME_24             "appvcom24"
#define APP_VCOM_DEV_NAME_25             "appvcom25"
#define APP_VCOM_DEV_NAME_26             "appvcom26"


#define APP_VCOM_DEV_NAME_27             "appvcom27"
#define APP_VCOM_DEV_NAME_28             "appvcom28"
#define APP_VCOM_DEV_NAME_29             "appvcom29"
#define APP_VCOM_DEV_NAME_30             "appvcom30"
#define APP_VCOM_DEV_NAME_31             "appvcom31"


#define APP_VCOM_DEV_NAME_32             "appvcom32"
#define APP_VCOM_DEV_NAME_33             "appvcom33"
#define APP_VCOM_DEV_NAME_34             "appvcom34"
#define APP_VCOM_DEV_NAME_35             "appvcom35"
#define APP_VCOM_DEV_NAME_36             "appvcom36"
#define APP_VCOM_DEV_NAME_37             "appvcom37"
#define APP_VCOM_DEV_NAME_38             "appvcom38"
#define APP_VCOM_DEV_NAME_39             "appvcom39"
#define APP_VCOM_DEV_NAME_40             "appvcom40"
#define APP_VCOM_DEV_NAME_41             "appvcom41"
#define APP_VCOM_DEV_NAME_42             "appvcom42"
#define APP_VCOM_DEV_NAME_43             "appvcom43"
#define APP_VCOM_DEV_NAME_44             "appvcom44"
#define APP_VCOM_DEV_NAME_45             "appvcom45"
#define APP_VCOM_DEV_NAME_46             "appvcom46"
#define APP_VCOM_DEV_NAME_47             "appvcom47"
#define APP_VCOM_DEV_NAME_48             "appvcom48"
#define APP_VCOM_DEV_NAME_49             "appvcom49"
#define APP_VCOM_DEV_NAME_50             "appvcom50"
#define APP_VCOM_DEV_NAME_51             "appvcom51"
#define APP_VCOM_DEV_NAME_52             "appvcom52"
#define APP_VCOM_DEV_NAME_53             "appvcomERRLOG"
#define APP_VCOM_DEV_NAME_54             "appvcomTLLOG"
#define APP_VCOM_DEV_NAME_55             "appvcomCBT"
#define APP_VCOM_DEV_NAME_56             "appvcomLOG"
#define APP_VCOM_DEV_NAME_57             "appvcomLOG1"
#define APP_VCOM_DEV_NAME_58             "appvcom58"
#define APP_VCOM_DEV_NAME_59             "appvcom59"
#define APP_VCOM_DEV_NAME_60             "appvcom60"
#define APP_VCOM_DEV_NAME_61             "appvcom61"
#define APP_VCOM_DEV_NAME_62             "appvcom62"
#define APP_VCOM_DEV_NAME_63             "appvcom63"


#endif

/*设备信号量名称*/
#define APP_VCOM_SEM_NAME_0               "SEM"
#define APP_VCOM_SEM_NAME_1               "SEM1"
#if (FEATURE_ON == FEATURE_VCOM_EXT)
#define APP_VCOM_SEM_NAME_2               "SEM2"
#define APP_VCOM_SEM_NAME_3               "SEM3"
#define APP_VCOM_SEM_NAME_4               "SEM4"
#define APP_VCOM_SEM_NAME_5               "SEM5"
#define APP_VCOM_SEM_NAME_6               "SEM6"
#define APP_VCOM_SEM_NAME_7               "SEM7"
#define APP_VCOM_SEM_NAME_8               "SEM8"
#define APP_VCOM_SEM_NAME_9               "SEM9"
#define APP_VCOM_SEM_NAME_10              "SEM10"
#define APP_VCOM_SEM_NAME_11              "SEM11"
#define APP_VCOM_SEM_NAME_12              "SEM12"
#define APP_VCOM_SEM_NAME_13              "SEM13"
#define APP_VCOM_SEM_NAME_14              "SEM14"
#define APP_VCOM_SEM_NAME_15              "SEM15"
#define APP_VCOM_SEM_NAME_16              "SEM16"
#define APP_VCOM_SEM_NAME_17               "SEM17"
#define APP_VCOM_SEM_NAME_18               "SEM18"
#define APP_VCOM_SEM_NAME_19               "SEM19"
#define APP_VCOM_SEM_NAME_20               "SEM20"
#define APP_VCOM_SEM_NAME_21               "SEM21"
#define APP_VCOM_SEM_NAME_22               "SEM22"
#define APP_VCOM_SEM_NAME_23               "SEM23"
#define APP_VCOM_SEM_NAME_24               "SEM24"
#define APP_VCOM_SEM_NAME_25               "SEM25"
#define APP_VCOM_SEM_NAME_26               "SEM26"
#define APP_VCOM_SEM_NAME_27               "SEM27"
#define APP_VCOM_SEM_NAME_28               "SEM28"
#define APP_VCOM_SEM_NAME_29               "SEM29"
#define APP_VCOM_SEM_NAME_30               "SEM30"
#define APP_VCOM_SEM_NAME_31               "SEM31"

#define APP_VCOM_SEM_NAME_32               "SEM32"
#define APP_VCOM_SEM_NAME_33               "SEM33"
#define APP_VCOM_SEM_NAME_34               "SEM34"
#define APP_VCOM_SEM_NAME_35               "SEM35"
#define APP_VCOM_SEM_NAME_36               "SEM36"
#define APP_VCOM_SEM_NAME_37               "SEM37"
#define APP_VCOM_SEM_NAME_38               "SEM38"
#define APP_VCOM_SEM_NAME_39               "SEM39"
#define APP_VCOM_SEM_NAME_40               "SEM40"
#define APP_VCOM_SEM_NAME_41               "SEM41"
#define APP_VCOM_SEM_NAME_42               "SEM42"
#define APP_VCOM_SEM_NAME_43               "SEM43"
#define APP_VCOM_SEM_NAME_44               "SEM44"
#define APP_VCOM_SEM_NAME_45               "SEM45"
#define APP_VCOM_SEM_NAME_46               "SEM46"
#define APP_VCOM_SEM_NAME_47               "SEM47"
#define APP_VCOM_SEM_NAME_48               "SEM48"
#define APP_VCOM_SEM_NAME_49               "SEM49"
#define APP_VCOM_SEM_NAME_50               "SEM50"
#define APP_VCOM_SEM_NAME_51               "SEM51"
#define APP_VCOM_SEM_NAME_52               "SEM52"
#define APP_VCOM_SEM_NAME_53               "SEM53"
#define APP_VCOM_SEM_NAME_54               "SEM54"
#define APP_VCOM_SEM_NAME_55               "SEM55"
#define APP_VCOM_SEM_NAME_56               "SEM56"
#define APP_VCOM_SEM_NAME_57               "SEM57"
#define APP_VCOM_SEM_NAME_58               "SEM58"
#define APP_VCOM_SEM_NAME_59               "SEM59"
#define APP_VCOM_SEM_NAME_60               "SEM60"
#define APP_VCOM_SEM_NAME_61               "SEM61"
#define APP_VCOM_SEM_NAME_62               "SEM62"
#define APP_VCOM_SEM_NAME_63               "SEM63"
#endif


/* E5口输入的AT命令的一般长度 */
#define APP_VCOM_NORMAL_CMD_LEN         (100)

/* 虚拟串口设备名称长度*/
#define APP_VCOM_DEV_NAME_MAX_LEN       (16)
#define APP_VCOM_SEM_NAME_MAX_LEN       (8)

#define APP_VCOM_READ_WAKE_LOCK_LEN     (100)

#define APP_VCOM_TRACE_BUF_LEN 	            (256)

#define APP_VCOM_TRACE_LEVEL_INFO           (0x00000001)
#define APP_VCOM_TRACE_LEVEL_NORM           (0x00000002)
#define APP_VCOM_TRACE_LEVEL_ERR            (0x00000004)
#define APP_VCOM_TRACE_LEVEL_DEBUG          (APP_VCOM_TRACE_LEVEL_INFO | APP_VCOM_TRACE_LEVEL_NORM | APP_VCOM_TRACE_LEVEL_ERR)

#define APP_VCOM_LOG_FORMAT(ulPrintLength, pcBuf, ulBufSize, pcFmt)\
            {\
                va_list pArgList;\
                va_start(pArgList, pcFmt);\
                ulPrintLength += vsnprintf(pcBuf + ulPrintLength,\
                                    ulBufSize - ulPrintLength, pcFmt, pArgList);\
                va_end(pArgList);\
                if (ulPrintLength > (ulBufSize - 1))\
                {\
                    ulPrintLength = ulBufSize - 1;\
                }\
                *(pcBuf + ulPrintLength) = '\0';\
            }

#if (VOS_OS_VER == VOS_WIN32) || defined(_lint)
#define APP_VCOM_DBG_PRINT(lvl, index, fmt, ...)\
            (VOS_VOID)vos_printf(fmt, index, ##__VA_ARGS__)
#else
#define APP_VCOM_DBG_PRINT(lvl, index, fmt, ...)\
            do\
            {\
                if ( index < 32 )\
                {\
                    if ((lvl == (g_stAppVcomDebugCfg.ulDebugLevel & lvl))\
                       && (0 != (g_stAppVcomDebugCfg.ulPortIdMask1 & APP_VCOM_SET_BIT(index))))\
                    {\
                        APP_VCOM_MNTN_LogPrintf("[VCOM:%2d][TICK:%u][%s][LINE:%d] "fmt"\n", index, VOS_GetTick(), __FUNCTION__, __LINE__, ##__VA_ARGS__);\
                    }\
                }\
                else\
                {\
                    if ((lvl == (g_stAppVcomDebugCfg.ulDebugLevel & lvl))\
                       && (0 != (g_stAppVcomDebugCfg.ulPortIdMask2 & APP_VCOM_SET_BIT((index - 32)))))\
                    {\
                        APP_VCOM_MNTN_LogPrintf("[VCOM:%2d][TICK:%u][%s][LINE:%d] "fmt"\n", index, VOS_GetTick(), __FUNCTION__, __LINE__, ##__VA_ARGS__);\
                    }\
                }\
            }while(0)
#endif

#define APP_VCOM_TRACE_INFO(index, ...)\
            APP_VCOM_DBG_PRINT(APP_VCOM_TRACE_LEVEL_INFO, index, __VA_ARGS__)

#define APP_VCOM_TRACE_NORM(index, ...)\
            APP_VCOM_DBG_PRINT(APP_VCOM_TRACE_LEVEL_NORM, index, __VA_ARGS__)

#define APP_VCOM_TRACE_ERR(index, ...)\
            APP_VCOM_DBG_PRINT(APP_VCOM_TRACE_LEVEL_ERR, index, __VA_ARGS__)

#define APP_VCOM_ERR_LOG(Index, String)\
            AT_ERR_LOG1(String, Index)

#define APP_VCOM_MAX_DATA_LENGTH        (64 * 1024)


#define APPVCOM_DYNAMIC_MALLOC_MEMORY(ucIndex)\
            ( (ucIndex == APP_VCOM_DEV_INDEX_CBT)\
           || (ucIndex == APP_VCOM_DEV_INDEX_LOG)\
           || (ucIndex == APP_VCOM_DEV_INDEX_LOG1) )

#define APPVCOM_STATIC_MALLOC_MEMORY(ucIndex)\
            ( (ucIndex != APP_VCOM_DEV_INDEX_CBT)\
           && (ucIndex != APP_VCOM_DEV_INDEX_LOG)\
           && (ucIndex != APP_VCOM_DEV_INDEX_LOG1) )
/*****************************************************************************
  3 枚举定义
*****************************************************************************/

enum APP_VCOM_RESULT_ENUM
{
    APP_VCOM_OK                         =  0,                                             /*NORMAL COMPLETION*/
    APP_VCOM_NOTSTARTED                 =  1,                                             /*OPERATION NOT STARTED*/
    APP_VCOM_INPROGRESS                 =  2,                                             /*OPERATION IN PROGRESS*/
    APP_VCOM_PERM                       =  3,                                             /*OPERATION NOT PERMITTED*/
    APP_VCOM_NOENT                      =  4,                                             /*NO SUCH ENTRY*/
    APP_VCOM_IO                         =  5,                                             /*INPUT/OUTPUT ERROR*/
    APP_VCOM_NXIO                       =  6,                                             /*DEVICE NOT CONFIGURED*/
    APP_VCOM_NOMEM                      =  7,                                             /*FAILED ALLOCATING MEMORY*/
    APP_VCOM_BUSY                       =  8,                                             /*RESOURCE IS BUSY*/
    APP_VCOM_NODEV                      =  9,                                             /*NO SUCH DEVICE*/
    APP_VCOM_INVAL                      = 10,                                             /*INVALID ARGUMENT*/
    APP_VCOM_NOTSUP                     = 11,                                             /*OPERATION NOT SUPPORTED*/
    APP_VCOM_TIMEDOUT                   = 12,                                             /*OPERATION TIMED OUT*/
    APP_VCOM_SUSPENDED                  = 13,                                             /*DEVICE IS SUSPENDED*/
    APP_VCOM_UNKNOWN                    = 14,                                             /*GENERAL-PURPOSE ERROR*/
    APP_VCOM_TEST_FAILED                = 15,                                             /*LOGICAL TEST FAILURE*/
    APP_VCOM_STATE                      = 16,                                             /*INCORRECT STATE*/
    APP_VCOM_STALLED                    = 17,                                             /*PIPE IS STALLED*/
    APP_VCOM_PARAM                      = 18,                                             /*INVALID PARAMETER*/
    APP_VCOM_ABORTED                    = 19,                                             /*OPERATION ABORTED*/
    APP_VCOM_SHORT                      = 20,                                             /*SHORT TRANSFER*/
    APP_VCOM_EXPIRED                    = 21,                                             /*EVALUATION TIME EXPIRED*/
    APP_VCOM_BUTT
};
typedef unsigned int APP_VCOM_RESULT_ENUM_UINT32;


enum APP_VCOM_ERR_ENUM
{
    APP_VCOM_EPERM                      =  0,                                             /* Operation not permitted */
    APP_VCOM_ENOENT                     =  1,                                             /* No such file or directory */
    APP_VCOM_ESRCH                      =  2,                                             /* No such process */
    APP_VCOM_EINTR                      =  3,                                             /* Interrupted system call */
    APP_VCOM_EIO                        =  4,                                             /* I/O error */
    APP_VCOM_ENXIO                      =  5,                                             /* No such device or address */
    APP_VCOM_E2BIG                      =  6,                                             /* Argument list too long */
    APP_VCOM_ENOEXEC                    =  7,                                             /* Exec format error */
    APP_VCOM_EBADF                      =  8,                                             /* Bad file number */
    APP_VCOM_ECHILD                     =  9,                                             /* No child processes */
    APP_VCOM_EAGAIN                     = 10,                                             /* Try again */
    APP_VCOM_ENOMEM                     = 11,                                             /* Out of memory */
    APP_VCOM_EACCES                     = 12,                                             /* Permission denied */
    APP_VCOM_EFAULT                     = 13,                                             /* Bad address */
    APP_VCOM_ENOTBLK                    = 14,                                             /* Block device required */
    APP_VCOM_EBUSY                      = 15,                                             /* Device or resource busy */
    APP_VCOM_EEXIST                     = 16,                                             /* File exists */
    APP_VCOM_EXDEV                      = 17,                                             /* Cross-device link */
    APP_VCOM_ENODEV                     = 18,                                             /* No such device */
    APP_VCOM_ENOTDIR                    = 19,                                             /* Not a directory */
    APP_VCOM_EISDIR                     = 20,                                             /* Is a directory */
    APP_VCOM_EINVAL                     = 21,                                             /* Invalid argument */
    APP_VCOM_ENFILE                     = 23,                                             /* File table overflow */
    APP_VCOM_EMFILE                     = 24,                                             /* Too many open files */
    APP_VCOM_ENOTTY                     = 25,                                             /* Not a typewriter */
    APP_VCOM_ETXTBSY                    = 26,                                             /* Text file busy */
    APP_VCOM_EFBIG                      = 27,                                             /* File too large */
    APP_VCOM_ENOSPC                     = 28,                                             /* No space left on device */
    APP_VCOM_ESPIPE                     = 29,                                             /* Illegal seek */
    APP_VCOM_EROFS                      = 30,                                             /* Read-only file system */
    APP_VCOM_EMLINK                     = 31,                                             /* Too many links */
    APP_VCOM_EPIPE                      = 32,                                             /* Broken pipe */
    APP_VCOM_EDOM                       = 33,                                             /* Math argument out of domain of func */
    APP_VCOM_ERANGE                     = 34,                                             /* Math result not representable */
    APP_VCOM_ERR_BUTT
};
typedef unsigned int APP_VCOM_ERR_ENUM_UINT32;


enum APP_VCOM_DEV_MAJORDEVID
{
    APP_VCOM_MAJOR_DEV_ID                  = 247,
    APP_VCOM_MAJOR_DEV_ID_1                = 248,
#if (FEATURE_ON == FEATURE_VCOM_EXT)
    APP_VCOM_MAJOR_DEV_ID_2                = 249,
    APP_VCOM_MAJOR_DEV_ID_3                = 250,
    APP_VCOM_MAJOR_DEV_ID_4                = 251,
    APP_VCOM_MAJOR_DEV_ID_5                = 252,
    APP_VCOM_MAJOR_DEV_ID_6                = 253,
    APP_VCOM_MAJOR_DEV_ID_7                = 254,
    APP_VCOM_MAJOR_DEV_ID_8                = 255,
    APP_VCOM_MAJOR_DEV_ID_9                = 256,
    APP_VCOM_MAJOR_DEV_ID_10               = 257,
    APP_VCOM_MAJOR_DEV_ID_11               = 258,
    APP_VCOM_MAJOR_DEV_ID_12               = 259,
    APP_VCOM_MAJOR_DEV_ID_13               = 260,
    APP_VCOM_MAJOR_DEV_ID_14               = 261,
    APP_VCOM_MAJOR_DEV_ID_15               = 262,
    APP_VCOM_MAJOR_DEV_ID_16               = 263,
    APP_VCOM_MAJOR_DEV_ID_17               = 264,
    APP_VCOM_MAJOR_DEV_ID_18               = 265,
    APP_VCOM_MAJOR_DEV_ID_19               = 266,
    APP_VCOM_MAJOR_DEV_ID_20               = 267,
    APP_VCOM_MAJOR_DEV_ID_21               = 268,
    APP_VCOM_MAJOR_DEV_ID_22               = 269,
    APP_VCOM_MAJOR_DEV_ID_23               = 270,
    APP_VCOM_MAJOR_DEV_ID_24               = 271,
    APP_VCOM_MAJOR_DEV_ID_25               = 272,
    APP_VCOM_MAJOR_DEV_ID_26               = 273,
    APP_VCOM_MAJOR_DEV_ID_27               = 274,
    APP_VCOM_MAJOR_DEV_ID_28               = 275,
    APP_VCOM_MAJOR_DEV_ID_29               = 276,
    APP_VCOM_MAJOR_DEV_ID_30               = 277,
    APP_VCOM_MAJOR_DEV_ID_31               = 278,

    APP_VCOM_MAJOR_DEV_ID_32               = 279,
    APP_VCOM_MAJOR_DEV_ID_33               = 280,
    APP_VCOM_MAJOR_DEV_ID_34               = 281,
    APP_VCOM_MAJOR_DEV_ID_35               = 282,
    APP_VCOM_MAJOR_DEV_ID_36               = 283,
    APP_VCOM_MAJOR_DEV_ID_37               = 284,
    APP_VCOM_MAJOR_DEV_ID_38               = 285,
    APP_VCOM_MAJOR_DEV_ID_39               = 286,
    APP_VCOM_MAJOR_DEV_ID_40               = 287,
    APP_VCOM_MAJOR_DEV_ID_41               = 288,
    APP_VCOM_MAJOR_DEV_ID_42               = 289,
    APP_VCOM_MAJOR_DEV_ID_43               = 290,
    APP_VCOM_MAJOR_DEV_ID_44               = 291,
    APP_VCOM_MAJOR_DEV_ID_45               = 292,
    APP_VCOM_MAJOR_DEV_ID_46               = 293,
    APP_VCOM_MAJOR_DEV_ID_47               = 294,
    APP_VCOM_MAJOR_DEV_ID_48               = 295,
    APP_VCOM_MAJOR_DEV_ID_49               = 296,
    APP_VCOM_MAJOR_DEV_ID_50               = 297,
    APP_VCOM_MAJOR_DEV_ID_51               = 298,
    APP_VCOM_MAJOR_DEV_ID_52               = 299,
    APP_VCOM_MAJOR_DEV_ID_53               = 300,
    APP_VCOM_MAJOR_DEV_ID_54               = 301,
    APP_VCOM_MAJOR_DEV_ID_55               = 302,
    APP_VCOM_MAJOR_DEV_ID_56               = 303,
    APP_VCOM_MAJOR_DEV_ID_57               = 304,
    APP_VCOM_MAJOR_DEV_ID_58               = 305,
    APP_VCOM_MAJOR_DEV_ID_59               = 306,
    APP_VCOM_MAJOR_DEV_ID_60               = 307,
    APP_VCOM_MAJOR_DEV_ID_61               = 308,
    APP_VCOM_MAJOR_DEV_ID_62               = 309,
    APP_VCOM_MAJOR_DEV_ID_63               = 310,
#endif
    APP_VCOM_MAJOR_DEV_ID_BUTT
};
typedef unsigned int APP_VCOM_DEV_MAJORDEVID_UINT32;

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
    struct cdev                         stAppVcomDev;                           /* 虚拟串口结构体 */
    VOS_UINT8                          *pucAppVcomMem;                          /* 设备内存 */
    wait_queue_head_t                   Read_Wait;                              /* 阻塞读用的等待队列头 */
    wait_queue_head_t                   Write_Wait;                             /* 阻塞写用的等待队列头 */
    struct semaphore                    stMsgSendSem;
    struct semaphore                    stWrtSem;
    VOS_UINT8                          *pucWrtBuffer;                           /* 写缓存 */
    VOS_UINT32                          ulWrtBufferLen;
    VOS_UINT8                           aucReserved[4];
    size_t                              current_len;                            /* fifo有效数据长度 */
    VOS_UINT32                          ulReadWakeUpFlg;
    VOS_UINT32                          ulIsDeviceOpen ;
    struct wake_lock                    stRdWakeLock;
    VOS_CHAR                            acWakeLockName[APP_VCOM_RD_WAKE_LOCK_NAME_LEN];
    struct mutex                        stMutex;                                /* 线程互斥锁 */
}APP_VCOM_DEV_ENTITY_STRU;


typedef struct
{
    APP_VCOM_DEV_ENTITY_STRU           *pstAppVcomDevEntity;                       /* 设备实体 */
    SEND_UL_AT_FUNC                     pSendUlAtFunc;                             /*对应的上行At码流发送函数*/
    VOS_UINT32                          ulAppVcomMajorId;                          /* 设备号 */
    VOS_CHAR                            aucAppVcomName[APP_VCOM_DEV_NAME_MAX_LEN]; /* 设备名称*/
    VOS_CHAR                            aucSendSemName[APP_VCOM_SEM_NAME_MAX_LEN]; /*信号量名称*/
    VOS_UINT8                           aucReserved[4];
    EVENT_FUNC                          pEventFunc;                                /* 事件处理回调 */
}APP_VCOM_DEV_CTX_STRU;



typedef struct
{
    VOS_CHAR                           *pcAppVcomName;
    VOS_CHAR                           *pcSendSemName;
    VOS_UINT32                          ulAppVcomMemSize;                       /* 设备缓存大小 */
    VOS_UINT32                          ulReserved;
}APP_VCOM_DEV_CONFIG_STRU;



typedef struct
{
    VOS_UINT32                          ulDevIndexErr;                          /* 设备Index错误 */
    VOS_UINT32                          ulVcomDevErr[APP_VCOM_MAX_NUM];         /* 获得设备实体指针错误 */
    VOS_UINT32                          ulMemFullErr[APP_VCOM_MAX_NUM];         /* 设备缓存满计数 */
    VOS_UINT32                          ulSendLenErr[APP_VCOM_MAX_NUM];         /* 发送数据长度为0 */
    VOS_UINT32                          ulReadLenErr[APP_VCOM_MAX_NUM];         /* 读取数据长度为0 */
    VOS_UINT32                          ulAtCallBackErr[APP_VCOM_MAX_NUM];      /* AT回调处理函数返回失败 */
}APP_VCOM_DEBUG_INFO_STRU;



typedef struct
{
    VOS_UINT32                          ulPortIdMask1;                          /* VCOM端口ID掩码vcom0-vcom31 */
    VOS_UINT32                          ulPortIdMask2;                          /* VCOM端口ID掩码 vcom32-vcom63*/
    VOS_UINT32                          ulDebugLevel;                           /* VCOM DEBUG级别:ERR,NORMAL,INFO,DEBUG */
} APP_VCOM_DEBUG_CFG_STRU;

extern APP_VCOM_DEBUG_CFG_STRU          g_stAppVcomDebugCfg;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/


int APP_VCOM_Open(struct inode *inode, struct file *filp );
int APP_VCOM_Release(struct inode *inode, struct file *filp );
ssize_t APP_VCOM_Read(struct file *stFilp, char __user *buf, size_t count, loff_t *ppos);
ssize_t APP_VCOM_Write(struct file *stFilp, const char __user *buf, size_t count, loff_t *ppos);
unsigned int APP_VCOM_Poll(struct file *fp, struct poll_table_struct *wait);
APP_VCOM_DEV_CTX_STRU* APP_VCOM_GetVcomCtxAddr(VOS_UINT8 ucIndex);
APP_VCOM_DEV_ENTITY_STRU* APP_VCOM_GetAppVcomDevEntity(VOS_UINT8 ucIndex);
VOS_UINT8 APP_VCOM_GetIndexFromMajorDevId(VOS_UINT ulMajorDevId);
VOS_VOID  APP_VCOM_InitSpecCtx(VOS_UINT8 ucDevIndex);
VOS_VOID APP_VCOM_Setup(APP_VCOM_DEV_ENTITY_STRU *pstDev, VOS_UINT8 ucIndex);
VOS_INT APP_VCOM_Init(VOS_VOID);

#if (VOS_WIN32 == VOS_OS_VER)
VOS_VOID APP_VCOM_FreeMem(VOS_VOID);
#endif

VOS_VOID APP_VCOM_SendDebugNvCfg(
    VOS_UINT32                          ulPortIdMask1,
    VOS_UINT32                          ulPortIdMask2,
    VOS_UINT32                          ulDebugLevel
);

VOS_VOID APP_VCOM_MNTN_LogPrintf(VOS_CHAR *pcFmt, ...);

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

#endif

