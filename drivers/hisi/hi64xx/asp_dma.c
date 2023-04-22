/*
 * asp_dma.c -- asp dma driver
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
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
#include <linux/hisi/hi64xx/asp_dma.h>
#include <linux/delay.h>

/*lint -e774 -e747 -e502*/

#define ASP_DMA_HWLOCK_ID (6)
#define HWLOCK_WAIT_TIME (50)

struct dma_callback{
    callback_t callback;
    unsigned long para;
};

struct asp_dma_priv {
	struct device *dev;
	int irq;
	spinlock_t lock;
	struct resource *res;
	struct hwspinlock *hwlock;
	struct wake_lock wake_lock;
	void __iomem *asp_dma_reg_base_addr;
	struct dma_callback callback[ASP_DMA_MAX_CHANNEL_NUM];
};

static struct asp_dma_priv *asp_dma_priv = NULL;

unsigned int _dmac_reg_read(unsigned int reg)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned long flag = 0;
	unsigned int ret = 0;

	BUG_ON(NULL == priv);

	if (hwspin_lock_timeout_irqsave(priv->hwlock, HWLOCK_WAIT_TIME, &flag)) {
		dev_err(priv->dev, "hwspinlock timeout\n");
		return 0;
	}

	ret = readl(priv->asp_dma_reg_base_addr + reg);

	hwspin_unlock_irqrestore(priv->hwlock, &flag);

	return ret;
}

static void _dmac_reg_write(unsigned int reg, unsigned int value)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned long flag = 0;

	BUG_ON(NULL == priv);

	if (hwspin_lock_timeout_irqsave(priv->hwlock, HWLOCK_WAIT_TIME, &flag)) {
		dev_err(priv->dev, "hwspinlock timeout\n");
		return;
	}

	writel(value, priv->asp_dma_reg_base_addr + reg);

	hwspin_unlock_irqrestore(priv->hwlock, &flag);
}

static void _dmac_reg_set_bit(unsigned int reg, unsigned int offset)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned int value = 0;
	unsigned long flag_hw = 0;
	unsigned long flag_sft = 0;

	BUG_ON(NULL == priv);

	if (hwspin_lock_timeout_irqsave(priv->hwlock, HWLOCK_WAIT_TIME, &flag_hw)) {
		dev_err(priv->dev, "hwspinlock timeout\n");
		return;
	}

	spin_lock_irqsave(&priv->lock, flag_sft);

	value = readl(priv->asp_dma_reg_base_addr + reg);
	value |= (1 << offset);
	writel(value, priv->asp_dma_reg_base_addr + reg);

	spin_unlock_irqrestore(&priv->lock, flag_sft);

	hwspin_unlock_irqrestore(priv->hwlock, &flag_hw);
}

static void _dmac_reg_clr_bit(unsigned int reg, unsigned int offset)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned int value = 0;
	unsigned long flag_hw = 0;
	unsigned long flag_sft = 0;

	BUG_ON(NULL == priv);

	if (hwspin_lock_timeout_irqsave(priv->hwlock, HWLOCK_WAIT_TIME, &flag_hw)) {
		dev_err(priv->dev, "hwspinlock timeout!\n");
		return;
	}

	spin_lock_irqsave(&priv->lock, flag_sft);

	value = readl(priv->asp_dma_reg_base_addr + reg);
	value &= ~(1UL << offset);
	writel(value, priv->asp_dma_reg_base_addr + reg);

	spin_unlock_irqrestore(&priv->lock, flag_sft);

	hwspin_unlock_irqrestore(priv->hwlock, &flag_hw);
}

static void _dmac_dump(unsigned int dma_channel)
{
	struct asp_dma_priv *priv = asp_dma_priv;

	BUG_ON(NULL == priv);

	if (dma_channel >= ASP_DMA_MAX_CHANNEL_NUM) {
		dev_err(priv->dev, "dma channel err:%d\n", dma_channel);
		return;
	}

	dev_dbg(priv->dev, "a count:0x%x\n", _dmac_reg_read(ASP_DMA_CX_CNT0(dma_channel)));
	dev_dbg(priv->dev, "src addr:0x%x\n", _dmac_reg_read(ASP_DMA_CX_SRC_ADDR(dma_channel)));
	dev_dbg(priv->dev, "des addr:0x%x\n", _dmac_reg_read(ASP_DMA_CX_DES_ADDR(dma_channel)));
	dev_dbg(priv->dev, "lli:0x%x\n", _dmac_reg_read(ASP_DMA_CX_LLI(dma_channel)));
	dev_dbg(priv->dev, "config:0x%x\n", _dmac_reg_read(ASP_DMA_CX_CONFIG(dma_channel)));

	dev_dbg(priv->dev, "c count:0x%x\n", _dmac_reg_read(ASP_DMA_CX_CNT1(dma_channel)));
	dev_dbg(priv->dev, "b index:0x%x\n", _dmac_reg_read(ASP_DMA_CX_BINDX(dma_channel)));
	dev_dbg(priv->dev, "c index:0x%x\n", _dmac_reg_read(ASP_DMA_CX_CINDX(dma_channel)));
}

