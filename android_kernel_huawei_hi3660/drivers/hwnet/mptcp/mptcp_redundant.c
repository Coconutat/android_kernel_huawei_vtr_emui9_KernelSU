/*
 *	MPTCP Scheduler to reduce latency and jitter.
 *
 *	This scheduler sends all packets redundantly on all available subflows.
 *
 *  If application set threshold, interface use mode will be decided based on primary link latency.
 *  The threshold will be compared with primary link latency.
 *  If it is less, there will be only primary link used, otherwise both primary
 *  and secondary link will be used.
 *
 *	Initial Design & Implementation:
 *	Tobias Erbshaeusser <erbshauesser@dvs.tu-darmstadt.de>
 *	Alexander Froemmgen <froemmge@dvs.tu-darmstadt.de>
 *
 *	Initial corrections & modifications:
 *	Christian Pinedo <christian.pinedo@ehu.eus>
 *	Igor Lopez <igor.lopez@ehu.eus>
 *
 *	This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <net/mptcp.h>
#include <linux/inet.h>
#ifdef CONFIG_HUAWEI_XENGINE
#include <huawei_platform/emcom/emcom_xengine.h>
#endif

enum  SCHED_STATE {
	SCHED_PRIM_ONLY,
	SCHED_REDUNDANT,
	SCHED_SECO_ONLY,
	SCHED_UNDEFINED,
};

#define SCHED_REDUNDANT_JIFFIES	msecs_to_jiffies(1000)
#define SCHED_SECO_ONLY_JIFFIES	msecs_to_jiffies(10000)
#define SCHED_MIN_JIFFIES	msecs_to_jiffies(100)

/* Struct to store the data of a single subflow */
struct redsched_sock_data {
	/* The skb or NULL */
	struct sk_buff *skb;
	/* End sequence number of the skb. This number should be checked
	 * to be valid before the skb field is used
	 */
	u32 skb_end_seq;
	u32 is_not_newly_added;
};

/* Struct to store the data of the control block */
struct redsched_cb_data {
	/* The next subflow where a skb should be sent or NULL */
	struct tcp_sock *next_subflow;
	u32 sched_state;
	u32 ts;
	u32 ts_last_sched;
};

/* Returns the socket data from a given subflow socket */
static struct redsched_sock_data *redsched_get_sock_data(struct tcp_sock *tp)
{
	return (struct redsched_sock_data *)&tp->mptcp->mptcp_sched[0];
}

/* Returns the control block data from a given meta socket */
static struct redsched_cb_data *redsched_get_cb_data(struct tcp_sock *tp)
{
	return (struct redsched_cb_data *)&tp->mpcb->mptcp_sched[0];
}

static int redsched_get_active_valid_sks(struct sock *meta_sk)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);
	struct mptcp_cb *mpcb = meta_tp->mpcb;
	struct sock *sk;
	int active_valid_sks = 0;

	mptcp_for_each_sk(mpcb, sk) {
		if (subflow_is_active((struct tcp_sock *)sk) &&
		    !mptcp_is_def_unavailable(sk))
			active_valid_sks++;
	}
	return active_valid_sks;
}

static bool redsched_use_subflow(struct sock *meta_sk,
				 int active_valid_sks,
				 struct tcp_sock *tp,
				 struct sk_buff *skb)
{
	if (!skb || !mptcp_is_available((struct sock *)tp, skb, false))
		return false;

	if (TCP_SKB_CB(skb)->path_mask != 0)
		return subflow_is_active(tp);

	if (TCP_SKB_CB(skb)->path_mask == 0) {
		if (active_valid_sks == -1)
			active_valid_sks = redsched_get_active_valid_sks(meta_sk);

		if (subflow_is_backup(tp) && active_valid_sks > 0)
			return false;
		else
			return true;
	}

	return false;
}

