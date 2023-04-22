/*
 * Copyright (C) 2016 The Huawei Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _NETWORK_AWARE_H
#define _NETWORK_AWARE_H

#include <asm/param.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/atomic.h>

#define MAX_UID_NUM 20

#define TIME_LONG_OF_HZ (1000/HZ)

#define MAX_FG_CTRL_TIME_LONG 100
#define MAX_FG_NET_STAT (MAX_FG_CTRL_TIME_LONG/TIME_LONG_OF_HZ)

#define MAX_ARRAY_LENGTH 256

#define MIN_BG_PACKAGE_COUNT 5
#define MAX_POLICY_RATIO 800


#define MIN_BG_COUNT 1
#define MAX_BG_COUNT 0x7fffffff

#define MIN_BG_SLEEP 1
#define MAX_BG_SLEEP 1000

#define DEFAULT_LIMIT_RATE 1

struct net_stat {
    unsigned long time;
    unsigned long read_count;
    unsigned long write_count;
};

struct net_stat_x {
    unsigned long time;
    unsigned long read_count;
    unsigned long total_read_len;
    unsigned long write_count;
    unsigned long total_write_len;
};


struct network_info {
    spinlock_t fg_lock;
    spinlock_t bg_lock;
    int fg_num;
    int fg_uids[MAX_UID_NUM];
    int bg_num;
    int bg_uids[MAX_UID_NUM];
    atomic_t bg_limit;
    struct net_stat fg_net_stat[MAX_FG_NET_STAT];
    struct net_stat_x bg_net_stat[MAX_FG_NET_STAT];
};

typedef struct tagAwareCtrl{
    volatile int enable;
    volatile int mode;
    volatile int limit_rate;
    volatile int limit_ratio;
    volatile int package_ratio;
}AwareCtrl;


void tcp_network_aware(bool isRecving);
void reinit_bg_stats(void);
void reinit_fg_stats(void);
void stat_bg_network_flow(bool isRecving, int len);


void reinit_ctrl_policy(int limit_ratio, int package_ratio);


#endif

