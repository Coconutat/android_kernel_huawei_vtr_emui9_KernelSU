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

#ifndef __BSP_OM_ENUM_H__
#define __BSP_OM_ENUM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* field id定义只允许添加，不允许删除，枚举定义删除之后，id值需要保留，新添加id需要跳过删除id */
/* 后续新增id放到DYNAMIC区域，添加AP新的field，放到DUMP_AP_FIELD_END之前，添加CP新的field，放到DUMP_CP_FIELD_END之前 */
/* 添加M3新的field，放到DUMP_M3_FIELD_END之前 */
typedef enum{
    /*AP FIELD IDs*/
    DUMP_MODEMAP_FIELD_BEGIN     = (0x010F0000),
     /* FIX */
    DUMP_MODEMAP_BASE_INFO       = (DUMP_MODEMAP_FIELD_BEGIN),
    DUMP_MODEMAP_TASK_SWITCH     = (0x010F0001),
    DUMP_MODEMAP_INTLOCK         = (0x010F0002),
    DUMP_MODEMAP_TASK_STACK      = (0x010F0003),
    DUMP_MODEMAP_INT_STACK       = (0x010F0004),
    DUMP_MODEMAP_ALLTASK         = (0x010F0005),
    DUMP_MODEMAP_ALLTASK_TCB     = (0x010F0006),
    DUMP_MODEMAP_PRINT           = (0x010F0007),
    DUMP_MODEMAP_REGS            = (0x010F0008),
    DUMP_MODEMAP_CPUVIEW         = (0x010F0009),
    DUMP_MODEMAP_USER_DATA       = (0x010F000B),
    DUMP_MODEMAP_LP              = (0x01100003),
    DUMP_MODEMAP_LP_BUSERROR     = (0x01100004),
    DUMP_MODEMAP_ICC             = (0x01100005),
    DUMP_MODEMAP_KERNEL_LOG      = (0x01100007),
    DUMP_MODEMAP_PM_OM           = (0x01100008),
    DUMP_MODEMAP_NV              = (0x01100009),
    DUMP_MODEMAP_CPUFREQ         = (0x0110000a),
    DUMP_MODEMAP_BASE_INFO_SMP   = (0x0110000b),

    DUMP_MODEMAP_FIELD_END       = (0x01ffffff),

    /*CP FIELD IDs*/
    DUMP_CP_FIELD_BEGIN          = (0x02000000),
    DUMP_CP_BASE_INFO            = DUMP_CP_FIELD_BEGIN,
    DUMP_CP_TASK_SWITCH          = (0x02000001),
    DUMP_CP_INTLOCK              = (0x02000002),
    DUMP_CP_TASK_STACK           = (0x02000003),
    DUMP_CP_SYSTEM_STACK         = (0x02000004),
    DUMP_CP_ALLTASK_NAME         = (0x02000005),
    DUMP_CP_ALLTASK_TCB          = (0x02000006),
    DUMP_CP_PRINT                = (0x02000007),
    DUMP_CP_DMESG                = (0x02000008),
    DUMP_CP_REGS                 = (0x02000009),
    DUMP_CP_CPUVIEW              = (0x0200000A),
    DUMP_CP_USER_DATA            = (0x0200000C),
    DUMP_CP_BBE16_TCM            = (0x0200000D),
    DUMP_CP_DRX                  = (0x0200000E),
    DUMP_CP_TASK_TCB             = (0x0200000F),
    DUMP_CP_RTOSCK_CDA           = (0x02000010),
    DUMP_CP_SIM0                 = (0x02000064),
    DUMP_CP_SIM1                 = (0x02000065),
    DUMP_CP_LP                   = (0x02000067),
    DUMP_CP_LP_BUSERROR          = (0x02000068),
    DUMP_CP_UTRACE0              = (0x02000069),
    DUMP_CP_UTRACE1              = (0x0200006a),
    DUMP_CP_UTRACE2              = (0x0200006b),
    DUMP_CP_UTRACE3              = (0x0200006c),
    DUMP_CP_ICC                  = (0x0200006E),
    DUMP_CP_AMON                 = (0x02000070),
    DUMP_CP_RUN_TRACE            = (0x02000072),
    DUMP_CP_SYSCTRL              = (0x02000074),
    DUMP_CP_PM_OM                = (0x02000075),
    DUMP_SAVE_MOD_DUAL_MODEM     = (0x02000076),
    DUMP_CP_CIPHER               = (0x02000077),
    DUMP_CP_IPF                  = (0x02000078),
    DUMP_CP_PSAM                 = (0x02000079),
    DUMP_CP_PDLOCK               = (0x0200007a),
    DUMP_CP_NV                   = (0x0200007b),
    DUMP_CP_OS_SYS               = (0x0200007C),
    DUMP_CP_TASK_SWITCH1         = (0x0200007d),
    DUMP_CP_TASK_SWITCH2         = (0x0200007e),
    DUMP_CP_TASK_SWITCH3         = (0x0200007f),
    DUMP_CP_SYSTEM_STACK1        = (0x02000080),
    DUMP_CP_SYSTEM_STACK2        = (0x02000081),
    DUMP_CP_SYSTEM_STACK3        = (0x02000082),
    DUMP_CP_SEM_INFO             = (0x02000083),
    DUMP_CP_NOC                  = (0x02000084),
    DUMP_CP_EDMA                 = (0x02000085),
    DUMP_CP_CPM_LTE0             = (0x02000086),
    DUMP_CP_CPM_LTE1             = (0x02000087),
    DUMP_CP_CPM_TDS              = (0x02000088),
    DUMP_CP_CPM_NXP              = (0x02000089),
    /*0x02F00000-- 专用于smp修改新增的特定格式段其他模块不得使用*/
    DUMP_CP_BASE_INFO_SMP        = (0x02f00000),
    DUMP_CP_CPUINFO              = (0x02f00001),
    DUMP_CP_CPUINFO1             = (0x02f00002),
    DUMP_CP_CPUINFO2             = (0x02f00003),
    DUMP_CP_CPUINFO3             = (0x02f00004),
    DUMP_CP_TASK_TCB_SMP         = (0x02f00005),

    DUMP_CP_FIELD_END            = (0x02ffffff),

    /*AP kernel FIELD IDs*/
    DUMP_KERNEL_FIELD_BEGIN     = (0x03000000),
    DUMP_KERNEL_BASE_INFO       = (DUMP_KERNEL_FIELD_BEGIN),
    DUMP_KERNEL_TASK_SWITCH     = (0x030F0001),
    DUMP_KERNEL_INTLOCK         = (0x030F0002),
    DUMP_KERNEL_TASK_STACK      = (0x030F0003),
    DUMP_KERNEL_INT_STACK       = (0x030F0004),
    DUMP_KERNEL_TASK_NAME       = (0x030F0005),
    DUMP_KERNEL_ALLTASK_TCB     = (0x030F0006),
    DUMP_KERNEL_PRINT           = (0x030F0007),
    DUMP_KERNEL_REGS            = (0x030F0008),
    DUMP_KERNEL_CPUVIEW         = (0x030F0009),
    DUMP_KERNEL_USER_DATA       = (0x030F000B),
    DUMP_KERNEL_AMON            = (0x030F000C),
    DUMP_KERNEL_RUNNING_TRACE   = (0x030F000D),/*0x030F000D ~ 0x030F0014 :8个预留给running trace使用*/
    DUMP_KERNEL_CPU_INFO        = (0x030F0015),/*0x030F0015 ~ 0x030F001D :8个预留给cpu info使用*/
    DUMP_KERNEL_BASE_INFO1      = (0x030F001E),/*SMP 系统新增扩展baseinfo ID*/
    DUMP_KERNEL_TASK_TCB        = (0x030F001F),
    DUMP_KERNEL_USB             = (0x03100000),
    DUMP_KERNEL_UTRACE0         = (0x03100001),
    DUMP_KERNEL_DRX             = (0x03100002),
    DUMP_KERNEL_UTRACE1         = (0x03100004),
    DUMP_KERNEL_UTRACE2         = (0x03100005),
    DUMP_KERNEL_UTRACE3         = (0x03100006),
    DUMP_KERNEL_FIELD_END       = (0x03ffffff),

    /*M3 FIELD IDs*/
    DUMP_M3_FIELD_BEGIN         = (0x04000000),
    DUMP_M3_LP_BUSERROR         = (DUMP_M3_FIELD_BEGIN),
    DUMP_M3_ICC                 = (0x04000001),
    DUMP_M3_PM_OM               = (0x04000002),
    DUMP_M3_BASE_INFO           = (0x04000020),
    DUMP_M3_TASK_SWITCH         = (0x04000021),
    DUMP_M3_INTLOCK             = (0x04000022),
    DUMP_M3_TASK_STACK          = (0x04000023),
    DUMP_M3_INT_STACK           = (0x04000024),
    DUMP_M3_ALLTASK             = (0x04000025),
    DUMP_M3_ALLTASK_TCB         = (0x04000026),
    DUMP_M3_LOG             = (0x04000027),
    DUMP_M3_PRINT           = (0x04000028),
    DUMP_M3_REGS                = (0x04000029),
    DUMP_M3_TCM0                = (0x0400002A),
    DUMP_M3_TCM1                = (0x0400002B),
    DUMP_M3_BOOST               = (0x04000064),

    /*0x04100000--0x04200000 专用于smp修改新增的段其他模块不得使用*/
    DUMP_M3_SMP_START           = (0x04100000),
    DUMP_M3_CPUINFO             = (DUMP_M3_SMP_START),
    DUMP_M3_BASEINFO_SMP        = (DUMP_M3_SMP_START+1),
    DUMP_M3_SMP_END             = (0x041FFFFF),

    DUMP_M3_FIELD_END           = (0x04FFFFFF),

    /* SHARE FIELD IDs */
    DUMP_SHARE_FIELD_BEGIN  = (0x05000000),
    DUMP_SHARE_GLOBAL_INFO  = (DUMP_SHARE_FIELD_BEGIN),
    DUMP_SHARE_LOAD_INFO    = (0x05000001),
    DUMP_SHARE_FASTBOOT_INFO= (0x05000002),
    DUMP_SHARE_FIELD_END,

    /* TEEOS FIELD IDs */
    DUMP_TEE_FIELD_BEGIN  = (0x06000000),
    DUMP_TEE_FIELD_LOG    = (0x06000001),
    DUMP_TEE_FIELD_END,
}DUMP_SAVE_MOD_ENUM;


#define AP_TRACE_ID(id)                 (DUMP_KERNEL_UTRACE##id)
#define CP_TRACE_ID(id)                 (DUMP_CP_UTRACE##id)

typedef enum _teeos_errno_e{
    TEEOS_ERRNO_LOAD_SEC_IMAGE  = 0x83000000,   /* modem单独复位中,加载安全镜像失败 */
    TEEOS_ERRNO_BUTT            = 0x83ffffff
}dump_teeos_errno_t;

typedef enum _hifi_errno_e{
    HIFI_ERRNO_MODEM_RESET      = 0x84000000,   /* modem单独复位中HIFI回调失败 */
    HIFI_ERRNO_BUTT             = 0x84ffffff
}dump_hifi_errno_t;

typedef enum _mcore_errno_e{
    LPM3_ERRNO_MODEM_RESET      = 0x85000000,   /* modem单独复位中M3异常 */
    LPM3_ERRNO_BUTT             = 0x85ffffff
}dump_mcore_errno_t;

#ifdef __cplusplus
}
#endif


#endif /* __BSP_OM_ENUM_H__ */


