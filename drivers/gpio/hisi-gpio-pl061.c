#include "hisi_gpio.h"
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/version.h>
#include <linux/hisi/hisi_log.h>

#define HISI_LOG_TAG HISI_GPIO_TAG

#ifdef CONFIG_HISI_TUI_PL061
#define GPIODIR 0x400
#define GPIOIS  0x404
#define GPIOIBE 0x408
#define GPIOIEV 0x40C
#define GPIOIE  0x410
#endif

/* Parse gpio base from DT */
int pl061_parse_gpio_base(struct device *dev)
{
	struct device_node *np = dev->of_node;
	int ret = -EINVAL;

	if (of_property_read_u32(np, "linux,gpio-base", (u32 *)&ret))
		return -ENOENT;
	if (ret >= 0)
		return ret;
	return -EINVAL;
}

int pl061_check_security_status(struct pl061_gpio *chip)
{
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0))
	WARN(chip->sec_status, "%s controller is busy", dev_name(chip->gc.parent));
#else
	WARN(chip->sec_status, "%s controller is busy", dev_name(chip->gc.dev));
#endif
	return chip->sec_status;
}

#ifdef CONFIG_HISI_TUI_PL061
#include "../hisi/tzdriver/tui.h"
int pl061_tui_request(struct device *dev)
{
	struct pl061_gpio *chip = dev_get_drvdata(dev);
	unsigned long flags;

	pr_debug("%s: is switching sec status\n", dev_name(dev));

	spin_lock_irqsave(&chip->lock, flags);

	chip->sec_status = 1;
	chip->csave_regs.gpio_dir = readb(chip->base + GPIODIR);
	chip->csave_regs.gpio_data = readb(chip->base + GPIODATA);
	chip->csave_regs.gpio_is = readb(chip->base + GPIOIS);
	chip->csave_regs.gpio_ibe = readb(chip->base + GPIOIBE);
	chip->csave_regs.gpio_iev = readb(chip->base + GPIOIEV);
	chip->csave_regs.gpio_ie = readb(chip->base + GPIOIE);
	writeb(0, chip->base + GPIOIE);

	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

int pl061_tui_release(struct device *dev)
{
	struct pl061_gpio *chip = dev_get_drvdata(dev);
	unsigned long flags;

	pr_debug("%s: is switching non-sec status\n", dev_name(dev));

	spin_lock_irqsave(&chip->lock, flags);

	writeb(chip->csave_regs.gpio_dir, chip->base + GPIODIR);
	writeb(chip->csave_regs.gpio_data, chip->base + GPIODATA);
	writeb(chip->csave_regs.gpio_is, chip->base + GPIOIS);
	writeb(chip->csave_regs.gpio_ibe, chip->base + GPIOIBE);
	writeb(chip->csave_regs.gpio_iev, chip->base + GPIOIEV);
	writeb(chip->csave_regs.gpio_ie, chip->base + GPIOIE);
	chip->sec_status = 0;

	spin_unlock_irqrestore(&chip->lock, flags);

	return 0;
}

int pl061_tui_switch_func(void *pdata, int secure)
{
	struct device *dev = pdata;
	int ret;

	if (secure)
		ret = pl061_tui_request(dev);
	else
		ret = pl061_tui_release(dev);

	return ret;
}

void pl061_register_TUI_driver(struct device_node *np, struct device *dev)
{
	int ret;

	if (of_get_property(np, "sec-controller", NULL)) {
		ret = register_tui_driver(pl061_tui_switch_func,
					  dev_name(dev), dev);
		if (ret)
			pr_err("%s: could not register switch\n",
			       dev_name(dev));
		else
			pr_debug("%s: supports sec property\n", dev_name(dev));
	}
}
#endif
