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

#ifndef __MDRV_ACORE_EFUSE_H__
#define __MDRV_ACORE_EFUSE_H__

#ifdef __cplusplus
extern "C"
{
#endif


    
    int mdrv_efuse_get_chipid(unsigned char* buf,int length);

    
    int mdrv_efuse_get_dieid(unsigned char* buf, int length);


#define MDRV_EFUSE_IOCTL_CMD_GET_SOCID          (0)
#define MDRV_EFUSE_IOCTL_CMD_SET_KCE            (1)
#define MDRV_EFUSE_IOCTL_CMD_GET_KCE            (2)
#define MDRV_EFUSE_IOCTL_CMD_SET_SECURESTATE    (3)
#define MDRV_EFUSE_IOCTL_CMD_GET_SECURESTATE    (4)
#define MDRV_EFUSE_IOCTL_CMD_SET_SECUREDEBUG    (5)
#define MDRV_EFUSE_IOCTL_CMD_GET_SECUREDEBUG    (6)
#define MDRV_EFUSE_IOCTL_CMD_SET_DBGEN          (7)
#define MDRV_EFUSE_IOCTL_CMD_GET_DBGEN          (8)
#define MDRV_EFUSE_IOCTL_CMD_SET_NIDEN          (9)
#define MDRV_EFUSE_IOCTL_CMD_GET_NIDEN          (10)
#define MDRV_EFUSE_IOCTL_CMD_SET_SPIDEN         (11)
#define MDRV_EFUSE_IOCTL_CMD_GET_SPIDEN         (12)
#define MDRV_EFUSE_IOCTL_CMD_SET_SPNIDEN        (13)
#define MDRV_EFUSE_IOCTL_CMD_GET_SPNIDEN        (14)
#define MDRV_EFUSE_IOCTL_CMD_SET_HIFIDBGEN      (15)
#define MDRV_EFUSE_IOCTL_CMD_GET_HIFIDBGEN      (16)
#define MDRV_EFUSE_IOCTL_CMD_SET_BBE16DBGEN     (17)
#define MDRV_EFUSE_IOCTL_CMD_GET_BBE16DBGEN     (18)
#define MDRV_EFUSE_IOCTL_CMD_SET_CSDEVICEEN     (19)
#define MDRV_EFUSE_IOCTL_CMD_GET_CSDEVICEEN     (20)
#define MDRV_EFUSE_IOCTL_CMD_SET_JTAGEN         (21)
#define MDRV_EFUSE_IOCTL_CMD_GET_JTAGEN         (22)
#define MDRV_EFUSE_IOCTL_CMD_SET_SECDBGRESET    (23)
#define MDRV_EFUSE_IOCTL_CMD_GET_SECDBGRESET    (24)
#define MDRV_EFUSE_IOCTL_CMD_SET_CSRESET        (25)
#define MDRV_EFUSE_IOCTL_CMD_GET_CSRESET        (26)
#define MDRV_EFUSE_IOCTL_CMD_SET_DFTSEL         (27)
#define MDRV_EFUSE_IOCTL_CMD_GET_DFTSEL         (28)
#define MDRV_EFUSE_IOCTL_CMD_SET_AUTHKEY        (29)
#define MDRV_EFUSE_IOCTL_CMD_GET_AUTHKEY        (30)
#define MDRV_EFUSE_IOCTL_CMD_SET_AUTHKEYRD      (31)
#define MDRV_EFUSE_IOCTL_CMD_GET_AUTHKEYRD      (32)
#define MDRV_EFUSE_IOCTL_CMD_SET_NSIVERIFY      (33)
#define MDRV_EFUSE_IOCTL_CMD_GET_NSIVERIFY      (34)


    
    int mdrv_efuse_ioctl(int cmd, int arg, unsigned char* buf, int len);


#ifdef __cplusplus
}
#endif
#endif

