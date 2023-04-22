


#include <bsp_nvim.h>
#include <bsp_blk.h>
#include <bsp_version.h>
#include "nv_index.h"
#include "nv_crc.h"
#include "nv_file.h"
#include "nv_ctrl.h"
#include "nv_cust.h"
#include "nv_index.h"
#include "nv_debug.h"
#include "nv_xml_dec.h"

bool nv_upgrade_xnv_compressed(void)
{
    nv_ctrl_info_s *ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    return (((ctrl_info->crc_mark & NV_CTRL_COMPRESS) == NV_CTRL_COMPRESS) ? true:false);
}

/*
 * xml decode
 * path     :xml path
 * map_path :xml map file path
 */
u32 nv_upgrade_dec_xml(s8* path,s8* map_path,u32 card_type)
{
    u32 ret = NV_ERROR;
    FILE* fp;

    nv_debug(NV_FUN_XML_DECODE,0,0,0,0);
    fp = nv_file_open(path,(s8*)NV_FILE_READ);
    if(NULL == fp)
    {
        ret = BSP_ERR_NV_NO_FILE;
        nv_debug(NV_FUN_XML_DECODE,1,ret,0,0);
        return ret;
    }
    nv_record("enter to decode  %s \n",path);
    ret = xml_decode_main(fp,map_path,card_type);
    nv_file_close(fp);
    if(ret)
    {
        nv_debug(NV_FUN_XML_DECODE,2,ret,0,0);
        goto xml_decode_err;
    }

    return NV_OK;
xml_decode_err:
    nv_record("file path :%s card type %d\n",path,card_type);
    nv_help(NV_FUN_XML_DECODE);
    return ret;
}


u32 nv_upgrade_dec_xml_all(void)
{
    u32 ret = NV_ERROR;

    if(!nv_file_access(NV_XNV_CARD1_PATH,0))
    {
        ret = nv_upgrade_dec_xml(NV_XNV_CARD1_PATH,NV_XNV_CARD1_MAP_PATH,NV_USIMM_CARD_1);
        if(ret)
        {
            return ret;
        }
    }

    if(!nv_file_access(NV_XNV_CARD2_PATH,0))
    {
        ret = nv_upgrade_dec_xml(NV_XNV_CARD2_PATH,NV_XNV_CARD2_MAP_PATH,NV_USIMM_CARD_2);
        if(ret)
        {
            return ret;
        }
    }

    if(!nv_file_access(NV_XNV_CARD3_PATH,0))
    {
        ret = nv_upgrade_dec_xml(NV_XNV_CARD3_PATH, NV_XNV_CARD3_MAP_PATH, NV_USIMM_CARD_3);
        if(ret)
        {
            return ret;
        }
    }

    /*CUST XML 无对应MAP文件，传入空值即可*/
    if(!nv_file_access(NV_CUST_CARD1_PATH,0))
    {
        ret = nv_upgrade_dec_xml(NV_CUST_CARD1_PATH,NULL,NV_USIMM_CARD_1);
        if(ret)
        {
            return ret;
        }
    }

    if(!nv_file_access(NV_CUST_CARD2_PATH,0))
    {
        ret = nv_upgrade_dec_xml(NV_CUST_CARD2_PATH,NULL,NV_USIMM_CARD_2);
        if(ret)
        {
            return ret;
        }
    }

    if(!nv_file_access(NV_CUST_CARD3_PATH,0))
    {
        ret = nv_upgrade_dec_xml(NV_CUST_CARD3_PATH, NULL, NV_USIMM_CARD_3);
        if(ret)
        {
            return ret;
        }
    }

    return NV_OK;
}

/* added by yangzhi for muti-carrier, End! */

u32 bsp_nvm_upgrade(void)
{
    u32 ret;
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    nv_debug(NV_FUN_UPGRADE_PROC,0,0,0,0);
    nv_record("Balong nv upgrade start!\n");

    /*判断fastboot阶段升级包文件解析异常，若出现异常，则需要重新解析升级包文件*/
    if(ddr_info->nvdload_boot_state != NV_BOOT_UPGRADE_SUCC_STATE)
    {   
        /* 解析xml nv或者解析压缩nv */
        if(nv_upgrade_xnv_compressed())
        {
            nv_record("fastboot cprs decode fail ,need kernel decode again!\n");
            ret =  nv_upgrade_customize();
        }
        else
        {
            nv_record("fastboot xml decode fail ,need kernel decode again!\n");
            ret = nv_upgrade_dec_xml_all();
        }
        
        if(ret)
        {
            nv_record("kernel cprs or xml decode failed 0x%x!\n", ret);
            nv_debug(NV_FUN_UPGRADE_PROC,1,ret,0,0);
            goto upgrade_fail_out;
        }
    }

    /* added by yangzhi for muti-carrier, Begin:*/

    /*升级恢复处理，烧片版本直接返回ok*/
    ret = nv_upgrade_revert_proc();
    if(ret)
    {
        nv_record("upgrade revert failed 0x%x!\n", ret);
        nv_debug(NV_FUN_UPGRADE_PROC,4,ret,0,0);
        goto upgrade_fail_out;
    }
    else
    {
        nv_record("upgrade revert success!\n");
    }
    /* added by yangzhi for muti-carrier, Begin:*/
    /* added by yangzhi for muti-carrier, End! */

    (void)nv_crc_make_ddr();
    nv_record("upgrade mkddr crc success!\n");

    /*将最新数据写入各个分区*/
    ret = nv_data_writeback();
    if(ret)
    {
        nv_record("upgrade writeback failed 0x%x!\n", ret);
        nv_debug(NV_FUN_UPGRADE_PROC,7,ret,0,0);
        goto upgrade_fail_out;
    }
    else
    {
        nv_record("upgrade writeback success!\n");
    }

    /*置升级包无效*/
    ret = nv_upgrade_set_flag((bool)false);
    if(ret)
    {
        nv_record("upgrade set dload packet invalid failed 0x%x!\n", ret);
        nv_debug(NV_FUN_UPGRADE_PROC,8,ret,0,0);
        goto upgrade_fail_out;
    }

    ret = nv_upgrade_file_updata();
    if(ret)
    {
        nv_record("upgrade nv_file_update failed 0x%x!\n", ret);
        nv_debug(NV_FUN_UPGRADE_PROC, 9,ret,0,0);
        goto upgrade_fail_out;
    }

    return NV_OK;
upgrade_fail_out:
    nv_record("\n%s\n",__func__);
    nv_help(NV_FUN_UPGRADE_PROC);
    return NV_ERROR;
}

