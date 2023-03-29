#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/time.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <uapi/linux/netlink.h>
#include <linux/kthread.h>
#include "nb_netlink.h"
#include "video_acceleration.h"
#include "smart_switch.h"

#undef HWLOG_TAG
#define HWLOG_TAG nb_netlink
HWLOG_REGIST();
MODULE_LICENSE("GPL");

#define NB_NETLINK_EXIT 0
#define NB_NETLINK_INIT 1

DEFINE_MUTEX(nb_receive_sem);
DEFINE_MUTEX(nb_send_sem);

/* netlink socket fd */
static struct sock *g_nlfd;

/* save user space progress pid when user space netlink socket registering. */
static unsigned int g_user_space_pid;
static unsigned int g_native_space_pid;
static struct task_struct *g_netlink_task;
static int g_nb_module_state = NB_NETLINK_EXIT;

struct nb_event_info {
	enum nb_evt_type type;
	union {
		struct vod_event vod_info;
		struct ksi_event ksi_info;
		struct native_event native_info;
	};
};
static struct nb_event_info g_event_info;
static DEFINE_SPINLOCK(g_event_info_lock);

static struct semaphore g_event_sema;

static int __nb_notify_event(enum nb_evt_type event_type, void *data, int size);

void nb_notify_event(enum nb_evt_type event_type, void *data, int size)
{
	if (!data) {
		hwlog_err("%s:null data\n", __func__);
		return;
	}

	if (g_nb_module_state != NB_NETLINK_INIT) {
		hwlog_err("%s:module not inited\n", __func__);
		return;
	}

	spin_lock_bh(&g_event_info_lock);
	switch (event_type) {
		case NBMSG_VOD_EVT:
			if (sizeof(struct vod_event) != size) {
				hwlog_err("%s:wrong vod event size %d\n", __func__, size);
				spin_unlock_bh(&g_event_info_lock);
				return;
			}
			g_event_info.type = event_type;
			memcpy(&(g_event_info.vod_info), data, size);
			break;
		case NBMSG_KSI_EVT:
			if (sizeof(struct ksi_event) != size) {
				hwlog_err("%s:wrong ksi event size %d\n", __func__, size);
				spin_unlock_bh(&g_event_info_lock);
				return;
			}
			g_event_info.type = event_type;
			memcpy(&(g_event_info.ksi_info), data, size);
			break;
		case NBMSG_NATIVE_RTT:
			if (sizeof(struct native_event) != size) {
				hwlog_err("%s:wrong native state size %d\n", __func__, size);
				spin_unlock_bh(&g_event_info_lock);
				return;
			}
			g_event_info.type = event_type;
			memcpy(&(g_event_info.native_info), data, size);
			break;
		default:
			g_event_info.type = NBMSG_EVT_INVALID;
			hwlog_err("%s:unsupported event type %d\n", __func__, event_type);
			spin_unlock_bh(&g_event_info_lock);
			return;
	}
	spin_unlock_bh(&g_event_info_lock);

	up(&g_event_sema);
}

void process_vod_request(struct vod_request *request)
{
	if (!request)
		return;

	set_vod_enable(request->nf_hook_enable, request->nl_event_enable);
	hwlog_info("%s:process_vod_request hook=%d event=%d\n", __func__,
		request->nf_hook_enable, request->nl_event_enable);
}

void process_ksi_request(struct ksi_request *request)
{
	if (!request)
		return;

	set_ksi_enable(request->nf_hook_enable, request->nl_event_enable);
	hwlog_info("%s:process_ksi_request hook=%d event=%d\n", __func__,
		request->nf_hook_enable, request->nl_event_enable);
}

void process_native_requst(struct native_requst *requst)
{
	struct native_event native_info;

	hwlog_info("%s:request addr=%x\n", __func__,requst);
	if (requst->len > MAX_RTT_LIST_LEN || requst->len < 0)
		return;

	get_rtt_list(&native_info, requst->len);
	native_info.len = requst->len;
	nb_notify_event(NBMSG_NATIVE_RTT, &native_info, sizeof(struct native_event));
}

