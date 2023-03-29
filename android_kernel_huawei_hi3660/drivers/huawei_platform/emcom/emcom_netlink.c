#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <net/ip.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <uapi/linux/netlink.h>
#include <linux/kthread.h>
#include "emcom_netlink.h"
#include "emcom_utils.h"
#ifdef CONFIG_HUAWEI_XENGINE
#include <huawei_platform/emcom/emcom_xengine.h>
#endif

#ifdef CONFIG_HUAWEI_NWEVAL
#include <huawei_platform/emcom/network_evaluation.h>
#endif

#ifdef CONFIG_HW_NETWORK_MEASUREMENT
#include <huawei_platform/emcom/smartcare/smartcare.h>
#endif

#undef HWLOG_TAG
#define HWLOG_TAG emcom_netlink
HWLOG_REGIST();
MODULE_LICENSE("GPL");

/************************************************************
                    MOCRO   DEFINES
*************************************************************/
DEFINE_MUTEX(emcom_receive_sem);
DEFINE_MUTEX(emcom_send_sem);
#define NL_SKB_QUEUE_MAXLEN    64

/********************************
    netlink variables for
    communicate between kernel and apk
*********************************/
static struct sock *g_emcom_nlfd = NULL; /*netlink socket fd*/
static uint32_t g_user_space_pid = 0; /*save user space progress pid when user space netlink socket registering.*/
static struct task_struct *g_emcom_netlink_task = NULL;
static int g_emcom_module_state = EMCOM_NETLINK_EXIT;
static struct semaphore g_emcom_netlink_sync_sema; /*tcp protocol use this semaphone to inform emcom netlink thread when data speed is slow*/
static spinlock_t socket_msg_lock; /*this lock is used to protect global variable.*/
static spinlock_t socket_state_lock; /*this lock is used to protect global variable.*/
/* Queue of skbs to send to emcomd */
static struct sk_buff_head emcom_skb_queue;

void emcom_send_msg2daemon(int cmd, const void*data, int len)
{
	struct nlmsghdr *nlh     = NULL;
	struct sk_buff  *skb_out = NULL;

	EMCOM_LOGD("emcom_send_msg2daemon: cmd  = %d.\n", cmd);

	if (g_emcom_module_state != EMCOM_NETLINK_INIT ||
	    skb_queue_len(&emcom_skb_queue) > NL_SKB_QUEUE_MAXLEN){
		EMCOM_LOGE(" emcom_send_msg2daemon: state wrong.\n");
		return;
	}

	/* May be called in any context. */
	skb_out = nlmsg_new(len, GFP_ATOMIC);
	if (!skb_out){
		EMCOM_LOGE(" emcom_send_msg2daemon: Out of memry.\n");
		return; /* Out of memory */
	}

	nlh = nlmsg_put(skb_out, 0, 0, cmd, len, 0);
	if (!nlh) {
		kfree_skb(skb_out);
		return; /* Out of memory */
	}

	NETLINK_CB(skb_out).dst_group = 0; /* For unicast */
	if (data && len > 0){
		memcpy((void *)nlmsg_data(nlh), (const void *)data, (size_t)len);
	}
	skb_queue_tail(&emcom_skb_queue, skb_out);
	up(&g_emcom_netlink_sync_sema);

	return;
}

/************************************************************
                    STATIC  FUNCTION  DEFINES
*************************************************************/
/*emcom common event process function. The event come frome emcom daemon.*/
static void emcom_common_evt_proc(struct nlmsghdr *nlh, uint8_t *pdata, uint16_t len)
{
    if( NULL == nlh )
    {
        return;
    }

    switch( nlh->nlmsg_type )
    {
        case NETLINK_EMCOM_DK_REG:
            /*save user space progress pid when register netlink socket.*/
            g_user_space_pid = nlh->nlmsg_pid;
            EMCOM_LOGD("emcom netlink receive reg packet: g_user_space_pid = %d\n",nlh->nlmsg_pid);
            #ifdef CONFIG_HUAWEI_NWEVAL
            nweval_on_dk_connected();
            #endif
#ifdef CONFIG_SMART_MP
            Emcom_Xengine_SmartMpOnDK_Connect();
#endif
            break;
        case NETLINK_EMCOM_DK_UNREG:
            EMCOM_LOGD("emcom netlink receive unreg packet\n");
            g_user_space_pid = 0;
            break;
        default:
            EMCOM_LOGI("emcom unsupport packet, the type is %d.\n", nlh->nlmsg_type);
            break;
    }
}


