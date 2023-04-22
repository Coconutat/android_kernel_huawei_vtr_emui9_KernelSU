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


#ifndef _NV_CRC_H_
#define _NV_CRC_H_


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "nv_comm.h"

#define NV_RESUME_NO               (0x0)
#define NV_RESUME_IMG              (0x1)
#define NV_RESUME_BAKUP            (0x2)
#define NV_RESUME_DEFAULT          (0x4)

/*是否进行CRC校验标志*/
#define NV_DATA4K_CRC_CHECK_YES    ((((nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR)->crc_mark)&NV_CTRL_DATA_CRC)
#define NV_CTRL_CRC_CHECK_YES      ((((nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR)->crc_mark)&NV_CTRL_CTRL_CRC)
#define NV_ITEM_CRC_CHECK_YES      ((((nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR)->crc_mark)&NV_CTRL_ITEM_CRC)
#define NV_MODEM_CRC_CHECK_YES     ((((nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR)->crc_mark)&NV_CTRL_MODEM_CRC)
#define NV_CTRL2_CRC_CHECK_YES     ((NV_ITEM_CRC_CHECK_YES) || (NV_MODEM_CRC_CHECK_YES))

u32  nv_cal_crc32(u8 *Packet, u32 dwLength);
u32  nv_cal_crc32_custom(u8 *Packet, u32 dwLength);
bool nv_crc_need_make_inwr(void);
bool nv_crc_need_check_inwr(nv_item_info_s *item_info, u32 datalen);
u32  nv_crc_check(u8* pdata, u32 len, u32 crc_code);
u32  nv_crc_check_item(nv_item_info_s *item_info, u32 modem_id);
u32  nv_crc_check_data_section(u32 resume, u32 *pitemid, u32 *pmodemid);
u32  nv_crc_check_ctrl_section(void);
u32  nv_crc_check_ddr(u32 resume);
u32  nv_crc_make_item(nv_item_info_s *item_info, u32 modemid);
u32  nv_crc_check_item_byid(u32 itemid, u32 modem_id);
u32  nv_crc_make_item_wr(nv_wr_req *wreq, nv_item_info_s *item_info, u8 **sbuff, u8 **obuff, u32 *obuff_size);
u32  nv_crc_make_ctrl_section(void);
u32  nv_crc_make_data_section(void);
u32  nv_crc_make_ddr(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif



#endif /*_NV_CRC_H_*/
