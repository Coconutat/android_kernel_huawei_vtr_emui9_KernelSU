/* Copyright (c) 2015-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/fence.h>
#include <linux/sync_file.h>

#include "hisi_dss_sync.h"
#include "hisi_fb.h"

#define HISI_DSS_SYNC_NAME_SIZE		64
#define HISI_DSS_SYNC_DRIVER_NAME	"hisi_dss"

/*
 * to_hisi_dss_fence - get hisi dss fence from fence base object
 * @fence: Pointer to fence base object
 */
static struct hisi_dss_fence *to_hisi_dss_fence(struct fence *fence)
{
	return container_of(fence, struct hisi_dss_fence, base);
}

/*
 * to_hisi_dss_timeline - get hisi dss timeline from fence base object
 * @fence: Pointer to fence base object
 */
static struct hisi_dss_timeline *to_hisi_dss_timeline(struct fence *fence)
{
	return container_of(fence->lock, struct hisi_dss_timeline, lock);
}

/*
 * hisi_dss_free_timeline - Free the given timeline object
 * @kref: Pointer to timeline kref object.
 */
static void hisi_dss_free_timeline(struct kref *kref)
{
	struct hisi_dss_timeline *tl =
		container_of(kref, struct hisi_dss_timeline, kref);

	kfree(tl);
}

/*
 * hisi_dss_put_timeline - Put the given timeline object
 * @tl: Pointer to timeline object.
 */
static void hisi_dss_put_timeline(struct hisi_dss_timeline *tl)
{
	if (!tl) {
		HISI_FB_ERR("invalid parameters\n");
		return;
	}

	kref_put(&tl->kref, hisi_dss_free_timeline);
}

/*
 * hisi_dss_get_timeline - Get the given timeline object
 * @tl: Pointer to timeline object.
 */
static void hisi_dss_get_timeline(struct hisi_dss_timeline *tl)
{
	if (!tl) {
		HISI_FB_ERR("invalid parameters\n");
		return;
	}

	kref_get(&tl->kref);
}

static const char *hisi_dss_fence_get_driver_name(struct fence *fence)
{
	return HISI_DSS_SYNC_DRIVER_NAME;
}

static const char *hisi_dss_fence_get_timeline_name(struct fence *fence)
{
	struct hisi_dss_timeline *tl = to_hisi_dss_timeline(fence);

	return tl->name;
}

static bool hisi_dss_fence_enable_signaling(struct fence *fence)
{
	return true;
}

static bool hisi_dss_fence_signaled(struct fence *fence)
{
	struct hisi_dss_timeline *tl = to_hisi_dss_timeline(fence);
	bool status;

	status = ((s32) (tl->value - fence->seqno)) >= 0;
	return status;
}

static void hisi_dss_fence_release(struct fence *fence)
{
	struct hisi_dss_fence *f = to_hisi_dss_fence(fence);
	struct hisi_dss_timeline *tl = to_hisi_dss_timeline(fence);
	unsigned long flags;

	if (g_debug_fence_timeline) {
		HISI_FB_INFO("release for fence %s\n", f->name);
	}
	spin_lock_irqsave(&tl->list_lock, flags);
	if (!list_empty(&f->fence_list))
		list_del(&f->fence_list);
	spin_unlock_irqrestore(&tl->list_lock, flags);
	hisi_dss_put_timeline(to_hisi_dss_timeline(fence));
	kfree_rcu(f, base.rcu); //lint !e571 !e666
}

static void hisi_dss_fence_value_str(struct fence *fence, char *str, int size)
{
	snprintf(str, size, "%u", fence->seqno);
}

static void hisi_dss_fence_timeline_value_str(struct fence *fence, char *str,
		int size)
{
	struct hisi_dss_timeline *tl = to_hisi_dss_timeline(fence);

	snprintf(str, size, "%u", tl->value);
}

static struct fence_ops hisi_dss_fence_ops = {
	.get_driver_name = hisi_dss_fence_get_driver_name,
	.get_timeline_name = hisi_dss_fence_get_timeline_name,
	.enable_signaling = hisi_dss_fence_enable_signaling,
	.signaled = hisi_dss_fence_signaled,
	.wait = fence_default_wait,
	.release = hisi_dss_fence_release,
	.fence_value_str = hisi_dss_fence_value_str,
	.timeline_value_str = hisi_dss_fence_timeline_value_str,
};

/*
 * hisi_dss_create_timeline - Create timeline object with the given name
 * @name: Pointer to name character string.
 */
struct hisi_dss_timeline *hisi_dss_create_timeline(const char *name)
{
	struct hisi_dss_timeline *tl;

	if (!name) {
		HISI_FB_ERR("invalid parameters\n");
		return NULL;
	}