static irqreturn_t _asp_dmac_irq_handler(int irq, void *data)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned int err1 = 0;
	unsigned int err2 = 0;
	unsigned int err3 = 0;
	unsigned int tc1 = 0;
	unsigned int tc2 = 0;
	unsigned int int_mask = 0;
	unsigned short int_state = 0;
	unsigned int int_type = 0;
	unsigned int i = 0;

	BUG_ON(NULL == priv);

	/* if have interupts */
	int_state = (unsigned short)_dmac_reg_read(ASP_DMA_INT_STAT_AP);

	if (0 == int_state)
		return IRQ_HANDLED;

	/* read interupt states */
	err1 = _dmac_reg_read(ASP_DMA_INT_ERR1_AP);
	err2 = _dmac_reg_read(ASP_DMA_INT_ERR2_AP);
	err3 = _dmac_reg_read(ASP_DMA_INT_ERR3_AP);
	tc1  = _dmac_reg_read(ASP_DMA_INT_TC1_AP);
	tc2  = _dmac_reg_read(ASP_DMA_INT_TC2_AP);

	/* clr interupt states */
	_dmac_reg_write(ASP_DMA_INT_TC1_RAW, int_state);
	_dmac_reg_write(ASP_DMA_INT_TC2_RAW, int_state);
	_dmac_reg_write(ASP_DMA_INT_ERR1_RAW, int_state);
	_dmac_reg_write(ASP_DMA_INT_ERR2_RAW, int_state);
	_dmac_reg_write(ASP_DMA_INT_ERR3_RAW, int_state);

	for (i = 0; i < ASP_DMA_MAX_CHANNEL_NUM; i++) {
		int_mask = 0x1L << i;

		if (int_state & int_mask) {
			if (priv->callback[i].callback) {
				if (err1 & int_mask)
					int_type = ASP_DMA_INT_TYPE_ERR1;
				else if (err2 & int_mask)
					int_type = ASP_DMA_INT_TYPE_ERR2;
				else if (err3 & int_mask)
					int_type = ASP_DMA_INT_TYPE_ERR3;
				else if (tc1 & int_mask)
					int_type = ASP_DMA_INT_TYPE_TC1;
				else if (tc2 & int_mask)
					int_type = ASP_DMA_INT_TYPE_TC2;
				else
					int_type = ASP_DMA_INT_TYPE_BUTT;

				priv->callback[i].callback(int_type, priv->callback[i].para, i);
			}
		}
	}

	return IRQ_HANDLED;
}

unsigned int asp_dma_get_des(unsigned short dma_channel)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned int dma_used_addr = 0;

	if (!priv) {
		pr_err("priv is null\n");
		return 0;
	}

	dma_used_addr = _dmac_reg_read(ASP_DMA_CX_DES_ADDR(dma_channel));

	return dma_used_addr;
}
EXPORT_SYMBOL(asp_dma_get_des);

unsigned int asp_dma_get_src(unsigned short dma_channel)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned int dma_used_addr = 0;

	if (!priv) {
		pr_err("priv is null\n");
		return 0;
	}

	dma_used_addr = _dmac_reg_read(ASP_DMA_CX_SRC_ADDR(dma_channel));

	return dma_used_addr;
}
EXPORT_SYMBOL(asp_dma_get_src);

/**
 * asp dma clk has the same life cycle as asp subsys clk.
 * ensure asp subsys clk enabled, when called this func.
 * asp subsys clk enabled in slimbus & codec controller.
 */
