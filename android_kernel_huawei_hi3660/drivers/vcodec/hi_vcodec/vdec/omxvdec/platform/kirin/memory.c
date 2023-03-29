


#include <linux/math64.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/file.h>
#include <linux/major.h>
#include <linux/syscalls.h>
#include <linux/dma-buf.h>

#include "omxvdec.h"
#include "platform.h"
#include "memory.h"
#include "vfmw.h"

/*lint -e774*/
/********************************** MACRO *************************************/
#define MAX_ION_MEM_NODE  (800)
#define MAX_CMA_MEM_NODE  (200)
#define MAX_MEM_NAME_LEN  (15)
#define CLIENT_BUF_NAME   "CLIENT"

extern OMXVDEC_ENTRY *g_pOmxVdec;
/********************************* STRUCT *************************************/
typedef struct
{
    HI_U8    is_secure;
    HI_U8    is_cached;
    HI_U8    is_contiguous;
    HI_U32   phys_addr;
    HI_VOID *virt_addr;
	HI_U32   size;
    HI_CHAR  node_name[MAX_MEM_NAME_LEN];
    HI_CHAR  zone_name[MAX_MEM_NAME_LEN];
    struct ion_handle *handle;
}ION_MEM_DESC_S;

typedef struct
{
    HI_U32   phys_addr;
    HI_VOID *virt_addr;
	HI_U32   size;
    HI_CHAR  node_name[MAX_MEM_NAME_LEN];
    HI_CHAR  zone_name[MAX_MEM_NAME_LEN];
}CMA_MEM_DESC_S;


/*********************************** VARS *************************************/
struct mutex   gMemMut;
struct ion_client *gIONClient  = HI_NULL;
HI_S32             gIONNodeNum = 0;
ION_MEM_DESC_S     gIONMemNode[MAX_ION_MEM_NODE];
HI_S32             gCMANodeNum = 0;
CMA_MEM_DESC_S     gCMAMemNode[MAX_CMA_MEM_NODE];


/******************************** LOCAL FUNC **********************************/
/*HI_VOID VDEC_MEM_DumpIONMem(HI_VOID)
{
    HI_S32 i;

    printk(KERN_INFO "------------------------- ION Memory -------------------------\n");
    printk(KERN_INFO "TotalNum         :%-10d\n\n", gIONNodeNum);
    if (gIONNodeNum > 0)
    {
	    printk(KERN_INFO " %-13s%-10s%-10s%-10s%-20s\n\n", "NAME", "OWNER", "PHYADDR", "SIZE", "VIRADDR");
    	for (i=0; i<MAX_ION_MEM_NODE; i++)
    	{
    	    if (gIONMemNode[i].phys_addr != 0)
    	    {
                printk(KERN_INFO " %-13s%-10s%-10x%-10d%-20pK",
                          gIONMemNode[i].node_name,
                          gIONMemNode[i].zone_name,
                          gIONMemNode[i].phys_addr,
                          gIONMemNode[i].size,
                          gIONMemNode[i].virt_addr);

                printk(KERN_INFO " %s %s %s",
                          (1==gIONMemNode[i].is_secure)    ? "(SEC)":"",
                          (1==gIONMemNode[i].is_cached)    ? "(CAH)":"",
                          (1==gIONMemNode[i].is_contiguous)? "(CTG)":"");

                printk(KERN_INFO "\n");
    	    }
    	}
        printk(KERN_INFO "\n--------------------------------------------------------------\n");
    }

    return;
}*/

/*----------------------------------------
    func: get PhyAddr on kernel 4.9
 ----------------------------------------*/
static HI_S32 VDEC_MEM_ion_phys(struct ion_client *client, struct ion_handle *handle, struct device *dev,
                 ion_phys_addr_t *addr, size_t *len)
{
    int ret = 0;
    if (NULL == client || NULL == handle) {
        OmxPrint(OMX_ERR, "%s hisifb_ion_phys NULL Pointer!\n", __func__);
        return HI_FAILURE;
    }

    struct sg_table *table = NULL;
    struct dma_buf *buf = NULL;
    struct dma_buf_attachment *attach = NULL;
    int shared_fd = 0;
    shared_fd = ion_share_dma_buf_fd(client, handle);
    if (shared_fd < 0) {
        OmxPrint(OMX_ERR, "%s Failed to share ion buffer(0x%pK)!\n", __func__, handle);
        return HI_FAILURE;
    }

    buf = dma_buf_get(shared_fd);
    if (IS_ERR(buf)) {
        sys_close(shared_fd);
        OmxPrint(OMX_ERR, "%s Invalid file handle(%d)\n", __func__, shared_fd);
        return HI_FAILURE;
    }

    attach = dma_buf_attach(buf, dev);
    if (IS_ERR(attach)) {
        dma_buf_put(buf);
        sys_close(shared_fd);
        return HI_FAILURE;
    }

    table = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
    if (IS_ERR(table)) {
        dma_buf_detach(buf, attach);
        dma_buf_put(buf);
        sys_close(shared_fd);
        return HI_FAILURE;
    }

    *addr = sg_phys(table->sgl);

    dma_buf_unmap_attachment(attach, table, DMA_BIDIRECTIONAL);
    dma_buf_detach(buf, attach);
    dma_buf_put(buf);
    sys_close(shared_fd);
    return HI_SUCCESS;
}

