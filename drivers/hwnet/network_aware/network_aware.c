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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>

#include <linux/slab.h>
#include <linux/seq_file.h>

#include <network_aware/network_aware.h>


// if happened in PERIOD_TIME jiffies, save in the same node; otherwise, create a new node
#define PERIOD_TIME 1

// when fg stat is beyond it, limit bg
#define FG_RANGE_COUNT 0

// us for bg to sleep
#define SLEEP_TIME_MS 10

static volatile unsigned long s_last_fg_time = 0;
static volatile unsigned long s_cur_fg_index = 0;

static unsigned long total_fg_rxtx_count = 0;

static volatile unsigned long s_cur_bg_index = 0;
static volatile unsigned long s_last_bg_time = 0;

static unsigned long s_time_long = 0;

static unsigned long cur_bg_rate = 0;
static unsigned long avg_bg_packet_long = 0;
static unsigned long total_bg_rxtx_size = 0;
static unsigned long total_bg_rxtx_count = 0;


AwareCtrl s_AwareNetCtrl = {
    .enable = 1,
    .mode = 1,
    .limit_rate = DEFAULT_LIMIT_RATE,
    .limit_ratio = 100,
     .package_ratio = 100,
};

struct network_info netinfo = {
    .fg_num = 0,
    .fg_uids = {0, },
    .bg_num = 0,
    .bg_uids = {0, },
    .fg_net_stat = {{0, }, },
    .bg_net_stat = {{0, }, },
};


struct bg_ctrl_policy_t{
    unsigned long package_count;
    unsigned int sleep_long;
};

static struct bg_ctrl_policy_t s_AwareNetBgCtrlPolicy[] = {
    {20, 1},
    {50, 10},
    {100, 25},
    {200, 10},
    {500, 25},
    {1000, 50},
    {1000, 100},
    {0x1fffffff, 100}
};

static bool is_bg_limit_enabled(void) {
    bool ret;
    ret = true;
    if (s_AwareNetCtrl.enable == 0){
        ret = false;
    }
    spin_lock(&(netinfo.fg_lock));
    if (netinfo.fg_uids[0] < 0){
        ret = false;
    }
    spin_unlock(&(netinfo.fg_lock));
    return ret;
}

static bool is_fg(int uid) {
    int i;
    bool ret;
    spin_lock(&(netinfo.fg_lock));
    ret = false;
    for (i = 0; i < netinfo.fg_num; i++) {
        if (uid == netinfo.fg_uids[i]) {
            ret = true;
            break;
        }
    }
    spin_unlock(&(netinfo.fg_lock));
    return ret;
}

static bool is_bg(int uid) {
    int i;
    bool ret;
    spin_lock(&(netinfo.bg_lock));
    ret = false;
    for (i = 0; i < netinfo.bg_num; i++) {
        if (uid == netinfo.bg_uids[i]) {
            ret = true;
            break;
        }
    }
    spin_unlock(&(netinfo.bg_lock));
    return ret;
}

// if fg net read or write count is beyond RANGE_COUNT, limit bg network
static void limit_bg(void) {
    int i;
    int level = 0;
    unsigned long cur_time;
    unsigned long network_sum = 0;
    unsigned int sleep_long = SLEEP_TIME_MS;

    if (!is_bg_limit_enabled()) {
        return;
    }

    cur_time = jiffies;
    if (s_AwareNetCtrl.mode < 7){
        level = s_AwareNetCtrl.mode-1;
        sleep_long = s_AwareNetBgCtrlPolicy[level].sleep_long;
    }else{
        spin_lock(&(netinfo.fg_lock));
        for (i = 0; i < MAX_FG_NET_STAT; i++) {
            struct net_stat *tmp_for_cnt = &netinfo.fg_net_stat[i];
            if (tmp_for_cnt->time + MAX_FG_NET_STAT * PERIOD_TIME < cur_time) {
                continue;
            }
            network_sum += tmp_for_cnt->read_count;
            network_sum += tmp_for_cnt->write_count;
        }
        spin_unlock(&(netinfo.fg_lock));
        total_fg_rxtx_count = network_sum;
        if (total_fg_rxtx_count == 0){
            return;
        }

        unsigned long oldest_time;
        unsigned long last_time = 0;
        unsigned long total_len_sum = 0;
        network_sum = 0;
        oldest_time = cur_time;
        spin_lock(&(netinfo.bg_lock));
        for (i = 0; i < MAX_FG_NET_STAT; i++) {
            struct net_stat_x *tmp_for_cnt = &netinfo.bg_net_stat[i];
            if (tmp_for_cnt->time + MAX_FG_NET_STAT < cur_time) {
                continue;
            }
            network_sum += tmp_for_cnt->read_count;
            network_sum += tmp_for_cnt->write_count;
            total_len_sum += tmp_for_cnt->total_read_len;
            total_len_sum += tmp_for_cnt->total_write_len;
            if ((oldest_time > tmp_for_cnt->time) && (tmp_for_cnt->time != 0)){
                oldest_time = tmp_for_cnt->time;
            }
            if (last_time < tmp_for_cnt->time){
                last_time = tmp_for_cnt->time;
            }
        }
        spin_unlock(&(netinfo.bg_lock));
        if (network_sum < MIN_BG_PACKAGE_COUNT)return;

        total_bg_rxtx_count = network_sum;
        total_bg_rxtx_size = total_len_sum;
        s_time_long = (last_time - oldest_time + 1)*TIME_LONG_OF_HZ;        ///time in ms
        avg_bg_packet_long = total_bg_rxtx_size/total_bg_rxtx_count;
        cur_bg_rate = total_bg_rxtx_size/s_time_long;            ///rate in kbytes

        if (cur_bg_rate < s_AwareNetCtrl.limit_rate){
            return;
        }

        level = s_AwareNetCtrl.mode - 1;
        sleep_long = s_AwareNetBgCtrlPolicy[level].sleep_long;
    }
    atomic_inc_unless_negative(&netinfo.bg_limit);
    msleep_interruptible(sleep_long);
    atomic_dec_if_positive(&netinfo.bg_limit);
    return;
}

