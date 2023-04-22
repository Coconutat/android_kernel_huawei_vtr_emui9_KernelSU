/*
 * PCIe host controller driver for Kirin 960 SoCs
 *
 * Copyright (C) 2015 Huawei Electronics Co., Ltd.
 *		http://www.huawei.com
 *
 * Author: Xiaowei Song <songxiaowei@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PCIE_KIRIN_API_H
#define _PCIE_KIRIN_API_H

enum kirin_pcie_event {
	KIRIN_PCIE_EVENT_MIN_INVALID = 0x0,		/*min invalid value*/
	KIRIN_PCIE_EVENT_LINKUP = 0x1,		/* linkup event  */
	KIRIN_PCIE_EVENT_LINKDOWN = 0x2,		/* linkdown event */
	KIRIN_PCIE_EVENT_WAKE = 0x4,	/* wake event*/
	KIRIN_PCIE_EVENT_L1SS = 0x8,	/* l1ss event*/
	KIRIN_PCIE_EVENT_CPL_TIMEOUT = 0x10,	/* completion timeout event */
	KIRIN_PCIE_EVENT_MAX_INVALID = 0x1F,	/* max invalid value*/
};

enum kirin_pcie_trigger {
	KIRIN_PCIE_TRIGGER_CALLBACK,
	KIRIN_PCIE_TRIGGER_COMPLETION,
};

struct kirin_pcie_notify {
	enum kirin_pcie_event event;
	void *user;
	void *data;
	u32 options;
};


struct kirin_pcie_register_event {
	u32 events;
	void *user;
	enum kirin_pcie_trigger mode;
	void (*callback)(struct kirin_pcie_notify *notify);
	struct kirin_pcie_notify notify;
	struct completion *completion;
	u32 options;
};

#ifdef CONFIG_PCIE_KIRIN
int kirin_pcie_register_event(struct kirin_pcie_register_event *reg);
int kirin_pcie_deregister_event(struct kirin_pcie_register_event *reg);
int kirin_pcie_pm_control(int power_ops, u32 rc_idx);
int kirin_pcie_ep_off(u32 rc_idx);
int kirin_pcie_lp_ctrl(u32 rc_idx, u32 enable);
int kirin_pcie_enumerate(u32 rc_idx);
int kirin_pcie_remove_ep(u32 rc_idx);
int kirin_pcie_rescan_ep(u32 rc_idx);
int pcie_ep_link_ltssm_notify(u32 rc_id, u32 link_status);
int kirin_pcie_power_notifiy_register(u32 rc_id, int (*poweron)(void* data),
				int (*poweroff)(void* data), void* data);

#else
static inline int kirin_pcie_register_event(struct kirin_pcie_register_event *reg)
{
	return -EINVAL;
}

static inline int kirin_pcie_deregister_event(struct kirin_pcie_register_event *reg)
{
	return -EINVAL;
}

static inline int kirin_pcie_pm_control(int power_ops, u32 rc_idx)
{
	return -EINVAL;
}

static inline int kirin_pcie_ep_off(u32 rc_idx)
{
	return -EINVAL;
}

static inline int kirin_pcie_lp_ctrl(u32 rc_idx, u32 enable)
{
	return -EINVAL;
}

static inline int kirin_pcie_enumerate(u32 rc_idx)
{
	return -EINVAL;
}

int kirin_pcie_remove_ep(u32 rc_idx)
{
	return -EINVAL;
}

int kirin_pcie_rescan_ep(u32 rc_idx)
{
	return -EINVAL;
}

static inline int pcie_ep_link_ltssm_notify(u32 rc_id, u32 link_status)
{
	return -EINVAL;
}

static inline int kirin_pcie_power_notifiy_register(u32 rc_id, int (*poweron)(void* data),
				int (*poweroff)(void* data), void* data)
{
	return -EINVAL;
}

#endif /* CONFIG_PCIE_KIRIN */

#endif