/*----------------------------------------
    func: normal memory alloc interface
 ----------------------------------------*/
static HI_S32 VDEC_MEM_AllocNormal_MMU(MEM_BUFFER_S *psMBuf, struct ion_handle **phandle)
{/*lint !e578 */
    HI_S32 ret;
    HI_U32 alloc_flags = 0;
    ion_phys_addr_t phys_addr = 0;
    HI_VOID *virt_addr = HI_NULL;
    struct iommu_map_format ion_fmt;

    alloc_flags = ION_FLAG_NOT_ZERO_BUFFER;

    if (1 == psMBuf->u8IsCached)
    {
        alloc_flags |= ION_FLAG_CACHED;
    }

    *phandle = ion_alloc(gIONClient, psMBuf->u32Size, PAGE_SIZE, ION_HEAP(ION_SYSTEM_HEAP_ID), alloc_flags);
    if (IS_ERR_OR_NULL(*phandle))
    {
        OmxPrint(OMX_ERR, "%s failed to alloc ion handle!\n", __func__);
        return HI_FAILURE;
    }

    virt_addr = ion_map_kernel(gIONClient, *phandle);
    if (IS_ERR_OR_NULL(virt_addr))
    {
        OmxPrint(OMX_ERR, "%s failed to map kernel virt addr!\n", __func__);
        goto err_ion_map;
    }

    memset(&ion_fmt, 0x00, sizeof(ion_fmt)); /* unsafe_function_ignore: memset */
    ret = ion_map_iommu(gIONClient, *phandle, &ion_fmt);
    if (ret)
    {
        OmxPrint(OMX_ERR, "ion map mmu failed!\n");
        goto err_ion_phys;
    }
    phys_addr = ion_fmt.iova_start;

    psMBuf->pStartVirAddr    = virt_addr;
    psMBuf->u32StartPhyAddr  = phys_addr;

    return HI_SUCCESS;

err_ion_phys:
    ion_unmap_kernel(gIONClient, *phandle);
err_ion_map:
    ion_free(gIONClient, *phandle);

    return HI_FAILURE;
}


/*----------------------------------------
    func: normal memory free interface
 ----------------------------------------*/
static HI_VOID VDEC_MEM_FreeNormal_MMU(ION_MEM_DESC_S *pMemNode)
{
#ifdef HIVDEC_SMMU_SUPPORT
    /* ion_unmap iommu */
    ion_unmap_iommu(gIONClient, pMemNode->handle);
#endif

    ion_unmap_kernel(gIONClient, pMemNode->handle);
    ion_free(gIONClient, pMemNode->handle);

    return;
}


/*----------------------------------------
    func: normal contiguous memory alloc interface
 ----------------------------------------*/
static HI_S32 VDEC_MEM_AllocNormal_CTG(MEM_BUFFER_S *psMBuf, struct ion_handle **phandle)
{/*lint !e578 */
    HI_S32 ret;
    ion_phys_addr_t phys_addr = 0;
    HI_VOID *virt_addr;
    size_t len = 0;
    HI_U32 alloc_flags = 0;

    if (1 == psMBuf->u8IsCached)
    {
        alloc_flags |= ION_FLAG_CACHED;
    }

    *phandle = ion_alloc(gIONClient, psMBuf->u32Size, PAGE_SIZE, ION_HEAP(ION_MISC_HEAP_ID), alloc_flags);
    if (IS_ERR_OR_NULL(*phandle))
    {
        OmxPrint(OMX_ERR, "%s failed to alloc ion handle!\n", __func__);
        return HI_FAILURE;
    }

    virt_addr = ion_map_kernel(gIONClient, *phandle);
    if (IS_ERR_OR_NULL(virt_addr))
    {
        OmxPrint(OMX_ERR, "%s failed to map kernel virt addr!\n", __func__);
        goto err_ion_map;
    }

    if (VDEC_MEM_ion_phys(gIONClient, *phandle, g_pOmxVdec->device, &phys_addr, &len) < 0)
    {
        OmxPrint(OMX_ERR, "%s failed to get phy addr!\n", __func__);
        goto err_ion_phys;
    }

    /*allocating contiguous physical address use cma pool not reserve memory, so need to clear the memory  */
    memset(virt_addr, 0, (size_t)psMBuf->u32Size); /* unsafe_function_ignore: memset */
    psMBuf->pStartVirAddr    = virt_addr;
    psMBuf->u32StartPhyAddr  = phys_addr;

    return HI_SUCCESS;

err_ion_phys:
    ion_unmap_kernel(gIONClient, *phandle);
err_ion_map:
    ion_free(gIONClient, *phandle);

    return HI_FAILURE;
}

/*----------------------------------------
    func: normal contiguous memory free interface
 ----------------------------------------*/
static HI_VOID VDEC_MEM_FreeNormal_CTG(ION_MEM_DESC_S *pMemNode)
{
    ion_unmap_kernel(gIONClient, pMemNode->handle);
    ion_free(gIONClient, pMemNode->handle);

    return;
}


/*----------------------------------------
    func: secure memory alloc interface
 ----------------------------------------*/