u32 nv_upgrade_file_updata(void)
{
    u32 ret;
    
    ret = nv_file_update((s8*)NV_DLOAD_PATH);
    if(ret)
    {
        nv_record("updata %s file info fail.\n", (s8*)NV_DLOAD_PATH);
        return NV_ERROR;
    }

    if(!nv_file_access((s8*)NV_DLOAD_CUST_PATH,0))
    {
        ret = nv_file_update(NV_DLOAD_CUST_PATH);
        if(ret)
        {
            nv_record("updata %s file info fail.\n", (s8*)NV_DLOAD_CUST_PATH);
            return NV_ERROR;
        }
    }
    
    return NV_OK;
}

/*获取升级包数据有效性
 *true :有效 false: 无效
 */
bool nv_get_upgrade_flag(void)
{
    nv_file_map_s nvctrl_info = {0};
    s32 ret;

    ret = bsp_blk_read((char*)NV_DLOAD_SEC_NAME,(loff_t)0,&nvctrl_info,sizeof(nvctrl_info));
    if(ret)
    {
        return false;
    }

    if(nvctrl_info.magic_num == NV_FILE_EXIST)
    {
        return true;
    }
    return false;
}

/*修改升级包标志
 *true :有效   false :无效
 */

u32 nv_modify_upgrade_flag(bool flag)
{
    nv_file_map_s nvctrl_info = {0};
    s32 ret;
    u32 old_magic;
    u32 new_magic;

    ret = bsp_blk_read((char*)NV_DLOAD_SEC_NAME,(loff_t)0,&nvctrl_info,sizeof(nvctrl_info));
    if(ret)
    {
        return (u32)ret;
    }
    if(flag)
    {
        old_magic = NV_DLOAD_INVALID_FLAG;
        new_magic = NV_FILE_EXIST;
    }
    else
    {
        new_magic = NV_DLOAD_INVALID_FLAG;
        old_magic = NV_FILE_EXIST;
    }
    nv_printf("old_magic=0x%x.new_magic=0x%x.\n", old_magic, new_magic);
    nvctrl_info.magic_num = (nvctrl_info.magic_num == old_magic) ? new_magic : nvctrl_info.magic_num;
    ret = bsp_blk_write((char*)NV_DLOAD_SEC_NAME,(loff_t)0,&nvctrl_info,sizeof(nvctrl_info));
    if(ret)
    {
        return (u32)ret;
    }
    return 0;

}

bool nv_upgrade_get_flag(void)
{
    bool ret;
    u32 flag = false;

    if(nv_upgrade_xnv_compressed())
    {
        if(!nv_file_access((s8*)NV_DLOAD_PATH,0))
        {
            flag = (u32)nv_cust_get_upgrade_flag((s8*)NV_DLOAD_PATH);
        }

        if(!nv_file_access((s8*)NV_DLOAD_CUST_PATH,0))
        {
            flag |= (u32)nv_cust_get_upgrade_flag((s8*)NV_DLOAD_CUST_PATH);
        }

        ret = (bool)flag;
    }
    else
    {        
        ret = nv_get_upgrade_flag();
    }

    return ret;
}


u32 nv_upgrade_set_flag(bool flag)
{
    u32 ret = NV_ERROR;
    
    if(nv_upgrade_xnv_compressed())
    {
        if(!nv_file_access((s8*)NV_DLOAD_PATH,0))
        {
            ret = nv_cust_set_upgrade_flag((s8*)NV_DLOAD_PATH, flag);
        }

        if(!nv_file_access((s8*)NV_DLOAD_CUST_PATH,0))
        {
            ret |= nv_cust_set_upgrade_flag((s8*)NV_DLOAD_CUST_PATH, flag);
        }
    }
    else
    {
        ret = nv_modify_upgrade_flag(flag);
    }

    return ret;
}

EXPORT_SYMBOL(nv_upgrade_xnv_compressed);
EXPORT_SYMBOL(nv_upgrade_dec_xml);
EXPORT_SYMBOL(nv_upgrade_dec_xml_all);
EXPORT_SYMBOL(bsp_nvm_upgrade);
EXPORT_SYMBOL(nv_upgrade_get_flag);
EXPORT_SYMBOL(nv_upgrade_set_flag);

