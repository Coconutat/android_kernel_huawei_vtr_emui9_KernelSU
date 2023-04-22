#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/time.h>
#include <linux/spinlock.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/route.h>
#include <net/inet_hashtables.h>
#include <net/net_namespace.h>
#include <linux/skbuff.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <uapi/linux/netlink.h>
#include <linux/kthread.h>
#include <linux/socket.h>
#include "ril_sim_netlink.h"

#undef HWLOG_TAG
#define HWLOG_TAG ril_sim_netlink
HWLOG_REGIST();
MODULE_LICENSE("GPL");

/************************************************************
    MOCRO   DEFINES
*************************************************************/
DEFINE_MUTEX(ril_sim_receive_sem);
DEFINE_MUTEX(ril_sim_send_sem);

#define KNL_SUCCESS     (0)
#define KNL_FAILED      (-1)

#define KNL_TRUE        (1)
#define KNL_FALSE       (0)

/************************************************************
    netlink variables for communicate between kernel and ril
*************************************************************/
/*netlink socket fd*/
static struct sock *g_ril_sim_nlfd = NULL;

/*save user space progress pid when receive req from ril.*/
static u32 g_ril_sim_req_pid = 0;

/*save user space message type when receive req from ril.*/
static u32 g_ril_sim_rcv_msg_type = 0;

/*save user space slot id when receive get_info_req from ril.*/
static int g_ril_sim_get_info_req_slot_id = -1;

static struct task_struct *g_ril_sim_netlink_task = NULL;

/*the semaphone is used when need thread deal with the message*/
static struct semaphore g_ril_sim_netlink_sync_sema;

/*this lock is used to protect global variable.*/
//static spinlock_t deact_sim_info_lock;

/* PIN info saved in kernel. */
static KNL_SIM_PIN_INFO gKnlSimPinInfo[MAX_SIM_CNT];

/************************************************************
      STATIC  FUNCTION  DEFINES
*************************************************************/

