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


#include <linux/err.h>
#include <product_config.h>
#include <bsp_blk.h>
#include "nv_file.h"
#include "nv_ctrl.h"
#include "nv_comm.h"
#include "nv_index.h"
#include "nv_debug.h"
#include "nv_crc.h"
#include "../../adrv/adrv.h"
#include "nv_partition_upgrade.h"
#include "nv_cust.h"

/*lint -save -e785*//*Info 785: (Info -- Too few initializers for aggregate 'g_emmc_info' of type 'struct nv_global_ctrl_stru')*/
static struct nv_emmc_file_header_stru g_nv_file[NV_FILE_BUTT] = {
                              {NULL,NV_FILE_DLOAD,          0,0,0,0,NV_DLOAD_PATH,          NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_DLOAD_CUST,     0,0,0,0,NV_DLOAD_CUST_PATH,     NV_DLOAD_CUST_SEC_NAME, NULL, {}},                                
                              {NULL,NV_FILE_BACKUP,         0,0,0,0,NV_BACK_PATH,           NV_BACK_SEC_NAME,  NULL, {}},
                              {NULL,NV_FILE_XNV_CARD_1,     0,0,0,0,NV_XNV_CARD1_PATH,      NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_XNV_CARD_2,     0,0,0,0,NV_XNV_CARD2_PATH,      NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_XNV_CARD_3,     0,0,0,0,NV_XNV_CARD3_PATH,      NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_CUST_CARD_1,    0,0,0,0,NV_CUST_CARD1_PATH,     NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_CUST_CARD_2,    0,0,0,0,NV_CUST_CARD2_PATH,     NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_CUST_CARD_3,    0,0,0,0,NV_CUST_CARD3_PATH,     NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_SYS_NV,         0,0,0,0,NV_FILE_SYS_NV_PATH,    NV_SYS_SEC_NAME,   NULL, {}},
                              {NULL,NV_FILE_DEFAULT,        0,0,0,0,NV_DEFAULT_PATH,        NV_DEF_SEC_NAME,   NULL, {}},
                              {NULL,NV_FILE_XNV_MAP_CARD_1, 0,0,0,0,NV_XNV_CARD1_MAP_PATH,  NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_XNV_MAP_CARD_2, 0,0,0,0,NV_XNV_CARD2_MAP_PATH,  NV_DLOAD_SEC_NAME, NULL, {}},
                              {NULL,NV_FILE_XNV_MAP_CARD_3, 0,0,0,0,NV_XNV_CARD3_MAP_PATH,  NV_DLOAD_SEC_NAME, NULL, {}},
                                                                      };

static struct nv_global_ctrl_stru g_emmc_info = {};
static struct nv_global_debug_stru g_emmc_debug[NV_FILE_OPS_MAX_API] = {};

/*reseverd1 used to reg branch*/
void nv_file_debug(u32 type,u32 reseverd1,u32 reserved2,u32 reserved3,u32 reserved4)
{
    g_emmc_debug[type].callnum++;
    g_emmc_debug[type].reseved1 = reseverd1;
    g_emmc_debug[type].reseved2 = reserved2;
    g_emmc_debug[type].reseved3 = reserved3;
    g_emmc_debug[type].reseved4 = reserved4;
}


void nv_emmc_help(u32 type)
{
    u32 i;
    if(type == NV_FILE_OPS_MAX_API)
    {
        for(i = 0;i< NV_FILE_OPS_MAX_API;i++)
        {
            nv_printf("************flash fun id %d************\n",i);
            nv_printf("call num             : 0x%x\n",g_emmc_debug[i].callnum);
            nv_printf("out branch (reseved1): 0x%x\n",g_emmc_debug[i].reseved1);
            nv_printf("reseved2             : 0x%x\n",g_emmc_debug[i].reseved2);
            nv_printf("reseved3             : 0x%x\n",g_emmc_debug[i].reseved3);
            nv_printf("reseved4             : 0x%x\n",g_emmc_debug[i].reseved4);
            nv_printf("***************************************\n");
        }
        return ;
    }

    i = type;

    nv_printf("************flash fun id %d************\n",i);
    nv_printf("call num             : 0x%x\n",g_emmc_debug[i].callnum);
    nv_printf("out branch (reseved1): 0x%x\n",g_emmc_debug[i].reseved1);
    nv_printf("reseved2             : 0x%x\n",g_emmc_debug[i].reseved2);
    nv_printf("reseved3             : 0x%x\n",g_emmc_debug[i].reseved3);
    nv_printf("reseved4             : 0x%x\n",g_emmc_debug[i].reseved4);
    nv_printf("**************************************\n");
}

/*
 * count off in this sec,bewteen sec head to off in this sec,EMMC:vir_off == phy_off
 */

/*lint -save -e715*//*715表示入参fd未使用*/
u32 nv_sec_off_count(struct nv_emmc_file_header_stru* fd,u32 vir_off, loff_t* phy_off)
{
    *phy_off = (loff_t)(unsigned long)vir_off;/* [false alarm]:屏蔽Fortify */
    return NV_OK;
}
/*lint -restore*/

/*
 *get file info in back ,default,nvdload
 */
u32 nv_sec_file_info_init(const s8* name, nv_file_map_s* sec_info)
{
    u32 ret;
    u32 file_len = 0;
    nv_file_map_s  info = {0};
    nv_ctrl_info_s ctrl_info = {0};
    u8* file_info;

    /*first: read nv ctrl file*/
    ret = (u32)bsp_blk_read((char*)name, (loff_t)0,&ctrl_info,sizeof(ctrl_info));
    if(NAND_OK != ret)
    {
        nv_printf("[%s]:patrition name %s,get file magic fail ret 0x%x,\n",__func__,name,ret);
        return ret;
    }

    /*second :check magic num in file head*/
    if(ctrl_info.magicnum != NV_CTRL_FILE_MAGIC_NUM)
    {
        nv_printf("[%s]:enter this way  1111! %s\n",__func__,name);
        return NV_OK;
    }

    if(ctrl_info.file_size > ctrl_info.file_num * sizeof(nv_file_info_s))
    {
        nv_printf("[%s]:ctrl_info.file_size:0x%x file num:0x%x\n",__func__,ctrl_info.file_size, ctrl_info.file_num);
        return BSP_ERR_NV_INVALID_PARAM;
    }

    /*third: read all nv ctrl file*/
    file_info = (u8*)nv_malloc((size_t)ctrl_info.file_size);
    if(NULL == file_info)
    {
        nv_printf("[%s]:enter this way  2222! %s\n",__func__,name);
        return BSP_ERR_NV_MALLOC_FAIL;
    }
    ret = (u32)bsp_blk_read((char*)name,sizeof(nv_ctrl_info_s),file_info, (size_t)ctrl_info.file_size);
    if(NAND_OK != ret)
    {
        nv_printf("[%s]:enter this way 3333! %s\n",__func__,name);
        goto init_end;
    }

    /*fourth: count nv file len base the ctrl file info*/
    ret = nv_get_bin_file_len(&ctrl_info,(nv_file_info_s*)(unsigned long)file_info,&file_len);
    if(ret)
    {
        nv_printf("[%s]:enter this way 4444! %s\n",__func__,name);
        goto init_end;
    }
    info.len       = file_len;
    info.magic_num = NV_FILE_EXIST;
    info.off       = 0;

    /* coverity[secure_coding] */
    memcpy(sec_info,&info,sizeof(info));
init_end:
    nv_free(file_info);
    return NV_OK;
}

