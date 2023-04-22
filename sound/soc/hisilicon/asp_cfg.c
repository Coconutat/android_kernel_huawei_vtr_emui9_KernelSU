/*
 * asp_cfg.c -- asp dma driver
 *
 * Copyright (c) 2017 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/hwspinlock.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include "asp_cfg.h"

/*lint -e774 -e747 -e502 -e429*/

enum {
	USB_H2X,
	ASP_H2X,
};

struct asp_cfg_priv {
	struct device *dev;
	struct clk *asp_subsys_clk;
	struct regulator_bulk_data regu;
	int irq;
	spinlock_t lock;
	spinlock_t h2x_lock;
	struct resource *res;
	struct wake_lock wake_lock;
	void __iomem *asp_cfg_reg_base_addr;
	unsigned int asp_h2x_module_count;
	bool usb_h2x_enable;
	bool asp_h2x_enable;
};

static struct asp_cfg_priv *asp_cfg_priv = NULL;


static unsigned int asp_cfg_reg_read(unsigned int reg)
{
	struct asp_cfg_priv *priv = asp_cfg_priv;
	unsigned int ret = 0;
	unsigned long flag_sft = 0;

	BUG_ON(NULL == priv);

	spin_lock_irqsave(&priv->lock, flag_sft);

	ret = readl(priv->asp_cfg_reg_base_addr + reg);

	spin_unlock_irqrestore(&priv->lock, flag_sft);

	return ret;
}


static void asp_cfg_reg_write(unsigned int reg, unsigned int value)
{
	struct asp_cfg_priv *priv = asp_cfg_priv;
	unsigned long flag_sft = 0;

	BUG_ON(NULL == priv);

	spin_lock_irqsave(&priv->lock, flag_sft);

	writel(value, priv->asp_cfg_reg_base_addr + reg);

	spin_unlock_irqrestore(&priv->lock, flag_sft);

}

static void asp_cfg_reg_set_bit(unsigned int reg, unsigned int offset)
{
	struct asp_cfg_priv *priv = asp_cfg_priv;
	unsigned int value = 0;
	unsigned long flag_sft = 0;

	BUG_ON(NULL == priv);

	spin_lock_irqsave(&priv->lock, flag_sft);

	value = readl(priv->asp_cfg_reg_base_addr + reg);
	value |= (1 << offset);
	writel(value, priv->asp_cfg_reg_base_addr + reg);

	spin_unlock_irqrestore(&priv->lock, flag_sft);
}

static void asp_cfg_reg_clr_bit(unsigned int reg, unsigned int offset)
{
	struct asp_cfg_priv *priv = asp_cfg_priv;
	unsigned int value = 0;
	unsigned long flag_sft = 0;

	BUG_ON(NULL == priv);

	spin_lock_irqsave(&priv->lock, flag_sft);

	value = readl(priv->asp_cfg_reg_base_addr + reg);
	value &= ~(1 << offset);
	writel(value, priv->asp_cfg_reg_base_addr + reg);

	spin_unlock_irqrestore(&priv->lock, flag_sft);
}

void h2x_module_set(unsigned int module, bool enable)
{
	struct asp_cfg_priv *priv = asp_cfg_priv;
	unsigned long flag_sft = 0;

	BUG_ON(NULL == priv);

	spin_lock_irqsave(&priv->h2x_lock, flag_sft);
	pr_info("%s:module %d, enable %d \n", __FUNCTION__, module, enable);
	if (module == ASP_H2X)
		priv->asp_h2x_enable = enable;
	else
		priv->usb_h2x_enable = enable;

	if (priv->asp_h2x_enable || priv->usb_h2x_enable) {
		asp_cfg_reg_write(ASP_CFG_R_RST_CTRLDIS_REG, RST_ASP_H2X_BIT);
		asp_cfg_reg_write(ASP_CFG_R_GATE_EN_REG, CLK_HDMI_MCLK_BIT);
	} else {
		asp_cfg_reg_write(ASP_CFG_R_GATE_DIS_REG, CLK_HDMI_MCLK_BIT);
		asp_cfg_reg_write(ASP_CFG_R_RST_CTRLEN_REG, RST_ASP_H2X_BIT);
	}
	spin_unlock_irqrestore(&priv->h2x_lock, flag_sft);
}

void dp_h2x_on(void)
{
	h2x_module_set(ASP_H2X, true);
}

void dp_h2x_off(void)
{
	h2x_module_set(ASP_H2X, false);
}

