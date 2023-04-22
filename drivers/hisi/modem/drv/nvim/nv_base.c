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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/printk.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <product_config.h>
#include <mdrv_rfile_common.h>
#include <osl_thread.h>
#include <bsp_nvim.h>
#include "nv_comm.h"
#include "nv_file.h"
#include "nv_ctrl.h"
#include "bsp_blk.h"
#include "nv_xml_dec.h"
#include "nv_debug.h"
#include "nv_index.h"
#include "nv_crc.h"
#include "nv_partition_img.h"
#include "nv_partition_bakup.h"
#include "nv_partition_upgrade.h"
#include "NVIM_ResumeId.h"
#include "bsp_dump.h"
#include "nv_msg.h"
#include "nv_proc.h"
#include "nv_cust.h"


struct nv_path_info_stru g_nv_path = {0};
u32 nv_readEx(u32 modem_id,u32 itemid,u32 offset,u8* pdata,u32 datalen)
{
    u32 ret;
    nv_file_info_s file_info = {0};
    nv_item_info_s item_info = {0};
    nv_rd_req      rreq;
    nv_ctrl_info_s* ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;

    nv_debug(NV_FUN_READ_EX,0,itemid,modem_id,datalen);

    if((NULL == pdata)||(0 == datalen))
    {
        nv_debug(NV_FUN_READ_EX,1,itemid,0,0);
        return BSP_ERR_NV_INVALID_PARAM;
    }

    ret = nv_search_byid(itemid,((u8*)NV_GLOBAL_CTRL_INFO_ADDR),&item_info,&file_info);
    if(ret)
    {
        nv_printf("can not find 0x%x !\n",itemid);
        return BSP_ERR_NV_NO_THIS_ID;
    }

    if((offset + datalen) > item_info.nv_len)
    {
        ret = BSP_ERR_NV_ITEM_LEN_ERR;
        nv_debug(NV_FUN_READ_EX,3,offset,datalen,item_info.nv_len);
        goto nv_readEx_err;
    }

    if((modem_id == 0) || (modem_id > ctrl_info->modem_num))
    {
        ret = BSP_ERR_NV_INVALID_MDMID_ERR;
        nv_debug(NV_FUN_READ_EX,4,ret,itemid,modem_id);
        goto nv_readEx_err;
    }

    if(modem_id > item_info.modem_num)
    {
        modem_id = 1;
    }

    rreq.itemid  = itemid;
    rreq.modemid = modem_id;
    rreq.offset  = offset;
    rreq.pdata   = pdata;
    rreq.size    = (datalen < item_info.nv_len) ? datalen : item_info.nv_len;
    (void)nv_read_from_mem(&rreq, &item_info);

    nv_debug_trace(pdata, datalen);

    return NV_OK;

nv_readEx_err:
    nv_record("\n[%s]:[0x%x]:[%d] 0x%x\n",__FUNCTION__,itemid,modem_id, ret);
    nv_help(NV_FUN_READ_EX);
    return ret;
}

u32 nv_writeEx(u32 modem_id, u32 itemid, u32 offset, u8* pdata, u32 datalen)
{
    u32 ret;
    nv_file_info_s  file_info = {0};
    nv_item_info_s  item_info = {0};
    nv_wr_req       wreq;
    u8  test_byte;
    nv_ctrl_info_s* ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;

    nv_debug(NV_FUN_WRITE_EX,0,itemid,modem_id,datalen);
    nv_debug_record(NV_DEBUG_WRITEEX_START|itemid<<16);

    if((NULL == pdata)||(0 == datalen))
    {
        nv_debug(NV_FUN_WRITE_EX,1,itemid,datalen,0);
        return BSP_ERR_NV_INVALID_PARAM;
    }

    /* test pdata is accessable */
    test_byte = *pdata;
    UNUSED(test_byte);

    ret = nv_search_byid(itemid,((u8*)NV_GLOBAL_CTRL_INFO_ADDR),&item_info,&file_info);
    if(ret)
    {
        nv_printf("can not find 0x%x !\n",itemid);
        nv_debug(NV_FUN_WRITE_EX,2,itemid,modem_id,offset);
        return BSP_ERR_NV_NO_THIS_ID;
    }

    nv_debug_trace(pdata, datalen);

    if((datalen + offset) >item_info.nv_len)
    {
        ret = BSP_ERR_NV_ITEM_LEN_ERR;
        nv_debug(NV_FUN_WRITE_EX,3,itemid,datalen,item_info.nv_len);
        goto nv_writeEx_err;
    }

    if((modem_id == 0) || (modem_id > ctrl_info->modem_num))
    {
        ret = BSP_ERR_NV_INVALID_MDMID_ERR;
        nv_debug(NV_FUN_WRITE_EX,4,itemid,datalen,item_info.nv_len);
        goto nv_writeEx_err;
    }

    if(modem_id > item_info.modem_num)
    {
        modem_id = 1;
    }

    /* check crc before write */
    if(nv_crc_need_check_inwr(&item_info, datalen))
    {
        ret = nv_crc_check_item(&item_info, modem_id);
        if(ret)
        {
            nv_debug(NV_FUN_WRITE_EX, 6, itemid,datalen,ret);
            ret = nv_resume_item(&item_info, itemid, modem_id);
            if(ret)
            {
                nv_debug(NV_FUN_WRITE_EX,7, itemid, modem_id, ret);
                goto nv_writeEx_err;
            }
        }
    }

    wreq.itemid    = itemid;
    wreq.modemid   = modem_id;
    wreq.offset    = offset;
    wreq.pdata     = pdata;
    wreq.size      = datalen;
    nv_debug_record(NV_DEBUG_WRITEEX_MEM_START);
    ret = nv_write_to_mem(&wreq, &item_info);
    if(ret)
    {
        nv_debug(NV_FUN_WRITE_EX,8,itemid,datalen,0);
        goto nv_writeEx_err;
    }

    nv_debug_record(NV_DEBUG_WRITEEX_FILE_START);
    ret = nv_write_to_file(&wreq, &item_info);
    if(ret)
    {
        nv_debug(NV_FUN_WRITE_EX,9,itemid,datalen,ret);
        goto nv_writeEx_err;
    }
    nv_debug_record(NV_DEBUG_WRITEEX_END|itemid<<16);

    return NV_OK;

nv_writeEx_err:
    nv_record("\n[%s]:[0x%x]:[%d]\n",__FUNCTION__,itemid,modem_id);
    nv_help(NV_FUN_WRITE_EX);
    return ret;
}