	tl = kzalloc(sizeof(struct hisi_dss_timeline), GFP_KERNEL);
	if (!tl)
		return NULL;

	kref_init(&tl->kref);
	snprintf(tl->name, sizeof(tl->name), "%s", name);
	spin_lock_init(&tl->lock);
	spin_lock_init(&tl->list_lock);
	tl->context = fence_context_alloc(1);
	INIT_LIST_HEAD(&tl->fence_list_head);

	return tl;
}

/*
 * hisi_dss_destroy_timeline - Destroy the given timeline object
 * @tl: Pointer to timeline object.
 */
void hisi_dss_destroy_timeline(struct hisi_dss_timeline *tl)
{
	hisi_dss_put_timeline(tl);
}

/*
 * hisi_dss_inc_timeline_locked - Increment timeline by given amount
 * @tl: Pointer to timeline object.
 * @increment: the amount to increase the timeline by.
 */
static int hisi_dss_inc_timeline_locked(struct hisi_dss_timeline *tl,
		int increment)
{
	struct hisi_dss_fence *f, *next;
	s32 val;
	bool is_signaled = false;
	struct list_head local_list_head;
	unsigned long flags;

	INIT_LIST_HEAD(&local_list_head);

	spin_lock(&tl->list_lock);
	if (list_empty(&tl->fence_list_head)) {
		HISI_FB_DEBUG("fence list is empty\n");
		tl->value += 1;
		spin_unlock(&tl->list_lock);
		return 0;
	}

	list_for_each_entry_safe(f, next, &tl->fence_list_head, fence_list)
		list_move(&f->fence_list, &local_list_head);
	spin_unlock(&tl->list_lock);

	spin_lock_irqsave(&tl->lock, flags);
	val = tl->next_value - tl->value;
	if (val >= increment)
		tl->value += increment;
	spin_unlock_irqrestore(&tl->lock, flags);

	list_for_each_entry_safe(f, next, &local_list_head, fence_list) {
		spin_lock_irqsave(&tl->lock, flags);
		is_signaled = fence_is_signaled_locked(&f->base);
		spin_unlock_irqrestore(&tl->lock, flags);
		if (is_signaled) {
			if (g_debug_fence_timeline) {
				HISI_FB_INFO("%s signaled\n", f->name);
			}
			list_del_init(&f->fence_list);
			fence_put(&f->base);
		} else {
			spin_lock(&tl->list_lock);
			list_move(&f->fence_list, &tl->fence_list_head);
			spin_unlock(&tl->list_lock);
		}
	}

	return 0;
}

/*
 * hisi_dss_resync_timeline - Resync timeline to last committed value
 * @tl: Pointer to timeline object.
 */
void hisi_dss_resync_timeline(struct hisi_dss_timeline *tl)
{
	s32 val;

	if (!tl) {
		pr_err("invalid parameters\n");
		return;
	}

	val = tl->next_value - tl->value;
	if (val > 0) {
		if (g_debug_fence_timeline) {
			HISI_FB_INFO("flush %s:%d TL(Nxt %d , Crnt %d)\n", tl->name, val,
				tl->next_value, tl->value);
		}
		hisi_dss_inc_timeline_locked(tl, val);
	}
}

/*lint -e429 */
/*
 * hisi_dss_get_sync_fence - Create fence object from the given timeline
 * @tl: Pointer to timeline object
 * @timestamp: Pointer to timestamp of the returned fence. Null if not required.
 * Return: pointer fence created on give time line.
 */
struct hisi_dss_fence *hisi_dss_get_sync_fence(
		struct hisi_dss_timeline *tl, const char *fence_name,
		u32 *timestamp, int value)
{
	struct hisi_dss_fence *f; //lint !e429
	unsigned long flags;

	if (!tl) {
		HISI_FB_ERR("invalid parameters\n");
		return NULL;
	}

	f = kzalloc(sizeof(struct hisi_dss_fence), GFP_KERNEL);
	if (!f)
		return NULL;

	INIT_LIST_HEAD(&f->fence_list);
	spin_lock_irqsave(&tl->lock, flags);
	tl->next_value = value;
	fence_init(&f->base, &hisi_dss_fence_ops, &tl->lock, tl->context, value);
	hisi_dss_get_timeline(tl);
	spin_unlock_irqrestore(&tl->lock, flags);

	spin_lock_irqsave(&tl->list_lock, flags);
	list_add_tail(&f->fence_list, &tl->fence_list_head);
	spin_unlock_irqrestore(&tl->list_lock, flags);
	snprintf(f->name, sizeof(f->name), "%s_%u", fence_name, value);

	if (timestamp)
		*timestamp = value;

	if (g_debug_fence_timeline) {
		HISI_FB_INFO("fence created at val=%u tl->name=%s tl->value=%d tl->next_value=%d\n",
			value, tl->name, tl->value, tl->next_value);
	}

	return (struct hisi_dss_fence *) &f->base; //lint !e429
}
/*lint +e429 */
/*
 * hisi_dss_inc_timeline - Increment timeline by given amount
 * @tl: Pointer to timeline object.
 * @increment: the amount to increase the timeline by.
 */