u32 nv_dload_file_info_init1(const s8* name, nv_file_map_s* sec_info)
{
    u32 ret;
    u32 total_len;
    nv_dload_head nv_dload;

    ret = (u32)bsp_blk_read((char*)name, (loff_t)0, &nv_dload, sizeof(nv_dload_head));
    if(ret)
    {
        nv_record("read nvdload error in %s sec! ret = 0x%x\n", name, ret);
        return ret;
    }

    if(nv_dload.nv_bin.magic_num != NV_FILE_EXIST)
    {
        nv_record("updata file not exist in %s sec!", name);
        return BSP_ERR_NV_NO_FILE;
    }

    total_len = sizeof(nv_dload_head);
    total_len += ((nv_dload.nv_bin.magic_num == NV_FILE_EXIST)?nv_dload.nv_bin.len:0);
    total_len += ((nv_dload.xnv_map.magic_num == NV_FILE_EXIST)?nv_dload.xnv_map.len:0);
    total_len += ((nv_dload.xnv.magic_num == NV_FILE_EXIST)?nv_dload.xnv.len:0);
    total_len += ((nv_dload.cust_map.magic_num == NV_FILE_EXIST)?nv_dload.cust_map.len:0);
    total_len += ((nv_dload.cust.magic_num == NV_FILE_EXIST)?nv_dload.cust.len:0);
    total_len += sizeof(nv_dload_tail);

    sec_info->magic_num = NV_FILE_EXIST;
    sec_info->off = 0;
    sec_info->len = total_len;
    
    return NV_OK;
}

u32 nv_dload_file_info_init(void)
{
    u32 ret;
    /*lint -save -e985*//*985表示nv_dload未初始化完全，但是初始化为{0}又编译不过*/
    nv_dload_packet_head_s nv_dload = {};
    /*lint -restore*/
    xnv_map_file_s * other_card_info = NULL;
    u32 multi_card;

    /*first read file packet head*/
    ret = (u32)bsp_blk_read((char*)NV_DLOAD_SEC_NAME,(loff_t)0,&nv_dload,sizeof(nv_dload));
    if(ret)
    {
        nv_printf("[%s]:ret 0x%x,\n",__func__,ret);
        return ret;
    }
    if((nv_dload.nv_bin.magic_num != NV_FILE_EXIST)&&(nv_dload.nv_bin.magic_num != NV_DLOAD_INVALID_FLAG))
    {
        nv_printf("dload file not exsit, nv_dload.nv_bin.magic_num = 0x%x\n", nv_dload.nv_bin.magic_num);

        /* coverity[secure_coding] */
        memset((void *)&(g_emmc_info.nv_dload), 0, sizeof(g_emmc_info.nv_dload));
        if(g_emmc_info.other_card_info)
        {
            nv_free(g_emmc_info.other_card_info);
            g_emmc_info.other_card_info = NULL;
        }

        return BSP_ERR_NV_NO_FILE;
    }
    multi_card = (nv_dload.ulSimNumEx > 0)&&(NV_CTRL_FILE_MAGIC_NUM != nv_dload.ulSimNumEx);

    /*是否支持双卡外的其他卡*/
    if(multi_card)
    {
        other_card_info = (xnv_map_file_s *)nv_malloc(nv_dload.ulSimNumEx * sizeof(xnv_map_file_s));
        g_emmc_info.other_card_info = (xnv_map_file_s *)nv_malloc(nv_dload.ulSimNumEx * sizeof(xnv_map_file_s));
        if((!other_card_info)||(!g_emmc_info.other_card_info))
        {
            // cppcheck-suppress *
            if(other_card_info)
            {
                nv_free(other_card_info);
            }
            // cppcheck-suppress *
            if(g_emmc_info.other_card_info)
            {
                // cppcheck-suppress *
                nv_free(g_emmc_info.other_card_info);
                g_emmc_info.other_card_info =NULL;
            }
            nv_printf("%s :malloc fail\n",__func__);
            /*lint -save -e438*//*Warning -- Last value assigned to variable 'other_card_info' (defined at line 206) not used*/
            return BSP_ERR_NV_MALLOC_FAIL;
            /*lint -restore*/
        }
        ret = (u32)bsp_blk_read((char*)NV_DLOAD_SEC_NAME,sizeof(nv_dload), other_card_info, \
            (nv_dload.ulSimNumEx * sizeof(xnv_map_file_s)));
        if(ret)
        {
            nv_free(other_card_info);
            // cppcheck-suppress *
            nv_free(g_emmc_info.other_card_info);
            g_emmc_info.other_card_info = NULL;
            nv_printf("%s :read nvdload error! ret = 0x%x\n",__func__,ret);

            /*lint -save -e438*//*Warning -- Last value assigned to variable 'other_card_info' (defined at line 206) not used*/
            return ret;
            /*lint -restore*/
        }
    }
    //lint -save -e593
    memcpy(&g_emmc_info.nv_dload,&nv_dload,sizeof(nv_dload));
    if(multi_card)
    {
        if(other_card_info)
        {
            memcpy(g_emmc_info.other_card_info, other_card_info, nv_dload.ulSimNumEx * sizeof(xnv_map_file_s));
            nv_free(other_card_info);
        }
    }

    return NV_OK;
    //lint -restore
}

/*
 * 读nand接口
 * mtd      :   mtd device
 * off      :   loggic offset in this file,need
 * len      :   data len write to flash ,len <= mtd->erasesize
 * ptr      :   the data need to write
 */
u32 nv_blk_read(struct nv_emmc_file_header_stru* fd,FSZ off,u32 len,u8* ptr)
{
    u32 ret;
    loff_t offset = 0;    /*传进来的偏移相对于文件头的逻辑偏移*/

    /* coverity[incompatible] */
    ret = nv_sec_off_count(fd, (u32)off,&offset);
    if(ret != NAND_OK)
    {
        return ret;
    }
    ret = (u32)bsp_blk_read((char*)fd->name, offset, ptr, (unsigned long)len);

    return ret;
}


/*
 * 写nand接口
 * mtd      :   mtd device
 * off      :   loggic offset in this file,need
 * len      :   data len write to flash ,len <= mtd->erasesize
 * ptr      :   the data need to write
 */
u32 nv_blk_write(struct nv_emmc_file_header_stru* fd,FSZ off,u32 len,u8* ptr)
{
    u32 ret;
    loff_t offset = 0;    /*传进来的偏移相对于文件头的逻辑偏移*/

    ret = nv_sec_off_count(fd, (u32)off, &offset);
    if(ret != NAND_OK)
    {
        nv_printf("%s\n",fd->name);
        return ret;
    }
    ret = (u32)bsp_blk_write((char*)fd->name,offset,(void *)ptr,(size_t)len);
    if(ret)
    {
        nv_printf("%s\n",fd->name);
        return ret;
    }
    return ret;
}