int asp_dma_config(
			unsigned short dma_channel,
			struct dma_lli_cfg *lli_cfg,
			callback_t callback,
			unsigned long para)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned int channel_mask = (0x1L << dma_channel);

	if (!priv) {
		pr_err("priv is null\n");
		return -EINVAL;
	}

	if (dma_channel >= ASP_DMA_MAX_CHANNEL_NUM) {
		dev_err(priv->dev, "dma channel err:%d\n", dma_channel);
		return -EINVAL;
	}

	if (!lli_cfg) {
		dev_err(priv->dev, "lli cfg is null.\n");
		return -EINVAL;
	}

	/* disable dma channel */
	_dmac_reg_clr_bit(ASP_DMA_CX_CONFIG(dma_channel), 0);

	_dmac_reg_write(ASP_DMA_CX_CNT0(dma_channel), lli_cfg->a_count);

	/* c count */
	_dmac_reg_write(ASP_DMA_CX_CNT1(dma_channel), 0);
	_dmac_reg_write(ASP_DMA_CX_BINDX(dma_channel), 0);
	_dmac_reg_write(ASP_DMA_CX_CINDX(dma_channel), 0);

	/* set dma src/des addr */
	_dmac_reg_write(ASP_DMA_CX_SRC_ADDR(dma_channel), lli_cfg->src_addr);
	_dmac_reg_write(ASP_DMA_CX_DES_ADDR(dma_channel), lli_cfg->des_addr);

	/* set dma lli config */
	_dmac_reg_write(ASP_DMA_CX_LLI(dma_channel), lli_cfg->lli );

	/* clr irq status of dma channel */
	_dmac_reg_write(ASP_DMA_INT_TC1_RAW, channel_mask);
	_dmac_reg_write(ASP_DMA_INT_TC2_RAW, channel_mask);
	_dmac_reg_write(ASP_DMA_INT_ERR1_RAW, channel_mask);
	_dmac_reg_write(ASP_DMA_INT_ERR2_RAW, channel_mask);
	_dmac_reg_write(ASP_DMA_INT_ERR3_RAW, channel_mask);

	if (NULL != callback) {
		priv->callback[dma_channel].callback = callback;
		priv->callback[dma_channel].para = para;

		/* release irq mask */
		_dmac_reg_set_bit(ASP_DMA_INT_ERR1_MASK_AP, dma_channel);
		_dmac_reg_set_bit(ASP_DMA_INT_ERR2_MASK_AP, dma_channel);
		_dmac_reg_set_bit(ASP_DMA_INT_ERR3_MASK_AP, dma_channel);
		_dmac_reg_set_bit(ASP_DMA_INT_TC1_MASK_AP, dma_channel);
		_dmac_reg_set_bit(ASP_DMA_INT_TC2_MASK_AP, dma_channel);
	}

	dev_info(priv->dev, "dma config succ.\n");

	return 0;
}
EXPORT_SYMBOL(asp_dma_config);

int asp_dma_start(
			unsigned short dma_channel,
			struct dma_lli_cfg *lli_cfg)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned int lli_reg;

	if (!priv) {
		pr_err("priv is null\n");
		return -EINVAL;
	}

	if (dma_channel >= ASP_DMA_MAX_CHANNEL_NUM) {
		dev_err(priv->dev, "dma channel err:%d\n", dma_channel);
		return -EINVAL;
	}

	if (!lli_cfg) {
		dev_err(priv->dev, "lli cfg is null.\n");
		return -EINVAL;
	}

	lli_reg = _dmac_reg_read(ASP_DMA_CX_LLI(dma_channel));
	if (lli_reg != lli_cfg->lli) {
		dev_err(priv->dev, "lli is changed, lli_reg: %d, lli_cfg: %d\n", lli_reg, lli_cfg->lli);
		return -EINVAL;
	}

	_dmac_reg_write(ASP_DMA_CX_CONFIG(dma_channel), lli_cfg->config);

	_dmac_dump(dma_channel);

	dev_info(priv->dev, "dma start succ.\n");

	return 0;
}
EXPORT_SYMBOL(asp_dma_start);

void asp_dma_stop(unsigned short dma_channel)
{
	struct asp_dma_priv *priv = asp_dma_priv;
	unsigned int channel_mask = (0x1U << dma_channel);
	unsigned int i = 40;

	if (!priv) {
		pr_err("priv is null\n");
		return;
	}

	if (dma_channel >= ASP_DMA_MAX_CHANNEL_NUM) {
		dev_err(priv->dev, "dma channel err:%d\n", dma_channel);
		return;
	}

	/* disable dma channel */
	_dmac_reg_clr_bit(ASP_DMA_CX_CONFIG(dma_channel), 0);

	do {
		if (0 == (_dmac_reg_read(ASP_DMA_CH_STAT) & channel_mask)) {
			break;
		}
		dev_info(priv->dev, "stopping dma_channel: %d\n", dma_channel);
		udelay(250);
	} while (--i);

	_dmac_reg_clr_bit(ASP_DMA_INT_ERR1_MASK_AP, dma_channel);
	_dmac_reg_clr_bit(ASP_DMA_INT_ERR2_MASK_AP, dma_channel);
	_dmac_reg_clr_bit(ASP_DMA_INT_ERR3_MASK_AP, dma_channel);
	_dmac_reg_clr_bit(ASP_DMA_INT_TC1_MASK_AP, dma_channel);
	_dmac_reg_clr_bit(ASP_DMA_INT_TC2_MASK_AP, dma_channel);

	memset(&priv->callback[dma_channel], 0, sizeof(struct dma_callback));

	if (!i) {
		dev_err(priv->dev, "dma_channel:%d stop fail, channel state: %d\n", dma_channel, (_dmac_reg_read(ASP_DMA_CH_STAT) & channel_mask));
		return;
	}
	dev_info(priv->dev, "dma_channel:%d stop succ\n", dma_channel);
}
EXPORT_SYMBOL(asp_dma_stop);
/*lint -e429*/
static int asp_dma_probe (struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct asp_dma_priv *priv = NULL;

	dev_info(dev, "probe begin.\n");

	priv = devm_kzalloc(dev, sizeof(struct asp_dma_priv), GFP_KERNEL);
	if (!priv) {
		dev_err(dev, "malloc failed.\n");
		return -ENOMEM;
	}

	priv->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!priv->res) {
		dev_err(dev,"get resource failed.\n");
		ret = -ENOENT;
		goto err_exit;
	}

