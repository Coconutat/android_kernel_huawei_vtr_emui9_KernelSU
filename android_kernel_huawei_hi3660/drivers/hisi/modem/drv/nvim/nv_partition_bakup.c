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


#include <linux/vmalloc.h>
#include <bsp_nvim.h>
#include "nv_comm.h"
#include "nv_ctrl.h"
#include "nv_debug.h"
#include "nv_index.h"
#include "nv_crc.h"
#include "nv_partition_bakup.h"
#include "bsp_dump.h"

nv_ctrl_info_s      *g_nv_bakup_ctrl    = NULL;
nv_global_info_s    *g_nv_bakup_global  = NULL;

u32 nv_bakup_ctrl(nv_ctrl_info_s** ctrl)
{
    nv_ctrl_info_s      *ctrl_temp = NULL;

    if(g_nv_bakup_ctrl)
    {
        *ctrl = g_nv_bakup_ctrl;
        return NV_OK;
    }
    else
    {
        ctrl_temp = vmalloc(sizeof(nv_ctrl_info_s));
        if(NULL == ctrl_temp)
        {
            *ctrl = NULL;
            return BSP_ERR_NV_MALLOC_FAIL;
        }
        else
        {
            FILE* fp;
            u32   ret;

            fp = nv_file_open(NV_BACK_PATH, (s8*)NV_FILE_READ);
            if(!fp)
            {
                vfree(ctrl_temp);
                *ctrl = NULL;
                 return BSP_ERR_NV_NO_FILE;
            }

            ret = (u32)nv_file_read((u8*)(ctrl_temp), 1 , (u32)sizeof(nv_ctrl_info_s),fp);
            nv_file_close(fp);
            if(ret != sizeof(nv_ctrl_info_s))
            {
                vfree(ctrl_temp);
                *ctrl = NULL;
                return BSP_ERR_NV_READ_FILE_FAIL;
            }
            else
            {
                *ctrl = ctrl_temp;
                g_nv_bakup_ctrl = ctrl_temp;
                return NV_OK;
            }
        }
    }
}

u32 nv_bakup_global(nv_global_info_s** global, nv_ctrl_info_s *index)
{
    u32 ret;
    nv_ctrl_info_s      *index_local = index;
    nv_global_info_s    *global_temp = NULL;

    if(g_nv_bakup_global)
    {
        *global = g_nv_bakup_global;
        return NV_OK;
    }
    else
    {
        global_temp = vmalloc(sizeof(nv_global_info_s)+1);
        if(NULL == global_temp)
        {
            *global = NULL;
            return BSP_ERR_NV_MALLOC_FAIL;
        }
        if(!index_local)
        {
             ret = nv_bakup_index_create(&index_local);
             if(NV_OK != ret)
             {
                vfree(global_temp);
                *global = NULL;
                return ret;
             }
        }

        ret = nv_init_file_info(index_local, global_temp);
        if(ret)
        {
            vfree(global_temp);
            nv_bakup_index_free(index_local);
            *global = NULL;
            return ret;
        }

        *global = global_temp;
        g_nv_bakup_global = global_temp;

        if(NULL == index)
        {
            nv_bakup_index_free(index_local);
        }

        return NV_OK;
    }
}

u32 nv_bakup_ctrl_reset(void)
{
    nv_ctrl_info_s      *ctrl_temp;
    FILE* fp;
    u32   ret;

    if(NULL == g_nv_bakup_ctrl)
    {
        return NV_OK;
    }

    ctrl_temp = g_nv_bakup_ctrl;
    g_nv_bakup_ctrl = NULL;

    fp = nv_file_open(NV_BACK_PATH, (s8*)NV_FILE_READ);
    if(!fp)
    {
        vfree(ctrl_temp);
        return BSP_ERR_NV_NO_FILE;
    }

    ret = (u32)nv_file_read((u8*)(ctrl_temp), 1, (u32)sizeof(nv_ctrl_info_s),fp);
    nv_file_close(fp);
    if(ret != sizeof(nv_ctrl_info_s))
    {
        vfree(ctrl_temp);
        return BSP_ERR_NV_READ_FILE_FAIL;
    }
    else
    {
        g_nv_bakup_ctrl = ctrl_temp;
        return NV_OK;
    }
}

u32 nv_bakup_global_reset(void)
{
    u32 ret;
    nv_ctrl_info_s      *index_local = NULL;
    nv_global_info_s    *global_temp;

    if(NULL == g_nv_bakup_global)
    {
        return NV_OK;
    }

    global_temp = g_nv_bakup_global;
    g_nv_bakup_global = NULL;

    ret = nv_bakup_index_create(&index_local);
    if(NV_OK != ret)
    {
        vfree(global_temp);
        return ret;
    }

    ret = nv_init_file_info(index_local, global_temp);
    nv_bakup_index_free(index_local);
    if(ret)
    {
        vfree(global_temp);
        return ret;
    }

    g_nv_bakup_global = global_temp;

    return NV_OK;
}

u32 nv_bakup_info_reset(void)
{
    u32 ret;
    ret  = nv_bakup_ctrl_reset();
    ret |= nv_bakup_global_reset();

    return ret;
}

u32 nv_bakup_index_create(nv_ctrl_info_s** index)
{
    u32 ret;
    FILE* fp;
    nv_ctrl_info_s *bakup_ctrl = NULL;
    nv_ctrl_info_s *index_tmp;

    ret = nv_bakup_ctrl(&bakup_ctrl);
    if(NV_OK != ret)
    {
        return ret;
    }

    index_tmp = (nv_ctrl_info_s*)vmalloc((size_t)bakup_ctrl->ctrl_size);
    if(NULL == index_tmp)
    {
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    fp = nv_file_open(NV_BACK_PATH,(s8*)NV_FILE_READ);
    if(!fp)
    {
        vfree(index_tmp);
        return BSP_ERR_NV_NO_FILE;
    }

    nv_file_seek(fp,0,SEEK_SET); /*jump to file head*/
    ret = (u32)nv_file_read((u8*)index_tmp,1,bakup_ctrl->ctrl_size,fp);
    nv_file_close(fp);
    if(ret != bakup_ctrl->ctrl_size)
    {
        vfree(index_tmp);
        return BSP_ERR_NV_READ_FILE_FAIL;
    }

    *index = index_tmp;
    return NV_OK;
}

void nv_bakup_index_free(nv_ctrl_info_s* index)
{
    vfree(index);
    return;
}

bool nv_bakup_validity(void)
{
    u32 ret;
    nv_ctrl_info_s *bakup_ctrl;

    /*文件不存在*/
    if(nv_file_access((s8*)NV_BACK_PATH,0))
    {
        return false;
    }

    /*有未写入完成的标志 */
    if(true == nv_flag_file_isExist((s8*)NV_BACK_FLAG_PATH))
    {
        nv_record("%s  last time write abornormal !\n",NV_BACK_FLAG_PATH);
        return false;
    }

    ret = nv_bakup_ctrl(&bakup_ctrl);
    if(ret || (!bakup_ctrl) || (NV_CTRL_FILE_MAGIC_NUM != bakup_ctrl->magicnum))
    {
        return false;
    }

    /*imei号检查*/
    /*
    ret = nv_imei_data_comp((s8*)filePath);
    if(ret)
    {
        nv_record("%s imei compare with factory data is not same ret :0x%x!\n",filePath,ret);
        return false;
    }
    */

    return true;
}

u32 nv_bakup_get_item_data(u32 itemid, void* buffer, u32 len)
{
    FILE* fp            = NULL;
    u32   ret;
    u32   file_offset   = 0;
    u32   item_len      = 0;
    nv_ctrl_info_s*     index       = NULL;
    nv_ctrl_info_s*     bakup_ctrl  = NULL;   /*bak file ctrl file head*/
    nv_global_info_s*   global_info = NULL;
    nv_file_info_s      file_info   = {0};
    nv_item_info_s      item_info   = {0};

    if(true != nv_check_file_validity((s8 *)NV_BACK_PATH, (s8 *)NV_BACK_FLAG_PATH))
    {
        return BSP_ERR_NV_BACUP_INVALID_ERR;
    }

    ret = nv_bakup_ctrl(&bakup_ctrl);
    if(ret)
    {
        nv_printf("get bakup ctrl failed!  0x%x itemid=0x%x\n", ret, itemid);
        return ret;
    }

    ret = nv_bakup_index_create(&index);
    if(NV_OK != ret)
    {
        nv_printf("create bakup index table failed! 0x%x itemid=0x%x\n", ret, itemid);
        return ret;
    }

    ret = nv_bakup_global(&global_info, index);
    if(NV_OK != ret)
    {
        nv_printf("get global failed!  0x%x itemid=0x%x\n", ret, itemid);
        goto out1;
    }

    ret = nv_search_byid(itemid,(u8*)index,&item_info,&file_info);
    if(ret)
    {
        nv_printf("there is no this nvid 0x%x in nv bakup! 0x%x\n", itemid, ret);
        goto out1;
    }

    file_offset = nv_get_item_fileoffset(&item_info, global_info);
    item_len    = nv_get_item_len(&item_info, bakup_ctrl);
    if(len < item_len)
    {
        ret = BSP_ERR_NV_INVALID_PARAM;
        nv_printf("param len is too small! 0x%x, 0x%x\n", len, item_len);
        goto out1;
    }

    fp = nv_file_open(NV_BACK_PATH,(s8*)NV_FILE_READ);
    if(!fp)
    {
        ret = BSP_ERR_NV_NO_FILE;
        nv_printf("open bakup file failed! 0x%x 0x%x\n", fp, itemid);
        goto out1;
    }

    nv_file_seek(fp, file_offset,SEEK_SET);
    ret = (u32)nv_file_read(buffer, 1, item_len, fp);
    nv_file_close(fp);
    if(ret != item_len)
    {
        ret = BSP_ERR_NV_READ_FILE_FAIL;
        nv_printf("read bakup file failed! 0x%x 0x%x\n", ret, item_len);
        goto out1;
    }

    nv_bakup_index_free(index);
    return NV_OK;

out1:
    nv_bakup_index_free(index);
    return ret;
}

u32 nv_bakup_get_item_mdmdata(u32 itemid, u32 modem_id, void* buffer, u32 len)
{
    FILE* fp            = NULL;
    u32   ret;
    u32   file_offset   = 0;
    u32   modem_len     = 0;
    nv_ctrl_info_s*     index       = NULL;
    nv_ctrl_info_s*     bakup_ctrl  = NULL;   /*bak file ctrl file head*/
    nv_global_info_s*   global_info = NULL;
    nv_file_info_s      file_info   = {0};
    nv_item_info_s      item_info   = {0};

    if(true != nv_check_file_validity((s8 *)NV_BACK_PATH, (s8 *)NV_BACK_FLAG_PATH))
    {
        nv_printf("bakup partition is invalid!  0x%x 0x%x\n", itemid, modem_id);
        return BSP_ERR_NV_BACUP_INVALID_ERR;
    }

    ret = nv_bakup_ctrl(&bakup_ctrl);
    if(ret)
    {
        nv_printf("get bakup ctrl failed!  0x%x itemid=0x%x 0x%x\n", ret, itemid, modem_id);
        return ret;
    }

    ret = nv_bakup_index_create(&index);
    if(NV_OK != ret)
    {
        nv_printf("create bakup index table failed! 0x%x itemid=0x%x 0x%x\n", ret, itemid, modem_id);
        return ret;
    }

    ret = nv_bakup_global(&global_info, index);
    if(NV_OK != ret)
    {
        nv_printf("get global failed!  0x%x itemid=0x%x\n", ret, itemid);
        goto out1;
    }

    ret = nv_search_byid(itemid,(u8*)index,&item_info,&file_info);
    if(ret)
    {
        nv_printf("there is no this nvid 0x%x in nv bakup! 0x%x\n", itemid, ret);
        goto out1;
    }

    file_offset  = nv_get_item_filemdmoffset(&item_info, modem_id, global_info, bakup_ctrl);
    modem_len    = nv_get_item_mdmlen(&item_info, bakup_ctrl);
    if(len < modem_len)
    {
        ret = BSP_ERR_NV_INVALID_PARAM;
        nv_printf("param len is too small! 0x%x, 0x%x\n", len, modem_len);
        goto out1;
    }

    fp = nv_file_open(NV_BACK_PATH,(s8*)NV_FILE_READ);
    if(!fp)
    {
        nv_printf("open bakup file failed! 0x%x 0x%x\n", fp, itemid);
        ret = BSP_ERR_NV_NO_FILE;
        goto out1;
    }

    nv_file_seek(fp,file_offset,SEEK_SET);
    ret = (u32)nv_file_read(buffer, 1, modem_len, fp);
    nv_file_close(fp);
    if(ret != modem_len)
    {
        nv_printf("read bakup file failed! 0x%x 0x%x\n", ret, modem_len);
        ret = BSP_ERR_NV_READ_FILE_FAIL;
        goto out1;
    }

    nv_bakup_index_free(index);
    return NV_OK;

out1:
    nv_bakup_index_free(index);
    return ret;
}

/*****************************************************************************
 函 数 名  : nv_resume_item_from_img
 功能描述  : 从工作恢复一个NV项
 输入参数  : void
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
u32 nv_bakup_resume_item(nv_item_info_s *item_info, u32 modem_id)
{
    u32 ret         = NV_ERROR;
    u8* item_base;
    u8* mdm_base;
    u32 item_len;
    u32 mdm_len;
    u8* resume_data = NULL;
    u32 resume_size = 0;
    u8* temp_buff;
    u32 buff_size   = 0;
    bool need_crc   = 0;
    u32 crc_code    = 0;
    u32 new_crc     = 0;

    item_base   = nv_get_item_base(item_info);
    mdm_base    = nv_get_item_mdmbase(item_info, modem_id, NULL);
    item_len    = nv_get_item_len(item_info, NULL);
    mdm_len     = nv_get_item_mdmlen(item_info, NULL);

    /* nvid crc */
    if(NV_ITEM_CRC_CHECK_YES)
    {
        resume_data     = item_base;
        resume_size     = item_len;
        buff_size       = resume_size;
        need_crc        = true;
    }
    /* modemid crc */
    else if(NV_MODEM_CRC_CHECK_YES)
    {
        resume_data     = mdm_base;
        resume_size     = mdm_len;
        buff_size       = resume_size;
        need_crc        = true;
    }
    else
    {
        resume_data     = mdm_base;
        resume_size     = mdm_len;
        buff_size       = resume_size;
        need_crc        = false;
    }

    temp_buff = vmalloc((size_t)buff_size);
    if(NULL == temp_buff)
    {
        nv_debug(NV_FUN_RESUME_BAK_ITEM,2,0,0,0);
        nv_record("resume nvid vmalloc fail 0x%x\n", item_info->itemid);
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    if(NV_ITEM_CRC_CHECK_YES)
    {
        ret = nv_bakup_get_item_data(item_info->itemid, temp_buff, buff_size);
    }
    else
    {
        ret = nv_bakup_get_item_mdmdata(item_info->itemid, modem_id, temp_buff, buff_size);
    }
    if(ret)
    {
        vfree(temp_buff);
        nv_debug(NV_FUN_RESUME_BAK_ITEM,3,0,0,0);
        nv_record("resume nvid read data failed 0x%x\n", item_info->itemid);
        return ret;
    }

    if(need_crc)
    {
        crc_code = *(u32*)((unsigned long)temp_buff+buff_size-4);
        new_crc  = nv_cal_crc32_custom((u8*)temp_buff, buff_size - 4);
        if(new_crc == crc_code)
        {
            *(u32*)((unsigned long)temp_buff+buff_size-4) = crc_code;
        }
        else
        {
            vfree(temp_buff);
            nv_debug(NV_FUN_RESUME_BAK_ITEM,4,0,0,0);
            nv_record("resume nvid check crc failed 0x%x\n", item_info->itemid);
            return BSP_ERR_NV_CRC_CODE_ERR;
        }
    }

    ret = nv_ipc_sem_take(IPC_SEM_NV_CRC, IPC_SME_TIME_OUT);
    if(ret)
    {
        nv_printf("take ipc sem timeout.\n");
        vfree(temp_buff);
        return ret;
    }
    /* coverity[secure_coding] */
    memcpy(resume_data, temp_buff, (size_t)resume_size);
    nv_flush_cache(resume_data, resume_size);
    nv_ipc_sem_give(IPC_SEM_NV_CRC);

    vfree(temp_buff);

    return NV_OK;
}

EXPORT_SYMBOL(nv_bakup_validity);
EXPORT_SYMBOL(nv_bakup_global);
EXPORT_SYMBOL(nv_bakup_get_item_data);
EXPORT_SYMBOL(nv_bakup_get_item_mdmdata);
EXPORT_SYMBOL(nv_bakup_resume_item);