static HI_S32 VDEC_MEM_AllocSecure(MEM_BUFFER_S *psMBuf, struct ion_handle **phandle)
{/*lint !e578 */
    HI_S32 ret = 0;
    size_t len = 0;
    HI_U32 alloc_flags;
    ion_phys_addr_t phys_addr = 0;

    alloc_flags = ION_FLAG_SECURE_BUFFER;

    if (1 == psMBuf->u8IsCached)
    {
        alloc_flags |= ION_FLAG_CACHED;  // secure heap not support cached flag !!!
    }

    *phandle = ion_alloc(gIONClient, psMBuf->u32Size, PAGE_SIZE, ION_HEAP(ION_DRM_HEAP_ID), alloc_flags);
    if (IS_ERR_OR_NULL(*phandle))
    {
        OmxPrint(OMX_ERR, "%s failed to alloc ion handle!\n", __func__);
        return HI_FAILURE;
    }

    if (VDEC_MEM_ion_phys(gIONClient, *phandle, g_pOmxVdec->device, &phys_addr, &len) < 0)
    {
        OmxPrint(OMX_ERR, "%s failed to get phy addr!\n", __func__);
        goto err_ion_free;
    }

    psMBuf->u32StartPhyAddr = phys_addr;
    psMBuf->pStartVirAddr   = (HI_VOID *)phys_addr;  // secure vir addr will be mapped in trusted firmware if needed, use phy addr for now.

    return HI_SUCCESS;

err_ion_free:
    ion_free(gIONClient, *phandle);
    return HI_FAILURE;
}


/*----------------------------------------
    func: secure memory free interface
 ----------------------------------------*/
static HI_VOID VDEC_MEM_FreeSecure(ION_MEM_DESC_S *pMemNode)
{
    ion_free(gIONClient, pMemNode->handle);
    return;
}


/*----------------------------------------
    func: share buffer for IPC
 ----------------------------------------*/
static HI_S32 VDEC_MEM_ShareBuffer(MEM_BUFFER_S *psMBuf, HI_S32 *pShareFd)
{
    HI_S32 i;
    HI_S32 share_fd;

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if (gIONClient == HI_NULL || psMBuf == HI_NULL || pShareFd == HI_NULL)
    {
        printk(KERN_ERR "%s FATAL: gIONClient(%pK)/psMBuf(%pK)/pShareFd(%pK) = NULL\n", __func__, gIONClient, psMBuf, pShareFd);
        goto err_exit;
    }

    for (i=0; i<MAX_ION_MEM_NODE; i++)
    {
        if ((psMBuf->u32StartPhyAddr != 0) && (psMBuf->u32StartPhyAddr == gIONMemNode[i].phys_addr))
        {
            break;
        }
    }

    if (i >= MAX_ION_MEM_NODE)
    {
        printk(KERN_ERR "%s FATAL: Ion mem node not found, phy_addr = 0x%x!\n", __func__, psMBuf->u32StartPhyAddr);
        goto err_exit;
    }

    share_fd = ion_share_dma_buf_fd(gIONClient, gIONMemNode[i].handle);
    if (share_fd < 0)
    {
        printk(KERN_ERR "%s FATAL: ion_share_dma_buf_fd failed!\n", __func__);
        VDEC_UP_INTERRUPTIBLE(&gMemMut);
        return HI_FAILURE;
    }

    *pShareFd = share_fd;

    VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_SUCCESS;

err_exit:
	VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_FAILURE;
}


/******************************** SHARE FUNC **********************************/
/*----------------------------------------
    func: memory module probe entry
 ----------------------------------------*/
HI_S32 VDEC_MEM_Probe(HI_VOID)
{
    gIONClient = HI_NULL;
	gIONNodeNum = 0;
	gCMANodeNum = 0;
    memset(&gIONMemNode, 0, MAX_ION_MEM_NODE*sizeof(ION_MEM_DESC_S)); /* unsafe_function_ignore: memset */
    memset(&gCMAMemNode, 0, MAX_CMA_MEM_NODE*sizeof(CMA_MEM_DESC_S)); /* unsafe_function_ignore: memset */

    VDEC_INIT_MUTEX(&gMemMut);

    return HI_SUCCESS;
}

/*----------------------------------------
    func: initialize memory module
 ----------------------------------------*/
HI_S32 VDEC_MEM_Init(HI_VOID)
{
    HI_S32 ret = HI_SUCCESS;

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    gIONClient = (struct ion_client *)hisi_ion_client_create("hi_vcodec_ion");
    if (IS_ERR_OR_NULL(gIONClient))
    {
        OmxPrint(OMX_ERR, "%s failed to create hi_vcodec ion client!\n", __func__);
        gIONClient = HI_NULL;
        ret = HI_FAILURE;
    }

	VDEC_UP_INTERRUPTIBLE(&gMemMut);

    return ret;
}


/*----------------------------------------
    func: deinitialize memory module
 ----------------------------------------*/
