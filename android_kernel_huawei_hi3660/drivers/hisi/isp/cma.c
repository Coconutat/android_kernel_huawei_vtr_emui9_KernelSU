/*
 * hisilicon ISP driver, hisi_fstcma.c
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/printk.h>
#include <linux/remoteproc.h>
#include <linux/hisi/hisi_drmdriver.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include <linux/of_reserved_mem.h>
#include <linux/version.h>

#define DTS_COMP_FSTCMA_NAME    "hisilicon,isp-fastboot-cma"

extern void create_mapping_late(phys_addr_t phys, unsigned long virt,
                                phys_addr_t size, pgprot_t prot);

struct hisi_fstcma_struct {
    struct device *device;
};

struct hisi_fstcma_struct fstcma_dev;

void *hisi_fstcma_alloc(dma_addr_t *dma_handle, size_t size, gfp_t flag)
{
    struct hisi_fstcma_struct *dev = (struct hisi_fstcma_struct *)&fstcma_dev;
    void *va;

    pr_info("%s: +\n", __func__);
    if (!dev->device) {
        pr_err("%s: failed.\n", __func__);
        return NULL;
    }

    va = dma_alloc_coherent(dev->device, size, dma_handle, flag);
    if (!va) {
        pr_err("%s: alloc failed.\n", __func__);
        return NULL;
    }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
    create_mapping_late((phys_addr_t)(*dma_handle), (unsigned long)phys_to_virt(*dma_handle), size, __pgprot(PROT_NORMAL_NC));
#endif
    pr_info("%s: -\n", __func__);
    return va;
}

void hisi_fstcma_free(void *va, dma_addr_t dma_handle, size_t size)
{
    struct hisi_fstcma_struct *dev = (struct hisi_fstcma_struct *)&fstcma_dev;

    pr_info("%s: +\n", __func__);

    if (va == NULL || dma_handle == 0) {
        pr_info("%s: cma_va.%pK\n", __func__, va);
        return;
    }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
    create_mapping_late((phys_addr_t)dma_handle, (unsigned long)phys_to_virt(dma_handle), size, __pgprot(PROT_NORMAL));
#endif
    dma_free_coherent(dev->device, size, va, dma_handle);

    pr_info("%s: -\n", __func__);
}

int hisi_fstcma_probe(struct platform_device *pdev)
{
    struct hisi_fstcma_struct *dev = (struct hisi_fstcma_struct *)&fstcma_dev;
    int ret;

    pr_info("%s: +\n", __func__);

    dev->device = &(pdev->dev);

    ret = of_reserved_mem_device_init(dev->device);
    if (0 != ret) {
        pr_err("%s: init failed, ret.%d\n", __func__, ret);
        goto out;
    }

    pr_info("%s: -\n", __func__);
    return 0;

out:
    dev->device = NULL;

    pr_err("%s: error-\n", __func__);
    return ret;
}

int hisi_fstcma_remove(struct platform_device *pdev)
{
    struct hisi_fstcma_struct *dev = (struct hisi_fstcma_struct *)&fstcma_dev;

    if (!dev->device) {
        pr_err("%s: failed.\n", __func__);
        return -EINVAL;
    }

    of_reserved_mem_device_release(dev->device);

    return 0;
}

static struct of_device_id hisi_fstcma_of_match[] = {
    { .compatible = DTS_COMP_FSTCMA_NAME},
    { },
};
MODULE_DEVICE_TABLE(of, hisi_fstcma_of_match);

static struct platform_driver hisi_fstcma_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = "fastboot_cma",
        .of_match_table = of_match_ptr(hisi_fstcma_of_match),
    },
    .probe  = hisi_fstcma_probe,
    .remove = hisi_fstcma_remove,
};

static int __init hisi_fstcma_init(void)
{
    pr_info("%s: +\n", __func__);
    return platform_driver_register(&hisi_fstcma_driver);
}
subsys_initcall(hisi_fstcma_init);

static void __exit hisi_fstcma_exit(void)
{
    platform_driver_unregister(&hisi_fstcma_driver);
}
module_exit(hisi_fstcma_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hisilicon ispcma module");
