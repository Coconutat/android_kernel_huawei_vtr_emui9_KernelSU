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


#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <osl_thread.h>
#include <bsp_nvim.h>
#include "nv_comm.h"
#include "nv_file.h"
#include "nv_ctrl.h"
#include "nv_xml_dec.h"
#include "nv_debug.h"
#include "nv_index.h"
#include "nv_crc.h"
#include "nv_partition_img.h"
#include "nv_partition_bakup.h"
#include "NVIM_ResumeId.h"
#include "bsp_dump.h"


/* 计算字符串的CRC */
u32 nv_cal_crc32(u8 *Packet, u32 dwLength)
{
    static u32 CRC32_TABLE[256] = {
        0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
        0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
        0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
        0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
        0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039, 0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
        0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
        0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
        0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
        0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
        0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
        0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
        0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
        0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
        0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
        0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
        0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
        0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
        0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
        0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
        0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
        0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
        0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
        0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
        0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
        0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
        0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
        0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
        0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
        0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
        0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
        0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
        0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4,
    };

    u32 CRC32 = 0;
    u32 i;


    for(i=0; i<dwLength; i++)
    {
        CRC32 = ((CRC32<<8)|Packet[i]) ^ (CRC32_TABLE[(CRC32>>24)&0xFF]); /* [false alarm]:fortify  */
    }

    for(i=0; i<4; i++)
    {
        CRC32 = ((CRC32<<8)|0x00) ^ (CRC32_TABLE[(CRC32>>24)&0xFF]); /* [false alarm]:fortify  */
    }

    return CRC32;
}

/****************************************************************************
*Function:          nv_cal_crc32_custom
*Description:       NV CRC定制，CRC计算出如果是0，则返回magic_number
*Calls:             nv_cal_crc32
*Input:             u8 *Packet 接受的数据包
*                   dwLength 接受的数据包长度
*Output:            NA
*Return:            CRC32
*Others:            NA
****************************************************************************/
u32 nv_cal_crc32_custom(u8 *Packet, u32 dwLength)
{
    u32 CRC32;
    u32 magic_number = 0x5B637EB3;

    if(NULL == Packet)
    {
        nv_printf("[nv_cal_crc32_custom] : input parameter is error!\n");
        return magic_number;
    }

    CRC32 = nv_cal_crc32(Packet, dwLength);

    if(0 == CRC32)
    {
        CRC32 = magic_number;
    }

    return CRC32;
}

bool nv_crc_need_check_inwr(nv_item_info_s *item_info, u32 datalen)
{
    bool need_check = false;

    /* 写全NV时，不需要check crc, 写部分nv时，需要在写前做crc */
    /* itemid crc */
    if(NV_ITEM_CRC_CHECK_YES)
    {
        if((1 == item_info->modem_num) && (datalen == item_info->nv_len))
        {
            need_check = false;
        }
        else
        {
            need_check = true;
        }
    }
    /* modemid crc */
    else if(NV_MODEM_CRC_CHECK_YES)
    {
        if(datalen == item_info->nv_len)
        {
            need_check = false;
        }
        else
        {
            need_check = true;
        }
    }
    else
    {
        need_check = false;
    }

    return need_check;
}

bool nv_crc_need_make_inwr(void)
{
    bool need_check = false;

    /* 写全NV时，不需要check crc, 写部分nv时，需要在写前做crc */
    /* itemid crc */
    if(NV_ITEM_CRC_CHECK_YES)
    {
        need_check = true;
    }
    /* modemid crc */
    else if(NV_MODEM_CRC_CHECK_YES)
    {
        need_check = true;
    }
    else
    {
        need_check = false;
    }

    return need_check;
}