HI_S32 VDEC_MEM_Exit(HI_VOID)
{
    HI_S32 i;

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if (gIONClient == HI_NULL)
    {
        printk(KERN_ERR "%s FATAL: gIONClient = NULL\n", __func__);
	    VDEC_UP_INTERRUPTIBLE(&gMemMut);
        return HI_FAILURE;
    }

    for (i=0; i<MAX_ION_MEM_NODE; i++)
    {
        if ((gIONMemNode[i].phys_addr != 0) && (gIONMemNode[i].handle != HI_NULL))
        {
            printk(KERN_ERR "Warning: ion mem leak, name %s, zone %s, phy 0x%x, size %d, free now.\n", gIONMemNode[i].node_name, gIONMemNode[i].zone_name, gIONMemNode[i].phys_addr, gIONMemNode[i].size);
            if (1 == gIONMemNode[i].is_secure)
            {
                VDEC_MEM_FreeSecure(&gIONMemNode[i]);
            }
            else
            {
                if (1 == gIONMemNode[i].is_contiguous)
                {
                    VDEC_MEM_FreeNormal_CTG(&gIONMemNode[i]);
                }
                else
                {
                    VDEC_MEM_FreeNormal_MMU(&gIONMemNode[i]);
                }
            }
	        memset(&gIONMemNode[i], 0, sizeof(ION_MEM_DESC_S)); /* unsafe_function_ignore: memset */
        }
    }

    for (i=0; i<MAX_CMA_MEM_NODE; i++)
    {
        if (gCMAMemNode[i].virt_addr != 0)
        {
            printk(KERN_ERR "Warning: cma mem leak, name %s, zone %s, phy 0x%x, size %d, free now.\n", gCMAMemNode[i].node_name, gCMAMemNode[i].zone_name, gCMAMemNode[i].phys_addr, gCMAMemNode[i].size);
            kfree(gCMAMemNode[i].virt_addr);
	        memset(&gCMAMemNode[i], 0, sizeof(CMA_MEM_DESC_S)); /* unsafe_function_ignore: memset */
        }
    }

    ion_client_destroy(gIONClient);
    gIONClient  = HI_NULL;
	gIONNodeNum = 0;
	gCMANodeNum = 0;

	VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_SUCCESS;
}

/*----------------------------------------
    func: basic ion alloc & map interface
 ----------------------------------------*/
HI_S32 VDEC_MEM_AllocAndMap(const HI_CHAR *bufname, HI_CHAR *zone_name, MEM_BUFFER_S *psMBuf)
{
    HI_S32 i;
    HI_S32 ret;
    struct ion_handle *handle = HI_NULL;

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if (psMBuf == NULL || HI_NULL == gIONClient)
    {
        printk(KERN_ERR "%s param psMBuf = NULL\n", __func__);
        goto err_exit;
    }

    if (HI_NULL != psMBuf->pStartVirAddr)
    {
        printk(KERN_ERR "%s param StartVirAddr %pK != NULL\n", __func__, psMBuf->pStartVirAddr);
        goto err_exit;
    }

    for (i=0; i<MAX_ION_MEM_NODE; i++)
    {
        if (0 == gIONMemNode[i].phys_addr && HI_NULL == gIONMemNode[i].handle)
        {
            break;
        }
    }

    if (i >= MAX_ION_MEM_NODE)
    {
        printk(KERN_ERR "%s FATAL: No free ion mem node!\n", __func__);
        goto err_exit;
    }

    if (psMBuf->u8IsSecure != 1 && psMBuf->u8IsSecure != 0)
    {
        printk(KERN_ERR "%s Warnning!! Buf %s IsSecure %d != 1&0\n", __func__, bufname, psMBuf->u8IsSecure);
    }

    if (1 == psMBuf->u8IsSecure)
    {
        ret = VDEC_MEM_AllocSecure(psMBuf, &handle);
    }
    else
    {
        if (1 == psMBuf->u8IsContiguous)
        {
            ret = VDEC_MEM_AllocNormal_CTG(psMBuf, &handle);
        }
        else
        {
            ret = VDEC_MEM_AllocNormal_MMU(psMBuf, &handle);
        }
    }
    if (ret != HI_SUCCESS)
    {
        printk(KERN_ERR "%s FATAL: alloc memory(size=%d) for %s failed!\n", __func__, psMBuf->u32Size, bufname);
        goto err_exit;
    }

    gIONMemNode[i].handle        = handle;
    gIONMemNode[i].is_secure     = psMBuf->u8IsSecure;
    gIONMemNode[i].is_cached     = psMBuf->u8IsCached;
    gIONMemNode[i].is_contiguous = psMBuf->u8IsContiguous;
    gIONMemNode[i].phys_addr     = psMBuf->u32StartPhyAddr;
    gIONMemNode[i].virt_addr     = psMBuf->pStartVirAddr;
    gIONMemNode[i].size          = psMBuf->u32Size;

	snprintf(gIONMemNode[i].node_name, MAX_MEM_NAME_LEN, bufname);/*lint !e592 */ /* unsafe_function_ignore: snprintf  */
	gIONMemNode[i].node_name[MAX_MEM_NAME_LEN-1] = '\0';

	snprintf(gIONMemNode[i].zone_name, MAX_MEM_NAME_LEN, zone_name);/*lint !e592 */ /* unsafe_function_ignore: snprintf  */
	gIONMemNode[i].zone_name[MAX_MEM_NAME_LEN-1] = '\0';

    gIONNodeNum++;

    //OmxPrint(OMX_MEM, "%s %s: phy 0x%x, vir %pK, size 0x%x, is_secure %d, is_cached %d, is_contiguous %d\n", __func__,
    //         gIONMemNode[i].node_name, gIONMemNode[i].phys_addr, gIONMemNode[i].virt_addr, gIONMemNode[i].size, gIONMemNode[i].is_secure, gIONMemNode[i].is_cached, gIONMemNode[i].is_contiguous);

	VDEC_UP_INTERRUPTIBLE(&gMemMut);
	return HI_SUCCESS;

err_exit:
	VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_FAILURE;
}


