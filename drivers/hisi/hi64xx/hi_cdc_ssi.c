/*
 *	slimbus is a kernel driver which is used to manager SLIMbus devices
 *	Copyright (C) 2014	Hisilicon

 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.

 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.

 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>

/*lint -e750 -e730 -e838 -e529 -e438 -e778 -e826 -e774 -e747 -e527 -e456 -e454 -e455*/

#define HI64XX_CFG_BASE_ADDR            0x20007000

#define HI64XX_PAGE_SELECT_MASK_SSI     (0xFF)
#define HI64XX_REG32_OFFSET_MASK_SSI    (0xFC)
#define HI64XX_REG_VAL_BIT              (8)
#define HI64XX_REG_VAL_MASK             (0xFF)

#define HI64XX_PAGE_SELECT_REG_0_SSI    (0x1FD)
#define HI64XX_PAGE_SELECT_REG_1_SSI    (0x1FE)
#define HI64XX_PAGE_SELECT_REG_2_SSI    (0x1FF)

#define HI64XX_RAM2AXI_RD_DATA0         (HI64XX_CFG_BASE_ADDR + 0x23)
#define HI64XX_RAM2AXI_RD_DATA1         (HI64XX_CFG_BASE_ADDR + 0x24)
#define HI64XX_RAM2AXI_RD_DATA2         (HI64XX_CFG_BASE_ADDR + 0x25)
#define HI64XX_RAM2AXI_RD_DATA3         (HI64XX_CFG_BASE_ADDR + 0x26)

struct hi_cdcssi_priv {
	struct device *dev;
	struct clk *codec_ssi_clk;
	struct clk *pmu_audio_clk;
	struct pinctrl *pctrl;
	struct pinctrl_state *pin_default;
	struct pinctrl_state *pin_idle;
	unsigned int ssi_page_sel;
	void __iomem *ssi_base;
	/* mutex for sr&rw */
	struct mutex sr_rw_lock;
	bool pm_runtime_support;
};

static struct hi_cdcssi_priv *pdata;

void _ssi_select_reg_page(unsigned int reg)
{
	unsigned int reg_page = reg & (~HI64XX_PAGE_SELECT_MASK_SSI);

	if(pdata->ssi_page_sel == reg_page) {
		return;
	} else {
		pdata->ssi_page_sel = reg_page;
	}

	reg_page = reg_page >> HI64XX_REG_VAL_BIT;
	writel(reg_page & HI64XX_REG_VAL_MASK,  pdata->ssi_base + (HI64XX_PAGE_SELECT_REG_0_SSI << 2));

	reg_page = reg_page >> HI64XX_REG_VAL_BIT;
	writel(reg_page & HI64XX_REG_VAL_MASK,  pdata->ssi_base + (HI64XX_PAGE_SELECT_REG_1_SSI << 2));

	reg_page = reg_page >> HI64XX_REG_VAL_BIT;
	writel(reg_page & HI64XX_REG_VAL_MASK,  pdata->ssi_base + (HI64XX_PAGE_SELECT_REG_2_SSI << 2));
}

unsigned int ssi_reg_read8(unsigned int reg)
{
	unsigned int val = 0x5a;
	int pm_ret = 0;

	mutex_lock(&pdata->sr_rw_lock);

	if (pdata->pm_runtime_support) {
		pm_ret = pm_runtime_get_sync(pdata->dev);
		if (pm_ret < 0) {
			pr_err("[%s:%d] pm resume error, pm_ret:%d\n", __FUNCTION__, __LINE__, pm_ret);
			mutex_unlock(&pdata->sr_rw_lock);
			BUG_ON(true);
			return 0;
		}
	}

	_ssi_select_reg_page(reg);
	readl((void*)pdata->ssi_base + ((reg & (HI64XX_REG_VAL_MASK)) << 2));
	val = readl((void*)pdata->ssi_base + ((reg & (HI64XX_REG_VAL_MASK)) << 2));

	if (pdata->pm_runtime_support) {
		pm_runtime_mark_last_busy(pdata->dev);
		pm_runtime_put_autosuspend(pdata->dev);
	}

	mutex_unlock(&pdata->sr_rw_lock);
	return val;
}
EXPORT_SYMBOL(ssi_reg_read8);

