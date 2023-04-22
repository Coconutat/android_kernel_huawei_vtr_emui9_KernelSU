/*
 * Generic device tree based pinctrl driver for one register per pin
 * type pinmux controllers
 *
 * Copyright (C) 2012 Texas Instruments, Inc.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/interrupt.h>

#include <linux/irqchip/chained_irq.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>

#include <linux/platform_data/pinctrl-single.h>

#include "core.h"
#include "pinconf.h"
#include "pinctrl-sec.h"

#include <asm/compiler.h>

#define DRIVER_NAME			"pinctrl-sec"
#define PCS_MUX_PINS_NAME		"pinctrl-single,pins"
#define PCS_MUX_BITS_NAME		"pinctrl-single,bits"
#define PCS_REG_NAME_LEN		((sizeof(unsigned long) * 2) + 3)
#define PCS_OFF_DISABLED		~0U
/**
 * struct pinctrl_sec_pingroup - pingroups for a function
 * @np:		pingroup device node pointer
 * @name:	pingroup name
 * @gpins:	array of the pins in the group
 * @ngpins:	number of pins in the group
 * @node:	list node
 */
struct pinctrl_sec_pingroup {
	struct device_node *np;
	char *name;
	int *gpins;
	int ngpins;
	struct list_head node;
};

/**
 * struct pinctrl_sec_func_vals - mux function register offset and value pair
 * @reg:	register virtual address
 * @val:	register value
 */

struct pinctrl_sec_func_vals {
	//void __iomem *reg;
	unsigned val;
	unsigned mask;
};

/**
 * struct pinctrl_sec_conf_vals - pinconf parameter, pinconf register offset
 * and value, enable, disable, mask
 * @param:	config parameter
 * @val:	user input bits in the pinconf register
 * @enable:	enable bits in the pinconf register
 * @disable:	disable bits in the pinconf register
 * @mask:	mask bits in the register value
 */
struct pinctrl_sec_conf_vals {
	enum pin_config_param param;
	unsigned val;
	unsigned enable;
	unsigned disable;
	unsigned mask;
};

/**
 * struct pinctrl_sec_conf_type - pinconf property name, pinconf param pair
 * @name:	property name in DTS file
 * @param:	config parameter
 */
struct pinctrl_sec_conf_type {
	const char *name;
	enum pin_config_param param;
};

/**
 * struct pinctrl_sec_function - pinctrl function
 * @name:	pinctrl function name
 * @vals:	register and vals array
 * @nvals:	number of entries in vals array
 * @pgnames:	array of pingroup names the function uses
 * @npgnames:	number of pingroup names the function uses
 * @node:	list node
 */
struct pinctrl_sec_function {
	const char *name;
	struct pinctrl_sec_func_vals *vals;
	unsigned nvals;
	const char **pgnames;
	int npgnames;
	struct pinctrl_sec_conf_vals *conf;
	int nconfs;
	struct list_head node;
};

/**
 * struct pinctrl_sec_gpiofunc_range - pin ranges with same mux value of gpio function
 * @offset:	offset base of pins
 * @npins:	number pins with the same mux value of gpio function
 * @gpiofunc:	mux value of gpio function
 * @node:	list node
 */
struct pinctrl_sec_gpiofunc_range {
	unsigned offset;
	unsigned npins;
	unsigned gpiofunc;
	struct list_head node;
};

/**
 * struct pinctrl_sec_data - wrapper for data needed by pinctrl framework
 * @pa:		pindesc array
 * @cur:	index to current element
 *
 * REVISIT: We should be able to drop this eventually by adding
 * support for registering pins individually in the pinctrl
 * framework for those drivers that don't need a static array.
 */
struct pinctrl_sec_data {
	struct pinctrl_pin_desc *pa;
	int cur;
};

/**
 * struct pinctrl_sec_name - register name for a pin
 * @name:	name of the pinctrl register
 *
 * REVISIT: We may want to make names optional in the pinctrl
 * framework as some drivers may not care about pin names to
 * avoid kernel bloat. The pin names can be deciphered by user
 * space tools using debugfs based on the register address and
 * SoC packaging information.
 */
struct pinctrl_sec_name {
	char name[PCS_REG_NAME_LEN];
};

/**
 * struct pinctrl_sec_soc_data - SoC specific settings
 * @flags:	initial SoC specific PCS_FEAT_xxx values
 * @irq:	optional interrupt for the controller
 * @irq_enable_mask:	optional SoC specific interrupt enable mask
 * @irq_status_mask:	optional SoC specific interrupt status mask
 * @rearm:	optional SoC specific wake-up rearm function
 */
/*lint -save -e754 -specific(-e754)*/
struct pinctrl_sec_soc_data {
	unsigned flags;
//	int irq;
//	unsigned irq_enable_mask;/*lint !e754 */
//	unsigned irq_status_mask;/*lint !e754 */
//	void (*rearm)(void);/*lint !e754 */
};
/*lint -restore*/

/**
 * struct pinctrl_sec_device - pinctrl device instance
 * @res:	resources
 * @base:	virtual address of the controller
 * @size:	size of the ioremapped area
 * @dev:	device entry
 * @pctl:	pin controller device
 * @flags:	mask of PCS_FEAT_xxx values
 * @lock:	spinlock for register access
 * @mutex:	mutex protecting the lists
 * @width:	bits per mux register
 * @fmask:	function register mask
 * @fshift:	function register shift
 * @foff:	value to turn mux off
 * @fmax:	max number of functions in fmask
 * @bits_per_pin:number of bits per pin
 * @names:	array of register names for pins
 * @pins:	physical pins on the SoC
 * @pgtree:	pingroup index radix tree
 * @ftree:	function index radix tree
 * @pingroups:	list of pingroups
 * @functions:	list of functions
 * @gpiofuncs:	list of gpio functions
 * @irqs:	list of interrupt registers
 * @chip:	chip container for this instance
 * @domain:	IRQ domain for this instance
 * @ngroups:	number of pingroups
 * @nfuncs:	number of functions
 * @desc:	pin controller descriptor
 * @read:	register read function to use
 * @write:	register write function to use
 */
struct pinctrl_sec_device {
	struct resource *res;
	void __iomem *base;
	unsigned size;
	struct device *dev;
	struct pinctrl_dev *pctl;
	unsigned flags;
/*lint -e750 -esym(750,*)*/
#define PCS_QUIRK_SHARED_IRQ	(1 << 2)
#define PCS_FEAT_IRQ		(1 << 1)
/*lint -e750 +esym(750,*)*/
#define PCS_FEAT_PINCONF	(1 << 0)
	struct pinctrl_sec_soc_data socdata;
	raw_spinlock_t lock;
	struct mutex mutex;
	unsigned width;
	unsigned fmask;
	unsigned fshift;
	unsigned foff;
	unsigned fmax;
	bool bits_per_mux;
	unsigned bits_per_pin;
	struct pinctrl_sec_name *names;
	struct pinctrl_sec_data pins;
	struct radix_tree_root pgtree;
	struct radix_tree_root ftree;
	struct list_head pingroups;
	struct list_head functions;
	struct list_head gpiofuncs;
//	struct list_head irqs;
//	struct irq_chip chip;
//	struct irq_domain *domain;
	unsigned ngroups;
	unsigned nfuncs;
	struct pinctrl_desc desc;
//	unsigned (*read)(void __iomem *reg);
	void (*write)(int val, int pin_id, int mask);
};