u32 bsp_nvm_get_nv_num(void)
{
    nv_ctrl_info_s* ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    return ctrl_info->ref_count;
}


u32 bsp_nvm_get_nvidlist(NV_LIST_INFO_STRU*  nvlist)
{
    u32 i;
    nv_ctrl_info_s* ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    nv_item_info_s* ref_info   = (nv_item_info_s*)((unsigned long)NV_GLOBAL_CTRL_INFO_ADDR+NV_GLOBAL_CTRL_INFO_SIZE\
                                                                    +NV_GLOBAL_FILE_ELEMENT_SIZE*ctrl_info->file_num);

    if(NULL == nvlist)
    {
        return NV_ERROR;
    }

    for(i = 0;i<ctrl_info->ref_count;i++)
    {
        nvlist[i].usNvId       = ref_info[i].itemid;
        nvlist[i].ucNvModemNum = ref_info[i].modem_num;
    }
    return NV_OK;
}


u32 bsp_nvm_getRevertList(u32 enNvItem, u8 *pusNvList, u32 ulNvNum)
{
    u8 *pData = NULL;

    if(NULL == pusNvList)
    {
        return NV_ERROR;
    }
    
    if (ulNvNum != (u32)bsp_nvm_getRevertNum((unsigned long)enNvItem))
    {
        return NV_ERROR;
    }

    switch(enNvItem)
    {
        case NV_MANUFACTURE_ITEM:
            pData = (u8 *)g_ausNvResumeManufactureIdList;
            break;
        case NV_USER_ITEM:
            pData = (u8 *)g_ausNvResumeUserIdList;
            break;
        case NV_SECURE_ITEM:
            pData = (u8 *)g_ausNvResumeSecureIdList;
            break;
        default:
            return NV_ERROR;
    }
    
    memcpy(pusNvList, pData, ulNvNum*sizeof(u16));

    return NV_OK;
}


u32 bsp_nvm_get_len(u32 itemid,u32* len)
{
    u32 ret;
    nv_item_info_s ref_info = {0};
    nv_file_info_s file_info = {0};

    nv_debug(NV_API_GETLEN,0,itemid,0,0);
    if(NULL == len)
    {
        nv_debug(NV_API_GETLEN,1,itemid,0,0);
        return BSP_ERR_NV_INVALID_PARAM;
    }

    /*check init state*/
    if(false == nv_read_right())
    {
        nv_debug(NV_API_GETLEN,3,itemid,0,0);
        return BSP_ERR_NV_MEM_INIT_FAIL;
    }
    ret = nv_search_byid(itemid,(u8*)NV_GLOBAL_CTRL_INFO_ADDR,&ref_info, &file_info);
    if(NV_OK == ret)
    {
        *len = ref_info.nv_len;
        return NV_OK;
    }
    return ret;
}

u32 bsp_nvm_authgetlen(u32 itemid,u32* len)
{
    return bsp_nvm_get_len(itemid,len);
}



u32 bsp_nvm_dcread_direct(u32 modem_id,u32 itemid, u8* pdata,u32 datalen)
{
    return nv_readEx(modem_id,itemid,0,pdata,datalen);
}

u32 bsp_nvm_dcread(u32 modem_id,u32 itemid, u8* pdata,u32 datalen)
{
    /*check init state*/
    if(false == nv_read_right())
    {
        return BSP_ERR_NV_MEM_INIT_FAIL;
    }

    return nv_readEx(modem_id,itemid,0,pdata,datalen);
}

u32 bsp_nvm_auth_dcread(u32 modem_id,u32 itemid, u8* pdata,u32 datalen)
{
    return bsp_nvm_dcread(modem_id,itemid,pdata,datalen);
}

u32 bsp_nvm_dcreadpart(u32 modem_id,u32 itemid,u32 offset,u8* pdata,u32 datalen)
{
    /*check init state*/
    if(false == nv_read_right())
    {
        return BSP_ERR_NV_MEM_INIT_FAIL;
    }

    return nv_readEx(modem_id,itemid,offset,pdata,datalen);
}

u32 bsp_nvm_dcwrite(u32 modem_id,u32 itemid, u8* pdata,u32 datalen)
{
    if(false == nv_write_right())
    {
        return BSP_ERR_NV_MEM_INIT_FAIL;
    }

    return nv_writeEx(modem_id,itemid,0,pdata,datalen);
}

u32 bsp_nvm_auth_dcwrite(u32 modem_id,u32 itemid, u8* pdata,u32 datalen)
{
    return bsp_nvm_dcwrite(modem_id,itemid,pdata,datalen);
}

