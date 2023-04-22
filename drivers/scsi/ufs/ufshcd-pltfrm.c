/*
 * Universal Flash Storage Host controller Platform bus based glue driver
 *
 * This code is based on drivers/scsi/ufs/ufshcd-pltfrm.c
 * Copyright (C) 2011-2013 Samsung India Software Operations
 *
 * Authors:
 *	Santosh Yaraganavi <santosh.sy@samsung.com>
 *	Vinayak Holikatti <h.vinayak@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * See the COPYING file in the top-level directory or visit
 * <http://www.gnu.org/licenses/gpl-2.0.html>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This program is provided "AS IS" and "WITH ALL FAULTS" and
 * without warranty of any kind. You are solely responsible for
 * determining the appropriateness of using and distributing
 * the program and assume all risks associated with your exercise
 * of rights with respect to the program, including but not limited
 * to infringement of third party rights, the risks and costs of
 * program errors, damage to or loss of data, programs or equipment,
 * and unavailability or interruption of operations. Under no
 * circumstances will the contributor of this Program be liable for
 * any damages of any kind arising from your use or distribution of
 * this program.
 */

#define pr_fmt(fmt) "ufshcd :" fmt

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/bootdevice.h>

#include "ufshcd.h"
#include "ufs-kirin.h"

extern const struct ufs_hba_variant_ops ufs_hba_kirin_vops;

static const struct of_device_id ufs_of_match[] = {
    {.compatible = "jedec,ufs-1.1"},
    {
	.compatible = "hisilicon,kirin-ufs", .data = &ufs_hba_kirin_vops,
    },
    {}, /*lint !e785*/
};

static struct ufs_hba_variant_ops *get_variant_ops(struct device *dev)
{
	if (dev->of_node) {
		const struct of_device_id *match;

		match = of_match_node(ufs_of_match, dev->of_node);
		if (match)
			return (struct ufs_hba_variant_ops *)match->data;
	}

	return NULL;
}

#ifdef CONFIG_PM
/**
 * ufshcd_pltfrm_suspend - suspend power management function
 * @dev: pointer to device handle
 *
 * Returns 0 if successful
 * Returns non-zero otherwise
 */
static int ufshcd_pltfrm_suspend(struct device *dev)
{
	int ret;
	struct ufs_hba *hba = (struct ufs_hba *)dev_get_drvdata(dev);
	struct Scsi_Host *host = hba->host;
	dev_info(dev, "%s:%d ++\n", __func__, __LINE__);
	if (host->queue_quirk_flag |
		SHOST_QUIRK(SHOST_QUIRK_SCSI_QUIESCE_IN_LLD)) {
#ifdef CONFIG_HISI_BLK
		blk_generic_freeze(hba->host->use_blk_mq
					   ? &(hba->host->tag_set.lld_func)
					   : &(hba->host->bqt->lld_func),
			BLK_LLD, true);
#endif
		__set_quiesce_for_each_device(hba->host);
	}
	ret = ufshcd_system_suspend(dev_get_drvdata(dev));
	if (ret) {
		if (host->queue_quirk_flag |
			SHOST_QUIRK(SHOST_QUIRK_SCSI_QUIESCE_IN_LLD)) {
			__clr_quiesce_for_each_device(hba->host);
#ifdef CONFIG_HISI_BLK
			blk_generic_freeze(
				hba->host->use_blk_mq
					? &(hba->host->tag_set.lld_func)
					: &(hba->host->bqt->lld_func),
				BLK_LLD, false);
#endif
		}
	}
	dev_info(dev, "%s:%d ret:%d--\n", __func__, __LINE__, ret);
	return ret;
}

/**
 * ufshcd_pltfrm_resume - resume power management function
 * @dev: pointer to device handle
 *
 * Returns 0 if successful
 * Returns non-zero otherwise
 */
static int ufshcd_pltfrm_resume(struct device *dev)
{
	int ret;
	struct ufs_hba *hba = (struct ufs_hba *)dev_get_drvdata(dev);
	struct Scsi_Host *host = hba->host;
	dev_info(dev, "%s:%d ++\n", __func__, __LINE__);
	ret = ufshcd_system_resume(dev_get_drvdata(dev));
	if (host->queue_quirk_flag |
		SHOST_QUIRK(SHOST_QUIRK_SCSI_QUIESCE_IN_LLD)) {
		__clr_quiesce_for_each_device(hba->host);
#ifdef CONFIG_HISI_BLK
		blk_generic_freeze(hba->host->use_blk_mq
					   ? &(hba->host->tag_set.lld_func)
					   : &(hba->host->bqt->lld_func),
			BLK_LLD, false);
#endif
	}
	dev_info(dev, "%s:%d ret:%d--\n", __func__, __LINE__, ret);
	return ret;
}

static int ufshcd_pltfrm_runtime_suspend(struct device *dev)
{
	return ufshcd_runtime_suspend((struct ufs_hba *)dev_get_drvdata(dev));
}

static int ufshcd_pltfrm_runtime_resume(struct device *dev)
{
	return ufshcd_runtime_resume((struct ufs_hba *)dev_get_drvdata(dev));
}

static int ufshcd_pltfrm_runtime_idle(struct device *dev)
{
	return ufshcd_runtime_idle((struct ufs_hba *)dev_get_drvdata(dev));
}
#else /* !CONFIG_PM */
#define ufshcd_pltfrm_suspend	NULL
#define ufshcd_pltfrm_resume	NULL
#define ufshcd_pltfrm_runtime_suspend	NULL
#define ufshcd_pltfrm_runtime_resume	NULL
#define ufshcd_pltfrm_runtime_idle	NULL
#endif /* CONFIG_PM */

