/*
 * DUBAI kernel statistics.
 *
 * Copyright (C) 2017 Huawei Device Co.,Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/hashtable.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/profile.h>

#include <huawei_platform/log/hwlog_kernel.h>
#include <chipset_common/dubai/dubai.h>
#include <chipset_common/dubai/dubai_common.h>

#define KWORKER_HASH_BITS			(2)
#define MAX_SYMBOL_LEN				(2)
#define MAX_DEVPATH_LEN				(2)
#define PRINT_MAX_LEN				(2)
#define MAX_BRIGHTNESS				(10000)
#define BINDER_STATS_HASH_BITS		(2)
#define DUBAI_AOD_DURATION_ENENT	(6)

#ifdef SENSORHUB_DUBAI_INTERFACE
extern uint64_t iomcu_dubai_log_fetch(uint8_t event_type);
#endif

#ifdef CONFIG_HISI_COUL
extern int hisi_battery_rm(void);
#endif

#define LOG_ENTRY_SIZE(head, info, count) \
	sizeof(head) \
		+ (long long)(count) * sizeof(info)

#pragma GCC diagnostic ignored "-Wunused-variable"
static struct brightness_info {
	atomic_t enable;
	uint64_t last_time;
	uint32_t last_brightness;
	uint64_t sum_time;
	uint64_t sum_brightness_time;
} _brightness_info;

static struct kworker_info {
	long long count;
	long long time;
	char symbol[MAX_SYMBOL_LEN];
} __packed _kworker_info;

static struct kworker_transmit {
	long long timestamp;
	int count;
	char data[0];
} __packed _kworker_transmit;

static struct kworker_entry {
	unsigned long address;
	struct kworker_info info;
	struct hlist_node hash;
} _kworker_entry;

static struct uevent_info {
	char devpath[MAX_DEVPATH_LEN];
	int actions[KOBJ_MAX];
} __packed _uevent_info;

static struct uevent_transmit {
	long long timestamp;
	int action_count;
	int count;
	char data[0];
} __packed _uevent_transmit;

static struct uevent_entry {
	struct list_head list;
	struct uevent_info uevent;
} _uevent_entry;

static struct binder_stats_list_entry {
	char name[NAME_LEN];
	struct list_head node;
} _binder_stats_list_entry;

static struct binder_proc_info {
	uid_t uid;
	pid_t pid;
	char name[NAME_LEN];
} __packed _binder_proc_info;

static struct binder_client_entry {
	struct binder_proc_info proc;
	int count;
	struct list_head node;
} _binder_client_entry;

static struct binder_stats_entry {
	struct binder_proc_info proc;
	int count;
	struct list_head client;
	struct list_head died;
	struct hlist_node hash;
} _binder_stats_entry;

static struct binder_stats_info {
	struct binder_proc_info service;
	struct binder_proc_info client;
	int count;
} __packed _binder_stats_info;

static struct binder_stats_transmit {
	long long timestamp;
	int count;
	char data[0];
} __packed _binder_stats_transmit;

static atomic_t kworker_count;
static atomic_t uevent_count;
static atomic_t log_stats_enable;
static struct brightness_info dubai_backlight;
static atomic_t binder_stats_enable;
static atomic_t binder_stats_count;
static atomic_t binder_client_count;

static DECLARE_HASHTABLE(kworker_hash_table, KWORKER_HASH_BITS);
static DEFINE_MUTEX(kworker_lock);
static LIST_HEAD(uevent_list);
static DEFINE_MUTEX(uevent_lock);
static DEFINE_MUTEX(brightness_lock);

static DECLARE_HASHTABLE(binder_stats_hash_table, BINDER_STATS_HASH_BITS);
static LIST_HEAD(binder_stats_died_list);
static DEFINE_MUTEX(binder_stats_hash_lock);
static LIST_HEAD(binder_stats_list);
static DEFINE_MUTEX(binder_stats_list_lock);

void ipf_get_waking_pkt(void* data, unsigned int len)
{
	if (data == NULL) {
		DUBAI_LOGE(" invalid param");
		return;
	}
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, data, PRINT_MAX_LEN, 0);
	return;
}

int dubai_get_battery_rm(void) {
#ifdef CONFIG_HISI_COUL
	return hisi_battery_rm();
#else
	return -EINVAL;
#endif
}

static struct kworker_entry *dubai_find_kworker_entry(unsigned long address)
{
	struct kworker_entry *entry = NULL;

	hash_for_each_possible(kworker_hash_table, entry, hash, address) {
		if (entry->address == address)
			return entry;
	}

	entry = kzalloc(sizeof(struct kworker_entry), GFP_ATOMIC);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to allocate memory");
		return NULL;
	}
	entry->address = address;
	atomic_inc(&kworker_count);
	hash_add(kworker_hash_table, &entry->hash, address);

	return entry;
}

void dubai_log_kworker(unsigned long address, unsigned long long enter_time)
{
	unsigned long long exit_time;
	struct kworker_entry *entry = NULL;

	if (!atomic_read(&log_stats_enable))
		return;

	exit_time = ktime_get_ns();

	if (!mutex_trylock(&kworker_lock))
		return;

	entry = dubai_find_kworker_entry(address);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to find kworker entry");
		goto out;
	}

	entry->info.count++;
	entry->info.time += exit_time - enter_time;

out:
	mutex_unlock(&kworker_lock);
}

int dubai_get_kworker_info(long long timestamp)
{
	int ret = 0, count = 0;
	long long size = 0;
	unsigned char *data = NULL;
	unsigned long bkt;
	struct kworker_entry *entry;
	struct hlist_node *tmp;
	struct buffered_log_entry *log_entry = NULL;
	struct kworker_transmit *transmit = NULL;

	if (!atomic_read(&log_stats_enable))
		return -EPERM;

	count = atomic_read(&kworker_count);
	if (count < 0)
		return -EINVAL;

	size = LOG_ENTRY_SIZE(struct kworker_transmit, struct kworker_info, count);
	log_entry = create_buffered_log_entry(size, BUFFERED_LOG_MAGIC_KWORKER);
	if (log_entry == NULL) {
		DUBAI_LOGE("Failed to create buffered log entry");
		return -ENOMEM;
	}
	transmit = (struct kworker_transmit *)log_entry->data;
	transmit->timestamp = timestamp;
	transmit->count = 0;
	data = transmit->data;

	mutex_lock(&kworker_lock);
	hash_for_each_safe(kworker_hash_table, bkt, tmp, entry, hash) {
		if (entry->info.count == 0 || entry->info.time == 0) {
			hash_del(&entry->hash);
			kfree(entry);
			atomic_dec(&kworker_count);
			continue;
		}

		if (strlen(entry->info.symbol) == 0) {
			char buffer[KSYM_SYMBOL_LEN] = {0};

			sprint_symbol_no_offset(buffer, entry->address);
			buffer[KSYM_SYMBOL_LEN - 1] = '\0';
			strncpy(entry->info.symbol, buffer, MAX_SYMBOL_LEN - 1);/* unsafe_function_ignore: strncpy */
		}

		if (transmit->count < count) {
			memcpy(data, &entry->info, sizeof(struct kworker_info));/* unsafe_function_ignore: memcpy */
			data += sizeof(struct kworker_info);
			transmit->count++;
		}
		entry->info.count = 0;
		entry->info.time = 0;
	}
	mutex_unlock(&kworker_lock);

	log_entry->length = LOG_ENTRY_SIZE(struct kworker_transmit,
							struct kworker_info, transmit->count);
	ret = send_buffered_log(log_entry);
	if (ret < 0)
		DUBAI_LOGE("Failed to send kworker log entry");
	free_buffered_log_entry(log_entry);

	return ret;
}