u32 bsp_nvm_dcwritepart(u32 modem_id,u32 itemid, u32 offset,u8* pdata,u32 datalen)
{
    if(false == nv_write_right())
    {
        return BSP_ERR_NV_MEM_INIT_FAIL;
    }

    return nv_writeEx(modem_id,itemid,offset,pdata,datalen);
}

u32 bsp_nvm_dcwrite_direct(u32 modem_id,u32 itemid, u8* pdata,u32 datalen)
{
    return nv_writeEx(modem_id,itemid,0,pdata,datalen);
}



u32 bsp_nvm_flush(void)
{
    u32 ret;
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    ret = nv_flush_wrbuf(ddr_info);
    if (ret) {
        nv_printf("Fail to flush low priority write buffer \n");
    }

    ret = nv_send_msg_sync(NV_TASK_MSG_FLUSH, 0, 0);
    return ret;
}


u32 bsp_nvm_flushSys(void)
{
    u32 ret = NV_ERROR;
    FILE* fp = NULL;
    u32 ulTotalLen = 0;
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    nv_create_flag_file((s8*)NV_SYS_FLAG_PATH);

    nv_debug(NV_FUN_FLUSH_SYS,0,0,0,0);
    if(nv_file_access((s8*)NV_FILE_SYS_NV_PATH,0))
    {
        fp = nv_file_open((s8*)NV_FILE_SYS_NV_PATH,(s8*)NV_FILE_WRITE);
    }
    else
    {
        fp = nv_file_open((s8*)NV_FILE_SYS_NV_PATH,(s8*)NV_FILE_RW);
    }
    if(NULL == fp)
    {
        nv_debug(NV_FUN_FLUSH_SYS,1,ret,0,0);
        ret = BSP_ERR_NV_NO_FILE;
        goto nv_flush_err;
    }
    ulTotalLen = ddr_info->file_len;
    /*在nvdload分区文件末尾置标志0xabcd8765*/
    *( unsigned int* )( NV_GLOBAL_CTRL_INFO_ADDR + ddr_info->file_len )
        = ( unsigned int )NV_FILE_TAIL_MAGIC_NUM;
    ulTotalLen += sizeof(unsigned int);
    /*系统分区数据不做CRC校验，因此回写时不考虑CRC校验码的存放位置*/
    ret = (u32)nv_file_write((u8*)NV_GLOBAL_CTRL_INFO_ADDR,1,ulTotalLen,fp);
    nv_file_close(fp);
    if(ret != ulTotalLen)
    {
        nv_debug(NV_FUN_FLUSH_SYS,3,ret,ulTotalLen,0);
        ret = BSP_ERR_NV_WRITE_FILE_FAIL;
        goto nv_flush_err;
    }

    nv_delete_flag_file((s8*)NV_SYS_FLAG_PATH);
    return NV_OK;

nv_flush_err:
    nv_record("\n[%s]\n",__func__);
    nv_help(NV_FUN_FLUSH_SYS);
    return ret;
}



u32 bsp_nvm_backup(u32 crc_flag)
{
    u32 ret = NV_ERROR;
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    FILE* fp = NULL;
    u32 writeLen = 0;

    nv_debug(NV_API_BACKUP,0,0,0,0);
    nv_record("Backup: nvm backup start!\n");

    if( (ddr_info->acore_init_state != NV_INIT_OK)&&
        (ddr_info->acore_init_state != NV_KERNEL_INIT_DOING))
    {
        nv_record("Backup: acore or ccore init err!a_init=%d, c_init=%d.\n",ddr_info->acore_init_state,ddr_info->acore_init_state);
        return NV_ERROR;
    }

    nv_create_flag_file((s8*)NV_BACK_FLAG_PATH);

    if(nv_file_access((s8*)NV_BACK_PATH,0))
    {
        fp = nv_file_open((s8*)NV_BACK_PATH,(s8*)NV_FILE_WRITE);
    }
    else
    {
        fp = nv_file_open((s8*)NV_BACK_PATH,(s8*)NV_FILE_RW);
    }
    if(NULL == fp)
    {
        ret = BSP_ERR_NV_NO_FILE;
        nv_debug(NV_API_BACKUP,1,ret,0,0);
        nv_record("Backup: open %s fail!\n",NV_BACK_PATH);
        goto nv_backup_fail;
    }

    writeLen = ddr_info->file_len;

    if(NV_FLAG_NEED_CRC == crc_flag)
    {
        ret = nv_crc_check_ddr(NV_RESUME_NO);
        if(ret)
        {
            nv_debug(NV_API_BACKUP,2,ret,0, 0);
            nv_record("Backup: crc_flag is invalid!\n");
            (void)nv_debug_store_ddr_data();
            goto nv_backup_fail;
        }
    }

    /* 如果需要进行CRC校验, 备份数据到备份区，内存中的数据较备份区更新，所以crc check不能带自动恢复,
       要保证写入备份区的数据，crc校验正确，同时备份过程中内存数据不被改写，所以需要锁住内存 */
    nv_ipc_sem_take(IPC_SEM_NV_CRC, IPC_SME_TIME_OUT);
    ret = (u32)nv_file_write((u8*)NV_GLOBAL_CTRL_INFO_ADDR,1,writeLen,fp);
    nv_ipc_sem_give(IPC_SEM_NV_CRC);
    nv_file_close(fp);
    fp = NULL;
    if(ret != writeLen)
    {
        nv_debug(NV_API_BACKUP,3,ret,writeLen,0);
        ret = BSP_ERR_NV_WRITE_FILE_FAIL;
        nv_record("Backup: write fail!write_len=0x%x,ret_len=0x%x.\n",writeLen,ret);
        goto nv_backup_fail;
    }

    (void)nv_bakup_info_reset();

    if(nv_file_update(NV_BACK_PATH))
    {
        nv_debug(NV_API_BACKUP, 4 , 0, 0, 0);
    }

    nv_delete_flag_file((s8*)NV_BACK_FLAG_PATH);

    nv_record("Backup: nvm backup end!\n");

    return NV_OK;
nv_backup_fail:
    if(fp){nv_file_close(fp);}
    nv_record("\n[%s]\n",__FUNCTION__);
    nv_help(NV_API_BACKUP);
    return ret;

}