/*lint -e750 -esym(750,*)*/
#define PCS_QUIRK_HAS_SHARED_IRQ	(pinctrl_sec->flags & PCS_QUIRK_SHARED_IRQ)
#define PCS_HAS_IRQ		(pinctrl_sec->flags & PCS_FEAT_IRQ)
/*lint -e750 +esym(750,*)*/
#define PCS_HAS_PINCONF		(pinctrl_sec->flags & PCS_FEAT_PINCONF)

static int pinctrl_sec_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin,
			   unsigned long *configs, unsigned num_configs);
static enum pin_config_param pinctrl_sec_bias[] = {
	PIN_CONFIG_BIAS_PULL_DOWN,
	PIN_CONFIG_BIAS_PULL_UP,
};

#define RTC_REGISTER_PINCTRL_ID             (0xc500ccc0)
noinline int atfd_hisi_service_pinctrl_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
     register u64 function_id asm("x0") = _function_id;
     register u64 arg0 asm("x1") = _arg0;
     register u64 arg1 asm("x2") = _arg1;
     register u64 arg2 asm("x3") = _arg2;
     asm volatile(
             __asmeq("%0", "x0")
             __asmeq("%1", "x1")
             __asmeq("%2", "x2")
             __asmeq("%3", "x3")
             "smc    #0\n"
         : "+r" (function_id)
         : "r" (arg0), "r" (arg1), "r" (arg2));
/*lint !e753 */
     return (int)function_id;
} /*lint !e715 */

/*lint -e749 -esym(749,*)*/
enum pinctrl_sec_ops{
	PINCTRL_SEC_READ = 0,
	PINCTRL_SEC_WRITE,
};
/*lint -e749 +esym(749,*)*/

static int pinctrl_sec_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct pinctrl_sec_device *pinctrl_sec;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);

	return (int)pinctrl_sec->ngroups;
}

static char *pinctrl_sec_get_group_name(struct pinctrl_dev *pctldev,
					unsigned gselector)
{
	struct pinctrl_sec_device *pinctrl_sec;
	struct pinctrl_sec_pingroup *group;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);
	group = radix_tree_lookup(&pinctrl_sec->pgtree, (unsigned long)gselector);
	if (!group) {
		dev_err(pinctrl_sec->dev, "%s could not find pingroup%i\n",
			__func__, gselector);
		return NULL;
	}
	return group->name;
}

static void pinctrl_sec_write(int value, int pin_id, int sec_mask)
{
	atfd_hisi_service_pinctrl_smc(RTC_REGISTER_PINCTRL_ID, value, pin_id, sec_mask); /*lint !e732 !e747 */
}

EXPORT_SYMBOL(pinctrl_sec_write);

static int pinctrl_sec_get_group_pins(struct pinctrl_dev *pctldev,
					unsigned gselector,
					const unsigned **pins,
					unsigned *npins)
{
	struct pinctrl_sec_device *pinctrl_sec;
	struct pinctrl_sec_pingroup *group;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);
	group = radix_tree_lookup(&pinctrl_sec->pgtree, (unsigned long)gselector);
	if (!group) {
		dev_err(pinctrl_sec->dev, "%s could not find pingroup%i\n",
			__func__, gselector);
		return -EINVAL;
	}

	*pins = (unsigned int *)group->gpins;
	*npins = (unsigned int)group->ngpins;

	return 0;
}

static void pinctrl_sec_dt_free_map(struct pinctrl_dev *pctldev,
				struct pinctrl_map *map, unsigned num_maps)
{
	struct pinctrl_sec_device *pinctrl_sec;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);
	devm_kfree(pinctrl_sec->dev, map);
} /*lint !e715 */

static int pinctrl_sec_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned *num_maps);

static const struct pinctrl_ops pinctrl_sec_pinctrl_ops = {
	.get_groups_count = pinctrl_sec_get_groups_count,
	.get_group_name = (const char *(*)(struct pinctrl_dev *, unsigned int)
					)pinctrl_sec_get_group_name,
	.get_group_pins = pinctrl_sec_get_group_pins,
	.dt_node_to_map = pinctrl_sec_dt_node_to_map,
	.dt_free_map = pinctrl_sec_dt_free_map,
};

static int pinctrl_sec_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct pinctrl_sec_device *pinctrl_sec;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);

	return (int)pinctrl_sec->nfuncs;
}

static const char *pinctrl_sec_get_function_name(struct pinctrl_dev *pctldev,
						unsigned fselector)
{
	struct pinctrl_sec_device *pinctrl_sec;
	struct pinctrl_sec_function *func;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);
	func = radix_tree_lookup(&pinctrl_sec->ftree, (unsigned long)fselector);
	if (!func) {
		dev_err(pinctrl_sec->dev, "%s could not find function%i\n",
			__func__, fselector);
		return NULL;
	}

	return func->name;
}

static int pinctrl_sec_get_function_groups(struct pinctrl_dev *pctldev,
					unsigned fselector,
					const char * const **groups,
					unsigned * const ngroups)
{
	struct pinctrl_sec_device *pinctrl_sec;
	struct pinctrl_sec_function *func;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);
	func = radix_tree_lookup(&pinctrl_sec->ftree, (unsigned long)fselector);
	if (!func) {
		dev_err(pinctrl_sec->dev, "%s could not find function%i\n",
			__func__, fselector);
		return -EINVAL;
	}
	*groups = func->pgnames;
	*ngroups = (unsigned int)func->npgnames;

	return 0;
}

static int pinctrl_sec_get_function(struct pinctrl_dev *pctldev, unsigned pin,
			    struct pinctrl_sec_function **func)
{
	struct pinctrl_sec_device *pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);
	struct pin_desc *pdesc = pin_desc_get(pctldev, pin);
	const struct pinctrl_setting_mux *setting;
	unsigned fselector;

	if(!pdesc){
		dev_err(pinctrl_sec->dev, "%s pdesc is NULL!\n",__func__);
		return -ENOTSUPP;
	}
	/* If pin is not described in DTS & enabled, mux_setting is NULL. */
	setting = pdesc->mux_setting;
	if (!setting)
		return -ENOTSUPP;
	fselector = setting->func;
	*func = radix_tree_lookup(&pinctrl_sec->ftree, (unsigned long)fselector);
	if (!(*func)) {
		dev_err(pinctrl_sec->dev, "%s could not find function%i\n",
			__func__, fselector);
		return -ENOTSUPP;
	}
	return 0;
}

