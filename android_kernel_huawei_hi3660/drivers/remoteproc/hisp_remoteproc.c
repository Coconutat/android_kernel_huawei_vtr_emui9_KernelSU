/*
 * HiStar Remote Processor driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/amba/bus.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_rproc.h>
#include "remoteproc_internal.h"
#include <linux/hisi/hisi_mailbox_dev.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <linux/rproc_share.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <global_ddr_map.h>
#include <isp_ddr_map.h>
#include <asm/cacheflush.h>
#include <linux/firmware.h>
#include <linux/iommu.h>
#include <linux/hisi/hisi-iommu.h>
#include <linux/crc32.h>
#include <linux/ion.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/spinlock.h>
#include "isp_ddr_map.h"
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <linux/syscalls.h>
#include <dsm/dsm_pub.h>
#include <linux/hisi/hisi_efuse.h>

#define DTS_COMP_NAME   "hisilicon,isp"
#define DTS_COMP_LOGIC_NAME   "hisilicon,isplogic"

struct dsm_client *client_isp;
struct dsm_client_ops ops6 = {
    .poll_state = NULL,
    .dump_func = NULL,
};

struct dsm_dev dev_isp = {
    .name = "dsm_isp",
    .device_name = NULL,
    .ic_name = NULL,
    .module_name = NULL,
    .fops = &ops6,
    .buff_size = 256,
};

typedef	unsigned int mbox_msg_t;
static unsigned int communicat_msg[8] = {0};
static int perf_para = 0x0;
struct rproc_shared_para *isp_share_para = NULL;
#ifdef DEBUG_HISI_ISP
module_param_named(perf_para, perf_para, int, S_IRUGO | S_IWUSR);
#endif
static int debug_mask = 0x05;
#ifdef DEBUG_HISI_ISP
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
#endif
#define RPROC_INFO(fmt, args...) \
	do { \
		if (debug_mask & 0x01) \
			printk("Hisi rprocDrv Info: [%s] " fmt, __func__, ##args); \
	} while (0)
#define RPROC_DEBUG(fmt, args...) \
	do { \
		if (debug_mask & 0x02) \
			printk("Hisi rprocDrv Debug: [%s] " fmt, __func__, ##args); \
	} while (0)
#define RPROC_ERR(fmt, args...) \
	do { \
		if (debug_mask & 0x04) \
			printk("Hisi rprocDrv Dump: [%s] " fmt, __func__, ##args); \
	} while (0)

#define HISP_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define RPMSG_RX_FIFO_DEEP  257
#define MBOX_REG_COUNT      8

#define TIMEOUT             200
#define DIS_RSTSATA_OFFSET  0x4
#define EN_RSTSATA_OFFSET   0x8
#define EN_SATA_OFFSET      0xC

#define A7PC_OFFSET         0xA0
#define PMEVCNTR0_EL0       0x1000
#define PMCCNTR_EL0         0x107C
#define DBGLAR              0x0FB0
#define DBGOSLAR            0x0300
#define PMLAR               0x1FB0
#define PMCR                0x1E04
#define PMXEVTYPER0         0x1400
#define PMCNTENSET          0x1C00

/* CRG Regs Offset */
#define CRG_C84_PERIPHISP_SEC_RSTDIS        (0xC84)
#define CRG_C88_PERIPHISP_SEC_RSTSTAT       (0xC88)
#define IP_RST_ISPA7                        (1 << 4)
#define IP_RST_ISP                          (1 << 0)

#define CRG_C90_PERIPHISP_ISPA7_CTRL_BASE   (0xC90)
#define ISPA7_DBGPWRDUP                     (1 << 2)
#define ISPA7_VINITHI_HIGH                  (1 << 1)
#define ISPA7_REMAP_ENABLE                  (1 << 11)

#define CRGPERIPH_CLKDIV10_ADDR         (0x0D0)
#define CRGPERIPH_CLKDIV18_ADDR         (0x0F0)
#define CRGPERIPH_CLKDIV20_ADDR         (0x0F8)
#define CRGPERIPH_PERCLKEN0_ADDR        (0x008)
#define CRGPERIPH_PERSTAT0_ADDR         (0x00C)
#define CRGPERIPH_PERCLKEN3_ADDR        (0x038)
#define CRGPERIPH_PERSTAT3_ADDR         (0x03C)
#define CRGPERIPH_PERCLKEN5_ADDR        (0x058)
#define CRGPERIPH_PERSTAT5_ADDR         (0x05C)
#define CRGPERIPH_PERRSTSTAT0_ADDR      (0x068)
#define CRGPERIPH_PERRSTSTAT3_ADDR      (0x08C)
#define CRGPERIPH_ISOSTAT_ADDR          (0x14C)
#define CRGPERIPH_PERPWRSTAT_ADDR       (0x158)
#define CRGPERIPH_PERPWRACK_ADDR        (0x15C)

/* PMCtrl Regs Offset */
#define PMCTRL_384_NOC_POWER_IDLEACK        (0x384)
#define PMCTRL_388_NOC_POWER_IDLE           (0x388)

/* ISP Regs Offset */
#define REVISION_ID_OFFSET                  (0x20008)

/* PCtrl Regs Offset */
#define PCTRL_PERI_STAT2_ADDR       (0x09C)
#define PCTRL_PERI_STAT10_ADDR      (0x0BC)

/* MEDIA1 Regs Offset */
#define MEDIA1_PERCLKEN0_ADDR        (0x008)
#define MEDIA1_PERSTAT0_ADDR         (0x00C)
#define MEDIA1_PERCLKEN1_ADDR        (0x018)
#define MEDIA1_PERSTAT1_ADDR         (0x01C)
#define MEDIA1_CLKDIV9_ADDR          (0x084)

enum hisi_boardid {
    HI3650_BOARDID  = 0x00000003,
    HI6250_BOARDID  = 0x00000004,
    INVALID_BOARDID = 0xFFFFFFFF,
};

#define IS_HI3650(id) (id == HI3650_BOARDID)
#define IS_HI6250(id) (id == HI6250_BOARDID)

enum hisi_power_state {
    HISP_ISP_POWER_OFF      = 0,
    HISP_ISP_POWER_ON       = 1,
    HISP_ISP_POWER_FAILE    = 2,
    HISP_ISP_POWER_CLEAN    = 3,
};

enum hisi_rp_mbox_messages {
    RP_MBOX_READY           = 0xFFFFFF00,
    RP_MBOX_PENDING_MSG     = 0xFFFFFF01,
    RP_MBOX_CRASH           = 0xFFFFFF02,
    RP_MBOX_ECHO_REQUEST    = 0xFFFFFF03,
    RP_MBOX_ECHO_REPLY      = 0xFFFFFF04,
    RP_MBOX_ABORT_REQUEST   = 0xFFFFFF05,
};

typedef struct rproc_camera {
    struct clk *aclk;
    struct clk *aclk_dss;
    struct clk *pclk_dss;
	/* pinctrl */
	struct pinctrl *isp_pinctrl;
	struct pinctrl_state *pinctrl_def;
	struct pinctrl_state *pinctrl_idle;
} rproc_camera_t;

struct rproc_boot_device {
    unsigned int boardid;
    struct platform_device *isp_pdev;
    u64 remap_addr;
    u64 nsec_remap_addr;
    void *remap_va;
    struct regulator *isp_subsys_ip;
    struct clk *ispa7_clk;
    struct clk *isp_timer;
    unsigned int ispa7_clk_value;
    unsigned int isppd_adb_flag;
    struct hisi_isp_rproc *isp_rproc;
    struct isp_plat_cfg tmp_plat_cfg;
    struct resource *r[HISP_MAX_REGTYPE];
    void __iomem *reg[HISP_MAX_REGTYPE];
    unsigned int reg_num;
    rproc_camera_t isp_data;
    int isp_subsys_power_flag;
    unsigned int a7_ap_mbox;
    unsigned int ap_a7_mbox;
    enum hisi_isp_rproc_case_attr case_type;
	int secisp;
    int loadbin;
	int rpmsg_status;
    int ispcpu_status;
    struct wake_lock ispcpu_wakelock;
    struct mutex ispcpu_mutex;
    struct wake_lock jpeg_wakelock;
    struct mutex jpeg_mutex;
    struct mutex sharedbuf_mutex;
    struct mutex rpmsg_ready_mutex;
    struct mutex hisi_isp_power_mutex;
    spinlock_t rpmsg_ready_spin_mutex;
    atomic_t rproc_enable_status;
    unsigned int rpmsg_ready_state;
    unsigned int hisi_isp_power_state;
    int bypass_pwr_updn;
#ifdef CONFIG_HISI_REMOTEPROC_DMAALLOC_DEBUG
    struct hisi_isp_vring_s hisi_isp_vring[HISI_ISP_VRING_NUM];
#endif
    int probe_finished;
    int sec_thread_wake;
    struct hisi_isp_clk_dump_s hisi_isp_clk;
    unsigned char isp_efuse_flag;
    void* isp_bin_vaddr;
    void* rsctable_vaddr;
    unsigned int isp_bin_state;
    struct task_struct *loadispbin;
} rproc_boot_dev;

int last_boot_state;

struct isp_rx_mbox {
    struct kfifo rpmsg_rx_fifo;
    spinlock_t rpmsg_rx_lock;
    wait_queue_head_t wait;
    struct task_struct *rpmsg_rx_tsk;
    int can_be_wakeup;
} *isp_rx_mbox;

struct rx_buf_data {
    bool is_have_data;
    unsigned int rpmsg_rx_len;
    mbox_msg_t rpmsg_rx_buf[MBOX_REG_COUNT];
};

struct hisi_isp_rproc {
    struct hisi_mbox *mbox;
    struct notifier_block nb;
    struct rproc *rproc;
};

struct rproc *hisi_rproc;

/*lint -save -e454*/
void hisp_lock_sharedbuf(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    mutex_lock(&dev->sharedbuf_mutex);
}

/*lint -save -e455*/
void hisp_unlock_sharedbuf(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    mutex_unlock(&dev->sharedbuf_mutex);
}

int is_ispcpu_powerup(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return dev->ispcpu_status;
}

static int is_use_loadbin(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return dev->loadbin;
}

static int is_use_secisp(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return dev->secisp;
}

int hisp_rproc_enable_iommu(struct rproc *rproc, iommu_fault_handler_t handler)
{
	struct iommu_domain *domain;

	domain = hisi_ion_enable_iommu(NULL);
	if (!domain)
		return -EINVAL;

	rproc->domain = domain;
	//iommu_set_fault_handler(domain, handler, rproc);

	return 0; /*lint !e715 !e438*/
}

void hisp_rproc_disable_iommu(struct rproc *rproc)
{
    rproc->domain = NULL;
}

void hisp_rproc_init(struct rproc *rproc)
{
    if (use_sec_isp() || is_use_loadbin())
        rproc->fw_ops = &rproc_bin_fw_ops;
    else
        rproc->fw_ops = &rproc_elf_fw_ops;
	pr_info("[%s] elf.%pK, bin.%pK, use.%pK\n", __func__, (void *)&rproc_elf_fw_ops, (void *)&rproc_bin_fw_ops, (void *)rproc->fw_ops);

	INIT_LIST_HEAD(&rproc->dynamic_mems);
	INIT_LIST_HEAD(&rproc->reserved_mems);
	INIT_LIST_HEAD(&rproc->caches);
	INIT_LIST_HEAD(&rproc->cdas);
	INIT_LIST_HEAD(&rproc->pages);
}

/* page order */
static const unsigned int orders[] = {8, 4, 0};
#define ISP_NUM_ORDERS ARRAY_SIZE(orders)
static inline unsigned int order_to_size(int order)
{
    return PAGE_SIZE << order;
}
/* try to alloc largest page orders */
static struct page *alloc_largest_pages(gfp_t gfp_mask, unsigned long size, unsigned int max_order, unsigned int *order)
{
    struct page *page   = NULL;
    gfp_t gfp_flags     = 0;
    int i               = 0;

    if (order == NULL){
        pr_err("%s: order is NULL \n", __func__);
        return NULL;
        }

    for (i = 0; i < ISP_NUM_ORDERS; i++) {/*lint !e574*/
        if (size < order_to_size(orders[i]))
            continue;
        if (max_order < orders[i])
            continue;

        if (orders[i] >= 8) {
            gfp_flags = gfp_mask & (~__GFP_RECLAIM);
            gfp_flags |= __GFP_NOWARN;
            gfp_flags |= __GFP_NORETRY;
        }
        else if ( orders[i] >= 4) {
            gfp_flags = gfp_mask & (~__GFP_DIRECT_RECLAIM);
            gfp_flags |= __GFP_NOWARN;
            gfp_flags |= __GFP_NORETRY;
        }
        else
            gfp_flags = gfp_mask;

        page = alloc_pages(gfp_flags,orders[i]);
        if (!page){
            continue;
        }
        *order = orders[i];

        return page;
    }

    return NULL;
}
/* dynamic alloc page and creat sg_table */
static struct sg_table *isp_mem_sg_table_allocate(unsigned long length)
{
    struct list_head pages_list             = { 0 };
    struct rproc_page_info *page_info       = NULL;
    struct rproc_page_info *tmp_page_info   = NULL;
    struct sg_table *table       = NULL;
    struct scatterlist *sg       = NULL;
    unsigned long size_remaining = PAGE_ALIGN(length);
    unsigned int max_order       = orders[0];
    int alloc_pages_num          = 0;

    INIT_LIST_HEAD(&pages_list);

    while (size_remaining > 0) {
        page_info = kzalloc(sizeof(struct rproc_page_info), GFP_KERNEL);
        if (!page_info) {
            pr_err("%s: alloc rproc_page_info fail\n", __func__);
            goto free_pages;
        }
        page_info->page = alloc_largest_pages(GFP_KERNEL, size_remaining, max_order, &page_info->order);
        if (!page_info->page) {
            pr_err("%s: alloc largest pages failed!\n",__func__);
            kfree(page_info);
            goto free_pages;
        }

        list_add_tail(&page_info->node, &pages_list);
        size_remaining -= PAGE_SIZE << page_info->order;
        max_order = page_info->order;
        alloc_pages_num++;
    }
    table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
    if (!table)
        goto free_pages;

    if (sg_alloc_table(table, alloc_pages_num, GFP_KERNEL))
        goto free_table;

    sg = table->sgl;
    list_for_each_entry_safe(page_info, tmp_page_info, &pages_list, node){
        sg_set_page(sg, page_info->page, PAGE_SIZE << page_info->order, 0);
        sg = sg_next(sg);
        list_del(&page_info->node);
        kfree(page_info);
    }
    pr_debug("%s: pages num = %d, length = 0x%lx \n", __func__, alloc_pages_num,length);
    return table;

free_table:
    kfree(table);
free_pages:
    list_for_each_entry_safe(page_info, tmp_page_info, &pages_list, node){
        __free_pages(page_info->page,page_info->order);
        list_del(&page_info->node);
        kfree(page_info);
    }

    return NULL;
}
/* dynamic free page and sg_table */
static void isp_mem_sg_table_free(struct sg_table *table)
{
    struct scatterlist *sg  = NULL;
    int i                   = 0;

    if(!table)
    {
        pr_err("%s: table is NULL \n", __func__);
        return;
    }
    pr_debug("%s: table = 0x%pK \n", __func__, table);
    for_each_sg(table->sgl, sg, table->nents, i){/*lint !e574*/
        __free_pages(sg_page(sg),get_order(PAGE_ALIGN(sg->length)));
    }
    sg_free_table(table);
    kfree(table);
}
/* dynamic map kernel addr with sg_table */
static void *isp_mem_map_kernel(struct sg_table *table,unsigned long length)
{
    struct scatterlist *sg  = NULL;
    struct page *page       = NULL;
    void *vaddr             = NULL;
    int npages = PAGE_ALIGN(length) / PAGE_SIZE;
    int npages_this_entry   = 0;
    struct page **pages     = vmalloc(sizeof(struct page *) * npages);
    struct page **tmp       = pages;
    int i, j;

    if(!pages)
    {
        pr_err("%s: vmalloc failed. \n", __func__);
        return NULL;
    }
    if(!table)
    {
        pr_err("%s: table is NULL \n", __func__);
        return NULL;
    }

    for_each_sg(table->sgl, sg, table->nents, i) {/*lint !e574*/
        npages_this_entry = PAGE_ALIGN(sg->length) / PAGE_SIZE;
        page = sg_page(sg);
        BUG_ON(i >= npages);
        for (j = 0; j < npages_this_entry; j++)
            *(tmp++) = page++;/*lint !e613*/
    }

    vaddr = vmap(pages, npages, VM_MAP, PAGE_KERNEL);
    vfree(pages);
    if (!vaddr) {
        pr_err("%s: vmap failed.\n", __func__);
        return NULL;
    }
    return vaddr;
}
/* dynamic unmap kernel addr with sg_table */
static void isp_mem_unmap_kernel(void *va)
{
    if(!va)
    {
        pr_err("%s: va is NULL \n", __func__);
        return;
    }
    vunmap(va);
}
/* dynamic sg_table list_add_tail in page list */
/*lint -e429*/
static int isp_mem_add_page_list(struct rproc *rproc, struct sg_table *table)
{
    struct rproc_page *r_page    = NULL;/*lint !e429 */

    if(!table || !rproc)
    {
        pr_err("%s: table or rproc is NULL!\n", __func__);
        return -ENOENT;
    }

    r_page = kzalloc(sizeof(struct rproc_page), GFP_KERNEL);
    if (!r_page) {
        pr_err("%s: kzalloc failed. \n", __func__);
        return -ENOMEM;
    }

    r_page->table = table;

    list_add_tail(&r_page->node, &rproc->pages);
    return 0;
}

void hisp_free_rsctable(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

    if(rproc_dev->rsctable_vaddr != NULL) {
        kfree(rproc_dev->rsctable_vaddr);
        rproc_dev->rsctable_vaddr = NULL;
    }
}

