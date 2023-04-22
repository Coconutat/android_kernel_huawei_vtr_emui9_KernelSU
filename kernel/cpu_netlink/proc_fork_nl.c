#include <cpu_netlink/cpu_netlink.h>
#define MAX_FORK_TIME 30

static int get_static_vip(struct task_struct* task)
{
    int vip = 0;
    get_task_struct(task);
    vip = task->static_vip;
    put_task_struct(task);
    return vip;
}

static pid_t get_pid_of_task(struct task_struct* task)
{
    pid_t pid = 0;
    get_task_struct(task);
    pid = task->pid;
    put_task_struct(task);
    return pid;
}

static pid_t get_tgid_of_task(struct task_struct* task)
{
    pid_t tgid = 0;
    get_task_struct(task);
    tgid = task->tgid;
    put_task_struct(task);
    return tgid;
}

static void send_vip_msg(int pid, int tgid)
{
    int dt[2];
    dt[0] = pid;
    dt[1] = tgid;
    send_to_user(PROC_FORK, 2, dt);
}

void iaware_proc_fork_connector(struct task_struct *task)
{
    pid_t cur_pid, cur_tgid;
    pid_t pid, ppid, tgid;
    int vip = 0;
    struct task_struct *parent;
    struct task_struct *temp_task = task;
    struct task_struct *tg_task;
    int time = 0;

    if (!task)
        return;

    cur_pid = pid = get_pid_of_task(task);
    cur_tgid = tgid = get_tgid_of_task(task);

    rcu_read_lock();
    do
    {
        if (pid != tgid)
        {
            tg_task = find_task_by_vpid(tgid);
            if (!tg_task)
                break;
            vip = get_static_vip(tg_task);

            if (vip == 1)
                break;
        }

        parent = rcu_dereference(temp_task->real_parent); /*lint !e1058 !e64*/
        if (!parent)
            break;
        vip = get_static_vip(parent);
        ppid = pid = get_pid_of_task(parent);
        tgid = get_tgid_of_task(parent);

        if (vip == 1)
            break;
        temp_task = parent;
    } while (ppid > 1 && time++ < MAX_FORK_TIME);
    rcu_read_unlock();

    if (vip == 1)
    {
        send_vip_msg(cur_pid, cur_tgid);
    }
}