static int pinctrl_sec_set_mux(struct pinctrl_dev *pctldev, unsigned fselector,
	unsigned group)
{
	struct pinctrl_sec_device *pinctrl_sec;
	struct pinctrl_sec_function *func;
	int i, pin_id;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);
	/* If function mask is null, needn't enable it. */
	if (!pinctrl_sec->fmask)
		return 0;
	func = radix_tree_lookup(&pinctrl_sec->ftree, (unsigned long)fselector);
	if (!func)
		return -EINVAL;

	pin_id = get_sec_pin_id(pinctrl_sec_get_group_name(pctldev, 0));

	dev_dbg(pinctrl_sec->dev, "enabling %s function%i\n",
		func->name, fselector); /*lint !e774 */

	for (i = 0; i < (int)func->nvals; i++) {
		struct pinctrl_sec_func_vals *vals;
		unsigned long flags;
		unsigned val, mask;

		vals = &func->vals[i];
		raw_spin_lock_irqsave(&pinctrl_sec->lock, flags); /*lint !e550 */

		if (pinctrl_sec->bits_per_mux)
			mask = vals->mask;
		else
			mask = pinctrl_sec->fmask;

		val = (vals->val & mask); /*lint !e838*/
		pinctrl_sec->write((int)val, pin_id, mask); /*lint !e713 */
		raw_spin_unlock_irqrestore(&pinctrl_sec->lock, flags); /*lint !e550 */
	}

	return 0;
} /*lint !e715 */

static int pinctrl_sec_request_gpio(struct pinctrl_dev *pctldev,
			    struct pinctrl_gpio_range *range, unsigned pin)
{
	return 0;
} /*lint !e715 */

static const struct pinmux_ops pinctrl_sec_pinmux_ops = {
	.get_functions_count = pinctrl_sec_get_functions_count,
	.get_function_name = pinctrl_sec_get_function_name,
	.get_function_groups = pinctrl_sec_get_function_groups,
	.set_mux = pinctrl_sec_set_mux,
	.gpio_request_enable = pinctrl_sec_request_gpio,
}; /*lint !e785 */

/* Clear BIAS value */
// cppcheck-suppress *
static void pinctrl_sec_pinconf_clear_bias(struct pinctrl_dev *pctldev, unsigned pin)
{
	unsigned long config;
	int i;
	for (i = 0; i < 2; i++) { /*lint !e846 */
		config = pinconf_to_config_packed(pinctrl_sec_bias[i], 0);
		pinctrl_sec_pinconf_set(pctldev, pin, &config, 1);
	}
}

/*
 * Check whether PIN_CONFIG_BIAS_DISABLE is valid.
 * It's depend on that PULL_DOWN & PULL_UP configs are all invalid.
 */

static int pinctrl_sec_pinconf_set(struct pinctrl_dev *pctldev,
				unsigned pin, unsigned long *configs,
				unsigned num_configs)
{
	struct pinctrl_sec_device *pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);
	struct pinctrl_sec_function *func;
	unsigned shift = 0, i, data = 0, sec_mask = 0;
	u16 arg;
	int j, pin_id, ret;
	char *name;

	ret = pinctrl_sec_get_function(pctldev, pin, &func);
	if (ret)
		return ret;

	name = pinctrl_sec_get_group_name(pctldev, 0);
	pin_id = get_sec_pin_id(name);
	for (j = 0; j < (int)num_configs; j++) {
		for (i = 0; (int)i < func->nconfs; i++) {
			if (pinconf_to_config_param(configs[j])
				!= func->conf[i].param)
				continue;

			arg = pinconf_to_config_argument(configs[j]);
			switch (func->conf[i].param) {
			/* 2 parameters */
			case PIN_CONFIG_INPUT_SCHMITT:
			case PIN_CONFIG_DRIVE_STRENGTH:
			case PIN_CONFIG_SLEW_RATE:
			case PIN_CONFIG_LOW_POWER_MODE:
				shift = ffs(func->conf[i].mask) - 1; /*lint !e713 !e732 */
				sec_mask = ~func->conf[i].mask;
				data = (arg << shift) & func->conf[i].mask; /*lint !e701 */
				break;
			/* 4 parameters */
			case PIN_CONFIG_BIAS_DISABLE:
				pinctrl_sec_pinconf_clear_bias(pctldev, pin);
				break;
			case PIN_CONFIG_BIAS_PULL_DOWN:
			case PIN_CONFIG_BIAS_PULL_UP:
				if (arg)
					pinctrl_sec_pinconf_clear_bias(pctldev, pin);
				/* fall through */
			case PIN_CONFIG_INPUT_SCHMITT_ENABLE: /*lint !e825 */
				sec_mask = ~func->conf[i].mask;
				if (arg)
					data = func->conf[i].enable;
				else
					data = func->conf[i].disable;
				break;
			default:
				return -ENOTSUPP;
			}
			pinctrl_sec->write((int)data, pin_id, sec_mask); /*lint !e713 */
			break;
		}
		if ((int)i >= func->nconfs)
			return -ENOTSUPP;
	} /* for each config */

	return 0;
}

static int pinctrl_sec_pinconf_group_set(struct pinctrl_dev *pctldev,
				unsigned group, unsigned long *configs,
				unsigned num_configs)
{
	const unsigned *pins;
	unsigned npins;
	int i, ret;

	ret = pinctrl_sec_get_group_pins(pctldev, group, &pins, &npins);
	if (ret)
		return ret;
	for (i = 0; i < (int)npins; i++) {
		if (pinctrl_sec_pinconf_set(pctldev, pins[i], configs, num_configs))
			return -ENOTSUPP;
	}
	return 0;
}

static void pinctrl_sec_pinconf_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned pin)
{
} /*lint !e715 */

static void pinctrl_sec_pinconf_group_dbg_show(struct pinctrl_dev *pctldev,
				struct seq_file *s, unsigned selector)
{
} /*lint !e715 */

static void pinctrl_sec_pinconf_config_dbg_show(struct pinctrl_dev *pctldev,
					struct seq_file *s,
					unsigned long config)
{
	pinconf_generic_dump_config(pctldev, s, config);
}

static const struct pinconf_ops pinctrl_sec_pinconf_ops = {
	.pin_config_set = pinctrl_sec_pinconf_set,
	.pin_config_group_set = pinctrl_sec_pinconf_group_set,
	.pin_config_dbg_show = pinctrl_sec_pinconf_dbg_show,
	.pin_config_group_dbg_show = pinctrl_sec_pinconf_group_dbg_show,
	.pin_config_config_dbg_show = pinctrl_sec_pinconf_config_dbg_show,
	.is_generic = true,
};

/**
 * pinctrl_sec_add_pin() - add a pin to the static per controller pin array
 * @pinctrl_sec: pinctrl_sec driver instance
 * @offset: register offset from base
 */
