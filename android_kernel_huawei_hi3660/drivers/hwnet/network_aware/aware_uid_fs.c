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


#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/syslog.h>
#include <linux/seq_file.h>

#include <net/net_namespace.h>

#include <../fs/proc/internal.h>
#include <network_aware/network_aware.h>

extern struct network_info netinfo;
extern AwareCtrl s_AwareNetCtrl;

static int fg_uids_show(struct seq_file *m, void *v)
{
    int i;
    for (i = 0; i < netinfo.fg_num; i++) {
      seq_printf(m, "fg_uids[%d]: %d\n", i, netinfo.fg_uids[i]);
    }
    return 0;
}

static int fg_uids_open(struct inode *inode, struct file *filp)
{
   return single_open(filp, fg_uids_show, inode);
}

static ssize_t fg_uids_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *ppos)
{
    char buffer[MAX_ARRAY_LENGTH];
    char* cur;
    char* p;
    int err = 0;
    int i = 0;

    memset(buffer, 0, sizeof(buffer));
    if (count > sizeof(buffer) - 1)
        count = sizeof(buffer) - 1;
    if (copy_from_user(buffer, buf, count)) {
        err = -EFAULT;
        goto out;
    }
    cur = buffer;

    spin_lock(&(netinfo.fg_lock));
    p = strsep(&cur, ";");
    for (i = 0; i < MAX_UID_NUM; i++) {
        if (p == NULL) {
            break;
        }
        netinfo.fg_uids[i] = simple_strtol(p, NULL, 0);
        p = strsep(&cur, ";");
    }
    netinfo.fg_num = i;
    for (i = 0; i < MAX_FG_NET_STAT; i++) {
        netinfo.fg_net_stat[i].read_count = 0;
        netinfo.fg_net_stat[i].write_count = 0;
        netinfo.fg_net_stat[i].time = 0;
    }

    reinit_fg_stats();

    spin_unlock(&(netinfo.fg_lock));

out:
    return err < 0 ? err : count;
}

static int bg_uids_show(struct seq_file *m, void *v)
{
    int i;
    for (i = 0; i < netinfo.bg_num; i++) {
      seq_printf(m, "bg_uids[%d]: %d\n", i, netinfo.bg_uids[i]);
    }
    seq_printf(m, "bg_is_limit:%d\n", atomic_read(&(netinfo.bg_limit)));
    return 0;
}

static int bg_uids_open(struct inode *inode, struct file *filp)
{
   return single_open(filp, bg_uids_show, inode);
}

static ssize_t bg_uids_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *ppos)
{
    char buffer[MAX_ARRAY_LENGTH];
    char* cur;
    char* p;

    int err = 0;
    int i = 0;

    memset(buffer, 0, sizeof(buffer));
    if (count > sizeof(buffer) - 1)
        count = sizeof(buffer) - 1;
    if (copy_from_user(buffer, buf, count)) {
        err = -EFAULT;
        goto out;
    }
    cur = buffer;

    spin_lock(&(netinfo.bg_lock));
    p = strsep(&cur, ";");
    for (i = 0; i < MAX_UID_NUM; i++) {
        if (p == NULL) {
            break;
        }
        netinfo.bg_uids[i] = simple_strtol(p, NULL, 0);
        p = strsep(&cur, ";");
    }
    netinfo.bg_num = i;

    memset(&netinfo.bg_net_stat, 0, sizeof(netinfo.bg_net_stat));

    reinit_bg_stats();

    spin_unlock(&(netinfo.bg_lock));

out:
    return err < 0 ? err : count;
}


static ssize_t aware_ctrl_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *ppos)
{
    int err = 0;
    char* cur;
    char* pos;
    char buffer[MAX_ARRAY_LENGTH];

    memset(buffer, 0, sizeof(buffer));
    if (count > sizeof(buffer) - 1)
        count = sizeof(buffer) - 1;

    if (copy_from_user(buffer, buf, count)) {
        err = -EFAULT;
        goto out;
    }

    cur = buffer;
    pos = strchr(cur, ':');
    if (pos == NULL) {
        goto out;
    }

    cur = pos+1;
    s_AwareNetCtrl.mode = simple_strtol(cur, NULL, 10);
    s_AwareNetCtrl.enable = (0 == s_AwareNetCtrl.mode) ? 0 : 1;

    int new_limit_ratio = 100;
    int new_package_ratio = 100;

    pos = strchr(cur, ':');
    if (pos == NULL) {
        s_AwareNetCtrl.limit_rate = DEFAULT_LIMIT_RATE;
        goto out;
    }

    cur = pos+1;
    s_AwareNetCtrl.limit_rate = simple_strtol(cur, NULL, 10);
    if (s_AwareNetCtrl.limit_rate < 0) {
        s_AwareNetCtrl.limit_rate = 0;
    }

    pos = strchr(cur, ':');
    if (pos == NULL) {
        goto out;
    }

    cur = pos+1;
    new_limit_ratio = simple_strtol(cur, NULL, 10);

    pos = strchr(cur, ':');
    if (pos == NULL) {
        goto out_1;
    }

    cur = pos+1;
    new_package_ratio = simple_strtol(cur, NULL, 10);

out_1:
    reinit_ctrl_policy(new_limit_ratio, new_package_ratio);


out:
    return err < 0 ? err : count;
}