static struct uevent_entry *dubai_find_uevent_entry(const char *devpath)
{
	struct uevent_entry *entry = NULL;

	list_for_each_entry(entry, &uevent_list, list) {
		if (!strncmp(devpath, entry->uevent.devpath, MAX_DEVPATH_LEN - 1))
			return entry;
	}

	entry = kzalloc(sizeof(struct uevent_entry), GFP_ATOMIC);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to allocate memory");
		return NULL;
	}
	strncpy(entry->uevent.devpath, devpath, MAX_DEVPATH_LEN - 1);/* unsafe_function_ignore: strncpy */
	atomic_inc(&uevent_count);
	list_add_tail(&entry->list, &uevent_list);

	return entry;
}

void dubai_log_uevent(const char *devpath, unsigned int action) {
	struct uevent_entry *entry = NULL;

	if (!atomic_read(&log_stats_enable)
		|| devpath == NULL
		|| action >= KOBJ_MAX)
		return;

	mutex_lock(&uevent_lock);
	entry = dubai_find_uevent_entry(devpath);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to find uevent entry");
		goto out;
	}
	(entry->uevent.actions[action])++;

out:
	mutex_unlock(&uevent_lock);
}

int dubai_get_uevent_info(long long timestamp)
{
	int ret = 0, count = 0;
	long long size = 0;
	unsigned char *data = NULL;
	struct uevent_entry *entry, *tmp;
	struct buffered_log_entry *log_entry = NULL;
	struct uevent_transmit *transmit = NULL;

	if (!atomic_read(&log_stats_enable))
		return -EPERM;

	count = atomic_read(&uevent_count);
	if (count < 0)
		return -EINVAL;

	size = LOG_ENTRY_SIZE(struct uevent_transmit, struct uevent_info, count);
	log_entry = create_buffered_log_entry(size, BUFFERED_LOG_MAGIC_UEVENT);
	if (log_entry == NULL) {
		DUBAI_LOGE("Failed to create buffered log entry");
		return -ENOMEM;
	}
	transmit = (struct uevent_transmit *)log_entry->data;
	transmit->timestamp = timestamp;
	transmit->action_count = KOBJ_MAX;
	transmit->count = 0;
	data = transmit->data;

	mutex_lock(&uevent_lock);
	list_for_each_entry_safe(entry, tmp, &uevent_list, list) {
		int i, total;

		for (i = 0, total = 0; i < KOBJ_MAX; i++) {
			total += entry->uevent.actions[i];
		}

		if (total == 0) {
			list_del_init(&entry->list);
			kfree(entry);
			atomic_dec(&uevent_count);
			continue;
		}
		if (transmit->count < count) {
			memcpy(data, &entry->uevent, sizeof(struct uevent_info));/* unsafe_function_ignore: memcpy */
			data += sizeof(struct uevent_info);
			transmit->count++;
		}
		memset(&(entry->uevent.actions), 0, KOBJ_MAX * sizeof(int));/* unsafe_function_ignore: memset */
	}
	mutex_unlock(&uevent_lock);

	log_entry->length = LOG_ENTRY_SIZE(struct uevent_transmit,
							struct uevent_info, transmit->count);
	ret = send_buffered_log(log_entry);
	if (ret < 0)
		DUBAI_LOGE("Failed to send uevent log entry");
	free_buffered_log_entry(log_entry);

	return ret;
}