/* added by yangzhi for muti-carrier, Begin:*/
/* added by yangzhi for muti-carrier, End! */


u32 bsp_nvm_update_default(void)
{
    u32 ret;
    FILE* fp = NULL;
    u32 datalen = 0;
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    nv_record("^INFORBU: factory bakup start!\n");

    if(ddr_info->acore_init_state != NV_INIT_OK)
    {
        nv_record("^INFORBU: factory bakup failed 1, nv is not init! %d\n", ddr_info->acore_init_state);
        return NV_ERROR;
    }
    
    ret = nv_set_resume_mode(NV_MODE_USER);
    if(ret)
    {
        nv_record("^INFORBU: factory bakup failed 3, set resume mode failed! 0x%x\n", ret);
        return NV_ERROR;
    }
    
    /*在写入文件前进行CRC校验，以防数据不正确*/
    ret = nv_crc_check_ddr(NV_RESUME_NO);
    if(ret)
    {
        nv_record("^INFORBU: factory bakup failed 4, ddr check failed! 0x%x\n", ret);
        (void)nv_debug_store_ddr_data();
        goto nv_update_default_err;
    }

    if(nv_file_access((s8*)NV_DEFAULT_PATH,0))
    {
        fp = nv_file_open((s8*)NV_DEFAULT_PATH,(s8*)NV_FILE_WRITE);
    }
    else
    {
        fp = nv_file_open((s8*)NV_DEFAULT_PATH,(s8*)NV_FILE_RW);
    }
    if(NULL == fp)
    {
        ret = BSP_ERR_NV_NO_FILE;
        nv_record("^INFORBU: factory bakup failed 5, open factory file failed! 0x%x\n", ret);
        goto nv_update_default_err;
    }

    /* 锁住NV内存，在写入文件前进行CRC校验，以防数据不正确，
       同时要保证当前的crc check不做自动恢复，带自动恢复的crc check会在恢复过程中获取ipc semaphore，导致死锁*/
    ret = nv_ipc_sem_take(IPC_SEM_NV_CRC, IPC_SME_TIME_OUT);
    if(ret)
    {
        nv_record("^INFORBU: factory bakup failed 6, ipc sem take failed! 0x%x\n", ret);
        goto nv_update_default_err;
    }
    datalen = (u32)nv_file_write((u8*)NV_GLOBAL_CTRL_INFO_ADDR,1,ddr_info->file_len,fp);
    nv_ipc_sem_give(IPC_SEM_NV_CRC);

    nv_file_close(fp);
    if(datalen != ddr_info->file_len)
    {
        ret = BSP_ERR_NV_WRITE_FILE_FAIL;
        nv_record("^INFORBU: factory bakup failed 7, write factory file failed! 0x%x 0x%x\n", datalen, ddr_info->file_len);
        goto nv_update_default_err;
    }

    ret = bsp_nvm_backup(NV_FLAG_NO_CRC);
    if(ret)
    {
        nv_record("^INFORBU: factory bakup failed 8, backup failed! 0x%x\n", ret);
        goto nv_update_default_err;
    }

    nv_record("^INFORBU: factory bakup end!\n");

    return NV_OK;
    
nv_update_default_err:
    /* coverity[deref_arg] */
    if(fp){nv_file_close(fp);}
    nv_record("\n[%s]\n",__FUNCTION__);
    nv_help(NV_FUN_UPDATE_DEFAULT);
    return ret;
}


u32 bsp_nvm_revert_default(void)
{
    u32 ret;

    /* 先把出厂区所有NV项恢复回来 */
    ret = nv_revert_data((s8*)NV_DEFAULT_PATH, NULL, 0, NV_FLAG_NEED_CRC);
    if(ret)
    {
        nv_record("revert from default partition failed, ret=0x%x\n!", ret);
        return ret;
    }

    /*机要nv项不恢复,重新恢复成img分区里的数据，需要重新生成CRC校验码*/
    ret = nv_revert_data(g_nv_path.file_path[NV_IMG],g_ausNvResumeSecureIdList,\
        (u32)bsp_nvm_getRevertNum((unsigned long)NV_SECURE_ITEM), NV_FLAG_NEED_CRC);
    if(ret)
    {
        nv_record("revert keeping secure list failed, ret=0x%x\n!", ret);
        return ret;
    }

    ret = nv_img_flush_all();
    ret |= bsp_nvm_flushSys();

    nv_record("bsp_nvm_revert_default success!\n!");

    return ret;
}

u32 nv_write2file_handle(nv_cmd_req *msg)
{
    u32 ret;
    nv_item_info_t *nv_info;
    nv_flush_item_s flush_item;

    nv_info = &msg->nv_item_info;
    flush_item.itemid = nv_info->itemid;
    flush_item.modemid = nv_info->modem_id;
    ret = nv_flushItem(&flush_item);
    if (ret)
    {
        return ret;
    }

    if (true == nv_isSysNv(nv_info->itemid))
    {
        ret = bsp_nvm_flushSys();
    }

    return ret;
}

