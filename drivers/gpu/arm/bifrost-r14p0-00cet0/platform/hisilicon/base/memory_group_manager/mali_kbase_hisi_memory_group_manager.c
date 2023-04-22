/*
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/compiler.h>
#ifdef CONFIG_HISI_LB
#include <linux/hisi/hisi_lb.h>
#endif

#include "mali_kbase_hisi_memory_group_manager.h"

struct hisi_mgm_device {
	struct memory_group_manager_device mgm_dev;
	struct device *dev;
	/* Further device specific data */
};

/**
 * manager_alloc_pages -- Allocate a page of memory
 *
 * @mgm_dev: The memory group manager the request is being made through.
 * @id: The physical memory group ID for the page.
 * @gfp_mask: Allocation flags.
 * @order: The page order.
 *
 * Return: The struct page of the allocated page.
 */
static struct page *manager_alloc_pages(struct memory_group_manager_device *mgm_dev,
					u8 id, gfp_t gfp_mask, unsigned int order)
{
	struct page *page = NULL;

	(void)mgm_dev;

	/* For normal memory with policy_id = 0, we use alloc_pages() directly. */
	if (id == 0) {
		return alloc_pages(gfp_mask, order);
	}

	/* Begin to handle the alloc request with non-zero policy id. */
	#ifdef CONFIG_HISI_LB
		/* L.B intergration: Transfer the last buffer policy-id to hisi-driver.
		 * 1. Hisi-driver provide the lb_alloc_pages() function to
		 *    receive the cache policy id. Like:
		 *    struct page * lb_alloc_pages(lb_policy_id, gfp, order);
		 * 2. Call lb_alloc_pages() with pool->lb_policy_id instead of alloc_pages().
		 */
		page = lb_alloc_pages(id, gfp_mask, order);
	#else
		/* hisi driver does not support last buffer, but we use the non-zero policy id.
		 * warn on it and use normal page instead.
		 */
		pr_err("[mali] Attempt to alloc last buffer with no hisi-driver support, policy_id(%u).\n", id);
		WARN_ON(1);

		page = alloc_pages(gfp_mask, order);
	#endif

	return page;
}

static void manager_free_pages(struct memory_group_manager_device *mgm_dev,
			       u8 id, struct page *page, unsigned int order)
{
	int result = 0;

	(void)mgm_dev;

	/* For normal memory with policy_id = 0, we use __free_pages() directly. */
	if (id == 0) {
		__free_pages(page, order);
		return;
	}

	/* Begin to handle the free request with non-zero policy id. */
	#ifdef CONFIG_HISI_LB
		/* L.B intergration: Transfer the last buffer policy-id to hisi-driver when free pages.
		 * 1. Hisi-driver provide the lb_free_pages() function to receive the
		 *    cache policy id. Like:
		 *    void lb_free_pages(lb_policy_id, struct page *, order);
		 * 2. Call lb_free_pages() with pool->lb_policy_id instead of __free_pages().
		 */
		result = lb_free_pages(id, page, order);
		if (result) {
			pr_err("[mali] Free last buffer with policy_id(%u) failed.\n", id);
			return;
		}
	#else
		/* hisi driver does not support last buffer, but we use the non-zero policy id.
		 * warn on it and free the page.
		 */
		pr_err("[mali] Attempt to free last buffer with no hisi-driver support, policy_id(%u).\n", id);
		WARN_ON(1);
		(void)result;
		__free_pages(page, order);
	#endif

	return;
}

static u8 manager_get_import_memory_id(struct memory_group_manager_device *mgm_dev,
				       struct memory_group_manager_import_data *data)
{
	struct hisi_mgm_device *himgm_dev = container_of(mgm_dev, struct hisi_mgm_device, mgm_dev);

	/* Optional. */
	(void)himgm_dev;

	return 0;
}

static u64 manager_gpu_map_page(u8 mmu_level, u64 pte)
{
#ifdef CONFIG_HISI_LB
	/* L.B intergration: Get the GID info.
	 * 1. In order to get the phys's GID, we will call hisi-driver function
	 *    lb_pte_attr(phy) here. like:
	 *    u64 phy_gid = lb_pte_attr(as_phys_addr_t(phy));
	 *
	 * 2. Call page_table_entry_set with the created phy_gid instead of phy.
	 */
	return lb_pte_attr(pte);
#endif

	return 0;
}

static int manager_vm_insert_pfn_prot(struct page **pages, unsigned long page_nr,
				      pgprot_t *prot)
{
	if (pages == NULL || page_nr == 0 || prot == NULL)
		return 1;

#ifdef CONFIG_HISI_LB
		/* L.B intergration: Add page's GID information to vm_page_prot, So SMMU can handle
		 *                   CPU PTE correctly.
		 * 1. Hisi-driver provide the function lb_prot_build(struct page*, pgprot_t*)
		 *    to add the GID info in the returned pgprot_t*. Like:
		 *    lb_prot_build(phys_to_page(phys), &vma->vm_page_prot);
		 * 2. Call lb_prot_build() function to get vm_page_prot with GID.
		 */
		/* check pages with same policy id. */

		int err = lb_prot_build(pages[0], prot);
		if (err) {
			pr_err("[mali] Get last buffer's prot failed.\n");
			return err;
		}
#endif
	return 0;
}

static int memory_group_manager_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hisi_mgm_device *himgm_dev;

	himgm_dev = devm_kzalloc(&pdev->dev, sizeof(*himgm_dev), GFP_KERNEL);
	if (!himgm_dev)
		return -ENOMEM;

	himgm_dev->dev = dev;

	himgm_dev->mgm_dev.ops.mgm_alloc_pages = manager_alloc_pages;
	himgm_dev->mgm_dev.ops.mgm_free_pages = manager_free_pages;
	himgm_dev->mgm_dev.ops.get_import_memory_id = manager_get_import_memory_id;
	himgm_dev->mgm_dev.ops.gpu_map_page = manager_gpu_map_page;
	himgm_dev->mgm_dev.ops.vm_insert_pfn_prot = manager_vm_insert_pfn_prot;
	himgm_dev->mgm_dev.data = himgm_dev;

	platform_set_drvdata(pdev, &himgm_dev->mgm_dev);

	return 0;
}

static int memory_group_manager_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id memory_group_manager_dt_ids[] = {
	{ .compatible = "arm,physical_memory_group_manager" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, memory_group_manager_dt_ids);
#endif

static struct platform_driver memory_group_manager_driver = {
	.probe = memory_group_manager_probe,
	.remove = memory_group_manager_remove,
	.driver = {
		.name = "hisi_memory_group_manager",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(memory_group_manager_dt_ids),
	}
};

static int __init memory_group_manager_driver_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&memory_group_manager_driver);
	if (ret)
		pr_err("[mali] memory_group_manager_driver_init failed!\n");

	return ret;
}

rootfs_initcall(memory_group_manager_driver_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HISI Ltd.");
MODULE_VERSION("1.0");
