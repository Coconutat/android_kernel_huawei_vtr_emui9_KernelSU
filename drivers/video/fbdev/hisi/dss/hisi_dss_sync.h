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
#ifndef HISI_DSS_SYNC_H
#define HISI_DSS_SYNC_H

#include <linux/types.h>
#include <linux/errno.h>

#define HISI_DSS_SYNC_NAME_SIZE             64

enum {
	HISI_DSS_RELEASE_FENCE = 0,
	HISI_DSS_RETIRE_FENCE,
};

/**
 * struct hisi_dss_fence - sync fence context
 * @base: base sync fence object
 * @name: name of this sync fence
 * @fence_list: linked list of outstanding sync fence
 */
struct hisi_dss_fence {
	struct fence base;
	char name[HISI_DSS_SYNC_NAME_SIZE];
	struct list_head fence_list;
};

/**
 * struct hisi_dss_timeline - sync timeline context
 * @kref: reference count of timeline
 * @lock: serialization lock for timeline and fence update
 * @name: name of timeline
 * @fence_name: fence name prefix
 * @next_value: next commit sequence number
 * @value: current retired sequence number
 * @context: fence context identifier
 * @fence_list_head: linked list of outstanding/active sync fence (unsignaled/errored)
 */

struct hisi_dss_timeline {
	struct kref kref;
	spinlock_t lock;
	spinlock_t list_lock;
	char name[HISI_DSS_SYNC_NAME_SIZE];
	u32 next_value;
	u32 value;
	u64 context;
	struct list_head fence_list_head;
};

#ifdef CONFIG_SYNC_FILE

/*
 * hisi_dss_create_timeline - Create timeline object with the given name
 * @name: Pointer to name character string.
 */
struct hisi_dss_timeline *hisi_dss_create_timeline(const char *name);

/*
 * hisi_dss_destroy_timeline - Destroy the given timeline object
 * @tl: Pointer to timeline object.
 */
void hisi_dss_destroy_timeline(struct hisi_dss_timeline *tl);

/*
 * hisi_dss_get_sync_fence - Create fence object from the given timeline
 * @tl: Pointer to timeline object
 * @timestamp: Pointer to timestamp of the returned fence. Null if not required.
 * Return: pointer fence created on give time line.
 */
struct hisi_dss_fence *hisi_dss_get_sync_fence(
		struct hisi_dss_timeline *tl, const char *fence_name,
		u32 *timestamp, int value);

/*
 * hisi_dss_put_sync_fence - Destroy given fence object
 * @fence: Pointer to fence object.
 */
void hisi_dss_put_sync_fence(struct hisi_dss_fence *fence);

/*
 * hisi_dss_wait_sync_fence - Wait until fence signal or timeout
 * @fence: Pointer to fence object.
 * @timeout: maximum wait time, in msec, for fence to signal.
 */
int hisi_dss_wait_sync_fence(struct hisi_dss_fence *fence,
		long timeout);

/*
 * hisi_dss_get_fd_sync_fence - Get fence object of given file descriptor
 * @fd: File description of fence object.
 */
struct hisi_dss_fence *hisi_dss_get_fd_sync_fence(int fd);

/*
 * hisi_dss_get_sync_fence_fd - Get file descriptor of given fence object
 * @fence: Pointer to fence object.
 * Return: File descriptor on success, or error code on error
 */
int hisi_dss_get_sync_fence_fd(struct hisi_dss_fence *fence);

/*
 * hisi_dss_get_sync_fence_name - get given fence object name
 * @fence: Pointer to fence object.
 * Return: fence name
 */
const char *hisi_dss_get_sync_fence_name(struct hisi_dss_fence *fence);

/*
 * hisi_dss_inc_timeline - Increment timeline by given amount
 * @tl: Pointer to timeline object.
 * @increment: the amount to increase the timeline by.
 */
int hisi_dss_inc_timeline(struct hisi_dss_timeline *tl, int increment);

/*
 * hisi_dss_resync_timeline - Resync timeline to last committed value
 * @tl: Pointer to timeline object.
 */
void hisi_dss_resync_timeline(struct hisi_dss_timeline *tl);

#else

#define hisi_dss_timeline sw_sync_timeline

struct hisi_dss_timeline *hisi_dss_create_timeline(const char *name)
{
	return sw_sync_timeline_create(name);
}

void hisi_dss_destroy_timeline(struct hisi_dss_timeline *tl)
{
	sync_timeline_destroy(tl);
}

int hisi_dss_inc_timeline(struct hisi_dss_timeline *tl, int increment)
{
	int rc;

	if (!tl) {
		pr_err("invalid parameters\n");
		return -EINVAL;
	}

	rc = sw_sync_timeline_inc(tl, increment);

	return rc;
}

#endif

#endif