/*read area init info*/
static inline u32 nv_get_nvbin_info(u32* offset,u32* len)
{
    if((g_emmc_info.nv_dload.nv_bin.magic_num== NV_FILE_EXIST) ||
       (g_emmc_info.nv_dload.nv_bin.magic_num== NV_DLOAD_INVALID_FLAG) )
    {
        *offset = g_emmc_info.nv_dload.nv_bin.off;
        *len    = g_emmc_info.nv_dload.nv_bin.len;
        return NV_OK;
    }
    else
    {
        return NV_ERROR;
    }
}

static inline u32 nv_get_xnv_info(u32 card_type,u32* offset,u32* len)
{
    if(card_type <= NV_USIMM_CARD_2)
    {
        if(g_emmc_info.nv_dload.xnv_xml[card_type-1].magic_num == NV_FILE_EXIST)
        {
            *offset = g_emmc_info.nv_dload.xnv_xml[card_type-1].off;
            *len    = g_emmc_info.nv_dload.xnv_xml[card_type-1].len;
            return NV_OK;
        }
    }
    else
    {
        if((g_emmc_info.other_card_info[card_type-3].stXnvFile.magic_num == NV_FILE_EXIST)&&(NV_SUPPORT_MULTI_CARD))
        {
            *offset = g_emmc_info.other_card_info[card_type-3].stXnvFile.off;
            *len    = g_emmc_info.other_card_info[card_type-3].stXnvFile.len;
            return NV_OK;
        }
        else
        {
            return NV_ERROR;
        }
    }
    return NV_ERROR;
}
static inline u32 nv_get_xnv_map_info(u32 card_type,u32* offset,u32* len)
{
    if(card_type <= NV_USIMM_CARD_2)
    {
        if(g_emmc_info.nv_dload.xnv_map[card_type-1].magic_num == NV_FILE_EXIST)
        {
            *offset = g_emmc_info.nv_dload.xnv_map[card_type-1].off;
            *len    = g_emmc_info.nv_dload.xnv_map[card_type-1].len;
            return NV_OK;
        }
    }
    else
    {
        if((g_emmc_info.other_card_info[card_type-3].stMapFile.magic_num == NV_FILE_EXIST)&&(NV_SUPPORT_MULTI_CARD))
        {
            *offset = g_emmc_info.other_card_info[card_type-3].stMapFile.off;
            *len    = g_emmc_info.other_card_info[card_type-3].stMapFile.len;
            return NV_OK;
        }
        else
        {
            return NV_ERROR;
        }
    }
    return NV_ERROR;
}
static inline u32 nv_get_cust_info(u32 card_type,u32* offset,u32* len)
{
    if(card_type <= NV_USIMM_CARD_2)
    {
        if(g_emmc_info.nv_dload.cust_xml[card_type-1].magic_num== NV_FILE_EXIST)
        {
            *offset = g_emmc_info.nv_dload.cust_xml[card_type-1].off;
            *len    = g_emmc_info.nv_dload.cust_xml[card_type-1].len;
            return NV_OK;
        }
    }
    else
    {
        if((g_emmc_info.other_card_info[card_type-3].stCustFile.magic_num == NV_FILE_EXIST)&&(NV_SUPPORT_MULTI_CARD))
        {
            *offset = g_emmc_info.other_card_info[card_type-3].stCustFile.off;
            *len    = g_emmc_info.other_card_info[card_type-3].stCustFile.len;
            return NV_OK;
        }
        else
        {
            return NV_ERROR;
        }
    }
    return NV_ERROR;
}

u32 nv_get_dload_nvmodem_info(u32* offset,u32* len)
{
    if(g_emmc_info.dload_nv.magic_num == NV_FILE_EXIST)
    {
        *offset = g_emmc_info.dload_nv.off;
        *len    = g_emmc_info.dload_nv.len;
        return NV_OK;
    }
    else
    {
        return NV_ERROR;
    }
}

u32 nv_get_dload_nv_info(u32* offset,u32* len)
{
    u32 ret;
    
    if(nv_upgrade_xnv_compressed())
    {
        ret = nv_get_dload_nvmodem_info(offset,len);
    }
    else
    {
        ret = nv_get_nvbin_info(offset,len);
    }

    return ret;
}

u32 nv_get_dload_nvcust_info(u32* offset,u32* len)
{
    if(g_emmc_info.dload_nvcust.magic_num == NV_FILE_EXIST)
    {
        *offset = g_emmc_info.dload_nvcust.off;
        *len    = g_emmc_info.dload_nvcust.len;
        return NV_OK;
    }
    else
    {
        return NV_ERROR;
    }
}

static inline u32 nv_get_sys_nv_info(const s8* mode,u32* offset,u32* len)
{
    s32 ret = strncmp(mode, NV_FILE_READ, sizeof(NV_FILE_READ));

    nv_check_file_mode(mode);

    if(B_READ  == ret)
    {
        if(g_emmc_info.sys_nv.magic_num != NV_FILE_EXIST)
        {
            return NV_ERROR;
        }
        *offset = g_emmc_info.sys_nv.off;
        *len    = g_emmc_info.sys_nv.len;
        return NV_OK;
    }
    else
    {
        *offset = 0;
        *len    = 0;
        return NV_OK;
    }

}
static inline u32 nv_get_default_info(const s8* mode,u32* offset,u32* len)
{
    s32 ret = strncmp(mode, NV_FILE_READ, sizeof(NV_FILE_READ));

    nv_check_file_mode(mode);

    if(B_READ  == ret)
    {
        if(g_emmc_info.def_sec.magic_num == NV_FILE_EXIST)
        {
            *offset = g_emmc_info.def_sec.off;
            *len    = g_emmc_info.def_sec.len;
            return NV_OK;
        }
        return NV_ERROR;
    }
    else
    {
        *offset = 0;
        *len    = 0;
        return NV_OK;
    }
}
static inline u32 nv_get_back_info(const s8* mode,u32* offset,u32* len)
{
    s32 ret = strncmp(mode, NV_FILE_READ, sizeof(NV_FILE_READ));

    nv_check_file_mode(mode);

    if(B_READ  == ret)
    {
        if(g_emmc_info.bak_sec.magic_num == NV_FILE_EXIST)
        {
            *offset = g_emmc_info.bak_sec.off;
            *len    = g_emmc_info.bak_sec.len;
            return NV_OK;
        }
        return NV_ERROR;
    }
    else
    {
        *offset = 0;
        *len    = 0;
        return NV_OK;
    }
}

u32 nv_init_dload_info(void)
{
    u32 ret;
    
    if(nv_upgrade_xnv_compressed())
    {
        nv_printf("nv.bin is compress file,\n");
        ret = nv_dload_file_info_init1(g_nv_file[NV_FILE_DLOAD].name, &g_emmc_info.dload_nv);
    }
    else
    {
        /*get dload info*/
        ret = nv_dload_file_info_init();
    }

    return ret;
}

u32 nv_init_dload_cust_info(void)
{
    u32 ret = NV_OK;
    
    if(nv_upgrade_xnv_compressed())
    {
        ret = nv_dload_file_info_init1(g_nv_file[NV_FILE_DLOAD_CUST].name, &g_emmc_info.dload_nvcust);
    }

    return ret;
}

