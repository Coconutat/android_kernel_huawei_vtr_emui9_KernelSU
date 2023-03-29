


#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include "bsp_nvim.h"
#include "bsp_version.h"
#include "nv_file.h"
#include "nv_ctrl.h"
#include "nv_cust.h"
#include "nv_index.h"
#include "nv_comm.h"

#define NV_FILE_UPGRADE_FLAG    NV_FILE_UPDATA

nv_customize_ctrl g_nv_bcust_ctrl;
nv_decompress_ctrl g_decprs_ctrl;

extern int zlib_inflate_blob(void *dst, unsigned dst_sz, const void *src, unsigned src_sz);
u32 nv_cust_decompress(s8* dst_buf, u32 max, s8* src_buf, u32 len)
{
	s8 *next_in = src_buf + 10;
	u32 avail_in = len - 10;
    
    return((u32)zlib_inflate_blob(dst_buf, max, next_in, avail_in));
}

u32 nv_cust_get_head_info(const s8 * file_name, nv_dload_head *dload_head)
{
    u32 ret;
    FILE *fp;
    
    /* check modem_nv.bin upgrade flag */
    fp = nv_file_open((s8*)file_name, (s8*)NV_FILE_READ);
    if(NULL == fp)
    {
        nv_record("Head_info: open %s file failed!\n", file_name);
        return BSP_ERR_NV_OPEN_FILE_FAIL;
    }

    ret = (u32)nv_file_read((u8*)dload_head, 1, (u32)sizeof(nv_dload_head), fp);
    (void)nv_file_close(fp);
    if(ret != sizeof(nv_dload_head))
    {
        nv_record("Head_info: read %s file dload head err!ret=0x%x.\n", file_name, ret);
        return BSP_ERR_NV_READ_FILE_FAIL;
    }

    return NV_OK;
}

u32 nv_cust_get_tail_info(const s8 * file_name, nv_dload_tail *dload_tail)
{
    u32 ret;
    u32 offset;
    FILE *fp;

    nv_dload_head dload_head;
    
    /* check modem_nv.bin upgrade flag */
    ret = nv_cust_get_head_info((s8*)file_name, &dload_head);
    if(ret)
    {
        nv_record("R_Tail_info: get %s file head info failed!\n", file_name);
        return BSP_ERR_NV_OPEN_FILE_FAIL;
    }

    if(dload_head.nv_bin.magic_num == NV_CTRL_FILE_MAGIC_NUM)
    {
        return NV_OK;
    }
    
    /*second count file total len*/
    offset = sizeof(nv_dload_head);
    offset += ((dload_head.nv_bin.magic_num == NV_FILE_EXIST)?dload_head.nv_bin.len:0);
    offset += ((dload_head.xnv_map.magic_num == NV_FILE_EXIST)?dload_head.xnv_map.len:0);
    offset += ((dload_head.xnv.magic_num == NV_FILE_EXIST)?dload_head.xnv.len:0);
    offset += ((dload_head.cust_map.magic_num == NV_FILE_EXIST)?dload_head.cust_map.len:0);
    offset += ((dload_head.cust.magic_num == NV_FILE_EXIST)?dload_head.cust.len:0);

    /* check modem_nv.bin upgrade flag */
    fp = nv_file_open((s8*)file_name, (s8*)NV_FILE_READ);
    if(NULL == fp)
    {
        nv_record("R_Tail_info: open %s file failed!\n", file_name);
        return BSP_ERR_NV_OPEN_FILE_FAIL;
    }

    (void)nv_file_seek(fp, offset, SEEK_SET);
    ret = (u32)nv_file_read((u8*)dload_tail, 1, (u32)sizeof(nv_dload_tail), fp);
    (void)nv_file_close(fp);
    if(ret != sizeof(nv_dload_tail))
    {
        nv_record("R_Tail_info: read %s file dload tail err!ret=0x%x\n", file_name, ret);
        return BSP_ERR_NV_READ_FILE_FAIL;
    }    

    return NV_OK;
}

