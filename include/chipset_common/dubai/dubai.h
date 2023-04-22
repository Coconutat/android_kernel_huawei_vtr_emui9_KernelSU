/*
 * DUBAI logging information API definition.
 *
 * Copyright (C) 2017 Huawei Device Co.,Ltd.
 */

#ifndef DUBAI_H
#define DUBAI_H

enum {
	WORK_ENTER = 0,
	WORK_EXIT,
};

int dubai_update_gpu_info(unsigned long freq, unsigned long busy_time,
	unsigned long total_time, unsigned long cycle_ms);
void dubai_update_wakeup_info(const char *name, int gpio);
void dubai_update_brightness(uint32_t brightness);
void dubai_log_kworker(unsigned long address, unsigned long long enter_time);
void dubai_log_uevent(const char *devpath, unsigned int action);
void dubai_log_binder_stats(int reply, uid_t c_uid, int c_pid, uid_t s_uid, int s_pid);

#endif