static struct sock *redundant_get_subflow(struct sock *meta_sk,
					  struct sk_buff *skb,
					  bool zero_wnd_test)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);
	struct mptcp_cb *mpcb = meta_tp->mpcb;
	struct redsched_cb_data *cb_data = redsched_get_cb_data(meta_tp);
	struct tcp_sock *first_tp = cb_data->next_subflow;
	struct sock *sk;
	struct tcp_sock *tp;

	/* Answer data_fin on same subflow */
	if (meta_sk->sk_shutdown & RCV_SHUTDOWN &&
	    skb && mptcp_is_data_fin(skb)) {
		mptcp_for_each_sk(mpcb, sk) {
			if (tcp_sk(sk)->mptcp->path_index ==
				mpcb->dfin_path_index &&
			    mptcp_is_available(sk, skb, zero_wnd_test))
				return sk;
		}
	}

	if (!first_tp)
		first_tp = mpcb->connection_list;
	tp = first_tp;

	/* still NULL (no subflow in connection_list?) */
	if (!first_tp)
		return NULL;

	/* Search for any subflow to send it */
	do {
		if (mptcp_is_available((struct sock *)tp, skb,
				       zero_wnd_test)) {
			cb_data->next_subflow = tp->mptcp->next;
			return (struct sock *)tp;
		}

		tp = tp->mptcp->next;
		if (!tp)
			tp = mpcb->connection_list;
	} while (tp != first_tp);

	/* No space */
	return NULL;
}

/* Corrects the stored skb pointers if they are invalid */
static void redsched_correct_skb_pointers(struct sock *meta_sk,
					  struct redsched_sock_data *sk_data)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);

	if (sk_data->skb && !after(sk_data->skb_end_seq, meta_tp->snd_una))
		sk_data->skb = NULL;
}

/* Returns the next skb from the queue */
static struct sk_buff *redundant_next_skb_from_queue(struct sk_buff_head *queue,
						     struct sk_buff *previous,
						     struct sock *meta_sk)
{
	if (skb_queue_empty(queue))
		return NULL;

	if (!previous)
		return skb_peek(queue);

	if (skb_queue_is_last(queue, previous))
		return NULL;

	/* sk_data->skb stores the last scheduled packet for this subflow.
	 * If sk_data->skb was scheduled but not sent (e.g., due to nagle),
	 * we have to schedule it again.
	 *
	 * For the redundant scheduler, there are two cases:
	 * 1. sk_data->skb was not sent on another subflow:
	 *    we have to schedule it again to ensure that we do not
	 *    skip this packet.
	 * 2. sk_data->skb was already sent on another subflow:
	 *    with regard to the redundant semantic, we have to
	 *    schedule it again. However, we keep it simple and ignore it,
	 *    as it was already sent by another subflow.
	 *    This might be changed in the future.
	 *
	 * For case 1, send_head is equal previous, as only a single
	 * packet can be skipped.
	 */
	if (tcp_send_head(meta_sk) == previous)
		return tcp_send_head(meta_sk);

	return skb_queue_next(queue, previous);
}

#ifdef CONFIG_HUAWEI_XENGINE

static void xengine_report_mptcp_path_switch(struct tcp_sock *primary,
					    struct tcp_sock *secondly)
{
	struct mptcp_hw_ext_sock_path_switch report;
	struct sock *meta_sk;
	struct sock *from_sk, *to_sk;
	struct dst_entry *dst;

	if (!primary || !secondly) {
		mptcp_debug("%s: subflow is NULL \n", __func__);
		return;
	}

	meta_sk = primary->meta_sk;
	if (!meta_sk || !meta_sk->sk_socket) {
		mptcp_debug("%s: sk is NULL \n", __func__);
		return;
	}

