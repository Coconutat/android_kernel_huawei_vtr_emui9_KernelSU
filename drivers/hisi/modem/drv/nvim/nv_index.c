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


#include <bsp_nvim.h>
#include "nv_comm.h"
#include "nv_ctrl.h"
#include "nv_debug.h"
#include "nv_index.h"
#include "nv_crc.h"
#include "bsp_dump.h"

u8* nv_get_item_base(nv_item_info_s *item_info)
{
    nv_global_info_s* global_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    if(!(item_info->file_id))
    {
        nv_printf("para error\n");
        return NULL;
    }

    return (u8*)(NV_GLOBAL_CTRL_INFO_ADDR 
                 + global_info->file_info[item_info->file_id-1].offset 
                 + item_info->nv_off);
}

u8* nv_get_item_base_byid(u32 itemid)
{
    u32 ret;
    nv_file_info_s file_info;
    nv_item_info_s item_info = {0};
    nv_global_info_s* global_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    ret = nv_search_byid(itemid,((u8*)NV_GLOBAL_CTRL_INFO_ADDR),&item_info,&file_info);
    if(ret)
    {
        nv_printf("can not find 0x%x !\n",itemid);
        return NULL;
    }

    if(!(item_info.file_id))
    {
        nv_printf("nvid:0x%x filed id: 0\n", itemid);
        return NULL;
    }

    return (u8*)(NV_GLOBAL_CTRL_INFO_ADDR 
                 + global_info->file_info[item_info.file_id-1].offset 
                 + item_info.nv_off);
}

u32 nv_get_item_offset(nv_item_info_s *item_info, nv_global_info_s *global_info)
{
    if(NULL == global_info)
    {
        global_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    }
    
    return (u32)(NV_GLOBAL_INFO_SIZE+global_info->file_info[item_info->file_id-1].offset + item_info->nv_off);
}

u32 nv_get_item_fileoffset(nv_item_info_s *item_info, nv_global_info_s *global_info)
{
    if(NULL == global_info)
    {
        global_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    }

    if(!(item_info->file_id))
    {
        nv_printf("para error\n");
        return BSP_ERR_NV_INVALID_PARAM;
    }
    
    return (u32)(global_info->file_info[item_info->file_id-1].offset + item_info->nv_off);
}

u32 nv_get_item_len(nv_item_info_s *item_info, nv_ctrl_info_s *ctrl_info)
{
    u32 item_len;

    if(!ctrl_info)
    {
        ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    }

    if((ctrl_info->crc_mark)&NV_CTRL_MODEM_CRC)
    {
        item_len = (item_info->nv_len + 4)*item_info->modem_num;
    }
    else if((ctrl_info->crc_mark)&NV_CTRL_ITEM_CRC)
    {
        item_len = item_info->nv_len*item_info->modem_num + 4;
    }
    else
    {
        item_len = item_info->nv_len*item_info->modem_num;
    }
    
    return item_len;
}

u8* nv_get_item_mdmbase(nv_item_info_s *item_info, u32 modem_id, nv_ctrl_info_s *ctrl_info)
{
    u8* mdm_base;

    if(!ctrl_info)
    {
        ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    }

    if((ctrl_info->crc_mark)&NV_CTRL_MODEM_CRC)
    {
        mdm_base = (u8 *)((unsigned long)nv_get_item_base(item_info)
                   + (unsigned long)(modem_id - NV_USIMM_CARD_1)*(item_info->nv_len + 4));
    }
    else
    {
        mdm_base = (u8 *)((unsigned long)nv_get_item_base(item_info) 
                   + (unsigned long)(modem_id - NV_USIMM_CARD_1)*item_info->nv_len);
    }
    
    return mdm_base;
}