/*lint +e429*/
void hisp_rproc_resource_cleanup(struct rproc *rproc)
{
	struct rproc_mem_entry *entry, *tmp;
	struct rproc_page *page_entry, *page_tmp;
	struct rproc_cache_entry *cache_entry, *cache_tmp;
	struct device *dev = &rproc->dev;

	/* clean up debugfs cda entries */
	list_for_each_entry_safe(entry, tmp, &rproc->cdas, node) {
		rproc_remove_trace_file(entry->priv);
		rproc->num_cdas--;
		list_del(&entry->node);
		kfree(entry);
	}

	/* clean up carveout allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->carveouts, node) {
		dma_free_coherent(dev->parent, entry->len, entry->va, entry->dma);
		list_del(&entry->node);
		kfree(entry);
	}

	/* clean up reserved allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->reserved_mems, node) {
            hisp_lock_sharedbuf();
            if (use_nonsec_isp())
                ;
            else {
                iounmap(entry->va);
            }
            /* reset the share parameter pointer */
            if (!use_sec_isp()) {
                if(!strncmp(entry->priv, "ISP_SHARED_MEM", strlen(entry->priv))) {
                    isp_share_para = NULL;
                    pr_info("reserved_mems : %s, isp_share_para: %pK\n",
                        (char *)entry->priv, isp_share_para);/*lint !e559 */
                }
            }
            hisp_unlock_sharedbuf();
	    list_del(&entry->node);
	    kfree(entry);
	}

	/* clean up dynamic allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->dynamic_mems, node) {
		vunmap(entry->va);
		list_del(&entry->node);
		kfree(entry);
	}

	/* free page */
	list_for_each_entry_safe(page_entry, page_tmp, &rproc->pages, node) {
		isp_mem_sg_table_free(page_entry->table);
		list_del(&page_entry->node);
		kfree(page_entry);
	}

	/* clean up cache entry */
	list_for_each_entry_safe(cache_entry, cache_tmp, &rproc->caches, node) {
		list_del(&cache_entry->node);
		kfree(cache_entry);
	}
	/*clean up sec tsctable mem*/
    if (is_use_secisp()) {
        if (use_sec_isp())
            free_secmem_rsctable();
        else if(is_use_loadbin())
            hisp_free_rsctable();
    }
	/* clean dynamic mem pool */
	hisp_dynamic_mem_pool_clean();
}

void hisp_virtio_boot_complete(struct rproc *rproc, int flag)
{
	if (flag != 0) {
		rproc->rproc_enable_flag = false;
		pr_err("[%s] Failed : handle resources flag.%d\n", __func__, flag);
	} else {
		rproc->rproc_enable_flag = true;
	}

	pr_info("[%s] flag.%d\n", __func__, flag);
	complete_all(&rproc->boot_comp);
	pr_info("[%s] -\n", __func__);
}

/*
* take a bootware
*/
static int rproc_bw_load(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	const char *name = rproc->bootware;
	int ret;

	ret = rproc_fw_sanity_check(rproc, fw);
	if (ret) {
		dev_err(dev, "%s:rproc_fw_sanity_check failed \n", __func__);
		return ret;
	}

	dev_info(dev, "Booting fw image %s, size %zd\n", name, fw->size);

	/* load the ELF segments to memory */
	ret = rproc_load_segments(rproc, fw);
	if (ret) {
		dev_err(dev, "Failed to load program segments.%s: %d\n", name, ret);
		return ret;
	}

	return 0;
}

int hisp_rproc_boot(struct rproc *rproc)
{
	const struct firmware *firmware_p = NULL;
	struct device *dev = NULL;
	int ret;

	if (!rproc) {
		pr_err("invalid rproc handle\n");
		return -EINVAL;
	}

	dev = &rproc->dev;

	ret = mutex_lock_interruptible(&rproc->lock);
	if (ret) {
		dev_err(dev, "can't lock rproc %s: %d\n", rproc->name, ret);
		return ret;
	}

	/* prevent underlying implementation from being removed */
	if (!try_module_get(dev->parent->driver->owner)) {
		dev_err(dev, "%s: can't get owner\n", __func__);
		ret = -EINVAL;
		goto unlock_mutex;
	}
	/* skip the boot process if rproc is already powered up */
	if (atomic_inc_return(&rproc->power) > 1) {
		dev_err(dev, "%s: rproc already powed up!\n", __func__);
		ret = -EEXIST;
		goto unlock_mutex;
	}

	dev_info(dev, "powering up %s\n", rproc->name);

    if (is_use_secisp()) {
        if (use_sec_isp()) {
            ret = sec_rproc_boot(rproc);
            if (0 != ret) {
            pr_err("%s: sec_rproc_boot failed.\n", __func__);
            }
            goto unlock_mutex;
        } else if(is_use_loadbin()){
            ret = nonsec_rproc_boot(rproc);
            if (0 != ret) {
            pr_err("%s: rproc_fw_boot failed.\n", __func__);
            } else {
                /* flush memory cache */
                rproc_memory_cache_flush(rproc);
            }
            goto unlock_mutex;
        }
    }

	/* loading a firmware is required */
	if (!rproc->firmware) {
		dev_err(dev, "%s: no firmware to load\n", __func__);
		ret = -EINVAL;
		goto unlock_mutex;
	}

	/* loading a bootware is required */
	if (!rproc->bootware) {
		dev_err(dev, "%s: no bootware to load\n", __func__);
		ret = -EINVAL;
		goto unlock_mutex;
	}

	/* load firmware */
	ret = request_firmware(&firmware_p, rproc->firmware, dev);
	if (ret < 0) {
		dev_err(dev, "request_firmware failed: %d\n", ret);
		goto unlock_mutex;
	}

	ret = rproc_fw_boot(rproc, firmware_p);
	release_firmware(firmware_p);
	if (0 != ret) {
		pr_err("%s: rproc_fw_boot failed.\n", __func__);
		goto unlock_mutex;
	}

	/* load bootware */
	ret = request_firmware(&firmware_p, rproc->bootware, dev);
	if (ret < 0) {
		dev_err(dev, "Failed: bootware request_firmware.%d\n", ret);
		goto unlock_mutex;
	}

	ret = rproc_bw_load(rproc, firmware_p);
	release_firmware(firmware_p);
	if (0 != ret) {
		pr_err("%s: rproc_bw_load failed.\n", __func__);
		goto unlock_mutex;
	}

	/* flush memory cache */
	rproc_memory_cache_flush(rproc);

unlock_mutex:
	mutex_unlock(&rproc->lock);/*lint !e455 */

	return ret;
}

unsigned long hisp_sg2virtio(struct virtqueue *vq, struct scatterlist *sg)
{
	struct virtio_device *vdev = vq->vdev;
	struct rproc_vdev *rvdev = container_of(vdev, struct rproc_vdev, vdev);
	struct rproc *rproc = rvdev->rproc;
	dma_addr_t dma;
    dma = sg_dma_address(sg);
	return ((dma - vq->dma_base) + rproc->ipc_addr);
}

/* rpmsg init msg dma and device addr */
void virtqueue_sg_init(struct scatterlist *sg, void *va, dma_addr_t dma, int size)
{
	sg_init_one(sg, va, size);
	sg_dma_address(sg) = dma;
	sg_dma_len(sg) = size;
}

/*lint -save -e429*/
int rpmsg_vdev_map_resource(struct virtio_device *vdev, dma_addr_t dma, int total_space)
{
    struct rproc_vdev *rvdev = container_of(vdev, struct rproc_vdev, vdev);
    struct rproc *rproc = rvdev->rproc;
    struct rproc_vring *rvring;
    struct rproc_mem_entry *vringmapping[RVDEV_NUM_VRINGS], *vqmapping = NULL;
    int i, j, index, ret = 0, size, prot = 0, unmaped;

    pr_info("[%s] +\n", __func__);
	if (use_sec_isp()) {
		/* map vring */
		for (i = 0; i < RVDEV_NUM_VRINGS; i++) {
			rvring = &rvdev->vring[i];
			index = A7VRING0 + i;
			size = PAGE_ALIGN(vring_size(rvring->len, rvring->align));/*lint !e666 */
			if ((ret = hisp_mem_type_pa_init(index, rvring->dma)) < 0) {
				pr_err("[%s] Failed : hisp_meminit.%d.(0x%x)\n", __func__, ret, index);
				return ret;
			}
		}

		/* map virtqueue */
		index = A7VQ;
		if ((ret = hisp_mem_type_pa_init(index, dma)) < 0) {
			pr_err("[%s] Failed : hisp_meminit.%d.(0x%x)\n", __func__, ret, index);
			return ret;
		}
		return 0;
	}

	if (!rproc->domain) {
		pr_err("[%s] Failed : domain don't exist!\n", __func__);
		return -EINVAL;
	}

	/* map vring */
    for (i = 0; i < RVDEV_NUM_VRINGS; i++)
        vringmapping[i] = NULL;
	for (i = 0; i < RVDEV_NUM_VRINGS; i++) {
		prot = IOMMU_READ | IOMMU_WRITE;
		rvring = &rvdev->vring[i];
		size = PAGE_ALIGN(vring_size(rvring->len, rvring->align));/*lint !e666 */

		ret = iommu_map(rproc->domain, rvring->da, rvring->dma, size, prot);
		if (ret) {
			pr_err("[%s] Failed : iommu_map.%d\n", __func__, ret);
			return ret;
		}

		vringmapping[i] = kzalloc(sizeof(struct rproc_mem_entry), GFP_KERNEL);
		if (!vringmapping[i]) {
			pr_err("[%s] Failed : vringmapping kzalloc\n", __func__);
			unmaped = iommu_unmap(rproc->domain, rvring->da, size);
			if (unmaped != size)
				pr_err("[%s] Failed : iommu_unmap size.%u, unmaped.%u\n",
						__func__, size, unmaped);
            ret = -ENOMEM;
            goto exit_vdevmap;
		}

		vringmapping[i]->da = rvring->da;
		vringmapping[i]->len = size;
		list_add_tail(&vringmapping[i]->node, &rproc->mappings);
	}

	/* map virtqueue*/
	prot = IOMMU_READ | IOMMU_WRITE;
	ret = iommu_map(rproc->domain, rproc->ipc_addr, (phys_addr_t)(dma),
                total_space, prot);
	if (ret) {
		pr_err("[%s] Failed : iommu_map.%d\n", __func__, ret);
        goto exit_vdevmap;
	}

	vqmapping = kzalloc(sizeof(struct rproc_mem_entry), GFP_KERNEL);
	if (!vqmapping) {
		pr_err("[%s] Failed : kzalloc\n", __func__);
		unmaped = iommu_unmap(rproc->domain, rproc->ipc_addr, total_space);
		if (unmaped != total_space)
			pr_err("[%s] Failed : iommu_unmap size.%u, unmaped.%u\n",
						__func__, total_space, unmaped);
		ret = -ENOMEM;
        goto exit_vdevmap;
	}
	vqmapping->da = rproc->ipc_addr;
	vqmapping->len = total_space;
	list_add_tail(&vqmapping->node, &rproc->mappings);

	return 0;
exit_vdevmap:
    for (j = 0; j < i; j ++)
        if (vringmapping[j])
            kfree(vringmapping[j]);
    return ret;
}

void set_rpmsg_status(int status)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    dev->rpmsg_status = status;
}

int sec_rproc_boot(struct rproc *rproc)
{
	struct device *dev = &rproc->dev;
	struct resource_table *table, *loaded_table;
	int ret, tablesz;

	if (!rproc->table_ptr)
		return -ENOMEM;

	/* look for the resource table */
	table = rproc_find_rsc_table(rproc, NULL, &tablesz);
	if (!table) {
	    pr_err("[%s] Failed : rproc_find_rsc_table.%pK\n", __func__, table);
		return -EINVAL;
	}
	/* Verify that resource table in loaded fw is unchanged */
	if (rproc->table_csum != crc32(0, table, tablesz)) {
		dev_err(dev, "Failed : resource checksum 0x%x = 0x%x\n", rproc->table_csum, crc32(0, table, tablesz));
		return -EINVAL;
	}
	/* set shared parameters for rproc*/
	ret = rproc_set_shared_para();
	if (ret) {
		dev_err(dev, "Failed : rproc_set_shared_para.%d\n", ret);
		goto clean_up;
	}

	/*
	 * The starting device has been given the rproc->cached_table as the
	 * resource table. The address of the vring along with the other
	 * allocated resources (carveouts etc) is stored in cached_table.
	 * In order to pass this information to the remote device we must
	 * copy this information to device memory.
	 */
	loaded_table = rproc_find_loaded_rsc_table(rproc, NULL);
	if (!loaded_table) {
	    pr_err("[%s] Failed : rproc_find_loaded_rsc_table.%pK\n", __func__, loaded_table);
	    ret = -EINVAL;
		goto clean_up;
	}

	memcpy(loaded_table, rproc->cached_table, tablesz);

	/* power up the remote processor */
	ret = rproc->ops->start(rproc);
	if (ret) {
		dev_err(dev, "can't start rproc %s: %d\n", rproc->name, ret);
		goto clean_up;
	}

	/*
	 * Update table_ptr so that all subsequent vring allocations and
	 * virtio fields manipulation update the actual loaded resource table
	 * in device memory.
	 */
	rproc->table_ptr = loaded_table;

	rproc->state = RPROC_RUNNING;

    return 0;
clean_up:
    rproc_resource_cleanup(rproc);
    return ret;
}

int hisp_rproc_rsctable_init(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

    pr_info("[%s] +\n", __func__);
    if (!rproc_dev->isp_bin_vaddr) {
        pr_err("[%s] isp_bin_vaddr.%pK\n", __func__, rproc_dev->isp_bin_vaddr);
        return -ENOMEM;
    }
    rproc_dev->rsctable_vaddr = kmemdup(rproc_dev->isp_bin_vaddr + 0x00003000, 0x1000, GFP_KERNEL);
    if (!rproc_dev->rsctable_vaddr) {
        pr_err("%s: kmalloc failed.\n", __func__);
        return -ENOMEM;
    }
    pr_info("[%s] -\n", __func__);

    return 0;
}

int hisi_firmware_load_func(struct rproc *rproc)
{
    int ret = 0;

    if (is_use_secisp()) {
        if (use_sec_isp()) {
            if ((ret = hisp_rsctable_init()) < 0) {
                pr_err("[%s] Failed : hisp_rsctable_init.%d\n", __func__, ret);
                return ret;
            }
        } else if(is_use_loadbin()){
            if ((ret = hisp_rproc_rsctable_init()) < 0) {
                pr_err("[%s] Failed : hisp_rproc_rsctable_init.%d\n", __func__, ret);
                return ret;
            }
        }
    }

    rproc_fw_config_virtio(NULL, rproc);
    return 0;
}
struct rproc_shared_para *rproc_get_share_para(void)
{
    pr_debug("%s: enter.\n", __func__);
    if (isp_share_para)
        return isp_share_para;

    pr_debug("%s: failed.\n", __func__);
    return NULL;
}

void set_shared_mdc_pa_addr(u64 mdc_pa_addr)
{
    struct rproc_shared_para *share_para = NULL;

    hisp_lock_sharedbuf();
    share_para = rproc_get_share_para();
    if (!share_para) {
        pr_err("[%s] Failed : share_para.%pK\n", __func__, share_para);
        hisp_unlock_sharedbuf();
        return;
    }

    share_para->mdc_pa_addr = mdc_pa_addr;
    hisp_unlock_sharedbuf();
}
/*lint -save -e429*/