	(void)memset(&report, 0, sizeof(report));
	if (tcp_sk(meta_sk)->mptcp_cap_flag == MPTCP_CAP_PID_FD) {
		report.sock.type = (enum mptcp_hw_ext_sock_cap)MPTCP_CAP_PID_FD;
		report.sock.uid = sock_i_uid(meta_sk).val;
		report.sock.pid = meta_sk->sk_socket->pid;
		report.sock.fd = meta_sk->sk_socket->fd;
		mptcp_debug("%s: uid:%d pid:%d fd:%d mptcp sock\n", __func__,
			report.sock.uid, report.sock.pid, report.sock.fd);
	} else {
		char ip_str[INET_ADDRSTRLEN];

		if (tcp_sk(meta_sk)->mptcp_cap_flag == MPTCP_CAP_UID)
			report.sock.type = (enum mptcp_hw_ext_sock_cap)MPTCP_CAP_UID;
		else
			report.sock.type = (enum mptcp_hw_ext_sock_cap)MPTCP_CAP_UID_DIP_DPORT;

		report.sock.uid = sock_i_uid(meta_sk).val;
		report.sock.dip = meta_sk->sk_daddr;
		if (mptcp_hw_ext_get_port_key(meta_sk, report.sock.port,
			sizeof(report.sock.port)) != 0)
			report.sock.port[0] = '\0';

		mptcp_debug("%s: uid:%d dst_addr:%s:%s mptcp sock\n", __func__,
			report.sock.uid, anonymousIPv4addr(report.sock.dip, ip_str, INET_ADDRSTRLEN),
			report.sock.port);
	}

	if (!secondly->mptcp->low_prio) {
		from_sk = (struct sock *)secondly;
		to_sk = (struct sock *)primary;
	} else {
		from_sk = (struct sock *)primary;
		to_sk = (struct sock *)secondly;
	}

	dst = sk_dst_get(from_sk);
	if (dst) {
		(void)strncpy(report.src_path, dst->dev->name, IFNAMSIZ);
		report.src_path[IFNAMSIZ - 1] = '\0';
		dst_release(dst);
	}
	report.src_rtt = tcp_sk(from_sk)->srtt_us >> 3;
	dst = sk_dst_get(to_sk);
	if (dst) {
		(void)strncpy(report.dst_path, dst->dev->name, IFNAMSIZ);
		report.dst_path[IFNAMSIZ - 1] = '\0';
		dst_release(dst);
	}
	report.dst_rtt = tcp_sk(to_sk)->srtt_us >> 3;

	mptcp_info("%s: switch from %s[%d] to %s[%d] \n", __func__,
		report.src_path, report.src_rtt, report.dst_path, report.dst_rtt);
	Emcom_Xengine_MptcpSocketSwitch(&report, sizeof(report));
}
#endif

static inline bool check_subsk_is_good(struct sock *sk, u32 threshold)
{
	struct tcp_sock *tp;

	if (TCP_CA_Loss == inet_csk(sk)->icsk_ca_state) {
		mptcp_debug("%s: sk %pK is in TCP_CA_Loss\n", __func__, sk);
		return false;
	}
	tp = (struct tcp_sock *)sk;
	/* make sure the srtt_us is update recently */
	if (before(tcp_time_stamp, tp->rcv_tstamp) ||
	    after(tcp_time_stamp, tp->rcv_tstamp + SCHED_REDUNDANT_JIFFIES)) {
		mptcp_debug("%s: sk %pK rtt is out of date\n", __func__, sk);
		return false;
	}

	if ((tp->srtt_us >> 3) > threshold)
		return false;

	return true;
}