static int pinctrl_sec_add_pin(struct pinctrl_sec_device *pinctrl_sec, unsigned offset,
		unsigned pin_pos)
{
	struct pinctrl_pin_desc *pin;
	struct pinctrl_sec_name *pn;
	int i;

	i = pinctrl_sec->pins.cur;
	if (i >= (int)pinctrl_sec->desc.npins) {
		dev_err(pinctrl_sec->dev, "too many pins, max %i\n",
			pinctrl_sec->desc.npins);
		return -ENOMEM;
	}

	pin = &pinctrl_sec->pins.pa[i];
	pn = &pinctrl_sec->names[i];
	snprintf(pn->name, sizeof(pn->name), "%lx.%u",/*lint !e421*/
		(unsigned long)pinctrl_sec->res->start + offset, pin_pos);
	pin->name = pn->name;
	pin->number = (unsigned int)i;
	pinctrl_sec->pins.cur++;

	return i;
}

/**
 * pinctrl_sec_allocate_pin_table() - adds all the pins for the pinctrl driver
 * @pinctrl_sec: pinctrl_sec driver instance
 *
 * In case of errors, resources are freed in pinctrl_sec_free_resources.
 *
 * If your hardware needs holes in the address space, then just set
 * up multiple driver instances.
 */
static int pinctrl_sec_allocate_pin_table(struct pinctrl_sec_device *pinctrl_sec)
{
	int mux_bytes, nr_pins, i;
	int num_pins_in_register = 0;

	mux_bytes = pinctrl_sec->width / BITS_PER_BYTE; /*lint !e713 */

	if (pinctrl_sec->bits_per_mux) {
		pinctrl_sec->bits_per_pin = fls(pinctrl_sec->fmask); /*lint !e713 !e732 */
		if(pinctrl_sec->bits_per_pin == 0) {
			return -ENOMEM;
		}
		nr_pins = (pinctrl_sec->size * BITS_PER_BYTE) / pinctrl_sec->bits_per_pin; /*lint !e713 */
		num_pins_in_register = pinctrl_sec->width / pinctrl_sec->bits_per_pin; /*lint !e713 */
	} else {
		nr_pins = (int)pinctrl_sec->size / mux_bytes;
	}

	dev_dbg(pinctrl_sec->dev, "allocating %i pins\n", nr_pins); /*lint !e774 */
	pinctrl_sec->pins.pa = devm_kzalloc(pinctrl_sec->dev,
				sizeof(*pinctrl_sec->pins.pa) * nr_pins, /*lint !e737 */
				GFP_KERNEL);
	if (!pinctrl_sec->pins.pa)
		return -ENOMEM;

	pinctrl_sec->names = devm_kzalloc(pinctrl_sec->dev,
				sizeof(struct pinctrl_sec_name) * nr_pins, /*lint !e737 */
				GFP_KERNEL);
	if (!pinctrl_sec->names)
		return -ENOMEM;

	pinctrl_sec->desc.pins = pinctrl_sec->pins.pa;
	pinctrl_sec->desc.npins = (unsigned int)nr_pins;

	for (i = 0; i < (int)pinctrl_sec->desc.npins; i++) {
		unsigned offset;
		int res;
		int pin_pos = 0;

		if (pinctrl_sec->bits_per_mux && num_pins_in_register != 0) {
			int byte_num;
			byte_num = (pinctrl_sec->bits_per_pin * i) / BITS_PER_BYTE; /*lint !e737 !e713 */
			offset = (unsigned int)((byte_num / mux_bytes) * mux_bytes);
			pin_pos = i % num_pins_in_register;
		} else {
			offset = (unsigned)(i * mux_bytes);
		}
		res = pinctrl_sec_add_pin(pinctrl_sec, offset, (unsigned int)pin_pos);
		if (res < 0) {
			dev_err(pinctrl_sec->dev, "error adding pins: %i\n", res);
			return res;
		}
	}

	return 0;
}

/**
 * pinctrl_sec_add_function() - adds a new function to the function list
 * @pinctrl_sec: pinctrl_sec driver instance
 * @np: device node of the mux entry
 * @name: name of the function
 * @vals: array of mux register value pairs used by the function
 * @nvals: number of mux register value pairs
 * @pgnames: array of pingroup names for the function
 * @npgnames: number of pingroup names
 */
static struct pinctrl_sec_function *pinctrl_sec_add_function(struct pinctrl_sec_device *pinctrl_sec,
					struct device_node *np,
					const char *name,
					struct pinctrl_sec_func_vals *vals,
					unsigned nvals,
					const char **pgnames,
					unsigned npgnames)
{
	struct pinctrl_sec_function *function;

	function = devm_kzalloc(pinctrl_sec->dev, sizeof(*function), GFP_KERNEL);
	if (!function)
		return NULL;

	function->name = name;
	function->vals = vals;
	function->nvals = nvals;
	function->pgnames = pgnames;
	function->npgnames = (int)npgnames;

	mutex_lock(&pinctrl_sec->mutex);
	list_add_tail(&function->node, &pinctrl_sec->functions);
	radix_tree_insert(&pinctrl_sec->ftree, (unsigned long)pinctrl_sec->nfuncs, function);
	pinctrl_sec->nfuncs++;
	mutex_unlock(&pinctrl_sec->mutex);

	return function;
}/*lint !e715 */

static void pinctrl_sec_remove_function(struct pinctrl_sec_device *pinctrl_sec,
				struct pinctrl_sec_function *function)
{
	int i;

	mutex_lock(&pinctrl_sec->mutex);
	for (i = 0; i < (int)pinctrl_sec->nfuncs; i++) {
		struct pinctrl_sec_function *found;

		found = radix_tree_lookup(&pinctrl_sec->ftree, i); /*lint !e732 !e747 */
		if (found == function)
			radix_tree_delete(&pinctrl_sec->ftree, i); /*lint !e732 !e747 */
	}
	list_del(&function->node);
	mutex_unlock(&pinctrl_sec->mutex);
}

/**
 * pinctrl_sec_add_pingroup() - add a pingroup to the pingroup list
 * @pinctrl_sec: pinctrl_sec driver instance
 * @np: device node of the mux entry
 * @name: name of the pingroup
 * @gpins: array of the pins that belong to the group
 * @ngpins: number of pins in the group
 */
static int pinctrl_sec_add_pingroup(struct pinctrl_sec_device *pinctrl_sec,
					struct device_node *np,
					char *name,
					int *gpins,
					int ngpins)
{
	struct pinctrl_sec_pingroup *pingroup;

	pingroup = devm_kzalloc(pinctrl_sec->dev, sizeof(*pingroup), GFP_KERNEL);
	if (!pingroup)
		return -ENOMEM;

	pingroup->name = name;
	pingroup->np = np;
	pingroup->gpins = gpins;
	pingroup->ngpins = ngpins;

	mutex_lock(&pinctrl_sec->mutex);
	list_add_tail(&pingroup->node, &pinctrl_sec->pingroups);
	radix_tree_insert(&pinctrl_sec->pgtree, (unsigned long)pinctrl_sec->ngroups, pingroup);
	pinctrl_sec->ngroups++;
	mutex_unlock(&pinctrl_sec->mutex);

	return 0;/*lint !e429 */
}