unsigned int ssi_reg_read32(unsigned int reg)
{
	unsigned int ret = 0;
	int pm_ret = 0;

	mutex_lock(&pdata->sr_rw_lock);

	if (pdata->pm_runtime_support) {
		pm_ret = pm_runtime_get_sync(pdata->dev);
		if (pm_ret < 0) {
			pr_err("[%s:%d] pm resume error, pm_ret:%d\n", __FUNCTION__, __LINE__, pm_ret);
			mutex_unlock(&pdata->sr_rw_lock);
			BUG_ON(true);
			return 0;
		}
	}

	_ssi_select_reg_page(reg );
	readl((void*)pdata->ssi_base + ((reg & (HI64XX_REG32_OFFSET_MASK_SSI)) << 2));
	ret = readl((void*)pdata->ssi_base + ((reg & (HI64XX_REG32_OFFSET_MASK_SSI)) << 2));

	_ssi_select_reg_page(HI64XX_RAM2AXI_RD_DATA0 );
	readl((void*)pdata->ssi_base + (((HI64XX_RAM2AXI_RD_DATA3 ) & (HI64XX_REG_VAL_MASK)) << 2));
	ret = readl((void*)pdata->ssi_base + (((HI64XX_RAM2AXI_RD_DATA3 ) & (HI64XX_REG_VAL_MASK)) << 2));
	readl((void*)pdata->ssi_base + (((HI64XX_RAM2AXI_RD_DATA2) & (HI64XX_REG_VAL_MASK)) << 2));
	ret = (ret<<8) + (0xFF & readl((void*)pdata->ssi_base + (((HI64XX_RAM2AXI_RD_DATA2) & (HI64XX_REG_VAL_MASK)) << 2)));
	readl((void*)pdata->ssi_base + (((HI64XX_RAM2AXI_RD_DATA1) & (HI64XX_REG_VAL_MASK)) << 2));
	ret = (ret<<8) + (0xFF & readl((void*)pdata->ssi_base + (((HI64XX_RAM2AXI_RD_DATA1) & (HI64XX_REG_VAL_MASK)) << 2)));
	readl((void*)pdata->ssi_base + (((HI64XX_RAM2AXI_RD_DATA0) & (HI64XX_REG_VAL_MASK)) << 2));
	ret = (ret<<8) + (0xFF & readl((void*)pdata->ssi_base + (((HI64XX_RAM2AXI_RD_DATA0) & (HI64XX_REG_VAL_MASK)) << 2)));

	if (pdata->pm_runtime_support) {
		pm_runtime_mark_last_busy(pdata->dev);
		pm_runtime_put_autosuspend(pdata->dev);
	}

	mutex_unlock(&pdata->sr_rw_lock);
	return ret ;
}
EXPORT_SYMBOL(ssi_reg_read32);

void ssi_reg_write8(unsigned int reg, unsigned int val)
{
	int pm_ret = 0;

	mutex_lock(&pdata->sr_rw_lock);

	if (pdata->pm_runtime_support) {
		pm_ret = pm_runtime_get_sync(pdata->dev);
		if (pm_ret < 0) {
			pr_err("[%s:%d] pm resume error, pm_ret:%d\n", __FUNCTION__, __LINE__, pm_ret);
			mutex_unlock(&pdata->sr_rw_lock);
			BUG_ON(true);
			return ;
		}
	}

	_ssi_select_reg_page(reg);

	writel(val & HI64XX_REG_VAL_MASK , (void*)pdata->ssi_base + ((reg & HI64XX_REG_VAL_MASK) << 2));

	if (pdata->pm_runtime_support) {
		pm_runtime_mark_last_busy(pdata->dev);
		pm_runtime_put_autosuspend(pdata->dev);
	}

	mutex_unlock(&pdata->sr_rw_lock);
}
EXPORT_SYMBOL(ssi_reg_write8);

void ssi_reg_write32(unsigned int reg, unsigned int val)
{
	int pm_ret = 0;

	mutex_lock(&pdata->sr_rw_lock);

	if (reg & 0x3) {
		pr_err("%s:reg is 0x%pK, it's not alignment!!\n", __FUNCTION__, (void *)(unsigned long)reg);
		mutex_unlock(&pdata->sr_rw_lock);
		return;
	}

	if (pdata->pm_runtime_support) {
		pm_ret = pm_runtime_get_sync(pdata->dev);
		if (pm_ret < 0) {
			pr_err("[%s:%d] pm resume error, pm_ret:%d\n", __FUNCTION__, __LINE__, pm_ret);
			mutex_unlock(&pdata->sr_rw_lock);
			BUG_ON(true);
			return ;
		}
	}

	_ssi_select_reg_page(reg);

	writel(val & HI64XX_REG_VAL_MASK, (void*)pdata->ssi_base + ((reg & HI64XX_REG_VAL_MASK) << 2));
	writel((val>>8) & HI64XX_REG_VAL_MASK, (void*)pdata->ssi_base + (((reg+1) & HI64XX_REG_VAL_MASK) << 2));
	writel((val>>16) & HI64XX_REG_VAL_MASK, (void*)pdata->ssi_base + (((reg+2) & HI64XX_REG_VAL_MASK) << 2));
	writel((val>>24) & HI64XX_REG_VAL_MASK, (void*)pdata->ssi_base + (((reg+3) & HI64XX_REG_VAL_MASK) << 2));

	if (pdata->pm_runtime_support) {
		pm_runtime_mark_last_busy(pdata->dev);
		pm_runtime_put_autosuspend(pdata->dev);
	}

	mutex_unlock(&pdata->sr_rw_lock);
}
EXPORT_SYMBOL(ssi_reg_write32);