static void update_fg_net_stat(bool isRecving) {
    struct net_stat *p_fg_net_stat;
    unsigned long cur_time;
    unsigned long period;
    spin_lock(&(netinfo.fg_lock));
    cur_time = jiffies;

    // first time, starts at 0
    if (s_last_fg_time == 0) {
        s_cur_fg_index = 0;
        p_fg_net_stat = &netinfo.fg_net_stat[0];
        p_fg_net_stat->time = cur_time;
        if (isRecving) {
            p_fg_net_stat->read_count = 1;
            p_fg_net_stat->write_count = 0;
        } else {
            p_fg_net_stat->read_count = 0;
            p_fg_net_stat->write_count = 1;
        }
        goto out;
    }

    period = (cur_time - s_last_fg_time) / PERIOD_TIME;
    if (period == 0) {
        // update cur data
        p_fg_net_stat = &netinfo.fg_net_stat[s_cur_fg_index];
        isRecving ? p_fg_net_stat->read_count++ : p_fg_net_stat->write_count++;
    } else {
        int i;
        // clear data during last time and cur time
        if (period > MAX_FG_NET_STAT) {
            period = MAX_FG_NET_STAT;
        }
        for (i = 1; i < period; i++) {
            int tmp_index = (s_cur_fg_index + i) % MAX_FG_NET_STAT;
            p_fg_net_stat = &netinfo.fg_net_stat[tmp_index];
            p_fg_net_stat->read_count = 0;
            p_fg_net_stat->write_count = 0;
            p_fg_net_stat->time = 0;
        }
        // update cur data
        s_cur_fg_index= (s_cur_fg_index + period) % MAX_FG_NET_STAT;
        p_fg_net_stat = &netinfo.fg_net_stat[s_cur_fg_index];
        if (isRecving) {
            p_fg_net_stat->read_count = 1;
            p_fg_net_stat->write_count = 0;
        } else {
            p_fg_net_stat->read_count = 0;
            p_fg_net_stat->write_count = 1;
        }
        p_fg_net_stat->time = cur_time;
    }

out:
    s_last_fg_time = cur_time;
    spin_unlock(&(netinfo.fg_lock));
    return;
}

void tcp_network_aware(bool isRecving) {
    int cur_uid;
    if (s_AwareNetCtrl.enable == 0){
        return;
    }

    cur_uid= current_uid().val;
    if (is_fg(cur_uid)) {
        update_fg_net_stat(isRecving);
        return;
    }

    if (is_bg(cur_uid)) {
        limit_bg();
    }
}