u32 nv_get_item_mdmoffset(nv_item_info_s *item_info, u32 modem_id, nv_global_info_s *global_info, nv_ctrl_info_s *ctrl_info)
{
    u32 mdm_offset;

    if(!ctrl_info)
    {
        ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    }

    if((ctrl_info->crc_mark)&NV_CTRL_MODEM_CRC)
    {
        mdm_offset = nv_get_item_offset(item_info, global_info) + (modem_id - NV_USIMM_CARD_1)*(item_info->nv_len + 4);
    }
    else
    {
        mdm_offset = nv_get_item_offset(item_info, global_info) + (modem_id - NV_USIMM_CARD_1)*item_info->nv_len;
    }

    
    return mdm_offset;
}

u32 nv_get_item_filemdmoffset(nv_item_info_s *item_info, u32 modem_id, nv_global_info_s *global_info, nv_ctrl_info_s *ctrl_info)
{
    u32 mdm_offset;

    if(!ctrl_info)
    {
        ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    }

    if((ctrl_info->crc_mark)&NV_CTRL_MODEM_CRC)
    {
        mdm_offset = nv_get_item_fileoffset(item_info, global_info) + (modem_id - NV_USIMM_CARD_1)*(item_info->nv_len + 4);
    }
    else
    {
        mdm_offset = nv_get_item_fileoffset(item_info, global_info) + (modem_id - NV_USIMM_CARD_1)*item_info->nv_len;
    }
    
    return mdm_offset;
}

u32 nv_get_item_mdmlen(nv_item_info_s *item_info, nv_ctrl_info_s *ctrl_info)
{
    u32 mdm_len;

    if(!ctrl_info)
    {
        ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    }

    if((ctrl_info->crc_mark)&NV_CTRL_MODEM_CRC)
    {
        mdm_len = item_info->nv_len + 4;
    }
    else
    {
        mdm_len = item_info->nv_len;
    }
    
    return mdm_len;
}
/*
 * search nv info by nv id
 * &pdata:  data start ddr
 * output: ref_info,file_info
 */
u32 nv_search_byid(u32 itemid,u8* pdata, nv_item_info_s* item_info, nv_file_info_s* file_info)
{
    u32 low;
    u32 high;
    u32 mid;
    u32 offset;
    nv_ctrl_info_s* ctrl_info = (nv_ctrl_info_s*)(unsigned long)pdata;

    high = ctrl_info->ref_count;
    low  = 1;

    nv_debug(NV_FUN_SEARCH_NV,0,itemid,high,(u32)(unsigned long)ctrl_info);

    while(low <= high)
    {
        mid = (low+high)/2;

        offset = (u32)(NV_GLOBAL_CTRL_INFO_SIZE + NV_GLOBAL_FILE_ELEMENT_SIZE*ctrl_info->file_num + (mid -1)*NV_REF_LIST_ITEM_SIZE);
        /* coverity[secure_coding] */
        memcpy((u8*)item_info,(u8*)ctrl_info+offset,NV_REF_LIST_ITEM_SIZE);

        if(itemid < item_info->itemid)
        {
            high = mid-1;
        }
        else if(itemid > item_info->itemid)
        {
            low = mid+1;
        }
        else
        {
            offset = NV_GLOBAL_CTRL_INFO_SIZE + NV_GLOBAL_FILE_ELEMENT_SIZE*(item_info->file_id -1);
            /* coverity[secure_coding] */
            memcpy((u8*)file_info,(u8*)ctrl_info+offset,NV_GLOBAL_FILE_ELEMENT_SIZE);
            return NV_OK;
        }
    }
    return BSP_ERR_NV_NO_THIS_ID;

}

EXPORT_SYMBOL(nv_get_item_base);
EXPORT_SYMBOL(nv_get_item_base_byid);
EXPORT_SYMBOL(nv_get_item_offset);
EXPORT_SYMBOL(nv_get_item_fileoffset);
EXPORT_SYMBOL(nv_get_item_len);
EXPORT_SYMBOL(nv_get_item_mdmbase);
EXPORT_SYMBOL(nv_get_item_mdmoffset);
EXPORT_SYMBOL(nv_get_item_filemdmoffset);
EXPORT_SYMBOL(nv_get_item_mdmlen);
EXPORT_SYMBOL(nv_search_byid);