#ifdef CONFIG_APP_QOE_AI_PREDICT
void process_app_qoe_params_request(struct app_qoe_request *request)
{
	if (!request)
		return;
	if (request->msg_type == APP_QOE_MSG_RSRP_REQ) {
		set_app_qoe_rsrp(request->rsrp, request->rsrq);
	} else if (request->msg_type == APP_QOE_MSG_UID_REQ) {
		set_app_qoe_uid(request->app_uid, request->report_period);
	}
}
#endif

static void nb_netlink_rcv(struct sk_buff *__skb)
{
	struct nlmsghdr *nlh = NULL;
	struct sk_buff *skb = NULL;

	// TODO:check sender permission of NETLINK_NETBOOSTER

	if (g_nb_module_state != NB_NETLINK_INIT) {
		hwlog_err("%s:module not inited\n", __func__);
		return;
	}

	if (NULL == __skb) {
		hwlog_err("Invalid parameter: zero pointer reference(__skb)\n");
		return;
	}

	skb = skb_get(__skb);
	if (NULL == skb) {
		hwlog_err("nb_netlink_rcv: skb = NULL\n");
		return;
	}

	mutex_lock(&nb_receive_sem);

	if (skb->len >= NLMSG_HDRLEN) {
		nlh = nlmsg_hdr(skb);
		if (NULL == nlh) {
			hwlog_err("nb_netlink_rcv:nlh = NULL\n");
			kfree_skb(skb);
			mutex_unlock(&nb_receive_sem);
			return;
		}
		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr)) && (skb->len >= nlh->nlmsg_len)) {
			switch(nlh->nlmsg_type) {
			case NBMSG_REG:
				/*
				 * Save user space progress pid when register
				 * netlink socket.
				 */
				g_user_space_pid = nlh->nlmsg_pid;
				break;
			case NBMSG_UNREG:
				g_user_space_pid = 0;
				break;
			case NBMSG_VOD_REQ:
				if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(struct vod_request))) {
					hwlog_err("nb_netlink_rcv:invalid nlmsg_len %d of nlmsg_type %d\n",
								nlh->nlmsg_len, nlh->nlmsg_type);
					break;
				}
				process_vod_request((struct vod_request *)NLMSG_DATA(nlh));
				break;
			case NBMSG_KSI_REQ:
				if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(struct ksi_request))) {
					hwlog_err("nb_netlink_rcv:invalid nlmsg_len %d of nlmsg_type %d\n",
								nlh->nlmsg_len, nlh->nlmsg_type);
					break;
				}
				process_ksi_request((struct ksi_request *)NLMSG_DATA(nlh));
				break;
			case NBMSG_NATIVE_REG:
				g_native_space_pid = nlh->nlmsg_pid;
				break;
			case NBMSG_NATIVE_UNREG:
				g_native_space_pid = 0;
				break;
			case NBMSG_NATIVE_GET_RTT:
				process_native_requst((struct native_requst *)NLMSG_DATA(nlh));
				break;
#ifdef CONFIG_APP_QOE_AI_PREDICT
			case NBMSG_APP_QOE_PARAMS_REQ: {
				if (nlh->nlmsg_len < NLMSG_LENGTH(sizeof(struct app_qoe_request))) {
					hwlog_err("nb_netlink_rcv:invalid nlmsg_len %d of nlmsg_type %d\n",
								nlh->nlmsg_len, nlh->nlmsg_type);
					break;
				}
				process_app_qoe_params_request((struct app_qoe_request *)NLMSG_DATA(nlh));
				break;
			}
#endif
			default:
				hwlog_err("nb_netlink_rcv:invalid nlmsg_type %d\n", nlh->nlmsg_type);
				break;
			}
		}
	}

	kfree_skb(skb);
	mutex_unlock(&nb_receive_sem);
}

