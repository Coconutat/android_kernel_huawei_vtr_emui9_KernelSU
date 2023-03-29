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

#include <linux/of.h>
#include <linux/delay.h>
#include <asm/string.h>
#include "osl_types.h"
#include "bsp_version.h"
#include "bsp_sram.h"
#include "bsp_dump_mem.h"
#include "bsp_coresight.h"
#include "drv_nv_def.h"
#include "mdrv_om.h"
#include "drv_nv_id.h"
#include "bsp_nvim.h" 
#include "bsp_dump.h" 
#include "dump_config.h"
#include "dump_print.h"

 
DUMP_FILE_CFG_STRU              g_dump_file_cfg;
NV_DUMP_STRU                    g_dump_cfg;
dump_product_type_t             g_product_type = DUMP_PRODUCT_BUTT;
enum EDITION_KIND               g_edition = EDITION_MAX;

 

/*****************************************************************************
* 函 数 名  : dump_product_type_init
* 功能描述  : 初始化产品形态
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_product_type_init(void)
{
    struct device_node *node = NULL;
    const char * product_type  = NULL;

    node = of_find_compatible_node(NULL, NULL, "hisilicon,smntn_type");
    if (!node)
    {
        dump_fetal("dts node mntn type not found!\n");
        return;
    }

    if(of_property_read_string(node, "product_type", &product_type))
    {
        dump_fetal("read product_type failed!\n");
        return;
    }

    if(0 == strncmp(product_type, "MBB", strlen("MBB")))
    {
        g_product_type = DUMP_MBB;
    }
    else if(0 == strncmp(product_type, "PHONE", strlen("PHONE")))
    {
        g_product_type = DUMP_PHONE;
    }
    else
    {
        g_product_type = DUMP_PRODUCT_BUTT;
    }
}

/*****************************************************************************
* 函 数 名  : dump_get_product_type
* 功能描述  : 获取dump当前工作的产品形态
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
dump_product_type_t dump_get_product_type(void)
{
    if(DUMP_PHONE == g_product_type)
    {
        return DUMP_PHONE;
    }
    else
    {
        return DUMP_MBB;
    }
}

/*****************************************************************************
* 函 数 名  : dump_file_cfg_init
* 功能描述  : 获取当前需要保存的文件配置表
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
int dump_file_cfg_init(void)
{
    if(BSP_OK != bsp_nvm_read(NV_ID_DRV_DUMP_FILE, (u8 *)&g_dump_file_cfg, sizeof(DUMP_FILE_CFG_STRU)))
    {
        g_dump_file_cfg.file_cnt = 2;
        g_dump_file_cfg.file_list.file_bits.mdm_dump = 0x1;
        g_dump_file_cfg.file_list.file_bits.mdm_share= 0x1;
        g_dump_file_cfg.file_list.file_bits.mdm_ddr  = 0x1;
        g_dump_file_cfg.file_list.file_bits.mdm_etb  = 0x1;
        g_dump_file_cfg.file_list.file_bits.lphy_tcm = 0x1;
        g_dump_file_cfg.file_list.file_bits.lpm3_tcm = 0x1;
        g_dump_file_cfg.file_list.file_bits.ap_etb   = 0x1;
        g_dump_file_cfg.file_list.file_bits.reset_log= 0x1;

        dump_error("read nv 0x%x fail, use default config\n", NV_ID_DRV_DUMP_FILE);

        return BSP_ERROR;
    }
    return BSP_OK;
}

/*****************************************************************************
* 函 数 名  : dump_get_file_cfg
* 功能描述  : 获取当前需要保存的文件配置表
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
DUMP_FILE_CFG_STRU* dump_get_file_cfg(void)
{
    return &g_dump_file_cfg;
}


/*****************************************************************************
* 函 数 名  : dump_feature_init
* 功能描述  : 获取当前功能配置
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 dump_feature_init(void)
{
    s32 ret = BSP_OK;

    ret = bsp_nvm_read(NVID_DUMP, (u8*)&g_dump_cfg, sizeof(NV_DUMP_STRU));
    if (BSP_OK != ret)
    {
        /*使用默认值*/
        g_dump_cfg.dump_cfg.Bits.dump_switch     = 0x1;
        g_dump_cfg.dump_cfg.Bits.ARMexc          = 0x1;
        g_dump_cfg.dump_cfg.Bits.stackFlow       = 0x1;
        g_dump_cfg.dump_cfg.Bits.taskSwitch      = 0x1;
        g_dump_cfg.dump_cfg.Bits.intSwitch       = 0x1;
        g_dump_cfg.dump_cfg.Bits.intLock         = 0x1;
        g_dump_cfg.dump_cfg.Bits.appRegSave1     = 0x0;
        g_dump_cfg.dump_cfg.Bits.appRegSave2     = 0x0;
        g_dump_cfg.dump_cfg.Bits.appRegSave3     = 0x0;
        g_dump_cfg.dump_cfg.Bits.commRegSave1    = 0x0;
        g_dump_cfg.dump_cfg.Bits.commRegSave2    = 0x0;
        g_dump_cfg.dump_cfg.Bits.commRegSave3    = 0x0;
        g_dump_cfg.dump_cfg.Bits.sysErrReboot    = 0x1;
        g_dump_cfg.dump_cfg.Bits.reset_log       = 0x1;
        g_dump_cfg.dump_cfg.Bits.fetal_err       = 0x1;

        dump_error("read NV failed, use the default value!.ret = %d nv id = 0x%x\n", ret, NVID_DUMP);
        return ret;
    }

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名  : dump_get_feature_cfg
* 功能描述  : 获取当前特性
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
NV_DUMP_STRU* dump_get_feature_cfg(void)
{
    return &g_dump_cfg;
}

/*****************************************************************************
* 函 数 名  : dump_get_edition_type
* 功能描述  : 获取当前的版本类型
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
enum EDITION_KIND dump_get_edition_type(void)
{

    if(g_edition == EDITION_MAX)
    {
        g_edition = (enum EDITION_KIND)bbox_check_edition();
    }

    return g_edition;
}
/*****************************************************************************
* 函 数 名  : dump_config_init
* 功能描述  : dump 配置初始化
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_config_init(void)
{
    s32 ret ;
    
    dump_product_type_init();

    ret = dump_feature_init();
    if(BSP_OK != ret)
    {
        dump_error("dump_config_init fail\n");
    }

    ret = dump_file_cfg_init();
    if(BSP_OK != ret)
    {
        dump_error("bsp_om_debug_init fail\n");
    }

}