void bsp_nvm_icc_task(void* parm)
{
    u32 ret = NV_ERROR;
    nv_cmd_req *msg;
    nv_item_info_t *nv_info;
	
    /* coverity[self_assign] */
    parm = parm;

    /* coverity[no_escape] */
    for(;;)
    {
        osl_sem_down(&g_nv_ctrl.task_sem);

        g_nv_ctrl.opState = NV_OPS_STATE;

        /*如果当前处于睡眠状态，则等待唤醒处理*/
        if(g_nv_ctrl.pmState == NV_SLEEP_STATE)
        {
            printk("%s cur state in sleeping,wait for resume end!\n",__func__);
            osl_sem_down(&g_nv_ctrl.suspend_sem);
        }

        msg = nv_get_cmd_req();
        if (msg == NULL) {
            g_nv_ctrl.opState = NV_IDLE_STATE;
            continue;
        }

        nv_debug_printf("msg type:0x%x\n", msg->msg_type);
        nv_info = &msg->nv_item_info;
        switch (msg->msg_type) {
            case NV_TASK_MSG_WRITE2FILE:
                ret = nv_write2file_handle(msg);
                break;

            case NV_TASK_MSG_FLUSH:
                /* there is no actual nv operation, return NV_OK and notify
                           * NV writing process the result actually
                           */
                 ret = NV_OK;
                break;

            case NV_TASK_MSG_RESUEM:
                ret = nv_resume_ddr_from_img();
                break;

            case NV_TASK_MSG_RESUEM_ITEM:
                ret = nv_resume_item(NULL, nv_info->itemid, nv_info->modem_id);
                break;

            default:
                nv_printf("msg type invalid, msg type;0x%x\n", msg->msg_type);
                break;
        }

        nv_debug_printf("deal msg ok\n");
        if (ret) {
            nv_record("flush nv to file fail, msg type:0x%x errno:0x%x\n", msg->msg_type, ret);
        }

        if (msg->nv_msg_callback) {
            msg->nv_msg_callback(ret, msg->sn);
        }

        nv_put_cmd_req(msg);
        g_nv_ctrl.task_proc_count++;
        g_nv_ctrl.opState = NV_IDLE_STATE;
    }
}


u32 bsp_nvm_xml_decode(void)
{
	return nv_upgrade_dec_xml_all();
}


u32 bsp_nvm_resume_bakup(void)
{
    u32 ret = NV_ERROR;

    if(true == nv_check_file_validity((s8 *)NV_BACK_PATH, (s8 *)NV_BACK_FLAG_PATH))
    {
        nv_record("load from %s\n",NV_BACK_PATH);

        ret = nv_read_ctrl_from_file(NV_BACK_PATH);
        if(ret)
        {
            nv_debug(NV_FUN_MEM_INIT,6,0,0,0);
            goto load_err_proc;
        }

        ret = nv_crc_check_ddr(NV_RESUME_NO);
        if(ret)
        {
            nv_debug(NV_FUN_MEM_INIT,8,ret,0,0);
            goto load_err_proc;
        }

        /*从备份区加载需要首先写入工作区*/
        ret = nv_img_flush_all();
        if(ret)
        {
            nv_debug(NV_FUN_MEM_INIT,9,0,0,0);
            goto load_err_proc;
        }

        return NV_OK;
    }

load_err_proc:
    ret = nv_load_err_proc();
    if(ret)
    {
        nv_record("%s %d ,err revert proc ,ret :0x%x\n",__func__,__LINE__,ret);
        nv_help(NV_FUN_MEM_INIT);
    }

    return ret;
}



u32 bsp_nvm_reload(void)
{
    u32 ret = NV_ERROR;

    /*工作分区数据存在，且无未写入完成的标志文件*/
    if( true == nv_check_file_validity((s8 *)g_nv_path.file_path[NV_IMG], (s8 *)NV_IMG_FLAG_PATH))
    {
        nv_record("load from %s current slice:0x%x\n",g_nv_path.file_path[NV_IMG], bsp_get_slice_value());

        ret = nv_read_ctrl_from_file((s8*)g_nv_path.file_path[NV_IMG]);
        if(ret)
        {
            nv_record("[%s] read %s fail, ret = 0x%x\n", __FUNCTION__, g_nv_path.file_path[NV_IMG], ret);
            goto load_bak;
        }

        /*reload在初始化过程中，需要从流程上保证此时不会有写NV的操作，从而做crc check的时候内存不会被改写
          带自动恢复的crc check，会在恢复过程中获取crc ipc semaphore，所以不能锁住内存做crc check*/
        ret = nv_crc_check_ddr(NV_RESUME_BAKUP);
        if(BSP_ERR_NV_CRC_RESUME_SUCC == ret)
        {
            ret = nv_img_flush_all();
            if(ret)
            {
                nv_record("nv resume write back failed 0x%x\n", ret);
                return ret;
            }
            nv_printf("img check crc error, but resume success, write back to img!\n");
        }
        else if(ret)
        {
            nv_record("nv image check crc failed %d...current slice:0x%x\n", ret, bsp_get_slice_value());

            /* 保存错误镜像，然后从bakup分区恢复 */
            (void)nv_debug_store_file(g_nv_path.file_path[NV_IMG]);
            if(nv_debug_is_resume_bakup())
            {
                ret = bsp_nvm_resume_bakup();
                if(ret)
                {
                    nv_record("nv resume bakup failed %d...current slice:0x%s \n", ret, bsp_get_slice_value());
                }
            }
            else
            {
                nv_record("config don't resume bakup...slice:0x%x \n",bsp_get_slice_value());
            }

            /* 复位系统 */
            if(nv_debug_is_reset())
            {
                system_error(DRV_ERRNO_NV_CRC_ERR, NV_FUN_MEM_INIT, 3, NULL, 0);
            }
        }

        return ret;
    }

load_bak:

    return bsp_nvm_resume_bakup();
}
/*****************************************************************************
 函 数 名  : bsp_nvm_write_buf_init
 功能描述  : 初始化写入NV时使用的buf和信号量
 输入参数  :
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
u32 bsp_nvm_buf_init(void)
{
    /*create sem*/
    osl_sem_init(1,&g_nv_ctrl.nv_list_sem);
    INIT_LIST_HEAD(&g_nv_ctrl.nv_list);

    return NV_OK;
}