static int hi_cdcssi_probe(struct platform_device *pdev)
{
	struct hi_cdcssi_priv *priv;
	struct device *dev = &pdev->dev;
	struct resource *resource;
	int ret = 0;

	priv = devm_kzalloc(dev, sizeof(struct hi_cdcssi_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(dev, "devm_kzalloc failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, priv);
	pdata = priv;
	pdata->dev = dev;

	priv->ssi_page_sel = 1;
	mutex_init(&priv->sr_rw_lock);
	/* get codec ssi clk */
	priv->codec_ssi_clk = devm_clk_get(dev, "clk_codecssi");
	if (IS_ERR(priv->codec_ssi_clk)) {
		dev_err(dev, "clk_get: codecssi clk not found!\n");
		ret = PTR_ERR(priv->codec_ssi_clk);
		goto exit;
	}

	ret = clk_prepare_enable(priv->codec_ssi_clk);
	if (0 != ret) {
		dev_err(dev, "codec_ssi_clk :clk prepare enable failed !\n");
		goto exit;
	}
	mdelay(1);

	/* get pinctrl */
	priv->pctrl = devm_pinctrl_get(dev);
	if (IS_ERR(priv->pctrl)) {
		dev_err(dev, "could not get pinctrl\n");
		ret = -EIO;
		goto err_exit;
	}

	priv->pin_default = pinctrl_lookup_state(priv->pctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(priv->pin_default)) {
		dev_err(dev, "could not get default state (%li)\n" , PTR_ERR(priv->pin_default));
		ret = -EIO;
		goto err_exit;
	}

	priv->pin_idle = pinctrl_lookup_state(priv->pctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR(priv->pin_idle)) {
		dev_err(dev, "could not get idle state (%li)\n", PTR_ERR(priv->pin_idle));
		ret = -EIO;
		goto err_exit;
	}

	if (pinctrl_select_state(priv->pctrl, priv->pin_default)) {
		dev_err(dev, "could not set pins to default state\n");
		ret = -EIO;
		goto err_exit;
	}

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!resource) {
		dev_err(dev, "get IORESOURCE_MEM failed\n");
		ret = -ENXIO;
		goto err_exit;
	}

	priv->ssi_base = ioremap(resource->start, resource_size(resource));
	if (!priv->ssi_base) {
		dev_err(dev, "remap base address %pK failed\n", (void*)resource->start);
		ret = -ENXIO;
		goto err_exit;
	}
	if (of_property_read_bool(dev->of_node, "pm_runtime_support"))
		priv->pm_runtime_support = true;

	pr_info("[%s:%d] pm_runtime_support:%d\n", __FUNCTION__, __LINE__, priv->pm_runtime_support);

	if (priv->pm_runtime_support) {
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_runtime_set_autosuspend_delay(&pdev->dev, 200); /* 200ms */
		pm_runtime_set_active(&pdev->dev);
		pm_runtime_enable(&pdev->dev);
	}

	return 0;

err_exit:
	clk_disable_unprepare(priv->codec_ssi_clk);
exit:
	mutex_destroy(&priv->sr_rw_lock);
	return ret;
}

static int hi_cdcssi_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct hi_cdcssi_priv *priv = platform_get_drvdata(pdev);
	struct device *dev;

	BUG_ON(NULL == priv);
	dev = &pdev->dev;

	if (priv->pm_runtime_support) {
		pm_runtime_resume(dev);
		pm_runtime_disable(dev);
	}

	iounmap(priv->ssi_base);
	pinctrl_put(priv->pctrl);
	clk_disable_unprepare(priv->codec_ssi_clk);
	if (priv->pm_runtime_support) {
		pm_runtime_set_suspended(dev);
	}

	mutex_destroy(&priv->sr_rw_lock);
	devm_kfree(dev, priv);

	return ret;
}

static int hi_cdcssi_suspend(struct device *device)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(device);
	struct hi_cdcssi_priv *priv = platform_get_drvdata(pdev);
	struct device *dev;
	int pm_ret = 0;

	BUG_ON(NULL == priv);
	dev = &pdev->dev;

	mutex_lock(&priv->sr_rw_lock);

	if (priv->pm_runtime_support) {
		pm_ret = pm_runtime_get_sync(device);
		if (pm_ret < 0) {
			pr_err("[%s:%d] pm resume error, pm_ret:%d\n", __FUNCTION__, __LINE__, pm_ret);
			mutex_unlock(&priv->sr_rw_lock);
			BUG_ON(true);
			return pm_ret;
		}
	}
	pr_info("[%s:%d] usage_count:0x%x status:0x%x disable_depth:%d clk:%d \n",__FUNCTION__, __LINE__,
		atomic_read(&(device->power.usage_count)), device->power.runtime_status, device->power.disable_depth, clk_get_enable_count(priv->codec_ssi_clk));

	ret = pinctrl_select_state(priv->pctrl, priv->pin_idle);
	if (ret) {
		mutex_unlock(&priv->sr_rw_lock);
		dev_err(dev, "could not set pins to idle state\n");
	}

	clk_disable_unprepare(priv->codec_ssi_clk);

	return ret;
}