static bool schduled_get_subflows(struct sock *meta_sk, struct sock **prim_subsk,
				  struct sock **seco_subsk)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk), *subtp = NULL;
	struct sock *subsk = NULL;
	struct mptcp_cb *mpcb = meta_tp->mpcb;
	bool is_new_subsk_added = false;
	struct redsched_sock_data *sk_data;
	u32 threshold = *(u32 *)&meta_tp->mptcp_sched_params[0];

	if (!prim_subsk || !seco_subsk) {
		pr_err("%s: para error\n", __func__);
		return is_new_subsk_added;
	}

	*prim_subsk = NULL;
	*seco_subsk = NULL;

	mptcp_for_each_sk(mpcb, subsk) {
		mptcp_debug("%s: meta_tp(%pK) sk_state:%d, primary flag:%d \n", __func__,
			meta_tp, subsk->sk_state, mptcp_is_primary_subflow(subsk));
		if (subsk->sk_state != TCP_ESTABLISHED)
			continue;

		subtp = tcp_sk(subsk);
		sk_data = redsched_get_sock_data(subtp);
		if (!sk_data->is_not_newly_added)
			is_new_subsk_added = true;

		if (mptcp_is_primary_subflow(subsk)) {
			if (!(*prim_subsk) || tcp_sk(*prim_subsk)->srtt_us > subtp->srtt_us) {
				mptcp_debug("%s: meta_tp(%pK) primary_tp(%pK) subtp->srtt_us:%u threshold:%u \n",
					__func__, meta_tp, subtp, (subtp->srtt_us >> 3), threshold);
				*prim_subsk = subsk;
			}
		} else if (!(*seco_subsk) ||
			tcp_sk(*seco_subsk)->srtt_us > subtp->srtt_us) {
			mptcp_debug("%s: meta_tp(%pK) second_tp(%pK) subtp->srtt_us:%u threshold:%u \n",
				__func__, meta_tp, subtp, (subtp->srtt_us >> 3), threshold);
			*seco_subsk = subsk;
		}
	}

	return is_new_subsk_added;
}


static void schduled_update_prio(struct sock *meta_sk, u32 old_state, u32 new_state,
				 struct sock *prim_subsk, struct sock *seco_subsk)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk), *subtp = NULL;
	struct sock *subsk = NULL;
	struct mptcp_cb *mpcb = meta_tp->mpcb;
	bool is_subtp_prio_changed = false;
	struct redsched_sock_data *sk_data;
	int low_prio;

	mptcp_for_each_sk(mpcb, subsk) {
		mptcp_debug("%s: meta_tp(%pK) chk again sub_sk->sk_state:%u primary_flag:%d, low_prio:%u\n",
			__func__, meta_tp, subsk->sk_state, mptcp_is_primary_subflow(subsk),
			((struct tcp_sock *)subsk)->mptcp->low_prio);

		if (subsk->sk_state != TCP_ESTABLISHED)
			continue;

		subtp = tcp_sk(subsk);
		sk_data = redsched_get_sock_data(subtp);
		sk_data->is_not_newly_added = true;

		switch (new_state) {
		case SCHED_PRIM_ONLY: {
			if (mptcp_is_primary_subflow(subsk)) {
				if (subtp->mptcp->low_prio == 1) {
					mptcp_info("%s: prim_subsk(%pK) low_prio from 1 to 0\n", __func__, subsk);
					subtp->mptcp->low_prio = 0;
					is_subtp_prio_changed = true;
				}
			} else {
				/* for a new subflow is added, need to init the
				 * low_prio base on old_state
				 */
				if (subtp->mptcp->low_prio == 1) {
					if (old_state != SCHED_PRIM_ONLY)
						subtp->mptcp->low_prio = 0;
				} else {
					is_subtp_prio_changed = true;
				}

				if (subtp->mptcp->low_prio == 0) {
					mptcp_info("%s: sk(%pK) low_prio from 0 to 1\n", __func__, subsk);
#ifdef CONFIG_HUAWEI_XENGINE
					if (old_state != new_state && subsk == seco_subsk)
						xengine_report_mptcp_path_switch(tcp_sk(prim_subsk), subtp);
#endif
				}
				subtp->mptcp->low_prio = 1;
			}
			break;
		}
		case SCHED_REDUNDANT:
		case SCHED_SECO_ONLY: {
			if (mptcp_is_primary_subflow(subsk)) {
				low_prio = (new_state == SCHED_SECO_ONLY)?1:0;
				if (low_prio != subtp->mptcp->low_prio) {
					is_subtp_prio_changed = true;
					mptcp_info("%s: prim_subsk(%pK) low_prio from %d to %d\n",
						__func__, subtp, subtp->mptcp->low_prio, low_prio);
					subtp->mptcp->low_prio = low_prio;
				}
			} else {
				/* for a new subflow is added, need to init the
				 * low_prio base on old_state
				 */
				if (subtp->mptcp->low_prio == 0) {
					if (old_state == SCHED_PRIM_ONLY)
						subtp->mptcp->low_prio = 1;
				} else {
					is_subtp_prio_changed = true;
				}

				if (subtp->mptcp->low_prio == 1) {
					mptcp_info("%s: sk(%pK) low_prio from 1 to 0\n", __func__, subsk);
#ifdef CONFIG_HUAWEI_XENGINE
					if (old_state != new_state && subsk == seco_subsk)
						xengine_report_mptcp_path_switch(tcp_sk(prim_subsk), subtp);
#endif
				} else if (old_state != new_state) {
					subtp->mptcp->low_prio = 1;
				}
				subtp->mptcp->low_prio = 0;
			}
			break;
		}
		default:
			pr_err("%s invalid sched_state\n", __func__);
			break;
		}
	}

	if (is_subtp_prio_changed) {
		mptcp_for_each_sk(mpcb, subsk) {
			if (subsk->sk_state != TCP_ESTABLISHED)
				continue;

			subtp = tcp_sk(subsk);
			subtp->mptcp->send_mp_prio = 1;
			subtp->mptcp->send_mp_other_prio = 1;
		}
	}
}