/*****************************************************************************
 函 数 名  : bsp_nvm_kernel_dir_init
 功能描述  : NV模块初始化目录
 输入参数  :
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
u32 bsp_nvm_kernel_dir_init(void)
{
	s8 sc_file_list[14][50] = 
	{
	    "/SC/Pers/CKFile.bin",
	    "/SC/Pers/DKFile.bin",
	    "/SC/Pers/AKFile.bin",
	    "/SC/Pers/PIFile.bin",
	    "/SC/Pers/ImeiFile_I0.bin",
	    "/SC/Pers/ImeiFile_I1.bin",
	    "/SC/Pers/ImeiFile_I2.bin",
	    "/SC/Pers/CKSign.hash",
	    "/SC/Pers/DKSign.hash",
	    "/SC/Pers/AKSign.hash",
	    "/SC/Pers/PISign.hash",
	    "/SC/Pers/ImeiFile_I0.hash",
	    "/SC/Pers/ImeiFile_I1.hash",
	    "/SC/Pers/ImeiFile_I2.hash"
	};
    u32 i = 0;
    if(!bsp_access((s8*)NV_DATA_ROOT_PATH,0))
    {
        g_nv_path.root_dir = NV_DATA_ROOT_PATH;
    }
    else if (!bsp_access((s8*)NV_ROOT_PATH,0))
    {
        g_nv_path.root_dir = NV_ROOT_PATH;
    }
    else
    {
        nv_record("can not find nv dir!\n");
        return NV_ERROR;
    }
    strncat(g_nv_path.file_path[NV_IMG], g_nv_path.root_dir, strlen(g_nv_path.root_dir));
    strncat(g_nv_path.file_path[NV_IMG], NV_IMG_PATH, strlen(NV_IMG_PATH));
    
    for (i = NV_SC_CK_FILE; i < NV_MAX; i++)
    {
        strncat(g_nv_path.file_path[i], g_nv_path.root_dir, strlen(g_nv_path.root_dir));
        strncat(g_nv_path.file_path[i], 
	             sc_file_list[i - NV_SC_CK_FILE], strlen(sc_file_list[i - NV_SC_CK_FILE]));
    }

    return NV_OK;
}
/*****************************************************************************
 函 数 名  : bsp_nvm_core_init
 功能描述  : NV升级或者加载
 输入参数  :
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
u32 bsp_nvm_core_init(u32 modem)
{
    u32 ret;
    if((modem == NV_MEM_DLOAD)  && (true == nv_upgrade_get_flag()))
    {
        ret = bsp_nvm_upgrade();
        if(ret)
        {
            nv_record("upgrade faided! 0x%x\n", ret);
            return ret;
        }
        else
        {
            nv_record("upgrade success!\n");
        }

        /*读取NV自管理配置*/
        ret = bsp_nvm_read(NV_ID_DRV_SELF_CTRL,(u8*)(&(g_nv_ctrl.nv_self_ctrl)),(u32)sizeof(NV_SELF_CTRL_STRU));
        if(ret)
        {
            g_nv_ctrl.nv_self_ctrl.ulResumeMode = NV_MODE_USER;
            nv_printf("read 0x%x fail,use default value! ret :0x%x\n",NV_ID_DRV_SELF_CTRL,ret);
        }
    }
    else
    {
        /*读取NV自管理配置*/
        ret = bsp_nvm_read(NV_ID_DRV_SELF_CTRL,(u8*)(&(g_nv_ctrl.nv_self_ctrl)), (u32)sizeof(NV_SELF_CTRL_STRU));
        if(ret)
        {
            g_nv_ctrl.nv_self_ctrl.ulResumeMode = NV_MODE_USER;
            nv_printf("read 0x%x fail,use default value! ret :0x%x\n",NV_ID_DRV_SELF_CTRL,ret);
        }

        /*重新加载最新数据*/
        ret = bsp_nvm_reload();
        if(ret)
        {
            nv_record("reload fail!ret=0x%x.\n",ret);
            return ret;
        }

        nv_printf("reload success!\n");
    }

    return NV_OK;
}