/**
 * pinctrl_sec_get_pin_by_offset() - get a pin index based on the register offset
 * @pinctrl_sec: pinctrl_sec driver instance
 * @offset: register offset from the base
 *
 * Note that this is OK as long as the pins are in a static array.
 */
static int pinctrl_sec_get_pin_by_offset(struct pinctrl_sec_device *pinctrl_sec, unsigned offset)
{
	unsigned index;

	if (offset >= pinctrl_sec->size) {
		dev_err(pinctrl_sec->dev, "mux offset out of range: 0x%x (0x%x)\n",
			offset, pinctrl_sec->size);
		return -EINVAL;
	}

	if (pinctrl_sec->bits_per_mux)
		index = (offset * BITS_PER_BYTE) / pinctrl_sec->bits_per_pin;
	else
		index = offset / (pinctrl_sec->width / BITS_PER_BYTE);

	return (int)index;
}

/*
 * check whether data matches enable bits or disable bits
 * Return value: 1 for matching enable bits, 0 for matching disable bits,
 *               and negative value for matching failure.
 */
static int pinctrl_sec_config_match(unsigned data, unsigned enable, unsigned disable)
{
	int ret = -EINVAL;

	if (data == enable)
		ret = 1;
	else if (data == disable)
		ret = 0;
	return ret;
}

static void add_config(struct pinctrl_sec_conf_vals **conf, enum pin_config_param param,
		       unsigned value, unsigned enable, unsigned disable,
		       unsigned mask)
{
	(*conf)->param = param;
	(*conf)->val = value;
	(*conf)->enable = enable;
	(*conf)->disable = disable;
	(*conf)->mask = mask;
	(*conf)++;
}

static void add_setting(unsigned long **setting, enum pin_config_param param,
			unsigned arg)
{
	**setting = pinconf_to_config_packed(param, (u16)arg);
	(*setting)++;
}

/* add pinconf setting with 2 parameters */
static void pinctrl_sec_add_conf2(struct pinctrl_sec_device *pinctrl_sec, struct device_node *np,
			  const char *name, enum pin_config_param param,
			  struct pinctrl_sec_conf_vals **conf, unsigned long **settings)
{
	unsigned value[2], shift;
	int ret;

	ret = of_property_read_u32_array(np, name, value, (unsigned long)2);
	if (ret)
		return;
	/* set value & mask */
	value[0] &= value[1];
	shift = (unsigned int)(ffs((int)value[1]) - 1);
	/* skip enable & disable */
	add_config(conf, param, value[0], 0, 0, value[1]);
	add_setting(settings, param, value[0] >> shift);
}/*lint !e715 */

/* add pinconf setting with 4 parameters */
static void pinctrl_sec_add_conf4(struct pinctrl_sec_device *pinctrl_sec, struct device_node *np,
			  const char *name, enum pin_config_param param,
			  struct pinctrl_sec_conf_vals **conf, unsigned long **settings)
{
	unsigned value[4];
	int ret;

	/* value to set, enable, disable, mask */
	ret = of_property_read_u32_array(np, name, value, (unsigned long)4);
	if (ret)
		return;
	if (!value[3]) {
		dev_err(pinctrl_sec->dev, "mask field of the property can't be 0\n");
		return;
	}
	value[0] &= value[3];
	value[1] &= value[3];
	value[2] &= value[3];
	ret = pinctrl_sec_config_match(value[0], value[1], value[2]);
	add_config(conf, param, value[0], value[1], value[2], value[3]);
	add_setting(settings, param, (unsigned int)ret);
}

static int pinctrl_sec_parse_pinconf(struct pinctrl_sec_device *pinctrl_sec, struct device_node *np,
			     struct pinctrl_sec_function *func,
			     struct pinctrl_map **map)

{
	struct pinctrl_map *m = *map;
	int i, nconfs = 0;
	unsigned long *settings, *s;
	struct pinctrl_sec_conf_vals *conf;
	struct pinctrl_sec_conf_type prop2[] = {
		{ "pinctrl-single,drive-strength", PIN_CONFIG_DRIVE_STRENGTH, },
		{ "pinctrl-single,slew-rate", PIN_CONFIG_SLEW_RATE, },
		{ "pinctrl-single,input-schmitt", PIN_CONFIG_INPUT_SCHMITT, },
		{ "pinctrl-single,low-power-mode", PIN_CONFIG_LOW_POWER_MODE, },
	};
	struct pinctrl_sec_conf_type prop4[] = {
		{ "pinctrl-single,bias-pullup", PIN_CONFIG_BIAS_PULL_UP, },
		{ "pinctrl-single,bias-pulldown", PIN_CONFIG_BIAS_PULL_DOWN, },
		{ "pinctrl-single,input-schmitt-enable",
			PIN_CONFIG_INPUT_SCHMITT_ENABLE, },
	};

	/* If pinconf isn't supported, don't parse properties in below. */
	if (!PCS_HAS_PINCONF)
		return 0;

	/* cacluate how much properties are supported in current node */
	for (i = 0; i < 4; i++) {
		if (of_find_property(np, prop2[i].name, NULL))
			nconfs++;
	}
	for (i = 0; i < 3; i++) {
		if (of_find_property(np, prop4[i].name, NULL))
			nconfs++;
	}
	if (!nconfs)
		return 0;

	func->conf = devm_kzalloc(pinctrl_sec->dev,
				  sizeof(struct pinctrl_sec_conf_vals) * nconfs,/*lint !e737 */
				  GFP_KERNEL);
	if (!func->conf)
		return -ENOMEM;
	func->nconfs = nconfs;
	conf = &(func->conf[0]);
	m++;
	settings = (unsigned long*)devm_kzalloc(pinctrl_sec->dev, sizeof(unsigned long) * nconfs, /*lint !e737 */
				GFP_KERNEL);
	if (!settings)
		return -ENOMEM;
	s = &settings[0];

	for (i = 0; i < 4; i++)
		pinctrl_sec_add_conf2(pinctrl_sec, np, prop2[i].name, prop2[i].param,
			      &conf, &s);
	for (i = 0; i < 3; i++)
		pinctrl_sec_add_conf4(pinctrl_sec, np, prop4[i].name, prop4[i].param,
			      &conf, &s);
	m->type = PIN_MAP_TYPE_CONFIGS_GROUP;
	m->data.configs.group_or_pin = np->name;
	m->data.configs.configs = settings;
	m->data.configs.num_configs = (unsigned int)nconfs;
	return 0;
}

static void pinctrl_sec_free_pingroups(struct pinctrl_sec_device *pinctrl_sec);

/**
 * smux_parse_one_pinctrl_entry() - parses a device tree mux entry
 * @pinctrl_sec: pinctrl driver instance
 * @np: device node of the mux entry
 * @map: map entry
 * @num_maps: number of map
 * @pgnames: pingroup names
 *
 * Note that this binding currently supports only sets of one register + value.
 *
 * Also note that this driver tries to avoid understanding pin and function
 * names because of the extra bloat they would cause especially in the case of
 * a large number of pins. This driver just sets what is specified for the board
 * in the .dts file. Further user space debugging tools can be developed to
 * decipher the pin and function names using debugfs.
 *
 * If you are concerned about the boot time, set up the static pins in
 * the bootloader, and only set up selected pins as device tree entries.
 */