void dubai_log_stats_enable(bool enable)
{
	atomic_set(&log_stats_enable, enable ? 1 : 0);
}

void dubai_update_wakeup_info(const char *name, int gpio)
{
	if (name == NULL) {
		DUBAI_LOGE("Invalid parameter");
		return;
	}

	HWDUBAI_LOGE("DUBAI_TAG_KERNEL_WAKEUP", "irq=%s gpio=%d", name, gpio);
}

void dubai_set_brightness_enable(bool enable)
{
	atomic_set(&dubai_backlight.enable, enable ? 1 : 0);
}

static int dubai_lock_set_brightness(uint32_t brightness)
{
	uint64_t current_time;

	if (brightness > MAX_BRIGHTNESS) {
		DUBAI_LOGE("Need valid data! brightness= %d", brightness);
		return -EINVAL;
	}

	current_time = ktime_get_ns() / NSEC_PER_MSEC;

	if (dubai_backlight.last_brightness > 0) {
		uint64_t diff_time = current_time - dubai_backlight.last_time;

		dubai_backlight.sum_time += diff_time;
		dubai_backlight.sum_brightness_time += dubai_backlight.last_brightness * diff_time;
	}

	dubai_backlight.last_time = current_time;
	dubai_backlight.last_brightness = brightness;

	return 0;
}