void free_nesc_addr_ion(struct hisi_isp_ion_s *hisi_nescaddr_ion)
{
    if(hisi_nescaddr_ion == NULL){
        return;
    }
    if(hisi_nescaddr_ion->ion_client == NULL || hisi_nescaddr_ion->ion_handle == NULL){
        pr_err("in %s ion_client & ion_handle is NULL!\n", __func__);
        return;
    }
    ion_unmap_kernel(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
    ion_free(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
    ion_client_destroy(hisi_nescaddr_ion->ion_client);
    hisi_nescaddr_ion->ion_client = NULL;
    hisi_nescaddr_ion->ion_handle = NULL;
    hisi_nescaddr_ion->virt_addr  = NULL;
}

int hisp_ion_phys(struct ion_client *client, struct ion_handle *handle,
         dma_addr_t *addr)
{
    int ret = -ENODEV;
    int share_fd = 0;
    struct dma_buf *buf = NULL;
    struct dma_buf_attachment *attach = NULL;
    struct sg_table *sgt = NULL;
    struct scatterlist *sgl;
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct device *device = NULL;

    pr_info("[%s] +\n", __func__);

    if ((IS_ERR(client))||(IS_ERR(handle))) {
        pr_err("hisp_ion_phys failed \n");
        return -ENODEV;
    }

    device = &rproc_dev->isp_pdev->dev;

    share_fd = ion_share_dma_buf_fd(client, handle);
    if (share_fd < 0) {
        pr_err("[%s] Failed : ion_share_dma_buf_fd, share_fd.%d\n", __func__, share_fd);
        return share_fd;
    }

    buf = dma_buf_get(share_fd);
    if (IS_ERR(buf)) {
        pr_err("[%s] Failed : dma_buf_get, buf.%pK\n", __func__, buf);
        goto err_dma_buf_get;
    }

    attach = dma_buf_attach(buf, device);
    if (IS_ERR(attach)) {
        pr_err("[%s] Failed : dma_buf_attach, attach.%pK\n", __func__, attach);
        goto err_dma_buf_attach;
    }

    sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
    if (IS_ERR(sgt)) {
        pr_err("[%s] Failed : dma_buf_map_attachment, sgt.%pK\n", __func__, sgt);
        goto err_dma_buf_map_attachment;
    }

    sgl = sgt->sgl;
    if (sgl == NULL) {
        pr_err("[%s] Failed : sgl.NULL\n", __func__);
        goto err_sgl;
    }

    // Get physical addresses from scatter list
    *addr = sg_phys(sgl);/*[false alarm]:it's not the bounds of allocated memory */

    pr_info("[%s] -\n", __func__);
    ret = 0;

err_sgl:
    dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
err_dma_buf_map_attachment:
    dma_buf_detach(buf, attach);
err_dma_buf_attach:
    dma_buf_put(buf);
err_dma_buf_get:
    sys_close(share_fd);
    return ret;
}

struct hisi_isp_ion_s *get_nesc_addr_ion(size_t size, size_t align, unsigned int heap_id_mask, unsigned int flags)
{
    struct hisi_isp_ion_s *hisi_nescaddr_ion = NULL;
    ion_phys_addr_t nesc_addr_ion_phys = 0x0 ;
    int ret = 0;

    hisi_nescaddr_ion = kzalloc(sizeof(struct hisi_isp_ion_s), GFP_KERNEL);/*lint !e838 */
    if (!hisi_nescaddr_ion) {
        pr_err("in %s hisi_nescaddr_ion kzalloc is failed\n", __func__);
        return NULL;
    }

    hisi_nescaddr_ion->ion_client = hisi_ion_client_create("nescaddr_ion_client");
    if (IS_ERR(hisi_nescaddr_ion->ion_client)) {
        pr_err("hisi hisi_nescaddr_ion create failed \n");
        kfree(hisi_nescaddr_ion);
        return NULL;
    }

    hisi_nescaddr_ion->ion_handle = ion_alloc(hisi_nescaddr_ion->ion_client, size, align, heap_id_mask, flags);
    if (IS_ERR(hisi_nescaddr_ion->ion_handle)){
        pr_err("hisi ion_alloc failed \n");
        ion_client_destroy(hisi_nescaddr_ion->ion_client);
        kfree(hisi_nescaddr_ion);
        return NULL;
    }
    hisi_nescaddr_ion->virt_addr = ion_map_kernel(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
    if(hisi_nescaddr_ion->virt_addr == NULL)
    {
        pr_err("hisi ion_map_kernel failed \n");
        ion_free(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
        ion_client_destroy(hisi_nescaddr_ion->ion_client);
        kfree(hisi_nescaddr_ion);
        return NULL;
    }

    ret = hisp_ion_phys(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle,(dma_addr_t *)&nesc_addr_ion_phys);/*lint !e838 */
    if (ret < 0)
    {
        pr_err("%s, failed to get phy addr,ret:%d!\n", __func__, ret);
        if((hisi_nescaddr_ion->ion_client != NULL) && (hisi_nescaddr_ion->ion_handle != NULL)){
        ion_unmap_kernel(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
        ion_free(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
        ion_client_destroy(hisi_nescaddr_ion->ion_client);
        kfree(hisi_nescaddr_ion);
        return NULL;
        }
        else{
            pr_err("%s, ion_client & ion_handle is NULL!\n", __func__);
        }
    }
    if(nesc_addr_ion_phys)
        hisi_nescaddr_ion->paddr = nesc_addr_ion_phys;

    return hisi_nescaddr_ion;
}
/*lint -restore */

/* rproc private da to va */
void *hisp_rproc_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct rproc_mem_entry *dynamic_mem, *reserved_mem;
	void *ptr = NULL;

	list_for_each_entry(reserved_mem, &rproc->reserved_mems, node) {
		int offset = da - reserved_mem->da;

		/* try next if da is too small */
		if (offset < 0)
			continue;

		/* try next if da is too large */
		if (offset + len > reserved_mem->len)
			continue;

		ptr = reserved_mem->va + offset;

		return ptr;
	}

	list_for_each_entry(dynamic_mem, &rproc->dynamic_mems, node) {
		int offset = da - dynamic_mem->da;

		/* try next if da is too small */
		if (offset < 0)
			continue;

		/* try next if da is too large */
		if (offset + len > dynamic_mem->len)
			continue;

		ptr = dynamic_mem->va + offset;

		return ptr;
	}

    return ptr;
}

/**
 * rproc_handle_version() - handle the verison information
 * @rproc: the remote processor
 * @rsc: the trace resource descriptor
 * @avail: size of available data (for sanity checking the image)
 */
int rproc_handle_version(struct rproc *rproc, struct fw_rsc_version *rsc,
                                int offset, int avail)
{
	printk("[%s] Firmware_version: magic = %x, module = %s, version = %s, build_time = %s, reserved = %s.\n", __func__,
							rsc->magic, rsc->module, rsc->version, rsc->build_time,
							rsc->reserved != 0?rsc->reserved:"NULL");

	return 0;
}

/**
 * rproc_handle_cda() - handle a shared trace buffer resource
 * @rproc: the remote processor
 * @rsc: the cda resource descriptor
 * @avail: size of available data (for sanity checking the image)
 *
 * In case the remote processor dumps cda bin into memory,
 * export it via debugfs.
 *
 * Currently, the 'da' member of @rsc should contain the device address
 * where the remote processor is dumping the cdas. Later we could also
 * support dynamically allocating this address using the generic
 * DMA API (but currently there isn't a use case for that).
 *
 * Returns 0 on success, or an appropriate error code otherwise
 */
 /*lint -save -e429 */
int rproc_handle_cda(struct rproc *rproc, struct fw_rsc_cda *rsc,
							int offset, int avail)
{
	struct rproc_mem_entry *cda;
	struct device *dev = &rproc->dev;
	void *ptr;
	char name[15];

	if (sizeof(*rsc) > avail) {/*lint !e574 */
		dev_err(dev, "cda rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "trace rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	/* what's the kernel address of this resource ? */
	ptr = rproc_da_to_va(rproc, rsc->da, rsc->len);
	if (!ptr) {
		dev_err(dev, "erroneous cda resource entry\n");
		return -EINVAL;
	}

	cda = kzalloc(sizeof(*cda), GFP_KERNEL);
	if (!cda) {
		dev_err(dev, "kzalloc trace failed\n");
		return -ENOMEM;
	}

	/* set the trace buffer dma properties */
	cda->len = rsc->len;
	cda->va = ptr;

	/* make sure snprintf always null terminates, even if truncating */
	snprintf(name, sizeof(name), "cda%d", rproc->num_cdas);

	/* create the debugfs entry */
	cda->priv = rproc_create_cda_file(name, rproc, cda);
	if (!cda->priv) {
		cda->va = NULL;
		kfree(cda);
		return -EINVAL;
	}

	list_add_tail(&cda->node, &rproc->cdas);

	rproc->num_cdas++;

	dev_dbg(dev, "%s added: va %pK, da 0x%x, len 0x%x\n", name, ptr,
						rsc->da, rsc->len);

	return 0;
}
/*lint -restore */
void rproc_memory_cache_flush(struct rproc *rproc)
{
	struct rproc_cache_entry *tmp;

	list_for_each_entry(tmp, &rproc->caches, node)
		__flush_dcache_area(tmp->va, tmp->len);
}

/**
* rproc_handle_dynamic_memory() - handle phys non-contiguous memory allocation requests
* @rproc: rproc handle
* @rsc: the resource entry
* @avail: size of available data (for image validation)
*
* This function will handle firmware requests for allocation of physically
* contiguous memory regions.
*
* These request entries should come first in the firmware's resource table,
* as other firmware entries might request placing other data objects inside
* these memory regions (e.g. data/code segments, trace resource entries, ...).
*
* Allocating memory this way helps utilizing the reserved physical memory
* (e.g. CMA) more efficiently, and also minimizes the number of TLB entries
* needed to map it (in case @rproc is using an IOMMU). Reducing the TLB
* pressure is important; it may have a substantial impact on performance.
*/
/*lint -save -e429*/
int rproc_handle_dynamic_memory(struct rproc *rproc,
										struct fw_rsc_dynamic_memory *rsc,
										int offset, int avail)
{
	struct rproc_mem_entry *dynamic_mem, *mapping;
	struct rproc_cache_entry *cache_entry;
	struct device *dev = &rproc->dev;
	struct sg_table *table;
	void *va;
	int ret = -1;

	if (sizeof(*rsc) > avail) {/*lint !e574 */
		dev_err(dev, "dynamic_mem rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "dynamic_mem rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	dev_info(dev, "dynamic_mem rsc: len %x, flags %x\n", rsc->len, rsc->flags);

	cache_entry = kzalloc(sizeof(*cache_entry), GFP_KERNEL);
	if (!cache_entry) {
		dev_err(dev, "kzalloc cache_entry failed\n");
		return -ENOMEM;
	}

	dynamic_mem = kzalloc(sizeof(*dynamic_mem), GFP_KERNEL);
	if (!dynamic_mem) {
		dev_err(dev, "kzalloc dynamic_mem failed\n");
		ret = -ENOMEM;
		goto free_cache;
	}

	table = isp_mem_sg_table_allocate(rsc->len);
	if(table == NULL){
		dev_err(dev, "%s:vaddr_to_sgl failed\n", __func__);
		goto free_memory;
	}

	ret = isp_mem_add_page_list(rproc,table);
	if (ret < 0) {
		dev_err(dev, "isp_mem_add_page_list failed: ret %d \n", ret);
		goto free_memory;
	}

	va = isp_mem_map_kernel(table,rsc->len);
	if(va == NULL){
		dev_err(dev, "%s:vaddr_to_sgl failed\n", __func__);
		goto free_table;
	}

	/* mapping */
	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			ret = -ENOMEM;
			goto free_map;
		}

        ret = iommu_map_sg(rproc->domain, rsc->da, table->sgl, sg_nents(table->sgl), rsc->flags);
		if (ret != rsc->len) {
			dev_err(dev, "hisi_iommu_map_range failed: ret %d len %d\n", ret, rsc->len);
			goto free_mapping;
		}

		/*
		* We'll need this info later when we'll want to unmap
		* everything (e.g. on shutdown).
		*
		* We can't trust the remote processor not to change the
		* resource table, so we must maintain this info independently.
		*/
		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);
	}

	/*
	* Some remote processors might need to know the pa
	* even though they are behind an IOMMU. E.g., OMAP4's
	* remote M3 processor needs this so it can control
	* on-chip hardware accelerators that are not behind
	* the IOMMU, and therefor must know the pa.
	*
	* Generally we don't want to expose physical addresses
	* if we don't have to (remote processors are generally
	* _not_ trusted), so we might want to do this only for
	* remote processor that _must_ have this (e.g. OMAP4's
	* dual M3 subsystem).
	*
	* Non-IOMMU processors might also want to have this info.
	* In this case, the device address and the physical address
	* are the same.
	*/

	dynamic_mem->va = va;
	dynamic_mem->len = rsc->len;
	dynamic_mem->dma = rsc->da;
	dynamic_mem->da = rsc->da;
	dynamic_mem->priv = rsc->name;

	list_add_tail(&dynamic_mem->node, &rproc->dynamic_mems);

	/* save cache entry */
	cache_entry->va = va;
	cache_entry->len = rsc->len;
	list_add_tail(&cache_entry->node, &rproc->caches);

	return 0;

free_mapping:
	kfree(mapping);
free_map:
	isp_mem_unmap_kernel(va);
free_table:
	isp_mem_sg_table_free(table);
free_memory:
	kfree(dynamic_mem);
free_cache:
	kfree(cache_entry);
	return ret;
}
/*lint -restore */
/* rproc_handle_reserved_memory() - handle phys reserved memory allocation requests */
/*lint -save -e429*/
int rproc_handle_reserved_memory(struct rproc *rproc,
											struct fw_rsc_reserved_memory *rsc,
											int offset, int avail)
{
	struct rproc_mem_entry *reserved_mem, *mapping;
	struct rproc_cache_entry *cache_entry;
	struct device *dev = &rproc->dev;
	u64 addr;
	void *va = NULL;
	int ret = -1;

    if (sizeof(*rsc) > avail) {/*lint !e574 */
        dev_err(dev, "memory rsc is truncated\n");
        return -EINVAL;
    }

    /* make sure reserved bytes are zeroes */
    if (rsc->reserved) {
        dev_err(dev, "memory rsc has non zero reserved bytes\n");
        return -EINVAL;
    }

    if (!strncmp(rsc->name, "ISP_MEM_BOOTWARE", strlen(rsc->name)))/*lint !e64 */
        addr = get_a7remap_addr();
    else if (!strncmp(rsc->name, "ISP_MDH_MEM", strlen(rsc->name))){/*lint !e64 */
        addr = get_mdc_addr_pa();/*lint !e747 */
        if (!addr)
        {
            dev_err(dev, "get_mdc_addr_pa err! \n");
            return -EINVAL;
        }
        set_shared_mdc_pa_addr(addr);
    }
    else
        addr = rsc->pa;

	cache_entry = kzalloc(sizeof(*cache_entry), GFP_KERNEL);
	if (!cache_entry) {
		dev_err(dev, "kzalloc cache_entry failed\n");
		return -ENOMEM;
	}

	reserved_mem = kzalloc(sizeof(*reserved_mem), GFP_KERNEL);
	if (!reserved_mem) {
		dev_err(dev, "kzalloc reserved_mem failed\n");
		ret = -ENOMEM;
		goto free_cache;
	}

    pr_info("%s: ioremap before.\n", __func__);
    if (use_nonsec_isp()) {
        if (!strncmp(rsc->name, "ISP_MEM_BOOTWARE", strlen(rsc->name))) {/*lint !e64 */
            va = get_a7remap_va();
            pr_info("%s: nsec boot\n", __func__);
        }
        else if(!strncmp(rsc->name, "ISP_MDH_MEM", strlen(rsc->name))){/*lint !e64 */
            va = get_mdc_addr_va();
            if(!va)
            {
                dev_err(dev, "get_mdc_addr_va NULL! \n");
                goto free_memory;
            }
            pr_debug("%s: ISP_MDH_MEM va.0x%pK\n", __func__,va);
        }
    } else {
        va = ioremap(addr, rsc->len);/*lint !e747 */
        if (!va) {
            dev_err(dev, "ioremap failed addr\n");
            ret = -ENOMEM;
            goto free_memory;
        }
    }

	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			ret = -ENOMEM;
			goto free_iomem;
		}

		ret = iommu_map(rproc->domain, rsc->da, addr, rsc->len, rsc->flags);
		if (ret) {
			dev_err(dev, "iommu_map failed: %d\n", ret);
			goto free_mapping;
		}

		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);
	}

	reserved_mem->va = va;
	reserved_mem->len = rsc->len;
	reserved_mem->dma = rsc->da;
	reserved_mem->da = rsc->da;
	reserved_mem->priv = rsc->name;

	list_add_tail(&reserved_mem->node, &rproc->reserved_mems);

	/* save cache entry */
	cache_entry->va = va;
	cache_entry->len = rsc->len;
	list_add_tail(&cache_entry->node, &rproc->caches);

	return 0;

free_mapping:
	kfree(mapping);
free_iomem:
if (use_nonsec_isp()) {
        if (!strncmp(rsc->name, "ISP_MEM_BOOTWARE", strlen(rsc->name)))/*lint !e64 */
            ;
    } else /*lint !e548 */
        iounmap(va);
free_memory:
	kfree(reserved_mem);
free_cache:
	kfree(cache_entry);
	return ret;
}
/*lint -restore */
/* rproc_handle_rdr_memory() - handle phys rdr memory allocation requests */
/*lint -save -e429*/
int rproc_handle_rdr_memory(struct rproc *rproc,
									struct fw_rsc_carveout *rsc,
									int offset, int avail)
{
	struct rproc_mem_entry *mapping;
	struct device *dev = &rproc->dev;
	int ret = -1;
	u64 isprdr_addr;

	isprdr_addr = get_isprdr_addr();
	pr_info("ispRDR ==>> (Len.0x%x, Flag.0x%x).%s\n", rsc->len, rsc->flags, rsc->name);

	if (0 == isprdr_addr) {
		pr_info("%s: rdr func is off.\n", __func__);
		return 0;
	}

	if (sizeof(*rsc) > avail) {/*lint !e574 */
		dev_err(dev, "memory rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "memory rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			return -ENOMEM;
		}

		ret = iommu_map(rproc->domain, rsc->da, isprdr_addr, rsc->len, rsc->flags);
		if (ret) {
			dev_err(dev, "iommu_map failed: %d\n", ret);
			goto free_mapping;
		}

		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);
	}

	return 0;

free_mapping:
	kfree(mapping);

	return ret;
}
/*lint -restore */
/* rproc_handle_shared_memory() - handle phys shared parameters memory allocation requests */
/*lint -save -e429*/
int rproc_handle_shared_memory(struct rproc *rproc,
										struct fw_rsc_carveout *rsc,
										int offset, int avail)
{
	struct rproc_mem_entry *reserved_mem, *mapping;
	struct device *dev = &rproc->dev;
	u64 a7sharedmem_addr;
	void *va;
	int ret = -1;

	pr_info("%s: entern.\n", __func__);
	if (sizeof(*rsc) > avail) {/*lint !e574 */
		dev_err(dev, "memory rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "memory rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	a7sharedmem_addr = get_a7sharedmem_addr();
	dev_err(dev, "%s: len %x, flags %x\n",
					rsc->name, rsc->len, rsc->flags);

	reserved_mem = kzalloc(sizeof(*reserved_mem), GFP_KERNEL);
	if (!reserved_mem) {
		dev_err(dev, "kzalloc reserved_mem failed\n");
		return -ENOMEM;
	}

    if (use_nonsec_isp())
        va = get_a7sharedmem_va();
    else {
        va = ioremap_wc(a7sharedmem_addr, rsc->len);
        if (!va) {
            dev_err(dev, "ioremap failed a7sharedmem_addr\n");
            ret = -ENOMEM;
            goto free_memory;
        }
    }
    pr_info("[%s] va.%pK\n", __func__, va);

	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			ret = -ENOMEM;
			goto free_iomem;
		}

		ret = iommu_map(rproc->domain, rsc->da, a7sharedmem_addr, rsc->len, rsc->flags);
		if (ret) {
			dev_err(dev, "iommu_map failed: %d\n", ret);
			goto free_mapping;
		}

		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);
	}

	reserved_mem->va = va;
	reserved_mem->len = rsc->len;
	reserved_mem->dma = rsc->da;
	reserved_mem->da = rsc->da;
	reserved_mem->priv = rsc->name;

	list_add_tail(&reserved_mem->node, &rproc->reserved_mems);
    pr_info("[%s] name.%s, (0x%x) -> %pK\n", __func__, rsc->name, rsc->len, va);

    hisp_lock_sharedbuf();
	isp_share_para = (struct rproc_shared_para *)va;
	memset(isp_share_para, 0, rsc->len);
    pr_info("[%s] isp_share_para.%pK, va.%pK\n", __func__, isp_share_para, va);
	hisp_unlock_sharedbuf();

    isploglevel_update();
#ifdef DEBUG_HISI_ISP
    ispperfctrl_update();
    ispmonitor_update();
    ispcoresight_update();
#endif
    pr_info("[%s] -\n", __func__);

	return 0;

free_mapping:
	kfree(mapping);
free_iomem:
    if (use_nonsec_isp())
        ;
    else
        iounmap(va);
free_memory:
	kfree(reserved_mem);

	return ret;
}

unsigned int dynamic_memory_map(struct scatterlist *sgl,\
    size_t addr,\
    size_t size,\
    unsigned int prot)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc *rproc;
    size_t phy_len = 0;

    if((rproc_dev->isp_rproc == NULL)||(rproc_dev->isp_rproc->rproc == NULL))
    {
        pr_err("%s: isp_rproc is NULL\n",__func__);
        return 0;
    }
    pr_info("[%s] size.0x%lx, prot.0x%x\n", __func__, size, prot);
    rproc = rproc_dev->isp_rproc->rproc;
    if(rproc->domain == NULL)
    {
        pr_err("%s: rproc->domain is NULL!\n",__func__);
        return 0;
    }
    if(sgl == NULL)
    {
        pr_err("%s: sgl is NULL!\n",__func__);
        return 0;
    }
    phy_len = iommu_map_sg(rproc->domain, addr, sgl, sg_nents(sgl), prot);
    if (phy_len != size) {
        pr_err("%s: iommu_map_sg failed! phy_len.0x%lx, size.0x%lx\n",__func__, phy_len, size);
        dynamic_memory_unmap(addr,phy_len);
        return 0;
    }
    return addr;
}
int dynamic_memory_unmap(size_t addr, size_t size)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc *rproc;
    size_t phy_len = 0;

    if((rproc_dev->isp_rproc == NULL)||(rproc_dev->isp_rproc->rproc == NULL))
    {
        pr_err("%s: isp_rproc is NULL\n",__func__);
        return -EINVAL;
    }
    rproc = rproc_dev->isp_rproc->rproc;
    if((addr != MEM_RAW2YUV_DA) || (size != MEM_RAW2YUV_SIZE)){
        pr_err("%s: check mem error!addr = 0x%lx; size = 0x%lx\n",__func__, addr, size);
        return -EINVAL;
    }
    if(rproc->domain == NULL)
    {
        pr_err("%s: rproc->domain is NULL!\n",__func__);
        return -ENOMEM;
    }
    phy_len = iommu_unmap(rproc->domain, addr, size);
    if (phy_len != size) {
        pr_err("%s: iommu_unmap failed: phy_len 0x%lx size 0x%lx\n\n",__func__, phy_len, size);
        return -EINVAL;
    }
    return 0;
}

/*lint -restore */