/* netlink socket's callback function,it will be called by system when
user space send a message to kernel.*/
static void kernel_ril_sim_receive(struct sk_buff *__skb)
{
    struct nlmsghdr         *nlh            = NULL;
    struct sk_buff          *skb            = NULL;
    KNL_SIM_PIN_INFO        *pinInfo        = NULL;
    KNL_RilSimCardInfo      *pCardInfo      = NULL;
    int                     slotId          = 0;

    hwlog_info("%s: enter\n", __func__);

    if (NULL == __skb)
    {
        hwlog_err("%s: Invalid parameter: NULL pointer reference(__skb)\n", __func__);
        return;
    }

    skb = skb_get(__skb);
    if (NULL == skb)
    {
        hwlog_err("%s: skb_get return NULL\n", __func__);
        return;
    }

    mutex_lock(&ril_sim_receive_sem);

    if (skb->len >= NLMSG_HDRLEN)
    {
        nlh = nlmsg_hdr(skb);
        if (NULL == nlh)
        {
            hwlog_err("%s: nlmsg_hdr return NULL\n", __func__);
            kfree_skb(skb);
            (void)mutex_unlock(&ril_sim_receive_sem);
            return;
        }

        if (nlh->nlmsg_pid <= 0)
        {
            hwlog_err("%s: invalid nlmsg_pid: 0x%x\n", __func__, nlh->nlmsg_pid);
            kfree_skb(skb);
            (void)mutex_unlock(&ril_sim_receive_sem);
            return;
        }

        if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr))
            && (skb->len >= nlh->nlmsg_len))
        {
            hwlog_info("%s: receive message form user space, nlmsg_type: %d, nlmsg_pid: %d\n", __func__,
                nlh->nlmsg_type, nlh->nlmsg_pid);

            if (NETLINK_KNL_SIM_UPDATE_IND_MSG == nlh->nlmsg_type)
            {
                pinInfo = (KNL_SIM_PIN_INFO *)nlmsg_data(nlh);
                if (NULL == pinInfo)
                {
                    hwlog_err("%s: pinInfo is NULL\n", __func__);
                    kfree_skb(skb);
                    mutex_unlock(&ril_sim_receive_sem);
                    return;
                }

                slotId = pinInfo->sim_slot_id;
                if ((slotId < 0) || (slotId >= MAX_SIM_CNT))
                {
                    hwlog_err("%s: NETLINK_KNL_SIM_UPDATE_IND_MSG with invalid slotId: %d\n", __func__, slotId);
                    kfree_skb(skb);
                    mutex_unlock(&ril_sim_receive_sem);
                    return;
                }

                /* 更新保存的PIN码信息 */
                memcpy(&gKnlSimPinInfo[slotId], pinInfo, sizeof(KNL_SIM_PIN_INFO));
            }
            else if (NETLINK_KNL_SIM_GET_PIN_INFO_REQ == nlh->nlmsg_type)
            {
                pCardInfo = (KNL_RilSimCardInfo *)nlmsg_data(nlh);
                if (NULL == pCardInfo)
                {
                    hwlog_err("%s: pCardInfo is NULL\n", __func__);
                    kfree_skb(skb);
                    mutex_unlock(&ril_sim_receive_sem);
                    return;
                }

                slotId = pCardInfo->sim_slot_id;
                if ((slotId < 0) || (slotId >= MAX_SIM_CNT))
                {
                    hwlog_err("%s: NETLINK_KNL_SIM_GET_PIN_INFO_REQ with invalid slotId: %d\n", __func__, slotId);
                    kfree_skb(skb);
                    mutex_unlock(&ril_sim_receive_sem);
                    return;
                }

                g_ril_sim_rcv_msg_type          = NETLINK_KNL_SIM_GET_PIN_INFO_REQ;
                g_ril_sim_req_pid               = nlh->nlmsg_pid;
                g_ril_sim_get_info_req_slot_id  = slotId;

                up(&g_ril_sim_netlink_sync_sema);
            }
            else if (NETLINK_KNL_SIM_QUERY_VARIFIED_REQ == nlh->nlmsg_type)
            {
                g_ril_sim_rcv_msg_type          = NETLINK_KNL_SIM_QUERY_VARIFIED_REQ;
                g_ril_sim_req_pid               = nlh->nlmsg_pid;

                up(&g_ril_sim_netlink_sync_sema);

                hwlog_info("%s: receive NETLINK_KNL_SIM_QUERY_VARIFIED_REQ success\n", __func__);
            }
            else if (NETLINK_KNL_SIM_CLEAR_ALL_VARIFIED_FLG == nlh->nlmsg_type)
            {
                for (slotId = 0; slotId < MAX_SIM_CNT; ++slotId)
                {
                    gKnlSimPinInfo[slotId].is_verified = KNL_FALSE;
                }
                hwlog_info("%s: clear is_verified success\n", __func__);
            }
            else
            {
                hwlog_err("%s: Invalid message from RIL, nlmsg_type: %d\n", __func__, nlh->nlmsg_type);
            }
        }
    }
    else
    {
        hwlog_err("%s: Invalid len message, skb->len: %d\n", __func__, skb->len);
    }
    hwlog_info("%s: exit, skb->len: %d\n", __func__, skb->len);

    kfree_skb(skb);
    mutex_unlock(&ril_sim_receive_sem);

}

