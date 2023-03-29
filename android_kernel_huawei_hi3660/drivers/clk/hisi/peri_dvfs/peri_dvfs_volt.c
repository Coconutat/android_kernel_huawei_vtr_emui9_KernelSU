/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/sched.h>
#include "peri_volt_internal.h"

static struct task_struct *perivolt_owner;
static int perivolt_refcnt;
static LIST_HEAD(perivolt_list);
static DEFINE_MUTEX(perivolt_poll_lock);

/***           locking             ***/
static void perivolt_lock(void)
{
	if (!mutex_trylock(&perivolt_poll_lock)) {
		if (perivolt_owner == current) {
			perivolt_refcnt++;
			return;
		}
		mutex_lock(&perivolt_poll_lock);
	}
	WARN_ON_ONCE(perivolt_owner != NULL);/*lint !e456*/
	WARN_ON_ONCE(perivolt_refcnt != 0);
	perivolt_owner = current;
	perivolt_refcnt = 1;
}/*lint !e454*/

static void perivolt_unlock(void)
{
	WARN_ON_ONCE(perivolt_owner != current);
	WARN_ON_ONCE(perivolt_refcnt == 0);

	if (--perivolt_refcnt)
		return;
	perivolt_owner = NULL;
	mutex_unlock(&perivolt_poll_lock);/*lint !e455*/
}

unsigned int __peri_get_volt(struct peri_volt_poll *pvp)
{
	if (!pvp) {
		return -EINVAL;/*lint !e570*/
	}
	return pvp->volt;
}

struct peri_volt_poll *__perivolt_lookup(unsigned int dev_id, const char *name)
{
	struct peri_volt_poll *pvp;

	if (!dev_id) {
		list_for_each_entry(pvp, &perivolt_list, node)
			if (!strncmp(pvp->name, name, strlen(pvp->name)))
				return pvp;
	}
	if (NULL == name) {
		list_for_each_entry(pvp, &perivolt_list, node)
			if (dev_id == pvp->dev_id)
				return pvp;
	}
	return NULL;
}

unsigned int __peri_recalc_volt(struct peri_volt_poll *pvp)
{
	if (pvp->ops->recalc_volt)
		pvp->volt = pvp->ops->recalc_volt(pvp);
	return pvp->volt;
}
EXPORT_SYMBOL_GPL(__peri_recalc_volt);

int __peri_poll_stat(struct peri_volt_poll *pvp)
{
	if (pvp->ops->get_poll_stat)
		pvp->stat = pvp->ops->get_poll_stat(pvp);
	return pvp->stat;
}
EXPORT_SYMBOL_GPL(__peri_poll_stat);

int peri_get_temperature(struct peri_volt_poll *pvp)
{
	return 0;
}
EXPORT_SYMBOL_GPL(peri_get_temperature);

struct peri_volt_poll *peri_volt_poll_get(unsigned int dev_id, const char *name)
{
	struct peri_volt_poll *pvp;

	if (!dev_id  && NULL == name)
		return NULL;

	perivolt_lock();
	pvp = __perivolt_lookup(dev_id, name);
	perivolt_unlock();

	return pvp;
}
EXPORT_SYMBOL_GPL(peri_volt_poll_get);
/**
 * peri_get_volt - return the volt of peri dvfs
 * @pvp: the peri_volt_poll whose volt is being returned.
 */
unsigned int peri_get_volt(struct peri_volt_poll *pvp)
{
	unsigned int volt;

	if (NULL == pvp)
		return -EINVAL;/*lint !e570*/

	perivolt_lock();

	volt = __peri_get_volt(pvp);

	if (pvp && (pvp->flags & PERI_GET_VOLT_NOCACHE))
		volt = __peri_recalc_volt(pvp);

	perivolt_unlock();

	return volt;
}
EXPORT_SYMBOL_GPL(peri_get_volt);

int peri_poll_stat(struct peri_volt_poll *pvp)
{
	unsigned int ret = 0;

	if (NULL == pvp)
		return -EINVAL;
	perivolt_lock();
	ret = __peri_poll_stat(pvp);
	if (ret == -EPERM)/*lint !e650*/
		pr_err("[%s] %s peri dvfs is off\n", __func__, pvp->name);
	perivolt_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(peri_poll_stat);

int __peri_set_volt(struct peri_volt_poll *pvp, unsigned int volt)
{
	int ret = 0;
	if (pvp->ops->set_volt)
		ret = pvp->ops->set_volt(pvp, volt);
	return ret;
}

/**
 * peri_set_volt - specify a new volt for pvp
 * @pvp: the peri_volt_poll whose volt is being changed
 * @volt: the new rate for pvp
 *
 * Returns 0 on success, -EERROR otherwise.
 */
int peri_set_volt(struct peri_volt_poll *pvp, unsigned int volt)
{
	int ret = 0;

	if (!pvp)
		return -EINVAL;

	/* prevent racing with updates to the clock topology */
	perivolt_lock();

	/* change the rates */
	ret = __peri_set_volt(pvp, volt);
	if (ret == -EPERM)
		pr_err("[%s] %s peri dvfs is off\n", __func__, pvp->name);
	pvp->volt = volt;

	perivolt_unlock();

	return ret;
}
EXPORT_SYMBOL_GPL(peri_set_volt);

int __perivolt_init(struct device *dev, struct peri_volt_poll *pvp)
{
	int ret = 0;

	if (!pvp)
		return -EINVAL;

	perivolt_lock();

	/* check to see if a pvp with this name is already registered */
	if (__perivolt_lookup(0, pvp->name)) {
		pr_debug("%s: pvp %d already initialized\n",
				__func__, pvp->dev_id);
		ret = -EEXIST;
		goto out;
	}
	list_add(&pvp->node, &perivolt_list);

	if (pvp->ops->init)
		pvp->ops->init(pvp, 0);
out:
	perivolt_unlock();

	return ret;
}

/**
 * perivolt_register - register a peri_volt_poll and return a cookie.
 * Returns 0 on success, otherwise an error code.
 */
int perivolt_register(struct device *dev, struct peri_volt_poll *pvp)
{
	int ret;
	struct peri_volt_poll *perivolt = NULL;

	perivolt = kzalloc(sizeof(*pvp), GFP_KERNEL);
	if (!perivolt) {
		pr_err("%s: could not allocate pvp\n", __func__);
		ret = -ENOMEM;
		goto fail_out;
	}

	perivolt->name = kstrdup(pvp->name, GFP_KERNEL);
	if (!perivolt->name) {
		pr_err("%s: could not allocate pvp->name\n", __func__);
		ret = -ENOMEM;
		goto fail_name;
	}
	perivolt->dev_id = pvp->dev_id;
	perivolt->addr = pvp->addr;
	perivolt->addr_0 = pvp->addr_0;
	perivolt->bitsmask = pvp->bitsmask;
	perivolt->bitsshift = pvp->bitsshift;
	perivolt->ops = pvp->ops;
	perivolt->volt = pvp->volt;
	perivolt->flags = pvp->flags;
	perivolt->priv = pvp->priv;
	perivolt->stat = pvp->stat;
	pr_info("%s: id=%d name=%s\n", __func__, perivolt->dev_id, perivolt->name);
	ret = __perivolt_init(dev, perivolt);
	if (!ret)
		return ret;
fail_name:
	kfree(perivolt);
	perivolt = NULL;
fail_out:
	return ret;
}
EXPORT_SYMBOL_GPL(perivolt_register);