static int get_a7log_mode(void)
{
    struct device_node *np = NULL;
    int ret = 0, a7log_mode = 0;

    np = of_find_compatible_node(NULL, NULL, "hisilicon,prktimer");
    if (!np) {
        printk("NOT FOUND device node 'hisilicon,prktimer'!\n");
        return -ENXIO;
    }

    ret = of_property_read_u32(np, "fpga_flag", &a7log_mode);/*lint !e64*/
    if (ret) {
        printk("failed to get fpga_flag resource.\n");
        return -ENXIO;
    }

    return a7log_mode;
}
u32 get_share_exc_flag(void)
{
    struct rproc_shared_para *share_para = NULL;
    u32 exc_flag = 0x0;

    hisp_lock_sharedbuf();
    share_para = rproc_get_share_para();
    if (!share_para) {
        RPROC_ERR("Failed : rproc_get_share_para.%pK\n", share_para);
        hisp_unlock_sharedbuf();
        return 0xFFFFFFFF;
    }
    exc_flag = share_para->exc_flag;
    hisp_unlock_sharedbuf();
    return exc_flag;
}

int set_plat_parameters(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    struct rproc_shared_para *param = NULL;

    hisp_lock_sharedbuf();
    param = rproc_get_share_para();
    if (!param) {
        RPROC_ERR("Failed : rproc_get_share_para.%pK\n", param);
        hisp_unlock_sharedbuf();
        return -EINVAL;
    }

    param->plat_cfg.perf_power      = perf_para;
    param->plat_cfg.platform_id     = hw_is_fpga_board();
    param->plat_cfg.isp_local_timer = dev->tmp_plat_cfg.isp_local_timer;
    param->rdr_enable_type         |= ISPCPU_RDR_USE_APCTRL; /*lint !e648*/
    param->logx_switch             |= ISPCPU_LOG_USE_APCTRL;
    param->exc_flag                 = 0x0;
    if (get_a7log_mode())
        param->logx_switch         |= ISPCPU_LOG_TIMESTAMP_FPGAMOD;

    param->bootware_paddr           = get_a7remap_addr();
    RPROC_INFO("platform_id = %d, isp_local_timer = %d, perf_power = %d, logx_switch.0x%x\n",
        param->plat_cfg.platform_id, param->plat_cfg.isp_local_timer,
        param->plat_cfg.perf_power, param->logx_switch);
    hisp_unlock_sharedbuf();

    isploglevel_update();
#ifdef DEBUG_HISI_ISP
    ispperfctrl_update();
    ispmonitor_update();
    ispcoresight_update();
#endif
    return 0;
}

int rproc_set_shared_para(void)
{
	struct rproc_shared_para *share_para = NULL;
	int ret, i;

	ret = set_plat_parameters();
	if (ret) {
		pr_err("%s: set_plat_parameters failed.\n", __func__);
		return ret;
	}

    hisp_lock_sharedbuf();
    share_para = rproc_get_share_para();
	if (!share_para) {
		pr_err("%s:rproc_get_share_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return -EINVAL;
	}

    share_para->bootware_paddr = get_a7remap_addr();

	if (get_isprdr_addr())
		share_para->rdr_enable = 1;

	share_para->rdr_enable_type |= RDR_CHOOSE_TYPE;
	for (i = 0; i < IRQ_NUM; i++)
		share_para->irq[i] = 0;

	pr_info("%s: platform_id = 0x%x, timer = 0x%x, rdr_enable = %d, rdr_enable_type = %d\n",
		__func__, share_para->plat_cfg.platform_id,
		share_para->plat_cfg.isp_local_timer, share_para->rdr_enable,
		share_para->rdr_enable_type);
    hisp_unlock_sharedbuf();

	return ret;
}

void rproc_set_shared_clk_value(int type,unsigned int value)
{
	struct rproc_shared_para *share_para = NULL;

	if((type >= ISP_CLK_MAX) || (type < 0)) {
		pr_err("%s:type error.%d\n", __func__,type);
		return;
	}

	hisp_lock_sharedbuf();
	share_para = rproc_get_share_para();
	if (!share_para) {
		pr_err("%s:rproc_get_share_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return;
	}
	share_para->clk_value[type] = value;
	pr_debug("%s type.%d = %u\n", __func__,type,value);
	hisp_unlock_sharedbuf();

	return;
}

static void rproc_set_shared_clk_init(void)
{
	struct rproc_shared_para *share_para = NULL;

	hisp_lock_sharedbuf();
	share_para = rproc_get_share_para();
	if (!share_para) {
		pr_err("%s:rproc_get_share_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return;
	}
	memset(share_para->clk_value, 0, sizeof(share_para->clk_value));/*lint !e838 */
	hisp_unlock_sharedbuf();

	return;
}

static void hisi_isp_efuse_deskew(void)
{
    int ret = 0;
    unsigned char efuse = 0xFF;
    struct rproc_shared_para *share_para = NULL;
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

    RPROC_INFO("+\n");
    if(rproc_dev->probe_finished != 1) {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return;
    }

    if (rproc_dev->isp_efuse_flag == 0) {
        RPROC_ERR("isp_efuse_flag.%d\n", rproc_dev->isp_efuse_flag);
        return;
    }

    if ((ret = get_efuse_deskew_value(&efuse, 1, 1000)) < 0) {
        pr_err("[%s] Failed: ret.%d\n", __func__, ret);
    }

    pr_err("[%s] : efuse.%d\n", __func__, ret);
    hisp_lock_sharedbuf();
    share_para = rproc_get_share_para();
    if (!share_para) {
        pr_err("%s:rproc_get_share_para failed.\n", __func__);
        hisp_unlock_sharedbuf();
        return;
    }
    share_para->isp_efuse = efuse;
    hisp_unlock_sharedbuf();

    RPROC_INFO("-\n");
}

int rproc_bootware_attach(struct rproc *rproc, const char *bootware)
{
	rproc->bootware = bootware;
	return 0;
}

int use_nonsec_isp(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return ((dev->case_type == NONSEC_CASE) ? 1 : 0);
}

int use_sec_isp(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return ((dev->case_type == SEC_CASE) ? 1 : 0);
}

int hisi_isp_rproc_case_set(enum hisi_isp_rproc_case_attr type)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    if (type >= INVAL_CASE) {
		pr_err("%s: invalid case, type = %u\n", __func__, type);
        return -EINVAL;
    }

    if (atomic_read(&dev->rproc_enable_status) > 0 ) {
        pr_err("[%s] hisi_isp_rproc had been enabled, rproc_enable_status.0x%x\n", __func__, atomic_read(&dev->rproc_enable_status));
        return -ENODEV;
    }

	if (sync_isplogcat() < 0)
	    pr_err("[%s] Failed: sync_isplogcat\n", __func__);

    dev->case_type = type;

	if (type == SEC_CASE)
		hisi_ispsec_share_para_set();

    if (likely(hisi_rproc)) { /*lint !e730 */
        if ((type == SEC_CASE) || is_use_loadbin())
            hisi_rproc->fw_ops = &rproc_bin_fw_ops;
        else
            hisi_rproc->fw_ops = &rproc_elf_fw_ops;
    }

	pr_info("%s.%d: type.%u, rporc.%pK\n", __func__, __LINE__, type, hisi_rproc);

	pr_info("[%s] elf_ops.%pK, bin_ops.%pK, use_ops.%pK\n",
			__func__, (void *)&rproc_elf_fw_ops,
			(void *)&rproc_bin_fw_ops,
			hisi_rproc ? (void *)hisi_rproc->fw_ops : NULL);

    return 0;
}

enum hisi_isp_rproc_case_attr hisi_isp_rproc_case_get(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    return dev->case_type;
}

char *hisp_get_comp_name(void)
{
	return DTS_COMP_NAME;
}

char *hisp_get_logicname(void)
{
	return DTS_COMP_LOGIC_NAME;
}

int hisp_set_clk_rate(unsigned int type, unsigned int rate)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = -EINVAL;
    if(dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }

    if (!check_dvfs_valid()) {
        pr_err("[%s] Failed : check_dvfs_valid\n", __func__);
        return -EINVAL;
    }

    switch (dev->case_type) {
        case NONSEC_CASE:
            if ((ret = nsec_setclkrate(type, rate)) < 0)
                pr_err("[%s] Failed : nsec_setclkrate.%d\n", __func__, ret);
            break;
        case SEC_CASE:
            if ((ret = sec_setclkrate(type, rate)) < 0)
                pr_err("[%s] Failed : sec_setclkrate.%d\n", __func__, ret);
            break;
        default:
            pr_err("[%s] Unsupported case_type.%d\n", __func__, dev->case_type);
            return -EINVAL;
    }

    return ret;
}

unsigned long hisp_get_clk_rate(unsigned int type)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    if(dev->probe_finished != 1) {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return 0;
    }

    if(get_rproc_enable_status() == 0) {
        pr_err("[%s] ispcpu not start!\n", __func__);
        return 0;
    }

    return secnsec_getclkrate(type);
}

int hisi_isp_rproc_setpinctl(struct pinctrl *isp_pinctrl, struct pinctrl_state *pinctrl_def, struct pinctrl_state *pinctrl_idle)
{
	struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

    if (hw_is_fpga_board()) {
        RPROC_ERR("this board is fpga board, don't need to init pinctrl.\n");
        return 0;
    }

	rproc_dev->isp_data.isp_pinctrl = isp_pinctrl;
	rproc_dev->isp_data.pinctrl_def = pinctrl_def;
	rproc_dev->isp_data.pinctrl_idle = pinctrl_idle;

	return 0;
}

int hisi_isp_rproc_setclkdepend(struct clk *aclk_dss, struct clk *pclk_dss)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

	if (hw_is_fpga_board()) {
		RPROC_ERR("this board is fpga board, don't need to init clkdepend.\n");
		return 0;
	}

	rproc_dev->isp_data.aclk_dss = aclk_dss;
	rproc_dev->isp_data.pclk_dss = pclk_dss;

	return 0;
}

int hisi_isp_dependent_clock_enable(void)
{
	struct rproc_boot_device *dev = (struct rproc_boot_device *)&rproc_boot_dev;
    int rc = 0;
    if(dev->boardid > HI6250_BOARDID)
        return rc;
    RPROC_INFO("Enable dss aclk, pclk\n");
    if(dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }

    rc = clk_prepare(dev->isp_data.aclk_dss);
    if (rc)
        RPROC_ERR("Failed : aclk_dss clk_prepare.%d\n", rc);

    rc = clk_enable(dev->isp_data.aclk_dss);
    if (rc)
        RPROC_ERR("Failed : aclk_dss clk_enable.%d\n", rc);

    rc = clk_prepare(dev->isp_data.pclk_dss);
    if (rc)
        RPROC_ERR("Failed : pclk_dss clk_prepare.%d", rc);

    rc = clk_enable(dev->isp_data.pclk_dss);
    if (rc)
        RPROC_ERR("Failed : pclk_dss clk_enable.%d\n", rc);

    return rc;
}

int hisi_isp_dependent_clock_disable(void)
{
    struct rproc_boot_device *dev = (struct rproc_boot_device *)&rproc_boot_dev;
    if(dev->boardid > HI6250_BOARDID)
        return 0;
    RPROC_INFO("Disable dss aclk, pclk.");
    if(dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }

    clk_disable(dev->isp_data.pclk_dss);
    clk_unprepare(dev->isp_data.pclk_dss);

    clk_disable(dev->isp_data.aclk_dss);
    clk_unprepare(dev->isp_data.aclk_dss);

    return 0;
}

u64 get_a7remap_addr(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    if (use_nonsec_isp())
        return dev->nsec_remap_addr;
    else
        return dev->remap_addr;
}

u64 get_a7sharedmem_addr(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    if (use_nonsec_isp())
        return (dev->nsec_remap_addr + 0xF000);
    else
        return (dev->remap_addr + 0xF000);
}

void set_a7mem_pa(u64 addr)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    dev->nsec_remap_addr = addr;
}

void *get_a7remap_va(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    pr_info("%s: va = %pK\n", __func__, dev->remap_va);
    return dev->remap_va;
}

void *get_a7sharedmem_va(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    //pr_info("%s: va = %pK\n", __func__, dev->remap_va + 0xf000);
    return (dev->remap_va + 0xF000);
}

void set_a7mem_va(void *addr)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    dev->remap_va = addr;
    pr_info("%s: remap_va = %pK\n", __func__, dev->remap_va);
}

int _stat_poll(void __iomem *addr, unsigned int value, void __iomem *stat_addr, unsigned int cond)
{
    unsigned int stat;
    int          timeout;

    __raw_writel(value, (volatile void __iomem *)addr);

    /* polling timeout */
    timeout = TIMEOUT;
    stat = __raw_readl((volatile void __iomem *)stat_addr);
    while (((stat & value) != cond) && timeout >= 0) {
        usleep_range(100, 110);
        stat = __raw_readl((volatile void __iomem *)stat_addr);
        --timeout;
    }

    return ((timeout < 0) ? (stat) : (0));
}

int en_stat_poll(void __iomem *addr, unsigned int value)
{
	return _stat_poll(addr, value, addr + EN_SATA_OFFSET, value);
}

int en_rststat_poll(void __iomem *addr, unsigned int value)
{
	return _stat_poll(addr, value, addr + EN_RSTSATA_OFFSET, value);
}

int dis_rststat_poll(void __iomem *addr, unsigned int value)
{
	return _stat_poll(addr, value, addr + DIS_RSTSATA_OFFSET, 0);
}

int noc_rststat_poll(void __iomem *addr, unsigned int value)
{
	return _stat_poll(addr, value, addr + EN_RSTSATA_OFFSET, 0);
}

static void dump_bootware(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    void __iomem *bw_base;
    int i = 0;

    bw_base = (void __iomem *)ioremap(dev->remap_addr, SZ_8K);
    for(i = 0; i < 1024; i += 16)
        pr_alert("0x%08x 0x%08x 0x%08x 0x%08x\n", __raw_readl(bw_base + i), __raw_readl(bw_base + i + 4), __raw_readl(bw_base + i + 8), __raw_readl(bw_base + i + 12));
    iounmap(bw_base);
}

static void dump_ispa7_regs(void)
{
#define DUMP_A7PC_TIMES (3)
    struct rproc_boot_device *dev = &rproc_boot_dev;
    void __iomem *crg_base = dev->reg[CRGPERI];
    void __iomem *cssys_base = dev->reg[CSSYS];
    void __iomem *isp_base = dev->reg[ISPCORE];
    unsigned int sec_rststat;
    int i = 0;

    if (crg_base == NULL || cssys_base == NULL || isp_base == NULL) {
        RPROC_ERR("Failed : ioremap.(crg.%pK, cssys.%pK, isp.%pK)\n", crg_base, cssys_base, isp_base);
        return;
    }

    if ((sec_rststat = __raw_readl(crg_base + CRG_C88_PERIPHISP_SEC_RSTSTAT)) != 0)
        pr_alert("sec_rststat.0x%x = 0x%x\n", CRG_C88_PERIPHISP_SEC_RSTSTAT, sec_rststat);

    if ((sec_rststat & IP_RST_ISPA7) == 0) {
        for (i = 0; i < DUMP_A7PC_TIMES; i ++)
            pr_alert("A7PC_OFFSET.0x%x, PMEVCNTR0_EL0.0x%x, PMCCNTR_EL0.0x%x\n",
                __raw_readl(cssys_base + A7PC_OFFSET), __raw_readl(cssys_base + PMEVCNTR0_EL0), __raw_readl(cssys_base + PMCCNTR_EL0));
    }

    if ((sec_rststat & IP_RST_ISP) == 0) {
        pr_alert("isp.dump.version.0x%x\n", __raw_readl(isp_base + REVISION_ID_OFFSET));
    }
}

static void dump_crg_regs(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    void __iomem *crg_base = dev->reg[CRGPERI];

    if (crg_base == NULL) {
        RPROC_ERR("Failed : ioremap crg_base\n");
        return;
    }

    pr_alert("DIV10.0x%x, DIV18.0x%x, DIV20.0x%x\n",
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_CLKDIV10_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_CLKDIV18_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_CLKDIV20_ADDR)));
    pr_alert("CLKEN0.0x%x, STAT0.0x%x, CLKEN3.0x%x, STAT3.0x%x, CLKEN5.0x%x, STAT5.0x%x\n",
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERCLKEN0_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERSTAT0_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERCLKEN3_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERSTAT3_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERCLKEN5_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERSTAT5_ADDR)));
    pr_alert("RSTSTAT0.0x%x, RSTSTAT3.0x%x, RSTSTAT.0x%x, ISOSTAT.0x%x, PWRSTAT.0x%x, PWRACK.0x%x\n",
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERRSTSTAT0_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERRSTSTAT3_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRG_C88_PERIPHISP_SEC_RSTSTAT)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_ISOSTAT_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERPWRSTAT_ADDR)),
        __raw_readl((volatile void __iomem*)(crg_base + CRGPERIPH_PERPWRACK_ADDR)));
}

static void dump_noc_regs(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    void __iomem *pmc_base = dev->reg[PMCTRL];

    if (pmc_base == NULL) {
        RPROC_ERR("Failed : ioremap pmc_base\n");
        return;
    }

    pr_alert("NOC_POWER: IDLEACK.0x%x, IDLE.0x%x\n",
        __raw_readl(pmc_base + PMCTRL_384_NOC_POWER_IDLEACK), __raw_readl(pmc_base + PMCTRL_388_NOC_POWER_IDLE));
}

static int dump_a7boot_stone(void)
{
    struct rproc_shared_para *param = NULL;
    int entry = 0;

    hisp_lock_sharedbuf();
    if ((param = rproc_get_share_para()) == NULL) {
        RPROC_ERR("Failed : rproc_get_share_para.%pK\n", param);
        hisp_unlock_sharedbuf();
        return -EINVAL;
    }

    RPROC_ERR("BOOTWARE::entry.%d, invalid_tlb.%d, enable_mmu.%d\n",
        param->bw_stat.entry, param->bw_stat.invalid_tlb, param->bw_stat.enable_mmu);
    RPROC_ERR("FIRMWARE::entry.%d, hard_boot_init.%d, hard_drv_init.%d, app_init.%d\n",
        param->fw_stat.entry, param->fw_stat.hard_boot_init,
        param->fw_stat.hard_drv_init, param->fw_stat.app_init);

    entry = param->bw_stat.entry;
    hisp_unlock_sharedbuf();

    return entry;
}

