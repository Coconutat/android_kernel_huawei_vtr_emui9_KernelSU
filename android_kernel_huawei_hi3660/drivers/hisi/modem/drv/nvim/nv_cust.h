

#ifndef __NV_CUST_CPRS_H__
#define __NV_CUST_CPRS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <bsp_shared_ddr.h>
#include <bsp_nvim.h>

typedef struct _nv_decompress_ctrl{
    u8 *obuf;
    u8 *tail;
    u8 *pcur;
}nv_decompress_ctrl;

/*************************** nv 产品定制管理结构体 **********************************/
typedef struct _nv_cust_prdt_info_ctrl{
    xnv_prdt_info      *prdt_info;
    xnv_prdt_info       valid_pinfo;
    u32                 prdt_num;
    u32                 offset;
    u32                 length;
}nv_cust_prdt_info_ctrl;

typedef struct _nv_cust_pmap_sec_ctrl{
    xnv_sec_data       *sec_data;
    u32                 length;
}nv_cust_pmap_sec_ctrl;

typedef struct _nv_cust_prdt_map_ctrl{
    u8*                 map_data;
    u32                 map_offset;
    u32                 map_length;
    nv_cust_pmap_sec_ctrl   class_ctrl;
    nv_cust_pmap_sec_ctrl   cate_ctrl;
    nv_cust_pmap_sec_ctrl   prdt_ctrl;
}nv_cust_prdt_map_ctrl;

typedef struct _nv_cust_file_ctrl{
    nv_file_map_s       mmap;
    nv_file_map_s       mbin;
    xnv_map_file        map_file;
    xnv_bin_file        bin_file;
    nv_cust_prdt_info_ctrl  prdt_info;
    nv_cust_prdt_map_ctrl   prdt_map;
    u32                 prdt_id;
}nv_cust_file_ctrl;

typedef struct _nv_customize_ctrl{
    u32                 product_id;
    s8                 *src_buff;
    s8                 *dst_buff;
    nv_dload_head       dload_head;
    nv_cust_file_ctrl   xnv_ctrl;
    nv_cust_file_ctrl   cust_ctrl;
}nv_customize_ctrl;

/*************************************函数及变量声明**************************************/
extern nv_customize_ctrl g_nv_bcust_ctrl;

u32 nv_cust_init_global(void);
void nv_cust_clear(void);
u32 nv_cust_analyze(xnv_sec *sec_file);
u32 nv_cust_file(FILE *fp, u32 prdt_id, nv_cust_file_ctrl *file_ctrl);
u32 nv_upgrade_mcarrier_customize(const s8 * mcar_path);
u32 nv_upgrade_customize(void);
u32 nv_upgrade_modemnv(u32 prdt_id);
u32 nv_upgrade_custnv(u32 prdt_id);
bool nv_cust_get_upgrade_flag(const s8 *file_name);
u32 nv_cust_set_upgrade_flag(const s8 *file_name, bool flag);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif






