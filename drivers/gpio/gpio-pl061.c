/*
 * Copyright (C) 2008, 2009 Provigent Ltd.
 *
 * Author: Baruch Siach <baruch@tkos.co.il>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for the ARM PrimeCell(tm) General Purpose Input/Output (PL061)
 *
 * Data sheet: ARM DDI 0190B, September 2000
 */
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/bitops.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl061.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>
#include "hisi_gpio.h"
#include <linux/arm-smccc.h>

#define GPIODIR 0x400
#define GPIOIS  0x404
#define GPIOIBE 0x408
#define GPIOIEV 0x40C
#define GPIOIE  0x410
#define GPIORIS 0x414
#define GPIOMIS 0x418
#define GPIOIC  0x41C

#define PL061_GPIO_NR	8

struct hwspinlock       *gpio_hwlock;

#define HISI_SECURE_GPIO_REG_READ   0xc5010004
#define HISI_SECURE_GPIO_REG_WRITE  0xc5010005
unsigned char pl061_readb(struct pl061_gpio *chip, unsigned offset)
{
	struct amba_device *adev = chip->adev;
	unsigned char v;
	struct arm_smccc_res res;

	if (adev->secure_mode) {
		arm_smccc_1_1_smc(HISI_SECURE_GPIO_REG_READ, offset, &res);//lint !e1514
		v = (u8)res.a1;
	} else {
		v = readb(chip->base + offset);
	}

	return v;
}

void pl061_writeb(struct pl061_gpio *chip, unsigned char v, unsigned offset)
{
	struct amba_device *adev = chip->adev;
	struct arm_smccc_res res;

	if (adev->secure_mode) {
		arm_smccc_1_1_smc(HISI_SECURE_GPIO_REG_WRITE, v, offset, &res);//lint !e1514
	} else {
		writeb(v, chip->base + offset);
	}
	return;
}

static int pl061_get_direction(struct gpio_chip *gc, unsigned offset)
{
	struct pl061_gpio *chip = gpiochip_get_data(gc);

	return !(pl061_readb(chip, GPIODIR) & BIT(offset));
}

static int pl061_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct pl061_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;
	unsigned char gpiodir;

	if (offset >= gc->ngpio)
		return -EINVAL;

	if (pl061_check_security_status(chip))
		return -EBUSY;

	spin_lock_irqsave(&chip->lock, flags);

	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		pr_err("%s: hwspinlock timeout!\n", __func__);
		spin_unlock_irqrestore(&chip->lock, flags);
		return -EBUSY;
	}

	gpiodir = pl061_readb(chip,  GPIODIR);
	gpiodir &= ~(BIT(offset));
	pl061_writeb(chip, gpiodir, GPIODIR);

	hwspin_unlock(gpio_hwlock);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int pl061_direction_output(struct gpio_chip *gc, unsigned offset,
		int value)
{
	struct pl061_gpio *chip = gpiochip_get_data(gc);
	unsigned long flags;
	unsigned char gpiodir;

	if (offset >= gc->ngpio)
		return -EINVAL;

	if (pl061_check_security_status(chip))
		return -EBUSY;

	spin_lock_irqsave(&chip->lock, flags);

	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		pr_err("%s: hwspinlock timeout!\n", __func__);
		spin_unlock_irqrestore(&chip->lock, flags);
		return -EBUSY;
	}

	pl061_writeb(chip, !!value << offset, (BIT(offset + 2)));//lint !e514
	gpiodir = pl061_readb(chip, GPIODIR);
	gpiodir |= BIT(offset);
	pl061_writeb(chip, gpiodir, GPIODIR);

	/*
	 * gpio value is set again, because pl061 doesn't allow to set value of
	 * a gpio pin before configuring it in OUT mode.
	 */
	pl061_writeb(chip, !!value << offset, (BIT(offset + 2)));//lint !e514
	hwspin_unlock(gpio_hwlock);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static int pl061_get_value(struct gpio_chip *gc, unsigned offset)
{
	struct pl061_gpio *chip = gpiochip_get_data(gc);

	if (pl061_check_security_status(chip))
		return -EBUSY;

	return !!pl061_readb(chip, (BIT(offset + 2)));
}

static void pl061_set_value(struct gpio_chip *gc, unsigned offset, int value)
{
	struct pl061_gpio *chip = gpiochip_get_data(gc);

	if (pl061_check_security_status(chip))
		return;

	pl061_writeb(chip, !!value << offset, (BIT(offset + 2)));//lint !e514
}

static int pl061_irq_type(struct irq_data *d, unsigned trigger)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061_gpio *chip = gpiochip_get_data(gc);
	int offset = irqd_to_hwirq(d);
	unsigned long flags;
	u8 gpiois, gpioibe, gpioiev;
	u8 bit = BIT(offset);

	if (offset < 0 || offset >= PL061_GPIO_NR)
		return -EINVAL;

	if ((trigger & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW)) &&
	    (trigger & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING)))
	{
		dev_err(gc->parent,
			"trying to configure line %d for both level and edge "
			"detection, choose one!\n",
			offset);
		return -EINVAL;
	}

	if (pl061_check_security_status(chip))
		return -EBUSY;

	spin_lock_irqsave(&chip->lock, flags);

	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		pr_err("%s: hwspinlock timeout!\n", __func__);
		spin_unlock_irqrestore(&chip->lock, flags);
		return -EBUSY;
	}

	gpioiev = pl061_readb(chip, GPIOIEV);
	gpiois = pl061_readb(chip, GPIOIS);
	gpioibe = pl061_readb(chip, GPIOIBE);

	if (trigger & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW)) {
		bool polarity = trigger & IRQ_TYPE_LEVEL_HIGH;

		/* Disable edge detection */
		gpioibe &= ~bit;
		/* Enable level detection */
		gpiois |= bit;
		/* Select polarity */
		if (polarity)
			gpioiev |= bit;
		else
			gpioiev &= ~bit;
		irq_set_handler_locked(d, handle_level_irq);
		dev_dbg(gc->parent, "line %d: IRQ on %s level\n",
			offset,
			polarity ? "HIGH" : "LOW");
	} else if ((trigger & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH) {
		/* Disable level detection */
		gpiois &= ~bit;
		/* Select both edges, setting this makes GPIOEV be ignored */
		gpioibe |= bit;
		irq_set_handler_locked(d, handle_edge_irq);
		dev_dbg(gc->parent, "line %d: IRQ on both edges\n", offset);
	} else if ((trigger & IRQ_TYPE_EDGE_RISING) ||
		   (trigger & IRQ_TYPE_EDGE_FALLING)) {
		bool rising = trigger & IRQ_TYPE_EDGE_RISING;

		/* Disable level detection */
		gpiois &= ~bit;
		/* Clear detection on both edges */
		gpioibe &= ~bit;
		/* Select edge */
		if (rising)
			gpioiev |= bit;
		else
			gpioiev &= ~bit;
		irq_set_handler_locked(d, handle_edge_irq);
		dev_dbg(gc->parent, "line %d: IRQ on %s edge\n",
			offset,
			rising ? "RISING" : "FALLING");
	} else {
		/* No trigger: disable everything */
		gpiois &= ~bit;
		gpioibe &= ~bit;
		gpioiev &= ~bit;
		irq_set_handler_locked(d, handle_bad_irq);
		dev_warn(gc->parent, "no trigger selected for line %d\n",
			 offset);
	}

	pl061_writeb(chip, gpiois, GPIOIS);
	pl061_writeb(chip, gpioibe, GPIOIBE);
	pl061_writeb(chip, gpioiev, GPIOIEV);

	hwspin_unlock(gpio_hwlock);
	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

static void pl061_irq_handler(struct irq_desc *desc)
{
	unsigned long pending;
	int offset;
	struct gpio_chip *gc = irq_desc_get_handler_data(desc);
	struct pl061_gpio *chip = gpiochip_get_data(gc);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);

	if (pl061_check_security_status(chip))
		return;

	chained_irq_enter(irqchip, desc);

	pending = pl061_readb(chip, GPIOMIS);
	if (pending) {
		for_each_set_bit(offset, &pending, PL061_GPIO_NR)
			generic_handle_irq(irq_find_mapping(gc->irqdomain,
							    offset));
	}

	chained_irq_exit(irqchip, desc);
}

static void pl061_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061_gpio *chip = gpiochip_get_data(gc);
	u8 mask = BIT(irqd_to_hwirq(d) % PL061_GPIO_NR);
	u8 gpioie;
	unsigned long flags;

	if (pl061_check_security_status(chip))
		return;

	spin_lock_irqsave(&chip->lock, flags);

	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		pr_err("%s: hwspinlock timeout!\n", __func__);
		spin_unlock_irqrestore(&chip->lock, flags);
		return;
	}

	gpioie = pl061_readb(chip, GPIOIE) & ~mask;
	pl061_writeb(chip, gpioie, GPIOIE);

	hwspin_unlock(gpio_hwlock);
	spin_unlock_irqrestore(&chip->lock, flags);
}

static void pl061_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061_gpio *chip = gpiochip_get_data(gc);
	u8 mask = BIT(irqd_to_hwirq(d) % PL061_GPIO_NR);
	u8 gpioie;
	unsigned long flags;

	if (pl061_check_security_status(chip))
		return;

	spin_lock_irqsave(&chip->lock, flags);

	if (hwspin_lock_timeout(gpio_hwlock, LOCK_TIMEOUT)) {
		pr_err("%s: hwspinlock timeout!\n!", __func__);
		spin_unlock_irqrestore(&chip->lock, flags);
		return;
	}

	gpioie = pl061_readb(chip, GPIOIE) | mask;
	pl061_writeb(chip, gpioie, GPIOIE);
	hwspin_unlock(gpio_hwlock);
	spin_unlock_irqrestore(&chip->lock, flags);
}

/**
 * pl061_irq_ack() - ACK an edge IRQ
 * @d: IRQ data for this IRQ
 *
 * This gets called from the edge IRQ handler to ACK the edge IRQ
 * in the GPIOIC (interrupt-clear) register. For level IRQs this is
 * not needed: these go away when the level signal goes away.
 */
static void pl061_irq_ack(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct pl061_gpio *chip = gpiochip_get_data(gc);
	u8 mask = BIT(irqd_to_hwirq(d) % PL061_GPIO_NR);

	spin_lock(&chip->lock);
	pl061_writeb(chip, mask, GPIOIC);
	spin_unlock(&chip->lock);
}

static int pl061_irq_set_wake(struct irq_data *d, unsigned int on)
{
	return 0;
}

static struct irq_chip pl061_irqchip = {
	.name		= "pl061",
	.irq_ack	= pl061_irq_ack,
	.irq_mask	= pl061_irq_mask,
	.irq_unmask	= pl061_irq_unmask,
	.irq_disable	= pl061_irq_mask,
	.irq_enable	= pl061_irq_unmask,
	.irq_set_type	= pl061_irq_type,
	.irq_set_wake	= pl061_irq_set_wake,
};

static int pl061_probe(struct amba_device *adev, const struct amba_id *id)
{
	struct device *dev = &adev->dev;
	struct pl061_platform_data *pdata = dev_get_platdata(dev);
	struct pl061_gpio *chip;
	int ret, irq, i, irq_base;
	struct device_node *np = dev->of_node;

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (chip == NULL)
		return -ENOMEM;

	chip->adev = adev;

	if (pdata) {
		chip->gc.base = pdata->gpio_base;
		irq_base = pdata->irq_base;
		if (irq_base <= 0) {
			dev_err(&adev->dev, "invalid IRQ base in pdata\n");
			return -ENODEV;
		}
	} else {
		chip->gc.base = pl061_parse_gpio_base(dev);
		irq_base = 0;
	}

	chip->base = devm_ioremap_resource(dev, &adev->res);
	if (IS_ERR(chip->base))
		return PTR_ERR(chip->base);

	spin_lock_init(&chip->lock);
	if (of_get_property(np, "gpio,hwspinlock", NULL)) {
		gpio_hwlock = hwspin_lock_request_specific(GPIO_HWLOCK_ID);
		if (gpio_hwlock == NULL)
			return -EBUSY;
	}

	/* clear sec-flag of the controller */
	chip->sec_status = 0;
#ifdef CONFIG_HISI_TUI_PL061
	pl061_register_TUI_driver(np, dev);
#endif
	/* Hook the request()/free() for pinctrl operation */
	if (of_get_property(dev->of_node, "gpio-ranges", NULL)) {
		chip->uses_pinctrl = true;
		chip->gc.request = gpiochip_generic_request;
		chip->gc.free = gpiochip_generic_free;
	}

	chip->gc.get_direction = pl061_get_direction;
	chip->gc.direction_input = pl061_direction_input;
	chip->gc.direction_output = pl061_direction_output;
	chip->gc.get = pl061_get_value;
	chip->gc.set = pl061_set_value;
	chip->gc.ngpio = PL061_GPIO_NR;
	chip->gc.label = dev_name(dev);
	chip->gc.parent = dev;
	chip->gc.owner = THIS_MODULE;

	ret = gpiochip_add_data(&chip->gc, chip);
	if (ret)
		return ret;

	/*
	 * irq_chip support
	 */
	pl061_writeb(chip, 0xff, GPIOIC);
	pl061_writeb(chip, 0, GPIOIE); /* disable irqs */
	irq = adev->irq[0];
	if (irq < 0) {
		dev_err(&adev->dev, "invalid IRQ\n");
		return -ENODEV;
	}

	ret = gpiochip_irqchip_add(&chip->gc, &pl061_irqchip,
				   irq_base, handle_bad_irq,
				   IRQ_TYPE_NONE);
	if (ret) {
		dev_info(&adev->dev, "could not add irqchip\n");
		return ret;
	}
	gpiochip_set_chained_irqchip(&chip->gc, &pl061_irqchip,
				     irq, pl061_irq_handler);

	for (i = 0; i < PL061_GPIO_NR; i++) {
		if (pdata) {
			if (pdata->directions & (BIT(i)))
				pl061_direction_output(&chip->gc, i,
						pdata->values & (BIT(i)));
			else
				pl061_direction_input(&chip->gc, i);
		}
	}

	amba_set_drvdata(adev, chip);
	dev_info(&adev->dev, "PL061 GPIO chip @%pa registered\n",
		 &adev->res.start);

	return 0;
}

#ifdef CONFIG_GPIO_PM_SUPPORT
static int pl061_suspend(struct device *dev)
{
	struct pl061_gpio *chip = dev_get_drvdata(dev);
	int offset;

	chip->csave_regs.gpio_data = 0;
	chip->csave_regs.gpio_dir = readb(chip->base + GPIODIR);
	chip->csave_regs.gpio_is = readb(chip->base + GPIOIS);
	chip->csave_regs.gpio_ibe = readb(chip->base + GPIOIBE);
	chip->csave_regs.gpio_iev = readb(chip->base + GPIOIEV);
	chip->csave_regs.gpio_ie = readb(chip->base + GPIOIE);

	for (offset = 0; offset < PL061_GPIO_NR; offset++) {
		if (chip->csave_regs.gpio_dir & (BIT(offset)))
			chip->csave_regs.gpio_data |=
				pl061_get_value(&chip->gc, offset) << offset;
	}

	return 0;
}

static int pl061_resume(struct device *dev)
{
	struct pl061_gpio *chip = dev_get_drvdata(dev);
	int offset;

	for (offset = 0; offset < PL061_GPIO_NR; offset++) {
		if (chip->csave_regs.gpio_dir & (BIT(offset)))
			pl061_direction_output(&chip->gc, offset,
					chip->csave_regs.gpio_data &
					(BIT(offset)));
		else
			pl061_direction_input(&chip->gc, offset);
	}

	pl061_writeb(chip, chip->csave_regs.gpio_is, GPIOIS);
	pl061_writeb(chip, chip->csave_regs.gpio_ibe, GPIOIBE);
	pl061_writeb(chip, chip->csave_regs.gpio_iev, GPIOIEV);
	pl061_writeb(chip, chip->csave_regs.gpio_ie, GPIOIE);

	return 0;
}

static const struct dev_pm_ops pl061_dev_pm_ops = {
	.suspend = pl061_suspend,
	.resume = pl061_resume,
	.freeze = pl061_suspend,
	.restore = pl061_resume,
};
#endif

static struct amba_id pl061_ids[] = {
	{
		.id	= 0x00041061,
		.mask	= 0x000fffff,
	},
	{ 0, 0 },
};

static struct amba_driver pl061_gpio_driver = {
	.drv = {
		.name	= "pl061_gpio",
#ifdef CONFIG_GPIO_PM_SUPPORT
		.pm	= &pl061_dev_pm_ops,
#endif
	},
	.id_table	= pl061_ids,
	.probe		= pl061_probe,
};

static int __init pl061_gpio_init(void)
{
	return amba_driver_register(&pl061_gpio_driver);
}
subsys_initcall(pl061_gpio_init);