int hisi_dss_inc_timeline(struct hisi_dss_timeline *tl, int increment)
{
	int rc;

	if (!tl) {
		HISI_FB_ERR("invalid parameters\n");
		return -EINVAL;
	}

	rc = hisi_dss_inc_timeline_locked(tl, increment);
	return rc;
}

/*
 * hisi_dss_get_timeline_commit_ts - Return commit tick of given timeline
 * @tl: Pointer to timeline object.
 */
u32 hisi_dss_get_timeline_commit_ts(struct hisi_dss_timeline *tl)
{
	if (!tl) {
		HISI_FB_ERR("invalid parameters\n");
		return 0;
	}

	return tl->next_value;
}

/*
 * hisi_dss_get_timeline_retire_ts - Return retire tick of given timeline
 * @tl: Pointer to timeline object.
 */
u32 hisi_dss_get_timeline_retire_ts(struct hisi_dss_timeline *tl)
{
	if (!tl) {
		HISI_FB_ERR("invalid parameters\n");
		return 0;
	}

	return tl->value;
}

/*
 * hisi_dss_put_sync_fence - Destroy given fence object
 * @fence: Pointer to fence object.
 */
void hisi_dss_put_sync_fence(struct hisi_dss_fence *fence)
{
	if (!fence) {
		HISI_FB_ERR("invalid parameters\n");
		return;
	}

	fence_put((struct fence *) fence);
}

/*
 * hisi_dss_wait_sync_fence - Wait until fence signal or timeout
 * @fence: Pointer to fence object.
 * @timeout: maximum wait time, in msec, for fence to signal.
 */
int hisi_dss_wait_sync_fence(struct hisi_dss_fence *fence,
		long timeout)
{
	int rc;

	if (!fence) {
		HISI_FB_ERR("invalid parameters\n");
		return -EINVAL;
	}

	rc = fence_wait_timeout((struct fence *) fence, false,
			msecs_to_jiffies(timeout));
	if (rc > 0) {
		if (g_debug_fence_timeline) {
			struct fence *input_fence = (struct fence *) fence;
			HISI_FB_INFO(
				"drv:%s timeline:%s seqno:%d\n",
				input_fence->ops->get_driver_name(input_fence),
				input_fence->ops->get_timeline_name(input_fence),
				input_fence->seqno);
		}
		rc = 0;
	} else if (rc == 0) {
		struct fence *input_fence = (struct fence *) fence;
		HISI_FB_ERR(
			"drv:%s timeline:%s seqno:%d.\n",
			input_fence->ops->get_driver_name(input_fence),
			input_fence->ops->get_timeline_name(input_fence),
			input_fence->seqno);
		rc = -ETIMEDOUT;
	}

	return rc;
}

/*
 * hisi_dss_get_fd_sync_fence - Get fence object of given file descriptor
 * @fd: File description of fence object.
 */
struct hisi_dss_fence *hisi_dss_get_fd_sync_fence(int fd)
{
	return (struct hisi_dss_fence *) sync_file_get_fence(fd);
}

/*
 * hisi_dss_get_sync_fence_fd - Get file descriptor of given fence object
 * @fence: Pointer to fence object.
 * Return: File descriptor on success, or error code on error
 */
int hisi_dss_get_sync_fence_fd(struct hisi_dss_fence *fence)
{
	int fd;
	struct sync_file *sync_file;

	if (!fence) {
		HISI_FB_ERR("invalid parameters\n");
		return -EINVAL;
	}

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		HISI_FB_ERR("fail to get unused fd\n");
		return fd;
	}

	sync_file = sync_file_create((struct fence *) fence);
	if (!sync_file) {
		put_unused_fd(fd);
		HISI_FB_ERR("failed to create sync file\n");
		return -ENOMEM;
	}

	fd_install(fd, sync_file->file);

	return fd;
}

/*
 * hisi_dss_get_sync_fence_name - get given fence object name
 * @fence: Pointer to fence object.
 * Return: fence name
 */
const char *hisi_dss_get_sync_fence_name(struct hisi_dss_fence *fence)
{
	if (!fence) {
		HISI_FB_ERR("invalid parameters\n");
		return NULL;
	}

	return fence->name;
}