static int pinctrl_sec_parse_one_pinctrl_entry(struct pinctrl_sec_device *pinctrl_sec,
						struct device_node *np,
						struct pinctrl_map **map,
						unsigned *num_maps,
						const char **pgnames)
{
	struct pinctrl_sec_func_vals *vals;
	const __be32 *mux;
	int size, rows, *pins, index = 0, found = 0, res = -ENOMEM;
	struct pinctrl_sec_function *function;

	mux = of_get_property(np, PCS_MUX_PINS_NAME, &size);
	if ((!mux) || (size < (int)sizeof(*mux) * 2)) {
		dev_err(pinctrl_sec->dev, "bad data for mux %s\n",
			np->name);
		return -EINVAL;
	}

	size /= (int)sizeof(*mux);	/* Number of elements in array */
	rows = size / 2;

	vals = devm_kzalloc(pinctrl_sec->dev, sizeof(*vals) * rows, GFP_KERNEL);/*lint !e737*/
	if (!vals)
		return -ENOMEM;

	pins = devm_kzalloc(pinctrl_sec->dev, sizeof(*pins) * rows, GFP_KERNEL);/*lint !e737*/
	if (!pins)
		goto free_vals;

	while (index < size) {
		unsigned offset, val;
		int pin;

		offset = be32_to_cpup(mux + index++);
		val = be32_to_cpup(mux + index++);
//		vals[found].reg = pinctrl_sec->base + (void __iomem*)offset;
		vals[found].val = val;

		pin = pinctrl_sec_get_pin_by_offset(pinctrl_sec, offset);
		if (pin < 0) {
			dev_err(pinctrl_sec->dev,
				"could not add functions for %s %ux\n",
				np->name, offset);
			break;
		}
		pins[found++] = pin;
	}

	pgnames[0] = np->name;
	function = pinctrl_sec_add_function(pinctrl_sec, np, np->name, vals, (unsigned int)found, pgnames, 1);
	if (!function)
		goto free_pins;

	res = pinctrl_sec_add_pingroup(pinctrl_sec, np, (char*)np->name, pins, found);
	if (res < 0)
		goto free_function;

	(*map)->type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)->data.mux.group = np->name;
	(*map)->data.mux.function = np->name;

	if (PCS_HAS_PINCONF) {
		res = pinctrl_sec_parse_pinconf(pinctrl_sec, np, function, map);
		if (res)
			goto free_pingroups;
		*num_maps = 2;
	} else {
		*num_maps = 1;
	}
	return 0;

free_pingroups:
	pinctrl_sec_free_pingroups(pinctrl_sec);
	*num_maps = 1;
free_function:
	pinctrl_sec_remove_function(pinctrl_sec, function);

free_pins:
	devm_kfree(pinctrl_sec->dev, pins);

free_vals:
	devm_kfree(pinctrl_sec->dev, vals);

	return res;/*lint !e593*/
}

#define PARAMS_FOR_BITS_PER_MUX 3

static int pinctrl_sec_parse_bits_in_pinctrl_entry(struct pinctrl_sec_device *pinctrl_sec,
						struct device_node *np,
						struct pinctrl_map **map,
						unsigned *num_maps,
						const char **pgnames)
{
	struct pinctrl_sec_func_vals *vals;
	const __be32 *mux;
	int size, rows, *pins, index = 0, found = 0, res = -ENOMEM;
	int npins_in_row;
	struct pinctrl_sec_function *function;

	mux = of_get_property(np, PCS_MUX_BITS_NAME, &size);

	if (!mux) {
		dev_err(pinctrl_sec->dev, "no valid property for %s\n", np->name);
		return -EINVAL;
	}

	if (size < (int)(sizeof(*mux) * PARAMS_FOR_BITS_PER_MUX)) {
		dev_err(pinctrl_sec->dev, "bad data for %s\n", np->name);
		return -EINVAL;
	}

	/* Number of elements in array */
	size /= (int)sizeof(*mux);

	rows = size / PARAMS_FOR_BITS_PER_MUX;
	npins_in_row = (int)(pinctrl_sec->width / pinctrl_sec->bits_per_pin);

	vals = devm_kzalloc(pinctrl_sec->dev, sizeof(*vals) * rows * npins_in_row,/*lint !e737 */
			GFP_KERNEL);
	if (!vals)
		return -ENOMEM;

	pins = devm_kzalloc(pinctrl_sec->dev, sizeof(*pins) * rows * npins_in_row,/*lint !e737 */
			GFP_KERNEL);
	if (!pins)
		goto free_vals;

	while (index < size) {
		unsigned offset, val;
		unsigned mask, mask_pos;
		int pin;

		offset = be32_to_cpup(mux + index++);
		val = be32_to_cpup(mux + index++);
		mask = be32_to_cpup(mux + index++);

		/* Parse pins in each row from LSB */
		while (mask) {
			unsigned bit_pos, val_pos, submask, pin_num_from_lsb; 
			bit_pos = (unsigned int)ffs((int)mask);
			pin_num_from_lsb = bit_pos / pinctrl_sec->bits_per_pin;
			mask_pos = ((pinctrl_sec->fmask) << (bit_pos - 1));
			val_pos = val & mask_pos;
			submask = mask & mask_pos;

			if ((mask & mask_pos) == 0) {
				dev_err(pinctrl_sec->dev,
					"Invalid mask for %s at 0x%x\n",
					np->name, offset);
				break;
			}

			mask &= ~mask_pos;

			if (submask != mask_pos) {
				dev_warn(pinctrl_sec->dev,
						"Invalid submask 0x%x for %s at 0x%x\n",
						submask, np->name, offset);
				continue;
			}

			vals[found].mask = submask;
//			vals[found].reg = (void __iomem*)(pinctrl_sec->base + offset);
			vals[found].val = val_pos;

			pin = pinctrl_sec_get_pin_by_offset(pinctrl_sec, offset);
			if (pin < 0) {
				dev_err(pinctrl_sec->dev,
					"could not add functions for %s %ux\n",
					np->name, offset);
				break;
			}
			pins[found++] = pin + (int)pin_num_from_lsb;
		}
	}

	pgnames[0] = np->name;
	function = pinctrl_sec_add_function(pinctrl_sec, np, np->name, vals, (unsigned int)found, pgnames, 1);
	if (!function)
		goto free_pins;

	res = pinctrl_sec_add_pingroup(pinctrl_sec, np, (char*)np->name, pins, found);
	if (res < 0)
		goto free_function;

	(*map)->type = PIN_MAP_TYPE_MUX_GROUP;
	(*map)->data.mux.group = np->name;
	(*map)->data.mux.function = np->name;

	if (PCS_HAS_PINCONF) {
		dev_err(pinctrl_sec->dev, "pinconf not supported\n");
		goto free_pingroups;
	}

	*num_maps = 1;
	return 0;

free_pingroups:
	pinctrl_sec_free_pingroups(pinctrl_sec);
	*num_maps = 1;
free_function:
	pinctrl_sec_remove_function(pinctrl_sec, function);

free_pins:
	devm_kfree(pinctrl_sec->dev, pins);

free_vals:
	devm_kfree(pinctrl_sec->dev, vals);

	return res;/*lint !e593*/
}
/**
 * pinctrl_sec_dt_node_to_map() - allocates and parses pinctrl maps
 * @pctldev: pinctrl instance
 * @np_config: device tree pinmux entry
 * @map: array of map entries
 * @num_maps: number of maps
 */
