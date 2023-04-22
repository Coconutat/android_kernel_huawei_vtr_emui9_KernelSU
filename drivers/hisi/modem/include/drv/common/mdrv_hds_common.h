/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
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

#ifndef __MDRV_HDS_COMMON_H__
#define __MDRV_HDS_COMMON_H__
#ifdef __cplusplus
extern "C"
{
#endif

/*底软cmdid注册规则:31-28bit(组件ID):0x9; 27-24bit(模指示); 23-19bit(消息类型):0x0; 18-0bit(消息ID):自己定义*/
/* BSP CFG BEGIN*/
#define DIAG_CMD_LOG_SET                        (0x90035308)

/* 文件操作类（0x5600-0x56ff）*/
#define DIAG_CMD_FS_QUERY_DIR                   (0x90015601)
#define DIAG_CMD_FS_SCAN_DIR                    (0x90015602)
#define DIAG_CMD_FS_MAKE_DIR                    (0x90015603)
#define DIAG_CMD_FS_OPEN                        (0x90015604)
#define DIAG_CMD_FS_IMPORT                      (0x90015605)
#define DIAG_CMD_FS_EXPORT                      (0x90015606)
#define DIAG_CMD_FS_DELETE                      (0x90015607)
#define DIAG_CMD_FS_SPACE                       (0x90015608)

/* NV操作类（0x5500-0x55ff）*/
#define DIAG_CMD_NV_WR                          (0x90015001)
#define DIAG_CMD_NV_RD                          (0x90015003)
#define DIAG_CMD_GET_NV_LIST                    (0x90015005)
#define DIAG_CMD_GET_NV_RESUM_LIST              (0x90015006)
#define DIAG_CMD_NV_AUTH                        (0x90015007)

/* BSP CFG END*/

typedef int (*bsp_hds_func)(unsigned char *pstReq);
typedef int (*hds_cnf_func)(void *hds_cnf, void *data, unsigned int len);
int mdrv_hds_printlog_conn(void);
int mdrv_hds_translog_conn(void);
int mdrv_hds_msg_proc(void *pstReq);
void mdrv_hds_cmd_register(unsigned int cmdid, bsp_hds_func fn);
void mdrv_hds_get_cmdlist(unsigned int *cmdlist, unsigned int *num);
void mdrv_hds_cnf_register(hds_cnf_func fn);

#ifdef __cplusplus
}
#endif
#endif