/*****************************************************************************
 函 数 名  : nv_check_nv_data_crc
 功能描述  : 对偏移为offset，长度为datalen的数据按照4k为单位，进行CRC校验
 输入参数  : offset:要写入的数据的偏移，相对于控制文件头
             datalen:待校验的数据长度
 输出参数  : NV_OK:校验通过
 返 回 值  : 无
*****************************************************************************/
u32 nv_crc_check_data_section(u32 resume, u32 *pitemid, u32 *pmodemid)
{
    nv_ctrl_info_s*   ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    nv_item_info_s*   item_tbl;
    u32 ret;
    u32 total;
    u32 i;
    bool has_resume = false;

    if(!((NV_ITEM_CRC_CHECK_YES) || (NV_MODEM_CRC_CHECK_YES)))
    {
        return NV_OK;
    }

    total    = ctrl_info->ref_count;
    item_tbl = (nv_item_info_s*)((unsigned long)ctrl_info
                + NV_GLOBAL_CTRL_INFO_SIZE
                + NV_GLOBAL_FILE_ELEMENT_SIZE*ctrl_info->file_num);

    for(i=0; i<total; i++)
    {
        if(NV_ITEM_CRC_CHECK_YES)
        {
            ret = nv_crc_check_item(&item_tbl[i], 1);
            if(ret)
            {
                if(resume & NV_RESUME_IMG)
                {
                    nv_printf("itemid[0x%x] check crc invalid, need to resume from img!\n", item_tbl[i].itemid);
                    ret = nv_img_resume_item(&item_tbl[i], 1);
                    if(NV_OK == ret)
                    {
                        has_resume = true;
                        nv_printf("itemid[0x%x] resume from img success!\n", item_tbl[i].itemid);
                        continue;
                    }
                    nv_printf("resume from img failed!\n", item_tbl[i].itemid);
                }

                if(resume & NV_RESUME_BAKUP)
                {
                    nv_printf("itemid[0x%x] check crc invalid, need to resume from bakup!\n", item_tbl[i].itemid);
                    ret = nv_bakup_resume_item(&item_tbl[i], 1);
                    if(ret)
                    {
                        nv_printf("resume from bakup failed!\n", item_tbl[i].itemid);
                        *pitemid  = item_tbl[i].itemid;
                        *pmodemid = 1;
                        return ret;
                    }
                    nv_printf("itemid[0x%x] resume from bakup success!\n", item_tbl[i].itemid);
                    has_resume = true;
                }
                else
                {
                    nv_printf("itemid[0x%x] check crc invalid, don't resume or resume failed! 0x%x\n", item_tbl[i].itemid, resume);
                    *pitemid  = item_tbl[i].itemid;
                    *pmodemid = 1;
                    return ret;
                }
            }
        }
        else if(NV_MODEM_CRC_CHECK_YES)
        {
            u32 modemid;

            for(modemid = 1; modemid<=item_tbl[i].modem_num; modemid++)
            {
                ret = nv_crc_check_item(&item_tbl[i], modemid);
                if(ret)
                {
                    if(resume & NV_RESUME_IMG)
                    {
                        nv_printf("itemid[0x%x %d] check crc invalid, need to resume from img!\n", item_tbl[i].itemid, modemid);
                        ret = nv_img_resume_item(&item_tbl[i], 1);
                        if(NV_OK == ret)
                        {
                            has_resume = true;
                            nv_printf("itemid[0x%x %d] resume from img success!\n", item_tbl[i].itemid, modemid);
                            continue;
                        }
                        nv_printf("resume from img failed!\n", item_tbl[i].itemid);
                    }

                    if(resume & NV_RESUME_BAKUP)
                    {
                        nv_printf("itemid[0x%x %d] check crc invalid, need to resume from bakup!\n", item_tbl[i].itemid, modemid);
                        ret = nv_bakup_resume_item(&item_tbl[i], modemid);
                        if(ret)
                        {
                            nv_printf("resume from bakup failed!\n", item_tbl[i].itemid);
                            *pitemid  = item_tbl[i].itemid;
                            *pmodemid = modemid;
                            return ret;
                        }
                        has_resume = true;
                        nv_printf("itemid[0x%x %d] resume from bakup success!\n", item_tbl[i].itemid, modemid);
                    }
                    else
                    {
                        nv_printf("itemid[0x%x] check crc invalid, don't resume or resume failed!\n", item_tbl[i].itemid);
                        *pitemid  = item_tbl[i].itemid;
                        *pmodemid = modemid;
                        return ret;
                    }
                }
            }
        }
    }

    if(has_resume)
    {
        return BSP_ERR_NV_CRC_RESUME_SUCC;
    }

    return NV_OK;
}