static int pinctrl_sec_dt_node_to_map(struct pinctrl_dev *pctldev,
				struct device_node *np_config,
				struct pinctrl_map **map, unsigned *num_maps)
{
	struct pinctrl_sec_device *pinctrl_sec;
	const char **pgnames;
	int ret;

	pinctrl_sec = pinctrl_dev_get_drvdata(pctldev);

	/* create 2 maps. One is for pinmux, and the other is for pinconf. */
	*map = devm_kzalloc(pinctrl_sec->dev, sizeof(**map) * 2, GFP_KERNEL);
	if (!*map)
		return -ENOMEM;

	*num_maps = 0;

	pgnames = devm_kzalloc(pinctrl_sec->dev, sizeof(*pgnames), GFP_KERNEL);
	if (!pgnames) {
		ret = -ENOMEM;
		goto free_map;
	}

	if (pinctrl_sec->bits_per_mux) {
		ret = pinctrl_sec_parse_bits_in_pinctrl_entry(pinctrl_sec, np_config, map,
				num_maps, pgnames);
		if (ret < 0) {
			dev_err(pinctrl_sec->dev, "no pins entries for %s\n",
				np_config->name);
			goto free_pgnames;
		}
	} else {
		ret = pinctrl_sec_parse_one_pinctrl_entry(pinctrl_sec, np_config, map,
				num_maps, pgnames);
		if (ret < 0) {
			dev_err(pinctrl_sec->dev, "no pins entries for %s\n",
				np_config->name);
			goto free_pgnames;
		}
	}

	return 0;

free_pgnames:
	devm_kfree(pinctrl_sec->dev, pgnames);
free_map:
	devm_kfree(pinctrl_sec->dev, *map);

	return ret;
}

/**
 * pinctrl_sec_free_funcs() - free memory used by functions
 * @pinctrl_sec: pinctrl_sec driver instance
 */
static void pinctrl_sec_free_funcs(struct pinctrl_sec_device *pinctrl_sec)
{
	struct list_head *pos, *tmp;
	int i;

	mutex_lock(&pinctrl_sec->mutex);
	for (i = 0; i < (int)pinctrl_sec->nfuncs; i++) {
		struct pinctrl_sec_function *func;

		func = radix_tree_lookup(&pinctrl_sec->ftree, i);/*lint !e732 !e747 */
		if (!func)
			continue;
		radix_tree_delete(&pinctrl_sec->ftree, i);/*lint !e732 !e747 */
	}
	list_for_each_safe(pos, tmp, &pinctrl_sec->functions) {
		struct pinctrl_sec_function *function;

		function = list_entry(pos, struct pinctrl_sec_function, node);/*lint !e826*/
		list_del(&function->node);
	}
	mutex_unlock(&pinctrl_sec->mutex);
}

/**
 * pinctrl_sec_free_pingroups() - free memory used by pingroups
 * @pinctrl_sec: pinctrl_sec driver instance
 */
static void pinctrl_sec_free_pingroups(struct pinctrl_sec_device *pinctrl_sec)
{
	struct list_head *pos, *tmp;
	int i;

	mutex_lock(&pinctrl_sec->mutex);
	for (i = 0; i < (int)pinctrl_sec->ngroups; i++) {
		struct pinctrl_sec_pingroup *pingroup;

		pingroup = radix_tree_lookup(&pinctrl_sec->pgtree, i);/*lint !e732 !e747 */
		if (!pingroup)
			continue;
		radix_tree_delete(&pinctrl_sec->pgtree, i);/*lint !e732 !e747 */
	}
	list_for_each_safe(pos, tmp, &pinctrl_sec->pingroups) {
		struct pinctrl_sec_pingroup *pingroup;

		pingroup = list_entry(pos, struct pinctrl_sec_pingroup, node);/*lint !e826*/
		list_del(&pingroup->node);
	}
	mutex_unlock(&pinctrl_sec->mutex);
}

/**
 * pinctrl_sec_irq_free() - free interrupt
 * @pinctrl_sec: pinctrl_sec driver instance
 */
/*static void pinctrl_sec_irq_free(struct pinctrl_sec_device *pinctrl_sec)
{
	struct pinctrl_sec_soc_data *pinctrl_sec_soc = &pinctrl_sec->socdata;

	if (pinctrl_sec_soc->irq < 0)
		return;

	if (pinctrl_sec->domain)
		irq_domain_remove(pinctrl_sec->domain);

	if (PCS_QUIRK_HAS_SHARED_IRQ)
		free_irq(pinctrl_sec_soc->irq, pinctrl_sec_soc);
	else
		irq_set_chained_handler(pinctrl_sec_soc->irq, NULL);
}*/

/**
 * pinctrl_sec_free_resources() - free memory used by this driver
 * @pinctrl_sec: pinctrl_sec driver instance
 */
static void pinctrl_sec_free_resources(struct pinctrl_sec_device *pinctrl_sec)
{
/*	pinctrl_sec_irq_free(pinctrl_sec);*/

	if (pinctrl_sec->pctl)
		pinctrl_unregister(pinctrl_sec->pctl);

	pinctrl_sec_free_funcs(pinctrl_sec);
	pinctrl_sec_free_pingroups(pinctrl_sec);
}

// cppcheck-suppress *
#define PCS_GET_PROP_U32(name, reg, err)				\
	do {								\
		ret = of_property_read_u32(np, name, reg);		\
		if (ret) {						\
			dev_err(pinctrl_sec->dev, err);				\
			return ret;					\
		}							\
	} while (0);

static const struct pinctrl_sec_soc_data pinctrl_single = {/*lint !e528 */
};/*lint !e785 */

static const struct pinctrl_sec_soc_data pinconf_single = {/*lint !e528 */
	.flags = PCS_FEAT_PINCONF,
};/*lint !e785 */

static const struct of_device_id pinctrl_sec_of_match[] = {
	{ .compatible = "hisilicon,pinctrl-sec", .data = &pinctrl_single },
	{ .compatible = "hisilicon,pinconf-sec", .data = &pinconf_single },
	{ },/*lint !e785 */
};

MODULE_DEVICE_TABLE(of, pinctrl_sec_of_match);