static int nb_netlink_thread(void *data)
{
	struct nb_event_info event_info = {0};

	while (!kthread_should_stop()) {

		down(&g_event_sema);

		spin_lock_bh(&g_event_info_lock);
		memcpy(&event_info, &g_event_info, sizeof(struct nb_event_info));
		g_event_info.type = NBMSG_EVT_INVALID;
		spin_unlock_bh(&g_event_info_lock);

		hwlog_info("%s:got event %d\n", __func__, event_info.type);

		if ((g_user_space_pid != 0) || (g_native_space_pid != 0)) {
			switch (event_info.type) {
			case NBMSG_VOD_EVT:
				__nb_notify_event(NBMSG_VOD_EVT, &event_info.vod_info, sizeof(struct vod_event));
				break;
			case NBMSG_KSI_EVT:
				__nb_notify_event(NBMSG_KSI_EVT, &event_info.ksi_info, sizeof(struct ksi_event));
				break;
			case NBMSG_NATIVE_RTT:
				__nb_notify_event(NBMSG_NATIVE_RTT, &event_info.native_info, sizeof(struct native_event));
				break;
			default:
				hwlog_err("%s:unexpected event type %d\n", __func__, event_info.type);
				break;
			}
		}
	}
	return 0;
}

/* netlink init function */
static int nb_netlink_init(void)
{
	struct netlink_kernel_cfg nb_nl_cfg = {
		.input = nb_netlink_rcv,
	};

	g_nlfd = netlink_kernel_create(&init_net,
		NETLINK_NETBOOSTER,
		&nb_nl_cfg);

	if (!g_nlfd) {
		hwlog_info("%s: nb_netlink_init failed\n", __func__);
		return -1;
	} else {
		hwlog_info("%s: nb_netlink_init success\n", __func__);
	}

	sema_init(&g_event_sema, 0);

	g_netlink_task = kthread_run(nb_netlink_thread, NULL, "nb_netlink");
	if (IS_ERR(g_netlink_task)) {
		hwlog_err("%s:failed to create nb_netlink thread\n", __func__);
		g_netlink_task = NULL;
		return -1;
	}
	return 0;
}

/* netlink deinit function */
static void nb_netlink_exit(void)
{
	if (g_nlfd && g_nlfd->sk_socket) {
		sock_release(g_nlfd->sk_socket);
		g_nlfd = NULL;
	}

	if (g_netlink_task) {
		kthread_stop(g_netlink_task);
		g_netlink_task = NULL;
	}
}

/* send a message to user space */
static int __nb_notify_event(enum nb_evt_type event, void *src, int size)
{
	int ret = 0;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct vod_event *pdata = NULL;

	mutex_lock(&nb_send_sem);
	if ((!g_user_space_pid && !g_native_space_pid) || !g_nlfd) {
		hwlog_err("%s: cannot notify event, pid = %d\n", __func__, g_user_space_pid);
		ret = -1;
		goto end;
	}

	skb = nlmsg_new(size, GFP_ATOMIC);
	if (!skb) {
		hwlog_info("%s: alloc skb fail\n", __func__);
		ret = -1;
		goto end;
	}
	nlh = nlmsg_put(skb, 0, 0, event, size, 0);
	if (!nlh) {
		kfree_skb(skb);
		skb = NULL;
		ret = -1;
		goto end;
	}

	pdata = nlmsg_data(nlh);
	memcpy(pdata, src, size);

	/* skb will be freed in netlink_unicast */
	switch (event) {
	case NBMSG_NATIVE_RTT:
		ret = netlink_unicast(g_nlfd, skb, g_native_space_pid, MSG_DONTWAIT);
		break;
	default:
		ret = netlink_unicast(g_nlfd, skb, g_user_space_pid, MSG_DONTWAIT);
		break;
	}

	goto end;

end:
	mutex_unlock(&nb_send_sem);
	return ret;
}

static int __init nb_netlink_module_init(void)
{
	if (nb_netlink_init()) {
		hwlog_err("%s:init nb_netlink module failed\n", __func__);
		g_nb_module_state = NB_NETLINK_EXIT;
		return 0;
	}

	hwlog_info("%s:nb_netlink module inited\n", __func__);
	g_nb_module_state = NB_NETLINK_INIT;
	video_acceleration_init();
	smart_switch_init();
	return 0;
}

static void __exit nb_netlink_module_exit(void)
{
	g_nb_module_state = NB_NETLINK_EXIT;
	smart_switch_exit();
	video_acceleration_exit();
	nb_netlink_exit();
}

module_init(nb_netlink_module_init);
module_exit(nb_netlink_module_exit);