static void schduled_depend_on_priority(struct sock *meta_sk)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);
	struct sock *prim_subsk = NULL, *seco_subsk = NULL;
	u32 sched_state;
	u32 threshold = *(u32 *)&meta_tp->mptcp_sched_params[0];
	struct redsched_cb_data *cb_data;
	bool is_new_subsk_added = true;

	is_new_subsk_added = schduled_get_subflows(meta_sk, &prim_subsk, &seco_subsk);

	/* if the cb_data->ts is not init(for the meta sk is newly created),
	 * just set it as the curent time stamp
	 */
	cb_data = redsched_get_cb_data(meta_tp);
	if (!cb_data->ts)
		cb_data->ts = tcp_time_stamp;

	sched_state = cb_data->sched_state;
	if (!prim_subsk || !seco_subsk) {
		mptcp_debug("%s: meta_tp(%pK) could not find both subflow\n", __func__, meta_tp);
		sched_state = prim_subsk?SCHED_PRIM_ONLY:SCHED_SECO_ONLY;
	} else {
		switch (cb_data->sched_state) {
		case SCHED_PRIM_ONLY:
			if (!check_subsk_is_good(prim_subsk, threshold))
				sched_state = SCHED_REDUNDANT;
			break;
		case SCHED_REDUNDANT:
			if (check_subsk_is_good(prim_subsk, threshold))
				sched_state = SCHED_PRIM_ONLY;
			else if (check_subsk_is_good(seco_subsk, threshold) &&
				after(tcp_time_stamp, cb_data->ts + SCHED_REDUNDANT_JIFFIES))
				sched_state = SCHED_SECO_ONLY;
			else if (!check_subsk_is_good(seco_subsk, threshold))
				cb_data->ts = tcp_time_stamp;
			break;
		case SCHED_SECO_ONLY:
			if (!check_subsk_is_good(seco_subsk, threshold) ||
			    after(tcp_time_stamp, cb_data->ts + SCHED_SECO_ONLY_JIFFIES))
				sched_state = SCHED_REDUNDANT;
			break;
		default:
			sched_state = SCHED_PRIM_ONLY;
			break;
		};
	}

	if (cb_data->sched_state != sched_state || (is_new_subsk_added == true))
		schduled_update_prio(meta_sk, cb_data->sched_state, sched_state,
				     prim_subsk, seco_subsk);

	if (cb_data->sched_state != sched_state) {
		mptcp_info("%s: sk(%pK) sched_state from %d to %d \n", __func__,
			meta_tp, cb_data->sched_state, sched_state);
		cb_data->sched_state = sched_state;
		cb_data->ts = tcp_time_stamp;
	}
}