void dubai_update_brightness(uint32_t brightness)
{
	if (!atomic_read(&dubai_backlight.enable))
		return;

	mutex_lock(&brightness_lock);
	dubai_lock_set_brightness(brightness);
	mutex_unlock(&brightness_lock);
}

int dubai_get_brightness_info(uint32_t *brightness)
{
	int ret = 0;

	if (brightness == NULL) {
		DUBAI_LOGE("Invalid param");
		return -EINVAL;
	}

	mutex_lock(&brightness_lock);
	ret = dubai_lock_set_brightness(dubai_backlight.last_brightness);

	if (dubai_backlight.sum_time > 0 && dubai_backlight.sum_brightness_time > 0) {
		*brightness = (uint32_t)(dubai_backlight.sum_brightness_time / dubai_backlight.sum_time);
	} else {
		*brightness = 0;
	}

	dubai_backlight.sum_time = 0;
	dubai_backlight.sum_brightness_time = 0;
	mutex_unlock(&brightness_lock);

	return ret;
}

uint64_t dubai_get_aod_duration(void)
{
#ifdef SENSORHUB_DUBAI_INTERFACE
	return iomcu_dubai_log_fetch(DUBAI_AOD_DURATION_ENENT);
#else
	return 0;
#endif
}

static void dubai_init_binder_client(struct list_head *head)
{
	struct binder_client_entry *client, *tmp;

	if (head != NULL) {
		list_for_each_entry_safe(client, tmp, head, node) {
			list_del_init(&client->node);
			kfree(client);
		}
	}
}

static void dubai_init_binder_stats(void)
{
	struct hlist_node *tmp;
	unsigned long bkt;
	struct binder_stats_entry *stats, *temp;

	mutex_lock(&binder_stats_hash_lock);
	hash_for_each_safe(binder_stats_hash_table, bkt, tmp, stats, hash) {
		dubai_init_binder_client(&stats->client);
		hash_del(&stats->hash);
		kfree(stats);
	}
	list_for_each_entry_safe(stats, temp, &binder_stats_died_list, died) {
		dubai_init_binder_client(&stats->client);
		list_del_init(&stats->died);
		kfree(stats);
	}
	mutex_unlock(&binder_stats_hash_lock);
	atomic_set(&binder_client_count, 0);
}

void dubai_init_binder_stats_list(void)
{
	struct binder_stats_list_entry *entry, *tmp;

	mutex_lock(&binder_stats_list_lock);
	list_for_each_entry_safe(entry, tmp, &binder_stats_list, node) {
		list_del_init(&entry->node);
		kfree(entry);
	}
	mutex_unlock(&binder_stats_list_lock);
	atomic_set(&binder_stats_count, 0);
}

static bool dubai_check_binder_stats_enable(char *name)
{
	struct binder_stats_list_entry *entry = NULL;
	bool rc = false;

	if (name == NULL) {
		DUBAI_LOGE("Invalid param");
		return false;
	}

	if (atomic_read(&binder_stats_count) == 0)
		return true;

	if (!mutex_trylock(&binder_stats_list_lock))
		return false;

	list_for_each_entry(entry, &binder_stats_list, node) {
		if (strncmp(entry->name, name, NAME_LEN - 1) == 0) {
			rc = true;
			goto out;
		}
	}
out:
	mutex_unlock(&binder_stats_list_lock);//lint !e455
	return rc;
}

static struct binder_client_entry *dubai_check_binder_client_entry(
	struct list_head *head, struct binder_proc_info *proc)
{
	struct binder_client_entry *entry = NULL;