s32 bsp_nvm_kernel_init(void)
{
    u32 ret;
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    nv_debug(NV_FUN_KERNEL_INIT,0,0,0,0);

    /*sem & lock init*/
    spin_lock_init(&g_nv_ctrl.spinlock);
    osl_sem_init(0,&g_nv_ctrl.task_sem);
    osl_sem_init(0,&g_nv_ctrl.suspend_sem);
    osl_sem_init(1,&g_nv_ctrl.rw_sem);
    osl_sem_init(0,&g_nv_ctrl.cc_sem);
    wake_lock_init(&g_nv_ctrl.wake_lock,WAKE_LOCK_SUSPEND,"nv_wakelock");
    g_nv_ctrl.shared_addr = (nv_global_info_s *)NV_GLOBAL_INFO_ADDR;

    nv_record("Balong nv init  start! %s %s\n",__DATE__,__TIME__);

    (void)nv_debug_init();

    /* nv root dir init */
    ret = (u32)bsp_nvm_kernel_dir_init();
    if(ret)
    {
        nv_record("nv root dir init faided!\n");
        nv_debug(NV_FUN_KERNEL_INIT,11,ret,0,0);
        goto out;
    }
    /* check nv file */
    ret = (u32)nv_img_boot_check(g_nv_path.root_dir);
    if(ret)
    {
        nv_debug(NV_FUN_KERNEL_INIT,1,ret,0,0);
        goto out;
    }

    /*file info init*/
    ret = nv_file_init();
    if(ret)
    {
        nv_debug(NV_FUN_KERNEL_INIT,2,ret,0,0);
        goto out;
    }
    
    if(ddr_info->acore_init_state != NV_BOOT_INIT_OK)
    {
        nv_record("fast boot nv init fail !\n");
        nv_show_fastboot_err();
        /* coverity[secure_coding] */
        memset(ddr_info,0,sizeof(nv_global_info_s));
    }

    ddr_info->acore_init_state = NV_KERNEL_INIT_DOING;
    nv_flush_cache((void*)NV_GLOBAL_INFO_ADDR, (u32)NV_GLOBAL_INFO_SIZE);

    ret = (u32)bsp_ipc_sem_create((u32)IPC_SEM_NV_CRC);
    if(ret)
    {
        nv_debug(NV_FUN_KERNEL_INIT, 3, ret, 0, 0);
        goto out;
    }

    ret = bsp_nvm_core_init(ddr_info->mem_file_type);
    if(ret)
    {
        goto out;
    }

    /*初始化双核使用的链表*/
    nv_flushListInit();

    ret = bsp_nvm_buf_init();
    if(ret)
    {
        nv_debug(NV_FUN_KERNEL_INIT,10,ret,0,0);
        goto out;
    }

    /*置初始化状态为OK*/
    ddr_info->acore_init_state = NV_INIT_OK;
    nv_flush_cache((void*)NV_GLOBAL_INFO_ADDR, (u32)NV_GLOBAL_INFO_SIZE);

    /*保证各分区数据正常写入*/
    nv_file_flag_check();

    INIT_LIST_HEAD(&g_nv_ctrl.stList);
    ret = (u32)osl_task_init("drv_nv",15,1024,(OSL_TASK_FUNC)bsp_nvm_icc_task,NULL,(OSL_TASK_ID*)&g_nv_ctrl.task_id);
    if(ret)
    {
        nv_record("[%s]:nv task init err! ret :0x%x\n",__func__,ret);
        goto out;
    }
    ret = nv_icc_chan_init(NV_RECV_FUNC_AC);
    if(ret)
    {
        nv_debug(NV_FUN_KERNEL_INIT,5,ret,0,0);
        goto out;
    }

    ret = nv_msg_init();
    if (ret) {
        nv_debug(NV_FUN_KERNEL_INIT,6,ret,0,0);
        goto out;
    }

    /*to do:nv id use macro define*/
    ret = bsp_nvm_read(NV_ID_MSP_FLASH_LESS_MID_THRED,(u8*)(&(g_nv_ctrl.mid_prio)),(u32)sizeof(u32));
    if(ret)
    {
        g_nv_ctrl.mid_prio = 20;
        nv_printf("read 0x%x fail,use default value! ret :0x%x\n",NV_ID_MSP_FLASH_LESS_MID_THRED,ret);
    }

    nv_record("Balong nv init ok!\n");

    return NV_OK;

out:
    nv_record("\n[%s]\n",__FUNCTION__);
    ddr_info->acore_init_state = NV_INIT_FAIL;
    nv_help(NV_FUN_KERNEL_INIT);
    nv_show_ddr_info();
    return -1;
}

static void bsp_nvm_exit(void)
{
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    /* coverity[self_assign] */
    ddr_info = ddr_info;

    /*关机写数据*/
    (void)bsp_nvm_flush();
    /*清除标志*/
    /* coverity[secure_coding] */
    memset(ddr_info,0,sizeof(nv_global_info_s));
}

void modem_nv_delay(void)
{
    u32  blk_size;
    char *blk_label;
    int i, ret = -1;

    /*最长等待时长10s*/
    for(i=0;i<10;i++)
    {
        nv_printf("modem nv wait for nv block device %d s\n",i);

        blk_label = (char*)NV_BACK_SEC_NAME;
        ret = bsp_blk_size(blk_label, &blk_size);
        if (ret) {
            nv_taskdelay(1000);
            nv_printf("get block device %s fail\n", blk_label);
            continue;
        }
        nv_printf("%s block size is 0x%x.\n", blk_label, blk_size);
        blk_label = (char*)NV_DLOAD_SEC_NAME;
        ret = bsp_blk_size(blk_label, &blk_size);
        if (ret) {
            nv_taskdelay(1000);
            nv_printf("get block device %s fail\n", blk_label);
            continue;
        }

        blk_label = (char*)NV_SYS_SEC_NAME;
        ret = bsp_blk_size(blk_label, &blk_size);
        if (ret) {
            nv_taskdelay(1000);
            nv_printf("get block device %s fail\n", blk_label);
            continue;
        }

        blk_label = (char*)NV_DEF_SEC_NAME;
        ret = bsp_blk_size(blk_label, &blk_size);
        if (ret) {
            nv_taskdelay(1000);
            nv_printf("get block device %s fail\n", blk_label);
            continue;
        }
        return;
    }
}

