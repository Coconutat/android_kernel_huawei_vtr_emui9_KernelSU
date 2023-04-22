/* bastet_reconn.c
 *
 * Bastet Tcp Reconnection.
 *
 * Copyright (C) 2014 Huawei Device Co.,Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/file.h>
#include <net/tcp.h>
#include <net/sock.h>
#include <huawei_platform/net/bastet/bastet_utils.h>
#include <huawei_platform/net/bastet/bastet.h>
extern int inet_release(struct socket *sock);

struct sk_reconn {
	struct list_head list;
	struct file *reconn_file;
	struct socket *old_sock;
	struct socket *orig_sock;
	struct sock *tmp_sk;
	struct bastet_reconn *rc;
	struct bst_sock_comm_prop comm;
};

struct list_head sk_reconn_list;

void bastet_reconn_init(void)
{
	INIT_LIST_HEAD(&sk_reconn_list);
}

void bastet_reconn_config(struct sock *sk, int val)
{
	if (IS_ERR_OR_NULL(sk)) {
		BASTET_LOGE("invalid parameter");
		return;
	}

	struct bastet_reconn *reconn;

	BASTET_LOGI("val=%d", val);
	if (sk->reconn) {
		if (sk->reconn->enable == val)
			return;

	} else {
		if (val == 0)
			return;

		BASTET_LOGI("new struct bastet_reconn");
		reconn = kmalloc(sizeof(struct bastet_reconn), GFP_KERNEL);
		if (NULL == reconn) {
			BASTET_LOGE("kmalloc struct bastet_reconn failed");
			return;
		}
		memset(reconn, 0, sizeof(struct bastet_reconn));
		sk->reconn = reconn;
		sk->reconn->flag = true;
		sk->reconn->err_close = false;
		atomic_set(&sk->reconn->handover, 0);
	}

	sk->reconn->enable = (val != 0) ? true : false;
	if (sk->reconn->auto_connect) {
		if (sk->reconn->enable) {
			init_waitqueue_head(&sk->reconn->wq);
		} else {
			if (!sk->reconn->flag)
				wake_up_interruptible(&sk->reconn->wq);
		}
	}
}

static void bastet_notify_close(struct sock *sk, bst_ind_type type)
{
	struct bst_sock_comm_prop comm_prop;
	int ret = 0;

	BASTET_LOGI("sk=%p, type=%d", sk, type);
	ret = bastet_get_comm_prop(sk, &comm_prop);
	if (ret != 0) {
		BASTET_LOGE("Failed to get comm prop, ret=%d", ret);
		return;
	}
	post_indicate_packet(type, &comm_prop,
		sizeof(struct bst_sock_comm_prop));
}

static bool check_sk_reconn(struct sk_reconn *reconn,
	struct bst_sock_comm_prop comm)
{
	bool ret = false;
#ifdef CONFIG_SECURITY
	if (reconn && reconn->tmp_sk && reconn->tmp_sk->sk_security) {
		if (memcmp(&reconn->comm, &comm,
			sizeof(struct bst_sock_comm_prop)) == 0)
			ret = true;
	}
#endif
	return ret;
}

void bastet_reconn_failed(struct bst_sock_comm_prop prop)
{
	struct sk_reconn *reconn = NULL;
	struct bastet_reconn *rc = NULL;
	struct list_head *p, *n;

	list_for_each_safe(p, n, &sk_reconn_list) {
		reconn = list_entry(p, struct sk_reconn, list);
		if (check_sk_reconn(reconn, prop)) {
			rc = reconn->rc;
			rc->flag = true;
			BASTET_LOGI("reconn break, wake up reconn wq");
			wake_up_interruptible(&rc->wq);
			return;
		}
	}
	BASTET_LOGI("cannot found valid sk_reconn, reset socket");
}

int bastet_reconn_proxyid_set(struct reconn_id id)
{
	struct sock *sk = NULL;
	struct bastet_reconn *rc = NULL;

	sk = get_sock_by_fd_pid(id.guide.fd, id.guide.pid);
	if (NULL == sk) {
		BASTET_LOGE("can not find sock by fd: %d pid: %d",
			id.guide.fd, id.guide.pid);
		return -ENOENT;
	}
	rc = sk->reconn;
	if (NULL == rc) {
		BASTET_LOGE("bastet_reconn is NULL");
		sock_put(sk);
		return -ENOENT;
	}
	rc->proxy_id = id.proxy_id;
	rc->auto_connect = id.auto_connect;
	sock_put(sk);

	return 0;
}

static inline bool is_socket_err_close(int err)
{
	if (err > 0)
		return false;

	/* the following error code is not really error close */
	switch (err) {
	case -EINTR:			/* Interrupted system call */
	case -EAGAIN:			/* Try again */
	case -ERESTARTSYS:		/* Normal close */
		return false;
	default:
		break;
	}

	return true;
}

void bastet_inet_release(struct sock *sk)
{
	if (IS_ERR_OR_NULL(sk)) {
		BASTET_LOGE("invalid parameter");
		return;
	}

	if (sk->reconn && !sk->reconn->err_close)
		bastet_notify_close(sk, BST_IND_SOCK_NORMAL_CLOSE);

	if (sk->prio_channel) {
		bastet_notify_close(sk, BST_IND_PRIO_SOCK_CLOSE);
		sk->prio_channel = false;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
inline int bastet_block_recv(struct socket *sock, struct msghdr *msg,
		 size_t size, int flags, int *addr_len, int err)
#else
inline int bastet_block_recv(struct kiocb *iocb,
		struct socket *sock, struct msghdr *msg,
		size_t size, int flags, int *addr_len, int err)
#endif
{
	if (err > 0 || !sock->sk)
		/* check whether receive data is
		 * heartbeat reply in userspace app recv thread
		 */
		return bastet_check_hb_reply(sock->sk, msg, err);

	if (sock->sk->prio_channel && is_socket_err_close(err)) {
		bastet_notify_close(sock->sk, BST_IND_PRIO_SOCK_CLOSE);
		sock->sk->prio_channel = false;
	}
	if (!sock->sk->reconn) {
		if (sock->sk->bastet && is_socket_err_close(err))
			bastet_notify_close(sock->sk, BST_IND_SOCK_ERR_CLOSE);
	} else {
		if (is_socket_err_close(err)) {
			/* net device down returns -ETIMEDOUT */
			if (err == -ETIMEDOUT)
				bastet_notify_close(sock->sk,
					BST_IND_SOCK_TIMEDOUT);
			else
				bastet_notify_close(sock->sk,
					BST_IND_SOCK_DISCONNECT);

			sock->sk->reconn->err_close = true;
		}
	}

	return err;
}