/* netlink socket's callback function,it will be called by system when user space send a message to kernel.
this function will save user space progress pid.*/
static void kernel_emcom_receive(struct sk_buff *__skb)
{
    struct nlmsghdr *nlh;
    struct sk_buff *skb;
    void *packet = NULL;
    uint16_t   data_len;
    uint8_t   subMod;

    EMCOM_LOGD("emcom: kernel_emcom_receive!\n");

    skb = skb_get(__skb);

    mutex_lock(&emcom_receive_sem);

    if (skb->len >= NLMSG_HDRLEN)
    {
        nlh = nlmsg_hdr(skb);
        packet = nlmsg_data(nlh);
        data_len = nlmsg_len(nlh);

        if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr))&&
            (skb->len >= nlh->nlmsg_len))
        {
            EMCOM_LOGD("emcom netlink receive a packet,nlmsg_type=%d\n",nlh->nlmsg_type);
            subMod  = ( nlh->nlmsg_type & EMCOM_SUB_MOD_MASK ) >> 8;
            switch(subMod)
            {
                case EMCOM_SUB_MOD_COMMON:
                    emcom_common_evt_proc(nlh,packet,data_len);
                    break;
                #ifdef CONFIG_HUAWEI_XENGINE
                case EMCOM_SUB_MOD_XENIGE:
                    Emcom_Xengine_EvtProc(nlh->nlmsg_type,packet,data_len);
                    break;
                #endif
                #ifdef CONFIG_HW_NETWORK_MEASUREMENT
                case EMCOM_SUB_MOD_SMARTCARE:
                    smartcare_event_process(nlh->nlmsg_type, packet, data_len);
                    break;
                #endif
                #ifdef CONFIG_HUAWEI_NWEVAL
                case EMCOM_SUB_MOD_NWEVAL:
                    nweval_event_process(nlh->nlmsg_type, packet, data_len);
                    break;
                #endif
                default:
                    EMCOM_LOGI("emcom netlink unsupport subMod, the subMod is %d.\n", subMod);
                    break;
            }
        }
    }
    mutex_unlock(&emcom_receive_sem);
}

/* netlink socket thread,
1.it will recieve the message from kernel;
2.maybe do some data process job;
3.send a message to user space;*/
static int emcom_netlink_thread(void* data)
{
	struct sk_buff *skb = NULL;

	while(1)
	{
		if(kthread_should_stop())
		{
			break;
		}

		/*netlink thread will block at this semaphone when no data coming.*/
		down(&g_emcom_netlink_sync_sema);
		EMCOM_LOGD("emcom_netlink_thread get sema success!\n");

		do {
			skb = skb_dequeue(&emcom_skb_queue);
			if (skb) {
				if (g_user_space_pid)
					netlink_unicast(g_emcom_nlfd, skb, g_user_space_pid, MSG_DONTWAIT);
				else
					kfree_skb(skb);
			}
		} while (!skb_queue_empty(&emcom_skb_queue));
	}
	return 0;
}

/*timer's expired process function.*/
#if 0
static void emcom_netlink_timer(unsigned long data)
{

}
#endif

/*netlink init function.*/
static void emcom_netlink_init(void)
{
    struct netlink_kernel_cfg emcom_nl_cfg = {
          .input = kernel_emcom_receive,
    };

    skb_queue_head_init(&emcom_skb_queue);
    g_emcom_nlfd = netlink_kernel_create(&init_net,
                                    NETLINK_EMCOM,
                                    &emcom_nl_cfg);
    if(!g_emcom_nlfd)
    EMCOM_LOGE(" %s: emcom_netlink_init failed\n",__func__);
    else
    EMCOM_LOGI("%s: emcom_netlink_init success\n",__func__);

    sema_init(&g_emcom_netlink_sync_sema, 0);
    spin_lock_init(&socket_msg_lock);
    spin_lock_init(&socket_state_lock);
    g_emcom_netlink_task = kthread_run(emcom_netlink_thread, NULL, "emcom_netlink_thread");

    return;
}

/*netlink deinit function.*/
static void emcom_netlink_deinit(void)
{
    if(g_emcom_nlfd && g_emcom_nlfd->sk_socket)
    {
        sock_release(g_emcom_nlfd->sk_socket);
        g_emcom_nlfd = NULL;
    }

    if(g_emcom_netlink_task)
    {
        kthread_stop(g_emcom_netlink_task);
        g_emcom_netlink_task = NULL;
    }
}

static int __init emcom_netlink_module_init(void)
{
    emcom_netlink_init();

    #ifdef CONFIG_HUAWEI_XENGINE
    Emcom_Xengine_Init();
    #endif

    #ifdef CONFIG_HUAWEI_NWEVAL
    nweval_init();
    #endif

    #ifdef CONFIG_HW_NETWORK_MEASUREMENT
    smartcare_init();
    #endif

    g_emcom_module_state = EMCOM_NETLINK_INIT;

    return 0;
}

static void __exit emcom_netlink_module_exit(void)
{
    g_emcom_module_state = EMCOM_NETLINK_EXIT;

    #ifdef CONFIG_HUAWEI_XENGINE
    Emcom_Xengine_clear();
    #endif

    #ifdef CONFIG_HW_NETWORK_MEASUREMENT
    smartcare_deinit();
    #endif

    emcom_netlink_deinit();
}

module_init(emcom_netlink_module_init);
module_exit(emcom_netlink_module_exit);