	if ((head == NULL) || (proc == NULL)) {
		DUBAI_LOGE("Invalid param");
		return NULL;
	}

	list_for_each_entry(entry, head, node) {
		if (((entry->proc.pid == proc->pid) && (entry->proc.uid == proc->uid))
			|| (strncmp(entry->proc.name, proc->name, NAME_LEN - 1) == 0)) {
			return entry;
		}
	}
	return NULL;
}

static struct binder_client_entry *dubai_find_binder_client_entry(
	struct binder_stats_entry *stats, struct binder_proc_info *proc)
{
	struct binder_client_entry *entry = NULL;

	if ((stats == NULL) || (proc == NULL)) {
		DUBAI_LOGE("Invalid param");
		return NULL;
	}

	entry = dubai_check_binder_client_entry(&stats->client, proc);
	if (entry == NULL) {
		entry = kzalloc(sizeof(struct binder_client_entry), GFP_ATOMIC);
		if (entry == NULL) {
			DUBAI_LOGE("Failed to allocate binder client entry");
			return NULL;
		}
		memset(&entry->proc, 0, sizeof(struct binder_proc_info));/* unsafe_function_ignore: memset */
		entry->count = 0;
		list_add_tail(&entry->node, &stats->client);
		stats->count++;
		atomic_inc(&binder_client_count);
	}
	return entry;
}


static struct binder_stats_entry *dubai_check_binder_stats_entry(struct binder_proc_info *proc)
{
	struct binder_stats_entry *entry = NULL;

	if (proc == NULL) {
		DUBAI_LOGE("Invalid param");
		return NULL;
	}

	hash_for_each_possible(binder_stats_hash_table, entry, hash, proc->pid) {//lint !e666
		if (entry->proc.pid == proc->pid) {
			if (entry->proc.uid != proc->uid) {
				entry->count = 0;
				dubai_init_binder_client(&entry->client);
				list_del_init(&entry->client);
			}
			return entry;
		}
	}
	return NULL;
}

static struct binder_stats_entry *dubai_find_binder_stats_entry(struct binder_proc_info *proc)
{
	struct binder_stats_entry *entry = NULL;

	if (proc == NULL) {
		DUBAI_LOGE("Invalid param");
		return NULL;
	}

	entry = dubai_check_binder_stats_entry(proc);
	if (entry == NULL) {
		entry = kzalloc(sizeof(struct binder_stats_entry), GFP_ATOMIC);
		if (entry == NULL) {
			DUBAI_LOGE("Failed to allocate binder stats entry");
			return NULL;
		}
		memset(&entry->proc, 0, sizeof(struct binder_proc_info));/* unsafe_function_ignore: memset */
		entry->count = 0;
		INIT_LIST_HEAD(&entry->client);
		hash_add(binder_stats_hash_table, &entry->hash, proc->pid);//lint !e666
	}
	return entry;
}