u32 nv_crc_check_ctrl_section(void)
{
    nv_ctrl_info_s*   ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    u32 *pcrc;
    u32 new_crc;

    if(NV_CTRL2_CRC_CHECK_YES)
    {
        pcrc = (u32*)((unsigned long)(ctrl_info) + ctrl_info->ctrl_size - 4);
        new_crc = nv_cal_crc32_custom((u8*)(ctrl_info), ctrl_info->ctrl_size - 4);
        if(*pcrc == new_crc)
        {
            return NV_OK;
        }
        else
        {
            return BSP_ERR_NV_CRC_CTRL_ERR;
        }
    }
    else
    {
        return NV_OK;
    }
}

/*****************************************************************************
 函 数 名  : nv_crc_make_crc
 功能描述  : 对NV数据nv.bin生成CRC校验码
 输入参数  : path 文件路径
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
u32 nv_crc_check_ddr(u32 resume)
{
    u32 ret;
    u32 itemid;
    u32 modemid;

    /*检查ctrl段的CRC校验码*/
    ret = nv_crc_check_ctrl_section();
    if(ret)
    {
        nv_printf("check ctrl crc error\n");
        return ret;
    }

    /*检查data段的CRC校验码*/
    ret = nv_crc_check_data_section(resume, &itemid, &modemid);
    if(ret)
    {
        nv_debug(NV_CRC32_DDR_CRC_CHECK, ret, 0, 0, 0);

        return ret;
    }
    return NV_OK;
}

u32 nv_crc_check(u8* pdata, u32 len, u32 crc_code)
{
    u32  new_crc;

    new_crc = nv_cal_crc32_custom(pdata, len);

    return (crc_code == new_crc) ? NV_OK : BSP_ERR_NV_CRC_CODE_ERR;
}

u32 nv_crc_check_item(nv_item_info_s *item_info, u32 modem_id)
{
    u8*  data;
    u32  len;
    u32  cur_crc;
    u32  new_crc;

    /* 写全NV时，不需要check crc, 写部分nv时，需要在写前做crc */
    /* itemid crc */
    if(NV_ITEM_CRC_CHECK_YES)
    {
        data = nv_get_item_base(item_info);
        len  = nv_get_item_len(item_info, NULL);
    }
    /* modemid crc */
    else if(NV_MODEM_CRC_CHECK_YES)
    {
        data = nv_get_item_mdmbase(item_info, modem_id, NULL);
        len  = nv_get_item_mdmlen(item_info, NULL);
    }
    else
    {
        return NV_OK;
    }

    /*IPC锁保护，防止在校验CRC时写NV操作还没有完成*/
    nv_debug_record(NV_DEBUG_WRITEEX_GET_IPC_START);
    nv_ipc_sem_take(IPC_SEM_NV_CRC, IPC_SME_TIME_OUT);
    nv_debug_record(NV_DEBUG_WRITEEX_GET_IPC_END);

    cur_crc = *(u32*)((unsigned long)data+len-4);
    new_crc = nv_cal_crc32_custom(data, len-4);

    nv_ipc_sem_give(IPC_SEM_NV_CRC);
    nv_debug_record(NV_DEBUG_WRITEEX_GIVE_IPC);

    return (cur_crc == new_crc) ? NV_OK : BSP_ERR_NV_CRC_CODE_ERR;
}

