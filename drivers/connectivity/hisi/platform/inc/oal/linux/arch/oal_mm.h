

#ifndef __OAL_LINUX_MM_H__
#define __OAL_LINUX_MM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
/*lint -e322*/
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/vmalloc.h>
#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
#include <linux/dma-mapping.h>
#endif

/*lint +e322*/

/*****************************************************************************
  2 宏定义
*****************************************************************************/
typedef dma_addr_t  oal_dma_addr;


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    OAL_BIDIRECTIONAL = 0,
    OAL_TO_DEVICE = 1,
    OAL_FROM_DEVICE = 2,
    OAL_NONE = 3,
}oal_data_direction;
typedef oal_uint8 oal_direction_uint8;


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

OAL_STATIC OAL_INLINE oal_void* oal_memalloc(oal_uint32 ul_size)
{
    oal_int32   l_flags = GFP_KERNEL;
    oal_void   *puc_mem_space;

    /* 不睡眠或在中断程序中标志置为GFP_ATOMIC */
    if (in_interrupt() || irqs_disabled())
    {
        l_flags = GFP_ATOMIC;
    }

    puc_mem_space = kmalloc(ul_size, l_flags);

    if (OAL_PTR_NULL == puc_mem_space)
    {
        return OAL_PTR_NULL;
    }

    return puc_mem_space;
}

#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT

OAL_STATIC OAL_INLINE oal_void* oal_mem_uncache_alloc(oal_uint32 ul_size, oal_uint32 *pul_phy_addr)
{
    oal_int32   l_flags = GFP_KERNEL;
    oal_void   *puc_mem_space;
    oal_uint32  ul_dma_real_addr;

    /* 不睡眠或在中断程序中标志置为GFP_ATOMIC */
    if (in_interrupt() || irqs_disabled())
    {
        l_flags = GFP_ATOMIC;
    }

    puc_mem_space = dma_alloc_coherent(NULL, ul_size, &ul_dma_real_addr, l_flags);

    /* 保存非cache内存的物理地址 */
    *pul_phy_addr = (oal_uint32)ul_dma_real_addr;

    if (OAL_PTR_NULL == puc_mem_space)
    {
        return OAL_PTR_NULL;
    }

    return puc_mem_space;

}


OAL_STATIC OAL_INLINE oal_void oal_mem_uncache_free(oal_uint32 ul_size, oal_void *p_buf, oal_uint32 ul_dma_addr)
{
    dma_free_coherent(NULL, ul_size, p_buf, ul_dma_addr);
}


OAL_STATIC OAL_INLINE oal_uint32 oal_dma_map_single(struct device *pst_dev, oal_void *p_buf, oal_uint32 ul_size, oal_direction_uint8 uc_dir)
{
    return dma_map_single(pst_dev, p_buf, ul_size, uc_dir);
}


OAL_STATIC OAL_INLINE oal_void oal_dma_unmap_single(struct device *pst_dev, oal_dma_addr ul_addr, oal_uint32 ul_size, oal_direction_uint8 uc_dir)
{
    dma_unmap_single(pst_dev, ul_addr, ul_size, uc_dir);
}
#endif


OAL_STATIC OAL_INLINE oal_void  oal_free(oal_void *p_buf)
{
    kfree(p_buf);
}


OAL_STATIC OAL_INLINE oal_void oal_memcopy(oal_void *p_dst, const oal_void *p_src, oal_uint32 ul_size)
{
    memcpy(p_dst, p_src, ul_size);
}


OAL_STATIC OAL_INLINE oal_void  oal_memmove(oal_void *p_dst, const oal_void *p_src, oal_uint32 ul_size)
{
    memmove(p_dst, p_src, ul_size);
}


OAL_STATIC OAL_INLINE oal_void  oal_memset(oal_void *p_buf, oal_int32 l_data, oal_uint32 ul_size)
{
    memset(p_buf, l_data, ul_size);
}


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oal_mm.h */

