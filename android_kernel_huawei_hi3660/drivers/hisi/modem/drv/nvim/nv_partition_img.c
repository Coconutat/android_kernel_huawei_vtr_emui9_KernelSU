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
#include <linux/syscalls.h>
#include <bsp_nvim.h>
#include <bsp_rfile.h>
#include <bsp_blk.h>
#include <bsp_onoff.h>
#include <ptable_com.h>
#include "nv_comm.h"
#include "nv_ctrl.h"
#include "nv_debug.h"
#include "nv_index.h"
#include "nv_crc.h"
#include "nv_partition_img.h"
#include "bsp_dump.h"

extern struct nv_path_info_stru g_nv_path;


u32 s_delet_ret = NV_OK;

void nv_img_clear_check_result(void)
{
    s_delet_ret = NV_OK;

    return;
}

/************************************************************************
 函 数 名  : nv_img_get_white_list_num
 功能描述  : 获取白名单文件个数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 白名单文件个数
*************************************************************************/
u32 nv_img_get_white_list_num(void)
{
    return NV_MAX;
}

/************************************************************************
 函 数 名  : nv_img_is_white_file
 功能描述  : 判定某个文件是否是白名单上的文件
 输入参数  : pdir 白名单文件跟目录
 输出参数  : 无
 返 回 值  : NV_OK
             NV_ERROR
*************************************************************************/
bool nv_img_is_white_file(const s8 * pfile)
{
    u32 index;
    u32 file_num;
   
    file_num = nv_img_get_white_list_num();

    for(index = 0; index < file_num; index++)
    {
        if(0 == strncmp((char *)pfile, (char *)g_nv_path.file_path[index],
                                strlen((char *)(g_nv_path.file_path[index])) + 1))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*****************************************************************************
 函 数 名  : nv_img_check_white_list
 功能描述  : 删除不符合白名单的文件或者目录,出现的异常处理原则如下
             1. 如果申请内存不成功，返回错误，需要定位原因
             2. 如果出现文件目录无法访问(如打开，读取等操作)，返回BSP_ERR_NV_NO_FILE，
                调用者，依据特性处理 支持:擦除当前文件，并复位系统，重新挂载文件系统或者继续执行检查操作
             3. 删除无效文件失败，返回BSP_ERR_NV_NO_FILE，
                调用者，依据特性处理 支持:擦除当前文件，并复位系统，重新挂载文件系统或者继续执行检查操作
 输入参数  : source 白名单所在的跟目录
 输出参数  : 无
 返 回 值  : NV_OK
             NV_ERROR
             BSP_ERR_NV_NO_FILE 失败，需要调用者复位系统
*****************************************************************************/
u32 nv_img_check_white_list(const s8 * source, u32 depth)
{
    s32 ret;
    u32 chk_ret = NV_OK;
    s32 cur_dir = 0;
    s32 sub_index = 0;
    unsigned long src_len = 0;

    s8 * pdir_rent = NULL;
    s8 * pdir_name = NULL;
   
    struct rfile_stat_stru s_stat;
    RFILE_DIRENT_STRU * pdirent;


    /* 获取当前文件信息 */
    ret = bsp_stat((s8 *)source, &s_stat);
    if((ret < 0) || (nv_debug_chk_invalid_type((const s8 *) source, INVALID_FILE_NO_STAT) != 0))
    {
        nv_record("bsp_stat [%s] failed.\n",source);
        s_delet_ret = BSP_ERR_NV_NO_FILE;
        return NV_OK;
    }

    /*如果是目录，递归检查目录下的文件 */
    if(S_ISDIR(s_stat.mode))
    {
        /* 打开当前文件目录 */
        cur_dir = bsp_opendir((s8*)source);
        if((cur_dir < 0) || (nv_debug_chk_invalid_type((const s8 *) source, INVALID_FILE_NO_ODIR) != 0))
        {
            nv_record("bsp_opendir [%s] failed.\n",source);
            s_delet_ret = BSP_ERR_NV_NO_FILE;
            return NV_OK;
        }
       
        /* 缓存子目录的名称 */
        pdir_rent = vmalloc((unsigned long)MAX_DIRENT_LEN);
        if(NULL == pdir_rent)
        {
            nv_printf("[%s] malloc failed.\n", __FUNCTION__);
            bsp_closedir(cur_dir);
            return NV_ERROR;
        }

        /* 读取当前目录下的文件 */
        ret = bsp_readdir((unsigned int)cur_dir, pdir_rent, MAX_DIRENT_LEN);
        if((ret <= 0) || (nv_debug_chk_invalid_type((const s8 *) source, INVALID_FILE_NO_RDIR) != 0))
        {
            nv_record("bsp_readdir [%s] failed.\n",source);
            vfree(pdir_rent);
            bsp_closedir(cur_dir);
            s_delet_ret = BSP_ERR_NV_NO_FILE;
            return NV_OK;
        }
        
        /* 遍历处理当前目录下所有的文件，包括子目录，如果是空目录删除 */
        for(sub_index = 0; sub_index < ret; )
        {
            /*lint -save -e826 -specific(-e826)*/
            pdirent = (RFILE_DIRENT_STRU*)(pdir_rent + sub_index);
            /*lint -restore*/
            sub_index += pdirent->d_reclen;

            if((0 == strncmp((char*)pdirent->d_name, ".", sizeof("."))) 
                    || (0 == strncmp((char*)pdirent->d_name, "..", sizeof(".."))))
            {
                continue;
            }

            /* 分配存储子文件/目录路径的缓存区 */   
            src_len = (unsigned long)(strlen((char*)source)+1+strlen((char*)pdirent->d_name)+1);
    
            pdir_name = vmalloc(src_len);
            if(NULL == pdir_name)
            {
                nv_printf("[%s] malloc src failed.\n", __FUNCTION__);
                vfree(pdir_rent);
                bsp_closedir(cur_dir);
                return NV_ERROR;
            }

            /* 存储文件/目录名 */
            memset((void*)pdir_name, 0, src_len);
            strncpy(pdir_name, source, src_len);
            strncat(pdir_name, "/", src_len);
            strncat(pdir_name, (char*)pdirent->d_name, src_len);
            
            // cppcheck-suppress *
            chk_ret = nv_img_check_white_list(pdir_name, depth+1);
            if(NV_ERROR == chk_ret)
            {
                 vfree(pdir_name); 
                 break;
            }
            vfree(pdir_name);          
        }

        /* 释放分配内存,关闭当前文件目录，删除空目录 */
        vfree(pdir_rent);
        bsp_closedir(cur_dir);

        if(depth){
            bsp_rmdir((s8*)source);
        }
 
    }
    /* 如果是源文件，做对应的处理 */
    else
    {
        /* 如果不是白名单上的文件则删除 */
        if(TRUE != nv_img_is_white_file((const s8 *)source))
        {
            ret = bsp_remove((const s8 *)source);
            if((ret != 0) || (nv_debug_chk_invalid_type((const s8 *) source, INVALID_FILE_NO_REMOVE) != 0))
            {
                nv_record("bsp_remove [%s] failed.\n",source);
                s_delet_ret = BSP_ERR_NV_NO_FILE;
                return NV_OK;
            }
        }
    }

    return chk_ret;
}

/**************************************************************************
 函 数 名  : nv_img_boot_check
 功能描述  : 删除不符合白名单的文件或者目录，外部调用
 输入参数  : pdir 白名单文件跟目录
 输出参数  : 无
 返 回 值  : NV_OK
             NV_ERROR
***************************************************************************/
u32 nv_img_boot_check(const s8 * pdir)
{
    u32 ret;
    u32 index;
    u32 file_num;

    file_num = nv_img_get_white_list_num();
    
    /* 检查文件路径是否超过长度限制 */
    for(index = 0; index < file_num; index++)
    {
        if(strlen((char*)(g_nv_path.file_path[index])) > DRV_NAME_MAX)
        {
            bsp_trace(BSP_LOG_LEVEL_ERROR, BSP_MODU_NV, "[%s] file len error.\n", __FUNCTION__);
            return NV_ERROR;
        }
    }

    /* 依据白名单对处理文件，对异常结果做处理 */
    ret = nv_img_check_white_list(pdir, 0);
    if(ret)
    {
        bsp_trace(BSP_LOG_LEVEL_ERROR, BSP_MODU_NV, "[%s] nv boot check failed.\n", __FUNCTION__);
        return NV_ERROR;
    }

    /* 异常处理复位特性，目前手机不复位, MBB复位 */    

    return OK;
}

u32 nv_img_write(u8* pdata, u32 len, u32 file_offset)
{
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    u32 ret         = NV_ERROR;
    FILE* fp = NULL;

    nv_debug(NV_API_FLUSH,0,file_offset,len,0);
    nv_debug_record(NV_DEBUG_FLUSHEX_START);

    if((file_offset + len) > (ddr_info->file_len))
    {
        nv_debug(NV_API_FLUSH,1,file_offset,len,ddr_info->file_len);
        nv_printf("nv len is to large! 0x%x 0x%x\n", file_offset, len);
        goto nv_flush_err;
    }

    nv_debug_record(NV_DEBUG_FLUSHEX_OPEN_START);

    if(nv_file_access((s8*)g_nv_path.file_path[NV_IMG],0))
    {
        nv_printf("no nv file, create when first write!\n");
        fp = nv_file_open((s8*)g_nv_path.file_path[NV_IMG],(s8*)NV_FILE_WRITE);
        if(NULL == fp)
        {
            nv_printf("create nv file failed!\n");
            nv_debug(NV_API_FLUSH, 2, ret,0,0);
            goto nv_flush_err;
        }
        nv_file_close(fp);

        return nv_img_flush_all();
    }
    else
    {
        fp = nv_file_open((s8*)g_nv_path.file_path[NV_IMG],(s8*)NV_FILE_RW);
        if(NULL == fp)
        {
            nv_printf("open nv file failed!\n");
            ret = BSP_ERR_NV_NO_FILE;
            nv_debug(NV_API_FLUSH,3,ret,0,0);
            goto nv_flush_err;
        }
    }
    nv_debug_record(NV_DEBUG_FLUSHEX_OPEN_END);
    
    (void)nv_file_seek(fp, file_offset ,SEEK_SET);/*jump to write*/
    ret = (u32)nv_file_write((u8*)pdata,1,len,fp);
    nv_file_close(fp);
    if(ret != len)
    {
        nv_printf("write nv file failed!\n");
        nv_debug(NV_API_FLUSH,4, file_offset , ret,len);
        ret = BSP_ERR_NV_WRITE_FILE_FAIL;
        goto nv_flush_err;
    }

    return NV_OK;

nv_flush_err:
    nv_record("\n[%s] len :0x%x, pdata :0x%x offset: 0x%x\n",__FUNCTION__,len,pdata, file_offset);
    nv_help(NV_API_FLUSH);
    return ret;
}


u32 nv_img_flush_all(void)
{
    u32 ret = NV_ERROR;
    FILE* fp;
    nv_global_info_s* ddr_info  = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    u32 writeLen = 0;

    nv_create_flag_file((s8*)NV_IMG_FLAG_PATH);
    nv_debug(NV_API_FLUSH,0,0,0,0);
    
    fp = nv_file_open((s8*)g_nv_path.file_path[NV_IMG],(s8*)NV_FILE_WRITE);
    if(NULL == fp)
    {
        ret = BSP_ERR_NV_NO_FILE;
        nv_debug(NV_API_FLUSH,1,ret,0,0);
        goto nv_flush_err;
    }

    writeLen = ddr_info->file_len;
    ret = (u32)nv_file_write((u8*)NV_GLOBAL_CTRL_INFO_ADDR,1, writeLen, fp);
    nv_file_close(fp);
    fp = NULL;
    if(ret != writeLen)
    {
        nv_debug(NV_API_FLUSH,5,(u32)(unsigned long)fp,ret,writeLen);
        ret = BSP_ERR_NV_WRITE_FILE_FAIL;
        goto nv_flush_err;
    }
    nv_delete_flag_file((s8*)NV_IMG_FLAG_PATH);

    return NV_OK;

nv_flush_err:
    nv_record("\n[%s]\n",__FUNCTION__);
    nv_help(NV_API_FLUSH);
    return ret;
}

/*****************************************************************************
 函 数 名  : nv_resume_item_from_img
 功能描述  : 从工作恢复一个NV项
 输入参数  : void
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
u32 nv_img_resume_item(nv_item_info_s *item_info, u32 modem_id)
{
    u32 ret;
    u8* item_base;
    u8* mdm_base;
    u32 item_offset;
    u32 mdm_offset;
    u32 item_len;
    u32 mdm_len;
    u8* resume_data = NULL;
    u32 resume_size = 0;
    u32 file_offset = 0;
    u8* temp_buff;
    u32 buff_size   = 0;
    bool need_crc   = 0;
    u32 crc_code    = 0;

    item_base   = nv_get_item_base(item_info);
    mdm_base    = nv_get_item_mdmbase(item_info, modem_id, NULL);
    item_offset = nv_get_item_fileoffset(item_info, NULL);
    mdm_offset  = nv_get_item_filemdmoffset(item_info, modem_id, NULL, NULL);
    item_len    = nv_get_item_len(item_info, NULL);
    mdm_len     = nv_get_item_mdmlen(item_info, NULL);

    /* nvid crc */
    if(NV_ITEM_CRC_CHECK_YES)
    {
        resume_data     = item_base;
        resume_size     = item_len;
        file_offset     = item_offset;
        buff_size       = resume_size;
        need_crc        = true;
    }
    /* modemid crc */
    else if(NV_MODEM_CRC_CHECK_YES)
    {
        resume_data     = mdm_base;
        resume_size     = mdm_len;
        file_offset     = mdm_offset;
        buff_size       = resume_size;
        need_crc        = true;
    }
    else
    {
        resume_data     = mdm_base;
        resume_size     = mdm_len;
        file_offset     = mdm_offset;
        buff_size       = resume_size;
        need_crc        = false;
    }

    if(true != nv_check_file_validity((s8 *)g_nv_path.file_path[NV_IMG], (s8 *)NV_IMG_FLAG_PATH))
    {
        nv_debug(NV_FUN_RESUME_IMG_ITEM,1,0,0,0);
        nv_record("resume nvid, img file is invalid itemid=0x%x!\n", item_info->itemid);
        return BSP_ERR_NV_READ_FILE_FAIL;
    }

    temp_buff = (u8*)vmalloc((size_t)buff_size);
    if(NULL == temp_buff)
    {
        nv_debug(NV_FUN_RESUME_IMG_ITEM,2,0,0,0);
        nv_record("resume nvid vmalloc fail 0x%x ...%s %s \n", item_info->itemid, __DATE__,__TIME__);
        return BSP_ERR_NV_MALLOC_FAIL;
    }
    
    ret = nv_read_file(g_nv_path.file_path[NV_IMG], file_offset, temp_buff, &buff_size);
    if(ret)
    {
        vfree(temp_buff);
        nv_debug(NV_FUN_RESUME_IMG_ITEM,3,ret,0,0);
        nv_record("resume nvid read file fail 0x%x 0x%x ...%s %s \n", ret, item_info->itemid, __DATE__,__TIME__);
        return ret;
    }

    if(need_crc)
    {
        crc_code = *(u32*)((unsigned long)temp_buff+buff_size-4);
        if(nv_crc_check(temp_buff, (buff_size-4), crc_code))
        {
            vfree(temp_buff);
            nv_debug(NV_FUN_RESUME_IMG_ITEM,4,ret,0,0);
            nv_record("resume nvid crc check fail 0x%x 0x%x ...%s %s \n", ret, item_info->itemid, __DATE__,__TIME__);
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

EXPORT_SYMBOL(nv_img_write);
EXPORT_SYMBOL(nv_img_flush_all);
EXPORT_SYMBOL(nv_img_resume_item);
EXPORT_SYMBOL(nv_img_boot_check);
EXPORT_SYMBOL(nv_img_clear_check_result);




