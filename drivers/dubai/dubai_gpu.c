/*
 * DUBAI gpu.
 *
 * Copyright (C) 2017 Huawei Device Co.,Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <chipset_common/dubai/dubai_common.h>

#define DUBAI_GPU_FREQ_MAX_SIZE		(20)
#define US_PER_MSEC					(1000)

/*
 * freq, gpu freq(HZ)
 * run_time, run time
 * idle_time, idle time
 */
struct gpu_info {
	unsigned long freq;
	unsigned long run_time;
	unsigned long idle_time;
};

struct gpu_store {
	struct gpu_info array[DUBAI_GPU_FREQ_MAX_SIZE];
	int num;
};

struct gpu_store store;
static spinlock_t gpu_lock;
static atomic_t enable_update_gpu_info;

/*
 * lookup or create the gpu information
 */
static struct gpu_info *dubai_gpu_lookup_add(unsigned long freq)
{
	int i = 0, num = store.num;

	if (freq == 0)
		return NULL;

	for (i = 0; i < num; i++) {
		if (freq == store.array[i].freq)
			return &store.array[i];
	}
	if (num < DUBAI_GPU_FREQ_MAX_SIZE - 1) {
		store.array[num].freq = freq;
		store.num++;
		return &store.array[num];
	}
	return NULL;
}

/*
 * entry func when gpu is running
 */
int dubai_update_gpu_info(unsigned long freq, unsigned long busy_time,
	unsigned long total_time, unsigned long cycle_ms)
{
	struct gpu_info *info = NULL;
	unsigned long temp = 0;

	if (!atomic_read(&enable_update_gpu_info) || 0 == freq)
		return -1;

	spin_lock_bh(&gpu_lock);
	info = dubai_gpu_lookup_add(freq);
	if (info == NULL) {
		spin_unlock_bh(&gpu_lock);
		DUBAI_LOGE("gpu num exceed %d, abort freq:%ld!",
			DUBAI_GPU_FREQ_MAX_SIZE, freq);
		return -1;
	}

	info->freq = freq;
	temp = cycle_ms * US_PER_MSEC * busy_time / total_time;
	info->run_time += temp;
	info->idle_time += (cycle_ms * US_PER_MSEC - temp);

	spin_unlock_bh(&gpu_lock);

	return 0;
}

void dubai_set_gpu_enable(bool enable)
{
	atomic_set(&enable_update_gpu_info, enable ? 1 : 0);
	DUBAI_LOGI("gpu enable: %d", enable);
}

int dubai_get_gpu_info(unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct dev_transmit_t *stat = NULL;
	int rc = 0, i = 0;
	uint8_t *pdata;
	size_t size;

	spin_lock_bh(&gpu_lock);
	if (store.num <= 0) {
		spin_unlock_bh(&gpu_lock);
		return 0;
	}
	size = store.num * sizeof(struct gpu_info)
		+ sizeof(struct dev_transmit_t);

	stat = kzalloc(size, GFP_ATOMIC);
	if (stat == NULL) {
		rc = -ENOMEM;
		spin_unlock_bh(&gpu_lock);
		return rc;
	}

	pdata = stat->data;
	for (i = 0; i < store.num; i++) {
		memcpy(pdata, &store.array[i], sizeof(struct gpu_info));/* unsafe_function_ignore: memcpy */
		pdata += sizeof(struct gpu_info);
	}
	stat->length = store.num;

	memset(&store, 0, sizeof(struct gpu_store));/* unsafe_function_ignore: memset */
	spin_unlock_bh(&gpu_lock);

	rc = copy_to_user(argp, stat, size);
	kfree(stat);

	return rc;
}

void dubai_gpu_init(void)
{
	memset(&store, 0, sizeof(struct gpu_store));/* unsafe_function_ignore: memset */
	spin_lock_init(&gpu_lock);
	atomic_set(&enable_update_gpu_info, 0);
}

void dubai_gpu_exit(void)
{
}