u32 nv_crc_check_item_byid(u32 itemid, u32 modem_id)
{
    u32 ret;
    nv_file_info_s file_info = {0};
    nv_item_info_s item_info = {0};

    ret = nv_search_byid(itemid,((u8*)NV_GLOBAL_CTRL_INFO_ADDR),&item_info,&file_info);
    if(ret)
    {
        nv_printf("can not find 0x%x !\n",itemid);
        return BSP_ERR_NV_NO_THIS_ID;
    }

    return nv_crc_check_item(&item_info, modem_id);
}

u32 nv_crc_make_item(nv_item_info_s *item_info, u32 modemid)
{
    u8* item_base;
    u8* mdm_base;
    u8* crc_data  = NULL;
    u32 crc_size  = 0;

    item_base = nv_get_item_base(item_info);
    mdm_base  = nv_get_item_mdmbase(item_info, modemid, NULL);

    /* nvid crc */
    if(NV_ITEM_CRC_CHECK_YES)
    {
        crc_data  = item_base;
        crc_size  = ((u32)item_info->nv_len)*((u32)item_info->modem_num);
    }
    else if(NV_MODEM_CRC_CHECK_YES)
    {
        crc_data  = mdm_base;
        crc_size  = (u32)item_info->nv_len;
    }
    else
    {
        return NV_OK;
    }

    if(!crc_data)
    {
        nv_printf("get nv:0x%x base is null\n", item_info->itemid);
        return NV_ERROR;
    }

    *(u32*)((unsigned long)crc_data+crc_size) = nv_cal_crc32_custom(crc_data, crc_size);

    return NV_OK;
}

