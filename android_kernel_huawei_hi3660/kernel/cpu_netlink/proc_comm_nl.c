#include <cpu_netlink/cpu_netlink.h>
#define BINDER_THREAD_NAME "Binder:"

static void send_thread_comm_msg(int pid, int tgid)
{
    int dt[2];
    dt[0] = pid;
    dt[1] = tgid;
    send_to_user(PROC_COMM, 2, dt);
}


void iaware_proc_comm_connector(struct task_struct *task, const char *comm)
{
    int pid, tgid;
    if (!task || !comm || !strstr(comm, BINDER_THREAD_NAME))
    {
        return;
    }
    get_task_struct(task);
    pid = task->pid;
    tgid = task->tgid;
    put_task_struct(task);
    send_thread_comm_msg(pid, tgid);
}