int usb_h2x_on(void)
{
	int ret = 0;
	struct asp_cfg_priv *priv = asp_cfg_priv;
	struct clk *asp_subsys_clk = NULL;
	BUG_ON(NULL == priv);

	ret = regulator_bulk_enable(1, &asp_cfg_priv->regu);
	if (0 != ret) {
		pr_err("[%s:%d] couldn't enable regulators %d\n", __func__, __LINE__, ret);
		return -EFAULT;
	}

	asp_subsys_clk = asp_cfg_priv->asp_subsys_clk;
	if (asp_subsys_clk) {
		ret = clk_prepare_enable(asp_subsys_clk);
		if (ret) {
			pr_err("asp_subsys_clk enable fail, error=%d\n", ret);
			regulator_bulk_disable(1, &asp_cfg_priv->regu);
			return -EFAULT;
		}
	}

	h2x_module_set(USB_H2X, true);
	pr_info("%s exit \n",__FUNCTION__);
	return ret;
}

int usb_h2x_off(void)
{
	struct asp_cfg_priv *priv = asp_cfg_priv;
	struct clk *asp_subsys_clk = NULL;
	BUG_ON(NULL == priv);

	asp_subsys_clk = priv->asp_subsys_clk;
	h2x_module_set(USB_H2X, false);

	if (asp_subsys_clk)
		clk_disable_unprepare(asp_subsys_clk);

	regulator_bulk_disable(1, &asp_cfg_priv->regu);
	pr_info("%s exit \n",__FUNCTION__);
	return 0;
}

static void asp_cfg_h2x_module_enable(void)
{
	struct asp_cfg_priv *priv = asp_cfg_priv;

	BUG_ON(NULL == priv);

	if (0 == priv->asp_h2x_module_count)
		dp_h2x_on();

	 priv->asp_h2x_module_count++;
	 pr_info("[%s:%d],+asp_h2x_module_count = %d\n", __FUNCTION__, __LINE__, priv->asp_h2x_module_count);
}

static void asp_cfg_h2x_module_disable(void)
{
	struct asp_cfg_priv *priv = asp_cfg_priv;

	BUG_ON(NULL == priv);

	priv->asp_h2x_module_count--;
	pr_info("[%s:%d],-asp_h2x_module_count = %d\n", __FUNCTION__, __LINE__, priv->asp_h2x_module_count);
	if (0 == priv->asp_h2x_module_count)
		dp_h2x_off();
}

void asp_cfg_hdmi_clk_sel(unsigned int value)
{
	asp_cfg_reg_write(ASP_CFG_R_HDMI_CLK_SEL_REG, value);
}

void asp_cfg_div_clk(unsigned int value)
{
	asp_cfg_reg_write(ASP_CFG_R_DIV_SPDIF_CLKSEL_REG, value);
}

void asp_cfg_enable_hdmi_interrupeter(void)
{
	asp_cfg_reg_set_bit(ASP_CFG_R_INTR_NS_EN_REG, ASP_CFG_ASP_HDMI_INT_OFFSET);
}

void asp_cfg_disable_hdmi_interrupeter(void)
{
	asp_cfg_reg_clr_bit(ASP_CFG_R_INTR_NS_EN_REG, ASP_CFG_ASP_HDMI_INT_OFFSET);
}

void asp_cfg_hdmi_module_enable(void)
{
	/*reset */
	asp_cfg_h2x_module_enable();
	asp_cfg_reg_write(ASP_CFG_R_RST_CTRLDIS_REG, RST_ASP_HDMI_BIT);

	/*enable clk */
	asp_cfg_reg_write(ASP_CFG_R_GATE_EN_REG, CLK_HDMI_HCLK_BIT);
	asp_cfg_reg_write(ASP_CFG_R_GATE_EN_REG, CLK_AUDIO_PLL_BIT);

	/*enable hdmimclk_div & hdmirefclk_div*/
	asp_cfg_reg_set_bit(ASP_CFG_R_GATE_CLKDIV_EN_REG, ASP_CFG_GT_HDMIREF_DIV_OFFSET);
}

void asp_cfg_hdmi_module_disable(void)
{
	/*disable clk */
	asp_cfg_reg_write(ASP_CFG_R_GATE_DIS_REG, CLK_AUDIO_PLL_BIT);
	asp_cfg_reg_write(ASP_CFG_R_GATE_DIS_REG, CLK_HDMI_HCLK_BIT);

	/*disable hdmimclk_div & hdmirefclk_div*/
	asp_cfg_reg_clr_bit(ASP_CFG_R_GATE_CLKDIV_EN_REG, ASP_CFG_GT_HDMIREF_DIV_OFFSET);

	/*enable reset*/
	/* asp_cfg_reg_write(ASP_CFG_R_RST_CTRLEN_REG, RST_ASP_HDMI_BIT); */
	asp_cfg_h2x_module_disable();
}