u32 nv_crc_make_item_wr(nv_wr_req *wreq, nv_item_info_s *item_info, u8 **sbuff, u8 **obuff, u32 *obuff_size)
{
    u8* item_base;
    u8* mdm_base;
    u8* crc_data  = NULL;
    u32 crc_size  = 0;
    u8* temp_buff;
    u32 buff_size = 0;
    u32 offset    = 0;

    item_base = nv_get_item_base(item_info);
    mdm_base  = nv_get_item_mdmbase(item_info, wreq->modemid, NULL);

    /* nvid crc */
    if(NV_ITEM_CRC_CHECK_YES)
    {
        crc_data  = item_base;
        crc_size  = ((u32)item_info->nv_len)*((u32)item_info->modem_num);
        buff_size = crc_size + 4;
        offset    = (u32)((unsigned long)mdm_base-(unsigned long)item_base) + wreq->offset;
    }
    else if(NV_MODEM_CRC_CHECK_YES)
    {
        crc_data  = mdm_base;
        crc_size  = (u32)item_info->nv_len;
        buff_size = crc_size + 4;
        offset    = wreq->offset;
    }
    else
    {
        *sbuff      = NULL;
        *obuff      = NULL;
        *obuff_size = 0;

        return NV_OK;
    }

    temp_buff = (u8*)vmalloc((unsigned long)buff_size);
    if(NULL == temp_buff)
    {
        nv_show_ddr_info();
        nv_printf("vmalloc failed, nvid 0x%x, nvsize 0x%x\n", item_info->itemid, item_info->nv_len);
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    if(nv_ipc_sem_take(IPC_SEM_NV_CRC, IPC_SME_TIME_OUT))
    {
        nv_printf("take ipc sem timeout.\n");
        vfree(temp_buff);
        return BSP_ERR_NV_TIME_OUT_ERR;
    }
    
    /* coverity[secure_coding] */
    memcpy(temp_buff, crc_data, (unsigned long)crc_size);
    nv_ipc_sem_give(IPC_SEM_NV_CRC);
    /* coverity[secure_coding] */
    memcpy(temp_buff+offset, wreq->pdata, (unsigned long)wreq->size);/* [false alarm]:fortify */
    *(u32*)((unsigned long)temp_buff+crc_size) = nv_cal_crc32_custom(temp_buff, crc_size);

    *sbuff      = crc_data;
    *obuff      = temp_buff;
    *obuff_size = buff_size;

    return NV_OK;
}



/*****************************************************************************
 函 数 名  : nv_crc_make_ctrl_section
 功能描述  : 生成NV CTRL段的CRC校验码
 输入参数  :

 输出参数  : NV_OK:成功
 返 回 值  : 无
*****************************************************************************/
u32 nv_crc_make_ctrl_section(void)
{
    nv_ctrl_info_s*   ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    u32 *pcrc;
    u32 new_crc;

    if(NV_CTRL2_CRC_CHECK_YES)
    {
        pcrc = (u32*)((unsigned long)(ctrl_info) + ctrl_info->ctrl_size - 4);
        new_crc = nv_cal_crc32_custom((u8*)(ctrl_info), ctrl_info->ctrl_size - 4);
        *pcrc = new_crc;
    }

    return NV_OK;
}


/*****************************************************************************
 函 数 名  : nv_crc_make_data_section
 功能描述  : 对NV写入的NV以4K为单位生成CRC校验码,并写入内存对应位置
 输入参数  : offset:写入偏移,相对于ctrl文件头
             datalen:计算CRC数据的长度
 输出参数  : NV_OK:成功
 返 回 值  : 无
*****************************************************************************/
u32 nv_crc_make_data_section(void)
{
    nv_ctrl_info_s*   ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    nv_item_info_s*   item_tbl;
    u32 total;
    u32 i;

    if(!((NV_ITEM_CRC_CHECK_YES) || (NV_MODEM_CRC_CHECK_YES)))
    {
        return NV_OK;
    }

    total    = ctrl_info->ref_count;
    item_tbl = (nv_item_info_s*)((unsigned long)ctrl_info
                + NV_GLOBAL_CTRL_INFO_SIZE
                + NV_GLOBAL_FILE_ELEMENT_SIZE*ctrl_info->file_num);

    for(i=0; i<total; i++)
    {
        if(NV_ITEM_CRC_CHECK_YES)
        {
            (void)nv_crc_make_item(&item_tbl[i], 1);
        }
        else if(NV_MODEM_CRC_CHECK_YES)
        {
            u32 modemid;

            for(modemid = 1; modemid<=item_tbl[i].modem_num; modemid++)
            {
                (void)nv_crc_make_item(&item_tbl[i], modemid);
            }
        }
    }
    return NV_OK;
}


/*****************************************************************************
 函 数 名  : nv_crc_make_ddr
 功能描述  : 对NV数据nv.bin生成CRC校验码
 输入参数  : path 文件路径
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
u32 nv_crc_make_ddr(void)
{
    /*对ctrl段计算CRC校验码*/
    (void)nv_crc_make_ctrl_section();

    /*对nv data段计算CRC校验码*/
    (void)nv_crc_make_data_section();

    return NV_OK;
}

EXPORT_SYMBOL(nv_cal_crc32);
EXPORT_SYMBOL(nv_cal_crc32_custom);
EXPORT_SYMBOL(nv_crc_need_make_inwr);
EXPORT_SYMBOL(nv_crc_need_check_inwr);
EXPORT_SYMBOL(nv_crc_check);
EXPORT_SYMBOL(nv_crc_check_item);
EXPORT_SYMBOL(nv_crc_check_data_section);
EXPORT_SYMBOL(nv_crc_check_ctrl_section);
EXPORT_SYMBOL(nv_crc_check_ddr);
EXPORT_SYMBOL(nv_crc_make_item);
EXPORT_SYMBOL(nv_crc_check_item_byid);
EXPORT_SYMBOL(nv_crc_make_item_wr);
EXPORT_SYMBOL(nv_crc_make_ctrl_section);
EXPORT_SYMBOL(nv_crc_make_data_section);
EXPORT_SYMBOL(nv_crc_make_ddr);