void dump_media1_regs(void)
{
#ifdef CONFIG_HUAWEI_CAMERA_USE_HISP200
    void __iomem *media1_base = NULL;
    void __iomem *pctrl_addr = NULL;
    int ret = 0;

    pr_info("[%s] +\n", __func__);
    if ((ret = get_media1_subsys_power_state()) == 0) {
        pr_err("[%s] Failed : get_media1_subsys_power_state. %d\n", __func__, ret);
        return ;
    }

    media1_base = get_regaddr_by_pa(MEDIA1);

    if (media1_base == NULL) {
        RPROC_ERR("Failed : ioremap media1_base\n");
        return;
    }

    pr_alert("MEDIA1_POWER:PERCLKEN0.0x%x, PERSTAT0.0x%x, PERCLKEN1.0x%x, PERSTAT1.0x%x, CLKDIV9.0x%x\n",
        __raw_readl(media1_base + MEDIA1_PERCLKEN0_ADDR),
        __raw_readl(media1_base + MEDIA1_PERSTAT0_ADDR),
        __raw_readl(media1_base + MEDIA1_PERCLKEN1_ADDR),
        __raw_readl(media1_base + MEDIA1_PERSTAT1_ADDR),
        __raw_readl(media1_base + MEDIA1_CLKDIV9_ADDR));

    pctrl_addr = get_regaddr_by_pa(PCTRL);
    if (!pctrl_addr) {
        RPROC_ERR("Failed : pctrl ioremap\n");
        return;
    }

    pr_alert("PCTRL:PERI_STAT2.0x%x\n",
        __raw_readl(pctrl_addr + PCTRL_PERI_STAT2_ADDR));
#endif
}

void dump_smmu500_regs(void)
{
#ifdef CONFIG_HUAWEI_CAMERA_USE_HISP200
    void __iomem *smmu500_addr = NULL;
    int ret = 0;

    pr_info("[%s] +\n", __func__);
    if ((ret = get_media1_subsys_power_state()) == 0) {
        pr_err("[%s] Failed : get_media1_subsys_power_state. %d\n", __func__, ret);
        return ;
    }

    smmu500_addr = get_regaddr_by_pa(ISPCORE);
    if (!smmu500_addr) {
        pr_alert("Failed : smmu500 ioremap\n");
        return;
    }

    smmu500_addr += 0x001F0000;

    pr_alert("SMMU500:SMMU_SIDR0.0x%x\n", __raw_readl(smmu500_addr + 0x0020));
    pr_alert("SMMU500:SMMU_SIDR1.0x%x\n", __raw_readl(smmu500_addr + 0x0024));
    pr_alert("SMMU500:SMMU_SIDR2.0x%x\n", __raw_readl(smmu500_addr + 0x0028));
    pr_alert("SMMU500:SMMU_SIDR7.0x%x\n", __raw_readl(smmu500_addr + 0x003C));
    pr_alert("SMMU500:SMMU_SGFSYNR0.0x%x\n", __raw_readl(smmu500_addr + 0x0050));
    pr_alert("SMMU500:SMMU_SGFSYNR1.0x%x\n", __raw_readl(smmu500_addr + 0x0054));
    pr_alert("SMMU500:SMMU_STLBGSTATUS.0x%x\n", __raw_readl(smmu500_addr + 0x0074));
    pr_alert("SMMU500:SMMU_DBGRDATATBU.0x%x\n", __raw_readl(smmu500_addr + 0x0084));
    pr_alert("SMMU500:SMMU_DBGRDATATCU.0x%x\n", __raw_readl(smmu500_addr + 0x008C));
    pr_alert("SMMU500:PMCGCR0.0x%x\n", __raw_readl(smmu500_addr + 0x3800));
    pr_alert("SMMU500:PMCFGR.0x%x\n", __raw_readl(smmu500_addr + 0x3E00));
    pr_alert("SMMU500:PMAUTHSTATUS.0x%x\n", __raw_readl(smmu500_addr + 0x3FB8));
    pr_alert("SMMU500:SMMU_CB0_FAR_LOW.0x%x\n", __raw_readl(smmu500_addr + 0x8060));
    pr_alert("SMMU500:SMMU_CB0_FAR_HIGH.0x%x\n", __raw_readl(smmu500_addr + 0x8064));
    pr_alert("SMMU500:SMMU_CB0_IPAFAR_LOW.0x%x\n", __raw_readl(smmu500_addr + 0x8070));
    pr_alert("SMMU500:SMMU_CB0_IPAFAR_HIGH.0x%x\n", __raw_readl(smmu500_addr + 0x8074));
    pr_alert("SMMU500:SMMU_CB0_PMAUTHSTATUS.0x%x\n", __raw_readl(smmu500_addr + 0x8FB8));
    pr_alert("SMMU500:SMMU_CB1_FAR_LOW.0x%x\n", __raw_readl(smmu500_addr + 0x9060));
    pr_alert("SMMU500:SMMU_CB1_FAR_HIGH.0x%x\n", __raw_readl(smmu500_addr + 0x9064));
    pr_alert("SMMU500:SMMU_CB1_IPAFAR_LOW.0x%x\n", __raw_readl(smmu500_addr + 0x9070));
    pr_alert("SMMU500:SMMU_CB1_IPAFAR_HIGH.0x%x\n", __raw_readl(smmu500_addr + 0x9074));
    pr_alert("SMMU500:SMMU_CB1_TLBSTATUS.0x%x\n", __raw_readl(smmu500_addr + 0x97F4));
    pr_alert("SMMU500:SMMU_CB1_ATSR.0x%x\n", __raw_readl(smmu500_addr + 0x98F0));
    pr_alert("SMMU500:SMMU_CB1_PMAUTHSTATUS.0x%x\n", __raw_readl(smmu500_addr + 0x9FB8));
#endif
}

void hisi_isp_boot_stat_dump(void)
{
    pr_alert("[%s] +\n", __func__);
	dump_a7boot_stone();
    if (use_sec_isp()) {
        hisi_secisp_dump();
        return;
    }

    if (use_nonsec_isp())
        return;

    dump_ispa7_regs();
    dump_crg_regs();
    dump_noc_regs();
    dump_bootware();

    pr_alert("[%s] -\n", __func__);
}

static int cpu_debug_init(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    void __iomem *cssys_base = dev->reg[CSSYS];

    if (cssys_base == NULL) {
        RPROC_ERR("Failed : ioremap cssys_base\n");
        return -1;
    }
    __raw_writel((unsigned int)0xC5ACCE55, (volatile void __iomem *)(cssys_base + DBGLAR));
    __raw_writel((unsigned int)0x00000000, (volatile void __iomem *)(cssys_base + DBGOSLAR));
    __raw_writel((unsigned int)0xC5ACCE55, (volatile void __iomem *)(cssys_base + PMLAR));
    __raw_writel((unsigned int)0x00000007, (volatile void __iomem *)(cssys_base + PMCR));
    __raw_writel((unsigned int)0x00000008, (volatile void __iomem *)(cssys_base + PMXEVTYPER0));
    __raw_writel((unsigned int)0x80000001, (volatile void __iomem *)(cssys_base + PMCNTENSET));

    return 0;
}

static int dis_reset_a7(struct rproc_boot_device *dev)
{
    volatile unsigned int value, ret = 0;
    void __iomem * addr;

    /*
     * CRGPERI_A7_SEC_RSTDIS: ip_rst_isp = 1, ip_arst_isp = 1,
     * ip_hrst_isp = 1, ip_rst_ispa7cfg = 1, ip_rst_ispa7 = 1
     */
    value = 0xFF;
    addr = CRG_C84_PERIPHISP_SEC_RSTDIS + dev->reg[CRGPERI];
    RPROC_INFO("CRGPERI_A7_SEC_RSTDIS : %pK = 0x%x\n", addr, value);
    if ((ret = dis_rststat_poll(addr, value)) != 0)
        RPROC_ERR("Failed : ISP_SEC_RSTDIS : %pK = 0x%x, ret.0x%x\n", addr, value, ret);

    if (cpu_debug_init() < 0)
        RPROC_ERR("Failed: cpu_debug_init");

    if (!dump_a7boot_stone())
        hisi_isp_boot_stat_dump();

    return ret;
}

static int power_up_isp_subsys(struct rproc_boot_device *dev)
{
    int ret;

    RPROC_INFO("Start Dis Reset ISP A7 clk.%d\n", dev->ispa7_clk_value);
    ret = clk_set_rate(dev->ispa7_clk, dev->ispa7_clk_value);
    if(ret < 0) {
        RPROC_ERR("Failed: clk_set_rate.%d\n", ret);
        return ret;
    }

    ret = clk_prepare_enable(dev->ispa7_clk);
    if(ret < 0) {
        RPROC_ERR("Failed: clk_prepare_enable.%d\n", ret);
        return ret;
    }

    if (IS_HI6250(dev->boardid)) {
        ret = clk_prepare(dev->isp_timer);
        if (ret) {
            clk_disable_unprepare(dev->ispa7_clk);
            RPROC_ERR("Failed : isp_timer clk_prepare boardid.0x%x, ret.%d\n", dev->boardid, ret);
            return ret;
        }

        ret = clk_enable(dev->isp_timer);
        if (ret) {
            clk_unprepare(dev->isp_timer);
            clk_disable_unprepare(dev->ispa7_clk);
            RPROC_ERR("Failed : isp_timer clk_enable boardid.0x%x, ret.%d\n", dev->boardid, ret);
            return ret;
        }
    }

    ret = regulator_enable(dev->isp_subsys_ip);
    if (0 != ret) {
        RPROC_ERR("Failed: regulator_enable.%d \n", ret);
        goto err;
    }

    dev->isp_subsys_power_flag = 1;
    RPROC_INFO("X...\n");
    return 0;

err:
    if (IS_HI6250(dev->boardid)) {
        clk_disable(dev->isp_timer);
        clk_unprepare(dev->isp_timer);
    }
    clk_disable_unprepare(dev->ispa7_clk);

    dev->isp_subsys_power_flag = 0;
    return ret;
}

static void remap_a7_entry(struct rproc_boot_device *dev)
{
    u64 addr = dev->remap_addr;

    if(dev->reg[CRGPERI] == NULL){
        pr_err("%s: CRGPERI is NULL\n", __func__);
        return;
    }

    __raw_writel((unsigned int)(addr >> 4) | ISPA7_VINITHI_HIGH | ISPA7_REMAP_ENABLE | ISPA7_DBGPWRDUP
            , (volatile void __iomem *)(CRG_C90_PERIPHISP_ISPA7_CTRL_BASE + dev->reg[CRGPERI]));
}

static void power_down_isp_subsys(struct rproc_boot_device *dev)
{
    int ret = 0;

    RPROC_INFO("Enter \n");
    ret = regulator_disable(dev->isp_subsys_ip);
    if (0 != ret) {
        RPROC_ERR("Failed: regulator_disable.%d\n", ret);
    }

    if (IS_HI6250(dev->boardid)) {
        clk_disable(dev->isp_timer);
        clk_unprepare(dev->isp_timer);
    }
    clk_disable_unprepare(dev->ispa7_clk);

    dev->isp_subsys_power_flag = 0;
}

static int default_isp_device_disable(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    RPROC_INFO("+\n");
    if (dev->isp_subsys_power_flag) {
        RPROC_INFO("isp_subsys_power_flag = %d \n", dev->isp_subsys_power_flag);
        power_down_isp_subsys(dev);
    }

    RPROC_INFO("-\n");

    return 0;
}

static int isp_device_disable(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = 0;

    RPROC_INFO("+\n");
    switch (dev->case_type) {
        case SEC_CASE:
            if ((ret = get_ispcpu_cfg_info()) < 0)
                RPROC_ERR("Failed : get_ispcpu_cfg_info.%d\n", ret);
            wait_firmware_coredump();
            if(get_ispcpu_idle_stat(dev->isppd_adb_flag) < 0) {
                if(dev->isppd_adb_flag){
                    dump_hisi_isp_boot(dev->isp_rproc->rproc,DUMP_ISP_BOOT_SIZE);
                    }
                }
            if ((ret = secisp_device_disable()) < 0)
                RPROC_ERR("Failed : secisp_device_disable.%d\n", ret);
            break;
        case NONSEC_CASE:
            if ((ret = get_ispcpu_cfg_info()) < 0)
                RPROC_ERR("Failed : get_ispcpu_cfg_info.%d\n", ret);
            wait_firmware_coredump();
            if(get_ispcpu_idle_stat(dev->isppd_adb_flag) < 0) {
                if(dev->isppd_adb_flag){
                    dump_hisi_isp_boot(dev->isp_rproc->rproc,DUMP_ISP_BOOT_SIZE);
                    }
                }
            if ((ret = nonsec_isp_device_disable()) < 0)
                RPROC_ERR("Failed : nonsec_isp_device_disable.%d\n", ret);
            break;
        default:
            if ((ret = default_isp_device_disable()) < 0)
                RPROC_ERR("Failed : default_isp_device_disable.%d\n", ret);
            break;
    }
    dev->ispcpu_status = 0;
    stop_isplogcat();

    if (ret != 0)
        RPROC_ERR("Failed : ispcpu power down fail.%d, dev->case_type.%d\n", ret, dev->case_type);
    mutex_lock(&dev->ispcpu_mutex);
    if (wake_lock_active(&dev->ispcpu_wakelock)) {
        wake_unlock(&dev->ispcpu_wakelock);
        RPROC_INFO("ispcpu power up wake unlock.\n");
    }
    mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
    RPROC_INFO("-\n");

    return ret;
}
static int default_isp_device_enable(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = -ENOMEM;

    RPROC_INFO("+\n");

    remap_a7_entry(dev);
    ret = power_up_isp_subsys(dev);
    if (0 != ret) {
        RPROC_ERR("power_up_isp_subsys failed.%d\n", ret);
        return ret;
    }

    if ( (ret = ispcpu_qos_cfg()) < 0) {
        RPROC_ERR("ispcpu_qos_cfg failed.%d\n", ret);
        goto err1;
    }

    ret = dis_reset_a7(dev);
    if (0 != ret) {
        RPROC_ERR("disreset_a7 failed. \n");
        goto err1;
    }
    RPROC_INFO("-\n");

    return 0;

err1:
    power_down_isp_subsys(dev);

    return (ret < 0 ? ret : -1);
}

/* enable rproc a7 and isp core*/
/*lint -save -e838  -e454*/
static int isp_device_enable(void)
{
#define TIMEOUT_ISPLOG_START (10)
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = -ENOMEM, timeout = 0;

    RPROC_INFO("+\n");
    last_boot_state = 0;
    mutex_lock(&dev->ispcpu_mutex);
    if (!wake_lock_active(&dev->ispcpu_wakelock)) {
        wake_lock(&dev->ispcpu_wakelock);
        RPROC_INFO("ispcpu power up wake lock.\n");
    }
    mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
    timeout = TIMEOUT_ISPLOG_START;
    do {
        if ((ret = start_isplogcat()) < 0)
            RPROC_ERR("Failed : secisp_device_enable.%d, timeout.%d\n", ret, timeout);
    } while (ret < 0 && timeout-- > 0);

    switch (dev->case_type) {
        case SEC_CASE:
            if ((ret = secisp_device_enable()) < 0)
                RPROC_ERR("Failed : secisp_device_enable.%d\n", ret);
            break;
        case NONSEC_CASE:
            if ((ret = nonsec_isp_device_enable()) < 0)
                RPROC_ERR("Failed : nonsec_isp_device_enable.%d\n", ret);
            if ( hisp_mntn_dumpregs() < 0)
                RPROC_ERR("Failed : get_ispcpu_cfg_info");
            break;
        default:
            if ((ret = default_isp_device_enable()) < 0)
                RPROC_ERR("Failed : default_isp_device_enable.%d\n", ret);
            break;
    }
    dev->ispcpu_status = 1;
    RPROC_INFO("-\n");

    if (ret != 0){
        RPROC_ERR("Failed : ispcpu power up fail.%d, dev->case_type.%d\n", ret, dev->case_type);
        mutex_lock(&dev->ispcpu_mutex);
        if (wake_lock_active(&dev->ispcpu_wakelock)) {
            wake_unlock(&dev->ispcpu_wakelock);
            RPROC_INFO("ispcpu power up wake unlock.\n");
        }
        mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
    }
    return ret;
}
/*lint -restore */
static void isp_mbox_rx_work(void)
{
    struct rx_buf_data mbox_reg;
    int ret;

    while (kfifo_len(&isp_rx_mbox->rpmsg_rx_fifo) >= sizeof(struct rx_buf_data)) {/*lint !e84 */
        if ((ret = kfifo_out_locked(&isp_rx_mbox->rpmsg_rx_fifo, (unsigned char *)&mbox_reg, \
                                    sizeof(struct rx_buf_data), &isp_rx_mbox->rpmsg_rx_lock)) < 0) {/*lint !e84 */
            RPROC_ERR("Failed : kfifo_out_locked.%d\n", ret);
            return ;
        }

        /* maybe here need the flag of is_have_data */
        if (rproc_vq_interrupt(hisi_rproc, mbox_reg.rpmsg_rx_buf[0]) == IRQ_NONE)
            RPROC_DEBUG("no message was found in vqid\n");
    }
}

static int isp_mbox_rx_thread(void *context)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int ret = 0;

    RPROC_INFO("+\n");
    while (!kthread_should_stop()) {
		ret = wait_event_interruptible(isp_rx_mbox->wait, isp_rx_mbox->can_be_wakeup == 1);
		isp_rx_mbox->can_be_wakeup = 0;
		if (ret) {
			RPROC_ERR("isp_mbox_rx_thread wait event failed\n");
		    continue;
		}
        mutex_lock(&rproc_dev->hisi_isp_power_mutex);
        if(rproc_dev->hisi_isp_power_state == HISP_ISP_POWER_OFF){
            pr_err("[%s] hisi_isp_rproc disable no power.0x%x\n", __func__, rproc_dev->hisi_isp_power_state);
            mutex_unlock(&rproc_dev->hisi_isp_power_mutex);
            return -ENODEV;
        }
        hisp_recvthread();
        isp_mbox_rx_work();
        mutex_unlock(&rproc_dev->hisi_isp_power_mutex);
    }
    RPROC_INFO("-\n");

    return 0 ;
}

int hisi_rproc_mbox_callback(struct notifier_block *this, unsigned long len, void *data)
{
    mbox_msg_t *msg = (mbox_msg_t *)data;
    struct rx_buf_data mbox_reg;
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret;
    unsigned int i;
    if(dev ->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }

    spin_lock_bh(&dev->rpmsg_ready_spin_mutex);
    if(!dev->rpmsg_ready_state){
        RPROC_INFO("isp is powered off state\n");
        spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
        return NOTIFY_DONE;
    }

    hisp_recvtask();
	switch (msg[0]) {
		case RP_MBOX_CRASH:/*lint !e650 */
			/*  just log this for now. later, we'll also do recovery */
			RPROC_INFO("hisi rproc crashed\n");

			break;
		case RP_MBOX_ECHO_REPLY:/*lint !e650 */
			RPROC_INFO("received echo reply \n");
			break;
		default:
			/*  msg contains the index of the triggered vring */
			RPROC_DEBUG("default.%d\n", msg[0]);

            mbox_reg.rpmsg_rx_len = MBOX_REG_COUNT;
            for (i = 0; i < mbox_reg.rpmsg_rx_len; i++) {
                mbox_reg.rpmsg_rx_buf[i] = msg[i];
            }

            if (kfifo_avail(&isp_rx_mbox->rpmsg_rx_fifo) < sizeof(struct rx_buf_data)) {/*lint !e84 */
                RPROC_ERR("rpmsg_rx_fifo is full \n");
                spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
                return -1;
            }

            ret = kfifo_in_locked(&isp_rx_mbox->rpmsg_rx_fifo, (unsigned char *)&mbox_reg, \
                                  sizeof(struct rx_buf_data), &isp_rx_mbox->rpmsg_rx_lock);/*lint !e84 */
            if (ret <= 0) {
                RPROC_ERR("kfifo_in_locked failed \n");
                spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
                return ret;
            }
            RPROC_DEBUG("kfifo_in_locked success !\n");

			isp_rx_mbox->can_be_wakeup = 1;
			wake_up_interruptible(&isp_rx_mbox->wait);
    }
    spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
    RPROC_DEBUG("----hisi_rproc_mbox_callback rx msg X----\n");

    return NOTIFY_DONE;
}
EXPORT_SYMBOL(hisi_rproc_mbox_callback);