void asp_cfg_dp_module_enable(void)
{
	/*enable clk */
	asp_cfg_reg_write(ASP_CFG_R_GATE_EN_REG, CLK_GT_SPDIF_BIT);
	asp_cfg_reg_write(ASP_CFG_R_GATE_EN_REG, CLK_SPDIF_HCLK_BIT);

	/*enable hdmirefclk_div*/
	asp_cfg_reg_set_bit(ASP_CFG_R_GATE_CLKDIV_EN_REG, ASP_CFG_GT_SPDIF_BCLK_DIV_OFFSET);
	pr_info("[%s:%d],asp_cfg_dp_module_enable", __FUNCTION__, __LINE__);
}

void asp_cfg_dp_module_disable(void)
{
	/*disable clk */
	asp_cfg_reg_write(ASP_CFG_R_GATE_DIS_REG, CLK_GT_SPDIF_BIT);
	asp_cfg_reg_write(ASP_CFG_R_GATE_DIS_REG, CLK_SPDIF_HCLK_BIT);

	/*disable hdmirefclk_div*/
	asp_cfg_reg_clr_bit(ASP_CFG_R_GATE_CLKDIV_EN_REG, ASP_CFG_GT_SPDIF_BCLK_DIV_OFFSET);
	pr_info("[%s:%d],asp_cfg_dp_module_disable", __FUNCTION__, __LINE__);
}

unsigned int asp_cfg_get_irq_value(void)
{
	return asp_cfg_reg_read(ASP_CFG_R_INTR_NS_INI_REG);
}

static int asp_cfg_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = NULL;
	struct asp_cfg_priv *priv = NULL;

	if (NULL == pdev) {
		pr_err("[%s:%d]  pdev is NULL!\n", __func__, __LINE__);
		return -ENOMEM;
	}

	dev = &pdev->dev;
	dev_info(dev, "probe begin.\n");

	priv = devm_kzalloc(dev, sizeof(struct asp_cfg_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(dev, "malloc failed.\n");
		return -ENOMEM;
	}

	priv->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!priv->res) {
		dev_err(dev, "get resource failed.\n");
		return -ENOENT;
	}

	priv->regu.supply = "asp-supply";
	ret = devm_regulator_bulk_get(dev, 1, &(priv->regu));
	if (0 != ret) {
		dev_err(dev, "couldn't get regulators %d\n", ret);
		return -EFAULT;
	}

	priv->asp_subsys_clk = devm_clk_get(dev, "clk_asp_subsys");
	if (IS_ERR_OR_NULL(priv->asp_subsys_clk)) {
		dev_err(dev, "devm_clk_get: clk_asp_subsys not found!\n");
		return -EFAULT;
	}

	priv->asp_cfg_reg_base_addr = devm_ioremap(dev, priv->res->start,
						resource_size(priv->res));
	if (!priv->asp_cfg_reg_base_addr) {
		dev_err(dev, "asp cfg reg addr ioremap failed.\n");
		return -ENOMEM;
	}

	dev_info(dev, "res->start.%pK\n", (void *)priv->res->start);
	dev_info(dev, "asp_cfg_reg_base_addr.%pK\n", (void *)priv->asp_cfg_reg_base_addr);

	wake_lock_init(&priv->wake_lock, WAKE_LOCK_SUSPEND, "asp_cfg");

	spin_lock_init(&priv->lock);

	spin_lock_init(&priv->h2x_lock);

	priv->dev = dev;

	platform_set_drvdata(pdev, priv);

	asp_cfg_priv = priv;

	dev_info(dev, "probe end.\n");

	return ret;
}

static int asp_cfg_remove(struct platform_device *pdev)
{
	struct asp_cfg_priv *priv =
		(struct asp_cfg_priv *)platform_get_drvdata(pdev);

	if (!priv)
		return 0;

	if (priv->asp_cfg_reg_base_addr)
		devm_iounmap(priv->dev, priv->asp_cfg_reg_base_addr);

	wake_lock_destroy(&priv->wake_lock);

	asp_cfg_priv = NULL;

	dev_info(priv->dev, "asp cfg driver remove succ.\n");

	return 0;
}

static const struct of_device_id of_asp_cfg_match[] = {
	{ .compatible = "hisilicon,asp-cfg", },
	{},
};

MODULE_DEVICE_TABLE(of, of_asp_cfg_match);

static struct platform_driver asp_cfg_driver = {
	.driver    = {
		.name      = "asp_cfg_drv",
		.owner     = THIS_MODULE,
		.of_match_table = of_asp_cfg_match,
	},
	.probe    = asp_cfg_probe,
	.remove   = asp_cfg_remove,
};

static int __init asp_cfg_init(void)
{
	return platform_driver_register(&asp_cfg_driver);
}
module_init(asp_cfg_init);

static void __exit asp_cfg_exit(void)
{
	platform_driver_unregister(&asp_cfg_driver);
}
module_exit(asp_cfg_exit);

MODULE_AUTHOR("wushengyang <wushengyang1@hisilicon.com>");
MODULE_DESCRIPTION("Hisilicon (R) ASP CFG Driver");
MODULE_LICENSE("GPL");
