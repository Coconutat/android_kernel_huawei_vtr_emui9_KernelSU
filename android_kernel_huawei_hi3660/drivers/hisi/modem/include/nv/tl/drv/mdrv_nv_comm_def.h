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

#ifndef __MDRV_NV_COMM_DEF_H__
#define __MDRV_NV_COMM_DEF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifndef __u8_defined
#define __u8_defined
typedef signed char          s8;
typedef unsigned char        u8;
typedef signed short         s16;
typedef unsigned short       u16;
typedef signed int           s32;
typedef unsigned int         u32;
typedef signed long long     s64;
typedef unsigned long long   u64;
#endif

/*****************************************************************************
 结构名       :  NV_FEMPIN_TO_GPIO_STRU
 结构说明  :  射频管脚编号和gpio编号对应关系nv定义
 NV ID               : 18003
*****************************************************************************/
#define FEM_IO_CFG_MAX    16 /*18002 nv中最多支持的配置个数*/
#define FEM_PIN_MAX       32 /*射频管脚个数*/

typedef struct{
    u32 uwFemPinToGpio[FEM_PIN_MAX];
}NV_FEMPIN_TO_GPIO_STRU;

typedef struct
{
    u32 uwGpioMask;              /*射频管脚掩码,0表示该bit位不需要控制*/
    u32 uwGpioValue;             /*射频管脚输出电平 值,0表示该低电平，1表示高电平*/
}RFFE_GPIO_VALUE_STRU;

/*****************************************************************************
 结构名    : RF_NV_MIPIDEV_CMD_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : MIPI命令控制结构体
*****************************************************************************/
typedef struct
{
    u32  bitByteCnt          :5;     /* 0表示该CMMAND无效，取值0~4 */
    u32  bitMipiPortSel      :3;     /* mipi port 0~7 */
    u32  bitSlaveID          :4;     /* slave id 0~15 */
    u32  bitRegAddr          :16;    /* reg addr 0~65535 */
    u32  bitRsv              :4;     /* 四字节对齐 */
} RF_NV_MIPIDEV_CMD_STRU;

typedef RF_NV_MIPIDEV_CMD_STRU PHY_MIPIDEV_CMD_STRU;

/*****************************************************************************
 结构名    : RF_NV_MIPIDEV_DATA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : MIPI数据结构体
*****************************************************************************/
typedef struct
{
    u32  bitByte0  :8;    /* 第一个byte */
    u32  bitByte1  :8;    /* 第二个byte */
    u32  bitByte2  :8;    /* 第三个byte */
    u32  bitByte3  :8;    /* 第四个byte */
} RF_NV_MIPIDEV_DATA_STRU;

typedef RF_NV_MIPIDEV_DATA_STRU PHY_MIPIDEV_DATA_STRU;

/*****************************************************************************
 结构名    : RF_NV_MIPIDEV_UNIT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : MIPI命令最小单元
*****************************************************************************/
typedef struct
{
    RF_NV_MIPIDEV_CMD_STRU      stCmd;    /*控制域*/
    RF_NV_MIPIDEV_DATA_STRU     stData;   /*数据域,一个MIPI占用2个UINT32*/
} RF_NV_MIPIDEV_UNIT_STRU;

typedef RF_NV_MIPIDEV_UNIT_STRU PHY_MIPIDEV_UNIT_STRU;

typedef struct{
    RF_NV_MIPIDEV_UNIT_STRU astAntMipiCtrlWord[4];
}RF_NV_MIPIDEV_04CMD;
typedef struct
{
    u32                     enMode;         /*配置模式*/
    RFFE_GPIO_VALUE_STRU    stGpioCtrl;     /*GPIO 配置  */
    RF_NV_MIPIDEV_04CMD     stMipiCtrl;     /*MIPI  配置  */
}FEM_GPIO_MIPIDEV_CTRL_STRU;

typedef struct
{
    FEM_GPIO_MIPIDEV_CTRL_STRU stFemGpioMipiCtrl[FEM_IO_CFG_MAX];
}NV_FEM_GPIO_MIPIDEV_CTRL_STRU;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

