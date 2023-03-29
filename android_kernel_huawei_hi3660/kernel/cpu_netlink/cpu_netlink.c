#include <cpu_netlink/cpu_netlink.h>

#define MAX_DATA_LEN 10

static __u32 pid;
static struct sock *netlinkfd;

int send_to_user(int sock_no, size_t len, int *data)
{
    int ret = -1;
    int i;
    int num = len + 2;
    int size;
    int dt[MAX_DATA_LEN];
    struct sk_buff *skb = NULL;
    struct nlmsghdr *nlh = NULL;
    if (!data)
        return ret;

    if (IS_ERR_OR_NULL(netlinkfd))
        return ret;

    if (pid == 0)
        return ret;

    if (num > MAX_DATA_LEN)
    {
        printk(KERN_ERR "cpu_netlink send oversize(%d,MAX:%d) data!\n", num, MAX_DATA_LEN);
        return ret;
    }

    dt[0] = sock_no;
    dt[1] = len;
    /*lint -save -e440 -e679*/
    for (i = 0; i + 2 < num; i++)
    {
        dt[i + 2] = data[i];
    }
    /*lint -restore*/
    size = sizeof(int) * num;

    skb = alloc_skb(NLMSG_SPACE(size), GFP_ATOMIC);
    if (IS_ERR_OR_NULL(skb)) {
        printk(KERN_ERR "cpu_netlink %s: alloc skb failed!\n", __func__);
        return -ENOMEM;
    }
    nlh = nlmsg_put(skb, 0, 0, 0, NLMSG_SPACE(size) - sizeof(struct nlmsghdr), 0);
    memcpy(NLMSG_DATA(nlh), (void *)dt, size);
    /*send up msg*/
    ret = netlink_unicast(netlinkfd, skb, pid, MSG_DONTWAIT);
    if (ret < 0) {
        printk(KERN_ERR "cpu_netlink: send_to_user netlink_unicast failed!\n");
    }

    return ret;
}

static void recv_from_user(struct sk_buff *skb)
{
    struct sk_buff *tmp_skb = NULL;
    struct nlmsghdr *nlh = NULL;

    if (IS_ERR_OR_NULL(skb)) {
        printk(KERN_ERR "cpu_netlink: recv_from_user: skb is NULL!\n");
        return;
    }

    tmp_skb = skb_get(skb);

    if (tmp_skb->len >= NLMSG_SPACE(0)) {
        nlh = nlmsg_hdr(tmp_skb);
        pid = nlh->nlmsg_pid;
    }
}

static int create_cpu_netlink(int socket_no)
{
    int ret = 0;
    struct netlink_kernel_cfg cfg = {
        .groups = 0,
        .input = recv_from_user,
    };
    netlinkfd = netlink_kernel_create(&init_net, socket_no, &cfg);
    if (IS_ERR_OR_NULL(netlinkfd)) {
        ret = PTR_ERR(netlinkfd);
        printk(KERN_ERR "cpu_netlink: create cpu netlink error! ret is %d\n", ret);
    }
    return ret;
}

static void destroy_cpu_netlink(void)
{
    if (!IS_ERR_OR_NULL(netlinkfd) && !IS_ERR_OR_NULL(netlinkfd->sk_socket)) {
        sock_release(netlinkfd->sk_socket);
        netlinkfd = NULL;
    }
}

static int __init cpu_netlink_init(void)
{
    create_cpu_netlink(NETLINK_HW_IAWARE_CPU);
    return 0;
}

static void __exit cpu_netlink_exit(void)
{
    destroy_cpu_netlink();
}

module_init(cpu_netlink_init);
module_exit(cpu_netlink_exit);