/*----------------------------------------
    func: basic ion unmap & free interface
 ----------------------------------------*/
HI_S32 VDEC_MEM_UnmapAndRelease(MEM_BUFFER_S *psMBuf)
{
    HI_S32 i;

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if (gIONClient == HI_NULL || psMBuf == HI_NULL)
    {
        printk(KERN_ERR "%s FATAL: gIONClient(%pK)/psMBuf(%pK) = NULL\n", __func__, gIONClient, psMBuf);
        goto err_exit;
    }

    for (i=0; i<MAX_ION_MEM_NODE; i++)
    {
        if ((psMBuf->u32StartPhyAddr != 0) && (psMBuf->u32StartPhyAddr == gIONMemNode[i].phys_addr) && (gIONMemNode[i].handle != HI_NULL))
        {
            break;
        }
    }

    if (i >= MAX_ION_MEM_NODE)
    {
        OmxPrint(OMX_INFO, "%s mem not found, phyaddr = %x.\n", __func__, psMBuf->u32StartPhyAddr);
        goto err_exit;
    }

    if (psMBuf->u8IsSecure != 1 && psMBuf->u8IsSecure != 0)
    {
        printk(KERN_ERR "%s Warnning!! Buf %s IsSecure %d != 1&0\n", __func__, gIONMemNode[i].node_name, psMBuf->u8IsSecure);
    }

    if (1 == psMBuf->u8IsSecure)
    {
        VDEC_MEM_FreeSecure(&gIONMemNode[i]);
    }
    else
    {
        if (1 == psMBuf->u8IsContiguous)
        {
            VDEC_MEM_FreeNormal_CTG(&gIONMemNode[i]);
        }
        else
        {
            VDEC_MEM_FreeNormal_MMU(&gIONMemNode[i]);
        }
    }

    //OmxPrint(OMX_MEM, "%s %s: phy 0x%x, vir %pK, size 0x%x, is_secure %d, is_cached %d, is_contiguous %d\n", __func__,
    //         gIONMemNode[i].node_name, gIONMemNode[i].phys_addr, gIONMemNode[i].virt_addr, gIONMemNode[i].size, gIONMemNode[i].is_secure, gIONMemNode[i].is_cached, gIONMemNode[i].is_contiguous);

    memset(&gIONMemNode[i], 0, sizeof(ION_MEM_DESC_S)); /* unsafe_function_ignore: memset */

    gIONNodeNum = (gIONNodeNum>0)? (gIONNodeNum-1): 0;

	VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_SUCCESS;

err_exit:
	VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_FAILURE;
}


/*----------------------------------------
    func: map ion buffer
 ----------------------------------------*/