static int dubai_get_binder_proc_name(struct binder_proc_info *service, struct binder_proc_info *client)
{
	struct binder_stats_entry *stats = NULL;
	struct binder_client_entry *entry = NULL;
	int rc = 0;
	struct process_name s, c;

	if ((service == NULL) || (client == NULL)) {
		DUBAI_LOGE("Invalid param");
		return -1;
	}

	if (!mutex_trylock(&binder_stats_hash_lock))
		return -1;

	stats = dubai_check_binder_stats_entry(service);
	if (stats == NULL) {
		mutex_unlock(&binder_stats_hash_lock);//lint !e455

		memset(&s, 0, sizeof(struct process_name));/* unsafe_function_ignore: memset */
		s.pid = service->pid;
		memset(&c, 0, sizeof(struct process_name));/* unsafe_function_ignore: memset */
		c.pid = client->pid;
		rc = dubai_get_proc_name(&s);
		if (rc <= 0) {
			DUBAI_LOGE("Failed to get service name: %d, %d", service->uid, service->pid);
			return -1;
		}
		rc = dubai_get_proc_name(&c);
		if (rc <= 0) {
			DUBAI_LOGE("Failed to get client name: %d, %d", client->uid, client->pid);
			return -1;
		}
		strncpy(service->name, s.name, NAME_LEN - 1);/* unsafe_function_ignore: strncpy */
		strncpy(client->name, c.name, NAME_LEN - 1);/* unsafe_function_ignore: strncpy */
	} else {
		strncpy(service->name, stats->proc.name, NAME_LEN - 1);/* unsafe_function_ignore: strncpy */
		entry = dubai_check_binder_client_entry(&stats->client, client);
		if (entry == NULL) {
			mutex_unlock(&binder_stats_hash_lock);//lint !e455

			memset(&c, 0, sizeof(struct process_name));/* unsafe_function_ignore: memset */
			c.pid = client->pid;
			rc = dubai_get_proc_name(&c);
			if (rc <= 0) {
				DUBAI_LOGE("Failed to get client name: %d, %d", client->uid, client->pid);
				return -1;
			}
			strncpy(client->name, c.name, NAME_LEN - 1);/* unsafe_function_ignore: strncpy */
		} else {
			strncpy(client->name, entry->proc.name, NAME_LEN - 1);/* unsafe_function_ignore: strncpy */
			mutex_unlock(&binder_stats_hash_lock);//lint !e455
		}
	}
	return 0;
}

static void dubai_add_binder_stats(struct binder_proc_info *service, struct binder_proc_info *client)
{
	struct binder_stats_entry *stats = NULL;
	struct binder_client_entry *entry = NULL;

	if ((service == NULL) || (client == NULL)) {
		DUBAI_LOGE("Invalid param");
		return;
	}

	if (!mutex_trylock(&binder_stats_hash_lock))
		return;

	stats = dubai_find_binder_stats_entry(service);
	if (stats == NULL) {
		DUBAI_LOGE("Failed to find binder stats entry");
		goto out;
	}
	stats->proc = *service;
	entry = dubai_find_binder_client_entry(stats, client);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to add binder client entry");
		goto out;
	}
	entry->proc = *client;
	entry->count++;

out:
	mutex_unlock(&binder_stats_hash_lock);//lint !e455
}

static void dubai_get_binder_client(
	struct binder_stats_entry *stats, struct binder_stats_transmit *transmit, int count)
{
	struct binder_client_entry *client, *temp;
	struct binder_stats_info info;
	unsigned char *data = NULL;

	if ((stats == NULL) || (transmit == NULL)) {
		DUBAI_LOGE("Invalid param");
		return;
	}

	data = transmit->data + transmit->count * sizeof(struct binder_stats_info);
	info.service = stats->proc;
	list_for_each_entry_safe(client, temp, &stats->client, node) {
		DUBAI_LOGD("service: %d, %s, %d, %d, client: %d, %s, %d, %d",
			stats->count, stats->proc.name, stats->proc.uid, stats->proc.pid,
			client->count, client->proc.name, client->proc.uid, client->proc.pid);

		info.client = client->proc;
		info.count = client->count;
		if (transmit->count < count) {
			memcpy(data, &info, sizeof(struct binder_stats_info));/* unsafe_function_ignore: memcpy */
			data += sizeof(struct binder_stats_info);
			transmit->count++;
		}

		list_del_init(&client->node);
		kfree(client);
	}
}

static void dubai_get_binder_stats_info(struct binder_stats_transmit *transmit, int count)
{
	unsigned long bkt;
	struct binder_stats_entry *stats;
	struct hlist_node *tmp;

	if (transmit == NULL) {
		DUBAI_LOGE("Invalid param");
		return;
	}

	hash_for_each_safe(binder_stats_hash_table, bkt, tmp, stats, hash) {
		if (stats->count == 0) {
			dubai_init_binder_client(&stats->client);
			hash_del(&stats->hash);
			kfree(stats);
			continue;
		}

		dubai_get_binder_client(stats, transmit, count);
		stats->count = 0;
		INIT_LIST_HEAD(&stats->client);
	}
}