/*lint -save -e715*//*715表示入参dev未使用*/
static int  modem_nv_probe(struct platform_device *dev)
{
    int ret;

    g_nv_ctrl.pmState = NV_WAKEUP_STATE;
    g_nv_ctrl.opState = NV_IDLE_STATE;

    modem_nv_delay();

    /* coverity[Event check_return] *//* coverity[Event unchecked_value] */
    if(mdrv_file_access("/modem_log/drv/nv",0))
        (void)mdrv_file_mkdir("/modem_log/drv/nv");


    ret = bsp_nvm_kernel_init();

    ret |= modemNv_ProcInit();

    return ret;
}

#define NV_SHUTDOWN_STATE   NV_BOOT_INIT_OK
static void modem_nv_shutdown(struct platform_device *dev)
{
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    printk("%s shutdown start %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n",__func__);

    /*read only*/
    ddr_info->acore_init_state = NV_SHUTDOWN_STATE;
    ddr_info->ccore_init_state = NV_SHUTDOWN_STATE;
    ddr_info->mcore_init_state = NV_SHUTDOWN_STATE;
}
/*lint -restore*/

/*lint -save -e785*//*785表示对结构体初始化的不完全modem_nv_pm_ops和modem_nv_drv modem_nv_device*/
/*lint -save -e715*//*715表示入参dev未使用*/
static s32 modem_nv_suspend(struct device *dev)
{
    static int count = 0;
    if(g_nv_ctrl.opState == NV_OPS_STATE)
    {
        printk(KERN_ERR"%s Modem nv in doing !\n",__func__);
        return -1;
    }
    g_nv_ctrl.pmState = NV_SLEEP_STATE;
    printk(KERN_ERR"Modem nv enter suspend! %d times\n",++count);
    return 0;
}
static s32 modem_nv_resume(struct device *dev)
{
    static int count = 0;
    
    g_nv_ctrl.pmState = NV_WAKEUP_STATE;
    if(NV_OPS_STATE== g_nv_ctrl.opState)
    {
        printk(KERN_ERR"%s need to enter task proc!\n",__func__);
        osl_sem_up(&g_nv_ctrl.suspend_sem);
    }
    printk(KERN_ERR"Modem nv enter resume! %d times\n",++count);
    return 0;
}
/*lint -restore*/
static const struct dev_pm_ops modem_nv_pm_ops ={
    .suspend = modem_nv_suspend,
    .resume  = modem_nv_resume,
};

#define MODEM_NV_PM_OPS (&modem_nv_pm_ops)

static struct platform_driver modem_nv_drv = {
    .shutdown   = modem_nv_shutdown,
    .driver     = {
        .name     = "modem_nv",
        .owner    = (struct module *)(unsigned long)THIS_MODULE,
        .pm       = MODEM_NV_PM_OPS,
    },
};


static struct platform_device modem_nv_device = {
    .name = "modem_nv",
    .id = 0,
    .dev = {
    .init_name = "modem_nv",
    },
};
/*lint -restore*/

int  modem_nv_init(void)
{
    struct platform_device *dev = NULL;
    int ret;
    if(0 == g_nv_ctrl.initStatus)
    {
        g_nv_ctrl.initStatus = 1;
    }
    else
    {
        show_stack(current, NULL);
    }

    ret = modem_nv_probe(dev);

    return ret;
}
/*仅用于初始化nv设备*/
int nv_init_dev(void)
{
    u32 ret;
    ret = (u32)platform_device_register(&modem_nv_device);
    if(ret)
    {
        printk(KERN_ERR"platform_device_register modem_nv_device fail !\n");
        return -1;
    }

    ret = (u32)platform_driver_register(&modem_nv_drv);
    if(ret)
    {
        printk(KERN_ERR"platform_device_register modem_nv_drv fail !\n");
        platform_device_unregister(&modem_nv_device);
        return -1;
    }
    nv_printf("init modem nv dev ok\n");
    return NV_OK;
}
void  modem_nv_exit(void)
{
    bsp_nvm_exit();
    platform_device_unregister(&modem_nv_device);
    platform_driver_unregister(&modem_nv_drv);
}

device_initcall(nv_init_dev);

void bsp_nvm_make_pclint_happy(void)
{
    (void)__initcall_nv_init_dev6();
}
EXPORT_SYMBOL(bsp_nvm_backup);
EXPORT_SYMBOL(bsp_nvm_dcread);
EXPORT_SYMBOL(bsp_nvm_kernel_init);
EXPORT_SYMBOL(bsp_nvm_update_default);
EXPORT_SYMBOL(bsp_nvm_revert_default);
EXPORT_SYMBOL(bsp_nvm_dcreadpart);
EXPORT_SYMBOL(bsp_nvm_get_len);
EXPORT_SYMBOL(bsp_nvm_dcwrite);
EXPORT_SYMBOL(bsp_nvm_flush);
EXPORT_SYMBOL(bsp_nvm_reload);
EXPORT_SYMBOL(nvm_read_rand);
EXPORT_SYMBOL(nvm_read_randex);
EXPORT_SYMBOL(bsp_nvm_dcread_direct);
EXPORT_SYMBOL(bsp_nvm_dcwrite_direct);
EXPORT_SYMBOL(bsp_nvm_auth_dcread);
EXPORT_SYMBOL(bsp_nvm_auth_dcwrite);
EXPORT_SYMBOL(bsp_nvm_dcwritepart);
EXPORT_SYMBOL(bsp_nvm_get_nvidlist);
EXPORT_SYMBOL(bsp_nvm_authgetlen);
EXPORT_SYMBOL(bsp_nvm_xml_decode);