/* fixme: need request mem region when soundtrigger
 * used this module to request dma
 */

	priv->asp_dma_reg_base_addr = devm_ioremap(dev, priv->res->start,
					resource_size(priv->res));
	if (!priv->asp_dma_reg_base_addr) {
		dev_err(dev,"asp dma reg addr ioremap failed.\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	priv->irq = platform_get_irq_byname(pdev, "asp_dma_irq");
	if (priv->irq < 0) {
		dev_err(dev,"get asp dma irq failed.\n");
		ret= -ENOENT;
		goto err_exit;
	}

	ret = request_irq(priv->irq, _asp_dmac_irq_handler,
			  IRQF_SHARED | IRQF_NO_SUSPEND, "asp_dma_irq", dev);
	if (ret) {
		dev_err(dev,"request asp dma irq failed.\n");
		ret = -ENOENT;
		goto err_exit;
	}

	priv->hwlock = hwspin_lock_request_specific(ASP_DMA_HWLOCK_ID);
	if (!priv->hwlock) {
		dev_err(dev,"request hw spinlok failed.\n");
		ret = -ENOENT;
		goto err_exit;
	}

	wake_lock_init(&priv->wake_lock, WAKE_LOCK_SUSPEND, "asp_dma");

	spin_lock_init(&priv->lock);

	priv->dev = dev;

	platform_set_drvdata(pdev, priv);

	asp_dma_priv = priv;

	dev_info(dev, "probe end.\n");

	return 0;

err_exit:
	if (priv->asp_dma_reg_base_addr)
		devm_iounmap(dev, priv->asp_dma_reg_base_addr);

	devm_kfree(dev, priv);

	dev_err(dev, "probe failed.\n");

	return ret;
}
/*lint +e429*/
static int asp_dma_remove (struct platform_device *pdev)
{
	struct asp_dma_priv *priv =
		(struct asp_dma_priv*)platform_get_drvdata(pdev);

	if (!priv)
		return 0;

	if (priv->asp_dma_reg_base_addr)
		devm_iounmap(priv->dev, priv->asp_dma_reg_base_addr);

	if (hwspin_lock_free(priv->hwlock)) {
		dev_err(priv->dev,"hwspinlock free failed.\n");
	}

	wake_lock_destroy(&priv->wake_lock);

	free_irq(priv->irq, priv);

	devm_kfree(priv->dev, priv);

	asp_dma_priv = NULL;

	return 0;
}

static const struct of_device_id of_asp_dma_match[] = {
	{ .compatible = "hisilicon,hi64xx-asp-dma", },
	{},
};

MODULE_DEVICE_TABLE(of, of_asp_dma_match);

static struct platform_driver asp_dma_driver = {
	.driver 	= {
		.name	= "hi64xx_asp_dma_drv",
		.owner	= THIS_MODULE,
		.of_match_table = of_asp_dma_match,
	},
	.probe		= asp_dma_probe,
	.remove 	= asp_dma_remove,
};

static int __init asp_dma_init(void)
{
	return platform_driver_register(&asp_dma_driver);
}
module_init(asp_dma_init);

static void __exit asp_dma_exit(void)
{
	platform_driver_unregister(&asp_dma_driver);
}
module_exit(asp_dma_exit);

MODULE_AUTHOR("LiuJinHong <liujinhong@hisilicon.com>");
MODULE_DESCRIPTION("Hisilicon (R) ASP DMA Driver");
MODULE_LICENSE("GPL");