static void dubai_get_binder_stats_died(struct binder_stats_transmit *transmit, int count)
{
	struct binder_stats_entry *tmp, *stats;

	if (transmit == NULL) {
		DUBAI_LOGE("Invalid param");
		return;
	}

	list_for_each_entry_safe(stats, tmp, &binder_stats_died_list, died) {
		dubai_get_binder_client(stats, transmit, count);
		list_del_init(&stats->died);
		kfree(stats);
	}
}

int dubai_get_binder_stats(long long timestamp)
{
	int ret = 0, count = 0;
	long long size = 0;
	struct buffered_log_entry *log_entry = NULL;
	struct binder_stats_transmit *transmit = NULL;

	if (!atomic_read(&binder_stats_enable))
		return -EPERM;

	count = atomic_read(&binder_client_count);
	size = LOG_ENTRY_SIZE(struct binder_stats_transmit, struct binder_stats_info, count);
	log_entry = create_buffered_log_entry(size, BUFFERED_LOG_MAGIC_BINDER_STATS);
	if (log_entry == NULL) {
		dubai_init_binder_stats();
		DUBAI_LOGE("Failed to create buffered log entry");
		return -ENOMEM;
	}
	transmit = (struct binder_stats_transmit *)log_entry->data;
	transmit->timestamp = timestamp;
	transmit->count = 0;

	mutex_lock(&binder_stats_hash_lock);
	dubai_get_binder_stats_died(transmit, count);
	dubai_get_binder_stats_info(transmit, count);
	mutex_unlock(&binder_stats_hash_lock);
	atomic_set(&binder_client_count, 0);

	if (transmit->count > 0) {
		log_entry->length = LOG_ENTRY_SIZE(struct binder_stats_transmit, struct binder_stats_info, transmit->count);
		ret = send_buffered_log(log_entry);
		if (ret < 0)
			DUBAI_LOGE("Failed to send binder stats log entry: %d", ret);
	}
	free_buffered_log_entry(log_entry);

	return ret;
}

void dubai_log_binder_stats(int reply, uid_t c_uid, int c_pid, uid_t s_uid, int s_pid)
{
	struct binder_proc_info client, service;
	bool enable = false;

	if (reply || !atomic_read(&binder_stats_enable))
		return;

	if (c_uid > 1000000 || s_uid > 1000000)
		return;

	memset(&service, 0, sizeof(struct binder_proc_info));/* unsafe_function_ignore: memset */
	service.uid = s_uid;
	service.pid = s_pid;
	memset(&client, 0, sizeof(struct binder_proc_info));/* unsafe_function_ignore: memset */
	client.uid = c_uid;
	client.pid = c_pid;

	if (dubai_get_binder_proc_name(&service, &client) == 0) {
		enable = dubai_check_binder_stats_enable(service.name);
		if (!enable)
			enable = dubai_check_binder_stats_enable(client.name);
	}
	if (enable)
		dubai_add_binder_stats(&service, &client);
	DUBAI_LOGD("ENABLE: %d, SNAME: %s, SUID: %d, SPID: %d, CNAME: %s, CUID: %d, CPID: %d",
		enable ? 1 : 0, service.name, s_uid, s_pid, client.name, c_uid, c_pid);
}

/*lint -esym(429,*)*/
void dubai_set_binder_stats_list(char *name)
{
	struct binder_stats_list_entry *entry = NULL;
	int size;

	if (name == NULL) {
		DUBAI_LOGE("Invalid param");
		return;
	}
	DUBAI_LOGI("Set binder stats name: %s", name);

	size = sizeof(struct binder_stats_list_entry);
	entry = kzalloc(size, GFP_KERNEL);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to allocate memory");
		return;
	}
	entry->name[NAME_LEN - 1] = '\0';
	strncpy(entry->name, name, NAME_LEN - 1);/* unsafe_function_ignore: strncpy */

	mutex_lock(&binder_stats_list_lock);
	list_add_tail(&entry->node, &binder_stats_list);
	mutex_unlock(&binder_stats_list_lock);
	atomic_inc(&binder_stats_count);
}
/*lint +esym(429,*)*/