void stat_bg_network_flow_x(bool isRecving, int len)
{
    unsigned long period;
    unsigned long cur_time;
    struct net_stat_x *temp_net_stat_x;

    // first time, starts at 0
    spin_lock(&(netinfo.bg_lock));
    cur_time = jiffies;
    if (s_last_bg_time == 0) {
        goto out;
    }

    period = (cur_time - s_last_bg_time);
    if (period == 0) {
        // update cur data
        goto out_2;
    } else {
        int tmp_index;
        int i;
        // clear data during last time and cur time
        if (period > MAX_FG_NET_STAT) {
            period = MAX_FG_NET_STAT;
        }
        for (i = 1 ; i < period; i++) {
            tmp_index = (s_cur_bg_index + i) % MAX_FG_NET_STAT;
            temp_net_stat_x = &netinfo.bg_net_stat[tmp_index];
            memset(temp_net_stat_x, 0, sizeof(*temp_net_stat_x));
        }
        // update cur data
        s_cur_bg_index = (s_cur_bg_index + period) % MAX_FG_NET_STAT;
    }

out:
    s_last_bg_time = cur_time;
out_2:
    temp_net_stat_x = &netinfo.bg_net_stat[s_cur_bg_index];

    temp_net_stat_x->time = s_last_bg_time;
    if (isRecving){
        temp_net_stat_x->read_count ++;
        temp_net_stat_x->total_read_len += len;
    }else{
        temp_net_stat_x->write_count ++;
        temp_net_stat_x->total_write_len += len;
    }
    spin_unlock(&(netinfo.bg_lock));
    return;
}

void stat_bg_network_flow(bool isRecving, int len)
{
    int cur_uid;

    cur_uid = current_uid().val;
    if (!is_bg(cur_uid)) {
        return;
    }

    stat_bg_network_flow_x(isRecving, len);
}

void reinit_bg_stats(void)
{
    s_cur_bg_index = 0;
    s_last_bg_time = 0;
}


void reinit_fg_stats()
{
    s_last_fg_time = 0;
    s_cur_fg_index = 0;
}

void reinit_ctrl_policy(int limit_ratio, int package_ratio)
{
    int i;
    int count = sizeof(s_AwareNetBgCtrlPolicy)/sizeof(s_AwareNetBgCtrlPolicy[0]);
    if (limit_ratio < 1){
        return;
    }
    if (package_ratio < 1){
        return;
    }
    for (i = 0; i < count; i++) {
        s_AwareNetBgCtrlPolicy[i].package_count = s_AwareNetBgCtrlPolicy[i].package_count
            * (package_ratio/s_AwareNetCtrl.package_ratio);
        if (s_AwareNetBgCtrlPolicy[i].package_count < MIN_BG_COUNT){
            s_AwareNetBgCtrlPolicy[i].package_count = MIN_BG_COUNT;
        }else if (s_AwareNetBgCtrlPolicy[i].package_count > MAX_BG_COUNT){
            s_AwareNetBgCtrlPolicy[i].package_count = MAX_BG_COUNT;
        }
        s_AwareNetCtrl.package_ratio = package_ratio;
        s_AwareNetBgCtrlPolicy[i].sleep_long = s_AwareNetBgCtrlPolicy[i].sleep_long
            *(limit_ratio/s_AwareNetCtrl.limit_ratio);

        if (s_AwareNetBgCtrlPolicy[i].sleep_long < MIN_BG_SLEEP){
            s_AwareNetBgCtrlPolicy[i].sleep_long = MIN_BG_SLEEP;
        }else if (s_AwareNetBgCtrlPolicy[i].sleep_long > MAX_BG_SLEEP){
            s_AwareNetBgCtrlPolicy[i].sleep_long = MAX_BG_SLEEP;
        }
        s_AwareNetCtrl.limit_ratio = limit_ratio;
    }

}


void dump_aware_net_stats(struct seq_file *m)
{
    unsigned long time = jiffies%(HZ *5);
    int pos = 100;
    int index;

    seq_printf(m, "fg_stats index:%lu  last_time:%lu\n",
        s_cur_fg_index,
        s_last_fg_time);

    seq_printf(m, "bg_stats rate:%lu  avg_long:%lu size:%lu count:(bg:%lu, fg:%lu) time_long:%lu\n",
        cur_bg_rate,
        avg_bg_packet_long,
        total_bg_rxtx_size,
        total_bg_rxtx_count, total_fg_rxtx_count,
        s_time_long);

    ///print one  time in 5
    if (time > pos && time < (pos + HZ)){
        for (index = 0; index < MAX_FG_NET_STAT; index++) {
            struct net_stat_x *tmp_for_cnt = &netinfo.bg_net_stat[index];
            if (tmp_for_cnt->time == 0){
                continue;
            }
            seq_printf(m, "time:%lu rp:%lu rb:%lu tp:%lu tb:%lu\n", 
                tmp_for_cnt->time,
                tmp_for_cnt->read_count,
                tmp_for_cnt->total_read_len,
                tmp_for_cnt->write_count,
                tmp_for_cnt->total_write_len);
        }
    }
}