int ril_sim_send_get_pin_info_cnf(u32 pid, int slotId)
{
    int ret                          = KNL_SUCCESS;
    int size                         = 0;
    struct sk_buff  *skb             = NULL;
    struct nlmsghdr *nlh             = NULL;

    mutex_lock(&ril_sim_send_sem);

    if (!pid || !g_ril_sim_nlfd)
    {
        hwlog_err("%s: Invalid pid or g_ril_sim_nlfd, pid: 0x%x\n", __func__, pid);
        ret = KNL_FAILED;
        goto end;
    }

    if ((slotId < 0) || (slotId >= MAX_SIM_CNT))
    {
        hwlog_err("%s: Invalid slotId: %d\n", __func__, slotId);
        ret = KNL_FAILED;
        goto end;
    }

    size = sizeof(KNL_SIM_PIN_INFO);
    /* 新申请一个socket buffer ，其大小为: socket消息头大小 + netlink 消息头大小 + 用户消息大小 */
    skb  = alloc_skb(size, GFP_ATOMIC);
    if (!skb)
    {
        hwlog_err("%s: alloc skb fail\n", __func__);
        ret = KNL_FAILED;
        goto end;
    }

    /* 填充部分netlink消息头: skb, pid, seq, type, len, flags */
    nlh = nlmsg_put(skb, pid, 0, NETLINK_KNL_SIM_GET_PIN_INFO_CNF, size, 0);
    if (!nlh)
    {
        hwlog_err("%s: nlmsg_put return fail\n", __func__);
        kfree_skb(skb);
        skb = NULL;
        ret = KNL_FAILED;
        goto end;
    }

    /* From kernel */
    NETLINK_CB(skb).portid = 0;
    /* 如果目的组为内核或某一进程，该字段也置0 */
    NETLINK_CB(skb).dst_group = 0;

    /* 填充用户区数据 */
    memcpy(nlmsg_data(nlh), &gKnlSimPinInfo[slotId], size);

    /*skb will be freed in netlink_unicast*/
    ret = netlink_unicast(g_ril_sim_nlfd, skb, pid, MSG_DONTWAIT);
    hwlog_info("%s: send NETLINK_KNL_SIM_GET_PIN_INFO_CNF to RIL, pid: %d, ret: %d\n", __func__, pid, ret);

end:
    mutex_unlock(&ril_sim_send_sem);
    return ret;
}

int ril_sim_send_query_verified_cnf(u32 pid)
{
    int ret                          = KNL_SUCCESS;
    int size                         = 0;
    int slotIdx                      = 0;
    int is_verified                  = KNL_TRUE;
    struct sk_buff  *skb             = NULL;
    struct nlmsghdr *nlh             = NULL;

    mutex_lock(&ril_sim_send_sem);

    if (!pid || !g_ril_sim_nlfd)
    {
        hwlog_err("%s: Invalid pid or g_ril_sim_nlfd, pid: 0x%x\n", __func__, pid);
        ret = KNL_FAILED;
        goto end;
    }

    size = sizeof(int);
    /* 新申请一个socket buffer ，其大小为:socket消息头大小 + netlink 消息头大小 + 用户消息大小*/
    skb  = alloc_skb(size, GFP_ATOMIC);
    if (!skb)
    {
        hwlog_err("%s: alloc skb fail\n", __func__);
        ret = KNL_FAILED;
        goto end;
    }

    /*填充部分netlink消息头: skb, pid, seq, type, len, flags*/
    nlh = nlmsg_put(skb, pid, 0, NETLINK_KNL_SIM_QUERY_VARIFIED_CNF, size, 0);
    if (!nlh)
    {
        hwlog_err("%s: nlmsg_put return fail\n", __func__);
        kfree_skb(skb);
        skb = NULL;
        ret = KNL_FAILED;
        goto end;
    }

    /* From kernel */
    NETLINK_CB(skb).portid = 0;
    /*如果目的组为内核或某一进程，该字段也置0 */
    NETLINK_CB(skb).dst_group = 0;

    /*填充用户区数据*/
    for (slotIdx = 0; slotIdx < MAX_SIM_CNT; ++slotIdx)
    {
        /* PIN信息有效，但未做PIN码校验，返回FALSE */
        if ((KNL_TRUE == gKnlSimPinInfo[slotIdx].is_valid_flg)
            && (KNL_FALSE == gKnlSimPinInfo[slotIdx].is_verified))
        {
            is_verified = KNL_FALSE;
            break;
        }
    }

    memcpy(nlmsg_data(nlh), &is_verified, size);

    /*skb will be freed in netlink_unicast*/
    ret = netlink_unicast(g_ril_sim_nlfd, skb, pid, MSG_DONTWAIT);
    hwlog_info("%s: send NETLINK_KNL_SIM_QUERY_VARIFIED_CNF to RIL, pid: %d, is_verified: %d, ret: %d\n",
        __func__, pid, is_verified, ret);

end:
    mutex_unlock(&ril_sim_send_sem);
    return ret;
}