static struct sk_buff *redundant_next_segment(struct sock *meta_sk,
					      int *reinject,
					      struct sock **subsk,
					      unsigned int *limit)
{
	struct tcp_sock *meta_tp = tcp_sk(meta_sk);
	struct mptcp_cb *mpcb = meta_tp->mpcb;
	struct redsched_cb_data *cb_data = redsched_get_cb_data(meta_tp);
	struct tcp_sock *first_tp = cb_data->next_subflow;
	struct tcp_sock *tp;
	struct sk_buff *skb;
	int active_valid_sks = -1;
	bool is_handover = false;

	if (0 == strcmp(meta_tp->mptcp_sched_name, MPTCP_SCHED_NAME_HANDOVER))
		is_handover = true;

	/* As we set it, we have to reset it as well. */
	*limit = 0;

	if (skb_queue_empty(&mpcb->reinject_queue) &&
	    skb_queue_empty(&meta_sk->sk_write_queue))
		/* Nothing to send */
		return NULL;

	/* First try reinjections */
	skb = skb_peek(&mpcb->reinject_queue);
	if (skb) {
		*subsk = get_available_subflow(meta_sk, skb, false);
		if (!*subsk)
			return NULL;
		*reinject = 1;
		return skb;
	}

	/* Then try indistinctly redundant and normal skbs */


	if (!first_tp)
		first_tp = mpcb->connection_list;

	/* still NULL (no subflow in connection_list?) */
	if (!first_tp)
		return NULL;

	tp = first_tp;

	*reinject = 0;

	/* avoid frequent and unnecessary call*/
	if (is_handover && tcp_time_stamp > (cb_data->ts_last_sched + SCHED_MIN_JIFFIES)) {
		schduled_depend_on_priority(meta_sk);
		cb_data->ts_last_sched = tcp_time_stamp;
	}

	active_valid_sks = redsched_get_active_valid_sks(meta_sk);
	mptcp_debug("%s: active_valid_sks:%d \n", __func__, active_valid_sks);

	do {
		struct redsched_sock_data *sk_data;

		/* Correct the skb pointers of the current subflow */
		sk_data = redsched_get_sock_data(tp);
		redsched_correct_skb_pointers(meta_sk, sk_data);

		skb = redundant_next_skb_from_queue(&meta_sk->sk_write_queue,
						    sk_data->skb, meta_sk);
		if (skb && redsched_use_subflow(meta_sk, active_valid_sks, tp,
						skb)) {
			sk_data->skb = skb;
			sk_data->skb_end_seq = TCP_SKB_CB(skb)->end_seq;
			cb_data->next_subflow = tp->mptcp->next;
			*subsk = (struct sock *)tp;

			if (TCP_SKB_CB(skb)->path_mask)
				*reinject = -1;
			return skb;
		}

		tp = tp->mptcp->next;
		if (!tp)
			tp = mpcb->connection_list;
	} while (tp != first_tp);

	/* Nothing to send */
	return NULL;
}

static void redundant_release(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct redsched_cb_data *cb_data = redsched_get_cb_data(tp);

	/* Check if the next subflow would be the released one. If yes correct
	 * the pointer
	 */
	if (cb_data->next_subflow == tp)
		cb_data->next_subflow = tp->mptcp->next;
}

static struct mptcp_sched_ops mptcp_sched_redundant = {
	.get_subflow = redundant_get_subflow,
	.next_segment = redundant_next_segment,
	.release = redundant_release,
	.name = "redundant",
	.owner = THIS_MODULE,
};

static int __init redundant_register(void)
{
	BUILD_BUG_ON(sizeof(struct redsched_sock_data) > MPTCP_SCHED_SIZE);
	BUILD_BUG_ON(sizeof(struct redsched_cb_data) > MPTCP_SCHED_DATA_SIZE);

	if (mptcp_register_scheduler(&mptcp_sched_redundant))
		return -1;

	return 0;
}

static void redundant_unregister(void)
{
	mptcp_unregister_scheduler(&mptcp_sched_redundant);
}

module_init(redundant_register);
module_exit(redundant_unregister);

MODULE_AUTHOR("Tobias Erbshaeusser, Alexander Froemmgen");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("REDUNDANT MPTCP");
MODULE_VERSION("0.90");