void dubai_binder_stats_enable(bool enable)
{
	DUBAI_LOGI("Set binder stats enable: %d", enable ? 1 : 0);
	atomic_set(&binder_stats_enable, enable ? 1 : 0);
	if (!enable) {
		dubai_init_binder_stats_list();
		dubai_init_binder_stats();
	}
}

static void dubai_process_binder_died(pid_t pid)
{
	unsigned long bkt;
	struct hlist_node *tmp;
	struct binder_stats_entry *stats;

	if (!atomic_read(&binder_stats_enable))
		return;

	mutex_lock(&binder_stats_hash_lock);
	hash_for_each_safe(binder_stats_hash_table, bkt, tmp, stats, hash) {
		if (stats->proc.pid == pid) {
			DUBAI_LOGD("service: %d, %s, %d, %d",
				stats->count, stats->proc.name, stats->proc.uid, stats->proc.pid);

			hash_del(&stats->hash);
			list_add_tail(&stats->died, &binder_stats_died_list);
			break;
		}
	}
	mutex_unlock(&binder_stats_hash_lock);
}

static int dubai_stats_process_notifier(struct notifier_block *self, unsigned long cmd, void *v)
{
	struct task_struct *task = v;

	if (task == NULL)
		goto exit;

	if (task->tgid == task->pid)
		dubai_process_binder_died(task->tgid);

exit:
	return NOTIFY_OK;
}

static int dubai_pm_notify(struct notifier_block *nb,
			unsigned long mode, void *data)
{
	switch (mode) {
	case PM_SUSPEND_PREPARE:
		HWDUBAI_LOGE("DUBAI_TAG_KERNEL_SUSPEND", "");
		break;
	case PM_POST_SUSPEND:
		HWDUBAI_LOGE("DUBAI_TAG_KERNEL_RESUME", "");
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block dubai_pm_nb = {
	.notifier_call = dubai_pm_notify,
};

static struct notifier_block process_notifier_block = {
	.notifier_call	= dubai_stats_process_notifier,
};

void dubai_stats_init(void)
{
	atomic_set(&binder_stats_enable, 0);
	atomic_set(&binder_stats_count, 0);
	atomic_set(&binder_client_count, 0);
	hash_init(binder_stats_hash_table);
	atomic_set(&log_stats_enable, 0);
	hash_init(kworker_hash_table);
	atomic_set(&kworker_count, 0);
	atomic_set(&uevent_count, 0);
	atomic_set(&dubai_backlight.enable, 0);
	dubai_backlight.last_time = 0;
	dubai_backlight.last_brightness = 0;
	dubai_backlight.sum_time = 0;
	dubai_backlight.sum_brightness_time = 0;
	register_pm_notifier(&dubai_pm_nb);
	profile_event_register(PROFILE_TASK_EXIT, &process_notifier_block);
}

void dubai_stats_exit(void)
{
	struct kworker_entry *kworker = NULL;
	struct hlist_node *tmp;
	struct uevent_entry *uevent, *temp;
	unsigned long bkt;

	profile_event_unregister(PROFILE_TASK_EXIT, &process_notifier_block);
	dubai_init_binder_stats_list();
	dubai_init_binder_stats();

	mutex_lock(&kworker_lock);
	hash_for_each_safe(kworker_hash_table, bkt, tmp, kworker, hash) {
		hash_del(&kworker->hash);
		kfree(kworker);
	}
	atomic_set(&kworker_count, 0);
	mutex_unlock(&kworker_lock);

	mutex_lock(&uevent_lock);
	list_for_each_entry_safe(uevent, temp, &uevent_list, list) {
		list_del_init(&uevent->list);
		kfree(uevent);
	}
	atomic_set(&uevent_count, 0);
	mutex_unlock(&uevent_lock);

	unregister_pm_notifier(&dubai_pm_nb);
}