#define SHUTDOWN_TIMEOUT (32 * 1000)
static void ufshcd_pltfrm_shutdown(struct platform_device *pdev)
{
	struct ufs_hba *hba = (struct ufs_hba *)platform_get_drvdata(pdev);
	struct Scsi_Host *host = hba->host;
	unsigned long timeout = SHUTDOWN_TIMEOUT;

	dev_err(&pdev->dev, "%s ++\n", __func__);
	if (host->queue_quirk_flag | SHOST_QUIRK(SHOST_QUIRK_SCSI_QUIESCE_IN_LLD)) {
	#ifdef CONFIG_HISI_BLK
		blk_generic_freeze(hba->host->use_blk_mq ? &(hba->host->tag_set.lld_func) : &(hba->host->bqt->lld_func), BLK_LLD, true);
	#endif
		/*set all scsi device state to quiet to forbid io form blk level*/
		__set_quiesce_for_each_device(hba->host);
	}

	while (hba->lrb_in_use) {
			if (timeout == 0) {
				dev_err(&pdev->dev, "%s: wait cmdq complete reqs timeout!\n",
					__func__);
			}
			timeout--;
			mdelay(1);
	}

	ufshcd_shutdown(hba);

	dev_err(&pdev->dev, "%s --\n", __func__);
}

/**
 * ufshcd_pltfrm_probe - probe routine of the driver
 * @pdev: pointer to Platform device handle
 *
 * Returns 0 on success, non-zero value on failure
 */
int ufshcd_pltfrm_probe(struct platform_device *pdev)
{
	struct ufs_hba *hba;
	void __iomem *mmio_base;
	struct resource *mem_res;
	int irq, timer_irq = -1, err;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	unsigned int timer_intr_index;

	if (get_bootdevice_type() != BOOT_DEVICE_UFS) {
		dev_err(dev, "system is't booted from UFS on ARIES FPGA board\n");
		return -ENODEV;
	}

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mmio_base = devm_ioremap_resource(dev, mem_res);
	if (IS_ERR(*(void **)&mmio_base)) {
		err = PTR_ERR(*(void **)&mmio_base);
		goto out;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "IRQ resource not available\n");
		err = -ENODEV;
		goto out;
	}

	err = ufshcd_alloc_host(dev, &hba);
	if (err) {
		dev_err(&pdev->dev, "Allocation failed\n");
		goto out;
	}

	hba->vops = get_variant_ops(&pdev->dev);

	if (!of_property_read_u32(
		    np, "timer-interrupt-index", &timer_intr_index)) {
		timer_irq = platform_get_irq(pdev, timer_intr_index);
		if (timer_irq < 0)
			dev_err(dev, "UFS timer interrupt is not available!\n");
	}


	pm_runtime_set_active(&pdev->dev);
	pm_runtime_irq_safe(&pdev->dev);
	pm_suspend_ignore_children(&pdev->dev, true);
	pm_runtime_set_autosuspend_delay(&pdev->dev, 5);
	pm_runtime_use_autosuspend(&pdev->dev);

	if (of_find_property(np, "ufs-kirin-disable-pm-runtime", NULL))
		hba->caps |= DISABLE_UFS_PMRUNTIME;
	/* auto hibern8 can not exist with pm runtime */
	if (hba->caps & DISABLE_UFS_PMRUNTIME ||
		of_find_property(np, "ufs-kirin-use-auto-H8", NULL)) {
		pm_runtime_forbid(hba->dev);
	}
	pm_runtime_enable(&pdev->dev);

	err = ufshcd_init(hba, mmio_base, irq, timer_irq);
	if (err) {
		dev_err(dev, "Initialization failed\n");
		goto out_disable_rpm;
	}

#ifndef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	/* to improve writing key efficiency, remap key regs with writecombine */
	err = ufshcd_keyregs_remap_wc(hba, mem_res->start);
	if (err) {
		dev_err(dev, "ufshcd_keyregs_remap_wc err\n");
		goto out_disable_rpm;
	}

#endif
#endif
	platform_set_drvdata(pdev, hba);

	return 0;

out_disable_rpm:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
out:
	return err;
}

/**
 * ufshcd_pltfrm_remove - remove platform driver routine
 * @pdev: pointer to platform device handle
 *
 * Returns 0 on success, non-zero value on failure
 */
static int ufshcd_pltfrm_remove(struct platform_device *pdev)
{
	struct ufs_hba *hba =  (struct ufs_hba *)platform_get_drvdata(pdev);

	pm_runtime_get_sync(&(pdev)->dev);
	ufshcd_remove(hba);
	return 0;
}

static const struct dev_pm_ops ufshcd_dev_pm_ops = {
	.suspend	= ufshcd_pltfrm_suspend,
	.resume		= ufshcd_pltfrm_resume,
	.runtime_suspend = ufshcd_pltfrm_runtime_suspend,
	.runtime_resume  = ufshcd_pltfrm_runtime_resume,
	.runtime_idle    = ufshcd_pltfrm_runtime_idle,
};

static struct platform_driver ufshcd_pltfrm_driver = {
	.probe	= ufshcd_pltfrm_probe,
	.remove	= ufshcd_pltfrm_remove,
	.shutdown = ufshcd_pltfrm_shutdown,
	.driver	= {
		.name	= "ufshcd",
		.pm	= &ufshcd_dev_pm_ops,
		.of_match_table = ufs_of_match,
	},
};

module_platform_driver(ufshcd_pltfrm_driver);

MODULE_AUTHOR("Santosh Yaragnavi <santosh.sy@samsung.com>");
MODULE_AUTHOR("Vinayak Holikatti <h.vinayak@samsung.com>");
MODULE_DESCRIPTION("UFS host controller Platform bus based glue driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(UFSHCD_DRIVER_VERSION);