u32 nv_cust_set_tail_info(const s8 * file_name, nv_dload_tail *dload_tail)
{
    u32 ret;
    u32 offset;
    FILE *fp;

    nv_dload_head dload_head;
    
    /* check modem_nv.bin upgrade flag */
    ret = nv_cust_get_head_info((s8*)file_name, &dload_head);
    if(ret)
    {
        nv_record("W_Tail_info: get %s file head info failed!\n", file_name);
        return BSP_ERR_NV_OPEN_FILE_FAIL;
    }

    if(dload_head.nv_bin.magic_num == NV_CTRL_FILE_MAGIC_NUM)
    {
        return NV_OK;
    }
    
    /*second count file total len*/
    offset = sizeof(nv_dload_head);
    offset += ((dload_head.nv_bin.magic_num == NV_FILE_EXIST)?dload_head.nv_bin.len:0);
    offset += ((dload_head.xnv_map.magic_num == NV_FILE_EXIST)?dload_head.xnv_map.len:0);
    offset += ((dload_head.xnv.magic_num == NV_FILE_EXIST)?dload_head.xnv.len:0);
    offset += ((dload_head.cust_map.magic_num == NV_FILE_EXIST)?dload_head.cust_map.len:0);
    offset += ((dload_head.cust.magic_num == NV_FILE_EXIST)?dload_head.cust.len:0);

    /* check modem_nv.bin upgrade flag */
    fp = nv_file_open((s8*)file_name, (s8*)NV_FILE_READ);
    if(NULL == fp)
    {
        nv_record("W_Tail_info: open %s file failed!\n", file_name);
        return BSP_ERR_NV_OPEN_FILE_FAIL;
    }

    (void)nv_file_seek(fp, offset, SEEK_SET);
    ret = (u32)nv_file_write((u8*)dload_tail, 1, (u32)sizeof(nv_dload_tail), fp);
    (void)nv_file_close(fp);
    if(ret != sizeof(nv_dload_tail))
    {
        nv_record("W_Tail_info: write %s file dload tail err!ret=0x%x\n", file_name, ret);
        return BSP_ERR_NV_READ_FILE_FAIL;
    }    

    return NV_OK;
}


bool nv_cust_get_upgrade_flag(const s8 *file_name)
{
    u32 ret;

    nv_dload_tail dload_tail = {0};

    ret = nv_cust_get_tail_info(file_name, &dload_tail);
    if(ret)
    {
        nv_record("Get %s file tail info err!\n", file_name);
        return false;
    }

    if(dload_tail.upgrade_flag == NV_FILE_UPGRADE_FLAG)
    {
        nv_printf("%s file is never upgrade!\n", file_name);
        return true;
    }

    nv_printf("%s file already updated!flag=0x%x.\n", file_name, dload_tail.upgrade_flag);
    
    return false;
}

u32 nv_cust_set_upgrade_flag(const s8 *file_name, bool flag)
{
    u32 ret;

    nv_dload_tail dload_tail;
    
    ret = nv_cust_get_tail_info(file_name, &dload_tail);
    if(ret)
    {
        nv_record("Get %s file tail info err!\n", file_name);
        return NV_ERROR;
    }

    if(flag)
    {
        dload_tail.upgrade_flag = NV_FILE_UPGRADE_FLAG;
    }
    else
    {
        dload_tail.upgrade_flag = NV_FILE_EXIST;
    }
    
    ret = nv_cust_set_tail_info(file_name, &dload_tail);
    if(ret)
    {
       nv_record("write %s file tail info err!\n", file_name); 
       return NV_ERROR;
    }

    return NV_OK;
}

bool nv_cust_file_valid(nv_cust_file_ctrl *file_ctrl)
{
    return (((file_ctrl->mmap.magic_num == NV_FILE_EXIST) && (file_ctrl->mbin.magic_num == NV_FILE_EXIST)) ? true:false);
}

