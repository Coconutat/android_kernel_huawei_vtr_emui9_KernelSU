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
#include <product_nv_id.h>
#include <product_nv_def.h>
#include <osl_common.h>
#include <mdrv_nvim.h>
#include <bsp_nvim.h>
#include "nv_crc.h"
#include "nv_comm.h"
#include "nv_factory_check.h"

/*****************************************************************************
 函 数 名  : nv_make_single_band_nv_crc
 功能描述  : 计算单个band的校准nv的CRC校验码
 输入参数  : singal_band_nvid_s* check_item_temp : 待计算的band的nvid列表的起始地址
             u32 modem_num : 待计算的band所属的modem num
             u32 valild_nvid_count : 该band中可用nvid的最大数量
             u32 *last_crc_value : 上一次生成的CRC校验码
 输出参数  : u32 *gen_crc_value : 生成的CRC校验码
 返 回 值  : NV_OK 计算CRC校验码成功 其他:计算错误
*****************************************************************************/
u32 nv_make_single_band_nv_crc(singal_band_nvid_t* check_item_temp, u32 modem_num, u32 valild_nvid_count, u32 *last_crc_value, u32 *gen_crc_value)
{
    u8 * temp_nv_data;
    u32 * pdata;
    u32 ret = 0;
    u32 data_len;
    u32 nv_len;
    u32 nvid_index;
    u32 crc_value = 0;

    /*配置的可用nvid的数量不能大于可用nvid数量的最大值*/
    if(check_item_temp->uhwValidCount > valild_nvid_count)
    {
        nv_printf("nv list size is larger than stuct, uhwValidCount: 0x%x , struct support is:0x%x\n", check_item_temp->uhwValidCount, valild_nvid_count);
        return BSP_ERR_NV_OVER_MEM_ERR;
    }

    pdata = (u32 *)vmalloc(sizeof(u32) + 2*NV_MAX_UNIT_SIZE);
    if(!pdata)
    {
        nv_printf("malloc pdata fail\n");
        return BSP_ERR_NV_MALLOC_FAIL;
    }
    nv_len = 0;
    if(last_crc_value != NULL)
    {
        *pdata = *last_crc_value;
        data_len = sizeof(u32);
    }
    else
    {
        data_len = 0;
    }

    temp_nv_data = (u8 *)((unsigned long)pdata + data_len);
    for(nvid_index = 0; nvid_index < check_item_temp->uhwValidCount; nvid_index++)
    {
        /*读取待校验nv*/
        ret = bsp_nvm_get_len(check_item_temp->auhwNeedCheckID[nvid_index], &nv_len);
        if(ret)
        {
            nv_printf("get nv len error nvid: 0x%x fail, ret = 0x%x\n", check_item_temp->auhwNeedCheckID[nvid_index], ret);
            vfree((void *)pdata);
            return ret;
        }
        ret = bsp_nvm_dcread(modem_num, check_item_temp->auhwNeedCheckID[nvid_index], temp_nv_data, nv_len);
        if(ret)
        {
            nv_printf("read nv 0x%x fail, ret = 0x%x\n", check_item_temp->auhwNeedCheckID[nvid_index], ret);
            vfree((void *)pdata);
            return ret;
        }

        data_len += nv_len;

        nv_debug_printf("to cal crc data len 0x%x\n", data_len);
        nv_debug_printf("to cal crc data content:0x%x 0x%x 0x%x 0x%x 0x%x\n", pdata[0], pdata[1], pdata[2], pdata[3], pdata[4]);
        /*计算CRC校验码*/
        crc_value = nv_cal_crc32((u8 *)pdata, data_len);
        nv_debug_printf("after cal nvid: 0x%x crc value 0x%x\n", check_item_temp->auhwNeedCheckID[nvid_index] ,crc_value);

        *pdata = crc_value;
        data_len = sizeof(u32);
        temp_nv_data = (u8*)((unsigned long)pdata + data_len);
    }

    *gen_crc_value = crc_value;
    vfree((void *)pdata);
    return NV_OK;
}


