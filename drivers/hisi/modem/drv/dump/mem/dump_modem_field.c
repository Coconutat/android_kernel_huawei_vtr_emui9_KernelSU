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

#include <product_config.h>
#include "bsp_dump_mem.h"
#include "dump_modem_field.h"
#include "dump_modem_area.h"
#include "dump_print.h"

struct dump_field_ctrl_info_s   g_st_field_ctrl;
u32    g_dump_mem_init = 0;

/*****************************************************************************
* 函 数 名  : dump_get_area_info
* 功能描述  : 验证field_id是否合法，异常id返回BSP_ERROR，正常id返回area id
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
 u32 dump_get_areaid_by_fieldid(u32 field_id)
{
    return DUMP_AREA_CP;
}
/*****************************************************************************
* 函 数 名  : dump_get_cp_save_done
* 功能描述  : 判断c核log是否保存完成
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 dump_get_cp_save_done(void)
{
    return BSP_OK;
}

/*****************************************************************************
* 函 数 名  : bsp_dump_get_field_addr
* 功能描述  : 获取field的地址
*
* 输入参数  :field_id
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
u8 * bsp_dump_get_field_addr(u32 field_id)
{

    return NULL;
}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_field_map
* 功能描述  : 获取field的field信息
*
* 输入参数  :field_id
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
u8 * bsp_dump_get_field_map(u32 field_id)
{

    return NULL;
}

/*****************************************************************************
* 函 数 名  : bsp_dump_register_field
* 功能描述  : 申请field信息
            1. 不带地址注册，传入参数时virt_addr,phy_addr必须传0，成功返回dump注册地址
            2. 自带地址注册，传入参数时phy_addr为自带物理地址，virt_addr为虚拟地址，同时在dump内存中分配相同大小内存，成功返回邋virt_addr

            3. 两种注册方式，都将在dump划分内存，对于自带地址的注册方式，在系统异常时，由dump模块做数据拷贝
            4. 每个注册区域需要由使用者传入对应的版本号，高8位为主版本号，低8位为次版本号
* 输入参数  :field_id
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
u8 * bsp_dump_register_field(u32 field_id, char * name, void * virt_addr, void * phy_addr, u32 length, u16 version)
{

    return NULL;
}

/*****************************************************************************
* 函 数 名  : bsp_dump_save_self_addr
* 功能描述  : 保存自注册的空间
*
* 输入参数  :field_id
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void bsp_dump_save_self_addr(void)
{


    return ;
}

/*****************************************************************************
* 函 数 名  : bsp_dump_field_init
* 功能描述  : cp dump fileld初始化
*
* 输入参数  :field_id
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 bsp_dump_field_init(void)
{

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名  : bsp_dump_get_field_phy_addr
* 功能描述  : 获取field的物理地址
*
* 输入参数  :field_id
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
u8 * bsp_dump_get_field_phy_addr(u32 field_id)
{

    return NULL;
}



/*****************************************************************************
* 函 数 名  : modem_dump_field_init
* 功能描述  : 初始化modem ap需要使用filed 空间
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/

void dump_mdmap_field_init(void)
{

}

/*****************************************************************************
* 函 数 名  : bsp_dump_mem_init
* 功能描述  : modem ap 可维可测空间初始化
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 bsp_dump_mem_init(void)
{
    if(g_dump_mem_init == 1)
    {
        return BSP_OK;
    }
    if(dump_area_init())
    {
        return BSP_ERROR;
    }

    if(bsp_dump_field_init())
    {
        return BSP_ERROR;
    }

    dump_fetal("bsp_dump_mem_init finish\n");
    g_dump_mem_init = 1;

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名  : dump_show_field
* 功能描述  : 调试接口，显示当前已经注册field使用情况
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_show_field(void)
{

}

arch_initcall(bsp_dump_mem_init);