static int init_hisi_ipc_resource(void)
{
    int ret;

    RPROC_INFO("+\n");
    isp_rx_mbox = kzalloc(sizeof(struct isp_rx_mbox), GFP_KERNEL);
    if (!isp_rx_mbox) {
        RPROC_ERR("Failed : kzalloc isp_rx_mbox\n");
        return -ENOMEM;
    }

    init_waitqueue_head(&isp_rx_mbox->wait);
    isp_rx_mbox->can_be_wakeup = 0;
    isp_rx_mbox->rpmsg_rx_tsk = kthread_create(isp_mbox_rx_thread, NULL, "rpmsg_tx_tsk");

	if (unlikely(IS_ERR(isp_rx_mbox->rpmsg_rx_tsk))) {
		RPROC_ERR("Failed : create kthread tx_kthread\n");
		ret = -EINVAL;
		goto kthread_failure;
	} else {
		struct sched_param param;
		/*set the thread's priority to 75, the bigger sched_priority, the higher priority*/
		param.sched_priority = (MAX_RT_PRIO - 25);
		ret = sched_setscheduler(isp_rx_mbox->rpmsg_rx_tsk, SCHED_RR, &param);
		if (ret < 0) {
		    RPROC_ERR("Failed : sched_setscheduler\n");
		    goto kthread_failure;
		}
		wake_up_process(isp_rx_mbox->rpmsg_rx_tsk);
	}

    spin_lock_init(&isp_rx_mbox->rpmsg_rx_lock);

    if (kfifo_alloc(&isp_rx_mbox->rpmsg_rx_fifo, sizeof(struct rx_buf_data) * RPMSG_RX_FIFO_DEEP, GFP_KERNEL)) {
        RPROC_ERR("Failed : kfifo_alloc\n");
        ret = -ENOMEM;
        goto kfifo_failure;
    }

    RPROC_INFO("-\n");
    return 0;
kfifo_failure:
kthread_failure:
    kfree(isp_rx_mbox);

    return ret;
}

static int hisi_rproc_start(struct rproc *rproc)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    spin_lock_bh(&rproc_dev->rpmsg_ready_spin_mutex);
    rproc_dev->rpmsg_ready_state = 1;
    spin_unlock_bh(&rproc_dev->rpmsg_ready_spin_mutex);/*lint !e456 */
    return 0;
}

static int hisi_rproc_stop(struct rproc *rproc)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

    RPROC_INFO("+\n");
    spin_lock_bh(&rproc_dev->rpmsg_ready_spin_mutex);
    rproc_dev->rpmsg_ready_state = 0;
    spin_unlock_bh(&rproc_dev->rpmsg_ready_spin_mutex);/*lint !e456 */
    RPROC_FLUSH_TX(rproc_dev->ap_a7_mbox);/*lint !e64 */

    isp_device_disable();
    RPROC_INFO("-\n");
	return 0;
}

static void hisi_rproc_kick(struct rproc *rproc, int vqid)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int ret;
    if(rproc_dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return;
    }

	/* Send the index of the triggered virtqueue in the mailbox payload */
	communicat_msg[0] = vqid;

    RPROC_DEBUG("+.0x%x \n", communicat_msg[0]);
	ret = RPROC_ASYNC_SEND(rproc_dev->ap_a7_mbox, communicat_msg, sizeof(communicat_msg[0]));/*lint !e64 */
	if(ret)
        RPROC_ERR("Failed: RPROC_ASYNC_SEND.%d\n", ret);
    hisp_sendx();
	communicat_msg[0] = 0;
    RPROC_DEBUG("-\n");
}

int get_rproc_enable_status(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    return(atomic_read(&rproc_dev->rproc_enable_status));
}

bool rproc_get_sync_flag(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc *rproc = rproc_dev->isp_rproc->rproc;

    return rproc->sync_flag;
}

void rproc_set_sync_flag(bool flag)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc *rproc = rproc_dev->isp_rproc->rproc;

    rproc->sync_flag = flag;
}

int set_isp_remap_addr(u64 remap_addr)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    struct rproc_shared_para *param = NULL;

    if(!remap_addr){
        RPROC_ERR("Failed : remap_addr.0\n");
        return -ENOMEM;
    }
    dev->remap_addr = remap_addr;
    hisp_lock_sharedbuf();
    param = rproc_get_share_para();
    if(param == NULL){
        RPROC_ERR("Failed : rproc_get_share_para\n");
        hisp_unlock_sharedbuf();
        return -ENOMEM;
    }
    param->bootware_paddr           = remap_addr;
    hisp_unlock_sharedbuf();

    return 0;
}

#ifdef DEBUG_HISI_ISP
static int ispupdate_elf_load(char **buffer, int *length)
{
#define PART_ISPFW_SIZE 0x00E00000
    int ret          = -1;
    char *pathname   = "/system/vendor/firmware/isp_fw.elf";
    struct file *fp;
    mm_segment_t fs;
    loff_t pos = 0;
    struct kstat m_stat;

    if ((NULL == buffer)) {
        RPROC_ERR("buffer(%pK) is null", buffer);
        return -1;
    }

    /*get resource*/
    fp = filp_open(pathname, O_RDONLY, 0600);
    if (IS_ERR(fp)) {
        RPROC_ERR("filp_open(%s) failed", pathname);
        return -ENOENT;
    }

    *buffer = vmalloc(PART_ISPFW_SIZE);
    if (*buffer == NULL) {
        RPROC_ERR("Failed : vmalloc.%pK\n", *buffer);
        goto error2;
    }

    ret = vfs_llseek(fp, 0, SEEK_SET);
    if (ret < 0) {
        RPROC_ERR("seek ops failed, ret %d", ret);
        goto error2;
    }

    fs = get_fs();/*lint !e501*/
    set_fs(KERNEL_DS);/*lint !e501 */

    if ((ret = vfs_stat(pathname, &m_stat)) < 0) {
        RPROC_ERR("Failed :%s vfs_stat: %d\n", pathname, ret);
        set_fs(fs);
        goto error2;
    }
    *length = m_stat.size;

    pos = fp->f_pos;/*lint !e613 */
    ret = vfs_read(fp, (char __user *)*buffer, *length, &pos);/*lint !e613 */
    if (ret != *length) {
        RPROC_ERR("read ops failed, ret=%d(len=%d)", ret, *length);
        set_fs(fs);
        goto error2;
    }
    set_fs(fs);

    filp_close(fp, NULL);/*lint !e668 */

    return 0;

error2:
    filp_close(fp, NULL);/*lint !e668 */
    RPROC_ERR("failed");
    return -1;
}

static int hisp_elf_load_segments(const u8 *data, void* dst, int fw_size)
{
    struct elf32_hdr *ehdr;
    struct elf32_phdr *phdr;
    int i, ret = 0;
    const u8 *elf_data = data;
    u32 start_addr = 0;

    ehdr = (struct elf32_hdr *)elf_data;
    phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);

    start_addr = phdr->p_paddr;
    pr_debug("[%s] start_addr.0x%x\n", __func__, start_addr);

    /* go through the available ELF segments */
    for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
        u32 da = phdr->p_paddr;
        u32 memsz = phdr->p_memsz;
        u32 filesz = phdr->p_filesz;
        u32 offset = phdr->p_offset;
        void *ptr;

        if (phdr->p_type != PT_LOAD)
            continue;

        pr_debug("phdr: type %d da 0x%x memsz 0x%x filesz 0x%x\n",
            phdr->p_type, da, memsz, filesz);

        if (filesz > memsz) {
            RPROC_ERR("bad phdr filesz 0x%x memsz 0x%x\n", filesz, memsz);
            ret = -EINVAL;
            break;
        }
        if (offset + filesz > (u32)fw_size) {
            RPROC_ERR("truncated fw: need 0x%x avail 0x%x\n", offset + filesz, fw_size);
            ret = -EINVAL;
            break;
        }
        /* grab the kernel address for this device address */
        ptr = dst + da - start_addr;
        if (!ptr) {
            RPROC_ERR("bad phdr da 0x%x mem 0x%x\n", da, memsz);
            ret = -EINVAL;
            break;
        }

        /* put the segment where the remote processor expects it */
        if (phdr->p_filesz)
            memcpy(ptr, elf_data + phdr->p_offset, filesz);

        /*
        * Zero out remaining memory for this segment.
        *
        * This isn't strictly required since dma_alloc_coherent already
        * did this for us. albeit harmless, we may consider removing
        * this.
        */
        if (memsz > filesz)
            memset(ptr + filesz, 0, memsz - filesz);
    }

    return ret;
}

static int isp_elf_load(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int ret = -1;
    void* isp_elf_vaddr = NULL;
    int length = 0;

    RPROC_INFO("+\n");
    if(rproc_dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }

    if (rproc_dev->isp_bin_vaddr == NULL) {
        RPROC_ERR("Failed : isp_bin_vaddr.NULL\n");
        return -ENOMEM;
    }

    if ((ret = ispupdate_elf_load((char **)&isp_elf_vaddr, &length)) < 0) {
        if (isp_elf_vaddr)
            vfree(isp_elf_vaddr);
        RPROC_ERR("Failed : ispupdate_elf_load\n");
        return ret;
    }

    if ((ret = hisp_elf_load_segments(isp_elf_vaddr, rproc_dev->isp_bin_vaddr, length)) < 0) {
        RPROC_ERR("Failed : hisp_elf_load_segments isp_elf_vaddr.0x%pK, isp_bin_vaddr.0x%pK, length.0x%x\n",
            isp_elf_vaddr, rproc_dev->isp_bin_vaddr, length);
    }
    vfree(isp_elf_vaddr);

    RPROC_INFO("-\n");
    return ret;
}
#endif

static int isp_bin_load(void *data)
{
#define PART_ISPFW_SIZE 0x00E00000
    int ret = -1;
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    unsigned int fw_size = (ISP_FW_SIZE>PART_ISPFW_SIZE)?ISP_FW_SIZE:PART_ISPFW_SIZE;

    RPROC_INFO("+\n");
    rproc_dev->isp_bin_vaddr = vmalloc(fw_size);
    if (rproc_dev->isp_bin_vaddr == NULL) {
        RPROC_ERR("Failed : vmalloc.%pK\n", rproc_dev->isp_bin_vaddr);
        return -ENOMEM;
    }

    if ((ret = hisp_bsp_read_bin("isp_firmware", 0, PART_ISPFW_SIZE, rproc_dev->isp_bin_vaddr)) < 0) {
        vfree(rproc_dev->isp_bin_vaddr);
        rproc_dev->isp_bin_vaddr = NULL;
        RPROC_ERR("Failed : hisp_bsp_read_bin.%d\n", ret);
        return ret;
    }
    rproc_dev->isp_bin_state = 1;
    RPROC_INFO("-\n");

    return 0;
}

int wakeup_ispbin_kthread(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int timeout = 1000;

    RPROC_INFO("+\n");

    if (rproc_dev->isp_bin_state == 1) {
        return 0;
    }

    if (rproc_dev->loadispbin == NULL) {
        RPROC_ERR("Failed : loadispbin is NULL\n");
        return -1;
    }

    wake_up_process(rproc_dev->loadispbin);
    do {
        timeout--;
        mdelay(10);
        if (rproc_dev->isp_bin_state == 1) {
            break;
        }
    } while (timeout > 0);

    RPROC_INFO("isp_bin_state.%d, timeout.%d\n", rproc_dev->isp_bin_state, timeout);

    return (timeout>0)? 0 : -1;
}

void *hisp_get_rsctable(int *tablesz)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

    if (use_sec_isp()) {
        return get_rsctable(tablesz);
    }

    *tablesz = 0x00001000;
    return rproc_dev->rsctable_vaddr;
}

int hisp_bin_load_segments(struct rproc *rproc)
{
#define MEM_BOOTWARE_DA                0xFFFF0000
#define MEM_BOOTWARE_SIZE              0xF000

	struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
	int ret = 0;
	void *ptr;
	u32 da = TEXT_BASE;
	u32 memsz = ISP_TEXT_SIZE;

    RPROC_INFO("+\n");

	/* go through the available ELF segments */
	/* grab the kernel address for this device address */
	ptr = rproc_da_to_va(rproc, da, memsz);
    RPROC_INFO("text section ptr = %pK\n", ptr);
	if (!ptr) {
		RPROC_ERR("bad phdr da 0x%x mem 0x%x\n", da, memsz);
		ret = -EINVAL;
		return ret;
	}
	/* put the segment where the remote processor expects it */
	memcpy(ptr, rproc_dev->isp_bin_vaddr, memsz);

	/* go through the available ELF segments */
	da = DATA_BASE;
	memsz = ISP_BIN_DATA_SIZE;
	ptr = rproc_da_to_va(rproc, da, memsz);
    RPROC_INFO("data section ptr = %pK\n", ptr);
	if (!ptr) {
		RPROC_ERR("bad phdr da 0x%x mem 0x%x\n", da, memsz);
		ret = -EINVAL;
		return ret;
	}
	/* put the segment where the remote processor expects it */
	memcpy(ptr, rproc_dev->isp_bin_vaddr +ISP_TEXT_SIZE, memsz);

	/* go through the available ELF segments */
	da = MEM_BOOTWARE_DA;
	memsz = MEM_BOOTWARE_SIZE;
	ptr = rproc_da_to_va(rproc, da, memsz);
	if (!ptr) {
		RPROC_ERR("bad phdr da 0x%x mem 0x%x\n", da, memsz);
		ret = -EINVAL;
		return ret;
	}
    RPROC_INFO("bootware ptr = %pK\n", ptr);
	/* put the segment where the remote processor expects it */
	memcpy(ptr, rproc_dev->isp_bin_vaddr, memsz);

	return 0;
}

int hisi_isp_rproc_disable(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc_vdev *rvdev, *rvtmp;
    struct rproc *rproc;
    int err = 0;

    RPROC_INFO("+\n");
    if(rproc_dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }
    if ((err = bypass_power_updn()) != 0 ) {/*lint !e838 */
        pr_err("[%s] bypass_power_updn.0x%x\n", __func__, err);
        return -ENODEV;
    }

    if (!rproc_dev->isp_rproc) {
        RPROC_INFO("Failed : isp_rproc.%pK\n", rproc_dev->isp_rproc);
        return -ENOMEM;
    }

    if (atomic_read(&rproc_dev->rproc_enable_status) == 0 ) {
        pr_err("[%s] hisi_isp_rproc disable err! rproc_enable_status.0x%x\n", __func__, atomic_read(&rproc_dev->rproc_enable_status));
        return -ENODEV;
    }
    mutex_lock(&rproc_dev->hisi_isp_power_mutex);
    if(rproc_dev->hisi_isp_power_state == HISP_ISP_POWER_OFF){
        pr_err("[%s] hisi_isp_rproc disable no power.0x%x\n", __func__, rproc_dev->hisi_isp_power_state);
        mutex_unlock(&rproc_dev->hisi_isp_power_mutex);
        return -ENODEV;
    }
    mutex_unlock(&rproc_dev->hisi_isp_power_mutex);

#ifdef DEBUG_HISI_ISP
    ispperf_stop_record();
#endif
    rproc = rproc_dev->isp_rproc->rproc;

    init_completion(&rproc->crash_comp);

    /* clean up remote vdev entries */
    list_for_each_entry_safe(rvdev, rvtmp, &rproc->rvdevs, node)
    {
        if(rvdev == NULL)
        {
            pr_err("[%s] list_for_each_entry_safe return rvdev null\n", __func__);
            err = -ENODEV;
            goto disable_out;
        }
        mutex_lock(&rproc_dev->hisi_isp_power_mutex);
        rproc_dev->hisi_isp_power_state = HISP_ISP_POWER_OFF;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
        rproc_remove_virtio_dev(rvdev);
#endif
        mutex_unlock(&rproc_dev->hisi_isp_power_mutex);
    }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
    rproc_shutdown(rproc);
#else
    /* Free the copy of the resource table */
    kfree(rproc->cached_table);
#endif
    rproc_set_shared_clk_init();
    rproc->domain = NULL;

    rproc_set_sync_flag(true);
disable_out:
    if (atomic_read(&rproc_dev->rproc_enable_status) == 1 )
        atomic_set(&rproc_dev->rproc_enable_status, 0);
    RPROC_INFO("-\n");
    return err;
}
EXPORT_SYMBOL(hisi_isp_rproc_disable);