extern void dump_aware_net_stats(struct seq_file *m);

static int aware_ctrl_show(struct seq_file *m, void *v)
{
    seq_printf(m, "aware_ctrl enable:%d mode:%d limit_rate:%dk limit_ratio:%d package_ratio:%d\n",
        s_AwareNetCtrl.enable,
        s_AwareNetCtrl.mode,
        s_AwareNetCtrl.limit_rate,
        s_AwareNetCtrl.limit_ratio,
        s_AwareNetCtrl.package_ratio);
    dump_aware_net_stats(m);

    return 0;
}

static int aware_ctrl_open(struct inode *inode, struct file *filp)
{
   return single_open(filp, aware_ctrl_show, inode);
}

static const struct file_operations proc_fg_uids_operations = {
   .open       = fg_uids_open,
   .read       = seq_read,
   .write      = fg_uids_write,
   .llseek     = seq_lseek,
   .release    = single_release,
};

static const struct file_operations proc_bg_uids_operations = {
   .open       = bg_uids_open,
   .read       = seq_read,
   .write      = bg_uids_write,
   .llseek     = seq_lseek,
   .release    = single_release,
};


static const struct file_operations proc_aware_ctrl_operations = {
   .open       = aware_ctrl_open,
   .read       = seq_read,
   .write      = aware_ctrl_write,
   .llseek     = seq_lseek,
   .release    = single_release,
};

#define AWARE_FS_NET_PATH "aware"
#define AWARE_FS_FG_UIDS "fg_uids"
#define AWARE_FS_BG_UIDS "bg_uids"
#define AWARE_FS_CTRL "aware_ctrl"

static void __net_init aware_uids_proc_fs_init(struct proc_dir_entry *p_parent)
{
    struct proc_dir_entry *p_temp1;
    struct proc_dir_entry *p_temp2;
    struct proc_dir_entry *p_temp3;
    if (NULL == p_parent){
        goto out_p_temp0;
    }

    p_temp1 = proc_create(AWARE_FS_BG_UIDS, S_IRUSR|S_IWUSR, p_parent, &proc_bg_uids_operations);
    if (NULL == p_temp1){
        goto out_p_temp0;
    }

    p_temp2 = proc_create(AWARE_FS_FG_UIDS, S_IRUSR|S_IWUSR, p_parent, &proc_fg_uids_operations);
    if (NULL == p_temp2){
        goto out_p_temp1;
    }

    p_temp3 = proc_create(AWARE_FS_CTRL, S_IRUSR|S_IWUSR, p_parent, &proc_aware_ctrl_operations);
    if (NULL == p_temp3){
        goto out_p_temp2;
    }

    spin_lock_init(&(netinfo.fg_lock));
    spin_lock_init(&(netinfo.bg_lock));

    atomic_set(&netinfo.bg_limit, 0);

    return ;
out_p_temp2:
    proc_remove(p_temp2);
out_p_temp1:
    proc_remove(p_temp1);
out_p_temp0:
    s_AwareNetCtrl.enable = 0;
    s_AwareNetCtrl.mode = 0;
    return ;
}

static __net_init int aware_net_init(struct net *net)
{
    struct proc_dir_entry *p_parent;
    if (NULL == net){
        return 0;
    }

    p_parent = proc_mkdir(AWARE_FS_NET_PATH, net->proc_net);
    if (NULL == p_parent){
        return -ENOMEM;
    }
    aware_uids_proc_fs_init(p_parent);
    return 0;
}

static  void aware_net_exit(struct net *net)
{
    if (NULL == net){
        return ;
    }
    remove_proc_entry(AWARE_FS_BG_UIDS, net->proc_net);
    remove_proc_entry(AWARE_FS_FG_UIDS, net->proc_net);
    remove_proc_entry(AWARE_FS_CTRL, net->proc_net);
    remove_proc_entry(AWARE_FS_NET_PATH, net->proc_net);
}

static __net_initdata struct pernet_operations aware_net_ops = {
    .init = aware_net_init,
    .exit = aware_net_exit,
};

static int __init aware_proc_init(void)
{
    return register_pernet_subsys(&aware_net_ops);
}

fs_initcall(aware_proc_init);

