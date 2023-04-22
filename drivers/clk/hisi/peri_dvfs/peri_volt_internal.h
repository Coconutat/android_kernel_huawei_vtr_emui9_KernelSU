/*
 * Copyright (C) 2015
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PERIVOLT_POLL_INTERNAL_H
#define __PERIVOLT_POLL_INTERNAL_H

#include <linux/spinlock.h>
#include <linux/device.h>

#define PERI_GET_VOLT_NOCACHE				0x1

struct peri_volt_poll;

struct peri_volt_ops {
	int (*set_volt)(struct peri_volt_poll *pvp, unsigned int volt);
	unsigned int (*get_volt)(struct peri_volt_poll *pvp);
	unsigned int (*recalc_volt)(struct peri_volt_poll *pvp);
	unsigned int (*get_poll_stat)(struct peri_volt_poll *pvp);
	void (*init)(struct peri_volt_poll *pvp, unsigned int en);
};

/**
 * struct peri_volt_poll - a device which usually  dvfs
 * @name: device poll name, will be used to invoke peri volt
 * @addr: platform-specific peri volt poll unit base address
 * @bitsmask: platform-specific peri volt poll ctrl reg
 * @ops: platform-specific  peri_volt_poll handlers
 * @volt: peri zone current volt
 * @flags: platform-specific tag
 * @stat: peri volt poll reg  stat
 * @lock:
 * @pri: private data;
 * @poll_count: device poll num
 */
struct peri_volt_poll {
	const char			*name;
	unsigned int		dev_id;
	void __iomem		*addr;
	void __iomem		*addr_0; /*lpmcu poll reg*/
	unsigned int		bitsmask;
	unsigned int		bitsshift;
	const struct peri_volt_ops   *ops;
	unsigned int		volt;
	unsigned int		flags;
	unsigned int		stat;
	struct list_head	node;
	spinlock_t 			lock;
	void 				*priv;
	unsigned int		poll_count;
};

struct of_device_id;

typedef void (*of_perivolt_init_cb_t)(struct device_node *);

#define PERIVOLT_OF_DECLARE(name, compat, fn)                        \
	static const struct of_device_id __perivolt_of_table_##name  \
		__used __section(__perivolt_of_table)                \
		= { .compatible = compat, .data = fn };

int perivolt_register(struct device *dev, struct peri_volt_poll *pvp);

#endif /* __PERIVOLT_POLL_INTERNAL_H */