u32 nv_updata_dload_info(void)
{
    u32 ret;
    if(nv_upgrade_xnv_compressed())
    {
        ret = nv_dload_file_info_init1(g_nv_file[NV_FILE_DLOAD].name,&g_emmc_info.dload_nv);
    }
    else
    {
        ret = nv_dload_file_info_init();
    }
    
    if(ret == BSP_ERR_NV_NO_FILE)
    {
        ret = NV_OK;
    }
    
    return ret;
}

u32 nv_exist_dload_info(void)
{
    u32 ret;
    
    if(nv_upgrade_xnv_compressed())
    {
        ret = (g_emmc_info.dload_nv.magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
    }
    else
    {
        ret = ((g_emmc_info.nv_dload.nv_bin.magic_num == NV_FILE_EXIST) ||
           (g_emmc_info.nv_dload.nv_bin.magic_num == NV_DLOAD_INVALID_FLAG))?NV_FILE_EXIST:0;
    }

    return ret;
}

u32 nv_emmc_init(void)
{
    u32 ret;
    u32 i;

    nv_file_debug(NV_FILE_INIT_API,0,0,0,0);

    /*first init every file sem*/
    for(i = 0;i<NV_FILE_BUTT;i++)
    {
        osl_sem_init(1,&g_nv_file[i].file_sem);
    }
    /* coverity[secure_coding] */
    memset(&g_emmc_info,0,sizeof(struct nv_global_ctrl_stru));

    //g_flash_emmc_info_ptr = &g_emmc_info;
    
    /*get sys nv info*/
    ret = nv_sec_file_info_init(g_nv_file[NV_FILE_SYS_NV].name,&g_emmc_info.sys_nv);
    if(ret)
    {
        nv_file_debug(NV_FILE_INIT_API,1,ret,(u32)(unsigned long)g_nv_file[NV_FILE_SYS_NV].mtd,NV_FILE_SYS_NV);
        goto nv_emmc_init_err;
    }

    /* get dlaod info */
    ret = nv_init_dload_info();
    if(ret)
    {
        nv_file_debug(NV_FILE_INIT_API,2,ret,0,NV_FILE_DLOAD);
    }

    /* get dload cust info */
    ret = nv_init_dload_cust_info();
    if(ret)
    {
        nv_file_debug(NV_FILE_INIT_API,3,ret,0,NV_FILE_DLOAD_CUST);
    }

    /*get backup info*/
    ret = nv_sec_file_info_init(g_nv_file[NV_FILE_BACKUP].name,&g_emmc_info.bak_sec);
    if(ret)
    {
        nv_file_debug(NV_FILE_INIT_API,4,ret,0,NV_FILE_BACKUP);
        goto nv_emmc_init_err;
    }

    /*get default info*/
    ret = nv_sec_file_info_init(g_nv_file[NV_FILE_DEFAULT].name,&g_emmc_info.def_sec);
    if(ret)
    {
        nv_file_debug(NV_FILE_INIT_API,5,ret,0,NV_FILE_DEFAULT);
        goto nv_emmc_init_err;
    }
    return NV_OK;
nv_emmc_init_err:
    nv_printf("\n[%s]\n",__func__);
    nv_emmc_help(NV_FILE_INIT_API);
    return NV_ERROR;
}




FILE* nv_emmc_open(const s8* path,const s8* mode)
{
    u32 ret = NV_ERROR;
    u32 i;
    struct nv_emmc_file_header_stru* fd = NULL;
    u32 offset = 0;
    u32 len = 0;

    nv_file_debug(NV_FILE_OPEN_API,0,0,0,0);

    for(i=0; i<NV_FILE_BUTT; i++)
    {
        if(0 == strncmp(path, g_nv_file[i].path, strlen(g_nv_file[i].path) + 1))
        {
            fd = &g_nv_file[i];
            break;
        }
    }
    if(NULL == fd)
    {
        nv_file_debug(NV_FILE_OPEN_API,1,0,0,0);
        return NULL;
    }
    osl_sem_down(&fd->file_sem);
    switch(fd->emmc_type)
    {
        case NV_FILE_DLOAD:
            ret = nv_get_dload_nv_info(&offset,&len);
            break;
        case NV_FILE_DLOAD_CUST:
            ret = nv_get_dload_nvcust_info(&offset,&len);
            break;
        case NV_FILE_BACKUP:
            ret = nv_sec_file_info_init(g_nv_file[NV_FILE_BACKUP].name,&g_emmc_info.bak_sec);
            ret |= nv_get_back_info(mode,&offset,&len);
            break;
        case NV_FILE_XNV_CARD_1:
            ret = nv_get_xnv_info(NV_USIMM_CARD_1,&offset,&len);
            break;
        case NV_FILE_CUST_CARD_1:
            ret = nv_get_cust_info(NV_USIMM_CARD_1,&offset,&len);
            break;
        case NV_FILE_XNV_CARD_2:
            ret = nv_get_xnv_info(NV_USIMM_CARD_2,&offset,&len);
            break;
        case NV_FILE_CUST_CARD_2:
            ret = nv_get_cust_info(NV_USIMM_CARD_2,&offset,&len);
            break;
        case NV_FILE_XNV_CARD_3:
            ret = nv_get_xnv_info(NV_USIMM_CARD_3,&offset,&len);
            break;
        case NV_FILE_CUST_CARD_3:
            ret = nv_get_cust_info(NV_USIMM_CARD_3,&offset,&len);
            break;
        case NV_FILE_SYS_NV:
            ret = nv_get_sys_nv_info(mode,&offset,&len);
            break;
        case NV_FILE_DEFAULT:
            ret = nv_get_default_info(mode,&offset,&len);
            break;
        case NV_FILE_XNV_MAP_CARD_1:
            ret = nv_get_xnv_map_info(NV_USIMM_CARD_1,&offset,&len);
            break;
        case NV_FILE_XNV_MAP_CARD_2:
            ret = nv_get_xnv_map_info(NV_USIMM_CARD_2,&offset,&len);
            break;
        case NV_FILE_XNV_MAP_CARD_3:
            ret = nv_get_xnv_map_info(NV_USIMM_CARD_3,&offset,&len);
            break;
        default:
            ret = BSP_ERR_NV_INVALID_PARAM;
    }

    if(NV_OK != ret)
    {
        osl_sem_up(&fd->file_sem);
        nv_file_debug(NV_FILE_OPEN_API,3,fd->emmc_type,ret,0);
        return NULL;
    }

    fd->ops ++;
    fd->seek   = 0;
    fd->length = len;
    fd->off    = offset;
    fd->fp     = fd;

    return fd;
}
 
u32 nv_emmc_read(u8* ptr, u32 size, u32 count, FILE* fp)
{
    u32 real_size;
    u32 ret;
    struct nv_emmc_file_header_stru* fd = (struct nv_emmc_file_header_stru*)fp;
    u32 len = size*count;


    nv_file_debug(NV_FILE_READ_API,0,0,size,count);

    if((NULL == fd)||(fd->fp != fd))
    {
        return BSP_ERR_NV_INVALID_PARAM;
    }

    real_size = ((fd->seek+len) < fd->length)? len: (fd->length - fd->seek );

    ret = nv_blk_read(fd, (FSZ)((unsigned long)fd->off+fd->seek),real_size,ptr);/*读取注意文件seek位置*/
    if(ret != NAND_OK)
    {
        nv_file_debug(NV_FILE_READ_API,2,(u32)ret,real_size,fd->emmc_type);
        nv_printf("\n[%s]\n",__func__);
        nv_emmc_help(NV_FILE_READ_API);
        return NV_ERROR;
    }
    fd->seek += real_size;
    return real_size;
}

 
u32 nv_emmc_write(u8* ptr, u32 size, u32 count, FILE* fp)
{
    u32 ret = NV_ERROR;
    u32 len = size*count;
    struct nv_emmc_file_header_stru* fd = (struct nv_emmc_file_header_stru*)fp;
    nv_file_map_s* file_info;

    nv_file_debug(NV_FILE_WRITE_API,0,0,size,count);

    if((NULL == fd)||(fd->fp != fd))
    {
        nv_file_debug(NV_FILE_WRITE_API,1,0,size,count);
        goto nv_flash_write_err;
    }

    switch(fd->emmc_type)
    {
        case NV_FILE_BACKUP:
            file_info = &g_emmc_info.bak_sec;
            break;
        case NV_FILE_SYS_NV:
            file_info = &g_emmc_info.sys_nv;
            break;
        case NV_FILE_DEFAULT:
            file_info = &g_emmc_info.def_sec;
            break;
        case NV_FILE_DLOAD:
            file_info = &g_emmc_info.dload_nv;
            break;
        case NV_FILE_DLOAD_CUST:
            file_info = &g_emmc_info.dload_nvcust;
            break;
        default:
            return NV_ERROR;
    }

    ret = nv_blk_write(fd,(FSZ)((unsigned long)fd->off + fd->seek),len,ptr);
    if(ret)
    {
        nv_file_debug(NV_FILE_WRITE_API,3,ret,len,fd->emmc_type);
        goto nv_flash_write_err;
    }

    file_info->magic_num = NV_FILE_EXIST;
    file_info->len = fd->off + fd->seek + len;
    file_info->off       = 0;
    fd->seek += len;
    return len;

nv_flash_write_err:
    nv_record("\n[%s]\n",__func__);
    nv_emmc_help(NV_FILE_WRITE_API);
    return BSP_ERR_NV_INVALID_PARAM;
}
 
u32 nv_emmc_seek(FILE* fp,u32 offset,s32 whence)
{

    u32 ret = 0;
    struct nv_emmc_file_header_stru* fd = (struct nv_emmc_file_header_stru*)fp;

    nv_file_debug(NV_FILE_SEEK_API,0,offset,(u32)whence,0);

    if((NULL == fd)||(fd->fp != fd))
    {
        nv_file_debug(NV_FILE_SEEK_API,1,offset,(u32)whence,0);
        goto out;
    }
    ret = fd->seek;
    switch(whence)
    {
        case SEEK_SET:
            nv_file_debug(NV_FILE_SEEK_API,2,offset,(u32)whence,ret);
            ret = offset;
            break;
        case SEEK_CUR:
            nv_file_debug(NV_FILE_SEEK_API,3,offset,(u32)whence,ret);
            ret += offset;
            break;
        case SEEK_END:
            nv_file_debug(NV_FILE_SEEK_API,4,offset,(u32)whence,ret);
            ret = fd->length + offset;
            break;
        default:
            nv_file_debug(NV_FILE_SEEK_API,5,offset,(u32)whence,ret);
            goto out;
    }
    fd->seek = ret;
    return NV_OK;
out:
    nv_printf("\n[%s]\n",__func__);
    nv_emmc_help(NV_FILE_SEEK_API);
    return BSP_ERR_NV_INVALID_PARAM;
}
 

static inline bool nv_dload_exist_file(void)
{
    if(   (g_emmc_info.nv_dload.nv_bin.magic_num      != NV_FILE_EXIST)
        &&(g_emmc_info.nv_dload.xnv_xml[0].magic_num  != NV_FILE_EXIST)
        &&(g_emmc_info.nv_dload.xnv_xml[1].magic_num  != NV_FILE_EXIST)
        &&(g_emmc_info.nv_dload.cust_xml[0].magic_num != NV_FILE_EXIST)
        &&(g_emmc_info.nv_dload.cust_xml[1].magic_num != NV_FILE_EXIST)
        &&(g_emmc_info.nv_dload.xnv_map[0].magic_num != NV_FILE_EXIST)
        &&(g_emmc_info.nv_dload.xnv_map[1].magic_num != NV_FILE_EXIST)
        &&(0 == g_emmc_info.nv_dload.ulSimNumEx)
        )
    {
        return false;
    }
    return true;
}
u32 nv_emmc_remove(const s8* path)
{
    u32 ret = NV_ERROR;
    struct nv_emmc_file_header_stru* fd = NULL;
    u32 i;

    nv_file_debug(NV_FILE_REMOVE_API,0,0,0,0);

    for(i=0;i<NV_FILE_BUTT;i++)
    {
        if(0 == strncmp(path, g_nv_file[i].path, strlen(g_nv_file[i].path) + 1))
        {
            fd = &g_nv_file[i];
            break;
        }
    }

    if(NULL == fd)
    {
        nv_file_debug(NV_FILE_REMOVE_API,1,0,0,0);
        return NV_ERROR;
    }
    switch(fd->emmc_type)
    {
        case NV_FILE_DLOAD:
            g_emmc_info.nv_dload.nv_bin.magic_num = NV_FLASH_NULL;
            break;
        case NV_FILE_DLOAD_CUST:
            g_emmc_info.dload_nvcust.magic_num = NV_FLASH_NULL;
            break;
        case NV_FILE_BACKUP:
            /* coverity[secure_coding] */
            memset(&g_emmc_info.bak_sec,NV_FLASH_FILL,sizeof(nv_file_map_s));
            goto flash_erase;
        case NV_FILE_CUST_CARD_1:
            g_emmc_info.nv_dload.cust_xml[0].magic_num = NV_FLASH_NULL;
            break;
        case NV_FILE_XNV_CARD_1:
            g_emmc_info.nv_dload.xnv_xml[0].magic_num = NV_FLASH_NULL;
            break;
        case NV_FILE_CUST_CARD_2:
            g_emmc_info.nv_dload.cust_xml[1].magic_num = NV_FLASH_NULL;
            break;
        case NV_FILE_XNV_CARD_2:
            g_emmc_info.nv_dload.xnv_xml[1].magic_num = NV_FLASH_NULL;
            break;
        case NV_FILE_CUST_CARD_3:
            if(NV_SUPPORT_MULTI_CARD)
            {
                /*将支持的其他卡数量改为0，则检测是否支持多卡时会返回不支持*/
                g_emmc_info.nv_dload.ulSimNumEx = 0;
                g_emmc_info.nv_dload.xnv_file[0].stCustFile.magic_num = NV_FLASH_NULL;
            }
            break;
        case NV_FILE_XNV_CARD_3:
            if(NV_SUPPORT_MULTI_CARD)
            {
                /*将支持的其他卡数量改为0，则检测是否支持多卡时会返回不支持*/
                g_emmc_info.nv_dload.ulSimNumEx = 0;
                g_emmc_info.nv_dload.xnv_file[0].stXnvFile.magic_num = NV_FLASH_NULL;
            }
            break;
        case NV_FILE_XNV_MAP_CARD_1:
            g_emmc_info.nv_dload.xnv_map[0].magic_num = NV_FLASH_NULL;
            break;
        case NV_FILE_XNV_MAP_CARD_2:
            g_emmc_info.nv_dload.xnv_map[1].magic_num = NV_FLASH_NULL;
            break;
        case NV_FILE_XNV_MAP_CARD_3:
            if(NV_SUPPORT_MULTI_CARD)
            {
                /*将支持的其他卡数量改为0，则检测是否支持多卡时会返回不支持*/
                g_emmc_info.nv_dload.ulSimNumEx = 0;
                g_emmc_info.nv_dload.xnv_file[0].stMapFile.magic_num = NV_FLASH_NULL;
            }
            break;
        case NV_FILE_DEFAULT:
            /* coverity[secure_coding] */
            memset(&g_emmc_info.def_sec,NV_FLASH_FILL,sizeof(nv_file_map_s));
            goto flash_erase;
        case NV_FILE_SYS_NV:
            /* coverity[secure_coding] */
            memset(&g_emmc_info.sys_nv,NV_FLASH_FILL,sizeof(g_emmc_info.sys_nv));
            goto flash_erase;
        default:
            return BSP_ERR_NV_INVALID_PARAM;
    }
    if(true == nv_dload_exist_file())
    {
        return NV_OK;
    }
flash_erase:
    ret = (u32)bsp_blk_erase(fd->name);
    if(ret)
    {
        nv_file_debug(NV_FILE_REMOVE_API,2,ret,fd->emmc_type,0);
        nv_printf("[%s]:ret 0x%x,mtd->name %s\n",__func__,ret,fd->name);
        return ret;
    }

    return NV_OK;
 }

u32 nv_emmc_close(FILE* fp)
{
    struct nv_emmc_file_header_stru* fd = (struct nv_emmc_file_header_stru*)fp;

    nv_file_debug(NV_FILE_CLOSE_API,0,0,0,0);

    if((NULL == fd)||(fd->fp != fd))
    {
        nv_file_debug(NV_FILE_CLOSE_API,1,0,0,0);
        return BSP_ERR_NV_INVALID_PARAM;
    }

    osl_sem_up(&fd->file_sem);

    fd->fp = NULL;
    fd->seek = 0;
    fd->length = 0;
    fd->off = 0;
    fd->ops --;
    if(fd->ops != 0)
    {
        nv_file_debug(NV_FILE_CLOSE_API,2,fd->ops,0,0);
        return BSP_ERR_NV_CLOSE_FILE_FAIL;
    }

    return NV_OK;
}

u32 nv_emmc_ftell(FILE* fp)
{
    struct nv_emmc_file_header_stru* fd = (struct nv_emmc_file_header_stru*)fp;

    nv_file_debug(NV_FILE_FTELL_API,0,0,0,0);

    if((NULL == fd)||(fd->fp != fd))
    {
        nv_file_debug(NV_FILE_FTELL_API,1,0,0,0);
        return BSP_ERR_NV_INVALID_PARAM;
    }
    return fd->seek;
}


u32 nv_emmc_access(const s8* path,s32 mode)
{
    u32 ret = NV_ERROR;
    u32 i;
    struct nv_emmc_file_header_stru* fd = NULL;


    for(i=0; i<NV_FILE_BUTT; i++)
    {
        if(0 == strncmp(path, g_nv_file[i].path, strlen(g_nv_file[i].path) + 1))
        {
            fd = &g_nv_file[i];
            break;
        }
    }
    if(NULL == fd)
    {
        return NV_ERROR;
    }
	/* coverity[self_assign] */
    mode = mode;
    switch(fd->emmc_type)
    {
        case NV_FILE_DLOAD:
            ret = nv_exist_dload_info();
            break;
        case NV_FILE_DLOAD_CUST:
            ret = (g_emmc_info.dload_nvcust.magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;            
        case NV_FILE_BACKUP:
            ret = (g_emmc_info.bak_sec.magic_num== NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_XNV_CARD_1:
            ret = (g_emmc_info.nv_dload.xnv_xml[0].magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_XNV_CARD_2:
            ret = (g_emmc_info.nv_dload.xnv_xml[1].magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_XNV_CARD_3:
            if(NV_SUPPORT_MULTI_CARD)
            {
                ret = (g_emmc_info.other_card_info[0].stXnvFile.magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            }
            break;
        case NV_FILE_CUST_CARD_1:
            ret = (g_emmc_info.nv_dload.cust_xml[0].magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_CUST_CARD_2:
            ret = (g_emmc_info.nv_dload.cust_xml[1].magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_CUST_CARD_3:
            if(NV_SUPPORT_MULTI_CARD)
            {
                ret = (g_emmc_info.other_card_info[0].stCustFile.magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            }
            break;
        case NV_FILE_SYS_NV:
            ret = (g_emmc_info.sys_nv.magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_DEFAULT:
            ret = (g_emmc_info.def_sec.magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_XNV_MAP_CARD_1:
            ret = (g_emmc_info.nv_dload.xnv_map[0].magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_XNV_MAP_CARD_2:
            ret = (g_emmc_info.nv_dload.xnv_map[1].magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            break;
        case NV_FILE_XNV_MAP_CARD_3:
            if(NV_SUPPORT_MULTI_CARD)
            {
                ret = (g_emmc_info.other_card_info[0].stMapFile.magic_num == NV_FILE_EXIST)?NV_FILE_EXIST:0;
            }
            break;
        default:
            return NV_ERROR;
    }

    if(ret != NV_FILE_EXIST)
    {
        return NV_ERROR;
    }
    return 0;
}


/*****************************************************************************
 函 数 名  : nv_support_multi_card
 功能描述  : 是否支持多卡nv
 输入参数  : void
 输出参数  : 无
 返 回 值  : 1:支持 0:不支持
*****************************************************************************/
u32 nv_support_multi_card(void)
{
    if((g_emmc_info.nv_dload.nv_bin.magic_num != NV_FILE_EXIST)&&(g_emmc_info.nv_dload.nv_bin.magic_num != NV_DLOAD_INVALID_FLAG))
    {
       return false;
    }
    else
    {
        return (u32)((g_emmc_info.nv_dload.ulSimNumEx > 0)
                &&(NV_CTRL_FILE_MAGIC_NUM != g_emmc_info.nv_dload.ulSimNumEx)
                &&(g_emmc_info.other_card_info != NULL));
    }

}
/*****************************************************************************
 函 数 名  : nv_get_dload_file_len
 功能描述  : 计算dload分区的大小,不包含最后的升级包的CRC校验码
 输入参数  : void
 输出参数  : 无
 返 回 值  : 1:支持 0:不支持
*****************************************************************************/
u32 nv_get_dload_file_len(void)
{
    u32 max_off = 0;
    u32 index = 0;
    u32 max_len = 0;

#define MAX_FILE(info, max, size) \
do{\
    if(((NV_FILE_EXIST == info.magic_num )||(NV_DLOAD_INVALID_FLAG == info.magic_num ))&&(max < info.off))\
    {\
        max = info.off; \
        size = info.len;\
    }\
}while(0)

    MAX_FILE(g_emmc_info.nv_dload.cust_xml[0], max_off, max_len);
    MAX_FILE(g_emmc_info.nv_dload.cust_xml[1], max_off, max_len);
    MAX_FILE(g_emmc_info.nv_dload.xnv_xml[0], max_off, max_len);
    MAX_FILE(g_emmc_info.nv_dload.xnv_xml[1], max_off, max_len);
    MAX_FILE(g_emmc_info.nv_dload.xnv_map[0], max_off, max_len);
    MAX_FILE(g_emmc_info.nv_dload.xnv_map[1], max_off, max_len);
    MAX_FILE(g_emmc_info.nv_dload.nv_bin, max_off, max_len);

    if(nv_support_multi_card())
    {
        for(index = 0; index < g_emmc_info.nv_dload.ulSimNumEx; index++)
        {
            MAX_FILE(g_emmc_info.other_card_info[index].stCustFile, max_off, max_len);
            MAX_FILE(g_emmc_info.other_card_info[index].stMapFile, max_off, max_len);
            MAX_FILE(g_emmc_info.other_card_info[index].stXnvFile, max_off, max_len);
        }
    }
    nv_printf("max:0x%x len:0x%x\n", max_off, max_len);
    return (max_off + max_len);
}

/*****************************************************************************
 函 数 名  : nv_flash_update_info
 功能描述  : 更新各个分区的信息
 输入参数  : void
 输出参数  : 无
 返 回 值  : 0 成功 其他失败
*****************************************************************************/
u32 nv_emmc_update_info(const s8* path)
{
    u32 ret;
    u32 i;

    for(i=0; i<NV_FILE_BUTT; i++)
    {
        if(0 == strncmp(path, g_nv_file[i].path, strlen(g_nv_file[i].path) + 1))
        {
            break;
        }
    }
    switch(i)
    {
        case NV_FILE_DLOAD:
            ret = nv_updata_dload_info();
            if(ret)
            {
                nv_printf("update nv_modem or nv file fail\n");
            }
            break;
        case NV_FILE_DLOAD_CUST:
            ret = nv_dload_file_info_init1(g_nv_file[NV_FILE_DLOAD_CUST].name,&g_emmc_info.dload_nvcust);
            if(ret)
            {
                nv_printf("update nv_cust file fail\n");
            }
            break;
        case NV_FILE_BACKUP:
            ret = nv_sec_file_info_init(g_nv_file[NV_FILE_BACKUP].name,&g_emmc_info.bak_sec);
            if(ret)
            {
                nv_printf("update backup file fail\n");
            }
            break;
        case NV_FILE_SYS_NV:
            ret = nv_sec_file_info_init(g_nv_file[NV_FILE_SYS_NV].name,&g_emmc_info.sys_nv);
            if(ret)
            {
                nv_printf("update sys file fail\n");
            }
            break;
        case NV_FILE_DEFAULT:
            ret = nv_sec_file_info_init(g_nv_file[NV_FILE_DEFAULT].name,&g_emmc_info.def_sec);
            if(ret)
            {
                nv_printf("update default file fail\n");
            }
            break;
        default:
            ret = BSP_ERR_NV_INVALID_PARAM;
    }

    return ret;
}
void show_emmc_info(void)
{
    nv_dload_packet_head_s nv_dload = {};
    nv_ctrl_info_s ctrl_info = {};
    u8* file_info;
    s32 ret ;
    u32 i = 0;

    nv_printf("\n******************img info*********************\n");
    ret = bsp_blk_read((char*)NV_SYS_SEC_NAME, (loff_t)0,&ctrl_info,sizeof(ctrl_info));
    if(ret)
        return;
    /* coverity[uninit_use] */
    if(ctrl_info.magicnum != NV_CTRL_FILE_MAGIC_NUM)
    {
        /* coverity[uninit_use] */
        ctrl_info.file_size = 144;
    }
    /* coverity[uninit_use] */
    file_info = (u8*)nv_malloc((unsigned long)ctrl_info.file_size);
    if(NULL == file_info)
    {
        return;
    }
    ret = bsp_blk_read((char*)NV_SYS_SEC_NAME, sizeof(ctrl_info), file_info, (unsigned long)ctrl_info.file_size);
    if(ret)
    {
        nv_free((void *)file_info);
        return;
    }
    nv_printf("\n********sys mem info*******\n");
    nv_printf("nv   :flag 0x%x,off 0x%x,len 0x%x\n",g_emmc_info.sys_nv.magic_num,\
        g_emmc_info.sys_nv.off,g_emmc_info.sys_nv.len);
    nv_printf("\n************sys info in nand**************\n");
    /* coverity[uninit_use] */
    nv_printf("magic :0x%x,file num: %d,nv num :0x%x,modem num :%d\n",\
        ctrl_info.magicnum,ctrl_info.file_num,ctrl_info.ref_count,ctrl_info.modem_num);

    nv_printf("\n******************dload info*******************\n");
    ret = bsp_blk_read((char*)NV_DLOAD_SEC_NAME, (loff_t)0,&nv_dload,sizeof(nv_dload));
    if(ret)
    {
        nv_free((void *)file_info);
        return;
    }
    nv_printf("\n********dload mem info*******\n");
    nv_printf("nv   : flag 0x%x,len 0x%x,off 0x%x\n",g_emmc_info.nv_dload.nv_bin.magic_num,\
        g_emmc_info.nv_dload.nv_bin.len,g_emmc_info.nv_dload.nv_bin.off);
    nv_printf("xnv1 : flag 0x%x,len 0x%x,off 0x%x\n",g_emmc_info.nv_dload.xnv_xml[0].magic_num,\
        g_emmc_info.nv_dload.xnv_xml[0].len,g_emmc_info.nv_dload.xnv_xml[0].off);
    nv_printf("cust1: flag 0x%x,len 0x%x,off 0x%x\n",g_emmc_info.nv_dload.cust_xml[0].magic_num,\
        g_emmc_info.nv_dload.cust_xml[0].len,g_emmc_info.nv_dload.cust_xml[0].off);

    nv_printf("xnv2 : flag 0x%x,len 0x%x,off 0x%x\n",g_emmc_info.nv_dload.xnv_xml[1].magic_num,\
        g_emmc_info.nv_dload.xnv_xml[1].len,g_emmc_info.nv_dload.xnv_xml[1].off);
    nv_printf("cust2: flag 0x%x,len 0x%x,off 0x%x\n",g_emmc_info.nv_dload.cust_xml[1].magic_num,\
        g_emmc_info.nv_dload.cust_xml[1].len,g_emmc_info.nv_dload.cust_xml[1].off);
    nv_printf("xnv map1: flag 0x%x,len 0x%x,off 0x%x\n",g_emmc_info.nv_dload.xnv_map[0].magic_num,\
        g_emmc_info.nv_dload.xnv_map[0].len,g_emmc_info.nv_dload.xnv_map[0].off);
    nv_printf("xnv map2: flag 0x%x,len 0x%x,off 0x%x\n",g_emmc_info.nv_dload.xnv_map[1].magic_num,\
        g_emmc_info.nv_dload.xnv_map[1].len,g_emmc_info.nv_dload.xnv_map[1].off);
    if(NV_SUPPORT_MULTI_CARD)
    {
        for(i = 0; i < g_emmc_info.nv_dload.ulSimNumEx; i++)
        {
            nv_printf("xnv%d : flag 0x%x,len 0x%x,off 0x%x\n",i, g_emmc_info.other_card_info[i].stXnvFile.magic_num,\
                g_emmc_info.other_card_info[i].stXnvFile.len, g_emmc_info.other_card_info[i].stXnvFile.off);
            nv_printf("cust%d: flag 0x%x,len 0x%x,off 0x%x\n",i ,g_emmc_info.other_card_info[i].stCustFile.magic_num,\
                g_emmc_info.other_card_info[i].stCustFile.len,g_emmc_info.other_card_info[i].stCustFile.off);
            nv_printf("xnv map%d: flag 0x%x,len 0x%x,off 0x%x\n",i, g_emmc_info.other_card_info[i].stMapFile.magic_num,\
                g_emmc_info.other_card_info[i].stMapFile.len, g_emmc_info.other_card_info[i].stMapFile.off);

        }
    }
    nv_printf("\n********dload mtd info*******\n");
    nv_printf("nv   : flag 0x%x,len 0x%x,off 0x%x\n",nv_dload.nv_bin.magic_num,\
        nv_dload.nv_bin.len,nv_dload.nv_bin.off);
    nv_printf("xnv1 : flag 0x%x,len 0x%x,off 0x%x\n",nv_dload.xnv_xml[0].magic_num,\
        nv_dload.xnv_xml[0].len,nv_dload.xnv_xml[0].off);
    nv_printf("cust1: flag 0x%x,len 0x%x,off 0x%x\n",nv_dload.cust_xml[0].magic_num,\
        nv_dload.cust_xml[0].len,nv_dload.cust_xml[0].off);

    nv_printf("xnv2 : flag 0x%x,len 0x%x,off 0x%x\n",nv_dload.xnv_xml[1].magic_num,\
        nv_dload.xnv_xml[1].len,nv_dload.xnv_xml[1].off);
    nv_printf("cust2: flag 0x%x,len 0x%x,off 0x%x\n",nv_dload.cust_xml[1].magic_num,\
        nv_dload.cust_xml[1].len,nv_dload.cust_xml[1].off);

    nv_printf("xnv map1: flag 0x%x,len 0x%x,off 0x%x\n",nv_dload.xnv_map[0].magic_num,\
        nv_dload.xnv_map[0].len,nv_dload.xnv_map[0].off);
    nv_printf("xnv map2: flag 0x%x,len 0x%x,off 0x%x\n",nv_dload.xnv_map[1].magic_num,\
        nv_dload.xnv_map[1].len,nv_dload.xnv_map[1].off);

    if(NV_SUPPORT_MULTI_CARD)
    {
        for(i = 0; i < nv_dload.ulSimNumEx; i++)
        {
            nv_printf("xnv%d : flag 0x%x,len 0x%x,off 0x%x\n",i ,nv_dload.xnv_file[i].stXnvFile.magic_num,\
                nv_dload.xnv_file[i].stXnvFile.len,nv_dload.xnv_file[i].stXnvFile.off);
             /* coverity[uninit_use] */
            nv_printf("cust%d: flag 0x%x,len 0x%x,off 0x%x\n",i ,nv_dload.xnv_file[i].stCustFile.magic_num,\
                nv_dload.xnv_file[i].stCustFile.len,nv_dload.xnv_file[i].stCustFile.off);
             nv_printf("xnv map%d: flag 0x%x,len 0x%x,off 0x%x\n",i ,nv_dload.xnv_file[i].stMapFile.magic_num,\
                 nv_dload.xnv_file[i].stMapFile.len,nv_dload.xnv_file[i].stMapFile.off);
        }
    }
    nv_printf("\n******************backup info******************\n");
    nv_printf("\n********backup mem info******\n");
    nv_printf("backup flag: 0x%x, len : 0x%x, off:0x%x\n",g_emmc_info.bak_sec.magic_num,\
        g_emmc_info.bak_sec.len,g_emmc_info.bak_sec.off);
    nv_printf("\n********backup mtd info******\n");
    ret = bsp_blk_read((char*)NV_BACK_SEC_NAME, (loff_t)0,&ctrl_info,sizeof(ctrl_info));
    if(ret)
    {
        nv_free((void *)file_info);
        return;
    }

    /* coverity[uninit_use] */
    if(ctrl_info.magicnum != NV_CTRL_FILE_MAGIC_NUM)
    {
        ctrl_info.file_size = 144;
    }
    ret = bsp_blk_read((char*)NV_BACK_SEC_NAME, sizeof(ctrl_info), file_info, (size_t)ctrl_info.file_size);
    if(ret)
    {
        nv_free((void *)file_info);
        return;
    }
    /* coverity[uninit_use] */
    nv_printf("magic :0x%x,file num: %d,nv num :0x%x,modem num :%d\n",\
        ctrl_info.magicnum,ctrl_info.file_num,ctrl_info.ref_count,ctrl_info.modem_num);

    nv_printf("\n******************default info*****************\n");
    nv_printf("\n********default mem info*****\n");
    nv_printf("default flag: 0x%x, len : 0x%x, off:0x%x\n",g_emmc_info.def_sec.magic_num,\
        g_emmc_info.def_sec.len,g_emmc_info.def_sec.off);

    nv_printf("\n********default mtd info*****\n");
    ret = bsp_blk_read((char*)NV_DEF_SEC_NAME,(loff_t)0, &ctrl_info, sizeof(ctrl_info));
    if(ret)
    {
        nv_free((void *)file_info);
        return;
    }
    /* coverity[uninit_use] */
    if(ctrl_info.magicnum != NV_CTRL_FILE_MAGIC_NUM)
    {
        /* coverity[uninit_use] */
        ctrl_info.file_size = 144;
    }
    ret = bsp_blk_read((char*)NV_DEF_SEC_NAME, sizeof(ctrl_info), file_info, (unsigned long)ctrl_info.file_size);
    if(ret)
    {
        nv_free((void *)file_info);
        return;
    }
    /* coverity[uninit_use] */
    nv_printf("magic :0x%x,file num: %d,nv num :0x%x,modem num :%d\n",\
        ctrl_info.magicnum,ctrl_info.file_num,ctrl_info.ref_count,ctrl_info.modem_num);


    nv_free(file_info);
}


EXPORT_SYMBOL(nv_emmc_help);
EXPORT_SYMBOL(show_emmc_info);
EXPORT_SYMBOL(nv_get_dload_file_len);
EXPORT_SYMBOL(nv_support_multi_card);
/*lint -restore*/