static int pinctrl_sec_add_gpio_func(struct device_node *node, struct pinctrl_sec_device *pinctrl_sec)
{
	const char *propname = "pinctrl-single,gpio-range";
	const char *cellname = "#pinctrl-single,gpio-range-cells";
	struct of_phandle_args gpiospec;
	struct pinctrl_sec_gpiofunc_range *range;
	int ret, i;

	for (i = 0; ; i++) {
		ret = of_parse_phandle_with_args(node, propname, cellname,
						 i, &gpiospec);
		/* Do not treat it as error. Only treat it as end condition. */
		if (ret) {
			ret = 0;
			break;
		}
		range = devm_kzalloc(pinctrl_sec->dev, sizeof(*range), GFP_KERNEL);
		if (!range) {
			ret = -ENOMEM;
			break;
		}
		range->offset = gpiospec.args[0];
		range->npins = gpiospec.args[1];
		range->gpiofunc = gpiospec.args[2];
		mutex_lock(&pinctrl_sec->mutex);
		list_add_tail(&range->node, &pinctrl_sec->gpiofuncs);
		mutex_unlock(&pinctrl_sec->mutex);
	}
	return ret;
}

static int pinctrl_sec_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *match;
	struct resource *res;
	struct pinctrl_sec_device *pinctrl_sec;
	const struct pinctrl_sec_soc_data *soc;
	int ret;
	match = of_match_device(pinctrl_sec_of_match, &pdev->dev);
	if (!match)
	{
		return -EINVAL;
	}
	pinctrl_sec = devm_kzalloc(&pdev->dev, sizeof(*pinctrl_sec), GFP_KERNEL);
	if (!pinctrl_sec) {
		dev_err(&pdev->dev, "could not allocate\n");
		return -ENOMEM;
	}
	pinctrl_sec->dev = &pdev->dev;
	raw_spin_lock_init(&pinctrl_sec->lock);
	mutex_init(&pinctrl_sec->mutex);
	INIT_LIST_HEAD(&pinctrl_sec->pingroups);
	INIT_LIST_HEAD(&pinctrl_sec->functions);
	INIT_LIST_HEAD(&pinctrl_sec->gpiofuncs);
	soc = match->data;
	pinctrl_sec->flags = soc->flags;
	memcpy(&pinctrl_sec->socdata, soc, sizeof(*soc));

	PCS_GET_PROP_U32("pinctrl-single,register-width", &pinctrl_sec->width,
			 "register width not specified\n");/*lint !e429*/

	ret = of_property_read_u32(np, "pinctrl-single,function-mask",
				   &pinctrl_sec->fmask);
	if (!ret) {
		pinctrl_sec->fshift = (unsigned int)(ffs((int)pinctrl_sec->fmask) - 1);
		pinctrl_sec->fmax = pinctrl_sec->fmask >> pinctrl_sec->fshift;
	} else {
		/* If mask property doesn't exist, function mux is invalid. */
		pinctrl_sec->fmask = 0;
		pinctrl_sec->fshift = 0;
		pinctrl_sec->fmax = 0;
	}

	ret = of_property_read_u32(np, "pinctrl-single,function-off",
					&pinctrl_sec->foff);
	if (ret)
		pinctrl_sec->foff = PCS_OFF_DISABLED;

	pinctrl_sec->bits_per_mux = of_property_read_bool(np,
						  "pinctrl-single,bit-per-mux");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(pinctrl_sec->dev, "could not get resource\n");
		devm_kfree(&pdev->dev,pinctrl_sec);
		return -ENODEV;/*lint !e429*/
	}

	pinctrl_sec->res = devm_request_mem_region(pinctrl_sec->dev, res->start,
			resource_size(res), DRIVER_NAME);
	if (!pinctrl_sec->res) {
		dev_err(pinctrl_sec->dev, "could not get mem_region\n");
		return -EBUSY;/*lint !e429*/
	}

	pinctrl_sec->size = (unsigned int)resource_size(pinctrl_sec->res);
	pinctrl_sec->base = devm_ioremap(pinctrl_sec->dev, pinctrl_sec->res->start, (unsigned long long)pinctrl_sec->size);
	if (!pinctrl_sec->base) {
		dev_err(pinctrl_sec->dev, "could not ioremap\n");
		return -ENODEV;/*lint !e429*/
	}

	INIT_RADIX_TREE(&pinctrl_sec->pgtree, GFP_KERNEL);
	INIT_RADIX_TREE(&pinctrl_sec->ftree, GFP_KERNEL);
	platform_set_drvdata(pdev, pinctrl_sec);
	pinctrl_sec->write = pinctrl_sec_write;
	pinctrl_sec->desc.name = DRIVER_NAME;
	pinctrl_sec->desc.pctlops = &pinctrl_sec_pinctrl_ops;
	pinctrl_sec->desc.pmxops = &pinctrl_sec_pinmux_ops;
	if (PCS_HAS_PINCONF)
		pinctrl_sec->desc.confops = &pinctrl_sec_pinconf_ops;
	pinctrl_sec->desc.owner = (struct module*)THIS_MODULE;/*lint !e826 */

	ret = pinctrl_sec_allocate_pin_table(pinctrl_sec);
	if (ret < 0)
		goto free;

	pinctrl_sec->pctl = pinctrl_register(&pinctrl_sec->desc, pinctrl_sec->dev, pinctrl_sec);
	if (!pinctrl_sec->pctl) {
		dev_err(pinctrl_sec->dev, "could not register single pinctrl driver\n");
		ret = -EINVAL;
		goto free;
	}

	ret = pinctrl_sec_add_gpio_func(np, pinctrl_sec);
	if (ret < 0)
		goto free;

	return 0;

free:
	pinctrl_sec_free_resources(pinctrl_sec);

	return ret;
}

static int pinctrl_sec_remove(struct platform_device *pdev)
{
	struct pinctrl_sec_device *pinctrl_sec = platform_get_drvdata(pdev);

	if (!pinctrl_sec)
		return 0;

	pinctrl_sec_free_resources(pinctrl_sec);

	return 0;
}

static struct platform_driver pinctrl_sec_driver = {
	.probe		= pinctrl_sec_probe,
	.remove		= pinctrl_sec_remove,
	.driver = {
		.owner          = (struct module*)THIS_MODULE,/*lint !e826 */
		.name		= DRIVER_NAME,
		.of_match_table	= pinctrl_sec_of_match,
	},/*lint !e785*/
};/*lint !e785 */

static int __init pinctrl_sec_init(void)
{
	return __platform_driver_register((struct platform_driver *)&pinctrl_sec_driver,
							(struct module*)THIS_MODULE);/*lint !e826 */
}

static void __exit pinctrl_sec_exit(void)
{
	platform_driver_unregister(&pinctrl_sec_driver);
}

/*lint -e528 -esym(528,*)*/
arch_initcall(pinctrl_sec_init);
module_exit(pinctrl_sec_exit);
/*lint -e528 +esym(528,*)*/

/*lint -e753 -esym(753,*)*/
MODULE_AUTHOR("w00347785<tony@atomide.com>");
MODULE_DESCRIPTION("One-register-per-pin type device tree based pinctrl driver");
MODULE_LICENSE("GPL v2");
/*lint -e753 +esym(753,*)*/