int hisi_isp_rproc_enable(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc *rproc;
    int err = 0;

    RPROC_INFO("+\n");
    if(rproc_dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }
    if ((err = bypass_power_updn()) != 0 ) {/*lint !e838 */
        pr_err("[%s] bypass_power_updn.0x%x\n", __func__, err);
        return -ENODEV;
    }

	pr_info("%s.%d: case type.%u\n", __func__, __LINE__,
			hisi_isp_rproc_case_get());

    if (!rproc_dev->isp_rproc) {
        RPROC_INFO("Failed : isp_rproc.%pK\n", rproc_dev->isp_rproc);
        return -ENOMEM;
    }
    if (atomic_read(&rproc_dev->rproc_enable_status) > 0 ) {
        pr_err("[%s] hisi_isp_rproc had been enabled, rproc_enable_status.0x%x\n", __func__, atomic_read(&rproc_dev->rproc_enable_status));
        return -ENODEV;
    }
    else
        atomic_set(&rproc_dev->rproc_enable_status, 1);

    if (is_use_secisp()) {
        if(rproc_dev->sec_thread_wake == 0) {
            wakeup_secisp_kthread();
            rproc_dev->sec_thread_wake = 1;
        }
    }

    if (is_use_loadbin()) {
        if ((err = wakeup_ispbin_kthread()) != 0) {
            RPROC_ERR("Failed : wakeup_ispbin_kthread.0x%x\n", err);
            atomic_set(&rproc_dev->rproc_enable_status, 0);
            return -ENODEV;
        }

#ifdef DEBUG_HISI_ISP
        if (use_nonsec_isp()) {
            if ((err = isp_elf_load()) < 0) {
                RPROC_ERR("Failed : isp_elf_load.0x%x\n", err);
                if (err != -ENOENT) {
                    atomic_set(&rproc_dev->rproc_enable_status, 0);
                    return err;
                }
            }
        }
#endif
    }

    rproc = rproc_dev->isp_rproc->rproc;

    if (!rproc_get_sync_flag()) {
        RPROC_INFO("sync_flag exception.\n");
        atomic_set(&rproc_dev->rproc_enable_status, 0);
        return -EAGAIN;
    }

    rproc_set_sync_flag(false);
    set_rpmsg_status(0);

    /* clean up the invalid exception entries */
    if (!list_empty(&rproc->rvdevs)) {
        RPROC_ERR("Failed : enable exception will disable...\n");
        atomic_set(&rproc_dev->rproc_enable_status, 2);
        mutex_lock(&rproc_dev->hisi_isp_power_mutex);
        rproc_dev->hisi_isp_power_state = HISP_ISP_POWER_CLEAN;
        mutex_unlock(&rproc_dev->hisi_isp_power_mutex);
        hisi_isp_rproc_disable();
    }

    init_completion(&rproc->boot_comp);
    rproc_dev->isp_subsys_power_flag = 0;

    RPROC_INFO("rproc_enable...\n");
    err = rproc_add_virtio_devices(rproc);
    if (err) {
        RPROC_ERR("Failed : rproc_enable.%d\n", err);
        rproc_set_sync_flag(true);/*lint !e747 */
        atomic_set(&rproc_dev->rproc_enable_status, 0);
        return err;
    }

    RPROC_INFO("waiting boot_comp...\n");
    wait_for_completion(&rproc->boot_comp);
    RPROC_INFO("wait boot_comp X\n");

    if (!rproc->rproc_enable_flag || !rproc_dev->rpmsg_status) {
        RPROC_ERR("Failed : rproc_enable rproc_enable_flag.%d, rpmsg_status.%d", rproc->rproc_enable_flag, rproc_dev->rpmsg_status);
        rproc_set_sync_flag(true);
        atomic_set(&rproc_dev->rproc_enable_status, 0);
        return -EAGAIN;
    }

    err = hisi_isp_rproc_pgd_set(rproc);
    if (0 != err) {
        RPROC_ERR("Failed : hisi_isp_rproc_pgd_set.%d\n", err);
        goto enable_err;
    }
    mutex_lock(&rproc_dev->hisi_isp_power_mutex);
    rproc_dev->hisi_isp_power_state = HISP_ISP_POWER_ON;
    mutex_unlock(&rproc_dev->hisi_isp_power_mutex);
    hisi_isp_efuse_deskew();
    rproc_set_shared_clk_init();
    err = isp_device_enable();
    if (0 != err) {
        RPROC_ERR("Failed : isp_device_enable.%d\n", err);
        goto enable_err;
    }

    RPROC_INFO("-\n");

    return 0;
enable_err:
    mutex_lock(&rproc_dev->hisi_isp_power_mutex);
    rproc_dev->hisi_isp_power_state = HISP_ISP_POWER_FAILE;
    mutex_unlock(&rproc_dev->hisi_isp_power_mutex);
    hisi_isp_rproc_disable();

    return err;
}
EXPORT_SYMBOL(hisi_isp_rproc_enable);
/*lint -save -e454*/
int hisp_jpeg_powerup(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = 0;

    if(dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }

    mutex_lock(&dev->jpeg_mutex);
    if (!wake_lock_active(&dev->jpeg_wakelock)) {
        wake_lock(&dev->jpeg_wakelock);
        RPROC_INFO("jpeg power up wake lock.\n");
    }
    mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
    switch (dev->case_type) {
        case NONSEC_CASE:
            ret = hisp_nsec_jpeg_powerup();
            break;
        case SEC_CASE:
            ret = hisp_sec_jpeg_powerup();
            break;
        default:
            ret = -EINVAL;
            break;
    }
    if (ret != 0) {
    RPROC_ERR("Failed : jpeg power up fail.%d, case_type.%d\n", ret, dev->case_type);
    mutex_lock(&dev->jpeg_mutex);
    if (wake_lock_active(&dev->jpeg_wakelock)) {
        wake_unlock(&dev->jpeg_wakelock);
        RPROC_INFO("jpeg power up wake unlock.\n");
    }
    mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
    }
    return ret;
}
/*lint -restore */
EXPORT_SYMBOL(hisp_jpeg_powerup);

int hisp_jpeg_powerdn(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = 0;

    if(dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }

    switch (dev->case_type) {
        case NONSEC_CASE:
            ret = hisp_nsec_jpeg_powerdn();
            break;
        case SEC_CASE:
            ret = hisp_sec_jpeg_powerdn();
            break;
        default:
            ret = -EINVAL;
            break;
    }
    mutex_lock(&dev->jpeg_mutex);
    if (wake_lock_active(&dev->jpeg_wakelock)) {
        wake_unlock(&dev->jpeg_wakelock);
        RPROC_INFO("jpeg power up wake unlock.\n");
    }
    mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
    if(ret != 0)
        RPROC_ERR("Failed : jpeg power down fail.%d, case_type.%d\n", ret, dev->case_type);
    return ret;
}
EXPORT_SYMBOL(hisp_jpeg_powerdn);

static struct rproc_ops hisi_rproc_ops = {
    .start      = hisi_rproc_start,
    .stop       = hisi_rproc_stop,
    .kick       = hisi_rproc_kick,
    .da_to_va   = hisp_rproc_da_to_va,
};

int hisi_rproc_select_def(void)
{
	struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int ret;

    RPROC_INFO("+\n");
    if(rproc_dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }
    if (!hw_is_fpga_board()) {
        ret = pinctrl_select_state(rproc_dev->isp_data.isp_pinctrl, rproc_dev->isp_data.pinctrl_def);
        if (0 != ret) {
            RPROC_ERR("Failed : could not set pins to default state.\n");
            return ret;
        }
    }
    RPROC_INFO("-\n");

    return 0;
}

int hisi_rproc_select_idle(void)
{
	struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int ret;

    RPROC_INFO("+\n");
    if(rproc_dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }
    if (!hw_is_fpga_board()) {
        ret = pinctrl_select_state(rproc_dev->isp_data.isp_pinctrl, rproc_dev->isp_data.pinctrl_idle);
        if (0 != ret) {
            RPROC_ERR("Failed : could not set pins to ilde state.\n");
            return ret;
        }
    }
	RPROC_INFO("-\n");

    return 0;
}

int bypass_power_updn(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return dev->bypass_pwr_updn;
}

int set_power_updn(int bypass)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    if (bypass != 0 && bypass != 1) {/*lint !e774 */
        RPROC_ERR("Failed : bypass.%x\n", bypass);
        return -EINVAL;
    }

    dev->bypass_pwr_updn = bypass;

    return 0;
}

unsigned int get_debug_isp_clk_enable(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return dev->hisi_isp_clk.enable;
}

int set_debug_isp_clk_enable(int state)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    if (state != 0 && state != 1) {/*lint !e774 */
        RPROC_ERR("Failed : state.%x\n", state);
        return -EINVAL;
    }

    dev->hisi_isp_clk.enable = state;

    return 0;
}

int set_debug_isp_clk_freq(unsigned int type, unsigned long value)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    if (type >= ISP_CLK_MAX || type & dev->hisi_isp_clk.freqmask) {
        pr_err("[%s] Failed : type.%d, freqmask.0x%x\n", __func__, type, dev->hisi_isp_clk.freqmask);
        return -EINVAL;
    }

    dev->hisi_isp_clk.freq[type] = value;
    pr_info("[%s] freq.%ld -> type.%d\n", __func__, value, type);

    return 0;
}

unsigned long get_debug_isp_clk_freq(unsigned int type)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    unsigned long freq = 0;

    if (!get_debug_isp_clk_enable()) {
        return 0;
    }

    if (type >= ISP_CLK_MAX || type & dev->hisi_isp_clk.freqmask) {
        pr_err("[%s] Failed : type.%d, freqmask.0x%x\n", __func__, type, dev->hisi_isp_clk.freqmask);
        return 0;
    }
    freq = dev->hisi_isp_clk.freq[type];
    pr_info("[%s] type.%d freq.%ld\n", __func__, type, freq);
    return freq;
}

static unsigned int hisp_get_isplogic(void)
{
    struct device_node *np = NULL;
    char *name = NULL;
    unsigned int isplogic = 1;
    int ret      = 0;

    name = hisp_get_logicname();

    np = of_find_compatible_node(NULL, NULL, name);
    if (!np) {
        pr_err("%s: of_find_compatible_node failed, %s\n", __func__, name);
        return ISP_UDP;
        }

    if ((ret = of_property_read_u32(np, "hisi,isplogic", (unsigned int *)(&isplogic))) < 0 ) {
        pr_err("[%s] Failed: isplogic of_property_read_u32.%d\n", __func__, ret);
        return ISP_FPGA_EXC;
        }

    pr_info("[%s] isplogic.0x%x\n", __func__, isplogic);
    return isplogic;
}

static unsigned int get_boardid(void)
{
#define BOARDID_SIZE (4)
    struct rproc_boot_device *dev = &rproc_boot_dev;
    unsigned int boardid[BOARDID_SIZE] = {0};
    struct device_node *root = NULL;
    unsigned int pctrl_stat10 = 0;
    unsigned int isplogic_type = 0;
    int ret = 0;

    dev->bypass_pwr_updn = 0;
    if ((root = of_find_node_by_path("/")) == NULL) {/*lint !e838 */
        RPROC_ERR("Failed : of_find_node_by_path.%pK\n", root);
        goto err_get_bid;
    }

    if ((ret = of_property_read_u32_array(root, "hisi,boardid", boardid, BOARDID_SIZE)) < 0 ) {
        RPROC_ERR("Failed : of_property_read_u32.%d\n", ret);
        goto err_get_bid;
    }
    RPROC_INFO("Board ID.(%x %x %x %x)\n", boardid[0], boardid[1], boardid[2], boardid[3]);

    isplogic_type = hisp_get_isplogic();
    switch(isplogic_type){
        case ISP_FPGA_EXCLUDE:
            RPROC_ERR("ISP/Camera Power Up and Down May be Bypassed isplogic state.%d\n", isplogic_type);
            dev->bypass_pwr_updn = 1;
            break;
        case ISP_FPGA:
            if (dev->reg[PCTRL] == NULL) {/*lint !e747  */
                RPROC_ERR("Failed : pctrl_base ioremap.\n");
                break;
            }
            pctrl_stat10 = readl((volatile void __iomem*)(dev->reg[PCTRL] + ISP_PCTRL_PERI_STAT_ADDR));/*lint !e732 */
            if ((pctrl_stat10 & (ISP_PCTRL_PERI_FLAG)) == 0) {
                RPROC_ERR("ISP/Camera Power Up and Down May be Bypassed, pctrl_stat10.0x%x\n", pctrl_stat10);
                dev->bypass_pwr_updn = 1;
            }
            break;
        case ISP_UDP:
        case ISP_FPGA_EXC:
            break;
        default :
            RPROC_ERR("ERROR: isplogic state.%d\n", isplogic_type);
            break;
        }
    return boardid[0];
err_get_bid:
    return INVALID_BOARDID;/*lint !e570 */
}

/*lint -save -e429*/
static struct hisi_rproc_data *hisi_rproc_data_dtget(struct device *pdev)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    struct device_node *np = pdev->of_node;
    struct hisi_rproc_data *data;
    const char *name = NULL;
    unsigned int addr, platform_info, ispa7_clk;
    unsigned int isppd_adb_flag = 0;
    unsigned int isp_efuse_flag = 0;
    int ret;

    if (!np) {
        RPROC_ERR("Failed : No dt node\n");
        return NULL;
    }

    if ((dev->boardid = get_boardid()) == INVALID_BOARDID)/*lint !e650 */
        return NULL;

    if (!use_sec_isp() && !use_nonsec_isp()) {
        dev->isp_subsys_ip = devm_regulator_get(pdev, "isp-core");
        if (IS_ERR(dev->isp_subsys_ip)) {
            RPROC_ERR("Failed : Couldn't get regulator ip.%pK\n", dev->isp_subsys_ip);
            return NULL;
        }
        RPROC_INFO("isp_subsys_ip.%pK\n", dev->isp_subsys_ip);
    }

    if(use_sec_isp() || use_nonsec_isp()){
        dev->ap_a7_mbox = HISI_RPROC_ISP_MBX23;
        dev->a7_ap_mbox = HISI_RPROC_ISP_MBX8;
    }else{
        dev->ap_a7_mbox = HISI_RPROC_ISP_MBX2;
        dev->a7_ap_mbox = HISI_RPROC_ISP_MBX0;
    }
    data = devm_kzalloc(pdev, sizeof(struct hisi_rproc_data), GFP_KERNEL);
    if (!data) {
        RPROC_ERR("Failed : cannot allocate platform data memory.%pK\n", data);
        return NULL;
    }

	if ((ret = of_property_read_string(np, "isp-names", &name)) < 0 ) {
		RPROC_ERR("Failed : isp-names.%s of_property_read_string.%d\n", name, ret);
		return NULL;
	}
	data->name = name;

	if ((ret = of_property_read_string(np, "firmware-names", &name)) < 0 ) {
		RPROC_ERR("Failed : firmware-names.%s of_property_read_string.%d\n", name, ret);
		return NULL;
	}
	data->firmware = name;

	if ((ret = of_property_read_string(np, "bootware-names", &name)) < 0 ) {
		RPROC_ERR("Failed : bootware-names.%s of_property_read_string.%d\n", name, ret);
		return NULL;
	}
	data->bootware = name;

	if ((ret = of_property_read_string(np, "mailbox-names", &name)) < 0 ) {
		RPROC_ERR("Failed : mailbox-names.%s of_property_read_32.%d\n", name, ret);
		return NULL;
	}
	data->mbox_name = name;

	if ((ret = of_property_read_u32(np, "isp-ipc-addr", &addr)) < 0 ) {
		RPROC_ERR("Failed : isp-ipc_addr.0x%x of_property_read_u32.%d\n", addr, ret);
		return NULL;
	}
    data->ipc_addr = addr;
    RPROC_DEBUG("isp-ipc-addr.%u \n", data->ipc_addr);

	if ((ret = of_property_read_u32(np, "isp-remap-addr", &addr)) < 0 ) {
		RPROC_ERR("Failed : isp-remap-addr.0x%x of_property_read_u32.%d\n", addr, ret);
		return NULL;
	}
    dev->remap_addr = addr;

	if ((ret = of_property_read_u32(np, "isp_local_timer", &platform_info)) < 0 ) {
		RPROC_ERR("Failed: isp_local_timer.0x%x of_property_read_u32.%d\n", platform_info, ret);
		return NULL;
	}
    dev->tmp_plat_cfg.isp_local_timer = platform_info;
	RPROC_INFO("isp_local_timer = %d\n", dev->tmp_plat_cfg.isp_local_timer);

    dev->ispa7_clk = devm_clk_get(pdev, NULL);
    if(IS_ERR_OR_NULL(dev->ispa7_clk)) {
        RPROC_ERR("Failed : ispa7_clk.%ld\n", PTR_ERR(dev->ispa7_clk));
        return NULL;
    }

	if ((ret = of_property_read_u32(np, "ispa7-default-clk", &ispa7_clk)) < 0 ) {
		RPROC_ERR("Failed: ispa7_clk.0x%x of_property_read_u32.%d\n", ispa7_clk, ret);
		return NULL;
	}
	dev->ispa7_clk_value = ispa7_clk;
	RPROC_INFO("ispa7_clk.%d\n", dev->ispa7_clk_value);

	if ((ret = of_property_read_u32(np, "isppd-adb-flag", &isppd_adb_flag)) < 0 ) {
		RPROC_ERR("Failed: isppd-adb-flag.0x%x of_property_read_u32.%d\n", isppd_adb_flag, ret);
		return NULL;
	}
	dev->isppd_adb_flag = isppd_adb_flag;
	RPROC_INFO("isppd_adb_flag.%d\n", dev->isppd_adb_flag);

	if ((ret = of_property_read_u32(np, "isp-efuse-flag", &isp_efuse_flag)) < 0 ) {
		RPROC_ERR("Failed: isp-efuse-flag.0x%x of_property_read_u32.%d\n", isp_efuse_flag, ret);
		return NULL;
	}
	dev->isp_efuse_flag = isp_efuse_flag;
	RPROC_INFO("isppd_adb_flag.%d\n", dev->isp_efuse_flag);

    if (IS_HI6250(dev->boardid)) {
        if ((ret = of_property_read_string_index(np, "clock-names", 1, &name)) < 0) {
            RPROC_ERR("Failed : isp_timer clock-names boardid.0x%x, ret.%d\n", dev->boardid, ret);
            return NULL;
        }
        RPROC_INFO("[ID.0x%x] isp_timer : clock-names.%s, \n", dev->boardid, name);

    dev->isp_timer = devm_clk_get(pdev, name);
    if (IS_ERR_OR_NULL(dev->isp_timer)) {
            RPROC_ERR("Failed : isp_timer devm_clk_get boardid.0x%x, ret.%d\n", dev->boardid, ret);
            return NULL;
        }
        RPROC_INFO("[ID.0x%x] isp_timer : %pK\n", dev->boardid, dev->isp_timer);
    }

    return data;
}

struct coresight_user_para {
    unsigned long long coresight_addr_da;   /*R8 addr*/
    unsigned long long coresight_addr_vir;  /*AP addr*/
};
struct coresight_user_para g_cs_para;

void coresight_mem_init(struct device *dev)
{
    void *addr_vir;
    dma_addr_t dma_addr = 0;

    if ((addr_vir = dma_alloc_coherent(dev, 0x1000, &dma_addr, GFP_KERNEL)) == NULL) {
        pr_err("[coresight] [%s] coresight_mem_init.%pK\n", __func__, addr_vir);
        return;
    }

    pr_err("[coresight] [%s] coresight_mem_init  addr_vir:%pK addr_ph:%pK \n", __func__, (void *)addr_vir, (void *)dma_addr);
    /*lint !e110 !e82 !e533*/
    g_cs_para.coresight_addr_da = (unsigned long long)dma_addr;  /*lint !e110 !e82 !e533 !e626 */
    g_cs_para.coresight_addr_vir = (unsigned long long)addr_vir; /*lint !e110 !e82 !e533 !e626 */

    pr_err("[coresight] coresight_mem_init  addr_vir:%llx addr_ph:%llx \n", g_cs_para.coresight_addr_vir, g_cs_para.coresight_addr_da);
    return;
}

/*lint -restore */
#ifdef CONFIG_HISI_REMOTEPROC_DMAALLOC_DEBUG
void *get_vring_dma_addr(u64 *dma_handle, size_t size, unsigned int index)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct hisi_isp_vring_s *hisi_isp_vring = NULL;
    if(rproc_dev == NULL){
        pr_err("%s: rproc_boot_device in NULL\n", __func__);
        return NULL;
    }
    RPROC_INFO("+\n");
    hisi_isp_vring = &rproc_dev->hisi_isp_vring[index];
    if(hisi_isp_vring == NULL){
        pr_err("%s: hisi_isp_vring is NULL\n", __func__);
        return NULL;
    }
    if(hisi_isp_vring->size != size){
        pr_err("%s: hisi_isp_vring size not same: 0x%lx --> 0x%lx\n", __func__,hisi_isp_vring->size,size);
        return NULL;
    }
    *dma_handle = hisi_isp_vring->paddr;
    RPROC_INFO("-\n");
    return hisi_isp_vring->virt_addr;
}