HI_S32 VDEC_MEM_MapKernel(HI_S32 share_fd, MEM_BUFFER_S *psMBuf)
{
    HI_S32 i;
    HI_S32 ret;
    size_t len;
    ion_phys_addr_t phy_addr = 0;
    HI_VOID* virt_addr  = HI_NULL;
    struct ion_handle* handle = HI_NULL;
    struct iommu_map_format ion_fmt;

    if (psMBuf == HI_NULL)
    {
        printk(KERN_ERR "invalid Param psMBuf is null for %s!\n", __func__);
        return HI_FAILURE;
    }

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if((gIONClient == HI_NULL) || (share_fd < 0))
    {
        printk(KERN_ERR "invalid Param for %s!\n", __func__);
        goto err_exit;
    }

    for (i=0; i<MAX_ION_MEM_NODE; i++)
    {
        if (0 == gIONMemNode[i].phys_addr && HI_NULL == gIONMemNode[i].handle)
        {
            break;
        }
    }

    if (i >= MAX_ION_MEM_NODE)
    {
        printk(KERN_ERR "%s FATAL: Ion mem node not found!\n", __func__);
        goto err_exit;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
    handle = ion_import_dma_buf_fd(gIONClient, share_fd);
#else
    handle = ion_import_dma_buf(gIONClient, share_fd);
#endif
    if (IS_ERR_OR_NULL(handle))
    {
        printk(KERN_ERR "%s, get ion handle failed!\n", __func__);
        goto err_exit;
    }

    if (0 == psMBuf->u8IsSecure)
    {
        virt_addr = ion_map_kernel(gIONClient, handle);
        if (IS_ERR_OR_NULL(virt_addr))
        {
            printk(KERN_ERR "%s failed to map kernel virt addr!\n", __func__);
            goto err_exit1;
        }
        psMBuf->pStartVirAddr = virt_addr;

#ifdef HIVDEC_SMMU_SUPPORT
        /* Get iova_start addr */

        memset(&ion_fmt, 0x0, sizeof(ion_fmt)); /* unsafe_function_ignore: memset */

        if(ion_map_iommu(gIONClient, handle, &ion_fmt) != 0)
        {
            printk(KERN_ERR "ion map mmu failed!\n");
            goto err_exit2;
        }
        phy_addr = ion_fmt.iova_start;
#else
        /* Get phy address directly */
        if (VDEC_MEM_ion_phys(gIONClient, handle, g_pOmxVdec->device, &phy_addr, &len) < 0)
        {
            printk(KERN_ERR "%s failed to ion_phys!\n", __func__);
            goto err_exit1;
        }
#endif
    }
    else
    {
        if (VDEC_MEM_ion_phys(gIONClient, handle, g_pOmxVdec->device, &phy_addr, &len) < 0)
        {
            printk(KERN_ERR "%s failed to ion_phys!\n", __func__);
            goto err_exit1;
        }
    }

    psMBuf->u32StartPhyAddr = (HI_U32)phy_addr;

    gIONMemNode[i].phys_addr = (HI_U32)phy_addr;
    gIONMemNode[i].virt_addr = psMBuf->pStartVirAddr;
    gIONMemNode[i].is_secure = psMBuf->u8IsSecure;
    gIONMemNode[i].handle    = handle;
    gIONMemNode[i].size      = psMBuf->u32Size;

    snprintf(gIONMemNode[i].node_name, MAX_MEM_NAME_LEN, "OMXVDEC_OUT");/* unsafe_function_ignore: snprintf  */
    gIONMemNode[i].node_name[MAX_MEM_NAME_LEN-1] = '\0';

    snprintf(gIONMemNode[i].zone_name, MAX_MEM_NAME_LEN, CLIENT_BUF_NAME);/* unsafe_function_ignore: snprintf  */
    gIONMemNode[i].zone_name[MAX_MEM_NAME_LEN-1] = '\0';

    gIONNodeNum++;

    //OmxPrint(OMX_MEM, "%s %s, phy 0x%x, vir %pK, size 0x%x, is_secure %d\n", __func__, gIONMemNode[i].node_name, gIONMemNode[i].phys_addr, gIONMemNode[i].virt_addr, gIONMemNode[i].size, gIONMemNode[i].is_secure);

    VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_SUCCESS;

err_exit2:
    ion_unmap_kernel(gIONClient, handle);
err_exit1:
    ion_free(gIONClient, handle);
err_exit:
    VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_FAILURE;
}


/*----------------------------------------
    func: unmap ion buffer
 ----------------------------------------*/
HI_S32 VDEC_MEM_UnmapKernel(MEM_BUFFER_S *psMBuf)
{
    HI_S32 i;

    if (psMBuf == HI_NULL)
    {
        printk(KERN_ERR "%s FATAL: psMBuf = NULL\n", __func__);
        return HI_FAILURE;
    }

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if (gIONClient == HI_NULL)
    {
        printk(KERN_ERR "%s FATAL: gIONClient(%pK)/psMBuf(%pK) = NULL\n", __func__, gIONClient, psMBuf);
        goto err_exit;
    }

    for (i=0; i<MAX_ION_MEM_NODE; i++)
    {
        if ((psMBuf->u32StartPhyAddr != 0) && (psMBuf->u32StartPhyAddr == gIONMemNode[i].phys_addr) && (gIONMemNode[i].handle != HI_NULL))
        {
            break;
        }
    }

    if (i >= MAX_ION_MEM_NODE)
    {
        printk(KERN_ERR "%s FATAL: Ion mem node not found!\n", __func__);
        goto err_exit;
    }

    if (0 == psMBuf->u8IsSecure)
    {
#ifdef HIVDEC_SMMU_SUPPORT
        /* ion_unmap iommu */
        ion_unmap_iommu(gIONClient, gIONMemNode[i].handle);
#endif
        ion_unmap_kernel(gIONClient, gIONMemNode[i].handle);
    }

    ion_free(gIONClient, gIONMemNode[i].handle);

    //OmxPrint(OMX_MEM, "%s %s, phy 0x%x, vir %pK, size 0x%x, is_secure %d\n", __func__, gIONMemNode[i].node_name, gIONMemNode[i].phys_addr, gIONMemNode[i].virt_addr, gIONMemNode[i].size, gIONMemNode[i].is_secure);

    memset(&gIONMemNode[i], 0, sizeof(ION_MEM_DESC_S)); /* unsafe_function_ignore: memset */

    gIONNodeNum = (gIONNodeNum > 0)? (gIONNodeNum-1): 0;

    VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_SUCCESS;

err_exit:
    VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_FAILURE;
}


/*----------------------------------------
    func: alloc buffer & share fd for omxvdec
          need share fd
 ----------------------------------------*/
HI_S32 VDEC_MEM_AllocAndShare(const HI_CHAR *bufname, HI_CHAR *zone_name, MEM_BUFFER_S *psMBuf, HI_S32 *pShareFd)
{
    HI_S32 ret;

    if (HI_NULL == psMBuf || HI_NULL == pShareFd)
    {
        OmxPrint(OMX_ERR, "%s param psMBuf(%pK)/pShareFd(%pK) = NULL\n", __func__, psMBuf, pShareFd);
        return HI_FAILURE;
    }

    ret = VDEC_MEM_AllocAndMap(bufname, zone_name, psMBuf);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s FATAL: VDEC_MEM_AllocAndMap failed!\n", __func__);
        return HI_FAILURE;
    }

    ret = VDEC_MEM_ShareBuffer(psMBuf, pShareFd);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s FATAL: VDEC_MEM_ShareBuffer failed!\n", __func__);
        VDEC_MEM_UnmapAndRelease(psMBuf);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


/*----------------------------------------
    func: close fd & free buffer for omxvdec
          need share fd
 ----------------------------------------*/
HI_S32 VDEC_MEM_CloseAndFree(MEM_BUFFER_S *psMBuf, HI_S32 *pShareFd)
{
    HI_S32 ret;

    if (HI_NULL == psMBuf || HI_NULL == pShareFd)
    {
        OmxPrint(OMX_ERR, "%s param psMBuf(%pK)/pShareFd(%pK) = NULL\n", __func__, psMBuf, pShareFd);
        return HI_FAILURE;
    }

    ret = VDEC_MEM_UnmapAndRelease(psMBuf);
    if (ret != HI_SUCCESS)
    {
        OmxPrint(OMX_ERR, "%s FATAL: VDEC_MEM_UnmapAndRelease failed!\n", __func__);
        return HI_FAILURE;
    }

    if (current->files != NULL)
    {
        sys_close(*pShareFd);
    }
    else
    {
        OmxPrint(OMX_ERR, "%s param current->files = NULL\n", __func__);
    }

    return HI_SUCCESS;
}


/*----------------------------------------
    func: basic kmalloc/kzalloc interface
 ----------------------------------------*/
HI_S32 VDEC_MEM_KAlloc(const HI_CHAR *bufname, HI_CHAR *zone_name, MEM_BUFFER_S *psMBuf, HI_U8 NeedZero)
{
    HI_S32 i;
    HI_VOID *virt_addr = HI_NULL;

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if (psMBuf == NULL)
    {
        printk(KERN_ERR "%s param psMBuf = NULL\n", __func__);
        goto err_exit;
    }

    if (HI_NULL != psMBuf->pStartVirAddr)
    {
        printk(KERN_ERR "%s param StartVirAddr %pK != NULL\n", __func__, psMBuf->pStartVirAddr);
        goto err_exit;
    }

    for (i=0; i<MAX_CMA_MEM_NODE; i++)
    {
        if (0 == gCMAMemNode[i].phys_addr && HI_NULL == gCMAMemNode[i].virt_addr)
        {
            break;
        }
    }

    if (i >= MAX_CMA_MEM_NODE)
    {
        printk(KERN_ERR "%s FATAL: No free cma mem node!\n", __func__);
        goto err_exit;
    }

    if (1 == NeedZero)
    {
        virt_addr = kzalloc(psMBuf->u32Size, GFP_KERNEL | GFP_DMA); //restrict [0 ~ 4G]
    }
    else
    {
        virt_addr = kmalloc(psMBuf->u32Size, GFP_KERNEL | GFP_DMA); //restrict [0 ~ 4G]
    }

    if (IS_ERR_OR_NULL(virt_addr))
    {
        printk(KERN_CRIT "%s call alloc size %d (NeedZero %d) failed.\n", __func__, psMBuf->u32Size, NeedZero);
        goto err_exit;
    }

    psMBuf->pStartVirAddr   = virt_addr;
    psMBuf->u32StartPhyAddr = __pa(virt_addr);/*lint !e648 */

    snprintf(gCMAMemNode[i].node_name, MAX_MEM_NAME_LEN, bufname);/*lint !e592 */ /* unsafe_function_ignore: snprintf  */
    gCMAMemNode[i].node_name[MAX_MEM_NAME_LEN-1] = '\0';

    snprintf(gCMAMemNode[i].zone_name, MAX_MEM_NAME_LEN, zone_name);/*lint !e592 */ /* unsafe_function_ignore: snprintf  */
    gCMAMemNode[i].zone_name[MAX_MEM_NAME_LEN-1] = '\0';

    gCMAMemNode[i].phys_addr = psMBuf->u32StartPhyAddr;
    gCMAMemNode[i].virt_addr = psMBuf->pStartVirAddr;
    gCMAMemNode[i].size      = psMBuf->u32Size;

    gCMANodeNum++;

    //OmxPrint(OMX_MEM, "%s %s: size = 0x%x\n", __func__, gCMAMemNode[i].node_name, gCMAMemNode[i].size);

    VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_SUCCESS;

err_exit:
    VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_FAILURE;/*lint !e429 */
}

/*----------------------------------------
    func: get vir addr from phy addr
 ----------------------------------------*/
HI_S32 VDEC_MEM_GetVirAddr_FromPhyAddr(HI_U8 **ppVirAddr, HI_U32 PhyAddr, HI_U32 Size)
{
    HI_S32 i;
    HI_S32 ret;

    VDEC_ASSERT_RETURN(ppVirAddr == HI_NULL);

    *ppVirAddr = HI_NULL;

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if (gIONClient == HI_NULL)
    {
        printk(KERN_ERR "%s FATAL: gIONClient = NULL\n", __func__);
        VDEC_UP_INTERRUPTIBLE(&gMemMut);
        return HI_FAILURE;
    }

    for (i=0; i<MAX_ION_MEM_NODE; i++)
    {
        if ((gIONMemNode[i].phys_addr <= PhyAddr) && (PhyAddr < gIONMemNode[i].phys_addr + gIONMemNode[i].size)
         && (gIONMemNode[i].size >= Size))
        {
            *ppVirAddr = gIONMemNode[i].virt_addr + PhyAddr - gIONMemNode[i].phys_addr;
            break;
        }
    }

    VDEC_UP_INTERRUPTIBLE(&gMemMut);

    ret = (i < MAX_ION_MEM_NODE) ? HI_SUCCESS : HI_FAILURE;

    return ret;
}


/*----------------------------------------
    func: basic mem free interface
 ----------------------------------------*/
HI_S32 VDEC_MEM_KFree(MEM_BUFFER_S *psMBuf)
{
    HI_S32 i;

    VDEC_DOWN_INTERRUPTIBLE(&gMemMut);

    if (psMBuf == HI_NULL)
    {
        printk(KERN_ERR "%s FATAL: psMBuf = NULL\n", __func__);
        goto err_exit;
    }

    for (i = 0; i < MAX_CMA_MEM_NODE; i++)
    {
        if ((psMBuf->pStartVirAddr != 0) && (psMBuf->pStartVirAddr == gCMAMemNode[i].virt_addr))
        {
            break;
        }
    }

    if (i >= MAX_CMA_MEM_NODE)
    {
        printk(KERN_ERR "%s FATAL: cma mem node not found, phy address = %x.\n", __func__, psMBuf->u32StartPhyAddr);
	    goto err_exit;
    }

    kfree(gCMAMemNode[i].virt_addr);

    OmxPrint(OMX_MEM, "%s %s: size = 0x%x\n", __func__, gCMAMemNode[i].node_name, gCMAMemNode[i].size);

    memset(&gCMAMemNode[i], 0, sizeof(CMA_MEM_DESC_S)); /* unsafe_function_ignore: memset */ /* [false alarm] */

    gCMANodeNum = (gCMANodeNum>0)? (gCMANodeNum-1): 0;

	VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_SUCCESS;

err_exit:
    VDEC_UP_INTERRUPTIBLE(&gMemMut);
    return HI_FAILURE;
}


/*----------------------------------------
    func: memory read proc entry
 ----------------------------------------*/
HI_VOID VDEC_MEM_Read_Proc(HI_VOID *pParam, HI_VOID *v)
{
    HI_S32 i = 0;
    HI_U32 total_size = 0;
    struct seq_file *p = (struct seq_file *)pParam;

    total_size = 0;
    PROC_PRINT(p, "------------------------- ION Memory -------------------------\n\n");
    if (gIONNodeNum > 0)
    {
	    PROC_PRINT(p, " %-13s%-10s%-10s%-10s%-20s\n\n", "NAME", "OWNER", "PHYADDR", "SIZE", "VIRADDR");
    	for (i=0; i<MAX_ION_MEM_NODE; i++)
    	{
    	    if (gIONMemNode[i].phys_addr != 0)
    	    {
                PROC_PRINT(p, " %-13s%-10s%-10x%-10d%-20pK",
                          gIONMemNode[i].node_name,
                          gIONMemNode[i].zone_name,
                          gIONMemNode[i].phys_addr,
                          gIONMemNode[i].size,
                          gIONMemNode[i].virt_addr);

                PROC_PRINT(p, " %s %s %s",
                          (1==gIONMemNode[i].is_secure)    ? "(SEC)":"",
                          (1==gIONMemNode[i].is_cached)    ? "(CAH)":"",
                          (1==gIONMemNode[i].is_contiguous)? "(CTG)":"");

                PROC_PRINT(p, "\n");

				total_size += gIONMemNode[i].size;
    	    }
    	}
        PROC_PRINT(p, "\n--------------------------------------------------------------\n\n");
    }
    PROC_PRINT(p, "TotalNum         :%-10d\n", gIONNodeNum);
    PROC_PRINT(p, "TotalSize        :%-10d\n", total_size);
    PROC_PRINT(p, "\n--------------------------------------------------------------\n\n");

    total_size = 0;
    PROC_PRINT(p, "------------------------- CMA Memory -------------------------\n\n");
    if (gCMANodeNum > 0)
    {
	    PROC_PRINT(p, " %-13s%-10s%-10s%-10s%-20s\n\n", "NAME", "OWNER", "PHYADDR", "SIZE", "VIRADDR");
    	for (i=0; i<MAX_CMA_MEM_NODE; i++)
    	{
    	    if (gCMAMemNode[i].phys_addr != 0)
    	    {
                PROC_PRINT(p, " %-13s%-10s%-10x%-10d%-20pK",
                          gCMAMemNode[i].node_name,
                          gCMAMemNode[i].zone_name,
                          gCMAMemNode[i].phys_addr,
                          gCMAMemNode[i].size,
                          gCMAMemNode[i].virt_addr);

                PROC_PRINT(p, "\n");

				total_size += gCMAMemNode[i].size;
    	    }
    	}
        PROC_PRINT(p, "\n--------------------------------------------------------------\n\n");
    }
    PROC_PRINT(p, "TotalNum         :%-10d\n", gCMANodeNum);
    PROC_PRINT(p, "TotalSize        :%-10d\n", total_size);
    PROC_PRINT(p, "\n--------------------------------------------------------------\n\n");

    return;
}