u32 nv_cust_init_global(void)
{
    memset(&g_nv_bcust_ctrl, 0, sizeof(nv_customize_ctrl));

    g_nv_bcust_ctrl.src_buff = vmalloc((unsigned long)XNV_SEC_MAX);
    if(NULL == g_nv_bcust_ctrl.src_buff)
    {
        nv_record("malloc src buf failed!\n");
        return BSP_ERR_NV_MALLOC_FAIL;
    }
    
    g_nv_bcust_ctrl.dst_buff = vmalloc((unsigned long)XNV_SEC_MAX);
    if(NULL == g_nv_bcust_ctrl.dst_buff)
    {
        nv_record("malloc dst buf failed!\n");
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    return BSP_OK;
}

void nv_cust_clear(void)
{
    /* free nv data buff and nv inflate data buff */
    if(g_nv_bcust_ctrl.src_buff)
    {
        vfree(g_nv_bcust_ctrl.src_buff);
        g_nv_bcust_ctrl.src_buff = NULL;
    }

    if(g_nv_bcust_ctrl.dst_buff)
    {
        vfree(g_nv_bcust_ctrl.dst_buff);
        g_nv_bcust_ctrl.dst_buff = NULL;
    }

    /* free xnv product info buff */
    if(g_nv_bcust_ctrl.xnv_ctrl.prdt_info.prdt_info)
    {
        nv_free(g_nv_bcust_ctrl.xnv_ctrl.prdt_info.prdt_info);
        g_nv_bcust_ctrl.xnv_ctrl.prdt_info.prdt_info = NULL;
    }

    /* free xnv pooduct map buff  */
    if(g_nv_bcust_ctrl.xnv_ctrl.prdt_map.map_data)
    {
        nv_free(g_nv_bcust_ctrl.xnv_ctrl.prdt_map.map_data);
        g_nv_bcust_ctrl.xnv_ctrl.prdt_map.map_data = NULL;
    }

    /* free cust product info buff */
    if(g_nv_bcust_ctrl.cust_ctrl.prdt_info.prdt_info)
    {
        nv_free(g_nv_bcust_ctrl.cust_ctrl.prdt_info.prdt_info);
        g_nv_bcust_ctrl.cust_ctrl.prdt_info.prdt_info = NULL;
    }

    /* free cust pooduct map buff  */
    if(g_nv_bcust_ctrl.cust_ctrl.prdt_map.map_data)
    {
        nv_free(g_nv_bcust_ctrl.cust_ctrl.prdt_map.map_data);
        g_nv_bcust_ctrl.cust_ctrl.prdt_map.map_data = NULL;
    }
    return;
}


xnv_prdt_info* nv_cust_search_pinfo(nv_cust_prdt_info_ctrl *pinfo_ctrl, u32 prdt_id)
{
    u32 i;
    xnv_prdt_info *prdt_info = pinfo_ctrl->prdt_info;

    for(i=0; i<pinfo_ctrl->prdt_num; i++)
    {
        if(prdt_id == prdt_info[i].prdt_id)
        {
            return &prdt_info[i];
        }
    }

    return NULL;
}

u32 nv_cust_get_pinfo(FILE *fp, nv_cust_file_ctrl *file_ctrl)
{
    u32 ret;
    xnv_prdt_info          *pinfo;
    nv_cust_prdt_info_ctrl *pinfo_ctrl = &(file_ctrl->prdt_info);

    /* read xnv product info & search product map */
    pinfo_ctrl->prdt_num    = file_ctrl->map_file.prdt_num;
    pinfo_ctrl->offset      = file_ctrl->mmap.off + sizeof(xnv_map_file);
    pinfo_ctrl->length      = (u32)(pinfo_ctrl->prdt_num * sizeof(xnv_prdt_info));
    pinfo_ctrl->prdt_info   = (xnv_prdt_info*)nv_malloc((unsigned long)pinfo_ctrl->length);
    if(NULL == pinfo_ctrl->prdt_info)
    {
       return BSP_ERR_NV_MALLOC_FAIL;
    }
    
    nv_file_seek(fp, pinfo_ctrl->offset, SEEK_SET);
    ret = (u32)nv_file_read((u8*)pinfo_ctrl->prdt_info, 1, pinfo_ctrl->length, fp);
    if(pinfo_ctrl->length !=  ret)
    {
        return BSP_ERR_NV_READ_FILE_FAIL;
    }

    /* find need productID info */
    pinfo = nv_cust_search_pinfo(pinfo_ctrl, file_ctrl->prdt_id);
    if(NULL == pinfo)
    {
        return BSP_ERR_NV_CUST_PINFO_ERR;
    }

    memcpy(&pinfo_ctrl->valid_pinfo, pinfo, sizeof(xnv_prdt_info));

    return BSP_OK;
}

u32 nv_cust_get_pmap(FILE *fp, nv_cust_file_ctrl *file_ctrl)
{
    u32 ret;
    nv_cust_pmap_sec_ctrl *class_ctrl;
    nv_cust_pmap_sec_ctrl *cate_ctrl;
    nv_cust_pmap_sec_ctrl *prdt_ctrl;
    nv_cust_prdt_map_ctrl *pmap_ctrl = &(file_ctrl->prdt_map);
    xnv_prdt_info         *pinfo = &(file_ctrl->prdt_info.valid_pinfo);

    pmap_ctrl->map_offset = file_ctrl->mmap.off + pinfo->map_data_offset;
    pmap_ctrl->map_length = pinfo->map_data_len;
    pmap_ctrl->map_data   = nv_malloc((unsigned long)pmap_ctrl->map_length);
    
    if(NULL == pmap_ctrl->map_data)
    {
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    /* read xnv product map, class */
    nv_file_seek(fp, pmap_ctrl->map_offset, SEEK_SET);
    ret = (u32)nv_file_read(pmap_ctrl->map_data, 1, pmap_ctrl->map_length, fp);
    if(pmap_ctrl->map_length !=  ret)
    {
        return BSP_ERR_NV_READ_FILE_FAIL;
    }

    /*get class section map*/
    class_ctrl           = &pmap_ctrl->class_ctrl;
    class_ctrl->sec_data = (xnv_sec_data *)((uintptr_t)pmap_ctrl->map_data);
    class_ctrl->length   = (u32)(class_ctrl->sec_data->cnt * sizeof(xnv_sec_info) + sizeof(u32));

    /*get cate section map*/
    cate_ctrl            = &pmap_ctrl->cate_ctrl;
    cate_ctrl->sec_data  = (xnv_sec_data *)((uintptr_t)pmap_ctrl->map_data + class_ctrl->length);
    cate_ctrl->length    = (u32)(cate_ctrl->sec_data->cnt * sizeof(xnv_sec_info) + sizeof(u32));

    /*get product section map*/
    prdt_ctrl            = &pmap_ctrl->prdt_ctrl;
    prdt_ctrl->sec_data  = (xnv_sec_data *)((uintptr_t)pmap_ctrl->map_data + class_ctrl->length + cate_ctrl->length);
    prdt_ctrl->length    = (u32)(prdt_ctrl->sec_data->cnt * sizeof(xnv_sec_info) + sizeof(u32));

    return BSP_OK;
}

u32 nv_cust_search_iteminfo(u16 itemid, nv_ctrl_info_s* ctrl_info, nv_item_info_s** item_info)
{
    u32 low;
    u32 high;
    u32 mid;
    u32 offset;
    nv_item_info_s *item_table;

    offset = (u32)(NV_GLOBAL_CTRL_INFO_SIZE + NV_GLOBAL_FILE_ELEMENT_SIZE*ctrl_info->file_num);
    item_table = (nv_item_info_s*)((uintptr_t)ctrl_info + offset);

    low  = 1;
    high = ctrl_info->ref_count;
    while(low <= high)
    {
        mid = (low+high)/2;

        *item_info = &item_table[mid-1];

        if(itemid < (*item_info)->itemid)
        {
            high = mid-1;
        }
        else if(itemid > (*item_info)->itemid)
        {
            low = mid+1;
        }
        else
        {
            return NV_OK;
        }
    }

    *item_info = NULL;
    
    return BSP_ERR_NV_NO_THIS_ID;
}

u32 nv_cust_write_to_mem(nv_wr_req *wreq, nv_item_info_s *item_info)
{
    return nv_write_to_mem(wreq, item_info);
}

u32 nv_cust_analyze(xnv_sec *sec_file)
{
    u32 ret;
    u32 i;
    nv_ctrl_info_s *ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    u32 last_len = 0;
    xnv_item *item;

    /* print filename */
    sec_file->file_name[63] = '\0';

    item = sec_file->item;
    for(i=0; i<sec_file->nv_num; i++)
    {
        nv_item_info_s *src_item;
        u32 j;

        if(XNV_ITEM_MAGIC != item->magic_num)
        {
            nv_record("Customize: invalid magic! %s, %d, 0x%x, id=0x%x\n", sec_file->file_name, i, item->magic_num, item->id);
            return BSP_ERR_NV_CUST_SECMAGIC_ERR;
        }

        ret = nv_cust_search_iteminfo(item->id, ctrl_info, &src_item);
        if(ret)
        {
            nv_printf("Customize: nvid 0x%x isn't exist in common file\n", item->id);
            goto next;
        }

        if((item->modem_num > src_item->modem_num) || (0 == item->modem_num))
        {
            nv_record("Customize: invalid modem num! %d, %d, 0x%x, 0x%x\n", 
                    item->modem_num, src_item->modem_num, item->id, src_item->itemid);
            return BSP_ERR_NV_CUST_MDMNUM_ERR;
        }

        if((item->has_priority) && (item->priority < NV_BUTT_PRIORITY))
        {
            src_item->priority = item->priority;
        }

        for(j=0; j<item->modem_num; j++)
        {
            xnv_modem *modem = (xnv_modem*)((uintptr_t)(&item->modem_data[0]) + (item->length + sizeof(xnv_modem))*j);
            nv_wr_req wreq;
            
            wreq.itemid  = item->id;
            wreq.modemid = (modem->modem_id + 1);
            wreq.pdata   = modem->data;
            wreq.size    = item->length;
            wreq.offset  = 0;

            (void)nv_cust_write_to_mem(&wreq, src_item);
        }

next:
        last_len = sizeof(xnv_item) + (item->length + sizeof(xnv_modem))*item->modem_num;
        item = (xnv_item*)((uintptr_t)item + last_len);
    }

    return BSP_OK;
}

u32 nv_cust_sec_handle(FILE *fp, nv_cust_file_ctrl *file_ctrl, nv_cust_pmap_sec_ctrl *sec_ctrl)
{
    u32 ret;
    u32 i;
    u32 sec_cnt = sec_ctrl->sec_data->cnt;
    s8* src_buf = g_nv_bcust_ctrl.src_buff;
    s8* dst_buf = g_nv_bcust_ctrl.dst_buff;
    nv_file_map_s *fmap_bin     = &file_ctrl->mbin;
    xnv_sec_info *sec_info  = sec_ctrl->sec_data->map;

    /*  */
    for(i=0; i<sec_cnt; i++)
    {
        xnv_sec_info *cur_sec = &sec_info[i];
        u32 offset = cur_sec->offset + fmap_bin->off;
         
        if(cur_sec->offset + cur_sec->length > fmap_bin->len)
        {
            nv_record("Customize: sec_info too long, 0x%x, 0x%x, 0x%x!\n", i, cur_sec->offset, cur_sec->length);
            return BSP_ERR_NV_CUST_SECINFO_ERR;
        }

        nv_file_seek(fp, offset, SEEK_SET);       
        ret = (u32)nv_file_read((u8*)src_buf, 1, cur_sec->length, fp);
        if(cur_sec->length != ret)
        {
            nv_record("Customize: file read failed, 0x%x, 0x%x!\n", i, ret);
            return BSP_ERR_NV_READ_FILE_FAIL;
        }

        ret = nv_cust_decompress(dst_buf, XNV_SEC_MAX, src_buf, cur_sec->length);
        if(ret != cur_sec->src_length)
        {
            nv_record("Customize: decompress err, src_len=0x%x, ret=0x%x.!\n", cur_sec->src_length, ret);
            return ret;
        }

        ret = nv_cust_analyze((xnv_sec*)((uintptr_t)dst_buf));
        if(ret)
        {
            nv_record("Customize: analyse failed, 0x%x, 0x%x!\n", i, ret);
            return ret;
        }
    }

    return BSP_OK;
}

u32 nv_cust_handle(FILE *fp, nv_cust_file_ctrl *file_ctrl)
{
    u32 ret;
    nv_cust_pmap_sec_ctrl   *sec_ctrl;

    /* customize class data */
    sec_ctrl = &file_ctrl->prdt_map.class_ctrl;
    ret = nv_cust_sec_handle(fp, file_ctrl, sec_ctrl);
    if(ret)
    {
        nv_record("Customize: class failed, 0x%x!\n", ret);
        return ret;
    }

    /* customize cate data */
    sec_ctrl = &file_ctrl->prdt_map.cate_ctrl;
    ret = nv_cust_sec_handle(fp, file_ctrl, sec_ctrl);
    if(ret)
    {
        nv_record("Customize: catagory failed, 0x%x!\n", ret);
        return ret;
    }

    /* customize product data */
    sec_ctrl = &file_ctrl->prdt_map.prdt_ctrl;
    ret = nv_cust_sec_handle(fp, file_ctrl, sec_ctrl);
    if(ret)
    {
        nv_record("Customize: product failed, 0x%x!\n", ret);
        return ret;
    }

    return BSP_OK;
}

u32 nv_cust_file(FILE *fp, u32 prdt_id, nv_cust_file_ctrl *file_ctrl)
{
    u32 ret;

    if((file_ctrl == NULL) || (fp == NULL))
    {
        nv_record("Customize: file_ctrl or fp is null.\n");
        return BSP_ERR_NV_INVALID_PARAM;
    }
    
    file_ctrl->prdt_id = prdt_id;
    
    /* read xnv map head & get product num */
    nv_file_seek(fp, file_ctrl->mmap.off, SEEK_SET);
    ret = (u32)nv_file_read((u8*)(&file_ctrl->map_file), 1, (u32)sizeof(xnv_map_file), fp);
    if(sizeof(xnv_map_file) !=  ret)
    {
        nv_record("Customize: file read failed, 0x%x!\n", ret);
        return BSP_ERR_NV_READ_FILE_FAIL;
    }

    /* get product info */
    ret = nv_cust_get_pinfo(fp, file_ctrl);
    if(ret)
    {
        nv_record("Customize: get product info failed, 0x%x!\n", ret);
        return BSP_ERR_NV_CUST_PINFO_ERR;
    }

    /* get product map */
    ret = nv_cust_get_pmap(fp, file_ctrl);
    if(ret)
    {
        nv_record("Customize: get product map failed, 0x%x!\n", ret);
        return BSP_ERR_NV_CUST_PMAP_ERR;
    }

    /* start customize handle, class/cate/product */
    ret = nv_cust_handle(fp, file_ctrl);
    if(ret)
    {
        nv_record("Customize: cust failed, 0x%x!\n", ret);
        return ret;
    }

    return BSP_OK;
}

void nv_cust_help(void)
{
    nv_printf("g_nv_bcust_ctrl:product_id = 0x%x.nvctrl.bin len=0x%x.\n", g_nv_bcust_ctrl.product_id, g_nv_bcust_ctrl.dload_head.nv_bin.len);

    /* nv.bin head */
    nv_printf("g_nv_bcust_ctrl.dload_head.xnv.map:magic_num = 0x%x, offfset = 0x%x, len = 0x%x.\n",
                                g_nv_bcust_ctrl.dload_head.xnv_map.magic_num,
                                    g_nv_bcust_ctrl.dload_head.xnv_map.off,
                                        g_nv_bcust_ctrl.dload_head.xnv_map.len);
    
    nv_printf("g_nv_bcust_ctrl.dload_head.xnv.bin:magic_num = 0x%x, offfset = 0x%x, len = 0x%x.\n",
                                g_nv_bcust_ctrl.dload_head.xnv.magic_num,
                                        g_nv_bcust_ctrl.dload_head.xnv.off,
                                            g_nv_bcust_ctrl.dload_head.xnv.len);

    /* xnv.map和xnv.bin head */
    nv_printf("g_nv_bcust_ctrl.xnv_ctrl.mmap: magic_num = 0x%x, offset = 0x%x, len = 0x%x.\n", 
                                g_nv_bcust_ctrl.xnv_ctrl.mmap.magic_num, 
                                        g_nv_bcust_ctrl.xnv_ctrl.mmap.off, 
                                             g_nv_bcust_ctrl.xnv_ctrl.mmap.len);
    nv_printf("g_nv_bcust_ctrl.xnv_ctrl.mbin: magic_num = 0x%x, offset = 0x%x, len = 0x%x.\n", 
                                g_nv_bcust_ctrl.xnv_ctrl.mbin.magic_num, 
                                        g_nv_bcust_ctrl.xnv_ctrl.mbin.off, 
                                             g_nv_bcust_ctrl.xnv_ctrl.mbin.len);

    /* product info */
    nv_printf("g_nv_bcust_ctrl.cust_ctrl.prdt_info.valid_pinfo:id = 0x%x,offset = 0x%x,len = 0x%x.\n", 
                                g_nv_bcust_ctrl.xnv_ctrl.prdt_info.valid_pinfo.prdt_id, 
                                    g_nv_bcust_ctrl.xnv_ctrl.prdt_info.valid_pinfo.map_data_offset,
                                        g_nv_bcust_ctrl.xnv_ctrl.prdt_info.valid_pinfo.map_data_len);

    return;    
}

u32 nv_upgrade_modemnv(u32 prdt_id)
{
    u32 ret;
    FILE *fp = NULL;

    nv_dload_head *dload_head;

    /* init global */
    ret = nv_cust_init_global();
    if(ret)
    {
        nv_record("Customize Modem:Init global err!\n");
        goto out;
    }

    /* open modem_nv.bin file */
    fp = nv_file_open((s8*)NV_DLOAD_PATH,(s8*)NV_FILE_READ);
    if(NULL == fp)
    {
        nv_record("Customize Modem: open file failed!\n");
        ret = BSP_ERR_NV_OPEN_FILE_FAIL;
        goto out;
    }

    dload_head = &g_nv_bcust_ctrl.dload_head;
    ret = (u32)nv_file_read((u8*)dload_head, 1, (u32)sizeof(nv_dload_head), fp);
    if(ret != sizeof(nv_dload_head))
    {
        nv_record("Customize Modem: read dload head err!\n");
        ret = BSP_ERR_NV_READ_FILE_FAIL;
        goto out;
    }
    
    if((dload_head->cust_map.magic_num == NV_FILE_EXIST) || (dload_head->cust.magic_num == NV_FILE_EXIST))
    {
        nv_record("Customize Modem: Not is modem_nv.bin");
        ret = BSP_ERR_NV_FILE_ERROR;
        goto out;
    }

     /* cust file customize */
    g_nv_bcust_ctrl.product_id = prdt_id;
    memcpy((s8 *)&g_nv_bcust_ctrl.xnv_ctrl.mmap, (s8 *)&dload_head->xnv_map, sizeof(nv_file_map_s));
    memcpy((s8 *)&g_nv_bcust_ctrl.xnv_ctrl.mbin, (s8 *)&dload_head->xnv, sizeof(nv_file_map_s));
    if(nv_cust_file_valid(&g_nv_bcust_ctrl.xnv_ctrl))
    {
        ret = nv_cust_file(fp, g_nv_bcust_ctrl.product_id, &g_nv_bcust_ctrl.xnv_ctrl);
        if(ret)
        {
            nv_record("Customize Modem: cust file failed!\n");
        }
    }
    else
    {
        /* xnv必须有效，防止把错误遗漏过去 */
        nv_record("Customize Modem: xnv file isn't exist, failed!\n");
        ret = BSP_ERR_NV_CUST_NOXNV_ERR;
    }

out:    
    if(fp){
        nv_file_close(fp);
    }
    nv_cust_clear();
    
    return ret; 
}

u32 nv_upgrade_custnv(u32 prdt_id)
{
    u32 ret;
    FILE *fp = NULL;

    nv_dload_head *dload_head;

    /* init global */
    ret = nv_cust_init_global();
    if(ret)
    {
        nv_record("Customize Cust:Init global err!\n");
        goto out;
    }

    /* open modem_nv.bin file */
    fp = nv_file_open((s8*)NV_DLOAD_CUST_PATH,(s8*)NV_FILE_READ);
    if(NULL == fp)
    {
        nv_record("Customize Cust: open file failed!\n");
        ret = BSP_ERR_NV_OPEN_FILE_FAIL;
        goto out;
    }

    dload_head = &g_nv_bcust_ctrl.dload_head;
    ret = (u32)nv_file_read((u8*)dload_head, 1, (u32)sizeof(nv_dload_head), fp);
    if(ret != sizeof(nv_dload_head))
    {
        nv_record("Customize Cust: read dload head err!\n");
        ret = BSP_ERR_NV_READ_FILE_FAIL;
        goto out;
    }
    
    if((dload_head->nv_bin.off != 0) || (dload_head->nv_bin.len != 0))
    {
        nv_record("Customize Cust: Not is modem_nv.bin");
        ret = BSP_ERR_NV_FILE_ERROR;
        goto out;
    }

        
    /* cust file customize */
    g_nv_bcust_ctrl.product_id = prdt_id;    
    memcpy((s8 *)&g_nv_bcust_ctrl.xnv_ctrl.mmap, (s8 *)&dload_head->xnv_map, sizeof(nv_file_map_s));
    memcpy((s8 *)&g_nv_bcust_ctrl.xnv_ctrl.mbin, (s8 *)&dload_head->xnv, sizeof(nv_file_map_s));
    if(nv_cust_file_valid(&g_nv_bcust_ctrl.xnv_ctrl))
    {
        ret = nv_cust_file(fp, g_nv_bcust_ctrl.product_id, &g_nv_bcust_ctrl.xnv_ctrl);
        if(ret)
        {
            nv_record("Customize Cust: cust file failed!\n");
            goto out;
        }
    }   
    else
    {
        /* xnv必须有效，暂时不支持xnv不存在的情况，防止把错误遗漏过去 */
        nv_record("Customize Cust: xnv file isn't exist, failed!\n");
        ret = BSP_ERR_NV_CUST_NOXNV_ERR;
        goto out;
    }

    /* cust file customize */
    g_nv_bcust_ctrl.product_id = prdt_id;
    memcpy((s8 *)&g_nv_bcust_ctrl.cust_ctrl.mmap, (s8 *)&dload_head->cust_map, sizeof(nv_file_map_s));
    memcpy((s8 *)&g_nv_bcust_ctrl.cust_ctrl.mbin, (s8 *)&dload_head->cust, sizeof(nv_file_map_s));
    if(nv_cust_file_valid(&g_nv_bcust_ctrl.cust_ctrl))
    {
        ret = nv_cust_file(fp, g_nv_bcust_ctrl.product_id, &g_nv_bcust_ctrl.cust_ctrl);
        if(ret)
        {
            nv_record("Customize: cust file failed!\n");
        }
    }
    
out:    
    if(fp){
        nv_file_close(fp);
    }
    nv_cust_clear();
    
    return ret; 
}

u32 nv_upgrade_customize(void)
{
    u32 ret;
    u32 product_id;
    u32 flag_modem = false;
    u32 flag_cust = false;
    
    /* get product_id */
    product_id = bsp_get_version_info()->board_id;
    nv_printf("Customize:  product id = 0x%x\n", product_id);

    /* open cust file */
    if(nv_cust_get_upgrade_flag((s8 *)NV_DLOAD_PATH))
    {
        ret = nv_upgrade_modemnv(product_id);
        if((NV_OK != ret) && (BSP_ERR_NV_CUST_PINFO_ERR != ret))
        {
            nv_record("Customize: upgrade modem_nv.bin fail.\n");
            nv_cust_help();
            return NV_ERROR;
        }
        else if(NV_OK == ret)
        {
            flag_modem = true;
        }
    }
    
    if(nv_cust_get_upgrade_flag((s8 *)NV_DLOAD_CUST_PATH))
    {
        ret = nv_upgrade_custnv(product_id);
        if((NV_OK != ret) && (BSP_ERR_NV_CUST_PINFO_ERR != ret))
        {
            nv_record("Customize: upgrade cust_nv.bin fail.\n");
            nv_cust_help();
            return NV_ERROR;
        }
        else if(NV_OK == ret)
        {
            flag_cust = true;
        }
    }

    if((flag_modem == true) && (flag_cust == true))
    {
        nv_record("Customize:nv_modem.bin and nv_cust.bin have same product id!\n");
        ret = NV_ERROR;
    }
    else if((flag_modem == false) && (flag_cust == false))
    {
        nv_record("Customize:nv_modem.bin and nv_cust.bin have not !\n");
        ret = NV_ERROR;
    }
    else
    {
        nv_record("Customize:upgrade sucessfull!\n");
        ret = NV_OK;          
    }

    return ret;
}

u32 nv_upgrade_mcarrier_customize(const s8 * mcar_path)
{
    u32 ret;
    FILE *fp = NULL;
    nv_dload_head *dload_head;

    /* get product_id */
    g_nv_bcust_ctrl.product_id = bsp_get_version_info()->board_id;
    nv_printf("Customize:  product id = 0x%x\n", g_nv_bcust_ctrl.product_id);
    
    /* init global */
    ret = nv_cust_init_global();
    if(ret == NV_OK)
    {
        ret = NV_ERROR;
        fp = nv_file_open((s8*)mcar_path,(s8*)NV_FILE_READ);
        if(NULL != fp)
        {
            nv_printf("open %s file sucess.\n", (s8 *)mcar_path);
            ret = NV_OK;
        }
    }

    dload_head = &g_nv_bcust_ctrl.dload_head;
    if(ret == NV_OK)
    {
        ret = (u32)nv_file_read((u8*)dload_head, 1, (u32)sizeof(nv_dload_head), fp); 
        if(ret == sizeof(nv_dload_head))
        {
            ret = NV_OK;
        }
    }

    if(ret == NV_OK)
    {
        ret = NV_ERROR;
        /* check mcarrier file valid */
        if((dload_head->nv_bin.magic_num != NV_FILE_EXIST) &&
                (dload_head->xnv_map.magic_num != NV_FILE_EXIST) &&
                        (dload_head->xnv.magic_num != NV_FILE_EXIST))
        {
             nv_printf("Customize: is mcarrier cust file!");
             ret = NV_OK;
        }
    } 

    if(ret == NV_OK)
    {
        ret = NV_ERROR;
        memcpy((s8 *)&g_nv_bcust_ctrl.cust_ctrl.mmap, (s8 *)&dload_head->cust_map, sizeof(nv_file_map_s));
        memcpy((s8 *)&g_nv_bcust_ctrl.cust_ctrl.mbin, (s8 *)&dload_head->cust, sizeof(nv_file_map_s));
            
        /* cust file customize */
        if(nv_cust_file_valid(&g_nv_bcust_ctrl.cust_ctrl))
        {   
            ret = nv_cust_file(fp, g_nv_bcust_ctrl.product_id, &g_nv_bcust_ctrl.cust_ctrl);      
        }
    }

    if(ret == NV_OK)
    {
        nv_record("Customize: Upgrade mcarrier cust file sucess");
    }
    nv_printf("Customize: Upgrade mcarrier customize end.ret = 0x%x.\n", ret);

    if(NULL != fp){
        nv_file_close(fp);
    }
    nv_cust_help();
    nv_cust_clear();
    return ret;  
}

EXPORT_SYMBOL(g_nv_bcust_ctrl);
EXPORT_SYMBOL(nv_cust_init_global);
EXPORT_SYMBOL(nv_cust_clear);
EXPORT_SYMBOL(nv_cust_analyze);
EXPORT_SYMBOL(nv_cust_file);
EXPORT_SYMBOL(nv_upgrade_customize);
EXPORT_SYMBOL(nv_upgrade_modemnv);
EXPORT_SYMBOL(nv_upgrade_custnv);
EXPORT_SYMBOL(nv_upgrade_mcarrier_customize);