static int get_vring_dma_addr_probe(struct platform_device *pdev)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct hisi_isp_vring_s *hisi_isp_vring = NULL;

    unsigned int i = 0;

    if((rproc_dev == NULL) || (pdev == NULL)){
        pr_err("%s: rproc_boot_device in NULL\n", __func__);
        return -ENOMEM;
    }
    for(i = 0;i < HISI_ISP_VRING_NUM;i++)
    {
        hisi_isp_vring = &rproc_dev->hisi_isp_vring[i];
        if(hisi_isp_vring == NULL){
            pr_err("%s: hisi_isp_vring is NULL\n", __func__);
            return -ENOMEM;
        }
        if(i < (HISI_ISP_VRING_NUM - 1))
           hisi_isp_vring->size =  HISI_ISP_VRING_SIEZ;
        else
           hisi_isp_vring->size =  HISI_ISP_VQUEUE_SIEZ;

        hisi_isp_vring->virt_addr = dma_alloc_coherent(&pdev->dev,
                         hisi_isp_vring->size, &hisi_isp_vring->paddr,
                         GFP_KERNEL);
        if (!hisi_isp_vring->virt_addr) {
            return -ENOMEM;
        }
    }
    return 0;
}
static int get_vring_dma_addr_remove(struct platform_device *pdev)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct hisi_isp_vring_s *hisi_isp_vring = NULL;
    unsigned int i = 0;
    if(rproc_dev == NULL){
        pr_err("%s: rproc_boot_device in NULL\n", __func__);
        return -ENOMEM;
    }
    for(i = 0;i < HISI_ISP_VRING_NUM;i++)
    {
        hisi_isp_vring = &rproc_dev->hisi_isp_vring[i];
        if(hisi_isp_vring == NULL){
            pr_err("%s: hisi_isp_vring is NULL\n", __func__);
            return -ENOMEM;
        }
        dma_free_coherent(&pdev->dev, hisi_isp_vring->size,\
                          hisi_isp_vring->virt_addr, hisi_isp_vring->paddr);
    }
    return 0;
}
#endif

void __iomem *get_regaddr_by_pa(unsigned int type)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    if (type >= HISP_MIN(HISP_MAX_REGTYPE, dev->reg_num)) {
        pr_err("[%s] unsupported type.0x%x\n", __func__, type);
        return NULL;
    }

    return (dev->reg[type] ? dev->reg[type] : NULL); /*lint !e661 !e662 */ 
}

static int hisp_regaddr_init(struct platform_device *pdev)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct device * dev = NULL;
    struct device_node *np = NULL;
    int i = 0, ret = 0;

    if (!pdev) {
        RPROC_ERR("Failed : platform device not exit\n");
        return -ENXIO;
    }

    dev = &pdev->dev;
    if (!dev) {
        RPROC_ERR("Failed : No device\n");
        return -ENXIO;
    }

    np = dev->of_node;
    if (!np) {
        RPROC_ERR("Failed : No dt node\n");
        return -ENXIO;
    }

    if ((ret = of_property_read_u32(np, "reg-num", (unsigned int *)(&rproc_dev->reg_num))) < 0){
        pr_err("[%s] Failed: reg-num of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }

    for (i = 0; i < HISP_MIN(HISP_MAX_REGTYPE, rproc_dev->reg_num); i ++) {/*lint !e574 */
        if ((rproc_dev->r[i] = platform_get_resource(pdev, IORESOURCE_MEM, i)) == NULL) {
            pr_err("[%s] Failed : platform_get_resource.%pK\n", __func__, rproc_dev->r[i]);
            return -ENXIO;
        }

        rproc_dev->reg[i] = (void __iomem *)ioremap(rproc_dev->r[i]->start, resource_size(rproc_dev->r[i]));
        if (rproc_dev->reg[i] == NULL) {
            pr_err("[%s] resource.%d ioremap fail\n", __func__, i);
            goto error;
        }
    }

    return 0;

error:
    for(; i >= 0; i --) {
        if (rproc_dev->reg[i] != NULL){
            iounmap(rproc_dev->reg[i]);
            rproc_dev->reg[i] = NULL;
        }
    }
    return -ENOMEM;
}

static void hisp_regaddr_deinit(void){
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int i;

    RPROC_INFO("+\n");
    for (i = 0; i < HISP_MIN(HISP_MAX_REGTYPE, rproc_dev->reg_num); i ++) {/*lint !e574 */ 
        if (rproc_dev->reg[i]) {
            iounmap(rproc_dev->reg[i]);
            rproc_dev->reg[i] = NULL;
        }
    }
    RPROC_INFO("-\n");
}

static int hisi_rproc_probe(struct platform_device *pdev)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct device *dev = &pdev->dev;
    struct device_node *np = dev->of_node;
    struct hisi_rproc_data *data;
    struct hisi_isp_rproc *hproc;
    int ret;

    RPROC_INFO("+\n");
    rproc_dev->probe_finished  = 0;
    rproc_dev->sec_thread_wake = 0;
    rproc_dev->isp_pdev = pdev;
    rproc_dev->isp_bin_state = 0;
    rproc_dev->isp_bin_vaddr = NULL;
    rproc_dev->rsctable_vaddr = NULL;
    memset(&rproc_dev->hisi_isp_clk,0,sizeof(struct hisi_isp_clk_dump_s));
    mutex_init(&rproc_dev->sharedbuf_mutex);
    wake_lock_init(&rproc_dev->jpeg_wakelock, WAKE_LOCK_SUSPEND, "jpeg_wakelock");
    wake_lock_init(&rproc_dev->ispcpu_wakelock, WAKE_LOCK_SUSPEND, "ispcpu_wakelock");
    mutex_init(&rproc_dev->jpeg_mutex);
    mutex_init(&rproc_dev->ispcpu_mutex);
    mutex_init(&rproc_dev->rpmsg_ready_mutex);
    mutex_init(&rproc_dev->hisi_isp_power_mutex);
    spin_lock_init(&rproc_dev->rpmsg_ready_spin_mutex);

    if ((ret = hisp_regaddr_init(pdev)) < 0) {
        pr_err("[%s] Failed : hisp_regaddr_init.%d\n", __func__, ret);
        return -ENOMEM;
    }

    ret = rdr_isp_init();
    if (ret) {
        pr_err("%s: rdr_isp_init failed.%d", __func__, ret);
        goto free_regaddr_init;
    }

    ret = hisi_isp_rproc_case_set(DEFAULT_CASE);
    if (ret) {
        pr_err("%s: case set falied, case.%x\n", __func__, DEFAULT_CASE);
        goto free_regaddr_init;
    }

	if ((ret = of_property_read_u32(np, "hisi,use_secisp", &rproc_dev->secisp)) < 0 ) {/*lint !e64 */
        RPROC_ERR("Failed: secisp.0x%x of_property_read_u32.%d\n", rproc_dev->secisp, ret);
        goto free_regaddr_init;
    }

    if ((ret = of_property_read_u32(np, "useloadbin", &rproc_dev->loadbin)) < 0 ) {/*lint !e64 */
        RPROC_ERR("Failed: loadbin.0x%x of_property_read_u32.%d\n", rproc_dev->loadbin, ret);
        goto free_regaddr_init;
    }

    if (rproc_dev->loadbin) {
        rproc_dev->loadispbin = kthread_create(isp_bin_load, NULL, "loadispbin");
        if (IS_ERR(rproc_dev->loadispbin)) {
            ret = -1;
            pr_err("[%s] Failed : kthread_create.%ld\n", __func__, PTR_ERR(rproc_dev->loadispbin));
            goto free_regaddr_init;
        }
    }

    if (is_use_secisp()) {
        ret = hisi_isp_rproc_case_set(SEC_CASE);
        if (ret) {
            pr_err("%s: case set falied, case.%x\n", __func__, SEC_CASE);
            goto free_regaddr_init;
        }

        pr_alert("%s: sec isp probe.\n", __func__);
        if ((ret = hisi_atfisp_probe(pdev)) < 0) {
            RPROC_ERR("Failed : hisi_atfisp_probe.%d\n", ret);
            goto free_regaddr_init;
        }

        pr_alert("%s: no sec isp probe.\n", __func__);
        if ((ret = hisi_isp_nsec_probe(pdev)) < 0) {
            RPROC_ERR("Failed : hisi_isp_nsec_probe.%d\n", ret);
            goto free_regaddr_init;
        }
        hisp_mem_pool_init();
        atomic_set(&rproc_dev->rproc_enable_status, 0);
        hisp_mdc_dev_init();
        if(get_isp_mdc_flag()){
            if ((ret = mdc_addr_pa_init()) < 0) {
                RPROC_ERR("Failed : mdc_addr_pa_init.%d\n", ret);
                goto free_regaddr_init;
            }
        }
    }
	if ((data = hisi_rproc_data_dtget(dev)) == NULL ) {
		dev_err(&pdev->dev, "hisi_rproc_data_dtget: %pK\n", data);
		goto free_regaddr_init;
	}

    ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(64));/*lint !e598 !e648 */
    if (ret) {
        dev_err(&pdev->dev, "dma_set_coherent_mask: %d\n", ret);
        goto free_regaddr_init;
    }

	RPROC_INFO("rproc_alloc\n");
    hisi_rproc = rproc_alloc(&pdev->dev, data->name, &hisi_rproc_ops,
                data->firmware, sizeof(*hproc));
    if (!hisi_rproc) {
        ret = -ENOMEM;
        goto free_regaddr_init;
    }

	if ((ret = rproc_bootware_attach(hisi_rproc, data->bootware)) < 0 ) {
		dev_err(&pdev->dev, "rproc_bootware_attach: %d\n", ret);
		goto free_hisi_rproc;
	}

    hisi_rproc->ipc_addr = data->ipc_addr;
    hisi_rproc->has_iommu = true;
    rproc_dev->hisi_isp_power_state = HISP_ISP_POWER_OFF;
    RPROC_INFO("hisi_rproc.%pK, priv.%pK, ipc_addr = 0x%x\n",
                        hisi_rproc, hisi_rproc->priv, hisi_rproc->ipc_addr);

    hproc = hisi_rproc->priv;
    hproc->rproc = hisi_rproc;
    rproc_dev->isp_rproc = hproc;
    rproc_dev->hisi_isp_clk.freqmask = (1 << ISPI2C_CLK);
    rproc_dev->hisi_isp_clk.enable = 0;

	RPROC_INFO("hproc->rproc.%pK, hproc.%pK\n", hproc->rproc, hproc);

    platform_set_drvdata(pdev, hisi_rproc);

	RPROC_INFO("rproc_add\n");

    ret = rproc_add(hisi_rproc);

    if (ret)
        goto free_hisi_rproc;
#ifdef CONFIG_HISI_REMOTEPROC_DMAALLOC_DEBUG
    ret = get_vring_dma_addr_probe(pdev);
    if (ret < 0)
        goto free_hisi_rproc;
#endif
	ret = init_hisi_ipc_resource();
	if (0 != ret) {
		RPROC_ERR("Failed : init_hisi_ipc_resource.%d\n", ret);
		goto free_hisi_rproc;
	}

	hproc->nb.notifier_call = hisi_rproc_mbox_callback;

	ret = RPROC_MONITOR_REGISTER(rproc_dev->a7_ap_mbox, &hproc->nb);/*lint !e64 */
	if (0 != ret) {
		RPROC_ERR("Failed : RPROC_MONITOR_REGISTER.%d\n", ret);
		goto free_hisi_rproc;
	}
    rproc_dev->rpmsg_ready_state = 0;
    hisi_rproc->rproc_enable_flag = false;
    rproc_set_sync_flag(true);

#ifdef ISP_CORESIGHT
    coresight_mem_init(dev);
#endif
    if ((ret = hisp_rpmsg_rdr_init()) < 0 ) {
        dev_err(&pdev->dev, "Fail :hisp_rpmsg_rdr_init: %d\n", ret);
    }
    if (!client_isp)
        client_isp = dsm_register_client(&dev_isp);
    rproc_dev->probe_finished = 1;
    RPROC_INFO("-\n");
    return 0;

free_hisi_rproc:
    rproc_put(hisi_rproc);
    wake_lock_destroy(&rproc_dev->jpeg_wakelock);
    wake_lock_destroy(&rproc_dev->ispcpu_wakelock);
    mutex_destroy(&rproc_dev->jpeg_mutex);
    mutex_destroy(&rproc_dev->ispcpu_mutex);
    mutex_destroy(&rproc_dev->rpmsg_ready_mutex);

free_regaddr_init:
    hisp_regaddr_deinit();
    return ret;
}

static int hisi_rproc_remove(struct platform_device *pdev)
{
    struct rproc *rproc = platform_get_drvdata(pdev);
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
	struct hisi_isp_rproc *hproc = NULL;
	int ret;

	RPROC_INFO("+\n");
    if (NULL == rproc) {
        RPROC_ERR("Failed : rproc.%pK\n", rproc);
        return -ENOMEM;
    }

    hproc = rproc->priv;
    rproc_dev->rpmsg_ready_state = 0;
    if (NULL != hproc) {
        ret = RPROC_MONITOR_UNREGISTER(rproc_dev->a7_ap_mbox, &hproc->nb);/*lint !e64 */
        if (0 != ret) {
            RPROC_ERR("Failed : RPROC_MONITOR_UNREGISTER.%d\n", ret);
        }
    } else {
		RPROC_ERR("rproc->priv.%pK\n", rproc->priv);
    }
	RPROC_FLUSH_TX(rproc_dev->ap_a7_mbox);/*lint !e64 */
	kthread_stop(isp_rx_mbox->rpmsg_rx_tsk);

	kfifo_free(&isp_rx_mbox->rpmsg_rx_fifo);
	kfree(isp_rx_mbox);
    wake_lock_destroy(&rproc_dev->jpeg_wakelock);
    wake_lock_destroy(&rproc_dev->ispcpu_wakelock);
    mutex_destroy(&rproc_dev->jpeg_mutex);
    mutex_destroy(&rproc_dev->ispcpu_mutex);
    mutex_destroy(&rproc_dev->sharedbuf_mutex);
    mutex_destroy(&rproc_dev->rpmsg_ready_mutex);

    rdr_isp_exit();

#ifdef CONFIG_HISI_REMOTEPROC_DMAALLOC_DEBUG
    ret = get_vring_dma_addr_remove(pdev);
    if (ret < 0)
        RPROC_ERR("Failed : get_vring_dma_addr_remove.%d\n", ret);
#endif
    rproc_del(rproc);
    rproc_put(rproc);

    if (is_use_secisp()) {
        if ((ret = hisi_atfisp_remove(pdev)) < 0)
            RPROC_ERR("Failed : hisi_atfisp_remove.%d\n", ret);

        if ((ret = hisi_isp_nsec_remove(pdev)) != 0)
            RPROC_ERR("Failed : hisi_isp_nsec_remove.%d\n", ret);
        free_mdc_ion(MEM_MDC_SIZE);
    }
    hisp_regaddr_deinit();
    if ((ret = hisp_rpmsg_rdr_deinit()) < 0 )
        RPROC_ERR("Fail :hisp_rpmsg_rdr_deinit: %d\n", ret);

    if (rproc_dev->loadbin) {
        if(rproc_dev->loadispbin) {
            kthread_stop(rproc_dev->loadispbin);
            rproc_dev->loadispbin = NULL;
        }
    }
    if (rproc_dev->isp_bin_vaddr != NULL)
        vfree(rproc_dev->isp_bin_vaddr);

    RPROC_INFO("-\n");
    return 0;
}

static struct of_device_id hisi_rproc_of_match[] = {
    { .compatible = DTS_COMP_NAME},
    { },
};
MODULE_DEVICE_TABLE(of, hisi_rproc_of_match);

static struct platform_driver hisi_rproc_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = "isp",
        .of_match_table = of_match_ptr(hisi_rproc_of_match),
    },
    .probe  = hisi_rproc_probe,
    .remove = hisi_rproc_remove,
};
module_platform_driver(hisi_rproc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HiStar V100 Remote Processor control driver");

#ifdef DEBUG_HISI_ISP
static int ispupdate_bin_load(char *buffer)
{
    int ret          = -1;
    char *pathname   = "/system/vendor/firmware/isp.bin";
    struct file *fp;
    mm_segment_t fs;
    loff_t pos = 0;
    struct kstat m_stat;
    int length;

    if ((NULL == buffer)) {
        pr_err("buffer(%pK) is null", buffer);
        return -1;
    }

    /*get resource*/
    fp = filp_open(pathname, O_RDONLY, 0600);
    if (IS_ERR(fp)) {
        pr_err("filp_open(%s) failed", pathname);
        goto error;
    }

    ret = vfs_llseek(fp, 0, SEEK_SET);
    if (ret < 0) {
        pr_err("seek ops failed, ret %d", ret);
        goto error2;
    }

    fs = get_fs();/*lint !e501*/
    set_fs(KERNEL_DS);/*lint !e501 */

    if ((ret = vfs_stat(pathname, &m_stat)) < 0) {
        RPROC_ERR("Failed :%s vfs_stat: %d\n", pathname, ret);
        set_fs(fs);
        goto error2;
    }
    length = m_stat.size;

    pos = fp->f_pos;/*lint !e613 */
    ret = vfs_read(fp, (char __user *)buffer, length, &pos);/*lint !e613 */
    if (ret != length) {
        pr_err("read ops failed, ret=%d(len=%d)", ret, length);
        set_fs(fs);
        goto error2;
    }
    set_fs(fs);

    filp_close(fp, NULL);/*lint !e668 */

    return 0;

error2:
    filp_close(fp, NULL);/*lint !e668 */
error:
    pr_err("failed");
    return -1;
}

ssize_t ispbinupdate_show(struct device *pdev, struct device_attribute *attr, char *buf)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int ret = 0;

    RPROC_INFO("+\n");

    if(rproc_dev->probe_finished != 1)
    {
        RPROC_ERR("hisi_rproc_probe failed\n");
        return -EPERM;
    }
    if (atomic_read(&rproc_dev->rproc_enable_status) > 0 ) {
        pr_err("[%s] hisi_isp_rproc had been enabled, rproc_enable_status.0x%x\n", __func__, atomic_read(&rproc_dev->rproc_enable_status));
        return -ENODEV;
    }

    if (rproc_dev->isp_bin_vaddr == NULL) {
        RPROC_ERR("Failed : isp_bin_vaddr.NULL\n");
        return -ENOMEM;
    }

    if ((ret = ispupdate_bin_load(rproc_dev->isp_bin_vaddr)) < 0) {
        RPROC_ERR("Failed : ispupdate_bin_load\n");
        return -ENOMEM;
    }
    RPROC_INFO("-\n");
    return 0;
}
#endif