static int hi_cdcssi_resume(struct device *device)
{
	int ret = 0;
	struct platform_device *pdev = to_platform_device(device);
	struct hi_cdcssi_priv *priv = platform_get_drvdata(pdev);
	struct device	*dev;

	BUG_ON(NULL == priv);
	dev = &pdev->dev;

	ret = clk_prepare_enable(priv->codec_ssi_clk);
	if (0 != ret) {
		dev_err(dev, "codec_ssi_clk :clk prepare enable failed !\n");
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
	if (ret) {
		dev_err(dev, "could not set pins to default state\n");
	}

	if (priv->pm_runtime_support) {
		pm_runtime_mark_last_busy(device);
		pm_runtime_put_autosuspend(device);

		pm_runtime_disable(device);
		pm_runtime_set_active(device);
		pm_runtime_enable(device);
	}

	pr_info("[%s:%d] usage_count:0x%x status:0x%x disable_depth:%d clk:%d \n",__FUNCTION__, __LINE__,
		atomic_read(&(device->power.usage_count)), device->power.runtime_status, device->power.disable_depth, clk_get_enable_count(priv->codec_ssi_clk));

	mutex_unlock(&priv->sr_rw_lock);

	return ret;
}

static int hi_cdcssi_runtime_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct hi_cdcssi_priv *priv = platform_get_drvdata(pdev);
	int ret = 0;

	ret = pinctrl_select_state(priv->pctrl, priv->pin_idle);
	if (ret) {
		mutex_unlock(&priv->sr_rw_lock);
		dev_err(device, "could not set pins to idle state\n");
	}

	clk_disable_unprepare(priv->codec_ssi_clk);

	return ret;
}

static int hi_cdcssi_runtime_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);
	struct hi_cdcssi_priv *priv = platform_get_drvdata(pdev);
	int ret = 0;

	ret = clk_prepare_enable(priv->codec_ssi_clk);
	if (0 != ret) {
		dev_err(device, "codec_ssi_clk :clk prepare enable failed !\n");
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
	if (ret) {
		dev_err(device, "could not set pins to default state\n");
	}

	mdelay(1);

	return ret;
}

static const struct dev_pm_ops cdcssi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(hi_cdcssi_suspend, hi_cdcssi_resume)
	SET_RUNTIME_PM_OPS(hi_cdcssi_runtime_suspend, hi_cdcssi_runtime_resume, NULL)
};

static const struct of_device_id codecssi_match[] = {
	{
		.compatible = "hisilicon,codecssi",
	},
	{},
};
MODULE_DEVICE_TABLE(of, codecssi_match);

static struct platform_driver codecssi_driver = {
	.probe	= hi_cdcssi_probe,
	.remove = hi_cdcssi_remove,
	.driver = {
		.name	= "hisilicon,codecssi",
		.owner	= THIS_MODULE,
		.pm = &cdcssi_pm_ops,
		.of_match_table = codecssi_match,
	},
};

static int __init hi_cdcssi_init(void)
{
	int ret;

	ret = platform_driver_register(&codecssi_driver);
	if (ret) {
		pr_err("driver register failed\n");
	}

	return ret;
}

static void __exit hi_cdcssi_exit(void)
{
	platform_driver_unregister(&codecssi_driver);
}
fs_initcall(hi_cdcssi_init);
module_exit(hi_cdcssi_exit);

MODULE_DESCRIPTION("hisi codecssi controller");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon");