u32 nv_check_mode_crc(factory_crc_check_info_t check_info)
{
    u32 index;
    u32 ret;
    singal_band_nvid_t * check_item_buff;
    singal_band_nvid_t * check_item_temp;
    u32 nv_len = 0;
    u32 last_crc_value = 0;
    u32 gen_crc_value = 0;
    u32 first_cal = 0;

    /*获取存放nv列表的nv的长度*/
    ret = bsp_nvm_get_len(check_info.list_nvid, &nv_len);
    if(ret)
    {
        nv_printf("get nv:0x%x len fail\n", check_info.list_nvid);
        return ret;
    }

    check_item_buff = (singal_band_nvid_t *)vmalloc((size_t)nv_len);
    if(!check_item_buff)
    {
        nv_printf("malloc fail,can not to read item nv\n");
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    ret = bsp_nvm_read(check_info.list_nvid, (u8 *)check_item_buff, nv_len);
    if(ret)
    {
        nv_printf("read check nv item fail nvid:0x%x\n", check_info.list_nvid);
        vfree(check_item_buff);
        return ret;
    }

    check_item_temp = check_item_buff;
    for(index = 0; index < check_info.valild_band_count; index++)
    {

        nv_debug_printf("next single band nv addr = %p\n", check_item_temp);

        /*如未使能该band的检查 或者配置可用数量为0，则不做检查*/
        if((SINGAL_NV_ENABLE != check_item_temp->uhwEnable)||(0 == check_item_temp->uhwValidCount))
        {
            nv_printf("index:0x%x single nv is not invalid, enable:0x%x validcount:0x%x\n", index, check_item_temp->uhwEnable, check_item_temp->uhwValidCount);
            check_item_temp = (singal_band_nvid_t *)((unsigned long)check_item_temp + check_info.singal_group_size);
            continue;
        }

        last_crc_value = gen_crc_value;
        if(0 == first_cal)
        {
            nv_debug_printf("cal first single band nv crc\n");
            ret = nv_make_single_band_nv_crc(check_item_temp, check_info.modem_num, check_info.valild_nvid_count, NULL, &gen_crc_value);
            first_cal = 1;
        }
        else
        {
            nv_debug_printf("cal not first single band nv crc\n");
            ret = nv_make_single_band_nv_crc(check_item_temp, check_info.modem_num, check_info.valild_nvid_count, &last_crc_value, &gen_crc_value);
        }
        if(ret)
        {
            nv_printf("make single band nv crc fail, ret = 0x%x\n", ret);
            vfree(check_item_buff);
            return ret;
        }

        check_item_temp = (singal_band_nvid_t *)((unsigned long)check_item_temp + check_info.singal_group_size);
    }

    vfree(check_item_buff);
    if(gen_crc_value == check_info.old_crc_value)
    {
        return NV_OK;
    }
    else
    {
        nv_printf("check crc fail\nnew crc:0x%x, old crc:0x%x\n", gen_crc_value, check_info.old_crc_value);
        return BSP_ERR_NV_CRC_CODE_ERR;
    }
}
/*****************************************************************************
 函 数 名  : nv_check_factory_nv_status
 功能描述  : 校验校准NV的CRC校验码
 输入参数  : u32 mode取值0: GU主卡 1:TL主卡 2:GU副卡
 输出参数  : 无
 返 回 值  : 0:校验通过
             0xxxxx027:CRC校验出错
             其他错误码执行过程中出错
*****************************************************************************/
u32 nv_check_factory_nv_status(u32 mode)
{
    NV_CRC_CHECK_RESULT_STRU crc_result_stru = {0};
    factory_crc_check_info_t check_info = {0};
    u32 ret;

    /*read crc value,nv 50000 is defined by product line*/
    ret = bsp_nvm_read(NV_ID_CRC_CHECK_RESULT, (u8*)&crc_result_stru, (u32)sizeof(NV_CRC_CHECK_RESULT_STRU));
    if(ret)
    {
        nv_printf("read crc result nv fail\n nv id:0x%x\n", NV_ID_CRC_CHECK_RESULT);
        return ret;
    }

    switch(mode)
    {
        case NV_FACTORY_CHECK_M1_GU:
            check_info.list_nvid = NV_ID_GUC_CHECK_ITEM;
            check_info.modem_num = NV_USIMM_CARD_1;
            check_info.old_crc_value = crc_result_stru.uwGucCrcResult;
            check_info.valild_band_count = MAX_NV_GUC_CHECK_ITEM_NUM;
            check_info.valild_nvid_count = MAX_SINGLE_GUC_BAND_CHECK_NUM;
            check_info.singal_group_size = sizeof(SINGLE_GUC_BAND_NV_ID_STRU);
            break;
        case NV_FACTORY_CHECK_M1_TL:
            check_info.list_nvid = NV_ID_TL_CHECK_ITEM;
            check_info.modem_num = NV_USIMM_CARD_1;
            check_info.old_crc_value = crc_result_stru.uwTlCrcResult;
            check_info.valild_band_count = MAX_NV_TL_CHECK_ITEM_NUM;
            check_info.valild_nvid_count = MAX_SINGLE_TL_BAND_CHECK_NUM;
            check_info.singal_group_size = sizeof(SINGLE_TL_BAND_NV_ID_STRU);
            break;
        case NV_FACTORY_CHECK_M2_GU:
            check_info.list_nvid = NV_ID_GUC_M2_CHECK_ITEM;
            check_info.modem_num = NV_USIMM_CARD_2;
            check_info.old_crc_value = crc_result_stru.uwGucM2CrcResult;
            check_info.valild_band_count = MAX_NV_GUC_CHECK_ITEM_NUM;
            check_info.valild_nvid_count = MAX_SINGLE_GUC_BAND_CHECK_NUM;
            check_info.singal_group_size = sizeof(SINGLE_GUC_BAND_NV_ID_STRU);
            break;
        default:
            nv_printf("para error, mode = 0x%x\n", mode);
            return BSP_ERR_NV_INVALID_PARAM;
    }

    return nv_check_mode_crc(check_info);
}

EXPORT_SYMBOL(nv_check_factory_nv_status);