static int ril_sim_netlink_thread(void *data)
{
    hwlog_info("%s: enter\n", __func__);

    while (1)
    {
        if (kthread_should_stop())
        {
            break;
        }

        /*netlink thread will block at this semaphone when no message received,
        only receive message from user space and then up the sema, this thread will go to next sentence.*/
        down(&g_ril_sim_netlink_sync_sema);

        hwlog_info("%s: g_ril_sim_rcv_msg_type: %d\n", __func__, g_ril_sim_rcv_msg_type);

        if (NETLINK_KNL_SIM_GET_PIN_INFO_REQ == g_ril_sim_rcv_msg_type)
        {
            hwlog_info("%s: return NETLINK_KNL_SIM_GET_PIN_INFO_CNF to RIL, PID: %d, slot id: %d\n", __func__,
                g_ril_sim_req_pid, g_ril_sim_get_info_req_slot_id);

            (void)ril_sim_send_get_pin_info_cnf(g_ril_sim_req_pid, g_ril_sim_get_info_req_slot_id);
        }
        else if (NETLINK_KNL_SIM_QUERY_VARIFIED_REQ == g_ril_sim_rcv_msg_type)
        {
            hwlog_info("%s: return NETLINK_KNL_SIM_QUERY_VARIFIED_CNF to RIL, PID: %d\n", __func__,
                g_ril_sim_req_pid);

            (void)ril_sim_send_query_verified_cnf(g_ril_sim_req_pid);
        }
    }

    hwlog_info("%s: end\n", __func__);
    return 0;
}

/*netlink init function.*/
static void ril_sim_netlink_init(void)
{
    struct netlink_kernel_cfg ril_nl_cfg =
    {
        .input = kernel_ril_sim_receive,
    };

    hwlog_info("%s: enter\n", __func__);

    g_ril_sim_nlfd = netlink_kernel_create(&init_net,
        NETLINK_RIL_EVENT_SIM,
        &ril_nl_cfg);
    if (!g_ril_sim_nlfd)
    {
        hwlog_err("%s: netlink_kernel_create fail\n", __func__);
    }
    else
    {
        hwlog_info("%s: netlink_kernel_create success\n", __func__);
    }

    sema_init(&g_ril_sim_netlink_sync_sema, 0);

    memset(gKnlSimPinInfo, 0, MAX_SIM_CNT * sizeof(KNL_SIM_PIN_INFO));

    g_ril_sim_netlink_task = kthread_run(ril_sim_netlink_thread, NULL, "ril_sim_netlink_thread");

    hwlog_info("%s: end\n", __func__);
}

/*netlink deinit function.*/
static void ril_sim_netlink_deinit(void)
{
    if (g_ril_sim_nlfd && g_ril_sim_nlfd->sk_socket)
    {
        sock_release(g_ril_sim_nlfd->sk_socket);
        g_ril_sim_nlfd = NULL;
    }

    if (g_ril_sim_netlink_task)
    {
        kthread_stop(g_ril_sim_netlink_task);
        g_ril_sim_netlink_task = NULL;
    }
}

static int __init ril_sim_netlink_module_init(void)
{
    ril_sim_netlink_init();
    return 0;
}

static void __exit ril_sim_netlink_module_exit(void)
{
    ril_sim_netlink_deinit();
    return;
}

module_init(ril_sim_netlink_module_init);
module_exit(ril_sim_netlink_module_exit);

